/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "layout-validation-fixtures.h"

using namespace LayoutValidation;
using namespace LayoutValidation::Fixtures;

namespace
{
struct Mutation
{
  StackLayout            stack;
  View                   stage, sibling, a, b, c;
  std::shared_ptr<Probe> pa, pb;
  float                  nudge{10};
  void                   Nudge()
  {
    sibling.SetRequestedWidth(nudge += 1);
  }
  std::shared_ptr<Trace> trace;
  struct Observation
  {
    bool                     armed{false};
    uint32_t                 calls{0};
    Diagnostic::ViewSnapshot root{};
    Diagnostic::ViewSnapshot trigger{};
    LayoutRect               nestedResult{};
  };
  std::shared_ptr<Observation> observation;
};
std::shared_ptr<Mutation> MountStack(Run& r, float width, float height)
{
  auto s   = std::make_shared<Mutation>();
  s->stack = StackLayout::New(StackOrientation::HORIZONTAL);
  Fixed(s->stack, width, height);
  s->sibling = Leaf(10, 10, "sibling");
  s->sibling.SetRequestedY(height + 10);
  s->stage = r.Stage(width, height + 30);
  s->stage.Add(s->stack);
  s->stage.Add(s->sibling);
  r.SetState(s);
  return s;
}
} // namespace

class TcLr31 : public Case
{
public:
  TcLr31()
  : Case("LR31", "Replay mutation")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Steps("K19.add-remove", "Changing the child set invalidates the cached replay on screen", {
      {"mount", 8, [](Run& r) {
         auto s = MountStack(r, 200, 60);
         s->stack.SetSpacing(7);
         s->a = Leaf(20, 10, "a");
         s->b = Leaf(30, 10, "b");
         s->c = Leaf(40, 10, "c");
         s->stack.Add(s->a);
         s->stack.Add(s->b);
         r.AfterRender({s->stage, s->stack, s->a, s->b, s->sibling}, [=](Run& r) {
           r.Rendered("initial-a", s->a, LayoutRect(0, 0, 20, 10));
           r.Rendered("initial-b", s->b, LayoutRect(27, 0, 30, 10));
         });
       }},
      {"swap", 10, [](Run& r) {
         auto s = r.State<Mutation>();
         s->stack.Remove(s->a);
         s->stack.Add(s->c);
         r.AfterRender({s->stage, s->stack, s->b, s->c, s->sibling}, [=](Run& r) {
           r.Equal("child-count", s->stack.GetChildViewCount(), 2);
           Identity(r, "first", s->stack.GetChildViewAt(0), s->b);
           r.Rendered("b", s->b, LayoutRect(0, 0, 30, 10));
           r.Rendered("c", s->c, LayoutRect(37, 0, 40, 10));
         });
       }},
      {"remove-all", 4, [](Run& r) {
         auto s = r.State<Mutation>();
         s->stack.RemoveAll();
         r.AfterRender({s->stage, s->stack, s->sibling}, [=](Run& r) {
           r.Size("empty", s->stack.GetMeasuredSize(), MeasuredSize(200, 60));
           r.Equal("empty-count", s->stack.GetChildViewCount(), 0);
           r.Truth("out-of-range", !s->stack.GetChildViewAt(0));
         });
       }},
    }));
    out.push_back(Steps("K20.mutate-during-producer", "A tree mutation inside a producer is applied by a safe later pass", {
      {"mount", 2, [](Run& r) {
         auto s = MountStack(r, 200, 60);
         s->pa  = Probed(r, 20, 10, false);
         s->b   = Leaf(30, 10, "b");
         s->stack.Add(s->pa->view);
         auto added       = std::make_shared<bool>(false);
         StackLayout host = s->stack;
         View        late = s->b;
         s->pa->onMeasure = [host, late, added]() mutable {
           if(!*added)
           {
             *added = true;
             host.Add(late);
           }
         };
         r.AfterRender({s->stage, s->stack, s->pa->view, s->sibling}, [=](Run& r) {
           s->pa->onMeasure = {};
           r.Truth("added", *added);
           r.Equal("children", s->stack.GetChildViewCount(), 2);
         });
       }},
      {"later-pass", 4, [](Run& r) {
         auto s = r.State<Mutation>();
         s->Nudge();
         r.AfterRender({s->stage, s->stack, s->pa->view, s->b, s->sibling}, [=](Run& r) { r.Rendered("later-child", s->b, LayoutRect(20, 0, 30, 10)); });
       }},
    }));
    for(unsigned mutation = 0; mutation < 4; ++mutation)
    {
      const char* names[] = {"K20.replay-property-mutation", "K20.replay-external-measure", "K20.replay-external-arrange", "K20.replay-reentrant-arrange"};
      out.push_back(Steps(names[mutation], "A PropertySetSignal observer mutates layout during a real cache-hit replay of a rendered tree", {
        {"mount", 9, [](Run& r) {
           auto s = MountStack(r, 200, 60);
           s->pa  = Probed(r, 20, 10);
           s->pb  = Probed(r, 30, 10);
           s->stack.Add(s->pa->view);
           s->stack.Add(s->pb->view);
           r.AfterRender({s->stage, s->stack, s->pa->view, s->pb->view, s->sibling}, [=](Run& r) {
             const auto initial = Diagnostic::GetViewSnapshot(s->stack);
             r.Truth("initial.published-cache", initial.measureCacheValid && initial.arrangeCacheValid);
             r.Rendered("initial.a", s->pa->view, LayoutRect(0, 0, 20, 10));
             r.Rendered("initial.b", s->pb->view, LayoutRect(20, 0, 30, 10));
           });
         }},
        {"replay", mutation == 3 ? 19u : 15u, [mutation](Run& r) {
           auto s         = r.State<Mutation>();
           s->observation = std::make_shared<Mutation::Observation>();
           s->trace       = std::make_shared<Trace>();
           r.Truth("capture", s->trace->Begin({s->stack, s->pa->view, s->pb->view}));
           View trigger = s->pa->view, target = s->pb->view;
           View root    = s->stack;
           auto observation = s->observation;
           trigger.PropertySetSignal().Connect(&r, [observation, root, trigger, target, mutation](Handle, Property::Index index, const Property::Value&) mutable {
             if(!observation->armed || index != Actor::Property::POSITION_X) return;
             observation->armed = false;
             ++observation->calls;
             observation->root    = Diagnostic::GetViewSnapshot(root);
             observation->trigger = Diagnostic::GetViewSnapshot(trigger);
             if(mutation == 0)
               target.SetMinimumWidth(45);
             else if(mutation == 1)
               target.Measure(12, 8);
             else if(mutation == 2)
               target.Arrange(LayoutRect(75, 20, 10, 5));
             else
               observation->nestedResult = root.Arrange(LayoutRect(0, 0, 999, 999));
           });
           Actor raw = trigger;
           raw.SetProperty(Actor::Property::POSITION_X, 99.f);
           observation->armed = true;
           // An out-of-band Arrange with the current slot replays the rendered tree from cache.
           root.Arrange(LayoutRect(0, 0, 200, 60));
           const auto after = Diagnostic::GetViewSnapshot(root);
           s->trace->End();
           r.Equal("observer.calls", observation->calls, 1);
           r.Truth("observer.root-replaying", observation->root.replayInProgress && observation->root.arrangeInProgress);
           r.Truth("observer.child-replaying", observation->trigger.replayInProgress && observation->trigger.arrangeInProgress);
           r.Truth("observer.processing", observation->root.processing);
           r.Equal("root.cache-hit", s->trace->Count(Diagnostic::EventKind::ARRANGE_HIT, 1), 1);
           r.Equal("root.producer-not-run", s->trace->Count(Diagnostic::EventKind::ARRANGE_PRODUCER, 1), 0);
           r.Truth("replay.flags-unwound", !after.replayInProgress && !after.arrangeInProgress);
           r.Truth("capture.no-overflow", !s->trace->result.overflow);
           r.Equal("after.measure-cache", after.measureCacheValid, mutation >= 2);
           r.Equal("after.arrange-cache", after.arrangeCacheValid, mutation == 3);
           r.Equal("after.measure-dirty", after.measureDirty, mutation == 0);
           r.Equal("after.arrange-dirty", after.arrangeDirty, mutation == 0);
           r.Equal("after.arrange-poison", after.arrangePoisoned, mutation == 0 || mutation == 3);
           r.Truth("replay.no-producer-publish-block", !after.arrangePublishBlocked);
           if(mutation == 3) r.Rect("nested.last-completed-result", observation->nestedResult, LayoutRect(0, 0, 200, 60));
         }},
        {"recover", 10, [mutation](Run& r) {
           auto s = r.State<Mutation>();
           s->Nudge();
           if(mutation == 0) s->stack.InvalidateMeasure();
           r.AfterRender({s->stage, s->stack, s->pa->view, s->pb->view, s->sibling}, [=](Run& r) {
             r.Rendered("recovered.a", s->pa->view, LayoutRect(0, 0, 20, 10));
             r.Rendered("recovered.b", s->pb->view, LayoutRect(20, 0, mutation == 0 ? 45 : 30, 10));
             const auto recovered = Diagnostic::GetViewSnapshot(s->stack);
             r.Truth("recovered.caches", recovered.measureCacheValid && recovered.arrangeCacheValid);
             r.Truth("recovered.clean", !recovered.measureDirty && !recovered.arrangeDirty && !recovered.arrangePoisoned && !recovered.replayInProgress);
           });
         }},
      }));
    }
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr31)
