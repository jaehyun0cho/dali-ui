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

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/group-selectable-trait.h>
#include <dali-ui-foundation/public-api/selectable-group.h>

#include <cstdio>
#include <vector>

using namespace Dali;
using namespace Dali::Ui;
using Dali::Ui::View;

namespace
{
const UiColor UNSELECTED_COLOR(0xE0E0E0); ///< Light grey for an unselected option
const UiColor SELECTED_COLOR(0x2962FF);   ///< Blue for the selected option
const UiColor LABEL_COLOR(0x000000);      ///< Black label text

constexpr int   OPTION_COUNT = 4;
constexpr float CIRCLE_SIZE  = 48.0f; ///< Half of the previous size
constexpr float CIRCLE_GAP   = 24.0f;

/// English ordinal suffix for n (1 -> "st", 2 -> "nd", 3 -> "rd", else "th").
const char* OrdinalSuffix(int n)
{
  const int mod100 = n % 100;
  const int mod10  = n % 10;
  if(mod100 < 11 || mod100 > 13)
  {
    if(mod10 == 1) return "st";
    if(mod10 == 2) return "nd";
    if(mod10 == 3) return "rd";
  }
  return "th";
}
} // namespace

/**
 * SelectableGroup / GroupSelectableTrait sample: a single-selection (radio-button
 * style) group of plain circular Views, stacked vertically.
 *
 * Each option is an ordinary View made round with a relative corner radius. The
 * options join a SelectableGroup, which enforces that at most one is selected at
 * a time. Toggle-by-click is on by default, so tapping a circle selects it and
 * deselects the previously selected one; re-tapping the selected circle is a
 * no-op. SelectedMemberChangedSignal recolours the previous and current options
 * and updates the top label to report which option is selected.
 *
 * Press Escape or Back to quit.
 */
class GroupSelectableTraitController : public ConnectionTracker
{
public:
  GroupSelectableTraitController(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &GroupSelectableTraitController::Create);
  }

  void Create(Application application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::WHITE);

    // Root vertical stack: the status label on top, then the options below it.
    StackLayout root = StackLayout::New(StackOrientation::VERTICAL);
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetSpacing(CIRCLE_GAP);
    root.SetPadding(Extents(40, 40, 40, 40)); // start, end, top, bottom

    // Status label at the top, reporting the current selection.
    mLabel = Label::New("No View is selected");
    mLabel.SetTextColor(LABEL_COLOR);
    root.Add(mLabel);

    mGroup = SelectableGroup::New();

    for(int i = 0; i < OPTION_COUNT; ++i)
    {
      View circle = View::New();
      circle.SetRequestedWidth(CIRCLE_SIZE);
      circle.SetRequestedHeight(CIRCLE_SIZE);
      circle.SetBackgroundColor(UNSELECTED_COLOR);

      // Make the (square) View a circle: a relative corner radius of 0.5 rounds
      // each corner to half the size.
      circle.SetCornerRadiusPolicyRelative();
      circle.SetCornerRadius(0.5f);

      // CENTER keeps the circle at its requested size (FILL would stretch it
      // across the stack width and turn it into an ellipse).
      circle.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::CENTER));

      // Join the group. This attaches a GroupSelectableTrait (with toggle-by-click
      // enabled), so tapping the circle selects it within the group. Add() must be
      // called before any View::AsSelectable() on the same View: a plain
      // SelectableTrait is never replaced, so the reverse order would fail.
      mGroup.Add(circle);

      root.Add(circle);
      mCircles.push_back(circle);
    }

    // On every selection change: previous option -> grey, current -> blue, and
    // update the top label to report which option is now selected.
    mGroup.SelectedMemberChangedSignal().Connect(
      this, [this](View previous, View current, InputEvent /*event*/) {
        if(previous)
        {
          previous.SetBackgroundColor(UNSELECTED_COLOR);
        }
        if(current)
        {
          current.SetBackgroundColor(SELECTED_COLOR);
        }
        UpdateLabel(current);
      });

    window.Add(root);

    // Start with the first option selected.
    mGroup.SelectMember(mCircles[0]);

    window.KeyEventSignal().Connect(this, &GroupSelectableTraitController::OnKeyEvent);
  }

  void UpdateLabel(View selected)
  {
    for(std::size_t i = 0; i < mCircles.size(); ++i)
    {
      if(mCircles[i] == selected)
      {
        const int n = static_cast<int>(i) + 1;
        char      text[64];
        std::snprintf(text, sizeof(text), "%d%s View is selected", n, OrdinalSuffix(n));
        mLabel.SetText(text);
        return;
      }
    }
    mLabel.SetText("No View is selected");
  }

  void OnKeyEvent(Window window, KeyEvent event)
  {
    if(event.GetState() == KeyEvent::DOWN)
    {
      if(IsKey(event, Dali::DALI_KEY_ESCAPE) || IsKey(event, Dali::DALI_KEY_BACK))
      {
        mApplication.Quit();
      }
    }
  }

private:
  Application&      mApplication;
  SelectableGroup   mGroup;
  Label             mLabel;
  std::vector<View> mCircles;
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv);

  // Initialise the UI configuration before any trait is created. SelectableGroup
  // enables toggle-by-click, which creates an InteractiveTrait, and that reads
  // UiConfig (e.g. the key-click policy) on construction.
  UiConfig config = UiConfig::New();
  config.Apply();

  GroupSelectableTraitController controller(application);
  application.MainLoop();
  return 0;
}
