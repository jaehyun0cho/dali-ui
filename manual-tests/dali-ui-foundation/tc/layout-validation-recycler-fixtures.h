/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include "layout-validation-support.h"
#include <dali-ui-foundation/integration-api/items-layouter-impl.h>
#include <dali-ui-foundation/public-api/views/recycler/linear-items-layouter.h>
#include <dali-ui-foundation/public-api/views/recycler/group-linear-items-layouter.h>
#include <dali-ui-foundation/public-api/views/recycler/group-data-source.h>
#include <dali-ui-foundation/public-api/views/recycler/recycler-view.h>
#include <algorithm>
#include <cmath>
#include <map>
namespace LayoutValidation
{
struct FixedRecycler : Dali::Ui::Recycler
{
  std::vector<View> items;
  std::vector<bool> active;
  float viewport{50};
  float cross{120};
  float before{0};
  float after{0};
  ItemOffsets offsets{};
  uint32_t gets{0};
  uint32_t recycled{0};
  explicit FixedRecycler(uint32_t count, float extent = 20, bool horizontal = false)
  {
    for(uint32_t i = 0; i < count; ++i) items.push_back(Leaf(horizontal ? extent : MATCH_PARENT, horizontal ? MATCH_PARENT : extent));
    active.resize(count, false);
  }
  View GetViewForPosition(uint32_t position) override { ++gets; if(position >= items.size()) return {}; active[position] = true; return items[position]; }
  void RecycleViewForPosition(uint32_t position) override { if(position < active.size() && active[position]) { active[position] = false; ++recycled; } }
  void RecycleAllViews() override { for(uint32_t i = 0; i < active.size(); ++i) RecycleViewForPosition(i); }
  uint32_t GetItemCount() const override { return static_cast<uint32_t>(items.size()); }
  float GetViewportExtent() const override { return viewport; }
  float GetCrossExtent() const override { return cross; }
  float GetCacheBefore() const override { return before; }
  float GetCacheAfter() const override { return after; }
  ItemOffsets GetDecorationOffsets(uint32_t) const override { return offsets; }
};
struct Groups : GroupDataSource
{
  std::vector<uint32_t> counts{1, 3, 0, 2};
  std::vector<bool> headers{true, false, true, false};
  uint32_t GetGroupCount() const override { return static_cast<uint32_t>(counts.size()); }
  uint32_t GetGroupItemCount(uint32_t group) const override { return counts.at(group); }
  bool HasGroupHeader(uint32_t group) const override { return headers.at(group); }
};
struct AdapterFixture
{
  ItemAdapter adapter;
  RecyclerView view;
  LinearItemsLayouter layouter;
  std::vector<uint32_t> ids;
  std::vector<float> heights;
  std::map<uint32_t, View> bound;
  uint32_t creates{0}, binds{0}, recycles{0};
};
inline std::shared_ptr<AdapterFixture> MakeAdapterFixture(Run& r, uint32_t count = 12)
{
  auto s = std::make_shared<AdapterFixture>();
  for(uint32_t i = 0; i < count; ++i) { s->ids.push_back(1000 + i); s->heights.push_back(20); }
  s->adapter = ItemAdapter::New();
  s->adapter.GetItemCountSignal().Connect(&r, [s]() { return static_cast<uint32_t>(s->ids.size()); });
  s->adapter.CreateViewHolderSignal().Connect(&r, [s](ItemViewHolder& holder) { ++s->creates; holder.view = Leaf(MATCH_PARENT, 20); });
  s->adapter.BindViewHolderSignal().Connect(&r, [s](ItemViewHolder& holder)
  {
    ++s->binds;
    holder.view.SetProperty(Actor::Property::NAME, Dali::String(Id("stable", s->ids.at(holder.position)).c_str()));
    holder.view.SetRequestedHeight(s->heights.at(holder.position));
    s->bound[holder.position] = holder.view;
  });
  s->adapter.RecycleViewHolderSignal().Connect(&r, [s](ItemViewHolder& holder)
  {
    ++s->recycles;
    auto found = s->bound.find(holder.position);
    if(found != s->bound.end() && found->second == holder.view) s->bound.erase(found);
  });
  s->layouter = LinearItemsLayouter::New(); s->layouter.SetItemExtent(20); s->layouter.SetItemSpacing(2);
  s->view = RecyclerView::New(); s->view.SetRequestedWidth(120); s->view.SetRequestedHeight(50);
  s->view.SetCacheExtent(0, 0); s->view.SetItemsLayouter(s->layouter); s->view.SetAdapter(s->adapter);
  return s;
}
inline void CheckBoundIds(Run& r, const AdapterFixture& s, const char* prefix)
{
  r.Truth((std::string(prefix) + ".nonempty").c_str(), !s.bound.empty());
  for(const auto& entry : s.bound)
  {
    r.Truth(Id("bound.valid_position", entry.first).c_str(), entry.first < s.ids.size());
    if(entry.first < s.ids.size())
    {
      const auto name = entry.second.GetProperty<Dali::String>(Actor::Property::NAME);
      r.Text(Id("bound.stable_id", entry.first).c_str(), std::string(name.CStr()), Id("stable", s.ids[entry.first]));
    }
  }
}
} // namespace LayoutValidation
