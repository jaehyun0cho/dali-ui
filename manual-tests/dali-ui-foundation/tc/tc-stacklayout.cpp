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
 * @brief Verifies main-axis sizing of a vertical StackLayout.
 *
 * The root is a vertical StackLayout filling the content area with a
 * 50px padding and a 10px spacing.  The top red bar and the bottom blue
 * bar have a fixed height of 100, while the green middle child takes all
 * the remaining space through StackLayoutParams weight 1.  Every child
 * uses LayoutAlignment::FILL so it stretches on the cross axis.
 *
 * Ported from samples/stacklayout/stacklayout-example.cpp.
 */
class TcStackLayout : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "StackLayout: Vertical Weight";
  }

  Dali::String GetDescription() const override
  {
    return "Fixed top/bottom bars with a weight-1 middle";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: vertical StackLayout filling the content area
    StackLayout root = StackLayout::New(StackOrientation::VERTICAL);
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetSpacing(10.0f);
    root.SetPadding(Extents(50, 50, 50, 50));

    // Top bar: fixed height, fill cross-axis
    View topBar = View::New();
    topBar.SetBackgroundColor(Color::RED);
    topBar.SetRequestedHeight(100.0f);
    topBar.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
    root.Add(topBar);

    // Middle: weighted to take remaining space, fill cross-axis
    View middle = View::New();
    middle.SetBackgroundColor(Color::GREEN);
    middle.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL));
    root.Add(middle);

    // Bottom bar: fixed height, fill cross-axis
    View bottomBar = View::New();
    bottomBar.SetBackgroundColor(Color::BLUE);
    bottomBar.SetRequestedHeight(100.0f);
    bottomBar.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
    root.Add(bottomBar);

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

REGISTER_MANUAL_TEST(TcStackLayout)
