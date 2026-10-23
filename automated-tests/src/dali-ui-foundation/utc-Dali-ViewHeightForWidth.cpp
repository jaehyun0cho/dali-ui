/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
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
 *
 */

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/integration-api/label-impl.h>
#include <dali-ui-test-suite-utils.h>
#include <dali.h>
#include <dali/integration-api/adaptor-framework/adaptor.h>

#include <algorithm>
#include <string>

using namespace Dali;
using namespace Dali::Ui;

// Height-for-width at the width a view is laid out at.
//
// A child whose WRAP_CONTENT height follows its width -- a multi-line Label, a container holding
// one, a wrapping row FlexLayout -- is measured against an at-most budget, but the width it is
// finally laid out at is decided later by its parent. These cases check that the supported
// containers, the layout-root / STANDALONE slot and ScrollView content give such a child the
// height it needs at its final width, and that everything else keeps its current layout.
//
// Every case runs in the default 480 x 800 test window at UI scale 1 with no padding or margin,
// unless it states otherwise (UtcDaliViewHeightForWidthScaledMarginsAndPaddingP runs at UI scale 2
// with margins and padding). A view added straight to the window is a layout root measured at the
// root layer's 480 x 800.
// H(w) denotes the text height of the wrapping Label at width w, read through
// Label::GetHeightForWidth() after the first settle (the very metric the resolution queries), so
// the expectations hold whatever the harness font metrics are. Each Label case first checks the
// precondition that makes it non-vacuous (the text wraps differently at the widths involved).

namespace
{
// 78 uppercase letters, wrapped by character: several lines at narrow widths.
const char* const WRAPPING_TEXT = "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
constexpr float   HFW_EPSILON   = 0.01f;

// Counts CountingBoxMeasure() calls. Not a test case: no parameter list.
int gCountingBoxMeasureCount = 0;

// Drives one full layout batch to completion.
void SettleLayout(UiTestApplication& application)
{
  application.SendNotification();
  application.Render();
  application.SendNotification();
  application.Render();
}

// A multi-line, character-wrapped Label; its height keeps the default WRAP_CONTENT.
Label CreateWrappingLabel(float requestedWidth)
{
  Label label = Label::New(WRAPPING_TEXT);
  label.SetMultiLine(true);
  label.SetLineWrapMode(Text::LineWrapMode::CHARACTER);
  label.SetRequestedWidth(requestedWidth);
  return label;
}

// The sibling that sets a wrapping parent's width: fixed 100 x 0.
View CreateSeed()
{
  View seed = View::New();
  seed.SetRequestedWidth(100.0f);
  seed.SetRequestedHeight(0.0f);
  return seed;
}

// A fixed-size leaf whose producer is a MeasureCallback: an UNSUPPORTED producer by design, so it
// is never looked into, and every call of its producer is counted.
MeasuredSize CountingBoxMeasure(View, float, float)
{
  ++gCountingBoxMeasureCount;
  return MeasuredSize(60.0f, 40.0f);
}

// A custom parent: arranges every child at the origin at its measured size.
LayoutRect ArrangeChildrenAtMeasuredSize(View view, const LayoutRect& bounds)
{
  for(uint32_t i = 0; i < view.GetChildViewCount(); ++i)
  {
    View               child    = view.GetChildViewAt(i);
    const MeasuredSize measured = child.GetMeasuredSize();
    child.Arrange(LayoutRect(0.0f, 0.0f, measured.width, measured.height));
  }
  return bounds;
}

float WidthOf(Actor actor)
{
  return actor.GetProperty<float>(Actor::Property::SIZE_WIDTH);
}

float HeightOf(Actor actor)
{
  return actor.GetProperty<float>(Actor::Property::SIZE_HEIGHT);
}

// A 480-wide, WRAP_CONTENT-height grid with two ABSOLUTE columns and one AUTO row.
GridLayout CreateTwoColumnGrid(float firstColumnWidth, float secondColumnWidth)
{
  GridLayout grid = GridLayout::New();
  grid.SetRequestedWidth(480.0f);
  Dali::Vector<GridLength> columns;
  columns.PushBack(GridLength::Absolute(firstColumnWidth));
  columns.PushBack(GridLength::Absolute(secondColumnWidth));
  Dali::Vector<GridLength> rows;
  rows.PushBack(GridLength::Auto());
  grid.SetColumnDefinitions(columns);
  grid.SetRowDefinitions(rows);
  return grid;
}

float PositionXOf(Actor actor)
{
  return actor.GetProperty<float>(Actor::Property::POSITION_X);
}

float PositionYOf(Actor actor)
{
  return actor.GetProperty<float>(Actor::Property::POSITION_Y);
}

} // namespace

void utc_dali_view_height_for_width_startup(void)
{
  test_return_value = TET_UNDEF;
}

void utc_dali_view_height_for_width_cleanup(void)
{
  test_return_value = TET_PASS;
}

// A WRAP_CONTENT x WRAP_CONTENT vertical StackLayout root holding a 100-wide seed and a
// MATCH_PARENT-width Label. The stack is measured at 480 x 800 (the Label reports H(480)) and is
// laid out 100 wide, so its root slot resolves the Label at 100: the stack and the Label both get
// H(100). Before: the stack and the Label were H(480) tall at width 100, clipping the text.
int UtcDaliViewHeightForWidthRootStackP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL); // WRAP_CONTENT x WRAP_CONTENT
  View        seed  = CreateSeed();
  Label       label = CreateWrappingLabel(MATCH_PARENT);
  stack.Add(seed);
  stack.Add(label);
  window.Add(stack);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 > h480);

  DALI_TEST_EQUALS(WidthOf(stack), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// Nested plain Views: P (480 wide, WRAP_CONTENT height) <- R (WRAP_CONTENT x WRAP_CONTENT) <-
// [seed, MATCH_PARENT-width Label C]. P's root slot resolves R at R's measured width 100, which
// resolves C at 100; P's arrange then hands R that height and R's arrange hands it to C.
// Before: P, R and C were H(480) tall.
int UtcDaliViewHeightForWidthNestedDefaultContainersP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  View p = View::New();
  p.SetRequestedWidth(480.0f); // WRAP_CONTENT height
  View  r    = View::New();    // WRAP_CONTENT x WRAP_CONTENT
  View  seed = CreateSeed();
  Label c    = CreateWrappingLabel(MATCH_PARENT);
  r.Add(seed);
  r.Add(c);
  p.Add(r);
  window.Add(p);

  SettleLayout(application);

  const float h100 = c.GetHeightForWidth(100.0f);
  const float h480 = c.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 > h480);

  DALI_TEST_EQUALS(WidthOf(p), 480.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(p), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(r), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(r), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(c), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(c), h100, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A FILL-aligned WRAP_CONTENT x WRAP_CONTENT container R in a 480-wide vertical stack is stretched
// to 480, so its MATCH_PARENT-width Label is resolved at 480, not at R's measured width 100: R, the
// Label and the stack keep H(480). A container that sized itself from its own measured width would
// have been H(100) tall -- too tall for the 480 it is actually given.
int UtcDaliViewHeightForWidthStackFillDoesNotOvergrowP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL);
  stack.SetRequestedWidth(480.0f); // WRAP_CONTENT height
  View r = View::New();            // WRAP_CONTENT x WRAP_CONTENT
  r.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::FILL));
  View  seed  = CreateSeed();
  Label label = CreateWrappingLabel(MATCH_PARENT);
  r.Add(seed);
  r.Add(label);
  stack.Add(r);
  window.Add(stack);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 > h480);

  DALI_TEST_EQUALS(WidthOf(r), 480.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(r), h480, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 480.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h480, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), h480, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A fixed 480 x 600 vertical stack: R (WRAP_CONTENT x WRAP_CONTENT, holding a seed and a
// MATCH_PARENT-width Label) and a weight-1 sibling W. R is resolved at its placement width 100
// before the weight redistribution, so W's share shrinks by R's growth. Before: R was H(480) tall
// and W took 600 - H(480) from y H(480).
int UtcDaliViewHeightForWidthStackWeightP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL);
  stack.SetRequestedWidth(480.0f);
  stack.SetRequestedHeight(600.0f);
  View  r     = View::New(); // WRAP_CONTENT x WRAP_CONTENT
  View  seed  = CreateSeed();
  Label label = CreateWrappingLabel(MATCH_PARENT);
  r.Add(seed);
  r.Add(label);
  stack.Add(r);
  View w = View::New();
  w.SetRequestedWidth(480.0f);
  w.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f));
  stack.Add(w);
  window.Add(stack);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 > h480);
  DALI_TEST_CHECK(h100 < 600.0f);

  DALI_TEST_EQUALS(HeightOf(r), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(w), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(w), 600.0f - h100, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A WRAP_CONTENT x WRAP_CONTENT horizontal stack holding R (a seed and a MATCH_PARENT-width
// Label). The stack's cross axis is its height: its root slot resolves R at its measured width
// 100, and the cross-axis step of its arrange gives R that height. Before: the stack and R were
// H(480) tall.
int UtcDaliViewHeightForWidthHorizontalStackP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::HORIZONTAL); // WRAP_CONTENT x WRAP_CONTENT
  View        r     = View::New();                                    // WRAP_CONTENT x WRAP_CONTENT
  View        seed  = CreateSeed();
  Label       label = CreateWrappingLabel(MATCH_PARENT);
  r.Add(seed);
  r.Add(label);
  stack.Add(r);
  window.Add(stack);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 > h480);

  DALI_TEST_EQUALS(WidthOf(stack), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(r), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(r), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(r), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h100, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A 480-wide grid with five 96-wide columns and one AUTO row: four MATCH_PARENT-width Labels
// (vertical alignment START, CENTER, END, FILL) and a 96 x 760 view. Each Label is resolved at its
// cell width 96; the AUTO row and the cell placement use the same value, so the START/CENTER/END
// Labels are H(96) tall at their aligned offsets and the FILL Label fills the 760 row. Before: the
// grid re-measured the Labels at the whole 480 and placed the aligned ones H(480) tall.
int UtcDaliViewHeightForWidthGridCellAlignmentsP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  constexpr float CELL_WIDTH = 96.0f;
  constexpr float ROW_HEIGHT = 760.0f;

  GridLayout grid = GridLayout::New();
  grid.SetRequestedWidth(480.0f); // WRAP_CONTENT height
  Dali::Vector<GridLength> columns;
  for(uint32_t i = 0; i < 5u; ++i)
  {
    columns.PushBack(GridLength::Absolute(CELL_WIDTH));
  }
  Dali::Vector<GridLength> rows;
  rows.PushBack(GridLength::Auto());
  grid.SetColumnDefinitions(columns);
  grid.SetRowDefinitions(rows);

  const LayoutAlignment alignments[] = {LayoutAlignment::START, LayoutAlignment::CENTER, LayoutAlignment::END, LayoutAlignment::FILL};
  Label                 labels[4];
  for(uint32_t i = 0; i < 4u; ++i)
  {
    labels[i] = CreateWrappingLabel(MATCH_PARENT);
    labels[i].SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(i).SetVerticalAlignment(alignments[i]));
    grid.Add(labels[i]);
  }
  View tall = View::New();
  tall.SetRequestedWidth(CELL_WIDTH);
  tall.SetRequestedHeight(ROW_HEIGHT);
  tall.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(4));
  grid.Add(tall);
  window.Add(grid);

  SettleLayout(application);

  const float h96  = labels[0].GetHeightForWidth(CELL_WIDTH);
  const float h480 = labels[0].GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h96 > h480);
  DALI_TEST_CHECK(h96 < ROW_HEIGHT);

  DALI_TEST_EQUALS(HeightOf(grid), ROW_HEIGHT, HFW_EPSILON, TEST_LOCATION);
  for(uint32_t i = 0; i < 4u; ++i)
  {
    DALI_TEST_EQUALS(WidthOf(labels[i]), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  }

  DALI_TEST_EQUALS(PositionYOf(labels[0]), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(labels[0]), h96, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(labels[1]), (ROW_HEIGHT - h96) * 0.5f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(labels[1]), h96, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(labels[2]), ROW_HEIGHT - h96, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(labels[2]), h96, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(labels[3]), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(labels[3]), ROW_HEIGHT, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A 480-wide grid with one 120-wide column and two AUTO rows: L1 (WRAP_CONTENT width) and L2
// (fixed 240 width). Both are wider than the cell, so the cell keeps its width and both Labels are
// resolved at 120, whatever their own width mode. Before: L1 was laid out 120 wide at the height
// of its measured width and L2 at H(240), both clipped.
int UtcDaliViewHeightForWidthGridNarrowCellP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  constexpr float CELL_WIDTH  = 120.0f;
  constexpr float FIXED_WIDTH = 240.0f;

  GridLayout grid = GridLayout::New();
  grid.SetRequestedWidth(480.0f); // WRAP_CONTENT height
  Dali::Vector<GridLength> columns;
  columns.PushBack(GridLength::Absolute(CELL_WIDTH));
  Dali::Vector<GridLength> rows;
  rows.PushBack(GridLength::Auto());
  rows.PushBack(GridLength::Auto());
  grid.SetColumnDefinitions(columns);
  grid.SetRowDefinitions(rows);

  Label l1 = CreateWrappingLabel(WRAP_CONTENT);
  l1.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(0));
  Label l2 = CreateWrappingLabel(FIXED_WIDTH);
  l2.SetLayoutParams(GridLayoutParams::New().SetRow(1).SetColumn(0));
  grid.Add(l1);
  grid.Add(l2);
  window.Add(grid);

  SettleLayout(application);

  const float l1MeasuredWidth = l1.GetMeasuredSize().width;
  const float h1Cell          = l1.GetHeightForWidth(CELL_WIDTH);
  const float h2Cell          = l2.GetHeightForWidth(CELL_WIDTH);
  DALI_TEST_CHECK(l1MeasuredWidth > CELL_WIDTH);
  DALI_TEST_CHECK(h1Cell > l1.GetHeightForWidth(l1MeasuredWidth));
  DALI_TEST_CHECK(h2Cell > l2.GetHeightForWidth(FIXED_WIDTH));

  DALI_TEST_EQUALS(WidthOf(l1), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(l1), h1Cell, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(l1), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(l2), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(l2), h2Cell, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(l2), h1Cell, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(grid), h1Cell + h2Cell, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The width a child is laid out at is its parent's decision, so the child's own minimum/maximum
// WIDTH does not enter the resolution (only its height limits do):
//  (a) a MATCH_PARENT-width Label with maximum width 200 in a 480-wide grid cell is laid out 480
//      wide and resolved at 480. Before: it was measured at the clamped 200 and laid out H(200)
//      tall, too tall for its 480 width.
//  (b) a MATCH_PARENT-width Label with minimum width 300 in a 100-wide stack is laid out 100 wide
//      and resolved at 100. Before: it was measured at the clamped 300 and laid out H(300) tall,
//      clipping the text.
int UtcDaliViewHeightForWidthIgnoresChildWidthLimitsP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  GridLayout grid = GridLayout::New();
  grid.SetRequestedWidth(480.0f); // WRAP_CONTENT height
  Dali::Vector<GridLength> columns;
  columns.PushBack(GridLength::Absolute(480.0f));
  Dali::Vector<GridLength> rows;
  rows.PushBack(GridLength::Auto());
  grid.SetColumnDefinitions(columns);
  grid.SetRowDefinitions(rows);
  Label capped = CreateWrappingLabel(MATCH_PARENT);
  capped.SetMaximumWidth(200.0f);
  grid.Add(capped);
  window.Add(grid);

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL);
  stack.SetRequestedWidth(100.0f); // WRAP_CONTENT height
  Label floored = CreateWrappingLabel(MATCH_PARENT);
  floored.SetMinimumWidth(300.0f);
  stack.Add(floored);
  window.Add(stack);

  SettleLayout(application);

  // (a)
  const float cappedH200 = capped.GetHeightForWidth(200.0f);
  const float cappedH480 = capped.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(cappedH200 > cappedH480);
  DALI_TEST_EQUALS(WidthOf(capped), 480.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(capped), cappedH480, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(grid), cappedH480, HFW_EPSILON, TEST_LOCATION);

  // (b)
  const float flooredH100 = floored.GetHeightForWidth(100.0f);
  const float flooredH300 = floored.GetHeightForWidth(300.0f);
  DALI_TEST_CHECK(flooredH100 > flooredH300);
  DALI_TEST_EQUALS(WidthOf(floored), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(floored), flooredH100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), flooredH100, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A 300-wide, WRAP_CONTENT-height, non-wrapping row FlexLayout (alignItems STRETCH) holding a
// WRAP_CONTENT Label wider than 300 and a 100 x 20 view. Shrink (factor 1 each, weighted by main
// size) takes the Label from 300 to 225 and the view from 100 to 75; the Label is resolved at its
// shrunk width 225, and the line -- and the flex -- follow. Before: the Label was laid out 225 wide
// at H(300), clipping the text.
int UtcDaliViewHeightForWidthFlexRowShrinkP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  FlexLayout flex = FlexLayout::New();
  flex.SetRequestedWidth(300.0f); // WRAP_CONTENT height
  flex.SetDirection(FlexDirection::ROW);
  flex.SetWrap(FlexWrap::NO_WRAP);
  flex.SetAlignItems(FlexAlign::STRETCH);
  Label label = CreateWrappingLabel(WRAP_CONTENT);
  View  box   = View::New();
  box.SetRequestedWidth(100.0f);
  box.SetRequestedHeight(20.0f);
  flex.Add(label);
  flex.Add(box);
  window.Add(flex);

  SettleLayout(application);

  const float h225 = label.GetHeightForWidth(225.0f);
  const float h300 = label.GetHeightForWidth(300.0f);
  DALI_TEST_CHECK(label.GetNaturalSize().width > 300.0f);
  DALI_TEST_CHECK(h225 > h300);

  DALI_TEST_EQUALS(HeightOf(flex), h225, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 225.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h225, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(box), 75.0f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// Fully deterministic (no text): P (WRAP_CONTENT x WRAP_CONTENT) <- [seed, F], F a MATCH_PARENT-
// width, WRAP_CONTENT-height, wrapping row FlexLayout holding two 60 x 40 views and a 60 x 40 leaf
// U whose producer is a counted MeasureCallback. F is measured at 480 (one 40-tall line) but laid
// out 100 wide, where its items break into three lines: a wrapping row depends on its width by
// itself, so P's root slot and P's arrange give F 120 and the lines fit. U is never looked into.
//
// The resolution itself never measures: U's producer runs exactly as often as before this change
// -- once when F is measured at 480 and once when P's arrange re-measures F at its 100 x 120 slot
// (MATCH_PARENT). An arrange-only pass re-measures nothing (the slot re-measure hits its cache),
// and a cold pass costs the same two producer calls again. Before: P and F were 40 tall (the three
// lines overflowed F), with the same 2 / 2 / 4 producer counts.
int UtcDaliViewHeightForWidthFlexRowWrapAndProducerCountP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  gCountingBoxMeasureCount = 0;

  View       p    = View::New(); // WRAP_CONTENT x WRAP_CONTENT
  View       seed = CreateSeed();
  FlexLayout flex = FlexLayout::New();
  flex.SetRequestedWidth(MATCH_PARENT); // WRAP_CONTENT height
  flex.SetDirection(FlexDirection::ROW);
  flex.SetWrap(FlexWrap::WRAP);
  View b1 = View::New();
  b1.SetRequestedWidth(60.0f);
  b1.SetRequestedHeight(40.0f);
  View b2 = View::New();
  b2.SetRequestedWidth(60.0f);
  b2.SetRequestedHeight(40.0f);
  View u = View::New();
  u.SetMeasureCallback(MeasureCallback::New(&CountingBoxMeasure));
  flex.Add(b1);
  flex.Add(b2);
  flex.Add(u);
  p.Add(seed);
  p.Add(flex);
  window.Add(p);

  auto checkGeometry = [&](const char* location)
  {
    DALI_TEST_EQUALS(WidthOf(p), 100.0f, HFW_EPSILON, location);
    DALI_TEST_EQUALS(HeightOf(p), 120.0f, HFW_EPSILON, location);
    DALI_TEST_EQUALS(WidthOf(flex), 100.0f, HFW_EPSILON, location);
    DALI_TEST_EQUALS(HeightOf(flex), 120.0f, HFW_EPSILON, location);
    DALI_TEST_EQUALS(PositionYOf(b1), 0.0f, HFW_EPSILON, location);
    DALI_TEST_EQUALS(PositionYOf(b2), 40.0f, HFW_EPSILON, location);
    DALI_TEST_EQUALS(PositionYOf(u), 80.0f, HFW_EPSILON, location);
  };

  SettleLayout(application);
  checkGeometry(TEST_LOCATION);
  DALI_TEST_EQUALS(gCountingBoxMeasureCount, 2, TEST_LOCATION);

  // Arrange only: every derivation re-runs, nothing is measured again.
  p.InvalidateArrange();
  SettleLayout(application);
  checkGeometry(TEST_LOCATION);
  DALI_TEST_EQUALS(gCountingBoxMeasureCount, 2, TEST_LOCATION);

  // A cold pass: F misses at 480 (U #3) and again at its slot (U #4), as before this change.
  p.InvalidateMeasure();
  SettleLayout(application);
  checkGeometry(TEST_LOCATION);
  DALI_TEST_EQUALS(gCountingBoxMeasureCount, 4, TEST_LOCATION);

  END_TEST;
}

// A WRAP_CONTENT x WRAP_CONTENT AbsoluteLayout holding a seed and a WRAP_CONTENT-height Label
// whose width is proportional (1.0 of the container). The proportional width does not feed the
// container's WRAP_CONTENT width, which the seed sets to 100, and the Label is resolved at that
// 100. Before: the Label was measured 480 wide and laid out 100 wide at that height.
int UtcDaliViewHeightForWidthAbsoluteProportionalWidthP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  AbsoluteLayout absolute = AbsoluteLayout::New(); // WRAP_CONTENT x WRAP_CONTENT
  View           seed     = CreateSeed();
  Label          label    = CreateWrappingLabel(WRAP_CONTENT);
  label.SetLayoutParams(AbsoluteLayoutParams::New()
                          .SetBounds(LayoutRect(0.0f, 0.0f, 1.0f, -1.0f))
                          .SetFlags(AbsoluteLayoutFlags::WIDTH_PROPORTIONAL));
  absolute.Add(seed);
  absolute.Add(label);
  window.Add(absolute);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 > h480);

  DALI_TEST_EQUALS(WidthOf(absolute), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(absolute), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h100, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A 300 x 300 vertical ScrollView whose content is a MATCH_PARENT-width, WRAP_CONTENT-height
// vertical stack holding a row (seed + MATCH_PARENT-width Label). The ScrollView resolves its
// content at its 300 slot width with an unbounded budget; the content's arrange then resolves the
// row at 100 and the row's arrange the Label. Before: the content was H(300) tall and the row and
// the Label were laid out at that height, 100 wide.
int UtcDaliViewHeightForWidthScrollViewContentP(void)
{
  UiTestApplication application;

  View root = View::New();
  root.SetRequestedWidth(400.0f);
  root.SetRequestedHeight(400.0f);
  application.GetScene().Add(root);

  ScrollView scrollView = ScrollView::New();
  scrollView.SetScrollDirection(ScrollDirection::Vertical);
  scrollView.SetVerticalScrollBarVisibility(ScrollBarVisibility::Never);
  scrollView.SetHorizontalScrollBarVisibility(ScrollBarVisibility::Never);
  scrollView.SetRequestedWidth(300.0f);
  scrollView.SetRequestedHeight(300.0f);
  root.Add(scrollView);

  StackLayout content = StackLayout::New(StackOrientation::VERTICAL);
  content.SetRequestedWidth(MATCH_PARENT); // WRAP_CONTENT height
  View  row   = View::New();               // WRAP_CONTENT x WRAP_CONTENT
  View  seed  = CreateSeed();
  Label label = CreateWrappingLabel(MATCH_PARENT);
  row.Add(seed);
  row.Add(label);
  content.Add(row);
  scrollView.SetContent(content);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h300 = label.GetHeightForWidth(300.0f);
  DALI_TEST_CHECK(h100 > h300);

  DALI_TEST_EQUALS(WidthOf(content), 300.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(content), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(row), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(row), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h100, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A WRAP_CONTENT x WRAP_CONTENT STANDALONE view R (seed + MATCH_PARENT-width Label) in a fixed
// 300 x 400 parent. R is measured against the window (480 x 800) and its slot is 100 wide, where
// the shared slot step resolves it: R and the Label get H(100). Both paths that derive R's slot --
// the parent's placement and R's own layout-root pass -- run the same step on the same snapshot
// and constraint, so the result holds after each of them alone. Before: R and the Label were
// H(480) tall.
int UtcDaliViewHeightForWidthStandaloneChildP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  View p = View::New();
  p.SetRequestedWidth(300.0f);
  p.SetRequestedHeight(400.0f);
  View r = View::New(); // WRAP_CONTENT x WRAP_CONTENT
  r.SetLayoutMode(LayoutMode::STANDALONE);
  View  seed  = CreateSeed();
  Label label = CreateWrappingLabel(MATCH_PARENT);
  r.Add(seed);
  r.Add(label);
  p.Add(r);
  window.Add(p);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 > h480);

  auto checkGeometry = [&](const char* location)
  {
    DALI_TEST_EQUALS(WidthOf(r), 100.0f, HFW_EPSILON, location);
    DALI_TEST_EQUALS(HeightOf(r), h100, HFW_EPSILON, location);
    DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, location);
    DALI_TEST_EQUALS(HeightOf(label), h100, HFW_EPSILON, location);
  };
  checkGeometry(TEST_LOCATION);

  // The parent's placement alone.
  p.InvalidateArrange();
  SettleLayout(application);
  checkGeometry(TEST_LOCATION);

  // R's own layout-root pass alone.
  r.InvalidateMeasure();
  SettleLayout(application);
  checkGeometry(TEST_LOCATION);

  END_TEST;
}

// Under a custom parent (an ArrangeCallback that arranges its children at their measured sizes)
// nothing above R is resolved, so R is given its measured height H(480). R's own arrange still
// resolves the Label at 100, but the Label's budget is R's height, so the clamp keeps the Label at
// H(480): R and the Label are exactly as before this change.
int UtcDaliViewHeightForWidthCustomParentKeepsMeasuredHeightP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  View p = View::New(); // WRAP_CONTENT x WRAP_CONTENT
  p.SetArrangeCallback(ArrangeCallback::New(&ArrangeChildrenAtMeasuredSize));
  View  r     = View::New(); // WRAP_CONTENT x WRAP_CONTENT
  View  seed  = CreateSeed();
  Label label = CreateWrappingLabel(MATCH_PARENT);
  r.Add(seed);
  r.Add(label);
  p.Add(r);
  window.Add(p);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 > h480);

  DALI_TEST_EQUALS(WidthOf(r), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(r), h480, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h480, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The fit fallback: a fixed-height vertical stack holding R (seed + MATCH_PARENT-width Label) and a
// 480 x 100 view S. With room to spare (800) the resolved layout fits and R gets H(100). Squeezed
// to H(480) + 100, the resolved run min(H(100), height) + 100 no longer fits, so the stack arranges
// exactly as before this change: R and the Label keep H(480) and S follows at H(480).
int UtcDaliViewHeightForWidthDoesNotOverflowFixedContainerP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL);
  stack.SetRequestedWidth(480.0f);
  stack.SetRequestedHeight(800.0f);
  View  r     = View::New(); // WRAP_CONTENT x WRAP_CONTENT
  View  seed  = CreateSeed();
  Label label = CreateWrappingLabel(MATCH_PARENT);
  r.Add(seed);
  r.Add(label);
  stack.Add(r);
  View s = View::New();
  s.SetRequestedWidth(480.0f);
  s.SetRequestedHeight(100.0f);
  stack.Add(s);
  window.Add(stack);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 > h480);
  DALI_TEST_CHECK(h100 + 100.0f <= 800.0f);

  // It fits: the resolved height is adopted.
  DALI_TEST_EQUALS(HeightOf(r), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(s), h100, HFW_EPSILON, TEST_LOCATION);

  const float tightHeight = h480 + 100.0f;
  DALI_TEST_CHECK(std::min(h100, tightHeight) + 100.0f > tightHeight);
  stack.SetRequestedHeight(tightHeight);

  SettleLayout(application);

  DALI_TEST_EQUALS(HeightOf(r), h480, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h480, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(s), h480, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A text-fit Label is not resolved (its measure runs under a temporarily changed font
// configuration that a metric query cannot reproduce): the WRAP_CONTENT x WRAP_CONTENT stack keeps
// its measured height and the Label keeps the slot the stack measured for it, although the stack
// is laid out 100 wide. A reference Label without text fit shows the content wraps differently at
// that width, so a resolution would have changed the layout.
int UtcDaliViewHeightForWidthTextFitLabelUnresolvedP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL); // WRAP_CONTENT x WRAP_CONTENT
  View        seed  = CreateSeed();
  Label       label = CreateWrappingLabel(MATCH_PARENT);
  label.SetTextFit(Text::Fit::Range(8.0f, 40.0f, 4.0f));
  stack.Add(seed);
  stack.Add(label);
  window.Add(stack);

  Label reference = CreateWrappingLabel(WRAP_CONTENT);
  window.Add(reference);

  SettleLayout(application);

  DALI_TEST_CHECK(reference.GetHeightForWidth(100.0f) > reference.GetHeightForWidth(480.0f));
  DALI_TEST_EQUALS(WidthOf(stack), 100.0f, HFW_EPSILON, TEST_LOCATION);

  DALI_TEST_EQUALS(HeightOf(stack), stack.GetMeasuredSize().height, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), HeightOf(stack), HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A Label in a slot that is 0 wide (its only parent is WRAP_CONTENT and the Label is
// MATCH_PARENT-wide, contributing no width):
//  (a) a single-line Label's height does not follow its width: it is not resolved, and the parent
//      and the Label keep the measured line height.
//  (b) a multi-line Label resolved at width 0 has no text width and reports 0, as its own measure
//      does at width 0: the parent and the Label collapse to 0. Before: both were H(480) tall.
int UtcDaliViewHeightForWidthSingleLineAndZeroWidthP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  View  p1 = View::New(); // WRAP_CONTENT x WRAP_CONTENT
  Label l1 = Label::New(WRAPPING_TEXT);
  l1.SetRequestedWidth(MATCH_PARENT); // single-line by default
  p1.Add(l1);
  window.Add(p1);

  View  p2 = View::New(); // WRAP_CONTENT x WRAP_CONTENT
  Label l2 = CreateWrappingLabel(MATCH_PARENT);
  p2.Add(l2);
  window.Add(p2);

  SettleLayout(application);

  // (a)
  DALI_TEST_CHECK(!l1.IsMultiLine());
  DALI_TEST_EQUALS(WidthOf(p1), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_CHECK(HeightOf(p1) > 0.0f);
  DALI_TEST_EQUALS(HeightOf(l1), HeightOf(p1), HFW_EPSILON, TEST_LOCATION);

  // (b)
  DALI_TEST_CHECK(l2.GetHeightForWidth(480.0f) > 0.0f);
  DALI_TEST_EQUALS(WidthOf(p2), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(p2), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(l2), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(l2), 0.0f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A WRAP_CONTENT x WRAP_CONTENT View P holding a 100-wide seed and F, a MATCH_PARENT-width,
// WRAP_CONTENT-height, non-wrapping COLUMN FlexLayout (alignItems STRETCH by default) with a
// MATCH_PARENT-width Label and a 50 x 30 view. F is measured 480 wide, where the Label reports
// H(480). Laid out 100 wide, F's single line spans the whole 100 (a non-wrapping line is at least
// the available cross extent), so the Label is resolved at 100 before grow/shrink and F's main
// extent follows. Before: P and F were H(480) + 30 tall and the Label was laid out 100 wide at
// H(480).
int UtcDaliViewHeightForWidthFlexColumnNoWrapP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  View       p    = View::New(); // WRAP_CONTENT x WRAP_CONTENT
  View       seed = CreateSeed();
  FlexLayout flex = FlexLayout::New();
  flex.SetRequestedWidth(MATCH_PARENT); // WRAP_CONTENT height
  flex.SetDirection(FlexDirection::COLUMN);
  flex.SetWrap(FlexWrap::NO_WRAP);
  Label label = CreateWrappingLabel(MATCH_PARENT);
  View  box   = View::New();
  box.SetRequestedWidth(50.0f);
  box.SetRequestedHeight(30.0f);
  flex.Add(label);
  flex.Add(box);
  p.Add(seed);
  p.Add(flex);
  window.Add(p);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 > h480);
  DALI_TEST_CHECK(h100 + 30.0f < 800.0f);

  DALI_TEST_EQUALS(WidthOf(p), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(p), h100 + 30.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(flex), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(flex), h100 + 30.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(box), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(box), 30.0f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A fixed 300 x 500 wrapping COLUMN FlexLayout with alignContent STRETCH (alignItems STRETCH by
// default) holding a 100 x 500 view and a MATCH_PARENT-width Label. The lines split once, at the
// measured heights: [view], [Label]. alignContent STRETCH shares the free cross extent equally, so
// the lines are 200 and 100 wide, and the Label is resolved at its line's 100 before grow/shrink.
// Before: the Label was placed in its 100-wide line at the height it measured 300 wide.
int UtcDaliViewHeightForWidthFlexColumnWrapStretchP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  FlexLayout flex = FlexLayout::New();
  flex.SetRequestedWidth(300.0f);
  flex.SetRequestedHeight(500.0f);
  flex.SetDirection(FlexDirection::COLUMN);
  flex.SetWrap(FlexWrap::WRAP);
  flex.SetAlignContent(FlexAlign::STRETCH);
  View tall = View::New();
  tall.SetRequestedWidth(100.0f);
  tall.SetRequestedHeight(500.0f);
  Label label = CreateWrappingLabel(MATCH_PARENT);
  flex.Add(tall);
  flex.Add(label);
  window.Add(flex);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h300 = label.GetHeightForWidth(300.0f);
  DALI_TEST_CHECK(h100 > h300);
  DALI_TEST_CHECK(h100 <= 500.0f);

  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionXOf(label), 200.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The visual-unit forms at UI scale 2: a WRAP_CONTENT x WRAP_CONTENT vertical stack with padding
// (10, 10, 5, 5) holding a 50 x 0 seed and a MATCH_PARENT-width Label with margin (4, 6, 3, 2). In
// visual units the padding is 20 / 20 / 10 / 10, the seed 100 x 0 and the Label's margin 8 / 12 /
// 6 / 4. The stack is measured against the window (content 440 wide; the Label at 420 reports
// H(420)) and laid out 140 wide: its content is 100 wide, so the Label is resolved at 100 - 20 = 80.
// Before: the stack was H(420) + 30 tall and the Label was laid out 80 wide at H(420).
int UtcDaliViewHeightForWidthScaledMarginsAndPaddingP(void)
{
  UiTestApplication application;

  const float originalScale = UiScaleManager::Get().GetScale();
  UiScaleManager::Get().SetScale(2.0f);

  Window window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL); // WRAP_CONTENT x WRAP_CONTENT
  stack.SetPadding(Insets(10.0f, 10.0f, 5.0f, 5.0f));
  View seed = View::New();
  seed.SetRequestedWidth(50.0f);
  seed.SetRequestedHeight(0.0f);
  Label label = CreateWrappingLabel(MATCH_PARENT);
  label.SetMargin(Insets(4.0f, 6.0f, 3.0f, 2.0f));
  stack.Add(seed);
  stack.Add(label);
  window.Add(stack);

  SettleLayout(application);

  // Read at scale 2, after settling: visual units, as the layout queries them.
  const float h80  = label.GetHeightForWidth(80.0f);
  const float h420 = label.GetHeightForWidth(420.0f);
  DALI_TEST_CHECK(h80 > h420);
  DALI_TEST_CHECK(h80 + 30.0f < 800.0f);

  DALI_TEST_EQUALS(WidthOf(stack), 140.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), h80 + 30.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 80.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h80, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionXOf(label), 28.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), 16.0f, HFW_EPSILON, TEST_LOCATION);

  UiScaleManager::Get().SetScale(originalScale);
  END_TEST;
}

// A WRAP_CONTENT x WRAP_CONTENT vertical stack root holding a seed, a MATCH_PARENT-width Label and
// a fixed 100 x 40 weight-1 view. With a WRAP_CONTENT main axis and no minimum height the weight
// child is aggregated like the others (the weight branch of the measure), so the root slot is
// H(100) + 40 once the Label is resolved at 100; the arrange then gives the Label H(100) and the
// weight child the 40 that is left. Before: the stack was H(480) + 40 tall and the Label H(480).
int UtcDaliViewHeightForWidthStackWeightAtWrapRootP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL); // WRAP_CONTENT x WRAP_CONTENT
  View        seed  = CreateSeed();
  Label       label = CreateWrappingLabel(MATCH_PARENT);
  View        w     = View::New();
  w.SetRequestedWidth(100.0f);
  w.SetRequestedHeight(40.0f);
  w.SetLayoutParams(StackLayoutParams::New().SetWeight(1.0f));
  stack.Add(seed);
  stack.Add(label);
  stack.Add(w);
  window.Add(stack);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 > h480);
  DALI_TEST_CHECK(h100 + 40.0f < 800.0f);

  DALI_TEST_EQUALS(WidthOf(stack), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), h100 + 40.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(w), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(w), 40.0f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A 480-wide grid with two 240-wide columns and two AUTO rows: a MATCH_PARENT-width Label spanning
// both rows of the first column, and a 240 x 4 view in each row of the second. The span pass
// shares the Label's deficit over the two rows equally, so with the Label resolved at its cell width
// 240 each row is 4 + (H(240) - 8) / 2 and the grid is H(240) tall. Before: the rows were built from
// the Label's height measured 480 wide.
int UtcDaliViewHeightForWidthGridRowSpanP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  constexpr float COLUMN_WIDTH = 240.0f;
  constexpr float SIDE_HEIGHT  = 4.0f;

  GridLayout grid = GridLayout::New();
  grid.SetRequestedWidth(480.0f); // WRAP_CONTENT height
  Dali::Vector<GridLength> columns;
  columns.PushBack(GridLength::Absolute(COLUMN_WIDTH));
  columns.PushBack(GridLength::Absolute(COLUMN_WIDTH));
  Dali::Vector<GridLength> rows;
  rows.PushBack(GridLength::Auto());
  rows.PushBack(GridLength::Auto());
  grid.SetColumnDefinitions(columns);
  grid.SetRowDefinitions(rows);

  Label label = CreateWrappingLabel(MATCH_PARENT);
  label.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(0).SetRowSpan(2));
  View a = View::New();
  a.SetRequestedWidth(COLUMN_WIDTH);
  a.SetRequestedHeight(SIDE_HEIGHT);
  a.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(1));
  View b = View::New();
  b.SetRequestedWidth(COLUMN_WIDTH);
  b.SetRequestedHeight(SIDE_HEIGHT);
  b.SetLayoutParams(GridLayoutParams::New().SetRow(1).SetColumn(1));
  grid.Add(label);
  grid.Add(a);
  grid.Add(b);
  window.Add(grid);

  SettleLayout(application);

  const float h240 = label.GetHeightForWidth(COLUMN_WIDTH);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h240 > h480);
  DALI_TEST_CHECK(h240 > 2.0f * SIDE_HEIGHT);
  DALI_TEST_CHECK(h240 < 800.0f);

  DALI_TEST_EQUALS(HeightOf(grid), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), COLUMN_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(b), SIDE_HEIGHT + (h240 - 2.0f * SIDE_HEIGHT) * 0.5f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A container never reports a height its own arrange would refuse. A WRAP_CONTENT x WRAP_CONTENT
// vertical stack (spacing 10) holding a seed and a MATCH_PARENT-width Label with 15 times the text:
// resolved at 100 the Label would need more than the window, so the stack's resolved content (the
// Label clamped at the 800 budget, plus the spacing) exceeds its budget and the stack keeps its
// measured height, with the Label arranged exactly as before. Without that guard the stack would
// be sized 810 tall around the Label still laid out at H(480).
int UtcDaliViewHeightForWidthOverBudgetKeepsMeasuredHeightP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  std::string longText;
  for(int i = 0; i < 15; ++i)
  {
    longText += WRAPPING_TEXT;
  }

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL); // WRAP_CONTENT x WRAP_CONTENT
  stack.SetSpacing(10.0f);
  View  seed  = CreateSeed();
  Label label = Label::New(longText.c_str());
  label.SetMultiLine(true);
  label.SetLineWrapMode(Text::LineWrapMode::CHARACTER);
  label.SetRequestedWidth(MATCH_PARENT);
  stack.Add(seed);
  stack.Add(label);
  window.Add(stack);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(h100 + 10.0f > 800.0f);
  DALI_TEST_CHECK(h480 + 10.0f < 800.0f);

  DALI_TEST_EQUALS(HeightOf(stack), stack.GetMeasuredSize().height, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), h480 + 10.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h480, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), 10.0f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The card shape: a 480-wide grid with two 240-wide columns and an AUTO row holding, in the first
// cell, a MATCH_PARENT-width, WRAP_CONTENT-height vertical stack (the card) with a WRAP_CONTENT
// Label wider than the cell. The grid will re-measure the card at its 240 cell before arranging it,
// which re-measures the Label at 240; the card's resolution therefore takes the Label at that
// width, so the row, the card and the Label are H(240) tall. Before: they kept the height of the
// Label's measured width, clipping the text.
int UtcDaliViewHeightForWidthCardInNarrowCellP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  constexpr float CELL_WIDTH = 240.0f;

  GridLayout grid = GridLayout::New();
  grid.SetRequestedWidth(480.0f); // WRAP_CONTENT height
  Dali::Vector<GridLength> columns;
  columns.PushBack(GridLength::Absolute(CELL_WIDTH));
  columns.PushBack(GridLength::Absolute(CELL_WIDTH));
  Dali::Vector<GridLength> rows;
  rows.PushBack(GridLength::Auto());
  grid.SetColumnDefinitions(columns);
  grid.SetRowDefinitions(rows);

  StackLayout card = StackLayout::New(StackOrientation::VERTICAL);
  card.SetRequestedWidth(MATCH_PARENT); // WRAP_CONTENT height
  card.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(0));
  Label label = CreateWrappingLabel(WRAP_CONTENT);
  card.Add(label);
  grid.Add(card);
  window.Add(grid);

  SettleLayout(application);

  const float natural = label.GetNaturalSize().width;
  const float h240    = label.GetHeightForWidth(CELL_WIDTH);
  DALI_TEST_CHECK(natural > CELL_WIDTH);
  DALI_TEST_CHECK(h240 > label.GetHeightForWidth(std::min(natural, 480.0f)));

  DALI_TEST_EQUALS(HeightOf(grid), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(card), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(card), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionXOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The card of UtcDaliViewHeightForWidthCardInNarrowCellP with a minimum width of 300, in a 120-wide
// cell. Measure() clamps the width the grid's cell re-measure hands the card by the card's own
// minimum, so the card is re-measured 300 wide and its Label becomes 300 wide; the card's resolution
// predicts exactly that (the clamped constraint, not the 120 slot). The card itself is arranged at
// its 120 cell (an arranged width is never clamped) with the Label overflowing it, and the row,
// the card and the Label are H(300) tall. Predicting from the slot would size the row and the card
// H(120) tall around the H(300) Label.
int UtcDaliViewHeightForWidthCardMinimumWidthP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  constexpr float CELL_WIDTH    = 120.0f;
  constexpr float MINIMUM_WIDTH = 300.0f;

  GridLayout  grid = CreateTwoColumnGrid(CELL_WIDTH, 480.0f - CELL_WIDTH);
  StackLayout card = StackLayout::New(StackOrientation::VERTICAL);
  card.SetRequestedWidth(MATCH_PARENT); // WRAP_CONTENT height
  card.SetMinimumWidth(MINIMUM_WIDTH);
  card.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(0));
  Label label = CreateWrappingLabel(WRAP_CONTENT);
  card.Add(label);
  grid.Add(card);
  window.Add(grid);

  SettleLayout(application);

  const float natural = label.GetNaturalSize().width;
  const float h300    = label.GetHeightForWidth(MINIMUM_WIDTH);
  DALI_TEST_CHECK(natural > MINIMUM_WIDTH);
  DALI_TEST_CHECK(h300 > label.GetHeightForWidth(480.0f));
  DALI_TEST_CHECK(label.GetHeightForWidth(CELL_WIDTH) > h300);

  DALI_TEST_EQUALS(HeightOf(grid), h300, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(card), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(card), h300, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(label.GetMeasuredSize().width, MINIMUM_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), MINIMUM_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h300, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The card shape with a plain View: a MATCH_PARENT-width, WRAP_CONTENT-height View in a 240-wide
// cell holding a WRAP_CONTENT Label wider than the cell. The View's resolution takes the Label at the
// width the cell re-measure gives it, 240. Before: the row, the View and the Label kept the height
// of the Label's measured width.
int UtcDaliViewHeightForWidthDefaultCardInNarrowCellP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  constexpr float CELL_WIDTH = 240.0f;

  GridLayout grid = CreateTwoColumnGrid(CELL_WIDTH, CELL_WIDTH);
  View       card = View::New();
  card.SetRequestedWidth(MATCH_PARENT); // WRAP_CONTENT height
  card.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(0));
  Label label = CreateWrappingLabel(WRAP_CONTENT);
  card.Add(label);
  grid.Add(card);
  window.Add(grid);

  SettleLayout(application);

  const float natural = label.GetNaturalSize().width;
  const float h240    = label.GetHeightForWidth(CELL_WIDTH);
  DALI_TEST_CHECK(natural > CELL_WIDTH);
  DALI_TEST_CHECK(h240 > label.GetHeightForWidth(std::min(natural, 480.0f)));

  DALI_TEST_EQUALS(HeightOf(grid), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(card), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(card), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h240, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The card shape with a horizontal StackLayout: a MATCH_PARENT-width, WRAP_CONTENT-height
// horizontal stack in a 240-wide cell holding a WRAP_CONTENT Label wider than the cell. The stack's
// resolution takes the Label at the width the cell re-measure gives it, 240, on its cross axis.
// Before: the row, the stack and the Label kept the height of the Label's measured width.
int UtcDaliViewHeightForWidthHorizontalStackCardInNarrowCellP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  constexpr float CELL_WIDTH = 240.0f;

  GridLayout  grid = CreateTwoColumnGrid(CELL_WIDTH, CELL_WIDTH);
  StackLayout card = StackLayout::New(StackOrientation::HORIZONTAL);
  card.SetRequestedWidth(MATCH_PARENT); // WRAP_CONTENT height
  card.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(0));
  Label label = CreateWrappingLabel(WRAP_CONTENT);
  card.Add(label);
  grid.Add(card);
  window.Add(grid);

  SettleLayout(application);

  const float natural = label.GetNaturalSize().width;
  const float h240    = label.GetHeightForWidth(CELL_WIDTH);
  DALI_TEST_CHECK(natural > CELL_WIDTH);
  DALI_TEST_CHECK(h240 > label.GetHeightForWidth(std::min(natural, 480.0f)));

  DALI_TEST_EQUALS(HeightOf(grid), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(card), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(card), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The card shape with an AbsoluteLayout: a MATCH_PARENT-width, WRAP_CONTENT-height AbsoluteLayout in
// a 240-wide cell holding a WRAP_CONTENT Label (default params, so its width is its own) wider than
// the cell. The layout's resolution takes the Label at the width the cell re-measure gives it, 240.
// Before: the row, the layout and the Label kept the height of the Label's measured width.
int UtcDaliViewHeightForWidthAbsoluteCardInNarrowCellP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  constexpr float CELL_WIDTH = 240.0f;

  GridLayout     grid = CreateTwoColumnGrid(CELL_WIDTH, CELL_WIDTH);
  AbsoluteLayout card = AbsoluteLayout::New();
  card.SetRequestedWidth(MATCH_PARENT); // WRAP_CONTENT height
  card.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(0));
  Label label = CreateWrappingLabel(WRAP_CONTENT);
  card.Add(label);
  grid.Add(card);
  window.Add(grid);

  SettleLayout(application);

  const float natural = label.GetNaturalSize().width;
  const float h240    = label.GetHeightForWidth(CELL_WIDTH);
  DALI_TEST_CHECK(natural > CELL_WIDTH);
  DALI_TEST_CHECK(h240 > label.GetHeightForWidth(std::min(natural, 480.0f)));

  DALI_TEST_EQUALS(HeightOf(grid), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(card), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(card), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionXOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The card shape with a COLUMN FlexLayout: a MATCH_PARENT-width, WRAP_CONTENT-height, non-wrapping
// column flex with alignItems FLEX_START (so the Label keeps its own width, not the line's) in a
// 240-wide cell holding a WRAP_CONTENT Label wider than the cell. The flex's resolution takes the
// Label at the width the cell re-measure gives it, 240. Before: the row, the flex and the Label kept
// the height of the Label's measured width.
int UtcDaliViewHeightForWidthFlexColumnCardInNarrowCellP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  constexpr float CELL_WIDTH = 240.0f;

  GridLayout grid = CreateTwoColumnGrid(CELL_WIDTH, CELL_WIDTH);
  FlexLayout card = FlexLayout::New();
  card.SetRequestedWidth(MATCH_PARENT); // WRAP_CONTENT height
  card.SetDirection(FlexDirection::COLUMN);
  card.SetWrap(FlexWrap::NO_WRAP);
  card.SetAlignItems(FlexAlign::FLEX_START);
  card.SetLayoutParams(GridLayoutParams::New().SetRow(0).SetColumn(0));
  Label label = CreateWrappingLabel(WRAP_CONTENT);
  card.Add(label);
  grid.Add(card);
  window.Add(grid);

  SettleLayout(application);

  const float natural = label.GetNaturalSize().width;
  const float h240    = label.GetHeightForWidth(CELL_WIDTH);
  DALI_TEST_CHECK(natural > CELL_WIDTH);
  DALI_TEST_CHECK(h240 > label.GetHeightForWidth(std::min(natural, 480.0f)));

  DALI_TEST_EQUALS(HeightOf(grid), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(card), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(card), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), CELL_WIDTH, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h240, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionXOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), 0.0f, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The over-budget guard of a plain View: a WRAP_CONTENT x WRAP_CONTENT View root holding a seed and a
// MATCH_PARENT-width Label placed at requested y 760. Resolved at 100 the Label's bottom would be
// 760 + H(100), beyond the 800 budget, so the root is not resolved: it keeps its measured height
// 760 + H(480), and the Label is arranged exactly as before. Without the guard the root would be
// sized to the 800 budget around the Label still laid out at H(480).
int UtcDaliViewHeightForWidthDefaultOverBudgetKeepsMeasuredHeightP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  constexpr float LABEL_Y = 760.0f;

  View  root  = View::New(); // WRAP_CONTENT x WRAP_CONTENT
  View  seed  = CreateSeed();
  Label label = CreateWrappingLabel(MATCH_PARENT);
  label.SetRequestedY(LABEL_Y);
  root.Add(seed);
  root.Add(label);
  window.Add(root);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h480 = label.GetHeightForWidth(480.0f);
  DALI_TEST_CHECK(LABEL_Y + h100 > 800.0f);
  DALI_TEST_CHECK(LABEL_Y + h480 < 800.0f);

  DALI_TEST_EQUALS(HeightOf(root), root.GetMeasuredSize().height, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(root), LABEL_Y + h480, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h480, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(label), LABEL_Y, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The text-height memo of a resolved Label keeps arrange-only passes stable: the root stack of
// UtcDaliViewHeightForWidthRootStackP (a 100-wide seed and a MATCH_PARENT-width Label) keeps the
// Label's height at 100 across repeated arrange-only passes, each served from the memo.
int UtcDaliViewHeightForWidthLabelMemoStableAcrossArrangePassesP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL); // WRAP_CONTENT x WRAP_CONTENT
  View        seed  = CreateSeed();
  Label       label = CreateWrappingLabel(MATCH_PARENT);
  stack.Add(seed);
  stack.Add(label);
  window.Add(stack);

  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  DALI_TEST_CHECK(h100 > label.GetHeightForWidth(480.0f));

  for(int pass = 0; pass < 4; ++pass)
  {
    if(pass > 0)
    {
      stack.InvalidateArrange();
      SettleLayout(application);
    }
    DALI_TEST_EQUALS(WidthOf(stack), 100.0f, HFW_EPSILON, TEST_LOCATION);
    DALI_TEST_EQUALS(HeightOf(stack), h100, HFW_EPSILON, TEST_LOCATION);
    DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
    DALI_TEST_EQUALS(HeightOf(label), h100, HFW_EPSILON, TEST_LOCATION);
  }

  END_TEST;
}

// A text change invalidates the Label's layout, which drops its memo: after doubling the text the
// stack and the Label follow the new text height at 100.
int UtcDaliViewHeightForWidthLabelMemoDroppedOnTextChangeP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL); // WRAP_CONTENT x WRAP_CONTENT
  View        seed  = CreateSeed();
  Label       label = CreateWrappingLabel(MATCH_PARENT);
  stack.Add(seed);
  stack.Add(label);
  window.Add(stack);

  SettleLayout(application);
  const float h0 = label.GetHeightForWidth(100.0f);

  label.SetText((std::string(WRAPPING_TEXT) + WRAPPING_TEXT).c_str());
  SettleLayout(application);

  const float h1 = label.GetHeightForWidth(100.0f);
  DALI_TEST_CHECK(h1 > h0);
  DALI_TEST_CHECK(h1 < 800.0f);

  DALI_TEST_EQUALS(WidthOf(stack), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), h1, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h1, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The memo holds the height of one width: when the seed widens the stack to 200 and narrows it back
// to 100, the Label -- never invalidated itself -- follows each width (the memo misses on the width).
int UtcDaliViewHeightForWidthLabelMemoFollowsWidthChangeP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL); // WRAP_CONTENT x WRAP_CONTENT
  View        seed  = CreateSeed();
  Label       label = CreateWrappingLabel(MATCH_PARENT);
  stack.Add(seed);
  stack.Add(label);
  window.Add(stack);

  SettleLayout(application);

  seed.SetRequestedWidth(200.0f);
  SettleLayout(application);

  const float h100 = label.GetHeightForWidth(100.0f);
  const float h200 = label.GetHeightForWidth(200.0f);
  DALI_TEST_CHECK(h100 > h200);

  DALI_TEST_EQUALS(WidthOf(stack), 200.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), h200, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 200.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h200, HFW_EPSILON, TEST_LOCATION);

  seed.SetRequestedWidth(100.0f);
  SettleLayout(application);

  DALI_TEST_EQUALS(WidthOf(stack), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), h100, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h100, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The letter spacing changes the text height without invalidating the layout. The memo keeps it as a
// key field, so the next resolution sees the change and computes the height again: after an
// arrange-only pass the stack and the Label take the Label's live height at 100.
int UtcDaliViewHeightForWidthLabelMemoLetterSpacingP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL); // WRAP_CONTENT x WRAP_CONTENT
  View        seed  = CreateSeed();
  Label       label = CreateWrappingLabel(MATCH_PARENT);
  stack.Add(seed);
  stack.Add(label);
  window.Add(stack);

  SettleLayout(application);
  const float h0 = label.GetHeightForWidth(100.0f);

  static_cast<Dali::Ui::Integration::LabelImpl&>(label.GetImplementation()).SetLetterSpacing(6.0f);
  const float h1 = label.GetHeightForWidth(100.0f);
  DALI_TEST_CHECK(h1 > h0);
  DALI_TEST_CHECK(h1 < 800.0f);

  stack.InvalidateArrange();
  SettleLayout(application);

  DALI_TEST_EQUALS(HeightOf(stack), h1, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h1, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The text outline is a layout input that does not invalidate the layout: its width moves the start
// of every line, so a 10-wide outline puts one glyph fewer on each line at 100. The memo keeps the
// outline width as a key field, so the next resolution sees the change. The stack is re-measured
// first, which re-measures the Label at 480 (the stack's arrange had re-measured it at its 100-wide
// slot), so the text controller's own height-for-width cache no longer holds the width 100: the root
// slot's resolution at 100 then takes the new text height. Without the outline key the memo would
// serve the old height.
int UtcDaliViewHeightForWidthLabelMemoOutlineP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL); // WRAP_CONTENT x WRAP_CONTENT
  View        seed  = CreateSeed();
  Label       label = CreateWrappingLabel(MATCH_PARENT);
  stack.Add(seed);
  stack.Add(label);
  window.Add(stack);

  SettleLayout(application);
  const float h0 = label.GetHeightForWidth(100.0f);

  Text::Outline outline;
  outline.SetWidth(10.0f);
  label.SetTextOutline(outline);
  stack.InvalidateMeasure();
  SettleLayout(application);

  const float h1 = label.GetHeightForWidth(100.0f);
  DALI_TEST_CHECK(h1 > h0);
  DALI_TEST_CHECK(h1 < 800.0f);

  DALI_TEST_EQUALS(WidthOf(stack), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), h1, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h1, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// A locale change drops a Label's font data without invalidating the layout; the memo keeps the
// locale generation as a key field. Smoke test: the observer connects at the first store, the
// emission bumps the generation, and the next resolution sees the mismatch and computes the height
// again. The harness font has locale-independent metrics, so the new height equals the old one and
// this case cannot tell a stale memo from a fresh one; it checks that the path runs and keeps the
// layout at the Label's live height at 100.
int UtcDaliViewHeightForWidthLabelMemoLocaleChangeP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL); // WRAP_CONTENT x WRAP_CONTENT
  View        seed  = CreateSeed();
  Label       label = CreateWrappingLabel(MATCH_PARENT);
  stack.Add(seed);
  stack.Add(label);
  window.Add(stack);

  SettleLayout(application);

  Dali::Adaptor::Get().LocaleChangedSignal().Emit("ko_KR");
  stack.InvalidateArrange();
  SettleLayout(application);

  const float h1 = label.GetHeightForWidth(100.0f);
  DALI_TEST_CHECK(h1 > 0.0f);
  DALI_TEST_CHECK(h1 < 800.0f);

  DALI_TEST_EQUALS(WidthOf(stack), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), h1, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(label), 100.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), h1, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The skip rule: three MATCH_PARENT-width Labels in a 300-wide, WRAP_CONTENT-height vertical stack are
// measured at their final width already, so their measured heights are their text heights. After the
// middle Label's text doubles, the stack's root slot takes its new measured height at 300 without
// laying the text out again, and the layout equals a full resolution: the Labels stack at their
// text heights.
int UtcDaliViewHeightForWidthFullWidthLabelsMatchTextHeightP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL);
  stack.SetRequestedWidth(300.0f); // WRAP_CONTENT height
  Label l1 = CreateWrappingLabel(MATCH_PARENT);
  Label l2 = CreateWrappingLabel(MATCH_PARENT);
  Label l3 = CreateWrappingLabel(MATCH_PARENT);
  stack.Add(l1);
  stack.Add(l2);
  stack.Add(l3);
  window.Add(stack);

  SettleLayout(application);

  const float h = l1.GetHeightForWidth(300.0f);
  DALI_TEST_CHECK(3.0f * h < 800.0f);

  DALI_TEST_EQUALS(WidthOf(l1), 300.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(l1), h, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(l1), 0.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(l2), h, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(l2), h, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(l3), h, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(l3), 2.0f * h, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(stack), 300.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), 3.0f * h, HFW_EPSILON, TEST_LOCATION);

  l2.SetText((std::string(WRAPPING_TEXT) + WRAPPING_TEXT).c_str());
  SettleLayout(application);

  const float h2 = l2.GetHeightForWidth(300.0f);
  DALI_TEST_CHECK(h2 > h);
  DALI_TEST_CHECK(2.0f * h + h2 < 800.0f);

  DALI_TEST_EQUALS(WidthOf(l2), 300.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(l2), h2, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(PositionYOf(l3), h + h2, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(WidthOf(stack), 300.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), 2.0f * h + h2, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}

// The trust bit: a fixed 300-wide multi-line Label in a 300-wide, WRAP_CONTENT-height vertical stack
// (not re-measured by the stack's arrange, so its last measure stays the one at 300 x 800). It is
// switched to single-line and re-measured -- its measured height becomes the single-line height -- and
// then switched back to multi-line, which does not invalidate the layout. The resolution that found
// it single-line distrusted its measured height, so the next resolution lays the text out again and
// the Label and the stack take the multi-line height. Trusting the measured height there would keep
// the single-line height.
int UtcDaliViewHeightForWidthLabelMultiLineSwitchAfterRemeasureP(void)
{
  UiTestApplication application;
  Window            window = application.GetWindow();

  StackLayout stack = StackLayout::New(StackOrientation::VERTICAL);
  stack.SetRequestedWidth(300.0f); // WRAP_CONTENT height
  Label label = CreateWrappingLabel(300.0f);
  stack.Add(label);
  window.Add(stack);

  SettleLayout(application);
  const float hm = label.GetHeightForWidth(300.0f);

  label.SetMultiLine(false);
  label.InvalidateMeasure();
  SettleLayout(application);

  const float hs = HeightOf(label);
  DALI_TEST_CHECK(hm > hs);
  DALI_TEST_EQUALS(HeightOf(stack), hs, HFW_EPSILON, TEST_LOCATION);

  label.SetMultiLine(true);
  stack.InvalidateArrange();
  SettleLayout(application);

  DALI_TEST_EQUALS(WidthOf(label), 300.0f, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(label), hm, HFW_EPSILON, TEST_LOCATION);
  DALI_TEST_EQUALS(HeightOf(stack), hm, HFW_EPSILON, TEST_LOCATION);

  END_TEST;
}
