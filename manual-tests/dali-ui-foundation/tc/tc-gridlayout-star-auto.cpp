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
 * @brief Verifies proportional GridLength::Star track sizing.
 *
 * Rows are [Absolute(100), Star(1), Absolute(50)] and columns are
 * [Absolute(100), Star(2), Star(1)].  The Star(2) column must receive
 * twice the width of the Star(1) column once the fixed column is
 * subtracted, and the header/footer cells span all three columns.
 *
 * Ported from samples/gridlayout/gridlayout-star-auto-example.cpp.
 */
class TcGridLayoutStarAuto : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "GridLayout: Star Sizing + Span";
  }

  Dali::String GetDescription() const override
  {
    return "Star(2) column takes twice the width of Star(1); header/footer span all columns";
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

    // Rows: fixed header, flexible middle, fixed footer
    root.AddRowDefinition(GridLength::Absolute(100.0f));
    root.AddRowDefinition(GridLength::Star(1.0f));
    root.AddRowDefinition(GridLength::Absolute(50.0f));

    // Columns: fixed sidebar, Star(2) content, Star(1) panel
    root.AddColumnDefinition(GridLength::Absolute(100.0f));
    root.AddColumnDefinition(GridLength::Star(2.0f));
    root.AddColumnDefinition(GridLength::Star(1.0f));

    // Header: spans all 3 columns
    View header = View::New();
    header.SetBackgroundColor(Color::RED);
    header.SetLayoutParams(GridLayoutParams::New().SetColumnSpan(3));
    root.Add(header);

    // Sidebar: row 1, column 0
    View sidebar = View::New();
    sidebar.SetBackgroundColor(Color::GREEN);
    sidebar.SetLayoutParams(GridLayoutParams::New().SetRow(1));
    root.Add(sidebar);

    // Content: row 1, column 1 (Star(2) - wider)
    View content = View::New();
    content.SetBackgroundColor(Color::BLUE);
    content.SetLayoutParams(GridLayoutParams::New().SetRow(1).SetColumn(1));
    root.Add(content);

    // Panel: row 1, column 2 (Star(1) - narrower)
    View panel = View::New();
    panel.SetBackgroundColor(Color::YELLOW);
    panel.SetLayoutParams(GridLayoutParams::New().SetRow(1).SetColumn(2));
    root.Add(panel);

    // Footer: spans all 3 columns
    View footer = View::New();
    footer.SetBackgroundColor(Color::CYAN);
    footer.SetLayoutParams(GridLayoutParams::New().SetRow(2).SetColumnSpan(3));
    root.Add(footer);

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

REGISTER_MANUAL_TEST(TcGridLayoutStarAuto)
