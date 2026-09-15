#pragma once

/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0 */

#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
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
uint32_t    NodeId(const ViewImpl* view);
void        UnregisterNode(const ViewImpl* view);
void        Record(EventKind kind, const ViewImpl* view = nullptr, const ViewImpl* related = nullptr,
                   uint32_t detail = 0u, float value0 = 0.0f, float value1 = 0.0f, const void* window = nullptr);
inline void RecordStorage(const ViewImpl* view, StorageSite site, std::size_t count, std::size_t elementSize)
{
  if(count != 0u)
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
#define DALI_UI_LAYOUT_TEST_EVENT(...) ::Dali::Ui::Internal::LayoutTestDiagnostics::Record(__VA_ARGS__)
#define DALI_UI_LAYOUT_TEST_STORAGE(...) ::Dali::Ui::Internal::LayoutTestDiagnostics::RecordStorage(__VA_ARGS__)
#else
#define DALI_UI_LAYOUT_TEST_EVENT(...) ((void)0)
#define DALI_UI_LAYOUT_TEST_STORAGE(...) ((void)0)
#endif
