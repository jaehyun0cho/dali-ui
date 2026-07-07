# CheckBox — Self-Contained Implementation Plan

> **You can implement the DALi UI `CheckBox` component from this file alone.**
> Every DALi symbol, include path, file, build command, and test used below was
> verified against the live source of the `dali-ui-claude` checkout
> (`/home/jae/dali/dali-ui-claude`). Copy the code blocks verbatim, fill the
> values marked `TODO(verify)`, provide the SVG assets, then build & test.

---

## 0. How to use this document

1. Read §1 (frozen scope) and §2 (what you must supply).
2. Create the 7 source files in §5 exactly as written (they are complete, not sketches).
3. Apply the one umbrella-header edit in §6.
4. Add the UTC file + CMake line in §7.
5. Build & run per §8; tick off §9.
6. §3 is a reference appendix — the exact signatures of every foundation API used,
   so you never need to open another header.

**Repo root** is referred to as `$ROOT` = `/home/jae/dali/dali-ui-claude`.
All source paths are relative to `$ROOT`.

---

## 1. Scope & decisions (FROZEN — do not re-open)

- **Component:** `Dali::Ui::CheckBox` only. No group / no `CheckBoxGroup`.
  (DALi's `SelectionGroup`/`GroupSelectableView` are *radio / mutually-exclusive*
  and are the wrong semantics for a checkbox — do not use them.)
- **State model:** BINARY — `checked` / `unchecked`. **No indeterminate / tri-state.**
  Selection is the inherited `IsSelected/SetSelected/SelectionChangedSignal/
  IsToggleByClickEnabled/SetToggleByClickEnabled` from `SelectableView` — **do not
  redeclare them** and **do not add a `SetIndeterminate`**.
- **Anatomy:** leading **box glyph** + **optional trailing text label** (One UI
  standalone anatomy: control leads, label trailing; the two form one hit target).
  Empty text ⇒ box-only.
- **GUI = SVG resources**, rendered with **two overlaid `Ui::ImageView` layers**:
  - `mBox` — the box; swaps *unchecked-outline* ↔ *checked-filled* SVG, tinted by a
    `UiColor` token (`OUTLINE` when unchecked, `PRIMARY`/control-activated when checked).
  - `mTick` — the checkmark; a separate SVG overlaid on the box, tinted `ON_PRIMARY`,
    made transparent when unchecked (alpha 0). *(Two layers are required because a
    single `mixColor` multiply cannot render two colours — box fill + tick — on one
    monochrome asset.)*
  Corner-radius / outline-width / tick-stroke geometry lives **inside the SVG**, not
  as DALi numeric fields.
- **Accessibility role** = `AccessibilityRole::CHECK_BOX`; the `CHECKED` state bit is
  mirrored from `selected`.
- **Numbers & assets are deferred** (§2). Colours are `UiColor` semantic tokens (hex
  resolved by the theme). Every concrete dp/ms/URL is a `TODO(verify)` placeholder —
  none is a real One UI value yet.
- **Verification target:** behaviour-compatible (Tier 1, GPU-free). No pixel/GUI
  comparison.
- Must build/run under **dali-core + dali-ui** (the SelectableView base and the SVG
  `ImageView` visual are both dali-core-compatible).

---

## 2. What YOU must supply before/at implementation (deferred inputs)

### 2.1 Numeric parameters — fill in `CheckBoxStyleImpl` defaults (§5.6)

| Field | Meaning | Placeholder (replace) | Unit |
|---|---|---|---|
| `mMinimumWidth`, `mMinimumHeight` | whole-control min touch target | `48.0f` | dp |
| `mPadding` | content inset (start,end,top,bottom) | `Extents(0,0,0,0)` | dp |
| `mBoxSize` | box glyph side (square) | `24.0f` | dp |
| `mLabelGap` | gap between box and label | `8.0f` | dp |
| check/uncheck motion duration | (optional cross-fade) | not implemented (instant) | ms, clamp 100–500 |

*(dp values are logical; the impl multiplies them by `GetEffectiveScale()`.)*

### 2.2 SVG assets — provide the files and their URLs

| Style field | Asset | Author as |
|---|---|---|
| `mUncheckedUrl` | unchecked box (outline square) | monochrome / white, so token tint works |
| `mCheckedUrl` | checked box (filled square) | monochrome / white |
| `mTickUrl` | checkmark | monochrome / white |

- Set the default URLs in `CheckBoxStyleImpl()` (§5.6) to the **installed asset paths**
  (or leave empty and have the app/theme set them via `CheckBoxStyle::Builder`).
- `ImageView::SetResourceUrl` auto-routes a `.svg` URL to the SVG (vector) visual.
- **Path resolution `TODO(verify)`:** decide where the component's SVGs install and
  how the URL is formed (e.g. an installed resource dir, or app-supplied absolute
  path). Until real assets exist, an empty URL renders nothing but everything still
  compiles and all Tier-1 logic tests pass.

### 2.3 Colour tokens (already chosen — hex resolved by theme, nothing to supply)

`OUTLINE` (unchecked box), `PRIMARY` (checked fill / control-activated),
`ON_PRIMARY` (tick), `ON_SURFACE` (label).

---

## 3. Verified foundation API reference (no other file needed)

### 3.1 Base classes / inheritance
- `class Dali::Ui::SelectableView : public InteractiveView` — header
  `<dali-ui-foundation/public-api/views/selectable-view.h>`. Inherited API you REUSE
  (do not redeclare): `Signal<void(View,bool,InputEvent)>& SelectionChangedSignal();`
  `bool IsSelected() const;` `void SetSelected(bool);` `bool IsToggleByClickEnabled() const;`
  `void SetToggleByClickEnabled(bool enabled=true);`
- `class Dali::Ui::Provider::SelectableViewImpl : public InteractiveViewImpl` — header
  `<dali-ui-foundation/provider-api/selectable-view-impl.h>`. Overridable: `void OnInitialize() override;`
  (call `SelectableViewImpl::OnInitialize()` FIRST). It also declares `SelectionChangedSignal()`,
  `IsSelected()`, `SetSelected(bool)` etc. usable from the impl. Public helper in this
  header: `GetImpl(SelectableView&) -> SelectableViewImpl&`.
- `SelectableView : InteractiveView : View : CustomActor : Actor` — so a CheckBox handle
  IS-A `Dali::Actor` and IS-A `Ui::View`.

### 3.2 ViewImpl services available inside the impl (from `SelectableViewImpl` → `ViewImpl`)
`Self()` (returns the `CustomActor`), `Initialize()`, `GetEffectiveScale()`,
`GetPadding()` (`Extents`), `GetRequestedWidth()/GetRequestedHeight()`,
`GetMinimumWidth()/GetMinimumHeight()`, `InvalidateMeasure()`.
Overridables: `OnInitialize()`, `MeasuredSize OnMeasure(float,float)`,
`MeasuredSize OnArrange(const LayoutRect&)`.
`ViewImpl` implements `ConnectionTrackerInterface`, so `signal.Connect(this, &Impl::Method)` is valid.

### 3.3 Laying out child `Ui::View`s (box, tick, label)
- Public helper (`<dali-ui-foundation/public-api/views/view-impl.h>`):
  `inline ViewImpl& GetImpl(Ui::View& view);` and its const overload. `ViewImpl` has
  PUBLIC `MeasuredSize Measure(float,float)` and `MeasuredSize Arrange(const LayoutRect&)`.
- **`GetImpl(Ui::ImageView&)` is NOT public** (it lives inside image-view.cpp). To
  measure/arrange an `ImageView` child, upcast it to `Ui::View` and use the public
  `GetImpl(Ui::View&)`:
  ```cpp
  Ui::View v = mBox;            // ImageView -> View (base-handle copy)
  GetImpl(v).Measure(w, h);
  GetImpl(v).Arrange(rect);
  ```
- `GetImpl(Ui::Label&) -> LabelImpl&` IS public (`<...text-controls/label.h>`), used
  directly like TextButton does: `GetImpl(mLabel).Measure(...)/.Arrange(...)`.

### 3.4 `Ui::ImageView` — header `<dali-ui-foundation/public-api/views/image/image-view.h>` (SVG box/tick)
`static ImageView New();` `void SetResourceUrl(const Dali::String& url);`
`Dali::String GetResourceUrl() const;` `void SetImageColor(const UiColor& color);`
`UiColor GetImageColor() const;` `void SetDesiredWidth(int);` `void SetDesiredHeight(int);`
A `.svg` URL is auto-routed to the SVG visual. `SetImageColor` multiplies (a monochrome
white asset takes the token colour; alpha 0 hides it).

### 3.5 `Ui::Label` — header `<dali-ui-foundation/public-api/views/text-controls/label.h>`
`static Label New();` `void SetText(const Dali::String&);` `Dali::String GetText() const;`
`void SetTextColor(const UiColor&);`

### 3.6 `UiColor` — header `<dali-ui-foundation/public-api/types/ui-color.h>`
Static theme tokens: `UiColor::PRIMARY`, `ON_PRIMARY`, `ON_SURFACE`, `OUTLINE`
(also `BACKGROUND`, `SURFACE`). Ctors: `UiColor(float r,g,b,a=1)`, `UiColor(uint32_t rgb,float a=1)`.
Helpers: `UiColor WithAlpha(float) const;` `Vector4 GetRgba() const;`
(implicit `operator Vector4()`).

### 3.7 Style base — headers `<...styles/ui-style.h>`, `<...styles/ui-style-key.h>`,
provider `<...provider-api/styles/ui-style-impl.h>`
`class UiStyle : public BaseHandle` (ctor `UiStyle(Internal*)`, `GetBaseObject()`).
`template<T> class UiStyleKey { static UiStyleKey Alloc(); };`
`class Provider::UiStyleImpl` — the impl base; protected `~() override`.
`StateEffect` — `<...views/effects/state-effect.h>`: `static StateEffect DefaultForInteractive();`
`static StateEffect None();` (a null `StateEffect` is falsy).
`UiConfig::GetCurrent().GetStyle(key)` and `DebugAssertStyleConfigApplied()` —
`<...configuration/ui-config.h>`, `<...provider-api/styles/ui-style-debug.h>`.

### 3.8 Accessibility — header `<dali-ui-foundation/public-api/views/view-accessibility-enums.h>`
`enum class AccessibilityRole : uint32_t { ... CHECK_BOX ... };` (namespace `Dali::Ui`).
`enum class AccessibilityState : uint32_t { ENABLED=0, SELECTED, CHECKED, BUSY, EXPANDED, MAX_COUNT };`
Property indices (INTEGER) on the view's actor — header `<...views/view.h>`:
`Ui::View::Property::ACCESSIBILITY_ROLE`, `Ui::View::Property::ACCESSIBILITY_STATES`.
Read/write with `Actor::GetProperty<int32_t>(index)` / `Actor::SetProperty(index, int32_t)`.
Writing `ACCESSIBILITY_STATES` auto-emits the AT `CHECKED` change (role gate satisfied by
CHECK_BOX); use read-modify-write to preserve `ENABLED`. `CHECKED` mask =
`1 << int(AccessibilityState::CHECKED)`.

### 3.9 Layout types — header `<dali-ui-foundation/public-api/layouts/layout-types.h>`
`struct MeasuredSize { float width; float height; MeasuredSize(float,float); };`
`struct LayoutRect { float x, y, width, height; };` sentinel `MATCH_PARENT`.

### 3.10 Handle plumbing helpers (from `view.h`, available transitively)
`Ui::View::DownCast<Handle,Impl>(BaseHandle)` (typed downcast),
`VerifyCustomActorPointer<Impl>(Dali::Internal::CustomActor*)`,
`DALI_UI_VIEW_WITH(<Name>)` — declares a `With(action,...)` extension hook only
(NOT fluent chaining, NOT plumbing). `DALI_UI_API` / `DALI_INTERNAL` export macros.

### 3.11 Type registration (from `<dali/devel-api/object/type-registry-helper.h>`,`type-registry.h`)
```cpp
namespace { BaseHandle Create() { return BaseHandle(); }
DALI_TYPE_REGISTRATION_BEGIN(CheckBoxImpl, Provider::SelectableViewImpl, Create)
DALI_TYPE_REGISTRATION_END() }
```

### 3.12 Build integration (verified)
- Component sources are **glob-collected** (`build/tizen/dali-ui-components/CMakeLists.txt`
  `FILE(GLOB_RECURSE ...)`); new `.cpp` need **no** build-file edit.
- The **only** manual edit is the umbrella header `dali-ui-components/dali-ui-components.h` (§6).
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

Every file starts with the standard Apache-2.0 header (2026 Samsung Electronics) —
copy it from any existing file in the repo.

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

namespace Dali
{
namespace Ui
{
namespace Internal
{
class CheckBoxImpl;
}

/**
 * @brief CheckBox is a binary (checked/unchecked) selectable control with an
 *        optional trailing text label. Its box and checkmark are SVG resources.
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
#include <dali-ui-foundation/public-api/views/image/image-view.h>
#include <dali-ui-foundation/public-api/views/text-controls/label.h>
#include <dali-ui-foundation/public-api/views/view.h>
#include <dali/public-api/common/dali-string.h>

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

protected:
  void         OnInitialize() override;
  MeasuredSize OnMeasure(float widthConstraint, float heightConstraint) override;
  MeasuredSize OnArrange(const LayoutRect& bounds) override;

  CheckBoxImpl();
  ~CheckBoxImpl() override;

private:
  void ApplyInitialStyle(CheckBoxStyle style);
  void OnSelectionChanged(View view, bool selected, InputEvent event);
  void UpdateVisualState(bool selected);

private:
  Ui::ImageView mBox;   ///< leading box: unchecked-outline <-> checked-filled SVG
  Ui::ImageView mTick;  ///< checkmark overlaid on the box (alpha 0 when unchecked)
  Ui::Label     mLabel; ///< optional trailing label

  Dali::String  mText;

  // Captured from the style in ApplyInitialStyle():
  float        mBoxSize{0.0f};
  float        mGap{0.0f};
  Dali::String mUncheckedUrl;
  Dali::String mCheckedUrl;
  Dali::String mTickUrl;
  UiColor      mOutlineColor;
  UiColor      mFillColor;
  UiColor      mTickColor;
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
#include <dali/devel-api/object/type-registry-helper.h>
#include <dali/devel-api/object/type-registry.h>
#include <algorithm>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/views/view-accessibility-enums.h>
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

const int32_t CHECKED_MASK = 1 << static_cast<int32_t>(AccessibilityState::CHECKED);

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
  InvalidateMeasure(); // label presence changes the measured size
}

Dali::String CheckBoxImpl::GetText() const
{
  return mText;
}

void CheckBoxImpl::OnInitialize()
{
  Provider::SelectableViewImpl::OnInitialize(); // base first

  // Accessibility role.
  Actor self = Self();
  self.SetProperty(Ui::View::Property::ACCESSIBILITY_ROLE,
                   static_cast<int32_t>(AccessibilityRole::CHECK_BOX));

  // Leading box layer.
  mBox = Ui::ImageView::New();
  Self().Add(mBox);

  // Checkmark overlay (same rect as the box; hidden until checked).
  mTick = Ui::ImageView::New();
  Self().Add(mTick);

  // Optional trailing label (empty until SetText()).
  mLabel = Ui::Label::New();
  Self().Add(mLabel);

  // React to selection changes (there is no virtual OnSelectedChanged hook on the base).
  SelectionChangedSignal().Connect(this, &CheckBoxImpl::OnSelectionChanged);
}

void CheckBoxImpl::ApplyInitialStyle(CheckBoxStyle style)
{
  Ui::View self = Ui::View::DownCast(Self());
  self.SetMinimumWidth(style.GetMinimumWidth());
  self.SetMinimumHeight(style.GetMinimumHeight());
  self.SetPadding(style.GetPadding());
  self.SetStateEffect(style.GetStateEffect());

  mBoxSize = style.GetBoxSize();
  mGap     = style.GetLabelGap();

  mUncheckedUrl = style.GetUncheckedBoxUrl();
  mCheckedUrl   = style.GetCheckedBoxUrl();
  mTickUrl      = style.GetTickUrl();

  mOutlineColor = style.GetOutlineColor();
  mFillColor    = style.GetFillColor();
  mTickColor    = style.GetTickColor();

  // Crisp SVG rasterization at the box size (square).
  const int px = static_cast<int>(mBoxSize);
  mTick.SetResourceUrl(mTickUrl);
  mTick.SetDesiredWidth(px);
  mTick.SetDesiredHeight(px);
  mBox.SetDesiredWidth(px);
  mBox.SetDesiredHeight(px);

  mLabel.SetTextColor(style.GetLabelColor());

  // Reflect the initial (unchecked) state onto the two layers + a11y bit.
  UpdateVisualState(IsSelected());
}

void CheckBoxImpl::OnSelectionChanged(View /*view*/, bool selected, InputEvent /*event*/)
{
  UpdateVisualState(selected);
}

void CheckBoxImpl::UpdateVisualState(bool selected)
{
  // Box artwork + tint.
  mBox.SetResourceUrl(selected ? mCheckedUrl : mUncheckedUrl);
  mBox.SetImageColor(selected ? mFillColor : mOutlineColor);

  // Checkmark overlay: ON_PRIMARY when checked, fully transparent when unchecked.
  mTick.SetImageColor(selected ? mTickColor : mTickColor.WithAlpha(0.0f));

  // Mirror the accessibility CHECKED bit (read-modify-write; preserve ENABLED etc.).
  Actor   self   = Self();
  int32_t states = self.GetProperty<int32_t>(Ui::View::Property::ACCESSIBILITY_STATES);
  states         = selected ? (states | CHECKED_MASK) : (states & ~CHECKED_MASK);
  self.SetProperty(Ui::View::Property::ACCESSIBILITY_STATES, states);
}

MeasuredSize CheckBoxImpl::OnMeasure(float widthConstraint, float heightConstraint)
{
  float s = GetEffectiveScale();

  Extents padding = GetPadding();
  float   visPadW = static_cast<float>(padding.start + padding.end) * s;
  float   visPadH = static_cast<float>(padding.top + padding.bottom) * s;

  float boxVis   = mBoxSize * s;
  float gapVis   = mGap * s;
  bool  hasLabel = !mText.empty();

  float requestedWidth  = GetRequestedWidth();
  float requestedHeight = GetRequestedHeight();
  float requestedVisW   = (requestedWidth >= 0.0f) ? requestedWidth * s : requestedWidth;
  float requestedVisH   = (requestedHeight >= 0.0f) ? requestedHeight * s : requestedHeight;

  float effectiveVisW = (requestedVisW >= 0.0f) ? requestedVisW : widthConstraint;
  float effectiveVisH = (requestedVisH >= 0.0f) ? requestedVisH : heightConstraint;
  float contentVisW   = (effectiveVisW >= 0.0f) ? std::max(0.0f, effectiveVisW - visPadW) : effectiveVisW;
  float contentVisH   = (effectiveVisH >= 0.0f) ? std::max(0.0f, effectiveVisH - visPadH) : effectiveVisH;

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

  float boxVis = mBoxSize * s;
  float gapVis = mGap * s;

  // Box (leading, vertically centered). The framework mirrors this under RTL; the
  // tick artwork inside the box is direction-independent and is NOT mirrored.
  LayoutRect boxRect;
  boxRect.x      = contentX;
  boxRect.y      = contentY + std::max(0.0f, (contentH - boxVis) * 0.5f);
  boxRect.width  = boxVis;
  boxRect.height = boxVis;

  Ui::View boxView = mBox; // ImageView -> View, use public GetImpl(Ui::View&)
  GetImpl(boxView).Measure(boxRect.width, boxRect.height);
  GetImpl(boxView).Arrange(boxRect);

  Ui::View tickView = mTick; // tick occupies the same rect as the box
  GetImpl(tickView).Measure(boxRect.width, boxRect.height);
  GetImpl(tickView).Arrange(boxRect);

  // Optional trailing label.
  LayoutRect labelRect;
  labelRect.x      = contentX + boxVis + gapVis;
  labelRect.y      = contentY;
  labelRect.width  = mText.empty() ? 0.0f : std::max(0.0f, contentW - boxVis - gapVis);
  labelRect.height = contentH;
  GetImpl(mLabel).Measure(labelRect.width, labelRect.height);
  GetImpl(mLabel).Arrange(labelRect);

  return MeasuredSize(bounds.width, bounds.height);
}

CheckBoxImpl::CheckBoxImpl()  = default;
CheckBoxImpl::~CheckBoxImpl() = default;

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
  static CheckBoxStyle             Default();
  static CheckBoxStyle             DownCast(BaseHandle handle);
  static CheckBoxStyle             StaticDownCast(UiStyle style);

  Builder Configure() const;

  float   GetMinimumWidth() const;
  float   GetMinimumHeight() const;
  Extents GetPadding() const;

  float GetBoxSize() const;
  float GetLabelGap() const;

  Dali::String GetUncheckedBoxUrl() const;
  Dali::String GetCheckedBoxUrl() const;
  Dali::String GetTickUrl() const;

  UiColor GetOutlineColor() const;
  UiColor GetFillColor() const;
  UiColor GetTickColor() const;
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

  Builder&  SetUncheckedBoxUrl(const Dali::String& url) &;
  Builder&& SetUncheckedBoxUrl(const Dali::String& url) &&;
  Builder&  SetCheckedBoxUrl(const Dali::String& url) &;
  Builder&& SetCheckedBoxUrl(const Dali::String& url) &&;
  Builder&  SetTickUrl(const Dali::String& url) &;
  Builder&& SetTickUrl(const Dali::String& url) &&;

  Builder&  SetOutlineColor(const UiColor& color) &;
  Builder&& SetOutlineColor(const UiColor& color) &&;
  Builder&  SetFillColor(const UiColor& color) &;
  Builder&& SetFillColor(const UiColor& color) &&;
  Builder&  SetTickColor(const UiColor& color) &;
  Builder&& SetTickColor(const UiColor& color) &&;
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

// EXTERNAL INCLUDES
#include <utility>

namespace Dali
{
namespace Ui
{
namespace
{
StateEffect CreateDefaultCheckBoxStateEffect()
{
  return StateEffect::DefaultForInteractive();
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
Dali::String CheckBoxStyle::GetUncheckedBoxUrl() const { return GetImpl(*this).GetUncheckedBoxUrl(); }
Dali::String CheckBoxStyle::GetCheckedBoxUrl() const { return GetImpl(*this).GetCheckedBoxUrl(); }
Dali::String CheckBoxStyle::GetTickUrl() const { return GetImpl(*this).GetTickUrl(); }
UiColor      CheckBoxStyle::GetOutlineColor() const { return GetImpl(*this).GetOutlineColor(); }
UiColor      CheckBoxStyle::GetFillColor() const { return GetImpl(*this).GetFillColor(); }
UiColor      CheckBoxStyle::GetTickColor() const { return GetImpl(*this).GetTickColor(); }
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
#define CBS_SETTER(Name, Type, Field)                                            \
  CheckBoxStyle::Builder& CheckBoxStyle::Builder::Name(Type v) &                 \
  {                                                                              \
    mImpl->Name(v);                                                             \
    return *this;                                                                \
  }                                                                              \
  CheckBoxStyle::Builder&& CheckBoxStyle::Builder::Name(Type v) &&               \
  {                                                                              \
    Name(v);                                                                     \
    return std::move(*this);                                                     \
  }

CBS_SETTER(SetMinimumWidth, float, mMinimumWidth)
CBS_SETTER(SetMinimumHeight, float, mMinimumHeight)
CBS_SETTER(SetPadding, const Extents&, mPadding)
CBS_SETTER(SetBoxSize, float, mBoxSize)
CBS_SETTER(SetLabelGap, float, mLabelGap)
CBS_SETTER(SetUncheckedBoxUrl, const Dali::String&, mUncheckedUrl)
CBS_SETTER(SetCheckedBoxUrl, const Dali::String&, mCheckedUrl)
CBS_SETTER(SetTickUrl, const Dali::String&, mTickUrl)
CBS_SETTER(SetOutlineColor, const UiColor&, mOutlineColor)
CBS_SETTER(SetFillColor, const UiColor&, mFillColor)
CBS_SETTER(SetTickColor, const UiColor&, mTickColor)
CBS_SETTER(SetLabelColor, const UiColor&, mLabelColor)
CBS_SETTER(SetStateEffect, StateEffect, mStateEffect)
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
void         CheckBoxStyleImpl::SetUncheckedBoxUrl(const Dali::String& v) { mUncheckedUrl = v; }
Dali::String CheckBoxStyleImpl::GetUncheckedBoxUrl() const { return mUncheckedUrl; }
void         CheckBoxStyleImpl::SetCheckedBoxUrl(const Dali::String& v) { mCheckedUrl = v; }
Dali::String CheckBoxStyleImpl::GetCheckedBoxUrl() const { return mCheckedUrl; }
void         CheckBoxStyleImpl::SetTickUrl(const Dali::String& v) { mTickUrl = v; }
Dali::String CheckBoxStyleImpl::GetTickUrl() const { return mTickUrl; }
void         CheckBoxStyleImpl::SetOutlineColor(const UiColor& v) { mOutlineColor = v; }
UiColor      CheckBoxStyleImpl::GetOutlineColor() const { return mOutlineColor; }
void         CheckBoxStyleImpl::SetFillColor(const UiColor& v) { mFillColor = v; }
UiColor      CheckBoxStyleImpl::GetFillColor() const { return mFillColor; }
void         CheckBoxStyleImpl::SetTickColor(const UiColor& v) { mTickColor = v; }
UiColor      CheckBoxStyleImpl::GetTickColor() const { return mTickColor; }
void         CheckBoxStyleImpl::SetLabelColor(const UiColor& v) { mLabelColor = v; }
UiColor      CheckBoxStyleImpl::GetLabelColor() const { return mLabelColor; }
void         CheckBoxStyleImpl::SetStateEffect(StateEffect v) { mStateEffect = v ? v : StateEffect::None(); }
StateEffect  CheckBoxStyleImpl::GetStateEffect() const { return mStateEffect; }

CheckBoxStyleImpl::CheckBoxStyleImpl()
: mMinimumWidth(48.0f),                    // TODO(verify): One UI 8 min touch target
  mMinimumHeight(48.0f),                   // TODO(verify)
  mPadding(0u, 0u, 0u, 0u),                // TODO(verify)
  mBoxSize(24.0f),                         // TODO(verify): One UI 8 box side
  mLabelGap(8.0f),                         // TODO(verify): box<->label gap
  mUncheckedUrl(""),                       // TODO(assets): unchecked box SVG URL
  mCheckedUrl(""),                         // TODO(assets): checked box SVG URL
  mTickUrl(""),                            // TODO(assets): checkmark SVG URL
  mOutlineColor(UiColor::OUTLINE),
  mFillColor(UiColor::PRIMARY),            // control-activated accent
  mTickColor(UiColor::ON_PRIMARY),
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
  mUncheckedUrl(rhs.mUncheckedUrl),
  mCheckedUrl(rhs.mCheckedUrl),
  mTickUrl(rhs.mTickUrl),
  mOutlineColor(rhs.mOutlineColor),
  mFillColor(rhs.mFillColor),
  mTickColor(rhs.mTickColor),
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

  void         SetUncheckedBoxUrl(const Dali::String& url);
  Dali::String GetUncheckedBoxUrl() const;
  void         SetCheckedBoxUrl(const Dali::String& url);
  Dali::String GetCheckedBoxUrl() const;
  void         SetTickUrl(const Dali::String& url);
  Dali::String GetTickUrl() const;

  void    SetOutlineColor(const UiColor& color);
  UiColor GetOutlineColor() const;
  void    SetFillColor(const UiColor& color);
  UiColor GetFillColor() const;
  void    SetTickColor(const UiColor& color);
  UiColor GetTickColor() const;
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
  Dali::String mUncheckedUrl;
  Dali::String mCheckedUrl;
  Dali::String mTickUrl;
  UiColor      mOutlineColor;
  UiColor      mFillColor;
  UiColor      mTickColor;
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
#include <dali-ui-foundation/public-api/views/view-accessibility-enums.h>

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
// Observes SelectionChangedSignal by reference (the signal stores a copy of this
// functor, but the references point back at the caller's locals).
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

int32_t GetAccessibilityStates(CheckBox cb)
{
  Actor actor = cb;
  return actor.GetProperty<int32_t>(Ui::View::Property::ACCESSIBILITY_STATES);
}
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

int UtcDaliCheckBoxDownCastP(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();
  BaseHandle        handle(cb);
  DALI_TEST_CHECK(CheckBox::DownCast(handle));
  END_TEST;
}

int UtcDaliCheckBoxRoleIsCheckBox(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();
  Actor             actor = cb;
  int32_t role = actor.GetProperty<int32_t>(Ui::View::Property::ACCESSIBILITY_ROLE);
  DALI_TEST_EQUALS(role, static_cast<int32_t>(AccessibilityRole::CHECK_BOX), TEST_LOCATION);
  END_TEST;
}

int UtcDaliCheckBoxSelectTogglesCheckedBitPreservesEnabled(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  const int32_t CHECKED  = 1 << static_cast<int32_t>(AccessibilityState::CHECKED);

  cb.SetSelected(true);
  DALI_TEST_CHECK((GetAccessibilityStates(cb) & CHECKED) != 0);

  cb.SetSelected(false);
  DALI_TEST_CHECK((GetAccessibilityStates(cb) & CHECKED) == 0);
  END_TEST;
}

int UtcDaliCheckBoxSelectionChangedFiresOncePerChange(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  int  count = 0;
  bool last  = false;
  cb.SelectionChangedSignal().Connect(SelectionSpy(count, last));

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

int UtcDaliCheckBoxStyleBuilderRoundTrip(void)
{
  UiTestApplication application(Components::UiConfig::New());

  CheckBoxStyle style = CheckBoxStyle::Builder()
                          .SetBoxSize(24.0f)
                          .SetLabelGap(8.0f)
                          .SetMinimumWidth(48.0f)
                          .SetUncheckedBoxUrl("cb_unchecked.svg")
                          .SetCheckedBoxUrl("cb_checked.svg")
                          .SetTickUrl("cb_tick.svg")
                          .Build();

  DALI_TEST_EQUALS(style.GetBoxSize(), 24.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(style.GetLabelGap(), 8.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(style.GetMinimumWidth(), 48.0f, TEST_LOCATION);
  DALI_TEST_EQUALS(style.GetUncheckedBoxUrl(), std::string("cb_unchecked.svg"), TEST_LOCATION);

  CheckBox cb = CheckBox::New(style);
  DALI_TEST_CHECK(cb);
  DALI_TEST_EQUALS(cb.GetMinimumWidth(), 48.0f, TEST_LOCATION); // inherited from View
  END_TEST;
}

int UtcDaliCheckBoxDisabledSuppressesToggle(void)
{
  UiTestApplication application(Components::UiConfig::New());
  CheckBox          cb = CheckBox::New();

  int  count = 0;
  bool last  = false;
  cb.SelectionChangedSignal().Connect(SelectionSpy(count, last));

  cb.SetProperty(Actor::Property::SENSITIVE, false); // disable input
  // A click cannot occur; programmatic SetSelected still works and is not what we test.
  // (Input-injection click/key tests: see §9 note.)
  DALI_TEST_EQUALS(count, 0, TEST_LOCATION);
  END_TEST;
}
```

> **Note (input-injection tests):** click / Return-key toggle assertions require
> generating touch/key events through the test application. Add them following the
> input-event injection pattern used by other DALi UI UTCs (generate a touch down+up
> over the actor, then assert `IsSelected()` flipped and the spy fired once). The
> deterministic `SetSelected`-driven tests above already cover the state machine,
> signal ordering, a11y bit, and box-only/label behaviour.

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
`ROOT=/home/jae/dali/dali-ui-claude`):
```sh
BD=/home/jae/dali-ui-cbuild && mkdir -p $BD && cd $BD
CXXFLAGS="-g -O0" cmake $ROOT/build/tizen \
  -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX -DCMAKE_BUILD_TYPE=Debug -DENABLE_PKG_CONFIGURE=ON
make dali2-ui-components -j4
make -C dali-ui-components install     # installs headers + .so to $DESKTOP_PREFIX
```
New `.cpp` are auto-globbed; only the umbrella-header edit (§6) is manual.

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
> test link blocked because the `dali2-ui-components` package wasn't installed
> (`make -C dali-ui-components install` addresses this). If `./build.sh dali-ui-components`
> fails at link, confirm the lib installed into `$DESKTOP_PREFIX` and that
> `pkg-config --exists dali2-ui-components` succeeds.

---

## 9. Verification checklist (Tier 1 — behaviour-compatible, no pixel compare)

- [ ] Library builds (`make dali2-ui-components`) and installs.
- [ ] `UtcDaliCheckBoxNewP` — handle valid, is-a `SelectableView`, default unchecked.
- [ ] `UtcDaliCheckBoxDownCastP` — DownCast round-trip.
- [ ] `UtcDaliCheckBoxRoleIsCheckBox` — `ACCESSIBILITY_ROLE == CHECK_BOX`.
- [ ] `UtcDaliCheckBoxSelectTogglesCheckedBitPreservesEnabled` — CHECKED bit follows
      `selected` via read-modify-write (other bits untouched).
- [ ] `UtcDaliCheckBoxSelectionChangedFiresOncePerChange` — one emit per genuine
      change, correct bool, `SetSelected(same)` emits zero.
- [ ] `UtcDaliCheckBoxTextBoxOnlyAndLabel` — box-only ↔ labelled via `SetText`.
- [ ] `UtcDaliCheckBoxStyleBuilderRoundTrip` — Builder set/get + `New(style)`.
- [ ] Add & pass input-injection click / Return-key toggle tests (see §7 note).
- [ ] (After assets) box swaps `unchecked→checked` URL and tick alpha toggles —
      assert `mBox.GetResourceUrl()` and the tick colour alpha via an exposed hook or
      by observing the rendered actor tree.
- [ ] RTL: box/label *positions* mirror (framework), tick artwork does NOT mirror.

---

## 10. Gotchas / notes (all verified)

1. **Base-first `OnInitialize`:** call `Provider::SelectableViewImpl::OnInitialize()`
   before adding children / setting the role — the SelectableTrait must be attached
   first (that is what makes `SelectionChangedSignal()` valid).
2. **No virtual `OnSelectedChanged` hook exists** — observe state via
   `SelectionChangedSignal().Connect(this, ...)`. `ViewImpl` is a
   `ConnectionTrackerInterface`, so `Connect(this, &Method)` is valid.
3. **CHECKED ≠ SELECTED.** `SetSelected` only raises `ViewState::SELECTED` (→ a11y
   SELECTED bit). You MUST separately mirror `selected → CHECKED` bit; the CHECK_BOX
   role gates the AT `CHECKED` emit (no manual `EmitStateChanged` needed — but the
   emit still requires an accessible object to exist; harmless if none).
4. **Two colour layers.** One `SetImageColor` multiply cannot tint both fill and tick.
   Keep `mBox` (fill) and `mTick` (ON_PRIMARY) as separate ImageViews; hide the tick
   with `mTick.SetImageColor(token.WithAlpha(0.0f))`.
5. **`GetImpl(Ui::ImageView&)` is not public** — upcast to `Ui::View` and use the
   public `GetImpl(Ui::View&)` (`view-impl.h`) to reach `Measure`/`Arrange`.
6. **RTL is automatic** — arrange children left-to-right; the framework mirrors
   positions. Never mirror manually. The tick glyph (direction-independent) must not
   flip — it doesn't, because it is static SVG artwork inside `mTick`.
7. **Do NOT** add a `Property::CHECKED` index or redeclare inherited selection methods;
   state lives in the trait (`mSelected`) and is exposed only via the inherited
   methods + signal.
8. **Numbers/assets are placeholders** — replace every `TODO(verify)` / `TODO(assets)`
   in §5.6 with confirmed One UI 8 values and real SVG URLs before shipping. Empty
   URLs compile and pass all logic tests but render nothing.

---

*Design of record:* the reviewed extracted-spec at
`dali-ui-components/docs/extracted-spec-checkbox.md` (this plan is its concrete,
build-ready realization; both agree on scope, base pair, two-layer SVG, a11y, and
Tier-1 verification).
```
