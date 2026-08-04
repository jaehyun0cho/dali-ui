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

namespace
{
constexpr float TOGGLE_BUTTON_WIDTH  = 220.0f;
constexpr float TOGGLE_BUTTON_HEIGHT = 50.0f;
constexpr float FIXED_PARENT_WIDTH   = 400.0f;
constexpr float FIXED_PARENT_HEIGHT  = 200.0f;
} // namespace

/**
 * @brief Verifies a WRAP_CONTENT parent holding a position-proportional label.
 *
 * A WRAP_CONTENT AbsoluteLayout parent sizes to a WRAP_CONTENT Label child
 * when the child's bounds use a proportional x/y position and WRAP_CONTENT
 * width/height.
 *
 *   - Root view is MATCH_PARENT and green.
 *   - Parent AbsoluteLayout is WRAP_CONTENT and red.
 *   - Child Label is WRAP_CONTENT and blue.
 *   - Child bounds are (0.5, 0.5, -1.0, -1.0) with POSITION_PROPORTIONAL.
 *
 * The toggle button switches the parent between WRAP_CONTENT and a fixed
 * 400x200 size.  It is horizontally centred by X_PROPORTIONAL bounds on the
 * backdrop AbsoluteLayout, so no window resize signal is needed.
 *
 * Ported from
 * samples/absolutelayout/absolutelayout-wrapcontent-position-proportional-label-example.cpp.
 */
class TcAbsoluteLayoutWrapContentPositionProportionalLabel : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "AbsoluteLayout: WrapContent Parent + Position-Proportional Label";
  }

  Dali::String GetDescription() const override
  {
    return "Toggle a WRAP_CONTENT AbsoluteLayout parent between wrap and fixed 400x200";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = AbsoluteLayout::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    View root = View::New();
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetBackgroundColor(Color::GREEN);
    root.SetLayoutParams(AbsoluteLayoutParams::New()
                           .SetBounds(LayoutRect(0.0f, 0.0f, 1.0f, 1.0f))
                           .SetFlags(AbsoluteLayoutFlags::SIZE_PROPORTIONAL));

    mParent = AbsoluteLayout::New();
    mParent.SetRequestedWidth(WRAP_CONTENT);
    mParent.SetRequestedHeight(WRAP_CONTENT);
    mParent.SetPadding(Extents(50, 50, 50, 50));
    mParent.SetBackgroundColor(Color::RED);
    root.Add(mParent);

    Label child = Label::New("WRAP Label");
    child.SetRequestedWidth(WRAP_CONTENT);
    child.SetRequestedHeight(WRAP_CONTENT);
    child.SetBackgroundColor(Color::BLUE);
    child.SetTextColor(UiColor(1.0f, 1.0f, 1.0f, 1.0f));
    child.SetLayoutParams(AbsoluteLayoutParams::New()
                            .SetBounds(LayoutRect(0.5f, 0.5f, -1.0f, -1.0f))
                            .SetFlags(AbsoluteLayoutFlags::POSITION_PROPORTIONAL));
    mParent.Add(child);

    mBackdrop.Add(root);
    CreateToggleButton();

    contentArea.Add(mBackdrop);
  }

  void OnExit() override
  {
    mToggleLabel.Reset();
    mToggleButton.Reset();
    mParent.Reset();
    mBackdrop.Reset();
    ResetState();
  }

private:
  void ResetState()
  {
    // The toggle label itself needs no reset here: CreateToggleButton()
    // recreates it with the initial "Click to set size" text on every entry.
    mFixedSize = false;
  }

  void CreateToggleButton()
  {
    mToggleButton = InteractiveView::New();
    mToggleButton.SetBackgroundColor(Vector4(0.0f, 0.0f, 0.0f, 0.65f));
    mToggleButton.SetRequestedWidth(TOGGLE_BUTTON_WIDTH);
    mToggleButton.SetRequestedHeight(TOGGLE_BUTTON_HEIGHT);
    mToggleButton.SetLayoutDirection(Dali::LayoutDirection::LEFT_TO_RIGHT);
    // Horizontally centred by the backdrop: x = (availableWidth - width) * 0.5
    mToggleButton.SetLayoutParams(AbsoluteLayoutParams::New()
                                    .SetBounds(LayoutRect(0.5f, 0.0f, TOGGLE_BUTTON_WIDTH, TOGGLE_BUTTON_HEIGHT))
                                    .SetFlags(AbsoluteLayoutFlags::X_PROPORTIONAL));

    mToggleLabel = Label::New("Click to set size");
    mToggleLabel.SetTextColor(UiColor(1.0f, 1.0f, 1.0f, 1.0f));
    mToggleLabel.SetRequestedWidth(MATCH_PARENT);
    mToggleLabel.SetRequestedHeight(MATCH_PARENT);
    mToggleLabel.SetHorizontalTextAlignment(Text::Alignment::CENTER);
    mToggleLabel.SetVerticalTextAlignment(Text::Alignment::CENTER);
    mToggleButton.Add(mToggleLabel);

    mToggleButton.ConnectClickedSignal(this, [this](View view, InputEvent event) -> bool {
      mFixedSize = !mFixedSize;

      if(mFixedSize)
      {
        mParent.SetRequestedWidth(FIXED_PARENT_WIDTH);
        mParent.SetRequestedHeight(FIXED_PARENT_HEIGHT);
        mToggleLabel.SetText("Click to set wrap");
      }
      else
      {
        mParent.SetRequestedWidth(WRAP_CONTENT);
        mParent.SetRequestedHeight(WRAP_CONTENT);
        mToggleLabel.SetText("Click to set size");
      }

      return true;
    });
    mBackdrop.Add(mToggleButton);
  }

  AbsoluteLayout  mBackdrop;
  AbsoluteLayout  mParent;
  InteractiveView mToggleButton;
  Label           mToggleLabel;
  bool            mFixedSize{false};
};

REGISTER_MANUAL_TEST(TcAbsoluteLayoutWrapContentPositionProportionalLabel)
