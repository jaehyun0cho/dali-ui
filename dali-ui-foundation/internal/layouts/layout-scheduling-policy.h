#ifndef DALI_UI_INTERNAL_LAYOUT_SCHEDULING_POLICY_H
#define DALI_UI_INTERNAL_LAYOUT_SCHEDULING_POLICY_H

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

#include <dali-ui-foundation/public-api/dali-ui-common.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>

namespace DALI_NAMESPACE
{
namespace Ui
{
namespace Internal
{
namespace LayoutScheduling
{
using Clock       = std::chrono::steady_clock;
using TimePoint   = Clock::time_point;
using Duration    = Clock::duration;
using NowFunction = TimePoint (*)();

/// Event-thread clock. The provider has one definition in layout-controller.cpp.
DALI_UI_API TimePoint Now();

/// Internal test seam; returns the previous provider. nullptr selects Clock::now.
/// Install and restore only on the event thread, outside live processing calls.
DALI_UI_API NowFunction SetNowFunctionForTesting(NowFunction provider);

/// The frame-rate dependency is isolated here for a future runtime FPS getter.
/// This period paces continuations; it does not synchronize them with VSYNC.
inline Duration FramePeriod()
{
  constexpr double framesPerSecond = 60.0;
  return std::chrono::duration_cast<Duration>(std::chrono::duration<double>(1.0 / framesPerSecond));
}

/// Round up so a positive sub-millisecond remainder never becomes an idle spin.
inline uint32_t TimerDelayMilliseconds(TimePoint deadline, TimePoint now)
{
  const auto remaining    = deadline > now ? deadline - now : Duration::zero();
  auto       milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(remaining);
  if(milliseconds < remaining)
  {
    milliseconds += std::chrono::milliseconds(1);
  }
  const auto value = std::max<int64_t>(1, milliseconds.count());
  return static_cast<uint32_t>(std::min<int64_t>(value, std::numeric_limits<uint32_t>::max()));
}

/**
 * @brief Pure continuation pacing policy, independent of timer delivery.
 */
class Policy
{
public:
  bool CanRun(TimePoint now) const
  {
    return !mContinuation || now >= mNotBefore;
  }

  bool IsContinuation() const
  {
    return mContinuation;
  }

  TimePoint NotBefore() const
  {
    return mNotBefore;
  }

  void BeginContinuation(TimePoint now)
  {
    if(!mContinuation)
    {
      mContinuation = true;
      mNotBefore    = now + FramePeriod();
    }
  }

  void FinishTurn(TimePoint workEnd, Duration workCost, bool outstanding)
  {
    if(outstanding)
    {
      mContinuation = true;
      mNotBefore    = workEnd + std::max(FramePeriod(), workCost);
    }
    else
    {
      Reset();
    }
  }

  void Reset()
  {
    mContinuation = false;
    mNotBefore    = TimePoint{};
  }

private:
  TimePoint mNotBefore{};
  bool      mContinuation{false};
};

} // namespace LayoutScheduling
} // namespace Internal
} // namespace Ui
} // namespace DALI_NAMESPACE

#endif // DALI_UI_INTERNAL_LAYOUT_SCHEDULING_POLICY_H
