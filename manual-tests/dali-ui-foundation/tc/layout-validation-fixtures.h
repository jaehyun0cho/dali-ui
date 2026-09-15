/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#pragma once
#include <dali-ui-foundation/integration-api/layout-test-diagnostics.h>
#include <dali-ui-foundation/public-api/configuration/ui-scale-manager.h>
#include <dali-ui-foundation/public-api/layouts/absolute-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/flex-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/grid-layout-manager.h>
#include <dali-ui-foundation/public-api/layouts/layout-controller.h>
#include <dali-ui-foundation/public-api/layouts/stack-layout-manager.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <utility>
#include "layout-validation-support.h"

namespace LayoutValidation
{
namespace Fixtures
{
struct Probe
{
  View                                         view;
  float                                        width{37.0f};
  float                                        height{19.0f};
  float                                        lastWidth{0};
  float                                        lastHeight{0};
  uint32_t                                     measures{0};
  uint32_t                                     arranges{0};
  bool                                         wrapping{false};
  std::function<void()>                        onMeasure;
  std::function<void()>                        onArrange;
  std::function<LayoutRect(const LayoutRect&)> returned;
  MeasuredSize                                 Measure(View, float w, float h)
  {
    ++measures;
    lastWidth  = w;
    lastHeight = h;
    if(onMeasure) onMeasure();
    float desiredHeight = wrapping && w < width ? height * 2.0f : height;
    return MeasuredSize(std::min(width, std::max(0.0f, w)), std::min(desiredHeight, std::max(0.0f, h)));
  }
  LayoutRect Arrange(View, const LayoutRect& b)
  {
    ++arranges;
    if(onArrange) onArrange();
    return returned ? returned(b) : b;
  }
};
inline std::shared_ptr<Probe> Probed(Run& run, float w = 37, float h = 19, bool arrange = true)
{
  auto p    = std::make_shared<Probe>();
  p->width  = w;
  p->height = h;
  p->view   = View::New();
  p->view.SetRequestedWidth(WRAP_CONTENT);
  p->view.SetRequestedHeight(WRAP_CONTENT);
  p->view.SetMeasureCallback(MeasureCallback::New(p.get(), &Probe::Measure));
  if(arrange) p->view.SetArrangeCallback(ArrangeCallback::New(p.get(), &Probe::Arrange));
  run.OnCleanup([p]()
  { p->view.SetMeasureCallback({}); p->view.SetArrangeCallback({}); p->onMeasure={}; p->onArrange={}; });
  return p;
}
inline View Container(unsigned kind, float w = WRAP_CONTENT, float h = WRAP_CONTENT)
{
  View v;
  switch(kind)
  {
    case 1:
      v = StackLayout::New(StackOrientation::HORIZONTAL);
      break;
    case 2:
      v = FlexLayout::New();
      break;
    case 3:
      v = GridLayout::New();
      break;
    case 4:
      v = AbsoluteLayout::New();
      break;
    default:
      v = View::New();
      break;
  }
  v.SetRequestedWidth(w);
  v.SetRequestedHeight(h);
  return v;
}
inline void Compute(View v, float width, float height)
{
  v.Measure(width, height);
  v.Arrange(LayoutRect(0, 0, width, height));
}
inline void Fixed(View v, float width, float height)
{
  v.SetRequestedWidth(width);
  v.SetRequestedHeight(height);
}
inline void GridTracks(GridLayout grid, const std::vector<float>& rows, const std::vector<float>& columns)
{
  grid.ClearRowDefinitions();
  grid.ClearColumnDefinitions();
  for(float value : rows) grid.AddRowDefinition(GridLength::Absolute(value));
  for(float value : columns) grid.AddColumnDefinition(GridLength::Absolute(value));
}
inline View Cell(GridLayout grid, uint32_t row, uint32_t column, uint32_t rows = 1, uint32_t columns = 1)
{
  View v = View::New();
  v.SetLayoutParams(GridLayoutParams::New().SetRow(row).SetColumn(column).SetRowSpan(rows).SetColumnSpan(columns));
  grid.Add(v);
  return v;
}
inline void Identity(Run& run, const char* id, View a, View b)
{
  run.Truth(id, a == b);
}

#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
namespace Diagnostic = Dali::Ui::Integration::LayoutTestDiagnostics;
struct Trace
{
  std::array<Diagnostic::Event, 16384> events{};
  Diagnostic::CaptureResult            result{};
  bool                                 started{false};
  bool                                 Begin(const std::vector<View>& views, uint64_t epoch = 1)
  {
    Diagnostic::ClearRegisteredNodes();
    for(uint32_t i = 0; i < views.size(); ++i)
      if(!Diagnostic::RegisterNode(views[i], i + 1)) return false;
    started = Diagnostic::BeginCapture(events.data(), events.size(), epoch);
    return started;
  }
  void End()
  {
    if(started)
    {
      result  = Diagnostic::EndCapture();
      started = false;
    }
  }
  uint64_t Count(Diagnostic::EventKind kind, uint32_t node = 0) const
  {
    uint64_t n = 0;
    for(size_t i = 0; i < result.count; ++i)
      if(events[i].kind == kind && (!node || events[i].nodeId == node)) ++n;
    return n;
  }
  ~Trace()
  {
    End();
    Diagnostic::ClearRegisteredNodes();
  }
};
#endif
inline LayoutRect SwapAxes(const LayoutRect& b)
{
  return LayoutRect(b.y, b.x, b.height, b.width);
}
} // namespace Fixtures
} // namespace LayoutValidation
