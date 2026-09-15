/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "layout-validation-fixtures.h"

using namespace LayoutValidation;
using namespace LayoutValidation::Fixtures;

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
    out.push_back(Single("K19.add-remove", "Changing child set invalidates cached replay", 14, [](Run& r)
    {auto s=StackLayout::New(StackOrientation::HORIZONTAL);Fixed(s,200,60);s.SetSpacing(7);View a=Leaf(20,10),b=Leaf(30,10),c=Leaf(40,10);s.Add(a);s.Add(b);Compute(s,200,60);s.Remove(a);s.Add(c);Compute(s,200,60);r.Equal("child-count",s.GetChildViewCount(),2);Identity(r,"first",s.GetChildViewAt(0),b);r.Rect("b",b,LayoutRect(0,0,30,10));r.Rect("c",c,LayoutRect(37,0,40,10));s.RemoveAll();r.Size("empty",s.Measure(200,60),MeasuredSize(200,60));r.Equal("empty-count",s.GetChildViewCount(),0);r.Truth("out-of-range",!s.GetChildViewAt(0)); }));
    out.push_back(Single("K20.mutate-during-producer", "Tree mutation during producer uses a safe later pass", 6, [](Run& r)
    {auto s=StackLayout::New(StackOrientation::HORIZONTAL);Fixed(s,200,60);auto p=Probed(r,20,10,false);View b=Leaf(30,10);s.Add(p->view);bool added=false;p->onMeasure=[s,b,&added]()mutable{if(!added){added=true;s.Add(b);}};s.Measure(200,60);p->onMeasure={};s.InvalidateMeasure();Compute(s,200,60);r.Equal("children",s.GetChildViewCount(),2);r.Truth("added",added);r.Rect("later-child",b,LayoutRect(20,0,30,10)); }));
    for(unsigned mutation = 0; mutation < 4; ++mutation)
    {
      const char* names[] = {"K20.replay-property-mutation", "K20.replay-external-measure", "K20.replay-external-arrange", "K20.replay-reentrant-arrange"};
      out.push_back(Single(names[mutation], "A real cache-hit PropertySetSignal observer changes layout during replay", mutation == 3 ? 30 : 26, [mutation](Run& r)
      {
        if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        struct Observation
        {
          bool                     armed{false};
          uint32_t                 calls{0};
          Diagnostic::ViewSnapshot root{};
          Diagnostic::ViewSnapshot trigger{};
          LayoutRect               nestedResult{};
        };
        auto root = StackLayout::New(StackOrientation::HORIZONTAL);
        Fixed(root, 200, 60);
        auto a = Probed(r, 20, 10);
        auto b = Probed(r, 30, 10);
        root.Add(a->view);
        root.Add(b->view);
        Compute(root, 200, 60);
        auto       observation = std::make_shared<Observation>();
        const auto initial     = Diagnostic::GetViewSnapshot(root);
        r.Truth("initial.published-cache", initial.measureCacheValid && initial.arrangeCacheValid);
        Trace trace;
        r.Truth("capture", trace.Begin({root, a->view, b->view}));
        View trigger = a->view, target = b->view;
        trigger.PropertySetSignal().Connect(&r, [observation, root, trigger, target, mutation](Handle, Property::Index index, const Property::Value&) mutable
        {
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
        root.Arrange(LayoutRect(0, 0, 200, 60));
        const auto after = Diagnostic::GetViewSnapshot(root);
        trace.End();
        r.Equal("observer.calls", observation->calls, 1);
        r.Truth("observer.root-replaying", observation->root.replayInProgress && observation->root.arrangeInProgress);
        r.Truth("observer.child-replaying", observation->trigger.replayInProgress && observation->trigger.arrangeInProgress);
        r.Truth("observer.processing", observation->root.processing);
        r.Equal("root.cache-hit", trace.Count(Diagnostic::EventKind::ARRANGE_HIT, 1), 1);
        r.Equal("root.producer-not-run", trace.Count(Diagnostic::EventKind::ARRANGE_PRODUCER, 1), 0);
        r.Truth("replay.flags-unwound", !after.replayInProgress && !after.arrangeInProgress);
        r.Truth("capture.no-overflow", !trace.result.overflow);
        r.Equal("after.measure-cache", after.measureCacheValid, mutation >= 2);
        r.Equal("after.arrange-cache", after.arrangeCacheValid, mutation == 3);
        r.Equal("after.measure-dirty", after.measureDirty, mutation == 0);
        r.Equal("after.arrange-dirty", after.arrangeDirty, mutation == 0);
        r.Equal("after.arrange-poison", after.arrangePoisoned, mutation == 0 || mutation == 3);
        r.Truth("replay.no-producer-publish-block", !after.arrangePublishBlocked);
        if(mutation == 3) r.Rect("nested.last-completed-result", observation->nestedResult, LayoutRect(0, 0, 200, 60));
        Compute(root, 200, 60);
        r.Rect("recovered.a", a->view, LayoutRect(0, 0, 20, 10));
        r.Rect("recovered.b", b->view, LayoutRect(20, 0, mutation == 0 ? 45 : 30, 10));
        const auto recovered = Diagnostic::GetViewSnapshot(root);
        r.Truth("recovered.caches", recovered.measureCacheValid && recovered.arrangeCacheValid);
        r.Truth("recovered.clean", !recovered.measureDirty && !recovered.arrangeDirty && !recovered.arrangePoisoned && !recovered.replayInProgress);
#endif
      }));
    }
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr31)
