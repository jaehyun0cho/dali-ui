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
 * @brief Verifies the per-axis proportional flags of AbsoluteLayout.
 *
 * Four children, each demonstrating a single axis-only proportional flag,
 * anchored to a distinct edge of the root (MATCH_PARENT) so that they do
 * not overlap for reasonable sizes (>= 400 x 400).
 *
 *   - Yellow : HEIGHT_PROPORTIONAL bounds(  0, 100, 100, 0.5)  (left strip)
 *   - Blue   : WIDTH_PROPORTIONAL  bounds(100,   0, 0.5, 100)  (top strip)
 *   - Red    : X_PROPORTIONAL      bounds(1.0,   0, 100, 100)  (top-right)
 *   - Green  : Y_PROPORTIONAL      bounds(  0, 1.0, 100, 100)  (bottom-left)
 *
 * Ported from samples/absolutelayout/absolutelayout-per-axis-proportional-example.cpp.
 */
class TcAbsoluteLayoutPerAxisProportional : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "AbsoluteLayout: Per-axis Proportional Flags";
  }

  Dali::String GetDescription() const override
  {
    return "HEIGHT_/WIDTH_/X_/Y_PROPORTIONAL used one axis at a time";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    AbsoluteLayout root = AbsoluteLayout::New();
    root.SetBackgroundColor(Color::GAINSBORO);
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);

    // Child 1: Height proportional only.
    View yellowBox = View::New();
    yellowBox.SetBackgroundColor(Color::YELLOW);
    yellowBox.SetLayoutParams(AbsoluteLayoutParams::New()
                                .SetBounds(LayoutRect(0.0f, 100.0f, 100.0f, 0.5f))
                                .SetFlags(AbsoluteLayoutFlags::HEIGHT_PROPORTIONAL));
    root.Add(yellowBox);

    // Child 2: Width proportional only.
    View blueBox = View::New();
    blueBox.SetBackgroundColor(Color::BLUE);
    blueBox.SetLayoutParams(AbsoluteLayoutParams::New()
                              .SetBounds(LayoutRect(100.0f, 0.0f, 0.5f, 100.0f))
                              .SetFlags(AbsoluteLayoutFlags::WIDTH_PROPORTIONAL));
    root.Add(blueBox);

    // Child 3: X proportional only.
    View redBox = View::New();
    redBox.SetBackgroundColor(Color::RED);
    redBox.SetLayoutParams(AbsoluteLayoutParams::New()
                             .SetBounds(LayoutRect(1.0f, 0.0f, 100.0f, 100.0f))
                             .SetFlags(AbsoluteLayoutFlags::X_PROPORTIONAL));
    root.Add(redBox);

    // Child 4: Y proportional only.
    View greenBox = View::New();
    greenBox.SetBackgroundColor(Color::GREEN);
    greenBox.SetLayoutParams(AbsoluteLayoutParams::New()
                               .SetBounds(LayoutRect(0.0f, 1.0f, 100.0f, 100.0f))
                               .SetFlags(AbsoluteLayoutFlags::Y_PROPORTIONAL));
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

REGISTER_MANUAL_TEST(TcAbsoluteLayoutPerAxisProportional)
