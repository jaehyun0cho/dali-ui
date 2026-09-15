/* Copyright (c) 2026 Samsung Electronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "layout-validation-transition-fixtures.h"
using namespace LayoutValidation;
using namespace LayoutValidation::TransitionFixtures;
class TcLr43 : public Case
{
public:
  TcLr43()
  : Case("LR43", "EXIT ghost interaction and focus")
  {
  }
  std::vector<Scenario> BuildScenarios() override
  {
    std::vector<Scenario> cases;
    for(unsigned operation = 0; operation < 4; ++operation)
      cases.push_back({"TR09.ghost-" + std::to_string(operation), "Logical/Actor child lists, repeated remove, same-parent defense and reparent", {{"mount", 4, [](Run& r)
      { Mount(r, NewState(r)); }},
                                                                                                                                                   {"start-exit", 7, [](Run& r)
      {
        if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        auto s = r.State<State>();
        Diagnostics::SetManualAnimatorTicks(s->window, true);
        CaptureLifecycle(s, .2f);
        for(auto property : {Actor::Property::SENSITIVE, Actor::Property::FOCUSABLE, Actor::Property::FOCUS_ON_TOUCH})
          s->child.SetProperty(property, true);
        s->root.Remove(s->child, RemovePolicy::ANIMATE_EXIT);
        r.Equal("ghost.logical.count", s->root.GetChildViewCount(), 0);
        r.Equal("ghost.actor.count", s->root.GetChildCount(), 1);
        r.Truth("ghost.parent.retained", s->child.GetParent() == s->root);
        for(auto property : {Actor::Property::SENSITIVE, Actor::Property::FOCUSABLE, Actor::Property::FOCUS_ON_TOUCH})
          r.Truth((std::string("ghost.interaction.disabled.") + (property == Actor::Property::SENSITIVE ? "sensitive" : property == Actor::Property::FOCUSABLE ? "focusable"
                                                                                                                                                               : "touch"))
                    .c_str(),
                  !s->child.GetProperty<bool>(property));
        r.Equal("ghost.start.once", s->starts[1], 1);
#endif
      }},
                                                                                                                                                   {"mutate-ghost", 4, [operation](Run& r)
      {
        auto s = r.State<State>();
        if(operation == 0) s->root.Remove(s->child, RemovePolicy::ANIMATE_EXIT);
        if(operation == 1) s->root.Add(s->child);
        if(operation == 2)
        {
          r.Attach(s->other);
          s->other.Add(s->child);
        }
        if(operation == 3) s->root.RemoveAll(RemovePolicy::ANIMATE_EXIT);
        r.Equal("ghost.no-duplicate-start", s->starts[1], 1);
        r.Equal("ghost.cancel.no-finish", s->finishes[1], 0);
        r.Truth("ghost.expected-parent", s->child.GetParent() == (operation == 2 ? s->other : s->root));
        r.Equal("ghost.logical-after-action", s->root.GetChildViewCount(), 0);
      }},
                                                                                                                                                   {"finish-or-cancel", 6, [operation](Run& r)
      {
        if(!r.RequireDiagnostics()) return;
#if defined(DALI_UI_LAYOUT_TEST_DIAGNOSTICS)
        auto s = r.State<State>();
        Diagnostics::SetManualAnimatorTicks(s->window, false);
        r.Delay(800, [s, operation](Run& v)
        {
          v.Equal("ghost.finish", s->finishes[1], operation == 2 ? 0 : 1);
          v.Truth("ghost.final-parent", operation == 2 ? s->child.GetParent() == s->other : !s->child.GetParent());
          for(auto property : {Actor::Property::SENSITIVE, Actor::Property::FOCUSABLE, Actor::Property::FOCUS_ON_TOUCH})
            v.Truth((std::string("ghost.interaction.restored.") + (property == Actor::Property::SENSITIVE ? "sensitive" : property == Actor::Property::FOCUSABLE ? "focusable"
                                                                                                                                                                 : "touch"))
                      .c_str(),
                    s->child.GetProperty<bool>(property));
          auto snapshot = Diagnostics::GetTransitionSnapshot(s->child);
          v.Truth("ghost.state.drained", !snapshot.exitActive);
        });
#endif
      }}}});
    cases.push_back({"TR10.focus-reentrant-reparent", "Actual focus invalidation may reparent during EXIT registration", {{"mount", 4, [](Run& r)
    { Mount(r, NewState(r)); }},
                                                                                                                          {"focused-remove", 8, [](Run& r)
    {
      auto s = r.State<State>();
      r.Attach(s->other);
      CaptureLifecycle(s);
      s->child.SetProperty(Actor::Property::FOCUSABLE, true);
      s->child.SetProperty(Actor::Property::FOCUS_ON_TOUCH, true);
      auto       focus    = FocusManager::Get();
      const auto previous = focus.GetCurrentFocusView();
      r.OnCleanup([focus, previous]() mutable
      { if(previous) focus.SetCurrentFocusView(previous); else focus.ClearFocus(); });
      r.Truth("focus.request", focus.SetCurrentFocusView(s->child));
      r.Truth("focus.actual", focus.GetCurrentFocusView() == s->child);
      focus.FocusChangedSignal().Connect(s.get(), &State::OnFocusChanged);
      s->focusReparent = true;
      s->root.Remove(s->child, RemovePolicy::ANIMATE_EXIT);
      r.Truth("focus.listener.reparent", s->child.GetParent() == s->other);
      r.Truth("focus.removed-view.not-focused", focus.GetCurrentFocusView() != s->child);
      r.Equal("focus.cancel.no-start", s->starts[1], 0);
      r.Equal("focus.cancel.no-finish", s->finishes[1], 0);
      r.Truth("focus.restore.focusable", s->child.GetProperty<bool>(Actor::Property::FOCUSABLE));
      r.Truth("focus.restore.touch", s->child.GetProperty<bool>(Actor::Property::FOCUS_ON_TOUCH));
    }}}});
    cases.push_back({"TR10.restore-before-remove-listener", "Interaction is restored before removal listeners and listener changes survive", {{"mount", 4, [](Run& r)
    { Mount(r, NewState(r)); }},
                                                                                                                                              {"remove", 1, [](Run& r)
    {
      auto s = r.State<State>();
      CaptureLifecycle(s, .2f);
      for(auto property : {Actor::Property::SENSITIVE, Actor::Property::FOCUSABLE, Actor::Property::FOCUS_ON_TOUCH}) s->child.SetProperty(property, true);
      s->root.ChildRemovedSignal().Connect(s.get(), &State::OnChildRemoved);
      s->alterAfterRemove = true;
      s->root.Remove(s->child, RemovePolicy::ANIMATE_EXIT);
      r.Equal("listener.exit.start", s->starts[1], 1);
    }},
                                                                                                                                              {"listener-result", 7, [](Run& r)
    {
      auto s = r.State<State>();
      r.Delay(750, [s](Run& v)
      {
        v.Equal("listener.restored-before-remove", s->interactionAtRemove, 7);
        v.Equal("listener.changes-visible-at-finish", s->interactionAtFinish, 0);
        for(auto property : {Actor::Property::SENSITIVE, Actor::Property::FOCUSABLE, Actor::Property::FOCUS_ON_TOUCH}) v.Truth((std::string("listener.change-preserved.") + (property == Actor::Property::SENSITIVE ? "sensitive" : property == Actor::Property::FOCUSABLE ? "focusable"
                                                                                                                                                                                                                                                                           : "touch"))
                                                                                                                                 .c_str(),
                                                                                                                               !s->child.GetProperty<bool>(property));
        v.Truth("listener.parent.empty", !s->child.GetParent());
        v.Equal("listener.finish.once", s->finishes[1], 1);
      });
    }}}});
    return cases;
  }
};
REGISTER_MANUAL_TEST(TcLr43)
