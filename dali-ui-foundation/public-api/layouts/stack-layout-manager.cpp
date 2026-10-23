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
#include <dali-ui-foundation/public-api/layouts/stack-layout-manager.h>

// EXTERNAL INCLUDES
#include <algorithm>
#include <limits>
#include <vector>

// INTERNAL INCLUDES
#include <dali-ui-foundation/internal/layouts/height-for-width-resolution.h>
#include <dali-ui-foundation/internal/layouts/layout-dependency-scope.h>
#include <dali-ui-foundation/internal/layouts/layout-manager-impl.h>
#include <dali-ui-foundation/internal/layouts/stack-layout-params-impl.h>
#include <dali-ui-foundation/public-api/layouts/layout-types.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>

namespace DALI_NAMESPACE
{
namespace Ui
{

namespace
{

float GetChildWeight(ViewImpl& childImpl)
{
  auto* params = Internal::StackLayoutParamsImpl::Get(childImpl);
  return params ? params->GetWeight() : 0.0f;
}

LayoutAlignment GetChildAlignment(ViewImpl& childImpl)
{
  auto* params = Internal::StackLayoutParamsImpl::Get(childImpl);
  return params ? params->GetAlignment() : LayoutAlignment::START;
}

bool IsChildStandalone(ViewImpl& childImpl)
{
  return childImpl.GetLayoutMode() == LayoutMode::STANDALONE;
}

struct StackMeasureFirstPassResult
{
  float    mainAxisNonWeight{0.0f};
  float    maxCrossAxis{0.0f};
  float    totalWeight{0.0f};
  uint32_t visibleChildCount{0};
};

StackMeasureFirstPassResult MeasureStackNonWeightChildren(std::vector<View>&         children,
                                                          std::vector<MeasuredSize>& workingSizes,
                                                          float                      contentWidth,
                                                          float                      contentHeight,
                                                          StackOrientation           orientation)
{
  StackMeasureFirstPassResult result;
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);

    if(IsChildStandalone(childImpl))
    {
      continue;
    }

    result.visibleChildCount++;
    float weight = GetChildWeight(childImpl);
    if(weight > 0.0f)
    {
      result.totalWeight += weight;
      continue;
    }
    float        childScale            = childImpl.GetEffectiveScale();
    Insets       margin                = childImpl.GetMargin();
    float        marginW               = static_cast<float>(margin.start + margin.end) * childScale;
    float        marginH               = static_cast<float>(margin.top + margin.bottom) * childScale;
    float        childWidthConstraint  = std::max(0.0f, contentWidth - marginW);
    float        childHeightConstraint = std::max(0.0f, contentHeight - marginH);
    MeasuredSize childSize             = childImpl.Measure(childWidthConstraint, childHeightConstraint);
    workingSizes[i]                    = childSize;
    if(orientation == StackOrientation::VERTICAL)
    {
      result.mainAxisNonWeight += childSize.height + marginH;
      result.maxCrossAxis = std::max(result.maxCrossAxis, childSize.width + marginW);
    }
    else
    {
      result.mainAxisNonWeight += childSize.width + marginW;
      result.maxCrossAxis = std::max(result.maxCrossAxis, childSize.height + marginH);
    }
  }
  return result;
}

void MeasureStackWeightChildren(std::vector<View>& children, std::vector<MeasuredSize>& workingSizes,
                                float contentMain, float contentWidth,
                                float contentHeight, float mainAxisNonWeight, float totalWeight,
                                uint32_t visibleChildCount, float spacing, StackOrientation orientation,
                                float& maxCrossAxisInOut)
{
  float spacingTotal  = (visibleChildCount > 1) ? spacing * (visibleChildCount - 1) : 0.0f;
  float remainingMain = contentMain - mainAxisNonWeight - spacingTotal;
  remainingMain       = std::max(0.0f, remainingMain);

  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);
    if(IsChildStandalone(childImpl))
    {
      continue;
    }
    float weight = GetChildWeight(childImpl);
    if(weight <= 0.0f)
    {
      continue;
    }

    float  share              = (weight / totalWeight) * remainingMain;
    float  childScale         = childImpl.GetEffectiveScale();
    Insets margin             = childImpl.GetMargin();
    float  marginH            = static_cast<float>(margin.top + margin.bottom) * childScale;
    float  marginW            = static_cast<float>(margin.start + margin.end) * childScale;
    float  mainAxisConstraint = std::max(0.0f, share - (orientation == StackOrientation::VERTICAL ? marginH : marginW));
    float  childWidthConstraint =
      (orientation == StackOrientation::VERTICAL) ? std::max(0.0f, contentWidth - marginW) : mainAxisConstraint;
    float childHeightConstraint =
      (orientation == StackOrientation::VERTICAL) ? mainAxisConstraint : std::max(0.0f, contentHeight - marginH);
    MeasuredSize childSize = childImpl.Measure(childWidthConstraint, childHeightConstraint);
    if(orientation == StackOrientation::VERTICAL)
    {
      workingSizes[i].width  = childSize.width;
      workingSizes[i].height = std::max(0.0f, share - marginH);
      maxCrossAxisInOut      = std::max(maxCrossAxisInOut, childSize.width + marginW);
    }
    else
    {
      workingSizes[i].width  = std::max(0.0f, share - marginW);
      workingSizes[i].height = childSize.height;
      maxCrossAxisInOut      = std::max(maxCrossAxisInOut, childSize.height + marginH);
    }
  }
}

// The width the VERTICAL placement in Arrange() gives a child: a MATCH_PARENT width takes the
// cross extent, a FILL-aligned WRAP_CONTENT width is stretched to it, every other width keeps
// the allocated one.
float StackVerticalChildWidth(ViewImpl& childImpl, float allocatedWidth, float crossAvailable)
{
  float childWidth = allocatedWidth;
  if(childImpl.GetRequestedWidth() == MATCH_PARENT)
  {
    childWidth = crossAvailable;
  }
  if(GetChildAlignment(childImpl) == LayoutAlignment::FILL && childImpl.GetRequestedWidth() == WRAP_CONTENT)
  {
    return crossAvailable;
  }
  return childWidth;
}

// The child height a VERTICAL stack aggregates while it resolves its own height: the height
// resolved at the placement width for a width-dependent WRAP_CONTENT height, the measured height
// otherwise. Sets dependsOut when the child depends on its width. With a predictedContentWidth (the
// stack is MATCH_PARENT wide, so its parent re-measures it at its slot before arranging it; the
// content width that re-measure gives the stack's measure) a WRAP_CONTENT-width child starts from
// the width that re-measure gives it -- its measured width capped by the predicted content width
// less its margin, the first measure pass's constraint form -- rather than its measured width;
// MATCH_PARENT and FILL-stretched widths still take the unclamped cross extent the arrange gives.
float StackVerticalAggregateHeight(ViewImpl& childImpl, float contentWidth, float contentHeightBudget, float marginW, float marginH, float predictedContentWidth, bool& dependsOut)
{
  const MeasuredSize measured = childImpl.GetMeasuredSize();
  if(childImpl.GetRequestedHeight() != WRAP_CONTENT)
  {
    return measured.height;
  }
  const float                   crossAvailable = std::max(0.0f, contentWidth - marginW);
  const float                   allocatedWidth = (predictedContentWidth >= 0.0f && childImpl.GetRequestedWidth() == WRAP_CONTENT)
                                                   ? Internal::PredictRemeasuredWidth(childImpl, measured.width, std::max(0.0f, predictedContentWidth - marginW))
                                                   : measured.width;
  const Internal::HeightAtWidth resolved =
    Internal::ResolveHeightAtWidth(childImpl,
                                   StackVerticalChildWidth(childImpl, allocatedWidth, crossAvailable),
                                   std::max(0.0f, contentHeightBudget - marginH));
  if(!resolved.dependsOnWidth)
  {
    return measured.height;
  }
  dependsOut = true;
  return resolved.height;
}

// Height-for-width, VERTICAL, before the weight redistribution in Arrange(): every non-weight
// WRAP_CONTENT-height child is resolved at the width the placement gives it. The resolved heights
// are adopted only when the main-axis run they produce -- the redistribution's non-weight sum plus
// the spacing -- fits the available height; the redistribution then hands the weight children what
// is left. Weight children are not resolved: their share decides their height.
void ResolveStackMainAxisHeights(std::vector<View>& children, std::vector<MeasuredSize>& allocations, float availableWidth, float availableHeight, float visSpacing)
{
  std::vector<float> resolved;
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);
    if(IsChildStandalone(childImpl) || GetChildWeight(childImpl) > 0.0f || childImpl.GetRequestedHeight() != WRAP_CONTENT)
    {
      continue;
    }

    const float                   childScale = childImpl.GetEffectiveScale();
    const Insets                  margin     = childImpl.GetMargin();
    const float                   marginW    = static_cast<float>(margin.start + margin.end) * childScale;
    const float                   marginH    = static_cast<float>(margin.top + margin.bottom) * childScale;
    const Internal::HeightAtWidth result =
      Internal::ResolveHeightAtWidth(childImpl,
                                     StackVerticalChildWidth(childImpl, allocations[i].width, std::max(0.0f, availableWidth - marginW)),
                                     std::max(0.0f, availableHeight - marginH));
    if(result.dependsOnWidth)
    {
      if(resolved.empty())
      {
        resolved.assign(children.size(), -1.0f);
      }
      resolved[i] = result.height;
    }
  }
  if(resolved.empty())
  {
    return;
  }

  // The non-weight main-axis run, in the redistribution's own form.
  float    nonWeightMain     = 0.0f;
  uint32_t visibleChildCount = 0;
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);
    if(IsChildStandalone(childImpl))
    {
      continue;
    }
    visibleChildCount++;
    if(GetChildWeight(childImpl) > 0.0f)
    {
      continue;
    }
    const float  childScale = childImpl.GetEffectiveScale();
    const Insets margin     = childImpl.GetMargin();
    const float  mainSize   = (resolved[i] >= 0.0f) ? resolved[i] : allocations[i].height;
    nonWeightMain += mainSize + static_cast<float>(margin.top + margin.bottom) * childScale;
  }
  const float spacingTotal = (visibleChildCount > 1) ? visSpacing * (visibleChildCount - 1) : 0.0f;
  if(nonWeightMain + spacingTotal > availableHeight + Internal::HEIGHT_FOR_WIDTH_FIT_EPSILON)
  {
    return; // Does not fit: arrange exactly as before.
  }

  for(uint32_t i = 0; i < children.size(); ++i)
  {
    if(resolved[i] >= 0.0f)
    {
      allocations[i].height = resolved[i];
    }
  }
}

// Height-for-width, HORIZONTAL, after the weight redistribution in Arrange() (a weight child's
// width is its share by then): every WRAP_CONTENT-height child is resolved at the width the
// placement gives it. The resolved heights are adopted only when every resulting cross extent fits
// the available height. A START/CENTER/END-aligned child is then placed at its resolved height; a
// FILL-aligned WRAP_CONTENT child keeps the whole cross extent as its slot.
void ResolveStackCrossAxisHeights(std::vector<View>& children, std::vector<MeasuredSize>& allocations, float availableWidth, float availableHeight)
{
  std::vector<float> resolved;
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);
    if(IsChildStandalone(childImpl) || childImpl.GetRequestedHeight() != WRAP_CONTENT)
    {
      continue;
    }

    const float  childScale = childImpl.GetEffectiveScale();
    const Insets margin     = childImpl.GetMargin();
    const float  marginW    = static_cast<float>(margin.start + margin.end) * childScale;
    const float  marginH    = static_cast<float>(margin.top + margin.bottom) * childScale;
    float        width      = allocations[i].width;
    if(GetChildWeight(childImpl) <= 0.0f && childImpl.GetRequestedWidth() == MATCH_PARENT)
    {
      width = std::max(0.0f, availableWidth - marginW);
    }
    const Internal::HeightAtWidth result = Internal::ResolveHeightAtWidth(childImpl, width, std::max(0.0f, availableHeight - marginH));
    if(result.dependsOnWidth)
    {
      if(resolved.empty())
      {
        resolved.assign(children.size(), -1.0f);
      }
      resolved[i] = result.height;
    }
  }
  if(resolved.empty())
  {
    return;
  }

  float needed = 0.0f;
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);
    if(IsChildStandalone(childImpl))
    {
      continue;
    }
    const float  childScale = childImpl.GetEffectiveScale();
    const Insets margin     = childImpl.GetMargin();
    const float  height     = (resolved[i] >= 0.0f) ? resolved[i] : allocations[i].height;
    needed                  = std::max(needed, height + static_cast<float>(margin.top + margin.bottom) * childScale);
  }
  if(needed > availableHeight + Internal::HEIGHT_FOR_WIDTH_FIT_EPSILON)
  {
    return; // Does not fit: arrange exactly as before.
  }

  for(uint32_t i = 0; i < children.size(); ++i)
  {
    if(resolved[i] >= 0.0f)
    {
      allocations[i].height = resolved[i];
    }
  }
}

} // namespace

class StackLayoutManager::Impl : public LayoutManager::Impl
{
public:
  Impl(StackOrientation orientation, float spacing)
  : mOrientation(orientation),
    mSpacing(spacing)
  {
  }

  StackOrientation mOrientation;
  float            mSpacing;
};

StackLayoutManager::StackLayoutManager(StackOrientation orientation, float spacing)
: LayoutManager(new Impl(orientation, spacing))
{
}

StackLayoutManager::~StackLayoutManager()
{
}

void StackLayoutManager::SetOrientation(StackOrientation orientation)
{
  Impl* impl = GetImplAs<Impl>();
  if(impl->mOrientation == orientation)
  {
    return;
  }
  impl->mOrientation = orientation;

  // This state is read by both Measure() and Arrange(), and neither the measure cache
  // key nor the arrange cache key can see it, so the owner has to be told. Reaching
  // this manager directly (through the protected View::GetLayoutManager()) used to
  // leave the change unscheduled; now it invalidates exactly as StackLayout's own
  // setter does. This also keeps ArrangePolicy::IF_CHANGED valid: the orientation is
  // layout-tracked precisely because of this call.
  InvalidateOwnerMeasure();
}

StackOrientation StackLayoutManager::GetOrientation() const
{
  return GetImplAs<Impl>()->mOrientation;
}

void StackLayoutManager::SetSpacing(float spacing)
{
  Impl* impl = GetImplAs<Impl>();
  if(impl->mSpacing == spacing)
  {
    return;
  }
  impl->mSpacing = spacing;

  // Spacing changes the accumulated main-axis extent, so it is a MEASURE input.
  // See SetOrientation for why the owner has to be invalidated from here.
  InvalidateOwnerMeasure();
}

float StackLayoutManager::GetSpacing() const
{
  return GetImplAs<Impl>()->mSpacing;
}

MeasuredSize StackLayoutManager::Measure(ViewImpl* view, float widthConstraint, float heightConstraint)
{
  if(!view)
  {
    return MeasuredSize(0.0f, 0.0f);
  }

  auto* impl = GetImplAs<Impl>();

  float s           = view->GetEffectiveScale();
  float visSpacing  = impl->mSpacing * s;
  float contentMain = (impl->mOrientation == StackOrientation::VERTICAL) ? heightConstraint : widthConstraint;

  // Collect children once and pass to helpers (helpers run twice).
  const uint32_t    count = GetChildViewCount(view);
  std::vector<View> children;
  children.reserve(count);
  for(uint32_t i = 0; i < count; ++i)
  {
    children.push_back(GetChildViewAt(view, i));
  }

  // Local working buffer (seed later from child measurements): weight
  // distribution writes allocation values here without persisting on the
  // child, so repeated layout passes do not accumulate.
  std::vector<MeasuredSize> workingSizes(children.size());

  StackMeasureFirstPassResult first = MeasureStackNonWeightChildren(
    children, workingSizes, widthConstraint, heightConstraint, impl->mOrientation);

  float maxCrossAxis = first.maxCrossAxis;
  float mainAxisTotal;

  if(first.totalWeight > 0.0f && first.visibleChildCount > 0)
  {
    float requestedMain  = (impl->mOrientation == StackOrientation::VERTICAL) ? view->GetRequestedHeight() : view->GetRequestedWidth();
    bool  isMainAxisWrap = (requestedMain != MATCH_PARENT && requestedMain < 0.0f);

    float spacingTotal = (first.visibleChildCount > 1) ? visSpacing * (first.visibleChildCount - 1) : 0.0f;
    float wrappedMain  = first.mainAxisNonWeight + spacingTotal;

    // Determine the target main-axis size for weight distribution.
    float targetMain;
    if(isMainAxisWrap)
    {
      Insets padding        = view->GetPadding();
      float  paddingMain    = (impl->mOrientation == StackOrientation::VERTICAL)
                                ? static_cast<float>(padding.top + padding.bottom) * s
                                : static_cast<float>(padding.start + padding.end) * s;
      float  minMain        = (impl->mOrientation == StackOrientation::VERTICAL) ? view->GetMinimumHeight() : view->GetMinimumWidth();
      float  minMainContent = std::max(0.0f, minMain * s - paddingMain);
      targetMain            = std::max(wrappedMain, minMainContent);
    }
    else
    {
      targetMain = contentMain;
    }

    if(targetMain > wrappedMain)
    {
      MeasureStackWeightChildren(children, workingSizes, targetMain, widthConstraint, heightConstraint,
                                 first.mainAxisNonWeight, first.totalWeight, first.visibleChildCount, visSpacing,
                                 impl->mOrientation, maxCrossAxis);
      mainAxisTotal = targetMain;
    }
    else
    {
      for(uint32_t i = 0; i < children.size(); ++i)
      {
        ViewImpl& childImpl = GetImpl(children[i]);
        if(IsChildStandalone(childImpl))
        {
          continue;
        }
        float weight = GetChildWeight(childImpl);
        if(weight <= 0.0f)
        {
          continue;
        }

        float        childScale            = childImpl.GetEffectiveScale();
        Insets       margin                = childImpl.GetMargin();
        float        marginW               = static_cast<float>(margin.start + margin.end) * childScale;
        float        marginH               = static_cast<float>(margin.top + margin.bottom) * childScale;
        float        childWidthConstraint  = std::max(0.0f, widthConstraint - marginW);
        float        childHeightConstraint = std::max(0.0f, heightConstraint - marginH);
        MeasuredSize childSize             = childImpl.Measure(childWidthConstraint, childHeightConstraint);
        workingSizes[i]                    = childSize;

        if(impl->mOrientation == StackOrientation::VERTICAL)
        {
          first.mainAxisNonWeight += childSize.height + marginH;
          maxCrossAxis = std::max(maxCrossAxis, childSize.width + marginW);
        }
        else
        {
          first.mainAxisNonWeight += childSize.width + marginW;
          maxCrossAxis = std::max(maxCrossAxis, childSize.height + marginH);
        }
      }
      mainAxisTotal = first.mainAxisNonWeight + spacingTotal;
    }
  }
  else
  {
    mainAxisTotal = first.mainAxisNonWeight;
    if(first.visibleChildCount > 1)
    {
      mainAxisTotal += visSpacing * (first.visibleChildCount - 1);
    }
  }

  MeasuredSize totalSize(0.0f, 0.0f);
  if(impl->mOrientation == StackOrientation::VERTICAL)
  {
    totalSize.width  = maxCrossAxis;
    totalSize.height = mainAxisTotal;
  }
  else
  {
    totalSize.width  = mainAxisTotal;
    totalSize.height = maxCrossAxis;
  }
  return totalSize;
}

void StackLayoutManager::Arrange(ViewImpl* view, const LayoutRect& bounds)
{
  if(!view)
  {
    return;
  }

  auto* impl = GetImplAs<Impl>();

  float s          = view->GetEffectiveScale();
  float visSpacing = impl->mSpacing * s;

  const uint32_t    count = GetChildViewCount(view);
  std::vector<View> children;
  children.reserve(count);
  for(uint32_t i = 0; i < count; ++i)
  {
    children.push_back(GetChildViewAt(view, i));
  }

  float availableWidth  = bounds.width;
  float availableHeight = bounds.height;
  float currentX        = bounds.x;
  float currentY        = bounds.y;

  // Local allocation buffer: seeded from each child's current measured size.
  std::vector<MeasuredSize> allocations(children.size());
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    allocations[i] = GetImpl(children[i]).GetMeasuredSize();
  }

  // Height-for-width: a child whose WRAP_CONTENT height follows its width is given the height it
  // needs at the width the placement below gives it (see height-for-width-resolution.h). Only an
  // exact StackLayoutManager on a view without layout callbacks does this; when no child depends on
  // its width the allocations stay exactly as measured.
  const bool resolveHeights = Internal::IsHeightForWidthContainer(*view);
  if(resolveHeights && impl->mOrientation == StackOrientation::VERTICAL)
  {
    ResolveStackMainAxisHeights(children, allocations, availableWidth, availableHeight, visSpacing);
  }

  // Re-distribute weight among weight children using actual arrange bounds.
  {
    float    totalWeight       = 0.0f;
    float    nonWeightMain     = 0.0f;
    uint32_t visibleChildCount = 0;

    for(uint32_t i = 0; i < children.size(); ++i)
    {
      ViewImpl& childImpl = GetImpl(children[i]);
      if(IsChildStandalone(childImpl))
      {
        continue;
      }
      visibleChildCount++;
      float weight = GetChildWeight(childImpl);
      if(weight > 0.0f)
      {
        totalWeight += weight;
      }
      else
      {
        float  childScale = childImpl.GetEffectiveScale();
        Insets margin     = childImpl.GetMargin();
        float  marginM    = (impl->mOrientation == StackOrientation::VERTICAL) ? static_cast<float>(margin.top + margin.bottom) * childScale
                                                                               : static_cast<float>(margin.start + margin.end) * childScale;
        float  mainSize   = (impl->mOrientation == StackOrientation::VERTICAL) ? allocations[i].height : allocations[i].width;
        nonWeightMain += mainSize + marginM;
      }
    }

    if(totalWeight > 0.0f)
    {
      float availableMain = (impl->mOrientation == StackOrientation::VERTICAL) ? availableHeight : availableWidth;
      float spacingTotal  = (visibleChildCount > 1) ? visSpacing * (visibleChildCount - 1) : 0.0f;
      float remainingMain = std::max(0.0f, availableMain - nonWeightMain - spacingTotal);

      for(uint32_t i = 0; i < children.size(); ++i)
      {
        ViewImpl& childImpl = GetImpl(children[i]);
        if(IsChildStandalone(childImpl))
        {
          continue;
        }
        float weight = GetChildWeight(childImpl);
        if(weight <= 0.0f)
        {
          continue;
        }

        float  share      = (weight / totalWeight) * remainingMain;
        float  childScale = childImpl.GetEffectiveScale();
        Insets margin     = childImpl.GetMargin();
        float  marginW    = static_cast<float>(margin.start + margin.end) * childScale;
        float  marginH    = static_cast<float>(margin.top + margin.bottom) * childScale;

        float childWidthConstraint;
        float childHeightConstraint;
        if(impl->mOrientation == StackOrientation::VERTICAL)
        {
          childWidthConstraint  = std::max(0.0f, availableWidth - marginW);
          childHeightConstraint = std::max(0.0f, share - marginH);
        }
        else
        {
          childWidthConstraint  = std::max(0.0f, share - marginW);
          childHeightConstraint = std::max(0.0f, availableHeight - marginH);
        }

        MeasuredSize childSize;
        {
          Internal::LayoutDependency::ArrangeOwnedMeasureScope ownerScope(view);
          childSize = childImpl.Measure(childWidthConstraint, childHeightConstraint);
        }

        if(impl->mOrientation == StackOrientation::VERTICAL)
        {
          allocations[i].width  = childSize.width;
          allocations[i].height = std::max(0.0f, share - marginH);
        }
        else
        {
          allocations[i].width  = std::max(0.0f, share - marginW);
          allocations[i].height = childSize.height;
        }
      }
    }
  }

  // Height-for-width on the cross axis, now that every width -- weight shares included -- is final.
  if(resolveHeights && impl->mOrientation == StackOrientation::HORIZONTAL)
  {
    ResolveStackCrossAxisHeights(children, allocations, availableWidth, availableHeight);
  }

  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);

    if(IsChildStandalone(childImpl))
    {
      continue;
    }

    float      childScale = childImpl.GetEffectiveScale();
    Insets     margin     = childImpl.GetMargin();
    float      marginW    = static_cast<float>(margin.start + margin.end) * childScale;
    float      marginH    = static_cast<float>(margin.top + margin.bottom) * childScale;
    LayoutRect childBounds;

    if(impl->mOrientation == StackOrientation::VERTICAL)
    {
      const float crossAvailable = std::max(0.0f, availableWidth - marginW);
      float       childHeight    = allocations[i].height;
      if(childImpl.GetRequestedHeight() == MATCH_PARENT && GetChildWeight(childImpl) <= 0.0f)
      {
        childHeight = std::max(0.0f, availableHeight - marginH);
      }
      const float slotHeight = childHeight + marginH;
      float       childWidth = allocations[i].width;
      if(childImpl.GetRequestedWidth() == MATCH_PARENT)
      {
        childWidth = crossAvailable;
      }

      LayoutAlignment crossAlign = GetChildAlignment(childImpl);
      float           crossX     = currentX + static_cast<float>(margin.start) * childScale;
      float           finalWidth = childWidth;
      switch(crossAlign)
      {
        case LayoutAlignment::FILL:
          if(childImpl.GetRequestedWidth() == WRAP_CONTENT)
          {
            finalWidth = crossAvailable;
          }
          break;
        case LayoutAlignment::CENTER:
          crossX += (crossAvailable - childWidth) * 0.5f;
          break;
        case LayoutAlignment::END:
          crossX += crossAvailable - childWidth;
          break;
        case LayoutAlignment::START:
        default:
          break;
      }

      childBounds.width  = finalWidth;
      childBounds.height = childHeight;
      childBounds.x      = crossX;
      childBounds.y      = currentY + static_cast<float>(margin.top) * childScale;

      if(childImpl.GetRequestedWidth() == MATCH_PARENT || childImpl.GetRequestedHeight() == MATCH_PARENT)
      {
        Internal::LayoutDependency::ArrangeOwnedMeasureScope ownerScope(view);
        childImpl.Measure(childBounds.width, childBounds.height);
      }
      childImpl.Arrange(childBounds);

      currentY += slotHeight + visSpacing;
    }
    else
    {
      const float crossAvailable = std::max(0.0f, availableHeight - marginH);
      float       childWidth     = allocations[i].width;
      if(childImpl.GetRequestedWidth() == MATCH_PARENT && GetChildWeight(childImpl) <= 0.0f)
      {
        childWidth = std::max(0.0f, availableWidth - marginW);
      }
      const float slotWidth   = childWidth + marginW;
      float       childHeight = allocations[i].height;
      if(childImpl.GetRequestedHeight() == MATCH_PARENT)
      {
        childHeight = crossAvailable;
      }

      LayoutAlignment crossAlign  = GetChildAlignment(childImpl);
      float           crossY      = currentY + static_cast<float>(margin.top) * childScale;
      float           finalHeight = childHeight;
      switch(crossAlign)
      {
        case LayoutAlignment::FILL:
          if(childImpl.GetRequestedHeight() == WRAP_CONTENT)
          {
            finalHeight = crossAvailable;
          }
          break;
        case LayoutAlignment::CENTER:
          crossY += (crossAvailable - childHeight) * 0.5f;
          break;
        case LayoutAlignment::END:
          crossY += crossAvailable - childHeight;
          break;
        case LayoutAlignment::START:
        default:
          break;
      }

      childBounds.width  = childWidth;
      childBounds.height = finalHeight;
      childBounds.x      = currentX + static_cast<float>(margin.start) * childScale;
      childBounds.y      = crossY;

      if(childImpl.GetRequestedWidth() == MATCH_PARENT || childImpl.GetRequestedHeight() == MATCH_PARENT)
      {
        Internal::LayoutDependency::ArrangeOwnedMeasureScope ownerScope(view);
        childImpl.Measure(childBounds.width, childBounds.height);
      }
      childImpl.Arrange(childBounds);

      currentX += slotWidth + visSpacing;
    }
  }
}

namespace Internal
{

bool ResolveStackContentHeightForWidth(const StackLayoutManager& manager, ViewImpl& view, float contentWidth, float contentHeightBudget, float& contentHeight, float predictedContentWidth)
{
  const float    s          = view.GetEffectiveScale();
  const float    visSpacing = manager.GetSpacing() * s;
  const uint32_t count      = view.GetChildViewCount();

  std::vector<Ui::View> children;
  children.reserve(count);
  for(uint32_t i = 0; i < count; ++i)
  {
    children.push_back(view.GetChildViewAt(i));
  }

  bool  depends = false;
  float height  = 0.0f;
  if(manager.GetOrientation() == StackOrientation::VERTICAL)
  {
    // The WRAP_CONTENT main-axis extent of Measure(), with every width-dependent child resolved at
    // the width the vertical placement gives it. First pass: the non-weight children.
    float    mainAxisNonWeight = 0.0f;
    float    totalWeight       = 0.0f;
    uint32_t visibleChildCount = 0;
    for(uint32_t i = 0; i < children.size(); ++i)
    {
      ViewImpl& childImpl = GetImpl(children[i]);
      if(IsChildStandalone(childImpl))
      {
        continue;
      }
      visibleChildCount++;
      const float weight = GetChildWeight(childImpl);
      if(weight > 0.0f)
      {
        totalWeight += weight;
        continue;
      }
      const float  childScale = childImpl.GetEffectiveScale();
      const Insets margin     = childImpl.GetMargin();
      const float  marginW    = static_cast<float>(margin.start + margin.end) * childScale;
      const float  marginH    = static_cast<float>(margin.top + margin.bottom) * childScale;
      mainAxisNonWeight += StackVerticalAggregateHeight(childImpl, contentWidth, contentHeightBudget, marginW, marginH, predictedContentWidth, depends) + marginH;
    }

    if(totalWeight > 0.0f && visibleChildCount > 0)
    {
      // The weight branch of Measure() for a WRAP_CONTENT main axis: a minimum height larger than
      // the wrapped run is shared by the weight children; otherwise they are aggregated like the
      // non-weight children.
      const float  spacingTotal   = (visibleChildCount > 1) ? visSpacing * (visibleChildCount - 1) : 0.0f;
      const float  wrappedMain    = mainAxisNonWeight + spacingTotal;
      const Insets padding        = view.GetPadding();
      const float  paddingMain    = static_cast<float>(padding.top + padding.bottom) * s;
      const float  minMainContent = std::max(0.0f, view.GetMinimumHeight() * s - paddingMain);
      const float  targetMain     = std::max(wrappedMain, minMainContent);
      if(targetMain > wrappedMain)
      {
        height = targetMain;
      }
      else
      {
        for(uint32_t i = 0; i < children.size(); ++i)
        {
          ViewImpl& childImpl = GetImpl(children[i]);
          if(IsChildStandalone(childImpl) || GetChildWeight(childImpl) <= 0.0f)
          {
            continue;
          }
          const float  childScale = childImpl.GetEffectiveScale();
          const Insets margin     = childImpl.GetMargin();
          const float  marginW    = static_cast<float>(margin.start + margin.end) * childScale;
          const float  marginH    = static_cast<float>(margin.top + margin.bottom) * childScale;
          mainAxisNonWeight += StackVerticalAggregateHeight(childImpl, contentWidth, contentHeightBudget, marginW, marginH, predictedContentWidth, depends) + marginH;
        }
        height = mainAxisNonWeight + spacingTotal;
      }
    }
    else
    {
      height = mainAxisNonWeight;
      if(visibleChildCount > 1)
      {
        height += visSpacing * (visibleChildCount - 1);
      }
    }
  }
  else
  {
    // The cross-axis extent of Measure(), with every child at the width the horizontal placement
    // gives it: a weight child its share of what the non-weight children leave (the arrange-time
    // redistribution), a non-weight MATCH_PARENT child the whole content width, every other child
    // its measured width -- for a WRAP_CONTENT width with a predictedContentWidth, the width the
    // stack's re-measure gives it (its measured width capped by the predicted content width less its
    // margin, the constraint the first measure pass hands it), in the non-weight run as well. The
    // weight shares and MATCH_PARENT widths keep the unclamped content width the arrange gives.
    auto nonWeightWidth = [predictedContentWidth](ViewImpl& childImpl, float measuredWidth, float marginW) -> float
    {
      return (predictedContentWidth >= 0.0f && childImpl.GetRequestedWidth() == WRAP_CONTENT)
               ? PredictRemeasuredWidth(childImpl, measuredWidth, std::max(0.0f, predictedContentWidth - marginW))
               : measuredWidth;
    };

    float    totalWeight       = 0.0f;
    float    nonWeightMain     = 0.0f;
    uint32_t visibleChildCount = 0;
    for(uint32_t i = 0; i < children.size(); ++i)
    {
      ViewImpl& childImpl = GetImpl(children[i]);
      if(IsChildStandalone(childImpl))
      {
        continue;
      }
      visibleChildCount++;
      const float weight = GetChildWeight(childImpl);
      if(weight > 0.0f)
      {
        totalWeight += weight;
      }
      else
      {
        const float  childScale = childImpl.GetEffectiveScale();
        const Insets margin     = childImpl.GetMargin();
        const float  marginW    = static_cast<float>(margin.start + margin.end) * childScale;
        nonWeightMain += nonWeightWidth(childImpl, childImpl.GetMeasuredSize().width, marginW) + marginW;
      }
    }
    const float spacingTotal  = (visibleChildCount > 1) ? visSpacing * (visibleChildCount - 1) : 0.0f;
    const float remainingMain = std::max(0.0f, contentWidth - nonWeightMain - spacingTotal);

    float maxCrossAxis = 0.0f;
    for(uint32_t i = 0; i < children.size(); ++i)
    {
      ViewImpl& childImpl = GetImpl(children[i]);
      if(IsChildStandalone(childImpl))
      {
        continue;
      }
      const float        childScale = childImpl.GetEffectiveScale();
      const Insets       margin     = childImpl.GetMargin();
      const float        marginW    = static_cast<float>(margin.start + margin.end) * childScale;
      const float        marginH    = static_cast<float>(margin.top + margin.bottom) * childScale;
      const MeasuredSize measured   = childImpl.GetMeasuredSize();
      const float        weight     = GetChildWeight(childImpl);

      float width = measured.width;
      if(weight > 0.0f)
      {
        width = std::max(0.0f, (weight / totalWeight) * remainingMain - marginW);
      }
      else if(childImpl.GetRequestedWidth() == MATCH_PARENT)
      {
        width = std::max(0.0f, contentWidth - marginW);
      }
      else
      {
        width = nonWeightWidth(childImpl, measured.width, marginW);
      }

      float childHeight = measured.height;
      if(childImpl.GetRequestedHeight() == WRAP_CONTENT)
      {
        const HeightAtWidth resolved = ResolveHeightAtWidth(childImpl, width, std::max(0.0f, contentHeightBudget - marginH));
        if(resolved.dependsOnWidth)
        {
          childHeight = resolved.height;
          depends     = true;
        }
      }
      maxCrossAxis = std::max(maxCrossAxis, childHeight + marginH);
    }
    height = maxCrossAxis;
  }

  if(depends)
  {
    contentHeight = height;
  }
  return depends;
}

} // namespace Internal

} // namespace Ui
} //namespace DALI_NAMESPACE
