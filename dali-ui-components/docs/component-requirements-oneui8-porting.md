# Samsung One UI 8 / 8.5 → DALi UI Component Porting — Requirements Checklist (Official-Guideline Based)

## Intro

**Purpose.** This document lets a human faithfully port one Samsung One UI 8 / 8.5 component's UI/GUI onto **DALi UI** (under `/home/jae/dali/dali-ui-claude/dali-ui-components`). The completed document is handed to an LLM coding agent, which implements a correct, DALi-idiomatic public component whose behavior and visuals match Samsung's rendering.

**Authority.** Only **official Samsung One UI design guidelines** (the `developer.samsung.com/one-ui/…` design pages) are treated as authority for One UI behavior/values, together with the **verified DALi UI foundation API** for the implementation mapping. No community, `sesl`, `OneUIProject`, or third-party source is authority anywhere. Where a value comes from a foreign system (Material, One UI Watch), it is disclosed as an **unofficial fallback**, never as One UI 8 fact. Where an "official" hex or easing curve cannot be pinned to a single stable value, it is marked **UNVERIFIED — confirm at port time** and resolved through a DALi token rather than baked in.

**Operating principle (read this first).** Behavior, structure, states, interaction, accessibility, and conventions are **PRE-DECIDED** here from official One UI 8 guidelines + DALi. The author supplies only two things:

1. **Which component** to port + one official reference (screenshot / design-page link).
2. **Exact numeric metrics** (sizes, colors, radii, durations, easing) — supplied *later* via the separated **Numeric Parameter Table**.

This document **never asks the author to re-decide** a behavioral or structural matter, and it **never invents exact numbers**. Every item is tagged so the author can see, at a glance, what is already decided (`PRE-FILLED`/`FIXED`), what number they still owe (`DEFERRED-NUMERIC`), and the rare genuinely-free choices (`USER-CHOICE`).

**Tag legend.**

- **(FIXED)** — invariant DALi/One UI rule; identical for every component; never edited.
- **(PRE-FILLED: …)** — the One UI 8 behavior/structure/default is decided; the decided value is stated inline.
- **(DEFERRED-NUMERIC: param → table)** — behavior is decided; only the exact number is owed, via the named Numeric Parameter Table row.
- **(USER-CHOICE)** — a genuinely irreducible author decision (only 2 remain: which component, and which reference image).
- **DALi:** — the verified foundation class/API/file that satisfies the item.
- **Accept:** — objective pass condition (a UTC assertion, a `grep`, or a fidelity check). For numeric items, Accept asserts equality to the author-supplied table value.

---

## 2. What YOU Provide (the entire author input surface)

This is intentionally the smallest section. You own exactly this:

1. **(USER-CHOICE) One UI component name + archetype.** Name the One UI 8 / 8.5 control (Button, Checkbox, Radio, Switch, Slider, Progress, Chip, …) and its archetype. The archetype mechanically selects the DALi base pair (see §3-base / the Archetype→Base Pair table) — you do **not** pick the base pair, role, states, or behavior. If the component is one of the seven named archetypes, even the archetype is auto-derivable from the name.
2. **(USER-CHOICE) One official reference.** One official Samsung reference — a One UI 8 / 8.5 component design page or a Galaxy default-app screenshot of that control — used purely for the final side-by-side visual diff. **No numbers are extracted from the image**; the Numeric Parameter Table is the sole source of metrics.
3. **A pointer to the completed Numeric Parameter Table** (below), which you fill in later.

**Chip caveat (the only extra confirmation).** One UI publishes **no** official chip anatomy. If (and only if) you chose *Chip*, additionally confirm the chip sub-type (input / filter / suggestion / action). Everything else about the chip — pill shape, `Color control activated` selected accent, role mapping, remove-affordance semantics — is pre-decided by One UI intent in §Pre-decided and §5. You confirm the sub-type; you do **not** re-decide chip behavior.

That is the whole input surface. Everything else is below and already decided.

---

## 3. Numeric Parameter Table (fill in later)

This is your **deferred-input surface**: every exact metric, and *only* exact metrics. No row here carries a behavioral or structural decision — those all live in §Pre-decided. Fill the value cell for the rows that apply to your component; leave the rest blank. Units are pinned to the DALi setter's actual unit (DALi text spacing setters take **absolute px, not em**).

Unit key: **vpx** = DALi visual px; **hex** = `UiColor`/hex; **op** = opacity 0..1; **ms** = milliseconds; **cbz** = cubic-bezier control points; **ratio** = contrast ratio.

### 3.1 Shape / geometry

| Param | Unit | Applies to | DALi setter / style field | One UI meaning |
|---|---|---|---|---|
| cornerRadius | vpx | contained/flat button, card, list-row | `<Comp>Style::Builder::SetCornerRadius(float\|Vector4)` + `CornerRadiusPolicy` (st-04) | One UI rounded-corner identity; radius differs per class (multi-card vs singular-content vs button vs pill) — value for THIS class. |
| iconBackgroundCornerRadius | vpx | icon-background archetypes | `SetCornerRadius` on icon-background visual (st-04) | Rounded-square app-icon background. |
| chipPillCornerRadius | vpx | chip | `ChipStyle::Builder::SetCornerRadius` (typically full pill) | Chip pill container radius. |
| chipContainerHeight | vpx | chip | chip container height in `OnMeasure` | Chip's characteristic compact fixed height (distinct from button). |
| chipContainerMinWidth | vpx | chip | chip container min width | Chip minimum width. |
| controlMinHeight | vpx | button / list-row | `<Comp>Style::Builder::SetMinimumHeight` (st-04) | Button/list-row minimum height. |
| horizontalPadding | vpx | button, chip | `SetPadding` (Extents) (st-04) | Inner horizontal padding. |
| contentMaxWidth | vpx | content container | `SetMaximumWidth` (st-04) | Content max width for large screens. |
| controlToLabelGap | vpx | box/ring/thumb ↔ label | gap in `OnArrange` | Control-glyph-to-label gap. |
| iconToTextGap | vpx | button/chip icon ↔ label | gap in `OnArrange` | Leading/trailing icon-to-label gap. |
| leadingIconSize | vpx | chip/button leading icon/avatar | visual size in `OnMeasure`/`OnArrange` | Leading icon/avatar size. |
| trailingIconSize | vpx | chip trailing remove/dropdown | trailing visual size | Trailing "×"/caret size. |
| chipSelectedCheckIconSize | vpx | filter chip (selected) | selected-state leading check visual size | Selected filter-chip check glyph size. |

### 3.2 Checkbox / radio

| Param | Unit | Applies to | DALi setter / style field | One UI meaning |
|---|---|---|---|---|
| boxSize | vpx | checkbox box | box visual size in `OnMeasure`/`OnArrange` | Checkbox box glyph size. |
| boxCornerRadius | vpx | checkbox box | box visual `SetCornerRadius` | Checkbox box corner radius. |
| borderOutlineWidth | vpx | unchecked box / radio ring / icon outline | outline stroke width on the visual | Unchecked outline / ring stroke width. |
| tickStrokeWidth | vpx | checkmark | checkmark glyph stroke width | Checkbox tick stroke. |
| indeterminateDashStrokeWidth | vpx | tri-state checkbox | indeterminate dash glyph stroke | Partial/mixed dash stroke (distinct glyph, not a tick). |
| indeterminateDashInset | vpx | tri-state checkbox | dash glyph inset in box | Partial dash inset from box edges. |
| radioOuterRingDiameter | vpx | radio | outer ring visual diameter | Radio outer ring diameter. |
| radioInnerDotDiameter | vpx | radio (selected) | inner dot visual diameter | Radio selected inner-dot diameter. |

### 3.3 Switch

| Param | Unit | Applies to | DALi setter / style field | One UI meaning |
|---|---|---|---|---|
| switchTrackWidth | vpx | switch track | track visual width in `OnMeasure` | Track (background pill) width. |
| switchTrackHeight | vpx | switch track | track visual height | Track height. |
| switchTrackCornerRadius | vpx | switch track | track `SetCornerRadius` | Track corner radius. |
| switchThumbDiameterOff | vpx | switch thumb (off) | thumb visual diameter, off-state | Off-state thumb diameter. |
| switchThumbDiameterOn | vpx | switch thumb (on) | thumb visual diameter, on-state | On-state thumb diameter (One UI may morph on/off). |
| switchThumbPressedStretchWidth | vpx | switch thumb (pressed) | thumb elongation on press-and-hold | Tangible press-stretch before slide. |
| switchThumbInset | vpx | switch thumb | thumb inset in `OnArrange` | Thumb inset from track edge. |
| switchThumbTravel | vpx | switch thumb | thumb off→on travel animation | Thumb travel distance. |
| switchThumbShadowBlur | vpx | switch/slider thumb | thumb-elevation shadow blur | Subtle soft shadow lifting the knob off the track. |
| switchThumbShadowOffsetXY | vpx (x,y) | switch/slider thumb | thumb shadow offset | Thumb shadow offset. |
| switchThumbShadowColorOpacity | hex + op | switch/slider thumb | thumb shadow color/opacity | Thumb shadow color/opacity. |

### 3.4 Slider

| Param | Unit | Applies to | DALi setter / style field | One UI meaning |
|---|---|---|---|---|
| sliderRestingTrackThickness | vpx | slider track (resting) | track visual thickness | Resting bar thickness. |
| sliderTouchedTrackThickness | vpx | slider track (dragging) | track thickness while touched | Enlarged track while touched (Tangible grow). |
| sliderThumbDiameter | vpx | slider thumb | thumb visual diameter | Handle diameter. |
| sliderThumbHaloDiameter | vpx | slider thumb (dragging) | active-thumb halo/footprint size | Enlarged thumb halo while dragging. |
| sliderTickMarkDiameter | vpx | discrete slider | tick-mark visual diameter | Discrete-division tick-mark size. |
| sliderTickMarkSpacing | vpx | discrete slider | tick-mark spacing in `OnArrange` | Spacing between division ticks. |
| sliderTickMarkColorHex | hex | discrete slider | tick-mark `UiColor` (st-03) | Tick-mark color. |
| sliderValueBubbleSize | vpx | slider (dragging) | value-label bubble container size | On-drag value-readout bubble size. |
| sliderValueBubbleOffset | vpx | slider (dragging) | bubble offset above thumb | Bubble offset. |
| sliderValueBubbleCornerRadius | vpx | slider (dragging) | bubble `SetCornerRadius` | Bubble corner radius. |
| sliderMin | scalar | slider | component impl min (vs-04) | Range minimum (canonical default 0; supply only if spec overrides). |
| sliderMax | scalar | slider | component impl max (vs-04) | Range maximum (canonical default 100; supply only if spec overrides). |
| sliderStep | scalar | slider | component impl step + `OnAccessibilityValueChange` increment (vs-04/kf-02) | Step/divisions (canonical default 1; supply only if spec overrides). |

### 3.5 Progress

| Param | Unit | Applies to | DALi setter / style field | One UI meaning |
|---|---|---|---|---|
| progressBarTrackThickness | vpx | determinate bar | bar track visual thickness | Determinate bar thickness. |
| progressBarCornerRadius | vpx | determinate bar | bar `SetCornerRadius` | Rounded bar ends. |
| progressCircleDiameter | vpx | indeterminate circle | spinner diameter | Indeterminate circle diameter. |
| progressCircleStrokeWidth | vpx | indeterminate circle | spinner stroke width | Spinner stroke thickness. |
| progressLinearIndeterminateSweepLength | vpx / fraction | indeterminate linear | sweeping-segment length | Sweeping-segment length for linear indeterminate bar. |
| progressBufferFillHex | hex | media determinate bar | secondary/buffer fill `UiColor` (st-03) | Buffer/secondary-progress fill (media). |

### 3.6 Colors (per role; each ships a light **and** dark variant resolved through one `UiColor` token — dark hex supplied only where Samsung publishes a distinct dark value)

> **Note on One UI accent color.** One UI's accent (`Primary` / `Color control activated`) is **user-customizable** (Color palette) and version-dependent (8 vs 8.5). The hexes below are **UNVERIFIED reference approximations — confirm against the target One UI version's official color resource at port time.** They live here (not in prose) precisely so they resolve through a runtime `UiColor` token; the runtime theme supplies the real value.

| Param | Unit | Applies to | DALi setter / style field | One UI meaning |
|---|---|---|---|---|
| primaryHex_light / _dark | hex | FAB, slider, input field, focus | `UiColor` PRIMARY-role field (st-03) | Primary role (`#0381fe`, UNVERIFIED). Dark resolves via same token. |
| primaryDarkHex_light / _dark | hex | contained-button bg, app-bar/text/dialog button, subtext | `UiColor` Primary-dark field (st-03) | Primary-dark (`#0072de` light / `#3e91ff` dark, UNVERIFIED); genuinely differs per theme. |
| colorControlActivatedHex | hex (single value, both themes) | checkbox fill, radio dot, switch on-track, selected filter-chip | `UiColor` control-activated field (st-03) | **"Color control activated"** — a single `#3e91ff` (UNVERIFIED) used in **both** light and dark, **not** re-shaded. |
| uncheckedOutlineHex_light / _dark | hex | unchecked box / unselected ring / off-track | `UiColor` off-state field (st-03) | Off/unselected outline & track color. |
| thumbColorHex_on / _off | hex | switch/slider thumb | `UiColor` thumb field (st-03) | Thumb color (on/off may differ). |
| tickColorHex | hex | checkmark | `UiColor` (defaults ON_PRIMARY token) (st-03) | Checkmark color on checked fill. |
| sliderActiveFillHex | hex (L+D) | slider filled portion | `UiColor` active-fill (st-03) | Filled progress portion color. |
| sliderInactiveTrackHex | hex (L+D) | slider remaining track | `UiColor` inactive-track (st-03) | Remaining-track color. |
| progressFillHex | hex | progress fill | `UiColor` fill (st-03) | Progress fill color. |
| progressTrackHex | hex | progress track | `UiColor` track (st-03) | Progress inactive-track color. |
| buttonHighEmphasisBgHex | hex (L+D) | high-emphasis contained button bg | `UiColor` (st-03) | High emphasis = colored background. |
| buttonMediumLowEmphasisBgHex | hex (L+D) | medium/low-emphasis button bg | `UiColor` (st-03) | Medium/low emphasis = gray background. |
| labelTextHex | hex (L+D) | label text | `UiColor` (st-03) | Enabled label/text color. |
| disabledTrackHex / disabledThumbHex / disabledFillHex | hex (L+D) | disabled switch/slider/progress | `UiColor` disabled-role fields (st-03/st-05) | One UI uses **distinct disabled colors**, not just an alpha multiply. |

### 3.7 State-layer / feedback

| Param | Unit | Applies to | DALi setter / style field | One UI meaning |
|---|---|---|---|---|
| disabledContentOpacity | op | disabled content | `StateEffect` disabled content opacity (st-05) | Disabled dimming. *(Material 0.38 is an unofficial fallback; prefer the disabled color tokens above.)* |
| disabledTrackVsThumbOpacitySplit | op | disabled switch/slider | separate track vs thumb opacity | Split-dim where the control dims track/thumb by different amounts. |
| pressStateLayerOpacity | op | pressed overlay | `StateEffect` pressed state-layer opacity (st-05) | Press highlight opacity (mobile uses a state-layer overlay). |
| pressStateLayerColorHex | hex | pressed overlay | `StateEffect` pressed overlay color (st-05) | Press highlight color. |
| selectedStateLayerColorHex / selectedStateLayerOpacity | hex + op | pressed-while-SELECTED | `StateEffect` SELECTED_PRESSED row (st-05) | Distinct pressed-while-selected overlay (control-activated tint). |
| stateLayerCornerRadius | vpx | all state layers | overlay corner radius/inset per shape (Plain/Round/ListItem) (st-05) | State-layer radius matches control shape so highlight doesn't bleed past rounded corners. |

### 3.8 Focus mark

| Param | Unit | Applies to | DALi setter / style field | One UI meaning |
|---|---|---|---|---|
| focusIndicatorStrokeWidth | vpx | focus mark | custom focus visual keyed off `FOCUS_INDICATED` (kf-04) | SR/AT focus-mark stroke width. |
| focusMarkOutwardOffset | vpx (may be negative inset) | focus mark | focus visual offset from bounds (kf-04) | One UI focus mark is typically an **outline drawn just outside** the element bounds. |
| focusIndicatorCornerRadius | vpx | focus mark | focus visual `SetCornerRadius` (kf-04) | Focus-mark corner radius. |
| focusMarkContrastRatio | ratio ≥3:1 | focus mark | verified vs component AND background (kf-04) | Non-text-contrast floor for the focus mark itself. |

> Focus-mark **color** is not a separate number: its token identity is PRE-FILLED as the PRIMARY/focus role (see kf-04), so it resolves through `primaryHex_light/_dark` — do not supply a separate focus color.

### 3.9 Typography (per role: title / headline / body / subtext / caption)

| Param | Unit | Applies to | DALi setter / style field | One UI meaning |
|---|---|---|---|---|
| fontSizePerRole | sp / vpx | text child | point-size per type role | One UI type scale not published; per-role size. |
| lineHeightPerRole | line-spacing **px** | text child | line-spacing per role | Per-style line height (absolute px, matching the setter). |
| letterSpacingPerRole | character-spacing **px** | text child | character-spacing per role | Per-style letter spacing (**absolute px, not em** — DALi setter takes absolute). |
| fontWeightPerRole | weight (200/300/400/600) | text child | font weight per role | One UI Sans weights; bold titles / regular body. |
| dynamicFontLevelMapping | level→size map (Level 3–7, ≤200%) | text reflow | scale honored in `OnMeasure` reflow (i18n-02/ly-04) | Dynamic-font Level 3–7; text functional at 200%. |
| minContrastRatioText | ratio (≥4.5:1 text / ≥3:1 large) | fg/bg pairs | verified per pair (a11y/st-05) | **FIXED** thresholds by Samsung; verify exact per-pair ratios (large = >18dp normal / >14dp bold). |

### 3.10 Depth / motion / misc

| Param | Unit | Applies to | DALi setter / style field | One UI meaning |
|---|---|---|---|---|
| blurRadius | vpx | UNRELATED-context backdrop | overlay blur visual radius | Uniform blur+dim of previous screen (refocus). |
| dimOpacity_perTheme | op (L/D) | scrim/dim | dim overlay opacity | Single-level dim; never combined with shadow. |
| shadowBlurRadius | vpx | RELATED-context surface | soft shadow blur | Soft shadow only when new screen is RELATED. |
| shadowOffsetXY | vpx (x,y) | RELATED surface | shadow offset | Soft shadow offset. |
| shadowColorOpacity | hex + op | RELATED surface | shadow color/opacity | Soft, low-opacity shadow (must not read as 3D). |
| transitionDuration | ms (floor 100, ceiling 500) | state/screen transitions | `Animation` duration (fb-01) | Scales with object size & variance; clamp 100–500. |
| easingControlPoints | cbz | animations | `AlphaFunction` cubic-bezier (fb-01) | Basic ≈ `(0.22,0.25,0.00,1.00)` **UNVERIFIED — confirm against the target One UI motion guideline**; other 4 styles author-supplied; **Linear only for opacity**. |
| growShrinkDuration | ms | slider track grow/shrink | grow/shrink `Animation` (fb-01) | Tangible slider track grow-on-touch / shrink-on-release. |
| toggleSlideDuration | ms | switch thumb travel + track color cross-fade | thumb travel `Animation` + easing (fb-01) | Toggle slide timing (thumb travel AND track color cross-fade share this timing); haptic matched. |
| checkAnimDuration | ms | check/uncheck | check/uncheck `Animation` (fb-01) | Tick / radio-dot appear timing. |
| indeterminateSpinPeriod | ms | indeterminate circle | spinner rotation period | Spin period. |
| determinateFillDuration | ms | determinate bar | fill `Animation` (fb-01) | Determinate fill animation. |
| progressLinearIndeterminatePeriod | ms | indeterminate linear | sweep `Animation` period | Linear sweep period. |
| fadeOutInOverlapMs | ms | crossfade | crossfade `Animation` timings (fb-01) | Images may overlap; text must fully clear before new text. |
| overscrollIndicatorThickness | vpx | scroll container | overscroll edge visual thickness | Straight-edge end-of-scroll indicator. |
| overscrollStretchDistance | vpx | scroll container | overscroll stretch/offset | Overscroll stretch + settle. |
| hapticDuration | ms | supplementary haptic | app/adaptor-layer haptic (fb-03) | "As brief as possible"; matched to motion. |
| autoDismissTimeout | ms (>5s needs stop control) | toast/tooltip | transient-surface timer | Long enough to read/act; >5s needs a stop control. |
| maxFlashFrequency | Hz | animation design | design constraint (photosensitivity) | Avoid strobe/rapid brightness change. |
| sideMargin | vpx (≥24dp) | screen scaffold | scaffold side margin in layout | Curved-edge safe side margin, documented ≥24dp. |
| gridColumnsGutterPerBreakpoint | count / px / breakpoint px | responsive layout | column/gutter/margin per breakpoint | Responsive grid; per-breakpoint values not published. |
| focusBlockPaddingAndGap | vpx | focus block | focus-block padding + inter-item gap | Focus Block grouping padding and gap. |
| gradientStopsAngleOpacity | hex stops / deg / op | gradation surfaces (Type 3) | gradient visual | Gradation uses analogous colors; used cautiously. |

---

## 4. Pre-decided — One UI 8 + DALi (do not re-enter)

These are already decided from official One UI 8 guidelines + verified DALi. Do not re-decide any of them; supply only the numbers named in §3.

- **State vocabulary.** One UI canonical set — **Normal / Pressed(Click) / Focused / Disabled** for every control, plus **Selected/Checked** for selection controls and **Busy** for loading — mapped onto DALi `ViewState` (NORMAL/PRESSED/FOCUS_INDICATED/DISABLED/SELECTED); indeterminate/busy are custom `ViewState` + `ACCESSIBILITY_STATES` bits. *DALi:* `view-state.h:35-92`; `view-accessibility-enums.h:27-35`.
- **Checked/on accent = "Color control activated."** The checked/on accent for checkbox/radio/switch (and, by One UI analogy, selected filter-chips) **always** comes from the official **"Color control activated"** role (`#3e91ff`, single value both themes) — never an arbitrary blue. `Primary` is reserved for FAB/slider/input/focus; `Primary-dark` for contained-button/text-button/dialog-button; `ON_PRIMARY` for checkmark/thumb-on-fill. Wired as `UiColor` semantic tokens so only the hex is deferred. *Source:* `color/system.html`.
- **Emphasis by background color; one button style per screen.** Contained high-emphasis = colored bg, medium/low = gray bg, flat = no-emphasis. Structural rule, not a per-instance choice. *Source:* One UI button guidance.
- **Standalone selection-control anatomy.** Outside a list: **switch label leads, control trailing**; **checkbox/radio control leads, label trailing**; label + control together form one hit target. *Source:* One UI selection-control guidance.
- **Selection-control list integration.** The whole list-row is the touch target; a row carries exactly one selection-control type and never also an icon; **Switch = immediate toggle, Checkbox = multi/non-exclusive, Radio = single mutually-exclusive**; value applies immediately on change. The row's accessible object still exposes the control's **role + state** (row announced as Checkbox/Radio/Switch, not a plain list item). *DALi:* Selectable/GroupSelectable traits + full-bounds hit target.
- **Radio exclusivity + reselect = no-op.** Pre-decided via GroupSelectable declarative grouping (on-scene parent auto-group, or `SetGroupName` wins); selecting a new member deselects the prior and fires `SelectedMemberChangedSignal` **once**; reselecting the current member is a no-op. *DALi:* `group-selectable-view.h:133` (`SetGroupName`); `SelectedMemberChangedSignal` is on `SelectionGroup` (`selection-group.h:185`; `selection-group.cpp:169`), the group obtained via grouping.
- **Switch/SwitchBar master-toggle pattern.** A SwitchBar master toggle at the top of a section toggles all children; it reflects a mixed state when only some children are on, and announces the scope/count it governs. Modeled as a master-over-group relationship (master reflects children; toggling master sets all children), distinct from single-toggle and radio exclusivity. *Source:* One UI SwitchBar pattern.
- **Slider Tangible motion (mandatory).** The track **grows while touched and shrinks on release**; instant value feedback on every change; drag via the component's own `PanGestureDetector`, clamped to `[min,max]`, snapped to step, suppressed while disabled. The active thumb enlarges (halo) while dragging; the thumb may press-stretch before sliding. *DALi:* component-owned `Dali::PanGestureDetector` (interactive trait has no pan dispatch, `view-impl.h:140`).
- **Slider value bubble + discrete ticks.** A **value-label bubble** appears above the thumb during drag (present for value-readout sliders such as brightness/volume/scrub). **Discrete** sliders render **tick marks** at each division with a snap detent. Presence per control is pre-decided by whether the control is discrete / value-reporting; only the bubble/tick metrics are deferred.
- **Progress determinacy rule + forms.** Determinate **BAR** when completion/time is known (gradually fills, isolated area, never screen-covering; supports a secondary/buffer fill for media); indeterminate **CIRCLE** when the wait is unknown (spins until done); an indeterminate **LINEAR** sweeping-bar form also exists for full-width waits. A determinate bar fills **start→end in logical direction and therefore auto-mirrors under RTL**; a spinner's rotation direction does **not** mirror. Prefer on-body / on-the-tapped-button; auto-dismiss on completion.
- **Switch on/off color cross-fade.** The switch track **cross-fades** off-outline → `Color control activated` fill (and swaps thumb color) **in sync with the thumb slide** (`toggleSlideDuration`), not a pop.
- **Motion is functional, not decorative.** Basic easing (fast-start / gentle-settle) is the default; **Linear only for opacity/dissolve**; durations clamped to **100ms floor / 500ms ceiling** and scaled by object size & variance; **text fully clears before new text** (no text-on-text); images may crossfade-overlap. *Source:* `motion/basic.html`.
- **Depth is flat-and-calm (never literal 3D).** **Blur+dim** the previous screen when the new screen is **UNRELATED** (refocus); **soft shadow ONLY** when **RELATED** (continuity); dim and shadow are **never combined**; blur is uniform; dim exposes a single level. *Source:* `structure/visual-depth.html`.
- **Light/dark = token identity + `ThemeChangedSignal`.** No dark-mode enum in DALi; every color role ships light & dark variants through **one** `UiColor` token and re-resolves on theme swap. Dark mode is an explicit eye-comfort feature. The token system must additionally survive One UI **High-contrast / color-inversion** accessibility modes as further theme resolutions (not only light↔dark). *DALi:* `ui-theme-manager.h:88`; `ui-color-manager.h:222-281`.
- **RTL is auto-mirrored by the framework.** Components lay out LEFT_TO_RIGHT logically and never mirror manually. **Directional-affordance glyphs** (chevrons, next/prev, submit arrows, **media transport play/ff/rewind**, progress direction) **swap the glyph itself** under RTL; **direction-independent glyphs** (checkmark, logo, clock) do **not**. *Note:* media-play IS directional and mirrors — do not group it with checkmark/logo. *DALi:* `view-impl.cpp:1329-1333,1458-1478`.
- **Keyboard/AT activation pre-wired.** Interactive archetypes are already focusable via the trait; the `activate` AT action produces the same effect as a pointer click; range archetypes step by their `step` on AT value-increment/decrement and keep the accessibility value in sync. Execution keys = **Return and Space** for buttons/toggles, bound via the inherited execution-key config. *DALi:* `interactive-trait-impl.cpp:270-271`; `ui-config-impl.cpp:79-83`.
- **Screen-reader contract (fixed by One UI).** Announce in the order **status value → name → element type/role → usage hint** (name mandatory; status mandatory when a state exists). Announce One UI standard phrases (Checked/Not checked, Selected, On/Off, "%d percent", Collapsed/Expanded, "Tab %d of %d", Disabled) via `ACCESSIBILITY_ROLE` + read-modify-write `ACCESSIBILITY_STATES`. *Source:* `accessibility/screen-reader.html`.
- **AccessibilityState bit limits (verified).** `AccessibilityState` exposes **only** ENABLED / SELECTED / CHECKED / BUSY / EXPANDED (`view-accessibility-enums.h:27-35`). There is **no** indeterminate/mixed, read-only, or error/invalid bit. Those states must be announced via `ACCESSIBILITY_DESCRIPTION`/name text (e.g. "Partially checked" / "Mixed") — **not** a STATES bit. Modeling indeterminate by merely clearing CHECKED would make it indistinguishable from "Not checked" to AT, so a dedicated announced phrase is required.
- **Accessibility invariants.** Never convey state by color alone (a shape/icon/label cue survives grayscale); support text scaling to 200% without clipping; a visible focus-mark renders when SR/AT is active (typically an outline just outside the bounds); provide non-drag/non-multifinger alternatives; no premature auto-dismiss and a stop control for timed content >5s; avoid strobe/flash. Disabled controls stay **announceable (role + name + Disabled) and generally focusable** but non-actionable. *Source:* `accessibility/layout-and-typo.html`.
- **Contrast thresholds (FIXED by Samsung).** ≥4.5:1 text, ≥3:1 large text (large = >18dp normal / >14dp bold); the focus mark itself meets ≥3:1 vs both component and background. Only exact per-pair ratios are verified later. *Source:* `accessibility/color-contrast.html`.
- **Typography role model.** One UI Sans / SamsungOne family; hierarchy by **weight+size**, not color alone (bold titles, regular body); only the numeric scale/weights are deferred.
- **Haptic and sound are supplementary.** User-suppressible channels only, never the sole feedback; when present, timing/direction/duration are matched to the visible motion. One UI's official **"Remove animations"** accessibility setting must be honored when the platform exposes it (skip non-essential motion: track grow, thumb bounce, ripple).
- **Chip (no official spec) pre-decided by One UI intent.** A rounded **pill** (label + optional leading icon/avatar + optional trailing remove/dropdown) with input/filter/suggestion/action roles and **"Color control activated"** accent on the selected state. Role mapping is fully determined by sub-type: **filter/suggestion → TOGGLE_BUTTON** (On/Off phrase); **action / input-with-remove → BUTTON**. The trailing **remove "×" is a SEPARATE tap target** with its own **"Remove"/"Delete"** AT action distinct from selecting the chip, and removal is announced. An ellipsized chip label still announces its **full** text. All chip metrics are author-supplied.

---

## 5. Requirement Sections (decisions precede consumers)

### Component Spec Header

- [ ] **h-archetype** — Choose the One UI component and its archetype; the DALi base pair follows mechanically. **(USER-CHOICE)** — the sole structural choice retained by the author. *DALi:* Archetype→Base Pair table (`selectable-view.h:34-47`): container→`View`/`ViewImpl`; button→`InteractiveView`/`Provider::InteractiveViewImpl`; toggle→`SelectableView`/`Provider::SelectableViewImpl`; radio→`GroupSelectableView`/`Provider::GroupSelectableViewImpl`; range→`View`(or `InteractiveView`)+component value/min/max/step. **Accept:** one sentence names the One UI component and one of the five archetypes → exactly one base-pair row matching the control's identity.
- [ ] **h-role** — Accessibility role. **(PRE-FILLED:** checkbox→CHECK_BOX, radio→RADIO_BUTTON, switch→TOGGLE_BUTTON, slider→ADJUSTABLE, progress→PROGRESS_BAR, button→BUTTON, container→CONTAINER; **chip filter/suggestion→TOGGLE_BUTTON, chip action/input→BUTTON).** *DALi:* `AccessibilityRole` enum (`view-accessibility-enums.h`, ROLE_START_INDEX=200). **Accept:** role matches the archetype mapping; extra ATSPI interfaces recorded (slider→Value).
- [ ] **h-reference** — Provide one official One UI 8/8.5 reference (screenshot or design page) for visual diffing. **(USER-CHOICE** — image; source is **PRE-FILLED** to Samsung's official design guideline / Galaxy default-app rendering for that control**).** *DALi:* n/a (external reference). **Accept:** a single official Samsung reference is recorded; numbers still come from §3, not the image.

### 1. Identity & Scope

- [ ] **id-01** — State the One UI component name, one-line purpose, and archetype. **(USER-CHOICE).** *DALi:* archetype selects the base pair (`selectable-view.h:34-47`). **Accept:** one sentence names the component and one of five archetypes → exactly one base-pair row.
- [ ] **id-02** — Semantic accessibility role and extra ATSPI interfaces. **(PRE-FILLED:** role per h-role; extra interfaces: **slider→Value, all others→none** — state via `ACCESSIBILITY_STATES`**).** *DALi:* `AccessibilityRole` enum. **Accept:** role matches archetype; only ADJUSTABLE/slider lists Value.
- [ ] **id-03** — Style class + runnable sample. **(PRE-FILLED: Style=yes AND sample=yes** for every visible One UI control; DALi auto-discovery makes both free. Non-visual container may record `no` with rationale.**)** *DALi:* Style pair `public-api/styles/<name>-style.{h,cpp}` + `internal/styles/<name>-style-impl.h`; sample auto-discovered by `samples/CMakeLists.txt:51-59`. **Accept:** Style=yes and sample=yes recorded for every visible control; §8/§16/§17 read "per id-03".

### 2. API Surface & Naming

- [ ] **api-01** — Name boolean-option APIs `Set<Noun>Enabled`/`Is<Noun>Enabled`; never `EnableX`/`SetEnableX`/`GetXEnabled`. **(FIXED).** *DALi:* `rules/api-naming.md:24-34,48-57`; `SetEnabled(bool)` on View is the sole exception. **Accept:** `grep` of the new public header finds no `EnableX`/`SetEnableX`/`GetXEnabled`.
- [ ] **api-02** — Provide static `New()` and, since id-03 Style=yes, `New(Style)`, via impl `New()` with two-phase `Initialize()`. **(FIXED).** *DALi:* `text-button-impl.cpp:64-72`; `text-button.cpp:33-55`. **Accept:** UTC — `New()` returns a valid handle; `New(style)` asserts style initialized; default ctor yields empty handle.
- [ ] **api-03** — Paired `GetImpl` helpers; every handle method forwards one line. **(FIXED).** *DALi:* `text-button-impl.h:79-89`; `text-button.cpp:80-88`. **Accept:** no public handle method has logic beyond a single forward; handle declares no data members.
- [ ] **api-04** — Archetype value/state property surface + typed change signal; **(PRE-FILLED:** One UI "value applies immediately" + update-then-notify; use the inherited archetype signal; signal fires once after stored state updates**).** *DALi:* Button—`ClickedSignal`/`PressedChangedSignal`/`LongPressedSignal`+`IsPressed`/`SetClickable` (`interactive-view.h:120-259`); Toggle—`SelectionChangedSignal`+`IsSelected`/`SetSelected` (`selectable-view.h:120-153`); Radio—`SetGroupName`/`GetGroupName`/`GetGroup` (`group-selectable-view.h:134,143,153`), group-level via `GetGroup()`/`Find`; Range—component value/min/max/step + `ValueChanged`. **Accept:** each value/state has getter/setter/change-signal carrying new value + `InputEvent`; fires once after the stored state updates; a handler reading the getter inside the signal sees the new value; UTC round-trips each default.
- [ ] **api-05** — Invoke `DALI_UI_VIEW_WITH(<Name>)` in the handle body. **(FIXED).** *DALi:* `text-button.h:58`; `view-with.h:30`; `DALI_UI_CHAIN_VIEW_METHODS` does NOT exist. **Accept:** handle body contains `DALI_UI_VIEW_WITH(<Name>)` and compiles; `grep` confirms no `DALI_UI_CHAIN_VIEW_METHODS`.
- [ ] **api-06** — Property-system surface. **(PRE-FILLED:** default **no** named/indexed property surface — state via typed getters/setters+signals — matching the reference controls; add a `-properties.h` only if the One UI control genuinely needs serialize/animate-by-index; rationale recorded**).** *DALi:* `chart-view-properties.h:37-58`; `property-registration-helper.h:78-84`. **Accept:** if yes, registration order matches index order (`static_assert`) and UTC round-trips by index AND name and rejects read-only writes; if no, absence of `-properties.h` is intentional and recorded.

### 3. Base Class & Inheritance

- [ ] **base-01** — Derive handle from the archetype handle base, impl from the matching provider impl base; don't stack guaranteed traits. **(PRE-FILLED:** base pair follows mechanically from the id-01 archetype**).** *DALi:* e.g. `class Foo : public InteractiveView` + `class FooImpl : public Provider::InteractiveViewImpl` (`text-button.h:41`; `text-button-impl.h:32`; `selectable-view.h:34-47`). **Accept:** UTC — archetype-base `DownCast` succeeds; handle base, impl provider base, and base-03 registration base are the same archetype row.
- [ ] **base-02** — `DownCast(BaseHandle)` via the View template helper + the two internal ctors. **(FIXED).** *DALi:* `text-button.cpp:57-60,150-159`; `view.h:2198-2233`. **Accept:** UTC — `Foo::DownCast` and `Dali::DownCast<Foo>` succeed on a real handle; `DownCastN` returns empty for an unrelated `BaseHandle`.
- [ ] **base-03** — Register the impl with `DALI_TYPE_REGISTRATION_BEGIN`; registration base = base-01 provider base. **(FIXED).** *DALi:* `text-button-impl.cpp:36-45`. **Accept:** registered base matches the archetype provider base; type registers without assertion; verifiable via `TypeRegistry`.
- [ ] **base-04** — Override `OnInitialize()`, chain to the immediate base FIRST, then build sub-views via `Self().Add(...)`; never touch `Self()` in the ctor. **(FIXED).** *DALi:* `text-button-impl.cpp:147-155`; `view-impl.cpp:2491`. **Accept:** base `OnInitialize` on line 1; no `Self()` in any ctor body.

### 4. Properties / Value / State Model

- [ ] **vs-01** — States, defaults, transition table. **(PRE-FILLED:** One UI set mapped to `ViewState` — NORMAL, PRESSED(Click), FOCUS_INDICATED, DISABLED, +SELECTED for selection controls, BUSY(custom) for loading, indeterminate(custom) where applicable; default on `New()` = NORMAL/unselected. The **full transition table per archetype is mechanical** (One UI immediate-toggle); the author supplies nothing here — only the state *set* differs by archetype, which id-01 already fixes**).** *DALi:* `view-state.h:35-92`; custom via `ViewState::Create(name)`; `GetState()`/`StateChangedSignal`. **Accept:** UTC asserts each getter's default after `New()`; every transition row is UTC-driven asserting resulting state + single emitted signal.
- [ ] **vs-02** — Toggle semantics + toggle-by-click + indeterminate. **(PRE-FILLED:** toggle-by-click on (One UI applies value immediately on tap); the tri-state (select-all) cycle is `unchecked → checked → indeterminate → unchecked`, indeterminate entered **only programmatically** from child state, a user tap from indeterminate goes to **CHECKED**; indeterminate is a custom `ViewState`, drawn as a **dash glyph** (not a tick), excluded from the emitted boolean, and announced **"Partially checked"/"Mixed" via description text — there is NO STATES bit for it).** *DALi:* `SelectableTrait` `IsSelected`/`SetSelected`, `IsToggleByClickEnabled`/`SetToggleByClickEnabled` (default true) (`selectable-trait.h:111-155`). **DEFERRED-NUMERIC:** indeterminateDashStrokeWidth, indeterminateDashInset → §3.2. **Accept:** UTC — click toggles selected; tap cycle matches the documented order; indeterminate never participates in the emitted boolean and is announced via description text.
- [ ] **vs-03** — Radio single-selection + reselect-no-op. **(PRE-FILLED:** One UI radio = single mutually-exclusive; selecting a new member deselects the prior; reselecting the current is a no-op**).** *DALi:* declarative membership — on-scene parent auto-group or `SetGroupName(name)` wins; group via `GetGroup()`/`SelectionGroup::Find`; group-level `GetSelectedMember`/`ClearSelection`/`SelectedMemberChangedSignal` (`group-selectable-view.h:134,143,153`). **Accept:** UTC — selecting a second member deselects the first and fires `SelectedMemberChangedSignal` once; clicking the already-selected member emits no change.
- [ ] **vs-04** — Range value/min/max/step/orientation/clamping. **(PRE-FILLED:** **horizontal** orientation; **clamp to [min,max] + snap to step always on**; `ValueChanged` fires only on genuine change; `ACCESSIBILITY_VALUE` kept in sync; canonical defaults **min=0/max=100/step=1/value=0, continuous**).** **(DEFERRED-NUMERIC:** sliderMin/sliderMax/sliderStep → §3.4 — supply **only** where the specific One UI control overrides the canonical values.**)** *DALi:* store value/min/max/step on the impl; `ACCESSIBILITY_VALUE` is a STRING for announcement only (`view.h:1782-1784`); AT stepping via `OnAccessibilityValueChange` (`view-impl.h:147`; `view-impl.cpp:2803-2806`). **Accept:** UTC — out-of-range set clamps, discrete step snaps, getter reflects clamped/snapped value, `ValueChanged` fires only on genuine change, and UTC asserts the exact author-supplied numbers.
- [ ] **vs-05** — Loading/busy state. **(PRE-FILLED:** for controls that show progress on themselves (One UI "progress on the tapped button") set the BUSY bit, suppress/queue activation while busy, cue is non-color; N/A only if the control never loads**).** *DALi:* `AccessibilityState::BUSY` (`view-accessibility-enums.h:32`); custom `ViewState`, read-modify-write preserving ENABLED. **Accept:** UTC — BUSY toggles while ENABLED preserved and activation is suppressed/queued while busy; or N/A recorded.
- [ ] **vs-06** — Error/validation and read-only, distinct from disabled. **(PRE-FILLED:** where the control has them (e.g. text field): read-only stays focusable/announced but non-editable; error uses a non-color cue + AT message; **both announced via description text — there is NO STATES bit** for read-only/error; N/A otherwise**).** *DALi:* custom `ViewState` + `ACCESSIBILITY_*`; contrast with disabled at `interactive-trait-impl.cpp:106-126`. **Accept:** UTC — read-only blocks value mutation while focusable/announced; disabled fully blocked; distinction documented; or N/A recorded.
- [ ] **vs-07** — Null/empty/absent inputs handled gracefully. **(FIXED).** *DALi:* Style `Default()`→`DefaultPreset()`; `StateEffect` null→`None()` (`text-button-style.cpp:57-67,549-557`). **Accept:** UTC — `New()` with no args, empty text, and no style override yields a valid non-crashing handle with documented defaults.

### 5. Interaction & Input

- [ ] **int-01** — Consume inherited `Clicked`/`PressedChanged`/`LongPressed`, not raw `TouchedSignal`. **(FIXED).** *DALi:* `interactive-trait-impl.cpp:263-277,351-362`. **Accept:** UTC — touch down+up emits `PressedChanged(true/false)` then `Clicked` once; no raw `Actor::TouchedSignal` handler.
- [ ] **int-02** — Key activation. **(PRE-FILLED:** `KeyClickPolicy` = **ON_RELEASE** (One UI activate-on-release); execution keys = **Return AND Space** for buttons/toggles, bound via the inherited execution-key config; inherited `Set/GetKeyClickPolicy` NOT redeclared**).** *DALi:* `key-click-policy.h:28-49`; `ui-config-impl.cpp:79-83`; `interactive-view.h:193,201`. **Accept:** `grep` finds no redeclared `Set/GetKeyClickPolicy`; UTC — Return and Space key-up fire `Clicked` under ON_RELEASE via the inherited API.
- [ ] **int-03** — Disabled (hard block) vs pseudo-disabled (visual-only). **(FIXED).** *DALi:* `interactive-trait-impl.cpp:106-126,167-178,292-295`. **Accept:** UTC — touch on disabled emits no `Clicked`; on pseudo-disabled still emits `Clicked`; disabling while pressed releases pressed.
- [ ] **int-04** — Programmatic changes pass `InputEvent::Programmatic()`; update before emit. **(FIXED).** *DALi:* `selectable-trait-impl.cpp:72-82`; `interactive-trait-impl.cpp:122,162,176`. **Accept:** UTC — programmatic `SetSelected` emits `SelectionChanged` with `IsProgrammatic()` true; a handler reading the value inside the signal sees the new value.
- [ ] **int-05** — Drag/pan for slider/range. **(PRE-FILLED:** One UI Tangible — value tracks the pan clamped to [min,max] snapped to step; `ValueChanged` during drag + final on release; **track grows on touch, shrinks on release; active thumb enlarges (halo); thumb may press-stretch before slide; a value bubble shows above the thumb during drag** for value-reporting sliders; discrete sliders draw tick marks with a snap detent; suppressed while disabled; N/A for non-draggable archetypes**).** *DALi:* component-owned `Dali::PanGestureDetector` (interactive trait has no pan dispatch; only `OnAccessibilityPan`, `view-impl.h:140`); grow/shrink `Animation`. **DEFERRED-NUMERIC:** sliderTouchedTrackThickness, sliderThumbHaloDiameter, sliderValueBubble*, sliderTickMark* → §3.4. **Accept:** UTC — synthesized pan makes value track the drag and clamp at ends, track grows/shrinks, drag suppressed when disabled; or N/A recorded.
- [ ] **int-06** — Hover + touch-and-hold. **(PRE-FILLED:** hover feedback is **pointer-only** via the component's own `HoverEventSignal`, never on touch-only activation; hover-revealed content is dismissable + keyboard-reachable; **icon-only controls and chips/buttons expose a touch-and-hold (long-press) label / secondary affordance**, wired via the inherited `LongPressed`; N/A if none**).** *DALi:* no HOVERED `ViewState` (`view-state.h`); `Actor::HoverEventSignal`. **Accept:** UTC/manual — hover enter/leave toggles hover visual, touch activation leaves no lingering hover, tooltip dismissable + keyboard-reachable, long-press reveals the label; or N/A recorded.
- [ ] **int-07** — No wheel handling in the interactive path; wire `Actor::WheelEventSignal` only if genuinely needed. **(FIXED).** *DALi:* `interactive-trait-impl.h:167-195`. **Accept:** if required, wired explicitly; otherwise no wheel code.

### 6. Keyboard & Focus

- [ ] **kf-01** — Focusability. **(PRE-FILLED:** interactive archetypes are already focusable via the trait — **no** component-level `SetFocusable`; non-interactive View archetypes call `SetFocusable(true)` in `OnInitialize`; a visible focus mark renders when SR/AT is active**).** *DALi:* `interactive-trait-impl.cpp:270-271`. **Accept:** UTC — after `New()`, `IsFocusable()` is true (no component `SetFocusable` for interactive archetypes); `FocusManager::SetCurrentFocusView` focuses it.
- [ ] **kf-02** — `activate` AT action = pointer activation; range steps by step + syncs value. **(PRE-FILLED:** `activate` triggers the same primary action as a click; slider/range overrides `OnAccessibilityValueChange` to step by `sliderStep`, clamp to [min,max], update `ACCESSIBILITY_VALUE` ("%d percent")**).** *DALi:* `view-data-impl.cpp:172-188`; `view-impl.cpp:2803-2806`. **Accept:** UTC — `activate` triggers the primary action identically to a click; AT value-inc/dec changes value by exactly one step, clamps, updates `ACCESSIBILITY_VALUE`.
- [ ] **kf-03** — Focus order + arrow navigation. **(PRE-FILLED:** One UI logical **L-to-R line-wrapping, non-looping** order; radio group = one tab stop with arrows move+select; slider = step keys; decorative parts non-focusable; related elements may group into one stop; **disabled controls stay announceable and generally remain focusable but non-actionable**).** *DALi:* `SetLeft/Right/Up/Down/Forward/Backward/Clockwise/CounterClockwiseFocusableView`; custom nav via `OnFocusNavigationRequested`/`SetFocusNavigationCallback` (`view.h:763-826`; `view-impl.h:995-1006`). **Accept:** UTC — `MoveFocus` lands on the intended neighbor; radio groups expose one tab stop and arrows move+select; no looping at edges.
- [ ] **kf-04** — Focus indicator + AT highlight (two independent systems). **(PRE-FILLED:** a visible focus mark renders whenever SR/AT is active, **keyed off `FOCUS_INDICATED`**, suppressing the default indicator to avoid double-render; the mark's **color = PRIMARY/focus token** (resolves via `primaryHex_light/_dark`, no separate color number)**).** **(DEFERRED-NUMERIC:** focusIndicatorStrokeWidth, focusMarkOutwardOffset, focusIndicatorCornerRadius, focusMarkContrastRatio → §3.8.**)** *DALi:* keyboard ring — `FocusManager::SetDefaultFocusIndicatorEnabled` + StateEffect suppression (`focus-manager.h:178-195`; `view-impl.h:539-563`); AT highlight — `ViewAccessible` `GrabHighlight`/`ClearHighlight` (`view-accessible.cpp:528-615`); custom visual keys off `ViewState::FOCUS_INDICATED` (`view-state.h:69-70`). **Accept:** a visible focus mark appears when SR/AT active, keys off `FOCUS_INDICATED`, suppresses the default indicator, no double-render; UTC asserts the applied focus-mark metrics equal the author-supplied values and ≥3:1 contrast.

### 7. Accessibility

- [ ] **a11y-01** — Set `ACCESSIBILITY_ROLE` in `OnInitialize`. **(PRE-FILLED:** role from h-role, never left NONE for a semantic control**).** *DALi:* `text-anchor-impl.cpp:162-167`; `view-accessible.cpp:301-305`. **Accept:** UTC — `ACCESSIBILITY_ROLE` round-trips the archetype role; component highlightable under AUTO (role != NONE).
- [ ] **a11y-02** — Accessible NAME (+ DESCRIPTION where useful). **(PRE-FILLED:** text-bearing controls override `GetNameRaw()` so the label auto-announces; icon-only controls expose alt text — no status/role inside alt text per One UI; announcement order **status → name → type → hint** with name mandatory**).** *DALi:* resolution GetName signal → `ACCESSIBILITY_NAME` → `GetNameRaw()` → Actor NAME (`view-accessible.cpp:232-289`). **Accept:** UTC/manual — AT announces a non-empty name; a text component announces its label without the app setting `ACCESSIBILITY_NAME`.
- [ ] **a11y-03** — Dynamic states via read-modify-write on `ACCESSIBILITY_STATES`. **(PRE-FILLED:** map to One UI phrases — Checkbox→Checked/Not checked, Radio→Selected, Switch→On/Off (SwitchBar master additionally announces its scope/count), Slider→"%d percent", Expandable→Collapsed/Expanded, disabled→Disabled — all read-modify-write preserving ENABLED; **indeterminate/mixed, read-only, error have NO bit and are announced via description text**; selection controls toggle the visual **immediately** AND emit the AT state-change even off-highlight (see a11y-04)**).** *DALi:* enum bits `view-accessibility-enums.h:27-35` (BUSY:32, EXPANDED:33); `group-selectable-trait-impl.cpp:557-568`. **Accept:** UTC — toggling selection/checked flips the bit while ENABLED remains set; disabling clears ENABLED; announced phrase matches the mapping.
- [ ] **a11y-04** — Emit ATSPI state-change events where the framework does not auto-emit. **(FIXED)** — because One UI applies value immediately even off screen-reader-highlight, programmatic/off-highlight changes emit manually. *DALi:* auto-emit only while highlighted (`view-accessible.cpp:742-769`; `view-accessibility-data.cpp:265-269`). **Accept:** UTC — a selection change while highlighted (auto-emit role) fires a state-change event; a component whose selection can change off-highlight emits its own event.
- [ ] **a11y-05** — Override `CreateAccessibleObject()` only for an interface not expressible via `ACCESSIBILITY_*`. **(PRE-FILLED:** present **only for slider** (Value interface) per id-02; all others rely on `ACCESSIBILITY_*` and omit the override**).** **Foundation caveat:** the foundation surfaces the accessibility value only as a **STRING** (`view-accessible.h:115`, `view.h:1784`); if faithful slider a11y requires the **numeric** ATSPI Value contract (min/max/current/increment), the slider port must extend the accessible (override `GetValue` and any numeric Value accessors) and this is flagged as a **known gap**, not "in sync." *DALi:* `view-impl.cpp:2813-2816`; `text-anchor-impl.cpp:170-179`. **Accept:** if id-02 lists an extra interface, its overrides return correct name/state/features; if none, no override exists and property-driven a11y is UTC-verified.

### 8. Visual States & Styling / Theming

- [ ] **st-01** — Public `<Comp>Style : public UiStyle` with defaulted ctor, move-only Builder, private impl ctor. **(FIXED).** *DALi:* mirror `TextButtonStyle` (`text-button-style.h:47-115`). **Accept:** UTC Builder round-trip for every field; `Build()` asserts on double-consume.
- [ ] **st-02** — `DownCast/StaticDownCast/DefaultKey/DefaultPreset/Default/Configure`; preset/Default call `DebugAssertStyleConfigApplied` first. **(FIXED).** *DALi:* `text-button-style.cpp:43-77`; `ui-style-debug.h:31-34`. **Accept:** UTC — with no sheet override `Default()==DefaultPreset()`; `StyleSheet::New().GetStyle(DefaultKey())` empty; registered creator makes `Default()` return the override; `SetStyle` after Apply asserts frozen.
- [ ] **st-03** — Every color-valued field is a `UiColor` defaulting to the correct One UI **token**; `StateEffect` defaults to `DefaultForInteractive()` coercing null to `None()`. **(PRE-FILLED:** the **token→role mapping is fixed** — control-activated→checked/on accent, PRIMARY→FAB/slider/input/focus, Primary-dark→contained/text/dialog button, ON_PRIMARY→checkmark/thumb-on-fill; the author does NOT re-pick which role a color plays**).** **(DEFERRED-NUMERIC:** the per-theme hex for each role → §3.6 (dark resolves through the same token; distinct dark hex only where Samsung publishes one)**).** *DALi:* `text-button-style.cpp:36-39,549-557,577-580`; `ui-color.h:56-63`. **Accept:** UTC — color fields keep token identity via `HasColorId()`/`GetColorId()`; a theme swap re-resolves; `SetStateEffect(None())` round-trips; each field's resolved rgba equals the author-supplied hex for its role/theme.
- [ ] **st-04** — Geometric/layout style fields applied in `ApplyInitialStyle`. **(PRE-FILLED:** One UI rounded-corner presence + "radius differs per class" + min-size identity**).** **(DEFERRED-NUMERIC:** cornerRadius, controlMinHeight, horizontalPadding, contentMaxWidth (and chip container height/min-width) → §3.1**).** *DALi:* `SetMinimum/MaximumWidth/Height`, `SetCornerRadius(+Policy)`, `SetPadding`, `SetBackgroundColor(UiColor)`, `SetStateEffect` (`text-button-impl.cpp:157-177`); max default `UNCONSTRAINED_MAX_SIZE=FLT_MAX`. **Accept:** UTC — after `New(style)` the View reports the author-supplied min/max/corner/padding; `Configure()` changes only overridden fields; corner radius non-zero.
- [ ] **st-05** — Interaction-state visual matrix via `StateEffect`/state-layer. **(PRE-FILLED:** rows = the One UI states from vs-01 (Normal/Pressed/Focus/Disabled/Selected, plus **SELECTED_PRESSED**, `view-state.h:78`); **mechanism = state-layer overlay (no press-scale on mobile)**; **every state distinguishable in grayscale (no color-only)**; **disabled dims AND suppresses input**; disabled uses **distinct disabled color tokens**, not only an alpha multiply; light/dark/high-contrast = token identity + `ThemeChangedSignal`**).** **(DEFERRED-NUMERIC:** pressStateLayerColorHex/Opacity, selectedStateLayerColorHex/Opacity, disabledContentOpacity, disabledTrack/Thumb/FillHex, disabledTrackVsThumbOpacitySplit, stateLayerCornerRadius → §3.6/§3.7**).** *DALi:* `SetStateEffect` (OverlayEffect presets Plain/Round/ListItem, `state-effect.h:37-88`; `overlay-effect.h:69-361`); re-resolution via `UiColorManager::SetColorOverride/ClearColorOverride/InvalidateCache` (`ui-color-manager.h:222-281`). **Accept:** UTC asserts `SetStateEffect` is called; the matrix has one row per supported state; manual — every state visually distinct, disabled dimmed+input-suppressed, each state distinguishable in grayscale; opacities/colors equal author-supplied values.

### 9. Layout & Sizing

- [ ] **ly-01** — `OnMeasure` in visual units honoring requested/min/max/padding/scale + reserving space for 200% text. **(PRE-FILLED:** incoming constraints already clamped — do NOT re-apply outer min/max; reflow for dynamic font rather than truncate**).** *DALi:* `text-button-impl.cpp:179-228`; `view-impl.cpp:1115-1141`. **Accept:** UTC — `Measure()`/`GetMeasuredSize()` honors padding, requested sizes, MATCH_PARENT/WRAP_CONTENT, min/max; enlarged text reflows without clipping.
- [ ] **ly-02** — `OnArrange` places children within padding-inset bounds LEFT_TO_RIGHT; never mirror for RTL. **(FIXED).** *DALi:* `text-button-impl.cpp:230-251`; `view.h:240-259`. **Accept:** UTC — children within padding-inset bounds; `SetLayoutDirection(RIGHT_TO_LEFT)` mirrors direct children automatically without component code.
- [ ] **ly-03** — Adequate interactive target via style min-size (no hit-area API). **(PRE-FILLED:** One UI "large enough + spaced"; **flat/text buttons must reserve the target via min-size even though their painted bounds are smaller** so they aren't under-target**).** **(DEFERRED-NUMERIC:** minTouchTargetSize, minTargetSpacing → §3.1 (Samsung publishes no dp)**).** *DALi:* `SetMinimumWidth/Height` from `style.GetMinimumWidth()` (`text-button-impl.cpp:161-163`); touch target = arranged bounds. **Accept:** UTC asserts the applied minimum equals the author-supplied `minTouchTargetSize`; adjacent targets have the author-supplied spacing.
- [ ] **ly-04** — Text overflow policy for bounded-width text. **(PRE-FILLED per archetype:** button/chip label → single-line **ELLIPSIS** at max width (One UI truncates pill/button labels, never wraps); list-row label → **WRAP/grow**; the **full text always stays in the accessible name** — including an ellipsized chip label; N/A for non-text controls**).** *DALi:* `ELLIPSIS` (`text-enumerations.h:189-192`); max-width via st-04 `SetMaximumWidth`. **Accept:** UTC/manual — an over-long string at max width shows the documented behavior without container clipping and full text in the accessible name; or N/A recorded.
- [ ] **ly-05** — Call `InvalidateMeasure`/`InvalidateArrange` after mutating layout-affecting state. **(FIXED).** *DALi:* `view.h:231-259`; `view-impl.cpp:1128-1140`. **Accept:** UTC — changing a size-affecting property re-measures on the next pass; callbacks (if used) return visual sizes and arrange children LTR.

### 10. Feedback (motion / haptic / sound)

- [ ] **fb-01** — Immediate position-aware press/state feedback via `StateEffect`. **(PRE-FILLED:** motion identity — functional, immediate, **Basic easing default**, **Linear only for opacity**, **text clears before text**, Tangible slider grow/shrink, toggle slide with synced track color cross-fade**).** **(DEFERRED-NUMERIC:** transitionDuration (100–500ms), easingControlPoints (Basic ≈`0.22,0.25,0.00,1.00` **UNVERIFIED — confirm**; others deferred), growShrinkDuration, toggleSlideDuration, checkAnimDuration → §3.10**).** *DALi:* `state-effect-impl.h:54-96`; `view.h:2163`; `Animation` + `AlphaFunction` cubic-bezier. **Accept:** manual — pressing shows an immediate response; UTC/manual asserts duration is within [100,500]ms and equals the author-supplied value, easing uses the specified control points, and slider/toggle motion matches One UI Tangible behavior.
- [ ] **fb-02** — Reduced motion. **(PRE-FILLED:** honor One UI's official **"Remove animations"** setting when the platform exposes it (skip non-essential motion: track grow, thumb bounce, ripple); no verified foundation API, so gate custom animation on the app-provided flag; essential motion kept; N/A if truly none**).** *DALi:* no built-in reduced-motion signal. **Accept:** if the flag is added, UTC asserts no animation is created when reduced-motion is on; no animation runs on a disabled/destroyed component; or N/A recorded.
- [ ] **fb-03** — Haptic/sound supplementary only. **(PRE-FILLED:** never sole feedback; state conveyed by shape/text/icon in addition to any color; if used, timing/direction matched to visible motion**).** **(DEFERRED-NUMERIC:** hapticDuration → §3.10, supplementary only**).** *DALi:* no verified foundation haptic/sound API; source from app/adaptor. **Accept:** the component remains fully operable and state-distinguishable with haptics/sound disabled; any haptic timing matches the visible motion.

### 11. Internationalization & RTL

- [ ] **i18n-01** — User-visible text is a plain `Dali::String` passthrough; no in-component localization; no `po/`. **(FIXED).** *DALi:* `text-button-impl.cpp:74-82`; app-layer `UiLocalizationManager` (`ui-localization-manager.h:210-304`). **Accept:** `grep` finds no `gettext`/`dgettext` and no `po/` dir; text setters are one-line passthroughs.
- [ ] **i18n-02** — Tolerate ~2× translated-string growth + dynamic font to 200%. **(PRE-FILLED:** label uses WRAP_CONTENT and reserves space; no fragment concatenation**).** *DALi:* `text-button-impl.cpp:179-228`; overflow governed by ly-04. **Accept:** manual with a ~2×-length string at 200% font shows no unintended clipping; no sentence built by concatenation.
- [ ] **i18n-03** — RTL via framework auto-mirroring + logical start/end; never physical left/right; don't mirror direction-independent glyphs. **(FIXED).** *DALi:* `view-impl.cpp:1329-1333,1458-1478`; `view.h:239-244`. **Accept:** UTC/manual — `SetLayoutDirection(RIGHT_TO_LEFT)` mirrors children; directional affordances mirror while logos/time icons do not.
- [ ] **i18n-04** — Swap directional-affordance glyphs under RTL; leave direction-independent glyphs un-mirrored. **(PRE-FILLED:** directional glyphs (chevrons, next/prev, submit arrows, **media transport play/ff/rewind**, progress-bar direction) swap under RTL; direction-independent glyphs (checkmark, logo, clock) never mirror; a determinate progress **bar** auto-mirrors its fill direction, a spinner does not; N/A if no directional icons**).** *DALi:* swap the glyph itself under RIGHT_TO_LEFT (position-mirroring alone points it wrong). **Accept:** UTC/manual — each directional glyph is swapped and each direction-independent glyph is not; or N/A recorded.

### 12. Robustness & Edge Cases

- [ ] **rb-01** — Idempotent setters: setting current value is a no-op with no signal; clamp/normalize before store. **(FIXED).** *DALi:* `selectable-trait-impl.cpp:72-82`. **Accept:** UTC — setting the current value again emits nothing; out-of-range set clamps; getter reflects clamped value.
- [ ] **rb-02** — Update-then-notify, exactly-once; disabled suppresses all emission. **(FIXED).** *DALi:* `interactive-trait-impl.cpp:167-178,292-295`. **Accept:** UTC — a handler reading the value sees the post-change value; one interaction → one emit; disabled emits nothing.
- [ ] **rb-03** — Rapid repeated activation. **(PRE-FILLED:** One UI applies each genuine change immediately (no debounce unless the spec dictates, with the collapse window documented)**).** *DALi:* no built-in debounce; key auto-repeat via `SetMinimumKeyRepeatCount` (`ui-config-impl.h:175`). **Accept:** UTC — N rapid activations produce exactly N genuine emissions in correct alternating order with consistent state (or a single settled emission per documented window if debounced).
- [ ] **rb-04** — Degenerate inputs handled without crashing. **(FIXED).** *DALi:* `text-button-style.cpp:57-67,549-557`. **Accept:** UTC covers empty-handle DownCast, no-style `New()`, and empty text without assertion or crash.

### 13. Performance & Lifecycle

- [ ] **pl-01** — Release callbacks/observers/timers and cancel in-flight animations on disconnect/destroy; no strong back-references. **(FIXED).** *DALi:* `view-impl.cpp:405-411,2915-2917`; `interactive-trait-impl.cpp:281-287`. **Accept:** UTC/manual — disconnecting and destroying fires no callbacks afterward; no observer leak.
- [ ] **pl-02** — Don't manually forward key/focus/scene/enabled lifecycle into the trait; override `On*` only for extra behavior and call base. **(FIXED).** *DALi:* `view-impl.cpp:405-411,447-457,721-723,2915-2917,3195-3197`. **Accept:** no component override re-forwards these; any override calls its base.
- [ ] **pl-03** — Skip unnecessary animation/relayout on disabled/reduced-motion paths. **(PRE-FILLED:** no animation/relayout while disabled or (if fb-02 flag exists) when reduced-motion is on and the change is non-visual — One UI: motion must not block tasks**).** *DALi:* gate on enabled-state + fb-02 flag; `InvalidateMeasure` only on layout-affecting change (`view.h:231-259`). **Accept:** UTC/manual — no animation or relayout scheduled while disabled or under reduced-motion for non-visual changes.

### 14. ABI & Conventions

- [ ] **abi-01** — `#pragma once` + Apache-2.0 header on every new `.h`; same header (no `#pragma once`) on every `.cpp`. **(FIXED).** *DALi:* `text-button.h:1-18`; `text-button.cpp:1-16`. **Accept:** every new file has the correct license header; headers have `#pragma once`, `.cpp` files do not.
- [ ] **abi-02** — Include grouping `// CLASS HEADER`, `// EXTERNAL INCLUDES`, `// INTERNAL INCLUDES`. **(FIXED).** *DALi:* `text-button-impl.cpp:18-28`; `text-button.h:20-26`. **Accept:** include blocks are comment-labeled and grouped.
- [ ] **abi-03** — Include foundation headers only from `public-api/` and `provider-api/`; never `integration-api/` or `internal/`. **(FIXED).** *DALi:* `component-boundaries.md:16-31`. **Accept:** `rg -n '#include <dali-ui-foundation/(integration-api|internal)/'` returns zero hits.
- [ ] **abi-04** — Add new public header(s) BY HAND to umbrella `dali-ui-components.h`; do NOT edit the globbed library CMake source list. **(FIXED).** *DALi:* `dali-ui-components.h:32-33`; `build/tizen/dali-ui-components/CMakeLists.txt:5-9`. **Accept:** umbrella includes the new header(s); component builds without any library CMake source-list edit.

### 15. Testing

- [ ] **test-01** — `utc-Dali-<Name>.cpp` added to `TC_SOURCES`, opened with `UiTestApplication(Components::UiConfig::New())`, ended with `END_TEST`. **(FIXED).** *DALi:* `utc-Dali-TextButton.cpp:24-77`; `CMakeLists.txt:8-21`. **Accept:** the UTC file is listed in `TC_SOURCES`; the suite builds and passes.
- [ ] **test-02** — Standard matrix (ConstructorP, NewP + overloads, property get/set, Copy/Move, DownCastP/N, every getter default). **(FIXED).** *DALi:* `utc-Dali-TextButton.cpp:34-153`. **Accept:** all standard-matrix cases pass; each documented default asserted.
- [ ] **test-03** — Interaction coverage. **(PRE-FILLED:** verify each One UI path — immediate toggle, **ON_RELEASE key activation on Return+Space**, disabled suppression, slider Tangible pan (track grow, halo, bubble), pointer-only hover, long-press label**).** *DALi:* `interactive-trait.h:104-140`; `interactive-trait-impl.cpp:290-466`. **Accept:** UTC verifies each supported interaction path emits (or suppresses) signals per the pre-decided policy.
- [ ] **test-04** — Measure/arrange (padding, requested sizes, MATCH_PARENT/WRAP_CONTENT, min/max, RTL auto-mirror, ly-04 overflow) + style Builder/Configure/DefaultKey. **(FIXED).** *DALi:* `view-impl.cpp:1115-1141,1458-1478`; `utc-Dali-TextButton.cpp:166-277`. **Accept:** UTC asserts measured sizes, RTL mirroring, Builder round-trip + Default/DefaultKey resolution + frozen-sheet assertion.
- [ ] **test-05** — Accessibility coverage. **(PRE-FILLED:** assert the One UI role round-trip, One UI state phrases via bit changes preserving ENABLED, **indeterminate/read-only/error via description text (no STATES bit)**, focusability, `activate` routing, and (range) `OnAccessibilityValueChange` + value sync**).** *DALi:* `view-data-impl.cpp:172-188,1673-1676`; `group-selectable-trait-impl.cpp:557-568`; `view-impl.cpp:1072-1075,2803-2806`. **Accept:** UTC asserts role round-trip, state-bit changes preserving ENABLED, description-announced states, focusability, activation routing, and (range) AT value stepping.
- [ ] **test-06** — Property-system coverage only if a property surface exists (api-06). **(PRE-FILLED:** default no property surface → record N/A; add coverage only if a `-properties.h` exists**).** **Accept:** UTC round-trips each property both ways and asserts read-only rejection; or N/A recorded.

### 16. Documentation

- [ ] **doc-01** — Mandatory `@brief` on every public class; `@brief/@param/@return` on factories and non-trivial methods; internal ctors under `@cond internal`. **(FIXED).** *DALi:* `text-button.h:38-86`; `interactive-view.h:261-277`. **Accept:** every public class has a `@brief`; `New()`/`DownCast` and non-trivial methods documented; internal ctors inside `@cond internal`.
- [ ] **doc-02** — Document `DefaultKey/DefaultPreset/Default` semantics and pre-Apply-register / post-Apply-resolve ordering with the `DebugAssertStyleConfigApplied` precondition (id-03 Style=yes). **(FIXED).** *DALi:* `text-button-style.h:56-75`; `ui-style-debug.h:31-34`. **Accept:** the Style header documents the resolution order and that preset/Default require `UiConfig::HasCurrent()`.

### 17. Samples & Manual Tests

- [ ] **sm-01** — `manual-tests/dali-ui-components/tc/tc-<name>-basics.cpp` subclassing `ManualTest::TestCase`, ending with `REGISTER_MANUAL_TEST(...)`. **(FIXED).** *DALi:* auto-globbed `tc/*.cpp`; `tc-text-button-basics.cpp:159-172,366`; `manual-tests CMakeLists.txt:16`. **Accept:** the manual tc builds, registers, and demonstrates the component's public API and sizing modes.
- [ ] **sm-02** — `samples/<name>/` with `CMakeLists.txt` + example `.cpp` (id-03 sample=yes). **(PRE-FILLED:** present for every visible control; showcases the control in light/dark against One UI intent**).** *DALi:* auto-discovered by `samples/CMakeLists.txt:51-59`; model on `samples/dialog`. **Accept:** the sample directory is auto-discovered and builds against `dali2-ui-components`.

### 18. Localization

- [ ] **loc-01** — No in-component localization: text is plain `Dali::String`; app localizes via `UiLocalizationManager`; no `dali-ui-components/po` dir. **(FIXED).** *DALi:* `ui-localization-manager.h:132,210-304`. **Accept:** the component exposes localizable text as a plain string setter and adds no `po/` dir or gettext calls; app-side binding documented where relevant.

---

## 6. Fidelity Acceptance

The ported DALi component is accepted as a faithful One UI 8 port when **all** of the following hold objectively:

1. **States & transitions.** Every One UI state (Normal / Pressed / Focused / Disabled, + Selected/Checked / Busy / indeterminate where applicable) exists, with the exact transition table asserted by UTC, and each state is **distinguishable in grayscale** (a non-color cue is present).
2. **Interaction.** Toggle/click/activate behavior matches One UI (immediate value application; Return+Space activation under ON_RELEASE; disabled suppression; slider Tangible pan with track grow, thumb halo, and value bubble; radio exclusivity with a single `SelectedMemberChangedSignal`).
3. **Accessibility.** Role matches the archetype; announcement order is **status → name → type → hint**; standard One UI phrases are announced; indeterminate/read-only/error are announced via description text (no STATES bit); off-highlight changes emit ATSPI events; slider exposes the Value contract (with the string-value caveat flagged if numeric is unavailable).
4. **Motion & feedback.** Durations fall within [100,500]ms and **equal** the author-supplied table values; Basic easing (or the confirmed per-style curve) is used; Linear is used only for opacity; text clears before new text; haptics/sound are suppressible and non-sole.
5. **Theming & RTL.** Colors resolve through `UiColor` tokens and re-resolve on light/dark/high-contrast theme swap; RTL auto-mirrors, directional glyphs (including media transport) swap while direction-independent glyphs do not.
6. **Exact metrics.** Every applicable Numeric Parameter Table value is filled and **each corresponding UTC asserts equality** to that value (min-size, corner radius, track/thumb dimensions, per-role hex, focus-mark metrics, durations).
7. **Visual diff.** Once the numbers are filled, a **side-by-side screenshot diff** against the official Samsung reference (in both light and dark themes) shows no perceptible divergence in shape, color, spacing, or state rendering.

---

## 7. Worked Example — One UI 8 **Switch**

A concrete end-to-end fill for a `switch/toggle` archetype. FIXED/PRE-FILLED items are satisfied by the `SelectableView` base pair and the pre-decided defaults; only the two author inputs and the numeric table need attention.

### 7.1 What YOU provide (the tiny block)

- **Component + archetype (USER-CHOICE):** "One UI 8 **Switch** — a toggle (boolean selected) control that applies its on/off value immediately on tap." → archetype `toggle` → base pair auto-derived.
- **Reference (USER-CHOICE):** one official Samsung reference — the One UI Switch design page / a Settings-screen Switch screenshot (used only for the final visual diff).
- **Numeric table:** filled later (illustrative values below).

*(That's the entire author input for Switch. Base pair, role, states, transitions, a11y, motion, RTL, and conventions are all pre-decided.)*

### 7.2 Which pre-decided defaults apply

Base pair `SelectableView`/`Provider::SelectableViewImpl`; role **TOGGLE_BUTTON**; state phrases **On/Off**; **immediate toggle** on tap (value applies before signal); **track color cross-fades** (off-outline → `Color control activated`) **in sync with the thumb slide**; ON_RELEASE key activation on **Return + Space**; focusable via the trait (no component `SetFocusable`); focus mark keyed off `FOCUS_INDICATED` in the PRIMARY/focus color; RTL auto-mirrors (thumb travel direction follows layout direction; no manual mirroring); light/dark via token identity; haptic supplementary, matched to `toggleSlideDuration`; Style=yes + sample=yes. Standalone anatomy: **label leads, switch trailing**; label+switch form one hit target.

### 7.3 DALi base pair

`class Switch : public SelectableView` + `class SwitchImpl : public Provider::SelectableViewImpl` (Selectable guarantees Interactive; do not stack Interactive). `DALI_TYPE_REGISTRATION_BEGIN(SwitchImpl, Provider::SelectableViewImpl, Create)`. Inherits `IsSelected`/`SetSelected`/`SelectionChangedSignal`. No `-properties.h` (state via typed API). Extra ATSPI interfaces: `none` (On/Off expressible via `ACCESSIBILITY_STATES` CHECKED bit + On/Off phrase).

### 7.4 Numeric Parameter Table — **illustratively filled (EXAMPLE values — replace with the confirmed One UI metrics)**

> These numbers are placeholders to show the *shape* of the fill, **not** authoritative One UI values. The author replaces every value from the confirmed One UI 8 spec / theme resource.

| Param | Unit | EXAMPLE value | Applies to |
|---|---|---|---|
| switchTrackWidth | vpx | `52` | track |
| switchTrackHeight | vpx | `32` | track |
| switchTrackCornerRadius | vpx | `16` | track |
| switchThumbDiameterOff | vpx | `24` | thumb (off) |
| switchThumbDiameterOn | vpx | `24` | thumb (on) |
| switchThumbPressedStretchWidth | vpx | `30` | thumb (pressed) |
| switchThumbInset | vpx | `4` | thumb |
| switchThumbTravel | vpx | `20` | thumb |
| switchThumbShadowBlur / OffsetXY / ColorOpacity | vpx / vpx / hex+op | `4` / `(0,1)` / `#000000 @ 0.20` | thumb elevation |
| colorControlActivatedHex | hex (both themes) | `#3e91ff` **(UNVERIFIED — confirm)** | on-track fill |
| uncheckedOutlineHex_light / _dark | hex | `#C4C4C4` / `#5A5A5A` **(EXAMPLE)** | off-track |
| thumbColorHex_on / _off | hex | `#FFFFFF` / `#FFFFFF` **(EXAMPLE)** | thumb |
| disabledTrackHex / disabledThumbHex | hex (L+D) | `#E0E0E0` / `#F5F5F5` **(EXAMPLE)** | disabled |
| pressStateLayerColorHex / Opacity | hex / op | `#000000` / `0.10` **(EXAMPLE)** | pressed overlay |
| toggleSlideDuration | ms | `200` **(within 100–500)** | thumb travel + track cross-fade |
| easingControlPoints | cbz | `0.22,0.25,0.00,1.00` **(UNVERIFIED — confirm)** | slide easing |
| minTouchTargetSize | vpx | `48` **(EXAMPLE)** | whole control |
| focusIndicatorStrokeWidth / focusMarkOutwardOffset / focusIndicatorCornerRadius | vpx | `2` / `2` / `18` **(EXAMPLE)** | focus mark |
| focusMarkContrastRatio | ratio | `≥3:1` **(FIXED floor)** | focus mark |
| fontSizePerRole (label) | sp | `16` **(EXAMPLE)** | trailing label |

### 7.5 Fidelity acceptance for Switch

Accepted when: On/Off toggles immediately on tap and on Return/Space key-up; the thumb slides `switchThumbTravel` over `toggleSlideDuration` while the track color cross-fades on the same timing (never popping); the thumb shows the elevation shadow and the pressed stretch; disabled uses the distinct disabled track/thumb tokens (not just an alpha), suppresses input, and stays distinguishable in grayscale; AT announces **"<name>, Switch, On/Off"** in status→name→type order and emits the state-change even off-highlight; a focus mark in the PRIMARY color renders just outside the bounds at ≥3:1 contrast when SR/AT is active; RTL mirrors thumb travel with no component code; **every filled numeric equals its UTC assertion**; and a light+dark side-by-side screenshot diff against the official Switch reference shows no perceptible divergence.

---

## 8. References (official Samsung sources only)

- **One UI Color — system tokens (Primary / Primary-dark / "Color control activated" #3e91ff single value both themes):** https://developer.samsung.com/one-ui/color/system.html
- **One UI Motion — Basic easing (0.22,0.25,0.00,1.00), 100–500ms, Linear-only-for-opacity, easing styles:** https://developer.samsung.com/one-ui/motion/basic.html
- **One UI Accessibility — Color contrast (≥4.5:1 text / ≥3:1 large; large = >18dp normal / >14dp bold):** https://developer.samsung.com/one-ui/accessibility/color-contrast.html
- **One UI Accessibility — Screen reader (announcement order: status → name → element type → hint; standard state phrases):** https://developer.samsung.com/one-ui/accessibility/screen-reader.html
- **One UI Accessibility — Layout & typography (200% text scaling; no published touch-target dp):** https://developer.samsung.com/one-ui/accessibility/layout-and-typo.html
- **One UI Structure — Visual depth (blur+dim when unrelated / soft shadow only when related; never combined; uniform blur):** https://developer.samsung.com/one-ui/structure/visual-depth.html
- **One UI Layout — Grid (≥24dp side margin; responsive grid):** https://developer.samsung.com/one-ui/layout/grid.html

> Values marked **UNVERIFIED** in the tables/prose (exact Primary/accent hexes, the Basic easing control points) must be confirmed against the official resource for the **target One UI version (8 vs 8.5)** at port time, and — for the accent — treated as a **user-customizable, runtime** `UiColor` token value, not a hard-coded constant.