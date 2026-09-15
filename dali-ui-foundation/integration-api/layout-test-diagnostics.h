#pragma once

/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

// This interface exists only in explicitly instrumented test builds.
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
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
enum class EventKind : uint16_t
{
  MEASURE_ENTER,
  MEASURE_HIT,
  MEASURE_PRODUCER,
  MEASURE_PUBLISH,
  ARRANGE_ENTER,
  ARRANGE_HIT,
  ARRANGE_PRODUCER,
  ARRANGE_PUBLISH,
  REPLAY_VISIT,
  GEOMETRY_WRITE,
  INVALIDATE_MEASURE,
  INVALIDATE_ARRANGE,
  ANCESTOR_VISIT,
  ANCESTOR_STOP,
  GENERATION_ADVANCE,
  OWNER_PUSH,
  OWNER_POP,
  ROOT_QUEUED,
  ROOT_DRAIN,
  WAKE_REQUEST,
  PARKED_REQUEST,
  PROCESS_BEGIN,
  PROCESS_END,
  COMPLETION,
  ROLLBACK_ROOT,
  SNAPSHOT_ALLOCATION,
  TRANSITION_BEGIN,
  TRANSITION_END,
  TRANSITION_TICK,
  TRANSITION_CANCEL
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
  bool        detached{false};
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

/// Event-thread-only. Register before BeginCapture; IDs must be unique and nonzero.
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
#endif
