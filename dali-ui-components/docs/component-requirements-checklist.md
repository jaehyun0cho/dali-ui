# DALi UI Component Requirements Checklist

## Purpose

This document is a fill-in-the-blank specification template. A human author fills in the component-specific slots for one concrete UI component, then hands the completed document to an LLM coding agent. The agent uses it to implement a complete, correct, DALi-idiomatic public component under `/home/jae/dali/dali-ui-claude/dali-ui-components`.

The template encodes two kinds of requirements:

- **FIXED** requirements are invariant rules that hold for every component. Do not edit, weaken, or remove them. The agent must satisfy every FIXED item regardless of component.
- **FILL-IN** requirements are component-specific. Replace each `[FILL-IN: ...]` slot with a concrete, unambiguous value for your component. If a FILL-IN item does not apply (e.g. a range item on a boolean toggle), write `N/A` plus a one-line rationale — never leave it blank.

## How to Use This Template

1. Fill in the **Component Spec Header** first. Several later items are guarded by header decisions (archetype, Style yes/no, sample yes/no); those decisions are blocking prerequisites and are referenced by id downstream.
2. Work top to bottom. Sections are ordered so that a decision is made before any later section consumes it.
3. For every FILL-IN item, write a single concrete value (one number, one enum, one policy) — never a range or a list of alternatives. Where the template gives "reference" values from external specs, pick exactly one canonical value and record it; the corresponding UTC must assert that exact value.
4. Keep all FIXED items verbatim in scope. The agent treats them as acceptance gates.
5. Every requirement is a checkbox line. The agent checks each box only when its **Accept:** condition is objectively met.

## Legend

- `- [ ] (FIXED)` — invariant rule; applies to every component; must not be altered.
- `- [ ] (FILL-IN)` — author supplies a concrete value for this component; `N/A + rationale` allowed only where genuinely inapplicable.
- **DALi:** — a faithful pointer to the concrete foundation class / API / file that satisfies the item. Cited paths are relative to the foundation tree unless noted.
- **Accept:** — an objective, preferably machine-checkable, pass condition (usually a UTC assertion or a `grep` result).
- **Refs:** — external-spec basis (WCAG / WAI-ARIA / Material 3 / Apple HIG); full URLs are collected in the References section.

---

## Component Spec Header (fill this first)

Fill every slot with a concrete value before proceeding.

- **Component name:** `[FILL-IN: PascalCase class name, e.g. Checkbox]`
- **One-line purpose:** `[FILL-IN: single sentence describing what the component does]`
- **Archetype (choose exactly one):** `[FILL-IN: one of — container/display | button/clickable | toggle (boolean selected) | radio (mutually-exclusive group member) | range/adjustable (slider/progress)]`
- **Chosen DALi base pair (determined mechanically from archetype — see table in §3):** `[FILL-IN: handle base + impl provider base]`
- **Accessibility role:** `[FILL-IN: one AccessibilityRole enum value — must match archetype per §7]`
- **Value/state model (one line):** `[FILL-IN: e.g. "boolean selected, default false" or "float value in [0,100], default 0, step 1"]`
- **Ships a per-instance Style class?** `[FILL-IN: yes | no]` (referenced downstream as "per id-03 Style")
- **Ships a runnable sample under `samples/<name>/`?** `[FILL-IN: yes | no]` (referenced downstream as "per id-03 sample")
- **Exposes properties to the DALi property system (named/indexed)?** `[FILL-IN: yes | no]` (referenced downstream as "per header property-surface decision")
- **Extra ATSPI interfaces beyond ACCESSIBILITY_\* properties (list them, or none):** `[FILL-IN: none | Value | Text | Selection | ...]` (referenced downstream by a11y-05)

### Archetype → Base Pair table (authoritative; use for the header and §3)

| Archetype | Handle base | Impl provider base | Typical role |
|---|---|---|---|
| container/display | `View` | `ViewImpl` | CONTAINER |
| button/clickable | `InteractiveView` | `Provider::InteractiveViewImpl` | BUTTON |
| toggle (boolean) | `SelectableView` | `Provider::SelectableViewImpl` | CHECK_BOX / TOGGLE_BUTTON |
| radio (group member) | `GroupSelectableView` | `Provider::GroupSelectableViewImpl` | RADIO_BUTTON |
| range/adjustable | `View` (or `InteractiveView` if it accepts pointer drag) + component-defined value/min/max/step | `ViewImpl` (or `Provider::InteractiveViewImpl`) | ADJUSTABLE / PROGRESS_BAR |

Note: `Selectable` guarantees `Interactive`; `GroupSelectable` guarantees `Selectable`. Never stack a trait a base already guarantees.

---

## Global Invariants (apply to every component)

These are FIXED and hold for every component regardless of archetype. The agent must satisfy all of them.

- [ ] (FIXED) Split every public component into a data-free public handle (`public-api/<name>.h/.cpp`) and a stateful internal impl (`internal/<name>-impl.h/.cpp`); the handle holds NO data members and forwards every method one-line via `GetImpl(*this).Xxx()`. **DALi:** rules/handle-body-pattern.md; text-button.cpp:80-88.
- [ ] (FIXED) Mark every public class `DALI_UI_API`; place the two internal ctors inside a `/// @cond internal ... /// @endcond` block. **DALi:** text-button.h:41,81-86; rules/public-api-abi.md:23-29. Note: `InteractiveView/SelectableView/GroupSelectableView` mark their internal ctors `DALI_UI_API` while `TextButton` uses `DALI_INTERNAL`; match the marker used by your chosen base pair rather than assuming one.
- [ ] (FIXED) Keep the public handle ABI-safe: non-virtual destructor, no virtual methods, no data members, and the full rule-of-five (default ctor, dtor, copy ctor, move ctor `noexcept`, copy-assign self-guarded, move-assign `noexcept`). **DALi:** view.h:135-137; text-button.h:44-56.
- [ ] (FIXED) Construct via two-phase init: `IntrusivePtr<Impl> impl(new Impl()); Handle h(*impl); impl->Initialize(); [apply style]; return h` — so `Self()` is valid inside `OnInitialize` but per-instance styling runs AFTER `Initialize`, never in the ctor. **DALi:** text-button-impl.cpp:64-72; view.cpp:46-51.
- [ ] (FIXED) In `OnInitialize()`, call the immediate base `OnInitialize()` FIRST (before building children or applying style) so the guaranteed trait is attached. **DALi:** text-button-impl.cpp:147-155; interactive-view-impl.cpp:54-58.
- [ ] (FIXED) Include foundation headers ONLY from `dali-ui-foundation/public-api/` and `provider-api/` — never `integration-api/` or `internal/`. **DALi:** rules/component-boundaries.md:16-31.
- [ ] (FIXED) In layout code (`OnArrange`/`ArrangeCallback`), arrange children in a LEFT_TO_RIGHT frame only; do NOT apply RTL mirroring yourself. The framework mirrors non-STANDALONE direct children automatically when effective direction is RIGHT_TO_LEFT. **DALi:** view-impl.cpp:1329-1333,1458-1478; view.h:239-244.
- [ ] (FIXED) Do NOT invent DALi APIs. Use ONLY verified foundation/provider APIs. `DALI_UI_VIEW_WITH` is the current fluent hook; `DALI_UI_CHAIN_VIEW_METHODS` does NOT exist. There is NO built-in wheel handler, NO touch-slop/hit-area expansion API, NO built-in pan/hover dispatch in the interactive trait, and NO dark-mode enum — theming is token-identity + `ThemeChangedSignal` only.
- [ ] (FIXED) Make every color-valued style field a `UiColor` (not `Vector4`) so it can carry a theme token resolved lazily at `GetRgba()`; semantic fields default to tokens (PRIMARY/ON_PRIMARY/SURFACE/etc.) and stay overridable. **DALi:** text-button-style.cpp:577-580; ui-color.h:56-63.
- [ ] (FIXED) Make setters idempotent: setting value/state to its current value is a no-op that emits NO change signal; validate/clamp new values before store; update internal state BEFORE the change signal fires (update-then-notify); emit exactly once per genuine change.
- [ ] (FIXED) A disabled (`Enabled=false`) control MUST suppress ALL interaction and emit NO interaction/change events (framework blocks touch, cancels pending press, releases pressed). This is distinct from pseudo-disabled (visual-only, still accepts input). **DALi:** interactive-trait-impl.cpp:106-126,292-295.
- [ ] (FIXED) Non-input-driven state changes MUST carry `InputEvent::Programmatic()`; lifecycle-driven cancellations MUST use `InputEvent::Programmatic().WithCancellation()`. **DALi:** selectable-trait-impl.cpp:72-82; interactive-trait-impl.cpp:122,162.
- [ ] (FIXED) Keep every user-visible string a plain `Dali::String` passthrough with NO in-component gettext/localization and NO components-level `po/` directory; localization is app-driven via `UiLocalizationManager`. **DALi:** text-button-impl.cpp:74-82; ui-localization-manager.h.
- [ ] (FIXED) Release registered callbacks/observers/animations/timers on teardown (`OnSceneDisconnection`/destroy); cancel in-flight animations cleanly. The trait auto-cancels a pending press on detach/disable/scene-disconnect.
- [ ] (FIXED) Perform every accessibility state write as read-modify-write on the `ACCESSIBILITY_STATES` bitset so ENABLED and other bits are preserved (`SetProperty` replaces the whole bitset). **DALi:** group-selectable-trait-impl.cpp:557-568; bit positions from the `AccessibilityState` enum in view-accessibility-enums.h (ENABLED/SELECTED/CHECKED at :27-35, BUSY:32, EXPANDED:33).
- [ ] (FIXED) Ship, per new component: a matching automated UTC file registered in `TC_SOURCES`, an auto-globbed manual `tc-*.cpp`, a mandatory `@brief` on the public class, and the new public header(s) added BY HAND to the umbrella `dali-ui-components.h`. The library `.cpp` files are globbed and need no build edit. **DALi:** CMakeLists.txt; dali-ui-components.h:32-33.

---

## 1. Identity & Scope

Establish what the component is, its archetype, and its semantic role so every downstream decision follows consistently.
**Refs:** WAI-ARIA APG role assignment; Apple HIG control role/labeling; Fluent/WinUI control-type selection.

- [ ] (FILL-IN) **id-01** — State the component's name, one-line purpose, and archetype: container/display, button/clickable, toggle (boolean selected), radio (mutually-exclusive group member), or range/adjustable (slider/progress). **DALi:** archetype selects the base pair per the Archetype→Base Pair table in the header (selectable-view.h:34-47). **Accept:** a single sentence names the component and one of the FIVE documented archetypes, and the archetype maps to exactly one row of the base-pair table.
- [ ] (FILL-IN) **id-02** — Declare the component's semantic accessibility role, and list any extra ATSPI interfaces it needs beyond `ACCESSIBILITY_*` properties (e.g. Value for a slider). **DALi:** `AccessibilityRole` enum in view-accessibility-enums.h (ROLE_START_INDEX=200; ADJUSTABLE, ALERT, BUTTON, CHECK_BOX, RADIO_BUTTON, TOGGLE_BUTTON, PROGRESS_BAR, CONTAINER, ...). **Accept:** the chosen role is named and matches the archetype (checkbox→CHECK_BOX, radio→RADIO_BUTTON, switch→TOGGLE_BUTTON, slider→ADJUSTABLE, progress→PROGRESS_BAR); extra-interface list is recorded (used by a11y-05).
- [ ] (FILL-IN) **id-03** — Record, as blocking prerequisites, whether the component ships a per-instance Style class and whether it warrants a runnable sample under `samples/<name>/`. **DALi:** Style pair public-api/styles/<name>-style.{h,cpp} + internal/styles/<name>-style-impl.h; sample dir auto-discovered by samples/CMakeLists.txt:51-59. **Accept:** a yes/no decision is recorded for BOTH Style class and sample with rationale (user-facing styled component → yes Style); every §8/§16/§17 item that depends on these reads the decision "per id-03".

---

## 2. API Surface & Naming

Fix the public method names, factories, and forwarding structure before implementation so the handle stays a thin, ABI-stable forwarder.
**Refs:** DALi public-api conventions (rules/api-naming.md); Flutter controlled-value binding; Fluent state/value model.

- [ ] (FIXED) **api-01** — Name boolean-option APIs `Set<Noun>Enabled(bool)`/`Is<Noun>Enabled() const` (or `Set<Adj>(bool)`/`Is<Adj>()`), never `EnableX`/`SetEnableX`/`GetXEnabled`. **DALi:** rules/api-naming.md:24-34,48-57; `SetEnabled(bool)` on View is the sole allowed exception. **Accept:** `grep` of the new public header finds no `EnableX`/`SetEnableX`/`GetXEnabled` identifiers.
- [ ] (FIXED) **api-02** — Provide static `New()` (parameterless) and, if id-03 Style=yes, `New(Style)` overloads, constructed through the impl's static `New()` with two-phase `Initialize()`. **DALi:** impl `New()` does `IntrusivePtr<Impl> impl(new Impl()); Ui::Foo h(*impl); impl->Initialize(); return h;`; parameterless `New()` forwards `Default()` (text-button-impl.cpp:64-72; text-button.cpp:33-55). **Accept:** UTC — `New()` returns a valid handle (`DALI_TEST_CHECK(foo)`); `New(style)` asserts the style is initialized; default ctor yields an empty handle (`!foo`).
- [ ] (FIXED) **api-03** — Declare paired `GetImpl(Ui::Foo&)` / `GetImpl(const Ui::Foo&)` inline helpers and forward every public handle method one-line into the impl. **DALi:** `inline Internal::FooImpl& GetImpl(Ui::Foo& h){ DALI_ASSERT_ALWAYS(h); return static_cast<Internal::FooImpl&>(h.GetImplementation()); }` (text-button-impl.h:79-89); handle body = `GetImpl(*this).Xxx()` (text-button.cpp:80-88). **Accept:** no public handle method contains logic beyond a single forwarding call; the handle declares no data members.
- [ ] (FILL-IN) **api-04** — Expose the archetype-specific value/state property surface and its typed change signal. **DALi:** Button — `ClickedSignal`/`PressedChangedSignal`/`LongPressedSignal` + `IsPressed`/`SetClickable` (interactive-view.h:120-259). Toggle — `SelectionChangedSignal` + `IsSelected`/`SetSelected` (selectable-view.h:120-153). Radio member — `SetGroupName`/`GetGroupName`/`GetGroup` (group-selectable-view.h:134,143,153); group-level `GetSelectedMember`/`ClearSelection`/`SelectedMemberChangedSignal` live on `SelectionGroup` obtained via `GetGroup()`/`Find` (selection-group.cpp:179,184; selection-group.h). Range — no built-in base; the component defines value/min/max/step + a `ValueChanged` signal itself. **Accept:** each intrinsic value/state has a getter, a setter (where user-settable), and a change signal carrying the new value plus an `InputEvent`; the signal fires ONLY after the impl's stored state is updated (update-then-notify), exactly once per genuine change, and a handler reading the getter inside the signal observes the new value (mirrors rb-02 for component-defined signals); UTC round-trips every getter default.
- [ ] (FIXED) **api-05** — Invoke `DALI_UI_VIEW_WITH(<Name>)` in the public handle body. **DALi:** `DALI_UI_VIEW_WITH(Foo)` (text-button.h:58; view-with.h:30); `DALI_UI_CHAIN_VIEW_METHODS` does NOT exist and MUST NOT be used. **Accept:** the handle body contains `DALI_UI_VIEW_WITH(<Name>)` and compiles; `grep` confirms no `DALI_UI_CHAIN_VIEW_METHODS`.
- [ ] (FILL-IN) **api-06** — If the header property-surface decision is yes, define a public `<Name>-properties.h` with a `<Name>PropertyIndex` struct (`PROPERTY_START_INDEX = Ui::VIEW_PROPERTY_END_INDEX + <gap>`, plus `PROPERTY_END_INDEX`) and register each property in the integration-api impl via `DALI_PROPERTY_REGISTRATION_EXTERNAL` (using the read-only / animatable variants where appropriate). If no, record the decision with rationale. **DALi:** public-api/chart/chart-view-properties.h:37-58; provider-api/property-registration-helper.h:78-84 (`DALI_PROPERTY_REGISTRATION_READ_ONLY_EXTERNAL`, `DALI_ANIMATABLE_PROPERTY_REGISTRATION_*`). **Accept:** if property-surface=yes, registration order matches the index order (`static_assert` passes), and UTC round-trips `SetProperty`/`GetProperty` by index AND by name and asserts read-only properties reject writes; if no, absence of a `-properties.h` is intentional and recorded.

---

## 3. Base Class & Inheritance

Bind the component to the correct guaranteed-trait base pair and wire the mandatory ABI/handle-impl boilerplate.
**Refs:** DALi View/InteractiveView/SelectableView/GroupSelectableView contract; Android CompoundButton/Checkable contract.

- [ ] (FILL-IN) **base-01** — Derive the public handle from the archetype handle base and the impl from the matching provider impl base, both taken mechanically from the id-01 archetype via the Archetype→Base Pair table; do not stack traits a base already guarantees. **DALi:** e.g. `class Foo : public InteractiveView` + `class FooImpl : public Provider::InteractiveViewImpl` (text-button.h:41; text-button-impl.h:32; selectable-view.h:34-47). **Accept:** UTC — the archetype base `DownCast` succeeds (e.g. `InteractiveView::DownCast(foo)` non-empty); the handle base, impl provider base, and the base-03 registration base are all the same archetype row.
- [ ] (FIXED) **base-02** — Implement static `DownCast(BaseHandle)` via the View template helper and the two internal ctors (`Impl&` and `CustomActor*` with `VerifyCustomActorPointer`). **DALi:** `return Ui::View::DownCast<Foo, Internal::FooImpl>(handle);` and `Foo::Foo(CustomActor* i):Base(i){ VerifyCustomActorPointer<Internal::FooImpl>(i); }` (text-button.cpp:57-60,150-159; view.h:2198-2233). **Accept:** UTC — `Foo::DownCast` and `Dali::DownCast<Foo>` succeed on a real handle; `DownCastN` returns empty for an unrelated `BaseHandle`.
- [ ] (FIXED) **base-03** — Register the impl type with `DALI_TYPE_REGISTRATION_BEGIN(FooImpl, <ImmediateBaseImpl>, Create)` and an anonymous `Create()` returning an empty `BaseHandle`; the registration base MUST equal the base-01 provider base for the id-01 archetype. **DALi:** `namespace{ BaseHandle Create(){return BaseHandle();} DALI_TYPE_REGISTRATION_BEGIN(FooImpl, Provider::InteractiveViewImpl, Create) DALI_TYPE_REGISTRATION_END() }` (text-button-impl.cpp:36-45). **Accept:** the registered base matches the archetype's provider base; the type registers without assertion at startup and is verifiable via `TypeRegistry`.
- [ ] (FIXED) **base-04** — Override `OnInitialize()`, chain to the immediate base FIRST, then build sub-views via `Self().Add(...)`; never touch `Self()` in the constructor. **DALi:** `void OnInitialize() override { Provider::InteractiveViewImpl::OnInitialize(); mChild = ...; Self().Add(mChild); }` (text-button-impl.cpp:147-155; view-impl.cpp:2491). **Accept:** the base `OnInitialize` is called on line 1 of the override; no `Self()` reference exists in any constructor body.

---

## 4. Properties / Value / State Model

Define the observable state machine, its transitions, defaults, and the value domain so correctness and event semantics are unambiguous.
**Refs:** WAI-ARIA state/value properties; Flutter controlled value + tristate; Fluent IsChecked/IsOn/RangeBase; HTML checked/indeterminate/range.

- [ ] (FILL-IN) **vs-01** — Enumerate every state the component can hold and its default value on `New()`; map user-settable states/values to setters. Provide a state-transition table: `{current state + trigger → next state + emitted signal}`, one row per state from this enumeration. **DALi:** built-in `ViewState` bitmask NORMAL/FOCUSED/FOCUS_INDICATED/PRESSED/DISABLED/PSEUDO_DISABLED/SELECTED (view-state.h:35-92; there is no built-in HOVERED or tri-state), custom states via `ViewState::Create(name)`, exposed via `GetState()`/`StateChangedSignal`. **Accept:** UTC asserts each getter returns its documented default immediately after `New()`; every transition-table row is driven by UTC, asserting the resulting state and the single emitted signal.
- [ ] (FILL-IN) **vs-02** — For toggle archetypes define boolean-selected semantics and toggle-by-click default; for tri-state/indeterminate define the visual-only third state (a custom `ViewState::Create(...)` + `ACCESSIBILITY_STATES` bit) and the exact tap cycle as an ordered table. **DALi:** `SelectableTrait` `IsSelected`/`SetSelected`, `IsToggleByClickEnabled`/`SetToggleByClickEnabled` (default true, auto-wired to `ClickedSignal`) (selectable-trait.h:111-155; selectable-trait-impl.cpp:47,190). Indeterminate has NO built-in tri-state — model it via a custom `ViewState` + `ACCESSIBILITY_STATES`. **Accept:** UTC — a click toggles selected when toggle-by-click is on; the tap cycle matches the documented ordered table exactly; indeterminate never participates in the submitted/emitted boolean value.
- [ ] (FILL-IN) **vs-03** — For radio archetypes guarantee the single-selection invariant and that re-selecting the current member is a no-op. **DALi:** membership is declarative — on-scene parent auto-group by default, or `SetGroupName(name)` which wins; the group is obtained (not constructed) via `GetGroup()`/`SelectionGroup::Find(name|parent)`; group-level `GetSelectedMember`/`ClearSelection`/`SelectedMemberChangedSignal` live on `SelectionGroup`, not the member (group-selectable-view.h:134,143,153; selection-group.cpp:179,184; selection-group.h). **Accept:** UTC — selecting a second member deselects the first and fires `SelectedMemberChangedSignal` once; clicking the already-selected member emits no change.
- [ ] (FILL-IN) **vs-04** — For range controls define value, min, max, step/divisions, orientation, and clamping — each as ONE concrete number (no alternatives). **DALi:** no built-in range base; store value/min/max/step on the impl. `ACCESSIBILITY_VALUE` is a STRING property for announcement only (view.h:1782-1784) and must be kept in sync on change — it is NOT a numeric model and does NOT provide the AT change path; AT increment/decrement requires overriding `ViewImpl::OnAccessibilityValueChange(bool isIncrease)` to step and return true (view-impl.h:147; view-impl.cpp:2803-2806). Canonical defaults unless the spec dictates otherwise: `min=0, max=100, step=1, value=0`. **Accept:** UTC — an out-of-range set clamps (no throw); a discrete step snaps; the getter reflects the clamped/snapped value; `ValueChanged` fires only on genuine change; UTC asserts the exact chosen numbers (e.g. `DALI_TEST_EQUALS(foo.GetMaximum(),100.0f,TEST_LOCATION)`).
- [ ] (FILL-IN) **vs-05** — Model any loading/busy state: set the `BUSY` bit in `ACCESSIBILITY_STATES` (read-modify-write, preserving ENABLED), suppress or defer primary activation while busy per spec, and make it visually distinct without relying on color alone. If none, record `N/A`. **DALi:** `AccessibilityState::BUSY` (view-accessibility-enums.h:32); model via a custom `ViewState`. **Accept:** UTC — the `BUSY` bit toggles while ENABLED is preserved, and activation is suppressed/queued while busy; or `N/A` recorded.
- [ ] (FILL-IN) **vs-06** — Model any error/validation state and any read-only state, keeping both distinct from disabled: read-only keeps the control focusable and value-announced but non-editable; error/invalid uses a non-color-only visual cue plus an AT-announced message. There is no built-in read-only in the foundation, so model and document it explicitly. If neither applies, record `N/A`. **DALi:** custom `ViewState` + `ACCESSIBILITY_*`; contrast with disabled at interactive-trait-impl.cpp:106-126. **Accept:** UTC — read-only blocks value mutation while remaining focusable/announced; disabled remains fully blocked; the read-only-vs-disabled distinction is documented; or `N/A` recorded.
- [ ] (FIXED) **vs-07** — Handle null/empty/absent inputs gracefully with documented defaults (empty label, unset value, no style) without crashing. **DALi:** Style `Default()` falls back to `DefaultPreset()` when unregistered; `StateEffect` coerces null to `StateEffect::None()` (text-button-style.cpp:57-67,549-557); text setters are plain passthrough (text-button-impl.cpp:74-82). **Accept:** UTC — `New()` with no arguments, empty text, and no style override produces a valid, non-crashing handle with documented defaults.

---

## 5. Interaction & Input

Wire touch/tap/long-press/click through the built-in trait pipeline, honor disabled vs pseudo-disabled, and emit correctly-ordered events with `InputEvent` provenance.
**Refs:** DALi InteractiveTrait pipeline; Material press/ripple state layers; WCAG 3.2.2 On Input; HTML/Qt disabled suppression.

- [ ] (FIXED) **int-01** — Consume the inherited `ClickedSignal`/`PressedChangedSignal`/`LongPressedSignal` rather than hooking `Actor::TouchedSignal` directly; rely on the trait's touch + single-tap + lazy long-press wiring. **DALi:** InteractiveTrait attaches `TouchEventSignal` + `TapGestureDetector` on attach; Clicked fires from OnTap so it runs after touch handlers (interactive-trait-impl.cpp:263-277,351-362). **Accept:** UTC — a synthesized touch down+up emits `PressedChanged(true/false)` then `Clicked` exactly once; component code registers no raw `Actor::TouchedSignal` handler.
- [ ] (FILL-IN) **int-02** — Choose and document the effective `KeyClickPolicy` default; activation keys come from `UiConfig`'s execution-key predicate (default Return). Do NOT redeclare `SetKeyClickPolicy`/`GetKeyClickPolicy` — they are already inherited from `InteractiveView` (interactive-view.h:193,201). **DALi:** `KeyClickPolicy` ON_RELEASE(default)/ON_PRESS/DISABLED; ON_RELEASE enables key long-press after the repeat threshold (key-click-policy.h:28-49; interactive-trait-impl.cpp:447-466; ui-config-impl.cpp:79-83). **Accept:** `grep` of the new public header finds no re-declared `Set/GetKeyClickPolicy`; UTC — Return key-up fires Clicked under ON_RELEASE, ON_PRESS fires on key-down, DISABLED fires no key-driven click, all via the inherited API.
- [ ] (FIXED) **int-03** — Distinguish disabled (hard block, framework-enforced) from pseudo-disabled (visual-only, still interactive); do not redundantly gate interaction handlers on enabled. **DALi:** `SetEnabled(bool)` cancels pending press + releases pressed + ignores touch; `SetPseudoDisabled(bool)` only sets `ViewState::PSEUDO_DISABLED` + emits `PseudoDisabledChangedSignal` (interactive-trait-impl.cpp:106-126,167-178,292-295). **Accept:** UTC — touch on a disabled component emits no Clicked; touch on a pseudo-disabled component still emits Clicked; disabling while pressed releases the pressed state.
- [ ] (FIXED) **int-04** — Pass `InputEvent::Programmatic()` for API-driven state changes and `Programmatic().WithCancellation()` for lifecycle cancellations; update state before emitting. **DALi:** `SetSelectedInternal(selected, InputEvent::Programmatic())`; cancellation via `InputEvent::Programmatic().WithCancellation()` (selectable-trait-impl.cpp:72-82; interactive-trait-impl.cpp:122,162,176). **Accept:** UTC — a programmatic `SetSelected` emits `SelectionChanged` whose `InputEvent.IsProgrammatic()` is true; a handler reading the value inside the signal sees the new value.
- [ ] (FILL-IN) **int-05** — Wire drag/pan for slider/range/swipeable archetypes via the component's own `Dali::PanGestureDetector` — the interactive trait has NO pan/drag dispatch (the only pan hook is `OnAccessibilityPan` for AT, view-impl.h:140). Update value from pan position, clamped to `[min,max]` and snapped to step, emitting `ValueChanged` during the drag and a final value on release; suppress drag while `Enabled=false`. If not a draggable archetype, record `N/A`. **Accept:** UTC — a synthesized pan makes the value track the drag and clamp at the ends; drag is suppressed when disabled; or `N/A` recorded.
- [ ] (FILL-IN) **int-06** — Wire hover state / hover-revealed content via the component's own `Actor::HoverEventSignal` — there is NO built-in HOVERED `ViewState` and NO trait hover dispatch. Hover feedback MUST be pointer-only (never on touch-only activation); hover-revealed content (tooltip) MUST be dismissable without moving the pointer, remain visible while hovered, and also be reachable on keyboard focus (WCAG 1.4.13). If no hover behavior, record `N/A`. **DALi:** no HOVERED state in view-state.h; hover is otherwise only used internally by focus-manager. **Accept:** UTC/manual — hover enter/leave toggles the hover visual, touch activation leaves no lingering hover state, and any tooltip is dismissable + keyboard-reachable; or `N/A` recorded.
- [ ] (FIXED) **int-07** — Do NOT implement wheel handling in the interactive path (the trait has no wheel dispatch); if wheel is needed, wire the component's own `Actor::WheelEventSignal`. **DALi:** WheelEvent is payload-only (`InputEventType::WHEEL_EVENT`); no `OnWheel` virtual in the trait (interactive-trait-impl.h:167-195). **Accept:** if wheel is required, the component wires `Actor::WheelEventSignal` explicitly; otherwise no wheel code exists.

---

## 6. Keyboard & Focus

Make the component keyboard-focusable, wire activation and directional/roving navigation, and respect the two independent highlight systems.
**Refs:** WCAG 2.1.1 Keyboard / 2.4.7 Focus Visible; WAI-ARIA APG keyboard tables & roving tabindex; Fluent/GTK/Qt focus + arrow navigation.

- [ ] (FILL-IN) **kf-01** — Interactive archetypes are ALREADY keyboard-focusable via `InteractiveTraitImpl::OnAttached`, which calls `SetFocusable(true)`/`SetTouchFocusable(true)` (interactive-trait-impl.cpp:270-271); do NOT re-set it. For a NON-interactive `View` archetype that must take focus, call `SetFocusable(true)` explicitly in `OnInitialize`. **Accept:** UTC — after `New()`, `IsFocusable()` is true (with NO component-level `SetFocusable` call for interactive archetypes); `FocusManager::SetCurrentFocusView` focuses the component.
- [ ] (FILL-IN) **kf-02** — Wire activation via the inherited `activate` AT action (connect `mAccessibilityActivateSignal` or override `OnAccessibilityActivated()`) so it produces the same effect as pointer activation. For range/adjustable archetypes ALSO override `ViewImpl::OnAccessibilityValueChange(bool isIncrease)` to step by the control's step (clamped to `[min,max]`) and return true, keeping `ACCESSIBILITY_VALUE` in sync. **DALi:** `activate` routes through `DoAction` → `PerformLegacyAccessibilityAction`; default `OnAccessibilityActivated` focuses self (view-impl.cpp:2793-2796; view-data-impl.cpp:172-188); value change at view-impl.cpp:2803-2806. **Accept:** UTC — dispatching `activate` triggers the primary action identically to a pointer click; for range archetypes, dispatching AT value-increment/decrement changes the value by exactly one step, clamps at bounds, and updates `ACCESSIBILITY_VALUE`.
- [ ] (FILL-IN) **kf-03** — Define keyboard focus order and, for composite/group widgets, arrow-key navigation (radio roving selection, slider step keys). **DALi:** directional links `SetLeft/Right/Up/Down/Forward/Backward/Clockwise/CounterClockwiseFocusableView(View)`; custom navigation via `OnFocusNavigationRequested` or `SetFocusNavigationCallback` (callback wins) (view.h:763-826; view-impl.h:995-1006). **Accept:** UTC — `MoveFocus` in each supported `FocusDirection` lands on the intended neighbor; for radio groups only one member is the tab stop and arrows move+select.
- [ ] (FILL-IN) **kf-04** — Treat the `FocusManager` keyboard-focus indicator and the AT screen-reader highlight as two independent systems. If the component draws its own focus visual, react to `ViewState::FOCUS_INDICATED` (the built-in focus-visible signal, distinct from `ViewState::FOCUSED` = "has focus", view-state.h:69-70) and suppress the default indicator via `StateEffect`. **DALi:** keyboard ring — `FocusManager::SetDefaultFocusIndicatorEnabled` + StateEffect suppression (`ViewImpl::IsDefaultFocusIndicatorSuppressedByStateEffect`, focus-manager.h:178-195; view-impl.h:539-563); AT highlight — `ViewAccessible` `GrabHighlight`/`ClearHighlight` + `AccessibilityHighlightOverlay` (view-accessible.cpp:528-615). **Accept:** a visible focus indicator appears on keyboard focus (manual); if a custom focus visual is drawn, it keys off `FOCUS_INDICATED`, the default indicator is suppressed, and no double-render occurs.

---

## 7. Accessibility

Expose role, name/description, dynamic states, and change notifications so screen-reader and keyboard users perceive and operate the control (Name-Role-Value).
**Refs:** WCAG 4.1.2 Name/Role/Value; WAI-ARIA APG per-pattern state attrs (aria-checked, aria-valuenow); Android AccessibilityNodeInfo; Apple VoiceOver traits.

- [ ] (FILL-IN) **a11y-01** — Set `ACCESSIBILITY_ROLE` in `OnInitialize` so AT announces the control's purpose and it becomes highlightable (AUTO highlightable is false for role NONE). **DALi:** `Self().SetProperty(View::Property::ACCESSIBILITY_ROLE, AccessibilityRole::<role>)`; `GetRole()` maps V2→ATSPI (BUTTON→PUSH_BUTTON); default is NONE — the reference TextButton leaves it NONE, so a semantic component MUST set one (text-anchor-impl.cpp:162-167; view-accessible.cpp:301-305). **Accept:** UTC — `ACCESSIBILITY_ROLE` round-trips the set value; the component is highlightable under AUTO (role != NONE).
- [ ] (FILL-IN) **a11y-02** — Provide an accessible NAME (and DESCRIPTION where useful); for text-bearing components override `GetNameRaw()` so label text auto-announces. **DALi:** set `ACCESSIBILITY_NAME`/`DESCRIPTION`, or connect `mAccessibilityGetName/GetDescriptionSignal`, or override `ViewAccessible::GetNameRaw()` returning `{text,true}`. Resolution order: GetName signal → `ACCESSIBILITY_NAME` → `GetNameRaw()` → Actor NAME (view-accessible.cpp:232-289). **Accept:** UTC/manual — AT announces a non-empty name; a text component announces its label text without the app setting `ACCESSIBILITY_NAME`.
- [ ] (FILL-IN) **a11y-03** — Report dynamic states (SELECTED/CHECKED/BUSY/EXPANDED plus ENABLED) via the `ACCESSIBILITY_STATES` bitset using read-modify-write, with bit positions taken from the `AccessibilityState` enum. **DALi:** `int32_t raw = Get<int32_t>(ACCESSIBILITY_STATES); Set(ACCESSIBILITY_STATES, checked ? raw|(1<<AccessibilityState::CHECKED) : raw&~(1<<AccessibilityState::CHECKED));` — the enum (view-accessibility-enums.h:27-35, e.g. BUSY:32, EXPANDED:33) is the source of the bit indices; default bitset is ENABLED-only (group-selectable-trait-impl.cpp:557-568; view-accessibility-data.cpp:91-96). **Accept:** UTC — toggling selection/checked flips the corresponding bit while ENABLED remains set; disabling clears ENABLED.
- [ ] (FILL-IN) **a11y-04** — Emit ATSPI state-change events when SELECTED/CHECKED toggles for cases the framework does not auto-emit (V1 roles, roles outside the auto set, or state changes while not highlighted). **DALi:** V2 CHECK_BOX/RADIO_BUTTON/TOGGLE_BUTTON (CHECKED) and BUTTON/LIST_ITEM/MENU_ITEM/TAB/SCROLL_BAR (SELECTED) auto-emit ONLY while highlighted; otherwise emit manually (view-accessible.cpp:742-769; view-accessibility-data.cpp:265-269). **Accept:** UTC — a selection change while highlighted (for an auto-emit role) fires a state-change event; a component whose selection can change off-highlight emits its own event.
- [ ] (FILL-IN) **a11y-05** — Override `CreateAccessibleObject()` to return a custom `ViewAccessible` ONLY when an ATSPI interface not expressible via `ACCESSIBILITY_*` properties is required (the extra-interface list recorded in id-02, e.g. Value/Text/Selection); otherwise rely on `ACCESSIBILITY_*` properties and the override MUST be absent. **DALi:** override `ViewImpl::CreateAccessibleObject()` → subclass of `Ui::ViewAccessible` overriding `GetNameRaw`/`GetDescriptionRaw`/`CalculateStates`/`InitDefaultFeatures` (view-impl.cpp:2813-2816; text-anchor-impl.cpp:170-179). **Accept:** if id-02 lists an extra interface, its overrides return correct name/state/features; if id-02 lists `none`, no `CreateAccessibleObject` override exists and property-driven a11y is verified via UTC.

---

## 8. Visual States & Styling / Theming

Build the immutable Builder-driven Style, wire `StateEffect` for interaction feedback, and keep colors as theme tokens for light/dark/high-contrast adaptation. Items st-01..st-05 apply only if id-03 Style=yes; if Style=no, record `N/A per id-03` for each.
**Refs:** DALi UiStyle/StateEffect/UiColor contract; Material 3 state layers & disabled opacities; Fluent VisualStateManager / high-contrast; Apple/Android theming tokens.

- [ ] (FILL-IN) **st-01** — (if id-03 Style=yes) Define a public style handle `DALI_UI_API <Comp>Style : public UiStyle` with a defaulted ctor, nested move-only Builder, and private ctor taking `Internal::<Comp>StyleImpl*`. **DALi:** mirror TextButtonStyle exactly (text-button-style.h:47-115); must derive `UiStyle` so `UiStyleSheet`'s `static_assert(is_base_of<UiStyle,StyleType>)` passes. **Accept:** UTC Builder round-trip — `Builder().SetX(...).Build()` then `GetX()` equality for every field; `Build()` asserts on double-consume.
- [ ] (FIXED) **st-02** — (if id-03 Style=yes) Implement `DownCast(BaseHandle)`/`StaticDownCast(UiStyle)`/`DefaultKey()`/`DefaultPreset()`/`Default()`/`Configure()`; `DefaultPreset()` and `Default()` call `DebugAssertStyleConfigApplied()` first. **DALi:** `DefaultKey`: `static UiStyleKey<T> k = UiStyleKey<T>::Alloc();`; `Default()`: `T s = UiConfig::GetCurrent().GetStyle(DefaultKey()); return s ? s : DefaultPreset();` (text-button-style.cpp:43-77); `DebugAssertStyleConfigApplied` asserts `UiConfig::HasCurrent()` (ui-style-debug.h:31-34). **Accept:** UTC — with no sheet override, `Default()` equals `DefaultPreset()`; `StyleSheet::New().GetStyle(DefaultKey())` is empty; a registered `UiStyleCreator` makes `Default()` return the override; `SetStyle` after `Apply` asserts "UiStyleSheet is frozen".
- [ ] (FILL-IN) **st-03** — (if id-03 Style=yes) Represent every color-valued field as `UiColor` defaulting to a semantic token; include a `StateEffect` field defaulting to `StateEffect::DefaultForInteractive()` and coercing null to `StateEffect::None()`. **DALi:** UiColor tokens (PRIMARY/ON_PRIMARY/SURFACE/ON_SURFACE/OUTLINE) resolve at `GetRgba()`; `StateEffect::DefaultForInteractive()` reads UiConfig default (text-button-style.cpp:36-39,549-557,577-580; ui-color.h:56-63). **Accept:** UTC — color fields keep token identity via `HasColorId()`/`GetColorId()`; `SetStateEffect(StateEffect::None())` round-trips; a theme swap re-resolves colors.
- [ ] (FILL-IN) **st-04** — (if id-03 Style=yes) Cover geometric/layout style fields: min/max width & height, corner radius (float + Vector4) + `CornerRadiusPolicy`, and `Extents` padding; apply them in `ApplyInitialStyle` after `Initialize`. **DALi:** `ApplyInitialStyle` pushes fields onto the View — `SetMinimum/MaximumWidth/Height`, `SetCornerRadius(+Policy)`, `SetPadding`, `SetBackgroundColor(UiColor)`, `SetStateEffect(...)` (text-button-impl.cpp:157-177; text-button-style.h:81-99); max defaults to `UNCONSTRAINED_MAX_SIZE=FLT_MAX`. **Accept:** UTC — after `New(style)`, the View reports the style's min/max/corner/padding; a `Configure()` override changes only the overridden fields.
- [ ] (FILL-IN) **st-05** — (if id-03 Style=yes) Fill the interaction-state visual matrix for the states your archetype supports (from vs-01) and map each to a `StateEffect`/state-layer visual; do NOT encode state by color alone. Note the state boundaries: `hover` must be self-wired via `Actor::HoverEventSignal` and is pointer-only (no built-in HOVER ViewState); `indeterminate` must be a custom `ViewState::Create(...)` + `ACCESSIBILITY_STATES`; `FOCUS_INDICATED` is the built-in focus-visible state; PRESSED/DISABLED/PSEUDO_DISABLED/SELECTED exist. Fixed reference opacities: hover 0.08, focus/pressed 0.10, disabled content 0.38. **DALi:** `SetStateEffect` drives pressed/hover/focus/disabled feedback (OverlayEffect presets Plain/Round/ListItem) (state-effect.h:37-88; overlay-effect.h:69-361); no dark-mode enum — light/dark/high-contrast is token-identity + `ThemeChangedSignal` on `UiThemeManager` (ui-theme-manager.h:88), with token re-resolution via `UiColorManager::SetColorOverride`/`ClearColorOverride`/`InvalidateCache` (ui-color-manager.h:222-281). **Accept:** UTC asserts `SetStateEffect` is called; a filled matrix `{state → StateEffect/state-layer, opacity}` exists with one row per supported state; manual test — every supported state is visually distinct, disabled is dimmed (0.38) and suppresses input, and each state stays distinguishable in grayscale (a non-color cue is present).

---

## 9. Layout & Sizing

Implement measure/arrange in visual units, honor requested/min/max sizing and padding, and guarantee an adequate interactive target size.
**Refs:** DALi Measure/Arrange template method; WCAG 2.5.8 Target Size (24×24 CSS px min); Material 48dp / Apple 44pt targets.

- [ ] (FILL-IN) **ly-01** — Override `OnMeasure(widthConstraint,heightConstraint)` returning a `MeasuredSize` in visual (scale-applied) units, honoring `GetRequestedWidth/Height` (fixed/WRAP_CONTENT/MATCH_PARENT), `GetPadding`, and `GetEffectiveScale`. **DALi:** incoming constraints are already clamped to this view's min/max and the result is re-clamped by `ApplyConstraints` — do NOT re-apply outer min/max (text-button-impl.cpp:179-228; view-impl.cpp:1115-1141). **Accept:** UTC — `Measure()`/`GetMeasuredSize()` honors padding, requested sizes, and MATCH_PARENT/WRAP_CONTENT, and reflects min/max clamping.
- [ ] (FILL-IN) **ly-02** — Override `OnArrange(bounds)` to place self and Measure+Arrange children within padding-inset content bounds in a LEFT_TO_RIGHT frame; leaf components may skip and rely on the base. **DALi:** `Self().SetProperty(POSITION_X/Y, SIZE_WIDTH/HEIGHT); child.Measure(...); child.Arrange(contentBounds)`; never mirror for RTL yourself (text-button-impl.cpp:230-251; view.h:240-259). **Accept:** UTC — children are positioned within padding-inset bounds; `SetLayoutDirection(RIGHT_TO_LEFT)` mirrors direct children automatically without component code.
- [ ] (FILL-IN) **ly-03** — Guarantee an adequate interactive target size via style min-size (there is NO touch-slop/hit-area expansion API); the touch target equals the arranged bounds. Pick ONE concrete minimum (canonical: `>=48dp` for touch controls; Apple 44pt; WCAG floor 24×24 CSS px). **DALi:** `View::SetMinimumWidth/Height` from `style.GetMinimumWidth()` (text-button-impl.cpp:161-163). **Accept:** UTC asserts the exact applied minimum size (e.g. `DALI_TEST_EQUALS(..., 48.0f, TEST_LOCATION)`); the chosen minimum meets the selected standard; adjacent targets have the recommended spacing.
- [ ] (FILL-IN) **ly-04** — For text-bearing components with bounded width (e.g. a max-width style field), choose ONE explicit overflow policy — wrap to multi-line, ellipsis-truncate, or grow — and apply it to the text child; keep the full text available to AT via the accessible name. **DALi:** `ELLIPSIS` text option (text-enumerations.h:189-192); max-width via st-04's `SetMaximumWidth`. **Accept:** UTC/manual with an over-long string at the max width shows the documented behavior (e.g. single-line ellipsis) with no clipping of the container and full text still in the accessible name; or `N/A` for non-text components.
- [ ] (FIXED) **ly-05** — Call `InvalidateMeasure()`/`InvalidateArrange()` after mutating any layout-affecting state; use `SetMeasureCallback`/`SetArrangeCallback`/`AttachLayoutManager` only if custom layout without an impl subclass is needed. **DALi:** `View::InvalidateMeasure()`; dispatch precedence in Measure/Arrange is callback > LayoutManager > OnMeasure/OnArrange (view.h:231-259; view-impl.cpp:1128-1140). **Accept:** UTC — changing a size-affecting property re-measures on the next layout pass; callbacks (if used) return visual sizes and arrange children LTR.

---

## 10. Feedback (motion / haptic / sound)

Provide immediate press/state feedback via motion, honor reduced-motion, and treat haptic/sound as optional supplementary channels.
**Refs:** Material ripple/state layers + motion tokens; WCAG 2.3.3 Animation from Interactions; Apple haptics + Reduce Motion; Android TalkBack multimodal.

- [ ] (FILL-IN) **fb-01** — Provide immediate, position-aware press/state feedback via the `StateEffect` (or a custom `StateEffectImpl`) rather than ad-hoc animation. **DALi:** `StateEffect` on the View reacts to `ViewState` changes; custom effects override `OnViewStateChanged`/`OnStateEffectTargetsChanged` (state-effect-impl.h:54-96; view.h:2163). Reference motion: standard easing `cubic-bezier(0.2,0,0,1)`, short durations 50-200ms. **Accept:** manual — pressing shows an immediate visual response; the effect follows the interaction state consistently.
- [ ] (FILL-IN) **fb-02** — (informational, not gated unless a concrete hook is added) There is NO verified reduced-motion API in the foundation. If reduced-motion is in scope, expose a settable `bool ReducedMotion` (component property) defaulting false; when true, skip non-essential animations and keep essential motion. If not in scope, record `N/A` — do not treat as a verifiable gate without the concrete flag. **DALi:** no built-in reduced-motion signal; gate on the app-provided flag (WCAG 2.3.3). **Accept:** if the flag is added, UTC asserts no animation is created when `ReducedMotion==true`; no animation runs on a disabled or destroyed component; or `N/A` recorded.
- [ ] (FILL-IN) **fb-03** — Treat haptic and sound as optional, user-suppressible supplementary channels; never make them the sole feedback and never rely on color alone. **DALi:** no dedicated haptic/sound API is verified in the foundation; if used, source it from the app/adaptor layer and keep the control fully usable without it. **Accept:** the component remains fully operable and state-distinguishable with haptics/sound disabled; state is conveyed by shape/text/icon in addition to any color.

---

## 11. Internationalization & RTL

Keep strings localizable app-side, tolerate text expansion, and mirror layout for RTL using framework auto-mirroring plus logical (not physical) placement.
**Refs:** W3C i18n text expansion; WCAG bidi/RTL mirroring; DALi layout-direction auto-mirroring; app-driven UiLocalizationManager.

- [ ] (FIXED) **i18n-01** — Keep all user-visible text a plain `Dali::String` passthrough with NO in-component gettext and NO components-level `po/` directory; localization is app-driven. **DALi:** component — `void SetText(const Dali::String&)`/`GetText()` passthrough (text-button-impl.cpp:74-82); app layer — `UiLocalizationManager::SetBindingResource(...)`/`GetLocalizedString(...)` (ui-localization-manager.h:210-304). **Accept:** `grep` finds no `gettext`/`dgettext` calls and no `po/` dir under the component; text setters are one-line passthroughs.
- [ ] (FILL-IN) **i18n-02** — Ensure the layout tolerates translated-string growth (design for ~2× English length) without truncation or clipping, and avoid fragment concatenation. **DALi:** achieved via `OnMeasure` using WRAP_CONTENT / label natural size rather than fixed widths (text-button-impl.cpp:179-228); overflow beyond the max width is governed by ly-04. **Accept:** manual test with a ~2×-length string shows no unintended clipping (any truncation is the ly-04 policy, not accidental); no user-facing sentence is built by concatenating fragments.
- [ ] (FIXED) **i18n-03** — Support RTL by relying on the framework's automatic child mirroring and logical start/end semantics; do NOT use physical left/right in arrange, and do NOT mirror direction-independent glyphs (logos, time, media). **DALi:** `ApplyLayoutDirection` mirrors non-STANDALONE direct children about `parentWidth` when effective direction is RIGHT_TO_LEFT; `OnArrange` must lay out LTR only (view-impl.cpp:1329-1333,1458-1478; view.h:239-244). **Accept:** UTC/manual — `SetLayoutDirection(RIGHT_TO_LEFT)` mirrors the component's children correctly; directional affordances mirror while logos/time icons do not.
- [ ] (FILL-IN) **i18n-04** — For components with directional-affordance icons (back/forward chevrons, next/prev, submit arrows, progress direction), mirror/swap the glyph itself under RIGHT_TO_LEFT — position-mirroring alone leaves the glyph pointing the wrong way — while keeping direction-independent glyphs (logos, media play, clock, checkmarks) un-mirrored. If no directional icons, record `N/A`. **Accept:** UTC/manual — after `SetLayoutDirection(RIGHT_TO_LEFT)` each directional glyph is mirrored/swapped and each direction-independent glyph is not; or `N/A` recorded.

---

## 12. Robustness & Edge Cases

Guarantee idempotent setters, correct event ordering, disabled suppression, and graceful handling of degenerate inputs.
**Refs:** cross-cutting robustness (idempotence, event ordering); HTML/Flutter disabled/null-callback idioms; WCAG predictable On Input.

- [ ] (FIXED) **rb-01** — Make setters idempotent: setting value/state to its current value is a no-op that emits no change signal; new values are clamped/normalized before store and emit. **DALi:** follow the trait pattern — `SetSelected` only emits when the value actually changes (selectable-trait-impl.cpp:72-82); range setters clamp to `[min,max]` in the component impl. **Accept:** UTC — setting the current value again emits nothing; an out-of-range set clamps rather than throwing; the getter reflects the clamped value.
- [ ] (FIXED) **rb-02** — Enforce update-then-notify ordering and exactly-once emission per genuine change; disabled suppresses all emission. **DALi:** ViewImpl dispatches lifecycle/input into the trait, which updates state then emits; the disabled path early-returns in OnTouch and cancels pending press (interactive-trait-impl.cpp:167-178,292-295). **Accept:** UTC — a signal handler reading the value sees the post-change value; one interaction yields one emit; a disabled component emits no interaction/change events.
- [ ] (FILL-IN) **rb-03** — Guarantee correct behavior under rapid repeated activation (fast double-tap / key auto-repeat producing back-to-back genuine changes): no dropped/duplicated emit, no handler re-entrancy corruption. If the spec calls for debounce/throttle, document the collapse window and collapse a burst within it to a single settled emission; otherwise emit one genuine change per activation. **DALi:** no built-in debounce; key auto-repeat governed by `SetMinimumKeyRepeatCount` (ui-config-impl.h:175). **Accept:** UTC — N rapid successive activations produce exactly N genuine change emissions in correct alternating order with state consistent after each (or, if debounced, a single settled emission per documented window).
- [ ] (FIXED) **rb-04** — Handle degenerate inputs (empty-handle downcast, empty label, unset/absent style, null `StateEffect`) without crashing, with documented defaults. **DALi:** `DownCastN` returns empty for an unrelated handle; Style `Default()` falls back to preset; `SetStateEffect(null)` → `StateEffect::None()` (text-button-style.cpp:57-67,549-557). **Accept:** UTC covers empty-handle DownCast, no-style `New()`, and empty text without assertion or crash.

---

## 13. Performance & Lifecycle

Release callbacks/animations/observers on teardown, cancel in-flight work, and avoid unnecessary layout/animation work.
**Refs:** cross-cutting lifecycle/memory cleanup; DALi scene connection/disconnection dispatch.

- [ ] (FIXED) **pl-01** — Release registered callbacks/observers/timers and cancel in-flight animations on scene disconnection/destroy; do NOT retain strong references from long-lived system observers back to the component. **DALi:** ViewImpl dispatches `OnSceneConnection`/`OnSceneDisconnection` into the trait, which cancels its pending press automatically; component-owned subscriptions must be released in the corresponding override (call base) (view-impl.cpp:405-411,2915-2917; interactive-trait-impl.cpp:281-287). **Accept:** UTC/manual — disconnecting and destroying the component fires no callbacks afterward; no leak of observers registered by the component.
- [ ] (FIXED) **pl-02** — Do NOT manually forward key/focus/scene/enabled lifecycle into the trait (ViewImpl already dispatches); override `On*` virtuals only for extra behavior and call the base. **DALi:** dispatch set — `OnSceneConnection`, `OnKeyEvent`, `OnFocusChanged`, `OnSceneDisconnection`, `SetEnabled`→`OnEnabledChanged` (view-impl.cpp:405-411,447-457,721-723,2915-2917,3195-3197). **Accept:** no component override re-forwards these into the trait; any override present calls its base.
- [ ] (FILL-IN) **pl-03** — Skip unnecessary animation/relayout work on disabled and (if the fb-02 flag exists) reduced-motion paths. **DALi:** gate custom animation on enabled-state and the fb-02 `ReducedMotion` flag; use `InvalidateMeasure` only when layout-affecting state actually changes (view.h:231-259). **Accept:** UTC/manual — no animation or relayout is scheduled while disabled or (if the flag exists) when `ReducedMotion==true` and the change is non-visual.

---

## 14. ABI & Conventions

Enforce file layout, license headers, include grouping, boundary rules, and umbrella registration required for a conforming public component.
**Refs:** DALi public-api-abi / component-boundaries / handle-body-pattern rules.

- [ ] (FIXED) **abi-01** — Add `#pragma once` and the Apache-2.0 "Copyright (c) 2026 Samsung Electronics Co., Ltd." license header to every new `.h`; add the same header (without `#pragma once`) to every new `.cpp`. **DALi:** header line 1 = `#pragma once` then the 18-line license block; `.cpp` begins with the license block (text-button.h:1-18; text-button.cpp:1-16). **Accept:** every new file has the correct license header; headers have `#pragma once`, `.cpp` files do not.
- [ ] (FIXED) **abi-02** — Order includes into labeled groups: `// CLASS HEADER`, `// EXTERNAL INCLUDES` (`dali/...` + std), `// INTERNAL INCLUDES` (`dali-ui-foundation/...` + `dali-ui-components/...`). **DALi:** impl `.cpp` leads with `// CLASS HEADER` of its own `-impl.h`; public headers use only INTERNAL + EXTERNAL groups (text-button-impl.cpp:18-28; text-button.h:20-26). **Accept:** include blocks are comment-labeled and grouped as specified.
- [ ] (FIXED) **abi-03** — Include foundation headers only from `public-api/` and `provider-api/`; never `integration-api/` or `internal/`. **DALi:** `#include <dali-ui-foundation/provider-api/interactive-view-impl.h>` and `public-api/...` only (component-boundaries.md:16-31). **Accept:** `rg -n '#include <dali-ui-foundation/(integration-api|internal)/'` over the component returns zero hits.
- [ ] (FIXED) **abi-04** — Add the new public header(s) (component and any style/properties header) BY HAND to the umbrella `dali-ui-components.h`; do NOT edit the library CMake source list (globbed). **DALi:** umbrella INTERNAL INCLUDES block is hand-edited (dali-ui-components.h:32-33); the library globs sources recursively (build/tizen/dali-ui-components/CMakeLists.txt:5-9). **Accept:** the umbrella includes the new header(s); the component builds without any library CMake source-list edit.

---

## 15. Testing

Provide the automated UTC coverage matrix, registered in `TC_SOURCES`, covering construction, downcast, copy/move, properties, interaction, layout, styling, and accessibility.
**Refs:** DALi automated-tests conventions (utc-Dali-TextButton); framework state/keyboard/accessibility test coverage.

- [ ] (FIXED) **test-01** — Create `automated-tests/src/dali-ui-components/utc-Dali-<Name>.cpp` and add it to `SET(TC_SOURCES ...)` (not globbed); open each test with `UiTestApplication(Components::UiConfig::New())` and end with `END_TEST`. **DALi:** `utc_dali_<name>_startup/cleanup` + cases mirroring utc-Dali-TextButton.cpp (utc-Dali-TextButton.cpp:24-77; CMakeLists.txt:8-21). **Accept:** the UTC file is listed in `TC_SOURCES` and the suite builds and passes.
- [ ] (FIXED) **test-02** — Cover the standard matrix: `ConstructorP` (`!handle`), `NewP` + every overload, primary property get/set, `CopyAndMoveP` + `AssignmentP` (original `!handle` after move), `DownCastP` (`Type::DownCast` and `Dali::DownCast<Type>`) + `DownCastN` (empty), and every getter default. **DALi:** `DALI_TEST_CHECK`/`DALI_TEST_EQUALS(..., TEST_LOCATION)` (utc-Dali-TextButton.cpp:34-153). **Accept:** all standard-matrix cases pass; each documented default is asserted.
- [ ] (FILL-IN) **test-03** — Add interaction coverage the reference omits: touch-driven `Clicked`/`PressedChanged`, key activation per `KeyClickPolicy` (ON_RELEASE/ON_PRESS/DISABLED), `LongPressed` consuming `Clicked` (return true), `Enabled=false` vs `SetPseudoDisabled` input behavior, and (where applicable) the rb-03 rapid-toggle burst, int-05 pan, and int-06 hover paths. **DALi:** contracts at interactive-trait.h:104-140 and interactive-trait-impl.cpp:290-466. **Accept:** UTC verifies each supported interaction path emits (or suppresses) signals per the documented policy.
- [ ] (FILL-IN) **test-04** — Add measure/arrange coverage (padding, `RequestedWidth/Height`, MATCH_PARENT/WRAP_CONTENT, min/max clamping, RTL auto-mirroring, ly-04 overflow) and, if id-03 Style=yes, style Builder/Configure/DefaultKey coverage. **DALi:** view-impl.cpp:1115-1141,1458-1478; style tests mirror `UtcDaliTextButtonStyle*` (utc-Dali-TextButton.cpp:166-277). **Accept:** UTC asserts measured sizes, RTL mirroring, and (if styled) Builder round-trip + Default/DefaultKey resolution + frozen-sheet assertion.
- [ ] (FILL-IN) **test-05** — Add accessibility coverage: `ACCESSIBILITY_ROLE` round-trip, `ACCESSIBILITY_STATES` bit changes with ENABLED preserved, keyboard focusability, the `activate` action triggering the primary action, and (for range archetypes) `OnAccessibilityValueChange` stepping + `ACCESSIBILITY_VALUE` sync. **DALi:** `ACCESSIBILITY_ROLE` round-trip (view-data-impl.cpp:1673-1676); states read-modify-write (group-selectable-trait-impl.cpp:557-568); `SetFocusable`→`KEYBOARD_FOCUSABLE` (view-impl.cpp:1072-1075); activate action (view-data-impl.cpp:172-188); value change (view-impl.cpp:2803-2806). **Accept:** UTC asserts role round-trip, state-bit changes preserving ENABLED, focusability, activation routing, and (range) AT value stepping.
- [ ] (FILL-IN) **test-06** — If the header property-surface decision is yes, add property-system coverage: `SetProperty`/`GetProperty` round-trip by index AND by name for each registered property, and read-only properties reject writes. If no, record `N/A per header property-surface decision`. **DALi:** api-06 registration. **Accept:** UTC round-trips each property both ways and asserts read-only rejection; or `N/A` recorded.

---

## 16. Documentation

Provide Doxygen docs on the public surface and document style resolution ordering and preconditions.
**Refs:** DALi docs-and-wiki rules; existing view-header Doxygen style.

- [ ] (FIXED) **doc-01** — Add a mandatory `@brief` on every public class; add `@brief`/`@param`/`@return` on factories and non-trivial public methods; mark internal ctors under `/// @cond internal ... /// @endcond`. **DALi:** mirror existing headers (text-button.h:38-86; interactive-view.h:261-277). **Accept:** every public class has a `@brief`; `New()`/`DownCast` and non-trivial methods are documented; internal ctors are inside a `@cond internal` block.
- [ ] (FILL-IN) **doc-02** — (if id-03 Style=yes) Document `DefaultKey()`/`DefaultPreset()`/`Default()` semantics and the pre-Apply-register / post-Apply-resolve ordering with the `DebugAssertStyleConfigApplied()` precondition. **DALi:** text-button-style.h:56-75; ui-style-debug.h:31-34. **Accept:** the Style header documents the resolution order and that preset/`Default` require `UiConfig::HasCurrent()`; or `N/A per id-03`.

---

## 17. Samples & Manual Tests

Provide an auto-globbed manual tc exercising the public API and, optionally, a runnable sample.
**Refs:** DALi manual-tests + samples conventions.

- [ ] (FILL-IN) **sm-01** — Add `manual-tests/dali-ui-components/tc/tc-<name>-basics.cpp` subclassing `ManualTest::TestCase` (and `ConnectionTracker` if wiring signals), overriding `GetName`/`GetDescription`/`OnEnter` and ending with `REGISTER_MANUAL_TEST(...)`. **DALi:** `tc/*.cpp` is auto-globbed (no CMake edit); wire signals via `signal.Connect(this, lambda)` (tc-text-button-basics.cpp:159-172,366; manual-tests CMakeLists.txt:16). **Accept:** the manual tc builds, registers, and demonstrates the component's public API and sizing modes.
- [ ] (FILL-IN) **sm-02** — (if id-03 sample=yes) Create `samples/<name>/` with its own `CMakeLists.txt` (`PKG_CHECK_MODULES dali2-ui-components`) and an example `.cpp`; if id-03 sample=no, the directory MUST be absent. **DALi:** auto-discovered by samples/CMakeLists.txt:51-59; model on samples/dialog (samples/dialog/CMakeLists.txt:1-13). **Accept:** if added, the sample directory is auto-discovered and builds against `dali2-ui-components`; if no, absence is intentional and recorded.

---

## 18. Localization

Confirm the no-in-component-localization rule and app-driven binding path.
**Refs:** app-driven UiLocalizationManager; W3C i18n externalized strings.

- [ ] (FIXED) **loc-01** — Confirm no localization logic ships inside the component: text is plain `Dali::String`; the application performs localization via `UiLocalizationManager` bindings; no `dali-ui-components/po` directory is created. **DALi:** app layer — `UiLocalizationManager::SetBindingResource(target,bindingId,resourceId,LocalizedStringCallback)` / `GetLocalizedString(resourceId)` (ui-localization-manager.h:132,210-304). **Accept:** the component exposes localizable text as a plain string setter and adds no `po/` directory or gettext calls; app-side binding is documented where relevant.

---

## Worked Example — "Checkbox" Component

This is the template filled in for a concrete toggle-archetype checkbox, showing the expected input shape. FIXED items are satisfied by the SelectableView base pair; only the component-specific FILL-IN answers are shown.

### Component Spec Header (filled)

- **Component name:** `Checkbox`
- **One-line purpose:** A binary control that toggles a boolean selected state on click and announces its checked state to assistive technology.
- **Archetype:** `toggle (boolean selected)`
- **Chosen DALi base pair:** handle `SelectableView`, impl `Provider::SelectableViewImpl` (Selectable guarantees Interactive).
- **Accessibility role:** `AccessibilityRole::CHECK_BOX`
- **Value/state model:** boolean `selected`, default `false`; optional visual-only `indeterminate` third state (see vs-02 answer).
- **Ships a per-instance Style class?** `yes` (user-facing styled control: box/check colors, corner radius, state layers).
- **Ships a runnable sample?** `yes` (`samples/checkbox/`).
- **Exposes DALi property-system properties?** `no` (state is exposed via `IsSelected`/`SetSelected` + `SelectionChangedSignal`; no serialization/animation-by-index need). Recorded per api-06.
- **Extra ATSPI interfaces:** `none` (CHECKED state is expressible via `ACCESSIBILITY_STATES`; no custom Accessible needed → a11y-05 override absent).

### Key FILL-IN answers

- **id-01:** "Checkbox is a toggle-archetype control mapping to the `SelectableView`/`Provider::SelectableViewImpl` base pair." → maps to exactly one table row. ✔
- **id-02:** role `CHECK_BOX`; extra ATSPI interfaces: `none`.
- **id-03:** Style = yes; sample = yes.
- **api-04:** inherits `IsSelected`/`SetSelected`/`SelectionChangedSignal` from `SelectableView`; adds `SetIndeterminate(bool)`/`IsIndeterminate()` for the third state; `SelectionChanged` fires after the stored boolean is updated, once per genuine change.
- **api-06:** `no` property surface — rationale recorded (no serialization/animatable-by-index requirement).
- **base-01:** `class Checkbox : public SelectableView`; `class CheckboxImpl : public Provider::SelectableViewImpl`. base-03 registers `DALI_TYPE_REGISTRATION_BEGIN(CheckboxImpl, Provider::SelectableViewImpl, Create)` — same archetype row.
- **vs-01:** states `{NORMAL, FOCUS_INDICATED, PRESSED, SELECTED, DISABLED, PSEUDO_DISABLED, indeterminate(custom)}`; default on `New()` = NORMAL, not selected. Transition table rows include `{unchecked + click → checked, emit SelectionChanged(true)}` and `{checked + click → unchecked, emit SelectionChanged(false)}`.
- **vs-02:** boolean selected; toggle-by-click default true (inherited). Tri-state tap cycle (ordered): `unchecked → checked → indeterminate → unchecked`; indeterminate is a custom `ViewState::Create("Indeterminate")` + the AT `CHECKED` bit is cleared while indeterminate; indeterminate never participates in the emitted boolean.
- **vs-03/vs-04/vs-05/vs-06:** `N/A` — not a radio, range, loading, or read-only/error control (rationale recorded for each).
- **int-02:** `KeyClickPolicy` = ON_RELEASE (default); Return toggles on key-up. Inherited `Set/GetKeyClickPolicy` not redeclared.
- **int-05/int-06:** `N/A` — no drag; no hover-revealed content (a checkbox needs no pointer-only hover affordance beyond the st-05 hover state layer).
- **kf-01:** already keyboard-focusable via the trait's `OnAttached`; no component `SetFocusable`.
- **kf-02:** `activate` action toggles selected identically to a click; no `OnAccessibilityValueChange` (not a range control).
- **kf-04:** default focus indicator used; no custom focus visual, so no `FOCUS_INDICATED` StateEffect suppression.
- **a11y-01:** `Self().SetProperty(ACCESSIBILITY_ROLE, AccessibilityRole::CHECK_BOX)` in `OnInitialize`.
- **a11y-03:** on selection change, read-modify-write `ACCESSIBILITY_STATES` toggling `(1<<AccessibilityState::CHECKED)` while preserving ENABLED.
- **a11y-04:** CHECK_BOX auto-emits the CHECKED state-change only while highlighted; since selection can change off-highlight (programmatic `SetSelected`), the impl emits the state-change event manually in that path.
- **st-01..st-05:** `CheckboxStyle : public UiStyle` with box/check `UiColor` tokens (`OUTLINE` for the empty box, `PRIMARY` for the checked fill, `ON_PRIMARY` for the checkmark), corner radius, padding, and a `StateEffect` defaulting to `DefaultForInteractive()`. State matrix rows: enabled/hover(0.08)/focus(0.10)/pressed(0.10)/selected/disabled(0.38), plus indeterminate; each mapped to a state-layer overlay; the checkmark vs empty box gives a non-color cue in grayscale.
- **ly-01/ly-02:** `OnMeasure` sizes the box + optional label within padding in visual units; `OnArrange` places them LTR (framework mirrors for RTL).
- **ly-03:** minimum touch target = `48dp` (canonical Material touch minimum); UTC asserts `48.0f`.
- **ly-04:** if a text label is present with a bounded width, overflow policy = single-line `ELLIPSIS`, full text preserved in the accessible name; otherwise `N/A`.
- **i18n-02:** label uses WRAP_CONTENT; tolerates ~2× growth. **i18n-04:** `N/A` — the checkmark is direction-independent and must not mirror.
- **rb-03:** rapid double-toggle emits exactly two alternating `SelectionChanged` events (true then false); no debounce specified.
- **fb-01:** press/state feedback via the style's `StateEffect`. **fb-02:** `N/A` reduced-motion (no non-essential animation). **fb-03:** `N/A` haptic/sound.
- **pl-03:** no custom animation to skip while disabled.
- **test-03/04/05:** interaction (touch/key toggle, disabled vs pseudo-disabled), measure/arrange + RTL + 48dp min + style Builder round-trip, and a11y (CHECK_BOX role round-trip, CHECKED bit toggling with ENABLED preserved, focusability, `activate` toggles). **test-06:** `N/A` (no property surface).
- **sm-01:** `tc-checkbox-basics.cpp` demonstrating check/uncheck, indeterminate cycle, disabled, and RTL. **sm-02:** `samples/checkbox/` builds against `dali2-ui-components`.

---

## References

- WCAG 2.1/2.2 Success Criteria: 2.1.1 Keyboard, 2.3.3 Animation from Interactions, 2.4.7 Focus Visible, 2.5.8 Target Size (Minimum, 24×24 CSS px), 1.4.13 Content on Hover or Focus, 3.2.2 On Input, 4.1.2 Name, Role, Value — https://www.w3.org/TR/WCAG22/
- WAI-ARIA Authoring Practices Guide (patterns: checkbox, radio group, slider; roving tabindex; aria-checked / aria-valuenow) — https://www.w3.org/WAI/ARIA/apg/
- Material Design 3 — state layers & interaction states (hover 8%, focus/pressed ~10-12%, disabled content 38%), touch target 48dp — https://m3.material.io/foundations/interaction/states
- Apple Human Interface Guidelines — controls, 44pt minimum hit target, haptics, Reduce Motion — https://developer.apple.com/design/human-interface-guidelines/
- W3C Internationalization — text expansion / string externalization — https://www.w3.org/International/