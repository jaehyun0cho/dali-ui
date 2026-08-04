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
 * @brief Verifies AbsoluteLayout margin and padding handling.
 *
 * 1. Padding: root AbsoluteLayout has 50px padding (children inset from the edges).
 * 2. Margin: children alternate between no margin and 50px uniform margin.
 *    - Red box: no margin (flush with padding edge).
 *    - Green box: 50px margin all sides.
 *    - Blue box: 50px margin all sides.
 * 3. Nested: a child AbsoluteLayout with its own padding, containing inner boxes
 *    with no margin and 50px margin respectively.
 *
 * Ported from samples/absolutelayout/absolutelayout-margin-padding-example.cpp.
 */
class TcAbsoluteLayoutMarginPadding : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "AbsoluteLayout: Margin / Padding";
  }

  Dali::String GetDescription() const override
  {
    return "Container padding, per-child margin and a nested AbsoluteLayout";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: AbsoluteLayout with padding (content inset from the edges)
    AbsoluteLayout root = AbsoluteLayout::New();
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetPadding(Extents(50, 50, 50, 50)); // start, end, top, bottom

    // --- Red box: no margin (positioned at padding edge) ---
    View redBox = View::New();
    redBox.SetBackgroundColor(Color::RED);
    redBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetWidth(100.0f).SetHeight(100.0f));
    root.Add(redBox);

    // --- Green box: 50px margin all sides ---
    View greenBox = View::New();
    greenBox.SetBackgroundColor(Color::GREEN);
    greenBox.SetMargin(Extents(50, 50, 50, 50));
    greenBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetY(100.0f).SetWidth(100.0f).SetHeight(50.0f));
    root.Add(greenBox);

    // --- Blue box: 50px margin all sides ---
    View blueBox = View::New();
    blueBox.SetBackgroundColor(Color::BLUE);
    blueBox.SetMargin(Extents(50, 50, 50, 50));
    blueBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetX(100.0f).SetY(200.0f).SetWidth(100.0f).SetHeight(50.0f));
    root.Add(blueBox);

    // --- Nested AbsoluteLayout with its own padding ---
    AbsoluteLayout nested = AbsoluteLayout::New();
    nested.SetBackgroundColor(Color::GRAY);
    nested.SetPadding(Extents(50, 50, 50, 50));
    nested.SetMargin(Extents(50, 50, 50, 50));
    nested.SetLayoutParams(AbsoluteLayoutParams::New()
                             .SetY(300.0f).SetWidth(200.0f).SetHeight(200.0f));

    View innerA = View::New();
    innerA.SetBackgroundColor(Color::MAGENTA);
    innerA.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(0.0f, 0.0f, 1.0f, 1.0f))
      .SetFlags(AbsoluteLayoutFlags::ALL));
    nested.Add(innerA);

    View innerB = View::New();
    innerB.SetBackgroundColor(Color::YELLOW);
    innerB.SetMargin(Extents(50, 50, 50, 50));
    innerB.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetX(50.0f).SetWidth(50.0f).SetHeight(50.0f));
    nested.Add(innerB);

    View innerC = View::New();
    innerC.SetBackgroundColor(Color::CYAN);
    innerC.SetMargin(Extents(50, 50, 50, 50));
    innerC.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetY(50.0f).SetWidth(50.0f).SetHeight(50.0f));
    nested.Add(innerC);

    root.Add(nested);

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
    // No mutable scalar state in this test case.
  }

  View mBackdrop;
};

REGISTER_MANUAL_TEST(TcAbsoluteLayoutMarginPadding)
