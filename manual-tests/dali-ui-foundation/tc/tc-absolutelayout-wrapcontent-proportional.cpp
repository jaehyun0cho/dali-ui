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
 * @brief Verifies a proportional child against a WRAP_CONTENT minSize parent.
 *
 *   - Root is an AbsoluteLayout with WRAP_CONTENT width, minWidth=400,
 *     and fixed height 200.
 *   - Child 1 has fixed size 200x200 at position (0, 0).
 *   - Child 2 (AbsoluteLayout) has proportional size (0.5, 1.0) at x=200,
 *     and contains one fixed (50x100) and one proportional grandchild.
 *
 * Expected: the layout wraps to 400 (driven by minWidth), child 2 gets
 * 0.5 * 400 = 200 wide, and inside it grandChild2 is 0.5 * 200 = 100 wide.
 *
 * Ported from samples/absolutelayout/absolutelayout-wrapcontent-proportional-example.cpp.
 */
class TcAbsoluteLayoutWrapContentProportional : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "AbsoluteLayout: WrapContent MinSize + Proportional Child";
  }

  Dali::String GetDescription() const override
  {
    return "Proportional child resolves against a minWidth-driven WRAP_CONTENT parent";
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
    root.SetMinimumWidth(400.0f);
    root.SetRequestedHeight(200.0f);

    // Child 1: fixed 200x200 at (0, 0).
    View child1 = View::New();
    child1.SetBackgroundColor(Color::RED);
    child1.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(0.0f, 0.0f, 200.0f, 200.0f)));
    root.Add(child1);

    // Child 2: AbsoluteLayout with proportional size (0.5 width, 1.0 height) at x=200.
    // Contains two grandchildren: one fixed, one proportional (0.5 of child2).
    AbsoluteLayout child2 = AbsoluteLayout::New();
    child2.SetBackgroundColor(Color::GREEN);
    child2.SetLayoutParams(AbsoluteLayoutParams::New()
                             .SetBounds(LayoutRect(200.0f, 0.0f, 0.5f, 1.0f))
                             .SetFlags(AbsoluteLayoutFlags::SIZE_PROPORTIONAL));
    root.Add(child2);

    View grandChild1 = View::New();
    grandChild1.SetBackgroundColor(Color::BLUE);
    grandChild1.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(0.0f, 0.0f, 50.0f, 100.0f)));
    child2.Add(grandChild1);

    View grandChild2 = View::New();
    grandChild2.SetBackgroundColor(Color::YELLOW);
    grandChild2.SetLayoutParams(AbsoluteLayoutParams::New()
      .SetBounds(LayoutRect(50.0f, 0.0f, 0.5f, 1.0f))
      .SetFlags(AbsoluteLayoutFlags::SIZE_PROPORTIONAL));
    child2.Add(grandChild2);

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

REGISTER_MANUAL_TEST(TcAbsoluteLayoutWrapContentProportional)
