/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "layout-validation-fixtures.h"
struct ParkState
{
  std::shared_ptr<LayoutValidation::Fixtures::Probe> probe;
  bool                                               active{false}, saturated{false};
  uint32_t                                           parkPasses{0};
};

using namespace LayoutValidation;
using namespace LayoutValidation::Fixtures;

class TcLr29 : public Case
{
public:
  TcLr29()
  : Case("LR29", "Parked invalidation")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    Scenario scenario;
    scenario.id          = "K17.passive-park";
    scenario.description = "No self-wake while in-processing work remains pending";
    scenario.steps.push_back({"prepare", 4, [](Run& r)
    {auto s=std::make_shared<ParkState>();s->probe=Probed(r,37,19);r.SetState(s);r.AfterLayout({s->probe->view},[s](Run&done){done.Rect("settled",done.Snapshot(s->probe->view),LayoutRect(0,0,37,19));});r.Attach(s->probe->view); }});
    scenario.steps.push_back({"arm-passive-observation", 8, [](Run& r)
    {
      r.SuppressAutomaticPresentation();
      auto s              = r.State<ParkState>();
      s->active           = true;
      s->probe->onMeasure = [s]()
      {if(!s->active)return;++s->parkPasses;if(s->parkPasses>4096){s->saturated=true;s->active=false;return;}std::printf("LR29_PARK producer=%u\n",s->parkPasses);std::fflush(stdout);s->probe->view.InvalidateMeasure(); };
      r.OnCleanup([s]()
      {s->active=false;s->probe->onMeasure={}; });
      s->probe->view.InvalidateMeasure();
      const auto before = Diagnostic::GetWindowSnapshot(r.GetWindow());
      Trace      trace;
      r.Truth("capture", trace.Begin({s->probe->view}));
      LayoutController::Get(r.GetWindow()).ProcessLayouts();
      const auto after = Diagnostic::GetWindowSnapshot(r.GetWindow());
      trace.End();
      r.Truth("producer-entered", s->parkPasses > 0);
      r.Truth("pending-retained", after.pendingRoots > 0);
      // A manual drain retains the wake queued by the preceding event-time invalidation.
      r.Equal("manual-wake-preserved", after.wakeArmed, before.wakeArmed);
      r.Truth("in-pass-request-parked", trace.Count(Diagnostic::EventKind::PARKED_REQUEST) > 0);
      r.Equal("no-in-pass-wake-request", trace.Count(Diagnostic::EventKind::WAKE_REQUEST), 0);
      r.Truth("capture.no-overflow", !trace.result.overflow);
      r.Truth("not-saturated", !s->saturated);
      r.RequireExternalVerification("LR29 requires the documented passive stdout quiet interval and external-event comparison");
      std::printf("LR29_PARK_ARMED producer=%u; quiet interval must exclude all UI input\n", s->parkPasses);
      std::fflush(stdout);
    }});
    scenario.steps.push_back({"external-event-and-stop", 5, [](Run& r)
    {auto s=r.State<ParkState>();uint32_t before=s->parkPasses;s->probe->view.InvalidateMeasure();LayoutController::Get(r.GetWindow()).ProcessLayouts();uint32_t after=s->parkPasses;s->active=false;s->probe->onMeasure={};r.Truth("external-progress",after>before);r.Truth("not-saturated",!s->saturated);r.Truth("disarmed",!s->active);r.Equal("captured-before-positive",before>0,1);r.Equal("captured-after-positive",after>0,1);std::printf("LR29_PARK_STOP before=%u after=%u\n",before,after);std::fflush(stdout);s->probe->view.InvalidateMeasure(); }});
    scenario.steps.push_back({"recover-window-and-drain", 25, [](Run& r)
    {
      r.SuppressAutomaticPresentation();
      auto           s             = r.State<ParkState>();
      const uint32_t stoppedPasses = s->parkPasses;
      r.Truth("recovery.disarmed", !s->active);
      r.Truth("recovery.callback-cleared", !s->probe->onMeasure);
      r.AfterLayout({s->probe->view}, [s, stoppedPasses](Run& done)
      {
        // This fence must arrive from the natural post-process episode, without a manual drain.
        done.Rect("recovery.geometry", done.Snapshot(s->probe->view), LayoutRect(0, 0, 37, 19));
        const auto window = Diagnostic::GetWindowSnapshot(done.GetWindow());
        const auto child  = Diagnostic::GetViewSnapshot(s->probe->view);
        done.Truth("recovery.window-valid", window.valid);
        done.Truth("recovery.natural-process", !window.manualProcessing);
        done.Equal("recovery.pending-roots", window.pendingRoots, 0);
        done.Equal("recovery.pending-completions", window.pendingCompletions, 0);
        done.Truth("recovery.clean-episode", !window.dirtySinceEmit);
        done.Truth("recovery.child-valid", child.valid);
        done.Truth("recovery.measure-cache", child.measureCacheValid);
        done.Truth("recovery.arrange-cache", child.arrangeCacheValid);
        done.Truth("recovery.child-clean", !child.measureDirty && !child.arrangeDirty);
        done.Equal("recovery.park-producer-stopped", s->parkPasses, stoppedPasses);
        // One observation after the fence lets an already queued idle wake retire.
        // This timer is created only after PARK is disarmed and naturally settled.
        done.Delay(100, [s, stoppedPasses](Run& drained)
        {
          const auto idle = Diagnostic::GetWindowSnapshot(drained.GetWindow());
          drained.Truth("drained.window-valid", idle.valid);
          drained.Equal("drained.pending-roots", idle.pendingRoots, 0);
          drained.Equal("drained.pending-completions", idle.pendingCompletions, 0);
          drained.Truth("drained.wake-not-armed", !idle.wakeArmed);
          drained.Equal("drained.process-depth", idle.processDepth, 0);
          drained.Truth("drained.not-manual", !idle.manualProcessing);
          drained.Truth("drained.clean-episode", !idle.dirtySinceEmit);
          drained.Equal("drained.park-producer-stopped", s->parkPasses, stoppedPasses);
          drained.Truth("drained.not-saturated", !s->saturated);
        });
      });
      s->probe->view.InvalidateMeasure();
    }});
    out.push_back(std::move(scenario));
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr29)
