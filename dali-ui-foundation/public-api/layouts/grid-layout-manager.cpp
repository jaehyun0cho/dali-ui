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
#include <dali-ui-foundation/public-api/layouts/grid-layout-manager.h>

// EXTERNAL INCLUDES
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

// INTERNAL INCLUDES
#include <dali-ui-foundation/internal/layouts/grid-layout-params-impl.h>
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

uint32_t GetChildRow(ViewImpl& childImpl)
{
  auto* params = Internal::GridLayoutParamsImpl::Get(childImpl);
  return params ? params->GetRow() : 0;
}

uint32_t GetChildColumn(ViewImpl& childImpl)
{
  auto* params = Internal::GridLayoutParamsImpl::Get(childImpl);
  return params ? params->GetColumn() : 0;
}

uint32_t GetChildRowSpan(ViewImpl& childImpl)
{
  auto* params = Internal::GridLayoutParamsImpl::Get(childImpl);
  return params ? params->GetRowSpan() : 1;
}

uint32_t GetChildColumnSpan(ViewImpl& childImpl)
{
  auto* params = Internal::GridLayoutParamsImpl::Get(childImpl);
  return params ? params->GetColumnSpan() : 1;
}

LayoutAlignment GetChildHorizontalAlignment(ViewImpl& childImpl)
{
  auto* params = Internal::GridLayoutParamsImpl::Get(childImpl);
  return params ? params->GetHorizontalAlignment() : LayoutAlignment::FILL;
}

LayoutAlignment GetChildVerticalAlignment(ViewImpl& childImpl)
{
  auto* params = Internal::GridLayoutParamsImpl::Get(childImpl);
  return params ? params->GetVerticalAlignment() : LayoutAlignment::FILL;
}

bool IsChildStandalone(ViewImpl& childImpl)
{
  return childImpl.GetLayoutMode() == LayoutMode::STANDALONE;
}

// Reports whether the track at the given index behaves as auto-sized. An axis
// with no definitions falls back to a single implicit track, which behaves
// like an auto-sized track here, capturing the child content size as a floor;
// ApplyGridDefinitions then lets it grow to fill a definite grid (so
// MATCH_PARENT children also work), matching a single star cell.
bool IsAutoLikeTrack(const Dali::Vector<GridLength>& defs, uint32_t index)
{
  return defs.Size() == 0u || (index < defs.Size() && defs[index].GetType() == GridLengthType::AUTO);
}

// Keep in sync with FillGridAxisFromExtents, the non-measuring per-axis copy the height-for-width
// resolution uses: the track arithmetic of the two must stay identical.
void MeasureGridChildrenAndFillAuto(std::vector<View>& children, float availableWidth, float availableHeight,
                                    uint32_t rowCount, uint32_t colCount,
                                    const Dali::Vector<GridLength>& rowDefs,
                                    const Dali::Vector<GridLength>& colDefs,
                                    std::vector<float>&             rowHeights,
                                    std::vector<float>&             colWidths,
                                    float rowSpacing, float colSpacing, float scale)
{
  // First pass: capture the content size of every non-spanning child as the
  // floor of its auto-sized track (auto track intrinsic sizing).
  for(auto& childView : children)
  {
    ViewImpl& childImpl = GetImpl(childView);

    if(IsChildStandalone(childImpl))
    {
      continue;
    }

    uint32_t row     = GetChildRow(childImpl);
    uint32_t col     = GetChildColumn(childImpl);
    uint32_t rowSpan = GetChildRowSpan(childImpl);
    uint32_t colSpan = GetChildColumnSpan(childImpl);
    row              = std::min(row, rowCount - 1);
    col              = std::min(col, colCount - 1);
    rowSpan          = std::min(rowSpan, rowCount - row);
    colSpan          = std::min(colSpan, colCount - col);

    float        childScale            = childImpl.GetEffectiveScale();
    Insets       margin                = childImpl.GetMargin();
    float        marginW               = (margin.start + margin.end) * childScale;
    float        marginH               = (margin.top + margin.bottom) * childScale;
    float        childWidthConstraint  = std::max(0.0f, availableWidth - marginW);
    float        childHeightConstraint = std::max(0.0f, availableHeight - marginH);
    MeasuredSize childSize             = childImpl.Measure(childWidthConstraint, childHeightConstraint);

    bool rowAutoLike = IsAutoLikeTrack(rowDefs, row);
    bool colAutoLike = IsAutoLikeTrack(colDefs, col);
    if(rowSpan == 1 && rowAutoLike)
    {
      rowHeights[row] = std::max(rowHeights[row], childSize.height + marginH);
    }
    if(colSpan == 1 && colAutoLike)
    {
      colWidths[col] = std::max(colWidths[col], childSize.width + marginW);
    }
  }

  // Between the passes, seed each ABSOLUTE track with its resolved base size
  // (def.GetValue() * scale, identical to ApplyGridDefinitions). Without this
  // the span pass below sees ABSOLUTE tracks as 0, overstates the deficit, and
  // dumps the surplus onto the spanned AUTO tracks. STAR tracks are left at
  // their intrinsic 0 here: their final size depends on the remaining space
  // (which in turn depends on the AUTO sizes computed below), so it is not yet
  // known. The minimum guarantee this restores is that AUTO is not over-grown
  // by the ABSOLUTE portion of a span.
  for(uint32_t i = 0; i < colCount; ++i)
  {
    if(i < colDefs.Size() && colDefs[i].GetType() == GridLengthType::ABSOLUTE)
    {
      colWidths[i] = colDefs[i].GetValue() * scale;
    }
  }
  for(uint32_t i = 0; i < rowCount; ++i)
  {
    if(i < rowDefs.Size() && rowDefs[i].GetType() == GridLengthType::ABSOLUTE)
    {
      rowHeights[i] = rowDefs[i].GetValue() * scale;
    }
  }

  // Second pass: distribute extra space for spanning children (span > 1) across
  // the auto-sized tracks they cross. After the first pass the auto tracks hold
  // their single-cell content floor;
  // a spanning child that needs more than the sum of the tracks (plus the gaps
  // between them) it currently spans grows the auto tracks so it is not
  // clipped. The deficit is shared equally across only the spanned auto tracks;
  // non-auto tracks (absolute/star) are left untouched here.
  for(auto& childView : children)
  {
    ViewImpl& childImpl = GetImpl(childView);

    if(IsChildStandalone(childImpl))
    {
      continue;
    }

    uint32_t row     = GetChildRow(childImpl);
    uint32_t col     = GetChildColumn(childImpl);
    uint32_t rowSpan = GetChildRowSpan(childImpl);
    uint32_t colSpan = GetChildColumnSpan(childImpl);
    row              = std::min(row, rowCount - 1);
    col              = std::min(col, colCount - 1);
    rowSpan          = std::min(rowSpan, rowCount - row);
    colSpan          = std::min(colSpan, colCount - col);

    if(rowSpan <= 1 && colSpan <= 1)
    {
      continue; // Non-spanning children are already handled by the first pass.
    }

    float        childScale            = childImpl.GetEffectiveScale();
    Insets       margin                = childImpl.GetMargin();
    float        marginW               = (margin.start + margin.end) * childScale;
    float        marginH               = (margin.top + margin.bottom) * childScale;
    float        childWidthConstraint  = std::max(0.0f, availableWidth - marginW);
    float        childHeightConstraint = std::max(0.0f, availableHeight - marginH);
    MeasuredSize childSize             = childImpl.Measure(childWidthConstraint, childHeightConstraint);

    // Column axis: grow spanned auto columns to fit the child's width.
    if(colSpan > 1)
    {
      uint32_t spannedAutoCount = 0u;
      float    currentSpanSize  = colSpacing * static_cast<float>(colSpan - 1); // Gaps between spanned tracks.
      for(uint32_t i = 0; i < colSpan; ++i)
      {
        currentSpanSize += colWidths[col + i];
        if(IsAutoLikeTrack(colDefs, col + i))
        {
          ++spannedAutoCount;
        }
      }
      if(spannedAutoCount > 0u)
      {
        float deficit = std::max(0.0f, (childSize.width + marginW) - currentSpanSize);
        if(deficit > 0.0f)
        {
          float share = deficit / static_cast<float>(spannedAutoCount);
          for(uint32_t i = 0; i < colSpan; ++i)
          {
            if(IsAutoLikeTrack(colDefs, col + i))
            {
              colWidths[col + i] += share;
            }
          }
        }
      }
    }

    // Row axis: grow spanned auto rows to fit the child's height.
    if(rowSpan > 1)
    {
      uint32_t spannedAutoCount = 0u;
      float    currentSpanSize  = rowSpacing * static_cast<float>(rowSpan - 1); // Gaps between spanned tracks.
      for(uint32_t i = 0; i < rowSpan; ++i)
      {
        currentSpanSize += rowHeights[row + i];
        if(IsAutoLikeTrack(rowDefs, row + i))
        {
          ++spannedAutoCount;
        }
      }
      if(spannedAutoCount > 0u)
      {
        float deficit = std::max(0.0f, (childSize.height + marginH) - currentSpanSize);
        if(deficit > 0.0f)
        {
          float share = deficit / static_cast<float>(spannedAutoCount);
          for(uint32_t i = 0; i < rowSpan; ++i)
          {
            if(IsAutoLikeTrack(rowDefs, row + i))
            {
              rowHeights[row + i] += share;
            }
          }
        }
      }
    }
  }
}

// ONE axis of MeasureGridChildrenAndFillAuto, fed with each child's extent on that axis instead of
// measuring: the single-span auto floors, the ABSOLUTE seed and the span pass, with the same index
// clamps, margins and deficit sharing. The height-for-width resolution uses it to rebuild the rows
// from resolved heights without issuing a Measure(). Keep in sync with
// MeasureGridChildrenAndFillAuto: the track arithmetic of the two must stay identical.
//
// @param[in] extents Per child (same indices as @p children): its height on the row axis, its width
//                    on the column axis, visual units, margins excluded
// @param[in] rowAxis True to fill the row tracks, false for the column tracks
// @param[in] defs The definitions of the axis being filled
// @param[in] spacing The spacing between the tracks of that axis, visual units
// @param[in,out] tracks The tracks of that axis, zero-initialized by the caller
void FillGridAxisFromExtents(std::vector<View>& children, const std::vector<float>& extents, bool rowAxis,
                             uint32_t rowCount, uint32_t colCount, const Dali::Vector<GridLength>& defs,
                             float spacing, float scale, std::vector<float>& tracks)
{
  const uint32_t trackCount = rowAxis ? rowCount : colCount;

  // First pass: the extent of every child that does not span this axis is the floor of its
  // auto-sized track.
  for(uint32_t index = 0; index < children.size(); ++index)
  {
    ViewImpl& childImpl = GetImpl(children[index]);

    if(IsChildStandalone(childImpl))
    {
      continue;
    }

    uint32_t row     = GetChildRow(childImpl);
    uint32_t col     = GetChildColumn(childImpl);
    uint32_t rowSpan = GetChildRowSpan(childImpl);
    uint32_t colSpan = GetChildColumnSpan(childImpl);
    row              = std::min(row, rowCount - 1);
    col              = std::min(col, colCount - 1);
    rowSpan          = std::min(rowSpan, rowCount - row);
    colSpan          = std::min(colSpan, colCount - col);

    float    childScale = childImpl.GetEffectiveScale();
    Insets   margin     = childImpl.GetMargin();
    float    marginAxis = rowAxis ? (margin.top + margin.bottom) * childScale : (margin.start + margin.end) * childScale;
    uint32_t track      = rowAxis ? row : col;
    uint32_t span       = rowAxis ? rowSpan : colSpan;
    if(span == 1 && IsAutoLikeTrack(defs, track))
    {
      tracks[track] = std::max(tracks[track], extents[index] + marginAxis);
    }
  }

  // Between the passes, seed each ABSOLUTE track with its resolved base size.
  for(uint32_t i = 0; i < trackCount; ++i)
  {
    if(i < defs.Size() && defs[i].GetType() == GridLengthType::ABSOLUTE)
    {
      tracks[i] = defs[i].GetValue() * scale;
    }
  }

  // Second pass: a child spanning more than one track of this axis grows the auto tracks it spans
  // by an equal share of its deficit.
  for(uint32_t index = 0; index < children.size(); ++index)
  {
    ViewImpl& childImpl = GetImpl(children[index]);

    if(IsChildStandalone(childImpl))
    {
      continue;
    }

    uint32_t row     = GetChildRow(childImpl);
    uint32_t col     = GetChildColumn(childImpl);
    uint32_t rowSpan = GetChildRowSpan(childImpl);
    uint32_t colSpan = GetChildColumnSpan(childImpl);
    row              = std::min(row, rowCount - 1);
    col              = std::min(col, colCount - 1);
    rowSpan          = std::min(rowSpan, rowCount - row);
    colSpan          = std::min(colSpan, colCount - col);

    uint32_t first = rowAxis ? row : col;
    uint32_t span  = rowAxis ? rowSpan : colSpan;
    if(span <= 1)
    {
      continue;
    }

    float  childScale = childImpl.GetEffectiveScale();
    Insets margin     = childImpl.GetMargin();
    float  marginAxis = rowAxis ? (margin.top + margin.bottom) * childScale : (margin.start + margin.end) * childScale;

    uint32_t spannedAutoCount = 0u;
    float    currentSpanSize  = spacing * static_cast<float>(span - 1); // Gaps between spanned tracks.
    for(uint32_t i = 0; i < span; ++i)
    {
      currentSpanSize += tracks[first + i];
      if(IsAutoLikeTrack(defs, first + i))
      {
        ++spannedAutoCount;
      }
    }
    if(spannedAutoCount > 0u)
    {
      float deficit = std::max(0.0f, (extents[index] + marginAxis) - currentSpanSize);
      if(deficit > 0.0f)
      {
        float share = deficit / static_cast<float>(spannedAutoCount);
        for(uint32_t i = 0; i < span; ++i)
        {
          if(IsAutoLikeTrack(defs, first + i))
          {
            tracks[first + i] += share;
          }
        }
      }
    }
  }
}

// Resolves the definitions of ONE axis: ABSOLUTE tracks take their value, STAR tracks share what the
// non-STAR tracks and the spacing leave of @p available, an implicit track (an axis with no
// definitions) behaves as Star(1) that never shrinks below its content floor, and an AUTO track
// keeps its content floor. Returns the axis extent: the tracks plus the spacing between them.
float ApplyGridAxisDefinitions(std::vector<float>& tracks, const Dali::Vector<GridLength>& defs, float available,
                               float spacing, uint32_t count, float scale)
{
  float totalAbsolute = 0.0f;
  float totalStar     = 0.0f;

  for(uint32_t i = 0; i < count; ++i)
  {
    if(i < defs.Size())
    {
      const auto& def = defs[i];
      if(def.GetType() == GridLengthType::ABSOLUTE)
      {
        tracks[i] = def.GetValue() * scale;
        totalAbsolute += tracks[i];
      }
      else if(def.GetType() == GridLengthType::STAR)
      {
        totalStar += def.GetValue();
      }
      else
      {
        totalAbsolute += tracks[i];
      }
    }
    else
    {
      // Implicit track (axis has no definitions): treat as Star(1) so it fills
      // a definite grid. Its auto content floor is preserved by the star pass.
      totalStar += 1.0f;
    }
  }

  float totalSpacing = spacing * (count > 0 ? count - 1 : 0);
  float remaining    = std::max(0.0f, available - totalAbsolute - totalSpacing);

  for(uint32_t i = 0; i < count; ++i)
  {
    if(i < defs.Size() && defs[i].GetType() == GridLengthType::STAR)
    {
      float starValue = defs[i].GetValue();
      tracks[i]       = (totalStar > 0) ? (starValue / totalStar) * remaining : 0.0f;
    }
    else if(i >= defs.Size())
    {
      // Implicit Star(1) track: fill the available space but never shrink below
      // the child content floor captured during the auto pass.
      float share = (totalStar > 0) ? (1.0f / totalStar) * remaining : 0.0f;
      tracks[i]   = std::max(tracks[i], share);
    }
  }

  return std::accumulate(tracks.begin(), tracks.end(), 0.0f) + totalSpacing;
}

// The column and the row definitions never read each other's state, so each axis is resolved on
// its own, by the same statements in the same order as before they were split.
void ApplyGridDefinitions(std::vector<float>& rowHeights, std::vector<float>& colWidths,
                          const Dali::Vector<GridLength>& rowDefs, const Dali::Vector<GridLength>& colDefs,
                          float availableWidth, float availableHeight, float rowSpacing, float colSpacing,
                          uint32_t rowCount, uint32_t colCount, float& totalWidthOut, float& totalHeightOut,
                          float scale)
{
  totalWidthOut  = ApplyGridAxisDefinitions(colWidths, colDefs, availableWidth, colSpacing, colCount, scale);
  totalHeightOut = ApplyGridAxisDefinitions(rowHeights, rowDefs, availableHeight, rowSpacing, rowCount, scale);
}

void ComputeGridPositions(const std::vector<float>& rowHeights, const std::vector<float>& colWidths,
                          const LayoutRect& bounds, float rowSpacing, float colSpacing, uint32_t rowCount,
                          uint32_t colCount, std::vector<float>& rowPositionsOut, std::vector<float>& colPositionsOut)
{
  rowPositionsOut.resize(rowCount + 1);
  colPositionsOut.resize(colCount + 1);
  rowPositionsOut[0] = bounds.y;
  for(uint32_t i = 0; i < rowCount; ++i)
  {
    rowPositionsOut[i + 1] = rowPositionsOut[i] + rowHeights[i] + (i < rowCount - 1 ? rowSpacing : 0.0f);
  }
  colPositionsOut[0] = bounds.x;
  for(uint32_t i = 0; i < colCount; ++i)
  {
    colPositionsOut[i + 1] = colPositionsOut[i] + colWidths[i] + (i < colCount - 1 ? colSpacing : 0.0f);
  }
}

// The extent of the non-STAR tracks of one axis plus the spacing between all its tracks: the extent
// a WRAP_CONTENT grid reports on that axis before its STAR tracks share any extra (Measure()).
float GridNonStarExtent(const std::vector<float>& tracks, const Dali::Vector<GridLength>& defs, uint32_t count, float spacing)
{
  float extent = 0.0f;
  for(uint32_t i = 0; i < count; ++i)
  {
    if(i >= defs.Size() || defs[i].GetType() != GridLengthType::STAR)
    {
      extent += tracks[i];
    }
  }
  extent += spacing * (count > 0 ? count - 1 : 0);
  return extent;
}

// Height-for-width: resolves every non-STANDALONE WRAP_CONTENT-height child at the width
// ArrangeGridChildrenToCells gives it -- its cell (spans and spacing included) less its margin, or
// its measured width when that is narrower and the child is START/CENTER/END aligned. The budget
// is the available height less the child's margin, as Measure() hands it. On the first dependent
// child @p resolved is assigned (child count, -1.0f); entry i >= 0 is child i's resolved height.
//
// @return True when at least one child depends on its width
bool ResolveGridChildHeights(std::vector<View>& children, const std::vector<float>& colPositions, uint32_t colCount,
                             float colSpacing, float availableHeight, std::vector<float>& resolved)
{
  for(uint32_t index = 0; index < children.size(); ++index)
  {
    ViewImpl& childImpl = GetImpl(children[index]);
    if(IsChildStandalone(childImpl) || childImpl.GetRequestedHeight() != WRAP_CONTENT)
    {
      continue;
    }

    // The column clamps of ArrangeGridChildrenToCells (the rows do not enter the width).
    uint32_t col     = std::min(GetChildColumn(childImpl), colCount - 1);
    uint32_t colSpan = std::min(GetChildColumnSpan(childImpl), colCount - col);

    float  childScale = childImpl.GetEffectiveScale();
    Insets margin     = childImpl.GetMargin();
    float  width      = colPositions[col + colSpan] - colPositions[col];
    if(col + colSpan < colCount)
    {
      width -= colSpacing;
    }
    width = std::max(0.0f, width - (margin.start + margin.end) * childScale);

    const float           measuredWidth = childImpl.GetMeasuredSize().width;
    const LayoutAlignment hAlign        = GetChildHorizontalAlignment(childImpl);
    if(measuredWidth > 0.0f && measuredWidth < width &&
       (hAlign == LayoutAlignment::START || hAlign == LayoutAlignment::CENTER || hAlign == LayoutAlignment::END))
    {
      width = measuredWidth;
    }

    const Internal::HeightAtWidth result = Internal::ResolveHeightAtWidth(childImpl, width, std::max(0.0f, availableHeight - (margin.top + margin.bottom) * childScale));
    if(result.dependsOnWidth)
    {
      if(resolved.empty())
      {
        resolved.assign(children.size(), -1.0f);
      }
      resolved[index] = result.height;
    }
  }
  return !resolved.empty();
}

// @param[in] owner The arranging View, threaded through so the final MATCH_PARENT
//                  re-measure below can be attributed to it. nullptr makes the scope inert.
// @param[in] resolvedHeights Per child, its height resolved at its cell width (>= 0), or nullptr
//                  when no height was resolved. A resolved height replaces the measured one for
//                  the cell placement, so it is what a START/CENTER/END-aligned child is given.
void ArrangeGridChildrenToCells(ViewImpl* owner, std::vector<View>& children, const std::vector<float>& rowPositions,
                                const std::vector<float>& colPositions, uint32_t rowCount, uint32_t colCount,
                                float rowSpacing, float colSpacing, const std::vector<float>* resolvedHeights)
{
  for(uint32_t index = 0; index < children.size(); ++index)
  {
    ViewImpl& childImpl = GetImpl(children[index]);

    if(IsChildStandalone(childImpl))
    {
      continue;
    }

    uint32_t row     = std::min(GetChildRow(childImpl), rowCount - 1);
    uint32_t col     = std::min(GetChildColumn(childImpl), colCount - 1);
    uint32_t rowSpan = std::min(GetChildRowSpan(childImpl), rowCount - row);
    uint32_t colSpan = std::min(GetChildColumnSpan(childImpl), colCount - col);

    float      cellX = colPositions[col];
    float      cellY = rowPositions[row];
    LayoutRect childBounds;
    childBounds.x      = cellX;
    childBounds.y      = cellY;
    childBounds.width  = colPositions[col + colSpan] - cellX;
    childBounds.height = rowPositions[row + rowSpan] - cellY;

    if(col + colSpan < colCount)
    {
      childBounds.width -= colSpacing;
    }
    if(row + rowSpan < rowCount)
    {
      childBounds.height -= rowSpacing;
    }

    float  childScale = childImpl.GetEffectiveScale();
    Insets margin     = childImpl.GetMargin();
    childBounds.x += margin.start * childScale;
    childBounds.y += margin.top * childScale;
    childBounds.width  = std::max(0.0f, childBounds.width - (margin.start + margin.end) * childScale);
    childBounds.height = std::max(0.0f, childBounds.height - (margin.top + margin.bottom) * childScale);

    float        cellWidth     = childBounds.width;
    float        cellHeight    = childBounds.height;
    MeasuredSize childMeasured = childImpl.GetMeasuredSize();
    float        childWidth    = childMeasured.width;
    float        childHeight   = (resolvedHeights && (*resolvedHeights)[index] >= 0.0f) ? (*resolvedHeights)[index] : childMeasured.height;

    LayoutAlignment hAlign = GetChildHorizontalAlignment(childImpl);
    if(childWidth > 0.0f && childWidth < cellWidth)
    {
      switch(hAlign)
      {
        case LayoutAlignment::CENTER:
          childBounds.x += (cellWidth - childWidth) * 0.5f;
          childBounds.width = childWidth;
          break;
        case LayoutAlignment::END:
          childBounds.x += cellWidth - childWidth;
          childBounds.width = childWidth;
          break;
        case LayoutAlignment::START:
          childBounds.width = childWidth;
          break;
        case LayoutAlignment::FILL:
        default:
          break;
      }
    }

    LayoutAlignment vAlign = GetChildVerticalAlignment(childImpl);
    if(childHeight > 0.0f && childHeight < cellHeight)
    {
      switch(vAlign)
      {
        case LayoutAlignment::CENTER:
          childBounds.y += (cellHeight - childHeight) * 0.5f;
          childBounds.height = childHeight;
          break;
        case LayoutAlignment::END:
          childBounds.y += cellHeight - childHeight;
          childBounds.height = childHeight;
          break;
        case LayoutAlignment::START:
          childBounds.height = childHeight;
          break;
        case LayoutAlignment::FILL:
        default:
          break;
      }
    }

    if(childImpl.GetRequestedWidth() == MATCH_PARENT || childImpl.GetRequestedHeight() == MATCH_PARENT)
    {
      Internal::LayoutDependency::ArrangeOwnedMeasureScope ownerScope(owner);
      childImpl.Measure(childBounds.width, childBounds.height);
    }
    childImpl.Arrange(childBounds);
  }
}

} // namespace

class GridLayoutManager::Impl : public LayoutManager::Impl
{
public:
  Impl(const Dali::Vector<GridLength>& rows, const Dali::Vector<GridLength>& columns, float rowSpacing,
       float columnSpacing)
  : mRowDefinitions(rows),
    mColumnDefinitions(columns),
    mRowSpacing(rowSpacing),
    mColumnSpacing(columnSpacing)
  {
  }

  Dali::Vector<GridLength> mRowDefinitions;
  Dali::Vector<GridLength> mColumnDefinitions;
  float                    mRowSpacing;
  float                    mColumnSpacing;
};

GridLayoutManager::GridLayoutManager(const Dali::Vector<GridLength>& rows, const Dali::Vector<GridLength>& columns,
                                     float rowSpacing, float columnSpacing)
: LayoutManager(new Impl(rows, columns, rowSpacing, columnSpacing))
{
}

GridLayoutManager::~GridLayoutManager()
{
}

void GridLayoutManager::SetRowDefinitions(const Dali::Vector<GridLength>& rows)
{
  GetImplAs<Impl>()->mRowDefinitions = rows;

  // Row definitions are read by both Measure() and Arrange(), and neither cache key
  // can see them, so the owner has to be told. Unguarded, unlike the scalar setters
  // below: GridLength has no equality operator, and comparing element-wise to save an
  // invalidation would cost more than the invalidation on any realistic grid.
  InvalidateOwnerMeasure();
}

const Dali::Vector<GridLength>& GridLayoutManager::GetRowDefinitions() const
{
  return GetImplAs<Impl>()->mRowDefinitions;
}

void GridLayoutManager::SetColumnDefinitions(const Dali::Vector<GridLength>& columns)
{
  GetImplAs<Impl>()->mColumnDefinitions = columns;

  // See SetRowDefinitions: same reasoning, same unguarded invalidation.
  InvalidateOwnerMeasure();
}

const Dali::Vector<GridLength>& GridLayoutManager::GetColumnDefinitions() const
{
  return GetImplAs<Impl>()->mColumnDefinitions;
}

void GridLayoutManager::SetRowSpacing(float spacing)
{
  Impl* impl = GetImplAs<Impl>();
  if(impl->mRowSpacing == spacing)
  {
    return;
  }
  impl->mRowSpacing = spacing;
  InvalidateOwnerMeasure();
}

float GridLayoutManager::GetRowSpacing() const
{
  return GetImplAs<Impl>()->mRowSpacing;
}

void GridLayoutManager::SetColumnSpacing(float spacing)
{
  Impl* impl = GetImplAs<Impl>();
  if(impl->mColumnSpacing == spacing)
  {
    return;
  }
  impl->mColumnSpacing = spacing;
  InvalidateOwnerMeasure();
}

float GridLayoutManager::GetColumnSpacing() const
{
  return GetImplAs<Impl>()->mColumnSpacing;
}

MeasuredSize GridLayoutManager::Measure(ViewImpl* view, float widthConstraint, float heightConstraint)
{
  if(!view)
  {
    return MeasuredSize(0.0f, 0.0f);
  }

  auto* impl = GetImplAs<Impl>();

  float s               = view->GetEffectiveScale();
  float visRowSpacing   = impl->mRowSpacing * s;
  float visColSpacing   = impl->mColumnSpacing * s;
  float availableWidth  = widthConstraint;
  float availableHeight = heightConstraint;

  // Collect children once for reuse across the measure phase helpers.
  const uint32_t    childCount = GetChildViewCount(view);
  std::vector<View> children;
  children.reserve(childCount);
  for(uint32_t i = 0; i < childCount; ++i)
  {
    children.push_back(GetChildViewAt(view, i));
  }

  uint32_t           rowCount = std::max(1u, static_cast<uint32_t>(impl->mRowDefinitions.Size()));
  uint32_t           colCount = std::max(1u, static_cast<uint32_t>(impl->mColumnDefinitions.Size()));
  std::vector<float> rowHeights(rowCount, 0.0f);
  std::vector<float> colWidths(colCount, 0.0f);

  MeasureGridChildrenAndFillAuto(children, availableWidth, availableHeight, rowCount,
                                 colCount, impl->mRowDefinitions, impl->mColumnDefinitions, rowHeights, colWidths,
                                 visRowSpacing, visColSpacing, s);

  Insets parentPadding   = view->GetPadding();
  float  requestedWidth  = view->GetRequestedWidth();
  float  requestedHeight = view->GetRequestedHeight();

  float starAvailableWidth  = availableWidth;
  float starAvailableHeight = availableHeight;

  if(requestedWidth != MATCH_PARENT && requestedWidth < 0.0f)
  {
    float nonStarWidth = 0.0f;
    for(uint32_t i = 0; i < colCount; ++i)
    {
      if(i >= impl->mColumnDefinitions.Size() || impl->mColumnDefinitions[i].GetType() != GridLengthType::STAR)
      {
        nonStarWidth += colWidths[i];
      }
    }
    nonStarWidth += visColSpacing * (colCount > 0 ? colCount - 1 : 0);
    float pw              = (parentPadding.start + parentPadding.end) * s;
    float minWidthContent = std::max(0.0f, view->GetMinimumWidth() * s - pw);
    starAvailableWidth    = std::max(nonStarWidth, minWidthContent);
  }

  if(requestedHeight != MATCH_PARENT && requestedHeight < 0.0f)
  {
    float nonStarHeight = 0.0f;
    for(uint32_t i = 0; i < rowCount; ++i)
    {
      if(i >= impl->mRowDefinitions.Size() || impl->mRowDefinitions[i].GetType() != GridLengthType::STAR)
      {
        nonStarHeight += rowHeights[i];
      }
    }
    nonStarHeight += visRowSpacing * (rowCount > 0 ? rowCount - 1 : 0);
    float ph               = (parentPadding.top + parentPadding.bottom) * s;
    float minHeightContent = std::max(0.0f, view->GetMinimumHeight() * s - ph);
    starAvailableHeight    = std::max(nonStarHeight, minHeightContent);
  }

  float totalWidth  = 0.0f;
  float totalHeight = 0.0f;
  ApplyGridDefinitions(rowHeights, colWidths, impl->mRowDefinitions, impl->mColumnDefinitions, starAvailableWidth, starAvailableHeight,
                       visRowSpacing, visColSpacing, rowCount, colCount, totalWidth, totalHeight, s);

  return MeasuredSize(totalWidth, totalHeight);
}

void GridLayoutManager::Arrange(ViewImpl* view, const LayoutRect& bounds)
{
  if(!view)
  {
    return;
  }

  auto* impl = GetImplAs<Impl>();

  float s               = view->GetEffectiveScale();
  float visRowSpacing   = impl->mRowSpacing * s;
  float visColSpacing   = impl->mColumnSpacing * s;
  float availableWidth  = bounds.width;
  float availableHeight = bounds.height;

  const uint32_t    childCount = GetChildViewCount(view);
  std::vector<View> children;
  children.reserve(childCount);
  for(uint32_t i = 0; i < childCount; ++i)
  {
    children.push_back(GetChildViewAt(view, i));
  }

  uint32_t           rowCount = std::max(1u, static_cast<uint32_t>(impl->mRowDefinitions.Size()));
  uint32_t           colCount = std::max(1u, static_cast<uint32_t>(impl->mColumnDefinitions.Size()));
  std::vector<float> rowHeights(rowCount, 0.0f);
  std::vector<float> colWidths(colCount, 0.0f);

  // Re-measure children to get fresh auto row/column sizes using actual arrange bounds.
  // The helper only measures (it never arranges), so one scope around the whole call has
  // exactly the narrow extent required and needs no threaded owner parameter.
  {
    Internal::LayoutDependency::ArrangeOwnedMeasureScope ownerScope(view);
    MeasureGridChildrenAndFillAuto(children, availableWidth, availableHeight, rowCount,
                                   colCount, impl->mRowDefinitions, impl->mColumnDefinitions, rowHeights, colWidths,
                                   visRowSpacing, visColSpacing, s);
  }

  float totalWidth  = 0.0f;
  float totalHeight = 0.0f;
  ApplyGridDefinitions(rowHeights, colWidths, impl->mRowDefinitions, impl->mColumnDefinitions, availableWidth, availableHeight,
                       visRowSpacing, visColSpacing, rowCount, colCount, totalWidth, totalHeight, s);

  std::vector<float> rowPositions;
  std::vector<float> colPositions;
  ComputeGridPositions(rowHeights, colWidths, bounds, visRowSpacing, visColSpacing, rowCount, colCount, rowPositions,
                       colPositions);

  // Height-for-width: the columns are final now, so a child whose WRAP_CONTENT height follows its
  // width is resolved at its cell width (see height-for-width-resolution.h). The AUTO rows are then
  // rebuilt from those heights -- without measuring -- and the cell placement uses the same values,
  // so a START/CENTER/END-aligned child gets the height its row was sized for. Adopted only when the
  // rebuilt non-STAR rows fit the available height (the STAR rows then share what is left);
  // otherwise, and whenever no child depends on its width, the rows and cells stay exactly as above.
  std::vector<float> resolvedHeights;
  if(Internal::IsHeightForWidthContainer(*view) &&
     ResolveGridChildHeights(children, colPositions, colCount, visColSpacing, availableHeight, resolvedHeights))
  {
    // The heights of the children that do not depend on their width are their measured heights,
    // fresh from the re-measure above.
    std::vector<float> extents(children.size(), 0.0f);
    for(uint32_t i = 0; i < children.size(); ++i)
    {
      extents[i] = (resolvedHeights[i] >= 0.0f) ? resolvedHeights[i] : GetImpl(children[i]).GetMeasuredSize().height;
    }

    std::vector<float> resolvedRows(rowCount, 0.0f);
    FillGridAxisFromExtents(children, extents, true, rowCount, colCount, impl->mRowDefinitions, visRowSpacing, s, resolvedRows);
    if(GridNonStarExtent(resolvedRows, impl->mRowDefinitions, rowCount, visRowSpacing) <= availableHeight + Internal::HEIGHT_FOR_WIDTH_FIT_EPSILON)
    {
      ApplyGridAxisDefinitions(resolvedRows, impl->mRowDefinitions, availableHeight, visRowSpacing, rowCount, s);
      rowHeights = std::move(resolvedRows);
      ComputeGridPositions(rowHeights, colWidths, bounds, visRowSpacing, visColSpacing, rowCount, colCount, rowPositions,
                           colPositions);
    }
    else
    {
      resolvedHeights.clear(); // Does not fit: arrange exactly as before.
    }
  }

  ArrangeGridChildrenToCells(view, children, rowPositions, colPositions, rowCount, colCount, visRowSpacing, visColSpacing,
                             resolvedHeights.empty() ? nullptr : &resolvedHeights);
}

namespace Internal
{

// predictedContentWidth is ignored. For ABSOLUTE and STAR columns the cell rule of
// ResolveGridChildHeights already takes the cell whenever a stale measured width is wider (a
// re-measured grid's cell never exceeds its new content width). AUTO columns are built from the
// children's last measured widths below, a documented approximation that the grid's arrange-time
// fit test covers.
bool ResolveGridContentHeightForWidth(const GridLayoutManager& manager, ViewImpl& view, float contentWidth, float contentHeightBudget, float& contentHeight, float /* predictedContentWidth */)
{
  const float                     s             = view.GetEffectiveScale();
  const Dali::Vector<GridLength>& rowDefs       = manager.GetRowDefinitions();
  const Dali::Vector<GridLength>& colDefs       = manager.GetColumnDefinitions();
  const float                     visRowSpacing = manager.GetRowSpacing() * s;
  const float                     visColSpacing = manager.GetColumnSpacing() * s;

  const uint32_t        childCount = view.GetChildViewCount();
  std::vector<Ui::View> children;
  children.reserve(childCount);
  for(uint32_t i = 0; i < childCount; ++i)
  {
    children.push_back(view.GetChildViewAt(i));
  }

  const uint32_t rowCount = std::max(1u, static_cast<uint32_t>(rowDefs.Size()));
  const uint32_t colCount = std::max(1u, static_cast<uint32_t>(colDefs.Size()));

  // The columns the arrange would build at exactly this content width, from the last measured
  // widths (the arrange re-measures first; a resolution cannot measure, so a mismatch there is left
  // to the arrange's own fit test). The positions start where the manager's Arrange() content box
  // starts, so every cell width is the one the arrange computes.
  std::vector<float> widths(children.size(), 0.0f);
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    widths[i] = GetImpl(children[i]).GetMeasuredSize().width;
  }
  std::vector<float> colWidths(colCount, 0.0f);
  FillGridAxisFromExtents(children, widths, false, rowCount, colCount, colDefs, visColSpacing, s, colWidths);
  ApplyGridAxisDefinitions(colWidths, colDefs, contentWidth, visColSpacing, colCount, s);

  std::vector<float> colPositions(colCount + 1, 0.0f);
  colPositions[0] = static_cast<float>(view.GetPadding().start) * s;
  for(uint32_t i = 0; i < colCount; ++i)
  {
    colPositions[i + 1] = colPositions[i] + colWidths[i] + (i < colCount - 1 ? visColSpacing : 0.0f);
  }

  std::vector<float> resolved;
  if(!ResolveGridChildHeights(children, colPositions, colCount, visColSpacing, contentHeightBudget, resolved))
  {
    return false;
  }

  // The WRAP_CONTENT height of Measure() with the resolved heights in the AUTO rows: the non-STAR
  // rows (at least the minimum height less the padding) are the extent the STAR rows share.
  std::vector<float> heights(children.size(), 0.0f);
  for(uint32_t i = 0; i < children.size(); ++i)
  {
    heights[i] = (resolved[i] >= 0.0f) ? resolved[i] : GetImpl(children[i]).GetMeasuredSize().height;
  }
  std::vector<float> rowHeights(rowCount, 0.0f);
  FillGridAxisFromExtents(children, heights, true, rowCount, colCount, rowDefs, visRowSpacing, s, rowHeights);

  const float  nonStarHeight    = GridNonStarExtent(rowHeights, rowDefs, rowCount, visRowSpacing);
  const Insets padding          = view.GetPadding();
  const float  ph               = (padding.top + padding.bottom) * s;
  const float  minHeightContent = std::max(0.0f, view.GetMinimumHeight() * s - ph);
  contentHeight                 = ApplyGridAxisDefinitions(rowHeights, rowDefs, std::max(nonStarHeight, minHeightContent), visRowSpacing, rowCount, s);
  return true;
}

} // namespace Internal

} // namespace Ui
} //namespace DALI_NAMESPACE
