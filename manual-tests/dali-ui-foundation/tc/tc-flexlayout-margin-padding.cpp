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
 * @brief Verifies margin and padding on a FlexLayout and on its children.
 *
 * 1. Padding: root FlexLayout (COLUMN) has 50px padding (content inset from the edges).
 * 2. Margin: children alternate between no margin and 50px uniform margin.
 *    - Red box: no margin (flush with padding edge).
 *    - Green box: 50px margin all sides.
 *    - Blue box: no margin.
 * 3. Nested: a horizontal FlexLayout row with 50px padding and 50px margin,
 *    containing children that alternate no margin / 50px margin.
 *
 * Ported from samples/flexlayout/flexlayout-margin-padding-example.cpp.
 */
class TcFlexLayoutMarginPadding : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "FlexLayout: Margin / Padding";
  }

  Dali::String GetDescription() const override
  {
    return "COLUMN container padding, per-child margin and a nested ROW";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: vertical flex layout with padding
    FlexLayout root = FlexLayout::New();
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetDirection(FlexDirection::COLUMN);
    root.SetPadding(Extents(50, 50, 50, 50)); // start, end, top, bottom

    // --- Red box: no margin (flush with padding edge) ---
    View redBox = View::New();
    redBox.SetBackgroundColor(Color::RED);
    redBox.SetRequestedHeight(50.0f);
    root.Add(redBox);

    // --- Green box: 50px margin all sides ---
    View greenBox = View::New();
    greenBox.SetBackgroundColor(Color::GREEN);
    greenBox.SetRequestedHeight(50.0f);
    greenBox.SetMargin(Extents(50, 50, 50, 50));
    root.Add(greenBox);

    // --- Blue box: no margin ---
    View blueBox = View::New();
    blueBox.SetBackgroundColor(Color::BLUE);
    blueBox.SetRequestedHeight(50.0f);
    root.Add(blueBox);

    // --- Nested: horizontal FlexLayout with padding + children with margins ---
    FlexLayout nestedRow = FlexLayout::New();
    nestedRow.SetBackgroundColor(Color::GRAY);
    nestedRow.SetRequestedHeight(400.0f);
    nestedRow.SetDirection(FlexDirection::ROW);
    nestedRow.SetAlignItems(FlexAlign::STRETCH);
    nestedRow.SetPadding(Extents(50, 50, 50, 50));
    nestedRow.SetMargin(Extents(50, 50, 50, 50));

    View childA = View::New();
    childA.SetBackgroundColor(Color::MAGENTA);
    childA.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f));
    nestedRow.Add(childA);

    View childB = View::New();
    childB.SetBackgroundColor(Color::YELLOW);
    childB.SetMargin(Extents(50, 50, 50, 50));
    childB.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f));
    nestedRow.Add(childB);

    View childC = View::New();
    childC.SetBackgroundColor(Color::CYAN);
    childC.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f));
    nestedRow.Add(childC);

    root.Add(nestedRow);

    mBackdrop.Add(root);
    contentArea.Add(mBackdrop);
  }

  void OnExit() override
  {
    mBackdrop.Reset();
    ResetState();
  }

private:
  void ResetState()
  {
    // No mutable state beyond the view handles released in OnExit().
  }

  View mBackdrop;
};

REGISTER_MANUAL_TEST(TcFlexLayoutMarginPadding)
