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

using namespace Dali;
using namespace Dali::Ui;

/**
 * Spec-mode fade and slide on one view's self transition.
 * ENTER: opacity 0 -> 1, final Y + 80 px -> final Y.
 * EXIT: opacity 1 -> 0, final Y -> final Y + 80 px.
 *
 * Tap ENTER / EXIT or press Up / Down. Wait for each transition to finish
 * before starting the next one. Escape / Back quits.
 */
class LayoutTransitionFadeSlideController : public ConnectionTracker
{
public:
  explicit LayoutTransitionFadeSlideController(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &LayoutTransitionFadeSlideController::Create);
  }

  ~LayoutTransitionFadeSlideController()
  {
    if(mTransition)
    {
      mTransition.SetOnFinished(LayoutLifecycleCallback());
    }
  }

private:
  enum class State
  {
    ABSENT,
    ENTERING,
    PRESENT,
    EXITING
  };

  static void SetFixedGeometry(View view, float width, float height)
  {
    view.SetRequestedWidth(width);
    view.SetRequestedHeight(height);
    view.SetLayoutParams(StackLayoutParams::New().SetAlignment(LayoutAlignment::START));
  }

  static Label MakeLabel(const Dali::String& text, float width, float height)
  {
    Label label = Label::New(text);
    SetFixedGeometry(label, width, height);
    label.SetHorizontalTextAlignment(Text::Alignment::CENTER);
    label.SetVerticalTextAlignment(Text::Alignment::CENTER);
    label.SetTextColor(Color::BLACK);
    return label;
  }

  void Create(Application application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::WHITE);

    Label enterButton = MakeLabel("ENTER (Up)", 174.0f, 60.0f);
    enterButton.SetBackgroundColor(Color::BLACK);
    enterButton.SetTextColor(Color::WHITE);
    enterButton.TouchEventSignal().Connect(this, &LayoutTransitionFadeSlideController::OnEnterTouched);

    Label exitButton = MakeLabel("EXIT (Down)", 174.0f, 60.0f);
    exitButton.SetBackgroundColor(Color::BLACK);
    exitButton.SetTextColor(Color::WHITE);
    exitButton.TouchEventSignal().Connect(this, &LayoutTransitionFadeSlideController::OnExitTouched);

    StackLayout buttons = StackLayout::New(StackOrientation::HORIZONTAL);
    SetFixedGeometry(buttons, 360.0f, 60.0f);
    buttons.SetSpacing(12.0f);
    buttons.Add(enterButton);
    buttons.Add(exitButton);

    mStatus = MakeLabel("Absent: press ENTER", 360.0f, 40.0f);

    // The host starts empty and reserves space for the card's 80 px slide.
    mHost = StackLayout::New();
    SetFixedGeometry(mHost, 360.0f, 220.0f);
    mHost.SetBackgroundColor(Color::LIGHT_GRAY);

    mCard = MakeLabel("Fade + 80 px slide", 260.0f, 110.0f);
    mCard.SetBackgroundColor(Color::BLUE);
    mCard.SetTextColor(Color::WHITE);
    mTransition = CreateFadeSlideTransition();
    mTransition.SetOnFinished(LayoutLifecycleCallback::New(
      this, &LayoutTransitionFadeSlideController::OnTransitionFinished));
    mCard.SetSelfLayoutTransition(mTransition);

    // Fixed sizes and START alignment keep the card's bounds unchanged
    // when status text changes or the window resizes during ENTER.
    StackLayout root = StackLayout::New();
    SetFixedGeometry(root, 360.0f, 400.0f);
    root.SetLayoutDirection(Dali::LayoutDirection::LEFT_TO_RIGHT);
    root.SetSpacing(10.0f);
    root.Add(MakeLabel("Spec fade / slide: 0.3 seconds", 360.0f, 40.0f));
    root.Add(buttons);
    root.Add(mStatus);
    root.Add(mHost);

    window.Add(root);
    window.KeyEventSignal().Connect(this, &LayoutTransitionFadeSlideController::OnKeyEvent);
  }

  static LayoutTransition CreateFadeSlideTransition()
  {
    const LayoutTransitionTiming timing{
      Duration(0.3f), AlphaFunction(AlphaFunction::EASE_IN_OUT), Duration()};

    ViewAnimationSpec enterSpec = ViewAnimationSpec::New();
    enterSpec.Opacity(1.0f, timing.duration, timing.alpha);

    ViewAnimationSpec exitSpec = ViewAnimationSpec::New();
    exitSpec.Opacity(0.0f, timing.duration, timing.alpha);

    LayoutTransition transition = LayoutTransition::New();
    transition.ClearChangeTiming()
      .SetEnterOnInitialMount(true)
      .SetEnterVisualSpec(enterSpec)
      .SetExitVisualSpec(exitSpec)
      .SetEnterBoundsEffect(LayoutBoundsEffects::SlideFrom(
        LayoutBoundsEdge::BOTTOM, LayoutBoundsLength::Pixel(80.0f), timing))
      .SetExitBoundsEffect(LayoutBoundsEffects::SlideTo(
        LayoutBoundsEdge::BOTTOM, LayoutBoundsLength::Pixel(80.0f), timing));
    return transition;
  }

  void Enter()
  {
    if(mState != State::ABSENT)
    {
      return;
    }
    mState = State::ENTERING;
    mStatus.SetText("Entering: please wait");
    // A visual spec declares a target; supply its starting opacity before Add.
    mCard.SetOpacity(0.0f);
    mHost.Add(mCard);
  }

  void Exit()
  {
    if(mState != State::PRESENT)
    {
      return;
    }
    mState = State::EXITING;
    mStatus.SetText("Exiting: please wait");
    mHost.Remove(mCard, RemovePolicy::ANIMATE_EXIT);
  }

  void OnTransitionFinished(View view, LayoutTransitionSlot slot)
  {
    if(view != mCard)
    {
      return;
    }
    if(slot == LayoutTransitionSlot::ENTER && mState == State::ENTERING)
    {
      mState = State::PRESENT;
      mStatus.SetText("Present: press EXIT");
    }
    else if(slot == LayoutTransitionSlot::EXIT && mState == State::EXITING)
    {
      // EXIT has unparented the card before this callback; it can be added again.
      mState = State::ABSENT;
      mStatus.SetText("Absent: press ENTER");
    }
  }

  bool OnEnterTouched(Actor /*actor*/, TouchEvent touch)
  {
    if(touch.GetState(0) != PointState::STARTED)
    {
      return false;
    }
    Enter();
    return true;
  }

  bool OnExitTouched(Actor /*actor*/, TouchEvent touch)
  {
    if(touch.GetState(0) != PointState::STARTED)
    {
      return false;
    }
    Exit();
    return true;
  }

  void OnKeyEvent(Window /*window*/, KeyEvent event)
  {
    if(event.GetState() != KeyEvent::DOWN)
    {
      return;
    }
    if(IsKey(event, Dali::DALI_KEY_ESCAPE) || IsKey(event, Dali::DALI_KEY_BACK))
    {
      mApplication.Quit();
    }
    else if(IsKey(event, Dali::DALI_KEY_CURSOR_UP))
    {
      Enter();
    }
    else if(IsKey(event, Dali::DALI_KEY_CURSOR_DOWN))
    {
      Exit();
    }
  }

  Application&     mApplication;
  StackLayout      mHost;
  Label            mCard;
  Label            mStatus;
  LayoutTransition mTransition;
  State            mState{State::ABSENT};
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application                         application = Application::New(&argc, &argv);
  LayoutTransitionFadeSlideController controller(application);
  application.MainLoop();
  return 0;
}
