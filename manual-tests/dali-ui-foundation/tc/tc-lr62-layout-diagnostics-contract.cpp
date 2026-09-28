/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include <array>
#include <limits>
#include <tuple>
#include "layout-validation-transition-fixtures.h"

using namespace LayoutValidation;
namespace TF = LayoutValidation::TransitionFixtures;

namespace
{
namespace D = Dali::Ui::Integration::LayoutTestDiagnostics;

void ResetCapture()
{
  D::EndCapture();
  D::ClearRegisteredNodes();
}

void Prepare(Run& run)
{
  ResetCapture();
  run.OnCleanup([]
  { ResetCapture(); });
}

bool SameRect(const LayoutRect& a, const LayoutRect& b)
{
  return std::tie(a.x, a.y, a.width, a.height) == std::tie(b.x, b.y, b.width, b.height);
}

bool SameView(const D::ViewSnapshot& a, const D::ViewSnapshot& b)
{
#define VIEW_FIELDS(v) std::tie(v.valid, v.nodeId, v.generation, v.measurePropagationGeneration, v.arrangePropagationGeneration, v.dependencyOwnerId, v.dependencyOwnerKind, v.measureCacheValid, v.arrangeCacheValid, v.measureDirty, v.arrangeDirty, v.effectiveScaleValid, v.effectiveScaleActorSynced, v.measureInProgress, v.arrangeInProgress, v.replayInProgress, v.measurePoisoned, v.arrangePoisoned, v.arrangePublishBlocked, v.measuredSlotUnconsumed, v.arrangedResultAvailable, v.arrangesIfChanged, v.processing, v.completionEmitting, v.measureScaleKey, v.effectiveScale, v.actorEffectiveScale, v.effectiveScalePropertyIndex, v.measuredSize.width, v.measuredSize.height, v.normalizedConstraint.width, v.normalizedConstraint.height)
  const bool same = VIEW_FIELDS(a) == VIEW_FIELDS(b) && SameRect(a.arrangedBounds, b.arrangedBounds) && SameRect(a.arrangeInput, b.arrangeInput);
#undef VIEW_FIELDS
  return same;
}

bool SameWindow(const D::WindowSnapshot& a, const D::WindowSnapshot& b)
{
#define WINDOW_FIELDS(v) std::tie(v.windowToken, v.valid, v.destroyPending, v.wakeArmed, v.dirtySinceEmit, v.manualProcessing, v.processDepth, v.generation, v.pendingRoots, v.registeredRoots, v.pendingCompletions, v.activeSpecs, v.activeAnimators, v.pendingExits)
  const bool same = WINDOW_FIELDS(a) == WINDOW_FIELDS(b);
#undef WINDOW_FIELDS
  return same;
}

bool SameText(const D::TextSnapshot& a, const D::TextSnapshot& b)
{
#define TEXT_FIELDS(v) std::tie(v.valid, v.ready, v.firstLineAvailable, v.lineCount, v.glyphCount, v.firstGlyphFontId, v.firstGlyphIndex, v.firstGlyphAdvance, v.layoutWidth, v.layoutHeight, v.controlWidth, v.controlHeight, v.firstAscender, v.firstDescender, v.firstLineWidth, v.firstLineSpacing, v.renderedOffsetX, v.renderedOffsetY, v.firstBaseline)
  const bool same = TEXT_FIELDS(a) == TEXT_FIELDS(b);
#undef TEXT_FIELDS
  return same;
}

bool SameTransition(const D::TransitionSnapshot& a, const D::TransitionSnapshot& b)
{
#define TRANSITION_FIELDS(v) std::tie(v.valid, v.specActive, v.animatorActive, v.exitActive, v.freshAnimator, v.animatorFinished, v.nodeId, v.ownerId, v.role, v.savedInteractionAvailable, v.savedSensitive, v.savedKeyboardFocusable, v.savedTouchFocusable, v.savedClipAvailable, v.savedClip, v.currentClip, v.parentId, v.slot, v.cause, v.elapsed, v.duration, v.delay)
  const bool same = TRANSITION_FIELDS(a) == TRANSITION_FIELDS(b) && SameRect(a.from, b.from) && SameRect(a.to, b.to) && SameRect(a.lastLerped, b.lastLerped);
#undef TRANSITION_FIELDS
  return same;
}

bool SameCapture(const D::CaptureResult& a, const D::CaptureResult& b)
{
  return std::tie(a.count, a.capacity, a.epoch, a.totalEvents, a.overflow, a.active) == std::tie(b.count, b.capacity, b.epoch, b.totalEvents, b.overflow, b.active);
}

// Every public snapshot field is compared with an independently constructed expectation.
// Each invocation emits 34 rows: 22 scalar fields and three four-component rectangles.
void CheckTransitionFields(Run& run, const char* prefix, const D::TransitionSnapshot& actual, const D::TransitionSnapshot& expected)
{
  const std::string key = std::string(prefix) + ".";
#define CHECK_TRANSITION_FIELD(field) run.Equal((key + #field).c_str(), actual.field, expected.field)
  CHECK_TRANSITION_FIELD(valid);
  CHECK_TRANSITION_FIELD(specActive);
  CHECK_TRANSITION_FIELD(animatorActive);
  CHECK_TRANSITION_FIELD(exitActive);
  CHECK_TRANSITION_FIELD(freshAnimator);
  CHECK_TRANSITION_FIELD(animatorFinished);
  CHECK_TRANSITION_FIELD(nodeId);
  CHECK_TRANSITION_FIELD(ownerId);
  CHECK_TRANSITION_FIELD(role);
  CHECK_TRANSITION_FIELD(savedInteractionAvailable);
  CHECK_TRANSITION_FIELD(savedSensitive);
  CHECK_TRANSITION_FIELD(savedKeyboardFocusable);
  CHECK_TRANSITION_FIELD(savedTouchFocusable);
  CHECK_TRANSITION_FIELD(savedClipAvailable);
  CHECK_TRANSITION_FIELD(savedClip);
  CHECK_TRANSITION_FIELD(currentClip);
  CHECK_TRANSITION_FIELD(parentId);
  CHECK_TRANSITION_FIELD(slot);
  CHECK_TRANSITION_FIELD(cause);
#undef CHECK_TRANSITION_FIELD
  run.Near((key + "elapsed").c_str(), actual.elapsed, expected.elapsed, 0);
  run.Near((key + "duration").c_str(), actual.duration, expected.duration, 0);
  run.Near((key + "delay").c_str(), actual.delay, expected.delay, 0);
  run.Rect((key + "from").c_str(), actual.from, expected.from);
  run.Rect((key + "to").c_str(), actual.to, expected.to);
  run.Rect((key + "lastLerped").c_str(), actual.lastLerped, expected.lastLerped);
}

Scenario ActiveSnapshotScenario(bool animator, LayoutTransitionSlot slot, bool registeredAtStart)
{
  const char*       slotName = slot == LayoutTransitionSlot::CHANGE ? "change" : slot == LayoutTransitionSlot::ENTER ? "enter"
                                                                                                                     : "exit";
  const std::string id       = std::string("D07.") + (animator ? "animator-" : "spec-") + slotName + (registeredAtStart ? "-registered" : "-unregistered");
  Scenario          scenario{id, "Compact historical state survives late registration, clearing and identity replacement", {}};
  scenario.steps.push_back({"mount", 4, [](Run& r)
  {
    Prepare(r);
    auto s = TF::NewState(r);
    s->root.Add(s->child);
    // HUD invalidation must not replay ENTER while the diagnostic state is being read.
    r.AttachToWindow(s->root);
    r.AfterLayout({s->child}, [s](Run& v)
    { v.Rect("snapshot.mount", v.Snapshot(s->child), {40, 30, 80, 40}); });
  }});
  scenario.steps.push_back({"historical-fields", 110, [animator, slot, registeredAtStart](Run& r)
  {
    auto s = r.State<TF::State>();
    r.Truth("snapshot.manual-clock", D::SetManualAnimatorTicks(s->window, true));
    bool registered = true;
    if(registeredAtStart)
    {
      const bool owner = D::RegisterNode(s->root, 11);
      const bool child = D::RegisterNode(s->child, 12);
      registered       = owner && child;
    }
    r.Truth("snapshot.initial-registration", registered);
    const auto idle = D::GetCaptureStatus();
    r.Truth("snapshot.capture-inactive", !idle.active);
    if(slot == LayoutTransitionSlot::ENTER) s->root.Remove(s->child, RemovePolicy::IMMEDIATE);
    s->child.SetProperty(Actor::Property::SENSITIVE, true);
    s->child.SetProperty(Actor::Property::FOCUSABLE, true);
    s->child.SetProperty(Actor::Property::FOCUS_ON_TOUCH, false);
    s->child.SetProperty(Actor::Property::CLIPPING_MODE, ClippingMode::DISABLED);
    s->Bind(LayoutTransition::New());
    s->transition.ClearChangeTiming();
    if(animator)
    {
      auto       callback = LayoutAnimatorCallback::New(s.get(), &TF::State::OnAnimator);
      const auto timing   = TF::AnimatorTiming(2.f, .25f);
      if(slot == LayoutTransitionSlot::CHANGE)
        s->transition.SetChangeAnimator(std::move(callback), timing);
      else if(slot == LayoutTransitionSlot::ENTER)
        s->transition.SetEnterAnimator(std::move(callback), timing);
      else
        s->transition.SetExitAnimator(std::move(callback), timing);
    }
    else if(slot == LayoutTransitionSlot::CHANGE)
    {
      s->transition.SetChangeTiming(TF::Timing(2.f, .25f));
    }
    else
    {
      auto effect = LayoutBoundsEffects::SlideFrom(LayoutBoundsEdge::LEFT, LayoutBoundsLength::Pixel(20), TF::Timing(2.f, .25f));
      effect.SetClipMode(LayoutBoundsClipMode::CLIP_TO_BOUNDING_BOX);
      if(slot == LayoutTransitionSlot::ENTER)
        s->transition.SetEnterBoundsEffect(effect);
      else
        s->transition.SetExitBoundsEffect(effect);
    }
    s->root.SetLayoutTransition(s->transition);
    if(slot == LayoutTransitionSlot::CHANGE)
      TF::SetFixtureBounds(s->child, {140, 30, 80, 40});
    else if(slot == LayoutTransitionSlot::ENTER)
      s->root.Add(s->child);
    else
      s->root.Remove(s->child, RemovePolicy::ANIMATE_EXIT);
    LayoutController::Get(s->window).ProcessLayouts();

    D::TransitionSnapshot expected;
    expected.valid          = true;
    expected.specActive     = !animator;
    expected.animatorActive = animator;
    expected.exitActive     = slot == LayoutTransitionSlot::EXIT;
    expected.freshAnimator  = animator;
    expected.nodeId         = registeredAtStart ? 12u : 0u;
    expected.parentId       = registeredAtStart ? 11u : 0u;
    expected.ownerId        = registeredAtStart ? 11u : 0u;
    expected.role           = registeredAtStart ? 2u : 0u; // DIRECT_PARENT, or no observed ownership at start.
    expected.slot           = static_cast<uint32_t>(slot);
    expected.cause          = animator || slot == LayoutTransitionSlot::CHANGE ? static_cast<uint32_t>(LayoutChangeCause::OTHER) : 0u;
    expected.duration       = animator || slot == LayoutTransitionSlot::CHANGE ? 2.f : 2.25f;
    expected.delay          = animator || slot == LayoutTransitionSlot::CHANGE ? .25f : 0.f;
    expected.from           = {40, 30, 80, 40};
    expected.to             = slot == LayoutTransitionSlot::CHANGE ? LayoutRect(140, 30, 80, 40) : expected.from;
    if(animator) expected.lastLerped = expected.from;
    if(!animator && slot == LayoutTransitionSlot::ENTER) expected.from.x = 20;
    if(!animator && slot == LayoutTransitionSlot::EXIT) expected.to.x = 20;
    expected.savedInteractionAvailable = expected.exitActive;
    expected.savedSensitive            = expected.exitActive;
    expected.savedKeyboardFocusable    = expected.exitActive;
    expected.savedClipAvailable        = !animator && slot != LayoutTransitionSlot::CHANGE;
    expected.currentClip               = expected.savedClipAvailable ? ClippingMode::CLIP_TO_BOUNDING_BOX : ClippingMode::DISABLED;

    CheckTransitionFields(r, "snapshot.started", D::GetTransitionSnapshot(s->child), expected);
    r.Truth("snapshot.animation-channel", static_cast<bool>(D::GetSpecAnimation(s->child)) == !animator);
    D::ClearRegisteredNodes();
    expected.nodeId = expected.parentId = 0;
    CheckTransitionFields(r, "snapshot.cleared", D::GetTransitionSnapshot(s->child), expected);
    r.Truth("snapshot.owner-rebound", D::RegisterNode(s->root, 201));
    r.Truth("snapshot.child-rebound", D::RegisterNode(s->child, 202));
    expected.nodeId   = 202;
    expected.parentId = 201;
    CheckTransitionFields(r, "snapshot.rebound", D::GetTransitionSnapshot(s->child), expected);
    const auto frozen    = D::GetTransitionSnapshot(s->child);
    bool       unchanged = true;
    for(unsigned i = 0; i < 100; ++i) unchanged &= SameTransition(frozen, D::GetTransitionSnapshot(s->child));
    r.Truth("snapshot.repeated-read-stable", unchanged);
    r.Truth("snapshot.capture-remains-inert", SameCapture(idle, D::GetCaptureStatus()));
  }});
  return scenario;
}

struct GuardedEvents
{
  static constexpr uint64_t CANARY = 0xA7316E49D502B8CFull;
  std::array<uint64_t, 4>   before{{CANARY, CANARY, CANARY, CANARY}};
  std::array<D::Event, 2>   events{};
  std::array<uint64_t, 4>   after{{CANARY, CANARY, CANARY, CANARY}};
  bool                      Intact() const
  {
    return std::all_of(before.begin(), before.end(), [](uint64_t value)
    { return value == CANARY; }) &&
           std::all_of(after.begin(), after.end(), [](uint64_t value)
    { return value == CANARY; });
  }
};

static_assert(offsetof(GuardedEvents, after) == offsetof(GuardedEvents, events) + sizeof(GuardedEvents::events), "The trailing canary must touch the capture buffer");

struct ClockObserver
{
  Window   window;
  uint32_t calls{0};
  float    lastRaw{-1};
  bool     nestedRejected{false};
  void     Tick(const LayoutAnimatorContext& context)
  {
    ++calls;
    lastRaw = context.rawProgress;
    if(calls == 1u) nestedRejected = !D::TickAnimatorsForTesting(window, .01f);
  }
};
} // namespace

class TcLr62 : public Case
{
public:
  TcLr62()
  : Case("LR62", "Diagnostics observation contract")
  {
  }

  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> cases;
    // The observation hooks are compiled into every build. This scenario proves both halves
    // of that invariant: the hooks are present and live, and they record nothing at all while
    // no capture is open. It replaces the former D00.profile-off scenario, which asserted the
    // compiled capability was absent - a statement about a build profile that no longer exists.
    cases.push_back(Single("D00.profile-always-on", "Hooks are compiled into every build and stay inert until a capture opens", 8, [](Run& r)
    {
      Prepare(r);
      // Taken before the fixture is built and before any capture is opened here. The counters
      // are only reset by BeginCapture, so this scenario compares against this baseline rather
      // than against zero: that keeps the detector independent of which scenarios ran before.
      const auto idle = D::GetCaptureStatus();
      // Only `active` is an absolute invariant here: `overflow` is one of the counters that
      // only BeginCapture resets, and D03 deliberately sets it, so it is compared as a delta
      // by always-on.pass-inert below rather than asserted to be clear.
      r.Truth("always-on.idle-inactive", !idle.active);
      auto root = AbsoluteLayout::New();
      root.SetRequestedWidth(240);
      root.SetRequestedHeight(100);
      auto leaf = Leaf(37, 19);
      leaf.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds({7, 11, 37, 19}));
      root.Add(leaf);
      r.Equal("always-on.unregistered-node-id", D::GetViewSnapshot(leaf).nodeId, 0);
      r.Attach(root);
      r.AfterLayout({leaf}, [leaf, idle](Run& v)
      {
        // Building and laying out the fixture is a complete layout pass driven with no capture
        // open, so every hook it passed through must have left the capture state untouched.
        const auto view = D::GetViewSnapshot(leaf);
        v.Truth("always-on.fixture-valid", view.valid && D::GetWindowSnapshot(v.GetWindow()).valid);
        v.Truth("always-on.pass-inert", SameCapture(idle, D::GetCaptureStatus()));
        std::array<D::Event, 16> events{};
        v.Truth("always-on.empty-begin", D::BeginCapture(events.data(), events.size(), 6209));
        const auto empty = D::EndCapture();
        v.Truth("always-on.empty-roundtrip", empty.count == 0u && empty.totalEvents == 0u && !empty.overflow && !empty.active && empty.epoch == 6209 && empty.capacity == events.size());
        v.Truth("always-on.live-begin", D::BeginCapture(events.data(), events.size(), 6210));
        View target = leaf;
        target.InvalidateArrange();
        const auto live = D::EndCapture();
        // Proves the branch behind each hook is live and was not dead-stripped. Liveness is
        // exactly "events were produced and at least one was stored"; an overflow of the
        // small buffer above would prove the same thing, so it must not fail the row.
        v.Truth("always-on.live-events", live.totalEvents > 0u && live.count > 0u);
        v.RequireExternalVerification("LR62 also requires the always-on artifact evidence: the diagnostics exports present in the loaded library, an unchanged ViewDataImpl object size versus the pre-change Release artifact, compact transition entries without observation heap allocation, and a single load plus conditional branch at each measure/arrange hook site");
      });
    }));

    cases.push_back(Single("D01.registry", "Explicit unique IDs, rejection and destruction cleanup", 20, [](Run& r)
    {
      Prepare(r);
      auto a = Leaf(37, 19);
      auto b = Leaf(23, 11);
      r.Truth("id.empty-rejected", !D::RegisterNode({}, 101));
      r.Truth("id.zero-rejected", !D::RegisterNode(a, 0));
      r.Truth("id.first-accepted", D::RegisterNode(a, 101));
      r.Truth("id.duplicate-rejected", !D::RegisterNode(b, 101));
      r.Truth("id.same-object-idempotent", D::RegisterNode(a, 101));
      r.Equal("id.a", D::GetViewSnapshot(a).nodeId, 101);
      r.Equal("id.b-unregistered", D::GetViewSnapshot(b).nodeId, 0);
      r.Truth("id.replace", D::RegisterNode(a, 102));
      r.Equal("id.replaced-value", D::GetViewSnapshot(a).nodeId, 102);
      r.Truth("id.released-value-reusable", D::RegisterNode(b, 101));
      std::array<D::Event, 8> events{};
      r.Truth("id.capture-start", D::BeginCapture(events.data(), events.size(), 6201));
      const auto before = D::GetCaptureStatus();
      r.Truth("id.active-change-rejected", !D::RegisterNode(a, 103));
      r.Truth("id.rejection-preserves-capture", SameCapture(before, D::GetCaptureStatus()));
      D::EndCapture();
      r.Equal("id.rejection-preserves-id", D::GetViewSnapshot(a).nodeId, 102);
      {
        auto temporary = Leaf(3, 4);
        r.Truth("id.temporary", D::RegisterNode(temporary, 103));
      }
      auto replacement = Leaf(5, 6);
      r.Truth("id.destroyed-registration-released", D::RegisterNode(replacement, 103));
      D::ClearRegisteredNodes();
      r.Equal("id.clear-a", D::GetViewSnapshot(a).nodeId, 0);
      r.Equal("id.clear-b", D::GetViewSnapshot(b).nodeId, 0);
      r.Equal("id.clear-replacement", D::GetViewSnapshot(replacement).nodeId, 0);
      r.Truth("id.ended", !D::GetCaptureStatus().active);
    }));

    cases.push_back(Single("D01.registry-capacity", "Bounded registry saturation, identity replacement and released slot reuse", 20, [](Run& r)
    {
      Prepare(r);
      constexpr uint32_t capacity = 8192;
      constexpr uint32_t removed  = capacity / 2;
      std::vector<View>  nodes;
      nodes.reserve(capacity);
      for(uint32_t i = 0; i < capacity; ++i) nodes.push_back(Leaf(1, 1));
      auto extra = Leaf(1, 1), spare = Leaf(1, 1);
      bool accepted = true;
      for(uint32_t i = 0; i < capacity; ++i) accepted = D::RegisterNode(nodes[i], i + 1) && accepted;
      bool exact = true;
      for(uint32_t i = 0; i < capacity; ++i) exact &= D::GetViewSnapshot(nodes[i]).nodeId == i + 1;
      r.Truth("capacity.fill-all-accepted", accepted);
      r.Truth("capacity.ids-exact", exact);
      r.Truth("capacity.overflow-rejected", !D::RegisterNode(extra, capacity + 1));
      r.Equal("capacity.overflow-extra-unregistered", D::GetViewSnapshot(extra).nodeId, 0);
      r.Truth("capacity.full-idempotent", D::RegisterNode(nodes[0], 1));
      r.Truth("capacity.full-rebind", D::RegisterNode(nodes[0], 90001));
      r.Equal("capacity.full-rebind-id", D::GetViewSnapshot(nodes[0]).nodeId, 90001);
      r.Truth("capacity.full-duplicate-rejected", !D::RegisterNode(extra, 90001));
      r.Truth("capacity.released-id-alone-no-slot", !D::RegisterNode(extra, 1));
      // The registry holds raw identities; dropping the last handle must free its slot.
      nodes[removed].Reset();
      r.Truth("capacity.destroyed-slot-reusable", D::RegisterNode(extra, removed + 1));
      r.Equal("capacity.reused-slot-id", D::GetViewSnapshot(extra).nodeId, removed + 1);
      r.Truth("capacity.full-again", !D::RegisterNode(spare, capacity + 2));
      nodes[removed] = extra;
      exact          = true;
      for(uint32_t i = 0; i < capacity; ++i) exact &= D::GetViewSnapshot(nodes[i]).nodeId == (i == 0 ? 90001 : i + 1);
      r.Truth("capacity.unaffected-identities", exact);
      D::ClearRegisteredNodes();
      exact = true;
      for(const auto& node : nodes) exact &= D::GetViewSnapshot(node).nodeId == 0;
      r.Truth("capacity.clear-releases-all", exact);
      accepted = true;
      for(uint32_t i = 0; i < capacity; ++i) accepted = D::RegisterNode(nodes[i], 20000 + i) && accepted;
      r.Truth("capacity.refill-all-accepted", accepted);
      exact = true;
      for(uint32_t i = 0; i < capacity; ++i) exact &= D::GetViewSnapshot(nodes[i]).nodeId == 20000 + i;
      r.Truth("capacity.refill-identities", exact);
      r.Truth("capacity.refill-full", !D::RegisterNode(spare, 90002));
      D::ClearRegisteredNodes();
      nodes.clear();
      extra.Reset();
      r.Truth("capacity.final-slot-reusable", D::RegisterNode(spare, 6209));
      r.Equal("capacity.final-id", D::GetViewSnapshot(spare).nodeId, 6209);
      D::ClearRegisteredNodes();
      r.Truth("capacity.final-clear", D::GetViewSnapshot(spare).nodeId == 0 && !D::GetCaptureStatus().active);
    }));

    cases.push_back(Single("D02.capture-validation", "Invalid arguments and nested capture cannot redirect storage", 17, [](Run& r)
    {
      Prepare(r);
      auto                     leaf = Leaf(37, 19);
      std::array<D::Event, 32> first{};
      std::array<D::Event, 4>  second{};
      second[0].epoch = 0xAABBCCDDu;
      r.Truth("capture.null-rejected", !D::BeginCapture(nullptr, first.size(), 1));
      r.Truth("capture.zero-capacity-rejected", !D::BeginCapture(first.data(), 0, 1));
      r.Truth("capture.zero-epoch-rejected", !D::BeginCapture(first.data(), first.size(), 0));
      r.Truth("capture.invalid-remains-inactive", !D::GetCaptureStatus().active);
      r.Truth("capture.begin", D::BeginCapture(first.data(), first.size(), 6202));
      const auto initial = D::GetCaptureStatus();
      r.Equal("capture.initial-count", initial.count, 0);
      r.Equal("capture.initial-total", initial.totalEvents, 0);
      r.Truth("capture.initial-active", initial.active && !initial.overflow);
      r.Truth("capture.nested-rejected", !D::BeginCapture(second.data(), second.size(), 888));
      r.Truth("capture.nested-state-preserved", SameCapture(initial, D::GetCaptureStatus()));
      leaf.InvalidateArrange();
      const auto ended = D::EndCapture();
      r.Truth("capture.recorded", ended.count > 0 && ended.count == ended.totalEvents);
      r.Truth("capture.ended-without-overflow", !ended.active && !ended.overflow);
      r.Equal("capture.original-capacity", ended.capacity, first.size());
      r.Equal("capture.original-epoch", ended.epoch, 6202);
      r.Equal("capture.first-sequence", first[0].sequence, 1);
      r.Equal("capture.first-epoch", first[0].epoch, 6202);
      r.Equal("capture.nested-buffer-untouched", second[0].epoch, 0xAABBCCDDu);
    }));

    cases.push_back(Single("D03.overflow-reset", "Guard bytes survive overflow; a new epoch resets all capture counters", 23, [](Run& r)
    {
      Prepare(r);
      auto          leaf = Leaf(37, 19);
      GuardedEvents buffer;
      r.Truth("overflow.begin", D::BeginCapture(buffer.events.data(), buffer.events.size(), 6203));
      for(unsigned i = 0; i < 8; ++i) leaf.InvalidateArrange();
      const auto ended = D::EndCapture();
      r.Truth("overflow.flag", ended.overflow);
      r.Truth("overflow.canaries", buffer.Intact());
      r.Equal("overflow.capacity", ended.capacity, 2);
      r.Equal("overflow.stored-count", ended.count, 2);
      r.Truth("overflow.total-not-truncated", ended.totalEvents >= 8 && ended.totalEvents > ended.count);
      r.Equal("overflow.sequence-first", buffer.events[0].sequence, 1);
      r.Equal("overflow.sequence-second", buffer.events[1].sequence, 2);
      r.Equal("overflow.epoch-first", buffer.events[0].epoch, 6203);
      r.Equal("overflow.epoch-second", buffer.events[1].epoch, 6203);
      leaf.InvalidateArrange();
      r.Truth("overflow.end-stops-recording", SameCapture(ended, D::GetCaptureStatus()));
      r.Truth("overflow.end-idempotent", SameCapture(ended, D::EndCapture()));
      std::array<D::Event, 32> next{};
      r.Truth("reset.begin", D::BeginCapture(next.data(), next.size(), 6204));
      const auto reset = D::GetCaptureStatus();
      r.Equal("reset.count", reset.count, 0);
      r.Equal("reset.total", reset.totalEvents, 0);
      r.Equal("reset.epoch", reset.epoch, 6204);
      r.Truth("reset.overflow-cleared", !reset.overflow && reset.active);
      leaf.InvalidateArrange();
      const auto nextEnd = D::EndCapture();
      r.Truth("reset.recorded", nextEnd.count > 0 && !nextEnd.overflow);
      r.Equal("reset.sequence", next[0].sequence, 1);
      r.Equal("reset.event-epoch", next[0].epoch, 6204);
      r.Truth("reset.begin-empty", D::BeginCapture(next.data(), next.size(), 6205));
      const auto empty = D::EndCapture();
      r.Truth("reset.empty-status", empty.count == 0 && empty.totalEvents == 0 && !empty.active && !empty.overflow && empty.epoch == 6205);
      r.Truth("reset.original-canaries", buffer.Intact());
    }));

    cases.push_back(Single("D03.active-registry-clear", "Clearing registration during capture must fail visibly and preserve IDs", 7, [](Run& r)
    {
      Prepare(r);
      auto                    leaf = Leaf(37, 19);
      std::array<D::Event, 4> events{};
      r.Truth("active-clear.register", D::RegisterNode(leaf, 6208));
      r.Truth("active-clear.begin", D::BeginCapture(events.data(), events.size(), 6208));
      D::ClearRegisteredNodes();
      const auto rejected = D::GetCaptureStatus();
      r.Truth("active-clear.error-visible", rejected.active && rejected.overflow);
      r.Truth("active-clear.no-invented-events", rejected.count == 0 && rejected.totalEvents == 0);
      r.Equal("active-clear.id-preserved", D::GetViewSnapshot(leaf).nodeId, 6208);
      r.Truth("active-clear.end-preserves-error", D::EndCapture().overflow);
      D::ClearRegisteredNodes();
      r.Equal("active-clear.inactive-clear", D::GetViewSnapshot(leaf).nodeId, 0);
    }));

    cases.push_back(Single("D04.invalid-readers", "Invalid handles do not create controllers or change capture state", 12, [](Run& r)
    {
      Prepare(r);
      auto                     detached = Leaf(37, 19);
      std::array<D::Event, 32> events{};
      r.Truth("invalid.begin", D::BeginCapture(events.data(), events.size(), 6206));
      const auto before             = D::GetCaptureStatus();
      const bool emptyView          = D::GetViewSnapshot({}).valid;
      const bool emptyWindow        = D::GetWindowSnapshot({}).valid;
      const bool emptyText          = D::GetTextSnapshot({}).valid;
      const bool emptyTransition    = D::GetTransitionSnapshot({}).valid;
      const bool emptyAnimation     = static_cast<bool>(D::GetSpecAnimation({}));
      const bool detachedTransition = D::GetTransitionSnapshot(detached).valid;
      const bool detachedAnimation  = static_cast<bool>(D::GetSpecAnimation(detached));
      const bool invalidTick        = D::TickAnimatorsForTesting({}, 0);
      const bool invalidManual      = D::SetManualAnimatorTicks({}, true);
      const auto after              = D::GetCaptureStatus();
      D::EndCapture();
      r.Truth("invalid.view", !emptyView);
      r.Truth("invalid.window", !emptyWindow);
      r.Truth("invalid.text", !emptyText);
      r.Truth("invalid.transition", !emptyTransition);
      r.Truth("invalid.animation", !emptyAnimation);
      r.Truth("invalid.detached-transition", !detachedTransition);
      r.Truth("invalid.detached-animation", !detachedAnimation);
      r.Truth("invalid.tick", !invalidTick);
      r.Truth("invalid.manual-mode", !invalidManual);
      r.Truth("invalid.capture-state", SameCapture(before, after));
      r.Equal("invalid.events", after.totalEvents, 0);
    }));

    cases.push_back(Single("D05.readonly-snapshots", "Repeated completed-model reads preserve geometry, caches, generation and capture", 15, [](Run& r)
    {
      Prepare(r);
      auto root = AbsoluteLayout::New();
      root.SetRequestedWidth(240);
      root.SetRequestedHeight(100);
      auto leaf = Leaf(37, 19);
      leaf.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds({7, 11, 37, 19}));
      auto label = Label::New("Hgj");
      label.SetFontSize(16);
      label.SetLayoutParams(AbsoluteLayoutParams::New().SetBounds({70, 5, 150, 70}));
      root.Add(leaf);
      root.Add(label);
      // Both registrations are evaluated: && would short-circuit the second one, leaving the
      // label unregistered whenever the leaf was refused.
      const bool leafRegistered  = D::RegisterNode(leaf, 6201);
      const bool labelRegistered = D::RegisterNode(label, 6202);
      const bool registered      = leafRegistered && labelRegistered;
      r.Attach(root);
      r.AfterLayout({leaf, label}, [leaf, label, registered](Run& v)
      {
        const auto               view         = D::GetViewSnapshot(leaf);
        const auto               textView     = D::GetViewSnapshot(label);
        const auto               window       = D::GetWindowSnapshot(v.GetWindow());
        const auto               text         = D::GetTextSnapshot(label);
        const auto               transition   = D::GetTransitionSnapshot(leaf);
        const auto               geometry     = LayoutValidation::Bounds(leaf);
        const auto               textGeometry = LayoutValidation::Bounds(label);
        std::array<D::Event, 16> events{};
        const bool               started    = D::BeginCapture(events.data(), events.size(), 6207);
        bool                     stableView = true, stableTextView = true, stableWindow = true, stableText = true, stableTransition = true, noAnimation = true;
        for(unsigned i = 0; i < 1000; ++i)
        {
          stableView &= SameView(view, D::GetViewSnapshot(leaf));
          stableTextView &= SameView(textView, D::GetViewSnapshot(label));
          stableWindow &= SameWindow(window, D::GetWindowSnapshot(v.GetWindow()));
          stableText &= SameText(text, D::GetTextSnapshot(label));
          stableTransition &= SameTransition(transition, D::GetTransitionSnapshot(leaf));
          noAnimation &= !D::GetSpecAnimation(leaf);
        }
        const auto captured       = D::EndCapture();
        bool       afterEndStable = true;
        for(unsigned i = 0; i < 1000; ++i)
        {
          afterEndStable &= SameView(view, D::GetViewSnapshot(leaf)) && SameText(text, D::GetTextSnapshot(label)) && SameWindow(window, D::GetWindowSnapshot(v.GetWindow()));
        }
        v.Truth("readonly.registered", registered);
        v.Truth("readonly.valid-fixture", view.valid && textView.valid && window.valid && transition.valid);
        v.Truth("readonly.text-ready", text.valid && text.ready && text.firstLineAvailable && text.lineCount > 0 && text.glyphCount > 0 && std::isfinite(text.firstBaseline));
        v.Truth("readonly.capture", started);
        v.Truth("readonly.view-fields", stableView);
        v.Truth("readonly.text-view-fields", stableTextView);
        v.Truth("readonly.window-fields", stableWindow);
        v.Truth("readonly.text-fields", stableText);
        v.Truth("readonly.transition-fields", stableTransition);
        v.Truth("readonly.animation-absent", noAnimation);
        v.Truth("readonly.capture-empty", captured.count == 0 && captured.totalEvents == 0 && !captured.overflow && !captured.active);
        v.Truth("readonly.leaf-geometry", SameRect(geometry, LayoutValidation::Bounds(leaf)));
        v.Truth("readonly.label-geometry", SameRect(textGeometry, LayoutValidation::Bounds(label)));
        v.Truth("readonly.after-end-fields", afterEndStable);
        v.Truth("readonly.after-end-capture", SameCapture(captured, D::GetCaptureStatus()));
      });
    }));

    cases.push_back({"D06.clock-restoration", "Reject nested and invalid ticks; restore the actual timer", {{"mount", 4, [](Run& r)
    { TF::Mount(r, TF::NewState(r)); }},
                                                                                                            {"manual-and-restore", 19, [](Run& r)
    {
      auto s           = r.State<TF::State>();
      auto observer    = std::make_shared<ClockObserver>();
      observer->window = s->window;
      r.OnCleanup([s, observer]
      { D::SetManualAnimatorTicks(s->window, false); });
      r.Truth("clock.manual-enable", D::SetManualAnimatorTicks(s->window, true));
      const auto before = D::GetWindowSnapshot(s->window);
      r.Truth("clock.negative-rejected", !D::TickAnimatorsForTesting(s->window, -1));
      r.Truth("clock.nan-rejected", !D::TickAnimatorsForTesting(s->window, std::numeric_limits<float>::quiet_NaN()));
      r.Truth("clock.inf-rejected", !D::TickAnimatorsForTesting(s->window, std::numeric_limits<float>::infinity()));
      r.Truth("clock.invalid-state-preserved", SameWindow(before, D::GetWindowSnapshot(s->window)));
      s->Bind(LayoutTransition::New());
      s->transition.SetChangeAnimator(LayoutAnimatorCallback::New(observer.get(), &ClockObserver::Tick), TF::AnimatorTiming(.15f));
      s->root.SetLayoutTransition(s->transition);
      TF::SetFixtureBounds(s->child, {140, 30, 80, 40});
      LayoutController::Get(s->window).ProcessLayouts();
      r.Truth("clock.active-before-tick", D::GetTransitionSnapshot(s->child).animatorActive);
      r.Truth("clock.first-tick", D::TickAnimatorsForTesting(s->window, .05f));
      r.Equal("clock.first-callback-count", observer->calls, 1);
      r.Near("clock.first-raw", observer->lastRaw, 0, 0);
      r.Truth("clock.nested-rejected", observer->nestedRejected);
      r.Truth("clock.restore", D::SetManualAnimatorTicks(s->window, false));
      const auto restored = D::GetTransitionSnapshot(s->child);
      r.Truth("clock.manual-disabled", !D::TickAnimatorsForTesting(s->window, .1f));
      r.Truth("clock.disabled-tick-preserves-state", SameTransition(restored, D::GetTransitionSnapshot(s->child)));
      r.Delay(800, [s, observer](Run& v)
      {
        v.Equal("clock.natural-start-once", s->starts[2], 1);
        v.Equal("clock.natural-finish-once", s->finishes[2], 1);
        v.Truth("clock.natural-callbacks", observer->calls > 1);
        v.Near("clock.natural-last-raw", observer->lastRaw, 1, 0);
        v.Truth("clock.natural-drained", !D::GetTransitionSnapshot(s->child).animatorActive);
        v.Truth("clock.restore-idempotent", D::SetManualAnimatorTicks(s->window, false));
      });
    }}}});
    for(bool animator : {false, true})
      for(auto slot : {LayoutTransitionSlot::CHANGE, LayoutTransitionSlot::ENTER, LayoutTransitionSlot::EXIT})
        for(bool registered : {false, true}) cases.push_back(ActiveSnapshotScenario(animator, slot, registered));
    return cases;
  }
};
REGISTER_MANUAL_TEST(TcLr62)
