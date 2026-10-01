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

#include "manual-test-case.h"

#include <dali-ui-foundation/public-api/layouts/layout-controller.h>
#include <dali/public-api/adaptor-framework/ui-context.h>
#include <chrono>
#include <cstdint>
#include <iostream>

using namespace Dali;
using namespace Dali::Ui;

// No timer, animation, watchdog, idle callback or internal event pump is used.
// The server must observe stdout and impose its timeout outside the application.
class TcLayoutContinuation : public ManualTest::TestCase, public ConnectionTracker
{
public:
  Dali::String GetName() const override
  {
    return "95. Layout Continuation: Quiet and Cost";
  }

  Dali::String GetDescription() const override
  {
    return "Verify autonomous follow-up passes and paced callbacks without a test timer";
  }

  void OnEnter(View contentArea) override
  {
    mWindow            = UiContext::Get().GetDefaultWindow();
    Label instructions = Label::New(
      "Keys 1/2/3/4: 1/2/3/12 passes\n"
      "c: continuous; x: stop; b: duplicate requests\n"
      "h: callback requests; m: manual callback passes\n"
      "p: costly arrange; w: costly callback\n"
      "Observe TC95 logs. Do not interact while waiting.");
    instructions.SetRequestedWidth(MATCH_PARENT);
    instructions.SetRequestedHeight(WRAP_CONTENT);
    instructions.SetFontSize(16.0f);
    contentArea.Add(instructions);

    mProbe = View::New();
    mProbe.SetRequestedWidth(160.0f);
    mProbe.SetRequestedHeight(80.0f);
    mProbe.SetBackgroundColor(UiColor(0x81C995u));
    mProbe.SetArrangeCallback(ArrangeCallback::New(this, &TcLayoutContinuation::Arrange), ArrangePolicy::ALWAYS);
    mProbe.LayoutFinishedSignal().Connect(this, &TcLayoutContinuation::Finished);
    contentArea.Add(mProbe);
    mWindow.KeyEventSignal().Connect(this, &TcLayoutContinuation::Key);
    std::cout << "TC95_READY" << std::endl;
  }

  void OnExit() override
  {
    std::cout << "TC95_EXIT run=" << mRun << std::endl;
    mArmed = false;
    DisconnectAll();
    mProbe.SetArrangeCallback(ArrangeCallback());
    mProbe.Reset();
    mWindow.Reset();
  }

private:
  using Clock = std::chrono::steady_clock;

  static int64_t NowUs()
  {
    return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch()).count();
  }

  static void CostlyWork()
  {
    // Deliberately consume 40 ms on the event thread. This is a cost-policy
    // probe, never an application scheduling mechanism or a delayed wake.
    const auto until = Clock::now() + std::chrono::milliseconds(40);
    while(Clock::now() < until)
    {
    }
  }

  bool NeedsAnother(int completed) const
  {
    return mProducing && (mLimit == 0 || completed < mLimit);
  }

  void Start(char mode, int limit)
  {
    ++mRun;
    mMode      = mode;
    mLimit     = limit;
    mArranges  = 0;
    mFinishes  = 0;
    mProducing = true;
    mArmed     = true;
    std::cout << "TC95_START run=" << mRun << " mode=" << mMode
              << " limit=" << mLimit << " at_us=" << NowUs() << std::endl;
    mProbe.InvalidateMeasure();
  }

  void Key(Window, KeyEvent event)
  {
    if(event.GetState() != KeyEvent::DOWN)
    {
      return;
    }
    const auto& key = event.GetKeyName();
    if(key == "1")
      Start('a', 1);
    else if(key == "2")
      Start('a', 2);
    else if(key == "3")
      Start('a', 3);
    else if(key == "4")
      Start('a', 12);
    else if(key == "c")
      Start('a', 0);
    else if(key == "h")
      Start('h', 8);
    else if(key == "m")
      Start('m', 8);
    else if(key == "p")
      Start('p', 6);
    else if(key == "w")
      Start('w', 6);
    else if(key == "x" && mArmed)
    {
      mProducing = false;
      std::cout << "TC95_STOP run=" << mRun << " at_us=" << NowUs() << std::endl;
    }
    else if(key == "b" && mArmed)
    {
      for(int i = 0; i < 100; ++i)
      {
        mProbe.InvalidateMeasure();
      }
      std::cout << "TC95_BURST run=" << mRun << " at_us=" << NowUs() << std::endl;
    }
  }

  LayoutRect Arrange(View view, const LayoutRect& bounds)
  {
    if(!mArmed)
    {
      return bounds;
    }
    const int  count = ++mArranges;
    const auto begin = NowUs();
    std::cout << "TC95_ARRANGE_BEGIN run=" << mRun << " count=" << count
              << " at_us=" << begin << std::endl;
    if(mMode == 'p')
    {
      CostlyWork();
    }
    if((mMode == 'a' || mMode == 'p') && NeedsAnother(count))
    {
      view.InvalidateMeasure();
    }
    std::cout << "TC95_ARRANGE_END run=" << mRun << " count=" << count
              << " at_us=" << NowUs() << std::endl;
    return bounds;
  }

  void Finished(View view, LayoutRect bounds)
  {
    if(!mArmed)
    {
      return;
    }
    const int count = ++mFinishes;
    std::cout << "TC95_FINISH_BEGIN run=" << mRun << " count=" << count
              << " at_us=" << NowUs() << " width=" << bounds.width << std::endl;
    if(mMode == 'w')
    {
      CostlyWork();
    }
    if((mMode == 'h' || mMode == 'w' || mMode == 'm') && NeedsAnother(count))
    {
      view.InvalidateMeasure();
      if(mMode == 'm')
      {
        LayoutController::Get(mWindow).ProcessLayouts();
      }
    }
    std::cout << "TC95_FINISH_END run=" << mRun << " count=" << count
              << " at_us=" << NowUs() << std::endl;
    if(mLimit > 0 && count == mLimit)
    {
      mProducing = false;
      std::cout << "TC95_DONE run=" << mRun << " arranged=" << mArranges
                << " finished=" << mFinishes << std::endl;
    }
  }

  Window mWindow;
  View   mProbe;
  int    mRun{0};
  int    mLimit{0};
  int    mArranges{0};
  int    mFinishes{0};
  char   mMode{'a'};
  bool   mArmed{false};
  bool   mProducing{false};
};

REGISTER_MANUAL_TEST(TcLayoutContinuation)
