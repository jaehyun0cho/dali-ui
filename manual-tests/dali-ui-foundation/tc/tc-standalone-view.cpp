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
 * @brief Verifies LayoutMode::STANDALONE inside a plain View.
 *
 * Same structure as the View margin sample, but the Blue child is set to
 * LayoutMode::STANDALONE with RequestedWidth/Height = (100, 100) and
 * SetRequestedX/Y = (300, 300).  It is excluded from the parent View's
 * WRAP_CONTENT accumulation and placed at (300, 300) in the parent's
 * coordinate space.
 *
 * Ported from samples/layoutmodestandalone/view-standalone-example.cpp.
 */
class TcStandaloneView : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "StandaloneMode: View";
  }

  Dali::String GetDescription() const override
  {
    return "STANDALONE child excluded from a plain View's WRAP_CONTENT accumulation";
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

    // Child 1: Red, width=MATCH_PARENT, height=200, margin=50
    View child1 = View::New();
    child1.SetBackgroundColor(Color::RED);
    child1.SetRequestedWidth(MATCH_PARENT);
    child1.SetRequestedHeight(200.0f);
    child1.SetMargin(Extents(50, 50, 50, 50));
    root.Add(child1);

    // Child 2: Green, width=WRAP_CONTENT, height=200, pos=(0,300), margin=50
    View child2 = View::New();
    child2.SetBackgroundColor(Color::GREEN);
    child2.SetRequestedWidth(WRAP_CONTENT);
    child2.SetRequestedHeight(200.0f);
    child2.SetRequestedY(300.0f);
    child2.SetMargin(Extents(50, 50, 50, 50));

    // Child 2's child: Yellow, 100x100
    View grandchild = View::New();
    grandchild.SetBackgroundColor(Color::YELLOW);
    grandchild.SetRequestedWidth(100.0f);
    grandchild.SetRequestedHeight(100.0f);
    child2.Add(grandchild);

    root.Add(child2);

    // Child 3: Blue, Standalone (100x100 at (300, 300))
    View child3 = View::New();
    child3.SetBackgroundColor(Color::BLUE);
    child3.SetRequestedWidth(100.0f);
    child3.SetRequestedHeight(100.0f);
    child3.SetRequestedX(300.0f);
    child3.SetRequestedY(300.0f);
    child3.SetLayoutMode(LayoutMode::STANDALONE);
    root.Add(child3);

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
    // No mutable scalar state; every handle is reassigned in OnEnter.
  }

  View mBackdrop;
};

REGISTER_MANUAL_TEST(TcStandaloneView)
