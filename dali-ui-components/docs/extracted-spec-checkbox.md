# CheckBox — Extracted-Spec Design Document (Architect Stage)

**Component:** `Dali::Ui::CheckBox` (new public component, `dali-ui-components`)
**Repo root:** `/home/jae/dali/dali-ui-claude` (`git rev-parse --show-toplevel` verified)
**Scope:** Binary checked/unchecked, box glyph + optional trailing label, SVG-resource-driven visuals.
**Deliverable:** design/spec only — NO source files. Verification target: **behavior-compatible (Tier 1, GPU-free) ONLY**, no pixel/GUI comparison.

---

## 1. Reference Audit

| Item | Status |
|---|---|
| Target design language | **One UI 8** standalone CheckBox (control leads, optional label trailing; box + label form one hit target). |
| Material available | One UI 8 official interaction/anatomy semantics (state set, activation, a11y role, RTL mirroring, min touch target). |
| Per-state coverage available | NORMAL, PRESSED, FOCUS_INDICATED, SELECTED (checked), DISABLED, PSEUDO_DISABLED — semantics known. |
| **Gaps (deferred numerics)** | Exact One UI hex colors, box size, box↔label gap, corner radius, tick stroke, motion durations, and the concrete **SVG asset filenames** are NOT available yet. Every concrete number/filename below is marked **verify needed** — none is frozen as a public default. |
| Corner-radius / outline-width / tick geometry | **Baked into the SVG asset**, not DALi numeric style fields (confirmed scope). |
| Color resolution | Through `UiColor` **semantic tokens** (`ui-color.h`), not raw hex, except where a token cannot match (raw `UiColor(0xRRGGBB,a)` fallback exists). |

**Fidelity target:** behavior-compatible / structural (**Tier 1**). We verify state transitions, signal order, click/key toggle, a11y role + CHECKED bit, RTL mirroring by framework, min-touch-target via style min-size, and per-state SVG resource-swap logic. **No rendered-pixel or GUI screenshot comparison is performed** (no Tier 2/3) — deferred numerics and SVG assets are not yet available, and GPU rasterization is out of the structural harness.

---

## 2. Component Archetype

| Layer | Type | Base |
|---|---|---|
| Handle | `class Dali::Ui::CheckBox` | `public Dali::Ui::SelectableView` (`selectable-view.h`, `DALI_UI_VIEW_WITH(SelectableView)` verified) |
| Impl | `class Internal::CheckBoxImpl` | `public Provider::SelectableViewImpl` (`provider-api/selectable-view-impl.h`) |
| Style | `class Dali::Ui::CheckBoxStyle` | `public UiStyle` |
| Style impl | `class Internal::CheckBoxStyleImpl` | `public Provider::UiStyleImpl` |

**Chain:** `SelectableView : InteractiveView : View`. Choosing `SelectableView` is deliberate: a checkbox is fundamentally a two-state selectable, and `SelectableView` **guarantees the Interactive layer beneath it** (focusable, touch/tap activation) via the inherited `InteractiveTrait` — `InteractiveTraitImpl::OnAttached` sets `SetFocusable(true)`/`SetTouchFocusable(true)` and wires touch+tap automatically (interactive-trait-impl.cpp:270-271). It also supplies the whole selection contract we reuse verbatim: `IsSelected/SetSelected/SelectionChangedSignal/IsToggleByClickEnabled/SetToggleByClickEnabled` (all verified present in `selectable-view.h`).

**A11y role:** `AccessibilityRole::CHECK_BOX` (verified `view-accessibility-enums.h`, enum value 203). The base does **not** set this role and does **not** set the a11y CHECKED bit — that is the impl's added responsibility (see §9).

**Confirmed-vs-live-API note:** all handle symbols above are re-confirmed against live source in this session; `SelectionChangedSignal()` returns `Signal<void(View,bool,InputEvent)>&`, and the five selection methods exist with the stated signatures. No confirmed-but-unverified symbol is relied upon.

---

## 3. State Matrix

**States that apply** (all are `ViewState` values, `public-api/types/view-state.h`, driven through `Integration::View::SetState`):

| State | ViewState | How it arises | Visual response |
|---|---|---|---|
| NORMAL | (none set) | default | unchecked box SVG |
| FOCUS_INDICATED | 70 | keyboard focus | StateEffect overlay |
| PRESSED | 71 | touch/key-press | StateEffect overlay |
| SELECTED (checked) | 74 | `SetSelected(true)` / toggle | swap to checked box SVG + CHECKED a11y bit |
| DISABLED | 72 | app disables | StateEffect DISABLED overlay; toggle suppressed |
| PSEUDO_DISABLED | 73 | ancestor-disabled | same as DISABLED overlay |
| composites | SELECTED_PRESSED(78), DISABLED_SELECTED(79) | framework-derived | overlay + checked artwork |

**Explicitly EXCLUDED:**
- **Indeterminate / tri-state** — user-descoped. State model is BINARY only. No `SetIndeterminate`, no third `ViewState`, no third SVG. `IsSelected()` is the single source of truth.
- **RADIO / mutually-exclusive** — N/A: this is a standalone independent checkbox; no `SelectionGroup`/`GroupSelectableView` is used (those are for radio semantics).
- **RANGE / adjustable (value)** — N/A: a checkbox has no continuous value; no `AccessibilityRole::ADJUSTABLE`.
- **BUSY / EXPANDED** — N/A: no async-load or disclosure semantics on the control itself.

---

## 4. Visual Token Table

Geometry (corner radius, outline width, tick stroke) is **baked into the SVG assets**, NOT expressed as DALi numeric fields. Colors resolve via `UiColor` tokens; tint is a **multiply** on a neutral/white-authored SVG (`SetImageColor` → visual `mixColor`, verified `image-view.h` `SetImageColor(const UiColor&)`).

| Param | Value | Unit | Source | Confidence | Applies To | DALi setter | Notes |
|---|---|---|---|---|---|---|---|
| Checked box fill | `UiColor::PRIMARY` (control-activated accent) | token | ui-color.h:56-63 | High (token) / hex **verify needed** | `mBox` (checked/filled asset) tint | `mBox.SetImageColor(UiColor)` | Matches interactive accent (TextButton uses PRIMARY). |
| Unchecked box outline | `UiColor::OUTLINE` | token | ui-color.h | High (token) / hex **verify needed** | `mBox` (unchecked/outline asset) tint | `mBox.SetImageColor(UiColor)` | Outline width is in the SVG. |
| Tick (checkmark) | `UiColor::ON_PRIMARY` | token | ui-color.h | High (token) / hex **verify needed** | **separate `mTick` ImageView** (checkmark asset), shown only when checked | `mTick.SetImageColor(UiColor)` | Dedicated tick layer so fill (PRIMARY) and tick (ON_PRIMARY) tint **independently** — one `mixColor` multiply cannot carry two colours on one monochrome asset. See §11. |
| Label text | `UiColor::ON_SURFACE` | token | ui-color.h | High (token) / hex **verify needed** | trailing label | pushed to child `Ui::Label` via style | Label color follows surface. |
| Disabled | *(not a token)* | — | text-button-impl.cpp:174 | High | all | `View::SetStateEffect(style.GetStateEffect())` | DISABLED handled by StateEffect overlay, not a separate color token. |
| Unchecked box asset | `<box-unchecked>.svg` | filename | — | **verify needed** | `mBox` glyph | `mBox.SetResourceUrl(String)` | Auto-routes `.svg`→SvgVisual (visual-factory-impl.cpp:182). Author neutral/white for tint. |
| Checked box asset | `<box-checked>.svg` | filename | — | **verify needed** | `mBox` glyph | `mBox.SetResourceUrl(String)` | Swap on selection change (outline→filled). |
| Tick asset | `<tick>.svg` | filename | — | **verify needed** | `mTick` glyph | `mTick.SetResourceUrl(String)` | Checkmark shape; `mTick` visibility toggled with checked state. |

**Tint semantics caveat (verify needed):** `mixColor` is a straight multiply — correct for monochrome glyphs. Because the box and tick are **separate single-colour layers**, each multiply-tinted by its own token, fill and tick recolour independently. Author each asset monochrome/white; if any asset ships multi-colour, tint will not recolour it — verify per-asset once filenames land. *(Simpler fallback if assets ship **pre-composed**: a single `mBox` whose checked asset bakes both fill and tick — then the ON_PRIMARY tick row becomes descriptive-only with no style setter, and `mTick` is dropped. The two-layer design above is the default because it keeps both colours as live theme tokens; the choice is finalised when the concrete assets land.)*

---

## 5. Layout Metric Table

Min-size is enforced automatically by the framework: `Measure()` calls `OnMeasure()` then floors to `GetMinimumWidth()*s` / `*Height()*s` in `ApplyConstraints` (view-impl.cpp:1180, 1728-1729). OnMeasure need not enforce it. Box geometry/radius live in the SVG, not here.

| Param | Value | Unit | Source | Confidence | Applies To | DALi setter | Notes |
|---|---|---|---|---|---|---|---|
| Box size (side) | `boxSide` (fixed square) | dp | One UI 8 | **verify needed** | box `ImageView` layout size | style box-size token → child requested W/H | Icon given fixed layout size so SVG rasterizes crisp at that px. |
| Box↔label gap | `gap` | dp | One UI 8 | **verify needed** | space between box and label | `CheckBoxStyle` gap accessor | Applied `*scale` in Arrange. |
| Min touch target | `minW × minH` (e.g. 48dp-equiv) | dp | One UI 8 / a11y | **verify needed** | whole control hit area | `View::SetMinimumWidth/Height` via `ApplyInitialStyle` | Guarantees touch min (view.h:591,605; enforced view-impl.cpp:1728). No HitArea API exists. |
| Label overflow policy | ellipsize / wrap | enum | One UI 8 | **verify needed** | trailing label | child `Ui::Label` config | Delegated to Label; label measured in remaining content width. |
| Padding | `padTop/Right/Bottom/Left` | dp | One UI 8 | **verify needed** | content inset | `GetPadding()` honored in Measure/Arrange | Same idiom as TextButton (text-button-impl.cpp:183-185). |

**Measure/Arrange sketch (two-child: leading box + optional trailing label), mirroring TextButton idioms (text-button-impl.cpp:179-251):**
- *Measure:* compute scale `s=GetEffectiveScale()`, visual padding, `contentVis*`. `boxSide` is fixed (style/min-height), independent of constraints. Give the label `labelAvailW = max(0, contentVisW − boxSide − gap*s)`; `labelSize = GetImpl(mLabel).Measure(labelAvailW, contentVisH)`. WRAP_CONTENT → `resultVisW = boxSide + gap*s + labelSize.width + visPadW`, `resultVisH = max(boxSide, labelSize.height) + visPadH`. MATCH_PARENT/requested branches identical to TextButton.
- *Arrange:* set self POSITION/SIZE from `bounds`; inset content by padding*s. Place box `{content.x, content.y+(content.height−boxSide)/2, boxSide, boxSide}`; place label `{content.x+boxSide+gap*s, content.y, content.width−boxSide−gap*s, content.height}`. Call `.Measure` then `.Arrange` on each child via `GetImpl(child)`. Return `MeasuredSize(bounds.width, bounds.height)`.
- **Box-only** (no text set): label child omitted/zero-width; `resultVisW = boxSide + visPadW`.
- **RTL:** leading/trailing **placement** is mirrored by the framework's layout direction (no manual swap in component code) — verified by absence of any manual RTL handling in TextButton; assert in UTC (§12). The **checkmark artwork itself is direction-INDEPENDENT and must NOT mirror** (checkmark is a direction-independent glyph — component-requirements-oneui8-porting.md:225); because the tick lives as static SVG artwork inside `mTick`, layout direction flips only the box/label *positions*, never the tick glyph.

---

## 6. Motion Table

All durations **verify needed**; per the pre-decided One UI 8 motion policy, transition durations are **clamped to a 100 ms floor / 500 ms ceiling** (component-requirements-oneui8-porting.md:222, :186). Only that policy *bound* is cited — the specific check/uncheck value is **not** invented here. Reduced-motion honored by framework/StateEffect.

| Param | Value | Unit | Source | Confidence | Applies To | DALi setter | Notes |
|---|---|---|---|---|---|---|---|
| Check ↔ uncheck cross-fade | `t_swap` (policy bound [100,500]) | ms | policy: oneui8-porting.md:222,186 | **verify needed** | box swap / tick fade | color-only via animatable `mixColor`, or asset swap | Prefer color-only cross-fade (mixColor is animatable, visual-properties.h:80-84) — cheaper, no visual rebuild. |
| Press feedback | StateEffect default | — | text-button-style.cpp | Medium | PRESSED overlay | `StateEffect::DefaultForInteractive()` | Declarative; framework-timed. |
| Focus indicate | StateEffect default | — | — | Medium | FOCUS_INDICATED overlay | StateEffect | — |
| Reduced motion | honor system | — | framework | **verify needed** | all | inherited | If asset shapes differ between states, cross-fade may fall back to instant swap under reduced-motion — verify. |

**Note:** if only the color differs between states, animate `mixColor` (no `SetResourceUrl`); if the shape differs (tick appears), `SetResourceUrl` rebuilds the visual (image-view-impl.cpp:888-959) and a cross-fade would need overlapping visuals — **verify feasibility** once assets land.

---

## 7. Interaction & State Machine

**Exact click→toggle signal order** (verified `selectable-trait-impl.cpp`):

1. User click → toggle-by-click flips: `SetSelectedInternal(!mSelected)`.
2. Short-circuit if unchanged (no-op, no emit).
3. **Store** `mSelected = selected` (l~97).
4. **`Integration::View::SetState(owner, ViewState::SELECTED, selected, event)`** (l98) — commits SELECTED state, drives StateEffect + raises a11y SELECTED bit.
5. **Internal commit observer** runs — the CheckBox impl reacts in its `SelectionChangedSignal` handler (step 6): **swap the box artwork** (`mBox.SetResourceUrl`/`SetImageColor`), **toggle the tick layer** (`mTick` visibility/opacity), and **mirror the CHECKED bit** (read-modify-write of `ACCESSIBILITY_STATES`, §9).
6. **`mSelectionChangedSignal.Emit(owner, selected, event)`** (l111) — public `SelectionChangedSignal` fires last, exactly once, with the committed `bool`.

The impl connects **`SelectionChangedSignal()` in `OnInitialize`** to perform the SVG swap + CHECKED mirror; there is **no virtual `OnSelectedChanged` hook** (confirmed absent — the ABI-frozen virtual block has no state hook), so a signal connection is the correct mechanism.

**Key toggle:** execution key → `HandleKeyPressed/Released` → `OnClicked` → `mClickedSignal.Emit` → `OnClickedForToggle` flips selected (interactive-trait-impl.cpp:369-371 → selectable-trait-impl.cpp). **Default execution key = `"Return"` only** (`DefaultExecutionKeyPredicate`, ui-config-impl.cpp:35-38); Space activates only if the app overrides the predicate. **Default `KeyClickPolicy::ON_RELEASE`** — toggle fires on Return **release** (ui-config-impl.cpp:79; interactive-trait-impl.cpp:452-454).

**Disabled suppression:** when DISABLED/PSEUDO_DISABLED, the interactive layer suppresses click/activation, so no toggle and no `SelectionChangedSignal` — assert in UTC.

**Transition table:**

| From | Input | To | Emits |
|---|---|---|---|
| unchecked | click / Return-release | checked | `SelectionChanged(view, true, event)` once |
| checked | click / Return-release | unchecked | `SelectionChanged(view, false, event)` once |
| any | `SetSelected(same)` | (unchanged) | **no emit** (short-circuit) |
| disabled | click / key | (unchanged) | **no emit** |

**Rapid double-toggle:** each genuine transition emits once; a re-entrant `SetSelected` during the commit cascade emits the local `selected` of that transition (not a re-read of `mSelected`), preventing lost/duplicated bool — verified by the commit-observer comment (selectable-trait-impl.cpp:104-110). Two fast clicks = two emits (true then false); a click that lands on the same state (programmatically pre-set) short-circuits.

---

## 8. Behavior Mapping (One UI CheckBox → DALi mechanism)

| Observable One UI behavior | DALi mechanism | Cited API |
|---|---|---|
| Tap toggles checked state | inherited toggle-by-click | `SetToggleByClickEnabled` (selectable-view.h); flip in selectable-trait-impl.cpp |
| Return key toggles | execution-key → clicked → toggle | interactive-trait-impl.cpp:369-371; ui-config-impl.cpp:35-38 |
| Box + label are one hit target | single View, label is a child measured inside content box | text-button-impl.cpp:197 idiom |
| Checked shows tick, unchecked shows empty box | per-state SVG resource swap | `ImageView::SetResourceUrl(String)` (image-view.h); SvgVisual routing visual-factory-impl.cpp:182 |
| Accent fill when checked | token tint | `ImageView::SetImageColor(UiColor::PRIMARY)` (image-view.h) |
| Announced as "checkbox, checked/not checked" | role + CHECKED bit → AT | `ACCESSIBILITY_ROLE=CHECK_BOX`; `ACCESSIBILITY_STATES` CHECKED bit; view-accessible.cpp:749-753 |
| Disabled: dimmed, non-interactive | StateEffect DISABLED overlay + interactive suppression | `SetStateEffect(StateEffect::DefaultForInteractive())` (text-button-impl.cpp:174) |
| Pressed / focus feedback | declarative StateEffect | `View::SetStateEffect` |
| RTL: control on right, label on left | framework layout-direction mirroring | no component code (verified TextButton has none) |
| Min touch target respected | style min-size, floored by framework | `SetMinimumWidth/Height` (view.h:591,605); view-impl.cpp:1728 |

*(One UI is named only in this analysis doc; no external-framework name will appear in source strings/identifiers.)*

---

## 9. Accessibility Plan

- **Role:** in `OnInitialize` (after `SelectableViewImpl::OnInitialize()` first), set `Self().SetProperty(View::Property::ACCESSIBILITY_ROLE, static_cast<int32_t>(AccessibilityRole::CHECK_BOX))`. Pattern verified at group-selectable-trait-impl.cpp:518 (RADIO_BUTTON analogue); enum verified `view-accessibility-enums.h`.
- **CHECKED bit mirroring:** mirror `selected → CHECKED` via **read-modify-write** preserving other bits (ENABLED):
  `raw = GetProperty<int32_t>(ACCESSIBILITY_STATES); SetProperty(ACCESSIBILITY_STATES, checked ? raw|CHECKED_MASK : raw&~CHECKED_MASK)` where `CHECKED_MASK = 1 << int(AccessibilityState::CHECKED)`. Verified reference `WriteCheckedState` (group-selectable-trait-impl.cpp:557-569, mask l45). The STATES property is a whole-bitset INTEGER replaced wholesale, so RMW is mandatory. Perform from the `SelectionChangedSignal` handler.
- **Programmatic vs highlighted emit — resolved for the role gate; one residual runtime dependency:** writing `ACCESSIBILITY_STATES` fires the actor `PropertySetSignal` → `OnStatePropertySet` → emits `Accessibility::State::CHECKED` **iff** the CHECKED bit changed AND role ∈ {CHECK_BOX, RADIO_BUTTON, TOGGLE_BUTTON} (view-accessible.cpp:742-763). This is **not** highlight-gated — correcting the prior a11y-04 note ("auto-emits only while highlighted"). `IsRoleV2(CHECK_BOX)` is **true** (CHECK_BOX=203, within [200, MAX_COUNT); view-accessible.cpp:112-116), so the V2 auto-emit path applies. **Residual (verify at runtime — §13-7):** the emit only reaches AT if an **accessible object** for the actor already exists to receive the `PropertySetSignal`; off-highlight with no AT client attached, that object may not be instantiated. Static reading proves the role gate but **cannot** prove the object exists on every path, so no blanket "no manual emit ever" claim is made — kept as verify-needed.
- **Announced phrases:** "Checked" / "Not checked" derived by AT from role + CHECKED state (no custom string needed for the state; label text supplies the name).
- **Focusability:** inherited (InteractiveTrait sets focusable/touch-focusable on attach, interactive-trait-impl.cpp:270-271).
- **Activation:** activate = click (Return by default; §7).
- **Never color-only:** the **tick shape** (present in the checked state via `mTick`) is the non-color state cue — distinguishable without color perception.
- **Direction-independent checkmark:** under RTL only the box/label *positions* mirror; the tick glyph inside the box is **not** mirrored (component-requirements-oneui8-porting.md:225) — asserted in §12.
- **200% text scaling:** label is a child `Ui::Label`, measured within the content box each layout pass; scaling honored via `GetEffectiveScale()` in Measure/Arrange.
- **Min touch target:** guaranteed by style min-size (see §5).

---

## 10. Public API Proposal (+ ABI delta)

**Handle `Dali::Ui::CheckBox : public Dali::Ui::SelectableView`** — holds no data, no virtual methods, non-virtual dtor; every method one-line-forwards `GetImpl(*this).X()`; full rule-of-five (copy delegates to base, copy-assign self-guarded, move `=default`). Mirrors TextButton ABI discipline.

```cpp
class DALI_UI_API CheckBox : public SelectableView {
public:
  static CheckBox New();                          // default style
  static CheckBox New(CheckBoxStyle style);
  static CheckBox New(const Dali::String& text);          // box + label
  static CheckBox New(const Dali::String& text, CheckBoxStyle style);
  CheckBox();
  ~CheckBox();                                    // non-virtual
  CheckBox(const CheckBox&); CheckBox(CheckBox&&) noexcept;
  CheckBox& operator=(const CheckBox&); CheckBox& operator=(CheckBox&&) noexcept;
  static CheckBox DownCast(BaseHandle handle);
  DALI_UI_VIEW_WITH(CheckBox)   // declares ONLY the With(action,...) extension hook (view-with.h:30-36) — NOT fluent chaining, NOT handle/impl plumbing

  void          SetText(const Dali::String& text);
  Dali::String  GetText() const;
  // (optional) label-gap accessor is exposed via CheckBoxStyle, not here.

  // Not intended for application developers:
  /// @cond internal
  explicit DALI_INTERNAL CheckBox(Internal::CheckBoxImpl& impl);
  explicit DALI_INTERNAL CheckBox(Dali::Internal::CustomActor* internal);
  /// @endcond
};
```

**INHERITED — deliberately NOT redeclared** (used as-is from `SelectableView`): `IsSelected()`, `SetSelected(bool)`, `SelectionChangedSignal()`, `IsToggleByClickEnabled()`, `SetToggleByClickEnabled(bool=true)`. Redeclaring them would duplicate ABI surface; the checkbox *is-a* selectable.

**No DALi property-index enum** for state. Rationale (recorded): state is exposed through **methods + signal** (`SetSelected`/`IsSelected`/`SelectionChangedSignal`), consistent with `SelectableView`'s contract; a `Property::CHECKED` index would create a second, redundant mutation path and risk divergence from the trait-committed `mSelected`. Type registration still occurs (below) but no public property enum is added.

**`CheckBoxStyle : public UiStyle`** — move-only `Builder`, `DefaultKey()/DefaultPreset()/Default()/DownCast/StaticDownCast/Configure()` (mirrors `TextButtonStyle`). Builder setters (all `&`/`&&` overloaded):
- checked-fill `UiColor` (default `PRIMARY`), unchecked-outline `UiColor` (default `OUTLINE`), tick `UiColor` (default `ON_PRIMARY`), label `UiColor` (default `ON_SURFACE`)
- box size, box↔label gap, min-width/min-height, padding
- per-state SVG resource URLs (unchecked/checked) — **filenames deferred**
- `StateEffect` (default `DefaultForInteractive()`)
- corner radius / outline width / tick stroke are **NOT** style fields (baked in SVG) — explicitly excluded.

**ABI delta:** additive only — new public headers `check-box.h`, `check-box-style.h`; one umbrella-header include line. No change to existing types.

---

## 11. Internal Implementation Plan

**`Internal::CheckBoxImpl : public Provider::SelectableViewImpl`.**

- **Type registration:** `BaseHandle Create(){ return BaseHandle(); }` then
  `DALI_TYPE_REGISTRATION_BEGIN(CheckBoxImpl, Provider::SelectableViewImpl, Create) DALI_TYPE_REGISTRATION_END()`. `Create()` returns an empty handle; instances built via `New(style)`. (Base is `SelectableViewImpl`, unlike TextButton's `InteractiveViewImpl`.)
- **Two-phase `New(style)`** (mirrors text-button-impl.cpp:64-72):
  `IntrusivePtr<CheckBoxImpl> impl(new CheckBoxImpl()); Ui::CheckBox handle(*impl); impl->Initialize(); impl->ApplyInitialStyle(style); return handle;`
- **`OnInitialize` (base-first):**
  1. `SelectableViewImpl::OnInitialize()` **first**.
  2. Set `ACCESSIBILITY_ROLE = CHECK_BOX` on `Self()`.
  3. Create the **box layer**: `mBox = Ui::ImageView::New(); Self().Add(mBox);` set initial unchecked SVG via `mBox.SetResourceUrl(uncheckedBoxUrl)` + `mBox.SetImageColor(outlineToken)`.
  4. Create the **tick layer**: `mTick = Ui::ImageView::New(); Self().Add(mTick);` `mTick.SetResourceUrl(tickUrl)` + `mTick.SetImageColor(onPrimaryToken)`; start hidden (`mTick.SetProperty(Actor::Property::VISIBLE, false)`) — the initialized default is **unchecked**. (Separate layer so fill+tick tint independently — §4.)
  5. `SelectionChangedSignal().Connect(this, &CheckBoxImpl::OnSelectionChanged)`.
  - **No label is created here.** The initialized default is **box-only**; `mLabel` is created lazily in `SetText` (two-phase: `OnInitialize` runs *before* any `New(text)`→`SetText`, so it cannot see constructor text — the earlier draft's "if constructed with text" step was removed as it contradicted the construction order).
- **State swap handler `OnSelectionChanged(view, selected, event)`:**
  - swap the box artwork: `mBox.SetResourceUrl(selected ? checkedBoxUrl : uncheckedBoxUrl)` + `mBox.SetImageColor(selected ? fillToken : outlineToken)` (if outline/fill differ by colour only, animate `mixColor` instead of `SetResourceUrl` — no visual rebuild, image-view-impl.cpp:612-625);
  - toggle the tick layer: `mTick.SetProperty(Actor::Property::VISIBLE, selected)` (or animate its opacity for the cross-fade);
  - mirror CHECKED bit via RMW on `ACCESSIBILITY_STATES` (§9).
- **`OnMeasure` / `OnArrange`:** leading box (`mBox`, with `mTick` occupying the **same** box rect, overlaid/centered) + optional trailing label per §5, honoring `GetPadding()/GetEffectiveScale()/GetRequestedWidth/Height()`, delegating to children via `GetImpl(mBox).Measure/Arrange`, `GetImpl(mTick).Measure/Arrange` (tick rect == box rect), and `GetImpl(mLabel).Measure/Arrange`, returning `MeasuredSize`. Layout width counts the box once — the tick overlays it and adds no width.
- **`ApplyInitialStyle(style)`:** push `SetMinimumWidth/Height`, padding, gap; set initial `mBox` + `mTick` SVG URLs + `SetImageColor` tokens; `Self().SetStateEffect(style.GetStateEffect())`; apply label colour to `mLabel` when present.
- **`SetText/GetText`:** create/populate `mLabel` lazily; box-only when text is empty; `InvalidateMeasure` on change.

**`Internal::CheckBoxStyleImpl : public Provider::UiStyleImpl`** — plain-value members with in-class defaults (UiColor tokens + `StateEffect::DefaultForInteractive()`), default ctor seeding tokens, copy ctor, protected `~() override`, `GetImpl` helpers via `GetBaseObject()`. Mirrors `TextButtonStyleImpl`.

**Build touch-points:**
- **ONE manual edit:** add the two new public headers to the umbrella `dali-ui-components/dali-ui-components.h` (currently ends at text-button.h; insert `check-box.h` and `styles/check-box-style.h` include lines).
- CMake `GLOB_RECURSE` (build/tizen/dali-ui-components/CMakeLists.txt:6) auto-compiles new `.cpp`; public headers auto-installed. No `file.list`.

**Exact new-file list** (design names only — files NOT created in this stage):
- `dali-ui-components/public-api/check-box.h` / `.cpp`
- `dali-ui-components/internal/check-box-impl.h` / `.cpp`
- `dali-ui-components/public-api/styles/check-box-style.h` / `.cpp`
- `dali-ui-components/internal/styles/check-box-style-impl.h` (+ `.cpp` if needed)

---

## 12. Verification Plan (Tier 1 ONLY — behavior-compatible, GPU-free)

UTC suite `utc-Dali-CheckBox.cpp` (automated-tests), structural assertions only:

1. **Role SET + round-trip:** after `New()`, `GetProperty<int32_t>(ACCESSIBILITY_ROLE) == int(CHECK_BOX)`.
2. **CHECKED bit toggles, ENABLED preserved:** set an ENABLED bit, `SetSelected(true)` → CHECKED set AND ENABLED still set; `SetSelected(false)` → CHECKED cleared, ENABLED intact (asserts the RMW).
3. **Toggle via click:** simulate ClickedSignal / touch → `IsSelected()` flips.
4. **Toggle via key:** inject Return (respecting ON_RELEASE) → `IsSelected()` flips; assert Space does **not** toggle under default predicate.
5. **SelectionChanged fires once per genuine change, correct bool + order:** connect handler; assert exactly one call with `(view, true, event)` then `(view, false, event)`; `SetSelected(sameValue)` fires **zero** times.
6. **SVG resource swap:** assert the `mBox` URL equals `uncheckedBoxUrl` when unchecked and `checkedBoxUrl` when checked, AND `mTick` VISIBLE is `false` when unchecked / `true` when checked — i.e. the per-state swap logic ran. (Asset content not rendered; only the URL / visibility properties are compared.)
7. **Disabled suppresses toggle:** DISABLE → click/key → `IsSelected()` unchanged, zero SelectionChanged emits.
8. **Min touch target:** measured size floored to sourced min; `GetMinimumWidth/Height` == style value; a measure with tiny constraint still yields ≥ min.
9. **Measure/Arrange geometry:** box+label layout — box is fixed square, label placed after gap; box-only case width == box+padding; **RTL mirroring** — under RTL layout direction, box/label *positions* mirror without component code, while the **tick artwork is NOT mirrored** (assert `mTick` stays overlaid on the box with unchanged local orientation/scale — direction-independent glyph).
10. **Style Builder round-trip:** set each token/size via Builder → read back equal.
11. **Box-only vs box+label:** `New()` yields box-only (no label child / zero-width label); `New(text)` yields label present with text; `SetText("")` reverts to box-only.

**No rendered/pixel comparison is performed** (no Tier 2/3). Justification: concrete colors, sizes, radii, motion timings and SVG asset files are deferred/unavailable, so there is no ground-truth image to diff; and SVG rasterization is a GPU/adaptor path outside the structural harness. All verified behaviors above are observable without rendering. Pixel/GUI fidelity is a documented FUTURE pass once numerics + assets land.

---

## 13. Unknowns / Verify-Needed (consolidated)

**Deferred numerics (no invented defaults):**
1. Checked-fill / unchecked-outline / tick / label exact **hex** values behind the tokens (One UI 8).
2. **Box size**, box↔label **gap**, **padding**, **min touch target** dp values.
3. Corner radius / outline width / tick stroke — baked in SVG; confirm the SVG encodes them at the intended box size.
4. Check/uncheck **motion duration** (clamp 100–500 ms) and press/focus timing; reduced-motion fallback behavior.

**Assets:**
5. Concrete **SVG filenames** for unchecked box and checked box (and whether tick is a separate asset or baked into the checked asset).
6. Whether the checked asset is **monochrome** (multiply-tint works) or **multi-color** (tint would not recolor) — pick monochrome/white authoring for token tinting.

**Mechanism verifications (mostly resolved this session):**
7. **Resolved for the role gate; residual runtime dependency = verify needed:** `IsRoleV2(CHECK_BOX)` is true and the CHECKED emit is **not** highlight-gated (view-accessible.cpp:112-116, 742-763), so no manual `EmitStateChanged` is expected. Residual: the emit reaches AT only if an **accessible object exists** to receive the STATES `PropertySetSignal` — confirm at runtime that off-highlight `SetSelected` with an AT client attached actually emits; if not, add a targeted manual emit.
8. **Verify needed:** whether the shipped default `UiConfig` preset (not the hard-coded fallback) installs a Return+Space execution-key predicate — code-level fallback is **Return-only** (ui-config-impl.cpp:35-38). If Space activation is required, the preset or app must set the predicate.
9. **Verify needed:** SvgVisual re-rasterization cost on dynamic box resize (static-SVG caching not audited; `REDRAW_IN_SCALING_*` apply to AnimatedVector, not static SVG). Relevant only if the CheckBox is frequently resized.
10. **Verify needed (build):** no existing `dali-ui-components` component consumes `Ui::ImageView` yet — CheckBox is the first integration; confirm by building the first wire-up.
11. **Verify needed:** cross-fade feasibility when state artwork differs by shape (asset swap rebuilds the visual, image-view-impl.cpp:888-959) vs color-only animation of `mixColor` (preferred).

---

## Review & Revision Log (Architect → Adversarial Reviewer)

This spec was produced by an Architect stage and then independently, adversarially reviewed. Reviewer verdict: **revise** (2 major + 4 minor). All findings were applied before sign-off:

| # | Sev | Section | Finding | Applied fix |
|---|---|---|---|---|
| 1 | major | 10 | Class sketch omitted `DALI_UI_API` + the two `DALI_INTERNAL` ctors, and mislabelled `DALI_UI_VIEW_WITH` as "plumbing". | Added `DALI_UI_API`, both internal ctors under `/// @cond internal`, corrected the macro comment (With()-hook only). |
| 2 | major | 4 + 11 | A single `mBox`/one `mixColor` cannot render PRIMARY fill + ON_PRIMARY tick, leaving the tick style setter dead. | Adopted a **two-layer** design (`mBox` box + separate `mTick`), each token has a real apply path; §11 OnInitialize/swap + §4 rows updated; pre-composed single-asset noted as fallback. |
| 3 | minor | 11 | OnInitialize "if constructed with text create label" contradicts two-phase construction. | Removed; box-only is the initialized default, `mLabel` created lazily in `SetText`. |
| 4 | minor | 6 | 100–500 ms motion clamp was uncited. | Cited the pre-decided policy (oneui8-porting.md:222,186); value itself stays verify-needed. |
| 5 | minor | 5 / 9 / 12 | Checkmark-not-mirrored invariant left implicit. | Stated explicitly in §5 and §9 (oneui8-porting.md:225) + added the §12 assertion. |
| 6 | minor | 9 / 13 | "RESOLVED, no manual emit" was stronger than static reading proves. | Downgraded to "resolved for the role gate; residual = accessible-object existence off-highlight → verify-needed (§13-7)". |

**Ship status of this stage:** extracted-spec complete and self-consistent; awaiting **human sign-off** before any source is written (skill gate 2.4). Implementation is deferred per the confirmed "spec-first / numbers-later" decision.