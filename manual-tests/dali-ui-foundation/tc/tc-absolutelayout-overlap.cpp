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
 * @brief Verifies that AbsoluteLayout children overlap in add order.
 *
 * Children are rendered in the order they are added (later children
 * are drawn on top of earlier ones).
 *
 * - Large red background box
 * - Medium green box overlapping the red box
 * - Small blue box overlapping both
 * - Cyan box using proportional position centered in the layout
 *
 * Ported from samples/absolutelayout/absolutelayout-overlap-example.cpp.
 */
class TcAbsoluteLayoutOverlap : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "AbsoluteLayout: Overlap / Z-order";
  }

  Dali::String GetDescription() const override
  {
    return "Overlapping children draw in add order";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    AbsoluteLayout root = AbsoluteLayout::New();
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    // Large red background box
    View redBox = View::New();
    redBox.SetBackgroundColor(Color::RED);
    redBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(50.0f, 50.0f, 400.0f, 400.0f)));
    root.Add(redBox);

    // Medium green box overlapping the red box
    View greenBox = View::New();
    greenBox.SetBackgroundColor(Color::GREEN);
    greenBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(100.0f, 100.0f, 200.0f, 200.0f)));
    root.Add(greenBox);

    // Small blue box overlapping both
    View blueBox = View::New();
    blueBox.SetBackgroundColor(Color::BLUE);
    blueBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(200.0f, 200.0f, 100.0f, 100.0f)));
    root.Add(blueBox);

    // Cyan box: proportional position centered, absolute size
    View cyanBox = View::New();
    cyanBox.SetBackgroundColor(Color::CYAN);
    cyanBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(0.5f, 0.5f, 50.0f, 50.0f))
      .SetFlags(AbsoluteLayoutFlags::POSITION_PROPORTIONAL));
    root.Add(cyanBox);

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

REGISTER_MANUAL_TEST(TcAbsoluteLayoutOverlap)
