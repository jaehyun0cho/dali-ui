#pragma once

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

// EXTERNAL INCLUDES
#include <algorithm>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/layouts/layout-types.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>

namespace Dali
{
namespace Ui
{
namespace Internal
{

/**
 * @brief The extent a boundary view's own pass places one axis at.
 *
 * The parent-supplied extent for a MATCH_PARENT axis, the measured size otherwise.
 *
 * UNCLAMPED on purpose: a caller that re-measures the view against the extent it is
 * about to be placed in must feed it THIS value and not the clamped result, so that
 * the measure cache KEY converges too. The clamp belongs to the placement (see
 * DeriveStandaloneRootBounds), not to the constraint the producer is run at.
 *
 * @param[in] match True when this axis requested MATCH_PARENT
 * @param[in] available The extent the parent makes available on this axis
 * @param[in] measured The view's measured extent on this axis
 * @return The extent this axis is placed at
 */
inline float ResolveStandaloneExtent(bool match, float available, float measured)
{
  return match ? available : measured;
}

/**
 * @brief THE one derivation of a boundary (STANDALONE) view's own layout slot.
 *
 * Called by BOTH LayoutController::ProcessLayoutRoot -- the framework-owned self pass
 * of a boundary view driven as a layout root in its own right -- and by
 * ArrangeStandaloneChild, the parent-driven placement of the same view. Sharing one
 * function is what makes the two results converge BY CONSTRUCTION, which is the
 * premise ViewDataImpl::InvalidateParentArrangeCacheForOutOfBandArrange's
 * framework-root-pass exemption rests on: that exemption leaves the parent's arrange
 * entry standing across a boundary view's self pass, and it may only do so while the
 * self pass produces the very bounds the parent's next miss would hand it.
 *
 * @p measured is passed in rather than read from @p view so that a caller which
 * re-measures after taking its snapshot still derives the slot from the SNAPSHOT --
 * the value the extents were resolved against.
 *
 * @param[in] view The boundary view being placed
 * @param[in] availableWidth The width extent the parent (or the root pass) makes available
 * @param[in] availableHeight The height extent the parent (or the root pass) makes available
 * @param[in] measured The view's measured size snapshot
 * @return The view's own layout slot, in visual (scale-applied) units
 */
inline LayoutRect DeriveStandaloneRootBounds(ViewImpl& view, float availableWidth, float availableHeight, const MeasuredSize& measured)
{
  const float  s      = view.GetEffectiveScale();
  const Insets margin = view.GetMargin();

  LayoutRect bounds;

  // ONE float association for the position, deliberately: the arrange cache KEY is an
  // EXACT compare (SameLayoutRect), so associating the multiply differently on the two
  // call sites could produce two values that differ in the last bit at scale != 1 and
  // could then never match.
  bounds.x = (view.GetRequestedX() + static_cast<float>(margin.start)) * s;
  bounds.y = (view.GetRequestedY() + static_cast<float>(margin.top)) * s;

  bounds.width  = ResolveStandaloneExtent(view.GetRequestedWidth() == MATCH_PARENT, availableWidth, measured.width);
  bounds.height = ResolveStandaloneExtent(view.GetRequestedHeight() == MATCH_PARENT, availableHeight, measured.height);

  // A boundary view has no parent layout to clamp it, so its own min/max is enforced
  // here. For a MATCH_PARENT axis the measured value was discarded above, so this is
  // the only place min/max reaches it; for the others ApplyConstraints already applied
  // the same clamp during Measure and this is idempotent.
  bounds.width  = std::min(std::max(bounds.width, view.GetMinimumWidth() * s), view.GetMaximumWidth() * s);
  bounds.height = std::min(std::max(bounds.height, view.GetMinimumHeight() * s), view.GetMaximumHeight() * s);

  return bounds;
}

} // namespace Internal
} // namespace Ui
} // namespace Dali
