/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include <dali-ui-foundation/integration-api/scroll-view-impl.h>
#include <dali-ui-foundation/public-api/views/scroll/page-scroll-view.h>
#include <dali-ui-foundation/public-api/views/scroll/scroll-view.h>
#include "layout-validation-fixtures.h"
struct ScrollState
{
  Dali::Ui::ScrollView scroll;
  Dali::Ui::View       content;
};

using namespace LayoutValidation;
using namespace LayoutValidation::Fixtures;

namespace
{
Integration::ScrollViewImpl& ScrollImpl(ScrollView view)
{
  return static_cast<Integration::ScrollViewImpl&>(view.GetImplementation());
}
void ConfigureScroll(ScrollView view, ScrollDirection direction)
{
  view.SetScrollDirection(direction);
  view.SetPanScrollEnabled(false);
  view.SetHorizontalScrollBarVisibility(ScrollBarVisibility::Never);
  view.SetVerticalScrollBarVisibility(ScrollBarVisibility::Never);
}
void AddConstraintCases(std::vector<Scenario>& out)
{
  for(unsigned bits = 0; bits < 4; ++bits) out.push_back(Single(Id("SC.match-constraints", bits).c_str(), "Each MATCH axis receives inner constraint; natural axes remain unconstrained", 18, [bits](Run& r)
  {
    auto scroll = ScrollView::New();
    ConfigureScroll(scroll, ScrollDirection::Both);
    Fixed(scroll, 100, 80);
    scroll.SetPadding({11, 17, 7, 13});
    auto p = Probed(r, 240, 200);
    p->view.SetRequestedWidth(bits & 1 ? MATCH_PARENT : WRAP_CONTENT);
    p->view.SetRequestedHeight(bits & 2 ? MATCH_PARENT : WRAP_CONTENT);
    scroll.SetContent(p->view);
    r.Size("owner.measure", scroll.Measure(500, 400), {100, 80});
    r.Near("measure.constraint.width", p->lastWidth, bits & 1 ? 72 : std::numeric_limits<float>::max(), 0);
    r.Near("measure.constraint.height", p->lastHeight, bits & 2 ? 60 : std::numeric_limits<float>::max(), 0);
    const float width = bits & 1 ? 72 : 240, height = bits & 2 ? 60 : 200;
    r.Size("content.measure", p->view.GetMeasuredSize(), {width, height});
    r.Equal("measure.producer", p->measures, 1);
    r.Rect("owner.arrange", scroll.Arrange({0, 0, 100, 80}), {0, 0, 100, 80});
    r.Rect("content.arranged", p->view, {11, 7, width, height});
    r.Near("arrange.constraint.width", p->lastWidth, bits ? width : std::numeric_limits<float>::max(), 0);
    r.Near("arrange.constraint.height", p->lastHeight, bits ? height : std::numeric_limits<float>::max(), 0);
    r.Equal("arrange.measure.producers", p->measures, bits == 1 || bits == 2 ? 2 : 1);
  }));
}
struct AxisState
{
  ScrollView scroll;
  View       content, inner;
  unsigned   mode{0};
  float      viewportWidth{100}, viewportHeight{80};
  Insets     padding{11, 17, 7, 13};
  Vector2    position{0, 0};
  float      Width() const
  {
    return mode == 0 ? 240 : std::max(0.f, viewportWidth - padding.start - padding.end);
  }
  float Height() const
  {
    return mode == 1 ? 200 : std::max(0.f, viewportHeight - padding.top - padding.bottom);
  }
  void ClampMaximum()
  {
    position = Vector2(std::max(0.f, Width() - viewportWidth), std::max(0.f, Height() - viewportHeight));
  }
  void Check(Run& r)
  {
    r.Rect("viewport", scroll, {0, 0, viewportWidth, viewportHeight});
    r.Rect("content", content, {padding.start - position.x, padding.top - position.y, Width(), Height()});
    r.Rect("nested-margin", inner, {5, 13, std::max(0.f, Width() - 14), std::max(0.f, Height() - 32)});
    const auto actual = scroll.GetScrollPosition();
    r.Near("scroll.x", actual.x, position.x);
    r.Near("scroll.y", actual.y, position.y);
    auto& impl = ScrollImpl(scroll);
    r.Near("viewport.width", impl.GetViewportWidth(), viewportWidth);
    r.Near("viewport.height", impl.GetViewportHeight(), viewportHeight);
    r.Near("scrollable.width", impl.GetScrollableWidth(), Width());
    r.Near("scrollable.height", impl.GetScrollableHeight(), Height());
    r.Equal("direction", static_cast<int>(scroll.GetScrollDirection()), static_cast<int>(mode == 0 ? ScrollDirection::Horizontal : mode == 1 ? ScrollDirection::Vertical
                                                                                                                                             : ScrollDirection::Both));
  }
};
void AddAxisCases(std::vector<Scenario>& out)
{
  for(unsigned mode = 0; mode < 3; ++mode)
  {
    Scenario scenario{Id("SC.axis-padding-margin", mode), "Axis-specific inner MATCH size, padding origin and nested asymmetric margin", {}};
    scenario.steps.push_back({"mount", 19, [mode](Run& r)
    {
      auto s    = std::make_shared<AxisState>();
      s->mode   = mode;
      s->scroll = ScrollView::New();
      ConfigureScroll(s->scroll, mode == 0 ? ScrollDirection::Horizontal : mode == 1 ? ScrollDirection::Vertical
                                                                                     : ScrollDirection::Both);
      Fixed(s->scroll, 100, 80);
      s->scroll.SetPadding(s->padding);
      s->content = Leaf(mode == 0 ? 240 : MATCH_PARENT, mode == 1 ? 200 : MATCH_PARENT);
      s->content.SetPadding({3, 5, 7, 11});
      s->inner = Leaf(MATCH_PARENT, MATCH_PARENT);
      s->inner.SetMargin({2, 4, 6, 8});
      s->content.Add(s->inner);
      s->scroll.SetContent(s->content);
      r.SetState(s);
      r.AfterLayout({s->scroll, s->content, s->inner}, [s](Run& done)
      { s->Check(done); });
      r.Attach(s->scroll);
    }});
    scenario.steps.push_back({"maximum", 19, [](Run& r)
    {auto s=r.State<AxisState>();s->ClampMaximum();s->scroll.ScrollTo(Vector2(999,999),false);s->Check(r); }});
    scenario.steps.push_back({"resize", 19, [](Run& r)
    {
      auto s            = r.State<AxisState>();
      s->viewportWidth  = 130;
      s->viewportHeight = 110;
      s->ClampMaximum();
      r.AfterLayout({s->scroll, s->content, s->inner}, [s](Run& done)
      { s->Check(done); });
      Fixed(s->scroll, 130, 110);
    }});
    scenario.steps.push_back({"padding-change", 19, [](Run& r)
    {
      auto s     = r.State<AxisState>();
      s->padding = {2, 4, 3, 5};
      s->ClampMaximum();
      r.AfterLayout({s->scroll, s->content, s->inner}, [s](Run& done)
      { s->Check(done); });
      s->scroll.SetPadding(s->padding);
    }});
    out.push_back(std::move(scenario));
  }
}
struct PageState
{
  PageScrollView    pager;
  View              content;
  std::vector<View> pages;
  bool              vertical{false};
  float             viewport{100};
  struct Event
  {
    int page;
    int count;
  };
  std::array<Event, 64> events{};
  uint32_t              eventCount{0};
  bool                  overflow{false};
  LayoutRect            Rect(float position, float length) const
  {
    return vertical ? LayoutRect(0, position, 80, length) : LayoutRect(position, 0, length, 80);
  }
  View NewPage(float length)
  {
    return Leaf(vertical ? 80 : length, vertical ? length : 80);
  }
  void Record(int page, int count)
  {
    if(eventCount < events.size())
      events[eventCount] = {page, count};
    else
      overflow = true;
    ++eventCount;
  }
  void Check(Run& r, const char* prefix, int count, int page, float offset, float length, float geometryLength = -1)
  {
    const std::string id = prefix;
    r.Equal((id + ".count").c_str(), pager.GetPageCount(), count);
    r.Equal((id + ".current").c_str(), pager.GetCurrentPage(), page);
    const auto pos = pager.GetScrollPosition();
    r.Near((id + ".scroll.main").c_str(), vertical ? pos.y : pos.x, offset);
    r.Near((id + ".scroll.cross").c_str(), vertical ? pos.x : pos.y, 0);
    r.Rect((id + ".content").c_str(), content, Rect(-offset, geometryLength < 0 ? length : geometryLength));
    auto& impl = ScrollImpl(pager);
    r.Near((id + ".scrollable").c_str(), vertical ? impl.GetScrollableHeight() : impl.GetScrollableWidth(), length);
    r.Near((id + ".viewport").c_str(), vertical ? impl.GetViewportHeight() : impl.GetViewportWidth(), viewport);
  }
  void SignalCheck(Run& r, int page, int count)
  {
    r.Truth("page.signal.buffer", !overflow && eventCount > 0 && eventCount <= events.size());
    const Event last = eventCount && eventCount <= events.size() ? events[eventCount - 1] : Event{-99, -99};
    r.Equal("page.signal.page", last.page, page);
    r.Equal("page.signal.count", last.count, count);
  }
};
void AddPageCases(std::vector<Scenario>& out)
{
  for(bool vertical : {false, true})
  {
    Scenario scenario{vertical ? "PAGE.vertical" : "PAGE.horizontal", "Layout-derived pages, viewport and explicit size, dynamic insert/remove", {}};
    scenario.steps.push_back({"mount-default-size", 14, [vertical](Run& r)
    {
      auto s      = std::make_shared<PageState>();
      s->vertical = vertical;
      s->pager    = PageScrollView::New();
      ConfigureScroll(s->pager, vertical ? ScrollDirection::Vertical : ScrollDirection::Horizontal);
      Fixed(s->pager, vertical ? 80 : 100, vertical ? 100 : 80);
      auto stack = StackLayout::New(vertical ? StackOrientation::VERTICAL : StackOrientation::HORIZONTAL);
      s->content = stack;
      Fixed(stack, vertical ? MATCH_PARENT : WRAP_CONTENT, vertical ? WRAP_CONTENT : MATCH_PARENT);
      for(float length : {100.f, 100.f, 50.f})
      {
        auto page = s->NewPage(length);
        stack.Add(page);
        s->pages.push_back(page);
      }
      s->pager.PageChangedSignal().Connect(&r, [s](int page, int count)
      { s->Record(page, count); });
      s->pager.SetContent(s->content);
      r.SetState(s);
      r.AfterLayout({s->pager, s->content}, [s](Run& done)
      {s->Check(done,"initial",3,0,0,250);s->SignalCheck(done,0,3);done.Equal("initial.signal-count",s->eventCount,1); });
      r.Attach(s->pager);
    }});
    scenario.steps.push_back({"page-boundaries", 23, [](Run& r)
    {
      auto s = r.State<PageState>();
      s->pager.ScrollToPage(-7, false);
      s->Check(r, "negative", 3, 0, 0, 250);
      s->pager.ScrollToPage(99, false);
      s->Check(r, "last-partial", 3, 2, 150, 250);
      s->SignalCheck(r, 2, 3);
    }});
    scenario.steps.push_back({"viewport-resize", 13, [](Run& r)
    {
      auto s      = r.State<PageState>();
      s->viewport = 125;
      r.AfterLayout({s->pager, s->content}, [s](Run& done)
      {s->Check(done,"resized",2,1,125,250);s->SignalCheck(done,1,2); });
      Fixed(s->pager, s->vertical ? 80 : 125, s->vertical ? 125 : 80);
    }});
    scenario.steps.push_back({"explicit-page-size", 15, [](Run& r)
    {
      auto s = r.State<PageState>();
      s->pager.SetPageSize(s->vertical ? Vector2(17, 100) : Vector2(100, 17));
      s->Check(r, "explicit", 3, 1, 125, 250);
      s->SignalCheck(r, 1, 3);
      const auto size = s->pager.GetPageSize();
      r.Near("explicit.width", size.x, s->vertical ? 17 : 100);
      r.Near("explicit.height", size.y, s->vertical ? 100 : 17);
    }});
    scenario.steps.push_back({"insert-before-current", 30, [](Run& r)
    {
      auto s        = r.State<PageState>();
      auto inserted = s->NewPage(100);
      s->content.Add(inserted);
      inserted.LowerToBottom(LayoutOrderPolicy::UPDATE);
      s->pages.insert(s->pages.begin(), inserted);
      const auto events = s->eventCount;
      s->pager.NotifyPagesInserted(0, 1);
      s->Check(r, "immediate-insert", 4, 2, 200, 400, 250);
      s->SignalCheck(r, 2, 4);
      r.Equal("insert.signal-delta", s->eventCount - events, 1);
      r.AfterLayout({s->pager, s->content}, [s](Run& done)
      {s->Check(done,"settled-insert",4,2,200,350);Identity(done,"insert.order",s->content.GetChildViewAt(1),s->pages[1]);done.Rect("old-current-local",s->pages[2],s->Rect(200,100));done.Near("old-current-visible",(s->vertical?LayoutValidation::Bounds(s->pages[2]).y:LayoutValidation::Bounds(s->pages[2]).x)-(s->vertical?s->pager.GetScrollPosition().y:s->pager.GetScrollPosition().x),0); });
    }});
    scenario.steps.push_back({"remove-before-current", 24, [](Run& r)
    {
      auto s = r.State<PageState>();
      s->content.Remove(s->pages.front());
      s->pages.erase(s->pages.begin());
      const auto events = s->eventCount;
      s->pager.NotifyPagesRemoved(0, 1);
      s->Check(r, "immediate-remove", 3, 1, 100, 300, 350);
      s->SignalCheck(r, 1, 3);
      r.Equal("remove.signal-delta", s->eventCount - events, 1);
      r.AfterLayout({s->pager, s->content}, [s](Run& done)
      { s->Check(done, "settled-remove", 3, 1, 100, 250); });
    }});
    scenario.steps.push_back({"remove-all", 24, [](Run& r)
    {
      auto s = r.State<PageState>();
      s->content.RemoveAll();
      s->pages.clear();
      const auto events = s->eventCount;
      s->pager.NotifyPagesRemoved(0, 3);
      s->Check(r, "immediate-empty", 0, -1, 0, 0, 250);
      s->SignalCheck(r, -1, 0);
      r.Equal("empty.signal-delta", s->eventCount - events, 1);
      r.AfterLayout({s->pager, s->content}, [s](Run& done)
      { s->Check(done, "settled-empty", 0, -1, 0, 0); });
    }});
    scenario.steps.push_back({"restore-default", 13, [](Run& r)
    {
      auto s = r.State<PageState>();
      s->pager.SetPageSize(Vector2(0, 0));
      auto page = s->NewPage(100);
      s->content.Add(page);
      s->pages.push_back(page);
      r.AfterLayout({s->pager, s->content}, [s](Run& done)
      {s->Check(done,"restored",1,0,0,100);s->SignalCheck(done,0,1); });
    }});
    out.push_back(std::move(scenario));
  }
}
std::shared_ptr<PageState> NewUniformPages(Run& r, bool vertical, unsigned count)
{
  auto s      = std::make_shared<PageState>();
  s->vertical = vertical;
  s->pager    = PageScrollView::New();
  ConfigureScroll(s->pager, vertical ? ScrollDirection::Vertical : ScrollDirection::Horizontal);
  Fixed(s->pager, vertical ? 80 : 100, vertical ? 100 : 80);
  auto stack = StackLayout::New(vertical ? StackOrientation::VERTICAL : StackOrientation::HORIZONTAL);
  s->content = stack;
  Fixed(stack, vertical ? MATCH_PARENT : WRAP_CONTENT, vertical ? WRAP_CONTENT : MATCH_PARENT);
  for(unsigned i = 0; i < count; ++i)
  {
    auto page = s->NewPage(100);
    stack.Add(page);
    s->pages.push_back(page);
  }
  s->pager.PageChangedSignal().Connect(&r, [s](int page, int total)
  { s->Record(page, total); });
  s->pager.SetContent(s->content);
  r.SetState(s);
  return s;
}
void MountUniformPages(Run& r, bool vertical, int current)
{
  auto s = NewUniformPages(r, vertical, 5);
  r.AfterLayout({s->pager, s->content}, [s, current](Run& done)
  {
    s->pager.ScrollToPage(current, false);
    s->Check(done, "mounted", 5, current, 100.f * current, 500);
    s->SignalCheck(done, current, 5);
    done.Equal("mounted.logical-count", s->content.GetChildViewCount(), 5);
  });
  r.Attach(s->pager);
}
struct PageNotificationCase
{
  const char* name;
  bool        insert;
  int         current;
  int         argumentIndex;
  int         argumentCount;
  unsigned    actualIndex;
  unsigned    actualCount;
  int         expectedCurrent;
};
void AddPageNotificationCases(std::vector<Scenario>& out)
{
  const PageNotificationCase inputs[] = {
    {"insert-at-current", true, 2, 2, 1, 2, 1, 3},
    {"insert-after", true, 2, 4, 1, 4, 1, 2},
    {"insert-negative-index", true, 2, -7, 1, 0, 1, 3},
    {"insert-past-end", true, 2, 99, 1, 5, 1, 2},
    {"remove-after", false, 2, 4, 1, 4, 1, 2},
    {"remove-current", false, 2, 2, 1, 2, 1, 2},
    {"remove-through-current", false, 2, 1, 3, 1, 3, 1},
    {"remove-last-current", false, 4, 4, 99, 4, 1, 3},
    {"remove-negative-index", false, 2, -7, 1, 0, 1, 1},
    {"remove-past-end", false, 2, 99, 99, 4, 1, 2},
    {"remove-all-clamped", false, 2, -7, 99, 0, 5, -1}};
  for(bool vertical : {false, true})
  {
    const std::string axis = vertical ? "vertical" : "horizontal";
    for(const auto input : inputs)
    {
      const unsigned total = input.insert ? 5 + input.actualCount : 5 - input.actualCount;
      Scenario       scenario{"PAGE.notify-" + axis + "." + input.name, "Uniform pages: notification index/count branches and visible identity", {}};
      scenario.steps.push_back({"mount", 14, [vertical, input](Run& r)
      { MountUniformPages(r, vertical, input.current); }});
      scenario.steps.push_back({"notify-and-settle", total ? 31u : 26u, [input, total](Run& r)
      {
        auto s = r.State<PageState>();
        if(input.insert)
        {
          for(unsigned i = 0; i < input.actualCount; ++i)
          {
            auto page = s->NewPage(100);
            s->content.Add(page);
            const unsigned index = input.actualIndex + i;
            if(index < s->pages.size()) page.LowerBelow(s->pages[index], LayoutOrderPolicy::UPDATE);
            s->pages.insert(s->pages.begin() + index, page);
          }
        }
        else
        {
          for(unsigned i = 0; i < input.actualCount; ++i)
          {
            s->content.Remove(s->pages[input.actualIndex]);
            s->pages.erase(s->pages.begin() + input.actualIndex);
          }
        }
        const auto  signalCount = s->eventCount;
        const float offset      = input.expectedCurrent < 0 ? 0.f : 100.f * input.expectedCurrent;
        if(input.insert)
          s->pager.NotifyPagesInserted(input.argumentIndex, input.argumentCount);
        else
          s->pager.NotifyPagesRemoved(input.argumentIndex, input.argumentCount);
        s->Check(r, "notify.immediate", total, input.expectedCurrent, offset, 100.f * total, 500);
        s->SignalCheck(r, input.expectedCurrent, total);
        r.Equal("notify.signal-delta", s->eventCount - signalCount, 1);
        r.Equal("notify.logical-count", s->content.GetChildViewCount(), total);
        const View visible = total ? s->pages[static_cast<unsigned>(input.expectedCurrent)] : View{};
        r.AfterLayout({s->pager, s->content}, [s, input, total, offset, visible](Run& done)
        {
          s->Check(done, "notify.settled", total, input.expectedCurrent, offset, 100.f * total);
          if(total)
          {
            Identity(done, "notify.visible-identity", s->content.GetChildViewAt(static_cast<unsigned>(input.expectedCurrent)), visible);
            done.Rect("notify.visible-local", visible, s->Rect(offset, 100));
            const auto bounds   = LayoutValidation::Bounds(visible);
            const auto position = s->pager.GetScrollPosition();
            done.Near("notify.visible-origin", (s->vertical ? bounds.y : bounds.x) - (s->vertical ? position.y : position.x), 0);
          }
          else
            done.Truth("notify.empty-children", s->content.GetChildViewCount() == 0 && s->pages.empty());
        });
      }});
      out.push_back(std::move(scenario));
    }
    Scenario noOp{"PAGE.notify-" + axis + ".nonpositive-count", "Nonpositive insert/remove counts preserve page state and emit no notification", {}};
    noOp.steps.push_back({"mount", 14, [vertical](Run& r)
    { MountUniformPages(r, vertical, 2); }});
    noOp.steps.push_back({"notify-noop", 48, [](Run& r)
    {
      auto s = r.State<PageState>();
      for(unsigned i = 0; i < 4; ++i)
      {
        const auto signals = s->eventCount;
        if(i < 2)
          s->pager.NotifyPagesInserted(0, i == 0 ? 0 : -3);
        else
          s->pager.NotifyPagesRemoved(0, i == 2 ? 0 : -3);
        const auto prefix = Id("noop", i);
        s->Check(r, prefix.c_str(), 5, 2, 200, 500);
        r.Equal((prefix + ".signal-delta").c_str(), s->eventCount - signals, 0);
      }
      s->SignalCheck(r, 2, 5);
      r.Equal("noop.logical-count", s->content.GetChildViewCount(), 5);
    }});
    out.push_back(std::move(noOp));
  }
}
void CheckUnmeasuredPage(Run& r, const PageState& s, const char* prefix, int count, int current, unsigned signals)
{
  const std::string id = prefix;
  r.Equal((id + ".count").c_str(), s.pager.GetPageCount(), count);
  r.Equal((id + ".current").c_str(), s.pager.GetCurrentPage(), current);
  const auto position = s.pager.GetScrollPosition();
  r.Near((id + ".scroll.x").c_str(), position.x, 0);
  r.Near((id + ".scroll.y").c_str(), position.y, 0);
  auto& impl = ScrollImpl(s.pager);
  r.Near((id + ".scrollable").c_str(), s.vertical ? impl.GetScrollableHeight() : impl.GetScrollableWidth(), 0);
  r.Near((id + ".viewport").c_str(), s.vertical ? impl.GetViewportHeight() : impl.GetViewportWidth(), 0);
  r.Equal((id + ".signal-count").c_str(), s.eventCount, signals);
}
void AddPreLayoutPageCases(std::vector<Scenario>& out)
{
  const char* modes[] = {"insert-before", "insert-after", "remove-empty"};
  for(bool vertical : {false, true})
    for(unsigned mode = 0; mode < 3; ++mode)
    {
      Scenario scenario{std::string("PAGE.pre-layout-") + (vertical ? "vertical." : "horizontal.") + modes[mode], "Notifications before the first actual layout, then real Window completion", {}};
      scenario.steps.push_back({"notify-before-layout", mode == 2 ? 48u : 41u, [vertical, mode](Run& r)
      {
        auto s = NewUniformPages(r, vertical, 1);
        CheckUnmeasuredPage(r, *s, "pending.initial", 1, 0, 0);
        const Vector2 explicitSize = vertical ? Vector2(17, 100) : Vector2(100, 17);
        s->pager.SetPageSize(explicitSize);
        CheckUnmeasuredPage(r, *s, "pending.explicit", 0, -1, 0);
        r.Near("pending.explicit.width", s->pager.GetPageSize().x, explicitSize.x);
        r.Near("pending.explicit.height", s->pager.GetPageSize().y, explicitSize.y);
        s->pager.SetPageSize(explicitSize);
        CheckUnmeasuredPage(r, *s, "pending.same-size", 0, -1, 0);
        s->pager.SetPageSize(Vector2(0, 0));
        CheckUnmeasuredPage(r, *s, "pending.default", 1, 0, 0);
        if(mode < 2)
        {
          auto page = s->NewPage(100);
          s->content.Add(page);
          if(mode == 0) page.LowerToBottom(LayoutOrderPolicy::UPDATE);
          s->pages.insert(mode == 0 ? s->pages.begin() : s->pages.end(), page);
          s->pager.NotifyPagesInserted(mode == 0 ? 0 : 1, 1);
        }
        else
        {
          s->content.Remove(s->pages.front());
          s->pages.clear();
          s->pager.NotifyPagesRemoved(0, 1);
        }
        const int count   = mode == 2 ? 0 : 2;
        const int current = mode == 2 ? -1 : mode == 0 ? 1
                                                       : 0;
        CheckUnmeasuredPage(r, *s, "pending.notified", count, current, 1);
        s->SignalCheck(r, current, count);
        r.Equal("pending.logical-count", s->content.GetChildViewCount(), count);
        if(mode == 2)
        {
          s->pager.NotifyPagesRemoved(99, 7);
          CheckUnmeasuredPage(r, *s, "pending.already-empty", 0, -1, 1);
        }
        // Deliberately remain detached: SetState owns this fixture until the attach action.
      }});
      scenario.steps.push_back({"attach-and-settle", mode == 2 ? 15u : 20u, [mode](Run& r)
      {
        auto        s       = r.State<PageState>();
        const int   count   = mode == 2 ? 0 : 2;
        const int   current = mode == 2 ? -1 : mode == 0 ? 1
                                                         : 0;
        const float offset  = current < 0 ? 0.f : current * 100.f;
        const View  visible = count ? s->pages[static_cast<unsigned>(current)] : View{};
        r.AfterLayout({s->pager, s->content}, [s, count, current, offset, visible](Run& done)
        {
          s->Check(done, "pending.settled", count, current, offset, count * 100.f);
          s->SignalCheck(done, current, count);
          done.Equal("pending.settled.logical-count", s->content.GetChildViewCount(), count);
          if(count)
          {
            Identity(done, "pending.visible-identity", s->content.GetChildViewAt(static_cast<unsigned>(current)), visible);
            done.Rect("pending.visible-local", visible, s->Rect(offset, 100));
            const auto bounds   = LayoutValidation::Bounds(visible);
            const auto position = s->pager.GetScrollPosition();
            done.Near("pending.visible-origin", (s->vertical ? bounds.y : bounds.x) - (s->vertical ? position.y : position.x), 0);
          }
          else
            done.Truth("pending.empty-children", s->pages.empty());
        });
        r.Attach(s->pager);
      }});
      out.push_back(std::move(scenario));
    }
}
} // namespace

class TcLr52 : public Case
{
public:
  TcLr52()
  : Case("LR52", "Scroll and page layout")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    Scenario              scenario;
    scenario.id          = "I05.scroll-preservation";
    scenario.description = "Scroll position survives layout and clamps when viewport changes";
    scenario.steps.push_back({"mount", 8, [](Run& r)
    {auto s=std::make_shared<ScrollState>();s->scroll=ScrollView::New();Fixed(s->scroll,100,80);s->scroll.SetScrollDirection(ScrollDirection::Both);s->scroll.SetPanScrollEnabled(false);s->scroll.SetHorizontalScrollBarVisibility(ScrollBarVisibility::Never);s->scroll.SetVerticalScrollBarVisibility(ScrollBarVisibility::Never);s->content=Leaf(240,200);s->scroll.SetContent(s->content);r.SetState(s);r.AfterLayout({s->scroll,s->content},[s](Run&done){done.Rect("viewport",done.Snapshot(s->scroll),LayoutRect(0,0,100,80));done.Rect("content",done.Snapshot(s->content),LayoutRect(0,0,240,200));});r.Attach(s->scroll); }});
    scenario.steps.push_back({"scroll-to", 6, [](Run& r)
    {auto s=r.State<ScrollState>();s->scroll.ScrollTo(Vector2(50,60),false);Vector2 p=s->scroll.GetScrollPosition();r.Near("scroll-x",p.x,50);r.Near("scroll-y",p.y,60);r.Rect("scrolled-content",s->content,LayoutRect(-50,-60,240,200)); }});
    scenario.steps.push_back({"settled-layout-replay", 6, [](Run& r)
    {auto s=r.State<ScrollState>();r.AfterLayout({s->content},[s](Run&done){done.Rect("preserved",done.Snapshot(s->content),LayoutRect(-50,-60,240,200));auto p=s->scroll.GetScrollPosition();done.Near("preserved-scroll-x",p.x,50);done.Near("preserved-scroll-y",p.y,60);});s->scroll.InvalidateArrange(); }});
    scenario.steps.push_back({"upper-clamp", 6, [](Run& r)
    {auto s=r.State<ScrollState>();s->scroll.ScrollTo(Vector2(999,999),false);auto p=s->scroll.GetScrollPosition();r.Near("maximum-x",p.x,140);r.Near("maximum-y",p.y,120);r.Rect("clamped-content",s->content,LayoutRect(-140,-120,240,200)); }});
    scenario.steps.push_back({"resize-viewport", 10, [](Run& r)
    {auto s=r.State<ScrollState>();r.AfterLayout({s->scroll,s->content},[s](Run&done){done.Rect("larger-viewport",done.Snapshot(s->scroll),LayoutRect(0,0,160,120));auto p=s->scroll.GetScrollPosition();done.Near("new-maximum-x",p.x,80);done.Near("new-maximum-y",p.y,80);done.Rect("actual-content",s->content,LayoutRect(-80,-80,240,200));});Fixed(s->scroll,160,120); }});
    scenario.steps.push_back({"negative-clamp", 6, [](Run& r)
    {auto s=r.State<ScrollState>();s->scroll.SetScrollPosition(Vector2(-5,-7));auto p=s->scroll.GetScrollPosition();r.Near("minimum-x",p.x,0);r.Near("minimum-y",p.y,0);r.Rect("origin-content",s->content,LayoutRect(0,0,240,200)); }});
    scenario.steps.push_back({"replace-smaller-content", 7, [](Run& r)
    {auto s=r.State<ScrollState>();View old=s->content;s->content=Leaf(60,40);r.AfterLayout({s->content},[s,old](Run&done){done.Rect("small-content",done.Snapshot(s->content),LayoutRect(0,0,60,40));done.Truth("old-detached",!old.GetParent());auto p=s->scroll.GetScrollPosition();done.Near("small-scroll-x",p.x,0);done.Near("small-scroll-y",p.y,0);});s->scroll.SetContent(s->content); }});
    out.push_back(std::move(scenario));
    AddConstraintCases(out);
    AddAxisCases(out);
    AddPageCases(out);
    AddPageNotificationCases(out);
    AddPreLayoutPageCases(out);
    return out;
  }
};

REGISTER_MANUAL_TEST(TcLr52)
