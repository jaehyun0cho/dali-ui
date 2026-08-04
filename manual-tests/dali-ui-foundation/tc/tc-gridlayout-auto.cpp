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
 * @brief Verifies GridLength::Auto sizing mixed with GridLength::Star.
 *
 * Rows are [Auto, Star(1), Auto] and columns are [Auto, Star(1)], declared
 * through the SetRowDefinitions / SetColumnDefinitions batch API.  Auto
 * tracks size to their content while the Star track absorbs the remaining
 * space.
 *
 * Ported from samples/gridlayout/gridlayout-auto-example.cpp.
 */
class TcGridLayoutAuto : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "GridLayout: Auto Sizing + Batch Definitions";
  }

  Dali::String GetDescription() const override
  {
    return "GridLength::Auto mixed with Star, set via SetRowDefinitions / SetColumnDefinitions";
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

    // Use batch API: SetRowDefinitions / SetColumnDefinitions
    Dali::Vector<GridLength> rowDefinition;
    rowDefinition.Reserve(3);
    rowDefinition.PushBack(GridLength::Auto());
    rowDefinition.PushBack(GridLength::Star(1.0f));
    rowDefinition.PushBack(GridLength::Auto());
    root.SetRowDefinitions(rowDefinition);

    Dali::Vector<GridLength> columnDefinition;
    columnDefinition.Reserve(2);
    columnDefinition.PushBack(GridLength::Auto());
    columnDefinition.PushBack(GridLength::Star(1.0f));
    root.SetColumnDefinitions(columnDefinition);

    // (0,0): Auto-width label - narrow fixed-size box simulating a label
    View label00 = View::New();
    label00.SetBackgroundColor(Color::RED);
    label00.SetRequestedWidth(100.0f);
    label00.SetRequestedHeight(50.0f);
    label00.SetLayoutParams(GridLayoutParams::New());
    root.Add(label00);

    // (0,1): Header area in Auto row, Star column
    View header = View::New();
    header.SetBackgroundColor(Vector4(1.0f, 0.6f, 0.6f, 1.0f));
    header.SetRequestedHeight(50.0f);
    header.SetLayoutParams(GridLayoutParams::New().SetColumn(1));
    root.Add(header);

    // (1,0): Sidebar in Auto column, Star row
    View sidebar = View::New();
    sidebar.SetBackgroundColor(Color::GREEN);
    sidebar.SetRequestedWidth(100.0f);
    sidebar.SetLayoutParams(GridLayoutParams::New().SetRow(1));
    root.Add(sidebar);

    // (1,1): Main content area (Star row x Star column)
    View content = View::New();
    content.SetBackgroundColor(Color::BLUE);
    content.SetLayoutParams(GridLayoutParams::New().SetRow(1).SetColumn(1));
    root.Add(content);

    // (2,0): Footer label in Auto row, Auto column
    View label20 = View::New();
    label20.SetBackgroundColor(Color::YELLOW);
    label20.SetRequestedWidth(100.0f);
    label20.SetRequestedHeight(50.0f);
    label20.SetLayoutParams(GridLayoutParams::New().SetRow(2));
    root.Add(label20);

    // (2,1): Footer content spanning the Star column
    View footer = View::New();
    footer.SetBackgroundColor(Color::CYAN);
    footer.SetRequestedHeight(50.0f);
    footer.SetLayoutParams(GridLayoutParams::New().SetRow(2).SetColumn(1));
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

REGISTER_MANUAL_TEST(TcGridLayoutAuto)
