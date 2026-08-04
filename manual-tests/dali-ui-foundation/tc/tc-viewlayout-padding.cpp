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
 * @brief Verifies padding on the children of a plain View.
 *
 * Same structure as ViewLayout: Margin but using padding instead of margin.
 * All 3 children have their own child view.
 *
 * 1. Red: width=MATCH_PARENT, height=200, padding=(50,50,50,50).
 *    -> Yellow grandchild: width=MATCH_PARENT, height=100.
 * 2. Green (WRAP_CONTENT): width=WRAP_CONTENT, height=200, pos=(0,200),
 *    padding=(50,50,50,50).
 *    -> Cyan grandchild: width=100, height=100 (NOT MATCH_PARENT).
 * 3. Blue: width=200, height=200, pos=(0,400), padding=(50,50,50,50).
 *    -> Magenta grandchild: width=MATCH_PARENT, height=100.
 *
 * Ported from samples/viewlayout/view-padding-example.cpp.  The sample's own
 * doc comment claims the second and third children sit at y=300 and y=600,
 * but its code requests y=200 and y=400; the code is authoritative here.
 */
class TcViewLayoutPadding : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "ViewLayout: Padding";
  }

  Dali::String GetDescription() const override
  {
    return "Padding insets children on MATCH_PARENT, WRAP_CONTENT and fixed parents";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: MATCH_PARENT
    View root = View::New();
    root.SetBackgroundColor(Color::GRAY);
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);

    // Child 1: Red, width=MATCH_PARENT, height=200, padding=50
    View child1 = View::New();
    child1.SetBackgroundColor(Color::RED);
    child1.SetRequestedWidth(MATCH_PARENT);
    child1.SetRequestedHeight(200.0f);
    child1.SetPadding(Extents(50, 50, 50, 50));

    // Child 1's child: Yellow, width=MATCH_PARENT, height=100
    View grandchild1 = View::New();
    grandchild1.SetBackgroundColor(Color::YELLOW);
    grandchild1.SetRequestedWidth(MATCH_PARENT);
    grandchild1.SetRequestedHeight(100.0f);
    child1.Add(grandchild1);

    root.Add(child1);

    // Child 2: Green, width=WRAP_CONTENT, height=200, pos=(0,200), padding=50
    View child2 = View::New();
    child2.SetBackgroundColor(Color::GREEN);
    child2.SetRequestedWidth(WRAP_CONTENT);
    child2.SetRequestedHeight(200.0f);
    child2.SetRequestedY(200.0f);
    child2.SetPadding(Extents(50, 50, 50, 50));

    // Child 2's child: Cyan, width=100, height=100 (NOT MATCH_PARENT)
    View grandchild2 = View::New();
    grandchild2.SetBackgroundColor(Color::CYAN);
    grandchild2.SetRequestedWidth(100.0f);
    grandchild2.SetRequestedHeight(100.0f);
    child2.Add(grandchild2);

    root.Add(child2);

    // Child 3: Blue, width=200, height=200, pos=(0,400), padding=50
    View child3 = View::New();
    child3.SetBackgroundColor(Color::BLUE);
    child3.SetRequestedWidth(200.0f);
    child3.SetRequestedHeight(200.0f);
    child3.SetRequestedY(400.0f);
    child3.SetPadding(Extents(50, 50, 50, 50));

    // Child 3's child: Magenta, width=MATCH_PARENT, height=100
    View grandchild3 = View::New();
    grandchild3.SetBackgroundColor(Color::MAGENTA);
    grandchild3.SetRequestedWidth(MATCH_PARENT);
    grandchild3.SetRequestedHeight(100.0f);
    child3.Add(grandchild3);

    root.Add(child3);

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

REGISTER_MANUAL_TEST(TcViewLayoutPadding)
