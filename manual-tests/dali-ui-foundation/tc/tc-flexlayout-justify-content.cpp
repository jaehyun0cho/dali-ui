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
 * @brief Verifies the FlexJustify modes not covered by the other flex tests:
 *   - FLEX_END:     items packed to the end
 *   - CENTER:       items centred
 *   - SPACE_AROUND: equal space around each item
 *   - SPACE_EVENLY: equal space between all items and edges
 *
 * Layout: four horizontal FlexLayout rows stacked vertically, each
 * using a different FlexJustify mode with three fixed-size boxes.
 *
 * Ported from samples/flexlayout/flexlayout-justify-content-example.cpp.
 */
class TcFlexLayoutJustifyContent : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "FlexLayout: JustifyContent Modes";
  }

  Dali::String GetDescription() const override
  {
    return "FLEX_END, CENTER, SPACE_AROUND and SPACE_EVENLY rows compared";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    StackLayout outer = StackLayout::New(StackOrientation::VERTICAL);
    outer.SetRequestedWidth(MATCH_PARENT);
    outer.SetRequestedHeight(MATCH_PARENT);
    outer.SetSpacing(50.0f);
    outer.SetPadding(Extents(50, 50, 50, 50));

    // Row 1: FlexEnd
    FlexLayout rowEnd = FlexLayout::New();
    rowEnd.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL));
    rowEnd.SetDirection(FlexDirection::ROW);
    rowEnd.SetJustifyContent(FlexJustify::FLEX_END);
    rowEnd.SetAlignItems(FlexAlign::CENTER);
    rowEnd.SetBackgroundColor(Vector4(0.95f, 0.95f, 0.95f, 1.0f));

    AddThreeBoxes(rowEnd, Color::RED, Color::GREEN, Color::BLUE);
    outer.Add(rowEnd);

    // Row 2: Center
    FlexLayout rowCenter = FlexLayout::New();
    rowCenter.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL));
    rowCenter.SetDirection(FlexDirection::ROW);
    rowCenter.SetJustifyContent(FlexJustify::CENTER);
    rowCenter.SetAlignItems(FlexAlign::CENTER);
    rowCenter.SetBackgroundColor(Vector4(0.9f, 0.9f, 0.9f, 1.0f));

    AddThreeBoxes(rowCenter, Color::YELLOW, Color::CYAN, Color::MAGENTA);
    outer.Add(rowCenter);

    // Row 3: SpaceAround
    FlexLayout rowAround = FlexLayout::New();
    rowAround.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL));
    rowAround.SetDirection(FlexDirection::ROW);
    rowAround.SetJustifyContent(FlexJustify::SPACE_AROUND);
    rowAround.SetAlignItems(FlexAlign::CENTER);
    rowAround.SetBackgroundColor(Vector4(0.95f, 0.95f, 0.95f, 1.0f));

    AddThreeBoxes(rowAround, Color::RED, Color::GREEN, Color::BLUE);
    outer.Add(rowAround);

    // Row 4: SpaceEvenly
    FlexLayout rowEvenly = FlexLayout::New();
    rowEvenly.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL));
    rowEvenly.SetDirection(FlexDirection::ROW);
    rowEvenly.SetJustifyContent(FlexJustify::SPACE_EVENLY);
    rowEvenly.SetAlignItems(FlexAlign::CENTER);
    rowEvenly.SetBackgroundColor(Vector4(0.9f, 0.9f, 0.9f, 1.0f));

    AddThreeBoxes(rowEvenly, Color::YELLOW, Color::CYAN, Color::MAGENTA);
    outer.Add(rowEvenly);

    mBackdrop.Add(outer);
    contentArea.Add(mBackdrop);
  }

  void OnExit() override
  {
    mBackdrop.Reset();
    ResetState();
  }

private:
  void AddThreeBoxes(FlexLayout& parent, Vector4 color1, Vector4 color2, Vector4 color3)
  {
    View box1 = View::New();
    box1.SetBackgroundColor(color1);
    box1.SetRequestedWidth(50.0f);
    box1.SetRequestedHeight(50.0f);
    parent.Add(box1);

    View box2 = View::New();
    box2.SetBackgroundColor(color2);
    box2.SetRequestedWidth(50.0f);
    box2.SetRequestedHeight(50.0f);
    parent.Add(box2);

    View box3 = View::New();
    box3.SetBackgroundColor(color3);
    box3.SetRequestedWidth(50.0f);
    box3.SetRequestedHeight(50.0f);
    parent.Add(box3);
  }

  void ResetState()
  {
    // No mutable state beyond the view handles released in OnExit().
  }

  View mBackdrop;
};

REGISTER_MANUAL_TEST(TcFlexLayoutJustifyContent)
