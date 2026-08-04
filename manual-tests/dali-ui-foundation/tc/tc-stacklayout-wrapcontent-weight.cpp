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

using namespace Dali;
using namespace Dali::Ui;

/**
 * @brief Verifies MATCH_PARENT children inside a minimum-width horizontal stack.
 *
 *   - The root is a horizontal StackLayout with a minimum width of 400 and a
 *     requested height of 200.
 *   - Child 1 has a fixed width of 200 and MATCH_PARENT height, so it fills
 *     the parent's cross axis (200px).
 *   - Child 2 has weight 1 and MATCH_PARENT height.  On the main axis (width)
 *     it fills the remaining space via weight (400 - 200 = 200).  On the cross
 *     axis (height) it fills the parent height (200).  Its two grand children
 *     repeat the same weight / MATCH_PARENT combination one level down.
 *
 * Naming caveat: the source sample file is called
 * stacklayout-wrapcontent-weight-example.cpp, but its class is
 * StackLayoutMatchParentController and its body demonstrates
 * SetMinimumWidth(400) combined with MATCH_PARENT, not WRAP_CONTENT.  The
 * test-case file name keeps the sample stem for traceability while the
 * displayed name and description describe what the code actually does.
 *
 * Ported from samples/stacklayout/stacklayout-wrapcontent-weight-example.cpp.
 */
class TcStackLayoutWrapContentWeight : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "StackLayout: MinWidth + MatchParent Weight";
  }

  Dali::String GetDescription() const override
  {
    return "MATCH_PARENT cross-axis with weight on a minWidth-400 horizontal stack";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: Horizontal StackLayout, minWidth=400, height=200.
    StackLayout root = StackLayout::New(StackOrientation::HORIZONTAL);
    root.SetBackgroundColor(Color::GAINSBORO);
    root.SetMinimumWidth(400.0f);
    root.SetRequestedHeight(200.0f);

    // Child 1: fixed width 200, MATCH_PARENT height (fills cross-axis = 200).
    View child1 = View::New();
    child1.SetBackgroundColor(Color::RED);
    child1.SetRequestedWidth(200.0f);
    child1.SetRequestedHeight(MATCH_PARENT);
    root.Add(child1);

    // Child 2: MATCH_PARENT width and height.
    // Main axis (width): fills the full available width.
    // Cross axis (height): fills the parent height (200).
    StackLayout child2 = StackLayout::New(StackOrientation::HORIZONTAL);
    child2.SetBackgroundColor(Color::GREEN);
    child2.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f));
    child2.SetRequestedHeight(MATCH_PARENT);
    root.Add(child2);

    View grandChild1 = View::New();
    grandChild1.SetBackgroundColor(Color::BLUE);
    grandChild1.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f));
    grandChild1.SetRequestedHeight(MATCH_PARENT);
    child2.Add(grandChild1);

    View grandChild2 = View::New();
    grandChild2.SetBackgroundColor(Color::YELLOW);
    grandChild2.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f));
    grandChild2.SetRequestedHeight(MATCH_PARENT);
    child2.Add(grandChild2);

    mBackdrop.Add(root);
    contentArea.Add(mBackdrop);
  }

  void OnExit() override
  {
    // No in-flight interaction, timer, animation or externally connected
    // signal exists in this test case, so only the member handles have to
    // be released before the subtree is discarded.
    ResetState();
  }

private:
  void ResetState()
  {
    mBackdrop.Reset();
  }

  View mBackdrop;
};

REGISTER_MANUAL_TEST(TcStackLayoutWrapContentWeight)
