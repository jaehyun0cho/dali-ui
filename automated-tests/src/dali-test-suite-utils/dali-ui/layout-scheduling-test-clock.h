#ifndef DALI_UI_LAYOUT_SCHEDULING_TEST_CLOCK_H
#define DALI_UI_LAYOUT_SCHEDULING_TEST_CLOCK_H

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

#include <dali-ui-foundation/internal/layouts/layout-scheduling-policy.h>
#include <chrono>

namespace Test
{
// The existing Timer stub broadcasts to all connected timers. Tests using this
// hook verify the controller's armed/deadline checks, not backend timer delivery.
void EmitGlobalTimerSignal();

/**
 * @brief Event-thread clock override. Construct before UiTestApplication so the
 * application and all its controllers are destroyed before the provider resets.
 */
class ScopedLayoutClock
{
public:
  using SchedulingClock = Dali::Ui::Internal::LayoutScheduling::Clock;
  using TimePoint       = Dali::Ui::Internal::LayoutScheduling::TimePoint;
  using Duration        = Dali::Ui::Internal::LayoutScheduling::Duration;

  ScopedLayoutClock()
  : mPreviousInstance(Active()),
    mPreviousProvider(Dali::Ui::Internal::LayoutScheduling::SetNowFunctionForTesting(&Read))
  {
    Active() = this;
  }

  ~ScopedLayoutClock()
  {
    Dali::Ui::Internal::LayoutScheduling::SetNowFunctionForTesting(mPreviousProvider);
    Active() = mPreviousInstance;
  }

  void Advance(Duration elapsed)
  {
    mNow += elapsed;
  }

  void AdvanceFrame()
  {
    Advance(std::chrono::milliseconds(17));
  }

  TimePoint Now() const
  {
    return mNow;
  }

  ScopedLayoutClock(const ScopedLayoutClock&)            = delete;
  ScopedLayoutClock& operator=(const ScopedLayoutClock&) = delete;

private:
  static ScopedLayoutClock*& Active()
  {
    static ScopedLayoutClock* active = nullptr;
    return active;
  }

  static TimePoint Read()
  {
    return Active()->mNow;
  }

  TimePoint                                         mNow{std::chrono::seconds(1)};
  ScopedLayoutClock*                                mPreviousInstance;
  Dali::Ui::Internal::LayoutScheduling::NowFunction mPreviousProvider;
};
} // namespace Test

#endif // DALI_UI_LAYOUT_SCHEDULING_TEST_CLOCK_H
