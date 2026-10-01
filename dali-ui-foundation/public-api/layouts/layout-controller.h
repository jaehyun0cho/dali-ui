#pragma once

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

// EXTERNAL INCLUDES
#include <dali/public-api/adaptor-framework/window.h>
#include <dali/public-api/common/unique-ptr.h>
#include <dali/public-api/signals/dali-signal.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/dali-ui-common.h>
#include <dali-ui-foundation/public-api/layouts/layout-types.h>

namespace DALI_NAMESPACE
{
namespace Ui
{

// Forward declarations
class ViewImpl;
namespace Integration
{
class LayoutControllerImpl;
} // namespace Integration

/**
 * @brief Layout controller manages layout invalidation and processing.
 *
 * The layout controller is responsible for scheduling layout passes when
 * layouts become invalid and processing them during the render loop.
 * It integrates with the DALi adaptor system as a processor.
 *
 * Each Window has its own LayoutController instance. The controller
 * batches layout requests and processes them once per frame to optimize
 * performance.
 */
class DALI_UI_API LayoutController
{
public:
  /**
   * @brief Gets the layout controller instance for a window.
   *
   * Creates the controller if it doesn't exist for the given window.
   *
   * @param[in] window The window to get the controller for
   * @return Reference to the layout controller
   */
  static LayoutController& Get(Window window);

  /**
   * @brief Destructor.
   */
  ~LayoutController();

  /**
   * @brief Schedules a view with layout capability for layout processing.
   *
   * Layout roots are views that have a LayoutManager and are at the top
   * of the layout hierarchy (e.g. directly under the window).
   * Outside layout processing, the controller batches these requests and arms one
   * coalesced outstanding ProcessEvents wake for a following processing cycle.
   *
   * Requests made during Measure/Arrange, transition setup in the root batch,
   * or LayoutFinished delivery are retained and coalesced for an automatically
   * scheduled continuation. A not-yet-started root in the current batch may
   * consume the request immediately. Remaining work needs no additional input
   * from the application while the adaptor continues processing events.
   *
   * Automatic continuations are paced on the event thread using a monotonic
   * deadline, initially based on 60fps. The delay after a turn is at least one
   * frame period and at least that turn's layout and completion-callback cost.
   * This is rate limiting, not VSYNC synchronization or a frame deadline promise.
   * Other event processing cannot bypass an active continuation's deadline.
   * Normal requests outside layout processing retain their low-latency idle wake.
   * In particular, animation completion from TickAnimators is outside the batch;
   * transition OnStart callbacks inside the batch create paced continuations.
   *
   * A producer or transition-setup exception retains failed work without
   * automatically retrying it. A fresh request outside layout processing or
   * explicit ProcessLayouts() allows another attempt. Completion records from
   * earlier successful passes remain independent of those failed roots.
   *
   * @param[in] view The view with layout capability to schedule
   */
  void RequestLayout(ViewImpl* view);

  /**
   * @brief Schedules a view for layout processing on the framework's behalf.
   *
   * @note For internal framework use only; applications must use RequestLayout().
   * This is the registration path taken by the invalidation walk itself. "Internal"
   * describes API visibility only: it is subject to the same processing-window wake
   * policy as RequestLayout(), so framework routing cannot bypass continuation pacing.
   *
   * @param[in] view The view with layout capability to schedule
   */
  void RequestLayoutInternal(ViewImpl* view);

  /**
   * @brief Removes the layout controller for the given window.
   *
   * Should be called when a window is being closed to release the
   * associated LayoutController and prevent stale entries.
   *
   * @param[in] window The window whose controller should be removed
   */
  static void Remove(Window window);

  /**
   * @brief Unregisters a view from the layout controller.
   *
   * Should be called when a layout root is being destroyed to prevent
   * dangling pointer access.
   *
   * @param[in] view The view to unregister
   */
  void UnregisterView(ViewImpl* view);

  /**
   * @brief Unregisters a view from all known layout controllers.
   *
   * Used when a view is being destroyed but its window cannot be
   * determined (e.g. the view is already off-scene).
   *
   * @param[in] view The view to unregister
   */
  static void UnregisterFromAll(ViewImpl* view);

  /**
   * @brief Called when the window is resized.
   *
   * This invalidates all layout roots and triggers a layout pass.
   *
   * @param[in] width The new window width
   * @param[in] height The new window height
   */
  void OnWindowResize(int32_t width, int32_t height);

  /**
   * @brief Processes all pending layout requests immediately.
   *
   * The system processes layout automatically during event processing. This
   * method may be called manually when immediate calculation is needed; its
   * completion signals are deferred until an automatic pre phase and the
   * following core Relayout and post-process phase.
   */
  void ProcessLayouts();

  /**
   * @brief Signal type of LayoutFinishedSignal().
   */
  using LayoutFinishedSignalType = Signal<void(Dali::Window)>;

  /**
   * @brief Emitted after a layout pass for this window completes successfully.
   *
   * A pass processes the batch of pending layout roots taken at its start.
   * Further requests may remain pending when this signal fires; it does not
   * indicate that the window's layout has stabilized.
   *
   * A slot connects with the signature:
   * @code
   *   void OnLayoutFinished(Dali::Window window);
   * @endcode
   *
   * Semantics:
   * - Fires once for each successful pass that processes at least one live
   *   root, including a pass that leaves pending requests or unchanged bounds.
   *   Empty processing and batches interrupted by an exception do not fire it.
   * - Emitted during post-process, AFTER DALi core size negotiation (Relayout).
   *   Measure and Arrange run before core Relayout. A manual ProcessLayouts()
   *   calculates synchronously but does not emit synchronously: its completion
   *   waits for an automatic pre phase and the following core Relayout and post.
   * - Multiple completed passes are delivered in completion order, without
   *   merging their notifications. Each pass's subscribed View signals precede
   *   its Window signal. A View callback's new requests do not cancel that
   *   Window signal. Passes completed during delivery wait for a later cycle.
   * - A Window notification is recorded only if this signal has a connection
   *   when the pass completes. Delivery uses the connections present at emit
   *   time; connecting does not replay passes for which none was recorded.
   * - Reflects calculation completion only, not presentation or the end of
   *   layout transition animations. Earlier pass results need not match live
   *   Actor properties when several passes are delivered together.
   * - A slot may invalidate layout, change properties, or mutate the tree.
   *   The work is retained and automatically scheduled as a paced continuation;
   *   it does not cancel this pass's remaining notifications. Use conditional
   *   changes: unconditional invalidation can produce an unending sequence of
   *   passes even though execution is rate limited.
   * - Explicit ProcessLayouts() remains synchronous and bypasses automatic
   *   calculation pacing. Its notifications still wait for a permitted automatic
   *   pre/core Relayout/post cycle; calling it from a slot cannot create an
   *   unbounded automatic notification loop. Repeated explicit manual calls are
   *   the caller's responsibility.
   * - If a callback throws, the exception propagates. The signal whose delivery
   *   started is not replayed; remaining View and Window notifications are kept
   *   for a later post-process invocation. Earlier successful passes remain
   *   reportable even if a later layout batch fails.
   * - LayoutController::Remove() inside a callback immediately stops further
   *   delivery and detaches the controller. Destruction waits until idle so
   *   DALi core cannot dereference a freed processor.
   *
   * @return The layout-finished signal
   * @note The Window identifies the controller whose pass completed. The
   * controller can be obtained again with Get().
   */
  LayoutFinishedSignalType& LayoutFinishedSignal();

public: // Not intended for application developers
  /// @cond internal
  /**
   * @brief Records @p view as arranged for View::LayoutFinishedSignal during the
   * currently active LayoutController pass. No-op outside a managed root pass.
   *
   * @param[in] view The view whose Arrange() has just completed
   */
  DALI_INTERNAL static void NotifyViewArranged(ViewImpl* view);

  /**
   * @brief Schedules an EXIT-slot layout transition for @p child under
   * @p parent.
   *
   * Called by ViewImpl::Remove / RemoveAll when the parent
   * has a LayoutTransition EXIT slot configured through a visual spec,
   * animator, or active bounds effect. The dispatcher fires the EXIT
   * animation and unparents the child only when the animation finishes.
   *
   * @param[in] parent          The child's direct (visual) parent (ghost host
   *                            / unparent target)
   * @param[in] child           The child view to remove (kept alive by a
   *                            strong ref inside the dispatcher until EXIT
   *                            completes)
   * @param[in] transitionOwner The view whose LayoutTransition drives the EXIT
   *                            effect; @c nullptr means @p parent (direct EXIT).
   *                            Differs from @p parent only for SUBTREE-scope
   *                            inherited EXIT.
   */
  DALI_INTERNAL void ScheduleLayoutExit(ViewImpl* parent, Ui::View child, ViewImpl* transitionOwner = nullptr);

  /**
   * @brief Notifies the layout transition dispatcher that @p child was
   * just attached to a (new) parent.
   *
   * Called by @c ViewImpl::OnChildAdd. When the child has an in-flight
   * transition under an old parent (reparent during EXIT), the dispatcher
   * cancels it so the application callback does not keep firing against
   * the old parent's coordinate system. No-op when there is no in-flight
   * state for @p child (the common fresh-add case).
   *
   * @param[in] child The child view whose actor was just attached
   */
  DALI_INTERNAL void NotifyChildReparented(ViewImpl* child);

  /**
   * @brief Notifies the dispatcher that @p child was added under @p directParent
   * so the ENTER of this add can be routed to the transition that governs it (the
   * child's own self transition, the direct parent's, or the closest ancestor
   * SUBTREE owner's).
   *
   * Called by @c ViewImpl::OnChildAdd for every add; a no-op when the direct
   * parent's own transition claims the child.
   *
   * @param[in] directParent The child's direct parent
   * @param[in] child         The freshly added child
   */
  DALI_INTERNAL void NotifyChildAdded(ViewImpl* directParent, Ui::View child);

  /**
   * @brief Drops inherited-ENTER candidates registered against @p owner in the
   * dispatcher, called when @p owner detaches its LayoutTransition.
   *
   * @param[in] owner The view whose transition was just detached
   */
  DALI_INTERNAL void ClearPendingInheritedEnters(ViewImpl* owner);

  /**
   * @brief Drops the self-role records the dispatcher holds for @p child -- the
   * add-time pending ENTER candidate and the current pass's snapshot -- called
   * when @p child detaches its self LayoutTransition or is gated by a non-AUTO
   * LayoutTransitionMode.
   *
   * @param[in] child The view whose self transition was just detached
   */
  DALI_INTERNAL void ClearDetachedSelfState(ViewImpl* child);
  /// @endcond

private:
  /**
   * @brief Private constructor.
   *
   * Use LayoutController::Get() to obtain an instance.
   *
   * @param[in] window The window this controller manages
   */
  explicit DALI_INTERNAL LayoutController(Window window);

  // Not copyable or movable
  LayoutController(const LayoutController&)            = delete;
  LayoutController(LayoutController&&)                 = delete;
  LayoutController& operator=(const LayoutController&) = delete;
  LayoutController& operator=(LayoutController&&)      = delete;

private: // Not be opened for application developer
  /**
   * @brief Gets the current window handle managed by this layout controller.
   *
   * Retrieves the window that this layout controller instance is associated with.
   * This is used internally to verify if the window has been replaced.
   *
   * @return The current window handle
   */
  DALI_INTERNAL Dali::Window GetCurrentWindow() const;

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
  DALI_INTERNAL void ReplaceCurrentWindow(Dali::Window window);

private:
  Dali::UniquePtr<Integration::LayoutControllerImpl> mImpl;
};

} // namespace Ui
} //namespace DALI_NAMESPACE
