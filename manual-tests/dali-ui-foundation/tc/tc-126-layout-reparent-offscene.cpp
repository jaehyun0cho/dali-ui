/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */
#include "layout-validation-fixtures.h"

using namespace LayoutValidation;
using namespace LayoutValidation::Fixtures;

namespace
{
struct Roots
{
  View        stage, root, child;
  StackLayout left, right;
  View        a, b, c;
};
struct Offscene
{
  std::shared_ptr<Probe> probe;
  View                   stage, parent;
  std::size_t            registeredRoots{0};
};
// The window extent a WRAP_CONTENT x WRAP_CONTENT standalone 300x90 probe (no margin) is measured
// against while it is in a window; the window size is an input read at check time.
MeasuredSize WindowBound(Run& r)
{
  const auto window = r.GetWindow().GetPositionSize();
  return MeasuredSize(std::min(300.0f, static_cast<float>(window.width)), std::min(90.0f, static_cast<float>(window.height)));
}
} // namespace

class Tc126 : public Case
{
public:
  Tc126()
  : Case("TC126", "Reparent and offscene")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> out;
    out.push_back(Steps("L01.reparent", "Reparenting invalidates the old and the new intrinsic roots on screen", {
      {"mount", 8, [](Run& r) {
         auto s   = std::make_shared<Roots>();
         s->left  = StackLayout::New(StackOrientation::HORIZONTAL);
         s->right = StackLayout::New(StackOrientation::HORIZONTAL);
         s->left.SetSpacing(7);
         s->right.SetSpacing(5);
         s->a = Leaf(20, 10, "a");
         s->b = Leaf(30, 15, "b");
         s->c = Leaf(40, 20, "c");
         s->left.Add(s->a);
         s->left.Add(s->b);
         s->right.Add(s->c);
         s->right.SetRequestedY(50);
         s->stage = r.Stage(200, 100);
         s->stage.Add(s->left);
         s->stage.Add(s->right);
         r.SetState(s);
         r.AfterRender({s->stage, s->left, s->right, s->a, s->b, s->c}, [=](Run& r) {
           r.Rendered("left", s->left, LayoutRect(0, 0, 57, 15));
           r.Rendered("right", s->right, LayoutRect(0, 50, 40, 20));
         });
       }},
      {"reparent", 12, [](Run& r) {
         auto s = r.State<Roots>();
         s->right.Add(s->b);
         r.AfterRender({s->stage, s->left, s->right, s->a, s->b, s->c}, [=](Run& r) {
           r.Size("old-root", s->left.GetMeasuredSize(), MeasuredSize(20, 10));
           r.Size("new-root", s->right.GetMeasuredSize(), MeasuredSize(75, 20));
           r.Equal("old-count", s->left.GetChildViewCount(), 1);
           r.Equal("new-count", s->right.GetChildViewCount(), 2);
           r.Rendered("moved", s->b, LayoutRect(45, 0, 30, 15));
           Identity(r, "parent", View::DownCast(s->b.GetParent()), s->right);
           Identity(r, "left-child", s->left.GetChildViewAt(0), s->a);
         });
       }},
    }));
    out.push_back(Steps("L02.detached-measure", "A detached child keeps a deterministic layout and re-attaches to the rendered root", {
      {"mount", 6, [](Run& r) {
         auto s   = std::make_shared<Roots>();
         s->root  = View::New();
         s->child = Leaf(37, 19, "child");
         s->child.SetRequestedX(7);
         s->child.SetRequestedY(9);
         s->root.Add(s->child);
         s->stage = Mount(r, s->root, 100, 60);
         r.SetState(s);
         r.AfterRender({s->stage, s->root, s->child}, [=](Run& r) {
           r.Size("attached-to-root", s->root.GetMeasuredSize(), MeasuredSize(44, 28));
           r.Rendered("attached.rendered", s->root, LayoutRect(0, 0, 44, 28));
         });
       }},
      {"remove", 6, [](Run& r) {
         auto s = r.State<Roots>();
         s->root.Remove(s->child);
         r.AfterRender({s->stage, s->root}, [=](Run& r) {
           r.Size("empty", s->root.GetMeasuredSize(), MeasuredSize(0, 0));
           r.Rendered("empty.rendered", s->root, LayoutRect(0, 0, 0, 0));
         });
       }},
      {"detached", 2, [](Run& r) {
         auto s = r.State<Roots>();
         r.Size("detached-child", s->child.Measure(100, 60), MeasuredSize(37, 19));
       }},
      {"reattach", 10, [](Run& r) {
         auto s = r.State<Roots>();
         s->root.Add(s->child);
         r.AfterRender({s->stage, s->root, s->child}, [=](Run& r) {
           r.Size("reattached", s->root.GetMeasuredSize(), MeasuredSize(44, 28));
           r.Rendered("reattached.rendered", s->root, LayoutRect(0, 0, 44, 28));
           r.Rendered("child", s->child, LayoutRect(7, 9, 37, 19));
         });
       }},
    }));
    // A window-bounded standalone probe under a plain View parent, on a stage that is its own
    // window root: measured against the window while on scene. The stage is removed from the window
    // itself -- its parent is a Layer, so no View OnChildRemoved resets the subtree caches -- and
    // only the scene disconnection acts: the probe drops its caches (they were measured against the
    // window it left) and its non-standalone parent keeps its own. They are read right after the
    // removal, because reconnecting a layout root resets its whole subtree again. An off-scene parent
    // Arrange then measures the probe against the parent's 200x80 extent, and putting the stage back
    // registers the stage and the probe as tracked roots again and measures the probe against the
    // window. The stage's parent is not a View, so it is never a required view of AfterRender.
    out.push_back(Steps("L05.standalone-offscene", "A window-bounded standalone child drops its caches when its tree leaves the window, falls back to the parent extent, and re-registers on reconnection", {
      {"mount", 6, [](Run& r) {
         auto s    = std::make_shared<Offscene>();
         s->stage  = r.StageOnWindow(200, 110);
         s->parent = Container(0, 200, 80);
         s->probe  = Probed(r, 300, 90, false);
         s->probe->view.SetLayoutMode(LayoutMode::STANDALONE);
         s->probe->view.SetRequestedX(7);
         s->probe->view.SetRequestedY(9);
         s->parent.Add(s->probe->view);
         ColorizeTree(s->parent);
         s->stage.Add(s->parent);
         r.SetState(s);
         r.AfterRender({s->parent, s->probe->view}, [=](Run& r) {
           const MeasuredSize bound = WindowBound(r);
           r.Rendered("probe", s->probe->view, LayoutRect(7, 9, bound.width, bound.height));
           r.Size("probe.measured", s->probe->view.GetMeasuredSize(), bound);
         });
       }},
      {"detach", 11, [](Run& r) {
         auto               s     = r.State<Offscene>();
         const MeasuredSize bound = WindowBound(r);
         s->stage.Unparent();
         const auto v      = Diagnostic::GetViewSnapshot(s->probe->view);
         const auto parent = Diagnostic::GetViewSnapshot(s->parent);
         r.Truth("dropped.measure-cache", !v.measureCacheValid);
         r.Truth("dropped.arrange-cache", !v.arrangeCacheValid);
         r.Truth("detach.parent-cache-kept", parent.measureCacheValid && parent.arrangeCacheValid);
         r.Size("kept.measured", s->probe->view.GetMeasuredSize(), bound);
         s->parent.Arrange(LayoutRect(0, 0, 200, 80));
         r.Rect("offscene.placement", s->probe->view, LayoutRect(7, 9, 200, 80));
         r.Size("offscene.measured", s->probe->view.GetMeasuredSize(), MeasuredSize(200, 80));
       }},
      {"reattach", 7, [](Run& r) {
         auto s             = r.State<Offscene>();
         s->registeredRoots = Diagnostic::GetWindowSnapshot(r.GetWindow()).registeredRoots;
         r.GetWindow().Add(s->stage);
         r.AfterRender({s->parent, s->probe->view}, [=](Run& r) {
           const MeasuredSize bound = WindowBound(r);
           r.Equal("reattach.registered-delta", static_cast<int64_t>(Diagnostic::GetWindowSnapshot(r.GetWindow()).registeredRoots) - static_cast<int64_t>(s->registeredRoots), 2);
           r.Rendered("probe", s->probe->view, LayoutRect(7, 9, bound.width, bound.height));
           r.Size("probe.measured", s->probe->view.GetMeasuredSize(), bound);
         });
       }},
    }));
    return out;
  }
};

REGISTER_MANUAL_TEST(Tc126)
