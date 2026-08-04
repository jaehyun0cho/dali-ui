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
 * @brief Verifies GridLayout container padding and per-cell margin.
 *
 * The grey root GridLayout has 50px padding on all sides and both row and
 * column spacing set to 0, so only the per-cell margin creates gaps.  Cells
 * alternate between no margin and a uniform 50px margin, and the last cell
 * spans two columns while still applying its own margin.
 *
 * Ported from samples/gridlayout/gridlayout-margin-padding-example.cpp.
 */
class TcGridLayoutMarginPadding : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "GridLayout: Margin / Padding";
  }

  Dali::String GetDescription() const override
  {
    return "Grid padding with per-cell margin and zero spacing";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: GridLayout with padding
    GridLayout root = GridLayout::New();
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetBackgroundColor(Color::GRAY);
    root.SetPadding(Extents(50, 50, 50, 50)); // start, end, top, bottom
    root.SetRowSpacing(0.0f);
    root.SetColumnSpacing(0.0f);

    // 3 rows x 2 columns, no spacing (margin only)
    root.AddRowDefinition(GridLength::Absolute(200.0f));
    root.AddRowDefinition(GridLength::Absolute(200.0f));
    root.AddRowDefinition(GridLength::Absolute(200.0f));
    root.AddColumnDefinition(GridLength::Star(1.0f));
    root.AddColumnDefinition(GridLength::Star(1.0f));

    // Cell (0,0): Red - no margin (flush with grid padding edge)
    View cell00 = View::New();
    cell00.SetBackgroundColor(Color::RED);
    cell00.SetLayoutParams(GridLayoutParams::New());
    root.Add(cell00);

    // Cell (0,1): Green - 50px margin all sides
    View cell01 = View::New();
    cell01.SetBackgroundColor(Color::GREEN);
    cell01.SetMargin(Extents(50, 50, 50, 50));
    cell01.SetLayoutParams(GridLayoutParams::New().SetColumn(1));
    root.Add(cell01);

    // Cell (1,0): Blue - 50px margin all sides
    View cell10 = View::New();
    cell10.SetBackgroundColor(Color::BLUE);
    cell10.SetMargin(Extents(50, 50, 50, 50));
    cell10.SetLayoutParams(GridLayoutParams::New().SetRow(1));
    root.Add(cell10);

    // Cell (1,1): Yellow - no margin (fills cell completely)
    View cell11 = View::New();
    cell11.SetBackgroundColor(Color::YELLOW);
    cell11.SetLayoutParams(GridLayoutParams::New().SetRow(1).SetColumn(1));
    root.Add(cell11);

    // Cell (2,0~1): Cyan - spanning 2 columns with 50px margin
    View cell20 = View::New();
    cell20.SetBackgroundColor(Color::CYAN);
    cell20.SetMargin(Extents(50, 50, 50, 50));
    cell20.SetLayoutParams(GridLayoutParams::New().SetRow(2).SetColumnSpan(2));
    root.Add(cell20);

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

REGISTER_MANUAL_TEST(TcGridLayoutMarginPadding)
