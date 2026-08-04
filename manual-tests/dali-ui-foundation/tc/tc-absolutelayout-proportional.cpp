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
 * @brief Verifies proportional positioning and sizing in an AbsoluteLayout.
 *
 * Demonstrates the composite AbsoluteLayoutFlags values
 * (SIZE_PROPORTIONAL, ALL, POSITION_PROPORTIONAL) stacked in add order:
 *
 *   - Blue  : SIZE_PROPORTIONAL     bounds(  0,   0, 1.0, 1.0)
 *   - Red   : ALL                   bounds(0.5, 0.5, 0.5, 0.5)
 *   - Green : POSITION_PROPORTIONAL bounds(0.5, 0.5, 100, 100)
 *
 * Ported from samples/absolutelayout/absolutelayout-proportional-example.cpp.
 */
class TcAbsoluteLayoutProportional : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "AbsoluteLayout: Proportional Position / Size";
  }

  Dali::String GetDescription() const override
  {
    return "SIZE_PROPORTIONAL, ALL and POSITION_PROPORTIONAL flag combinations";
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

    // Blue box: SIZE_PROPORTIONAL fills the entire root.
    View blueBox = View::New();
    blueBox.SetBackgroundColor(Color::BLUE);
    blueBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(0.0f, 0.0f, 1.0f, 1.0f))
      .SetFlags(AbsoluteLayoutFlags::SIZE_PROPORTIONAL));
    root.Add(blueBox);

    // Red box: ALL centers a half-size child.
    View redBox = View::New();
    redBox.SetBackgroundColor(Color::RED);
    redBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(0.5f, 0.5f, 0.5f, 0.5f))
      .SetFlags(AbsoluteLayoutFlags::ALL));
    root.Add(redBox);

    // Green box: POSITION_PROPORTIONAL centers a fixed 100x100 box.
    View greenBox = View::New();
    greenBox.SetBackgroundColor(Color::GREEN);
    greenBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(0.5f, 0.5f, 100.0f, 100.0f))
      .SetFlags(AbsoluteLayoutFlags::POSITION_PROPORTIONAL));
    root.Add(greenBox);

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

REGISTER_MANUAL_TEST(TcAbsoluteLayoutProportional)
