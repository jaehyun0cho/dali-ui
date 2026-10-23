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
#include <algorithm>
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
 * @brief A multi-line Label without text fit: the WRAP_CONTENT height branch of its measure, at
 * the exact width.
 *
 * Mirrors LabelImpl::OnMeasure: the text height at the width (0 for a width of 0), clamped by the
 * budget and the maximum first and then by the minimum (the minimum wins there), followed by the
 * clamp Measure() applies to every result (ApplyConstraints, where the maximum wins). Clamping the
 * raw budget instead of Measure()'s pre-clamped one gives the same value for every min/max order.
 * GetHeightForWidth() is the very text metric query the measure itself makes, so no new kind of
 * side effect is introduced; the Label's own code is not involved beyond it.
 */
HeightAtWidth ResolveLabelHeight(Ui::Integration::LabelImpl& label, float width, float heightBudget)
{
  const float s                = label.GetEffectiveScale();
  const float minHeight        = label.GetMinimumHeight() * s;
  const float maxHeight        = label.GetMaximumHeight() * s;
  const float allowedMaxHeight = (heightBudget >= 0.0f) ? std::min(maxHeight, heightBudget) : maxHeight;
  const float height           = (width > 0.0f) ? std::max(0.0f, label.GetHeightForWidth(width)) : 0.0f;
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

  // The producers are ranked as Measure() and Arrange() rank them: a layout callback outranks
  // everything, so the view's own derivation would not describe its layout.
  const ViewDataImpl& data = ViewDataImpl::Get(view);
  if(data.HasLayoutCallback())
  {
    return Unresolved(view);
  }

  // A layout manager outranks OnMeasure()/OnArrange(), whatever the view type (a plain View may
  // own one through View::AttachLayoutManager).
  if(LayoutManager* manager = data.GetLayoutManager())
  {
    return ResolveManagedHeight(view, *manager, width, heightBudget);
  }

  const std::type_info& type = typeid(view);
  if(type == typeid(ViewImpl))
  {
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

} // namespace Internal
} // namespace Ui
} // namespace DALI_NAMESPACE
