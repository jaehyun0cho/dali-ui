# CheckBox — Self-Contained Implementation Plan (Lottie edition)

> **You can implement the DALi UI `CheckBox` component from this file alone.**
> Every DALi symbol, include path, file, build command, and test used below was
> verified against the live source of the current checkout. Copy the code blocks
> verbatim, fill the values marked `TODO(verify)`, provide the Lottie asset(s), then
> build & test.
>
> **What changed vs the earlier SVG draft:** the check/uncheck visual is now a single
> **Lottie** animation (frame-range play + per-frame inner-fill recolour), not two
> overlaid SVG `ImageView` layers. A `SelectionAnimationMode` API, a `Ghost` style
> preset, theme-change refresh, and circular touch-dim are added; three NUI behaviours
> are intentionally **descoped** and documented (§1.4).

---

## 0. How to use this document

1. Read §1 (frozen scope + divergences) and §2 (what you must supply).
2. Create the 7 source files in §5 exactly as written (complete, not sketches);
   the parts that depend on the shipped Lottie asset are marked `TODO(verify)`.
3. Apply the one umbrella-header edit in §6.
4. Add the UTC file + CMake line in §7.
5. Build & run per §8; tick off §9.
6. §3 is a reference appendix — the exact signatures of every foundation API used,
   so you never need to open another header.

**Repo root** is referred to as `$ROOT` = the repository root of the current checkout
(`git rev-parse --show-toplevel`; e.g. `/home/jae/dali/dali-ui`). All source paths are
relative to `$ROOT`. Never hard-code a specific worktree path.

---

## 1. Scope & decisions (FROZEN — do not re-open)

### 1.1 Component & state
- **Component:** `Dali::Ui::CheckBox` only. No group / no `CheckBoxGroup`.
  (DALi's `SelectionGroup`/`GroupSelectableView` are *radio / mutually-exclusive*
  and are the wrong semantics for a checkbox — do not use them.)
- **State model:** BINARY — `checked` / `unchecked`. **No indeterminate / tri-state.**
  Selection is the inherited `IsSelected/SetSelected/SelectionChangedSignal/
  IsToggleByClickEnabled/SetToggleByClickEnabled` from `SelectableView` — **do not
  redeclare them** and **do not add a `SetIndeterminate`**. (Foundation has no
  indeterminate: the trait stores a plain `bool` and `ViewState` has no MIXED value.)
- **Anatomy:** leading **box glyph** + **optional trailing text label** (control leads,
  label trailing; the two form one hit target). Empty text ⇒ box-only.

### 1.2 Visual = one Lottie animation (ported from the NUI reference)
- **GUI = a single Lottie asset** (`checkbox.json`, shipped in `dali-ui-components/images/`
  and installed to `<dali image dir>/components/checkbox.json`), rendered with **one
  `Ui::LottieAnimationView` child** (`mIcon`). It replaces the earlier two SVG layers.
- **Check/uncheck is a frame-range animation on one json** (asset markers `check`/`uncheck`):
  select plays `[0,30]`, deselect plays `[30,48]`; resting frames `30` (checked) and `48`
  (unchecked). *(Ranges are asset-coupled — `TODO(verify)`.)*
- **Fill recolour is per-frame** via a Lottie *dynamic property*: the
  `checked-fill.fill-group.fill-color` layer is tinted with the **selected** token when
  checked (the **deselected** token at the unchecked rest frames `0`/`48`, where the fill
  is invisible), so one callback serves both directions. *(Exact `keyPath` is
  asset-coupled — `TODO(verify)`.)*
- **Selection-transition animation policy** (`SelectionAnimationMode`, default `AUTO`):
  animate only when the view is **on-scene AND visible** AND (`ENABLED`, or `AUTO` AND
  the change came from a **user gesture/key** rather than a programmatic `SetSelected`).
  Programmatic changes **snap** to the resting frame; user taps/keys **animate**.
  `DISABLED` never animates. This mirrors the NUI `IsSelectionAnimationRequired` policy
  using the inherited `SelectionChangedSignal`'s `InputEvent` cause
  (`InputEvent::IsProgrammatic()`) plus inherited `IsOnScene()/IsVisible()`.
- **Resting-frame refresh:** the icon re-seats its resting frame when it is (re-)attached
  to the scene (`OnSceneConnection`) and after an animation finishes; token colours are
  re-resolved on theme change (§1.3).

### 1.3 Theming, variants, layout, accessibility, feedback
- **Colours are `UiColor` semantic tokens** (hex resolved by the theme): `IconColor`
  (deselected inner fill), `SelectedIconColor` (selected inner fill), `LabelColor`.
  On a live **theme change** the impl re-resolves the tokens and rebuilds the recolour
  dynamic property (immediately if idle; deferred to `AnimationFinishedSignal` if a
  segment is playing) — via `UiThemeManager::ThemeChangedSignal()`.
- **Variants:** two style presets — `Default` and `Ghost` (the Ghost preset points
  `IconUrl` at a different json, e.g. `i_check_box_ghost.json`). Swapping the json url
  via the style/Builder is the sanctioned substitute for the NUI `IconGenerator`.
- **Layout:** `WrapContent` width/height, style `Padding` (default `8dp`),
  `ClippingMode::CLIP_TO_BOUNDING_BOX`, a fixed-size icon (default `36dp`) placed
  leading + vertically centred, optional trailing label after `LabelGap`.
- **Accessibility role** = `Accessibility::Role::CHECK_BOX`; the `CHECKED` state bit is
  mirrored from `selected` with `AddAccessibilityState/RemoveAccessibilityState`
  (preserves `ENABLED` and other bits by construction). The children (`mIcon`, `mLabel`)
  are `ACCESSIBILITY_HIDDEN` so the CheckBox is a single accessible node; the label text
  is mirrored onto the CheckBox `ACCESSIBILITY_NAME`, and a usage-hint
  `ACCESSIBILITY_DESCRIPTION` ("Double tap to select" / "Double tap to deselect") is set
  and recomputed on selection change (app-localizable; plain-string passthrough).
- **Touch feedback:** circular dim overlay via `OverlayEffect::Round()` (a `StateEffect`);
  a null effect coerces to `StateEffect::None()`. The effect is retargeted to the **icon
  glyph** with `View::SetStateEffectTarget(mIcon)`, so press/focus feedback animates only
  the checkbox glyph — never the label or the rest of the control.
- **Numbers & assets are deferred** (§2). Every concrete dp/frame/URL/keyPath is a
  `TODO(verify)` placeholder — none is a confirmed One UI value yet.
- **Verification target:** behaviour-compatible (Tier 1, GPU-free). No pixel/GUI compare;
  frame/recolour fidelity is verified manually once the real asset lands.

### 1.4 Deliberate divergences from the NUI reference (descoped, documented)
These NUI behaviours are **intentionally not ported** in v1 because the DALi foundation
provides no public API for them; each is an accepted, documented divergence rather than a
hidden gap:

1. **The glyph's own press-scale is NOT suppressed.** Feedback is scoped to the icon via
   `View::SetStateEffectTarget(mIcon)` (so only the glyph animates, not the label or the
   rest of the control), but within the glyph the `OverlayEffect` still applies its dim
   *and* recoil scale — DALi has no public way to dim the target *without* also scaling it.
   NUI additionally keeps the icon itself from scaling (`GetTouchEffectSecondaryTarget()
   =>null`); that glyph-level scale suppression is *not* reproduced. *Accepted divergence.*
   Exact parity would need a new foundation API (e.g. an `OverlayEffect` "no-scale" variant).
2. **Default click sound is NOT played.** NUI plays a click sound on every activation.
   DALi exposes no public click/feedback-sound mechanism (the internal `FeedbackStyle`
   is unwired). *Accepted divergence;* would require a new foundation sound hook.
3. **A mutable `IsSelectable` gate with auto-deselect-on-disable is NOT added.** NUI's
   `IsSelectable=false` auto-deselects and blocks toggles while leaving programmatic set
   working. In DALi, use the inherited `View::SetEnabled(false)` (blocks user toggles via
   disabled interaction); note DALi does **not** auto-deselect on disable. Exact parity
   would require new CheckBox API; out of scope for v1.

*(Everything else in the NUI checkbox — binary selection + two write paths, toggle-on-
click, deselect-by-tap, `SelectedChanged` carrying the input cause, touch tap/long-press,
execution-key `ClickKeyType` timing, highlightable — is inherited from
`SelectableView`/`InteractiveView` and needs no CheckBox code.)*

---

## 2. What YOU must supply before/at implementation (deferred inputs)

### 2.1 Numeric parameters — fill in `CheckBoxStyleImpl` defaults (§5.6)

| Field | Meaning | Placeholder (replace) | Unit |
|---|---|---|---|
| `mMinimumWidth`, `mMinimumHeight` | whole-control min touch target | `0.0f` (unset — app sizes it) | dp |
| `mPadding` | content inset (start,end,top,bottom) | `Extents(0,0,0,0)` | dp |
| `mBoxSize` | icon glyph side (square); `0` ⇒ icon fills the content height | `0.0f` (unset — app sizes it) | dp |
| `mLabelGap` | gap between icon and label | `8.0f` | dp |
| selection / deselection frame ranges | asset segments (from markers `check`/`uncheck`) | `[0,30]` / `[30,48]` | frame idx |

**Size policy (per app request):** the CheckBox does NOT hard-code a default control/icon
size. The application sizes the control (e.g. `checkBox.SetRequestedWidth/Height(...)`);
the Lottie icon is a leading square whose side is `mBoxSize` when a style provides one,
else the arranged content height. `mMinimumWidth/Height` default to `0`.
*(dp values are logical; the impl multiplies them by `GetEffectiveScale()`. The frame
ranges are impl constants tied to the shipped json — see §2.2.)*

### 2.2 Lottie asset(s) — provide the file(s), URL(s), key path, and frame layout

| Style field | Asset | Notes |
|---|---|---|
| `mIconUrl` (Default) | `checkbox.json` | shipped in `dali-ui-components/images/`; the `check`/`uncheck` segments + a recolourable `checked-fill` layer |
| `mIconUrl` (Ghost preset) | same file (v1) | same segment/keyPath contract |

- **Packaged resource (no dev path):** `checkbox.json` lives in
  `dali-ui-components/images/` and is installed by the existing
  `INSTALL(DIRECTORY images/ …)` rule to `<dali image dir>/components/checkbox.json`. The
  default `mIconUrl` is resolved at runtime as
  `Dali::Ui::Integration::AssetManager::GetDaliImagePath() + "components/checkbox.json"`
  (the same `GetDaliImagePath()` pattern the foundation uses for its own images) — no
  absolute path is hard-coded.
- **Asset markers/frames:** `checkbox.json` (`one-ui-like-checkbox-toggle`, 120×120,
  `fr:60`, `op:48`) defines markers `unchecked` (frame 0), `check` `[1,19]`, `checked`
  `[19,30]`, `uncheck` `[30,48]`. The impl plays `SELECT=[0,30]` (check + settle) and
  `DESELECT=[30,48]`; resting frames `30` (checked) / `48` (unchecked).
- **Recolour key path (from the asset's layer tree):** `checked-fill`(layer) →
  `fill-group`(group) → `fill-color`(fill). The impl uses
  `keyPath = "checked-fill.fill-group.fill-color"` to tint the checked box fill with the
  selected token; a mismatch silently no-ops the recolour (no compile error). Stroke
  layers (`unchecked-outline`, `checkmark`) keep their baked colours in v1.
- `LottieAnimationView::SetResourceUrl` routes a `.json` url to the Lottie visual;
  reloading the url is the verified way to reset dynamic properties.

### 2.3 Colour tokens (already chosen — hex resolved by theme, nothing to supply)

`IconColor` (deselected inner fill — e.g. an on-surface container token),
`SelectedIconColor` (selected inner fill — `PRIMARY` / control-activated),
`LabelColor` (`ON_SURFACE`).

---

## 3. Verified foundation API reference (no other file needed)

### 3.1 Base classes / inheritance
- `class Dali::Ui::SelectableView : public InteractiveView` — header
  `<dali-ui-foundation/public-api/views/selectable-view.h>`. Inherited API you REUSE
  (do not redeclare): `Signal<void(View,bool,InputEvent)>& SelectionChangedSignal();`
  `bool IsSelected() const;` `void SetSelected(bool);` `bool IsToggleByClickEnabled() const;`
  `void SetToggleByClickEnabled(bool enabled=true);` (selectable-view.h:113-153).
- `class Dali::Ui::Provider::SelectableViewImpl : public InteractiveViewImpl` — header
  `<dali-ui-foundation/provider-api/selectable-view-impl.h>`. Overridable: `void OnInitialize() override;`
  (call `SelectableViewImpl::OnInitialize()` FIRST — it runs `EnsureSelectableTrait`, which
  is what makes `SelectionChangedSignal()`/toggle-by-click valid). Public helper in this
  header: `GetImpl(SelectableView&) -> SelectableViewImpl&`.
- `SelectableView : InteractiveView : View : CustomActor : Actor` — so a CheckBox handle
  IS-A `Dali::Actor` and IS-A `Ui::View`.

### 3.2 ViewImpl services available inside the impl (from `SelectableViewImpl` → `ViewImpl`)
`Self()` (returns the `CustomActor`), `Initialize()`, `GetEffectiveScale()`,
`GetPadding()` (`Extents`), `GetRequestedWidth()/GetRequestedHeight()`,
`GetMinimumWidth()/GetMinimumHeight()`, `InvalidateMeasure()`,
`bool IsOnScene() const` (view-impl.h:1036), `bool IsVisible() const` (view-impl.h:279).
Overridables: `OnInitialize()`, `MeasuredSize OnMeasure(float,float)`,
`MeasuredSize OnArrange(const LayoutRect&)`, `void OnSceneConnection(int depth)`
(view-impl.h:1134 — call `ViewImpl::OnSceneConnection(depth)` first).
`ViewImpl` implements `ConnectionTrackerInterface`, so `signal.Connect(this, &Impl::Method)`
is valid and auto-disconnects on destroy.

### 3.3 Laying out child `Ui::View`s (icon, label)
- Public helper (`<dali-ui-foundation/public-api/views/view-impl.h>`):
  `inline ViewImpl& GetImpl(Ui::View& view);` and its const overload. `ViewImpl` has
  PUBLIC `MeasuredSize Measure(float,float)` and `MeasuredSize Arrange(const LayoutRect&)`.
- To measure/arrange a `LottieAnimationView` (or any non-Label) child, upcast to `Ui::View`
  and use the public `GetImpl(Ui::View&)`:
  ```cpp
  Ui::View v = mIcon;            // LottieAnimationView -> View (base-handle copy)
  GetImpl(v).Measure(w, h);
  GetImpl(v).Arrange(rect);
  ```
- There is **no** public `GetImpl(Ui::Label&)`; `Ui::Label` IS-A `Ui::View`
  (`label.h:55`), so `GetImpl(mLabel).Measure(...)/.Arrange(...)` binds to the SAME
  public `GetImpl(Ui::View&)` overload (dispatching virtually to `LabelImpl::OnMeasure/
  OnArrange`) — exactly as `text-button-impl.cpp:247-248` does.

### 3.4 `Ui::LottieAnimationView` — header `<dali-ui-foundation/public-api/views/image/lottie-animation-view.h>`
Verified members (line refs in that header):
`static LottieAnimationView New(const Dali::String& url = "");` (:101)
`void SetResourceUrl(const Dali::String& url);` (:153)
`void Play();` (:168) `void Pause();` `void Stop();`
`void SetLoopCount(int count);` (:193 — `1` = play the range once; `-1` = infinite)
`void JumpToFrame(int frame);` (:208 — deferred, works off-scene)
`void SetMinMaxFrame(int minFrame, int maxFrame);` (:220 — deferred; flushed by the next JumpToFrame/Play)
`AnimatedImage::PlayState GetPlayState() const;` (:290 — enum `{STOPPED, PLAYING, PAUSED}`,
`<...image/animated-image-enumerations.h>`:37; reliable only for a live on-scene visual)
`int GetCurrentFrame() const;` (:297) `int GetTotalFrame() const;` (:304)
`void SetDynamicProperty(const LottieAnimation::DynamicPropertyInfo& info);` (:440)
`AnimationFinishedSignalType& AnimationFinishedSignal();` (:575 — `Signal<void(View)>`).
The NUI 3-step `Play(start,end)` maps to `SetMinMaxFrame(start,end); JumpToFrame(start); Play();`.

**Dynamic property (per-frame recolour)** — header `<...image/lottie-animation-types.h>`:
```cpp
struct LottieAnimation::DynamicPropertyInfo {
  int32_t                     id;       // unique id passed to the callback
  Dali::String                keyPath;  // e.g. "check_box.inner_fill.color", or "**" for all
  LottieAnimation::VectorProperty property; // FILL_COLOR / FILL_OPACITY / ... (lottie-animation-enumerations.h:50)
  CallbackBase*               callback; // ownership transfers to the visual
};
```
The callback runs on a **worker thread — do NOT call any DALi API from it**; only read
pre-resolved values. Signature (verified in `manual-tests/dali-ui-foundation/tc/tc-lottie-dynamic-property.cpp`):
```cpp
#include <dali/devel-api/adaptor-framework/vector-animation-renderer.h>
static Dali::Property::Value OnInnerFill(int32_t id,
                                         Dali::VectorAnimationRenderer::VectorProperty /*property*/,
                                         uint32_t frameNumber);
// registered with: info.callback = MakeCallback(&OnInnerFill);
```
`MakeCallback` in the verified example wraps a **capture-less free function**, so
per-instance colours must be reached via the `id` (an id→colours lookup) rather than a
lambda capture — see §5.4 and its `TODO(verify)`. There is no explicit "remove dynamic
property" call; **reload the url** to reset (verified reset idiom).

### 3.5 `Ui::Label` — header `<dali-ui-foundation/public-api/views/text-controls/label.h>`
`static Label New();` `void SetText(const Dali::String&);` `Dali::String GetText() const;`
`void SetTextColor(const UiColor&);`

### 3.6 `UiColor` — header `<dali-ui-foundation/public-api/types/ui-color.h>`
Static theme tokens: `UiColor::PRIMARY`, `ON_PRIMARY`, `ON_SURFACE`, `OUTLINE`, `SURFACE`,
`BACKGROUND` (and container tokens). Ctors: `UiColor(float r,g,b,a=1)`,
`UiColor(uint32_t rgb,float a=1)`. Helpers: `UiColor WithAlpha(float) const;`
`Vector4 GetRgba() const;` (implicit `operator Vector4()`). **Resolve a token to `Vector4`
on the main thread (`GetRgba()`) and capture that value for the worker-thread recolour
callback** — do not resolve a token inside the callback.

### 3.7 Style base + touch effect
Headers `<...styles/ui-style.h>`, `<...styles/ui-style-key.h>`, provider
`<...provider-api/styles/ui-style-impl.h>`.
`class UiStyle : public BaseHandle` (ctor `UiStyle(Internal*)`, `GetBaseObject()`).
`template<T> class UiStyleKey { static UiStyleKey Alloc(); };`
`class Provider::UiStyleImpl` — the impl base; protected `~() override`.
`StateEffect` — `<...views/effects/state-effect.h>`: `static StateEffect DefaultForInteractive();`
`static StateEffect None();` (a null `StateEffect` is falsy).
`OverlayEffect` — `<...views/effects/overlay-effect.h>`: `class OverlayEffect : public StateEffect`
with `static const OverlayEffect& Round();` (overlay-effect.h:70,91) — a circular dim overlay.
`View::SetStateEffectTarget(View target)` / `GetStateEffectTarget()` (view.h:2278,2287) —
redirects the owner's state effect to render on `target` (a descendant) instead of the whole
owner; per overlay-effect.h:52-56 the overlay then applies to that View. Used to scope
CheckBox feedback to the icon glyph.
`UiConfig::GetCurrent().GetStyle(key)` and `DebugAssertStyleConfigApplied()` —
`<...configuration/ui-config.h>`, `<...provider-api/styles/ui-style-debug.h>`.

### 3.8 Accessibility — header `<dali-ui-foundation/public-api/views/view-accessibility-types.h>`
```cpp
namespace Dali { namespace Ui { namespace Accessibility {
enum class State : uint32_t { ENABLED = 0, SELECTED, CHECKED, BUSY, EXPANDED, MAX_COUNT }; // :45-53
enum class Role  : uint32_t { /* ... */ CHECK_BOX /* ... */ };     // :71-105 (CHECK_BOX at :76)
}}}
```
View methods (`<...views/view.h>`): `void AddAccessibilityState(Accessibility::State);`
(view.h:2096) `void RemoveAccessibilityState(Accessibility::State);` (:2105)
`bool HasAccessibilityState(Accessibility::State) const;` (:2122).
These perform a safe add/subtract that **preserves other bits** (same primitive
`GroupSelectableTrait` uses) and auto-emit the AT state-change when an accessible object
exists (role-gated by CHECK_BOX). String/int properties on the view actor
(`Ui::View::Property::…`): `ACCESSIBILITY_NAME` (view.h:1782, string),
`ACCESSIBILITY_DESCRIPTION` (:1788, string), `ACCESSIBILITY_ROLE` (:1803, int),
`ACCESSIBILITY_HIDDEN` (:1830, bool). **There is NO `ACCESSIBILITY_STATES` property** —
use `Add/Remove/HasAccessibilityState`. Set the role with
`SetProperty(ACCESSIBILITY_ROLE, static_cast<int32_t>(Accessibility::Role::CHECK_BOX))`.

### 3.9 Theme change — header `<dali-ui-foundation/public-api/configuration/ui-theme-manager.h>`
`UiThemeManager::Get().ThemeChangedSignal()` → `Signal<void()>` (ui-theme-manager.h:52,88).
Connect from the impl (a `ConnectionTracker`) to re-resolve token colours.

### 3.10 Layout types — header `<dali-ui-foundation/public-api/layouts/layout-types.h>`
`struct MeasuredSize { float width; float height; MeasuredSize(float,float); };`
`struct LayoutRect { float x, y, width, height; };` sentinel `MATCH_PARENT`.

### 3.11 Handle plumbing helpers (from `view.h`, available transitively)
`Ui::View::DownCast<Handle,Impl>(BaseHandle)` (typed downcast),
`VerifyCustomActorPointer<Impl>(Dali::Internal::CustomActor*)`,
`DALI_UI_VIEW_WITH(<Name>)` — declares a `With(action,...)` extension hook only
(NOT fluent chaining). `DALI_UI_API` / `DALI_INTERNAL` export macros.

### 3.12 Type registration (from `<dali/devel-api/object/type-registry-helper.h>`,`type-registry.h`)
```cpp
namespace { BaseHandle Create() { return BaseHandle(); }
DALI_TYPE_REGISTRATION_BEGIN(CheckBoxImpl, Provider::SelectableViewImpl, Create)
DALI_TYPE_REGISTRATION_END() }
```

### 3.13 Build integration (verified)
- Component sources are **glob-collected** (`build/tizen/dali-ui-components/CMakeLists.txt`
  `FILE(GLOB_RECURSE ...)`); new `.cpp` need **no** build-file edit.
- The **only** manual source edit is the umbrella header `dali-ui-components/dali-ui-components.h` (§6).
- Library target = `dali2-ui-components`.

---

## 4. Files to create

```
$ROOT/dali-ui-components/public-api/check-box.h                    (§5.1)
$ROOT/dali-ui-components/public-api/check-box.cpp                  (§5.2)
$ROOT/dali-ui-components/internal/check-box-impl.h                 (§5.3)
$ROOT/dali-ui-components/internal/check-box-impl.cpp               (§5.4)
$ROOT/dali-ui-components/public-api/styles/check-box-style.h       (§5.5)
$ROOT/dali-ui-components/public-api/styles/check-box-style.cpp     (§5.6)
$ROOT/dali-ui-components/internal/styles/check-box-style-impl.h    (§5.7)
```
Edit: `dali-ui-components/dali-ui-components.h` (§6),
`automated-tests/src/dali-ui-components/CMakeLists.txt` (§7),
add `automated-tests/src/dali-ui-components/utc-Dali-CheckBox.cpp` (§7).
Provide the Lottie asset(s) (§2.2). A sample under `samples/check-box/` and a manual test
under `manual-tests/dali-ui-components/tc/` are recommended follow-ups (auto-discovered).

Every file starts with the standard Apache-2.0 header (Samsung Electronics) — copy it
from any existing file in the repo.

---

## 5. Full source

### 5.1 `dali-ui-components/public-api/check-box.h`

```cpp
#pragma once
// <Apache-2.0 header>

// INTERNAL INCLUDES
#include <dali-ui-components/public-api/styles/check-box-style.h>
#include <dali-ui-foundation/public-api/views/selectable-view.h>
#include <dali/public-api/common/dali-string.h>

// EXTERNAL INCLUDES
#include <cstdint>

namespace Dali
{
namespace Ui
{
namespace Internal
{
class CheckBoxImpl;
}

/**
 * @brief Controls whether a selection-state change animates.
 *
 * AUTO     — animate only when the change came from a user gesture/key (programmatic
 *            SetSelected snaps instantly). (default)
 * ENABLED  — always animate (on-scene + visible permitting).
 * DISABLED — never animate; state changes snap.
 */
enum class SelectionAnimationMode : uint8_t
{
  AUTO = 0,
  ENABLED,
  DISABLED
};

/**
 * @brief CheckBox is a binary (checked/unchecked) selectable control with an
 *        optional trailing text label. Its glyph is a single Lottie animation whose
 *        check/uncheck transition plays a frame range and recolours its inner fill.
 *
 * Selection state is inherited from SelectableView
 * (IsSelected/SetSelected/SelectionChangedSignal/IsToggleByClickEnabled/
 * SetToggleByClickEnabled) and is intentionally NOT redeclared here.
 */
class DALI_UI_API CheckBox : public SelectableView
{
public:
  CheckBox();
  ~CheckBox();

  static CheckBox New();
  static CheckBox New(CheckBoxStyle style);
  static CheckBox New(const Dali::String& text);
  static CheckBox New(const Dali::String& text, CheckBoxStyle style);
  static CheckBox DownCast(BaseHandle handle);

  CheckBox(const CheckBox& handle);
  CheckBox(CheckBox&& rhs) noexcept;
  CheckBox& operator=(const CheckBox& handle);
  CheckBox& operator=(CheckBox&& rhs) noexcept;

  DALI_UI_VIEW_WITH(CheckBox)

  /**
   * @brief Sets the trailing label text. An empty string yields a box-only checkbox.
   */
  void         SetText(const Dali::String& text);
  Dali::String GetText() const;

  /**
   * @brief Sets/gets how a selection-state change animates (default AUTO).
   */
  void                   SetSelectionAnimationMode(SelectionAnimationMode mode);
  SelectionAnimationMode GetSelectionAnimationMode() const;

public: // Not intended for application developers
  /// @cond internal
  explicit DALI_INTERNAL CheckBox(Internal::CheckBoxImpl& implementation);
  explicit DALI_INTERNAL CheckBox(Dali::Internal::CustomActor* internal);
  /// @endcond
};

} // namespace Ui
} // namespace Dali
```

### 5.2 `dali-ui-components/public-api/check-box.cpp`

```cpp
// <Apache-2.0 header>

// CLASS HEADER
#include <dali-ui-components/public-api/check-box.h>

// INTERNAL INCLUDES
#include <dali-ui-components/internal/check-box-impl.h>

namespace Dali
{
namespace Ui
{

CheckBox::CheckBox()  = default;
CheckBox::~CheckBox() = default;

CheckBox CheckBox::New()
{
  return New(CheckBoxStyle::Default());
}

CheckBox CheckBox::New(CheckBoxStyle style)
{
  return Internal::CheckBoxImpl::New(style);
}

CheckBox CheckBox::New(const Dali::String& text)
{
  CheckBox checkBox = New(CheckBoxStyle::Default());
  checkBox.SetText(text);
  return checkBox;
}

CheckBox CheckBox::New(const Dali::String& text, CheckBoxStyle style)
{
  CheckBox checkBox = New(style);
  checkBox.SetText(text);
  return checkBox;
}

CheckBox CheckBox::DownCast(BaseHandle handle)
{
  return Ui::View::DownCast<CheckBox, Internal::CheckBoxImpl>(handle);
}

CheckBox::CheckBox(const CheckBox& handle)
: SelectableView(handle)
{
}

CheckBox::CheckBox(CheckBox&& rhs) noexcept = default;

CheckBox& CheckBox::operator=(const CheckBox& handle)
{
  if(&handle != this)
  {
    SelectableView::operator=(handle);
  }
  return *this;
}

CheckBox& CheckBox::operator=(CheckBox&& rhs) noexcept = default;

void CheckBox::SetText(const Dali::String& text)
{
  GetImpl(*this).SetText(text);
}

Dali::String CheckBox::GetText() const
{
  return GetImpl(*this).GetText();
}

void CheckBox::SetSelectionAnimationMode(SelectionAnimationMode mode)
{
  GetImpl(*this).SetSelectionAnimationMode(mode);
}

SelectionAnimationMode CheckBox::GetSelectionAnimationMode() const
{
  return GetImpl(*this).GetSelectionAnimationMode();
}

CheckBox::CheckBox(Internal::CheckBoxImpl& implementation)
: SelectableView(implementation)
{
}

CheckBox::CheckBox(Dali::Internal::CustomActor* internal)
: SelectableView(internal)
{
  VerifyCustomActorPointer<Internal::CheckBoxImpl>(internal);
}

} // namespace Ui
} // namespace Dali
```

### 5.3 `dali-ui-components/internal/check-box-impl.h`

```cpp
#pragma once
// <Apache-2.0 header>

// INTERNAL INCLUDES
#include <dali-ui-components/public-api/check-box.h>
#include <dali-ui-components/public-api/styles/check-box-style.h>
#include <dali-ui-foundation/provider-api/selectable-view-impl.h>
#include <dali-ui-foundation/public-api/input/input-event.h>
#include <dali-ui-foundation/public-api/layouts/layout-types.h>
#include <dali-ui-foundation/public-api/types/ui-color.h>
#include <dali-ui-foundation/public-api/views/image/lottie-animation-view.h>
#include <dali-ui-foundation/public-api/views/text-controls/label.h>
#include <dali-ui-foundation/public-api/views/view.h>
#include <dali/public-api/common/dali-string.h>

// EXTERNAL INCLUDES
#include <cstdint>

namespace Dali
{
namespace Ui
{
namespace Internal
{

class CheckBoxImpl : public Provider::SelectableViewImpl
{
public:
  static Ui::CheckBox New(CheckBoxStyle style);

  void         SetText(const Dali::String& text);
  Dali::String GetText() const;

  void                   SetSelectionAnimationMode(SelectionAnimationMode mode);
  SelectionAnimationMode GetSelectionAnimationMode() const;

protected:
  void         OnInitialize() override;
  void         OnSceneConnection(int depth) override;
  MeasuredSize OnMeasure(float widthConstraint, float heightConstraint) override;
  MeasuredSize OnArrange(const LayoutRect& bounds) override;

  CheckBoxImpl();
  ~CheckBoxImpl() override;

private:
  void ApplyInitialStyle(CheckBoxStyle style);
  void OnSelectionChanged(View view, bool selected, InputEvent event);
  void OnThemeChanged();
  void OnAnimationFinished(View view);

  // Decide, from the change's cause + on-scene/visible, whether to animate.
  bool IsSelectionAnimationRequired(const InputEvent& event) const;
  // Play the correct segment (animate) or snap to its end frame (instant).
  void ApplySelectionVisual(bool selected, bool animate);
  // Jump to the resting frame for the current IsSelected() when not playing.
  void RefreshRestingFrame();
  // (Re)register the inner-fill recolour dynamic property from the current tokens.
  void RegisterInnerFillRecolor();
  // Mirror selected -> a11y CHECKED bit + usage-hint description.
  void UpdateAccessibility(bool selected);

private:
  Ui::LottieAnimationView mIcon;  ///< single Lottie glyph (frame-range + recolour)
  Ui::Label               mLabel; ///< optional trailing label

  Dali::String mText;

  // Captured from the style in ApplyInitialStyle():
  float        mBoxSize{0.0f};
  float        mGap{0.0f};
  Dali::String mIconUrl;
  UiColor      mIconColor;         ///< deselected inner-fill token
  UiColor      mSelectedIconColor; ///< selected inner-fill token

  SelectionAnimationMode mSelectionAnimationMode{SelectionAnimationMode::AUTO};

  int32_t mDynamicPropertyId{0}; ///< unique id keying this instance's recolour callback
  bool    mThemeRefreshPending{false};
};

} // namespace Internal

inline Internal::CheckBoxImpl& GetImpl(Ui::CheckBox& checkBox)
{
  DALI_ASSERT_ALWAYS(checkBox);
  return static_cast<Internal::CheckBoxImpl&>(checkBox.GetImplementation());
}

inline const Internal::CheckBoxImpl& GetImpl(const Ui::CheckBox& checkBox)
{
  DALI_ASSERT_ALWAYS(checkBox);
  return static_cast<const Internal::CheckBoxImpl&>(checkBox.GetImplementation());
}

} // namespace Ui
} // namespace Dali
```

### 5.4 `dali-ui-components/internal/check-box-impl.cpp`

```cpp
// <Apache-2.0 header>

// CLASS HEADER
#include <dali-ui-components/internal/check-box-impl.h>

// EXTERNAL INCLUDES
#include <dali/devel-api/adaptor-framework/vector-animation-renderer.h>
#include <dali/devel-api/object/type-registry-helper.h>
#include <dali/devel-api/object/type-registry.h>
#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/configuration/ui-theme-manager.h>
#include <dali-ui-foundation/public-api/views/view-accessibility-types.h>
#include <dali-ui-foundation/public-api/views/view-impl.h> // public GetImpl(Ui::View&)

namespace Dali
{
namespace Ui
{
namespace Internal
{
namespace
{
BaseHandle Create()
{
  return BaseHandle();
}

DALI_TYPE_REGISTRATION_BEGIN(CheckBoxImpl, Provider::SelectableViewImpl, Create)
DALI_TYPE_REGISTRATION_END()

// Segment layout from the checkbox.json markers: unchecked=0, check=[1,19],
// checked=[19,30], uncheck=[30,48]. Select plays [0,30] (check + settle), deselect plays
// [30,48]; resting frames checked=30 / unchecked=48 (frame 0 and 48 both render unchecked).
constexpr int SELECT_START   = 0;
constexpr int SELECT_END     = 30;
constexpr int DESELECT_START = 30;
constexpr int DESELECT_END   = 48;

// Fill layer path from the asset tree (checked-fill > fill-group > fill-color): the
// checked box fill, tinted with the selected token.
const char* const INNER_FILL_KEY_PATH = "checked-fill.fill-group.fill-color";

// ---------------------------------------------------------------------------
// Per-frame recolour callback plumbing.
//
// MakeCallback wraps a capture-less free function (verified in
// tc-lottie-dynamic-property.cpp), so per-instance colours are reached through the
// DynamicPropertyInfo.id via a small process-wide registry. The callback runs on a
// worker thread and must read ONLY pre-resolved Vector4 colours (no DALi API calls).
//
// The registry/mutex are leaked singletons (never destroyed) so a worker-thread callback
// firing during process shutdown cannot touch destroyed statics.
// TODO(verify): (1) whether Dali::MakeCallback also accepts a stateful functor (which
// would remove this registry); (2) whether re-calling SetDynamicProperty with the same id
// replaces vs accumulates on the renderer — each animated/snap transition rebuilds the
// visual, which clears prior callbacks, so the toggle path does not accumulate; only the
// idle theme-change re-register is unverified.
// ---------------------------------------------------------------------------
struct InnerColors
{
  Vector4 deselected;
  Vector4 selected;
};

std::mutex& RecolorMutex()
{
  static std::mutex* m = new std::mutex(); // leaked: must outlive worker callbacks at shutdown
  return *m;
}
std::map<int32_t, InnerColors>& RecolorRegistry()
{
  static auto* r = new std::map<int32_t, InnerColors>(); // leaked (see above)
  return *r;
}

int32_t AllocateDynamicPropertyId()
{
  static std::atomic<int32_t> sNext{1};
  return sNext++;
}

Dali::Property::Value OnInnerFillColor(int32_t                                        id,
                                       Dali::VectorAnimationRenderer::VectorProperty /*property*/,
                                       uint32_t                                       frameNumber)
{
  // Worker thread — registry read only, no DALi API calls.
  InnerColors colors{Vector4(1.f, 1.f, 1.f, 1.f), Vector4(1.f, 1.f, 1.f, 1.f)};
  {
    std::lock_guard<std::mutex> lock(RecolorMutex());
    auto                        it = RecolorRegistry().find(id);
    if(it != RecolorRegistry().end())
    {
      colors = it->second;
    }
  }
  // Deselected colour at the rest frames (0 / 38); selected colour elsewhere.
  const bool atDeselectedRest = (frameNumber == static_cast<uint32_t>(SELECT_START)) ||
                                (frameNumber == static_cast<uint32_t>(DESELECT_END));
  return Dali::Property::Value(atDeselectedRest ? colors.deselected : colors.selected);
}

} // namespace

Ui::CheckBox CheckBoxImpl::New(CheckBoxStyle style)
{
  DALI_ASSERT_ALWAYS(style && "CheckBoxStyle must be initialized");
  IntrusivePtr<CheckBoxImpl> impl(new CheckBoxImpl());
  Ui::CheckBox               handle(*impl);
  impl->Initialize();
  impl->ApplyInitialStyle(style);
  return handle;
}

void CheckBoxImpl::SetText(const Dali::String& text)
{
  mText = text;
  mLabel.SetText(text);

  // Mirror the label onto the single accessible node's name.
  Ui::View self = Ui::View::DownCast(Self());
  self.SetProperty(Ui::View::Property::ACCESSIBILITY_NAME, text);

  InvalidateMeasure(); // label presence changes the measured size
}

Dali::String CheckBoxImpl::GetText() const
{
  return mText;
}

void CheckBoxImpl::SetSelectionAnimationMode(SelectionAnimationMode mode)
{
  mSelectionAnimationMode = mode;
}

SelectionAnimationMode CheckBoxImpl::GetSelectionAnimationMode() const
{
  return mSelectionAnimationMode;
}

void CheckBoxImpl::OnInitialize()
{
  Provider::SelectableViewImpl::OnInitialize(); // base first (attaches the SelectableTrait)

  Ui::View self = Ui::View::DownCast(Self());

  // Accessibility role; clip children to the control box.
  self.SetProperty(Ui::View::Property::ACCESSIBILITY_ROLE,
                   static_cast<int32_t>(Accessibility::Role::CHECK_BOX));
  Self().SetProperty(Actor::Property::CLIPPING_MODE, ClippingMode::CLIP_TO_BOUNDING_BOX);

  // Single Lottie glyph.
  mIcon = Ui::LottieAnimationView::New();
  mIcon.SetLoopCount(1); // a segment plays once
  Ui::View iconView = mIcon;
  iconView.SetProperty(Ui::View::Property::ACCESSIBILITY_HIDDEN, true);
  Self().Add(mIcon);

  // Optional trailing label (empty until SetText()), vertically centered against the icon.
  mLabel = Ui::Label::New();
  mLabel.SetVerticalTextAlignment(Text::Alignment::CENTER);
  mLabel.SetProperty(Ui::View::Property::ACCESSIBILITY_HIDDEN, true);
  Self().Add(mLabel);

  // React to selection changes (no virtual OnSelectedChanged hook exists on the base).
  SelectionChangedSignal().Connect(this, &CheckBoxImpl::OnSelectionChanged);
  mIcon.AnimationFinishedSignal().Connect(this, &CheckBoxImpl::OnAnimationFinished);
  UiThemeManager::Get().ThemeChangedSignal().Connect(this, &CheckBoxImpl::OnThemeChanged);
}

void CheckBoxImpl::ApplyInitialStyle(CheckBoxStyle style)
{
  Ui::View self = Ui::View::DownCast(Self());
  self.SetMinimumWidth(style.GetMinimumWidth());
  self.SetMinimumHeight(style.GetMinimumHeight());
  self.SetPadding(style.GetPadding());
  self.SetStateEffect(style.GetStateEffect());
  // Confine press/focus feedback to the glyph only: the interactive area stays the whole
  // CheckBox (icon + label), but the overlay effect targets the icon so the label area
  // (and any other region) does not animate on touch press/release.
  Ui::View iconView = mIcon;
  self.SetStateEffectTarget(iconView);

  mBoxSize           = style.GetBoxSize();
  mGap               = style.GetLabelGap();
  mIconUrl           = style.GetIconUrl();
  mIconColor         = style.GetIconColor();
  mSelectedIconColor = style.GetSelectedIconColor();

  mDynamicPropertyId = AllocateDynamicPropertyId();

  mIcon.SetResourceUrl(mIconUrl);

  mLabel.SetTextColor(style.GetLabelColor());

  // Seat the initial (unchecked) resting frame + a11y bit.
  RefreshRestingFrame();
  UpdateAccessibility(IsSelected());
}

void CheckBoxImpl::OnSelectionChanged(View /*view*/, bool selected, InputEvent event)
{
  ApplySelectionVisual(selected, IsSelectionAnimationRequired(event));
  UpdateAccessibility(selected);
}

bool CheckBoxImpl::IsSelectionAnimationRequired(const InputEvent& event) const
{
  if(mSelectionAnimationMode == SelectionAnimationMode::DISABLED)
  {
    return false;
  }
  if(!(IsOnScene() && IsVisible())) // animatable only when on-scene and visible
  {
    return false;
  }
  if(mSelectionAnimationMode == SelectionAnimationMode::ENABLED)
  {
    return true;
  }
  // AUTO: animate only user-initiated changes; programmatic SetSelected snaps.
  return !event.IsProgrammatic();
}

void CheckBoxImpl::ApplySelectionVisual(bool selected, bool animate)
{
  const int start = selected ? SELECT_START : DESELECT_START;
  const int end   = selected ? SELECT_END : DESELECT_END;

  // SetMinMaxFrame()+JumpToFrame() rebuild the Lottie visual (mVisualDirty), which BOTH
  // drops any registered dynamic property AND constrains JumpToFrame to the play range.
  // So: set the segment range, jump to the target frame, then RE-SEAT the inner-fill
  // recolour on the rebuilt visual (SetDynamicProperty applies without re-dirtying) before
  // playing. Without the re-seat the themed recolour is lost on the first animated toggle.
  mIcon.SetMinMaxFrame(start, end);
  mIcon.JumpToFrame(animate ? start : end);
  RegisterInnerFillRecolor();
  if(animate)
  {
    mIcon.Play();
  }
}

void CheckBoxImpl::RefreshRestingFrame()
{
  if(mIcon.GetPlayState() == AnimatedImage::PlayState::PLAYING)
  {
    return; // don't disturb a running segment
  }
  ApplySelectionVisual(IsSelected(), false); // snap to the correct resting frame + range
}

void CheckBoxImpl::RegisterInnerFillRecolor()
{
  {
    std::lock_guard<std::mutex> lock(RecolorMutex());
    RecolorRegistry()[mDynamicPropertyId] = InnerColors{mIconColor.GetRgba(), mSelectedIconColor.GetRgba()};
  }
  LottieAnimation::DynamicPropertyInfo info;
  info.id       = mDynamicPropertyId;
  info.keyPath  = INNER_FILL_KEY_PATH;
  info.property = LottieAnimation::VectorProperty::FILL_COLOR;
  info.callback = MakeCallback(&OnInnerFillColor);
  mIcon.SetDynamicProperty(info);
}

void CheckBoxImpl::UpdateAccessibility(bool selected)
{
  Ui::View self = Ui::View::DownCast(Self());
  if(selected)
  {
    self.AddAccessibilityState(Accessibility::State::CHECKED);
  }
  else
  {
    self.RemoveAccessibilityState(Accessibility::State::CHECKED);
  }
  // Usage hint (app-localizable; plain-string passthrough). TODO(verify): source the
  // localized strings from the app's i18n rather than hard-coding English.
  self.SetProperty(Ui::View::Property::ACCESSIBILITY_DESCRIPTION,
                   Dali::String(selected ? "Double tap to deselect" : "Double tap to select"));
}

void CheckBoxImpl::OnThemeChanged()
{
  // Re-resolve the tokens for the recolour callback.
  if(mIcon.GetPlayState() == AnimatedImage::PlayState::PLAYING)
  {
    mThemeRefreshPending = true; // apply when the current segment finishes
    return;
  }
  RegisterInnerFillRecolor();
  RefreshRestingFrame();
}

void CheckBoxImpl::OnAnimationFinished(View /*view*/)
{
  if(mThemeRefreshPending)
  {
    RegisterInnerFillRecolor();
    mThemeRefreshPending = false;
  }
  RefreshRestingFrame(); // re-assert the resting frame after playback
}

void CheckBoxImpl::OnSceneConnection(int depth)
{
  Provider::SelectableViewImpl::OnSceneConnection(depth); // base first
  RefreshRestingFrame();                                  // show the correct resting frame on (re-)attach
}

MeasuredSize CheckBoxImpl::OnMeasure(float widthConstraint, float heightConstraint)
{
  float s = GetEffectiveScale();

  Extents padding = GetPadding();
  float   visPadW = static_cast<float>(padding.start + padding.end) * s;
  float   visPadH = static_cast<float>(padding.top + padding.bottom) * s;

  float boxVis   = mBoxSize * s;
  float gapVis   = mGap * s;
  bool  hasLabel = !mText.Empty(); // Dali::String uses Empty() (capital E)

  float requestedWidth  = GetRequestedWidth();
  float requestedHeight = GetRequestedHeight();
  float requestedVisW   = (requestedWidth >= 0.0f) ? requestedWidth * s : requestedWidth;
  float requestedVisH   = (requestedHeight >= 0.0f) ? requestedHeight * s : requestedHeight;

  float effectiveVisW = (requestedVisW >= 0.0f) ? requestedVisW : widthConstraint;
  float effectiveVisH = (requestedVisH >= 0.0f) ? requestedVisH : heightConstraint;
  float contentVisW   = (effectiveVisW >= 0.0f) ? std::max(0.0f, effectiveVisW - visPadW) : effectiveVisW;
  float contentVisH   = (effectiveVisH >= 0.0f) ? std::max(0.0f, effectiveVisH - visPadH) : effectiveVisH;

  // No fixed box size => the icon fills the content height (the app sizes the control).
  if(mBoxSize <= 0.0f)
  {
    boxVis = (contentVisH >= 0.0f) ? contentVisH : 0.0f;
  }

  float labelW = 0.0f;
  float labelH = 0.0f;
  if(hasLabel)
  {
    float labelAvailW = (contentVisW >= 0.0f) ? std::max(0.0f, contentVisW - boxVis - gapVis) : contentVisW;
    MeasuredSize labelSize = GetImpl(mLabel).Measure(labelAvailW, contentVisH);
    labelW = labelSize.width;
    labelH = labelSize.height;
  }

  float naturalW = boxVis + (hasLabel ? gapVis + labelW : 0.0f);
  float naturalH = std::max(boxVis, labelH);

  float resultVisW = 0.0f;
  if(requestedVisW >= 0.0f)
  {
    resultVisW = requestedVisW;
  }
  else if(requestedWidth == MATCH_PARENT)
  {
    resultVisW = GetMinimumWidth() * s;
  }
  else
  {
    resultVisW = naturalW + visPadW;
  }

  float resultVisH = 0.0f;
  if(requestedVisH >= 0.0f)
  {
    resultVisH = requestedVisH;
  }
  else if(requestedHeight == MATCH_PARENT)
  {
    resultVisH = GetMinimumHeight() * s;
  }
  else
  {
    resultVisH = naturalH + visPadH;
  }

  return MeasuredSize(resultVisW, resultVisH);
}

MeasuredSize CheckBoxImpl::OnArrange(const LayoutRect& bounds)
{
  Actor self = Self();
  self.SetProperty(Actor::Property::POSITION_X, bounds.x);
  self.SetProperty(Actor::Property::POSITION_Y, bounds.y);
  self.SetProperty(Actor::Property::SIZE_WIDTH, bounds.width);
  self.SetProperty(Actor::Property::SIZE_HEIGHT, bounds.height);

  float   s       = GetEffectiveScale();
  Extents padding = GetPadding();

  float contentX = static_cast<float>(padding.start) * s;
  float contentY = static_cast<float>(padding.top) * s;
  float contentW = std::max(0.0f, bounds.width - static_cast<float>(padding.start + padding.end) * s);
  float contentH = std::max(0.0f, bounds.height - static_cast<float>(padding.top + padding.bottom) * s);

  // No fixed box size => the icon fills the content height (the app sizes the control).
  float boxVis = (mBoxSize > 0.0f) ? (mBoxSize * s) : contentH;
  float gapVis = mGap * s;

  // Icon (leading, vertically centered). The framework mirrors position under RTL; the
  // Lottie artwork itself is direction-independent and is NOT mirrored.
  LayoutRect iconRect;
  iconRect.x      = contentX;
  iconRect.y      = contentY + std::max(0.0f, (contentH - boxVis) * 0.5f);
  iconRect.width  = boxVis;
  iconRect.height = boxVis;

  Ui::View iconView = mIcon; // LottieAnimationView -> View, use public GetImpl(Ui::View&)
  GetImpl(iconView).Measure(iconRect.width, iconRect.height);
  GetImpl(iconView).Arrange(iconRect);

  // Optional trailing label.
  LayoutRect labelRect;
  labelRect.x      = contentX + boxVis + gapVis;
  labelRect.y      = contentY;
  labelRect.width  = mText.Empty() ? 0.0f : std::max(0.0f, contentW - boxVis - gapVis);
  labelRect.height = contentH;
  GetImpl(mLabel).Measure(labelRect.width, labelRect.height);
  GetImpl(mLabel).Arrange(labelRect);

  return MeasuredSize(bounds.width, bounds.height);
}

CheckBoxImpl::CheckBoxImpl() = default;

CheckBoxImpl::~CheckBoxImpl()
{
  if(mDynamicPropertyId != 0)
  {
    std::lock_guard<std::mutex> lock(RecolorMutex());
    RecolorRegistry().erase(mDynamicPropertyId);
  }
}

} // namespace Internal
} // namespace Ui
} // namespace Dali
```

### 5.5 `dali-ui-components/public-api/styles/check-box-style.h`

```cpp
#pragma once
// <Apache-2.0 header>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/styles/ui-style-key.h>
#include <dali-ui-foundation/public-api/styles/ui-style.h>
#include <dali-ui-foundation/public-api/types/ui-color.h>
#include <dali-ui-foundation/public-api/views/effects/state-effect.h>
#include <dali/public-api/common/dali-string.h>
#include <dali/public-api/common/extents.h>
#include <dali/public-api/common/intrusive-ptr.h>

namespace Dali
{
namespace Ui
{
namespace Internal
{
class CheckBoxStyleImpl;
}

/**
 * @brief Style values used to initialize CheckBox appearance and layout.
 */
class DALI_UI_API CheckBoxStyle : public UiStyle
{
public:
  class Builder;

  CheckBoxStyle() = default;

  static UiStyleKey<CheckBoxStyle> DefaultKey();
  static CheckBoxStyle             DefaultPreset();
  static CheckBoxStyle             GhostPreset();
  static CheckBoxStyle             Default();
  static CheckBoxStyle             DownCast(BaseHandle handle);
  static CheckBoxStyle             StaticDownCast(UiStyle style);

  Builder Configure() const;

  float   GetMinimumWidth() const;
  float   GetMinimumHeight() const;
  Extents GetPadding() const;

  float GetBoxSize() const;
  float GetLabelGap() const;

  Dali::String GetIconUrl() const;

  UiColor GetIconColor() const;
  UiColor GetSelectedIconColor() const;
  UiColor GetLabelColor() const;

  StateEffect GetStateEffect() const;

public: // Not intended for application developers
  /// @cond internal
  explicit DALI_INTERNAL CheckBoxStyle(Internal::CheckBoxStyleImpl* impl);
  /// @endcond
};

/**
 * @brief Mutable builder used to create CheckBoxStyle handles.
 */
class DALI_UI_API CheckBoxStyle::Builder
{
public:
  Builder();
  Builder(Builder&& rhs) noexcept;
  Builder& operator=(Builder&& rhs) noexcept;
  Builder(const Builder&)            = delete;
  Builder& operator=(const Builder&) = delete;
  ~Builder();

  Builder&  SetMinimumWidth(float width) &;
  Builder&& SetMinimumWidth(float width) &&;
  Builder&  SetMinimumHeight(float height) &;
  Builder&& SetMinimumHeight(float height) &&;
  Builder&  SetPadding(const Extents& padding) &;
  Builder&& SetPadding(const Extents& padding) &&;

  Builder&  SetBoxSize(float size) &;
  Builder&& SetBoxSize(float size) &&;
  Builder&  SetLabelGap(float gap) &;
  Builder&& SetLabelGap(float gap) &&;

  Builder&  SetIconUrl(const Dali::String& url) &;
  Builder&& SetIconUrl(const Dali::String& url) &&;

  Builder&  SetIconColor(const UiColor& color) &;
  Builder&& SetIconColor(const UiColor& color) &&;
  Builder&  SetSelectedIconColor(const UiColor& color) &;
  Builder&& SetSelectedIconColor(const UiColor& color) &&;
  Builder&  SetLabelColor(const UiColor& color) &;
  Builder&& SetLabelColor(const UiColor& color) &&;

  Builder&  SetStateEffect(StateEffect effect) &;
  Builder&& SetStateEffect(StateEffect effect) &&;

  CheckBoxStyle Build() &&;

private:
  explicit Builder(Internal::CheckBoxStyleImpl* impl);
  friend class CheckBoxStyle;

private:
  IntrusivePtr<Internal::CheckBoxStyleImpl> mImpl;
};

} // namespace Ui
} // namespace Dali
```

### 5.6 `dali-ui-components/public-api/styles/check-box-style.cpp`

```cpp
// <Apache-2.0 header>

// CLASS HEADER
#include <dali-ui-components/public-api/styles/check-box-style.h>

// INTERNAL INCLUDES
#include <dali-ui-components/internal/styles/check-box-style-impl.h>
#include <dali-ui-foundation/provider-api/styles/ui-style-debug.h>
#include <dali-ui-foundation/public-api/configuration/ui-config.h>
#include <dali-ui-foundation/public-api/views/effects/overlay-effect.h>
#include <dali-ui-foundation/integration-api/asset-manager/asset-manager.h>

// EXTERNAL INCLUDES
#include <string>
#include <utility>

namespace Dali
{
namespace Ui
{
namespace
{
// Circular dim overlay on press/focus (matches the reference feedback shape).
StateEffect CreateDefaultCheckBoxStateEffect()
{
  return OverlayEffect::Round();
}
// The Lottie asset ships in dali-ui-components/images/ and installs to
// <dali image dir>/components/checkbox.json; resolve that path at runtime.
Dali::String DefaultIconUrl()
{
  const std::string path = Dali::Ui::Integration::AssetManager::GetDaliImagePath() + "components/checkbox.json";
  return Dali::String(path.c_str());
}
} // namespace

UiStyleKey<CheckBoxStyle> CheckBoxStyle::DefaultKey()
{
  static UiStyleKey<CheckBoxStyle> key = UiStyleKey<CheckBoxStyle>::Alloc();
  return key;
}

CheckBoxStyle CheckBoxStyle::DefaultPreset()
{
  DebugAssertStyleConfigApplied();
  static CheckBoxStyle style = CheckBoxStyle::Builder().Build();
  return style;
}

CheckBoxStyle CheckBoxStyle::GhostPreset()
{
  DebugAssertStyleConfigApplied();
  static CheckBoxStyle style = CheckBoxStyle::Builder()
                                 .SetIconUrl(DefaultIconUrl())
                                 .Build();
  return style;
}

CheckBoxStyle CheckBoxStyle::Default()
{
  DebugAssertStyleConfigApplied();
  CheckBoxStyle style = UiConfig::GetCurrent().GetStyle(DefaultKey());
  if(style)
  {
    return style;
  }
  return DefaultPreset();
}

CheckBoxStyle CheckBoxStyle::DownCast(BaseHandle handle)
{
  return CheckBoxStyle(dynamic_cast<Internal::CheckBoxStyleImpl*>(handle.GetObjectPtr()));
}

CheckBoxStyle CheckBoxStyle::StaticDownCast(UiStyle style)
{
  return CheckBoxStyle(static_cast<Internal::CheckBoxStyleImpl*>(style.GetObjectPtr()));
}

CheckBoxStyle::Builder CheckBoxStyle::Configure() const
{
  IntrusivePtr<Internal::CheckBoxStyleImpl> impl(new Internal::CheckBoxStyleImpl(GetImpl(*this)));
  return Builder(impl.Get());
}

float        CheckBoxStyle::GetMinimumWidth() const { return GetImpl(*this).GetMinimumWidth(); }
float        CheckBoxStyle::GetMinimumHeight() const { return GetImpl(*this).GetMinimumHeight(); }
Extents      CheckBoxStyle::GetPadding() const { return GetImpl(*this).GetPadding(); }
float        CheckBoxStyle::GetBoxSize() const { return GetImpl(*this).GetBoxSize(); }
float        CheckBoxStyle::GetLabelGap() const { return GetImpl(*this).GetLabelGap(); }
Dali::String CheckBoxStyle::GetIconUrl() const { return GetImpl(*this).GetIconUrl(); }
UiColor      CheckBoxStyle::GetIconColor() const { return GetImpl(*this).GetIconColor(); }
UiColor      CheckBoxStyle::GetSelectedIconColor() const { return GetImpl(*this).GetSelectedIconColor(); }
UiColor      CheckBoxStyle::GetLabelColor() const { return GetImpl(*this).GetLabelColor(); }
StateEffect  CheckBoxStyle::GetStateEffect() const { return GetImpl(*this).GetStateEffect(); }

CheckBoxStyle::CheckBoxStyle(Internal::CheckBoxStyleImpl* impl)
: UiStyle(impl)
{
}

CheckBoxStyle::Builder::Builder()
: mImpl(new Internal::CheckBoxStyleImpl())
{
}
CheckBoxStyle::Builder::Builder(Builder&& rhs) noexcept            = default;
CheckBoxStyle::Builder& CheckBoxStyle::Builder::operator=(Builder&& rhs) noexcept = default;
CheckBoxStyle::Builder::~Builder()                                 = default;

// --- lvalue setters mutate mImpl; rvalue setters delegate then std::move ---
#define CBS_SETTER(Name, Type)                                                   \
  CheckBoxStyle::Builder& CheckBoxStyle::Builder::Name(Type v) &                 \
  {                                                                              \
    mImpl->Name(v);                                                              \
    return *this;                                                                \
  }                                                                              \
  CheckBoxStyle::Builder&& CheckBoxStyle::Builder::Name(Type v) &&               \
  {                                                                              \
    Name(v);                                                                     \
    return std::move(*this);                                                     \
  }

CBS_SETTER(SetMinimumWidth, float)
CBS_SETTER(SetMinimumHeight, float)
CBS_SETTER(SetPadding, const Extents&)
CBS_SETTER(SetBoxSize, float)
CBS_SETTER(SetLabelGap, float)
CBS_SETTER(SetIconUrl, const Dali::String&)
CBS_SETTER(SetIconColor, const UiColor&)
CBS_SETTER(SetSelectedIconColor, const UiColor&)
CBS_SETTER(SetLabelColor, const UiColor&)
CBS_SETTER(SetStateEffect, StateEffect)
#undef CBS_SETTER

CheckBoxStyle CheckBoxStyle::Builder::Build() &&
{
  DALI_ASSERT_ALWAYS(mImpl && "CheckBoxStyle::Builder has already been consumed");
  CheckBoxStyle style(mImpl.Get());
  mImpl.Reset();
  return style;
}

CheckBoxStyle::Builder::Builder(Internal::CheckBoxStyleImpl* impl)
: mImpl(impl)
{
}

namespace Internal
{
// Trivial member get/set forwarders.
void         CheckBoxStyleImpl::SetMinimumWidth(float v) { mMinimumWidth = v; }
float        CheckBoxStyleImpl::GetMinimumWidth() const { return mMinimumWidth; }
void         CheckBoxStyleImpl::SetMinimumHeight(float v) { mMinimumHeight = v; }
float        CheckBoxStyleImpl::GetMinimumHeight() const { return mMinimumHeight; }
void         CheckBoxStyleImpl::SetPadding(const Extents& v) { mPadding = v; }
Extents      CheckBoxStyleImpl::GetPadding() const { return mPadding; }
void         CheckBoxStyleImpl::SetBoxSize(float v) { mBoxSize = v; }
float        CheckBoxStyleImpl::GetBoxSize() const { return mBoxSize; }
void         CheckBoxStyleImpl::SetLabelGap(float v) { mLabelGap = v; }
float        CheckBoxStyleImpl::GetLabelGap() const { return mLabelGap; }
void         CheckBoxStyleImpl::SetIconUrl(const Dali::String& v) { mIconUrl = v; }
Dali::String CheckBoxStyleImpl::GetIconUrl() const { return mIconUrl; }
void         CheckBoxStyleImpl::SetIconColor(const UiColor& v) { mIconColor = v; }
UiColor      CheckBoxStyleImpl::GetIconColor() const { return mIconColor; }
void         CheckBoxStyleImpl::SetSelectedIconColor(const UiColor& v) { mSelectedIconColor = v; }
UiColor      CheckBoxStyleImpl::GetSelectedIconColor() const { return mSelectedIconColor; }
void         CheckBoxStyleImpl::SetLabelColor(const UiColor& v) { mLabelColor = v; }
UiColor      CheckBoxStyleImpl::GetLabelColor() const { return mLabelColor; }
void         CheckBoxStyleImpl::SetStateEffect(StateEffect v) { mStateEffect = v ? v : StateEffect::None(); }
StateEffect  CheckBoxStyleImpl::GetStateEffect() const { return mStateEffect; }

CheckBoxStyleImpl::CheckBoxStyleImpl()
: mMinimumWidth(0.0f),                     // no default: the app sizes the control
  mMinimumHeight(0.0f),                    // no default: the app sizes the control
  mPadding(0u, 0u, 0u, 0u),
  mBoxSize(0.0f),                          // 0 => icon fills the content height
  mLabelGap(8.0f),                         // TODO(verify): box<->label gap
  mIconUrl(DefaultIconUrl()),              // <dali image dir>/components/checkbox.json
  mIconColor(UiColor::OUTLINE),            // TODO(verify): deselected inner-fill token
  mSelectedIconColor(UiColor::PRIMARY),    // control-activated accent
  mLabelColor(UiColor::ON_SURFACE),
  mStateEffect(CreateDefaultCheckBoxStateEffect())
{
}

CheckBoxStyleImpl::CheckBoxStyleImpl(const CheckBoxStyleImpl& rhs)
: mMinimumWidth(rhs.mMinimumWidth),
  mMinimumHeight(rhs.mMinimumHeight),
  mPadding(rhs.mPadding),
  mBoxSize(rhs.mBoxSize),
  mLabelGap(rhs.mLabelGap),
  mIconUrl(rhs.mIconUrl),
  mIconColor(rhs.mIconColor),
  mSelectedIconColor(rhs.mSelectedIconColor),
  mLabelColor(rhs.mLabelColor),
  mStateEffect(rhs.mStateEffect)
{
}

CheckBoxStyleImpl::~CheckBoxStyleImpl() = default;

} // namespace Internal
} // namespace Ui
} // namespace Dali
```

### 5.7 `dali-ui-components/internal/styles/check-box-style-impl.h`

```cpp
#pragma once
// <Apache-2.0 header>

// INTERNAL INCLUDES
#include <dali-ui-components/public-api/styles/check-box-style.h>
#include <dali-ui-foundation/provider-api/styles/ui-style-impl.h>

namespace Dali
{
namespace Ui
{
namespace Internal
{

class CheckBoxStyleImpl : public Provider::UiStyleImpl
{
public:
  CheckBoxStyleImpl();
  CheckBoxStyleImpl(const CheckBoxStyleImpl& rhs);

  void  SetMinimumWidth(float width);
  float GetMinimumWidth() const;
  void  SetMinimumHeight(float height);
  float GetMinimumHeight() const;
  void    SetPadding(const Extents& padding);
  Extents GetPadding() const;

  void  SetBoxSize(float size);
  float GetBoxSize() const;
  void  SetLabelGap(float gap);
  float GetLabelGap() const;

  void         SetIconUrl(const Dali::String& url);
  Dali::String GetIconUrl() const;

  void    SetIconColor(const UiColor& color);
  UiColor GetIconColor() const;
  void    SetSelectedIconColor(const UiColor& color);
  UiColor GetSelectedIconColor() const;
  void    SetLabelColor(const UiColor& color);
  UiColor GetLabelColor() const;

  void        SetStateEffect(StateEffect effect);
  StateEffect GetStateEffect() const;

protected:
  ~CheckBoxStyleImpl() override;

private:
  float        mMinimumWidth{0.0f};
  float        mMinimumHeight{0.0f};
  Extents      mPadding;
  float        mBoxSize{0.0f};
  float        mLabelGap{0.0f};
  Dali::String mIconUrl;
  UiColor      mIconColor;
  UiColor      mSelectedIconColor;
  UiColor      mLabelColor;
  StateEffect  mStateEffect;
};

} // namespace Internal

inline Internal::CheckBoxStyleImpl& GetImpl(Ui::CheckBoxStyle& style)
{
  BaseObject& handle = style.GetBaseObject();
  return static_cast<Internal::CheckBoxStyleImpl&>(handle);
}

inline const Internal::CheckBoxStyleImpl& GetImpl(const Ui::CheckBoxStyle& style)
{
  const BaseObject& handle = style.GetBaseObject();
  return static_cast<const Internal::CheckBoxStyleImpl&>(handle);
}

} // namespace Ui
} // namespace Dali
```

---

## 6. Umbrella header edit — `dali-ui-components/dali-ui-components.h`

Add two `#include` lines (alphabetical grouping matches the existing list):

- After `#include <dali-ui-components/public-api/chart/scatter-series.h>` and before
  `#include <dali-ui-components/public-api/components-ui-config.h>`:
  ```cpp
  #include <dali-ui-components/public-api/check-box.h>
  ```
- In the styles block, before
  `#include <dali-ui-components/public-api/styles/components-style-sheet.h>`:
  ```cpp
  #include <dali-ui-components/public-api/styles/check-box-style.h>
  ```
(Order is cosmetic; both must be present so `dali-ui-components.h` exposes the new
public types.)

---

## 7. Tests — `automated-tests/src/dali-ui-components/`

### 7.1 Register the test file
Edit `automated-tests/src/dali-ui-components/CMakeLists.txt`, add to `SET(TC_SOURCES ...)`:
```
    utc-Dali-CheckBox.cpp
```

### 7.2 `utc-Dali-CheckBox.cpp` (Tier 1, GPU-free)

```cpp
// <Apache-2.0 header>

#include <dali-ui-test-suite-utils.h>
#include <dali-ui-components/dali-ui-components.h>
#include <dali-ui-foundation/public-api/views/view.h>
#include <dali-ui-foundation/public-api/views/view-accessibility-types.h>

using namespace Dali;
using namespace Dali::Ui;

void utc_dali_check_box_startup(void)
{
  test_return_value = TET_UNDEF;
}
void utc_dali_check_box_cleanup(void)
{
  test_return_value = TET_PASS;
}

namespace
{
// Observes SelectionChangedSignal by reference.
struct SelectionSpy
{
  SelectionSpy(int& count, bool& last)
  : mCount(count), mLast(last)
  {
  }
  void operator()(View, bool selected, InputEvent)
  {
    ++mCount;
    mLast = selected;
  }
  int&  mCount;
  bool& mLast;
};
} // namespace

int UtcDaliCheckBoxNewP(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();
  DALI_TEST_CHECK(cb);
  DALI_TEST_CHECK(SelectableView::DownCast(cb)); // is-a selectable
  DALI_TEST_CHECK(!cb.IsSelected());             // default unchecked
  END_TEST;
}

int UtcDaliCheckBoxCopyMoveDownCast(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  CheckBox copy(cb);
  DALI_TEST_CHECK(copy == cb);
  CheckBox moved(std::move(copy));
  DALI_TEST_CHECK(moved == cb);

  BaseHandle handle(cb);
  DALI_TEST_CHECK(CheckBox::DownCast(handle));
  DALI_TEST_CHECK(!CheckBox::DownCast(BaseHandle())); // empty for unrelated
  END_TEST;
}

int UtcDaliCheckBoxRoleIsCheckBox(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb    = CheckBox::New();
  Actor             actor = cb;
  int32_t           role  = actor.GetProperty<int32_t>(Ui::View::Property::ACCESSIBILITY_ROLE);
  DALI_TEST_EQUALS(role, static_cast<int32_t>(Accessibility::Role::CHECK_BOX), TEST_LOCATION);
  END_TEST;
}

int UtcDaliCheckBoxSelectTogglesCheckedBitPreservesEnabled(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  // Capture the ENABLED bit up front, then assert the CHECKED toggle preserves it
  // (AddAccessibilityState/RemoveAccessibilityState are additive/subtractive, not a
  // whole-bitset replacement).
  const bool enabledBefore = cb.HasAccessibilityState(Accessibility::State::ENABLED);

  cb.SetSelected(true);
  DALI_TEST_CHECK(cb.HasAccessibilityState(Accessibility::State::CHECKED));
  DALI_TEST_EQUALS(cb.HasAccessibilityState(Accessibility::State::ENABLED), enabledBefore, TEST_LOCATION);

  cb.SetSelected(false);
  DALI_TEST_CHECK(!cb.HasAccessibilityState(Accessibility::State::CHECKED));
  DALI_TEST_EQUALS(cb.HasAccessibilityState(Accessibility::State::ENABLED), enabledBefore, TEST_LOCATION);
  END_TEST;
}

int UtcDaliCheckBoxSelectionChangedFiresOncePerChange(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  int  count = 0;
  bool last  = false;
  cb.SelectionChangedSignal().Connect(&application, SelectionSpy(count, last)); // stateful functor needs a tracker

  cb.SetSelected(true);
  DALI_TEST_EQUALS(count, 1, TEST_LOCATION);
  DALI_TEST_EQUALS(last, true, TEST_LOCATION);

  cb.SetSelected(true); // same value -> no emit (short-circuit)
  DALI_TEST_EQUALS(count, 1, TEST_LOCATION);

  cb.SetSelected(false);
  DALI_TEST_EQUALS(count, 2, TEST_LOCATION);
  DALI_TEST_EQUALS(last, false, TEST_LOCATION);
  END_TEST;
}

int UtcDaliCheckBoxTextBoxOnlyAndLabel(void)
{
  UiTestApplication application(Components::UiConfig::New());

  CheckBox boxOnly = CheckBox::New();
  DALI_TEST_EQUALS(boxOnly.GetText(), std::string(""), TEST_LOCATION);

  CheckBox labelled = CheckBox::New("Agree");
  DALI_TEST_EQUALS(labelled.GetText(), std::string("Agree"), TEST_LOCATION);

  labelled.SetText("");
  DALI_TEST_EQUALS(labelled.GetText(), std::string(""), TEST_LOCATION);
  END_TEST;
}

int UtcDaliCheckBoxSelectionAnimationMode(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  DALI_TEST_EQUALS(static_cast<int>(cb.GetSelectionAnimationMode()),
                   static_cast<int>(SelectionAnimationMode::AUTO), TEST_LOCATION); // default

  cb.SetSelectionAnimationMode(SelectionAnimationMode::DISABLED);
  DALI_TEST_EQUALS(static_cast<int>(cb.GetSelectionAnimationMode()),
                   static_cast<int>(SelectionAnimationMode::DISABLED), TEST_LOCATION);

  // Off-scene programmatic changes must snap without crashing regardless of mode.
  cb.SetSelectionAnimationMode(SelectionAnimationMode::ENABLED);
  cb.SetSelected(true);
  DALI_TEST_CHECK(cb.IsSelected());
  END_TEST;
}

int UtcDaliCheckBoxStyleBuilderRoundTrip(void)
{
  UiTestApplication application(Components::UiConfig::New());

  CheckBoxStyle style = CheckBoxStyle::Builder()
                          .SetBoxSize(36.0f)
                          .SetLabelGap(8.0f)
                          .SetMinimumWidth(48.0f)
                          .SetIconUrl("i_check_box.json")
                          .Build();

  DALI_TEST_EQUALS(style.GetBoxSize(), 36.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(style.GetLabelGap(), 8.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(style.GetMinimumWidth(), 48.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(style.GetIconUrl(), std::string("i_check_box.json"), TEST_LOCATION);

  CheckBox cb = CheckBox::New(style);
  DALI_TEST_CHECK(cb);
  DALI_TEST_EQUALS(cb.GetMinimumWidth(), 48.0f, TEST_LOCATION); // inherited from View
  END_TEST;
}

int UtcDaliCheckBoxDisabledState(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  // §1.4.3: disabling uses the inherited View::SetEnabled(false), which blocks user
  // toggles via disabled interaction (DALi does NOT auto-deselect on disable).
  cb.SetEnabled(false);
  DALI_TEST_CHECK(!cb.IsEnabled());

  // Programmatic SetSelected is unaffected by disable.
  cb.SetSelected(true);
  DALI_TEST_CHECK(cb.IsSelected());

  // Note: asserting that a user CLICK is suppressed while disabled requires touch/key
  // injection (see §7 notes); the deterministic checks above cover the disable API.
  END_TEST;
}
```

> **Note (input-injection tests):** click / Return-key toggle assertions require
> generating touch/key events through the test application. Add them following the
> input-event injection pattern used by other DALi UI UTCs (generate a touch down+up
> over the actor, then assert `IsSelected()` flipped and the spy fired once). The
> deterministic `SetSelected`-driven tests above already cover the state machine,
> signal ordering, a11y bit, animation-mode API, and box-only/label behaviour.
> **Note (execution key):** assert key activation against the shipped Components
> `UiConfig` key-click policy / execution-key predicate, not a hard-coded Return-only or
> Return+Space assumption.

---

## 8. Build & test

**Environment (export all four; `~/setenv` is NOT auto-sourced in tool shells):**
```sh
export PKG_CONFIG_PATH=/home/jae/dali/dali-core/dali-env/opt/lib/pkgconfig:/usr/lib/pkgconfig:/usr/share/pkgconfig
export LD_LIBRARY_PATH=/home/jae/dali/dali-core/dali-env/opt/lib:$LD_LIBRARY_PATH
export PATH=/home/jae/dali/dali-core/dali-env/opt/bin:$PATH
export DESKTOP_PREFIX=/home/jae/dali/dali-core/dali-env/opt
```

**1) Build & install the component library** (configure into a dir OUTSIDE `/tmp`;
`ROOT` = current checkout root, e.g. `/home/jae/dali/dali-ui`):
```sh
BD=/home/jae/dali-ui-cbuild && mkdir -p $BD && cd $BD
CXXFLAGS="-g -O0" cmake $ROOT/build/tizen \
  -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX -DCMAKE_BUILD_TYPE=Debug -DENABLE_PKG_CONFIGURE=ON
make dali2-ui-components -j4
make -C dali-ui-components install     # installs headers + .so to $DESKTOP_PREFIX
```
New `.cpp` are auto-globbed; only the umbrella-header edit (§6) is manual.

Alternatively, build everything (source + automated-tests + samples) at once with the
project script: `/home/jae/dali/build_all.sh`.

**2) Build the component test suite:**
```sh
cd $ROOT/automated-tests && ./build.sh dali-ui-components
```
(Adding a new `utc-*.cpp` requires the CMake edit in §7.1; after adding/renaming a
TEST CASE, regenerate the tct header first:
`(cd src/dali-ui-components && ../../scripts/tcheadgen.sh tct-dali-ui-components-core.h)`.)

**3) Run the tests:**
```sh
cd $ROOT/automated-tests/build/src/dali-ui-components
DALI_WINDOW_WIDTH=480 DALI_WINDOW_HEIGHT=800 \
  ./tct-dali-ui-components-core UtcDaliCheckBoxNewP        # exit 0 = pass
# ... repeat per case, or run the whole suite with no arg
```

**Fast type-check of one TU without a full build** (working-tree headers + installed dali):
```sh
g++ -std=c++17 -fsyntax-only -DHIDE_DALI_INTERNALS \
  -I$ROOT -I$DESKTOP_PREFIX/include -I$DESKTOP_PREFIX/include/dali \
  $ROOT/dali-ui-components/internal/check-box-impl.cpp
```

> **Known env caveat (verify first):** a prior session found the `dali2-ui-components`
> test link blocked because the package wasn't installed
> (`make -C dali-ui-components install` addresses this). If `./build.sh dali-ui-components`
> fails at link, confirm the lib installed into `$DESKTOP_PREFIX` and that
> `pkg-config --exists dali2-ui-components` succeeds.

---

## 9. Verification checklist (Tier 1 — behaviour-compatible, no pixel compare)

- [ ] Library builds (`make dali2-ui-components`) and installs.
- [ ] `UtcDaliCheckBoxNewP` — handle valid, is-a `SelectableView`, default unchecked.
- [ ] `UtcDaliCheckBoxCopyMoveDownCast` — copy/move + DownCast round-trip (+ empty for unrelated).
- [ ] `UtcDaliCheckBoxRoleIsCheckBox` — `ACCESSIBILITY_ROLE == Accessibility::Role::CHECK_BOX`.
- [ ] `UtcDaliCheckBoxSelectTogglesCheckedBitPreservesEnabled` — `HasAccessibilityState(CHECKED)`
      follows `selected`; other bits untouched by add/remove.
- [ ] `UtcDaliCheckBoxSelectionChangedFiresOncePerChange` — one emit per genuine change,
      correct bool, `SetSelected(same)` emits zero.
- [ ] `UtcDaliCheckBoxTextBoxOnlyAndLabel` — box-only ↔ labelled via `SetText`.
- [ ] `UtcDaliCheckBoxSelectionAnimationMode` — default AUTO, get/set, off-scene set snaps.
- [ ] `UtcDaliCheckBoxStyleBuilderRoundTrip` — Builder set/get (icon url) + `New(style)`.
- [ ] Add & pass input-injection click / execution-key toggle tests (see §7 notes).
- [ ] (After asset) manual test: select plays `[0,30]`, deselect plays `[30,48]`, snaps on
      programmatic set, inner fill recolours (deselected token at rest frames 0/38).
- [ ] (After asset) theme change re-resolves inner-fill colours; visibility/scene re-attach
      shows the correct resting frame.
- [ ] RTL: icon/label *positions* mirror (framework); the Lottie artwork does NOT mirror.
- [ ] `grep` guard on the new `.h/.cpp/UTC` files: no `EnableX/GetXEnabled` naming, no
      `DALI_UI_CHAIN_VIEW_METHODS`, no `gettext`/`po/`, and no competitor-framework names.

---

## 10. Gotchas / notes (all verified)

1. **Base-first `OnInitialize`:** call `Provider::SelectableViewImpl::OnInitialize()`
   before adding children / setting the role — the SelectableTrait must be attached first
   (that is what makes `SelectionChangedSignal()` valid).
2. **No virtual `OnSelectedChanged` hook** — observe state via
   `SelectionChangedSignal().Connect(this, ...)`. `ViewImpl` is a `ConnectionTracker`, so
   `Connect(this, &Method)` is valid and auto-disconnects on destroy.
3. **CHECKED ≠ SELECTED.** `SetSelected` only raises `ViewState::SELECTED`. Mirror
   `selected → CHECKED` separately with `AddAccessibilityState/RemoveAccessibilityState`
   (NOT a non-existent `ACCESSIBILITY_STATES` property). The CHECK_BOX role gates the AT
   emit; add/remove preserves `ENABLED` and other bits by construction.
4. **Animate vs snap is driven by the change's cause.** Use the `SelectionChangedSignal`
   `InputEvent`: `IsProgrammatic()` (a `SetSelected` from code) ⇒ snap; a user gesture/key
   ⇒ animate (in AUTO). Also require `IsOnScene() && IsVisible()`.
5. **Lottie recolour callback runs on a worker thread** — never call a DALi API in it;
   read only pre-resolved `Vector4` colours (resolve tokens via `GetRgba()` on the main
   thread). `MakeCallback` here wraps a free function, so per-instance colours are keyed
   by `DynamicPropertyInfo.id` through a registry — see the `TODO(verify)` in §5.4.
6. **Resetting a dynamic property = reloading the url** (no explicit remove API). Re-seat
   the frame with `JumpToFrame` afterwards.
7. **`GetImpl(Ui::LottieAnimationView&)` is not public** — upcast to `Ui::View` and use the
   public `GetImpl(Ui::View&)` (`view-impl.h`) to reach `Measure`/`Arrange`.
8. **RTL is automatic** — arrange children left-to-right; the framework mirrors positions.
   Never mirror manually. The Lottie artwork (direction-independent) must not flip.
9. **Do NOT** add a `Property::CHECKED` index or redeclare inherited selection methods;
   state lives in the trait (`mSelected`) and is exposed only via the inherited methods
   + signal.
10. **Numbers/assets/keyPath are placeholders** — replace every `TODO(verify)` /
    `TODO(assets)` in §2/§5 with confirmed One UI 8 values, the real json url(s), and the
    real inner-fill `keyPath` before shipping. Empty url compiles and passes all logic
    tests but renders nothing.
11. **Descoped divergences (§1.4):** press-scale suppression, click sound, and a mutable
    `IsSelectable` gate are intentionally not ported; do not silently "fix" them without a
    foundation-API discussion.

---

*Design of record:* the reviewed extracted-spec at
`dali-ui-components/docs/extracted-spec-checkbox.md` established the standalone/binary
scope, base pair, accessibility, and Tier-1 verification; this plan is its concrete,
build-ready realization, updated to render the glyph as a single Lottie animation (with
per-frame recolour + a `SelectionAnimationMode` policy) ported from the NUI reference.
```
