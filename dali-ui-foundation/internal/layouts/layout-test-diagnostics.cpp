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
 *
 */

// CLASS HEADER
#include <dali-ui-foundation/internal/layouts/layout-test-diagnostics.h>

// EXTERNAL INCLUDES
#include <dali/public-api/adaptor-framework/window.h>
#include <algorithm>
#include <cmath>
#include <new>

// INTERNAL INCLUDES
#include <dali-ui-foundation/internal/layouts/layout-invalidation-generation.h>
#include <dali-ui-foundation/internal/views/view/view-data-impl.h>
#include <dali-ui-foundation/public-api/views/view-impl.h>

namespace DALI_NAMESPACE
{
namespace Ui
{
namespace Internal
{
namespace LayoutTestDiagnostics
{
namespace
{
constexpr std::size_t MAX_REGISTERED_NODES   = 8192u;
constexpr std::size_t MAX_REGISTERED_WINDOWS = 64u;
struct NodeRecord
{
  const ViewImpl* view{nullptr};
  uint32_t        id{0};
};
/// Allocated on the first successful RegisterNode and never grown or freed: production
/// builds never register a node, so the registry costs one pointer instead of 128 KB of BSS.
NodeRecord*   gNodes{nullptr};
std::size_t   gNodeCount{0};     ///< Number of occupied node slots.
std::size_t   gNodeHighWater{0}; ///< One past the highest slot ever occupied since the last clear.
WindowReader  gWindows[MAX_REGISTERED_WINDOWS];
Event*        gEvents{nullptr};
CaptureResult gCapture;
bool          gRegistryOverflow{false};

const WindowReader* FindWindow(const void* key)
{
  for(const auto& entry : gWindows)
  {
    if(entry.key == key && entry.context)
    {
      return &entry;
    }
  }
  return nullptr;
}

/// Recomputes the observation gate after every mutation of the two conditions it folds
/// together, so that the hook sites read a single bool instead of two globals.
void UpdateObserving()
{
  gObserving = gCaptureActive || gNodeCount != 0u;
}
} // namespace

bool gCaptureActive{false};
bool gObserving{false};

uint32_t NodeId(const ViewImpl* view)
{
  // Event-thread only: scan the occupied prefix, and nothing at all until a test registers.
  if(view && gNodeCount != 0u)
  {
    for(std::size_t i = 0; i < gNodeHighWater; ++i)
    {
      if(gNodes[i].view == view)
      {
        return gNodes[i].id;
      }
    }
  }
  return 0u;
}

void UnregisterNode(const ViewImpl* view)
{
  if(gNodeCount == 0u)
  {
    return;
  }
  for(std::size_t i = 0; i < gNodeHighWater; ++i)
  {
    if(gNodes[i].view == view)
    {
      gNodes[i] = {};
      --gNodeCount;
      while(gNodeHighWater != 0u && gNodes[gNodeHighWater - 1u].view == nullptr)
      {
        --gNodeHighWater;
      }
      UpdateObserving();
      return;
    }
  }
}

void Record(EventKind kind, const ViewImpl* view, const ViewImpl* related, uint32_t detail, float value0, float value1, const void* window)
{
  // Defensive inner check; the hook macros test IsCapturing() before evaluating any argument.
  if(!gCaptureActive)
  {
    return;
  }
  ++gCapture.totalEvents;
  if(gCapture.count == gCapture.capacity)
  {
    gCapture.overflow = true;
    return;
  }
  Event& event        = gEvents[gCapture.count++];
  event.epoch         = gCapture.epoch;
  event.sequence      = gCapture.totalEvents;
  event.kind          = kind;
  event.nodeId        = NodeId(view);
  event.relatedNodeId = NodeId(related);
  event.windowToken   = reinterpret_cast<uintptr_t>(window);
  event.generation    = LayoutInvalidation::CurrentGeneration();
  event.detail        = detail;
  event.value0        = value0;
  event.value1        = value1;
}

void RegisterWindow(WindowReader reader)
{
  for(auto& entry : gWindows)
  {
    if(entry.context && entry.key == reader.key)
    {
      entry = reader;
      return;
    }
  }
  for(auto& entry : gWindows)
  {
    if(!entry.context)
    {
      entry = reader;
      return;
    }
  }
  gRegistryOverflow = true;
  gCapture.overflow = true;
}

void UnregisterWindow(void* context)
{
  for(auto& entry : gWindows)
  {
    if(entry.context == context)
    {
      entry = {};
      // A freed slot lifts the registry overflow so later captures can start again.
      gRegistryOverflow = false;
    }
  }
}
} // namespace LayoutTestDiagnostics
} // namespace Internal

namespace Integration
{
namespace LayoutTestDiagnostics
{
bool RegisterNode(Ui::View view, uint32_t id)
{
  using namespace Internal::LayoutTestDiagnostics;
  if(!view || id == 0u || gCaptureActive)
  {
    return false;
  }
  const ViewImpl* object = &GetImpl(view);
  for(std::size_t i = 0; i < gNodeHighWater; ++i)
  {
    if(gNodes[i].id == id && gNodes[i].view != object)
    {
      return false;
    }
  }
  for(std::size_t i = 0; i < gNodeHighWater; ++i)
  {
    if(gNodes[i].view == object)
    {
      gNodes[i].id = id;
      return true;
    }
  }
  if(!gNodes)
  {
    gNodes = new(std::nothrow) NodeRecord[MAX_REGISTERED_NODES]{};
    if(!gNodes)
    {
      return false;
    }
  }
  for(std::size_t i = 0; i < MAX_REGISTERED_NODES; ++i)
  {
    if(!gNodes[i].view)
    {
      gNodes[i] = {object, id};
      ++gNodeCount;
      gNodeHighWater = std::max(gNodeHighWater, i + 1u);
      UpdateObserving();
      return true;
    }
  }
  return false;
}

void ClearRegisteredNodes()
{
  using namespace Internal::LayoutTestDiagnostics;
  if(gCaptureActive)
  {
    gCapture.overflow = true;
    return;
  }
  for(std::size_t i = 0; i < gNodeHighWater; ++i)
  {
    gNodes[i] = {};
  }
  gNodeCount     = 0u;
  gNodeHighWater = 0u;
  UpdateObserving();
}

bool BeginCapture(Event* buffer, std::size_t capacity, uint64_t epoch)
{
  using namespace Internal::LayoutTestDiagnostics;
  auto& state = gCapture;
  if(gCaptureActive || !buffer || capacity == 0u || epoch == 0u || gRegistryOverflow)
  {
    return false;
  }
  gEvents        = buffer;
  state          = {};
  state.capacity = capacity;
  state.epoch    = epoch;
  gCaptureActive = true;
  UpdateObserving();
  return true;
}

CaptureResult EndCapture()
{
  using namespace Internal::LayoutTestDiagnostics;
  gCaptureActive  = false;
  gCapture.active = false;
  gEvents         = nullptr;
  UpdateObserving();
  return gCapture;
}

CaptureResult GetCaptureStatus()
{
  CaptureResult result = Internal::LayoutTestDiagnostics::gCapture;
  result.active        = Internal::LayoutTestDiagnostics::gCaptureActive;
  return result;
}

ViewSnapshot GetViewSnapshot(Ui::View view)
{
  return view ? Internal::ViewDataImpl::Get(GetImpl(view)).GetLayoutTestSnapshot() : ViewSnapshot{};
}

WindowSnapshot GetWindowSnapshot(Window window)
{
  if(window)
  {
    if(const auto* reader = Internal::LayoutTestDiagnostics::FindWindow(window.GetObjectPtr()))
    {
      return reader->snapshot(reader->context);
    }
  }
  return {};
}

TransitionSnapshot GetTransitionSnapshot(Ui::View view)
{
  if(view)
  {
    Window window = Window::Get(view);
    if(window)
    {
      if(const auto* reader = Internal::LayoutTestDiagnostics::FindWindow(window.GetObjectPtr()))
      {
        return reader->transition(reader->context, view);
      }
    }
  }
  return {};
}

Animation GetSpecAnimation(Ui::View view)
{
  if(view)
  {
    Window window = Window::Get(view);
    if(window)
    {
      if(const auto* reader = Internal::LayoutTestDiagnostics::FindWindow(window.GetObjectPtr()))
      {
        return reader->animation(reader->context, view);
      }
    }
  }
  return {};
}

bool TickAnimatorsForTesting(Window window, float elapsedSeconds)
{
  if(window && std::isfinite(elapsedSeconds) && elapsedSeconds >= 0.0f)
  {
    if(const auto* reader = Internal::LayoutTestDiagnostics::FindWindow(window.GetObjectPtr()))
    {
      return reader->tick(reader->context, elapsedSeconds);
    }
  }
  return false;
}

bool SetManualAnimatorTicks(Window window, bool enabled)
{
  if(window)
  {
    if(const auto* reader = Internal::LayoutTestDiagnostics::FindWindow(window.GetObjectPtr()))
    {
      return reader->manualTicks(reader->context, enabled);
    }
  }
  return false;
}
} // namespace LayoutTestDiagnostics
} // namespace Integration
} // namespace Ui
} // namespace DALI_NAMESPACE
