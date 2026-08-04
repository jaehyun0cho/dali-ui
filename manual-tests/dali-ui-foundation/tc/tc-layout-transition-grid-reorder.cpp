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
#include <cmath>
#include <cstdint>
#include <vector>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
constexpr uint32_t COLUMNS   = 3u;
constexpr float    GRID_ICON = 100.0f; ///< Grid tile size
constexpr float    TOP_ICON  = 50.0f;  ///< Notification / edit tile size
constexpr float    BTN_ICON  = 100.0f; ///< Wi-Fi / Bluetooth tile size
constexpr float    CELL_W    = 120.0f; ///< Grid cell width
constexpr float    CELL_H    = 150.0f; ///< Grid cell height

constexpr uint32_t LONG_PRESS_MS  = 500u;  ///< Hold time that starts a drag
constexpr float    MOVE_THRESHOLD = 20.0f; ///< Pre-drag slip that cancels the long press

constexpr float    AUTO_SCROLL_EDGE_ZONE = 60.0f; ///< px from viewport edge that triggers auto-scroll
constexpr float    AUTO_SCROLL_MAX_STEP  = 12.0f; ///< px scrolled per tick at the very edge
constexpr uint32_t AUTO_SCROLL_TICK_MS   = 16u;   ///< ~60 Hz

/// One distinct hue per grid cell so every cell stays individually
/// identifiable while it is dragged through the order.
constexpr uint32_t TILE_COLORS[] = {
  0xE53935, 0xFB8C00, 0xFDD835, 0x7CB342,
  0x00897B, 0x00ACC1, 0x1E88E5, 0x3949AB,
  0x8E24AA, 0xD81B60, 0x6D4C41, 0x546E7A};

constexpr uint32_t COLOR_NOTIFICATION = 0xFFC107;
constexpr uint32_t COLOR_EDIT         = 0x9E9E9E;
constexpr uint32_t COLOR_WIFI         = 0x03A9F4;
constexpr uint32_t COLOR_BLUETOOTH    = 0x304FFE;

/// Names shown under each grid tile; also the logical identity of a cell.
constexpr const char* const GRID_ICON_NAMES[] = {
  "brightness", "color", "energy", "game",
  "multi view", "output display", "picture", "settings",
  "share", "sound", "support", "timer"};

/**
 * @brief Builds one square, rounded, solid-colour tile.
 *
 * The sample this test is ported from loads an SVG per icon from its own
 * resource directory. The manual-test resource directory ships no SVGs, so
 * every icon is substituted by a same-sized coloured tile. The reorder
 * behaviour under test is unaffected: sizes, labels and cell geometry are
 * unchanged, and the distinct colours keep each cell identifiable.
 */
View MakeTile(float size, uint32_t color)
{
  View tile = View::New();
  tile.SetRequestedWidth(size);
  tile.SetRequestedHeight(size);
  tile.SetBackgroundColor(UiColor(color));
  tile.SetCornerRadius(size * 0.25f);
  return tile;
}
} // namespace

/**
 * @brief Verifies long-press drag reorder of a GridLayout.
 *
 * A white AbsoluteLayout root holds a translucent rounded panel at (80, 80)
 * with a right-aligned notification / edit row, a Wi-Fi / Bluetooth button
 * row, and a scrollable 3-column GridLayout. The grid owns a
 * LayoutTransition whose CHANGE slot uses the 0.4s EASE_IN_OUT_SINE timing
 * shared by the other layout-transition tests. Long-pressing a cell floats
 * it at its press-time bounds while an invisible proxy reserves its slot;
 * dragging moves the proxy to the cell under the finger and reassigns every
 * cell's Row / Column, so the CHANGE slot animates the reflow. Dragging
 * near the top / bottom of the grid auto-scrolls.
 *
 * Ported from samples/layout-transition/layout-transition-grid-reorder-example.cpp.
 */
class TcLayoutTransitionGridReorder : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "LayoutTransition: Grid Reorder (Long-press Drag)";
  }

  Dali::String GetDescription() const override
  {
    return "Long-press drag reorders GridLayout cells; CHANGE animates the reflow";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    // LayoutController::Get() only accepts a Window, so cache the window that
    // owns contentArea. The handle is READ-ONLY: no window state is ever
    // modified from here, and it is released again in OnExit.
    mWindow = Window::Get(contentArea);

    // CHANGE timing shared with the other layout-transition tests.
    LayoutTransitionTiming timing{Duration(0.4f),
                                  AlphaFunction(AlphaFunction::EASE_IN_OUT_SINE),
                                  Duration()};

    // ── Child 1: transparent row, right-aligned notification + edit ──────────
    StackLayout topRow = StackLayout::New(StackOrientation::HORIZONTAL);
    topRow.SetRequestedWidth(MATCH_PARENT);
    topRow.SetRequestedHeight(WRAP_CONTENT);
    topRow.SetSpacing(20.0f);

    View topSpacer = View::New();
    topSpacer.SetRequestedHeight(1.0f);
    topSpacer.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f));
    topRow.Add(topSpacer);
    topRow.Add(MakeTile(TOP_ICON, COLOR_NOTIFICATION));
    topRow.Add(MakeTile(TOP_ICON, COLOR_EDIT));

    // ── Child 2: Wi-Fi button (left) and Bluetooth button (right) ────────────
    StackLayout midRow = StackLayout::New(StackOrientation::HORIZONTAL);
    midRow.SetRequestedWidth(MATCH_PARENT);
    midRow.SetRequestedHeight(WRAP_CONTENT);

    View midSpacer = View::New();
    midSpacer.SetRequestedHeight(1.0f);
    midSpacer.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f));
    midRow.Add(MakeIconButton("wifi", COLOR_WIFI));
    midRow.Add(midSpacer);
    midRow.Add(MakeIconButton("bluetooth", COLOR_BLUETOOTH));

    // ── Child 3: scrollable 3-column reorderable grid ────────────────────────
    const uint32_t iconCount = static_cast<uint32_t>(sizeof(GRID_ICON_NAMES) / sizeof(GRID_ICON_NAMES[0]));
    const uint32_t rowCount  = (iconCount + COLUMNS - 1u) / COLUMNS;

    mGrid = GridLayout::New();
    mGrid.SetRequestedWidth(MATCH_PARENT);
    // WRAP_CONTENT so the grid can grow taller than the ScrollView viewport
    // and the content becomes scrollable.
    mGrid.SetRequestedHeight(WRAP_CONTENT);
    for(uint32_t c = 0u; c < COLUMNS; ++c)
    {
      mGrid.AddColumnDefinition(GridLength::Absolute(CELL_W));
    }
    for(uint32_t r = 0u; r < rowCount; ++r)
    {
      mGrid.AddRowDefinition(GridLength::Absolute(CELL_H));
    }

    LayoutTransition transition = LayoutTransition::New();
    transition.SetChangeTiming(timing);
    mGrid.SetLayoutTransition(transition);

    for(uint32_t i = 0u; i < iconCount; ++i)
    {
      View cell = MakeGridCell(GRID_ICON_NAMES[i], TILE_COLORS[i]);
      mGridItems.push_back(cell);
      mGrid.Add(cell);
    }
    ApplyGridOrder();

    // The ScrollView's height comes from the StackLayout weight (it fills the
    // space left by the two rows above), so its main-axis RequestedHeight is
    // left unset; only the cross-axis width is requested.
    mScrollView = ScrollView::New();
    mScrollView.SetScrollDirection(ScrollDirection::Vertical);
    mScrollView.SetOverScrollMode(OverScrollMode::ContentScrolls);
    mScrollView.SetRequestedWidth(MATCH_PARENT);
    mScrollView.SetContent(mGrid);
    mScrollView.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f));

    // ── Outer translucent panel ──────────────────────────────────────────────
    mOuterStack = StackLayout::New(StackOrientation::VERTICAL);
    // A definite main-axis size is required for the weighted ScrollView child to
    // fill the remaining space; without it the stack wraps to its content and
    // the ScrollView grows to the grid's full (scrollable) height instead.
    mOuterStack.SetRequestedWidth(400.0f);
    mOuterStack.SetRequestedHeight(600.0f);
    mOuterStack.SetSpacing(20.0f);
    mOuterStack.SetPadding(Extents(20, 20, 20, 20));
    mOuterStack.SetBackgroundColor(Vector4(0.5f, 0.5f, 0.5f, 0.5f));
    mOuterStack.SetCornerRadius(40.0f);
    mOuterStack.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds(LayoutRect(80.0f, 80.0f, 400.0f, 600.0f)));
    mOuterStack.Add(topRow);
    mOuterStack.Add(midRow);
    mOuterStack.Add(mScrollView);

    // ── White root filling the content area ──────────────────────────────────
    // This root already paints the sample's window background, so it doubles
    // as the test's backdrop; no extra wrapper view is needed.
    mRoot = AbsoluteLayout::New();
    mRoot.SetRequestedWidth(MATCH_PARENT);
    mRoot.SetRequestedHeight(MATCH_PARENT);
    mRoot.SetBackgroundColor(Color::WHITE);
    mRoot.Add(mOuterStack);

    contentArea.Add(mRoot);
  }

  void OnExit() override
  {
    // 1. Abort any in-flight interaction WITHOUT touching the scene graph:
    //    the subtree is about to be discarded, so FinishDrag()'s Add / Remove
    //    and ProcessLayouts() must not run here.
    AbortDrag();
    CancelLongPress();

    // 2. Stop and release both timers so the next entry re-creates them and
    //    reconnects TickSignal.
    if(mAutoScrollTimer)
    {
      mAutoScrollTimer.Stop();
      mAutoScrollTimer.Reset();
    }
    if(mLongPressTimer)
    {
      mLongPressTimer.Stop();
      mLongPressTimer.Reset();
    }

    // 3-4. Animations / externally-connected signals: none in this TC (no-op).

    // 5. Release every retained handle and clear the handle container.
    mGridItems.clear();
    mRoot.Reset();
    mOuterStack.Reset();
    mScrollView.Reset();
    mGrid.Reset();
    mPressedItem.Reset();

    // 6. Release the cached window handle.
    mWindow.Reset();

    // 7. Remaining scalars back to their initial values.
    mPressIndex        = 0u;
    mPressRootPosition = Vector2::ZERO;
  }

private:
  void ResetState()
  {
    AbortDrag();
    CancelLongPress();
    // Cleared before the grid is rebuilt so a second entry starts from the
    // canonical order instead of appending to the previous entry's list.
    mGridItems.clear();
    mPressIndex        = 0u;
    mPressRootPosition = Vector2::ZERO;
  }

  /**
   * @brief Drops all drag state without touching the scene graph.
   *
   * Used on entry / exit, where the tree the drag refers to is either not
   * built yet or about to be discarded. FinishDrag() must not be used for
   * that: it performs Add / Remove and drives a layout pass.
   */
  void AbortDrag()
  {
    StopAutoScrollTimer();
    mDragging = false;
    mDraggedChild.Reset();
    mDragProxy.Reset();
    mDraggedIndex          = 0u;
    mDragBounds            = {};
    mDragGrabOffset        = Vector2::ZERO;
    mDraggedOriginalReqW   = 0.0f;
    mDraggedOriginalReqH   = 0.0f;
    mLastTouchRootPosition = Vector2::ZERO;
  }

  // ── Builders ───────────────────────────────────────────────────────────────

  // Wi-Fi / Bluetooth button: 100x100 tile with the name label to its right.
  View MakeIconButton(const Dali::String& name, uint32_t color)
  {
    StackLayout button = StackLayout::New(StackOrientation::HORIZONTAL);
    button.SetRequestedWidth(WRAP_CONTENT);
    button.SetRequestedHeight(WRAP_CONTENT);
    button.SetSpacing(8.0f);
    button.Add(MakeTile(BTN_ICON, color));

    Label label = Label::New(name);
    label.SetTextColor(Color::WHITE);
    label.SetVerticalTextAlignment(Text::Alignment::CENTER);
    label.SetFontSize(16.0f);
    label.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::CENTER));
    button.Add(label);
    return button;
  }

  // Grid cell: 100x100 tile with the name label below it. The tile and
  // label are insensitive so the cell itself is the touch / capture target.
  View MakeGridCell(const Dali::String& name, uint32_t color)
  {
    StackLayout cell = StackLayout::New(StackOrientation::VERTICAL);
    cell.SetRequestedWidth(WRAP_CONTENT);
    cell.SetRequestedHeight(WRAP_CONTENT);
    cell.SetSpacing(4.0f);

    View icon = MakeTile(GRID_ICON, color);
    icon.SetProperty(Actor::Property::SENSITIVE, false);
    icon.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::CENTER));
    cell.Add(icon);

    Label label = Label::New(name);
    label.SetTextColor(Color::WHITE);
    label.SetHorizontalTextAlignment(Text::Alignment::CENTER);
    label.SetFontSize(14.0f);
    label.SetProperty(Actor::Property::SENSITIVE, false);
    label.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::CENTER));
    cell.Add(label);

    // Capture all touch on the cell after the initial DOWN so the drag stream
    // stays on the cell once it is reparented to the root and bypasses the
    // ScrollView's gesture intercept during a reorder.
    cell.SetProperty(DevelActor::Property::CAPTURE_ALL_TOUCH_AFTER_START, true);
    cell.TouchEventSignal().Connect(this, &TcLayoutTransitionGridReorder::OnItemTouched);
    return cell;
  }

  // Reassigns every logical item's grid cell from its index in mGridItems.
  // SetLayoutParams invalidates measure, so the next layout pass reflows and
  // the CHANGE slot animates the items that moved.
  void ApplyGridOrder()
  {
    for(uint32_t i = 0u; i < mGridItems.size(); ++i)
    {
      mGridItems[i].SetLayoutParams(GridLayoutParams::New()
                                      .SetRow(i / COLUMNS)
                                      .SetColumn(i % COLUMNS)
                                      .SetHorizontalAlignment(LayoutAlignment::CENTER)
                                      .SetVerticalAlignment(LayoutAlignment::CENTER));
    }
  }

  // ── Touch handling ───────────────────────────────────────────────────────

  bool OnItemTouched(Actor actor, TouchEvent touch)
  {
    if(touch.GetPointCount() < 1u)
    {
      return false;
    }

    Vector2 rootPosition;
    if(!GetRootLocalPosition(touch, rootPosition))
    {
      return false;
    }

    const PointState::Type state = touch.GetState(0u);

    if(!mDragging)
    {
      // DOWN on a tracked cell arms a long press; the drag only begins when
      // the long-press timer fires with the finger still down and still.
      if(IsDownState(state))
      {
        View          cell      = View::DownCast(actor);
        const int32_t itemIndex = cell ? FindItemIndex(cell) : -1;
        if(itemIndex < 0)
        {
          return false;
        }
        mPressedItem           = cell;
        mPressIndex            = static_cast<uint32_t>(itemIndex);
        mPressRootPosition     = rootPosition;
        mLastTouchRootPosition = rootPosition;
        mLongPressArmed        = true;
        StartLongPressTimer();
        return true;
      }

      if(mLongPressArmed && state == PointState::MOTION)
      {
        mLastTouchRootPosition = rootPosition;
        // Moving too far before the long press fires cancels it (treated as a
        // scroll / slip rather than a reorder intent).
        if((rootPosition - mPressRootPosition).Length() > MOVE_THRESHOLD)
        {
          CancelLongPress();
        }
        return true;
      }

      if(IsUpState(state))
      {
        CancelLongPress();
        return true;
      }
      return false;
    }

    // Drag in progress.
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

  // INTERRUPTED is deliberately NOT treated as a release. Reparenting the
  // pressed cell out of the grid in BeginDrag (driven by the long-press
  // timer, i.e. outside touch-event processing) makes the touch framework
  // deliver a spurious INTERRUPTED on the captured stream. Treating it as a
  // release would finish the drag the instant it starts. The capture keeps
  // delivering MOTION / UP, so a real release still arrives as UP.
  static bool IsUpState(PointState::Type state)
  {
    return state == PointState::UP ||
           state == PointState::FINISHED ||
           state == PointState::LEAVE;
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

  int32_t FindItemIndex(View cell) const
  {
    for(uint32_t i = 0u; i < mGridItems.size(); ++i)
    {
      if(mGridItems[i] == cell)
      {
        return static_cast<int32_t>(i);
      }
    }
    return -1;
  }

  // X / Y of mGrid's top-left in mRoot-local space. mGrid is the ScrollView
  // content, so the ScrollView's offset inside mOuterStack and mOuterStack's
  // offset inside mRoot are added; mGrid.POSITION already folds in the scroll
  // offset (ScrollView scrolls by writing its content POSITION). This mirrors
  // the list reorder test's GetStackWorldX/Y.
  float GetGridWorldX() const
  {
    return mOuterStack.GetCurrentProperty<float>(Actor::Property::POSITION_X) +
           mScrollView.GetCurrentProperty<float>(Actor::Property::POSITION_X) +
           mGrid.GetCurrentProperty<float>(Actor::Property::POSITION_X);
  }

  float GetGridWorldY() const
  {
    return mOuterStack.GetCurrentProperty<float>(Actor::Property::POSITION_Y) +
           mScrollView.GetCurrentProperty<float>(Actor::Property::POSITION_Y) +
           mGrid.GetCurrentProperty<float>(Actor::Property::POSITION_Y);
  }

  // Cell bounds in mRoot-local space.
  LayoutRect GetItemRootBounds(View cell) const
  {
    LayoutRect bounds;
    bounds.x      = GetGridWorldX() + cell.GetCurrentProperty<float>(Actor::Property::POSITION_X);
    bounds.y      = GetGridWorldY() + cell.GetCurrentProperty<float>(Actor::Property::POSITION_Y);
    bounds.width  = cell.GetCurrentProperty<float>(Actor::Property::SIZE_WIDTH);
    bounds.height = cell.GetCurrentProperty<float>(Actor::Property::SIZE_HEIGHT);
    return bounds;
  }

  // Row-major target index for the dragged cell. The floating cell's centre
  // is mapped into mGrid-local space (by subtracting the grid origin) and
  // quantised to the grid cell it sits over, so a cell only changes slot
  // once the dragged centre has crossed more than half a cell past a neighbour
  // (i.e. exceeded the neighbour's position by half the cell size).
  uint32_t ComputeTargetIndex(const LayoutRect& bounds) const
  {
    const uint32_t count = static_cast<uint32_t>(mGridItems.size());
    if(count == 0u)
    {
      return 0u;
    }
    const uint32_t rows = (count + COLUMNS - 1u) / COLUMNS;

    const float centerX = bounds.x + bounds.width * 0.5f - GetGridWorldX();
    const float centerY = bounds.y + bounds.height * 0.5f - GetGridWorldY();

    int col = static_cast<int>(std::floor(centerX / CELL_W));
    int row = static_cast<int>(std::floor(centerY / CELL_H));
    col     = std::clamp(col, 0, static_cast<int>(COLUMNS) - 1);
    row     = std::clamp(row, 0, static_cast<int>(rows) - 1);

    const uint32_t index = static_cast<uint32_t>(row) * COLUMNS + static_cast<uint32_t>(col);
    return std::min(index, count - 1u);
  }

  // Precondition: no drag is active (only called from OnLongPressTick, which
  // guards with !mDragging).
  void BeginDrag(View cell, uint32_t itemIndex, const Vector2& rootPosition)
  {
    // Bounds of the pressed cell in mRoot-local space. The floating cell uses
    // these directly as its position and size, and the grab offset is the
    // finger relative to the cell's top-left in the same space — exactly how
    // the list reorder test floats its dragged child.
    const LayoutRect bounds = GetItemRootBounds(cell);

    mDraggedChild          = cell;
    mDraggedIndex          = itemIndex;
    mDragBounds            = bounds;
    mDragGrabOffset        = rootPosition - Vector2(bounds.x, bounds.y);
    mDraggedOriginalReqW   = cell.GetRequestedWidth();
    mDraggedOriginalReqH   = cell.GetRequestedHeight();
    mLastTouchRootPosition = rootPosition;
    mDragging              = true;

    // Swap the pressed cell for a same-sized invisible proxy with no
    // transition so the swap is visually silent; re-attach right after so the
    // in-drag reorders animate through the CHANGE slot.
    LayoutTransition savedTransition = mGrid.GetLayoutTransition();
    mGrid.SetLayoutTransition(LayoutTransition());
    mGrid.Remove(cell, RemovePolicy::IMMEDIATE);

    mDragProxy = View::New();
    mDragProxy.SetRequestedWidth(bounds.width);
    mDragProxy.SetRequestedHeight(bounds.height);
    mDragProxy.SetProperty(Actor::Property::OPACITY, 0.0f);
    mDragProxy.SetProperty(Actor::Property::SENSITIVE, false);
    mGridItems[itemIndex] = mDragProxy;
    mGrid.Add(mDragProxy);
    ApplyGridOrder();

    mGrid.SetLayoutTransition(savedTransition);

    // Float the cell at its press-time position.
    cell.SetRequestedWidth(mDragBounds.width);
    cell.SetRequestedHeight(mDragBounds.height);
    cell.SetRequestedX(mDragBounds.x);
    cell.SetRequestedY(mDragBounds.y);
    // The float stays INSIDE the test's own subtree instead of going to the
    // window: LayoutMode::STANDALONE excludes it from mRoot's placement and
    // positions it straight from RequestedX/Y in mRoot's coordinate space —
    // exactly the space mDragBounds is expressed in. Keeping it under mRoot
    // also means the launcher reclaims it if the user leaves the test case
    // mid-drag.
    cell.SetLayoutMode(LayoutMode::STANDALONE);
    mRoot.Add(cell);
    cell.RaiseToTop(LayoutOrderPolicy::PRESERVE);
    // Force the floating cell's layout pass so it lands at its requested
    // position immediately rather than after the next frame's pass.
    if(mWindow)
    {
      LayoutController::Get(mWindow).ProcessLayouts();
    }

    StartAutoScrollTimer();
  }

  void UpdateDrag(const Vector2& rootPosition)
  {
    if(!mDragging || !mDraggedChild)
    {
      return;
    }

    mLastTouchRootPosition = rootPosition;

    // Clamp the floating cell to the visible ScrollView viewport (Y) and to the
    // grid's column band (X) so it stays on screen.
    const float viewportTop    = mOuterStack.GetCurrentProperty<float>(Actor::Property::POSITION_Y) +
                                 mScrollView.GetCurrentProperty<float>(Actor::Property::POSITION_Y);
    const float viewportHeight = mScrollView.GetCurrentProperty<float>(Actor::Property::SIZE_HEIGHT);
    const float minY           = viewportTop;
    const float maxY           = std::max(minY, viewportTop + viewportHeight - mDragBounds.height);

    const float gridLeft  = GetGridWorldX();
    const float gridRight = gridLeft + static_cast<float>(COLUMNS) * CELL_W;
    const float minX      = gridLeft;
    const float maxX      = std::max(minX, gridRight - mDragBounds.width);

    mDragBounds.x = std::clamp(rootPosition.x - mDragGrabOffset.x, minX, maxX);
    mDragBounds.y = std::clamp(rootPosition.y - mDragGrabOffset.y, minY, maxY);

    mDraggedChild.SetRequestedX(mDragBounds.x);
    mDraggedChild.SetRequestedY(mDragBounds.y);

    const uint32_t targetIndex = ComputeTargetIndex(mDragBounds);
    if(targetIndex != mDraggedIndex)
    {
      MoveProxy(mDraggedIndex, targetIndex);
      mDraggedIndex = targetIndex;
      // Reassign every cell's Row/Column for the new order. SetLayoutParams
      // marks the grid dirty, but its InvalidateMeasure early-exits if the grid
      // was already dirty, so the reflow can be skipped. Drive a synchronous
      // layout pass right here to guarantee the cells move to their new cells;
      // the transition stays attached so the CHANGE slot animates the reflow.
      ApplyGridOrder();
      if(mWindow)
      {
        LayoutController::Get(mWindow).ProcessLayouts();
      }
    }
  }

  void FinishDrag()
  {
    if(!mDragging)
    {
      return;
    }

    StopAutoScrollTimer();

    View             droppedChild = mDraggedChild;
    View             proxyToRemove = mDragProxy;
    const uint32_t   targetIndex   = mDraggedIndex;
    const float      originalReqW  = mDraggedOriginalReqW;
    const float      originalReqH  = mDraggedOriginalReqH;
    const LayoutRect dragBounds    = mDragBounds;

    mDragging            = false;
    mDraggedChild        = View();
    mDragProxy           = View();
    mDraggedIndex        = 0u;
    mDraggedOriginalReqW = 0.0f;
    mDraggedOriginalReqH = 0.0f;
    mDragGrabOffset      = Vector2(0.0f, 0.0f);
    mDragBounds          = {};

    if(!droppedChild || !proxyToRemove)
    {
      return;
    }

    // Pre-bake the dropped cell's arranged bounds to the drop position
    // expressed in mGrid-local coordinates while it is still floating, so the
    // CHANGE animation starts from the on-screen drop position rather than
    // teleporting before sliding into the slot.
    const float dropGridX = dragBounds.x - GetGridWorldX();
    const float dropGridY = dragBounds.y - GetGridWorldY();
    droppedChild.SetRequestedX(dropGridX);
    droppedChild.SetRequestedY(dropGridY);
    if(mWindow)
    {
      LayoutController::Get(mWindow).ProcessLayouts();
    }

    // Mirror BeginDrag's swap: detach the transition so removing the proxy and
    // re-inserting the dropped cell is silent, then re-attach so the next
    // layout pass dispatches CHANGE from the pre-baked snapshot.
    LayoutTransition savedTransition = mGrid.GetLayoutTransition();
    mGrid.SetLayoutTransition(LayoutTransition());

    mGrid.Remove(proxyToRemove, RemovePolicy::IMMEDIATE);
    if(droppedChild.GetParent())
    {
      droppedChild.Unparent();
    }
    // Give the cell back to the parent's layout: BeginDrag switched it to
    // LayoutMode::STANDALONE so it could float from RequestedX/Y, and the
    // grid must own its placement again.
    droppedChild.SetLayoutMode(LayoutMode::DEFAULT);
    droppedChild.SetRequestedWidth(originalReqW);
    droppedChild.SetRequestedHeight(originalReqH);
    droppedChild.SetRequestedX(0.0f);
    droppedChild.SetRequestedY(0.0f);
    mGridItems[targetIndex] = droppedChild;
    mGrid.Add(droppedChild);
    ApplyGridOrder();

    mGrid.SetLayoutTransition(savedTransition);
  }

  void MoveProxy(uint32_t from, uint32_t to)
  {
    if(from == to || from >= mGridItems.size() || to >= mGridItems.size())
    {
      return;
    }
    View proxy = mGridItems[from];
    mGridItems.erase(mGridItems.begin() + from);
    mGridItems.insert(mGridItems.begin() + to, proxy);
  }

  // ── Long press ─────────────────────────────────────────────────────────────

  void StartLongPressTimer()
  {
    if(!mLongPressTimer)
    {
      mLongPressTimer = Timer::New(LONG_PRESS_MS);
      mLongPressTimer.TickSignal().Connect(this, &TcLayoutTransitionGridReorder::OnLongPressTick);
    }
    mLongPressTimer.SetInterval(LONG_PRESS_MS);
    mLongPressTimer.Start();
  }

  void CancelLongPress()
  {
    mLongPressArmed = false;
    mPressedItem    = View();
    if(mLongPressTimer && mLongPressTimer.IsRunning())
    {
      mLongPressTimer.Stop();
    }
  }

  bool OnLongPressTick()
  {
    if(mLongPressArmed && mPressedItem && !mDragging)
    {
      View     cell  = mPressedItem;
      uint32_t index = mPressIndex;
      mLongPressArmed = false;
      mPressedItem    = View();
      BeginDrag(cell, index, mLastTouchRootPosition);
    }
    return false; // one-shot
  }

  // ── Auto-scroll while dragging near ScrollView edges ────────────────────────

  void StartAutoScrollTimer()
  {
    if(!mAutoScrollTimer)
    {
      mAutoScrollTimer = Timer::New(AUTO_SCROLL_TICK_MS);
      mAutoScrollTimer.TickSignal().Connect(this, &TcLayoutTransitionGridReorder::OnAutoScrollTick);
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

    const float viewportTop    = mOuterStack.GetCurrentProperty<float>(Actor::Property::POSITION_Y) +
                                 mScrollView.GetCurrentProperty<float>(Actor::Property::POSITION_Y);
    const float viewportHeight = mScrollView.GetCurrentProperty<float>(Actor::Property::SIZE_HEIGHT);
    const float viewportBottom = viewportTop + viewportHeight;
    const float touchY         = mLastTouchRootPosition.y;

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
      mScrollView.ScrollTo(Vector2(current.x, current.y + scrollStepY), false);
      // Finger has not moved but content has, so re-evaluate the proxy slot.
      UpdateDrag(mLastTouchRootPosition);
    }

    return true;
  }

  Window            mWindow;     ///< Read-only handle needed by LayoutController::Get
  AbsoluteLayout    mRoot;       ///< White root; doubles as this test's backdrop
  StackLayout       mOuterStack;
  ScrollView        mScrollView;
  GridLayout        mGrid;
  std::vector<View> mGridItems; ///< Logical order of grid cells (proxy swapped in during a drag)

  bool       mDragging = false;
  View       mDraggedChild;          ///< The dragged cell, floating under mRoot during a drag
  View       mDragProxy;             ///< Invisible slot reserving the dragged cell's place in the grid
  uint32_t   mDraggedIndex = 0u;     ///< Current index of mDragProxy in mGridItems
  LayoutRect mDragBounds{};          ///< Dragged cell's mRoot-local bounds (driven by the finger)
  Vector2    mDragGrabOffset{0.0f, 0.0f};
  float      mDraggedOriginalReqW = 0.0f; ///< Restored to mDraggedChild on FinishDrag
  float      mDraggedOriginalReqH = 0.0f; ///< Restored to mDraggedChild on FinishDrag
  Vector2    mLastTouchRootPosition{0.0f, 0.0f};

  Timer    mAutoScrollTimer; ///< Fires while a drag is in flight to apply edge-zone auto-scroll
  Timer    mLongPressTimer;  ///< Fires once after a hold to start a drag
  bool     mLongPressArmed = false;
  View     mPressedItem;
  uint32_t mPressIndex = 0u;
  Vector2  mPressRootPosition{0.0f, 0.0f};
};

REGISTER_MANUAL_TEST(TcLayoutTransitionGridReorder)
