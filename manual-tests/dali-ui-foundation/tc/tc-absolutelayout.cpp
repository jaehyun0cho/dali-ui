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
 * @brief Verifies absolute pixel positioning inside an AbsoluteLayout.
 *
 * Children are placed at explicit (x, y) coordinates with explicit sizes.
 * - Red box at top-left (50, 50)
 * - Green box at center area (100, 100)
 * - Blue box at bottom-right area (200, 200)
 *
 * Ported from samples/absolutelayout/absolutelayout-example.cpp.
 */
class TcAbsoluteLayout : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "AbsoluteLayout: Absolute Position";
  }

  Dali::String GetDescription() const override
  {
    return "Children placed at explicit LayoutRect pixel bounds";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: AbsoluteLayout filling the content area
    AbsoluteLayout root = AbsoluteLayout::New();
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    // Red box: top-left corner, absolute position and size
    View redBox = View::New();
    redBox.SetBackgroundColor(Color::RED);
    redBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(50.0f, 50.0f, 50.0f, 50.0f)));
    root.Add(redBox);

    // Green box: center area
    View greenBox = View::New();
    greenBox.SetBackgroundColor(Color::GREEN);
    greenBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(100.0f, 100.0f, 100.0f, 100.0f)));
    root.Add(greenBox);

    // Blue box: lower-right area
    View blueBox = View::New();
    blueBox.SetBackgroundColor(Color::BLUE);
    blueBox.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(200.0f, 200.0f, 100.0f, 50.0f)));
    root.Add(blueBox);

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

REGISTER_MANUAL_TEST(TcAbsoluteLayout)
