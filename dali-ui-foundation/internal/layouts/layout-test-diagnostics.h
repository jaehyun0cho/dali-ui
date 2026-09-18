#pragma once

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

#include <dali-ui-foundation/integration-api/layout-test-diagnostics.h>
namespace DALI_NAMESPACE
{
namespace Ui
{
class ViewImpl;
namespace Internal
{
namespace LayoutTestDiagnostics
{
using namespace Integration::LayoutTestDiagnostics;

/// Whether a capture is currently open. Event-thread only and unsynchronised, matching the
/// contract documented on the integration-api header. Read inline by the hook macros so the
/// inactive path costs one load and one branch, with no argument evaluation.
extern bool gCaptureActive;
inline bool IsCapturing()
{
  return gCaptureActive;
}

/// Whether anything can observe layout at all: a capture is open, or a test has registered at
/// least one node. It gates only the transition ownership walk and the ownerId/role it yields:
/// without a registered node that result has nothing to name. Geometry and timing (from/to,
/// duration, delay, cause) are stored unconditionally, outside the gate, as they already were,
/// because a reader consumes them even when registration happens after a transition starts.
/// What changed is the stored shape, not the gate: an entry keeps those few values instead of
/// embedding a composed public TransitionSnapshot.
/// Same thread and synchronisation contract as gCaptureActive; read inline so the inactive path
/// costs one load and one branch, with no argument evaluation.
extern bool gObserving;
inline bool IsObserving()
{
  return gObserving;
}

uint32_t    NodeId(const ViewImpl* view);
void        UnregisterNode(const ViewImpl* view);
void        Record(EventKind kind, const ViewImpl* view = nullptr, const ViewImpl* related = nullptr,
                   uint32_t detail = 0u, float value0 = 0.0f, float value1 = 0.0f, const void* window = nullptr);
inline void RecordStorage(const ViewImpl* view, StorageSite site, std::size_t count, std::size_t elementSize)
{
  if(gCaptureActive && count != 0u)
  {
    Record(EventKind::SNAPSHOT_ALLOCATION, view, nullptr, static_cast<uint32_t>(site),
           static_cast<float>(count), static_cast<float>(count * elementSize));
  }
}

struct WindowReader
{
  const void* key{nullptr};
  void*       context{nullptr};
  WindowSnapshot (*snapshot)(void*){nullptr};
  TransitionSnapshot (*transition)(void*, Ui::View){nullptr};
  Animation (*animation)(void*, Ui::View){nullptr};
  bool (*tick)(void*, float){nullptr};
  bool (*manualTicks)(void*, bool){nullptr};
};
void RegisterWindow(WindowReader reader);
void UnregisterWindow(void* context);
} // namespace LayoutTestDiagnostics
} // namespace Internal
} // namespace Ui
} // namespace DALI_NAMESPACE
/// Observation hooks. The capture test is lexically outside the argument list so that no
/// argument is evaluated while no capture is open.
#define DALI_UI_LAYOUT_TEST_EVENT(...)                                  \
  do                                                                    \
  {                                                                     \
    if(::Dali::Ui::Internal::LayoutTestDiagnostics::IsCapturing())      \
    {                                                                   \
      ::Dali::Ui::Internal::LayoutTestDiagnostics::Record(__VA_ARGS__); \
    }                                                                   \
  } while(false)

#define DALI_UI_LAYOUT_TEST_STORAGE(...)                                       \
  do                                                                           \
  {                                                                            \
    if(::Dali::Ui::Internal::LayoutTestDiagnostics::IsCapturing())             \
    {                                                                          \
      ::Dali::Ui::Internal::LayoutTestDiagnostics::RecordStorage(__VA_ARGS__); \
    }                                                                          \
  } while(false)
