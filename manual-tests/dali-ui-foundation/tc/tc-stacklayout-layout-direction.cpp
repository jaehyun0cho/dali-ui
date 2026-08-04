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
 * @brief Verifies LayoutDirection mirroring of a horizontal StackLayout.
 *
 * The root is a horizontal StackLayout (left / middle / right).  A 200x50
 * translucent-black standalone child at the top-left toggles the root
 * view's LayoutDirection between LEFT_TO_RIGHT and RIGHT_TO_LEFT on click.
 * RTL flips direct children horizontally; the standalone toggle button is
 * excluded from mirroring and stays at the top-left.
 *
 * Ported from samples/stacklayout/stacklayout-layout-direction-example.cpp.
 */
class TcStackLayoutLayoutDirection : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "StackLayout: LayoutDirection Toggle";
  }

  Dali::String GetDescription() const override
  {
    return "RTL flips a horizontal stack; the STANDALONE toggle stays put";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Root: horizontal StackLayout filling the content area
    mRoot = StackLayout::New(StackOrientation::HORIZONTAL);
    mRoot.SetRequestedWidth(MATCH_PARENT);
    mRoot.SetRequestedHeight(MATCH_PARENT);
    mRoot.SetSpacing(10.0f);
    mRoot.SetPadding(Extents(50, 50, 50, 50));

    // Left bar: fixed width, fill cross-axis
    View leftBar = View::New();
    leftBar.SetBackgroundColor(Color::RED);
    leftBar.SetRequestedWidth(100.0f);
    leftBar.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
    mRoot.Add(leftBar);

    // Middle: weighted to take remaining space, fill cross-axis
    View middle = View::New();
    middle.SetBackgroundColor(Color::GREEN);
    middle.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL));
    mRoot.Add(middle);

    // Right bar: fixed width, fill cross-axis
    View rightBar = View::New();
    rightBar.SetBackgroundColor(Color::BLUE);
    rightBar.SetRequestedWidth(100.0f);
    rightBar.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
    mRoot.Add(rightBar);

    // Toggle button: 200x50 translucent-black standalone child at top-left
    // with a centered white "Change LayoutDirection" label.
    InteractiveView toggleBtn = InteractiveView::New();
    toggleBtn.SetBackgroundColor(Vector4(0.0f, 0.0f, 0.0f, 0.5f));
    toggleBtn.SetRequestedWidth(200.0f);
    toggleBtn.SetRequestedHeight(50.0f);
    toggleBtn.SetRequestedX(0.0f);
    toggleBtn.SetRequestedY(0.0f);
    toggleBtn.SetLayoutMode(LayoutMode::STANDALONE);
    toggleBtn.SetLayoutDirection(Dali::LayoutDirection::LEFT_TO_RIGHT);
    Label toggleLabel = Label::New("Change LayoutDirection");
    toggleLabel.SetTextColor(UiColor(1.0f, 1.0f, 1.0f, 1.0f));
    toggleLabel.SetRequestedWidth(MATCH_PARENT);
    toggleLabel.SetRequestedHeight(MATCH_PARENT);
    toggleLabel.SetHorizontalTextAlignment(Text::Alignment::CENTER);
    toggleLabel.SetVerticalTextAlignment(Text::Alignment::CENTER);
    toggleBtn.Add(toggleLabel);
    toggleBtn.ConnectClickedSignal(this, [this](View view, InputEvent event) -> bool {
      mIsRtl = !mIsRtl;
      mRoot.SetLayoutDirection(mIsRtl ? Dali::LayoutDirection::RIGHT_TO_LEFT : Dali::LayoutDirection::LEFT_TO_RIGHT);
      return true;
    });
    mRoot.Add(toggleBtn);

    mBackdrop.Add(mRoot);
    contentArea.Add(mBackdrop);
  }

  void OnExit() override
  {
    // The clicked signal is owned by a view inside the discarded subtree, and
    // there is no timer or animation, so releasing the member handles and
    // restoring the initial direction state is all that is required.
    ResetState();
  }

private:
  void ResetState()
  {
    mBackdrop.Reset();
    mRoot.Reset();
    mIsRtl = false;
  }

  View        mBackdrop;
  StackLayout mRoot;
  bool        mIsRtl{false};
};

REGISTER_MANUAL_TEST(TcStackLayoutLayoutDirection)
