/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// CLASS HEADER
#include <dali-ui-foundation/public-api/layouts/layout-controller.h>

// EXTERNAL INCLUDES
#include <dali/integration-api/adaptor-framework/adaptor.h>
#include <dali/integration-api/debug.h>
#include <dali/integration-api/processor-interface.h>
#include <dali/public-api/actors/actor.h>
#include <dali/public-api/object/weak-handle.h>
#include <dali/public-api/signals/callback.h>
#include <dali/public-api/signals/connection-tracker.h>
#include <algorithm>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// INTERNAL INCLUDES
#include <dali-ui-foundation/internal/layouts/layout-invalidation-generation.h>
#include <dali-ui-foundation/internal/layouts/layout-transition-dispatcher.h>
#include <dali-ui-foundation/internal/layouts/standalone-bounds-utils.h>
#include <dali-ui-foundation/internal/views/view/view-data-impl.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>
#include <dali-ui-foundation/public-api/views/view.h>
#include <algorithm>

namespace DALI_NAMESPACE
{
namespace Ui
{

namespace
{
// File-static map to store LayoutController instances per Window (internal use only; not exposed in public API).
std::unordered_map<void*, std::unique_ptr<LayoutController>> gLayoutControllers;

// Innermost LayoutControllerImpl whose ProcessLayoutRoot is on the stack, so
// ViewImpl::Arrange routes arranged subscribers in O(1). Managed by RAII
// ActiveLayoutFinishedScope (exception-safe: DALI_ASSERT_ALWAYS throws).
Integration::LayoutControllerImpl* gActiveLayoutFinishedController{nullptr};

// Controllers detached from gLayoutControllers but not yet freed.
//
// A controller is a dali-core Integration::Processor, and dali-core keeps
// dereferencing the processor pointer AFTER Process() returns (Core::RunProcessors
// reads GetProcessorName() for its trace record). Worse, ANY code that runs while
// core's processor loop is on the stack - another processor's Process(), a signal
// it emits, app code those reach - can call back into this file's public entry
// points, so no synchronous entry point here can ever prove the loop is NOT on
// the stack. Teardown is therefore split in two: detach (make inert, hand
// ownership over here) happens immediately, and the free runs from exactly two
// contexts that are outside the loop by construction - the platform idle
// callback, and static destruction at process exit. Nothing else frees.
std::vector<std::unique_ptr<LayoutController>> gDetachedLayoutControllers;

// Number of LayoutControllerImpl::Process() frames on the stack, summed over ALL
// controllers. Defense-in-depth for the idle reap: the idle callback can only see
// a non-zero value when an application spins a nested event loop from inside
// layout code (Measure/Arrange/a layout slot). The reap skips that round and
// re-arms instead of freeing while layout processing is on the stack.
int32_t gGlobalProcessDepth{0};

// True when a no-self-wake layout request was recorded while a
// LayoutController::Process() frame was on the stack. Such work stays in the
// controller's pending set but deliberately does not request another idle
// ProcessEvents cycle. When the outermost frame unwinds, end the propagation
// generation so a later event-time invalidation can walk to the root again and
// upgrade that parked work into a real idle wake request.
bool gDeferredLayoutRequestDuringProcess{false};

/**
 * @brief RAII for gGlobalProcessDepth.
 *
 * RAII (not a manual increment/decrement pair) is REQUIRED for the same reason
 * spelled out on ActiveLayoutFinishedScope below: a DALI_ASSERT_ALWAYS anywhere
 * in the Measure/Arrange call stack throws Dali::DaliException and Process() has
 * no try/catch. A manual decrement would be skipped on unwind, stranding the
 * counter above zero and permanently disabling the deferred free for every
 * controller from that point on.
 */
struct GlobalProcessDepthScope
{
  GlobalProcessDepthScope()
  {
    ++gGlobalProcessDepth;
  }
  ~GlobalProcessDepthScope()
  {
    if(--gGlobalProcessDepth == 0 && gDeferredLayoutRequestDuringProcess)
    {
      gDeferredLayoutRequestDuringProcess = false;
      Internal::LayoutInvalidation::AdvanceGeneration();
    }
  }
  GlobalProcessDepthScope(const GlobalProcessDepthScope&)            = delete;
  GlobalProcessDepthScope& operator=(const GlobalProcessDepthScope&) = delete;
};

// Forward declaration: the idle handler re-arms itself while work remains.
void ScheduleReapDetachedLayoutControllers();

/**
 * @brief Idle-time free of every detached controller.
 *
 * Runs from the platform idle handler: the main loop dispatches it directly, so
 * Core::ProcessEvents() - and with it core's processor loop - is not on the
 * stack. The one exception is an application spinning a nested event loop from
 * inside layout code; the depth guard skips that round and re-arms.
 *
 * This function and static destruction of gDetachedLayoutControllers are the
 * ONLY two places a detached controller is freed.
 */
void OnIdleReapDetachedLayoutControllers()
{
  if(gGlobalProcessDepth == 0)
  {
    // Swap out before destroying, so the global stays coherent if a controller
    // destructor ever re-enters this file.
    std::vector<std::unique_ptr<LayoutController>> doomed;
    doomed.swap(gDetachedLayoutControllers);
    // doomed unwinds here, running every ~LayoutController.
  }

  // Re-arm while anything is left: either the depth guard skipped this round,
  // or more controllers were detached meanwhile. No-op on an empty queue, so
  // this terminates once the queue drains.
  ScheduleReapDetachedLayoutControllers();
}

/**
 * @brief Requests an idle-time reap.
 *
 * Deliberately no scheduled-already flag: a pending idle is silently DISCARDED
 * (not run) when the adaptor stops, so such a flag could strand as "scheduled"
 * forever; a duplicate idle just finds an empty queue and stops. If AddIdle
 * fails (adaptor already stopped) the queue stays put and static destruction
 * frees it - the controllers in it are already inert either way.
 */
void ScheduleReapDetachedLayoutControllers()
{
  if(!gDetachedLayoutControllers.empty() && DALI_LIKELY(Adaptor::IsAvailable()))
  {
    Adaptor::Get().AddIdle(MakeCallback(&OnIdleReapDetachedLayoutControllers), false);
  }
}
} // namespace

namespace Integration
{

/**
 * @brief Internal implementation of LayoutController.
 *
 * Implements the Integration::Processor interface to hook into
 * DALi's per-frame processing pipeline.
 */
class LayoutControllerImpl : public Dali::Integration::Processor, public ConnectionTracker
{
public:
  /**
   * @brief Data for a tracked layout root.
   *
   * Stores a weak handle (non-ref-counted) for validity checking and
   * the raw implementation pointer for fast access during layout passes.
   */
  struct LayoutRootEntry
  {
    WeakHandle<Ui::View> weakHandle; ///< Non-ref-counted weak reference; auto-nullified on destruction
    ViewImpl*            view;       ///< Raw pointer for direct access
  };

  /**
   * @brief One View's target bounds in a completed layout pass.
   */
  struct PendingViewLayoutFinishedEvent
  {
    ViewImpl*                  view{nullptr}; ///< Identity only; tombstoned on unregister, never dereferenced
    Dali::WeakHandle<Ui::View> weakHandle;
    LayoutRect                 bounds;
  };

  /**
   * @brief A successful batch and its progress through post-process delivery.
   *
   * Cursors advance before calling user code. If a slot throws, an already
   * started signal is not replayed, while the remaining signals stay queued.
   */
  struct LayoutPassCompletion
  {
    std::vector<PendingViewLayoutFinishedEvent> viewEvents;
    std::size_t                                 nextView{0u};
    bool                                        notifyWindow{false};
  };

  using LayoutPassCompletions = std::list<LayoutPassCompletion>;

  /**
   * @brief Exposes stack-local snapshots to unregister without allocating.
   *
   * Leaving the scene cancels earlier snapshots even if the View survives or
   * rejoins the same window. Nested batches each keep their own record.
   */
  struct ActiveCompletionScope
  {
    ActiveCompletionScope(LayoutControllerImpl& self, LayoutPassCompletion& completion)
    : mSelf(self),
      mCompletion(completion),
      mPrevious(self.mActiveCompletionScope)
    {
      mSelf.mActiveCompletionScope = this;
    }

    ~ActiveCompletionScope()
    {
      mSelf.mActiveCompletionScope = mPrevious;
    }

    ActiveCompletionScope(const ActiveCompletionScope&)            = delete;
    ActiveCompletionScope& operator=(const ActiveCompletionScope&) = delete;

    LayoutControllerImpl&  mSelf;
    LayoutPassCompletion&  mCompletion;
    ActiveCompletionScope* mPrevious;
  };

  /**
   * @brief One active per-root collector frame (stack-local containers owned by
   * the ProcessLayouts loop). A member STACK of these makes nested ProcessLayouts
   * re-entrancy safe: UnregisterView scrubs a destroyed view from EVERY frame,
   * not just the innermost, closing the nested-rebind use-after-free.
   */
  struct ActiveCollectorFrame
  {
    ViewImpl*                      root;
    std::vector<ViewImpl*>*        views;
    std::unordered_set<ViewImpl*>* set;
  };

  /**
   * @brief RAII: push a collector frame + set the file-static pointer on entry;
   * pop + restore on exit. RAII (not manual save/restore) is REQUIRED because a
   * DALI_ASSERT_ALWAYS inside the arrange call stack throws Dali::DaliException
   * and there is no try/catch here; the destructor still pops/restores on unwind.
   */
  struct ActiveLayoutFinishedScope
  {
    ActiveLayoutFinishedScope(LayoutControllerImpl&          self,
                              ViewImpl*                      root,
                              std::vector<ViewImpl*>&        views,
                              std::unordered_set<ViewImpl*>& set)
    : mSelf(self),
      mPrevController(gActiveLayoutFinishedController)
    {
      mSelf.mActiveCollectorStack.push_back(ActiveCollectorFrame{root, &views, &set});
      gActiveLayoutFinishedController = &self;
    }
    ~ActiveLayoutFinishedScope()
    {
      gActiveLayoutFinishedController = mPrevController;
      mSelf.mActiveCollectorStack.pop_back();
    }
    ActiveLayoutFinishedScope(const ActiveLayoutFinishedScope&)            = delete;
    ActiveLayoutFinishedScope& operator=(const ActiveLayoutFinishedScope&) = delete;

    LayoutControllerImpl& mSelf;
    LayoutControllerImpl* mPrevController;
  };

  /**
   * @brief RAII for LayoutControllerImpl::mProcessDepth.
   *
   * Only the outermost automatic pre phase promotes completed passes for
   * delivery, and only the outermost post phase emits them. The scope restores
   * the depth and honours deferred destruction on both normal and exceptional
   * exits, including exceptions from application callbacks.
   */
  struct ProcessDepthScope
  {
    explicit ProcessDepthScope(LayoutControllerImpl& self)
    : mSelf(self)
    {
      ++mSelf.mProcessDepth;
    }
    ~ProcessDepthScope()
    {
      // Detach - never free - when unwinding the outermost Process frame. This
      // point is still INSIDE dali-core's processor loop, which dereferences the
      // processor pointer again after Process() returns, so the free is queued for
      // a later context (see gDetachedLayoutControllers).
      if(--mSelf.mProcessDepth == 0 && mSelf.mDestroyPending)
      {
        mSelf.DetachAndQueueForFree();
        // *this is now owned by gDetachedLayoutControllers. It is still alive, so
        // members remain valid, but it must do no further work.
      }
    }
    ProcessDepthScope(const ProcessDepthScope&)            = delete;
    ProcessDepthScope& operator=(const ProcessDepthScope&) = delete;

    LayoutControllerImpl& mSelf;
  };

  /**
   * @brief RAII for LayoutTransitionDispatcher::BeginLayoutPass / EndLayoutPass.
   *
   * ProcessLayouts had three exits and only two of them ended the pass: the empty-pending
   * early return and the normal end did, an exception out of any producer or lifecycle
   * callback did not. An unbalanced Begin leaves mLayoutPassDepth above zero for the rest
   * of the process, so EndLayoutPass never again reaches its reset branch and the per-pass
   * flags it clears (mInWindowResize) stay latched -- every later layout is then
   * classified as resize-driven.
   *
   * Inert without a dispatcher, so the caller needs no null test of its own.
   */
  struct LayoutPassScope
  {
    explicit LayoutPassScope(Internal::LayoutTransitionDispatcher* dispatcher)
    : mDispatcher(dispatcher)
    {
      if(mDispatcher)
      {
        // Mark the start of this batch. Paired with the destructor at EVERY exit point,
        // this protects per-pass flags (mInWindowResize) from being cleared by a nested
        // ProcessLayouts call invoked from a lifecycle callback during a
        // window-resize-driven pass.
        mDispatcher->BeginLayoutPass();
      }
    }
    ~LayoutPassScope()
    {
      if(mDispatcher)
      {
        // Reset per-pass flags after the whole batch. Per-root reset would clear them
        // after the first root and misclassify the rest.
        mDispatcher->EndLayoutPass();
      }
    }
    LayoutPassScope(const LayoutPassScope&)            = delete;
    LayoutPassScope& operator=(const LayoutPassScope&) = delete;

    Internal::LayoutTransitionDispatcher* mDispatcher;
  };

  /**
   * @brief RAII rollback for the local processing batch.
   *
   * ProcessLayouts SWAPS the pending set into a stack-local vector and then erases each
   * root from the member set immediately before its turn. Between the swap and the end of
   * the loop the member set therefore names NONE of the roots that still have to run, and
   * the vector is a stack object: an exception out of any producer (Measure, Arrange, a
   * transition lifecycle callback) destroys it and LOSES every remaining root. The batch
   * is not retried and nothing re-registers them, so their layout simply never runs again
   * until some unrelated invalidation happens to reach them.
   *
   * Restoring is more than re-inserting pointers. The CURRENT root's pass has already
   * consumed its dirty bits (both guards consume at ENTRY) and cleared its cache-valid
   * bits, so a re-registered-but-clean root would be drained by the next pass with no
   * record that it has unconsumed work. RearmLayoutDirtyForAbortedPass() puts that record
   * back, directly, on the one root that lost it.
   *
   * Every restore goes through RetainLayoutRootWithoutWake, never RequestLayout and never
   * ViewImpl::InvalidateMeasure: see that helper for why no wake, and note that
   * InvalidateMeasure would additionally walk to a DIFFERENT root than the one being
   * restored. A failed batch never publishes its completion record; records
   * from earlier successful batches, including nested ones, remain independent.
   *
   * Disarmed by Commit() on the normal path, so normal flow costs one bool store.
   *
   * ORDERING. Destructors on the same unwind run innermost first, and this scope sits
   * between ActiveLayoutFinishedScope (inner) and LayoutPassScope /
   * GlobalProcessDepthScope / ProcessDepthScope (outer). By the time it runs: every
   * Measure/Arrange guard has unwound (gActiveLayoutPassDepth == 0), the collector frame
   * has been popped and gActiveLayoutFinishedController restored, and mDetached /
   * mDestroyPending are still readable and are still honoured. It touches only members
   * that outlive it -- mPendingViews and mAllLayoutRoots -- and the
   * dispatcher's EndLayoutPass still runs AFTER it, the same relative order as the normal
   * path (the manual EndLayoutPass came after the loop).
   */
  struct PendingBatchRollbackScope
  {
    PendingBatchRollbackScope(LayoutControllerImpl& self, const std::vector<ViewImpl*>& batch)
    : mSelf(self),
      mBatch(batch)
    {
    }

    ~PendingBatchRollbackScope()
    {
      if(!mArmed)
      {
        return;
      }

      // The restore below writes to containers that may allocate (RetainLayoutRootWithoutWake
      // inserts into mPendingViews), and this destructor runs while an exception is
      // propagating -- so an escaping std::bad_alloc would std::terminate the process
      // mid-unwind. Swallowing it degrades to exactly the pre-rollback behaviour: the
      // un-taken roots stay unregistered and their work stalls until the next natural
      // invalidation re-registers them. A stalled layout is strictly better than a
      // terminated process, and the original exception continues to propagate either way.
      try
      {
        for(std::size_t index = mCurrentTurn; index < mBatch.size(); ++index)
        {
          ViewImpl* view = mBatch[index];

          // The same aliveness test the loop performs, and for the same reason: a root whose
          // View was destroyed mid-batch must not be resurrected as a dangling raw pointer.
          // A dead entry the loop already cleaned up (mAllLayoutRoots.erase) fails it too.
          auto it = mSelf.mAllLayoutRoots.find(view);
          if(!view || it == mSelf.mAllLayoutRoots.end() || !it->second.weakHandle.GetHandle())
          {
            continue;
          }

          if(index == mCurrentTurn)
          {
            // The root whose turn was unwinding. Its guards consumed its dirty bits at
            // entry, so re-registration alone would schedule a pass with no record of work.
            // Conservative at the tail of a turn: a root that had already published and then
            // threw from StartTransitionsAfterLayout is re-armed too, which costs it one
            // extra pass that its live caches serve as a hit.
            Internal::ViewDataImpl::Get(*view).RearmLayoutDirtyForAbortedPass();
          }

          mSelf.RetainLayoutRootWithoutWake(view);
        }
      }
      catch(...)
      {
      }
    }

    /// Records that the root at @p index is taking its turn -- called immediately before
    /// that root's member-set entry is erased, so the rollback knows where to resume.
    void BeginTurn(std::size_t index)
    {
      mCurrentTurn = index;
    }

    /// Normal completion: the batch drained and the member set is authoritative again.
    void Commit()
    {
      mArmed = false;
    }

    PendingBatchRollbackScope(const PendingBatchRollbackScope&)            = delete;
    PendingBatchRollbackScope& operator=(const PendingBatchRollbackScope&) = delete;

    LayoutControllerImpl&         mSelf;
    const std::vector<ViewImpl*>& mBatch;
    std::size_t                   mCurrentTurn{0u};
    bool                          mArmed{true};
  };

  /**
   * @brief Constructor.
   */
  explicit LayoutControllerImpl(Window window)
  : mWindow(window),
    mTransitionDispatcher(new Internal::LayoutTransitionDispatcher()),
    mWindowWidth(0),
    mWindowHeight(0),
    mWindowObjectPtr(window.GetObjectPtr()),
    mIdleWakeArmed(false)
  {
    // Get initial window size
    auto    positionSize = window.GetPositionSize();
    Vector2 size         = Vector2(static_cast<float>(positionSize.width), static_cast<float>(positionSize.height));
    mWindowWidth         = static_cast<int32_t>(size.width);
    mWindowHeight        = static_cast<int32_t>(size.height);

    // Register as a processor with the adaptor twice:
    //  - pre  (postProcess=false): run Measure/Arrange before dali Relayout.
    //  - post (postProcess=true):  emit LayoutFinished after size negotiation.
    if(DALI_LIKELY(Adaptor::IsAvailable()))
    {
      Adaptor::Get().RegisterProcessor(*this, false);
      Adaptor::Get().RegisterProcessor(*this, true);

      // Connect to window resize signal
      window.ResizedSignal().Connect(this, &LayoutControllerImpl::OnWindowResized);
    }
  }

  /**
   * @brief Destructor.
   */
  ~LayoutControllerImpl() override
  {
    // Idempotent: already done if this controller was detached before being freed.
    Detach();
  }

  /**
   * @brief Makes this controller inert without freeing anything it owns.
   *
   * Unregisters both processor registrations, drops every ConnectionTracker
   * signal connection (the window ResizedSignal), and releases the transition
   * tick driver, so nothing in the event loop can reach this controller again.
   * Idempotent.
   *
   * Deliberately does NOT clear the pending-view / completion containers:
   * completion delivery may still reference the current record on the stack.
   * They die with the controller at reap time.
   */
  void Detach()
  {
    if(mDetached)
    {
      return;
    }
    mDetached = true;

    // Unregister from adaptor (both the pre and post registrations)
    if(DALI_LIKELY(Adaptor::IsAvailable()))
    {
      Adaptor::Get().UnregisterProcessor(*this, false);
      Adaptor::Get().UnregisterProcessor(*this, true);
    }

    // Drop the window ResizedSignal connection so a resize cannot reach a
    // detached controller.
    DisconnectAll();

    if(mTransitionDispatcher)
    {
      // Stop the self-driving animator tick. The dispatcher is not destroyed
      // here; this can be reached from inside its own tick callback.
      mTransitionDispatcher->Shutdown();
    }
  }

  /**
   * @brief Detaches this controller and hands its ownership to the pending-free
   * list. @em This stays ALIVE - only the free is deferred.
   *
   * Idempotent, and the guard is essential rather than cosmetic: a slot may call
   * LayoutController::Remove() (running this once) and then Get(), which creates
   * a NEW controller under the same window key. The Process() unwind then calls
   * this a second time; without the guard that second call would move the new,
   * innocent controller into the pending-free list.
   */
  void DetachAndQueueForFree()
  {
    if(mDetached)
    {
      return;
    }

    // Suppress any emit still pending for this frame.
    mDestroyPending = true;
    Detach();

    // This registration target is about to disappear. End the propagation
    // generation unconditionally: pending containers may already have been
    // swapped into a local processing batch or changed by re-entrancy, so their
    // apparent emptiness is not a sufficient proof that no recorded walk names
    // this controller. A later invalidation must reach the replacement target.
    Internal::LayoutInvalidation::AdvanceGeneration();

    // The entry under mWindowObjectPtr is necessarily THIS controller here:
    // the only way a controller leaves the map before destruction is this very
    // function, which runs at most once per controller (mDetached guard).
    auto it = gLayoutControllers.find(mWindowObjectPtr);
    if(it != gLayoutControllers.end())
    {
      gDetachedLayoutControllers.push_back(std::move(it->second));
      gLayoutControllers.erase(it);
    }

    // Only ever SCHEDULE here, never free: freeing would run `delete this` from
    // a member function of this very object, possibly while core's processor
    // loop still holds the pointer. The idle callback is the only runtime
    // context that frees.
    ScheduleReapDetachedLayoutControllers();
  }

  /**
   * @brief Retains @p view as a pending layout root WITHOUT requesting an idle wake.
   *
   * The registration half of RequestLayout. Split out for PendingBatchRollbackScope,
   * which runs while an exception is propagating out of a layout pass. A wake there
   * would re-drive the producer that has just thrown, and a deterministically throwing
   * producer would then spin the main loop at full rate -- the same failure the in-pass
   * park rule exists to prevent for invalidation. Note that the pass guards have already
   * unwound by the time the rollback runs, so RequestIdleWakeIfAllowed() would NOT park
   * the request of its own accord: not calling it is the mechanism, not an optimisation.
   *
   * @return True when the view was retained
   */
  bool RetainLayoutRootWithoutWake(ViewImpl* view)
  {
    if(!view || mDetached || mDestroyPending)
    {
      return false;
    }

    // Track this layout root (weak handle for validity checking without extending lifetime)
    Ui::View handle       = Ui::View::DownCast(view->Self());
    mAllLayoutRoots[view] = LayoutRootEntry{WeakHandle<Ui::View>(handle), view};

    // Add to pending (dirty) set
    mPendingViews.insert(view);

    return true;
  }

  /**
   * @brief Schedules a view for layout processing.
   *
   * Any View that is a layout root (has LayoutManager or children) can be registered.
   */
  void RequestLayout(ViewImpl* view)
  {
    if(RetainLayoutRootWithoutWake(view))
    {
      RequestIdleWakeIfAllowed();
    }
  }

  /**
   * @brief Schedules an EXIT-slot transition (forwarded from ViewImpl::Remove).
   */
  void ScheduleLayoutExit(ViewImpl* parent, Ui::View child, ViewImpl* transitionOwner)
  {
    if(mTransitionDispatcher)
    {
      mTransitionDispatcher->ScheduleExit(parent, child, transitionOwner);
    }
  }

  /**
   * @brief Notifies the dispatcher that @p child was just attached to a
   * (new) parent, so any in-flight transition under the old parent can be
   * cancelled before the new parent dispatches ENTER.
   *
   * Forwarded from @c ViewImpl::OnChildAdd. No-op when @p child has no
   * in-flight transition (the common fresh-add case).
   */
  void NotifyChildReparented(ViewImpl* child)
  {
    if(mTransitionDispatcher)
    {
      mTransitionDispatcher->OnChildReparented(child);
    }
  }

  void NotifyChildAdded(ViewImpl* directParent, Ui::View child)
  {
    if(mTransitionDispatcher)
    {
      mTransitionDispatcher->NotifyChildAdded(directParent, child);
    }
  }

  void ClearPendingInheritedEnters(ViewImpl* owner)
  {
    if(mTransitionDispatcher)
    {
      mTransitionDispatcher->ClearPendingInheritedEnters(owner);
    }
  }

  void ClearDetachedSelfState(ViewImpl* child)
  {
    if(mTransitionDispatcher)
    {
      mTransitionDispatcher->ClearDetachedSelfState(child);
    }
  }

  /**
   * @brief Removes a view from tracking on scene disconnection or destruction.
   */
  void UnregisterView(ViewImpl* view)
  {
    mAllLayoutRoots.erase(view);
    mPendingViews.erase(view);

    // A pending registration may have just been dropped without ever being
    // processed. Any view whose recorded propagation generation says "the root I walked to
    // is registered" could be pointing at that dropped entry, so end the generation and
    // make every such record re-walk. Unconditional rather than gated on "was it
    // actually pending": this runs on view teardown, not per frame.
    Internal::LayoutInvalidation::AdvanceGeneration();

    // Scrub EVERY active collector frame (nested passes each have their own),
    // closing the nested-rebind use-after-free: a view collected in an OUTER
    // frame and destroyed during an INNER pass must not remain a dangling raw
    // pointer that the outer snapshot dereferences.
    for(ActiveCollectorFrame& frame : mActiveCollectorStack)
    {
      frame.set->erase(view);
      frame.views->erase(std::remove(frame.views->begin(), frame.views->end(), view), frame.views->end());
    }
    // Unregister also runs on scene disconnection, when a weak handle can still
    // be live. Cancel every snapshot from before the disconnect, including an
    // outer pass's local record and an in-progress post delivery cohort.
    auto forgetSnapshot = [view](LayoutPassCompletion& completion)
    {
      for(auto& event : completion.viewEvents)
      {
        if(event.view == view)
        {
          event.view = nullptr;
        }
      }
    };
    for(auto* scope = mActiveCompletionScope; scope; scope = scope->mPrevious)
    {
      forgetSnapshot(scope->mCompletion);
    }
    for(auto& completion : mPendingPassCompletions)
    {
      forgetSnapshot(completion);
    }
    for(auto& completion : mReadyPassCompletions)
    {
      forgetSnapshot(completion);
    }

    if(mTransitionDispatcher)
    {
      mTransitionDispatcher->OnViewDestroyed(view);
    }
  }

  /**
   * @brief Gets the current window handle managed by this layout controller.
   *
   * Retrieves the window that this layout controller instance is associated with.
   * This is used internally to verify if the window has been replaced.
   *
   * @return The current window handle
   */
  Dali::Window GetCurrentWindow() const
  {
    return mWindow.GetHandle();
  }

  /**
   * @brief Replaces the current window with a new one.
   *
   * Updates the layout controller to manage a different window instance.
   * This is called when a window object has been replaced but the same
   * LayoutController instance should continue managing layouts for the new window.
   * The method reconnects the window resize signal to ensure layout invalidation
   * continues to work correctly.
   *
   * @param[in] window The new window to manage
   */
  void ReplaceCurrentWindow(Dali::Window window)
  {
    DALI_ASSERT_ALWAYS(mWindowObjectPtr == window.GetObjectPtr() && "ReplaceCurrentWindow should be called only for same object ptr case!");

    mWindow = window;
    if(DALI_LIKELY(Adaptor::IsAvailable()))
    {
      // Connect to window resize signal
      window.ResizedSignal().Connect(this, &LayoutControllerImpl::OnWindowResized);
    }
  }

  /**
   * @brief Called when window is resized.
   */
  void OnWindowResize(int32_t width, int32_t height)
  {
    if(mDetached || mDestroyPending)
    {
      return;
    }

    mWindowWidth  = width;
    mWindowHeight = height;

    if(mTransitionDispatcher)
    {
      mTransitionDispatcher->NotifyWindowResize();
    }

    // Invalidate ALL known layout roots (not just pending ones)
    // Collect dead entries to remove after iteration
    std::vector<ViewImpl*> deadEntries;
    for(auto& pair : mAllLayoutRoots)
    {
      if(pair.second.weakHandle.GetHandle())
      {
        // Through the internal primitive, not ViewImpl::InvalidateMeasure(): a
        // window resize is a framework-internal event, not an application call.
        Internal::ViewDataImpl::Get(*pair.second.view).InvalidateMeasure();
      }
      else
      {
        deadEntries.push_back(pair.first);
      }
    }
    for(auto* dead : deadEntries)
    {
      mAllLayoutRoots.erase(dead);
      mPendingViews.erase(dead);
    }
    if(!deadEntries.empty())
    {
      // Same reason as in UnregisterView: pending registrations were dropped without
      // being processed, so no recorded propagation generation can be trusted to describe
      // a live one.
      Internal::LayoutInvalidation::AdvanceGeneration();
    }

    // Preserve the resize wake even when there are no live roots, but coalesce
    // it with any wake already requested by the invalidation walk above. If a
    // resize is delivered re-entrantly from Measure/Arrange or LayoutFinished,
    // its work is parked under the same no-self-wake rule as every other layout
    // request in that window.
    RequestIdleWakeIfAllowed();
  }
  /**
   * @brief Processes all pending views with layout capability.
   */
  void ProcessLayouts()
  {
    // Manual processing is synchronous, but neither consumes an outstanding
    // platform wake nor promotes records before core Relayout has run.
    ProcessImpl(false, true);
  }

  /**
   * @brief Implementation of Integration::Processor::Process.
   */
  void Process(bool postProcess) override
  {
    ProcessImpl(postProcess, false);
  }

  /**
   * @brief Processes one invocation with an explicit origin.
   *
   * A per-call flag keeps a nested automatic invocation distinct from the
   * manual call whose application callback happened to enter the event loop.
   */
  void ProcessImpl(bool postProcess, bool manualInvocation)
  {
    if(DALI_UNLIKELY(mDetached))
    {
      // Detached but not yet freed. Both registrations are already gone, so this
      // can only be a stale call; do nothing.
      return;
    }

    ProcessDepthScope       depthScope(*this);
    GlobalProcessDepthScope globalDepthScope;

    Dali::Window window = mWindow.GetHandle();
    if(DALI_UNLIKELY(!window))
    {
      // The window is gone. Defer self-destruct to the outermost Process frame
      // (handled below) so a re-entrant Process/emit frame is never left
      // running on a destroyed controller.
      mDestroyPending = true;
    }
    else if(!postProcess)
    {
      ProcessLayouts(window, manualInvocation);

      if(!mDestroyPending && mTransitionDispatcher)
      {
        // The dispatcher computes deltaSec from its own wall clock and,
        // once an animator becomes active, also drives a periodic tick
        // timer so subsequent ticks fire even when no other event wakes
        // the event thread.
        mTransitionDispatcher->TickAnimators();
      }

      if(mProcessDepth == 1)
      {
        if(!manualInvocation && !mDestroyPending)
        {
          // The following core Relayout covers every record completed so far.
          // Manual passes from a later post callback remain in the pending queue,
          // even if this controller's post processor has not run yet.
          mReadyPassCompletions.splice(mReadyPassCompletions.end(), mPendingPassCompletions);
        }
        if(mPendingViews.empty())
        {
          mParkedWorkLogged = false;
        }
        else
        {
          LogParkedWorkOnce();
        }
      }
    }
    else if(mProcessDepth == 1 && !mReadyPassCompletions.empty())
    {
      // Preserve the existing no-self-wake policy across both View and Window
      // callbacks. Pending work does not cancel a successfully completed pass.
      Internal::LayoutInvalidation::ScopedLayoutFinishedEmit emitScope;
      EmitLayoutFinishedSignals(window);
      if(!mDestroyPending)
      {
        if(mPendingViews.empty())
        {
          mParkedWorkLogged = false;
        }
        else
        {
          LogParkedWorkOnce();
        }
      }
    }
  }

  /**
   * @brief Implementation of Integration::Processor::GetProcessorName.
   */
  std::string_view GetProcessorName() const override
  {
    return "Ui::LayoutController";
  }

  /**
   * @brief Returns the signal emitted after a successful layout pass.
   */
  LayoutController::LayoutFinishedSignalType& LayoutFinishedSignal()
  {
    return mLayoutFinishedSignal;
  }

  /**
   * @brief Whether @p view is inside @p root's actor subtree. Null-safe off-scene:
   * a null root actor / GetParent()-empty ends the walk.
   */
  bool IsWithinActiveLayoutFinishedRoot(ViewImpl* root, ViewImpl* view) const
  {
    if(!root || !view)
    {
      return false;
    }
    Actor rootActor = root->Self();
    void* rootPtr   = rootActor ? rootActor.GetObjectPtr() : nullptr;
    if(!rootPtr)
    {
      return false;
    }
    for(Actor actor = view->Self(); actor; actor = actor.GetParent())
    {
      if(actor.GetObjectPtr() == rootPtr)
      {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief Records @p view as a candidate for the innermost (top) active root.
   * MUST be public: the free-function forwarder calls it on a LayoutControllerImpl*.
   */
  void NotifyViewArranged(ViewImpl* view)
  {
    if(mActiveCollectorStack.empty() || !view)
    {
      return;
    }
    ActiveCollectorFrame& top = mActiveCollectorStack.back();
    if(!IsWithinActiveLayoutFinishedRoot(top.root, view))
    {
      return;
    }
    if(top.set->insert(view).second)
    {
      top.views->push_back(view);
    }
  }

  /**
   * @brief Captures subscribed Views after RTL and before transition setup.
   *
   * Repeated arrangements merge only within this batch, preserving the first
   * traversal position. Nested batches own separate records and cannot replace
   * the result of an earlier completed pass.
   */
  void SnapshotViewLayoutFinishedCandidates(const std::vector<ViewImpl*>&               arrangedViews,
                                            LayoutPassCompletion&                       completion,
                                            std::unordered_map<ViewImpl*, std::size_t>& eventIndex)
  {
    for(ViewImpl* arranged : arrangedViews)
    {
      if(!arranged)
      {
        continue;
      }
      Ui::View viewHandle = Ui::View::DownCast(arranged->Self());
      if(!viewHandle)
      {
        continue;
      }
      ViewImpl& arrangedImpl = GetImpl(viewHandle);
      if(!Internal::ViewDataImpl::Get(arrangedImpl).HasLayoutFinishedSignalConnections())
      {
        continue;
      }
      Actor      actor = arrangedImpl.Self();
      LayoutRect bounds(
        actor.GetProperty<float>(Actor::Property::POSITION_X),
        actor.GetProperty<float>(Actor::Property::POSITION_Y),
        actor.GetProperty<float>(Actor::Property::SIZE_WIDTH),
        actor.GetProperty<float>(Actor::Property::SIZE_HEIGHT));

      auto indexIt = eventIndex.find(arranged);
      if(indexIt != eventIndex.end())
      {
        PendingViewLayoutFinishedEvent& event        = completion.viewEvents[indexIt->second];
        Ui::View                        previousView = event.weakHandle.GetHandle();
        if(event.view && previousView && previousView.GetObjectPtr() == viewHandle.GetObjectPtr())
        {
          event.bounds = bounds;
          continue;
        }
        // A destroyed implementation address may have been reused. Its expired
        // snapshot keeps its old position; the replacement is a distinct entry.
      }
      eventIndex[arranged] = completion.viewEvents.size();
      completion.viewEvents.push_back(PendingViewLayoutFinishedEvent{arranged, Dali::WeakHandle<Ui::View>(viewHandle), bounds});
    }
  }

  /**
   * @brief Delivers the fixed post-ready cohort in pass-completion order.
   */
  void EmitLayoutFinishedSignals(Dali::Window window)
  {
    // Keep records in the member queue so unregister can tombstone even a
    // currently delivered cohort. Detach deliberately does not clear this queue.
    // Nested processing cannot promote new records
    // at this depth; fixing the count also excludes any later additions.
    std::size_t remaining = mReadyPassCompletions.size();
    while(remaining > 0u && !mDestroyPending)
    {
      LayoutPassCompletion& completion = mReadyPassCompletions.front();
      while(completion.nextView < completion.viewEvents.size() && !mDestroyPending)
      {
        // Advance before invoking a signal: a thrown slot must not replay an
        // already started signal when the remaining records are delivered later.
        const auto& event = completion.viewEvents[completion.nextView++];
        Ui::View    view  = event.view ? event.weakHandle.GetHandle() : Ui::View();
        if(view)
        {
          Internal::ViewDataImpl& viewDataImpl = Internal::ViewDataImpl::Get(GetImpl(view));
          if(viewDataImpl.HasLayoutFinishedSignalConnections())
          {
            viewDataImpl.EmitLayoutFinishedSignal(event.bounds);
          }
        }
      }
      if(mDestroyPending)
      {
        return;
      }
      if(completion.notifyWindow)
      {
        completion.notifyWindow = false;
        mLayoutFinishedSignal.Emit(window);
      }
      mReadyPassCompletions.pop_front();
      --remaining;
    }
  }

private:
  /**
   * @brief Requests one coalesced idle wake outside the no-self-wake layout window.
   *
   * Pending work and wake state are intentionally independent. A request made
   * from Measure/Arrange or LayoutFinished is retained in mPendingViews but
   * cannot make the current layout episode perpetually wake the event loop.
   * Outside that window -- including LayoutTransition lifecycle callbacks after
   * the layout pass -- the first request arms one idle wake; duplicates coalesce
   * until pre-processing consumes that wake.
   */
  void RequestIdleWakeIfAllowed()
  {
    if(mDetached || mDestroyPending)
    {
      return;
    }

    if(Internal::ViewDataImpl::IsLayoutPassOnStack() ||
       Internal::LayoutInvalidation::IsLayoutFinishedEmitInProgress())
    {
      if(gGlobalProcessDepth != 0)
      {
        gDeferredLayoutRequestDuringProcess = true;
      }
      return;
    }

    if(!mIdleWakeArmed && DALI_LIKELY(Adaptor::IsAvailable()))
    {
      mIdleWakeArmed = true;
      Adaptor::Get().RequestProcessEventsOnIdle();
    }
  }

  /**
   * @brief Logs, once per parked episode, that layout work stayed pending with no wake.
   *
   * The per-view diagnostic in ViewDataImpl covers only the public entry points; work
   * parked through framework-internal paths (a child added from OnArrange, a resource
   * callback landing mid-pass) would otherwise defer silently. This is the field-side
   * trace for "why does this view update only on the next touch". One line per episode:
   * the latch resets when the pending set drains, so a persistently diverging producer
   * cannot flood the log at event rate. Skipped while an idle wake is outstanding --
   * such work is about to be serviced and is not at risk of going stale.
   */
  void LogParkedWorkOnce()
  {
    if(mParkedWorkLogged || mPendingViews.empty() || mIdleWakeArmed)
    {
      return;
    }
    mParkedWorkLogged = true;

    // Identify one pending root as helpfully as the handle allows (same ladder as
    // ViewDataImpl::LogInPassInvalidation; this runs at most once per episode).
    Dali::String rootName;
    if(ViewImpl* sample = *mPendingViews.begin())
    {
      Dali::CustomActor self = sample->Self();
      if(self)
      {
        rootName = self.GetProperty<Dali::String>(Dali::Actor::Property::NAME);
        if(rootName.Empty())
        {
          rootName = self.GetTypeName();
        }
      }
    }

    DALI_LOG_ERROR(
      "LayoutController: %zu layout root(s) (e.g. '%s') remain pending after layout "
      "processing, with no idle wake: invalidating layout during Measure/Arrange or "
      "LayoutFinished is prohibited in principle and only honoured best-effort, so such "
      "work never wakes the event loop itself. It is serviced by the next externally "
      "triggered ProcessEvents cycle, which on a quiescent application may be "
      "indefinitely later. LayoutFinished reports completed passes even while this "
      "work remains pending. Defer invalidation to event time for prompt processing.\n",
      mPendingViews.size(),
      rootName.Empty() ? "View" : rootName.CStr());
  }

  /**
   * @brief Processes all pending views with layout capability.
   */
  void ProcessLayouts(Dali::Window window, bool manualInvocation)
  {
    // Begins the dispatcher's batch here and ends it at EVERY exit -- the early return
    // just below, the normal end, and an exception out of any producer or lifecycle
    // callback. The early return still drains per-pass dispatcher state (e.g.
    // mInWindowResize set by NotifyWindowResize) so a stale flag cannot leak into the
    // next pass when every layout root happens to be dead at resize time.
    LayoutPassScope layoutPass(mTransitionDispatcher.get());

    if(mPendingViews.empty())
    {
      // This pre-process invocation consumes any outstanding idle wake, even
      // when its pending root disappeared before the pass. A manual ProcessLayouts
      // call does not consume the already-queued platform callback.
      if(!manualInvocation)
      {
        mIdleWakeArmed = false;
      }
      return;
    }

    // Copy pending views and clear (in case new views are added during processing)
    decltype(mPendingViews) viewsSet;
    viewsSet.swap(mPendingViews);
    if(!manualInvocation)
    {
      mIdleWakeArmed = false;
    }

    // Every pending registration has just been consumed, so no view's recorded
    // propagation generation describes a live registration any more: end the generation here,
    // at the swap, and the next invalidation on any view walks its ancestor chain and
    // re-registers in full. Bumping anywhere later would leave a window in which a
    // view could skip a walk whose registration this swap had already taken.
    //
    // An invalidation raised DURING the drain lands in the now-empty mPendingViews.
    // If its target root has not started its turn in this batch, the registration
    // is consumed immediately before that turn; otherwise it remains pending for
    // a later independently driven pass. Its record is written against the new
    // generation either way.
    Internal::LayoutInvalidation::AdvanceGeneration();

    // Default: window size (when root is directly under window or parent size unknown).
    auto    positionSize     = window.GetPositionSize();
    Vector2 windowSize       = Vector2(static_cast<float>(positionSize.width), static_cast<float>(positionSize.height));
    float   widthConstraint  = static_cast<float>(std::max(0, static_cast<int32_t>(windowSize.width)));
    float   heightConstraint = static_cast<float>(std::max(0, static_cast<int32_t>(windowSize.height)));

    // Sort pending roots by tree depth (outer-most first). When a parent
    // and a descendant root are both in the batch (e.g. a transition-
    // bearing parent and one of its standalone children that is its own
    // layout root), the parent must run first so its arrange pass can
    // measure the descendant via MeasureStandaloneChildren / parent's
    // recursive arrange. Without this ordering the descendant would
    // measure with a stale or zero parent SIZE, dispatch its transitions
    // with bad bounds, and then have those transitions cancelled when
    // the parent's later pass corrects the bounds.
    std::vector<ViewImpl*> viewsToProcess(viewsSet.begin(), viewsSet.end());

    // Armed from HERE, before the sort: from this point the member pending set names none
    // of these roots, so any throw between here and Commit() below would lose them. (The
    // one gap left is a bad_alloc in the vector construction just above, where the batch
    // is genuinely unrecoverable and the process is already failing.)
    PendingBatchRollbackScope rollback(*this, viewsToProcess);

    {
      auto depthOf = [](ViewImpl* v) -> int
      {
        int d = 0;
        if(v)
        {
          for(Dali::Actor p = v->Self().GetParent(); p; p = p.GetParent())
          {
            ++d;
          }
        }
        return d;
      };
      std::sort(viewsToProcess.begin(), viewsToProcess.end(),
                [&depthOf](ViewImpl* a, ViewImpl* b)
      {
        return depthOf(a) < depthOf(b);
      });
    }

    LayoutPassCompletion                       completion;
    std::unordered_map<ViewImpl*, std::size_t> eventIndex;
    bool                                       processedRoot = false;
    ActiveCompletionScope                      completionScope(*this, completion);

    // Process each layout root. Only this local record is discarded on failure;
    // records already published by successful nested batches remain queued.
    for(std::size_t index = 0u; index < viewsToProcess.size(); ++index)
    {
      if(mDestroyPending)
      {
        rollback.Commit();
        return;
      }
      ViewImpl* view = viewsToProcess[index];

      // From here the member set no longer names this root even transiently, so tell the
      // rollback where the batch stopped before anything can throw.
      rollback.BeginTurn(index);

      // Verify the view is still tracked and alive
      auto it = mAllLayoutRoots.find(view);
      if(view && it != mAllLayoutRoots.end() && it->second.weakHandle.GetHandle())
      {
        if(mTransitionDispatcher)
        {
          mTransitionDispatcher->CaptureBeforeLayout(view);
        }

        // A preceding root may have dirtied and re-registered this root after
        // the batch swap. Because this root has not begun its own pass yet, the
        // pass below observes and consumes that work; remove only that pre-turn
        // duplicate. Any invalidation raised from Measure/Arrange itself lands
        // after this erase and therefore remains parked for a later pass.
        mPendingViews.erase(view);

        // Collect subscribed Views arranged under this root into STACK-LOCAL
        // containers; the RAII scope pushes a frame (and sets the file-static)
        // for the duration of ProcessLayoutRoot, popping on exit even if an
        // assert throws mid-arrange.
        std::vector<ViewImpl*>        arrangedViews;
        std::unordered_set<ViewImpl*> arrangedSet;
        {
          ActiveLayoutFinishedScope scope(*this, view, arrangedViews, arrangedSet);
          ProcessLayoutRoot(view, widthConstraint, heightConstraint);
        }

        // Snapshot now: ProcessLayoutRoot has applied RTL (actor POSITION_X final)
        // and StartTransitions has not yet overwritten actor props. Views destroyed
        // during ProcessLayoutRoot were scrubbed from arrangedViews by UnregisterView
        // (across all frames), and only property reads run here, so Self() is safe.
        if(mDestroyPending)
        {
          rollback.Commit();
          return;
        }
        SnapshotViewLayoutFinishedCandidates(arrangedViews, completion, eventIndex);

        if(mTransitionDispatcher)
        {
          mTransitionDispatcher->StartTransitionsAfterLayout(view);
        }
        processedRoot = true;
      }
      else
      {
        mPendingViews.erase(view);
        if(it != mAllLayoutRoots.end())
        {
          // Dead entry — clean up
          mAllLayoutRoots.erase(it);
        }
      }
    }

    if(processedRoot && !mDestroyPending)
    {
      completion.notifyWindow = !mLayoutFinishedSignal.Empty();
      if(completion.notifyWindow || !completion.viewEvents.empty())
      {
        // A batch with no subscribers needs no completion allocation. Window
        // eligibility is captured here; delivery uses the current connections.
        mPendingPassCompletions.push_back(std::move(completion));
      }
    }

    // The batch drained normally; the member set is authoritative again.
    rollback.Commit();
  }

  /**
   * @brief Processes a single view with layout capability (layout root).
   *
   * Constraint priority chain (per-dimension):
   *   1. Explicit RequestedWidth/Height (>= 0): use that value.
   *   2. Else if the view's Actor has a parent: use parent.Actor.SIZE (as-is,
   *      including 0). When this view is a standalone (boundary) child, a
   *      parent size of 0 means the parent has not been arranged yet —
   *      measuring with 0 is correct because the parent's subsequent
   *      Measure/Arrange pass will re-measure this child via
   *      MeasureStandaloneChildren/ArrangeStandaloneChildren with the real
   *      size.
   *   3. Else (no parent actor): use the window-size constraint passed in.
   *
   * - WRAP_CONTENT / MATCH_PARENT: RequestedWidth/Height < 0 → fall through
   *   to parent/window path.
   * - Fixed (>= 0): takes precedence over parent/window.
   *
   * Every branch yields a VISUAL (scale-applied) constraint. An explicit
   * requested size is stored in NATURAL units and is therefore multiplied by
   * the view's effective scale here, while the parent actor SIZE and the
   * window size are already visual. The margin subtraction below is visual for
   * the same reason, so a FIXED axis composes to
   * (requested - marginNatural) * s.
   */
  void ProcessLayoutRoot(ViewImpl* view, float widthConstraint, float heightConstraint)
  {
    if(!view)
    {
      return;
    }

    // Snapshotted BEFORE Measure(): the available extent below and the slot derivation
    // at the end of this function must be fed the same values, even if the view's own
    // measure producer mutates its scale, margin or requested size mid-pass.
    const Internal::StandaloneSlotInputs inputs = Internal::SnapshotStandaloneSlotInputs(*view);

    float layoutWidth  = view->GetRequestedWidth();
    float layoutHeight = view->GetRequestedHeight();

    Actor self   = view->Self();
    Actor parent = self.GetParent();

    // The requested size is stored in NATURAL units, and the constraint this
    // function builds is VISUAL, like the other two sources (the window size
    // and the parent actor SIZE, both already visual) and like the parameter
    // ViewDataImpl::Measure takes. Convert here, or the producer is run at a
    // constraint 1/s too small.
    if(layoutWidth >= 0.0f)
    {
      widthConstraint = layoutWidth * inputs.scale;
    }
    else if(parent)
    {
      widthConstraint = parent.GetProperty<float>(Actor::Property::SIZE_WIDTH);
    }

    if(layoutHeight >= 0.0f)
    {
      heightConstraint = layoutHeight * inputs.scale;
    }
    else if(parent)
    {
      heightConstraint = parent.GetProperty<float>(Actor::Property::SIZE_HEIGHT);
    }

    // Root view has no parent that subtracts margin, so do it here. Margin,
    // requested position and min/max are stored in natural units, while the
    // layout pass works in visual (scale-applied) units. Convert with the
    // root's effective scale so the root matches the non-root path, which
    // already scales these properties.
    const float marginW = (inputs.margin.start + inputs.margin.end) * inputs.scale;
    const float marginH = (inputs.margin.top + inputs.margin.bottom) * inputs.scale;
    widthConstraint     = std::max(0.0f, widthConstraint - marginW);
    heightConstraint    = std::max(0.0f, heightConstraint - marginH);

    // Measure pass
    MeasuredSize measuredSize = view->Measure(widthConstraint, heightConstraint);

    // Arrange pass: use the user-set position (parent is not a layout).
    // MATCH_PARENT roots fill the available constraint rather than using their
    // measured (minimum) size, and the root's own min/max is enforced on the result.
    // The whole derivation -- position, extents and clamp -- is the shared helper, so
    // this root pass and the parent-driven ArrangeStandaloneChild placement of the
    // same view cannot drift apart.
    const LayoutRect bounds = Internal::DeriveStandaloneRootBounds(inputs, widthConstraint, heightConstraint, measuredSize);

    // Use the internal root entry point rather than the public Arrange path. For a
    // STANDALONE boundary this identifies the framework-owned self pass whose bounds
    // are derived by the SAME helper as the parent's ArrangeStandaloneChild path
    // (DeriveStandaloneRootBounds); an application calling View::Arrange directly
    // carries no such ownership and retracts the parent's arrange entry when it
    // rewrites the child's records.
    Internal::ViewDataImpl::Get(*view).ArrangeAsLayoutRoot(bounds);
  }

  /**
   * @brief Callback for window resize signal.
   */
  void OnWindowResized(Window window, Window::WindowSize size)
  {
    OnWindowResize(size.GetWidth(), size.GetHeight());
  }

private:
  Dali::WeakHandle<Window>                              mWindow;
  std::unique_ptr<Internal::LayoutTransitionDispatcher> mTransitionDispatcher;
  std::unordered_map<ViewImpl*, LayoutRootEntry>        mAllLayoutRoots; ///< All known layout roots (weak-referenced)
  std::unordered_set<ViewImpl*>                         mPendingViews;   ///< Dirty layout roots needing processing
  int32_t                                               mWindowWidth;
  int32_t                                               mWindowHeight;
  void*                                                 mWindowObjectPtr;                ///< For self-destruct case.
  bool                                                  mIdleWakeArmed;                  ///< True only while one idle ProcessEvents wake is outstanding
  bool                                                  mParkedWorkLogged{false};        ///< One diagnostic per parked episode
  LayoutController::LayoutFinishedSignalType            mLayoutFinishedSignal;           ///< Emitted for successful layout passes
  int                                                   mProcessDepth{0};                ///< Re-entrancy depth and deferred-destroy safe point
  bool                                                  mDestroyPending{false};          ///< Deferred self-destruct requested during processing
  bool                                                  mDetached{false};                ///< Detached and awaiting idle-time destruction
  std::vector<ActiveCollectorFrame>                     mActiveCollectorStack;           ///< Nested per-root collectors
  ActiveCompletionScope*                                mActiveCompletionScope{nullptr}; ///< Stack-local pass snapshots awaiting publication
  LayoutPassCompletions                                 mPendingPassCompletions;         ///< Successful passes awaiting automatic pre processing
  LayoutPassCompletions                                 mReadyPassCompletions;           ///< Passes covered by the following core Relayout
};

} // namespace Integration

LayoutController& LayoutController::Get(Window window)
{
  DALI_ASSERT_ALWAYS(Adaptor::IsAvailable() && "LayoutController::Get() could not be called from worker thread, or app is not running!");

  void* key = window.GetObjectPtr();

  auto it = gLayoutControllers.find(key);
  if(it != gLayoutControllers.end())
  {
    auto& layoutController = *(it->second);
    if(DALI_UNLIKELY(layoutController.GetCurrentWindow() != window))
    {
      layoutController.ReplaceCurrentWindow(window);
    }
    return *(it->second);
  }

  // Create new controller for this window
  auto  controller        = std::unique_ptr<LayoutController>(new LayoutController(window));
  auto& ref               = *controller;
  gLayoutControllers[key] = std::move(controller);

  return ref;
}

void LayoutController::Remove(Window window)
{
  if(DALI_LIKELY(Adaptor::IsAvailable()))
  {
    if(window)
    {
      auto it = gLayoutControllers.find(window.GetObjectPtr());
      if(it != gLayoutControllers.end())
      {
        // Never free inline. Remove() is a public entry point that can run with
        // dali-core's processor loop on the stack (a LayoutFinished slot, or app
        // code reached from ANY processor's Process()), and dali-ui cannot
        // observe that loop's boundary. Detach now - the controller stops
        // processing and emitting immediately - and let the idle reap free it
        // outside the loop.
        it->second->mImpl->DetachAndQueueForFree();
      }
    }
  }
}

void LayoutController::UnregisterFromAll(ViewImpl* view)
{
  if(DALI_LIKELY(Adaptor::IsAvailable()))
  {
    for(auto& pair : gLayoutControllers)
    {
      pair.second->UnregisterView(view);
    }
    // Detached controllers are out of the live map but are still ALIVE and still
    // hold raw ViewImpl pointers until they are reaped, so they must be scrubbed
    // too - otherwise a View destroyed inside the detach->reap window would leave
    // a dangling pointer behind.
    for(auto& controller : gDetachedLayoutControllers)
    {
      controller->UnregisterView(view);
    }
  }
}

LayoutController::LayoutController(Window window)
: mImpl(MakeUnique<Integration::LayoutControllerImpl>(window))
{
}

LayoutController::~LayoutController()
{
}

void LayoutController::RequestLayout(ViewImpl* view)
{
  // Keep the diagnostic on the application-facing entry point, but never drop
  // the request. LayoutControllerImpl records it in the pending set and applies
  // the centralized no-self-wake policy to both public and internal paths.
  if(view != nullptr &&
     (Internal::ViewDataImpl::IsLayoutPassOnStack() ||
      Internal::LayoutInvalidation::IsLayoutFinishedEmitInProgress()))
  {
    Internal::ViewDataImpl::Get(*view).LogInPassInvalidation("LayoutController::RequestLayout");
  }

  mImpl->RequestLayout(view);
}

void LayoutController::RequestLayoutInternal(ViewImpl* view)
{
  mImpl->RequestLayout(view);
}

void LayoutController::UnregisterView(ViewImpl* view)
{
  mImpl->UnregisterView(view);
}

void LayoutController::OnWindowResize(int32_t width, int32_t height)
{
  mImpl->OnWindowResize(width, height);
}

void LayoutController::ProcessLayouts()
{
  mImpl->ProcessLayouts();
}

void LayoutController::NotifyViewArranged(ViewImpl* view)
{
  if(gActiveLayoutFinishedController)
  {
    gActiveLayoutFinishedController->NotifyViewArranged(view);
  }
}

LayoutController::LayoutFinishedSignalType& LayoutController::LayoutFinishedSignal()
{
  return mImpl->LayoutFinishedSignal();
}

void LayoutController::ScheduleLayoutExit(ViewImpl* parent, Ui::View child, ViewImpl* transitionOwner)
{
  mImpl->ScheduleLayoutExit(parent, child, transitionOwner);
}

void LayoutController::NotifyChildReparented(ViewImpl* child)
{
  mImpl->NotifyChildReparented(child);
}

void LayoutController::NotifyChildAdded(ViewImpl* directParent, Ui::View child)
{
  mImpl->NotifyChildAdded(directParent, child);
}

void LayoutController::ClearPendingInheritedEnters(ViewImpl* owner)
{
  mImpl->ClearPendingInheritedEnters(owner);
}

void LayoutController::ClearDetachedSelfState(ViewImpl* child)
{
  mImpl->ClearDetachedSelfState(child);
}

Dali::Window LayoutController::GetCurrentWindow() const
{
  return mImpl->GetCurrentWindow();
}

void LayoutController::ReplaceCurrentWindow(Dali::Window window)
{
  mImpl->ReplaceCurrentWindow(window);
}

} // namespace Ui
} //namespace DALI_NAMESPACE
