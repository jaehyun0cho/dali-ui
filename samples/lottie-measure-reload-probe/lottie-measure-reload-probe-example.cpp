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
 */

// On-device probe: what happens when a view RELOADS a Lottie child from inside its own
// measure pass?
//
// MeasureReloadProbeView (measure-reload-probe-view.h) creates a LottieAnimationView in
// its construction path, sets its resource with SetResourceUrl() and adds it as a child.
// It then installs a measure callback with View::SetMeasureCallback(), and from inside
// that callback it touches the same child before measuring it -- either with
// SetResourceUrl() carrying the url the child already has, or with Reload().
//
// What to observe on a device:
//  1. DEFAULT mode applies the current URL, which is a no-op. After resource
//     readiness completes, measure and completion counts should stop increasing.
//  2. Press 5 for explicit Reload() on every measure. This intentionally produces
//     continuous work. Layout continuations are scheduled automatically and paced;
//     each successful pass still reports LayoutFinished when logging is enabled.
//  3. Press 3 before comparing CPU cost: logging itself adds event-thread work.
//     Rate lines report wall time for 25 measures; no fixed cross-device CPU cap
//     or exact 60 Hz rate is implied.
//  4. Press 1 to disable the producer or 5 to restore same-URL no-op behavior.
//     Retained requests should drain without another user action, then stop.
//
// Keys:
//  1 - toggle the in-measure call; on by default
//  2 - print current counters
//  3 - toggle per-pass trace; on by default
//  4 - play / pause the animation; paused by default
//  5 - toggle explicit Reload(); off by default
//  Escape / Back - quit
//
// Reload invalidates the child while its parent's measure is in progress. The
// dirty/cache state is retained, and the parent's in-progress cache publication
// is withheld. Remaining work receives a paced continuation. The child can rebuild
// its visual in the current measure, while asynchronous resource readiness can
// generate independent events; those events do not bypass a pending continuation
// deadline. Same-URL SetResourceUrl leaves the visual clean and remains a no-op.
//
// This deliberately unconditional producer diagnoses repeated work. Prefer state
// setters and conditional invalidation in applications. A pass completion signal
// does not imply stable geometry or resource/presentation completion.

#include <dali-ui-foundation/dali-ui-foundation.h>
#include <dali-ui-foundation/public-api/layouts/layout-controller.h>
#include <dali-ui-foundation/public-api/layouts/stack-layout.h>
#include <dali-ui-foundation/public-api/views/text-controls/label.h>
#include <dali/devel-api/adaptor-framework/application.h>

#include <iostream>

#include "measure-reload-probe-view.h"

using namespace Dali;
using namespace Dali::Ui;
using namespace LottieProbeSample;

namespace
{
Label MakeLabel(const Dali::String& text, float fontSize, uint32_t color)
{
  Label label = Label::New(text);
  label.SetRequestedWidth(MATCH_PARENT);
  label.SetRequestedHeight(WRAP_CONTENT);
  label.SetFontSize(fontSize);
  label.SetTextColor(UiColor(color));
  return label;
}
} // namespace

class LottieMeasureReloadProbeExample : public ConnectionTracker
{
public:
  explicit LottieMeasureReloadProbeExample(Application& application)
  : mApplication(application)
  {
    mApplication.InitSignal().Connect(this, &LottieMeasureReloadProbeExample::Create);
  }

  void Create(Application application)
  {
    Window window = application.GetWindow();
    window.SetBackgroundColor(Color::WHITE);
    window.KeyEventSignal().Connect(this, &LottieMeasureReloadProbeExample::OnKeyEvent);

    StackLayout root = StackLayout::New(StackOrientation::VERTICAL);
    root.SetRequestedWidth(MATCH_PARENT);
    root.SetRequestedHeight(MATCH_PARENT);
    root.SetSpacing(12.0f);
    root.SetPadding(Insets(24.0f, 24.0f, 24.0f, 24.0f));

    root.Add(MakeLabel("Lottie reload from inside Measure", 24.0f, 0x202124u));
    root.Add(MakeLabel("The probe below touches its Lottie child from its measure callback.", 14.0f, 0x5F6368u));
    root.Add(MakeLabel("Keys: 1 call on/off, 2 counters, 3 per-pass log, 4 play/pause, 5 Reload().", 14.0f, 0x5F6368u));

    mProbe = MeasureReloadProbeView::New();
    root.Add(mProbe);

    window.Add(root);

    LayoutController::Get(window).LayoutFinishedSignal().Connect(this, &LottieMeasureReloadProbeExample::OnLayoutFinished);

    std::cout << "probe started: in-measure call " << (mProbe.IsReloadInMeasure() ? "ON" : "OFF")
              << ", explicit Reload() " << (mProbe.IsExplicitReload() ? "ON" : "OFF")
              << ", per-pass log " << (mProbe.IsPerPassLogging() ? "ON" : "OFF")
              << ", playback " << (mProbe.IsPlaying() ? "ON" : "OFF") << std::endl;
  }

  void OnLayoutFinished(Window /*window*/)
  {
    if(mProbe.IsPerPassLogging())
    {
      std::cout << "LayoutFinished: window pass completed; measures=" << mProbe.GetMeasureCount()
                << ", in-pass calls=" << mProbe.GetReloadCount() << std::endl;
    }
  }

  void OnKeyEvent(Window /*window*/, KeyEvent event)
  {
    if(event.GetState() != KeyEvent::DOWN)
    {
      return;
    }

    if(event.GetKeyName() == "1")
    {
      mProbe.SetReloadInMeasure(!mProbe.IsReloadInMeasure());
      std::cout << "in-measure call " << (mProbe.IsReloadInMeasure() ? "ON" : "OFF") << std::endl;
    }
    else if(event.GetKeyName() == "2")
    {
      std::cout << "counters: " << mProbe.GetMeasureCount() << " measure passes, "
                << mProbe.GetReloadCount() << " in-pass calls" << std::endl;
    }
    else if(event.GetKeyName() == "3")
    {
      mProbe.SetPerPassLogging(!mProbe.IsPerPassLogging());
      std::cout << "per-pass log " << (mProbe.IsPerPassLogging() ? "ON" : "OFF") << std::endl;
    }
    else if(event.GetKeyName() == "4")
    {
      mProbe.SetPlaying(!mProbe.IsPlaying());
      std::cout << "playback " << (mProbe.IsPlaying() ? "ON" : "OFF") << std::endl;
    }
    else if(event.GetKeyName() == "5")
    {
      mProbe.SetExplicitReload(!mProbe.IsExplicitReload());
      std::cout << "explicit Reload() " << (mProbe.IsExplicitReload() ? "ON" : "OFF")
                << " (the in-measure call is now "
                << (mProbe.IsExplicitReload() ? "Reload()" : "SetResourceUrl() with the current url")
                << ")" << std::endl;
    }
    else if(IsKey(event, Dali::DALI_KEY_ESCAPE) || IsKey(event, Dali::DALI_KEY_BACK))
    {
      mApplication.Quit();
    }
  }

private:
  Application&           mApplication;
  MeasureReloadProbeView mProbe;
};

int DALI_EXPORT_API main(int argc, char** argv)
{
  Application                     application = Application::New(&argc, &argv);
  LottieMeasureReloadProbeExample example(application);
  application.MainLoop();
  return 0;
}
