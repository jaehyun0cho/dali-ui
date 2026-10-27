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
  bool                     budgetSeen{false};
  MeasuredSize             budget{-1, -1}; ///< Constraint of the child's first producer call
  MeasuredSize             owned{-1, -1};  ///< Constraint of the last producer call made while the parent arranges
};
} // namespace

class Tc119 : public Case
{
public:
  Tc119()
  : Case("TC119", "Dependency ownership")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    for(unsigned kind : {0u, 1u, 2u, 3u, 4u})
      out.push_back(Single(Id("K13.owner", kind).c_str(), "The arrange-time final-slot measure of a MATCH_PARENT child whose parent has no size of its own runs under the arranging parent's ownership", 25, [kind](Run& r) {
        // A wrapping parent whose only child is MATCH_PARENT has no size of its own: it keeps the
        // child's at-most probe at the 300x100 stage constraint, measures 30x10 and re-measures the
        // child at the 30x10 final slot inside Arrange.
        View parent = Container(kind);
        auto p      = Probed(r, 30, 10, false);
        p->view.SetRequestedWidth(MATCH_PARENT);
        p->view.SetRequestedHeight(MATCH_PARENT);
        parent.Add(p->view);
        auto   observation = std::make_shared<Observation>();
        View   child       = p->view;
        Probe* probe       = p.get();
        p->onMeasure       = [observation, parent, child, probe]() {
          if(!observation->budgetSeen)
          {
            observation->budgetSeen = true;
            observation->budget     = MeasuredSize(probe->lastWidth, probe->lastHeight);
          }
          const auto owner = Diagnostic::GetViewSnapshot(parent);
          if(owner.arrangeInProgress)
          {
            ++observation->calls;
            observation->parent = owner;
            observation->child  = Diagnostic::GetViewSnapshot(child);
            observation->owned  = MeasuredSize(probe->lastWidth, probe->lastHeight);
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
          r.Size("budget-constraint", observation->budget, MeasuredSize(300, 100));
          r.Size("owned-constraint", observation->owned, MeasuredSize(30, 10));
          r.Size("final-measure-constraint", settled.normalizedConstraint, MeasuredSize(30, 10));
          r.Rendered("final-slot", child, LayoutRect(0, 0, 30, 10));
        });
      }));
    for(unsigned kind : {0u, 4u})
      out.push_back(Single(Id("K13.measure-pass-slot", kind).c_str(), "A parent whose own minimum gives it a size measures its MATCH_PARENT child at the final slot in the measure pass; Arrange owns no re-measure", 16, [kind](Run& r) {
        // The 120x60 minimum gives the wrapping parent a size of its own, so the child is probed at
        // the 300x100 stage constraint and then measured EXACTLY at the 120x60 slot inside Measure;
        // Arrange finds that measurement and skips the re-measure.
        View parent = Container(kind);
        parent.SetMinimumWidth(120);
        parent.SetMinimumHeight(60);
        auto p = Probed(r, 30, 10, false);
        p->view.SetRequestedWidth(MATCH_PARENT);
        p->view.SetRequestedHeight(MATCH_PARENT);
        parent.Add(p->view);
        auto observation = std::make_shared<Observation>();
        View child       = p->view;
        p->onMeasure     = [observation, parent]() {
          if(Diagnostic::GetViewSnapshot(parent).arrangeInProgress) ++observation->calls;
        };
        auto trace = std::make_shared<Trace>();
        r.Truth("capture", trace->Begin({parent, child}));
        View stage = Mount(r, parent, 300, 100);
        r.AfterRender({stage, parent, child}, [=](Run& r) {
          p->onMeasure       = {};
          const auto settled = Diagnostic::GetViewSnapshot(child);
          trace->End();
          r.Equal("owned-calls", observation->calls, 0);
          r.Equal("parent-owner-push", trace->Count(Diagnostic::EventKind::OWNER_PUSH, 1), 0);
          r.Equal("child-producer", trace->Count(Diagnostic::EventKind::MEASURE_PRODUCER, 2), 2);
          r.Equal("child-enter", trace->Count(Diagnostic::EventKind::MEASURE_ENTER, 2), 2);
          r.Equal("child-hit", trace->Count(Diagnostic::EventKind::MEASURE_HIT, 2), 0);
          r.Equal("child-arrange-enter", trace->Count(Diagnostic::EventKind::ARRANGE_ENTER, 2), 1);
          r.Size("final-measure-constraint", settled.normalizedConstraint, MeasuredSize(120, 60));
          r.Size("measured", child.GetMeasuredSize(), MeasuredSize(120, 60));
          r.Rendered("child", child, LayoutRect(0, 0, 120, 60));
          r.Truth("no-overflow", !trace->result.overflow);
        });
      }));
    return out;
  }
};

REGISTER_MANUAL_TEST(Tc119)
