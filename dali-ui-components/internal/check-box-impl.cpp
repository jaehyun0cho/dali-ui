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

// CLASS HEADER
#include <dali-ui-components/internal/check-box-impl.h>

// EXTERNAL INCLUDES
#include <dali/devel-api/object/type-registry-helper.h>
#include <dali/devel-api/object/type-registry.h>
#include <algorithm>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/configuration/ui-theme-manager.h>
#include <dali-ui-foundation/public-api/views/view-accessibility-types.h>
#include <dali-ui-foundation/public-api/views/view-impl.h> // public GetImpl(Ui::View&)

namespace Dali
{
namespace Ui
{
namespace Internal
{
namespace
{
BaseHandle Create()
{
  return BaseHandle();
}

DALI_TYPE_REGISTRATION_BEGIN(CheckBoxImpl, Extension::SelectableViewImpl, Create)
DALI_TYPE_REGISTRATION_END()

// The DEFAULT asset (checkbox.json) has the segment layout: unchecked=0, check=[1,19],
// checked=[19,30], uncheck=[30,48] — so the default select plays [0,30] (check + settle)
// and deselect plays [30,48]. The style's icon generator builds the selectable image (an
// ISelectableImage that composes the Lottie glyph); the image owns the frame-range playback
// and the per-frame inner-fill recolour internally.

} // namespace

Ui::CheckBox CheckBoxImpl::New(CheckBoxStyle style)
{
  DALI_ASSERT_ALWAYS(style && "CheckBoxStyle must be initialized");
  IntrusivePtr<CheckBoxImpl> impl(new CheckBoxImpl());
  Ui::CheckBox               handle(*impl);
  impl->Initialize();
  impl->ApplyInitialStyle(style);
  return handle;
}

void CheckBoxImpl::SetText(const Dali::String& text)
{
  mText = text;
  mLabel.SetText(text);

  // Mirror the label onto the single accessible node's name.
  Ui::View self = Ui::View::DownCast(Self());
  self.SetProperty(Ui::View::Property::ACCESSIBILITY_NAME, text);

  InvalidateMeasure(); // label presence changes the measured size
}

Dali::String CheckBoxImpl::GetText() const
{
  return mText;
}

void CheckBoxImpl::SetSelectionAnimationMode(SelectionAnimationMode mode)
{
  mSelectionAnimationMode = mode;
}

SelectionAnimationMode CheckBoxImpl::GetSelectionAnimationMode() const
{
  return mSelectionAnimationMode;
}

void CheckBoxImpl::OnInitialize()
{
  Ui::Extension::SelectableViewImpl::OnInitialize(); // base first (attaches the SelectableTrait)

  Ui::View self = Ui::View::DownCast(Self());

  // Accessibility role; clip children to the control box.
  self.SetProperty(Ui::View::Property::ACCESSIBILITY_ROLE,
                   static_cast<int32_t>(Accessibility::Role::CHECK_BOX));
  Self().SetProperty(Actor::Property::CLIPPING_MODE, ClippingMode::CLIP_TO_BOUNDING_BOX);

  // The Lottie glyph (mIcon) needs the url + frame ranges from the style, so it is created
  // in ApplyInitialStyle(); OnInitialize() only sets up the label and the selection/theme
  // wiring that does not depend on the glyph.

  // Optional trailing label (empty until SetText()), vertically centered against the icon.
  mLabel = Ui::Label::New();
  mLabel.SetVerticalTextAlignment(Text::Alignment::CENTER);
  mLabel.SetProperty(Ui::View::Property::ACCESSIBILITY_HIDDEN, true);
  Self().Add(mLabel);

  // React to selection changes (no virtual OnSelectedChanged hook exists on the base).
  SelectionChangedSignal().Connect(this, &CheckBoxImpl::OnSelectionChanged);
  UiThemeManager::Get().ThemeChangedSignal().Connect(this, &CheckBoxImpl::OnThemeChanged);
}

void CheckBoxImpl::ApplyInitialStyle(CheckBoxStyle style)
{
  Ui::View self = Ui::View::DownCast(Self());
  self.SetMinimumWidth(style.GetMinimumWidth());
  self.SetMinimumHeight(style.GetMinimumHeight());
  self.SetPadding(style.GetPadding());
  self.SetStateEffect(style.GetStateEffect());

  mBoxSize = style.GetBoxSize();
  mGap     = style.GetLabelGap();

  mIconColor         = style.GetIconColor();
  mSelectedIconColor = style.GetSelectedIconColor();

  // The style's icon generator builds the selectable image (url + explicit integer frame
  // ranges are baked into it); the image owns the frame-range playback and the per-frame
  // inner-fill recolour. GetView() is the scene view the image draws into.
  mIcon             = style.CreateIcon();
  Ui::View iconView = mIcon.GetView();
  iconView.SetProperty(Ui::View::Property::ACCESSIBILITY_HIDDEN, true);
  Self().Add(iconView);
  mIcon.TransitionFinishedSignal().Connect(this, &CheckBoxImpl::OnAnimationFinished);

  // Confine press/focus feedback to the glyph only: the interactive area stays the whole
  // CheckBox (icon + label), but the overlay effect targets the icon so the label area
  // (and any other region) does not animate on touch press/release.
  self.SetStateEffectTarget(iconView);

  // Push the resolved state colours into the glyph, then seat the initial resting frame.
  PushStateColors();

  mLabel.SetTextColor(style.GetLabelColor());

  RefreshRestingFrame();
  UpdateAccessibility(IsSelected());
}

void CheckBoxImpl::OnSelectionChanged(View /*view*/, bool selected, InputEvent event)
{
  // Selection authority stays here; the glyph view only renders the requested state.
  mIcon.SetSelected(selected, IsSelectionAnimationRequired(event));
  UpdateAccessibility(selected);
}

bool CheckBoxImpl::IsSelectionAnimationRequired(const InputEvent& event) const
{
  if(mSelectionAnimationMode == SelectionAnimationMode::DISABLED)
  {
    return false;
  }
  if(!(Self().GetProperty<bool>(Dali::Actor::Property::CONNECTED_TO_SCENE) && Self().IsVisible())) // animatable only when on-scene and visible
  {
    return false;
  }
  if(mSelectionAnimationMode == SelectionAnimationMode::ENABLED)
  {
    return true;
  }
  // AUTO: animate only user-initiated changes; programmatic SetSelected snaps.
  return !event.IsProgrammatic();
}

void CheckBoxImpl::RefreshRestingFrame()
{
  if(mIcon.IsTransitioning())
  {
    return; // don't disturb a running segment
  }
  mIcon.SetSelected(IsSelected(), false); // snap to the correct resting frame + range
}

void CheckBoxImpl::PushStateColors()
{
  // Resolve the deselected/selected tokens against the current theme and push the RGBA into
  // the glyph view, which seats them on its inner-fill recolour callback.
  mIcon.SetStateColors(mIconColor.GetRgba(), mSelectedIconColor.GetRgba());
}

void CheckBoxImpl::UpdateAccessibility(bool selected)
{
  Ui::View self = Ui::View::DownCast(Self());
  if(selected)
  {
    self.AddAccessibilityState(Accessibility::State::CHECKED);
  }
  else
  {
    self.RemoveAccessibilityState(Accessibility::State::CHECKED);
  }
  // Usage hint (app-localizable; plain-string passthrough). TODO(verify): source the
  // localized strings from the app's i18n rather than hard-coding English.
  self.SetProperty(Ui::View::Property::ACCESSIBILITY_DESCRIPTION,
                   Dali::String(selected ? "Double tap to deselect" : "Double tap to select"));
}

void CheckBoxImpl::OnThemeChanged()
{
  // Re-resolve the tokens and push them into the glyph.
  if(mIcon.IsTransitioning())
  {
    mThemeRefreshPending = true; // apply when the current segment finishes
    return;
  }
  PushStateColors();
  RefreshRestingFrame();
}

void CheckBoxImpl::OnAnimationFinished(View /*view*/)
{
  if(mThemeRefreshPending)
  {
    PushStateColors();
    mThemeRefreshPending = false;
  }
  RefreshRestingFrame(); // re-assert the resting frame after playback
}

void CheckBoxImpl::OnSceneConnection(int depth)
{
  Ui::Extension::SelectableViewImpl::OnSceneConnection(depth); // base first
  RefreshRestingFrame();                                       // show the correct resting frame on (re-)attach
}

MeasuredSize CheckBoxImpl::OnMeasure(float widthConstraint, float heightConstraint)
{
  float s = GetEffectiveScale();

  Extents padding = GetPadding();
  float   visPadW = static_cast<float>(padding.start + padding.end) * s;
  float   visPadH = static_cast<float>(padding.top + padding.bottom) * s;

  float boxVis   = mBoxSize * s;
  float gapVis   = mGap * s;
  bool  hasLabel = !mText.Empty();

  float requestedWidth  = GetRequestedWidth();
  float requestedHeight = GetRequestedHeight();
  float requestedVisW   = (requestedWidth >= 0.0f) ? requestedWidth * s : requestedWidth;
  float requestedVisH   = (requestedHeight >= 0.0f) ? requestedHeight * s : requestedHeight;

  float effectiveVisW = (requestedVisW >= 0.0f) ? requestedVisW : widthConstraint;
  float effectiveVisH = (requestedVisH >= 0.0f) ? requestedVisH : heightConstraint;
  float contentVisW   = (effectiveVisW >= 0.0f) ? std::max(0.0f, effectiveVisW - visPadW) : effectiveVisW;
  float contentVisH   = (effectiveVisH >= 0.0f) ? std::max(0.0f, effectiveVisH - visPadH) : effectiveVisH;

  // No fixed box size => the icon fills the content height (the app sizes the control).
  if(mBoxSize <= 0.0f)
  {
    boxVis = (contentVisH >= 0.0f) ? contentVisH : 0.0f;
  }

  float labelW = 0.0f;
  float labelH = 0.0f;
  if(hasLabel)
  {
    float        labelAvailW = (contentVisW >= 0.0f) ? std::max(0.0f, contentVisW - boxVis - gapVis) : contentVisW;
    MeasuredSize labelSize   = GetImpl(mLabel).Measure(labelAvailW, contentVisH);
    labelW                   = labelSize.width;
    labelH                   = labelSize.height;
  }

  float naturalW = boxVis + (hasLabel ? gapVis + labelW : 0.0f);
  float naturalH = std::max(boxVis, labelH);

  float resultVisW = 0.0f;
  if(requestedVisW >= 0.0f)
  {
    resultVisW = requestedVisW;
  }
  else if(requestedWidth == MATCH_PARENT)
  {
    resultVisW = GetMinimumWidth() * s;
  }
  else
  {
    resultVisW = naturalW + visPadW;
  }

  float resultVisH = 0.0f;
  if(requestedVisH >= 0.0f)
  {
    resultVisH = requestedVisH;
  }
  else if(requestedHeight == MATCH_PARENT)
  {
    resultVisH = GetMinimumHeight() * s;
  }
  else
  {
    resultVisH = naturalH + visPadH;
  }

  return MeasuredSize(resultVisW, resultVisH);
}

MeasuredSize CheckBoxImpl::OnArrange(const LayoutRect& bounds)
{
  Actor self = Self();
  self.SetProperty(Actor::Property::POSITION_X, bounds.x);
  self.SetProperty(Actor::Property::POSITION_Y, bounds.y);
  self.SetProperty(Actor::Property::SIZE_WIDTH, bounds.width);
  self.SetProperty(Actor::Property::SIZE_HEIGHT, bounds.height);

  float   s       = GetEffectiveScale();
  Extents padding = GetPadding();

  float contentX = static_cast<float>(padding.start) * s;
  float contentY = static_cast<float>(padding.top) * s;
  float contentW = std::max(0.0f, bounds.width - static_cast<float>(padding.start + padding.end) * s);
  float contentH = std::max(0.0f, bounds.height - static_cast<float>(padding.top + padding.bottom) * s);

  // No fixed box size => the icon fills the content height (the app sizes the control).
  float boxVis = (mBoxSize > 0.0f) ? (mBoxSize * s) : contentH;
  float gapVis = mGap * s;

  // Icon (leading, vertically centered). The framework mirrors position under RTL; the
  // Lottie artwork itself is direction-independent and is NOT mirrored.
  LayoutRect iconRect;
  iconRect.x      = contentX;
  iconRect.y      = contentY + std::max(0.0f, (contentH - boxVis) * 0.5f);
  iconRect.width  = boxVis;
  iconRect.height = boxVis;

  Ui::View iconView = mIcon.GetView(); // the composed drawing view; use public GetImpl(Ui::View&)
  GetImpl(iconView).Measure(iconRect.width, iconRect.height);
  GetImpl(iconView).Arrange(iconRect);

  // Optional trailing label.
  LayoutRect labelRect;
  labelRect.x      = contentX + boxVis + gapVis;
  labelRect.y      = contentY;
  labelRect.width  = mText.Empty() ? 0.0f : std::max(0.0f, contentW - boxVis - gapVis);
  labelRect.height = contentH;
  GetImpl(mLabel).Measure(labelRect.width, labelRect.height);
  GetImpl(mLabel).Arrange(labelRect);

  return MeasuredSize(bounds.width, bounds.height);
}

CheckBoxImpl::CheckBoxImpl() = default;

CheckBoxImpl::~CheckBoxImpl() = default;

} // namespace Internal
} // namespace Ui
} // namespace Dali
