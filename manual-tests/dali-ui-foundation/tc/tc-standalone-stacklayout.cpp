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
 * @brief Verifies LayoutMode::STANDALONE inside a StackLayout.
 *
 * Same structure as the StackLayout sample, but the Blue bottom bar is set to
 * LayoutMode::STANDALONE with RequestedWidth/Height = (100, 100) and
 * SetRequestedX/Y = (300, 300).  It is excluded from the stack accumulation
 * (and from spacing) and instead drawn at (300, 300) in the parent's
 * coordinate space, while Red (top bar) and Green (weighted) still stack
 * normally.
 *
 * Ported from samples/layoutmodestandalone/stacklayout-standalone-example.cpp.
 */
class TcStandaloneStackLayout : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "StandaloneMode: StackLayout";
  }

  Dali::String GetDescription() const override
  {
    return "STANDALONE child excluded from stack accumulation and spacing";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: vertical StackLayout filling the window
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

    // Bottom bar: Blue, Standalone (100x100 at (300, 300))
    View bottomBar = View::New();
    bottomBar.SetBackgroundColor(Color::BLUE);
    bottomBar.SetRequestedWidth(100.0f);
    bottomBar.SetRequestedHeight(100.0f);
    bottomBar.SetRequestedX(300.0f);
    bottomBar.SetRequestedY(300.0f);
    bottomBar.SetLayoutMode(LayoutMode::STANDALONE);
    root.Add(bottomBar);

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

REGISTER_MANUAL_TEST(TcStandaloneStackLayout)
