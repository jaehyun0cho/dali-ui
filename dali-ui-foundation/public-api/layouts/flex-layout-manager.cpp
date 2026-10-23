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
#include <dali-ui-foundation/public-api/layouts/flex-layout-manager.h>

// EXTERNAL INCLUDES
#include <algorithm>
#include <vector>

// INTERNAL INCLUDES
#include <dali-ui-foundation/internal/layouts/flex-layout-params-impl.h>
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

float GetFlexGrow(ViewImpl& childImpl)
{
  auto* params = Internal::FlexLayoutParamsImpl::Get(childImpl);
  return params ? params->GetFlexGrow() : 0.0f;
}

float GetFlexShrink(ViewImpl& childImpl)
{
  auto* params = Internal::FlexLayoutParamsImpl::Get(childImpl);
  return params ? params->GetFlexShrink() : 1.0f;
}

FlexAlign GetAlignSelf(ViewImpl& childImpl)
{
  auto* params = Internal::FlexLayoutParamsImpl::Get(childImpl);
  return params ? params->GetAlignSelf() : FlexAlign::AUTO;
}

float GetFlexBasis(ViewImpl& childImpl)
{
  auto* params = Internal::FlexLayoutParamsImpl::Get(childImpl);
  return params ? params->GetFlexBasis() : WRAP_CONTENT;
}

bool IsChildStandalone(ViewImpl& childImpl)
{
  return childImpl.GetLayoutMode() == LayoutMode::STANDALONE;
}

struct FlexLine
{
  std::vector<uint32_t> childIndices;
  float                 mainSize{0.0f};
  float                 crossSize{0.0f};
  float                 totalFlexGrow{0.0f};
  float                 totalFlexShrink{0.0f};
};

std::vector<FlexLine> BuildFlexLinesForArrange(std::vector<View>&         children,
                                               std::vector<MeasuredSize>& workingSizes,
                                               float                      availableMain,
                                               bool                       isMainAxisHorizontal,
                                               FlexWrap                   wrap)
{
  std::vector<FlexLine> lines;
  FlexLine              currentLine;
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);
    if(IsChildStandalone(childImpl))
    {
      continue;
    }
    float  childScale = childImpl.GetEffectiveScale();
    Insets margin     = childImpl.GetMargin();

    float basis = GetFlexBasis(childImpl);
    if(basis >= 0.0f)
    {
      if(isMainAxisHorizontal)
      {
        workingSizes[i].width = basis * childScale;
      }
      else
      {
        workingSizes[i].height = basis * childScale;
      }
    }

    float visMarginMainH = (margin.top + margin.bottom) * childScale;
    float visMarginMainW = (margin.start + margin.end) * childScale;
    float childMainSize  = isMainAxisHorizontal ? workingSizes[i].width + visMarginMainW
                                                : workingSizes[i].height + visMarginMainH;
    bool  shouldWrap     = (wrap != FlexWrap::NO_WRAP) && !currentLine.childIndices.empty() &&
                      (currentLine.mainSize + childMainSize > availableMain);
    if(shouldWrap)
    {
      lines.push_back(currentLine);
      currentLine = FlexLine();
    }
    currentLine.childIndices.push_back(i);
    currentLine.mainSize += childMainSize;
    currentLine.totalFlexGrow += GetFlexGrow(childImpl);
    currentLine.totalFlexShrink += GetFlexShrink(childImpl);
    float childCrossSize  = isMainAxisHorizontal ? workingSizes[i].height + visMarginMainH
                                                 : workingSizes[i].width + visMarginMainW;
    currentLine.crossSize = std::max(currentLine.crossSize, childCrossSize);
  }
  if(!currentLine.childIndices.empty())
  {
    lines.push_back(currentLine);
  }
  return lines;
}

void ApplyFlexGrowShrink(FlexLine& line, std::vector<View>& children,
                         std::vector<MeasuredSize>& workingSizes, float availableMain,
                         bool isMainAxisHorizontal)
{
  float freeSpace = availableMain - line.mainSize;

  if(freeSpace > 0.0f && line.totalFlexGrow > 0.0f)
  {
    for(uint32_t idx : line.childIndices)
    {
      ViewImpl& childImpl = GetImpl(children[idx]);
      float     grow      = GetFlexGrow(childImpl);
      if(grow > 0.0f)
      {
        float extra = (grow / line.totalFlexGrow) * freeSpace;
        if(isMainAxisHorizontal)
        {
          workingSizes[idx].width += extra;
        }
        else
        {
          workingSizes[idx].height += extra;
        }
      }
    }
    line.mainSize = availableMain;
  }
  else if(freeSpace < 0.0f && line.totalFlexShrink > 0.0f)
  {
    float totalWeightedShrink = 0.0f;
    for(uint32_t idx : line.childIndices)
    {
      ViewImpl& childImpl     = GetImpl(children[idx]);
      float     childMainSize = isMainAxisHorizontal ? workingSizes[idx].width : workingSizes[idx].height;
      float     shrink        = GetFlexShrink(childImpl);
      totalWeightedShrink += shrink * childMainSize;
    }
    if(totalWeightedShrink > 0.0f)
    {
      float overflow = -freeSpace;
      for(uint32_t idx : line.childIndices)
      {
        ViewImpl& childImpl     = GetImpl(children[idx]);
        float     childMainSize = isMainAxisHorizontal ? workingSizes[idx].width : workingSizes[idx].height;
        float     shrink        = GetFlexShrink(childImpl);
        float     reduction     = (shrink * childMainSize / totalWeightedShrink) * overflow;
        if(isMainAxisHorizontal)
        {
          workingSizes[idx].width = std::max(0.0f, workingSizes[idx].width - reduction);
        }
        else
        {
          workingSizes[idx].height = std::max(0.0f, workingSizes[idx].height - reduction);
        }
      }
      line.mainSize = availableMain;
    }
  }
}

struct FlexJustifyOffsets
{
  float mainOffset{0.0f};
  float spacing{0.0f};
};

FlexJustifyOffsets GetFlexJustifyOffsets(float freeSpace, FlexJustify justify, size_t lineChildCount)
{
  FlexJustifyOffsets out;
  switch(justify)
  {
    case FlexJustify::FLEX_START:
      break;
    case FlexJustify::FLEX_END:
      out.mainOffset = freeSpace;
      break;
    case FlexJustify::CENTER:
      out.mainOffset = freeSpace / 2.0f;
      break;
    // On overflow (freeSpace < 0) the distributed modes must not produce
    // negative gaps that overlap children: fall back to start packing
    // (SPACE_BETWEEN) or centring the overflow (SPACE_AROUND/EVENLY).
    case FlexJustify::SPACE_BETWEEN:
      if(freeSpace > 0.0f && lineChildCount > 1)
      {
        out.spacing = freeSpace / (lineChildCount - 1);
      }
      break;
    case FlexJustify::SPACE_AROUND:
      if(freeSpace > 0.0f)
      {
        out.spacing    = freeSpace / lineChildCount;
        out.mainOffset = out.spacing / 2.0f;
      }
      else
      {
        out.mainOffset = freeSpace / 2.0f;
      }
      break;
    case FlexJustify::SPACE_EVENLY:
      if(freeSpace > 0.0f)
      {
        out.spacing    = freeSpace / (lineChildCount + 1);
        out.mainOffset = out.spacing;
      }
      else
      {
        out.mainOffset = freeSpace / 2.0f;
      }
      break;
  }
  return out;
}

// @param[in] owner The arranging View, threaded through so the final MATCH_PARENT
//                  re-measure below can be attributed to it. nullptr makes the scope inert.
void ArrangeOneFlexLine(ViewImpl* owner, FlexLine& line, std::vector<View>& children,
                        std::vector<MeasuredSize>& workingSizes, const LayoutRect& bounds,
                        float contentWidth, float contentHeight, float& crossOffsetInOut, float& mainOffsetInOut,
                        float spacing, FlexAlign alignItems, bool isMainAxisHorizontal, bool isMainAxisReversed)
{
  for(uint32_t idx : line.childIndices)
  {
    ViewImpl& childImpl  = GetImpl(children[idx]);
    float     childScale = childImpl.GetEffectiveScale();
    Insets    margin     = childImpl.GetMargin();

    float childMainSize  = isMainAxisHorizontal ? workingSizes[idx].width : workingSizes[idx].height;
    float childCrossSize = isMainAxisHorizontal ? workingSizes[idx].height : workingSizes[idx].width;
    float marginMain     = isMainAxisHorizontal ? (margin.start + margin.end) * childScale
                                                : (margin.top + margin.bottom) * childScale;
    float marginCross    = isMainAxisHorizontal ? (margin.top + margin.bottom) * childScale
                                                : (margin.start + margin.end) * childScale;

    bool crossIsMatchParent = isMainAxisHorizontal ? (childImpl.GetRequestedHeight() == MATCH_PARENT)
                                                   : (childImpl.GetRequestedWidth() == MATCH_PARENT);
    if(crossIsMatchParent)
    {
      childCrossSize = line.crossSize - marginCross;
    }

    bool mainIsMatchParent = isMainAxisHorizontal ? (childImpl.GetRequestedWidth() == MATCH_PARENT)
                                                  : (childImpl.GetRequestedHeight() == MATCH_PARENT);
    // A main-axis MATCH_PARENT child only fills the whole line when it does not
    // participate in flex-grow distribution. When flex-grow is set, the grow
    // distribution already produced the child's share in workingSizes; trusting
    // that value keeps siblings from being overlapped by a full-line overwrite.
    if(mainIsMatchParent && GetFlexGrow(childImpl) == 0.0f)
    {
      float availMain = isMainAxisHorizontal ? contentWidth : contentHeight;
      childMainSize   = std::max(0.0f, availMain - marginMain);
    }

    FlexAlign effectiveAlign = GetAlignSelf(childImpl);
    if(effectiveAlign == FlexAlign::AUTO)
    {
      effectiveAlign = alignItems;
    }

    float childCrossOffset = crossOffsetInOut;
    float crossSpace       = line.crossSize - childCrossSize - marginCross;
    switch(effectiveAlign)
    {
      case FlexAlign::FLEX_START:
      case FlexAlign::AUTO:
        break;
      case FlexAlign::FLEX_END:
        childCrossOffset += crossSpace;
        break;
      case FlexAlign::CENTER:
        childCrossOffset += crossSpace / 2.0f;
        break;
      case FlexAlign::STRETCH:
        childCrossSize = line.crossSize - marginCross;
        break;
      case FlexAlign::BASELINE:
        break;
    }

    float      allocMain  = childMainSize + marginMain;
    float      allocCross = childCrossSize + marginCross;
    LayoutRect childBounds;
    if(isMainAxisHorizontal)
    {
      if(isMainAxisReversed)
      {
        mainOffsetInOut -= allocMain;
        childBounds.x = bounds.x + mainOffsetInOut;
        mainOffsetInOut -= spacing;
      }
      else
      {
        childBounds.x = bounds.x + mainOffsetInOut;
        mainOffsetInOut += allocMain + spacing;
      }
      childBounds.y      = bounds.y + childCrossOffset;
      childBounds.width  = allocMain;
      childBounds.height = allocCross;
    }
    else
    {
      if(isMainAxisReversed)
      {
        mainOffsetInOut -= allocMain;
        childBounds.y = bounds.y + mainOffsetInOut;
        mainOffsetInOut -= spacing;
      }
      else
      {
        childBounds.y = bounds.y + mainOffsetInOut;
        mainOffsetInOut += allocMain + spacing;
      }
      childBounds.x      = bounds.x + childCrossOffset;
      childBounds.width  = allocCross;
      childBounds.height = allocMain;
    }
    childBounds.x += margin.start * childScale;
    childBounds.y += margin.top * childScale;
    childBounds.width  = std::max(0.0f, childBounds.width - (margin.start + margin.end) * childScale);
    childBounds.height = std::max(0.0f, childBounds.height - (margin.top + margin.bottom) * childScale);

    if(childImpl.GetRequestedWidth() == MATCH_PARENT || childImpl.GetRequestedHeight() == MATCH_PARENT)
    {
      Internal::LayoutDependency::ArrangeOwnedMeasureScope ownerScope(owner);
      childImpl.Measure(childBounds.width, childBounds.height);
    }
    childImpl.Arrange(childBounds);
  }
  crossOffsetInOut += line.crossSize;
}

// ROW (main axis horizontal): the width ArrangeOneFlexLine gives a child -- its grown/shrunk main
// size, or the whole content width for a MATCH_PARENT width without flex-grow -- with the margin
// added and subtracted in the same float order as the placement.
float FlexRowChildWidth(ViewImpl& childImpl, float workingWidth, float contentWidth, float marginMain)
{
  float childMainSize = workingWidth;
  if(childImpl.GetRequestedWidth() == MATCH_PARENT && GetFlexGrow(childImpl) == 0.0f)
  {
    childMainSize = std::max(0.0f, contentWidth - marginMain);
  }
  return std::max(0.0f, (childMainSize + marginMain) - marginMain);
}

// COLUMN (main axis vertical): the width ArrangeOneFlexLine gives a child for a line cross extent --
// the line cross for a MATCH_PARENT width or a STRETCH alignment, its measured width otherwise --
// with the margin added and subtracted in the same float order as the placement.
float FlexColumnChildWidth(ViewImpl& childImpl, float workingWidth, float lineCross, float marginCross, FlexAlign alignItems)
{
  float childCrossSize = workingWidth;
  if(childImpl.GetRequestedWidth() == MATCH_PARENT)
  {
    childCrossSize = lineCross - marginCross;
  }
  FlexAlign effectiveAlign = GetAlignSelf(childImpl);
  if(effectiveAlign == FlexAlign::AUTO)
  {
    effectiveAlign = alignItems;
  }
  if(effectiveAlign == FlexAlign::STRETCH)
  {
    childCrossSize = lineCross - marginCross;
  }
  return std::max(0.0f, (childCrossSize + marginCross) - marginCross);
}

// The cross extent of every line as Arrange() places it, in the order of @p lines: a single NO_WRAP
// line spans at least the available cross extent, and wrapping lines under alignContent STRETCH
// share the free cross extent equally. The total is summed in placement order (Arrange() reverses
// the lines first for WRAP_REVERSE), so the shares are the very values the placement computes.
// Only the line cross extents are read, which the main-axis grow/shrink does not change.
std::vector<float> FlexPlacementLineCross(const std::vector<FlexLine>& lines, float availableCross, FlexWrap wrap, FlexAlign alignContent)
{
  std::vector<float> lineCross(lines.size(), 0.0f);
  for(size_t l = 0; l < lines.size(); ++l)
  {
    lineCross[l] = lines[l].crossSize;
  }

  if(wrap == FlexWrap::NO_WRAP && lines.size() == 1)
  {
    lineCross[0] = std::max(lineCross[0], availableCross);
  }

  if(wrap != FlexWrap::NO_WRAP && !lines.empty() && alignContent == FlexAlign::STRETCH)
  {
    float totalLineCross = 0.0f;
    if(wrap == FlexWrap::WRAP_REVERSE)
    {
      for(size_t l = lines.size(); l > 0; --l)
      {
        totalLineCross += lineCross[l - 1];
      }
    }
    else
    {
      for(size_t l = 0; l < lines.size(); ++l)
      {
        totalLineCross += lineCross[l];
      }
    }
    const float freeCross    = std::max(0.0f, availableCross - totalLineCross);
    const float extraPerLine = freeCross / static_cast<float>(lines.size());
    for(float& cross : lineCross)
    {
      cross += extraPerLine;
    }
  }
  return lineCross;
}

// Height-for-width, ROW, after the grow/shrink in Arrange(): every WRAP_CONTENT-height child is
// resolved at the width the placement gives it, with the budget Measure() hands it. Adopted only
// when the line cross extents rebuilt from those heights -- in the line builder's own form -- fit
// the content height together; the lines and the dependent children's heights are then updated.
void ApplyFlexRowResolvedHeights(std::vector<View>& children, std::vector<FlexLine>& lines,
                                 std::vector<MeasuredSize>& workingSizes, float contentWidth, float contentHeight)
{
  std::vector<float> resolved;
  for(const FlexLine& line : lines)
  {
    for(uint32_t idx : line.childIndices)
    {
      ViewImpl& childImpl = GetImpl(children[idx]);
      if(childImpl.GetRequestedHeight() != WRAP_CONTENT)
      {
        continue;
      }
      float                         childScale  = childImpl.GetEffectiveScale();
      Insets                        margin      = childImpl.GetMargin();
      float                         marginMain  = (margin.start + margin.end) * childScale;
      float                         marginCross = (margin.top + margin.bottom) * childScale;
      const Internal::HeightAtWidth result =
        Internal::ResolveHeightAtWidth(childImpl,
                                       FlexRowChildWidth(childImpl, workingSizes[idx].width, contentWidth, marginMain),
                                       std::max(0.0f, contentHeight - marginCross));
      if(result.dependsOnWidth)
      {
        if(resolved.empty())
        {
          resolved.assign(children.size(), -1.0f);
        }
        resolved[idx] = result.height;
      }
    }
  }
  if(resolved.empty())
  {
    return;
  }

  std::vector<float> lineCross(lines.size(), 0.0f);
  float              needed = 0.0f;
  for(size_t l = 0; l < lines.size(); ++l)
  {
    float crossSize = 0.0f;
    for(uint32_t idx : lines[l].childIndices)
    {
      ViewImpl& childImpl  = GetImpl(children[idx]);
      float     childScale = childImpl.GetEffectiveScale();
      Insets    margin     = childImpl.GetMargin();
      float     height     = (resolved[idx] >= 0.0f) ? resolved[idx] : workingSizes[idx].height;
      crossSize            = std::max(crossSize, height + (margin.top + margin.bottom) * childScale);
    }
    lineCross[l] = crossSize;
    needed += crossSize;
  }
  if(needed > contentHeight + Internal::HEIGHT_FOR_WIDTH_FIT_EPSILON)
  {
    return; // Does not fit: arrange exactly as before.
  }

  for(size_t l = 0; l < lines.size(); ++l)
  {
    lines[l].crossSize = lineCross[l];
  }
  for(uint32_t idx = 0; idx < resolved.size(); ++idx)
  {
    if(resolved[idx] >= 0.0f)
    {
      workingSizes[idx].height = resolved[idx];
    }
  }
}

// Height-for-width, COLUMN, before the grow/shrink in Arrange(): every WRAP_CONTENT-height child
// without a flex basis (a basis is an explicit main size) is resolved at the width the placement
// gives it for its line's cross extent. Adopted only when every line's main extent rebuilt from
// those heights -- in the line builder's own form -- fits the content height; the line main extents
// and the dependent children's heights are then updated, so grow/shrink starts from them. The lines
// themselves are not re-split: the break points stay those of the measured heights.
void ApplyFlexColumnResolvedHeights(std::vector<View>& children, std::vector<FlexLine>& lines,
                                    std::vector<MeasuredSize>& workingSizes, float contentWidth, float contentHeight,
                                    FlexWrap wrap, FlexAlign alignItems, FlexAlign alignContent)
{
  const std::vector<float> lineCross = FlexPlacementLineCross(lines, contentWidth, wrap, alignContent);

  std::vector<float> resolved;
  for(size_t l = 0; l < lines.size(); ++l)
  {
    for(uint32_t idx : lines[l].childIndices)
    {
      ViewImpl& childImpl = GetImpl(children[idx]);
      if(childImpl.GetRequestedHeight() != WRAP_CONTENT || GetFlexBasis(childImpl) >= 0.0f)
      {
        continue;
      }
      float                         childScale  = childImpl.GetEffectiveScale();
      Insets                        margin      = childImpl.GetMargin();
      float                         marginMain  = (margin.top + margin.bottom) * childScale;
      float                         marginCross = (margin.start + margin.end) * childScale;
      const Internal::HeightAtWidth result =
        Internal::ResolveHeightAtWidth(childImpl,
                                       FlexColumnChildWidth(childImpl, workingSizes[idx].width, lineCross[l], marginCross, alignItems),
                                       std::max(0.0f, contentHeight - marginMain));
      if(result.dependsOnWidth)
      {
        if(resolved.empty())
        {
          resolved.assign(children.size(), -1.0f);
        }
        resolved[idx] = result.height;
      }
    }
  }
  if(resolved.empty())
  {
    return;
  }

  std::vector<float> lineMain(lines.size(), 0.0f);
  float              needed = 0.0f;
  for(size_t l = 0; l < lines.size(); ++l)
  {
    float mainSize = 0.0f;
    for(uint32_t idx : lines[l].childIndices)
    {
      ViewImpl& childImpl  = GetImpl(children[idx]);
      float     childScale = childImpl.GetEffectiveScale();
      Insets    margin     = childImpl.GetMargin();
      float     height     = (resolved[idx] >= 0.0f) ? resolved[idx] : workingSizes[idx].height;
      mainSize += height + (margin.top + margin.bottom) * childScale;
    }
    lineMain[l] = mainSize;
    needed      = std::max(needed, mainSize);
  }
  if(needed > contentHeight + Internal::HEIGHT_FOR_WIDTH_FIT_EPSILON)
  {
    return; // Does not fit: arrange exactly as before.
  }

  for(size_t l = 0; l < lines.size(); ++l)
  {
    lines[l].mainSize = lineMain[l];
  }
  for(uint32_t idx = 0; idx < resolved.size(); ++idx)
  {
    if(resolved[idx] >= 0.0f)
    {
      workingSizes[idx].height = resolved[idx];
    }
  }
}

} // namespace

class FlexLayoutManager::Impl : public LayoutManager::Impl
{
public:
  Impl(FlexDirection direction, FlexWrap wrap, FlexJustify justify, FlexAlign alignItems, FlexAlign alignContent)
  : mDirection(direction),
    mWrap(wrap),
    mJustifyContent(justify),
    mAlignItems(alignItems),
    mAlignContent(alignContent)
  {
  }

  FlexDirection mDirection;
  FlexWrap      mWrap;
  FlexJustify   mJustifyContent;
  FlexAlign     mAlignItems;
  FlexAlign     mAlignContent;
};

FlexLayoutManager::FlexLayoutManager(FlexDirection direction, FlexWrap wrap, FlexJustify justify, FlexAlign alignItems,
                                     FlexAlign alignContent)
: LayoutManager(new Impl(direction, wrap, justify, alignItems, alignContent))
{
}

FlexLayoutManager::~FlexLayoutManager()
{
}

void FlexLayoutManager::SetDirection(FlexDirection direction)
{
  Impl* impl = GetImplAs<Impl>();
  if(impl->mDirection == direction)
  {
    return;
  }
  impl->mDirection = direction;

  // A MEASURE input: the direction maps which constraint is the main axis, so
  // line breaking and the accumulated extents both change with it. Neither cache
  // key can see this manager's own state, so the owner has to be told. See
  // LayoutManager::InvalidateOwnerMeasure.
  InvalidateOwnerMeasure();
}

FlexDirection FlexLayoutManager::GetDirection() const
{
  return GetImplAs<Impl>()->mDirection;
}

void FlexLayoutManager::SetWrap(FlexWrap wrap)
{
  Impl* impl = GetImplAs<Impl>();
  if(impl->mWrap == wrap)
  {
    return;
  }
  impl->mWrap = wrap;

  // A MEASURE input, like the direction: wrapping decides how many lines the
  // children break into, which is what the measured extents accumulate.
  InvalidateOwnerMeasure();
}

FlexWrap FlexLayoutManager::GetWrap() const
{
  return GetImplAs<Impl>()->mWrap;
}

void FlexLayoutManager::SetJustifyContent(FlexJustify justify)
{
  Impl* impl = GetImplAs<Impl>();
  if(impl->mJustifyContent == justify)
  {
    return;
  }
  impl->mJustifyContent = justify;

  // PLACEMENT-only: Measure() reads the direction and the wrap but never this
  // value -- justification distributes the free space of an already-measured
  // line, so no measured size can change. Invalidating the ARRANGE axis alone
  // keeps every measure cache in the subtree warm; the pass this schedules
  // re-runs only Arrange (and its arrange-owned re-measures, which hit their
  // measure caches at the unchanged constraints).
  InvalidateOwnerArrange();
}

FlexJustify FlexLayoutManager::GetJustifyContent() const
{
  return GetImplAs<Impl>()->mJustifyContent;
}

void FlexLayoutManager::SetAlignItems(FlexAlign align)
{
  Impl* impl = GetImplAs<Impl>();
  if(impl->mAlignItems == align)
  {
    return;
  }
  impl->mAlignItems = align;

  // PLACEMENT-only, like the justification: cross-axis alignment (stretching
  // included) is applied while placing a line and never feeds the measured
  // extents. See SetJustifyContent.
  InvalidateOwnerArrange();
}

FlexAlign FlexLayoutManager::GetAlignItems() const
{
  return GetImplAs<Impl>()->mAlignItems;
}

void FlexLayoutManager::SetAlignContent(FlexAlign align)
{
  Impl* impl = GetImplAs<Impl>();
  if(impl->mAlignContent == align)
  {
    return;
  }
  impl->mAlignContent = align;

  // PLACEMENT-only: line distribution along the cross axis within the bounds
  // handed to Arrange(). See SetJustifyContent.
  InvalidateOwnerArrange();
}

FlexAlign FlexLayoutManager::GetAlignContent() const
{
  return GetImplAs<Impl>()->mAlignContent;
}

bool FlexLayoutManager::IsMainAxisHorizontal() const
{
  const FlexDirection direction = GetImplAs<Impl>()->mDirection;
  return direction == FlexDirection::ROW || direction == FlexDirection::ROW_REVERSE;
}

bool FlexLayoutManager::IsMainAxisReversed() const
{
  const FlexDirection direction = GetImplAs<Impl>()->mDirection;
  return direction == FlexDirection::ROW_REVERSE || direction == FlexDirection::COLUMN_REVERSE;
}

MeasuredSize FlexLayoutManager::Measure(ViewImpl* view, float widthConstraint, float heightConstraint)
{
  if(!view)
  {
    return MeasuredSize(0.0f, 0.0f);
  }

  auto* impl = GetImplAs<Impl>();

  const uint32_t    childCount = GetChildViewCount(view);
  std::vector<View> children;
  children.reserve(childCount);
  for(uint32_t i = 0; i < childCount; ++i)
  {
    children.push_back(GetChildViewAt(view, i));
  }

  float availableMain = IsMainAxisHorizontal() ? widthConstraint : heightConstraint;

  std::vector<FlexLine> lines;
  FlexLine              currentLine;

  for(uint32_t i = 0; i < children.size(); ++i)
  {
    ViewImpl& childImpl = GetImpl(children[i]);

    if(IsChildStandalone(childImpl))
    {
      continue;
    }

    float        childScale            = childImpl.GetEffectiveScale();
    Insets       margin                = childImpl.GetMargin();
    float        marginW               = (margin.start + margin.end) * childScale;
    float        marginH               = (margin.top + margin.bottom) * childScale;
    float        childWidthConstraint  = std::max(0.0f, widthConstraint - marginW);
    float        childHeightConstraint = std::max(0.0f, heightConstraint - marginH);
    MeasuredSize childSize             = childImpl.Measure(childWidthConstraint, childHeightConstraint);

    MeasuredSize workingSize = childSize;
    float        basis       = GetFlexBasis(childImpl);
    if(basis >= 0.0f)
    {
      if(IsMainAxisHorizontal())
      {
        workingSize.width = basis * childScale;
      }
      else
      {
        workingSize.height = basis * childScale;
      }
    }

    float childMainSize  = IsMainAxisHorizontal() ? workingSize.width + marginW
                                                  : workingSize.height + marginH;
    float childCrossSize = IsMainAxisHorizontal() ? workingSize.height + marginH
                                                  : workingSize.width + marginW;

    bool shouldWrap = (impl->mWrap != FlexWrap::NO_WRAP) && !currentLine.childIndices.empty() &&
                      (currentLine.mainSize + childMainSize > availableMain);

    if(shouldWrap)
    {
      lines.push_back(currentLine);
      currentLine = FlexLine();
    }

    currentLine.childIndices.push_back(i);
    currentLine.mainSize += childMainSize;
    currentLine.crossSize = std::max(currentLine.crossSize, childCrossSize);
  }

  if(!currentLine.childIndices.empty())
  {
    lines.push_back(currentLine);
  }

  float totalMainSize  = 0.0f;
  float totalCrossSize = 0.0f;

  for(const auto& line : lines)
  {
    totalMainSize = std::max(totalMainSize, line.mainSize);
    totalCrossSize += line.crossSize;
  }

  MeasuredSize result;
  if(IsMainAxisHorizontal())
  {
    result.width  = totalMainSize;
    result.height = totalCrossSize;
  }
  else
  {
    result.width  = totalCrossSize;
    result.height = totalMainSize;
  }

  return result;
}

void FlexLayoutManager::Arrange(ViewImpl* view, const LayoutRect& bounds)
{
  if(!view)
  {
    return;
  }

  auto* impl = GetImplAs<Impl>();

  const uint32_t    childCount = GetChildViewCount(view);
  std::vector<View> children;
  children.reserve(childCount);
  for(uint32_t i = 0; i < childCount; ++i)
  {
    children.push_back(GetChildViewAt(view, i));
  }

  float contentWidth  = bounds.width;
  float contentHeight = bounds.height;
  float availableMain = IsMainAxisHorizontal() ? contentWidth : contentHeight;

  // Local working buffer: seeded from each child's current measured size.
  std::vector<MeasuredSize> workingSizes(children.size());
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    workingSizes[i] = GetImpl(children[i]).GetMeasuredSize();
  }

  std::vector<FlexLine> lines =
    BuildFlexLinesForArrange(children, workingSizes, availableMain, IsMainAxisHorizontal(), impl->mWrap);

  // Height-for-width (see height-for-width-resolution.h): a child whose WRAP_CONTENT height follows
  // its width is given the height it needs at the width the placement below gives it -- in a column,
  // before grow/shrink distributes the main axis; in a row, after it has fixed every width. Only an
  // exact FlexLayoutManager on a view without layout callbacks does this; when no child depends on
  // its width the lines and sizes stay exactly as built.
  const bool resolveHeights = Internal::IsHeightForWidthContainer(*view);
  if(resolveHeights && !IsMainAxisHorizontal())
  {
    ApplyFlexColumnResolvedHeights(children, lines, workingSizes, contentWidth, contentHeight, impl->mWrap, impl->mAlignItems, impl->mAlignContent);
  }

  for(auto& line : lines)
  {
    ApplyFlexGrowShrink(line, children, workingSizes, availableMain, IsMainAxisHorizontal());
  }

  if(resolveHeights && IsMainAxisHorizontal())
  {
    ApplyFlexRowResolvedHeights(children, lines, workingSizes, contentWidth, contentHeight);
  }

  if(impl->mWrap == FlexWrap::WRAP_REVERSE)
  {
    std::reverse(lines.begin(), lines.end());
  }

  float availableCross = IsMainAxisHorizontal() ? contentHeight : contentWidth;

  if(impl->mWrap == FlexWrap::NO_WRAP && lines.size() == 1)
  {
    lines[0].crossSize = std::max(lines[0].crossSize, availableCross);
  }

  float totalLineCross = 0.0f;
  for(const auto& line : lines)
  {
    totalLineCross += line.crossSize;
  }
  float freeCross = std::max(0.0f, availableCross - totalLineCross);

  float crossOffset = 0.0f;

  if(impl->mWrap != FlexWrap::NO_WRAP && !lines.empty())
  {
    switch(impl->mAlignContent)
    {
      case FlexAlign::FLEX_END:
        crossOffset = freeCross;
        break;
      case FlexAlign::CENTER:
        crossOffset = freeCross / 2.0f;
        break;
      case FlexAlign::STRETCH:
      {
        float extraPerLine = freeCross / static_cast<float>(lines.size());
        for(auto& line : lines)
        {
          line.crossSize += extraPerLine;
        }
        break;
      }
      case FlexAlign::FLEX_START:
      case FlexAlign::AUTO:
      case FlexAlign::BASELINE:
      default:
        break;
    }
  }

  for(auto& line : lines)
  {
    float              freeSpace  = availableMain - line.mainSize;
    FlexJustifyOffsets justify    = GetFlexJustifyOffsets(freeSpace, impl->mJustifyContent, line.childIndices.size());
    float              mainOffset = justify.mainOffset;
    if(IsMainAxisReversed())
    {
      float contentMain = IsMainAxisHorizontal() ? contentWidth : contentHeight;
      mainOffset        = contentMain - justify.mainOffset;
    }
    ArrangeOneFlexLine(view, line, children, workingSizes, bounds, contentWidth, contentHeight, crossOffset, mainOffset,
                       justify.spacing, impl->mAlignItems, IsMainAxisHorizontal(), IsMainAxisReversed());
  }
}

namespace Internal
{

bool ResolveFlexContentHeightForWidth(const FlexLayoutManager& manager, ViewImpl& view, float contentWidth, float contentHeightBudget, float& contentHeight, float predictedContentWidth)
{
  const uint32_t        childCount = view.GetChildViewCount();
  std::vector<Ui::View> children;
  children.reserve(childCount);
  for(uint32_t i = 0; i < childCount; ++i)
  {
    children.push_back(view.GetChildViewAt(i));
  }

  std::vector<MeasuredSize> workingSizes(children.size());
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    workingSizes[i] = GetImpl(children[i]).GetMeasuredSize();
  }

  const FlexWrap wrap    = manager.GetWrap();
  bool           depends = false;
  float          height  = 0.0f;
  if(manager.IsMainAxisHorizontal())
  {
    // ROW: the lines the arrange builds at exactly this content width, grown/shrunk there, and the
    // sum of their cross extents (Measure()'s WRAP_CONTENT height) with every width-dependent child
    // resolved at its placement width. A wrapping row depends on its width by itself: its line
    // breaks follow the width.
    std::vector<FlexLine> lines = BuildFlexLinesForArrange(children, workingSizes, contentWidth, true, wrap);
    for(auto& line : lines)
    {
      ApplyFlexGrowShrink(line, children, workingSizes, contentWidth, true);
    }
    depends = (wrap != FlexWrap::NO_WRAP);

    for(const FlexLine& line : lines)
    {
      float crossSize = 0.0f;
      for(uint32_t idx : line.childIndices)
      {
        ViewImpl& childImpl   = GetImpl(children[idx]);
        float     childScale  = childImpl.GetEffectiveScale();
        Insets    margin      = childImpl.GetMargin();
        float     marginMain  = (margin.start + margin.end) * childScale;
        float     marginCross = (margin.top + margin.bottom) * childScale;
        float     childHeight = workingSizes[idx].height;
        if(childImpl.GetRequestedHeight() == WRAP_CONTENT)
        {
          const HeightAtWidth resolved = ResolveHeightAtWidth(childImpl, FlexRowChildWidth(childImpl, workingSizes[idx].width, contentWidth, marginMain), std::max(0.0f, contentHeightBudget - marginCross));
          if(resolved.dependsOnWidth)
          {
            childHeight = resolved.height;
            depends     = true;
          }
        }
        crossSize = std::max(crossSize, childHeight + marginCross);
      }
      height += crossSize;
    }
  }
  else
  {
    // COLUMN: the lines Measure() builds against the height budget, and the largest line main
    // extent (Measure()'s WRAP_CONTENT height) with every width-dependent child -- no flex basis --
    // resolved at the width the placement gives it for its line's cross extent.
    //
    // A MATCH_PARENT-width flex is re-measured at its slot by its parent before it is arranged: its
    // WRAP_CONTENT-width children are then taken at the width that re-measure gives them (their
    // measured width capped by the predicted content width less their margin, the constraint
    // Measure() hands them), for the line cross extents and for their own width alike. The line
    // cross extents themselves still use the unclamped content width the arrange gives. A row is
    // left alone: its grow/shrink decides the widths.
    if(predictedContentWidth >= 0.0f)
    {
      for(uint32_t i = 0; i < children.size(); ++i)
      {
        ViewImpl& childImpl = GetImpl(children[i]);
        if(IsChildStandalone(childImpl) || childImpl.GetRequestedWidth() != WRAP_CONTENT)
        {
          continue;
        }
        Insets margin         = childImpl.GetMargin();
        workingSizes[i].width = PredictRemeasuredWidth(childImpl, workingSizes[i].width, std::max(0.0f, predictedContentWidth - (margin.start + margin.end) * childImpl.GetEffectiveScale()));
      }
    }

    std::vector<FlexLine>    lines     = BuildFlexLinesForArrange(children, workingSizes, contentHeightBudget, false, wrap);
    const std::vector<float> lineCross = FlexPlacementLineCross(lines, contentWidth, wrap, manager.GetAlignContent());
    const FlexAlign          align     = manager.GetAlignItems();

    for(size_t l = 0; l < lines.size(); ++l)
    {
      float mainSize = 0.0f;
      for(uint32_t idx : lines[l].childIndices)
      {
        ViewImpl& childImpl   = GetImpl(children[idx]);
        float     childScale  = childImpl.GetEffectiveScale();
        Insets    margin      = childImpl.GetMargin();
        float     marginMain  = (margin.top + margin.bottom) * childScale;
        float     marginCross = (margin.start + margin.end) * childScale;
        float     childHeight = workingSizes[idx].height;
        if(childImpl.GetRequestedHeight() == WRAP_CONTENT && GetFlexBasis(childImpl) < 0.0f)
        {
          const HeightAtWidth resolved = ResolveHeightAtWidth(childImpl, FlexColumnChildWidth(childImpl, workingSizes[idx].width, lineCross[l], marginCross, align), std::max(0.0f, contentHeightBudget - marginMain));
          if(resolved.dependsOnWidth)
          {
            childHeight = resolved.height;
            depends     = true;
          }
        }
        mainSize += childHeight + marginMain;
      }
      height = std::max(height, mainSize);
    }
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
