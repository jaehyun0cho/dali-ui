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
 * @brief Verifies FlexJustify::SPACE_BETWEEN together with per-item AlignSelf.
 *
 * - Six fixed-size boxes with the same width and height
 * - JustifyContent: SPACE_BETWEEN (even spacing between items)
 * - AlignItems: CENTER (vertically centred on the cross axis)
 * - Four children override the container alignment through AlignSelf
 *
 * Ported from samples/flexlayout/flexlayout-justify-align-example.cpp.
 */
class TcFlexLayoutJustifyAlign : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "FlexLayout: SpaceBetween + AlignSelf";
  }

  Dali::String GetDescription() const override
  {
    return "JustifyContent SPACE_BETWEEN with per-item AlignSelf overrides";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    FlexLayout root = FlexLayout::New();
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetDirection(FlexDirection::ROW);
    root.SetJustifyContent(FlexJustify::SPACE_BETWEEN);
    root.SetAlignItems(FlexAlign::CENTER);
    root.SetPadding(Extents(50, 50, 50, 50));

    // Red box
    View redBox = View::New();
    redBox.SetBackgroundColor(Color::RED);
    redBox.SetRequestedWidth(50.0f);
    redBox.SetRequestedHeight(200.0f);
    root.Add(redBox);

    // Green box
    View greenBox = View::New();
    greenBox.SetBackgroundColor(Color::GREEN);
    greenBox.SetRequestedWidth(50.0f);
    greenBox.SetRequestedHeight(200.0f);
    greenBox.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::FLEX_START));
    root.Add(greenBox);

    // Blue box
    View blueBox = View::New();
    blueBox.SetBackgroundColor(Color::BLUE);
    blueBox.SetRequestedWidth(50.0f);
    blueBox.SetRequestedHeight(200.0f);
    blueBox.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::CENTER));
    root.Add(blueBox);

    // Yellow box
    View yellowBox = View::New();
    yellowBox.SetBackgroundColor(Color::YELLOW);
    yellowBox.SetRequestedWidth(50.0f);
    yellowBox.SetRequestedHeight(200.0f);
    yellowBox.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::FLEX_END));
    root.Add(yellowBox);

    // Cyan box
    View cyanBox = View::New();
    cyanBox.SetBackgroundColor(Color::CYAN);
    cyanBox.SetRequestedWidth(50.0f);
    cyanBox.SetRequestedHeight(200.0f);
    cyanBox.SetLayoutParams(FlexLayoutParams::New().SetAlignSelf(FlexAlign::BASELINE));
    root.Add(cyanBox);

    // Magenta box
    View magentaBox = View::New();
    magentaBox.SetBackgroundColor(Color::MAGENTA);
    magentaBox.SetRequestedWidth(50.0f);
    magentaBox.SetRequestedHeight(200.0f);
    root.Add(magentaBox);

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
    // No mutable state beyond the view handles released in OnExit().
  }

  View mBackdrop;
};

REGISTER_MANUAL_TEST(TcFlexLayoutJustifyAlign)
