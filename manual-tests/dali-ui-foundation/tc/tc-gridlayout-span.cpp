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
 * @brief Verifies GridLayoutParams RowSpan and ColumnSpan.
 *
 * A 3x3 grid whose rows and columns are all GridLength::Star(1) hosts five
 * cells.  Red spans 2 columns, Yellow spans 2 rows and 2 columns, and the
 * remaining cells occupy a single cell each, so the spanning cells must
 * cover the merged track areas including the spacing between them.
 *
 * Ported from samples/gridlayout/gridlayout-span-example.cpp.
 */
class TcGridLayoutSpan : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "GridLayout: RowSpan / ColumnSpan";
  }

  Dali::String GetDescription() const override
  {
    return "Cells spanning multiple rows and columns in a 3x3 Star grid";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    GridLayout root = GridLayout::New();
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetPadding(Extents(50, 50, 50, 50));
    root.SetRowSpacing(10.0f);
    root.SetColumnSpacing(10.0f);

    // 3 rows x 3 columns, all proportional
    root.AddRowDefinition(GridLength::Star(1.0f));
    root.AddRowDefinition(GridLength::Star(1.0f));
    root.AddRowDefinition(GridLength::Star(1.0f));
    root.AddColumnDefinition(GridLength::Star(1.0f));
    root.AddColumnDefinition(GridLength::Star(1.0f));
    root.AddColumnDefinition(GridLength::Star(1.0f));

    // Red: row 0, column 0, spans 2 columns
    View red = View::New();
    red.SetBackgroundColor(Color::RED);
    red.SetLayoutParams(GridLayoutParams::New().SetColumnSpan(2));
    root.Add(red);

    // Green: row 0, column 2
    View green = View::New();
    green.SetBackgroundColor(Color::GREEN);
    green.SetLayoutParams(GridLayoutParams::New().SetColumn(2));
    root.Add(green);

    // Blue: row 1, column 0
    View blue = View::New();
    blue.SetBackgroundColor(Color::BLUE);
    blue.SetLayoutParams(GridLayoutParams::New().SetRow(1));
    root.Add(blue);

    // Yellow: row 1, column 1, spans 2 columns and 2 rows
    View yellow = View::New();
    yellow.SetBackgroundColor(Color::YELLOW);
    yellow.SetLayoutParams(GridLayoutParams::New().SetRow(1).SetColumn(1).SetColumnSpan(2).SetRowSpan(2));
    root.Add(yellow);

    // Cyan: row 2, column 0
    View cyan = View::New();
    cyan.SetBackgroundColor(Color::CYAN);
    cyan.SetLayoutParams(GridLayoutParams::New().SetRow(2));
    root.Add(cyan);

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

REGISTER_MANUAL_TEST(TcGridLayoutSpan)
