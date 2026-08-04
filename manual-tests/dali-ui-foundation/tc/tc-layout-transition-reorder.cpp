/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
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
 */

#include "manual-test-case.h"

#include <dali/devel-api/actors/actor-devel.h>
#include <dali/public-api/adaptor-framework/timer.h>

#include <algorithm>
#include <cstdint>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
constexpr Vector4 PALETTE[] = {
  Color::RED, Color::GREEN, Color::BLUE,
  Color::YELLOW, Color::CYAN, Color::MAGENTA};
constexpr uint32_t PALETTE_SIZE = sizeof(PALETTE) / sizeof(PALETTE[0]);

// ── Auto-scroll while dragging near ScrollView edges ─────────────────────────
constexpr float    AUTO_SCROLL_EDGE_ZONE = 60.0f; ///< px from viewport edge that triggers auto-scroll
constexpr float    AUTO_SCROLL_MAX_STEP  = 12.0f; ///< px scrolled per tick at the very edge
constexpr uint32_t AUTO_SCROLL_TICK_MS   = 16u;   ///< ~60 Hz
} // namespace

/**
 * @brief Verifies drag reorder driven by the LayoutTransition CHANGE slot.
 *
 * A vertical StackLayout owns a LayoutTransition with declarative ENTER /
 * EXIT / CHANGE specs sharing a single 0.4s EASE_IN_OUT_SINE timing. The
 * stack is the content of a vertical ScrollView so it can hold more items
 * than fit on screen. The edit button toggles reorder mode. In edit mode,
 * pressing a child removes it from the StackLayout, floats it inside the
 * test's own root at its press-time bounds, and inserts a same-sized
 * invisible (OPACITY 0) proxy into its slot to reserve the layout space.
 * Drag moves the floating child by RequestedX/Y; each time the child's Y
 * crosses a sibling's Y the proxy is Inserted at the new index and the
 * CHANGE slot animates the sibling reflow. Near the top / bottom edge of
 * the ScrollView viewport a timer-driven auto-scroll kicks in. On release
 * the proxy is removed and the child returns to the stack at the proxy's
 * index, with its arranged bounds pre-baked so the slide-into-slot
 * animation starts from the on-screen drop position.
 *
 * Ported from samples/layout-transition/layout-transition-reorder-example.cpp.
 */
class TcLayoutTransitionReorder : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "LayoutTransition: List Reorder (Drag)";
  }

  Dali::String GetDescription() const override
  {
    return "Edit-mode drag reorder with proxy slot, CHANGE reflow and edge auto-scroll";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    // LayoutController::Get() only accepts a Window, so cache the window that
    // owns contentArea. The handle is READ-ONLY: no window state is ever
    // modified from here, and it is released again in OnExit.
    mWindow = Window::Get(contentArea);

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Three Clickable labels. The third one toggles reorder edit mode.
    mEnterButton = MakeClickableLabel("Click to ENTER");
    mEnterButton.TouchEventSignal().Connect(this, &TcLayoutTransitionReorder::OnEnterButtonTouched);

    mExitButton = MakeClickableLabel("Click to EXIT");
    mExitButton.TouchEventSignal().Connect(this, &TcLayoutTransitionReorder::OnExitButtonTouched);

    mEditButton = MakeClickableLabel("Click to Edit");
    mEditButton.TouchEventSignal().Connect(this, &TcLayoutTransitionReorder::OnEditButtonTouched);

    mStack = StackLayout::New();
    mStack.SetRequestedWidth(MATCH_PARENT);
    // Stack must be tall enough to hold all children — let StackLayout
    // wrap its content vertically so the ScrollView treats it as
    // scrollable content rather than a viewport-sized rectangle.
    mStack.SetRequestedHeight(WRAP_CONTENT);
    mStack.SetSpacing(10.0f);

    LayoutTransitionTiming timing{Duration(0.4f),
                                  AlphaFunction(AlphaFunction::EASE_IN_OUT_SINE),
                                  Duration()};

    ViewAnimationSpec enterSpec = ViewAnimationSpec::New();
    enterSpec.Opacity(1.0f, Duration(0.4f), AlphaFunction(AlphaFunction::EASE_IN_OUT_SINE));

    ViewAnimationSpec exitSpec = ViewAnimationSpec::New();
    exitSpec.Opacity(0.0f, Duration(0.4f), AlphaFunction(AlphaFunction::EASE_IN_OUT_SINE));

    LayoutTransition transition = LayoutTransition::New();
    transition.SetEnterVisualSpec(enterSpec)
      .SetExitVisualSpec(exitSpec)
      .SetEnterBoundsEffect(LayoutBoundsEffects::ExpandFrom(
        LayoutBoundsEdge::TOP, timing))
      .SetExitBoundsEffect(LayoutBoundsEffects::ShrinkTo(
        LayoutBoundsEdge::TOP, timing))
      .SetChangeTiming(timing);

    mStack.SetLayoutTransition(transition);

    // Start with enough items to overflow the viewport so the
    // ScrollView's edge-zone auto-scroll is immediately demonstrable.
    for(int i = 0; i < 12; ++i)
    {
      AppendChild();
    }

    FlexLayout buttonRow = FlexLayout::New();
    buttonRow.SetRequestedWidth(MATCH_PARENT);
    buttonRow.SetRequestedHeight(60.0f);
    buttonRow.SetDirection(FlexDirection::ROW);
    buttonRow.SetAlignItems(FlexAlign::STRETCH);
    buttonRow.Add(mEnterButton);
    buttonRow.Add(mExitButton);
    buttonRow.Add(mEditButton);

    // Vertical ScrollView wraps mStack so the list can grow taller than
    // the available area.
    mScrollView = ScrollView::New();
    mScrollView.SetScrollDirection(ScrollDirection::Vertical);
    mScrollView.SetOverScrollMode(OverScrollMode::ContentScrolls);
    mScrollView.SetRequestedWidth(MATCH_PARENT);
    mScrollView.SetRequestedHeight(MATCH_PARENT);
    mScrollView.SetContent(mStack);

    mRoot = StackLayout::New();
    mRoot.SetRequestedWidth(MATCH_PARENT);
    mRoot.SetRequestedHeight(MATCH_PARENT);
    mRoot.Add(buttonRow);
    mRoot.Add(mScrollView);

    mBackdrop.Add(mRoot);
    contentArea.Add(mBackdrop);
  }

  void OnExit() override
  {
    // 1. Abort any in-flight drag WITHOUT touching the scene graph: the
    //    subtree is about to be discarded, so FinishDrag()'s Insert / Remove
    //    and ProcessLayouts() must not run here.
    AbortDrag();

    // 2. Stop and release the timer so the next entry re-creates it and
    //    reconnects TickSignal.
    if(mAutoScrollTimer)
    {
      mAutoScrollTimer.Stop();
      mAutoScrollTimer.Reset();
    }

    // 3-4. Animations / externally-connected signals: none in this TC (no-op).

    // 5. Release every retained handle.
    mBackdrop.Reset();
    mRoot.Reset();
    mScrollView.Reset();
    mStack.Reset();
    mEnterButton.Reset();
    mExitButton.Reset();
    mEditButton.Reset();

    // 6. Release the cached window handle.
    mWindow.Reset();

    // 7. Remaining scalars back to their initial values.
    mNextColorIndex = 0u;
    mEditMode       = false;
  }

private:
  void ResetState()
  {
    AbortDrag();
    mNextColorIndex = 0u;
    mEditMode       = false;
  }

  /**
   * @brief Drops all drag state without touching the scene graph.
   *
   * Used on entry / exit, where the tree the drag refers to is either not
   * built yet or about to be discarded. FinishDrag() must not be used for
   * that: it performs Insert / Remove and drives a layout pass.
   */
  void AbortDrag()
  {
    StopAutoScrollTimer();
    mDragging = false;
    mDraggedChild.Reset();
    mDragProxy.Reset();
    mDraggedIndex         = 0u;
    mDragBounds           = {};
    mDragGrabOffsetY      = 0.0f;
    mDraggedOriginalReqW  = 0.0f;
    mDraggedOriginalReqH  = 0.0f;
    mLastDragRootPosition = Vector2::ZERO;
  }

  Label MakeClickableLabel(const Dali::String& text)
  {
    Label label = Label::New();
    label.SetText(text);
    label.SetTextColor(Color::WHITE);
    label.SetHorizontalTextAlignment(Text::Alignment::CENTER);
    label.SetVerticalTextAlignment(Text::Alignment::CENTER);
    label.SetBackgroundColor(Color::BLACK);
    label.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f));
    label.SetMargin(Extents(5, 5, 0, 0));
    return label;
  }

  void AppendChild()
  {
    CancelDrag();

    View child = View::New();
    child.SetBackgroundColor(PALETTE[mNextColorIndex % PALETTE_SIZE]);
    child.SetRequestedWidth(MATCH_PARENT);
    child.SetRequestedHeight(80.0f);
    child.SetProperty(Actor::Property::OPACITY, 0.0f);
    // Capture all subsequent touch events on the actor that received the
    // initial DOWN. During reorder drag the child is reparented to the
    // test root and the finger frequently leaves its bounds (auto-scroll
    // zones, clamped at viewport edge, fast motion). Capture keeps the
    // event stream on the dragged child and — critically — bypasses the
    // parent ScrollView's gesture intercept so the ScrollView never tries
    // to pan while a reorder is in progress.
    child.SetProperty(DevelActor::Property::CAPTURE_ALL_TOUCH_AFTER_START, true);
    child.TouchEventSignal().Connect(this, &TcLayoutTransitionReorder::OnChildTouched);

    mStack.Add(child);
    ++mNextColorIndex;
  }

  void RemoveLastChild()
  {
    CancelDrag();

    const uint32_t count = mStack.GetChildViewCount();
    if(count == 0u)
    {
      return;
    }

    // The logical child list excludes any in-flight EXIT ghost, so the last
    // logical child is the visually-last live item (CancelDrag above already
    // removed the drag proxy and reinserted any dropped child).
    View last = mStack.GetChildViewAt(count - 1u);
    mStack.Remove(last, RemovePolicy::ANIMATE_EXIT);
  }

  bool OnEnterButtonTouched(Actor /*actor*/, TouchEvent touch)
  {
    if(IsTouchStart(touch))
    {
      AppendChild();
      return true;
    }
    return false;
  }

  bool OnExitButtonTouched(Actor /*actor*/, TouchEvent touch)
  {
    if(IsTouchStart(touch))
    {
      RemoveLastChild();
      return true;
    }
    return false;
  }

  bool OnEditButtonTouched(Actor /*actor*/, TouchEvent touch)
  {
    if(!IsTouchStart(touch))
    {
      return false;
    }

    CancelDrag();
    mEditMode = !mEditMode;
    mEditButton.SetBackgroundColor(mEditMode ? Vector4(0.0f, 0.35f, 0.65f, 1.0f) : Color::BLACK);
    return true;
  }

  bool OnChildTouched(Actor actor, TouchEvent touch)
  {
    if(!mEditMode || touch.GetPointCount() < 1u)
    {
      return false;
    }

    Vector2 rootPosition;
    if(!GetRootLocalPosition(touch, rootPosition))
    {
      return false;
    }

    const PointState::Type state = touch.GetState(0u);

    // Not yet dragging: only DOWN on a tracked item starts a drag. The
    // index lookup rejects DOWN events that arrive through mStack
    // (gap area between children) so an empty-area press cannot begin a
    // drag on an unrelated actor.
    if(!mDragging)
    {
      if(!IsDownState(state))
      {
        return false;
      }
      View          child     = View::DownCast(actor);
      const int32_t itemIndex = child ? FindIndexInStack(child) : -1;
      if(itemIndex < 0)
      {
        return false;
      }
      BeginDrag(child, static_cast<uint32_t>(itemIndex), rootPosition);
      return true;
    }

    // Drag in progress: every MOTION / UP routes through here because
    // CAPTURE_ALL_TOUCH_AFTER_START on the originally-pressed child pins
    // the touch stream to that child regardless of where the finger
    // actually lands on screen.
    if(state == PointState::MOTION)
    {
      UpdateDrag(rootPosition);
      return true;
    }
    if(IsUpState(state))
    {
      UpdateDrag(rootPosition);
      FinishDrag();
      return true;
    }
    return true;
  }

  static bool IsDownState(PointState::Type state)
  {
    return state == PointState::DOWN || state == PointState::STARTED;
  }

  static bool IsUpState(PointState::Type state)
  {
    return state == PointState::UP ||
           state == PointState::FINISHED ||
           state == PointState::INTERRUPTED ||
           state == PointState::LEAVE;
  }

  static bool IsTouchStart(TouchEvent touch)
  {
    return touch.GetPointCount() > 0u && IsDownState(touch.GetState(0u));
  }

  bool GetRootLocalPosition(TouchEvent touch, Vector2& localPosition)
  {
    const Vector2 screenPosition = touch.GetScreenPosition(0u);
    float         localX         = 0.0f;
    float         localY         = 0.0f;
    if(!mRoot.ScreenToLocal(localX, localY, screenPosition.x, screenPosition.y))
    {
      return false;
    }

    localPosition = Vector2(localX, localY);
    return true;
  }

  int32_t FindIndexInStack(View child) const
  {
    return mStack.IndexOfChildView(child);
  }

  // X / Y of mStack's top-left in mRoot-local space. mStack lives inside
  // the ScrollView, so the ScrollView's own offset inside mRoot must be
  // added; mStack.POSITION already includes any scroll offset because
  // ScrollView drives scrolling by writing to its content's
  // POSITION_X / POSITION_Y.
  float GetStackWorldX() const
  {
    return mScrollView.GetCurrentProperty<float>(Actor::Property::POSITION_X) +
           mStack.GetCurrentProperty<float>(Actor::Property::POSITION_X);
  }

  float GetStackWorldY() const
  {
    return mScrollView.GetCurrentProperty<float>(Actor::Property::POSITION_Y) +
           mStack.GetCurrentProperty<float>(Actor::Property::POSITION_Y);
  }

  LayoutRect GetChildRootBounds(View child) const
  {
    const float stackX = GetStackWorldX();
    const float stackY = GetStackWorldY();
    LayoutRect  bounds;
    bounds.x      = stackX + child.GetCurrentProperty<float>(Actor::Property::POSITION_X);
    bounds.y      = stackY + child.GetCurrentProperty<float>(Actor::Property::POSITION_Y);
    bounds.width  = child.GetCurrentProperty<float>(Actor::Property::SIZE_WIDTH);
    bounds.height = child.GetCurrentProperty<float>(Actor::Property::SIZE_HEIGHT);
    return bounds;
  }

  uint32_t ComputeTargetIndexFromDraggedY(float draggedRootY) const
  {
    const uint32_t count = mStack.GetChildViewCount();
    if(count == 0u)
    {
      return 0u;
    }

    // The proxy reserves mDraggedChild's slot inside mStack while the actual
    // child floats above the list. The target index is "how many non-proxy
    // children sit above the dragged child" — when the dragged child's Y
    // crosses an existing sibling's Y, the count changes and Insert moves
    // the proxy through the logical order accordingly.
    //
    // Compared in mStack-local space so the read of child POSITION_Y is
    // directly comparable; both are derived from the same root mapping.
    // GetStackWorldY() adds ScrollView's own offset and mStack's
    // scroll-driven POSITION_Y, so this stays correct as the ScrollView
    // scrolls underneath the floating dragged child.
    const float stackY        = GetStackWorldY();
    const float draggedStackY = draggedRootY - stackY;

    uint32_t target = 0u;
    for(uint32_t i = 0u; i < count; ++i)
    {
      View child = mStack.GetChildViewAt(i);
      if(child == mDragProxy)
      {
        continue;
      }
      const float childY = child.GetCurrentProperty<float>(Actor::Property::POSITION_Y);
      if(childY < draggedStackY)
      {
        ++target;
      }
    }
    return target;
  }

  void BeginDrag(View child, uint32_t itemIndex, const Vector2& rootPosition)
  {
    CancelDrag();

    // Bounds of the pressed child at press time, in mRoot-local space. The
    // floating dragged child uses these directly as its initial position and
    // size, and the proxy's reserved slot matches because it inherits the
    // child's layout-request size below.
    const LayoutRect bounds = GetChildRootBounds(child);

    mDraggedChild         = child;
    mDraggedIndex         = itemIndex;
    mDragBounds           = bounds;
    mDragGrabOffsetY      = rootPosition.y - bounds.y;
    mDraggedOriginalReqW  = child.GetRequestedWidth();
    mDraggedOriginalReqH  = child.GetRequestedHeight();
    mLastDragRootPosition = rootPosition;
    mDragging             = true;

    // Swap the pressed child for the proxy invisibly. The dragged child is
    // removed with RemovePolicy::IMMEDIATE (unparent now, no EXIT animation).
    // We also detach the LayoutTransition for the swap so the proxy's
    // subsequent Insert is not marked as a pending ENTER and faded in from
    // OPACITY 0. The swap is supposed to be invisible — siblings should not
    // move and the slot should look unchanged — so we re-attach right after,
    // which leaves CHANGE animations for the in-drag reorders.
    LayoutTransition savedTransition = mStack.GetLayoutTransition();
    mStack.SetLayoutTransition(LayoutTransition());
    mStack.Remove(mDraggedChild, RemovePolicy::IMMEDIATE);

    // Proxy reserves the dragged child's slot. Same layout-request size
    // means the layout pass produces the same arranged rectangle, so
    // siblings around the slot do not shift. OPACITY 0 keeps the slot
    // visually empty; SENSITIVE false lets touches pass through to whatever
    // is underneath if the finger ever moves over the empty slot.
    mDragProxy = View::New();
    mDragProxy.SetRequestedWidth(mDraggedOriginalReqW);
    mDragProxy.SetRequestedHeight(mDraggedOriginalReqH);
    mDragProxy.SetProperty(Actor::Property::OPACITY, 0.0f);
    mDragProxy.SetProperty(Actor::Property::SENSITIVE, false);
    mStack.Insert(mDraggedIndex, mDragProxy);

    mStack.SetLayoutTransition(savedTransition);

    // Float the actual dragged child with its press-time bounds.
    // RequestedWidth/Height are set to explicit pixel values (overriding
    // e.g. MATCH_PARENT) so the layout pass does not resize it to fill its
    // new parent. The original request values were saved above and are
    // restored on FinishDrag.
    mDraggedChild.SetRequestedWidth(bounds.width);
    mDraggedChild.SetRequestedHeight(bounds.height);
    mDraggedChild.SetRequestedX(bounds.x);
    mDraggedChild.SetRequestedY(bounds.y);
    // The float stays INSIDE the test's own subtree instead of going to the
    // window: LayoutMode::STANDALONE excludes it from mRoot's accumulation
    // and positions it straight from RequestedX/Y in mRoot's coordinate
    // space — exactly the space mDragBounds is expressed in. Keeping it
    // under mRoot also means the launcher reclaims it if the user leaves
    // the test case mid-drag.
    mDraggedChild.SetLayoutMode(LayoutMode::STANDALONE);
    mRoot.Add(mDraggedChild);
    mDraggedChild.RaiseToTop(LayoutOrderPolicy::PRESERVE);

    StartAutoScrollTimer();
  }

  void UpdateDrag(const Vector2& rootPosition)
  {
    if(!mDragging || !mDraggedChild)
    {
      return;
    }

    // Cache so the auto-scroll timer can re-run the drag update with the
    // same finger position after each scroll step.
    mLastDragRootPosition = rootPosition;

    // Clamp to the visible ScrollView viewport instead of the full stack:
    // mStack now extends beyond the viewport vertically (that's the point
    // of wrapping it in a ScrollView), and the floating dragged child has
    // to stay on screen so the user can see what they are dragging.
    const float scrollViewY = mScrollView.GetCurrentProperty<float>(Actor::Property::POSITION_Y);
    const float scrollViewH = mScrollView.GetCurrentProperty<float>(Actor::Property::SIZE_HEIGHT);
    const float minY        = scrollViewY;
    const float maxY        = std::max(minY, scrollViewY + scrollViewH - mDragBounds.height);
    const float draggedY    = std::clamp(rootPosition.y - mDragGrabOffsetY, minY, maxY);
    mDragBounds.y           = draggedY;

    // Drive position through RequestedX/Y: the dragged child is a
    // STANDALONE child of mRoot, so mRoot's arrange writes its
    // POSITION_X/Y from GetRequestedX/Y.
    mDraggedChild.SetRequestedX(mDragBounds.x);
    mDraggedChild.SetRequestedY(mDragBounds.y);

    const uint32_t targetIndex = ComputeTargetIndexFromDraggedY(mDragBounds.y);
    if(targetIndex != mDraggedIndex)
    {
      // mStack still has the proxy at mDraggedIndex; Insert reorders it to
      // targetIndex and fires REORDERED CHANGE on every sibling so the
      // visual reflow animates. The transition stays attached here so the
      // configured CHANGE timing drives the animation.
      mStack.Insert(targetIndex, mDragProxy);
      mDraggedIndex = targetIndex;
    }
  }

  void FinishDrag()
  {
    if(!mDragging)
    {
      return;
    }

    StopAutoScrollTimer();

    View       droppedChild  = mDraggedChild;
    View       proxyToRemove = mDragProxy;
    const uint32_t   targetIndex  = mDraggedIndex;
    const float      originalReqW = mDraggedOriginalReqW;
    const float      originalReqH = mDraggedOriginalReqH;
    const LayoutRect dragBounds   = mDragBounds;

    // Reset state first so any layout work triggered by the swap below
    // sees a clean controller (no pending drag). mLastDragRootPosition
    // is left alone — the next BeginDrag overwrites it before any
    // consumer reads it.
    mDragging            = false;
    mDraggedChild        = View();
    mDragProxy           = View();
    mDraggedIndex        = 0u;
    mDraggedOriginalReqW = 0.0f;
    mDraggedOriginalReqH = 0.0f;
    mDragGrabOffsetY     = 0.0f;
    mDragBounds          = {};

    if(!droppedChild || !proxyToRemove)
    {
      return;
    }

    // Pre-bake the dropped child's mArrangedBounds to the drop position
    // expressed in mStack-local coordinates while it is still floating.
    // Without this pre-bake, the next CHANGE transition would read the
    // floating mRoot-local value (mArrangedBounds.y == drop root Y) as the
    // snapshot, and then re-interpret it as stack-local — producing a
    // SetProperty that teleports the actor down by mStack's offset before
    // the slide-in animation starts. The CHANGE animation must instead
    // start from the drop position on screen.
    //
    // Stack-local Y that maps to the drop position when the child is
    // re-parented under mStack:
    //     dropStackY = drop root Y - mStack root Y
    // mStack's root Y has to include the ScrollView's own offset and the
    // current scroll-driven content offset (both folded into
    // GetStackWorldY()).
    const float stackWorldX = GetStackWorldX();
    const float stackWorldY = GetStackWorldY();
    const float dropStackX  = dragBounds.x - stackWorldX;
    const float dropStackY  = dragBounds.y - stackWorldY;
    droppedChild.SetRequestedX(dropStackX);
    droppedChild.SetRequestedY(dropStackY);
    // Drive the layout pass synchronously so the Arrange that writes
    // mArrangedBounds happens BEFORE the swap below — between the swap
    // and the next mStack layout pass nothing else calls Arrange on the
    // dropped child, so the stack-local value above is the value the
    // CHANGE snapshot will see.
    if(mWindow)
    {
      LayoutController::Get(mWindow).ProcessLayouts();
    }

    // Mirror of BeginDrag's swap: remove the proxy with RemovePolicy::IMMEDIATE
    // (no EXIT) and detach the transition so Insert on the dropped child does
    // not mark it for ENTER. Re-attach right after so the next mStack layout
    // pass dispatches CHANGE on the dropped child using the pre-baked snapshot.
    LayoutTransition savedTransition = mStack.GetLayoutTransition();
    mStack.SetLayoutTransition(LayoutTransition());

    mStack.Remove(proxyToRemove, RemovePolicy::IMMEDIATE);

    if(droppedChild.GetParent())
    {
      droppedChild.Unparent();
    }
    // Give the child back to the parent's layout: BeginDrag switched it to
    // LayoutMode::STANDALONE so it could float from RequestedX/Y, and
    // mStack must own its placement again.
    droppedChild.SetLayoutMode(LayoutMode::DEFAULT);
    // Restore original layout-request so mStack arranges the child back
    // into a normal slot (e.g. MATCH_PARENT width). Clear the explicit
    // position so the parent arrange — not the floating path — owns the
    // final placement. These setters only flip RequestedW/H and
    // RequestedX/Y; they do NOT call Arrange, so the mArrangedBounds value
    // baked above is preserved until the next mStack layout pass takes its
    // snapshot.
    droppedChild.SetRequestedWidth(originalReqW);
    droppedChild.SetRequestedHeight(originalReqH);
    droppedChild.SetRequestedX(0.0f);
    droppedChild.SetRequestedY(0.0f);
    mStack.Insert(targetIndex, droppedChild);

    mStack.SetLayoutTransition(savedTransition);
  }

  void CancelDrag()
  {
    if(mDragging)
    {
      FinishDrag();
    }
  }

  // ── Auto-scroll while dragging near ScrollView edges ───────────────────────
  //
  // The timer runs for the lifetime of a drag. Each tick computes how far
  // the last finger position has pushed into the top / bottom edge zone of
  // the visible viewport and feeds a proportional scroll delta into
  // ScrollView::ScrollTo. After scrolling, UpdateDrag is re-evaluated with
  // the unchanged finger position so the proxy reorder keeps tracking the
  // finger as the content slides underneath the floating dragged child.

  void StartAutoScrollTimer()
  {
    if(!mAutoScrollTimer)
    {
      mAutoScrollTimer = Timer::New(AUTO_SCROLL_TICK_MS);
      mAutoScrollTimer.TickSignal().Connect(this, &TcLayoutTransitionReorder::OnAutoScrollTick);
    }
    if(!mAutoScrollTimer.IsRunning())
    {
      mAutoScrollTimer.Start();
    }
  }

  void StopAutoScrollTimer()
  {
    if(mAutoScrollTimer && mAutoScrollTimer.IsRunning())
    {
      mAutoScrollTimer.Stop();
    }
  }

  bool OnAutoScrollTick()
  {
    if(!mDragging || !mScrollView)
    {
      return false;
    }

    // Viewport bounds in mRoot-local space — the same coordinate system as
    // mLastDragRootPosition, which comes from mRoot.ScreenToLocal on the
    // most recent touch event.
    const float viewportTop    = mScrollView.GetCurrentProperty<float>(Actor::Property::POSITION_Y);
    const float viewportHeight = mScrollView.GetCurrentProperty<float>(Actor::Property::SIZE_HEIGHT);
    const float viewportBottom = viewportTop + viewportHeight;
    const float touchY         = mLastDragRootPosition.y;

    // Intensity ramps linearly from 0 at AUTO_SCROLL_EDGE_ZONE inside the
    // viewport to 1 (clamped) at the edge or beyond. Past the edge the
    // value would exceed 1 — clamp so a far-out finger does not produce
    // runaway scroll speed.
    float scrollStepY = 0.0f;
    if(touchY < viewportTop + AUTO_SCROLL_EDGE_ZONE)
    {
      const float intensity = std::clamp((viewportTop + AUTO_SCROLL_EDGE_ZONE - touchY) / AUTO_SCROLL_EDGE_ZONE, 0.0f, 1.0f);
      scrollStepY           = -AUTO_SCROLL_MAX_STEP * intensity;
    }
    else if(touchY > viewportBottom - AUTO_SCROLL_EDGE_ZONE)
    {
      const float intensity = std::clamp((touchY - (viewportBottom - AUTO_SCROLL_EDGE_ZONE)) / AUTO_SCROLL_EDGE_ZONE, 0.0f, 1.0f);
      scrollStepY           = AUTO_SCROLL_MAX_STEP * intensity;
    }

    if(scrollStepY != 0.0f)
    {
      const Vector2 current = mScrollView.GetScrollPosition();
      const Vector2 target(current.x, current.y + scrollStepY);
      mScrollView.ScrollTo(target, false);

      // Re-evaluate the proxy slot: the finger has not moved but the
      // content has, so the dragged child's mStack-local Y is different
      // and the target index may need to change.
      UpdateDrag(mLastDragRootPosition);
    }

    return true;
  }

  View        mBackdrop;
  Window      mWindow;              ///< Read-only handle needed by LayoutController::Get
  StackLayout mRoot;
  ScrollView  mScrollView;          ///< Vertical scroll wrapper around mStack
  StackLayout mStack;
  Label       mEnterButton;
  Label       mExitButton;
  Label       mEditButton;
  uint32_t    mNextColorIndex = 0u;
  bool        mEditMode       = false;
  bool        mDragging       = false;
  View        mDraggedChild;        ///< The actual child, floating under mRoot during drag
  View        mDragProxy;           ///< Empty (OPACITY 0) slot reserving the dragged child's place in mStack
  uint32_t    mDraggedIndex = 0u;   ///< Current index of mDragProxy in mStack
  LayoutRect  mDragBounds{};        ///< Dragged child's mRoot-local bounds (driven by finger)
  float       mDragGrabOffsetY     = 0.0f;
  float       mDraggedOriginalReqW = 0.0f; ///< Restored to mDraggedChild on FinishDrag
  float       mDraggedOriginalReqH = 0.0f; ///< Restored to mDraggedChild on FinishDrag
  Vector2     mLastDragRootPosition{0.0f, 0.0f}; ///< Latest finger position in mRoot-local space; drives auto-scroll
  Timer       mAutoScrollTimer;     ///< Fires while a drag is in flight to apply edge-zone auto-scroll
};

REGISTER_MANUAL_TEST(TcLayoutTransitionReorder)
