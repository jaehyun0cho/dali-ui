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
#include <dali/devel-api/adaptor-framework/application.h>
#include <cstdio>

using namespace Dali;
using namespace Dali::Ui;

namespace
{
constexpr char SIGNAL_COUNT_PROPERTY[] = "sampleLayoutDirectionSignalCount";

const char* DirectionName(LayoutDirection::Type direction)
{
  switch(direction)
  {
    case LayoutDirection::LEFT_TO_RIGHT:
      return "LTR";
    case LayoutDirection::RIGHT_TO_LEFT:
      return "RTL";
    case LayoutDirection::INHERIT:
      return "INHERIT";
  }
  return "UNKNOWN";
}

Label MakeLabel(const char* text, float width, float height)
{
  Label label = Label::New(text);
  label.SetRequestedWidth(width);
  label.SetRequestedHeight(height);
  label.SetFontSize(16.0f);
  label.SetMultiLine(true);
  label.SetTextColor(Color::BLACK);
  label.SetHorizontalTextAlignment(Text::Alignment::CENTER);
  label.SetVerticalTextAlignment(Text::Alignment::CENTER);
  return label;
}

void UpdateObservedPosition(Label label, LayoutDirection::Type direction)
{
  // Both widths are fixed layout inputs, available before the first Arrange.
  // STANDALONE positions are parent-local and are not automatically mirrored.
  const View  parent = View::DownCast(label.GetParent());
  const float x      = direction == LayoutDirection::RIGHT_TO_LEFT
                         ? parent.GetRequestedWidth() - label.GetRequestedWidth() - 50.0f
                         : 50.0f;
  label.SetRequestedX(x);
  label.SetRequestedY(12.0f);
}

void UpdateObservedLabel(Label label, LayoutDirection::Type direction)
{
  const int count = label.GetProperty<int>(label.GetPropertyIndex(SIGNAL_COUNT_PROPERTY));
  char      text[256];
  std::snprintf(text, sizeof(text), "%s\nPolicy: %s\nEffective: %s\nSignals: %d\nRequested: (%.0f, %.0f)",
                label.GetName().CStr(), DirectionName(label.GetLayoutDirection()),
                DirectionName(direction), count, label.GetRequestedX(), label.GetRequestedY());
  label.SetText(text);
}

void ObserveDirection(Label label)
{
  // Unary + converts this captureless lambda to the signal's function pointer.
  // The emitting actor supplies the label and its parent; no controller is captured.
  label.LayoutDirectionChangedSignal().Connect(+[](Actor actor, LayoutDirection::Type direction)
  {
    Label                 target     = Label::DownCast(actor);
    const Property::Index countIndex = target.GetPropertyIndex(SIGNAL_COUNT_PROPERTY);
    const int             count      = target.GetProperty<int>(countIndex) + 1;
    target.SetProperty(countIndex, count);
    UpdateObservedPosition(target, direction);
    UpdateObservedLabel(target, direction);
    std::printf("LayoutDirectionChangedSignal: %s -> %s (count=%d, requested x=%.0f, y=%.0f)\n",
                target.GetName().CStr(), DirectionName(direction), count,
                target.GetRequestedX(), target.GetRequestedY());
    std::fflush(stdout);
  });

  // Initial state is a query, not a synthetic signal emission.
  const LayoutDirection::Type direction = label.GetEffectiveLayoutDirection();
  UpdateObservedPosition(label, direction);
  UpdateObservedLabel(label, direction);
}
} // namespace

class StackLayoutDirectionSignalController : public ConnectionTracker
{
public:
  explicit StackLayoutDirectionSignalController(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &StackLayoutDirectionSignalController::Create);
  }

  void Create(Application application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::WHITE);

    // Keep this ancestor in the system's inheritance chain. Only the separate
    // controls row has a fixed direction, so it cannot block the observed parent.
    StackLayout outer = StackLayout::New();
    outer.SetRequestedWidth(400.0f);
    outer.SetRequestedHeight(500.0f);
    outer.SetPadding(Insets(16.0f, 16.0f, 16.0f, 16.0f));
    outer.SetSpacing(12.0f);
    outer.SetLayoutDirection(LayoutDirection::INHERIT);
    outer.Add(MakeLabel("LayoutDirectionChangedSignal", 368.0f, 48.0f));

    StackLayout controls = StackLayout::New(StackOrientation::HORIZONTAL);
    controls.SetRequestedWidth(368.0f);
    controls.SetRequestedHeight(48.0f);
    controls.SetSpacing(8.0f);
    controls.SetLayoutDirection(LayoutDirection::LEFT_TO_RIGHT);
    controls.Add(MakeButton("LTR", 112.0f, LayoutDirection::LEFT_TO_RIGHT));
    controls.Add(MakeButton("RTL", 112.0f, LayoutDirection::RIGHT_TO_LEFT));
    controls.Add(MakeButton("INHERIT", 128.0f, LayoutDirection::INHERIT));
    outer.Add(controls);

    mParentStatus = MakeLabel("", 368.0f, 56.0f);
    mParentStatus.SetLayoutDirection(LayoutDirection::LEFT_TO_RIGHT);
    outer.Add(mParentStatus);

    mParent = StackLayout::New(StackOrientation::HORIZONTAL);
    mParent.SetRequestedWidth(368.0f);
    mParent.SetRequestedHeight(144.0f);
    mParent.SetPadding(Insets(12.0f, 12.0f, 12.0f, 12.0f));
    mParent.SetSpacing(12.0f);
    mParent.SetBackgroundColor(Color::LIGHT_GRAY);
    mParent.SetLayoutDirection(LayoutDirection::INHERIT);

    Label inherited = MakeObservedLabel();
    mParent.Add(inherited);
    outer.Add(mParent);
    outer.Add(MakeLabel("STANDALONE child: x=50 in LTR.\nRTL: right edge is 50 from parent.\nINHERIT follows the system direction.\nEsc / Back: quit", 368.0f, 112.0f));
    window.Add(outer);

    // Observe only after mounting, so the initial inherited direction is not
    // counted as a user-triggered change, including on an RTL system.
    ObserveDirection(inherited);
    mParent.LayoutDirectionChangedSignal().Connect(this, &StackLayoutDirectionSignalController::OnParentDirectionChanged);
    UpdateParentStatus();
    window.KeyEventSignal().Connect(this, &StackLayoutDirectionSignalController::OnKeyEvent);
  }

private:
  InteractiveView MakeButton(const char* text, float width, LayoutDirection::Type direction)
  {
    InteractiveView button = InteractiveView::New();
    button.SetRequestedWidth(width);
    button.SetRequestedHeight(48.0f);
    button.SetBackgroundColor(Color::BLACK);
    Label label = MakeLabel(text, width, 48.0f);
    label.SetTextColor(Color::WHITE);
    button.Add(label);
    button.ConnectClickedSignal(this, [this, direction](View, InputEvent)
    {
      mParent.SetLayoutDirection(direction);
      // A policy change need not change the effective direction or emit a signal.
      UpdateParentStatus();
    });
    return button;
  }

  Label MakeObservedLabel()
  {
    Label label = MakeLabel("", 166.0f, 120.0f);
    label.SetName("Inherited child");
    label.SetLayoutDirection(LayoutDirection::INHERIT);
    label.SetLayoutMode(LayoutMode::STANDALONE);
    label.SetBackgroundColor(Color::CYAN);
    label.RegisterProperty(SIGNAL_COUNT_PROPERTY, 0, Property::READ_WRITE);
    return label;
  }

  void UpdateParentStatus()
  {
    char text[128];
    std::snprintf(text, sizeof(text), "Parent policy: %s\nParent effective: %s",
                  DirectionName(mParent.GetLayoutDirection()),
                  DirectionName(mParent.GetEffectiveLayoutDirection()));
    mParentStatus.SetText(text);
  }

  void OnParentDirectionChanged(Actor, LayoutDirection::Type)
  {
    UpdateParentStatus();
  }

  void OnKeyEvent(Window, KeyEvent event)
  {
    if(event.GetState() == KeyEvent::DOWN &&
       (IsKey(event, DALI_KEY_ESCAPE) || IsKey(event, DALI_KEY_BACK)))
    {
      mApplication.Quit();
    }
  }

  Application& mApplication;
  StackLayout  mParent;
  Label        mParentStatus;
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application application = Application::New(&argc, &argv);
  UiConfig    config      = UiConfig::New();
  config.SetDefaultStateEffectForInteractive(OverlayEffect::Plain());
  config.Apply();
  StackLayoutDirectionSignalController controller(application);
  application.MainLoop();
  return 0;
}
