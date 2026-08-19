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
#include <cstdint>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/dali-ui-common.h>

namespace Dali
{
namespace Ui
{
namespace Internal
{
/**
 * @brief The window in which one invalidation's walk to the layout root can stand in
 * for the next one's.
 *
 * `InvalidateMeasure()` / `InvalidateArrange()` do two separable things: they record
 * local state on the view they are called on, and they walk the ancestor chain to
 * register a layout root with the LayoutController. The local half must run every
 * time. The walk is idempotent -- each level sets its own flags and the controller
 * coalesces duplicate registrations into a pending set -- so repeating it once the
 * root is already registered is pure cost, and it is not small: every level performs
 * two Actor::GetParent() calls and two handle DownCasts, and the root additionally
 * resolves its Window and re-registers with the UI scale manager. An application that
 * touches a few hundred views before the next layout pass pays all of that per touch.
 *
 * A generation is what lets the second and subsequent walks be skipped SAFELY. Each view
 * records the generation in which its walk completed; a later invalidation may skip the
 * walk only while that record still matches the current generation, which is exactly the
 * window in which "the root is registered and has not been processed yet" holds.
 *
 * @note This deliberately does NOT reintroduce the dirty-flag short-circuit that used
 * to live in InvalidateMeasure(). That one asked "is this view already dirty?", which
 * a view whose parent never arranges it answers "yes" forever, swallowing every later
 * invalidation. This asks "has the registration this walk would make already been
 * made and not yet consumed?", and the answer goes false again at the next drain.
 *
 * @note Event thread only, and not thread-safe: every reader and writer runs on the
 * event thread, in the same synchronous call graph as the layout pass itself.
 */
namespace LayoutInvalidation
{
/**
 * @brief Returns the current invalidation generation.
 *
 * Never returns 0, so 0 is usable as a "never propagated" initial value.
 *
 * @return The current generation
 */
DALI_UI_API uint32_t CurrentGeneration();

/**
 * @brief Ends the current generation and starts a new one.
 *
 * Called wherever a record could otherwise outlive the state its walk established:
 *
 *  - by the LayoutController when a pass drains the pending set, and when a root is
 *    dropped from it without being processed (the REGISTRATION half of the record's
 *    claim stops holding);
 *  - by the outermost Measure/Arrange pass guard on exit (the MARKING half: a pass is
 *    the only consumer of dirty bits, and a manual Measure()/Arrange() on an ancestor
 *    -- both public API -- can consume a walked chain's dirty without any drain).
 *
 * Every recorded generation becomes stale at that moment, so the next invalidation on any
 * view walks and re-registers in full. Cache hits construct no pass guard, so a
 * settled tree bumps nothing.
 *
 * Wrap-around is nearly harmless and self-healing: a stale record can only collide
 * with the current generation after 2^32 bumps, a collision merely coalesces one walk that
 * should have run, and the next pass re-ends the generation. Skipping 0 on wrap keeps the
 * "never propagated" value unreachable.
 */
DALI_UI_API void AdvanceGeneration();

/**
 * @brief Returns whether the LayoutController is currently delivering LayoutFinished signals.
 *
 * This is the second half of the LAYOUT PROCESSING WINDOW. The window is open while
 * EITHER a Measure/Arrange pass is on the stack (ViewDataImpl::IsLayoutPassOnStack())
 * OR a LayoutFinished emit is in progress (this function), and while it is open the
 * public-API entry points View::InvalidateMeasure() / View::InvalidateArrange() -- and
 * the public LayoutController::RequestLayout() -- are WARNED ONCE PER VIEW AND IGNORED
 * rather than honoured.
 *
 * The window exists so that layout processing can never re-arm the idle pump. An
 * invalidation raised from inside it would register a layout root, which schedules
 * another pass, which settles, which emits again, which invalidates again: the event
 * loop is kept awake every frame and never goes idle. Closing the window at the public
 * boundary is what breaks that circuit. Framework-internal invalidations (the walk
 * itself, tree mutations driven from a slot, resource loads) go through the internal
 * ViewDataImpl primitives and are deliberately EXEMPT -- they still walk, poison and
 * register, which is what lets a slot-driven tree mutation converge on the next frame.
 *
 * The emit half is a separate counter rather than a bump of the pass depth because a
 * slot runs at pass depth 0 by design: the emit happens in the post-process phase,
 * after every Measure/Arrange guard has unwound.
 *
 * @return True while a LayoutFinished emit is in progress
 */
DALI_UI_API bool IsLayoutFinishedEmitInProgress();

/**
 * @brief Opens the LayoutFinished half of the layout processing window.
 *
 * Nests: the counter it increments allows a re-entrant emit (a slot that drives a
 * nested settle) to keep the window open until the OUTERMOST emit unwinds.
 *
 * Prefer ScopedLayoutFinishedEmit over calling this directly, so that an exception
 * or an early return out of a slot cannot leave the window stuck open.
 */
DALI_UI_API void BeginLayoutFinishedEmit();

/**
 * @brief Closes one nesting level of the LayoutFinished half of the window.
 *
 * Must be paired with exactly one BeginLayoutFinishedEmit().
 */
DALI_UI_API void EndLayoutFinishedEmit();

/**
 * @brief RAII scope that holds the LayoutFinished half of the layout processing window open.
 *
 * Construct one around every emit site -- both the per-View LayoutFinished delivery and
 * the window-level LayoutController::LayoutFinishedSignal -- so that the whole emit,
 * including anything a slot re-enters, runs inside the window.
 */
struct ScopedLayoutFinishedEmit
{
  ScopedLayoutFinishedEmit()
  {
    BeginLayoutFinishedEmit();
  }

  ~ScopedLayoutFinishedEmit()
  {
    EndLayoutFinishedEmit();
  }

  ScopedLayoutFinishedEmit(const ScopedLayoutFinishedEmit&)            = delete;
  ScopedLayoutFinishedEmit& operator=(const ScopedLayoutFinishedEmit&) = delete;
};

} // namespace LayoutInvalidation
} // namespace Internal
} // namespace Ui
} // namespace Dali
