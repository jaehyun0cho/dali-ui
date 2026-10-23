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
#include <dali-ui-foundation/internal/layouts/height-for-width-resolution.h>

// EXTERNAL INCLUDES
#include <dali/integration-api/adaptor-framework/adaptor.h>
#include <dali/public-api/signals/connection-tracker.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <typeinfo>
#include <vector>

// INTERNAL INCLUDES
#include <dali-ui-foundation/integration-api/label-impl.h>
#include <dali-ui-foundation/internal/views/view/view-data-impl.h>
#include <dali-ui-foundation/public-api/layouts/absolute-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/flex-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/grid-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/scroll-view-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/stack-layout-manager.h>
#include <dali-ui-foundation/public-api/traits/trait-id.h>
#include <dali-ui-foundation/public-api/traits/trait-object.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>

namespace DALI_NAMESPACE
{
namespace Ui
{
namespace Internal
{
namespace
{

/**
 * @brief The height budget a view's producer would receive from Measure().
 *
 * Measure() clamps the incoming constraint to the view's own min/max height (in natural units)
 * before it hands it to the producer (ViewDataImpl::Measure). The resolution starts from the same
 * budget, so a container's derivation sees the extent its measure would have seen.
 */
float ClampHeightBudget(const ViewImpl& view, float heightBudget, float s)
{
  const float natural = (heightBudget > 0.0f && s > 0.0f) ? heightBudget / s : heightBudget;
  const float clamped = std::min(std::max(natural, view.GetMinimumHeight()), view.GetMaximumHeight());
  return (clamped >= 0.0f) ? clamped * s : clamped;
}

/**
 * @brief The view's own min/max height applied to a visual height, as ViewDataImpl::ApplyConstraints
 * applies it to a measured height (the maximum wins a conflict).
 */
float ApplyHeightLimits(const ViewImpl& view, float height, float s)
{
  return std::min(std::max(height, view.GetMinimumHeight() * s), view.GetMaximumHeight() * s);
}

/**
 * @brief The result for a view whose height is not resolved: its measured height, not dependent.
 */
HeightAtWidth Unresolved(const ViewImpl& view)
{
  return HeightAtWidth{view.GetMeasuredSize().height, false};
}

/**
 * @brief The trait id of a resolved Label's text-height memo (LabelHeightForWidthMemo).
 *
 * File-local: TraitId::Alloc() hands out unique ids in any static-initialization order.
 */
const TraitId LABEL_HEIGHT_FOR_WIDTH_MEMO_TRAIT_ID = TraitId::Alloc();

/**
 * @brief The generation of the locale: bumped on every locale change the adaptor reports.
 *
 * Event thread only, like every resolution, so a plain counter. A memo stores the generation it was
 * taken under (LabelHeightForWidthMemo::localeGeneration); a later locale change makes it differ.
 */
uint32_t gLocaleGeneration = 0u;

/**
 * @brief Bumps gLocaleGeneration from the adaptor's locale-changed signal.
 *
 * A Label reacts to a locale change by dropping its font data (LabelImpl::OnLocaleChanged), which
 * changes its text height without invalidating the layout.
 */
class LocaleChangeObserver final : public ConnectionTracker
{
public:
  void OnLocaleChanged(std::string /* locale */)
  {
    ++gLocaleGeneration;
  }
};

/**
 * @brief The one observer, constructed on the first memo store.
 *
 * Either destruction order is safe: a dying signal disconnects the observer, and a dying observer
 * disconnects from a living signal.
 */
LocaleChangeObserver& GetLocaleChangeObserver()
{
  static LocaleChangeObserver observer;
  return observer;
}

/**
 * @brief Connects the observer while it is unconnected: on the first memo store, and again after a
 * destroyed adaptor dropped the connection. Without an adaptor there is no locale change to miss.
 */
void ObserveLocaleChanges()
{
  LocaleChangeObserver& observer = GetLocaleChangeObserver();
  if(observer.GetConnectionCount() == 0u && Dali::Adaptor::IsAvailable())
  {
    Dali::Adaptor::Get().LocaleChangedSignal().Connect(&observer, &LocaleChangeObserver::OnLocaleChanged);
  }
}

/**
 * @brief The outline width the text layout consumes: the outline width while an outline is enabled,
 * 0 otherwise.
 *
 * GetTextOutline() returns by value (one allocation) and returns Outline::None() while no outline is
 * enabled, whose GetWidth() asserts, so None is tested first.
 */
float LayoutOutlineWidth(const Ui::Integration::LabelImpl& label)
{
  const Ui::Text::Outline outline = label.GetTextOutline();
  return (outline == Ui::Text::Outline::None()) ? 0.0f : outline.GetWidth();
}

/**
 * @brief The text height of the width a multi-line Label was last resolved at.
 *
 * Attached to a Label as a trait by its first resolution while its measure cache entry stands, and
 * dropped (valid = false) wherever the Label's layout caches are (DropHeightForWidthMemo, from
 * ViewDataImpl::InvalidateLayoutCaches). Every Label input that changes GetHeightForWidth() and
 * invalidates the layout drops it that way. The inputs that change the text height WITHOUT
 * invalidating the layout -- the line wrap mode, the letter spacing, the overflow mode, the layout
 * direction mode, the text outline (LayoutOutlineWidth) and the locale (gLocaleGeneration) -- are
 * kept as key fields and compared on every read; the multi-line switch keeps the Label out of the
 * resolution altogether while it is single-line.
 */
class LabelHeightForWidthMemo final : public TraitObject
{
public:
  // rawHeight is the text height at width, text plus Label padding: max(0, GetHeightForWidth(width)),
  // or the measured height when the skip rule stored it (the same value after the resolution's
  // clamps, MeasuredHeightIsTextHeight).
  float                         width{0.0f};         ///< The width rawHeight was taken at, visual units
  float                         rawHeight{0.0f};     ///< The text height at width, unclamped (see above)
  Ui::Text::LineWrapMode        lineWrapMode{};      ///< Key fields: the inputs that change the text height without
  float                         letterSpacing{0.0f}; ///< invalidating the layout, as of the last store
  Ui::Text::OverflowMode        overflowMode{};
  Ui::Text::LayoutDirectionMode layoutDirectionMode{};
  float                         outlineWidth{0.0f};     ///< LayoutOutlineWidth()
  uint32_t                      localeGeneration{0u};   ///< gLocaleGeneration
  bool                          valid{false};           ///< width/rawHeight hold a value; cleared with the layout caches
  bool                          measuredTrusted{false}; ///< The measured height may stand in for the text height

protected:
  ~LabelHeightForWidthMemo() override = default;
};

/**
 * @brief The Label's memo, or nullptr.
 */
LabelHeightForWidthMemo* GetLabelHeightForWidthMemo(const ViewDataImpl& data, IntrusivePtr<TraitObject>& holder)
{
  holder = data.GetTrait(LABEL_HEIGHT_FOR_WIDTH_MEMO_TRAIT_ID);
  return static_cast<LabelHeightForWidthMemo*>(holder.Get());
}

/**
 * @brief Whether the Label's measured height provably equals its text height at @p width, up to the
 * clamps the resolution re-applies.
 *
 * True when the last measure ran LabelImpl::OnMeasure's WRAP_CONTENT height branch at (within
 * HEIGHT_FOR_WIDTH_FIT_EPSILON) this width and its result was not clamped by its budget or its
 * maximum: OnMeasure then computed hm = max(0, GetHeightForWidth(width)) and the measured height is
 * max(hm, minimum height), which the resolution's clamps map to the same result as hm for every
 * budget. The caller only asks while the measure cache entry stands, so that measure ran under
 * today's inputs (only a producer pass re-validates the entry).
 *
 * @param[in] label The Label
 * @param[in] data Its ViewDataImpl
 * @param[in] width The width it is being resolved at, visual units
 * @param[in] s Its effective scale
 * @param[in] maxHeight Its maximum height, visual units
 */
bool MeasuredHeightIsTextHeight(Ui::Integration::LabelImpl& label, const ViewDataImpl& data, float width, float s, float maxHeight)
{
  // Exact, like the measure hit test.
  if(data.GetLastMeasureScale() != s)
  {
    return false;
  }

  // The effective natural constraint of the last measure: times the scale it is the constraint
  // OnMeasure received.
  const MeasuredSize key      = data.GetLastMeasureConstraint();
  const MeasuredSize measured = label.GetMeasuredSize();

  // OnMeasure's widthForHeight: its width constraint for a MATCH_PARENT width, its measured width
  // otherwise -- which ApplyConstraints leaves untouched while the minimum width does not exceed
  // the maximum (OnMeasure's measured width lies between them then).
  float widthForHeight = 0.0f;
  if(label.GetRequestedWidth() == MATCH_PARENT)
  {
    widthForHeight = key.width * s;
  }
  else if(label.GetMinimumWidth() <= label.GetMaximumWidth())
  {
    widthForHeight = measured.width;
  }
  else
  {
    return false;
  }

  if(!(widthForHeight > 0.0f) || std::abs(width - widthForHeight) > HEIGHT_FOR_WIDTH_FIT_EPSILON)
  {
    return false;
  }

  // Not clamped: below both the maximum height and the height constraint OnMeasure received.
  return measured.height + HEIGHT_FOR_WIDTH_FIT_EPSILON < std::min(maxHeight, key.height * s);
}

/**
 * @brief max(0, label.GetHeightForWidth(width)) for a width > 0, served from the memo or from the
 * measured height when they provably hold it.
 *
 * Nothing is read from or written to the memo while the Label's measure cache entry is invalid: a
 * text change that does not invalidate the layout again (the Label latches its own invalidation
 * until its next measure) can only happen while the entry is invalid, so a memo stored while it
 * stands always predates such a change, and the drop that invalidated the entry has cleared it.
 *
 * Widths within HEIGHT_FOR_WIDTH_FIT_EPSILON count as one width: the resolver's own tolerance (the
 * fit test's), coarser than the tolerance of the text controller's own height-for-width cache
 * (Math::MACHINE_EPSILON_1000, about 1.19e-4).
 *
 * @param[in] label The Label
 * @param[in] width The width it is being resolved at, visual units, > 0
 * @param[in] s Its effective scale
 * @param[in] maxHeight Its maximum height, visual units
 * @return The text height at @p width, unclamped
 */
float TextHeightAtWidth(Ui::Integration::LabelImpl& label, float width, float s, float maxHeight)
{
  ViewDataImpl& data = ViewDataImpl::Get(label);
  if(!data.IsMeasureCacheValid())
  {
    return std::max(0.0f, label.GetHeightForWidth(width));
  }

  const float outlineWidth = LayoutOutlineWidth(label);

  IntrusivePtr<TraitObject> holder;
  LabelHeightForWidthMemo*  memo = GetLabelHeightForWidthMemo(data, holder);

  const Ui::Text::LineWrapMode        lineWrapMode        = label.GetLineWrapMode();
  const float                         letterSpacing       = label.GetLetterSpacing();
  const Ui::Text::OverflowMode        overflowMode        = label.GetTextOverflowMode();
  const Ui::Text::LayoutDirectionMode layoutDirectionMode = label.GetLayoutDirectionMode();

  const bool sameInputs = memo && memo->lineWrapMode == lineWrapMode && memo->letterSpacing == letterSpacing &&
                          memo->overflowMode == overflowMode && memo->layoutDirectionMode == layoutDirectionMode &&
                          memo->outlineWidth == outlineWidth && memo->localeGeneration == gLocaleGeneration;
  if(memo && !sameInputs)
  {
    // A change that did not invalidate the layout, seen now: the measured height may predate it.
    memo->measuredTrusted = false;
  }

  if(sameInputs && memo->valid && std::abs(width - memo->width) <= HEIGHT_FOR_WIDTH_FIT_EPSILON)
  {
    return memo->rawHeight;
  }

  float height = 0.0f;
  if(sameInputs && memo->measuredTrusted && MeasuredHeightIsTextHeight(label, data, width, s, maxHeight))
  {
    height = label.GetMeasuredSize().height;
  }
  else
  {
    height = std::max(0.0f, label.GetHeightForWidth(width));
  }

  if(!memo)
  {
    // A fresh memo does not trust the measured height: the Label may have changed, without
    // invalidating the layout, since it was measured.
    holder = new LabelHeightForWidthMemo();
    memo   = static_cast<LabelHeightForWidthMemo*>(holder.Get());
    data.SetTrait(LABEL_HEIGHT_FOR_WIDTH_MEMO_TRAIT_ID, holder);
  }
  ObserveLocaleChanges();
  memo->width               = width;
  memo->rawHeight           = height;
  memo->lineWrapMode        = lineWrapMode;
  memo->letterSpacing       = letterSpacing;
  memo->overflowMode        = overflowMode;
  memo->layoutDirectionMode = layoutDirectionMode;
  memo->outlineWidth        = outlineWidth;
  memo->localeGeneration    = gLocaleGeneration;
  memo->valid               = true;
  return height;
}

/**
 * @brief A resolution found the Label single-line or text-fit: its measured height no longer stands
 * for its multi-line text height, so the skip rule must not use it until the next invalidation.
 */
void DistrustMeasuredTextHeight(const ViewDataImpl& data)
{
  IntrusivePtr<TraitObject> holder;
  if(LabelHeightForWidthMemo* memo = GetLabelHeightForWidthMemo(data, holder))
  {
    memo->measuredTrusted = false;
  }
}

/**
 * @brief A multi-line Label without text fit: the WRAP_CONTENT height branch of its measure, at
 * the exact width.
 *
 * Mirrors LabelImpl::OnMeasure: the text height at the width (0 for a width of 0), clamped by the
 * budget and the maximum first and then by the minimum (the minimum wins there), followed by the
 * clamp Measure() applies to every result (ApplyConstraints, where the maximum wins). Clamping the
 * raw budget instead of Measure()'s pre-clamped one gives the same value for every min/max order.
 * GetHeightForWidth() is the very text metric query the measure itself makes, so no new kind of
 * side effect is introduced; the Label's own code is not involved beyond it. The text height comes
 * from TextHeightAtWidth(): the memo or the measured height when they provably hold it, else
 * GetHeightForWidth().
 */
HeightAtWidth ResolveLabelHeight(Ui::Integration::LabelImpl& label, float width, float heightBudget)
{
  const float s                = label.GetEffectiveScale();
  const float minHeight        = label.GetMinimumHeight() * s;
  const float maxHeight        = label.GetMaximumHeight() * s;
  const float allowedMaxHeight = (heightBudget >= 0.0f) ? std::min(maxHeight, heightBudget) : maxHeight;
  const float height           = (width > 0.0f) ? TextHeightAtWidth(label, width, s, maxHeight) : 0.0f;
  float       result           = std::max(std::min(height, allowedMaxHeight), minHeight);
  result                       = std::min(std::max(result, minHeight), maxHeight);
  return HeightAtWidth{result, true};
}

/**
 * @brief A plain View: the WRAP_CONTENT height of its default measure, with the children derived
 * at the exact width (DeriveDefaultChildHeights).
 *
 * The padding is subtracted per edge, the way ArrangeDefault subtracts it, so the derivation hands
 * a MATCH_PARENT child the very width the default arrange gives it. A MATCH_PARENT-width view is
 * re-measured at its slot by its parent right before it is arranged, so its WRAP_CONTENT-width
 * children are taken at the width that re-measure gives them (PredictRemeasuredWidth), from the
 * content width the default measure gets there: the slot width clamped by the view's own
 * minimum/maximum width (ClampedWidthConstraint), in natural units less the padding, times the
 * scale -- the default measure's own form.
 *
 * The derived content must fit the content budget. The view's own arrange re-derives its
 * children within the height it is given and refuses a layout that does not fit, so a view whose
 * resolved content exceeds its budget is not resolved: its box would otherwise be sized for
 * content it does not show. Past that guard the result is the largest child bottom plus the
 * vertical padding, capped by the budget as the WRAP_CONTENT branch of the default measure caps it
 * (after the guard the cap can trim at most the fit tolerance), then clamped by the view's own
 * min/max height.
 */
HeightAtWidth ResolveDefaultHeight(ViewImpl& view, float width, float heightBudget)
{
  const float  s             = view.GetEffectiveScale();
  const Insets padding       = view.GetPadding();
  const float  padL          = static_cast<float>(padding.start) * s;
  const float  padR          = static_cast<float>(padding.end) * s;
  const float  padT          = static_cast<float>(padding.top) * s;
  const float  padB          = static_cast<float>(padding.bottom) * s;
  const float  budget        = ClampHeightBudget(view, heightBudget, s);
  const float  contentBudget = budget - padT - padB;

  // The children in their child order, captured the way the layout managers capture them.
  const uint32_t        count = view.GetChildViewCount();
  std::vector<Ui::View> children;
  children.reserve(count);
  for(uint32_t i = 0; i < count; ++i)
  {
    children.push_back(view.GetChildViewAt(i));
  }

  // Every supported parent re-measures a MATCH_PARENT-width child at its slot -- this very width --
  // right before arranging it (see PredictRemeasuredWidth); Measure() clamps that width by this
  // view's own min/max before the default measure takes its content width from it.
  float predictedContentWidth = NO_REMEASURE_PREDICTION;
  if(view.GetRequestedWidth() == MATCH_PARENT)
  {
    const float constraint = ClampedWidthConstraint(view, width);
    const float natural    = (constraint >= 0.0f && s > 0.0f) ? constraint / s : constraint;
    predictedContentWidth  = std::max(0.0f, natural - static_cast<float>(padding.start + padding.end)) * s;
  }

  std::vector<float> unused;
  float              needed = 0.0f;
  if(!DeriveDefaultChildHeights(view, children, width - padL - padR, contentBudget, predictedContentWidth, unused, needed))
  {
    return Unresolved(view);
  }

  // Its own arrange would refuse content taller than the content budget (see above).
  if(needed > contentBudget + HEIGHT_FOR_WIDTH_FIT_EPSILON)
  {
    return Unresolved(view);
  }

  // The cap of the default measure's WRAP_CONTENT branch; after the guard it trims at most the
  // fit tolerance.
  const float height = std::min(needed + padT + padB, budget);
  return HeightAtWidth{ApplyHeightLimits(view, height, s), true};
}

/**
 * @brief A view with a supported layout manager: the manager's derivation at the content width.
 *
 * The content extents follow the layout-manager dispatch of Measure() and Arrange(): the content
 * width is the width less the padding (the content box the manager's Arrange() receives), the
 * content budget is the clamped budget less the padding (the content constraint the manager's
 * Measure() receives). The WRAP_CONTENT result adds the padding back, without a budget cap, as the
 * dispatch does, and then the view's own min/max height applies. Content that exceeds the content
 * budget is not resolved, for the reason ResolveDefaultHeight gives. A MATCH_PARENT-width view is
 * re-measured at its slot before it is arranged, so the derivation predicts its WRAP_CONTENT-width
 * children's widths (PredictRemeasuredWidth) from the content width the manager's measure gets
 * there: the slot width clamped by the view's own minimum/maximum width (ClampedWidthConstraint),
 * less the padding, as the dispatch computes it. Only the exact manager types are recognized;
 * every other manager -- ScrollViewLayoutManager and any custom or derived one -- is not resolved.
 */
HeightAtWidth ResolveManagedHeight(ViewImpl& view, LayoutManager& manager, float width, float heightBudget)
{
  const float  s             = view.GetEffectiveScale();
  const Insets padding       = view.GetPadding();
  const float  visPadW       = static_cast<float>(padding.start + padding.end) * s;
  const float  visPadH       = static_cast<float>(padding.top + padding.bottom) * s;
  const float  contentWidth  = std::max(0.0f, width - visPadW);
  const float  contentBudget = std::max(0.0f, ClampHeightBudget(view, heightBudget, s) - visPadH);

  // Every supported parent re-measures a MATCH_PARENT-width child at its slot -- this very width --
  // right before arranging it (see PredictRemeasuredWidth); Measure() clamps that width by this
  // view's own min/max before the dispatch subtracts the padding.
  const float predictedContentWidth = (view.GetRequestedWidth() == MATCH_PARENT) ? std::max(0.0f, ClampedWidthConstraint(view, width) - visPadW) : NO_REMEASURE_PREDICTION;

  float                 contentHeight = 0.0f;
  bool                  depends       = false;
  const std::type_info& type          = typeid(manager);
  if(type == typeid(StackLayoutManager))
  {
    depends = ResolveStackContentHeightForWidth(static_cast<const StackLayoutManager&>(manager), view, contentWidth, contentBudget, contentHeight, predictedContentWidth);
  }
  else if(type == typeid(GridLayoutManager))
  {
    depends = ResolveGridContentHeightForWidth(static_cast<const GridLayoutManager&>(manager), view, contentWidth, contentBudget, contentHeight, predictedContentWidth);
  }
  else if(type == typeid(FlexLayoutManager))
  {
    depends = ResolveFlexContentHeightForWidth(static_cast<const FlexLayoutManager&>(manager), view, contentWidth, contentBudget, contentHeight, predictedContentWidth);
  }
  else if(type == typeid(AbsoluteLayoutManager))
  {
    depends = ResolveAbsoluteContentHeightForWidth(view, contentWidth, contentBudget, contentHeight, predictedContentWidth);
  }

  if(!depends)
  {
    return Unresolved(view);
  }

  // Its own arrange would refuse content taller than the content budget (see ResolveDefaultHeight).
  if(contentHeight > contentBudget + HEIGHT_FOR_WIDTH_FIT_EPSILON)
  {
    return Unresolved(view);
  }
  return HeightAtWidth{ApplyHeightLimits(view, contentHeight + visPadH, s), true};
}

/**
 * @brief Whether a layout manager has a derivation of its own: the ResolveManagedHeight dispatch.
 */
bool HasManagerDerivation(const LayoutManager& manager)
{
  const std::type_info& type = typeid(manager);
  return type == typeid(StackLayoutManager) ||
         type == typeid(GridLayoutManager) ||
         type == typeid(FlexLayoutManager) ||
         type == typeid(AbsoluteLayoutManager);
}

/**
 * @brief The structural verdict: can a resolution of @p view depend on its width -- now, or after a
 * change that does not invalidate the layout (a Label's multi-line or text-fit switch)?
 *
 * Over-approximates HeightAtWidth::dependsOnWidth: a resolution can report a dependence only through
 * a resolved Label, a wrapping horizontal FlexLayoutManager, or a child that reports one, and every
 * derivation visits only non-STANDALONE children with a WRAP_CONTENT height. So when no Label with a
 * WRAP_CONTENT height and no wrapping horizontal FlexLayoutManager is reachable through supported
 * containers, every resolution of @p view reports its measured height, not dependent, whatever the
 * width and budget. The verdict of a supported container is stored on its view
 * (ViewDataImpl::GetHeightForWidthState) until a structural change resets it; a Label's is never
 * stored (volatile: its multi-line and text-fit switches do not invalidate the layout), and an
 * unsupported view answers false at once.
 */
bool MayDependOnWidth(ViewImpl& view)
{
  if(view.GetRequestedHeight() != WRAP_CONTENT)
  {
    return false;
  }

  ViewDataImpl&                           data  = ViewDataImpl::Get(view);
  const ViewDataImpl::HeightForWidthState state = data.GetHeightForWidthState();
  if(state != ViewDataImpl::HeightForWidthState::UNKNOWN)
  {
    return state == ViewDataImpl::HeightForWidthState::DEPENDENT;
  }

  if(data.HasLayoutCallback())
  {
    return false;
  }

  LayoutManager* manager = data.GetLayoutManager();
  if(manager)
  {
    if(!HasManagerDerivation(*manager))
    {
      return false; // ScrollViewLayoutManager and any custom or derived manager
    }
  }
  else if(typeid(view) == typeid(Ui::Integration::LabelImpl))
  {
    return true; // Volatile: never stored
  }
  else if(typeid(view) != typeid(ViewImpl))
  {
    return false;
  }

  bool depends = false;
  if(manager && typeid(*manager) == typeid(FlexLayoutManager))
  {
    // A wrapping row's line breaks follow its width by themselves.
    const auto& flex = static_cast<const FlexLayoutManager&>(*manager);
    depends          = flex.IsMainAxisHorizontal() && flex.GetWrap() != FlexWrap::NO_WRAP;
  }
  for(uint32_t i = 0, count = view.GetChildViewCount(); i < count && !depends; ++i)
  {
    Ui::View  child     = view.GetChildViewAt(i);
    ViewImpl& childImpl = GetImpl(child);
    depends             = childImpl.GetLayoutMode() != LayoutMode::STANDALONE && MayDependOnWidth(childImpl);
  }

  data.SetHeightForWidthState(depends ? ViewDataImpl::HeightForWidthState::DEPENDENT : ViewDataImpl::HeightForWidthState::INDEPENDENT);
  return depends;
}

} // namespace

bool IsHeightForWidthContainer(ViewImpl& view)
{
  const ViewDataImpl& data = ViewDataImpl::Get(view);
  if(data.HasLayoutCallback())
  {
    return false;
  }

  if(LayoutManager* manager = data.GetLayoutManager())
  {
    const std::type_info& type = typeid(*manager);
    return type == typeid(StackLayoutManager) ||
           type == typeid(GridLayoutManager) ||
           type == typeid(FlexLayoutManager) ||
           type == typeid(AbsoluteLayoutManager) ||
           type == typeid(ScrollViewLayoutManager);
  }

  return typeid(view) == typeid(ViewImpl);
}

HeightAtWidth ResolveHeightAtWidth(ViewImpl& view, float width, float heightBudget)
{
  // Only a WRAP_CONTENT height follows the content; a MATCH_PARENT or explicit height is decided
  // by the parent's rule. The sentinel compare is exact: the requested-size setters snap a
  // near-sentinel input to the exact sentinel.
  if(view.GetRequestedHeight() != WRAP_CONTENT)
  {
    return Unresolved(view);
  }

  // A subtree whose verdict says it cannot depend on its width is answered without being looked
  // into (MayDependOnWidth): exactly the result a full walk would reach.
  ViewDataImpl& data = ViewDataImpl::Get(view);
  if(data.GetHeightForWidthState() == ViewDataImpl::HeightForWidthState::INDEPENDENT)
  {
    return Unresolved(view);
  }

  // The producers are ranked as Measure() and Arrange() rank them: a layout callback outranks
  // everything, so the view's own derivation would not describe its layout.
  if(data.HasLayoutCallback())
  {
    return Unresolved(view);
  }

  // A layout manager outranks OnMeasure()/OnArrange(), whatever the view type (a plain View may
  // own one through View::AttachLayoutManager).
  if(LayoutManager* manager = data.GetLayoutManager())
  {
    if(!MayDependOnWidth(view))
    {
      return Unresolved(view);
    }
    return ResolveManagedHeight(view, *manager, width, heightBudget);
  }

  const std::type_info& type = typeid(view);
  if(type == typeid(ViewImpl))
  {
    if(!MayDependOnWidth(view))
    {
      return Unresolved(view);
    }
    return ResolveDefaultHeight(view, width, heightBudget);
  }

  if(type == typeid(Ui::Integration::LabelImpl))
  {
    auto& label = static_cast<Ui::Integration::LabelImpl&>(view);
    // A single-line Label's height does not follow its width. A text-fit Label (IsTextFitActive(),
    // the condition its measure tests) measures under a temporarily changed font configuration,
    // which a metric query cannot reproduce.
    if(label.IsMultiLine() && !label.IsTextFitActive())
    {
      return ResolveLabelHeight(label, width, heightBudget);
    }

    // Its measured height no longer stands for its multi-line text height (TextHeightAtWidth): a
    // switch back to multi-line does not invalidate the layout.
    DistrustMeasuredTextHeight(data);
  }

  return Unresolved(view);
}

bool DeriveDefaultChildHeights(ViewImpl& container, const std::vector<Ui::View>& children, float contentWidth, float contentHeight, float predictedContentWidth, std::vector<float>& resolvedHeights, float& neededContentHeight)
{
  const float s         = container.GetEffectiveScale();
  bool        depends   = false;
  float       maxBottom = 0.0f;

  for(uint32_t i = 0; i < children.size(); ++i)
  {
    Ui::View  childView = children[i];
    ViewImpl& childImpl = GetImpl(childView);
    if(childImpl.GetLayoutMode() == LayoutMode::STANDALONE)
    {
      continue;
    }

    // The margin forms of ArrangeDefault: each edge scaled on its own.
    const float        childScale = childImpl.GetEffectiveScale();
    const Insets       margin     = childImpl.GetMargin();
    const float        visMarginW = static_cast<float>(margin.start) * childScale + static_cast<float>(margin.end) * childScale;
    const float        visMarginH = static_cast<float>(margin.top) * childScale + static_cast<float>(margin.bottom) * childScale;
    const MeasuredSize measured   = childImpl.GetMeasuredSize();

    float height = measured.height;
    if(childImpl.GetRequestedHeight() == WRAP_CONTENT)
    {
      // The width ArrangeDefault gives the child, and the budget the default measure gives it with
      // the final content height in place of the measure-time one.
      float width = measured.width;
      if(childImpl.GetRequestedWidth() == MATCH_PARENT)
      {
        width = std::max(0.0f, contentWidth - visMarginW);
      }
      else if(predictedContentWidth >= 0.0f && childImpl.GetRequestedWidth() == WRAP_CONTENT)
      {
        // The resolution of a MATCH_PARENT-width container: its parent re-measures it at its slot
        // before it is arranged, and that re-measure hands this child the predicted content width
        // less its margin (the default measure's constraint form). A MATCH_PARENT child above keeps
        // the unclamped content width: the arrange gives it the slot.
        width = PredictRemeasuredWidth(childImpl, measured.width, std::max(0.0f, predictedContentWidth - static_cast<float>(margin.start + margin.end) * childScale));
      }

      const HeightAtWidth resolved = ResolveHeightAtWidth(childImpl, width, std::max(0.0f, contentHeight - visMarginH));
      if(resolved.dependsOnWidth)
      {
        if(resolvedHeights.empty())
        {
          resolvedHeights.assign(children.size(), -1.0f);
        }
        resolvedHeights[i] = resolved.height;
        height             = resolved.height;
        depends            = true;
      }
    }

    // The requested position is scaled by the OWNER scale, as ArrangeDefault places it.
    maxBottom = std::max(maxBottom, childImpl.GetRequestedY() * s + visMarginH + height);
  }

  if(depends)
  {
    neededContentHeight = maxBottom;
  }
  return depends;
}

void ApplyRootHeightForWidth(ViewImpl& view, const StandaloneSlotInputs& inputs, float heightBudget, LayoutRect& bounds)
{
  if(inputs.requestedHeight != WRAP_CONTENT)
  {
    return;
  }

  const HeightAtWidth resolved = ResolveHeightAtWidth(view, bounds.width, heightBudget);
  if(!resolved.dependsOnWidth)
  {
    return; // The derived slot stands, exactly as before.
  }

  // The clamp DeriveStandaloneRootBounds applies, from the same snapshot.
  bounds.height = std::min(std::max(resolved.height, inputs.minimumHeight * inputs.scale), inputs.maximumHeight * inputs.scale);
}

void DropHeightForWidthMemo(ViewDataImpl& data)
{
  IntrusivePtr<TraitObject> holder;
  if(LabelHeightForWidthMemo* memo = GetLabelHeightForWidthMemo(data, holder))
  {
    memo->valid = false;
    // The next measure follows this invalidation, so its result may stand in for the text height
    // until a change that does not invalidate the layout is seen.
    memo->measuredTrusted = true;
  }
}

} // namespace Internal
} // namespace Ui
} // namespace DALI_NAMESPACE
