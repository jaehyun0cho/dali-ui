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
 * @brief Verifies horizontal StackLayout orientation and vertical alignment.
 *
 * A vertical StackLayout holds four horizontal rows.  Each row is a
 * horizontal StackLayout showing a different cross-axis alignment on its
 * children, plus LayoutWeight for proportional sizing:
 *   Row 1: START   (top-aligned)
 *   Row 2: CENTER  (center-aligned)
 *   Row 3: END     (bottom-aligned)
 *   Row 4: FILL    (stretched to fill)
 *
 * Ported from samples/stacklayout/stacklayout-horizontal-example.cpp.
 */
class TcStackLayoutHorizontal : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "StackLayout: Horizontal Rows + Alignment";
  }

  Dali::String GetDescription() const override
  {
    return "Four horizontal rows showing START / CENTER / END / FILL with weight";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Outer vertical stack to hold multiple horizontal rows
    StackLayout outer = StackLayout::New(StackOrientation::VERTICAL);
    outer.SetRequestedWidth(MATCH_PARENT);
    outer.SetRequestedHeight(MATCH_PARENT);
    outer.SetSpacing(10.0f);
    outer.SetPadding(Extents(50, 50, 50, 50));

    // Row 1: Horizontal stack, children aligned to Start (top)
    StackLayout row1 = StackLayout::New(StackOrientation::HORIZONTAL);
    row1.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL));
    row1.SetSpacing(10.0f);
    row1.SetBackgroundColor(Vector4(0.95f, 0.95f, 0.95f, 1.0f));

    View box1a = View::New();
    box1a.SetBackgroundColor(Color::RED);
    box1a.SetRequestedWidth(50.0f);
    box1a.SetRequestedHeight(50.0f);
    box1a.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::START));
    row1.Add(box1a);

    View box1b = View::New();
    box1b.SetBackgroundColor(Color::GREEN);
    box1b.SetRequestedWidth(50.0f);
    box1b.SetRequestedHeight(50.0f);
    box1b.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::START));
    row1.Add(box1b);

    View box1c = View::New();
    box1c.SetBackgroundColor(Color::BLUE);
    box1c.SetRequestedWidth(WRAP_CONTENT);
    box1c.SetRequestedHeight(50.0f);
    box1c.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::START).SetWeight(1.0f));
    row1.Add(box1c);

    outer.Add(row1);

    // Row 2: Horizontal stack, children aligned to Center
    StackLayout row2 = StackLayout::New(StackOrientation::HORIZONTAL);
    row2.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL));
    row2.SetSpacing(10.0f);
    row2.SetBackgroundColor(Vector4(0.9f, 0.9f, 0.9f, 1.0f));

    View box2a = View::New();
    box2a.SetBackgroundColor(Color::RED);
    box2a.SetRequestedWidth(50.0f);
    box2a.SetRequestedHeight(50.0f);
    box2a.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::CENTER));
    row2.Add(box2a);

    View box2b = View::New();
    box2b.SetBackgroundColor(Color::GREEN);
    box2b.SetRequestedWidth(50.0f);
    box2b.SetRequestedHeight(50.0f);
    box2b.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::CENTER));
    row2.Add(box2b);

    View box2c = View::New();
    box2c.SetBackgroundColor(Color::BLUE);
    box2c.SetRequestedWidth(WRAP_CONTENT);
    box2c.SetRequestedHeight(50.0f);
    box2c.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::CENTER).SetWeight(1.0f));
    row2.Add(box2c);

    outer.Add(row2);

    // Row 3: Horizontal stack, children aligned to End (bottom)
    StackLayout row3 = StackLayout::New(StackOrientation::HORIZONTAL);
    row3.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL));
    row3.SetSpacing(10.0f);
    row3.SetBackgroundColor(Vector4(0.95f, 0.95f, 0.95f, 1.0f));

    View box3a = View::New();
    box3a.SetBackgroundColor(Color::RED);
    box3a.SetRequestedWidth(50.0f);
    box3a.SetRequestedHeight(50.0f);
    box3a.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::END));
    row3.Add(box3a);

    View box3b = View::New();
    box3b.SetBackgroundColor(Color::GREEN);
    box3b.SetRequestedWidth(50.0f);
    box3b.SetRequestedHeight(50.0f);
    box3b.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::END));
    row3.Add(box3b);

    View box3c = View::New();
    box3c.SetBackgroundColor(Color::BLUE);
    box3c.SetRequestedWidth(WRAP_CONTENT);
    box3c.SetRequestedHeight(50.0f);
    box3c.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::END).SetWeight(1.0f));
    row3.Add(box3c);

    outer.Add(row3);

    // Row 4: Horizontal stack, children use FILL to stretch cross-axis (height)
    StackLayout row4 = StackLayout::New(StackOrientation::HORIZONTAL);
    row4.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL));
    row4.SetSpacing(10.0f);
    row4.SetBackgroundColor(Vector4(0.9f, 0.9f, 0.9f, 1.0f));

    View box4a = View::New();
    box4a.SetBackgroundColor(Color::RED);
    box4a.SetRequestedWidth(50.0f);
    box4a.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
    row4.Add(box4a);

    View box4b = View::New();
    box4b.SetBackgroundColor(Color::GREEN);
    box4b.SetRequestedWidth(50.0f);
    box4b.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
    row4.Add(box4b);

    View box4c = View::New();
    box4c.SetBackgroundColor(Color::BLUE);
    box4c.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL));
    row4.Add(box4c);

    outer.Add(row4);

    mBackdrop.Add(outer);
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

REGISTER_MANUAL_TEST(TcStackLayoutHorizontal)
