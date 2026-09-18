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

// Layout test observation interface. The hooks are compiled into every build; they stay
// inert until a test calls BeginCapture(). This is a supported surface: the enums, aggregates
// and the 12 functions below cannot be removed or reshaped within the v2.5.x series. The same
// commitment covers LabelImpl::GetLayoutTestTextSnapshot(), which this header's TextSnapshot
// is returned from and which is now a permanent export on an installed class. Both it and
// ViewDataImpl::GetLayoutTestSnapshot() are additive non-virtual members, so no vtable and no
// object layout changed; ViewDataImpl is not installed, so only the LabelImpl member is a new
// public export.
#include <dali-ui-foundation/public-api/dali-ui-common.h>
#include <dali-ui-foundation/public-api/layouts/layout-types.h>
#include <dali-ui-foundation/public-api/views/text-controls/label.h>
#include <dali-ui-foundation/public-api/views/view.h>
#include <dali/public-api/adaptor-framework/window.h>
#include <dali/public-api/animation/animation.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace DALI_NAMESPACE
{
namespace Ui
{
namespace Integration
{
namespace LayoutTestDiagnostics
{
/// Observation points recorded into the caller's buffer while a capture is active. Every hook
/// runs on the event thread. `detail`, `value0` and `value1` carry the per-kind payload below;
/// unlisted fields are zero. Node identities are the caller's RegisterNode ids; an event of a
/// view that was not registered carries nodeId 0.
enum class EventKind : uint16_t
{
  MEASURE_ENTER,       ///< View::Measure entered. value0/value1 = incoming visual constraint.
  MEASURE_HIT,         ///< Measure served from the cache (no producer ran).
  MEASURE_PRODUCER,    ///< The measure producer (callback, manager or default) is about to run.
  MEASURE_PUBLISH,     ///< Measured size written. value0/value1 = width/height.
  ARRANGE_ENTER,       ///< Arrange entered. value0/value1 = incoming width/height.
  ARRANGE_HIT,         ///< Arrange served from the cache; the subtree is replayed.
  ARRANGE_PRODUCER,    ///< The arrange producer is about to run.
  ARRANGE_PUBLISH,     ///< Arranged bounds published. value0/value1 = width/height.
  REPLAY_VISIT,        ///< A node visited by a cache-hit subtree replay.
  GEOMETRY_WRITE,      ///< An actor property changed by layout. detail: 0 x, 1 y, 2 width, 3 height; value0 = new value. Also emitted for the parent's right-to-left mirror write (detail 0, relatedNodeId = parent); on a cache-hit replay the mirror is folded into the node's single x write, which then carries relatedNodeId = parent.
  INVALIDATE_MEASURE,  ///< Entry of InvalidateMeasure on THIS node; the propagation enters each ancestor separately, so one request produces one event per chain node. detail 1 = raised by a parent Add (relatedNodeId = parent).
  INVALIDATE_ARRANGE,  ///< Entry of InvalidateArrange on this node (same per-node semantics).
  ANCESTOR_VISIT,      ///< A node visited by an invalidation walk. detail: 0 measure, 1 arrange, 2 measure-miss cache walk (relatedNodeId = missing view), 3 parent-add propagation. Detail 0/1/3 pass a generation gate first; detail 2 has none, so it reports every node the walk reaches.
  ANCESTOR_STOP,       ///< A measure-miss walk stopped at this node. detail: 1 dependency owner, 2 measuring node, 3 direct arranging parent, 4 standalone boundary. A walk that ends because self is standalone, or because the parent chain ran out, emits no ANCESTOR_STOP: one stop per walk is not an invariant.
  GENERATION_ADVANCE,  ///< A new outer layout operation generation started (global event).
  OWNER_PUSH,          ///< A dependency owner scope was pushed for nodeId (relatedNodeId = previous owner). detail = owner kind (1 arrange, 2 recycler).
  OWNER_POP,           ///< The matching scope was popped.
  ROOT_QUEUED,         ///< A layout root was queued at the controller (global; nodeId of the root if registered).
  ROOT_DRAIN,          ///< A queued root is being processed. value0/value1 = root constraint.
  WAKE_REQUEST,        ///< The controller armed an idle wake (windowToken identifies the window). A manual ProcessLayouts does not consume it.
  PARKED_REQUEST,      ///< A request raised during a pass was parked until the next external event.
  PROCESS_BEGIN,       ///< ProcessLayouts entered for windowToken.
  PROCESS_END,         ///< ProcessLayouts left (also on early return and exception).
  COMPLETION,          ///< A View LayoutFinished signal is about to be emitted (detail 1).
  ROLLBACK_ROOT,       ///< A root retained after a producer exception.
  SNAPSHOT_ALLOCATION, ///< A named storage site allocated. detail = StorageSite; value0 = element count; value1 = requested bytes.
  TRANSITION_BEGIN,    ///< A spec, animator or exit entry was registered. detail = LayoutTransitionSlot (0 ENTER, 1 EXIT, 2 CHANGE).
  TRANSITION_END,      ///< The entry finished naturally. detail = slot.
  TRANSITION_TICK,     ///< An animator tick was dispatched. detail = slot; value0/value1 = raw/eased progress.
  TRANSITION_CANCEL    ///< The entry was cancelled or replaced. detail = slot.
};

/// Successful allocations at explicit, known Layout storage sites. These are not
/// process malloc totals; value0 is element count and value1 is requested bytes.
enum class StorageSite : uint32_t
{
  VIEW_DEFAULT_MEASURE_CHILDREN = 1,
  VIEW_REPLAY_CHILDREN          = 2,
  VIEW_DEFAULT_ARRANGE_CHILDREN = 3,
  ABSOLUTE_MEASURE_CHILDREN     = 10,
  ABSOLUTE_ARRANGE_CHILDREN     = 11,
  STACK_MEASURE_CHILDREN        = 20,
  STACK_ARRANGE_CHILDREN        = 21,
  STACK_MEASURE_WORK            = 22,
  STACK_ARRANGE_WORK            = 23,
  FLEX_MEASURE_CHILDREN         = 30,
  FLEX_ARRANGE_CHILDREN         = 31,
  FLEX_ARRANGE_WORK             = 32,
  GRID_MEASURE_CHILDREN         = 40,
  GRID_ARRANGE_CHILDREN         = 41,
  GRID_MEASURE_ROWS             = 42,
  GRID_MEASURE_COLUMNS          = 43,
  TRANSITION_TICK_DISPATCH      = 50
};

/// One recorded observation. `epoch` is the caller's tag passed to BeginCapture; `sequence` is
/// the 1-based global ordinal of the event within the capture and keeps counting when the
/// buffer is full (compare CaptureResult::totalEvents with CaptureResult::count).
struct Event
{
  uint64_t  epoch{0};
  uint64_t  sequence{0};
  EventKind kind{EventKind::MEASURE_ENTER};
  uint32_t  nodeId{0}; ///< 0 means a global event or a view not registered by the TC.
  uint32_t  relatedNodeId{0};
  uintptr_t windowToken{0}; ///< Opaque identity; compare with WindowSnapshot::windowToken.
  uint32_t  generation{0};
  uint32_t  detail{0}; ///< ANCESTOR_STOP: 1 owner, 2 measuring, 3 direct arranger, 4 standalone.
  float     value0{0};
  float     value1{0};
};

/// Capture status. `overflow` is raised by a full buffer, by a registry change while the
/// capture was active, or by an exhausted window registry; EndCapture always reports
/// `active == false`.
struct CaptureResult
{
  std::size_t count{0};
  std::size_t capacity{0};
  uint64_t    epoch{0};
  uint64_t    totalEvents{0};
  bool        overflow{false};
  bool        active{false};
};

static_assert(std::is_trivially_copyable<Event>::value, "Capture events must not own storage");
static_assert(std::is_trivially_copyable<CaptureResult>::value, "Capture status must not own storage");

struct ViewSnapshot
{
  bool            valid{false};
  uint32_t        nodeId{0};
  uint32_t        generation{0};
  uint32_t        measurePropagationGeneration{0};
  uint32_t        arrangePropagationGeneration{0};
  uint32_t        dependencyOwnerId{0};
  uint32_t        dependencyOwnerKind{0}; ///< 0 none, 1 arrange, 2 recycler.
  bool            measureCacheValid{false};
  bool            arrangeCacheValid{false};
  bool            measureDirty{false};
  bool            arrangeDirty{false};
  bool            effectiveScaleValid{false};
  bool            effectiveScaleActorSynced{false};
  bool            measureInProgress{false};
  bool            arrangeInProgress{false};
  bool            replayInProgress{false};
  bool            measurePoisoned{false};
  bool            arrangePoisoned{false};
  bool            arrangePublishBlocked{false};
  bool            measuredSlotUnconsumed{false};
  bool            arrangedResultAvailable{false};
  bool            arrangesIfChanged{false};
  bool            processing{false};
  bool            completionEmitting{false};
  float           measureScaleKey{0}; ///< Meaningful only when measureCacheValid.
  float           effectiveScale{0};  ///< Meaningful only when effectiveScaleValid.
  float           actorEffectiveScale{0};
  Property::Index effectiveScalePropertyIndex{Property::INVALID_INDEX};
  MeasuredSize    measuredSize{};
  MeasuredSize    normalizedConstraint{};
  LayoutRect      arrangedBounds{}; ///< Parent-local logical result, before RTL.
  LayoutRect      arrangeInput{};   ///< Meaningful only when arrangeCacheValid.
};

struct WindowSnapshot
{
  uintptr_t   windowToken{0};
  bool        valid{false};
  bool        destroyPending{false};
  bool        wakeArmed{false};
  bool        dirtySinceEmit{false};
  bool        manualProcessing{false};
  uint32_t    processDepth{0};
  uint32_t    generation{0};
  std::size_t pendingRoots{0};
  std::size_t registeredRoots{0};
  std::size_t pendingCompletions{0};
  std::size_t activeSpecs{0};
  std::size_t activeAnimators{0};
  std::size_t pendingExits{0};
};

struct TextSnapshot
{
  bool     valid{false};
  bool     ready{false};
  bool     firstLineAvailable{false};
  uint32_t lineCount{0};
  uint32_t glyphCount{0};
  uint32_t firstGlyphFontId{0};
  uint32_t firstGlyphIndex{0};
  float    firstGlyphAdvance{0};
  float    layoutWidth{0};
  float    layoutHeight{0};
  float    controlWidth{0};
  float    controlHeight{0};
  float    firstAscender{0};
  float    firstDescender{0};
  float    firstLineWidth{0};
  float    firstLineSpacing{0};
  float    renderedOffsetX{0};
  float    renderedOffsetY{0};
  float    firstBaseline{0}; ///< Local visual baseline of the first line.
};

/// Transition state of one view. Ownership (ownerId/role) and the from/to/cause/duration/delay
/// values are captured when the entry was started; nodeId/parentId are refreshed live. When a
/// view holds several entries at once the animator entry takes precedence over the spec entry,
/// which takes precedence over a pending exit. For a view without any entry, ownerId/role
/// describe the transition that would govern an ENTER of the view. `slot` and `cause` are
/// meaningful only while an entry is active (0 collides with ENTER / SIBLING_ADDED).
struct TransitionSnapshot
{
  bool       valid{false};
  bool       specActive{false};
  bool       animatorActive{false};
  bool       exitActive{false};
  bool       freshAnimator{false};
  bool       animatorFinished{false};
  uint32_t   nodeId{0};
  uint32_t   ownerId{0};
  uint32_t   role{0};
  bool       savedInteractionAvailable{false};
  bool       savedSensitive{false};
  bool       savedKeyboardFocusable{false};
  bool       savedTouchFocusable{false};
  bool       savedClipAvailable{false};
  int        savedClip{0};
  int        currentClip{0};
  uint32_t   parentId{0};
  uint32_t   slot{0};
  uint32_t   cause{0};
  float      elapsed{0};
  float      duration{0};
  float      delay{0};
  LayoutRect from{};
  LayoutRect to{};
  LayoutRect lastLerped{};
};

/// All functions and hooks are event-thread only and unsynchronised. Register nodes before
/// BeginCapture (ids must be unique and nonzero; registration during an active capture is
/// refused); the caller keeps `buffer` alive until EndCapture. At most 8192 nodes and 64
/// windows are tracked.
DALI_UI_API bool               RegisterNode(Ui::View view, uint32_t id);
DALI_UI_API void               ClearRegisteredNodes();
DALI_UI_API bool               BeginCapture(Event* buffer, std::size_t capacity, uint64_t epoch = 1u);
DALI_UI_API CaptureResult      EndCapture();
DALI_UI_API CaptureResult      GetCaptureStatus();
DALI_UI_API ViewSnapshot       GetViewSnapshot(Ui::View view);
DALI_UI_API WindowSnapshot     GetWindowSnapshot(Window window);
DALI_UI_API TextSnapshot       GetTextSnapshot(Ui::Label label);
DALI_UI_API TransitionSnapshot GetTransitionSnapshot(Ui::View view);
DALI_UI_API Animation          GetSpecAnimation(Ui::View view);
/// Advances the existing animator update path by the supplied delta; retains its cap and first-tick rules.
DALI_UI_API bool TickAnimatorsForTesting(Window window, float elapsedSeconds);
/// Prevent automatic ticks while manually advancing one window's animators. Always restore on TC exit.
DALI_UI_API bool SetManualAnimatorTicks(Window window, bool enabled);
} // namespace LayoutTestDiagnostics
} // namespace Integration
} // namespace Ui
} // namespace DALI_NAMESPACE
