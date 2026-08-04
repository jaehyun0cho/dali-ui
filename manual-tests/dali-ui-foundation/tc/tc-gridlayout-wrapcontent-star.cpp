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
 * @brief Verifies that a Star column absorbs the space created by minWidth.
 *
 * The root GridLayout wraps its content but carries a 400px minimum width
 * and a fixed 200px height.  Its columns are [Auto, Star(1)]: the Auto
 * column is driven to 200px by child 1, so the Star column must receive the
 * remaining 400 - 200 = 200px.  The nested GridLayout inside the Star
 * column splits that space between two Star(1) grandchildren.
 *
 * Ported from samples/gridlayout/gridlayout-wrapcontent-star-example.cpp.
 */
class TcGridLayoutWrapContentStar : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "GridLayout: WrapContent MinSize + Star Column";
  }

  Dali::String GetDescription() const override
  {
    return "Star column absorbs the space created by minWidth on a wrapping grid";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Columns: AUTO, 1*STAR.  Row: 1*STAR.
    GridLayout root = GridLayout::New();
    root.SetBackgroundColor(Color::GAINSBORO);
    root.SetMinimumWidth(400.0f);
    root.SetRequestedHeight(200.0f);
    root.AddColumnDefinition(GridLength::Auto());
    root.AddColumnDefinition(GridLength::Star(1.0f));
    root.AddRowDefinition(GridLength::Star(1.0f));

    // Child 1: col 0 (AUTO) — fixed 200 wide, drives the AUTO column size.
    View child1 = View::New();
    child1.SetBackgroundColor(Color::RED);
    child1.SetRequestedWidth(200.0f);
    child1.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(0));
    root.Add(child1);

    // Child 2: col 1 (STAR) — GridLayout that fills the remaining 200px (400 - 200).
    // Contains two grandchildren in 1*STAR columns, each getting 100px.
    GridLayout child2 = GridLayout::New();
    child2.SetBackgroundColor(Color::GREEN);
    child2.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(1));
    child2.AddColumnDefinition(GridLength::Star(1.0f));
    child2.AddColumnDefinition(GridLength::Star(1.0f));
    child2.AddRowDefinition(GridLength::Star(1.0f));
    root.Add(child2);

    View grandChild1 = View::New();
    grandChild1.SetBackgroundColor(Color::BLUE);
    grandChild1.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(0));
    child2.Add(grandChild1);

    View grandChild2 = View::New();
    grandChild2.SetBackgroundColor(Color::YELLOW);
    grandChild2.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(1));
    child2.Add(grandChild2);

    mBackdrop.Add(root);
    contentArea.Add(mBackdrop);
  }

  void OnExit() override
  {
    // Release every member handle so the previous subtree is not retained.
    mBackdrop.Reset();
    // Restore scalar members to their initial values.
    ResetState();
  }

private:
  void ResetState()
  {
    // This test case holds no mutable state.
  }

  View mBackdrop;
};

REGISTER_MANUAL_TEST(TcGridLayoutWrapContentStar)
