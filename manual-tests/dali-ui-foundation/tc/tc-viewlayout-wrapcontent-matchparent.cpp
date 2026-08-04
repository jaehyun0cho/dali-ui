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
 * @brief Verifies MATCH_PARENT "follower" semantics under a WRAP_CONTENT parent.
 *
 *   - The root View uses WRAP_CONTENT for both width and height.
 *   - Child 1 has a fixed size of 300x300 with margin 50.
 *   - Child 2 uses MATCH_PARENT for both axes with margin 100.
 *
 * Expected result:
 *   The root wraps to 400x400 (driven by child 1: 300 + 50*2 margin).
 *   The MATCH_PARENT child fills 400x400 minus its own margin (200x200),
 *   sitting behind the fixed child (both are positioned at the same origin
 *   since a plain View overlaps its children).
 *
 * Ported from samples/viewlayout/view-wrapcontent-matchparent-example.cpp.
 */
class TcViewLayoutWrapContentMatchParent : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "ViewLayout: MATCH_PARENT under WRAP_CONTENT";
  }

  Dali::String GetDescription() const override
  {
    return "MATCH_PARENT follower semantics inside a WRAP_CONTENT parent";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: WRAP_CONTENT on both axes — size is determined by children.
    View root = View::New();
    root.SetBackgroundColor(Color::GAINSBORO);
    root.SetRequestedWidth(WRAP_CONTENT);
    root.SetRequestedHeight(WRAP_CONTENT);

    // Child 1: Fixed 300x300 — drives the WRAP_CONTENT parent's size.
    View child1 = View::New();
    child1.SetBackgroundColor(Color::RED);
    child1.SetRequestedWidth(300.0f);
    child1.SetRequestedHeight(300.0f);
    child1.SetMargin(Extents(50, 50, 50, 50));
    root.Add(child1);

    // Child 2: MATCH_PARENT — follows the parent size (which is driven by child 1).
    View child2 = View::New();
    child2.SetBackgroundColor(Color::GREEN);
    child2.SetRequestedWidth(MATCH_PARENT);
    child2.SetRequestedHeight(MATCH_PARENT);
    child2.SetMargin(Extents(100, 100, 100, 100));
    root.Add(child2);

    View grandChild1 = View::New();
    grandChild1.SetBackgroundColor(Color::BLUE);
    grandChild1.SetRequestedWidth(MATCH_PARENT);
    grandChild1.SetRequestedHeight(MATCH_PARENT);
    grandChild1.SetMargin(Extents(50, 50, 50, 50));
    child2.Add(grandChild1);

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

REGISTER_MANUAL_TEST(TcViewLayoutWrapContentMatchParent)
