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
#include <dali-ui-foundation/public-api/layouts/absolute-layout-manager.h>

// EXTERNAL INCLUDES
#include <algorithm>
#include <vector>

// INTERNAL INCLUDES
#include <dali-ui-foundation/internal/layouts/absolute-layout-params-impl.h>
#include <dali-ui-foundation/internal/layouts/height-for-width-resolution.h>
#include <dali-ui-foundation/internal/layouts/layout-dependency-scope.h>
#include <dali-ui-foundation/internal/layouts/layout-manager-impl.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>

namespace DALI_NAMESPACE
{
namespace Ui
{

namespace
{

LayoutRect GetChildBounds(Internal::AbsoluteLayoutParamsImpl* params)
{
  if(params)
  {
    return params->GetBounds();
  }
  return LayoutRect(0.0f, 0.0f, -1.0f, -1.0f);
}

AbsoluteLayoutFlags GetChildFlags(Internal::AbsoluteLayoutParamsImpl* params)
{
  if(params)
  {
    return params->GetFlags();
  }
  return AbsoluteLayoutFlags::NONE;
}

bool HasAbsoluteFlag(AbsoluteLayoutFlags flags, AbsoluteLayoutFlags flag)
{
  return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(flag)) != 0;
}

// The width Arrange() gives a child: its explicit or proportional width, the available width less
// its margin for a MATCH_PARENT width, its measured width otherwise. With a predictedContentWidth
// (only the resolution of a MATCH_PARENT-width container, which its parent re-measures at its slot
// before arranging it; the content width that re-measure gives the container's measure) a
// WRAP_CONTENT-width child is taken at the width that re-measure gives it: its measured width capped
// by the predicted content width less its margin, the constraint Measure() hands a child without an
// explicit width. Explicit, proportional and MATCH_PARENT widths keep the unclamped available width.
float AbsoluteArrangeWidth(ViewImpl& childImpl, const LayoutRect& spec, bool widthProportional, float availableWidth, float childScale, float predictedContentWidth)
{
  float w = spec.width;
  if(widthProportional)
  {
    w *= availableWidth;
  }
  else if(w > 0.0f)
  {
    w *= childScale;
  }
  if(w < 0)
  {
    if(childImpl.GetRequestedWidth() == MATCH_PARENT)
    {
      Insets margin = childImpl.GetMargin();
      w             = std::max(0.0f, availableWidth - (margin.start + margin.end) * childScale);
    }
    else if(predictedContentWidth >= 0.0f && childImpl.GetRequestedWidth() == WRAP_CONTENT)
    {
      Insets margin = childImpl.GetMargin();
      w             = Internal::PredictRemeasuredWidth(childImpl, childImpl.GetMeasuredSize().width, std::max(0.0f, predictedContentWidth - (margin.start + margin.end) * childScale));
    }
    else
    {
      w = childImpl.GetMeasuredSize().width;
    }
  }
  return w;
}

// The height Arrange() takes from a child's spec before its MATCH_PARENT / measured fallback: the
// explicit or proportional height, negative when the spec sets none.
float AbsoluteSpecHeight(const LayoutRect& spec, bool heightProportional, float availableHeight, float childScale)
{
  float h = spec.height;
  if(heightProportional)
  {
    h *= availableHeight;
  }
  else if(h > 0.0f)
  {
    h *= childScale;
  }
  return h;
}

// Height-for-width: resolves every non-STANDALONE child whose height is its own WRAP_CONTENT (no
// explicit or proportional height in its spec) at the width Arrange() gives it, with the budget
// Measure() hands it. On the first dependent child @p resolved is assigned (child count, -1.0f);
// entry i >= 0 is child i's resolved height. @p predictedContentWidth is not negative only for the
// resolution of a MATCH_PARENT-width container (see AbsoluteArrangeWidth); Arrange() passes
// Internal::NO_REMEASURE_PREDICTION.
//
// @return True when at least one child depends on its width
bool ResolveAbsoluteChildHeights(std::vector<View>& children, float availableWidth, float availableHeight, float predictedContentWidth, std::vector<float>& resolved)
{
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);
    if(childImpl.GetLayoutMode() == LayoutMode::STANDALONE || childImpl.GetRequestedHeight() != WRAP_CONTENT)
    {
      continue;
    }

    float                     childScale = childImpl.GetEffectiveScale();
    auto*                     params     = Internal::AbsoluteLayoutParamsImpl::Get(childImpl);
    const LayoutRect          spec       = GetChildBounds(params);
    const AbsoluteLayoutFlags flags      = GetChildFlags(params);
    if(AbsoluteSpecHeight(spec, HasAbsoluteFlag(flags, AbsoluteLayoutFlags::HEIGHT_PROPORTIONAL), availableHeight, childScale) >= 0.0f)
    {
      continue; // The spec decides the height.
    }

    Insets                        margin = childImpl.GetMargin();
    const Internal::HeightAtWidth result =
      Internal::ResolveHeightAtWidth(childImpl,
                                     AbsoluteArrangeWidth(childImpl, spec, HasAbsoluteFlag(flags, AbsoluteLayoutFlags::WIDTH_PROPORTIONAL), availableWidth, childScale, predictedContentWidth),
                                     std::max(0.0f, availableHeight - (margin.top + margin.bottom) * childScale));
    if(result.dependsOnWidth)
    {
      if(resolved.empty())
      {
        resolved.assign(children.size(), -1.0f);
      }
      resolved[i] = result.height;
    }
  }
  return !resolved.empty();
}

// The bottom extent Measure() reports for a WRAP_CONTENT height, with the resolved heights in place
// of the measured ones: a HEIGHT_PROPORTIONAL child is excluded (its height would be defined by the
// extent it depends on), a Y_PROPORTIONAL child contributes its extent only, every other child its
// scaled spec position plus its extent. A child's extent is its explicit height, 0 for an explicit
// 0, else its resolved height or its measured height, plus its vertical margin.
float AbsoluteWrapContentBottom(std::vector<View>& children, const std::vector<float>& resolved)
{
  float maxBottom = 0.0f;
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);
    if(childImpl.GetLayoutMode() == LayoutMode::STANDALONE)
    {
      continue;
    }

    float                     childScale = childImpl.GetEffectiveScale();
    auto*                     params     = Internal::AbsoluteLayoutParamsImpl::Get(childImpl);
    const LayoutRect          spec       = GetChildBounds(params);
    const AbsoluteLayoutFlags flags      = GetChildFlags(params);
    if(HasAbsoluteFlag(flags, AbsoluteLayoutFlags::HEIGHT_PROPORTIONAL))
    {
      continue;
    }

    float h = spec.height;
    if(h > 0.0f)
    {
      h *= childScale;
    }
    if(h < 0)
    {
      h = (i < resolved.size() && resolved[i] >= 0.0f) ? resolved[i] : childImpl.GetMeasuredSize().height;
    }

    Insets margin  = childImpl.GetMargin();
    float  marginH = (margin.top + margin.bottom) * childScale;
    if(HasAbsoluteFlag(flags, AbsoluteLayoutFlags::Y_PROPORTIONAL))
    {
      maxBottom = std::max(maxBottom, h + marginH);
    }
    else
    {
      maxBottom = std::max(maxBottom, spec.y * childScale + h + marginH);
    }
  }
  return maxBottom;
}

} // namespace

class AbsoluteLayoutManager::Impl : public LayoutManager::Impl
{
};

AbsoluteLayoutManager::AbsoluteLayoutManager()
: LayoutManager(new Impl())
{
}

AbsoluteLayoutManager::~AbsoluteLayoutManager()
{
}

MeasuredSize AbsoluteLayoutManager::Measure(ViewImpl* view, float widthConstraint, float heightConstraint)
{
  if(!view)
  {
    return MeasuredSize(0.0f, 0.0f);
  }

  const uint32_t count         = GetChildViewCount(view);
  float          contentWidth  = widthConstraint;
  float          contentHeight = heightConstraint;

  // Snapshot children up front: a child's Measure callback may remove a
  // sibling mid-pass, which would invalidate a cached-index live traversal.
  std::vector<View> children;
  children.reserve(count);
  for(uint32_t i = 0; i < count; ++i)
  {
    children.push_back(GetChildViewAt(view, i));
  }

  float maxRight  = 0.0f;
  float maxBottom = 0.0f;

  // Size circularity guard. When the container axis is WRAP_CONTENT,
  // its intrinsic size is what we are computing here, yet a proportional
  // child resolves its extent/position against that very (still undetermined)
  // size. Letting such a child feed maxRight/maxBottom would make the
  // proportional result define the basis it depends on. A proportional value
  // therefore does not contribute to the WRAP axis intrinsic size.
  const bool containerWrapW = (view->GetRequestedWidth() == WRAP_CONTENT);
  const bool containerWrapH = (view->GetRequestedHeight() == WRAP_CONTENT);

  for(auto& child : children)
  {
    ViewImpl& childImpl = GetImpl(child);

    if(IsStandalone(&childImpl))
    {
      continue;
    }

    float               childScale = childImpl.GetEffectiveScale();
    auto*               params     = Internal::AbsoluteLayoutParamsImpl::Get(childImpl);
    LayoutRect          bounds     = GetChildBounds(params);
    AbsoluteLayoutFlags flags      = GetChildFlags(params);

    float x = bounds.x;
    float y = bounds.y;
    float w = bounds.width;
    float h = bounds.height;

    const uint8_t rawFlags = static_cast<uint8_t>(flags);
    const bool    xProportional =
      (rawFlags & static_cast<uint8_t>(AbsoluteLayoutFlags::X_PROPORTIONAL)) != 0;
    const bool yProportional =
      (rawFlags & static_cast<uint8_t>(AbsoluteLayoutFlags::Y_PROPORTIONAL)) != 0;
    const bool widthProportional =
      (rawFlags & static_cast<uint8_t>(AbsoluteLayoutFlags::WIDTH_PROPORTIONAL)) != 0;
    const bool heightProportional =
      (rawFlags & static_cast<uint8_t>(AbsoluteLayoutFlags::HEIGHT_PROPORTIONAL)) != 0;

    if(widthProportional)
    {
      w *= contentWidth;
    }
    else if(w > 0.0f)
    {
      w *= childScale;
    }
    if(heightProportional)
    {
      h *= contentHeight;
    }
    else if(h > 0.0f)
    {
      h *= childScale;
    }

    Insets margin  = childImpl.GetMargin();
    float  marginW = (margin.start + margin.end) * childScale;
    float  marginH = (margin.top + margin.bottom) * childScale;

    // Always measure children. When the child has no explicit positive extent
    // on an axis (WRAP_CONTENT / MATCH_PARENT), the constraint is the available
    // content budget clamped to be non-negative; for explicit positive bounds
    // the constraint is the bounds value itself. Calling Measure unconditionally
    // keeps the child's measure cache and dirty state coherent with the layout
    // pass — required so a later InvalidateMeasure can propagate. The child's
    // own cache no-ops when constraints repeat.
    float        measureW  = w >= 0.0f ? w : std::max(0.0f, contentWidth - marginW);
    float        measureH  = h >= 0.0f ? h : std::max(0.0f, contentHeight - marginH);
    MeasuredSize childSize = childImpl.Measure(measureW, measureH);

    if(w < 0)
    {
      w = childSize.width;
    }
    if(h < 0)
    {
      h = childSize.height;
    }

    // Proportional position: axis = (available - childExtent) * proportion
    if(xProportional)
    {
      x = (contentWidth - w) * bounds.x;
    }
    else
    {
      x *= childScale;
    }
    if(yProportional)
    {
      y = (contentHeight - h) * bounds.y;
    }
    else
    {
      y *= childScale;
    }

    // On a WRAP container axis the intrinsic size is what we are
    // computing, so only the genuinely circular contribution must be dropped.
    // A WIDTH_PROPORTIONAL child resolves its extent against that undetermined
    // size, so it is excluded entirely. A position-only proportional child
    // (X_PROPORTIONAL && !WIDTH_PROPORTIONAL) has a determinate extent w; only
    // its proportional position offset x = (contentWidth - w) * p is circular.
    // For it, contribute the determinate extent (w + marginW) while dropping
    // the circular position offset. Non-WRAP axes accumulate normally.
    if(containerWrapW && widthProportional)
    {
      // Size-proportional on a WRAP axis: fully circular, exclude.
    }
    else if(containerWrapW && xProportional)
    {
      // Position-only proportional on a WRAP axis: drop the circular x offset,
      // keep the determinate extent.
      maxRight = std::max(maxRight, w + marginW);
    }
    else
    {
      maxRight = std::max(maxRight, x + w + marginW);
    }

    if(containerWrapH && heightProportional)
    {
      // Size-proportional on a WRAP axis: fully circular, exclude.
    }
    else if(containerWrapH && yProportional)
    {
      maxBottom = std::max(maxBottom, h + marginH);
    }
    else
    {
      maxBottom = std::max(maxBottom, y + h + marginH);
    }
  }

  return MeasuredSize(maxRight, maxBottom);
}

void AbsoluteLayoutManager::Arrange(ViewImpl* view, const LayoutRect& bounds)
{
  if(!view)
  {
    return;
  }

  const uint32_t count           = GetChildViewCount(view);
  float          availableWidth  = bounds.width;
  float          availableHeight = bounds.height;

  // Snapshot children up front: a child's Arrange callback may remove a
  // sibling mid-pass, which would invalidate a cached-index live traversal.
  std::vector<View> children;
  children.reserve(count);
  for(uint32_t i = 0; i < count; ++i)
  {
    children.push_back(GetChildViewAt(view, i));
  }

  // Height-for-width (see height-for-width-resolution.h): a child whose WRAP_CONTENT height follows
  // its width is given the height it needs at the width it is placed at below. Adopted only when
  // the bottom extent of the WRAP_CONTENT measure, rebuilt with those heights, fits the available
  // height; otherwise, and whenever no child depends on its width, every child keeps its measured
  // height. Only an exact AbsoluteLayoutManager on a view without layout callbacks does this.
  std::vector<float> resolvedHeights;
  if(Internal::IsHeightForWidthContainer(*view) &&
     ResolveAbsoluteChildHeights(children, availableWidth, availableHeight, Internal::NO_REMEASURE_PREDICTION, resolvedHeights) &&
     AbsoluteWrapContentBottom(children, resolvedHeights) > availableHeight + Internal::HEIGHT_FOR_WIDTH_FIT_EPSILON)
  {
    resolvedHeights.clear(); // Does not fit: arrange exactly as before.
  }

  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);

    if(IsStandalone(&childImpl))
    {
      continue;
    }

    float               childScale      = childImpl.GetEffectiveScale();
    auto*               params          = Internal::AbsoluteLayoutParamsImpl::Get(childImpl);
    LayoutRect          childBoundsSpec = GetChildBounds(params);
    AbsoluteLayoutFlags flags           = GetChildFlags(params);

    float x = childBoundsSpec.x;
    float y = childBoundsSpec.y;
    float w = childBoundsSpec.width;
    float h = childBoundsSpec.height;

    const uint8_t rawFlags = static_cast<uint8_t>(flags);
    const bool    xProportional =
      (rawFlags & static_cast<uint8_t>(AbsoluteLayoutFlags::X_PROPORTIONAL)) != 0;
    const bool yProportional =
      (rawFlags & static_cast<uint8_t>(AbsoluteLayoutFlags::Y_PROPORTIONAL)) != 0;
    const bool widthProportional =
      (rawFlags & static_cast<uint8_t>(AbsoluteLayoutFlags::WIDTH_PROPORTIONAL)) != 0;
    const bool heightProportional =
      (rawFlags & static_cast<uint8_t>(AbsoluteLayoutFlags::HEIGHT_PROPORTIONAL)) != 0;

    if(widthProportional)
    {
      w *= availableWidth;
    }
    else if(w > 0.0f)
    {
      w *= childScale;
    }
    if(heightProportional)
    {
      h *= availableHeight;
    }
    else if(h > 0.0f)
    {
      h *= childScale;
    }

    if(w < 0 || h < 0)
    {
      // For MATCH_PARENT axes we recompute from available space here; for
      // WRAP_CONTENT axes we reuse the natural size already recorded on the
      // child by Measure().
      MeasuredSize childMeasured = childImpl.GetMeasuredSize();
      if(w < 0)
      {
        if(childImpl.GetRequestedWidth() == MATCH_PARENT)
        {
          Insets margin = childImpl.GetMargin();
          w             = std::max(0.0f, availableWidth - (margin.start + margin.end) * childScale);
        }
        else
        {
          w = childMeasured.width;
        }
      }
      if(h < 0)
      {
        if(childImpl.GetRequestedHeight() == MATCH_PARENT)
        {
          Insets margin = childImpl.GetMargin();
          h             = std::max(0.0f, availableHeight - (margin.top + margin.bottom) * childScale);
        }
        else
        {
          h = (!resolvedHeights.empty() && resolvedHeights[i] >= 0.0f) ? resolvedHeights[i] : childMeasured.height;
        }
      }
    }

    // Proportional position: axis = (available - childExtent) * proportion
    if(xProportional)
    {
      x = (availableWidth - w) * childBoundsSpec.x;
    }
    else
    {
      x *= childScale;
    }
    if(yProportional)
    {
      y = (availableHeight - h) * childBoundsSpec.y;
    }
    else
    {
      y *= childScale;
    }

    Insets margin = childImpl.GetMargin();

    LayoutRect childBounds;
    childBounds.x      = bounds.x + x + margin.start * childScale;
    childBounds.y      = bounds.y + y + margin.top * childScale;
    childBounds.width  = w;
    childBounds.height = h;

    // Re-measure MATCH_PARENT children with their final (scaled) size.
    if(childImpl.GetRequestedWidth() == MATCH_PARENT || childImpl.GetRequestedHeight() == MATCH_PARENT)
    {
      Internal::LayoutDependency::ArrangeOwnedMeasureScope ownerScope(view);
      childImpl.Measure(w, h);
    }
    childImpl.Arrange(childBounds);
  }
}

namespace Internal
{

bool ResolveAbsoluteContentHeightForWidth(ViewImpl& view, float contentWidth, float contentHeightBudget, float& contentHeight, float predictedContentWidth)
{
  const uint32_t        count = view.GetChildViewCount();
  std::vector<Ui::View> children;
  children.reserve(count);
  for(uint32_t i = 0; i < count; ++i)
  {
    children.push_back(view.GetChildViewAt(i));
  }

  // The WRAP_CONTENT height of Measure(), with every child whose height is its own WRAP_CONTENT
  // resolved at the width the arrange gives it at exactly this content width.
  std::vector<float> resolved;
  if(!ResolveAbsoluteChildHeights(children, contentWidth, contentHeightBudget, predictedContentWidth, resolved))
  {
    return false;
  }
  contentHeight = AbsoluteWrapContentBottom(children, resolved);
  return true;
}

} // namespace Internal

} // namespace Ui
} //namespace DALI_NAMESPACE
