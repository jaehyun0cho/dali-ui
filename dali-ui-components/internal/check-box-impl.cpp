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
#include <dali/devel-api/adaptor-framework/vector-animation-renderer.h>
#include <dali/devel-api/object/type-registry-helper.h>
#include <dali/devel-api/object/type-registry.h>
#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>

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

DALI_TYPE_REGISTRATION_BEGIN(CheckBoxImpl, Provider::SelectableViewImpl, Create)
DALI_TYPE_REGISTRATION_END()

// Segment layout from the checkbox.json markers: unchecked=0, check=[1,19],
// checked=[19,30], uncheck=[30,48]. Select plays [0,30] (check + settle), deselect plays
// [30,48]; resting frames checked=30 / unchecked=48 (frame 0 and 48 both render unchecked).
constexpr int SELECT_START   = 0;
constexpr int SELECT_END     = 30;
constexpr int DESELECT_START = 30;
constexpr int DESELECT_END   = 48;

// Fill layer path from the asset tree (checked-fill > fill-group > fill-color): the
// checked box fill, tinted with the selected token.
const char* const INNER_FILL_KEY_PATH = "checked-fill.fill-group.fill-color";

// ---------------------------------------------------------------------------
// Per-frame recolour callback plumbing.
//
// MakeCallback wraps a capture-less free function (verified in
// tc-lottie-dynamic-property.cpp), so per-instance colours are reached through the
// DynamicPropertyInfo.id via a small process-wide registry. The callback runs on a
// worker thread and must read ONLY pre-resolved Vector4 colours (no DALi API calls).
//
// The registry/mutex are leaked singletons (never destroyed) so a worker-thread callback
// firing during process shutdown cannot touch destroyed statics.
// TODO(verify): (1) whether Dali::MakeCallback also accepts a stateful functor (which
// would remove this registry); (2) whether re-calling SetDynamicProperty with the same id
// replaces vs accumulates on the renderer — each animated/snap transition rebuilds the
// visual, which clears prior callbacks, so the toggle path does not accumulate; only the
// idle theme-change re-register is unverified.
// ---------------------------------------------------------------------------
struct InnerColors
{
  Vector4 deselected;
  Vector4 selected;
};

std::mutex& RecolorMutex()
{
  static std::mutex* m = new std::mutex(); // leaked: must outlive worker callbacks at shutdown
  return *m;
}
std::map<int32_t, InnerColors>& RecolorRegistry()
{
  static auto* r = new std::map<int32_t, InnerColors>(); // leaked (see above)
  return *r;
}

int32_t AllocateDynamicPropertyId()
{
  static std::atomic<int32_t> sNext{1};
  return sNext++;
}

Dali::Property::Value OnInnerFillColor(int32_t id,
                                       Dali::VectorAnimationRenderer::VectorProperty /*property*/,
                                       uint32_t frameNumber)
{
  // Worker thread — registry read only, no DALi API calls.
  InnerColors colors{Vector4(1.f, 1.f, 1.f, 1.f), Vector4(1.f, 1.f, 1.f, 1.f)};
  {
    std::lock_guard<std::mutex> lock(RecolorMutex());
    auto                        it = RecolorRegistry().find(id);
    if(it != RecolorRegistry().end())
    {
      colors = it->second;
    }
  }
  // Deselected colour at the rest frames (0 / 38); selected colour elsewhere.
  const bool atDeselectedRest = (frameNumber == static_cast<uint32_t>(SELECT_START)) ||
                                (frameNumber == static_cast<uint32_t>(DESELECT_END));
  return Dali::Property::Value(atDeselectedRest ? colors.deselected : colors.selected);
}

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
  Provider::SelectableViewImpl::OnInitialize(); // base first (attaches the SelectableTrait)

  Ui::View self = Ui::View::DownCast(Self());

  // Accessibility role; clip children to the control box.
  self.SetProperty(Ui::View::Property::ACCESSIBILITY_ROLE,
                   static_cast<int32_t>(Accessibility::Role::CHECK_BOX));
  Self().SetProperty(Actor::Property::CLIPPING_MODE, ClippingMode::CLIP_TO_BOUNDING_BOX);

  // Single Lottie glyph.
  mIcon = Ui::LottieAnimationView::New();
  mIcon.SetLoopCount(1); // a segment plays once
  Ui::View iconView = mIcon;
  iconView.SetProperty(Ui::View::Property::ACCESSIBILITY_HIDDEN, true);
  Self().Add(mIcon);

  // Optional trailing label (empty until SetText()), vertically centered against the icon.
  mLabel = Ui::Label::New();
  mLabel.SetVerticalTextAlignment(Text::Alignment::CENTER);
  mLabel.SetProperty(Ui::View::Property::ACCESSIBILITY_HIDDEN, true);
  Self().Add(mLabel);

  // React to selection changes (no virtual OnSelectedChanged hook exists on the base).
  SelectionChangedSignal().Connect(this, &CheckBoxImpl::OnSelectionChanged);
  mIcon.AnimationFinishedSignal().Connect(this, &CheckBoxImpl::OnAnimationFinished);
  UiThemeManager::Get().ThemeChangedSignal().Connect(this, &CheckBoxImpl::OnThemeChanged);
}

void CheckBoxImpl::ApplyInitialStyle(CheckBoxStyle style)
{
  Ui::View self = Ui::View::DownCast(Self());
  self.SetMinimumWidth(style.GetMinimumWidth());
  self.SetMinimumHeight(style.GetMinimumHeight());
  self.SetPadding(style.GetPadding());
  self.SetStateEffect(style.GetStateEffect());
  // Confine press/focus feedback to the glyph only: the interactive area stays the whole
  // CheckBox (icon + label), but the overlay effect targets the icon so the label area
  // (and any other region) does not animate on touch press/release.
  Ui::View iconView = mIcon;
  self.SetStateEffectTarget(iconView);

  mBoxSize           = style.GetBoxSize();
  mGap               = style.GetLabelGap();
  mIconUrl           = style.GetIconUrl();
  mIconColor         = style.GetIconColor();
  mSelectedIconColor = style.GetSelectedIconColor();

  mDynamicPropertyId = AllocateDynamicPropertyId();

  mIcon.SetResourceUrl(mIconUrl);

  mLabel.SetTextColor(style.GetLabelColor());

  // Seat the initial (unchecked) resting frame + a11y bit.
  RefreshRestingFrame();
  UpdateAccessibility(IsSelected());
}

void CheckBoxImpl::OnSelectionChanged(View /*view*/, bool selected, InputEvent event)
{
  ApplySelectionVisual(selected, IsSelectionAnimationRequired(event));
  UpdateAccessibility(selected);
}

bool CheckBoxImpl::IsSelectionAnimationRequired(const InputEvent& event) const
{
  if(mSelectionAnimationMode == SelectionAnimationMode::DISABLED)
  {
    return false;
  }
  if(!(IsOnScene() && IsVisible())) // animatable only when on-scene and visible
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

void CheckBoxImpl::ApplySelectionVisual(bool selected, bool animate)
{
  const int start = selected ? SELECT_START : DESELECT_START;
  const int end   = selected ? SELECT_END : DESELECT_END;

  // SetMinMaxFrame()+JumpToFrame() rebuild the Lottie visual (mVisualDirty), which BOTH
  // drops any registered dynamic property AND constrains JumpToFrame to the play range.
  // So: set the segment range, jump to the target frame, then RE-SEAT the inner-fill
  // recolour on the rebuilt visual (SetDynamicProperty applies without re-dirtying) before
  // playing. Without the re-seat the themed recolour is lost on the first animated toggle.
  mIcon.SetMinMaxFrame(start, end);
  mIcon.JumpToFrame(animate ? start : end);
  RegisterInnerFillRecolor();
  if(animate)
  {
    mIcon.Play();
  }
}

void CheckBoxImpl::RefreshRestingFrame()
{
  if(mIcon.GetPlayState() == AnimatedImage::PlayState::PLAYING)
  {
    return; // don't disturb a running segment
  }
  ApplySelectionVisual(IsSelected(), false); // snap to the correct resting frame + range
}

void CheckBoxImpl::RegisterInnerFillRecolor()
{
  {
    std::lock_guard<std::mutex> lock(RecolorMutex());
    RecolorRegistry()[mDynamicPropertyId] = InnerColors{mIconColor.GetRgba(), mSelectedIconColor.GetRgba()};
  }
  LottieAnimation::DynamicPropertyInfo info;
  info.id       = mDynamicPropertyId;
  info.keyPath  = INNER_FILL_KEY_PATH;
  info.property = LottieAnimation::VectorProperty::FILL_COLOR;
  info.callback = MakeCallback(&OnInnerFillColor);
  mIcon.SetDynamicProperty(info);
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
  // Re-resolve the tokens for the recolour callback.
  if(mIcon.GetPlayState() == AnimatedImage::PlayState::PLAYING)
  {
    mThemeRefreshPending = true; // apply when the current segment finishes
    return;
  }
  RegisterInnerFillRecolor();
  RefreshRestingFrame();
}

void CheckBoxImpl::OnAnimationFinished(View /*view*/)
{
  if(mThemeRefreshPending)
  {
    RegisterInnerFillRecolor();
    mThemeRefreshPending = false;
  }
  RefreshRestingFrame(); // re-assert the resting frame after playback
}

void CheckBoxImpl::OnSceneConnection(int depth)
{
  Provider::SelectableViewImpl::OnSceneConnection(depth); // base first
  RefreshRestingFrame();                                  // show the correct resting frame on (re-)attach
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

  Ui::View iconView = mIcon; // LottieAnimationView -> View, use public GetImpl(Ui::View&)
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

CheckBoxImpl::~CheckBoxImpl()
{
  if(mDynamicPropertyId != 0)
  {
    std::lock_guard<std::mutex> lock(RecolorMutex());
    RecolorRegistry().erase(mDynamicPropertyId);
  }
}

} // namespace Internal
} // namespace Ui
} // namespace Dali
