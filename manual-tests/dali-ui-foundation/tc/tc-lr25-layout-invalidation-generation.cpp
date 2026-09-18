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
struct Generation
{
  std::shared_ptr<Probe>   probe;
  View                     stage, root;
  std::shared_ptr<Trace>   trace;
  Diagnostic::ViewSnapshot before{};
};
} // namespace

class TcLr25 : public Case
{
public:
  TcLr25()
  : Case("LR25", "Invalidation generation")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Steps("K06.generation", "Outer operation generations and dirty propagation through the controller", {
      {"mount", 5, [](Run& r) {
         auto s   = std::make_shared<Generation>();
         s->root  = StackLayout::New(StackOrientation::HORIZONTAL);
         s->probe = Probed(r, 20, 10, false);
         s->root.Add(s->probe->view);
         s->stage = Mount(r, s->root, 100, 50);
         r.SetState(s);
         r.AfterRender({s->stage, s->root, s->probe->view}, [=](Run& r) {
           s->before = Diagnostic::GetViewSnapshot(s->root);
           r.Truth("initial-cache", s->before.valid && s->before.measureCacheValid);
           r.Rendered("probe", s->probe->view, LayoutRect(0, 0, 20, 10));
         });
       }},
      {"invalidate", 8, [](Run& r) {
         auto s   = r.State<Generation>();
         s->trace = std::make_shared<Trace>();
         r.Truth("capture", s->trace->Begin({s->root, s->probe->view}));
         s->probe->view.InvalidateMeasure();
         const auto dirty = Diagnostic::GetViewSnapshot(s->root);
         r.AfterRender({s->stage, s->root, s->probe->view}, [=](Run& r) {
           const auto after = Diagnostic::GetViewSnapshot(s->root);
           s->trace->End();
           r.Truth("valid", dirty.valid && after.valid);
           r.Truth("propagated-dirty", dirty.measureDirty);
           r.Truth("cache-invalidated", !dirty.measureCacheValid);
           r.Truth("new-generation", after.generation != s->before.generation);
           r.Truth("new-cache", after.measureCacheValid && !after.measureDirty);
           r.Truth("ancestor-visited", s->trace->Count(Diagnostic::EventKind::ANCESTOR_VISIT) > 0);
           r.Truth("no-overflow", !s->trace->result.overflow);
         });
       }},
    }));
    out.push_back(Steps("K07.arrange-only", "Arrange invalidation preserves the measure cache", {
      {"mount", 5, [](Run& r) {
         auto s   = std::make_shared<Generation>();
         s->probe = Probed(r);
         s->stage = Mount(r, s->probe->view, 100, 50);
         r.SetState(s);
         r.AfterRender({s->stage, s->probe->view}, [=](Run& r) {
           s->before = Diagnostic::GetViewSnapshot(s->probe->view);
           r.Truth("initial-caches", s->before.valid && s->before.measureCacheValid && s->before.arrangeCacheValid);
           r.Rendered("probe", s->probe->view, LayoutRect(0, 0, 37, 19));
         });
       }},
      {"invalidate-arrange", 7, [](Run& r) {
         auto s = r.State<Generation>();
         s->probe->view.InvalidateArrange();
         const auto dirty = Diagnostic::GetViewSnapshot(s->probe->view);
         r.AfterRender({s->stage, s->probe->view}, [=](Run& r) {
           const auto after = Diagnostic::GetViewSnapshot(s->probe->view);
           r.Truth("valid", dirty.valid && after.valid);
           r.Truth("measure-preserved", dirty.measureCacheValid);
           r.Truth("arrange-invalid", !dirty.arrangeCacheValid);
           r.Truth("dirty", dirty.arrangeDirty);
           r.Truth("settled", after.arrangeCacheValid && !after.arrangeDirty);
           r.Equal("measure-producer", s->probe->measures, 1);
           r.Equal("arrange-producer", s->probe->arranges, 2);
         });
       }},
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr25)
