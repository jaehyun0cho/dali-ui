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
 * @brief Verifies StackLayout margin, padding and nesting.
 *
 * 1. Padding: the root stack has SetPadding so content is inset from the edges.
 * 2. Margin: child views use SetMargin (different values) so gaps are visible.
 * 3. Nested: one row is a horizontal StackLayout (row of boxes) inside the
 *    vertical stack.
 *
 * Ported from samples/stacklayout/stacklayout-margin-padding-nested-example.cpp.
 */
class TcStackLayoutMarginPaddingNested : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "StackLayout: Margin / Padding / Nested";
  }

  Dali::String GetDescription() const override
  {
    return "Root padding, per-child margins and a nested horizontal row";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: vertical stack with padding (content inset from the content area edges)
    StackLayout root = StackLayout::New(StackOrientation::VERTICAL);
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetPadding(Extents(50, 50, 50, 50)); // start, end, top, bottom

    // --- Section 1: Padding (visible as gap around the red bar) ---
    View padLabel = View::New();
    padLabel.SetBackgroundColor(Color::RED);
    padLabel.SetRequestedHeight(50.0f);
    padLabel.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
    root.Add(padLabel);

    // --- Section 2: Margin (children with different margins) ---
    View marginA = View::New();
    marginA.SetBackgroundColor(Color::GREEN);
    marginA.SetRequestedHeight(50.0f);
    marginA.SetMargin(Extents(50, 50, 0, 0));
    marginA.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
    root.Add(marginA);

    View marginB = View::New();
    marginB.SetBackgroundColor(Color::BLUE);
    marginB.SetRequestedHeight(50.0f);
    marginB.SetMargin(Extents(0, 0, 50, 50)); // top and bottom margin only
    marginB.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
    root.Add(marginB);

    View marginC = View::New();
    marginC.SetBackgroundColor(Color::CYAN);
    marginC.SetRequestedHeight(50.0f);
    marginC.SetMargin(Extents(50, 50, 50, 50));
    marginC.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
    root.Add(marginC);

    // --- Section 3: Nested StackLayout (horizontal stack inside vertical) ---
    StackLayout horizontalRow = StackLayout::New(StackOrientation::HORIZONTAL);
    horizontalRow.SetBackgroundColor(Color::GRAY);
    horizontalRow.SetSpacing(10.0f);
    horizontalRow.SetMargin(Extents(50, 50, 50, 50));
    horizontalRow.SetLayoutParams(StackLayoutParams::New()
                                    .SetAlignment(LayoutAlignment::FILL)
                                    .SetWeight(1.0f));

    View box1 = View::New();
    box1.SetBackgroundColor(Color::MAGENTA);
    box1.SetLayoutParams(StackLayoutParams::New()
      .SetAlignment(LayoutAlignment::FILL)
      .SetWeight(1.0f));
    horizontalRow.Add(box1);

    View box2 = View::New();
    box2.SetBackgroundColor(Color::YELLOW);
    box2.SetRequestedHeight(50.0f);
    box2.SetLayoutParams(StackLayoutParams::New()
      .SetAlignment(LayoutAlignment::START)
      .SetWeight(1.0f));
    horizontalRow.Add(box2);

    View box3 = View::New();
    box3.SetBackgroundColor(Color::RED);
    box3.SetRequestedHeight(50.0f);
    box3.SetLayoutParams(StackLayoutParams::New()
      .SetAlignment(LayoutAlignment::CENTER)
      .SetWeight(1.0f));
    horizontalRow.Add(box3);

    View box4 = View::New();
    box4.SetBackgroundColor(Color::GREEN);
    box4.SetRequestedHeight(50.0f);
    box4.SetLayoutParams(StackLayoutParams::New()
      .SetAlignment(LayoutAlignment::END)
      .SetWeight(1.0f));
    horizontalRow.Add(box4);

    root.Add(horizontalRow);

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

REGISTER_MANUAL_TEST(TcStackLayoutMarginPaddingNested)
