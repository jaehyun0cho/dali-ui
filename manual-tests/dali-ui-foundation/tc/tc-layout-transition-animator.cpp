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
constexpr Vector4 PALETTE[] = {
  Color::RED, Color::GREEN, Color::BLUE,
  Color::YELLOW, Color::CYAN, Color::MAGENTA};
constexpr uint32_t PALETTE_SIZE = sizeof(PALETTE) / sizeof(PALETTE[0]);

/**
 * @brief The three animator callbacks registered on the LayoutTransition.
 *
 * They live in the anonymous namespace so nothing at file scope but the test
 * case class has external linkage, and so their names cannot be confused with
 * the TestCase::OnEnter / OnExit overrides.
 */
struct Animators
{
  /// ENTER animator: height 0 -> toBounds.height anchored at the TOP edge,
  /// combined with opacity 0 -> 1. Width and position stay at the
  /// layout-applied values so the child grows down into its allocated slot.
  static void AnimateEnter(const LayoutAnimatorContext& ctx)
  {
    Actor actor = ctx.view;
    if(!actor)
    {
      return;
    }
    const float p = ctx.progress;
    actor.SetProperty(Actor::Property::OPACITY, p);
    actor.SetHeight(ctx.toBounds.height * p);
  }

  /// EXIT animator: height fromBounds.height -> 0 anchored at the TOP edge,
  /// combined with opacity 1 -> 0.
  static void AnimateExit(const LayoutAnimatorContext& ctx)
  {
    Actor actor = ctx.view;
    if(!actor)
    {
      return;
    }
    const float p = ctx.progress;
    actor.SetProperty(Actor::Property::OPACITY, 1.0f - p);
    actor.SetHeight(ctx.fromBounds.height * (1.0f - p));
  }

  /// CHANGE animator: linear interpolation of arranged bounds.
  static void AnimateChange(const LayoutAnimatorContext& ctx)
  {
    Actor actor = ctx.view;
    if(!actor)
    {
      return;
    }
    const float p   = ctx.progress;
    const float x   = ctx.fromBounds.x      + (ctx.toBounds.x - ctx.fromBounds.x) * p;
    const float y   = ctx.fromBounds.y      + (ctx.toBounds.y - ctx.fromBounds.y) * p;
    const float w   = ctx.fromBounds.width  + (ctx.toBounds.width  - ctx.fromBounds.width)  * p;
    const float h   = ctx.fromBounds.height + (ctx.toBounds.height - ctx.fromBounds.height) * p;
    actor.SetPositionX(x);
    actor.SetPositionY(y);
    actor.SetWidth(w);
    actor.SetHeight(h);
  }
};
} // namespace

/**
 * @brief Verifies LayoutTransition in animator-callback mode.
 *
 * The application owns the per-frame interpolation: the framework sweeps
 * progress 0..1 across the supplied timing and invokes the callback every
 * frame. The callback writes SIZE / OPACITY itself, mirroring the
 * spec-mode test's height-expand + opacity-fade contract but driving the
 * per-frame values from the application instead of a declarative spec.
 * All three slots share a single 0.4s EASE_IN_OUT_SINE timing.
 *
 * Ported from samples/layout-transition/layout-transition-animator-example.cpp.
 */
class TcLayoutTransitionAnimator : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "LayoutTransition: Animator Callback";
  }

  Dali::String GetDescription() const override
  {
    return "Application-driven per-frame interpolation via LayoutAnimatorCallback";
  }

  void OnEnter(View contentArea) override
  {
    ResetState();

    mBackdrop = View::New();
    mBackdrop.SetRequestedWidth(MATCH_PARENT);
    mBackdrop.SetRequestedHeight(MATCH_PARENT);
    mBackdrop.SetBackgroundColor(Color::WHITE);

    // Three Clickable labels, one per LayoutTransition slot.
    mEnterButton = MakeClickableLabel("Click to ENTER");
    mEnterButton.TouchEventSignal().Connect(this, &TcLayoutTransitionAnimator::OnEnterButtonTouched);

    mExitButton = MakeClickableLabel("Click to EXIT");
    mExitButton.TouchEventSignal().Connect(this, &TcLayoutTransitionAnimator::OnExitButtonTouched);

    mChangeButton = MakeClickableLabel("Click to CHANGE");
    mChangeButton.TouchEventSignal().Connect(this, &TcLayoutTransitionAnimator::OnChangeButtonTouched);

    mStack = StackLayout::New();
    mStack.SetRequestedWidth(MATCH_PARENT);
    mStack.SetRequestedHeight(MATCH_PARENT);
    mStack.SetSpacing(10.0f);

    // Single timing shared by ENTER / EXIT / CHANGE: 0.4s EASE_IN_OUT_SINE.
    LayoutAnimatorTiming timing;
    timing.duration = Duration(0.4f);
    timing.alpha    = AlphaFunction(AlphaFunction::EASE_IN_OUT_SINE);

    LayoutTransition transition = LayoutTransition::New();
    transition
      .SetEnterAnimator(LayoutAnimatorCallback::New(&Animators::AnimateEnter), timing)
      .SetExitAnimator(LayoutAnimatorCallback::New(&Animators::AnimateExit), timing)
      .SetChangeAnimator(LayoutAnimatorCallback::New(&Animators::AnimateChange), timing);

    mStack.SetLayoutTransition(transition);

    // Seed with three children. The framework default suppresses ENTER on
    // the parent's first arrange pass; animator-mode ENTER is skipped
    // entirely (no property writes), so these initial children appear at
    // their default OPACITY = 1 and the layout-applied height immediately.
    // Runtime adds via the ENTER button expand + fade in normally.
    AppendChild();
    AppendChild();
    AppendChild();

    // Button row hosts the three Clickables; equal width via FlexGrow 1.
    FlexLayout buttonRow = FlexLayout::New();
    buttonRow.SetRequestedWidth(MATCH_PARENT);
    buttonRow.SetRequestedHeight(60.0f);
    buttonRow.SetDirection(FlexDirection::ROW);
    buttonRow.SetAlignItems(FlexAlign::STRETCH);
    buttonRow.Add(mEnterButton);
    buttonRow.Add(mExitButton);
    buttonRow.Add(mChangeButton);

    StackLayout root = StackLayout::New();
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.Add(buttonRow);
    root.Add(mStack);

    mBackdrop.Add(root);
    contentArea.Add(mBackdrop);
  }

  void OnExit() override
  {
    // Release every retained handle so the previous subtree is not kept
    // alive for the lifetime of the launcher.
    mBackdrop.Reset();
    mStack.Reset();
    mEnterButton.Reset();
    mExitButton.Reset();
    mChangeButton.Reset();

    // Remaining scalars back to their initial values.
    mNextColorIndex = 0u;
    mExpanded       = false;
  }

private:
  void ResetState()
  {
    mNextColorIndex = 0u;
    mExpanded       = false;
  }

  Label MakeClickableLabel(const Dali::String& text)
  {
    Label label = Label::New();
    label.SetText(text);
    label.SetTextColor(Color::WHITE);
    label.SetHorizontalTextAlignment(Text::Alignment::CENTER);
    label.SetVerticalTextAlignment(Text::Alignment::CENTER);
    label.SetBackgroundColor(Color::BLACK);
    label.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(1.0f));
    // 5px on each side gives a 10px visual gap between adjacent buttons.
    label.SetMargin(Extents(5, 5, 0, 0));
    return label;
  }

  void AppendChild()
  {
    View child = View::New();
    child.SetBackgroundColor(PALETTE[mNextColorIndex % PALETTE_SIZE]);
    child.SetRequestedWidth(MATCH_PARENT);
    child.SetRequestedHeight(mExpanded ? 160.0f : 80.0f);
    // Leave OPACITY and SIZE_HEIGHT at their actor / layout defaults.
    // The AnimateEnter animator drives OPACITY from 0 and SIZE_HEIGHT from 0
    // up to the layout-applied values for runtime adds. On initial mount
    // the dispatcher suppresses animator ENTER entirely, so initial
    // children appear at defaults without animation; pre-setting either
    // here would leave initial-mount children stuck at the pre-set value.
    mStack.Add(child);
    ++mNextColorIndex;
  }

  void RemoveLastChild()
  {
    const uint32_t count = mStack.GetChildViewCount();
    if(count == 0u)
    {
      return;
    }
    // The logical child list excludes any in-flight EXIT ghost, so the last
    // logical child is the last live item.
    View last = mStack.GetChildViewAt(count - 1u);
    mStack.Remove(last, RemovePolicy::ANIMATE_EXIT);
  }

  bool OnEnterButtonTouched(Actor /*actor*/, TouchEvent touch)
  {
    if(touch.GetState(0) != PointState::STARTED)
    {
      return false;
    }
    AppendChild();
    return true;
  }

  bool OnExitButtonTouched(Actor /*actor*/, TouchEvent touch)
  {
    if(touch.GetState(0) != PointState::STARTED)
    {
      return false;
    }
    RemoveLastChild();
    return true;
  }

  bool OnChangeButtonTouched(Actor /*actor*/, TouchEvent touch)
  {
    if(touch.GetState(0) != PointState::STARTED)
    {
      return false;
    }
    mExpanded             = !mExpanded;
    const float newHeight = mExpanded ? 160.0f : 80.0f;
    const uint32_t count  = mStack.GetChildViewCount();
    for(uint32_t i = 0; i < count; ++i)
    {
      mStack.GetChildViewAt(i).SetRequestedHeight(newHeight);
    }
    return true;
  }

  View        mBackdrop;
  StackLayout mStack;
  Label       mEnterButton;
  Label       mExitButton;
  Label       mChangeButton;
  uint32_t    mNextColorIndex = 0u;
  bool        mExpanded       = false;
};

REGISTER_MANUAL_TEST(TcLayoutTransitionAnimator)
