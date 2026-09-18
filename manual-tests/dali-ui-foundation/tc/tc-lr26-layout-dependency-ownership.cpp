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
struct Observation
{
  uint32_t                 calls{0};
  Diagnostic::ViewSnapshot child{};
  Diagnostic::ViewSnapshot parent{};
};
} // namespace

class TcLr26 : public Case
{
public:
  TcLr26()
  : Case("LR26", "Dependency ownership")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(unsigned kind : {0u, 1u, 2u, 3u, 4u})
      out.push_back(Single(Id("K13.owner", kind).c_str(), "The final-slot measure of a MATCH_PARENT child runs under its arranging parent's ownership", 21, [kind](Run& r) {
        // A wrapping parent with a 120x60 minimum measures the child at the 300x100 stage
        // constraint and then re-measures it at the 120x60 final slot inside Arrange.
        View parent = Container(kind);
        parent.SetMinimumWidth(120);
        parent.SetMinimumHeight(60);
        auto p = Probed(r, 30, 10, false);
        p->view.SetRequestedWidth(MATCH_PARENT);
        p->view.SetRequestedHeight(MATCH_PARENT);
        parent.Add(p->view);
        auto observation = std::make_shared<Observation>();
        View child       = p->view;
        p->onMeasure     = [observation, parent, child]() {
          const auto owner = Diagnostic::GetViewSnapshot(parent);
          if(owner.arrangeInProgress)
          {
            ++observation->calls;
            observation->parent = owner;
            observation->child  = Diagnostic::GetViewSnapshot(child);
          }
        };
        auto trace = std::make_shared<Trace>();
        r.Truth("capture", trace->Begin({parent, child}));
        View stage = Mount(r, parent, 300, 100);
        r.AfterRender({stage, parent, child}, [=](Run& r) {
          p->onMeasure       = {};
          const auto settled = Diagnostic::GetViewSnapshot(child);
          const auto clean   = Diagnostic::GetViewSnapshot(parent);
          View invalidated = child;
          invalidated.InvalidateMeasure();
          const auto dirty = Diagnostic::GetViewSnapshot(parent);
          trace->End();
          r.Truth("owned-producer-observed", observation->calls > 0);
          r.Truth("valid", observation->child.valid && observation->parent.valid && clean.valid && dirty.valid && settled.valid);
          r.Equal("owner-in-measure", observation->child.dependencyOwnerId, 1);
          r.Equal("owner-kind-in-measure", observation->child.dependencyOwnerKind, 1);
          r.Truth("parent-arranging", observation->parent.arrangeInProgress);
          r.Truth("child-measuring", observation->child.measureInProgress);
          r.Truth("owner-measure-cache-preserved", observation->parent.measureCacheValid);
          r.Equal("owner-after-scope", settled.dependencyOwnerId, 0);
          r.Equal("owner-kind-after-scope", settled.dependencyOwnerKind, 0);
          r.Truth("owner-cache-published", clean.measureCacheValid && clean.arrangeCacheValid);
          r.Truth("explicit-invalidation-reaches-owner", dirty.arrangeDirty);
          r.Truth("owned-scope", trace->Count(Diagnostic::EventKind::OWNER_PUSH, 1) > 0);
          r.Equal("balanced-scopes", trace->Count(Diagnostic::EventKind::OWNER_PUSH), trace->Count(Diagnostic::EventKind::OWNER_POP));
          r.Truth("no-overflow", !trace->result.overflow);
          r.Size("final-measure-constraint", settled.normalizedConstraint, MeasuredSize(120, 60));
          r.Rendered("final-slot", child, LayoutRect(0, 0, 120, 60));
        });
      }));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr26)
