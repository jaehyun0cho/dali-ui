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
 * @brief Verifies LayoutMode::STANDALONE inside a FlexLayout.
 *
 * Same structure as the FlexLayout sample, but the Blue child is set to
 * LayoutMode::STANDALONE with RequestedWidth/Height = (100, 100) and
 * SetRequestedX/Y = (300, 300).  It is excluded from the flex line and placed
 * at (300, 300) in the parent's coordinate space, while Red and Green still
 * participate in the flex row layout.
 *
 * Ported from samples/layoutmodestandalone/flexlayout-standalone-example.cpp.
 */
class TcStandaloneFlexLayout : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "StandaloneMode: FlexLayout";
  }

  Dali::String GetDescription() const override
  {
    return "STANDALONE child excluded from the flex line";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: FlexLayout in row direction
    FlexLayout root = FlexLayout::New();
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetDirection(FlexDirection::ROW);
    root.SetAlignItems(FlexAlign::STRETCH);
    root.SetPadding(Extents(50, 50, 50, 50));

    // Red box: fixed width
    View redBox = View::New();
    redBox.SetBackgroundColor(Color::RED);
    redBox.SetRequestedWidth(100.0f);
    root.Add(redBox);

    // Green box: flex-grow 1 (fills remaining space)
    View greenBox = View::New();
    greenBox.SetBackgroundColor(Color::GREEN);
    greenBox.SetRequestedWidth(WRAP_CONTENT);
    greenBox.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f));
    root.Add(greenBox);

    // Blue box: Standalone (100x100 at (300, 300))
    View blueBox = View::New();
    blueBox.SetBackgroundColor(Color::BLUE);
    blueBox.SetRequestedWidth(100.0f);
    blueBox.SetRequestedHeight(100.0f);
    blueBox.SetRequestedX(300.0f);
    blueBox.SetRequestedY(300.0f);
    blueBox.SetLayoutMode(LayoutMode::STANDALONE);
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
    // No mutable scalar state; every handle is reassigned in OnEnter.
  }

  View mBackdrop;
};

REGISTER_MANUAL_TEST(TcStandaloneFlexLayout)
