# DALi UI Layout — 수정 보고서 (review0)

> 날짜: 2026-06-17 · 브랜치 `claude` · worktree `/home/jae/dali/dali-ui-claude` · **이 파일 단독으로 모든 수정사항과 근거를 파악할 수 있도록 작성**


## 이 보고서 읽는 법

- 각 항목은 **위치(file:line) → 현상(버그) → 근거(일반 프레임워크 관례 + 검증자 코드분석) → 재현 → 수정 방향** 순서로, 읽고 바로 고칠 수 있게 구성.

- 대상: DALi UI 레이아웃 서브시스템(코어 Measure/Arrange 엔진, Stack/Flex/Grid/Absolute 매니저, LayoutParams, 트랜지션, ScrollView; 약 14k LOC) + UTC 테스트.

- 비교 기준: Android `View.measure/onLayout`+`MeasureSpec`, WPF/UWP `Measure/Arrange`(star sizing), W3C **CSS Flexbox §9.7**, CSS Grid/WPF Grid, AndroidX Transition.

- 검증: 발견마다 독립 회의론자 2~3명이 코드 재독해로 적대적 검증 → 다수결 `confirmed`만 '수정 필요'로 분류(`refuted` 폐기). high/critical은 3명.

- **수정 필요(confirmed): 115건** (high 24 · medium 47 · low 38 · info 6), 의도적 설계라 재검토 권장 25건, 폐기 11건.

- ⚠️ 정적 분석 기반 — 특히 HIGH는 수정 전 실제 빌드/런타임 재현으로 최종 확인 권장.


---
## 우선순위 수정 체크리스트 (HIGH)

- [ ] **HIGH** [`measure-engine/measure-01`](#fix-measure-engine-measure-01) — No MeasureSpec mode: constraints are a bare float, so AT_MOST/EXACTLY/UNSPECIFIED cannot be distinguished  ·  `view-impl.cpp:881`

- [ ] **HIGH** [`measure-engine/measure-02`](#fix-measure-engine-measure-02) — WRAP_CONTENT result is never clamped to the incoming (AT_MOST) constraint — oversized WRAP children silently overflow  ·  `view-impl.cpp:1004-1006`

- [ ] **HIGH** [`invalidation-controller/inval-01`](#fix-invalidation-controller-inval-01) — Entire LayoutController UTC suite is commented out — zero coverage for invalidation/scheduling  ·  `/home/jae/dali/dali-ui-claude/automated-tests/src/dali-ui-foundation/utc-Dali-LayoutController.cpp:37-92`

- [ ] **HIGH** [`stack-layout/stack-01`](#fix-stack-layout-stack-01) — Non-weight MATCH_PARENT child on the MAIN axis fills the whole axis and overlaps siblings (no main-axis fill arbitration)  ·  `stack-layout-manager.cpp:464-467`

- [ ] **HIGH** [`stack-layout/stack-02`](#fix-stack-layout-stack-02) — Arrange weight leftover ignores main-axis MATCH_PARENT non-weight children (computes leftover as if they take 0)  ·  `stack-layout-manager.cpp:382-388`

- [ ] **HIGH** [`flex-layout/flex-01`](#fix-flex-layout-flex-01) — flex-basis: 0 is ignored; falls back to content size, breaking the canonical `flex: 1` idiom  ·  `flex-layout-manager.cpp:94`

- [ ] **HIGH** [`flex-layout/flex-02`](#fix-flex-layout-flex-02) — No iterative min/max freeze-and-redistribute loop (CSS 9.7) — single pass can violate min sizes and leave space mis-filled  ·  `flex-layout-manager.cpp:133-191`

- [ ] **HIGH** [`grid-layout/grid-01`](#fix-grid-layout-grid-01) — Spanning cells contribute nothing to AUTO track sizing  ·  `grid-layout-manager.cpp:120-127`

- [ ] **HIGH** [`absolute-layout/abs-01`](#fix-absolute-layout-abs-01) — RTL unconditionally mirrors absolute X — diverges from CSS `left`/WPF Canvas, and no flag to opt out  ·  `view-impl.cpp:1193-1213`

- [ ] **HIGH** [`absolute-layout/abs-03`](#fix-absolute-layout-abs-03) — Proportional WIDTH/HEIGHT in Measure resolves against the incoming constraint, inflating a WRAP_CONTENT container (size circularity)  ·  `absolute-layout-manager.cpp:114-117`

- [ ] **HIGH** [`layout-base-callbacks/layout-01`](#fix-layout-base-callbacks-layout-01) — Callback Measure/Arrange path ignores the view's padding, unlike the LayoutManager and default paths  ·  `view-impl.cpp:918-921`

- [ ] **HIGH** [`layout-base-callbacks/layout-02`](#fix-layout-base-callbacks-layout-02) — Callback Measure path ignores RequestedWidth/RequestedHeight; manager and default paths force them  ·  `view-impl.cpp:905-906`

- [ ] **HIGH** [`scroll-bounds-effects/scroll-01`](#fix-scroll-bounds-effects-scroll-01) — Cross-axis content is measured unbounded; overflow on the non-scroll axis is silently clipped and unreachable  ·  `scroll-view-layout-manager.cpp:78-89`

- [ ] **HIGH** [`x-matchparent-wrapcontent-matrix/layout-02`](#fix-x-matchparent-wrapcontent-matrix-layout-02) — FlexLayout MATCH_PARENT on the main axis overrides and double-counts flex-grow  ·  `flex-layout-manager.cpp:272-278`

- [ ] **HIGH** [`x-measure-cache/cache-01`](#fix-x-measure-cache-cache-01) — Several Label size-affecting setters never invalidate the measure cache → stale measured size  ·  `dali-ui-foundation/integration-api/label-impl.cpp:230-235`

- [ ] **HIGH** [`x-layout-direction-rtl/rtl-01`](#fix-x-layout-direction-rtl-rtl-01) — RTL mirror axis is the layout's measured width, not a content box enclosing children — under-measured/overflowing custom & absolute layouts mirror wrong  ·  `public-api/view-impl.cpp:1079`

- [ ] **HIGH** [`x-margin-padding-alignment/pad-01`](#fix-x-margin-padding-alignment-pad-01) — Padding double-counted for WRAP_CONTENT leaf View with a background visual  ·  `view-impl.cpp:2983-2984`

- [ ] **HIGH** [`x-distribution-rounding/flex-01`](#fix-x-distribution-rounding-flex-01) — Flex grow/shrink and stack weight never freeze/respect child min/max or intrinsic min-content; single non-iterative pass overruns  ·  `flex-layout-manager.cpp:139-189`

- [ ] **HIGH** [`x-lifecycle-mutation/child-01`](#fix-x-lifecycle-mutation-child-01) — OnChildOrderChanged wholesale-rebuilds mChildren from Actor order, silently discarding PRESERVE / Insert layout-order divergence  ·  `view-impl.cpp:2826-2851`

- [ ] **HIGH** [`x-lifecycle-mutation/child-02`](#fix-x-lifecycle-mutation-child-02) — Mutating children during the plain-View Measure/Arrange pass invalidates the live mChildren iterator (UB)  ·  `view-impl.cpp:966`

- [ ] **HIGH** [`x-numeric-edge-cases/num-01`](#fix-x-numeric-edge-cases-num-01) — RequestedWidth/Height, Minimum/Maximum size accept negative/NaN/Inf with no validation; negative Minimum defeats the sentinel firewall  ·  `view-data-impl.cpp:1381-1471`

- [ ] **HIGH** [`x-test-coverage-gaps/flex-01`](#fix-x-test-coverage-gaps-flex-01) — flex-grow / flex-shrink distribution has NO value-asserting test anywhere  ·  `utc-Dali-FlexLayout.cpp:378-398`

- [ ] **HIGH** [`x-test-coverage-gaps/cov-01`](#fix-x-test-coverage-gaps-cov-01) — LayoutController has zero active tests (entire UTC commented out)  ·  `utc-Dali-LayoutController.cpp:37-92`

- [ ] **HIGH** [`x-test-coverage-gaps/cov-02`](#fix-x-test-coverage-gaps-cov-02) — View min/max width/height clamping is untested for all layouts  ·  `view-impl.cpp:905-906`


<details><summary>MEDIUM 수정 체크리스트 (펼치기)</summary>


- [ ] MEDIUM [`measure-engine/measure-04`](#fix-measure-engine-measure-04) — Min/Max is applied twice and in an unusual role: it clamps the INCOMING available space, not just the desired size

- [ ] MEDIUM [`arrange-engine/arr-02`](#fix-arrange-engine-arr-02) — No clip / RenderSize-vs-DesiredSize contract: arranging smaller than measured silently shrinks the child

- [ ] MEDIUM [`arrange-engine/arr-04`](#fix-arrange-engine-arr-04) — Stack main-axis MATCH_PARENT gives each child the FULL main extent -> multiple such children overflow/overlap

- [ ] MEDIUM [`standalone-children/standalone-01`](#fix-standalone-children-standalone-01) — Standalone MATCH_PARENT child min/max clamp depends on which of two arrange paths runs

- [ ] MEDIUM [`invalidation-controller/inval-04`](#fix-invalidation-controller-inval-04) — OnWindowResize mutates mAllLayoutRoots while iterating it (potential iterator invalidation)

- [ ] MEDIUM [`stack-layout/stack-05`](#fix-stack-layout-stack-05) — Weight child ignores its requested main-axis size entirely (deviates from Android layout_weight / flex-grow base size)

- [ ] MEDIUM [`stack-layout/stack-07`](#fix-stack-layout-stack-07) — Missing test coverage for multiple weight children, weight+rounding, and multiple main-axis MATCH_PARENT collision

- [ ] MEDIUM [`flex-layout/flex-03`](#fix-flex-layout-flex-03) — align-items/align-self STRETCH overrides a child's explicit cross size (CSS stretches only auto)

- [ ] MEDIUM [`flex-layout/flex-04`](#fix-flex-layout-flex-04) — Main-axis MATCH_PARENT takes full available main, overlapping siblings and ignoring flex distribution

- [ ] MEDIUM [`flex-layout/flex-05`](#fix-flex-layout-flex-05) — Flex factors resolved in Arrange only; Measure reports un-flexed sum (WRAP_CONTENT container size wrong)

- [ ] MEDIUM [`flex-layout/flex-06`](#fix-flex-layout-flex-06) — ROW_REVERSE/COLUMN_REVERSE combined with RTL double-reverses (untested interaction)

- [ ] MEDIUM [`flex-layout/flex-09`](#fix-flex-layout-flex-09) — Grow/shrink test coverage asserts only container measured size, never child sizes/positions after distribution

- [ ] MEDIUM [`grid-layout/grid-02`](#fix-grid-layout-grid-02) — Star/fr distribution has no per-track min/max and no redistribution

- [ ] MEDIUM [`grid-layout/grid-03`](#fix-grid-layout-grid-03) — No auto-placement: every unparameterized child stacks at cell (0,0)

- [ ] MEDIUM [`grid-layout/grid-04`](#fix-grid-layout-grid-04) — Out-of-range row/column and over-long span clamp into the last track (no implicit tracks)

- [ ] MEDIUM [`absolute-layout/abs-02`](#fix-absolute-layout-abs-02) — Proportional position is an alignment fraction (available-child)*p, not a CSS percentage inset

- [ ] MEDIUM [`absolute-layout/abs-04`](#fix-absolute-layout-abs-04) — MATCH_PARENT child with no explicit bounds is measured to natural size in Measure but contributes natural (not min) size to the bounding box

- [ ] MEDIUM [`absolute-layout/abs-06`](#fix-absolute-layout-abs-06) — Asymmetric container padding breaks RTL mirroring of absolute children

- [ ] MEDIUM [`layout-base-callbacks/layout-03`](#fix-layout-base-callbacks-layout-03) — No detach/replace for an attached LayoutManager — attach-once-for-life with a hard runtime assert

- [ ] MEDIUM [`layout-params/lp-03`](#fix-layout-params-lp-03) — Foreign params type is silently ignored — no checkLayoutParams/generateLayoutParams conversion or assert

- [ ] MEDIUM [`layout-params/lp-04`](#fix-layout-params-lp-04) — Stale params of other types persist across reparenting and resurrect (no trait cleanup on layout change)

- [ ] MEDIUM [`scroll-bounds-effects/scroll-02`](#fix-scroll-bounds-effects-scroll-02) — Manager Arrange double-applies effective scale to the content's scroll offset

- [ ] MEDIUM [`scroll-bounds-effects/scroll-03`](#fix-scroll-bounds-effects-scroll-03) — Manager Arrange ignores the content-box origin (padding) and depends on an external imperative scroll model for placement

- [ ] MEDIUM [`x-matchparent-wrapcontent-matrix/layout-03`](#fix-x-matchparent-wrapcontent-matrix-layout-03) — ScrollView reports full viewport (not minimum) for a MATCH_PARENT child in Measure, violating the follower contract

- [ ] MEDIUM [`x-matchparent-wrapcontent-matrix/layout-04`](#fix-x-matchparent-wrapcontent-matrix-layout-04) — GridLayout silently ignores per-child START/CENTER/END alignment for MATCH_PARENT cells

- [ ] MEDIUM [`x-matchparent-wrapcontent-matrix/layout-05`](#fix-x-matchparent-wrapcontent-matrix-layout-05) — FlexBasis > 0 overrides MATCH_PARENT minimum in Measure, so a basis-sized MATCH_PARENT child DOES contribute to a WRAP parent

- [ ] MEDIUM [`x-measure-cache/cache-05`](#fix-x-measure-cache-cache-05) — flex-grow / flex-shrink resize a child's main size but the child is NOT re-measured at the grown/shrunk size unless it is MATCH_PARENT

- [ ] MEDIUM [`x-measure-cache/cache-06`](#fix-x-measure-cache-cache-06) — Correctness depends on manual InvalidateMeasure at every mutation; there is no AffectsMeasure metadata or forceLayout backstop

- [ ] MEDIUM [`x-layout-direction-rtl/rtl-02`](#fix-x-layout-direction-rtl-rtl-02) — AbsoluteLayout mirrors explicit X bounds under RTL — deviates from Android AbsoluteLayout/FrameLayout and CSS position:absolute (physical left does NOT mirror)

- [ ] MEDIUM [`x-layout-direction-rtl/rtl-03`](#fix-x-layout-direction-rtl-rtl-03) — flex-direction:ROW_REVERSE combined with RTL (the CSS double-negative) is untested

- [ ] MEDIUM [`x-margin-padding-alignment/margin-01`](#fix-x-margin-padding-alignment-margin-01) — ScrollViewLayoutManager ignores per-child margin entirely (both axes, Measure and Arrange)

- [ ] MEDIUM [`x-distribution-rounding/flex-02`](#fix-x-distribution-rounding-flex-02) — Flex shrink overflow that hits the per-child zero floor is silently lost (line overflows container)

- [ ] MEDIUM [`x-distribution-rounding/grid-04`](#fix-x-distribution-rounding-grid-04) — Negative Star factor and negative flex-basis are not clamped, producing negative/garbage track and item sizes

- [ ] MEDIUM [`x-distribution-rounding/stack-05`](#fix-x-distribution-rounding-stack-05) — Stack weight is not weighted by basis and can over-fill when remainingMain<0 is clamped, plus weight children ignore min/max in Arrange

- [ ] MEDIUM [`x-distribution-rounding/grid-08`](#fix-x-distribution-rounding-grid-08) — Grid star distribution lacks per-track min/max clamp+re-proportion; auto content floor only honored on implicit tracks, not explicit Star tracks

- [ ] MEDIUM [`x-lifecycle-mutation/child-03`](#fix-x-lifecycle-mutation-child-03) — Concrete LayoutManagers iterate a pre-pass child SNAPSHOT, not the live list — mid-pass add/remove is silently ignored

- [ ] MEDIUM [`x-lifecycle-mutation/child-04`](#fix-x-lifecycle-mutation-child-04) — GetChildAt/GetChildCount/IndexOfChild return logical (layout) order, which can diverge from Actor visual order, but this is undocumented

- [ ] MEDIUM [`x-numeric-edge-cases/num-03`](#fix-x-numeric-edge-cases-num-03) — MeasuredSize has no measured-state/overflow flag — clamping and overflow are silent and undetectable by parents

- [ ] MEDIUM [`x-numeric-edge-cases/num-04`](#fix-x-numeric-edge-cases-num-04) — GridLength::Absolute accepts negative pixels and leaks straight into track widths and remaining-space math

- [ ] MEDIUM [`x-numeric-edge-cases/num-05`](#fix-x-numeric-edge-cases-num-05) — Stack weight and Flex basis accept arbitrary values; NaN weight/basis propagates through division and max() poisoning the whole line

- [ ] MEDIUM [`x-test-coverage-gaps/flex-02`](#fix-x-test-coverage-gaps-flex-02) — flex-basis of 0 is ignored (treated as WRAP_CONTENT) — deviates from CSS

- [ ] MEDIUM [`x-test-coverage-gaps/flex-03`](#fix-x-test-coverage-gaps-flex-03) — No flex min/max-content clamping during grow/shrink

- [ ] MEDIUM [`x-test-coverage-gaps/grid-01`](#fix-x-test-coverage-gaps-grid-01) — Spanned children never contribute to AUTO track sizing

- [ ] MEDIUM [`x-test-coverage-gaps/cov-03`](#fix-x-test-coverage-gaps-cov-03) — Params-sharing footgun (same handle on multiple views) is documented but never tested

- [ ] MEDIUM [`x-test-coverage-gaps/cov-04`](#fix-x-test-coverage-gaps-cov-04) — Weight/star/grow rounding-sum (fill-to-edge) never asserted

- [ ] MEDIUM [`x-test-coverage-gaps/stack-01`](#fix-x-test-coverage-gaps-stack-01) — Weak/tautological assertions pin questionable Stack & Grid results

- [ ] MEDIUM [`x-test-coverage-gaps/cov-06`](#fix-x-test-coverage-gaps-cov-06) — Mutation-during-layout and nested-manager layouts untested


</details>


<details><summary>LOW / INFO 수정 체크리스트 (펼치기)</summary>


- [ ] LOW [`measure-engine/measure-07`](#fix-measure-engine-measure-07) — MATCH_PARENT 'follower' reports GetMinimumWidth() (default 0) as desired size — a documented but convention-divergent measure result

- [ ] LOW [`arrange-engine/arr-05`](#fix-arrange-engine-arr-05) — RTL mirroring uses full parent width, breaking with asymmetric start/end padding

- [ ] LOW [`standalone-children/standalone-03`](#fix-standalone-children-standalone-03) — InvalidateArrange standalone boundary omits the transition-parent invalidation that InvalidateMeasure performs

- [ ] INFO [`standalone-children/standalone-04`](#fix-standalone-children-standalone-04) — Standalone children are not re-measured on a parent Measure cache hit (relies on self-root path)

- [ ] LOW [`invalidation-controller/inval-05`](#fix-invalidation-controller-inval-05) — mProcessingScheduled is dead control state — set/cleared but never gates anything

- [ ] LOW [`invalidation-controller/inval-06`](#fix-invalidation-controller-inval-06) — Depth-sort recomputes ancestor depth by full parent walk per comparison

- [ ] LOW [`invalidation-controller/inval-07`](#fix-invalidation-controller-inval-07) — ReplaceCurrentWindow reconnects ResizeSignal without disconnecting the prior connection

- [ ] INFO [`invalidation-controller/inval-08`](#fix-invalidation-controller-inval-08) — Dirty signal fused into the constraint cache sentinel instead of a discrete dirty flag (deviation from Android/WPF)

- [ ] LOW [`stack-layout/stack-08`](#fix-stack-layout-stack-08) — Cross-axis CENTER/END do not clamp when child exceeds available cross size (negative offset, child can spill before the start edge)

- [ ] INFO [`stack-layout/stack-04`](#fix-stack-layout-stack-04) — Trailing spacing is added after the last child in Arrange (asymmetric with Measure's (N-1) model)

- [ ] LOW [`flex-layout/flex-07`](#fix-flex-layout-flex-07) — Grow distributed proportionally to grow factor but with no remainder/rounding correction (subpixel gaps)

- [ ] LOW [`flex-layout/flex-10`](#fix-flex-layout-flex-10) — NO_WRAP single line is force-stretched to full cross size, silently disabling align-content and overriding the line's natural cross extent

- [ ] LOW [`grid-layout/grid-06`](#fix-grid-layout-grid-06) — WRAP_CONTENT grid collapses STAR tracks to ~0 with no content floor

- [ ] LOW [`grid-layout/grid-07`](#fix-grid-layout-grid-07) — Overlapping cells have no defined z-order or conflict handling

- [ ] LOW [`grid-layout/grid-08`](#fix-grid-layout-grid-08) — Auto-track contribution and span correctness are untested

- [ ] LOW [`absolute-layout/abs-08`](#fix-absolute-layout-abs-08) — AbsoluteLayoutFlags has no anchor/origin or edge (right/bottom) flags — only a single (x,y,w,h) rect, unlike every compared framework

- [ ] LOW [`layout-base-callbacks/layout-05`](#fix-layout-base-callbacks-layout-05) — SetMeasureCallback({}) / SetArrangeCallback({}) leaves the LayoutCallbacks trait object permanently allocated; doc says it 'removes'

- [ ] LOW [`layout-base-callbacks/layout-07`](#fix-layout-base-callbacks-layout-07) — A LayoutManager that does not skip STANDALONE children double-arranges them; correctness depends on manager author calling IsStandalone

- [ ] INFO [`layout-base-callbacks/layout-08`](#fix-layout-base-callbacks-layout-08) — layout-impl.cpp is an empty stub: the documented 'LayoutImpl::OnMeasure/OnArrange override' and 'mLayoutManager moved to LayoutImpl' were never implemented

- [ ] LOW [`layout-params/lp-05`](#fix-layout-params-lp-05) — SetLayoutParams with an empty/uninitialized handle dereferences a null BaseObject (crash, no guard)

- [ ] LOW [`layout-params/lp-06`](#fix-layout-params-lp-06) — Sharing/mutation/foreign-type/reparent semantics are entirely untested in UTCs

- [ ] LOW [`layout-transition/lt-02`](#fix-layout-transition-lt-02) — Animator wall-clock delta is clamped per tick (MAX_TICK_DELTA), so a frame hitch stretches the transition in slow motion instead of catching up

- [ ] LOW [`layout-transition/lt-03`](#fix-layout-transition-lt-03) — Spec-mode CHANGE/ENTER animations are never cancelled when their child's ancestor (not the child) is destroyed off-scene

- [ ] LOW [`x-matchparent-wrapcontent-matrix/layout-06`](#fix-x-matchparent-wrapcontent-matrix-layout-06) — Grid implicit (definition-less) track is treated as Star(1) and fills, so a MATCH_PARENT-only implicit grid behaves differently from an explicit AUTO track

- [ ] LOW [`x-matchparent-wrapcontent-matrix/layout-07`](#fix-x-matchparent-wrapcontent-matrix-layout-07) — AbsoluteLayout sizes a WRAP_CONTENT parent from default-positioned children's natural size, unlike CSS absolute positioning

- [ ] LOW [`x-measure-cache/cache-03`](#fix-x-measure-cache-cache-03) — Measure cache compares constraints with a 0.001 epsilon instead of exact equality

- [ ] INFO [`x-measure-cache/cache-04`](#fix-x-measure-cache-cache-04) — Cache-hit guard checks width>=0 but not height>=0 (asymmetric defensive check)

- [ ] LOW [`x-layout-direction-rtl/rtl-04`](#fix-x-layout-direction-rtl-rtl-04) — Vertical StackLayout cross-axis START/END alignment mirroring under RTL is untested

- [ ] LOW [`x-layout-direction-rtl/rtl-05`](#fix-x-layout-direction-rtl-rtl-05) — RTL mirror reads live actor POSITION_X/SIZE_WIDTH and rewrites in place — correctness depends on every Arrange re-running the manager first (fragile idempotency)

- [ ] LOW [`x-layout-direction-rtl/rtl-06`](#fix-x-layout-direction-rtl-rtl-06) — Default OnRelayout (non-layout legacy path) swaps padding for RTL but the main layout pipeline never uses that code; two divergent RTL implementations

- [ ] LOW [`x-margin-padding-alignment/align-01`](#fix-x-margin-padding-alignment-align-01) — CENTER alignment uses unrounded float midpoint (asymmetric / sub-pixel centering)

- [ ] LOW [`x-margin-padding-alignment/stack-fill-01`](#fix-x-margin-padding-alignment-stack-fill-01) — Stack cross-axis FILL only stretches WRAP_CONTENT children; fixed-size children silently START-align

- [ ] LOW [`x-margin-padding-alignment/neg-margin-01`](#fix-x-margin-padding-alignment-neg-margin-01) — Negative margins partially honored but untested across all managers

- [ ] LOW [`x-distribution-rounding/dist-03`](#fix-x-distribution-rounding-dist-03) — No rounding-remainder reconciliation in any distributor; accumulated float error means shares do not sum exactly to available

- [ ] LOW [`x-distribution-rounding/dist-06`](#fix-x-distribution-rounding-dist-06) — weightSum/growSum guarded against zero but a child carrying a factor while sum>0 yet target space is zero yields silent collapse; divide-by-zero avoided everywhere (positive finding) — but NaN/Inf from factors can still propagate

- [ ] LOW [`x-distribution-rounding/flex-07`](#fix-x-distribution-rounding-flex-07) — Flex Measure pass does NOT apply grow/shrink, so a flex container's measured (desired) size ignores flex sizing — inconsistent with Arrange

- [ ] LOW [`x-lifecycle-mutation/child-05`](#fix-x-lifecycle-mutation-child-05) — Dual reorder API surface: inherited Dali::Actor::Raise() (no-arg) coexists with Raise(LayoutOrderPolicy) and behaves like UPDATE, a footgun

- [ ] LOW [`x-lifecycle-mutation/child-06`](#fix-x-lifecycle-mutation-child-06) — Insert invalidates Measure while OnChildOrderChanged invalidates only Arrange for the same logical-reorder operation

- [ ] INFO [`x-lifecycle-mutation/child-07`](#fix-x-lifecycle-mutation-child-07) — StackLayoutManager defines a private IsChildStandalone instead of the base LayoutManager::IsStandalone helper used by Absolute/ScrollView

- [ ] LOW [`x-numeric-edge-cases/num-06`](#fix-x-numeric-edge-cases-num-06) — Min/Max conflict resolution is max-wins via order-dependent max-then-min, not a documented invariant

- [ ] LOW [`x-numeric-edge-cases/num-07`](#fix-x-numeric-edge-cases-num-07) — Absolute layout cannot express a real -1px/-2px bound and silently reinterprets any negative bound as auto

- [ ] LOW [`x-numeric-edge-cases/num-08`](#fix-x-numeric-edge-cases-num-08) — NaN constraint defeats the Measure cache (FloatEqual is false for NaN), forcing re-measure every frame

- [ ] LOW [`x-test-coverage-gaps/cov-05`](#fix-x-test-coverage-gaps-cov-05) — Axis asymmetry: cross-axis MATCH_PARENT/justify/align tested mostly on one axis only

- [ ] LOW [`x-test-coverage-gaps/abs-01`](#fix-x-test-coverage-gaps-abs-01) — AbsoluteLayout ignores parent padding (inconsistent with other managers)


</details>


---
## 확정 수정사항 (상세)


### Core Measure 엔진 (`measure-engine`) — 4건


<a id="fix-measure-engine-measure-01"></a>

#### 🟠 HIGH · `measure-01` — No MeasureSpec mode: constraints are a bare float, so AT_MOST/EXACTLY/UNSPECIFIED cannot be distinguished

- **위치**: `view-impl.cpp:881 signature; view-impl.cpp:956 `(mImpl->mRequestedWidth >= 0) ? mImpl->mRequestedWidth : natW`; layout-types.h:307 `using MeasureCallback = Callback<MeasuredSize(View, float, float)>``

- **현상(버그)**: Measure/OnMeasure take plain `float widthConstraint/heightConstraint` (view-impl.cpp:881, 945). The only signal is the sign (`>= 0`). There is no mode field, no EXACTLY vs AT_MOST vs UNSPECIFIED, and grep finds zero occurrences of MeasureSpec/AT_MOST/EXACTLY/UNSPECIFIED in dali-ui-foundation.

- **근거 — 일반 프레임워크 기대동작**: A general UI framework encodes mode+size (Android packed MeasureSpec) or uses Infinity for 'size to content' (WPF). The child must be able to tell 'you have at most N' (clamp to N) from 'you are exactly N' (fill N) from 'unbounded' (size to content).

- **근거 — 프레임워크 레퍼런스**: Android View.MeasureSpec (EXACTLY/AT_MOST/UNSPECIFIED); WPF UIElement.Measure(availableSize) with Double.PositiveInfinity

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Every cited fact holds against the source at /home/jae/dali/dali-ui-claude.  SIGNATURES (verified): view-impl.cpp:881 `MeasuredSize ViewImpl::Measure(float visualW, float visualH)` and view-impl.cpp:945 `MeasuredSize ViewImpl::OnMeasure(float widthConstraint, float heightConstraint)` are both bare floats with no mode parameter. layout-types.h:307 `using MeasureCallback = Callback<MeasuredSize(View, float, float)>` matches exactly — two plain floats, no MeasureSpec struct. view-impl.cpp:956 reads `(mImpl->mRequestedWidth >= 0) ? mImpl->mRequestedWidth : natW`, exactly as cited.  NO MODE EXISTS (verified): grep for MeasureSpec/AT_MOST/EXACTLY/U…

    - `intent`(intentional-design [보정:low]): VERIFIED FACTS (all under /home/jae/dali/dali-ui-claude): - Signatures use a bare float: view-impl.h:583 `MeasuredSize Measure(float widthConstraint, float heightConstraint)`; view-impl.h:984 `virtual MeasuredSize OnMeasure(float widthConstraint, float heightConstraint)`; layout-types.h:307 `using MeasureCallback = Callback<MeasuredSize(View, float, float)>`. So the finding's "bare float, no mode field" is factually correct. - Grep over dali-ui-foundation/, docs/, wiki/ returns ZERO hits for MeasureSpec/AT_MOST/EXACTLY/UNSPECIFIED — confirmed, no packed-mode encoding exists anywhere. - The single float is NOT just sign-as-signal; it is an ava…

- **반론(소수의견)**: `code2`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: A horizontal Stack with a WRAP child measures the child with the line's remaining width. The child cannot know whether that number is a hard cap (AT_MOST) or merely informational, so it always sizes to content and may overflow.

- **▶ 수정 방향**: `view-impl.cpp:881` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-measure-engine-measure-02"></a>

#### 🟠 HIGH · `measure-02` — WRAP_CONTENT result is never clamped to the incoming (AT_MOST) constraint — oversized WRAP children silently overflow

- **위치**: `view-impl.cpp:1004-1006 `size.width = maxRight + pw;` with no min against natW; ApplyConstraints view-impl.cpp:1424-1427 clamps only to GetMinimum/MaximumWidth`

- **현상(버그)**: For a WRAP container, OnMeasure returns `maxRight + pw` (content bounding box, view-impl.cpp:1006/1018) or `naturalSize + pw` for a leaf (1035/1048). The incoming constraint `natW` is used only to derive children's available space (line 956-960); the container's own returned size is never `min(content, natW)`. ApplyConstraints (1419) clamps only to this view's own min/max, not to the parent-supplied available space.

- **근거 — 일반 프레임워크 기대동작**: Android resolveSize() under AT_MOST returns min(desired, specSize) and flags MEASURED_STATE_TOO_SMALL; WPF clamps DesiredSize to availableSize. The measured size of a wrap view should not exceed the space the parent said was available.

- **근거 — 프레임워크 레퍼런스**: Android View.resolveSize/resolveSizeAndState (AT_MOST branch); WPF MeasureCore clamping DesiredSize to availableSize

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read all cited code in /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp and the data flow matches the finding (line numbers shifted ~2 vs the finding's, but content is correct).  WRAP_CONTENT return is unclamped to the incoming constraint: - Container-with-children WRAP width = `size.width = maxRight + pw;` (view-impl.cpp:1006), height = `maxBottom + ph;` (1018). No `min(., natW)`. - Leaf WRAP width = `naturalSize.width + pw` (1035), height = `naturalSize.height + ph` (1048). No clamp. - The incoming constraint `natW` is consumed only to derive children's available space: `effectiveWidth = (mRequestedWidth >= 0) …

    - `intent`(intentional-design [보정:low]): The finding's CODE claims are accurate (line numbers off by ~12 vs the real file /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp). Verified: WRAP_CONTENT returns `size.width = maxRight + pw;` (view-impl.cpp:1006) for a container and `naturalSize.width + pw` (view-impl.cpp:1035) for a leaf, with no min against the incoming constraint. The incoming constraint `natW` is consumed only to derive children's available space (view-impl.cpp:956-960, 981-982). ApplyConstraints (view-impl.cpp:1419-1428) clamps only to this view's own GetMinimum/MaximumWidth*scale (lines 1424-1427), never to the parent-supplied size. So "WRAP re…

    - `code2`(intentional-design): I re-read every cited line. The finding's CODE FACTS are all accurate:  1. WRAP container returns its content bounding box without clamping to the incoming constraint. view-impl.cpp:1004-1006 `size.width = maxRight + pw;` and 1016-1018 `size.height = maxBottom + ph;` (children branch); leaf branch 1034-1035 `size.width = naturalSize.width + pw` and 1047-1048 for height. No `std::min` against natW/natH anywhere in OnMeasure. 2. The incoming natW/natH are consumed ONLY to derive children's available space: natW→effectiveWidth (view-impl.cpp:956)→contentWidth (959)→child constraints (981-982). The container's own return never references natW/nat…

- **재현**: Parent content width = 100; a WRAP child whose natural content is 300 returns 300. The parent (also WRAP) then reports 300+, overflowing whatever bounded it, and the original 100 cap is lost.

- **▶ 수정 방향**: `view-impl.cpp:1004-1006` 에서 위 '기대동작'을 만족하도록 수정. (분류: spec-violation)


<a id="fix-measure-engine-measure-04"></a>

#### 🟡 MEDIUM · `measure-04` — Min/Max is applied twice and in an unusual role: it clamps the INCOMING available space, not just the desired size

- **위치**: `view-impl.cpp:905-906 (clamp of constraint) and view-impl.cpp:1424-1427 (clamp of result); comment at 900-904 explicitly justifies clamping the constraint`

- **현상(버그)**: Measure clamps the incoming constraint to this view's own min/max before measuring: `effNatW = min(max(natW, minW), maxW)` (view-impl.cpp:905-906), then ApplyConstraints clamps the RESULT to the same min/max again (1424-1427). So a view's Maximum bounds the space its own children are measured against, not only its reported size.

- **근거 — 일반 프레임워크 기대동작**: Android/WPF apply Min/Max to the resolved DesiredSize only. The available space handed to a view is whatever the parent allotted; a child's own MaxWidth should cap the child's size, not shrink the constraint passed down to grandchildren.

- **근거 — 프레임워크 레퍼런스**: WPF FrameworkElement Min/Max clamp on DesiredSize; Android resolveSize applies bounds to desired, not to the spec passed to children

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read the cited code (in this checkout the file is dali-ui-foundation/public-api/view-impl.cpp; the finding's line numbers 905-906/1424-1427 match within a few lines).  (1) Incoming-constraint clamp CONFIRMED. view-impl.cpp:905-906 computes `effNatW = std::min(std::max(natW, GetMinimumWidth()), GetMaximumWidth())` (and the height analog). The explanatory comment at 900-904 explicitly states the intent is so that "OnMeasure (and therefore child measurements) see the effective available space" — i.e. it deliberately shrinks the constraint, not just the reported size.  (2) The clamped value propagates to children CONFIRMED by direct data-flo…

    - `intent`(intentional-design [보정:info]): The finding's MECHANISM is accurately observed but its "expected" framework claim is wrong, and the behavior is a deliberate, documented design decision — not an unintended deviation.  VERIFIED MECHANICS (cited): - view-impl.cpp:905-906 clamps the incoming constraint to this view's own min/max BEFORE OnMeasure: effNatW = min(max(natW, GetMinimumWidth()), GetMaximumWidth()). (Finding cited 905-906; line numbers match exactly in /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp.) - view-impl.cpp:1419-1428 ApplyConstraints clamps the RESULT to the same min/max again. (Finding cited 1424-1427; the clamp block is 1424-1427,…

- **재현**: View has MaximumWidth=50 and a WRAP child with 200px content, given constraint 1000. effNatW becomes 50, so the child is measured against contentWidth≈50 and squeezed to 50, even though the natural WRAP result (200, then clamped to 50) would have been the same for the view but the child's internal layout is now driven by 50 instead of its content. The grandchild sizing is coupled to the ancestor's Maximum.

- **▶ 수정 방향**: `view-impl.cpp:905-906` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-measure-engine-measure-07"></a>

#### 🟢 LOW · `measure-07` — MATCH_PARENT 'follower' reports GetMinimumWidth() (default 0) as desired size — a documented but convention-divergent measure result

- **위치**: `view-impl.cpp:1000-1002 and 1028-1030 (`size.width = mImpl->GetMinimumWidth()` for MATCH); re-measure at view-impl.cpp:1148-1151; docs/layout-structure.md:125-149`

- **현상(버그)**: A MATCH_PARENT view returns `GetMinimumWidth()` (default 0) from OnMeasure (view-impl.cpp:1000-1002, 1028-1030) and from DispatchMeasureWithLayoutManager (2444-2445), then is re-measured/re-sized to fill during Arrange (1136-1151). The docs (layout-structure.md:131-139) deliberately specify this 'reports minimum in Measure, fills in Arrange' model.

- **근거 — 일반 프레임워크 기대동작**: Android MATCH_PARENT children are measured with an EXACTLY spec equal to the parent's resolved content size during the parent's own measure pass, so the child's measured size already equals its final size (single pass). DALi instead returns ~0 in Measure and corrects in Arrange with a forced re-measure.

- **근거 — 프레임워크 레퍼런스**: Android ViewGroup.getChildMeasureSpec MATCH_PARENT -> EXACTLY(parentSize); single-pass measured size is final

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read every cited location in the worktree (real path: /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp; the finding's "view-impl.cpp:1000-1002/1028-1030/2444-2445/1148-1151" line numbers map exactly to this file).  The "observed" claim holds on every point: - view-impl.cpp:1000-1002 and 1012-1014: in the absolute/follower OnMeasure path, `mRequestedWidth == MATCH_PARENT -> size.width = mImpl->GetMinimumWidth()` (and height analog). Confirmed verbatim. - view-impl.cpp:1028-1030 and 1041-1043: in the natural-size OnMeasure path, MATCH_PARENT again returns `GetMinimumWidth()`/`GetMinimumHeight()`. Confirmed verbatim…

    - `intent`(intentional-design [보정:info]): All cited code is verified accurate against /home/jae/dali/dali-ui-claude. MATCH_PARENT returns GetMinimumWidth()/GetMinimumHeight() (default 0) in OnMeasure at view-impl.cpp:1000-1002 and 1028-1030 (file lives at dali-ui-foundation/public-api/view-impl.cpp, not internal/ as the prior auditor wrote, but same content). The LayoutManager dispatch path does the same: view-impl.cpp:2444-2445 (`else if(requestedWidth == MATCH_PARENT) resultVisW = GetMinimumWidth() * s;`). Arrange fills to parent content area and force re-measures the MATCH child at view-impl.cpp:1136-1151. This is NOT an accidental deviation — it is deliberately specified design: …

- **재현**: A WRAP parent with one fixed 100px child and one MATCH_PARENT child sizes to 100 (MATCH contributes 0), matching the doc example; but any consumer reading GetMeasuredSize() between Measure and Arrange sees ~0 for a MATCH view, which is surprising relative to Android where the measured size is already final.

- **▶ 수정 방향**: `view-impl.cpp:1000-1002` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


### Core Arrange 엔진 (`arrange-engine`) — 3건


<a id="fix-arrange-engine-arr-02"></a>

#### 🟡 MEDIUM · `arr-02` — No clip / RenderSize-vs-DesiredSize contract: arranging smaller than measured silently shrinks the child

- **위치**: `view-impl.cpp:653-657, 1090-1101, 2460-2479; no clip logic in Arrange; no overflow test in utc-Dali-ViewLayoutBoundary.cpp (its 'boundary' tests are invalidation boundaries, not visual clipping).`

- **현상(버그)**: OnArrange sets the actor SIZE_WIDTH/HEIGHT directly to the arranged bounds (view-impl.cpp:1100-1101; same in DispatchArrangeWithLayoutManager 2465-2466). GetSize() reads back that SIZE (653-657). There is no stored DesiredSize/RenderSize distinction and no clip step anywhere in Arrange; clipping happens only if the application independently set Actor CLIPPING_MODE (the only CLIP references are property pass-through at 200-204/2877).

- **근거 — 일반 프레임워크 기대동작**: WPF keeps DesiredSize and RenderSize separate and auto-clips when finalRect < DesiredSize. Android keeps measuredWidth/Height and clips children to the parent frame by default. A general framework defines overflow as 'clip' or at least 'keep desired size'. Here the child is simply resized to the smaller box (its own subtree re-lays into the smaller area), which is a third, surprising behavior and is undocumented.

- **근거 — 프레임워크 레퍼런스**: WPF UIElement.Arrange clipping when finalRect < DesiredSize; Android default child clipping to frame

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): All cited code verified at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp (note: the finding's path "internal/view-impl.cpp" is slightly off; the real path is public-api/view-impl.cpp, but the cited line numbers match exactly).  1) Arrange writes the box straight to the actor SIZE, no clip: - OnArrange (1090-1101): sets POSITION_X/Y and SIZE_WIDTH/HEIGHT to bounds.x/y/width/height directly. Confirmed verbatim. - DispatchArrangeWithLayoutManager (2460-2466): same direct SIZE write to visualBounds, then manager->Arrange into the padded content box (2477), returns {visualBounds.width, visualBounds.height} (2479). Confi…

    - `intent`(intentional-design [보정:info]): Code claims verified at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp (path differs from finding's bare "view-impl.cpp" but content matches): OnArrange writes the actor SIZE_WIDTH/HEIGHT directly to the arranged bounds (lines 1098-1101); DispatchArrangeWithLayoutManager does the same (2465-2466, plus 2487-2488); GetSize() reads that SIZE back (653-657). The only CLIPPING references are pass-through of the application-set Actor CLIPPING_MODE property: CreateClippingRenderer (197-210) merely installs a transparent background so DALi's existing actor-level CLIP_CHILDREN works, and OnPropertySet handles CLIPPING_MODE (…

- **재현**: A View measures 200x200 but its parent layout assigns it a 120x120 slot (e.g. constrained Grid cell with FILL). The child is resized to 120x120 and its contents re-flow/compress rather than rendering at 200x200 clipped to 120x120.

- **▶ 수정 방향**: `view-impl.cpp:653-657` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-arrange-engine-arr-04"></a>

#### 🟡 MEDIUM · `arr-04` — Stack main-axis MATCH_PARENT gives each child the FULL main extent -> multiple such children overflow/overlap

- **위치**: `stack-layout-manager.cpp:464-473,508,514-523,558; docs/layout-structure.md:151-154; only utc-Dali-StackLayout.cpp:289-290 covers MATCH_PARENT main-axis and it is a single child.`

- **현상(버그)**: For a main-axis MATCH_PARENT child, Stack sets the main size to the full available extent (stack-layout-manager.cpp:466 childHeight=availableHeight-marginH in VERTICAL; 516 in HORIZONTAL), then advances the cursor by that full slot (508 currentY += slotHeight+spacing; 558). With two main-axis MATCH_PARENT children, the first consumes the whole container and the second is placed past the container's bottom/right edge, both sized to the full extent.

- **근거 — 일반 프레임워크 기대동작**: Android vertical LinearLayout treats height=MATCH_PARENT children together with weights; the canonical resolution is weight-based sharing, not 'each gets 100%'. The docs only say 'use weight for proportional sharing' and never define the multi-child main-axis MATCH_PARENT case, which here produces silent overflow. Single-child main-axis MATCH_PARENT is the only case exercised in tests (utc-Dali-StackLayout.cpp:289-290).

- **근거 — 프레임워크 레퍼런스**: Android LinearLayout weight resolution for multiple MATCH_PARENT children on the main axis

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read the actual code (the file lives at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp, not the path in the finding, but the cited line numbers and code match exactly). The finding's mechanical claims all hold:  1. Arrange VERTICAL, main-axis MATCH_PARENT: stack-layout-manager.cpp:464-467 sets childHeight = std::max(0, availableHeight - marginH) (full container extent, where availableHeight = bounds.height per line 350). Line 468 slotHeight = childHeight + marginH (full slot). Line 508 currentY += slotHeight + visSpacing. HORIZONTAL mirror: lines 514-518 (childWidth = availableWidth - marginW…

    - `intent`(intentional-design [보정:info]): The mechanical observation is accurate (line numbers in the finding are shifted ~+2; the real file is dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp). I re-read it directly:  - VERTICAL: stack-layout-manager.cpp:464-468 — an unweighted MATCH_PARENT child gets childHeight = availableHeight - marginH (full main extent), slotHeight = childHeight + marginH. - stack-layout-manager.cpp:508 — currentY += slotHeight + visSpacing. - HORIZONTAL mirror: 514-518 and 558. So with two unweighted main-axis MATCH_PARENT children the first consumes the whole container and the second is placed off-edge, both at full extent. The repro is correct…

- **재현**: Vertical StackLayout height 400 with two children each RequestedHeight=MATCH_PARENT, no weight. Child0 arranged 0..400, child1 arranged 400..800 (off-container), both 400 tall.

- **▶ 수정 방향**: `stack-layout-manager.cpp:464-473` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-arrange-engine-arr-05"></a>

#### 🟢 LOW · `arr-05` — RTL mirroring uses full parent width, breaking with asymmetric start/end padding

- **위치**: `view-impl.cpp:1193-1213; padding applied as padLeft=start at 1106 and consumed in child origin at 1144.`

- **현상(버그)**: ApplyLayoutDirection mirrors each child via POSITION_X = parentWidth - oldX - childW (view-impl.cpp:1211), where oldX already includes left (start) padding and leading margin. Mirroring about the full parent width makes the post-mirror right gap equal to the original left inset. If padding.start != padding.end the result is wrong: a child inset by padding.start from the left ends up inset by padding.start from the right, instead of by padding.end.

- **근거 — 일반 프레임워크 기대동작**: RTL mirroring should reflect within the content box (account for start vs end padding) so that the 'start' edge maps to the physical right with the correct end padding. WPF/Android resolve start/end against the resolved layout direction before placement rather than mirroring the final LTR coordinate about the full width.

- **근거 — 프레임워크 레퍼런스**: Android RTL start/end padding resolution; WPF FlowDirection mirroring within content bounds

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read the cited code in the worktree (file is at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp, not the path the finding cited, but same content).  (1) The mirror formula is exactly as claimed: view-impl.cpp:1211 sets POSITION_X = parentWidth - oldX - childW, and the caller passes the FULL arrange width: ApplyLayoutDirection(bounds.width) at view-impl.cpp:1079 (bounds is the view's own arrange rect, so bounds.width is the full parent width, not a content box).  (2) oldX is an LTR position that includes START padding without any RTL swap, on BOTH arrange paths:   - Default OnArrange: visPadLeft = mPadding.start …

- **반론(소수의견)**: `intent`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: Container width 200, padding start=40 end=0, one fixed 50-wide child. LTR x=40 (40..90). After RTL mirror x=200-40-50=110 (110..160), leaving a 40px right gap though end padding is 0; the child should sit flush right (x=150).

- **▶ 수정 방향**: `view-impl.cpp:1193-1213` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


### Standalone 자식 (`standalone-children`) — 3건


<a id="fix-standalone-children-standalone-01"></a>

#### 🟡 MEDIUM · `standalone-01` — Standalone MATCH_PARENT child min/max clamp depends on which of two arrange paths runs

- **위치**: `view-impl.cpp:281-290 (ArrangeStandaloneChild, no clamp) vs layout-controller.cpp:485-486 (root path clamps). childImpl.Arrange does not reapply min/max (view-impl.cpp:1053-1088).`

- **현상(버그)**: A standalone child can be arranged by two different routines that disagree on min/max enforcement. Via the parent's pass, ArrangeStandaloneChild sets MATCH_PARENT size = parentFull - margin with NO clamp to the child's Minimum/Maximum (view-impl.cpp:281-290); the subsequent childImpl.Arrange just writes SIZE and never reapplies constraints. Via the self-root path, ProcessLayoutRoot computes the same fill size but then clamps bounds.width/height to view->GetMinimumWidth()/MaximumWidth (layout-controller.cpp:485-486).

- **근거 — 일반 프레임워크 기대동작**: A given child should resolve to the same final size regardless of internal scheduling. A general framework applies the element's min/max once, consistently, on whichever path produces the final arranged size.

- **근거 — 프레임워크 레퍼런스**: WPF FrameworkElement MinWidth/MaxWidth are applied in MeasureCore/ArrangeCore uniformly; CSS max-width always constrains the used width of an absolutely positioned box.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Re-read all cited code under /home/jae/dali/dali-ui-claude (note: files live at dali-ui-foundation/public-api/, not src/, but line numbers match).  Parent pass (no clamp): ArrangeStandaloneChild [view-impl.cpp:279-295] — for MATCH_PARENT it sets childW = max(0, parentFullWidth - marginW) (281), childH analogously (285), then builds LayoutRect bounds with width=childW/height=childH (292-294) and calls childImpl.Arrange(bounds) (295). No min/max clamp is applied to childW/childH. ArrangeStandaloneChildren [1180-1191] forwards the parent's bounds.width/height into this. The default in-flow path OnArrange [1136-1154] behaves identically (childW =…

    - `intent`(confirmed [보정:medium]): Re-verified the two arrange paths and they genuinely disagree on max/min enforcement for a MATCH_PARENT standalone child, and a standalone child reaches BOTH paths depending on what was invalidated.  Path A — parent's arrange pass: ArrangeStandaloneChildren (view-impl.cpp:1180-1191) calls ArrangeStandaloneChild (view-impl.cpp:268-296). For a MATCH_PARENT child it sets childW = std::max(0, parentFullWidth - marginW) (line 281) with NO clamp to the child's Minimum/Maximum, then childImpl.Measure(childW,childH) (289) and childImpl.Arrange(bounds) with bounds.width = childW (292-295). Arrange/OnArrange just writes SIZE_WIDTH = bounds.width (view-…

- **재현**: Standalone child with RequestedWidth=MATCH_PARENT and MaximumWidth=300 inside a 1000px parent. When re-arranged by the parent's pass (e.g. parent re-layout), width becomes 1000; when the same child is processed as its own dirty layout root (e.g. only the child was invalidated), width is clamped to 300. The rendered width flip-flops depending on what triggered the layout.

- **▶ 수정 방향**: `view-impl.cpp:281-290` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-standalone-children-standalone-03"></a>

#### 🟢 LOW · `standalone-03` — InvalidateArrange standalone boundary omits the transition-parent invalidation that InvalidateMeasure performs

- **위치**: `view-impl.cpp:1328-1336 (measure branch invalidates transition parent) vs view-impl.cpp:1371-1375 (arrange branch does not). Depth-sorted batch rationale at layout-controller.cpp:347-355.`

- **현상(버그)**: InvalidateMeasure's standalone branch, when the parent has a LayoutTransition, invalidates the parent so the transition dispatcher runs in the same batch (view-impl.cpp:1328-1336). InvalidateArrange's standalone branch self-registers but does NOT perform the parallel parent-transition invalidation (view-impl.cpp:1371-1375).

- **근거 — 일반 프레임워크 기대동작**: Either both boundary branches keep a transition-bearing parent in the same layout batch, or neither does. The asymmetry means an arrange-only change to a standalone child can dispatch its transition with a parent whose CaptureBeforeLayout pass did not run in the same batch.

- **근거 — 프레임워크 레퍼런스**: N/A (DALi-specific layout-transition batching); general principle: invalidation predicates for the two phases should be symmetric when the dependency they guard is symmetric.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read the cited code at /home/jae/dali/dali-ui-claude. The finding's structural claim holds exactly. InvalidateMeasure's standalone branch (view-impl.cpp:1328-1336) checks `GetParentView() && GetImpl(parentView).GetLayoutTransition()` and, if true, calls `GetImpl(parentView).InvalidateMeasure()` before self-registering — pulling a transition-bearing parent into the same batch. InvalidateArrange's standalone branch (view-impl.cpp:1371-1375) only does `RegisterWithLayoutController(); return;` with NO parallel parent-transition invalidation. The asymmetry is genuine.  The consequence the finding describes is also supported by the dispatcher …

    - `intent`(intentional-design [보정:info]): The factual asymmetry the finding cites is real and verified: InvalidateMeasure's standalone branch invalidates a transition-bearing parent (view-impl.cpp:1328-1336, parentView && GetImpl(parentView).GetLayoutTransition() -> GetImpl(parentView).InvalidateMeasure()), while InvalidateArrange's standalone branch only self-registers (view-impl.cpp:1371-1375). But the finding's conclusion — that this is an unintended asymmetry that can mis-time a CHANGE-slot transition — is wrong, because the dependency the two phases guard is itself asymmetric.\n\n1) Ownership/dispatch: A standalone child's CHANGE transition is owned and dispatched by the PARENT,…

- **재현**: Standalone child under a transition-bearing parent; only child.InvalidateArrange() is called (position change, no remeasure). The parent is not pulled into the batch, so its CaptureBeforeLayout/StartTransitionsAfterLayout may not co-run, potentially mis-timing the child's CHANGE-slot transition.

- **▶ 수정 방향**: `view-impl.cpp:1328-1336` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-standalone-children-standalone-04"></a>

#### ⚪ INFO · `standalone-04` — Standalone children are not re-measured on a parent Measure cache hit (relies on self-root path)

- **위치**: `view-impl.cpp:908-912 (early return) precedes view-impl.cpp:940 (MeasureStandaloneChildren); view-impl.cpp:1053-1073 (Arrange always runs ArrangeStandaloneChildren).`

- **현상(버그)**: ViewImpl::Measure early-returns the cached MeasuredSize when the constraint is unchanged (view-impl.cpp:908-912), returning BEFORE the MeasureStandaloneChildren(effVisW,effVisH) call at view-impl.cpp:940. ViewImpl::Arrange has no such early return, so ArrangeStandaloneChildren always runs (view-impl.cpp:1073).

- **근거 — 일반 프레임워크 기대동작**: A reader of ViewImpl::Measure might assume standalone children are always measured when the parent is measured; they are not on a cache hit. Correctness is preserved only because a dirtied standalone self-registers as its own layout root (view-impl.cpp:1335) and is measured by ProcessLayoutRoot.

- **근거 — 프레임워크 레퍼런스**: WPF measure invalidation walks each dirty element independently via the LayoutManager queue; DALi mirrors this only through the standalone self-root, not through the parent.

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:info]): All cited structural facts verified in /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp: - Measure early-return on unchanged constraint at view-impl.cpp:908-912 returns mImpl->mMeasuredSize BEFORE MeasureStandaloneChildren(effVisW, effVisH) at :940 (confirmed exactly). - Arrange (:1053-1073) has no cache gate; ArrangeStandaloneChildren(bounds) at :1073 always runs (confirmed). - Standalone self-root: InvalidateMeasure explicitly treats a standalone view as a layout boundary, stops upward propagation, and calls RegisterWithLayoutController() at :1335 (block :1328-1337). InvalidateArrange mirrors this at :1371-1375. So …

    - `code`(confirmed [보정:info]): All four cited code facts hold exactly as the finding states; I re-read every cited line under /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/.  1. Early return precedes MeasureStandaloneChildren: view-impl.cpp:908-912 returns mImpl->mMeasuredSize when mLastMeasuredConstraint matches effNatW/effNatH (cache hit), BEFORE the MeasureStandaloneChildren(effVisW, effVisH) call at view-impl.cpp:940. Confirmed — on a parent Measure cache hit, MeasureStandaloneChildren is never reached. (The finding cites the line as 940; in this worktree it is exactly 940.)  2. Arrange has no early return: ViewImpl::Arrange (view-impl.cpp:1053-1088) unco…

- **재현**: Parent constraint unchanged, standalone WRAP_CONTENT child's content changes. Parent Measure is a cache hit and skips the child; the child is re-measured only via its self-root registration. The two mechanisms are coupled — removing the boundary self-registration would silently leave standalone children un-measured on cache hits.

- **▶ 수정 방향**: `view-impl.cpp:908-912` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


### 무효화 & LayoutController (`invalidation-controller`) — 6건


<a id="fix-invalidation-controller-inval-01"></a>

#### 🟠 HIGH · `inval-01` — Entire LayoutController UTC suite is commented out — zero coverage for invalidation/scheduling

- **위치**: `/home/jae/dali/dali-ui-claude/automated-tests/src/dali-ui-foundation/utc-Dali-LayoutController.cpp:37-92 (all bodies commented)`

- **현상(버그)**: utc-Dali-LayoutController.cpp contains only startup/cleanup stubs; every test body (Get, OnWindowResize, ProcessLayouts, same-window-returns-same-instance) is commented out (lines 37-92). No test exercises RequestLayout dedup, mPendingViews draining, re-entrant invalidation, multi-window controllers, destroy-before-Process, or the depth sort.

- **근거 — 일반 프레임워크 기대동작**: A scheduling/invalidation subsystem this intricate (depth sort, weak-handle revalidation, re-entrancy, window-replacement) should have executable regression tests, as Android/WPF layout-pass logic does.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:high]): I read the cited file in full. /home/jae/dali/dali-ui-claude/automated-tests/src/dali-ui-foundation/utc-Dali-LayoutController.cpp contains only the startup/cleanup stubs (lines 27-35) and four entirely commented-out test bodies: UtcDaliLayoutControllerGetP (37-49), UtcDaliLayoutControllerOnWindowResizeP (51-63), UtcDaliLayoutControllerProcessLayoutsP (65-77), and UtcDaliLayoutControllerGetSameWindowReturnsSameInstanceP (79-92). Every `//`-prefixed line confirms none compile into runnable tests. The file IS in the build (CMakeLists.txt:43) so it produces zero executable LayoutController coverage rather than being absent.  The intricate schedul…

    - `code2`(confirmed [보정:medium]): Factual core verified true. Read /home/jae/dali/dali-ui-claude/automated-tests/src/dali-ui-foundation/utc-Dali-LayoutController.cpp:27-93 — only utc_dali_layoutcontroller_startup/cleanup are live; all 4 test bodies (UtcDaliLayoutControllerGetP, OnWindowResizeP, ProcessLayoutsP, GetSameWindowReturnsSameInstanceP) are fully commented (lines 37-92). git log shows commit 9e8bfa42 "Add dummy adaptor to test-suites and enable some tcs" (2026-04-03) edited line 39 inside this block yet deliberately left it commented, so the dead suite survived a pass that enabled other TCs — a real defect.  The implementation it should cover is genuinely intricate (…

    - `intent`(confirmed [보정:medium]): VERIFIED the cited file /home/jae/dali/dali-ui-claude/automated-tests/src/dali-ui-foundation/utc-Dali-LayoutController.cpp:1-93. The file contains only the startup/cleanup stubs (lines 27-35); all four test functions — UtcDaliLayoutControllerGetP, UtcDaliLayoutControllerOnWindowResizeP, UtcDaliLayoutControllerProcessLayoutsP, UtcDaliLayoutControllerGetSameWindowReturnsSameInstanceP — are fully commented out (lines 37-92). The finding's factual claim is accurate.  The subsystem genuinely IS intricate, matching the finding's characterization: dali-ui-foundation/public-api/layouts/layout-controller.cpp is 619 lines and implements depth-sort of p…

- **재현**: Any regression in ProcessLayouts ordering, dedup, or the destroy-before-Process guard ships undetected; the only indirect coverage is via ViewLayoutBoundary/LayoutTransition tests.

- **▶ 수정 방향**: `/home/jae/dali/dali-ui-claude/automated-tests/src/dali-ui-foundation/utc-Dali-LayoutController.cpp:37-92` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-invalidation-controller-inval-04"></a>

#### 🟡 MEDIUM · `inval-04` — OnWindowResize mutates mAllLayoutRoots while iterating it (potential iterator invalidation)

- **위치**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-controller.cpp:246-251 (iterate+invalidate), :126 (insert during RequestLayout); /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:1328-1336 (standalone invalidates transition-bearing parent)`

- **현상(버그)**: OnWindowResize iterates mAllLayoutRoots with a range-for and calls pair.second.view->InvalidateMeasure() inside the loop (layout-controller.cpp:246-251). For a plain root this re-assigns the SAME existing key (operator[] at 126, no rehash — safe). But a STANDALONE root whose parent carries a LayoutTransition triggers GetImpl(parentView).InvalidateMeasure() (view-impl.cpp:1333), which can register the PARENT's layout root — a NEW key — via mAllLayoutRoots[parentRoot]=... (layout-controller.cpp:126) mid-iteration, which can rehash and invalidate the active iterator (UB).

- **근거 — 일반 프레임워크 기대동작**: Mutating an unordered_map during range iteration is unsafe when a new key may be inserted. A snapshot of keys (as already done for dead entries via deadEntries) should be used before invalidating.

- **근거 — 검증자 코드분석**:

    - `intent`(confirmed [보정:low]): VERIFIED CODE FACTS. (1) mAllLayoutRoots is std::unordered_map<ViewImpl*, LayoutRootEntry> (layout-controller.cpp:502), so inserting a NEW key via operator[] (line 126) can rehash and, per the C++ standard, invalidate all iterators; re-assigning an existing key does not. (2) OnWindowResize range-iterates mAllLayoutRoots (line 246) and calls pair.second.view->InvalidateMeasure() inside the loop (line 250). (3) The deadEntries snapshot (lines 244-261; git blame shows it was added 2026-04-02 by Jiyun Yang) defends only against erase-during-iteration, NOT insert. (4) The standalone+transition path is real: view-impl.cpp:1328-1336 — for a standalo…

- **반론(소수의견)**: `code`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: Window with a standalone layout-root child whose parent has a LayoutTransition, and the parent is not yet a tracked root. Resize -> iterate roots -> standalone's InvalidateMeasure invalidates+registers the parent root -> insert into mAllLayoutRoots during iteration -> rehash -> dangling iterator.

- **▶ 수정 방향**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-controller.cpp:246-251` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-invalidation-controller-inval-05"></a>

#### 🟢 LOW · `inval-05` — mProcessingScheduled is dead control state — set/cleared but never gates anything

- **위치**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-controller.cpp:132-135 (set), :340 (clear), :507 (member); grep shows no read sites`

- **현상(버그)**: mProcessingScheduled is set true in RequestLayout (132-135), reset false in ProcessLayouts (340), but is never read to decide whether to schedule/skip work. The controller is a per-frame Processor that runs Process() every frame regardless; the empty-mPendingViews early-return (325) is the only real gate.

- **근거 — 일반 프레임워크 기대동작**: Android/WPF use a 'layout requested' flag to schedule a one-shot traversal and avoid redundant scheduling. Here the flag mimics that pattern but has no effect, implying either a missing optimization (don't run when nothing pending — already handled by the empty check) or leftover scaffolding.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read layout-controller.cpp at all five mProcessingScheduled sites and traced the control/data flow.  Sites (verified via grep + Read): init false at :83; `if(!mProcessingScheduled){ mProcessingScheduled = true; }` at :132-135 inside RequestLayout; `mProcessingScheduled = false;` at :340 inside ProcessLayouts; member decl at :507.  The finding's substantive thesis is CONFIRMED: the flag never gates whether layout work runs. Proof of the real scheduling model: - :93 `Adaptor::Get().RegisterProcessor(*this, false)` — the LayoutController registers a PERSISTENT processor (not RegisterProcessorOnce). So Process() (:276) is invoked every frame…

- **재현**: Refactoring confusion: a reader assumes processing is gated by this flag; it is not. No functional bug, but misleading state.

- **▶ 수정 방향**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-controller.cpp:132-135` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-invalidation-controller-inval-06"></a>

#### 🟢 LOW · `inval-06` — Depth-sort recomputes ancestor depth by full parent walk per comparison

- **위치**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-controller.cpp:358-374`

- **현상(버그)**: ProcessLayouts sorts pending roots by tree depth using a depthOf lambda that walks v->Self().GetParent() to the top on EVERY comparison (layout-controller.cpp:358-374). std::sort calls the comparator O(n log n) times, each O(depth), so total O(n log n * depth) actor-parent traversals per frame.

- **근거 — 일반 프레임워크 기대동작**: Compute each root's depth once into a vector of (depth, view) pairs, then sort. Re-walking the actor parent chain inside the comparator is wasteful and unusual.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): Re-read /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-controller.cpp:356-374. The code matches the finding exactly. Line 356 builds `std::vector<ViewImpl*> viewsToProcess` (plain pointers, NOT (depth,view) pairs). Lines 358-369 define `depthOf`, which for each call walks the actor parent chain `for(Dali::Actor p = v->Self().GetParent(); p; p = p.GetParent()) ++d;` — O(depth) per invocation. Lines 370-374 call `std::sort` with a comparator `depthOf(a) < depthOf(b)` that invokes `depthOf` on both operands every comparison. There is no caching/memoization of depth anywhere in the surrounding scope (verified the whole…

    - `intent`(confirmed [보정:low]): Re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-controller.cpp:356-375. The finding is accurate: `depthOf` (lines 358-369) walks `v->Self().GetParent()` to the root counting depth, and `std::sort` (lines 370-374) invokes it twice per comparison via the lambda `depthOf(a) < depthOf(b)`. Since std::sort calls its comparator O(n log n) times and each call does two full parent-chain walks of cost O(depth), the per-frame cost is O(n log n * depth) actor-parent traversals — the depth is NOT memoized. This is a genuine, unintended micro-inefficiency.  The ORDERING itself is intentional and well-do…

- **재현**: Many simultaneously-dirty deep roots (e.g. window resize invalidating all roots) — comparator repeats the parent walk for each pair instead of once per root.

- **▶ 수정 방향**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-controller.cpp:358-374` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-invalidation-controller-inval-07"></a>

#### 🟢 LOW · `inval-07` — ReplaceCurrentWindow reconnects ResizeSignal without disconnecting the prior connection

- **위치**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-controller.cpp:96 (initial Connect), :220-227 (assert same ptr, reconnect, no Disconnect)`

- **현상(버그)**: ReplaceCurrentWindow calls window.ResizeSignal().Connect(this, &OnWindowResized) (layout-controller.cpp:226) a second time (the constructor already connected at :96) with no preceding Disconnect. It asserts the same window object ptr (220), so this re-binds to the same underlying window/signal.

- **근거 — 일반 프레임워크 기대동작**: Either the connection is the same signal+slot and the second Connect is redundant (dead code) — or, if DALi does not dedupe identical connections, OnWindowResized fires twice per resize, double-invalidating all roots. No Disconnect is present either way.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): All "observed" code-level claims hold as written under /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-controller.cpp.  (1) Constructor connects the resize signal at line 96: `window.ResizeSignal().Connect(this, &LayoutControllerImpl::OnWindowResized)` (the slot is `OnWindowResized`, defined at line 494; line 233's `OnWindowResize` is a different forwarding helper that the slot calls — the finding's "OnWindowResized" name is correct).  (2) ReplaceCurrentWindow (lines 218-228) connects the SAME signal+slot again at line 226 with NO preceding Disconnect. A grep over the file for "Disconnect" returns zero hits, confirm…

- **반론(소수의견)**: `intent`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: A window handle is replaced (Get() detects GetCurrentWindow()!=window at :522 and calls ReplaceCurrentWindow). Subsequent resize may invoke OnWindowResized once or twice depending on signal dedup semantics; the double-invalidate is harmless functionally but redundant.

- **▶ 수정 방향**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-controller.cpp:96` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-invalidation-controller-inval-08"></a>

#### ⚪ INFO · `inval-08` — Dirty signal fused into the constraint cache sentinel instead of a discrete dirty flag (deviation from Android/WPF)

- **위치**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:113-124 (three-state encoding), :1276-1285 (NaN to bypass guard), :908 (>=0 cache test conflates clean vs dirty)`

- **현상(버그)**: There is no per-view 'measureDirty' boolean. Dirtiness is encoded as mLastMeasuredConstraint = -1.0f (MEASURE_CACHE_DIRTY), overloaded with the legitimate constraint value, and distinguished from 'never measured' (NaN) only by IEEE-754 inequality (view-impl.cpp:113-124, 1305, 908). ResetEffectiveScaleRecursive must use NaN specifically to dodge the DIRTY guard (1276-1285).

- **근거 — 일반 프레임워크 기대동작**: Android (PFLAG_FORCE_LAYOUT) and WPF (MeasureDirty flag + dirty queue) keep the dirty bit orthogonal to the cached MeasureSpec/availableSize, so the dirty decision and the cache-equality decision never interfere. Overloading -1 as both 'invalid constraint' and 'dirty' makes the guard logic subtle and forces NaN gymnastics elsewhere.

- **근거 — 프레임워크 레퍼런스**: Android View.PFLAG_FORCE_LAYOUT vs MeasureSpec cache; WPF UIElement.MeasureDirty + ContextLayoutManager MeasureQueue.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:info]): I re-read every cited location and the finding's "observed" claims hold exactly.  1. No discrete measureDirty boolean. view-data-impl.h:585-593 declares mLastMeasuredConstraint (a MeasuredSize) and mArrangeDirty (a bool). The only dirty boolean is mArrangeDirty, and it tracks ARRANGE, not measure (comment at h:589-592: "true = invalidated since the last arrange"). Measure dirtiness has no boolean — it is carried by the constraint value itself.  2. Three-state overloading is real and self-documented. view-impl.cpp:113-124 spells out the trichotomy: NaN = initial/never-measured, MEASURE_CACHE_DIRTY (-1.0f) = invalidated, positive = last constra…

- **재현**: Design comprehension/maintenance hazard rather than a runtime bug: any new caller that writes the cache or invalidates must understand the NaN-vs-DIRTY-vs-positive trichotomy and the guard interaction, which is easy to get wrong (the ResetEffectiveScaleRecursive comment is essentially a warning about this).

- **▶ 수정 방향**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:113-124` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


### StackLayout (`stack-layout`) — 6건


<a id="fix-stack-layout-stack-01"></a>

#### 🟠 HIGH · `stack-01` — Non-weight MATCH_PARENT child on the MAIN axis fills the whole axis and overlaps siblings (no main-axis fill arbitration)

- **위치**: `stack-layout-manager.cpp:464-467 and 514-517 set childMain to availableMain-margin unconditionally for MATCH_PARENT; Measure contribution is 0 via view-impl.cpp:1012/1043.`

- **현상(버그)**: In Measure a non-weight child with main-axis RequestedHeight/Width==MATCH_PARENT returns minimum size (0) and contributes ~0 to mainAxisNonWeight (stack-layout-manager.cpp:94-105 + view-impl.cpp:1012-1014). In Arrange the placement loop overrides its main size to the FULL available main axis: childHeight = availableHeight-marginH (stack-layout-manager.cpp:464-467; horizontal 514-517), then advances currentY by that full slot. With one fixed sibling + one MATCH_PARENT child the MATCH_PARENT child consumes the whole main axis and the following/other children are pushed off the end or overlap; with TWO main-axis MATCH_PARENT children both receive the full axis and overlap completely.

- **근거 — 일반 프레임워크 기대동작**: In Android LinearLayout / CSS flex you cannot have multiple children fill the main axis without weight/flex-grow; a main-axis 'fill' is resolved by distributing remaining space (single MATCH_PARENT gets leftover, multiple share it). Stack should either treat main-axis MATCH_PARENT as weight=1 (share leftover) or clamp it to remaining space, not give every such child the full axis.

- **근거 — 프레임워크 레퍼런스**: Android LinearLayout main-axis sizing (match_parent + weight); CSS Flexbox 9.7 resolving flexible lengths

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read the cited code in the worktree and the core mechanism holds.  MEASURE contribution ~0: For a main-axis MATCH_PARENT child, ViewImpl::Measure returns GetMinimumHeight()/GetMinimumWidth() (default 0) at view-impl.cpp:1012-1014 (vertical) and 1028-1030 (horizontal). The stack first pass calls childImpl.Measure() at stack-layout-manager.cpp:94 and accumulates childSize.height into mainAxisNonWeight (line 98), so a non-weight MATCH_PARENT child contributes ~marginH only (effectively 0). Confirmed.  ARRANGE full-axis override: stack-layout-manager.cpp:464-467 unconditionally sets childHeight = max(0, availableHeight - marginH) when GetReq…

    - `intent`(intentional-design [보정:low]): I re-read all cited code at /home/jae/dali/dali-ui-claude.  MECHANISM CONFIRMED (the observed facts are accurate): - Measure: a non-weight MATCH_PARENT child returns minimum size (default 0) from its own Measure — view-impl.cpp:1000-1003/1012-1014 (layout-children branch) and 1028-1031/1041-1044 (leaf branch): `size.width/height = GetMinimumWidth()/GetMinimumHeight()`. So in the stack first pass it contributes only `0 + marginH` to mainAxisNonWeight (stack-layout-manager.cpp:96-105, line 98/103). - Arrange: a non-weight MATCH_PARENT main-axis child has its main size overridden to the FULL available main axis — vertical stack-layout-manager.cp…

    - `code2`(intentional-design [보정:low]): I re-derived the behavior from the cited code at /home/jae/dali/dali-ui-claude.  MEASURE side (confirmed): A leaf MATCH_PARENT child returns its minimum size. view-impl.cpp:1012-1014 and 1041-1043 set size.height = mImpl->GetMinimumHeight(); GetMinimumHeight() defaults to 0 (view-data-impl.h:636-639, returns 0 when mSizeConstraints is null). ApplyConstraints (view-impl.cpp:931) clamps the result to >= min, so GetMeasuredSize().height = 0 for a no-min MATCH_PARENT child. In stack-layout-manager.cpp:98 (vertical) / :103 (horizontal) this 0 is what gets added to result.mainAxisNonWeight. So the child contributes ~0 to the parent's main-axis accu…

- **재현**: Vertical StackLayout 200x120, child A fixed height 40, child B RequestedHeight=MATCH_PARENT (no weight). B is arranged at height 120 starting at y=0..40 region; A at y=0 h40, B at y=40 h120 → B overflows to 160 (>120). Two MATCH_PARENT children both arrange at full 120 and overlap.

- **▶ 수정 방향**: `stack-layout-manager.cpp:464-467` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-stack-layout-stack-02"></a>

#### 🟠 HIGH · `stack-02` — Arrange weight leftover ignores main-axis MATCH_PARENT non-weight children (computes leftover as if they take 0)

- **위치**: `stack-layout-manager.cpp:382-388 (nonWeightMain from allocations), 395 (remainingMain), 464-467/514-517 (MATCH_PARENT child later takes full axis).`

- **현상(버그)**: In the Arrange weight redistribution, nonWeightMain sums allocations[i].main for weight==0 children (stack-layout-manager.cpp:382-388). allocations were seeded from GetMeasuredSize (358), which for a main-axis MATCH_PARENT non-weight child is the minimum (0). So remainingMain (395) treats that child as occupying 0, giving weight children a share of nearly the whole axis — yet the same MATCH_PARENT child is then expanded to the full axis at placement (464/514). The weight child and the MATCH_PARENT child then both claim almost the entire main axis.

- **근거 — 일반 프레임워크 기대동작**: Leftover for weight distribution must subtract the space actually consumed by non-weight fillers. A consistent algorithm would resolve all fixed/fill sizes first, then distribute the true remainder to weighted children.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox 9.7 (resolve inflexible items before distributing free space); Android LinearLayout measure-then-distribute

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:high]): I re-read the cited code and verified every load-bearing claim against the actual source at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp and view-impl.cpp.  DATA FLOW (Arrange weight redistribution, stack-layout-manager.cpp): - Line 358: allocations[i] is seeded from GetImpl(children[i]).GetMeasuredSize() for every child. - For a non-weight child (weight==0), lines 382-388 take the else branch: mainSize = allocations[i].width (horizontal) / .height (vertical), and nonWeightMain += mainSize + margin. The weight loop (lines 397-441) only overwrites allocations[i] for weight>0 children (433/438), s…

    - `intent`(confirmed [보정:medium]): I re-read the cited code and verified every mechanical claim:  1. allocations seeded from each child's GetMeasuredSize (stack-layout-manager.cpp:356-359). For a main-axis MATCH_PARENT non-weight child, ViewImpl::Measure returns GetMinimumWidth()*s / GetMinimumHeight()*s — i.e. 0 by default (view-impl.cpp:2444-2445, 2452-2453). So its seeded main-axis allocation is 0.  2. The Arrange weight pass sums nonWeightMain from allocations[i].main for weight==0 children (stack-layout-manager.cpp:382-388), so a main-axis MATCH_PARENT filler contributes 0 to nonWeightMain.  3. remainingMain = availableMain - nonWeightMain - spacingTotal (395) therefore t…

    - `code2`(intentional-design [보정:low]): The finding's mechanical claims are VERIFIED against the actual source, but its "high-severity bug" framing is REFUTED; the behavior is the documented consequence of combining two mechanisms the design presents as mutually-exclusive alternatives.  Verified facts: 1. Arrange seeds allocations from GetMeasuredSize (stack-layout-manager.cpp:358), and nonWeightMain sums allocations[i].main + marginM for weight==0 children (lines 382-388). remainingMain = max(0, availableMain - nonWeightMain - spacingTotal) (line 395). CONFIRMED. 2. A main-axis MATCH_PARENT child's measured size IS its minimum (default 0): view-impl.cpp:2444-2445 (DispatchMeasureW…

- **재현**: Horizontal stack width 300: child A RequestedWidth=MATCH_PARENT no weight; child B weight=1. nonWeightMain≈0 → B gets ≈300; A also expanded to ≈300 → both overlap across the full width.

- **▶ 수정 방향**: `stack-layout-manager.cpp:382-388` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-stack-layout-stack-05"></a>

#### 🟡 MEDIUM · `stack-05` — Weight child ignores its requested main-axis size entirely (deviates from Android layout_weight / flex-grow base size)

- **위치**: `stack-layout-manager.cpp:83-87 (skip measuring weight child for base), 147/152 and 434/438 (allocation = share only). Documented at stack-layout.h:50-54.`

- **현상(버그)**: A weight>0 child is never measured for its main-axis base in pass 1 (stack-layout-manager.cpp:83-87 skip) and its main allocation is set purely to share-margin (147/152, 434/438). Its RequestedWidth/Height on the main axis has no effect on size.

- **근거 — 일반 프레임워크 기대동작**: Android layout_weight and CSS flex-grow add the proportional share ON TOP of the child's base (measured/flex-basis) size; the requested size is a floor, not ignored. Setting flex-grow does not discard width.

- **근거 — 프레임워크 레퍼런스**: Android LinearLayout layout_weight (excess on top of measured size); CSS flex-grow over flex-basis

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read every cited line in /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp and the cited line numbers match exactly.  1) Pass-1 skip (stack-layout-manager.cpp:82-87): in MeasureStackNonWeightChildren, a child with weight>0 does `result.totalWeight += weight; continue;` BEFORE any Measure call. Its main-axis base is never measured in pass 1, and its RequestedWidth/Height is never read here. Confirmed exactly as claimed.  2) Weight allocation = share only (stack-layout-manager.cpp:133, 147, 152): MeasureStackWeightChildren computes `share = (weight/totalWeight)*remainingMain`. The weight child IS …

    - `intent`(intentional-design [보정:low]): MECHANICAL CLAIMS — VERIFIED. (1) Weight children are skipped in pass 1's base measurement: stack-layout-manager.cpp:83-87 — `if(weight > 0.0f){ result.totalWeight += weight; continue; }` exits before the child's Measure() at line 94, so the child's main-axis base size is never measured for the wrap total. (2) Weight allocation is share-only: line 147 `workingSizes[i].height = std::max(0.0f, share - marginH)` (vertical) and line 152/438 `width = std::max(0.0f, share - marginW)` (horizontal) — the main-axis allocation is set purely from `share = (weight/totalWeight)*remainingMain` (line 133/410), with no `max(base, share)` floor. The requested…

- **재현**: Child with RequestedHeight=200 and weight=1 in a 100px-leftover vertical stack ends up 100px, not max(200, base+share). Surprising to LinearLayout/flex users though documented.

- **▶ 수정 방향**: `stack-layout-manager.cpp:83-87` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-stack-layout-stack-07"></a>

#### 🟡 MEDIUM · `stack-07` — Missing test coverage for multiple weight children, weight+rounding, and multiple main-axis MATCH_PARENT collision

- **위치**: `utc-Dali-StackLayout.cpp:325-348 is the only weighted size assertion; grep shows 6 SetWeight calls total, none placing >1 weight child in one stack with exact-size asserts.`

- **현상(버그)**: utc-Dali-StackLayout.cpp has exactly one weight-layout test asserting actual size (UtcDaliStackLayoutChildWithWeightP, 325-348) and it only checks height>=50 (a weak bound). No test arranges two+ weight children and asserts their pixel shares, no test mixes a fixed child with multiple weights to verify leftover math, and no test exercises two main-axis MATCH_PARENT children (the stack-01/stack-02 collision is unguarded).

- **근거 — 일반 프레임워크 기대동작**: A linear-layout suite should pin exact weighted shares (e.g. 1:2 → 1/3,2/3 of leftover), leftover after fixed children + spacing, and multi-filler behavior, so regressions in the Measure/Arrange double-distribution are caught.

- **근거 — 프레임워크 레퍼런스**: n/a (coverage)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I read the cited file /home/jae/dali/dali-ui-claude/automated-tests/src/dali-ui-foundation/utc-Dali-StackLayout.cpp and the impl /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp. The finding's observed claims hold:  1. "Only one weight test asserting actual size, checking height>=50": CONFIRMED. UtcDaliStackLayoutChildWithWeightP (325-348) is the only test asserting a weighted child's post-Arrange size, and line 346 uses DALI_TEST_CHECK(weightChild.GetSize().height >= 50.0f) — a weak lower bound. The geometry (120 height - 40 fixed - 5 spacing) yields an exact leftover of 75.0f, so the test could pi…

    - `intent`(confirmed [보정:low]): Re-read the cited code at /home/jae/dali/dali-ui-claude. The finding's COVERAGE GAP is real, though its claim "exactly one weight-layout test" is imprecise.  Tests (utc-Dali-StackLayout.cpp): - UtcDaliStackLayoutChildWithWeightP (325-348) is the ONLY test that Arranges a weighted child. Its only size assertions on the weight child are width==200 (cross-axis fill, line 345) and `height >= 50.0f` (line 346) — a weak lower bound, NOT an exact share. With layout height 120, fixed child 40, spacing 5, the weighted child should arrive at exactly 75px; the test never pins this. - UtcDaliStackLayoutWeightMultipleP (210-223) DOES add two weight childr…

- **재현**: A regression that changes remainingMain math (stack-01/02) would pass the entire current suite because no test asserts a weighted child's exact main size against a fixed sibling.

- **▶ 수정 방향**: `utc-Dali-StackLayout.cpp:325-348` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-stack-layout-stack-08"></a>

#### 🟢 LOW · `stack-08` — Cross-axis CENTER/END do not clamp when child exceeds available cross size (negative offset, child can spill before the start edge)

- **위치**: `stack-layout-manager.cpp:487 and 490 (vertical), 537 and 540 (horizontal) — no max(0,...) on the offset.`

- **현상(버그)**: For CENTER, crossX += (crossAvailable - childWidth)*0.5f (stack-layout-manager.cpp:487); for END, crossX += crossAvailable - childWidth (490). If childWidth>crossAvailable (e.g. a fixed child wider than the cross axis) the offset is negative and the child is positioned before bounds.x / above bounds.y (it spills past the start edge rather than being clamped to 0).

- **근거 — 일반 프레임워크 기대동작**: This actually matches typical gravity math (overflow centers/anchors symmetrically), so it is conventional — but combined with no cross-axis clamp it means an oversized child silently overhangs the container start, which some frameworks clamp.

- **근거 — 프레임워크 레퍼런스**: Android gravity CENTER/END overflow behavior

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): Re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp (note: file lives under public-api/layouts/, not the internal/ path the finding implied, but the line numbers match exactly). The cross-axis offset math is verified at the exact cited lines: - VERTICAL CENTER: stack-layout-manager.cpp:487 `crossX += (crossAvailable - childWidth) * 0.5f;` - VERTICAL END: stack-layout-manager.cpp:490 `crossX += crossAvailable - childWidth;` - HORIZONTAL CENTER: stack-layout-manager.cpp:537 `crossY += (crossAvailable - childHeight) * 0.5f;` - HORIZONTAL END: stack-layout-manager.cpp:540 `crossY …

    - `intent`(intentional-design [보정:info]): Code verified: stack-layout-manager.cpp:487 (CENTER vertical) `crossX += (crossAvailable - childWidth) * 0.5f`, :490 (END vertical) `crossX += crossAvailable - childWidth`, and the horizontal mirror at :537/:540. There is no `std::max(0.0f, ...)` guarding the offset, so a child wider than `crossAvailable` yields a negative offset and the child starts before bounds.x/bounds.y. The cited line numbers are accurate, and the repro math (200-cross, 80→120 for END; an oversized 160 child would give -60) is correct in principle.  However the finding's own framing refutes its "unusual-design" label. Its "expected" text concedes: "This actually matches…

- **재현**: Vertical stack cross width 100, child fixed width 160, alignment END → crossX = currentX + (100-160) = currentX-60, child starts 60px before the container's left content edge.

- **▶ 수정 방향**: `stack-layout-manager.cpp:487` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-stack-layout-stack-04"></a>

#### ⚪ INFO · `stack-04` — Trailing spacing is added after the last child in Arrange (asymmetric with Measure's (N-1) model)

- **위치**: `stack-layout-manager.cpp:508 and 558 add visSpacing every iteration; Measure uses (visibleChildCount-1) at 116/241/311.`

- **현상(버그)**: The placement loop unconditionally does currentY += slotHeight + visSpacing (stack-layout-manager.cpp:508) / currentX += slotWidth + visSpacing (558) for every child including the last, so a phantom trailing gap is added to the cursor. Measure correctly uses (visibleChildCount-1)*spacing (e.g. 116, 241, 311).

- **근거 — 일반 프레임워크 기대동작**: Spacing is applied only BETWEEN children = (N-1)*spacing, never after the last (Android divider SHOW_DIVIDER_MIDDLE, CSS gap).

- **근거 — 프레임워크 레퍼런스**: CSS gap / Android LinearLayout dividers ( (N-1) gaps )

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:info]): Re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp (note: file is under public-api/layouts/, not internal/layout/, but it is the only stack-layout-manager.cpp in the tree).  Arrange/placement loop runs `for(i; i<children.size(); ++i)` (line 445) with a `continue` for standalone children (449-452). Cursors `currentX`/`currentY` are seeded from `bounds.x`/`bounds.y` (351-352). At the end of each visible iteration the loop unconditionally does `currentY += slotHeight + visSpacing;` (508, vertical branch) and `currentX += slotWidth + visSpacing;` (558, horizontal branch). There i…

- **반론(소수의견)**: `intent`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: Harmless for positions (nothing follows the last child) but means the manager internally consumes sum+N*spacing; any future code reading the final cursor as 'content extent' would over-count by one spacing.

- **▶ 수정 방향**: `stack-layout-manager.cpp:508` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


### FlexLayout (`flex-layout`) — 9건


<a id="fix-flex-layout-flex-01"></a>

#### 🟠 HIGH · `flex-01` — flex-basis: 0 is ignored; falls back to content size, breaking the canonical `flex: 1` idiom

- **위치**: `flex-layout-manager.cpp:94 `if(basis > 0.0f)`; flex-layout-manager.cpp:491 `if(basis > 0.0f)`; default basis is WRAP_CONTENT(-1) per flex-layout-params-impl.h:51`

- **현상(버그)**: Both the Measure path (flex-layout-manager.cpp:491) and the Arrange line-builder (flex-layout-manager.cpp:94-95) only apply flex-basis when `basis > 0.0f`. A flexBasis of exactly 0 (and the default WRAP_CONTENT/-1) therefore uses the child's measured content size as the base, not 0.

- **근거 — 일반 프레임워크 기대동작**: CSS treats flex-basis:0 as a definite zero base size; `flex: 1 1 0` is the standard way to make children share space equally regardless of content. Here, two children with grow=1 and basis=0 but different content sizes do NOT end up equal-width: they get (content + equal share of free space), not (equal share of total).

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox 7.2.3 flex-basis (definite 0), 9.2 'flex base size', 9.7 resolve flexible lengths

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:high]): I re-read the cited code under /home/jae/dali/dali-ui-claude and the finding holds exactly.  Control flow (all verified, no doc reliance): - flex-layout-manager.cpp:59 — GetFlexBasis returns params->GetFlexBasis() or WRAP_CONTENT if no params. - flex-layout-params-impl.h:51 — default mFlexBasis(WRAP_CONTENT). layout-types.h:65 — WRAP_CONTENT = -1.0f. So the default basis is -1, and any explicit basis of exactly 0 are both <= 0. - Measure path: flex-layout-manager.cpp:489-501 — workingSize seeded from childSize (measured content, line 489); basis overrides it ONLY inside `if(basis > 0.0f)` (line 491). basis==0 / -1 leaves content size as the m…

    - `intent`(confirmed [보정:medium]): Re-verified against actual source (paths differ from the finding's citation but the code matches). The flex manager lives at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp, not the cited integration-api path.  CORE MECHANISM CONFIRMED: - Measure path: flex-layout-manager.cpp:490-501 reads basis and applies it only `if(basis > 0.0f)` (line 491). Otherwise workingSize stays the measured content size (line 489 `MeasuredSize workingSize = childSize;`). - Arrange line-builder: BuildFlexLinesForArrange applies basis only `if(basis > 0.0f)` (line 95); otherwise workingSizes[i] retains the seeded measured …

    - `code2`(confirmed [보정:medium]): Verified the cited code in /home/jae/dali/dali-ui-claude (actual path is dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp; the finding's path prefix was wrong but the line numbers, code, and behavior match exactly).  BRANCH GUARD (both confirmed): - flex-layout-manager.cpp:95 `if(basis > 0.0f)` in BuildFlexLinesForArrange (Arrange path). Only when basis>0 does it overwrite workingSizes[i].width/height with basis*childScale (lines 97-104). Otherwise the working size stays seeded from the child's measured content size (Arrange seeds workingSizes from GetMeasuredSize() at line 576). - flex-layout-manager.cpp:491 `if(basis > 0.0f)` i…

- **재현**: Row flex, container 300px. Child A content 50px, child B content 150px, both FlexLayoutParams grow=1 basis=0. Expected (CSS): each 150px. Actual: A=50+50=100, B=150+50=200.

- **▶ 수정 방향**: `flex-layout-manager.cpp:94` 에서 위 '기대동작'을 만족하도록 수정. (분류: spec-violation)


<a id="fix-flex-layout-flex-02"></a>

#### 🟠 HIGH · `flex-02` — No iterative min/max freeze-and-redistribute loop (CSS 9.7) — single pass can violate min sizes and leave space mis-filled

- **위치**: `flex-layout-manager.cpp:133-191 (whole function is non-iterative); grow branch has no max clamp (147-156); shrink floors per-item at 0 only (181,185) with no leftover redistribution`

- **현상(버그)**: ApplyFlexGrowShrink distributes free space in exactly one pass. Grow adds (grow/total)*freeSpace with no upper (max-size) clamp (flex-layout-manager.cpp:147-156). Shrink reduces each item and only floors the individual result at 0 (flex-layout-manager.cpp:181,185); when an item hits 0 (or its min) the un-absorbed overflow is NOT redistributed to the remaining flexible items.

- **근거 — 일반 프레임워크 기대동작**: CSS 9.7 'Resolve Flexible Lengths' is an iterative loop: compute distribution, clamp each item to its min/max, freeze the clamped (min/max-violating) items, subtract their frozen size, and repeat with the remaining free space until none are unfrozen. This guarantees the line is exactly filled while honoring every item's min and max main size.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox 9.7 'Resolving Flexible Lengths' (loop: c..e freeze min/max violations)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read the cited code in full at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp.  The finding's MECHANICAL claims about the code are all accurate: - ApplyFlexGrowShrink (lines 133-191) is a single, non-iterative function. There is no while/do-while loop over freeze-and-redistribute; it is one `if (grow) ... else if (shrink) ...` block executed exactly once per line, called once per line from Arrange (lines 582-585). Confirmed. - Grow branch (lines 145-156): `float extra = (grow / line.totalFlexGrow) * freeSpace;` added directly to width/height with NO upper/max clamp. Confirmed — no max-main-siz…

    - `intent`(confirmed [보정:medium]): I re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp.  CODE FACTS (verified): - ApplyFlexGrowShrink is non-iterative: the grow branch (lines 139-159) is a single for-loop, the shrink branch (160-190) is a single for-loop. There is no freeze/repeat outer loop (whole function 133-191). CONFIRMED. - Grow distribution (line 147): `extra = (grow/line.totalFlexGrow)*freeSpace` is added unconditionally at lines 150/154 with NO upper (max-size) clamp. CONFIRMED. - Shrink (lines 181/185): `workingSizes[idx].width = std::max(0.0f, workingSizes[idx].width - reduction)` floors the result …

    - `code2`(confirmed [보정:medium]): All four mechanical claims verify against the actual source at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp.  (1) Single-pass / non-iterative: ApplyFlexGrowShrink (lines 133-191) runs the grow branch (139-159) OR the shrink branch (160-190) exactly once over line.childIndices. There is no outer while-loop, no per-item freeze set, no recomputation. Confirmed.  (2) Grow has no max clamp: lines 147-156 compute extra=(grow/totalFlexGrow)*freeSpace and do workingSizes[idx].width/height += extra with no std::min against GetMaximumWidth/Height. A child's max main size is ignored during grow. Confirmed. …

- **재현**: Row, container 100px. A base=60 shrink=1, B base=60 shrink=1, B has a min size of 50. Single pass shrinks both by 10 -> A=50,B=50; if min(B)=55 the code floors B at its computed value not its min, so B can be driven below min, and the 5px deficit is silently lost / the line overflows.

- **▶ 수정 방향**: `flex-layout-manager.cpp:133-191` 에서 위 '기대동작'을 만족하도록 수정. (분류: spec-violation)


<a id="fix-flex-layout-flex-03"></a>

#### 🟡 MEDIUM · `flex-03` — align-items/align-self STRETCH overrides a child's explicit cross size (CSS stretches only auto)

- **위치**: `flex-layout-manager.cpp:299-300 (STRETCH unconditional); default STRETCH at flex-layout-impl.cpp:65; no GetRequestedWidth/Height==explicit guard in the STRETCH branch`

- **현상(버그)**: In ArrangeOneFlexLine the STRETCH case unconditionally sets childCrossSize = line.crossSize - marginCross (flex-layout-manager.cpp:299-300) without checking whether the child has an explicit RequestedWidth/Height on the cross axis. Default alignItems for FlexLayout is STRETCH (flex-layout-impl.cpp:65).

- **근거 — 일반 프레임워크 기대동작**: In CSS, `align-items: stretch` only stretches items whose used cross-axis size is auto; an item with an explicit height (in a row) keeps that height. Here a child given an explicit cross size is forcibly resized to the line's cross extent under the default alignment.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox 8.3 'align-items: stretch' + 9.4 cross size determination (auto-only stretch)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read the cited code at /home/jae/dali/dali-ui-claude (note: file lives at dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp, not the path implied by the finding, but the cited line numbers match exactly).  1. STRETCH unconditional override CONFIRMED. In ArrangeOneFlexLine, the cross-size switch at flex-layout-manager.cpp:288-304 has `case FlexAlign::STRETCH: childCrossSize = line.crossSize - marginCross;` at lines 299-300. There is NO guard checking GetRequestedWidth/GetRequestedHeight for an explicit value. The only cross-size guards in the function are for MATCH_PARENT (line 265-270) — an explicit pixel cross size (e.g. Req…

    - `intent`(confirmed [보정:medium]): Code observation verified. In ArrangeOneFlexLine, the STRETCH branch at flex-layout-manager.cpp:299-301 unconditionally executes `childCrossSize = line.crossSize - marginCross;` with no guard for an explicit cross-axis request. The only cross-size override that IS conditional is the MATCH_PARENT case at lines 265-270 (`crossIsMatchParent` -> set to line.crossSize), but STRETCH runs afterward in the switch (280-304) and overwrites childCrossSize regardless of whether the child set an explicit RequestedWidth/Height. So a child with e.g. SetRequestedHeight(30) in a ROW under STRETCH is arranged at the line cross extent, not 30.  Default alignIte…

- **재현**: Row, line cross (height) = 80. Child with SetRequestedHeight(30) and default align-self AUTO -> inherits STRETCH -> arranged at height 80 instead of CSS's 30.

- **▶ 수정 방향**: `flex-layout-manager.cpp:299-300` 에서 위 '기대동작'을 만족하도록 수정. (분류: spec-violation)


<a id="fix-flex-layout-flex-04"></a>

#### 🟡 MEDIUM · `flex-04` — Main-axis MATCH_PARENT takes full available main, overlapping siblings and ignoring flex distribution

- **위치**: `flex-layout-manager.cpp:272-278; doc claim docs/layout-structure.md:157 ('Main-axis MATCH_PARENT fills the available main axis')`

- **현상(버그)**: When a child's main-axis requested size is MATCH_PARENT, ArrangeOneFlexLine sets childMainSize = availMain - marginMain (flex-layout-manager.cpp:272-278), i.e. the full content main extent, independent of other children, free space, or grow factors. Multiple MATCH_PARENT main children all get the full main extent and overlap; a MATCH_PARENT child next to fixed children overflows/overlaps them.

- **근거 — 일반 프레임워크 기대동작**: A general flow layout has no item that unconditionally claims the entire main axis while siblings are present; the equivalent intent (fill remaining space) is flex-grow. CSS has no MATCH_PARENT. The follower contract in the docs says MATCH_PARENT main 'fills the available main axis; use flex-grow for proportional distribution', but filling full main while siblings exist produces overlap, not a fill of the remainder.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox flex-grow (remainder distribution); Android weight; no canonical 'fill full main while siblings present'

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I traced the full Arrange data flow and the cited code does exactly what the finding claims.  CONFIRMED FACTS: - ArrangeOneFlexLine, flex-layout-manager.cpp:272-278: for a main-axis MATCH_PARENT child, `availMain = isMainAxisHorizontal ? contentWidth : contentHeight` and `childMainSize = max(0, availMain - marginMain)`. This overrides whatever was in workingSizes[idx]. - contentWidth/contentHeight are set to the FULL container content extent (bounds.width/bounds.height) at flex-layout-manager.cpp:568-569, NOT remaining free space and NOT a per-child slice. They are passed unchanged into ArrangeOneFlexLine at :645. - The main offset still adva…

    - `intent`(intentional-design [보정:medium]): I re-read the actual code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp (note: the file is under public-api/layouts/, NOT internal/layout/ as the finding cited; line numbers 272-278 do match). The observed mechanic is exactly as claimed and is fully traceable:  1. Arrange seeds workingSizes[i] from GetMeasuredSize() (line 576). Per docs (layout-structure.md:132-135) a MATCH_PARENT view's Measure returns its minimum size (~0). 2. BuildFlexLinesForArrange accumulates line.mainSize from that minSize (line 119), so the MATCH_PARENT child contributes ~0 to the line, and freeSpace = availableMain - l…

- **재현**: Row, 200px container. Child A fixed 50px at x=0..50. Child B MATCH_PARENT -> arranged main = 200, placed after A so its rect spans 50..250, overflowing the container and any later child.

- **▶ 수정 방향**: `flex-layout-manager.cpp:272-278` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-flex-layout-flex-05"></a>

#### 🟡 MEDIUM · `flex-05` — Flex factors resolved in Arrange only; Measure reports un-flexed sum (WRAP_CONTENT container size wrong)

- **위치**: `Measure has no grow/shrink (flex-layout-manager.cpp:472-548); duplicate line-break logic at 508-509 vs BuildFlexLinesForArrange 111-117`

- **현상(버그)**: Measure computes container size purely from base/working sizes and never calls ApplyFlexGrowShrink (flex-layout-manager.cpp:472-548). Grow/shrink are applied only in Arrange from a separately re-seeded workingSizes buffer (flex-layout-manager.cpp:574-585).

- **근거 — 일반 프레임워크 기대동작**: Splitting measure/arrange is acceptable in a follower model, but it means a WRAP_CONTENT flex container measures to the sum of un-grown / un-shrunk base sizes, and on overflow the Measure-reported main size exceeds the container (shrink not yet applied). Line-breaking in Measure (508) and Arrange (BuildFlexLinesForArrange) is duplicated and could diverge if one path is edited without the other.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox 9.x (flex resolution is part of sizing); WPF Measure/Arrange consistency

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): The finding cites a non-existent path (internal/layouting/flex-layout-manager.cpp) and wrong line numbers, but the technical substance holds against the real file /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp.  (1) Measure never resolves grow/shrink: FlexLayoutManager::Measure [flex-layout-manager.cpp:450-549] accumulates currentLine.mainSize from un-flexed workingSize (basis is substituted at 489-501, but no grow/shrink), and returns totalMainSize/totalCrossSize as the plain sum [527-548]. It does NOT call ApplyFlexGrowShrink, and structurally does not even accumulate totalFlexGrow/totalFlexShrin…

    - `intent`(intentional-design [보정:low]): CODE-LEVEL CLAIMS VERIFIED (paths differ from the finding: the file is at dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp, NOT .../internal/layouting/...).  1) Measure has no grow/shrink: confirmed. FlexLayoutManager::Measure (flex-layout-manager.cpp:450-549) builds lines from childSize/basis only (487-501), accumulates line.mainSize from un-flexed childMainSize (503,518), and returns totalMainSize = max(line.mainSize) (527-548). ApplyFlexGrowShrink is never called in Measure. 2) Grow/shrink applied only in Arrange from a re-seeded buffer: confirmed. Arrange seeds workingSizes from each child's GetMeasuredSize() (573-577), build…

- **재현**: WRAP_CONTENT row container with children base totalling 300 but parent later constrains to 200 with shrink: Measure returns width 300; only Arrange shrinks. Any consumer reading the measured size sees the pre-shrink 300.

- **▶ 수정 방향**: `flex-layout-manager.cpp:472-548` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-flex-layout-flex-06"></a>

#### 🟡 MEDIUM · `flex-06` — ROW_REVERSE/COLUMN_REVERSE combined with RTL double-reverses (untested interaction)

- **위치**: `Manager reversal flex-layout-manager.cpp:311-341,640-643; blanket mirror view-impl.cpp:1200-1212; tests: ROW+RTL only (utc-Dali-FlexLayout.cpp:636-669), ROW_REVERSE without RTL (465-488)`

- **현상(버그)**: The manager applies main-axis reversal for ROW_REVERSE/COLUMN_REVERSE internally (flex-layout-manager.cpp:311-341, 640-643). Independently, ViewImpl::ApplyLayoutDirection mirrors ALL non-standalone direct children when effective layout direction is RIGHT_TO_LEFT (view-impl.cpp:1195-1212), unconditionally on flex-direction. ROW_REVERSE + RTL therefore reverses once in the manager and mirrors again afterward.

- **근거 — 일반 프레임워크 기대동작**: CSS resolves the main-axis start/end from BOTH flex-direction and the writing-mode/direction in a single coherent model; row-reverse in an RTL context yields left-to-right visual order. The DALi composition (manager reversal then a blanket post-mirror) happens to compose, but is two independent mechanisms with no shared invariant, and the ROW_REVERSE+RTL (and COLUMN_REVERSE) combinations are not covered by any UTC (utc only tests ROW+RTL at utc-Dali-FlexLayout.cpp:636 and ROW_REVERSE+LTR at 465).

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox 5.1 (flex-direction resolves against writing-mode/direction)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read every cited location and the surrounding control flow; the finding's reading of the code is accurate.  (1) Manager-internal main-axis reversal for ROW_REVERSE/COLUMN_REVERSE: confirmed. FlexLayoutManager::IsMainAxisReversed() returns true for ROW_REVERSE || COLUMN_REVERSE (flex-layout-manager.cpp:444-448). It drives the mainOffset recomputation in the arrange loop (flex-layout-manager.cpp:640-646: mainOffset = contentMain - justify.mainOffset) and the per-child reversed placement branch (flex-layout-manager.cpp:311-341).  (2) Blanket post-mirror independent of flex-direction: confirmed. ViewImpl::ApplyLayoutDirection (view-impl.cpp:…

    - `intent`(intentional-design [보정:low]): I re-read all cited code in /home/jae/dali/dali-ui-claude.  FACTS VERIFIED: - The manager's main-axis reversal depends ONLY on flex-direction: FlexLayoutManager::IsMainAxisReversed() returns ROW_REVERSE||COLUMN_REVERSE (flex-layout-manager.cpp:444-448), consumed at the reverse branches (flex-layout-manager.cpp:311-316, 328-333). No LayoutDirection/RTL input. Confirmed. - ViewImpl::ApplyLayoutDirection (view-impl.cpp:1193-1212) early-returns unless GetEffectiveLayoutDirection()==RIGHT_TO_LEFT, then mirrors x for EVERY non-standalone direct child (x = parentWidth - oldX - childW), unconditional on flex-direction. Confirmed: the two mechanisms a…

- **재현**: FlexLayout ROW_REVERSE with LayoutDirection RIGHT_TO_LEFT, 3 children: manager lays them right-anchored reversed, then ApplyLayoutDirection mirrors every child's x, net effect equivalent to LTR ROW — easy to get wrong and currently unverified.

- **▶ 수정 방향**: `flex-layout-manager.cpp:311-341` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-flex-layout-flex-09"></a>

#### 🟡 MEDIUM · `flex-09` — Grow/shrink test coverage asserts only container measured size, never child sizes/positions after distribution

- **위치**: `utc-Dali-FlexLayout.cpp:190-266 (params getters only), 401-462 (justify/align assert only container size), no child-size assertion after grow/shrink anywhere in the 671-line file`

- **현상(버그)**: The flex-grow/shrink/basis UTCs only assert the params getters (utc-Dali-FlexLayout.cpp:190-266) and at most the container measured size; no test arranges children with grow!=0 / shrink!=0 / explicit basis and asserts resulting child GetSize()/GetPositionX(). justify/align variant tests (401-462) call Arrange but assert only container size, not child placement.

- **근거 — 일반 프레임워크 기대동작**: The core flex distribution math (proportional grow, weighted shrink, basis override, min/max behavior, wrap leftover) should have UTCs asserting concrete child sizes/positions. Without them, findings flex-01/02/03/04/07 are unguarded by the suite, so a regression in the distribution math would not be caught.

- **근거 — 프레임워크 레퍼런스**: N/A (test-coverage gap)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read the entire 671-line utc-Dali-FlexLayout.cpp and the implementation in flex-layout-manager.cpp. The finding's three concrete claims all hold:  1. "Grow/shrink/basis UTCs only assert params getters (190-266)." Verified: grep shows SetFlexGrow/SetFlexShrink/SetFlexBasis appear ONLY at lines 196, 218, 242, 254 (utc-Dali-FlexLayout.cpp). Each enclosing test (UtcDaliFlexLayoutSetFlexGrowP 190-199, SetFlexShrinkP 212-221, NegativeFlexFactorClamped 234-246, SetFlexBasisP 248-257, plus the Get* mirrors) asserts ONLY GetFlexGrow()/GetFlexShrink()/GetFlexBasis() (e.g. line 197, 219, 255). An awk+grep over NR 190-268 for "Arrange|GetSize|GetPos…

    - `intent`(confirmed [보정:medium]): I read the entire 671-line utc-Dali-FlexLayout.cpp and grep-confirmed every grow/shrink/basis and child-size reference.  CLAIM A (params getters only, 190-266): VERIFIED. UtcDaliFlexLayoutSetFlexGrowP/GetFlexGrowP (190-210), SetFlexShrinkP/GetFlexShrinkP (212-232), NegativeFlexFactorClampedP (234-246), SetFlexBasisP/GetFlexBasisP (248-268) all only assert child.GetLayoutParams<FlexLayoutParams>().GetFlexGrow()/Shrink()/Basis() — i.e. the stored param value. None call Measure/Arrange or inspect child GetSize()/GetPosition.  CLAIM B (justify/align tests assert only container size, 401-462): VERIFIED. UtcDaliFlexLayoutJustifyContentVariantsP (40…

- **재현**: A change that breaks proportional grow distribution still passes the entire FlexLayout UTC suite because no test inspects post-grow child widths.

- **▶ 수정 방향**: `utc-Dali-FlexLayout.cpp:190-266` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-flex-layout-flex-07"></a>

#### 🟢 LOW · `flex-07` — Grow distributed proportionally to grow factor but with no remainder/rounding correction (subpixel gaps)

- **위치**: `flex-layout-manager.cpp:147 (float share), 314-341 (float positions, no round)`

- **현상(버그)**: Free space is split as (grow/totalGrow)*freeSpace per item in float with no accumulation of rounding error and no final adjustment; positions and sizes are written as raw floats throughout Arrange (e.g. flex-layout-manager.cpp:147, 314-341). No rounding to device pixels.

- **근거 — 일반 프레임워크 기대동작**: CSS layout engines round used sizes/positions to device pixels and carry the rounding remainder so distributed tracks sum exactly to the container with no 1px seams. Here cumulative float error can leave a thin gap or overlap at line end, and fractional positions are passed straight to the actor.

- **근거 — 프레임워크 레퍼런스**: CSS layout pixel snapping / Chromium LayoutNG fraction accumulation

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read every cited line and the downstream commit path; the finding's "observed" reading is accurate.  1) Float share with no remainder tracking — flex-layout-manager.cpp:147 computes `float extra = (grow / line.totalFlexGrow) * freeSpace;` inside ApplyFlexGrowShrink (133-191). Each grow child independently gets a float proportion of freeSpace; there is no accumulator for rounding error and no final corrective pass on the last item. After distribution it simply sets `line.mainSize = availableMain` (158) as if the floats summed exactly, but float division (e.g. 1/3*100) does not.  2) Raw float positions/sizes in Arrange — ArrangeOneFlexLine…

- **반론(소수의견)**: `intent`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: Row 100px, 3 grow=1 children base 0: each gets 33.333..; last child ends at 99.999.. leaving a sub-pixel gap at the trailing edge.

- **▶ 수정 방향**: `flex-layout-manager.cpp:147` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-flex-layout-flex-10"></a>

#### 🟢 LOW · `flex-10` — NO_WRAP single line is force-stretched to full cross size, silently disabling align-content and overriding the line's natural cross extent

- **위치**: `flex-layout-manager.cpp:594-597; align-content NO_WRAP guard 608`

- **현상(버그)**: When wrap==NO_WRAP and exactly one line, line[0].crossSize is forced to max(crossSize, availableCross) (flex-layout-manager.cpp:594-597). align-content is then skipped for NO_WRAP (guarded at 608). align-self FLEX_END/CENTER offsets use this inflated line.crossSize as the alignment box.

- **근거 — 일반 프레임워크 기대동작**: This is acceptable (single-line flex container's line fills the cross size, so cross alignment has room to work), and matches the intent that align-items operates over the full cross size. But combined with flex-03 (stretch overrides explicit size) it means by default every child in a NO_WRAP row is resized to the full container height regardless of its requested height, which will surprise developers porting from CSS.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox 9.4 (single-line cross size) + align-items default

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:info]): I re-read every cited location and the full data/control flow; all four links of the finding's chain hold:  1. NO_WRAP single-line cross inflation: flex-layout-manager.cpp:594-597 — `if(impl->mWrap == FlexWrap::NO_WRAP && lines.size() == 1) { lines[0].crossSize = std::max(lines[0].crossSize, availableCross); }`. Confirmed verbatim.  2. align-content skipped for NO_WRAP: line 608 — `if(impl->mWrap != FlexWrap::NO_WRAP && !lines.empty())` guards the entire alignContent switch (610-632). Confirmed.  3. align-self FLEX_END/CENTER use the inflated line.crossSize as the alignment box: in ArrangeOneFlexLine, line 287 `crossSpace = line.crossSize - c…

    - `intent`(intentional-design [보정:info]): Re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp (note: file lives under public-api/layouts/, not internal/). Verified facts:  1. Lines 594-597: `if(impl->mWrap == FlexWrap::NO_WRAP && lines.size() == 1) { lines[0].crossSize = std::max(lines[0].crossSize, availableCross); }` — exactly as cited. 2. Line 608: `if(impl->mWrap != FlexWrap::NO_WRAP && !lines.empty())` guards align-content, so align-content is skipped for NO_WRAP — exactly as cited. 3. align-self FLEX_END/CENTER (lines 293-298) and the alignment box (crossSpace = line.crossSize - childCrossSize - marginCross, line…

- **재현**: NO_WRAP row, container height 100, child requested height 30, default align STRETCH -> line cross forced to 100, child stretched to 100 (see flex-03).

- **▶ 수정 방향**: `flex-layout-manager.cpp:594-597` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


### GridLayout (`grid-layout`) — 7건


<a id="fix-grid-layout-grid-01"></a>

#### 🟠 HIGH · `grid-01` — Spanning cells contribute nothing to AUTO track sizing

- **위치**: `grid-layout-manager.cpp:120-127 (the `rowSpan == 1` / `colSpan == 1` guards)`

- **현상(버그)**: In MeasureGridChildrenAndFillAuto, an AUTO (or implicit) row/column is grown to fit a child only when that child's span on that axis equals 1: `if(rowSpan == 1 && rowAutoLike) rowHeights[row]=max(...)` and the analogous column guard. Any child with rowSpan>1 or colSpan>1 contributes zero to every auto track it covers.

- **근거 — 일반 프레임워크 기대동작**: WPF Grid and CSS Grid distribute a spanning item's desired size across the auto/min-content tracks it spans (after accounting for tracks already sized), so a column-spanning auto cell still forces its auto tracks to be large enough to contain it.

- **근거 — 프레임워크 레퍼런스**: WPF Grid star/auto measure (spanned cell distribution); CSS Grid Layout §11.5 'Resolve Intrinsic Track Sizes' — distributing spanning items across spanned tracks

- **근거 — 검증자 코드분석**:

    - `intent`(confirmed [보정:high]): I re-read the cited code at grid-layout-manager.cpp:81-129. The guards are verbatim as claimed: line 120 `if(rowSpan == 1 && rowAutoLike)` then line 122 `rowHeights[row] = std::max(rowHeights[row], childSize.height + marginH);`, and line 124 `if(colSpan == 1 && colAutoLike)` then line 126 `colWidths[col] = std::max(colWidths[col], childSize.width + marginW);`. rowAutoLike/colAutoLike (lines 118-119) cover both explicit AUTO tracks and the implicit (no-definition) track. Because the writes are gated on span==1, any child with rowSpan>1 or colSpan>1 contributes ZERO to every auto/implicit track it covers. I confirmed there is no compensating sp…

    - `code`(confirmed [보정:high]): I re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp.  The guards exist verbatim: line 120 `if(rowSpan == 1 && rowAutoLike) { rowHeights[row] = std::max(rowHeights[row], childSize.height + marginH); }` and line 124 `if(colSpan == 1 && colAutoLike) { colWidths[col] = std::max(colWidths[col], childSize.width + marginW); }`. `MeasureGridChildrenAndFillAuto` (lines 81-128) is the ONLY place where a child's measured content contributes a floor to AUTO/implicit tracks. A child with rowSpan>1 or colSpan>1 skips both branches entirely, so it contributes zero to every auto track it cov…

    - `code2`(confirmed [보정:high]): I re-derived the algorithm from source at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp (the finding's cited path "grid-layout-manager.cpp:120-127" resolves to this public-api file; the line numbers 120-127 match exactly).  CITED GUARDS VERIFIED (grid-layout-manager.cpp:120-127): The auto/implicit track growth is gated by span==1: - L120-123: `if(rowSpan == 1 && rowAutoLike) rowHeights[row] = std::max(rowHeights[row], childSize.height + marginH);` - L124-127: `if(colSpan == 1 && colAutoLike) colWidths[col] = std::max(colWidths[col], childSize.width + marginW);` `rowAutoLike`/`colAutoLike` (L118-11…

- **재현**: Grid with two AUTO columns and one child at col0 colSpan=2 whose natural width is 300px. Both auto columns stay at 0 (plus any star fill), the grid under-measures to ~0 content width, and the child is squashed/overflowed instead of forcing the two columns to total 300.

- **▶ 수정 방향**: `grid-layout-manager.cpp:120-127` 에서 위 '기대동작'을 만족하도록 수정. (분류: spec-violation)


<a id="fix-grid-layout-grid-02"></a>

#### 🟡 MEDIUM · `grid-02` — Star/fr distribution has no per-track min/max and no redistribution

- **위치**: `grid-layout-manager.cpp:199-228 (single-pass star split, no clamp); layout-types.h:530-532 (GridLength has only mType, mValue)`

- **현상(버그)**: STAR tracks are sized in a single proportional pass: colWidths[i] = (starValue/totalStarWidth)*remainingWidth, with no lower bound from the cell's content and no Min/Max clamping or surplus redistribution. GridLength itself stores only {type,value} with no min/max field.

- **근거 — 일반 프레임워크 기대동작**: WPF allocates star space then clamps each star track to MinWidth/MaxWidth and redistributes the leftover among the remaining unconstrained stars (iterative). CSS minmax()/fr does the equivalent. DALi cannot express or honor any track min/max.

- **근거 — 프레임워크 레퍼런스**: WPF Grid star allocation with MinWidth/MaxWidth constraint redistribution

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Verified all cited code. grid-layout-manager.cpp:204 (explicit STAR columns) and :219 (explicit STAR rows) do `colWidths[i] = (starValue/totalStarWidth)*remainingWidth` / `rowHeights[i] = (starValue/totalStarHeight)*remainingHeight` — unconditional assignment, no std::max content floor, no per-track Min/Max clamp, and no iterative surplus redistribution. remainingWidth/Height is computed once at :196-197 with no redistribution loop. layout-types.h:530-532 confirms GridLength stores only mType and mValue; the public factories (Absolute/Star/Auto, :479-512) expose no min/max, so a track min/max cannot be expressed at all — matching the finding'…

    - `intent`(intentional-design [보정:info]): Re-read all cited code under /home/jae/dali/dali-ui-claude. The citations are accurate: grid-layout-manager.cpp:199-205 sizes explicit STAR columns as colWidths[i] = (starValue/totalStarWidth)*remainingWidth with NO content-min clamp and NO redistribution (rows identical at 216-219); layout-types.h:530-532 confirms GridLength stores only {mType, mValue}. The single-pass star split with no per-track min/max is REAL.  However the framing as a spec-violation/defect is wrong for this lens:  1) No min/max API exists to honor. GridLength's entire public surface is Absolute/Star/Auto factories (layout-types.h:442-456 GridLengthType; 461-533 the clas…

- **재현**: Two Star(1) columns where the left cell's content min-width is 150 but total grid width is 100 → each star gets 50 and the left content overflows/clips; WPF would honor a MinWidth and shrink the other star.

- **▶ 수정 방향**: `grid-layout-manager.cpp:199-228` 에서 위 '기대동작'을 만족하도록 수정. (분류: spec-violation)


<a id="fix-grid-layout-grid-03"></a>

#### 🟡 MEDIUM · `grid-03` — No auto-placement: every unparameterized child stacks at cell (0,0)

- **위치**: `grid-layout-manager.cpp:42-50 (defaults 0/0); no flow logic anywhere in Measure/Arrange (422-546)`

- **현상(버그)**: GetChildRow/GetChildColumn default to 0 when no GridLayoutParams trait is attached, and there is no placement cursor. Children without explicit row/col all land in row 0 / col 0 and overlap, with z-order being arbitrary actor order and no detection.

- **근거 — 일반 프레임워크 기대동작**: CSS Grid auto-places items that lack explicit positions into successive cells following the grid-auto-flow cursor (RTL-aware). WPF requires explicit Grid.Row/Column but also defaults to (0,0). At minimum a general grid offers opt-in auto-flow; here it is absent and undocumented.

- **근거 — 프레임워크 레퍼런스**: CSS Grid Layout §8 Placing Grid Items / grid-auto-flow

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:info]): The finding's code observations are factually accurate but the "deviation" framing is wrong because DALi's GridLayout is a WPF-model grid, not a CSS Grid, and WPF's documented behavior is exactly this.  Verified facts: - grid-layout-manager.cpp:40-50: GetChildRow/GetChildColumn return params->GetRow()/GetColumn() or 0 when no GridLayoutParamsImpl trait is attached. Confirmed. - grid-layout-params-impl.h:49-52: defaults are mRow(0), mColumn(0), mRowSpan(1), mColumnSpan(1). So a child with no params (or with params but no SetRow/SetColumn) resolves to cell (0,0). Confirmed. - No auto-flow/placement-cursor logic exists anywhere: grep for auto-fl…

    - `code`(confirmed [보정:low]): I re-read the cited code and surrounding context. The finding's factual claims hold exactly.  1. Defaults to (0,0): grid-layout-manager.cpp:40-50 — GetChildRow/GetChildColumn return `params ? params->GetRow()/GetColumn() : 0` when no GridLayoutParamsImpl trait is attached. Even when params ARE attached, the impl default constructor sets mRow(0), mColumn(0) (grid-layout-params-impl.h:48-49). So every unparameterized child resolves to cell (0,0).  2. No placement cursor / auto-flow anywhere: A scoped grep over grid-layout-manager.cpp, grid-layout-params-impl.h, and grid-layout-params.cpp for auto-flow|placement|cursor|next-free|occupancy return…

- **재현**: Add 4 children to a 2x2 grid without setting GridLayoutParams → all 4 overlap in the top-left cell instead of filling the four cells.

- **▶ 수정 방향**: `grid-layout-manager.cpp:42-50` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-grid-layout-grid-04"></a>

#### 🟡 MEDIUM · `grid-04` — Out-of-range row/column and over-long span clamp into the last track (no implicit tracks)

- **위치**: `grid-layout-manager.cpp:101-104 (measure clamp) and 265-268 (arrange clamp)`

- **현상(버그)**: row=min(row,rowCount-1), col=min(col,colCount-1), span=min(span, count-pos). A child placed at column 9 in a 3-column grid is silently moved into column 2; a span exceeding the remaining columns is truncated to fit. No implicit tracks are created and no warning is emitted.

- **근거 — 일반 프레임워크 기대동작**: CSS Grid creates implicit tracks for positions/spans beyond the explicit grid (the grid grows). WPF treats out-of-range Grid.Row/Column as the last line/clamps similarly, but DALi's silent clamp combined with no implicit-track support means content collapses into existing cells with no diagnostic.

- **근거 — 프레임워크 레퍼런스**: CSS Grid implicit tracks; WPF Grid line clamping

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp.  Measure clamp (lines 101-104) reads exactly as claimed:   row = std::min(row, rowCount - 1);   col = std::min(col, colCount - 1);   rowSpan = std::min(rowSpan, rowCount - row);   colSpan = std::min(colSpan, colCount - col);  Arrange clamp (lines 265-268) is identical:   uint32_t row = std::min(GetChildRow(childImpl), rowCount - 1);   uint32_t col = std::min(GetChildColumn(childImpl), colCount - 1);   uint32_t rowSpan = std::min(GetChildRowSpan(childImpl), rowCount - row);   uint32_t colSpan = std::min(GetChildColumnSpan(ch…

    - `intent`(intentional-design [보정:info]): I re-read the cited code. The clamp logic exists exactly as described, but at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp (NOT the internal/ path the finding cited): measure clamp at lines 101-104 (row=std::min(row,rowCount-1); col=std::min(col,colCount-1); rowSpan=std::min(rowSpan,rowCount-row); colSpan=std::min(colSpan,colCount-col)) and the identical arrange clamp at lines 265-268. So the mechanical observation is accurate.  WHY intentional-design rather than confirmed-bug: (1) The finding's OWN "expected" section concedes that WPF clamps out-of-range Grid.Row/Grid.Column to the last line — w…

- **재현**: 3x3 grid, child at row=5 col=5 colSpan=3 → ends up in cell (2,2) with span 1, silently mis-placed rather than extending the grid.

- **▶ 수정 방향**: `grid-layout-manager.cpp:101-104` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-grid-layout-grid-06"></a>

#### 🟢 LOW · `grid-06` — WRAP_CONTENT grid collapses STAR tracks to ~0 with no content floor

- **위치**: `grid-layout-manager.cpp:461-475 (width branch) and 477-491 (height branch); star fill at 199-228 uses this reduced available`

- **현상(버그)**: When the grid is WRAP_CONTENT (requestedWidth<0 and != MATCH_PARENT), starAvailableWidth is set to max(nonStarWidth, minWidthContent) where nonStarWidth counts only ABSOLUTE+AUTO tracks; the star pass then divides only that residual, so star tracks get ~0 and any content placed solely in star cells disappears.

- **근거 — 일반 프레임워크 기대동작**: Under an auto/indefinite container, CSS treats fr tracks by their min-content (content still fits) and WPF treats Star as Auto when the grid is size-to-content. Here star cells in a WRAP_CONTENT grid collapse with no content floor, dropping their children to zero size.

- **근거 — 프레임워크 레퍼런스**: WPF: Star behaves as Auto under size-to-content; CSS fr uses min-content under indefinite sizing

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:low]): Mechanism verified at the cited lines. In grid-layout-manager.cpp:118-127, MeasureGridChildrenAndFillAuto captures child content into colWidths/rowHeights ONLY for AUTO-like tracks (colAutoLike/rowAutoLike); a child in an explicit STAR track contributes nothing. In the WRAP_CONTENT width branch (461-475) nonStarWidth sums only non-STAR tracks, and starAvailableWidth = max(nonStarWidth, minWidthContent) where minWidthContent derives solely from GetMinimumWidth (line 473), NOT from STAR-cell content. The height branch (477-491) is symmetric. ApplyGridDefinitions (199-228) then divides remainingWidth (from starAvailableWidth) across STAR tracks,…

    - `code`(confirmed [보정:low]): I re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp and traced the full data flow for the repro (WRAP_CONTENT grid, explicit STAR(1) column, 100px child). Every step the finding claims holds:  1. Measure/auto-capture pass: MeasureGridChildrenAndFillAuto computes colAutoLike at line 119 as `colDefs[col].GetType() == GridLengthType::AUTO` (or empty defs). For an EXPLICIT STAR column this is false, so lines 124-127 (`colWidths[col] = max(..., childSize.width+marginW)`) do NOT run. The 100px child content is never captured into colWidths[col]; it stays 0 for STAR tracks.  2. WRAP…

- **재현**: WRAP_CONTENT grid, single Star(1) column containing a 100px child → column width resolves to ~0 and the child is squashed, instead of the grid sizing to 100 (star→auto fallback).

- **▶ 수정 방향**: `grid-layout-manager.cpp:461-475` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-grid-layout-grid-07"></a>

#### 🟢 LOW · `grid-07` — Overlapping cells have no defined z-order or conflict handling

- **위치**: `grid-layout-manager.cpp:252-348 (ArrangeGridChildrenToCells iterates children with no overlap/z handling); no overlap test in utc-Dali-GridLayout.cpp (Utc list lines 37-644)`

- **현상(버그)**: Two children mapped to the same cell (explicitly or via clamping/default-(0,0)) are both arranged into identical bounds. There is no z-order assignment; paint order is whatever actor sibling order happens to be, and no UTC covers overlap.

- **근거 — 일반 프레임워크 기대동작**: CSS Grid/WPF allow overlap but define stacking (source order / ZIndex). DALi neither documents nor controls this; it is left to incidental actor order.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read the cited code at grid-layout-manager.cpp:252-350 (ArrangeGridChildrenToCells). The function iterates children in collection order (line 256, `for(auto& childView : children)`), and computes each child's bounds purely from its row/col/span (lines 265-348). row/col are clamped via std::min to [0, count-1] (lines 265-266), and GetChildRow/GetChildColumn default to 0 when no GridLayoutParams exist (grid-layout-manager.cpp:40-50). Therefore two children with the same (row,col) — explicitly, via clamping, or via the (0,0) default — receive identical childBounds.x/y/width/height and are arranged into the same rect. There is NO cell-occupa…

    - `intent`(intentional-design [보정:info]): Re-read the cited code at grid-layout-manager.cpp (actual path: /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp, NOT the internal path the finding cited). The code-level facts hold: ArrangeGridChildrenToCells (lines 252-349) iterates children in GetChildAt order, computes childBounds purely from row/col/span (lines 270-276), and calls childImpl.Arrange(childBounds) with zero z-order logic. Two children resolving to the same cell -- via std::min clamping (lines 265-268) or default (0,0) from GetChildRow/GetChildColumn returning 0 when params absent (lines 43,49) -- do get identical bounds. And the UT…

- **재현**: Two children at row0/col0 → render stacked with undefined which-on-top, and no API to control it.

- **▶ 수정 방향**: `grid-layout-manager.cpp:252-348` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-grid-layout-grid-08"></a>

#### 🟢 LOW · `grid-08` — Auto-track contribution and span correctness are untested

- **위치**: `utc-Dali-GridLayout.cpp:482-520 (AutoDefinitions and RowColumnSpan tests assert only span getters, not geometry); samples/gridlayout/gridlayout-span-example.cpp:62-90 (Star-only span)`

- **현상(버그)**: UtcDaliGridLayoutAutoDefinitionsP only checks a single non-spanning auto cell (m.width>=60). No test exercises a spanning child against AUTO tracks, star min/max, out-of-range placement, or overlap. The span sample (gridlayout-span-example) uses only Star tracks, so the span-into-auto gap (grid-01) is never visible in samples or UTCs.

- **근거 — 일반 프레임워크 기대동작**: Behavioral coverage for span+auto, star clamp, and out-of-range placement so regressions in the track-sizing algorithm are caught.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read all cited code and the surrounding test/impl context; every claim in the finding holds.  1. AutoDefinitions coverage is weak as claimed. utc-Dali-GridLayout.cpp:482-499 (UtcDaliGridLayoutAutoDefinitionsP) adds one Auto row + one Auto column and a single child at Row(0)/Column(0) with default span 1. It asserts only m.GetWidth() >= 60.0f and m.GetHeight() >= 40.0f (lines 497-498) — weak inequalities on the container, no per-cell geometry, and no spanning child against the auto tracks.  2. The span test asserts only getters. utc-Dali-GridLayout.cpp:502-519 (UtcDaliGridLayoutRowColumnSpanP) calls Measure/Arrange but asserts only GetRow…

    - `intent`(confirmed [보정:low]): All cited evidence verified against /home/jae/dali/dali-ui-claude. The finding is filed as missing-coverage and on that framing it is accurate.  UTC facts (utc-Dali-GridLayout.cpp): UtcDaliGridLayoutAutoDefinitionsP (482-499) only asserts m.GetWidth()>=60 / m.GetHeight()>=40 for a SINGLE non-spanning auto cell (single Auto row + single Auto col, child at 0,0, span defaults 1). UtcDaliGridLayoutRowColumnSpanP (502-520) uses Absolute(40)/Absolute(60) tracks and asserts ONLY the span getters (GetRowSpan==2, GetColumnSpan==1) — no geometry, and never touches an Auto track. Grepping the whole file confirms no test exercises span+Auto contribution,…

- **재현**: A regression dropping spanning-cell auto contribution (already the current behavior) passes all existing tests.

- **▶ 수정 방향**: `utc-Dali-GridLayout.cpp:482-520` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


### AbsoluteLayout (`absolute-layout`) — 6건


<a id="fix-absolute-layout-abs-01"></a>

#### 🟠 HIGH · `abs-01` — RTL unconditionally mirrors absolute X — diverges from CSS `left`/WPF Canvas, and no flag to opt out

- **위치**: `view-impl.cpp:1193-1213 (ApplyLayoutDirection), view-impl.cpp:1079 (call site), utc-Dali-AbsoluteLayout.cpp:481-511 (RTL test codifies the flip)`

- **현상(버그)**: After the manager arranges children, ViewImpl::Arrange calls ApplyLayoutDirection(bounds.width) which, when the effective layout direction is RIGHT_TO_LEFT, rewrites every non-standalone child's X as parentWidth - oldX - childW (view-impl.cpp:1079, 1193-1213). The AbsoluteLayout manager itself is direction-agnostic and never sees the flag. UTC asserts red at x=10 becomes 200-10-50=140 under RTL (utc-Dali-AbsoluteLayout.cpp:487,502).

- **근거 — 일반 프레임워크 기대동작**: In CSS, an absolutely-positioned box's `left` is a physical inset and is NOT mirrored under `direction:rtl` (only logical `inset-inline-start` flips). WPF Canvas.Left is likewise not flipped by FlowDirection for absolute positioning. A general framework gives absolute coordinates a stable physical meaning or a separate logical/start-end opt-in; it does not silently flip every explicit X.

- **근거 — 프레임워크 레퍼런스**: CSS Positioned Layout / CSS Logical Properties (physical `left` vs logical `inset-inline-start`); WPF Canvas + FlowDirection

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): All cited claims verified against the actual source (file is at dali-ui-foundation/public-api/view-impl.cpp, not internal/, but content matches).  1. Call site: ViewImpl::Arrange unconditionally calls ApplyLayoutDirection(bounds.width) at view-impl.cpp:1079, AFTER the manager/callback/OnArrange has already placed children (line 1062). The comment at 1075-1078 explicitly states the intent: "Mirror direct children when the effective layout direction resolves to RIGHT_TO_LEFT ... keeping layout managers direction-agnostic." This runs for ALL layout managers, including AbsoluteLayout.  2. The mirror math: ApplyLayoutDirection (view-impl.cpp:1193-…

    - `intent`(intentional-design [보정:low]): All cited facts are verified, but the behavior is a deliberate, documented design choice with a real opt-out — not an unintended deviation, and the severity/framing is overstated.  Verified facts: - ApplyLayoutDirection (view-impl.cpp:1193-1213) mirrors X as `parentWidth - oldX - childW` for every non-standalone child, called once per Arrange at view-impl.cpp:1079, so all layout managers (including AbsoluteLayout) stay direction-agnostic. grep over absolute-layout-impl.cpp returned no direction/mirror handling, confirming the manager never sees the flag. - UTC UtcDaliAbsoluteLayoutDirectionRtlP (utc-Dali-AbsoluteLayout.cpp:481-511) deliberate…

    - `code2`(intentional-design [보정:low]): All cited code facts are accurate and re-verified. ViewImpl::Arrange calls ApplyLayoutDirection(bounds.width) at view-impl.cpp:1079 after every Arrange variant (LayoutManager / callback / default). ApplyLayoutDirection (view-impl.cpp:1193-1213) early-returns unless GetEffectiveLayoutDirection()==RIGHT_TO_LEFT (line 1195), then for each non-standalone child rewrites POSITION_X = parentWidth - oldX - childW (line 1211). AbsoluteLayoutManager::Arrange (absolute-layout-manager.cpp:179-304) is genuinely direction-agnostic: it computes X purely from childBoundsSpec.x and never reads layout direction. The RTL UTC (utc-Dali-AbsoluteLayout.cpp:481-512…

- **재현**: App places a 'Close' button at absolute x=10 (top-left). On an Arabic locale the parent's layout direction resolves RTL and the button jumps to the far right (x=140) with no way to pin it physically, because AbsoluteLayoutParams has no start/end vs left/right distinction.

- **▶ 수정 방향**: `view-impl.cpp:1193-1213` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-absolute-layout-abs-03"></a>

#### 🟠 HIGH · `abs-03` — Proportional WIDTH/HEIGHT in Measure resolves against the incoming constraint, inflating a WRAP_CONTENT container (size circularity)

- **위치**: `absolute-layout-manager.cpp:114-117 (Measure uses contentWidth) vs 220-221 (Arrange uses availableWidth); maxRight at 172; sample relies on minWidth dominating to hide it (samples/absolutelayout/absolutelayout-wrapcontent-proportional-example.cpp:28-40)`

- **현상(버그)**: In Measure, a WIDTH_PROPORTIONAL child computes w = bounds.width * contentWidth, where contentWidth == the incoming widthConstraint (absolute-layout-manager.cpp:114-117,78). That w then feeds maxRight = x + w + marginW (line 172), and Measure returns maxRight as the container's WRAP_CONTENT size (view-impl.cpp:2447). For a WRAP_CONTENT container the incoming constraint is the grandparent's available space, NOT the container's own (still-unknown) size, so a SIZE_PROPORTIONAL child makes the container shrink-wrap to a fraction of the grandparent — a different value than the Arrange phase uses (availableWidth = the container's resolved content width, line 187,220-221).

- **근거 — 일반 프레임워크 기대동작**: A shrink-to-fit (WRAP_CONTENT) container cannot resolve a child whose size is a percentage of that same container (CSS treats such percentages as 0/auto to break the cycle; WPF Canvas reports (0,0) and never shrink-wraps). The Measure pass should exclude proportional-size children from the bounding box (or treat their proportional extent as 0), as MATCH_PARENT children already are excluded by reporting min size. Here they are included with a value derived from the wrong reference, so Measure and Arrange disagree.

- **근거 — 프레임워크 레퍼런스**: CSS percentage resolution against a shrink-to-fit/auto containing block (treated as auto); WPF Canvas measure-to-infinity

- **근거 — 검증자 코드분석**:

    - `code2`(confirmed [보정:high]): I independently re-derived the algorithm from source and the finding holds in full.  WRAP_CONTENT container path (re-read): For a WRAP_CONTENT-width container, ViewImpl::DispatchMeasureWithLayoutManager sets requestedVisW<0, so effectiveVisW = widthConstraint (the INCOMING constraint, i.e. grandparent available space) [view-impl.cpp:2432-2434], contentVisW = effectiveVisW - padding [2436], and passes that as contentWidth to manager->Measure [2439]. In AbsoluteLayoutManager::Measure, contentWidth = widthConstraint [absolute-layout-manager.cpp:78], and a WIDTH_PROPORTIONAL child computes w *= contentWidth [114-116]. That w feeds maxRight = std:…

    - `code`(confirmed [보정:high]): I traced the real data flow and the finding holds in full.  WRAP_CONTENT container measure path (absolute-layout-manager.cpp + view-impl.cpp DispatchMeasureWithLayoutManager): - view-impl.cpp:2429-2435: for a WRAP_CONTENT width, requestedWidth<0 so requestedVisW<0, hence effectiveVisW = widthConstraint (the constraint handed down by the parent/grandparent, e.g. window/grandparent available width). contentVisW = that minus padding (2436). - view-impl.cpp:2439: manager->Measure(this, contentVisW, ...). Inside absolute-layout-manager.cpp:78 contentWidth = widthConstraint == the grandparent-derived value. - absolute-layout-manager.cpp:114-117: fo…

    - `intent`(confirmed [보정:medium]): Re-read the actual source (file at dali-ui-foundation/public-api/layouts/, not integration-api as the finding cited, but line numbers match). The mechanism is verified end-to-end:  1. absolute-layout-manager.cpp:78 sets contentWidth = widthConstraint; :114-116 computes w *= contentWidth for WIDTH_PROPORTIONAL; :172 folds it into maxRight = max(maxRight, x+w+marginW); :176 returns MeasuredSize(maxRight, maxBottom). 2. view-impl.cpp:2434 shows that for a WRAP_CONTENT container (requestedVisW < 0) effectiveVisW = widthConstraint — i.e., the INCOMING ancestor constraint — and :2436/:2439 pass that (minus padding) into manager->Measure as contentV…

- **재현**: AbsoluteLayout with WRAP_CONTENT width (no minWidth), one child SIZE_PROPORTIONAL bounds.width=0.5 at x=0. Scene hands the container a 1080px constraint. Measure returns maxRight=0.5*1080=540, so the WRAP container becomes 540 wide; Arrange then computes the child as 0.5*540=270 — container and child no longer agree, and the container size depends on the ancestor constraint rather than its content.

- **▶ 수정 방향**: `absolute-layout-manager.cpp:114-117` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-absolute-layout-abs-02"></a>

#### 🟡 MEDIUM · `abs-02` — Proportional position is an alignment fraction (available-child)*p, not a CSS percentage inset

- **위치**: `absolute-layout-manager.cpp:154-170 and 269-285; layout-structure.md does not describe the formula`

- **현상(버그)**: With X_PROPORTIONAL, x = (contentWidth - w) * bounds.x in both Measure and Arrange (absolute-layout-manager.cpp:157, 272). So bounds.x=0.25 with child width 100 in a 200 container yields x=(200-100)*0.25=25, and 0.5 centers, 1.0 right-aligns. UTC and the proportional sample codify this (utc-Dali-AbsoluteLayout.cpp:273-274; samples/absolutelayout/absolutelayout-proportional-example.cpp:30-37 comments '0.5,0.5 => center').

- **근거 — 일반 프레임워크 기대동작**: CSS `left: 25%` resolves to 25% of the containing block width = 50px as a pure offset, independent of the child's own width. The DALi formula instead couples position to the child's measured size and only spans the gap (0..available-child). This is a legitimate 'gravity/anchor' design (like Android gravity fractions), but it is surprising for anyone expecting percentage positioning and is not flagged as a deviation in the public docs.

- **근거 — 프레임워크 레퍼런스**: CSS percentage of containing block for `left`; vs Android View gravity fractions

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:low]): CODE CONFIRMED. The cited formula is exactly as claimed. In Measure: absolute-layout-manager.cpp:157 `x = (contentWidth - w) * bounds.x` (and :165 for y); in Arrange: :272 `x = (availableWidth - w) * childBoundsSpec.x` (and :280 for y), each guarded by xProportional/yProportional flags (lines 155, 163, 270, 278). So bounds.x=0.25, w=100, container 200 -> x=25; 0.5 -> centered; 1.0 -> right-anchored. The mechanic in the finding is accurate.  INTENT — DELIBERATE. (1) The UTC pins this exact arithmetic on purpose, not incidentally: utc-Dali-AbsoluteLayout.cpp:273-274 `// X = (200 - 100) * 0.25 = 25` with DALI_TEST_EQUALS(GetPositionX(), 25.0f); …

    - `code`(confirmed [보정:medium]): I re-read the cited code (the file is actually at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/absolute-layout-manager.cpp, not the path in the finding, but it is the same logical file). The observed claim holds exactly:  - Measure: line 157 `x = (contentWidth - w) * bounds.x;` (under `if(xProportional)`, line 155); line 165 `y = (contentHeight - h) * bounds.y;`. The comment on line 154 states "Proportional position: axis = (available - childExtent) * proportion". - Arrange: line 272 `x = (availableWidth - w) * childBoundsSpec.x;` (under `if(xProportional)`, line 270); line 280 same for y. Comment on line 269 repeats th…

- **재현**: Developer ports a CSS layout using `left:50%; width:40px` expecting the box's left edge at 50% (100px) — in DALi it is centered at (200-40)*0.5=80px instead. Same proportion, different result, with no documentation of the difference.

- **▶ 수정 방향**: `absolute-layout-manager.cpp:154-170` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-absolute-layout-abs-04"></a>

#### 🟡 MEDIUM · `abs-04` — MATCH_PARENT child with no explicit bounds is measured to natural size in Measure but contributes natural (not min) size to the bounding box

- **위치**: `absolute-layout-manager.cpp:141-152 (Measure path, no RequestedWidth check) vs 244-266 (Arrange path checks RequestedWidth); view-impl.cpp:2444-2445 (MATCH_PARENT⇒min in dispatch)`

- **현상(버그)**: The manager keys MATCH_PARENT handling off bounds sign, not the child's RequestedWidth. In Measure, a child with default bounds (-1,-1) is measured with the full content space and w is set to childSize.width (absolute-layout-manager.cpp:141-152); for a MATCH_PARENT child childImpl.Measure returns its MINIMUM size per the dispatch contract (view-impl.cpp:2444-2445), so that is what flows into maxRight. Only in Arrange does the code special-case GetRequestedWidth()==MATCH_PARENT to fill available space (line 245-249,257-261). The doc claims AbsoluteLayout MATCH_PARENT 'fill the available content area (when no explicit bounds are set)' (layout-structure.md:160-161) but a WRAP_CONTENT parent will shrink-wrap to the MATCH_PARENT child's MIN size, which is the intended follower rule yet is undocumented for this manager and untested.

- **근거 — 일반 프레임워크 기대동작**: Per the documented follower contract MATCH_PARENT should report minimum size in Measure (consistent) — but the manager never references GetRequestedWidth in Measure, so the behavior is incidental to the dispatch returning min size, not explicit. A WRAP parent + MATCH_PARENT-only child collapses to min (0) width; there is no UTC covering a non-standalone MATCH_PARENT child under AbsoluteLayout to lock this in.

- **근거 — 프레임워크 레퍼런스**: Android FrameLayout MATCH_PARENT child under wrap_content parent (child clamped to parent's other-child-driven size)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read the cited code and the finding holds in full.  1. AbsoluteLayoutManager::Measure (absolute-layout-manager.cpp:84-176) keys MATCH_PARENT handling off the bounds sign, never off GetRequestedWidth. For a child with default bounds (-1,-1): w=bounds.width=-1, not proportional, so measureW = max(0, contentWidth - marginW) (line 141); childImpl.Measure(measureW,...) is called (line 143); then because w<0, w = childSize.width (line 147) and maxRight = max(maxRight, x+w+marginW) (line 172). There is NO GetRequestedWidth/MATCH_PARENT check anywhere in this loop.  2. The dispatch contract returns MIN for MATCH_PARENT, confirmed in two places: …

    - `intent`(intentional-design [보정:info]): Re-read the cited code. AbsoluteLayoutManager::Measure (absolute-layout-manager.cpp:141-152) indeed never references GetRequestedWidth: for a negative axis it sets measureW=contentWidth-margin, calls childImpl.Measure, then for w<0 sets w=childSize.width and folds that into maxRight (line 172). The dispatch childImpl.Measure returns MIN size for MATCH_PARENT (view-impl.cpp:2444-2445, and identically at 1000-1002 / 1028-1030), so min — not natural — flows into the bounding box. (The finding's TITLE says "natural size"; that wording is wrong, but its observed body correctly says min flows through, so the title is just mislabeled.) Arrange (abso…

- **재현**: AbsoluteLayout WRAP_CONTENT width contains a single child with RequestedWidth=MATCH_PARENT and default bounds. The container shrink-wraps to width 0 (min), then Arrange fills the MATCH child to 0 — the child vanishes, whereas a developer reading 'fills the available content area' expects it to fill.

- **▶ 수정 방향**: `absolute-layout-manager.cpp:141-152` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-absolute-layout-abs-06"></a>

#### 🟡 MEDIUM · `abs-06` — Asymmetric container padding breaks RTL mirroring of absolute children

- **위치**: `view-impl.cpp:2472-2475 (content offset by padding.start), 1211 (mirror uses full width), absolute-layout-manager.cpp:287-291 (child x includes content offset)`

- **현상(버그)**: The manager arranges children inside the content box (DispatchArrange passes visContentBounds with x=padding.start*scale, view-impl.cpp:2472), and the manager adds bounds.x to the child x (absolute-layout-manager.cpp:290), so a child at proportional/absolute content-x=0 ends up at absolute X=padding.start. RTL mirroring then uses newX = fullParentWidth - oldX - childW (view-impl.cpp:1211) with the FULL width, ignoring that the usable region is inset by padding.start on the left and padding.end on the right. With padding.start != padding.end the mirrored child is no longer symmetric within the content box.

- **근거 — 일반 프레임워크 기대동작**: A correct RTL mirror about the content box should be newX = (padStart) + (contentWidth - (oldX_content) - childW), i.e. mirror within [padStart, fullWidth-padEnd]. Using full width only coincides when padStart==padEnd. CSS/WPF keep abspos children inside the padding box under direction changes.

- **근거 — 프레임워크 레퍼런스**: CSS padding box as the containing block for abspos under RTL; box-model symmetry

- **근거 — 검증자 코드분석**:

    - `intent`(confirmed [보정:medium]): I re-read all cited code under /home/jae/dali/dali-ui-claude and the finding is accurate.  GEOMETRY VERIFIED: - view-impl.cpp:2471-2475 — DispatchArrangeWithLayoutManager builds visContentBounds with x = padding.start*s and width = visualBounds.width - (start+end)*s, then passes it to manager->Arrange. So the manager's coordinate origin is the content box, offset by padStart from the parent actor's top-left. - absolute-layout-manager.cpp:289-291 — childBounds.x = bounds.x + x + margin.start*scale, i.e. the child POSITION_X includes the padStart content offset. A child at content-x=0 lands at POSITION_X = padStart*s. - The same offset exists f…

    - `code`(confirmed [보정:medium]): I re-read every cited line in the worktree (the files are under .../dali-ui-foundation/public-api/, not internal/, but are the same code). All three legs of the finding's data flow hold:  1. Content-box offset by padding.start: view-impl.cpp:2471-2475 — DispatchArrangeWithLayoutManager builds visContentBounds.x = padding.start * s and passes it as `bounds` to manager->Arrange (line 2477).  2. Manager adds bounds.x into the child x: absolute-layout-manager.cpp:289-300 — childBounds.x = bounds.x + x + margin.start*childScale, then childImpl.Arrange(childBounds). For a leaf child, OnArrange sets POSITION_X = bounds.x (view-impl.cpp:1092,1098). S…

- **재현**: AbsoluteLayout padding (start=40,end=0), child absolute bounds x=0,w=50. LTR: child at X=40. RTL: newX=200-40-50=110, leaving 90px gap on the right and 40px reserved-but-empty on the left — the padding is effectively applied on the wrong side after the flip.

- **▶ 수정 방향**: `view-impl.cpp:2472-2475` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-absolute-layout-abs-08"></a>

#### 🟢 LOW · `abs-08` — AbsoluteLayoutFlags has no anchor/origin or edge (right/bottom) flags — only a single (x,y,w,h) rect, unlike every compared framework

- **위치**: `layout-types.h:543-577 (flag enum), absolute-layout-params-impl.h:46-49,154-155 (only mBounds+mFlags)`

- **현상(버그)**: The flag set is exactly the four per-axis proportional bits plus composites (layout-types.h:543-577). There is no flag for anchoring to right/bottom edges, no top/left/right/bottom inset model, and no z-order control; placement is a single LayoutRect with origin implicitly top-left (absolute-layout-params-impl.h:154).

- **근거 — 일반 프레임워크 기대동작**: CSS abspos and WPF Canvas both expose opposite-edge anchors (right/bottom, Canvas.Right/Bottom) so a box can be pinned to the far edge and stretch with the container. DALi AbsoluteLayout cannot pin to the right edge except via the alignment-fraction trick (X_PROPORTIONAL with bounds.x=1.0), which couples to child size and does not stretch. This is a capability gap relative to the canonical equivalents the audit compares against.

- **근거 — 프레임워크 레퍼런스**: CSS top/left/right/bottom insets; WPF Canvas.Right/Canvas.Bottom

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read every cited location under /home/jae/dali/dali-ui-claude and the full consuming algorithm.  Structural claims verified: - layout-types.h:543-577 defines AbsoluteLayoutFlags as exactly the four per-axis proportional bits (X_PROPORTIONAL, Y_PROPORTIONAL, WIDTH_PROPORTIONAL, HEIGHT_PROPORTIONAL) plus composites (POSITION_PROPORTIONAL, SIZE_PROPORTIONAL, ALL, NONE). There is no right/bottom edge flag and no z-order flag. Confirmed. - absolute-layout-params-impl.h:154-155 stores exactly two members: a single LayoutRect mBounds and AbsoluteLayoutFlags mFlags. Default bounds (0,0,-1,-1) (line 48). The only placement state is the (x,y,w,h) …

    - `intent`(intentional-design [보정:info]): VERIFIED the structure (cited paths were stale but content matches). AbsoluteLayoutParamsImpl stores exactly mBounds (LayoutRect x,y,w,h default 0,0,-1,-1) + mFlags (absolute-layout-params-impl.h:48-49,154-155). The flag enum AbsoluteLayoutFlags (layout-types.h:543-577) is exactly four per-axis proportional bits {X,Y,WIDTH,HEIGHT}_PROPORTIONAL plus composites POSITION_PROPORTIONAL/SIZE_PROPORTIONAL/ALL. No right/bottom anchor, no inset model, no z-order flag — the auditor's factual description is accurate.  This is INTENTIONAL DESIGN, deliberately documented and tested, not an accidental omission: - git blame: the flag set was purposely intro…

- **재현**: Developer wants a footer pinned 10px from the bottom-right that grows with the window. No right/bottom anchor exists; they must recompute bounds on every resize or accept the size-coupled proportional anchor.

- **▶ 수정 방향**: `layout-types.h:543-577` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


### Layout 베이스 & 콜백 (`layout-base-callbacks`) — 6건


<a id="fix-layout-base-callbacks-layout-01"></a>

#### 🟠 HIGH · `layout-01` — Callback Measure/Arrange path ignores the view's padding, unlike the LayoutManager and default paths

- **위치**: `view-impl.cpp:918-921, view-impl.cpp:2436-2447, view-impl.cpp:2471-2477, view-impl.cpp:2482-2490`

- **현상(버그)**: In ViewImpl::Measure the callback branch calls callback->Invoke(view, effVisW, effVisH) with the full constraint and uses the return verbatim (view-impl.cpp:918-921, 931). In Arrange, DispatchArrangeWithCallback passes the full visualBounds with no padding inset (view-impl.cpp:2482-2490). By contrast DispatchMeasureWithLayoutManager subtracts padding (contentVisW=effectiveVisW-visPadW) and re-adds it (view-impl.cpp:2436-2447), and DispatchArrangeWithLayoutManager insets the content origin/size by padding (view-impl.cpp:2471-2477); default OnMeasure/OnArrange also honor padding (view-impl.cpp:953-960, 1106-1144).

- **근거 — 일반 프레임워크 기대동작**: All three customization paths for the same View should observe the same content rect. A general framework (WPF MeasureOverride / Android onMeasure) always presents the content/available size with the container's own padding already removed, and offsets children by it during arrange, regardless of how the layout logic is supplied.

- **근거 — 프레임워크 레퍼런스**: WPF Panel MeasureOverride/ArrangeOverride (availableSize/finalSize are content-relative after padding); Android ViewGroup padding contract

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Re-read every cited site in /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp; the observed claim holds in every particular.  Measure callback path: ViewImpl::Measure computes effVisW/effVisH as only the min/max-clamped full constraint (view-impl.cpp:905-906, 915-916) and passes them verbatim to the callback: callback->Invoke(view, effVisW, effVisH) (918-921). No padding is subtracted. The return is run only through ApplyConstraints (931), which is min/max only.  Manager Measure path: DispatchMeasureWithLayoutManager subtracts padding to form contentVisW/contentVisH (view-impl.cpp:2426-2437) before calling manager->Mea…

    - `intent`(confirmed [보정:medium]): Index 2 = id "layout-01" (finderKey "layout-base-callbacks"): callback Measure/Arrange paths ignore the View's padding while the LayoutManager and default paths honor it.  CODE FACTS (all re-read under /home/jae/dali/dali-ui-claude): - view-impl.cpp:918-921 — callback Measure branch invokes callback->Invoke(view, effVisW, effVisH) with the FULL constraint; no padding subtracted. Result used verbatim via ApplyConstraints (931), which only clamps min/max. - view-impl.cpp:2482-2490 (DispatchArrangeWithCallback) — passes visualBounds straight to the callback; no padding inset of origin or size. view-impl.cpp:1056-1058 confirms the raw `bounds` is…

    - `code2`(confirmed [보정:medium]): I re-derived the behavior from the cited source under /home/jae/dali/dali-ui-claude and the divergence is real.  Measure dispatch (view-impl.cpp:917-930): the callback branch (918-921) invokes `callback->Invoke(view, effVisW, effVisH)` with the FULL clamped constraint and uses the return verbatim (only ApplyConstraints/min-max runs at :931). By contrast DispatchMeasureWithLayoutManager (view-impl.cpp:2421-2457) subtracts padding at :2436-2437 (`contentVisW = effectiveVisW - visPadW`) before calling manager->Measure and re-adds it at :2447. Default OnMeasure honors padding at :953-960 (`pw`, `contentWidth = effectiveWidth - pw`).  Arrange disp…

- **재현**: layout.SetViewPadding({10,10,10,10}); layout.SetMeasureCallback(...); layout.SetArrangeCallback(...). The callback receives the full 200px (not 180px content) and arranges children starting at bounds.x=0 instead of 10 — padding is silently dropped, whereas the same layout expressed as a StackLayoutManager honors padding.

- **▶ 수정 방향**: `view-impl.cpp:918-921` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-layout-base-callbacks-layout-02"></a>

#### 🟠 HIGH · `layout-02` — Callback Measure path ignores RequestedWidth/RequestedHeight; manager and default paths force them

- **위치**: `view-impl.cpp:905-906, view-impl.cpp:931, view-impl.cpp:2442-2443, view-impl.cpp:996-998`

- **현상(버그)**: ViewImpl::Measure clamps only to min/max before the callback (effNatW=clamp(natW,min,max), view-impl.cpp:905-906) and the callback's return is only run through ApplyConstraints (min/max only, view-impl.cpp:931, 1419-1428). RequestedWidth/Height never override a callback result. But DispatchMeasureWithLayoutManager forces resultVisW=requestedVisW when requested>=0 (view-impl.cpp:2442-2443) and default OnMeasure forces size.width=mRequestedWidth (view-impl.cpp:996-998).

- **근거 — 일반 프레임워크 기대동작**: An explicit fixed size set on the container (SetRequestedWidth) should be authoritative for the container's measured size irrespective of which layout mechanism is active, as it is for managers and the default path.

- **근거 — 프레임워크 레퍼런스**: WPF Width/Height vs MeasureOverride desired size; Android exactly/AT_MOST MeasureSpec

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:high]): I re-read every cited location in /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp and the UTC.  1. Callback path (view-impl.cpp:918-922): when a MeasureCallback exists, Measure() takes the `if(auto* callback = GetMeasureCallback())` branch FIRST, invokes the callback with effVisW/effVisH (= clamp(natW, min, max) only, lines 905-906/915-916), and assigns its return directly. RequestedWidth/Height are never read on this path.  2. The only post-callback processing is ApplyConstraints (view-impl.cpp:931 -> 1419-1428), which clamps solely to GetMinimumWidth/Height * s and GetMaximumWidth/Height * s. It never references mR…

    - `intent`(intentional-design [보정:low]): The code-level divergence is real and accurately described. Re-read /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:918-921 + :931 (callback measure invokes callback->Invoke(view,effVisW,effVisH) and only runs ApplyConstraints, which at :1419-1428 enforces min/max only — never RequestedWidth). Contrast DispatchMeasureWithLayoutManager at :2442-2447 (forces resultVisW=requestedVisW when requestedVisW>=0) and default OnMeasure at :996-998 (forces size.width=mRequestedWidth). So a measure callback returning {50,50} on a layout with SetRequestedWidth(200) does yield measured width 50, while the manager/default paths yiel…

    - `code2`(confirmed [보정:medium]): Element index 3 = finding "layout-02" (callback Measure path ignores RequestedWidth/RequestedHeight). Every cited fact re-verified against /home/jae/dali/dali-ui-claude:  1. Callback path in ViewImpl::Measure (view-impl.cpp:897-935): natW/natH are clamped ONLY to min/max -> effNatW (905-906); effVisW = effNatW*s (915); the callback branch is just `visual = callback->Invoke(view, effVisW, effVisH)` (918-921); the only post-processing is `visual = ApplyConstraints(visual)` (931). ApplyConstraints (1419-1428) applies ONLY min/max (max with min*s, min with max*s). RequestedWidth/Height are never read or forced on this path. Confirmed.  2. Manager…

- **재현**: layout.SetRequestedWidth(200); layout.SetMeasureCallback(cb) where cb returns {50,50}. Measured width becomes 50, not 200. With a StackLayoutManager + RequestedWidth(200) it would be 200. The only two UTC callback tests (utc-Dali-Layout.cpp:329-350) set RequestedWidth==constraint==200 and a callback that returns widthConstraint, masking this divergence.

- **▶ 수정 방향**: `view-impl.cpp:905-906` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-layout-base-callbacks-layout-03"></a>

#### 🟡 MEDIUM · `layout-03` — No detach/replace for an attached LayoutManager — attach-once-for-life with a hard runtime assert

- **위치**: `view-impl.cpp:1580, dali-ui-foundation/public-api/view.h:296-305`

- **현상(버그)**: AttachLayoutManager asserts !HasLayoutManager() with DALI_ASSERT_ALWAYS and there is no DetachLayoutManager / SetLayoutManager(nullptr) anywhere in the public or impl API (view-impl.cpp:1577-1585; grep over dali-ui-foundation finds no detach). view.h:296-301 documents this as intentional ("Only one LayoutManager may be attached for the lifetime of a View").

- **근거 — 일반 프레임워크 기대동작**: Canonical containers allow swapping the layout strategy at runtime (Android RecyclerView.setLayoutManager(lm) including null to clear). A hard ASSERT_ALWAYS on re-attach makes the operation crash rather than no-op or replace.

- **근거 — 프레임워크 레퍼런스**: Android RecyclerView.setLayoutManager (replaceable, nullable)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): The "observed" claim is factually accurate at every cited point. view-impl.cpp:1580 contains `DALI_ASSERT_ALWAYS(!HasLayoutManager() && "LayoutManager already set. Cannot replace an existing LayoutManager.")`, so a second AttachLayoutManager call on a View that already has a manager aborts the process. A repo-wide grep for DetachLayoutManager/SetLayoutManager/RemoveLayoutManager/ClearLayoutManager/ReplaceLayoutManager over dali-ui-foundation returns nothing — AttachLayoutManager (view-impl.cpp:1577, declared view.h:305 and view-impl.h:751) is the only manager-mutation entry point, and it transfers ownership via a UniquePtr with no null-clear …

    - `intent`(intentional-design [보정:info]): The observed code facts are accurate: AttachLayoutManager hard-asserts on re-attach via DALI_ASSERT_ALWAYS(!HasLayoutManager() && "LayoutManager already set. Cannot replace an existing LayoutManager.") (view-impl.cpp:1580), and no DetachLayoutManager/SetLayoutManager(nullptr) exists in the foundation. However, this is a deliberate, documented design choice, not a defect.  Evidence of intent: (1) view.h:295-301 explicitly documents "Only one LayoutManager may be attached for the lifetime of a View. Passing a null UniquePtr asserts. Attaching when a LayoutManager is already present asserts." (2) git blame shows the assert and its message were a…

- **재현**: An app wants to switch a container from Stack to Grid at runtime: view.AttachLayoutManager(stack); ... view.AttachLayoutManager(grid); the second call aborts the process via DALI_ASSERT_ALWAYS.

- **▶ 수정 방향**: `view-impl.cpp:1580` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-layout-base-callbacks-layout-05"></a>

#### 🟢 LOW · `layout-05` — SetMeasureCallback({}) / SetArrangeCallback({}) leaves the LayoutCallbacks trait object permanently allocated; doc says it 'removes'

- **위치**: `view-impl.cpp:1553-1556, layout-callbacks-object.cpp:46-54, dali-ui-foundation/public-api/view.h:222-223`

- **현상(버그)**: SetMeasureCallback always calls EnsureLayoutCallbacksObject(this) (creating the trait if absent) then stores the (possibly empty) callback (view-impl.cpp:1553-1556; layout-callbacks-object.cpp:36-44). Passing {} stores an empty Callback so GetMeasureCallback returns nullptr (fallback to manager works) but the LayoutCallbacksObject trait is never removed/freed. view.h:211/223 and the design doc describe {} as 'remove'.

- **근거 — 일반 프레임워크 기대동작**: 'Remove' should detach/free the trait, or the docs should say the slot is cleared but the holder persists. Calling SetMeasureCallback({}) on a View that never had a callback needlessly allocates and attaches a trait object.

- **근거 — 프레임워크 레퍼런스**: N/A (resource-hygiene / API-contract accuracy)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): Every claim in the finding holds against the actual source under /home/jae/dali/dali-ui-claude.  1. SetMeasureCallback/SetArrangeCallback always call EnsureLayoutCallbacksObject(this): verified at view-impl.cpp:1553-1556 and 1559-1562. They unconditionally invoke EnsureLayoutCallbacksObject(this)->Set...Callback(std::move(callback)) with no empty-callback branch.  2. EnsureLayoutCallbacksObject (view-impl.cpp:244-254) allocates a new Internal::LayoutCallbacksObject and attaches it via ViewDataImpl::SetTrait(LAYOUT_SIGNALS, newObject) whenever GetLayoutCallbacksObject returns null. There is no guard against an empty callback, so SetMeasureCall…

    - `intent`(intentional-design [보정:info]): The mechanics in the finding are accurate but the "contract mismatch" is not real. Verified: SetMeasureCallback({}) calls EnsureLayoutCallbacksObject (view-impl.cpp:1555 -> 244-254) which lazily creates and attaches the LAYOUT_SIGNALS trait if absent, then stores an empty Callback (layout-callbacks-object.cpp:36-39). The trait holder is never detached for an empty callback, so it persists for the View's lifetime.  However, the documented "remove" contract IS honored at the observable level: an empty mOnMeasure makes GetMeasureCallback() return nullptr (layout-callbacks-object.cpp:46-49), so the measure pass at view-impl.cpp:918 skips the call…

- **재현**: view.SetMeasureCallback({}); on a fresh View allocates a LayoutCallbacksObject and attaches it under LAYOUT_SIGNALS even though no callback is present; it is never reclaimed for the View's lifetime.

- **▶ 수정 방향**: `view-impl.cpp:1553-1556` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-layout-base-callbacks-layout-07"></a>

#### 🟢 LOW · `layout-07` — A LayoutManager that does not skip STANDALONE children double-arranges them; correctness depends on manager author calling IsStandalone

- **위치**: `view-impl.cpp:940, view-impl.cpp:1071-1073, layout-manager.cpp:47-60, custom-layout-manager-example.cpp:57,82`

- **현상(버그)**: After the manager/callback runs, ViewImpl::Arrange unconditionally calls ArrangeStandaloneChildren (view-impl.cpp:1071-1073) and Measure calls MeasureStandaloneChildren (940). The manager iterates children via GetChildCount/GetChildAt (layout-manager.cpp:47-55) which INCLUDE standalone children; only the protected IsStandalone helper (layout-manager.cpp:57-60) lets the author skip them. A custom manager that omits the IsStandalone check will measure/arrange standalone children once itself and the framework will do it again.

- **근거 — 일반 프레임워크 기대동작**: The framework iteration handed to a layout manager should already exclude standalone children (as the default OnMeasure/OnArrange do at view-impl.cpp:972/1117), so a naive custom manager cannot double-process them.

- **근거 — 프레임워크 레퍼런스**: Android ViewGroup skips GONE children in measure/layout loops by framework convention; here it is opt-in per child via author discipline

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read every cited location under /home/jae/dali/dali-ui-claude and the observed control/data flow holds exactly as described.  1. Manager dispatch iterates ALL children, no standalone filter: DispatchMeasureWithLayoutManager (view-impl.cpp:2421-2458) calls manager->Measure(this,...) at 2439; DispatchArrangeWithLayoutManager (2460-2480) calls manager->Arrange(this,...) at 2477. The only enumeration handed to a manager is LayoutManager::GetChildCount (layout-manager.cpp:47-50 -> view->GetChildCount()) and GetChildAt (52-55 -> view->GetChildAt(index)); neither filters STANDALONE. The opt-in skip is IsStandalone (57-60), checking GetLayoutMod…

    - `intent`(intentional-design [보정:info]): The code mechanics are accurate, but the finding is an intentional, documented design choice and its "expected" framework claim is wrong, so it does not represent a real deviation.  Mechanics confirmed: ViewImpl::Measure calls MeasureStandaloneChildren unconditionally after the manager runs (view-impl.cpp:940), Arrange calls ArrangeStandaloneChildren after the manager (view-impl.cpp:1073). The LayoutManager helpers GetChildCount/GetChildAt iterate ALL children including standalone ones (layout-manager.cpp:47-55), and IsStandalone (layout-manager.cpp:57-60) is opt-in. So a manager author who omits IsStandalone would touch a standalone child on…

- **재현**: A custom LayoutManager loops 0..GetChildCount and arranges each child without calling IsStandalone; a STANDALONE child gets positioned by the manager and then re-positioned by ArrangeStandaloneChildren, last-writer-wins, surprising the manager author.

- **▶ 수정 방향**: `view-impl.cpp:940` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-layout-base-callbacks-layout-08"></a>

#### ⚪ INFO · `layout-08` — layout-impl.cpp is an empty stub: the documented 'LayoutImpl::OnMeasure/OnArrange override' and 'mLayoutManager moved to LayoutImpl' were never implemented

- **위치**: `layout-impl.cpp:46-58, view-integ.cpp:132-135, /home/jae/dali/custom-layout-design.md:36-118`

- **현상(버그)**: LayoutImpl has no members and no method overrides (layout-impl.cpp:46-58; layout-impl.h:46-73). The manager is stored on ViewImpl via a trait and dispatched in ViewImpl::Measure/Arrange. This contradicts custom-layout-design.md §2/§3/§4 which specify moving mLayoutManager into LayoutImpl, overriding OnMeasure/OnArrange there, and making IsLayout() a virtual returning mLayoutManager!=nullptr. The shipped IsLayout (view-integ.cpp:132) instead means 'impl is a LayoutImpl handle'.

- **근거 — 일반 프레임워크 기대동작**: Either the design doc reflects the implementation or vice-versa. The 'Layout-only can have a layout algorithm' invariant from the design doc is NOT enforced — a plain View can hold a manager/callback and is dispatched identically.

- **근거 — 프레임워크 레퍼런스**: N/A (doc/impl drift)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:info]): The "observed" claim holds against the actual source. (1) layout-impl.cpp:46-58 and layout-impl.h:46-73 show LayoutImpl is a near-empty stub: it declares/defines only New(), the ctor, dtor, and deleted copy/move ops — no data members and NO method overrides (no OnMeasure/OnArrange/IsLayout). (2) The LayoutManager is stored on ViewImpl via a trait object: ViewImpl::AttachLayoutManager creates an Internal::LayoutManagerObject trait (view-impl.cpp:1581-1582), and ViewImpl::GetLayoutManager/HasLayoutManager read it back (view-impl.cpp:1587-1596) — these accessors live on ViewImpl, not LayoutImpl, contradicting design §2/§4 ("mLayoutManager → Layo…

    - `intent`(intentional-design [보정:info]): The finding's raw code observations are accurate: LayoutImpl is an empty stub (layout-impl.cpp:46-58, layout-impl.h:46-73 — no members, no OnMeasure/OnArrange override), the LayoutManager lives on ViewImpl as a Trait and is dispatched in ViewImpl::Measure (view-impl.cpp:918-930, AttachLayoutManager at 1577, HasLayoutManager/GetLayoutManager at 1587-1599), and IsLayout (view-integ.cpp:132-135) means "is a Layout handle" via DownCast, while HasLayoutCapability (view-integ.cpp:137-140) is the OR of IsLayout/HasLayoutManager/HasLayoutCallback. So a plain View CAN hold a manager/callback and is dispatched identically.  However, the finding frames …

- **재현**: Reading custom-layout-design.md to find where layout dispatch lives points a maintainer to LayoutImpl::OnMeasure, which does not exist; the logic is in ViewImpl and gated by traits, not by LayoutImpl type.

- **▶ 수정 방향**: `layout-impl.cpp:46-58` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


### LayoutParams (`layout-params`) — 4건


<a id="fix-layout-params-lp-03"></a>

#### 🟡 MEDIUM · `lp-03` — Foreign params type is silently ignored — no checkLayoutParams/generateLayoutParams conversion or assert

- **위치**: `stack-layout-manager.cpp:42,48; grid-layout-params-impl.h:69-72; flex-layout-manager.cpp:40-58; absolute-layout-manager.cpp:95,201 (each reads only its own id); view-impl.cpp:2170-2175 (no parent-type check)`

- **현상(버그)**: Each manager reads only its own trait id (StackLayoutManager -> StackLayoutParamsImpl::Get queries STACK_LAYOUT_PARAMS only, stack-layout-manager.cpp:42,48 + stack-layout-params-impl.h:98-103). Setting GridLayoutParams on a child of a StackLayout stores it under GRID_LAYOUT_PARAMS (grid-layout-params-impl.h:69-72); the StackLayoutManager never looks there, so weight/alignment fall back to defaults (weight 0, START). No assert, no warning, no conversion. SetLayoutParams (view-impl.cpp:2170-2175) does not consult the parent layout type at all.

- **근거 — 일반 프레임워크 기대동작**: Android ViewGroup.checkLayoutParams()/generateLayoutParams() convert or wrap foreign LayoutParams so the child still lays out meaningfully (or the framework warns). Silently discarding the developer's intent with no diagnostic is surprising.

- **근거 — 프레임워크 레퍼런스**: Android ViewGroup.checkLayoutParams / generateLayoutParams

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read every cited line and the finding's "observed" claims hold exactly.  1. Each manager reads only its own trait id via its own *ParamsImpl::Get, which queries a single ReservedTraitId: - StackLayoutManager: GetChildWeight/GetChildAlignment call Internal::StackLayoutParamsImpl::Get (stack-layout-manager.cpp:42,48), and that Get queries only ReservedTraitId::STACK_LAYOUT_PARAMS (stack-layout-params-impl.h:100), returning nullptr otherwise -> falls back to weight 0.0f (line 43) and LayoutAlignment::START (line 49). - FlexLayoutManager: all accessors call FlexLayoutParamsImpl::Get with defaults grow 0.0f / shrink 1.0f / AUTO / WRAP_CONTENT…

    - `intent`(intentional-design [보정:low]): The cited code is all accurate. SetLayoutParams (view-impl.cpp:2170-2175) stores the trait under the params' OWN GetTraitId() (line 2173), regardless of parent layout type — no parent consultation. Each manager reads only its own reserved trait id and falls back to a hardcoded default: StackLayoutManager via StackLayoutParamsImpl::Get querying STACK_LAYOUT_PARAMS (stack-layout-manager.cpp:42-43,48-49 -> stack-layout-params-impl.h:98-103), Flex via FlexLayoutParamsImpl::Get (flex-layout-manager.cpp:40-58), Absolute via AbsoluteLayoutParamsImpl::Get (absolute-layout-manager.cpp:95,201). GridLayoutParamsImpl reports GRID_LAYOUT_PARAMS (grid-layo…

- **재현**: StackLayout child: child.SetLayoutParams(GridLayoutParams::New().SetRow(2).SetColumn(3)); — laid out as if no params were set; the grid intent is silently dropped.

- **▶ 수정 방향**: `stack-layout-manager.cpp:42` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-layout-params-lp-04"></a>

#### 🟡 MEDIUM · `lp-04` — Stale params of other types persist across reparenting and resurrect (no trait cleanup on layout change)

- **위치**: `view-impl.cpp:2170-2175 (SetLayoutParams only adds/replaces its own id, never clears siblings); no RemoveTrait of *_LAYOUT_PARAMS in OnChildAdd/OnChildRemove (grep of view-impl.cpp/view-data-impl.cpp shows none)`

- **현상(버그)**: SetLayoutParams keys the trait by params type and never clears other layout-params traits; there is no trait removal in the child add/remove path. A View that held GridLayoutParams keeps that GRID_LAYOUT_PARAMS trait even after being moved under a StackLayout and given StackLayoutParams. If later reparented back under a GridLayout, the OLD grid params silently take effect again.

- **근거 — 일반 프레임워크 기대동작**: Android resolves params at attach time via generateLayoutParams; stale params for a layout type the child is no longer in do not silently re-activate on a later reparent. A child's effective params should reflect its current parent, not an accumulation of every layout type it has ever lived under.

- **근거 — 프레임워크 레퍼런스**: Android ViewGroup attach-time generateLayoutParams

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:low]): Mechanical claims VERIFIED against /home/jae/dali/dali-ui-claude source:  1. SetLayoutParams keys by the params' OWN trait id and never clears other layout-param traits — confirmed at view-impl.cpp:2170-2175: `mImpl->SetTrait(paramsImpl.GetTraitId(), ToTraitObject(params))`. ToTraitId (view-impl.cpp:2144-2159) maps each LayoutParamsType to a distinct ReservedTraitId (ABSOLUTE/STACK/GRID/FLEX_LAYOUT_PARAMS), so each type lives in its own independent trait slot.  2. Each manager reads its own type's trait via Get(), returning it whenever present, e.g. grid-layout-params-impl.h:171-176 returns the GRID_LAYOUT_PARAMS trait if attached. So a stale…

    - `code`(confirmed [보정:medium]): I re-read the cited code (line numbers shifted vs. the finding because the file is public-api/view-impl.cpp, but the logic matches) and confirmed every link in the claimed chain.  1. SetLayoutParams only sets the trait keyed by the params' own type and clears nothing else. view-impl.cpp:2170-2175: `SetLayoutParams` does `mImpl->SetTrait(paramsImpl.GetTraitId(), ToTraitObject(params))`. ToTraitId/GetTraitId map each LayoutParamsType to a distinct reserved id (ABSOLUTE/STACK/GRID/FLEX_LAYOUT_PARAMS — view-impl.cpp:2144-2159). So setting StackLayoutParams writes STACK_LAYOUT_PARAMS and leaves any pre-existing GRID_LAYOUT_PARAMS trait untouched. …

- **재현**: child.SetLayoutParams(GridLayoutParams::New().SetColumn(5)); move child to a StackLayout, set stack params; later move child back into a GridLayout — column 5 silently reapplies although the developer never re-set grid params for this placement.

- **▶ 수정 방향**: `view-impl.cpp:2170-2175` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-layout-params-lp-05"></a>

#### 🟢 LOW · `lp-05` — SetLayoutParams with an empty/uninitialized handle dereferences a null BaseObject (crash, no guard)

- **위치**: `view-impl.cpp:2170-2175 (no `if(!params)` guard before GetBaseObject); contrast ToTraitObject which DOES guard `if(!handle) return nullptr` (view-impl.cpp:103-106) — the guard exists one layer down but the GetBaseObject() above it already crashed`

- **현상(버그)**: SetLayoutParams unconditionally calls params.GetBaseObject() and static_casts it (view-impl.cpp:2172) with no validity check. An empty LayoutParams handle (e.g. LayoutParams{} or a default-constructed StackLayoutParams, stack-layout-params.cpp:32-34) has no BaseObject; GetBaseObject() on an empty BaseHandle asserts/derefs null. There is no way to clear params via SetLayoutParams({}) — it crashes instead of being a no-op or a removal.

- **근거 — 일반 프레임워크 기대동작**: Passing an empty handle should be a defined operation: either a no-op, a clear (Android allows replacing params), or a clean assert with a clear message. WPF ClearValue exists for attached properties. Here it is an unchecked null deref.

- **근거 — 프레임워크 레퍼런스**: WPF DependencyObject.ClearValue; Android setLayoutParams(null) throwing a documented NPE

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read the cited code at /home/jae/dali/dali-ui-claude and the finding holds.  1. view-impl.cpp:2170-2175 — `ViewImpl::SetLayoutParams(Ui::LayoutParams params)` unconditionally does `auto& paramsImpl = static_cast<Internal::LayoutParamsImpl&>(params.GetBaseObject());` at line 2172 with NO `if(!params)` validity guard, exactly as claimed. (Note: actual path is dali-ui-foundation/public-api/view-impl.cpp, not the bare "view-impl.cpp" the finding wrote, but the line number 2170/2172 matches precisely.)  2. BaseHandle::GetBaseObject() (dali-core base-handle.cpp:69-72) returns `static_cast<BaseObject&>(*mObjectHandle)` with no null check. On an…

    - `intent`(confirmed [보정:low]): Re-read the cited code in the worktree. The structural facts hold and the crash is real:  1) view-impl.cpp:2170-2174 (file is at dali-ui-foundation/public-api/view-impl.cpp, not the path the finding cited): SetLayoutParams unconditionally does `auto& paramsImpl = static_cast<Internal::LayoutParamsImpl&>(params.GetBaseObject());` with no `if(!params)` guard.  2) GetBaseObject on an empty handle is UB. dali-core base-handle.cpp:69-71: `BaseObject& BaseHandle::GetBaseObject() { return static_cast<BaseObject&>(*mObjectHandle); }`. With an empty handle mObjectHandle is null, so this dereferences null before any downstream guard runs. The finding's…

- **재현**: view.SetLayoutParams(StackLayoutParams{}); or view.SetLayoutParams(LayoutParams{}); — null BaseObject access rather than a defined clear/no-op.

- **▶ 수정 방향**: `view-impl.cpp:2170-2175` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-layout-params-lp-06"></a>

#### 🟢 LOW · `lp-06` — Sharing/mutation/foreign-type/reparent semantics are entirely untested in UTCs

- **위치**: `utc-Dali-StackLayout.cpp:162-163,173-176,218-221 (all fresh New(), get==set only); grep of automated-tests shows no New(other)/shared-handle/mutation-after-set tests`

- **현상(버그)**: Every params usage in the UTCs constructs a fresh New() per child and asserts only get-back-what-was-set (e.g. utc-Dali-StackLayout.cpp:162-163, 218-221). No test covers: setting one handle on two views and mutating it, post-SetLayoutParams mutation invalidation, setting a foreign params type, or params surviving reparent. The most error-prone behaviors of this subsystem have no coverage, so regressions in lp-01..lp-04 would not be caught.

- **근거 — 일반 프레임워크 기대동작**: A subsystem whose central documented warning is about shared-by-reference state (docs/layout-structure.md:58) should have at least one test pinning that behavior (and ideally a test that New(other) yields independence).

- **근거 — 프레임워크 레퍼런스**: n/a (test-coverage gap)

- **근거 — 검증자 코드분석**:

    - `intent`(confirmed [보정:low]): Re-read the cited code under /home/jae/dali/dali-ui-claude. The core gap is real but the finding is partly overstated.  CONFIRMED facts: (1) Cited UTCs match exactly — utc-Dali-StackLayout.cpp:162-163, 173-176, 218-221 all construct fresh StackLayoutParams::New() per child and assert get==set only. (2) Sharing-by-reference is real: ViewImpl::SetLayoutParams (dali-ui-foundation/public-api/view-impl.cpp:2170-2175) stores the handle as-is via ToTraitObject(params) with NO deep copy, then InvalidateMeasure(); two views given the same handle share state. This matches the documented warning at docs/layout-structure.md:58 and the New(other) copy gui…

- **반론(소수의견)**: `code`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: Refactor SetLayoutParams to deep-copy (fixing lp-01) — no UTC fails, so the behavior change is invisible to CI; conversely the current sharing footgun is never asserted as intended.

- **▶ 수정 방향**: `utc-Dali-StackLayout.cpp:162-163` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


### Layout 트랜지션 (`layout-transition`) — 2건


<a id="fix-layout-transition-lt-02"></a>

#### 🟢 LOW · `lt-02` — Animator wall-clock delta is clamped per tick (MAX_TICK_DELTA), so a frame hitch stretches the transition in slow motion instead of catching up

- **위치**: `dispatcher:1694-1701 (MAX_TICK_DELTA=0.1f, clampedDeltaSec=min(delta,0.1)), 1719 (elapsed += clampedDeltaSec); header comment 437-441 frames it only as idle-jump protection`

- **현상(버그)**: TickAnimators clamps deltaSec to 0.1s before adding it to elapsed (dispatcher:1700-1701, 1719). After any pause/hitch longer than 100ms (e.g. 200ms GC stall, app backgrounded, debugger break), an in-flight animator advances only 100ms of progress that tick; the remaining real time is permanently lost, so the transition runs slower than its configured wall-clock duration.

- **근거 — 일반 프레임워크 기대동작**: Canonical animation clocks (Android ValueAnimator, WPF Clock/Storyboard) are absolute-time-driven: after a stall the animation jumps to the progress corresponding to real elapsed time (or clamps to the end), it never runs in slow motion. The clamp here trades correctness-of-duration for avoiding a single end-snap.

- **근거 — 프레임워크 레퍼런스**: Android ValueAnimator AnimationHandler (absolute getFrameTime); WPF Timeline clock

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read the cited code and the data flow holds exactly as the finding claims.  CLAMP MECHANICS (confirmed): layout-transition-dispatcher.cpp:1689-1692 computes deltaSec = (now - mLastTickTime) from steady_clock and then resets mLastTickTime = now. Line 1700-1701: `constexpr float MAX_TICK_DELTA = 0.1f;` and `const float clampedDeltaSec = std::min(deltaSec, MAX_TICK_DELTA);`. Line 1719: `entry.second.elapsed += clampedDeltaSec;` for every non-freshlyCreated active animator. So a tick after a >100ms stall advances elapsed by at most 0.1s.  PERMANENT LOSS / SLOW MOTION (confirmed): `elapsed` is a pure monotonic accumulator of clamped deltas. T…

    - `intent`(confirmed [보정:low]): Mechanical facts verified against source at /home/jae/dali/dali-ui-claude/dali-ui-foundation/internal/layouts/layout-transition-dispatcher.cpp: - Line 1700: `constexpr float MAX_TICK_DELTA = 0.1f;` and 1701: `clampedDeltaSec = std::min(deltaSec, MAX_TICK_DELTA);` — exactly as cited. - Line 1719: `entry.second.elapsed += clampedDeltaSec;` — the clamped delta is added to per-animator elapsed unconditionally for all non-freshly-created animators. The lost real time is never recovered (no carry-over of `deltaSec - clampedDeltaSec`), so an animator finishes later than its configured wall-clock duration: confirmed slow-motion-on-hitch. - The end-of…

- **재현**: A 300ms CHANGE animator is in flight; the main thread stalls 250ms. The animator only advances ~100ms that frame and finishes well after the intended 300ms wall-clock window, visibly desyncing from any concurrent dali-core spec animation (which IS absolute-time-driven), so spec-mode and animator-mode siblings drift apart.

- **▶ 수정 방향**: `dispatcher:1694-1701 (MAX_TICK_DELTA=0.1f, clampedDeltaSec=min(delta,0.1)), 1719 (elapsed += clampedDeltaSec); header comment 437-441 frames it only as idle-jump protection` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-layout-transition-lt-03"></a>

#### 🟢 LOW · `lt-03` — Spec-mode CHANGE/ENTER animations are never cancelled when their child's ancestor (not the child) is destroyed off-scene

- **위치**: `dispatcher:2032-2090 (orphan scan covers only mPendingExits + EXIT mActiveAnimators, not mActiveAnimations); mActiveAnimations is keyed by child with no parentRef (struct ActiveSpecAnimation, header 394-400)`

- **현상(버그)**: OnViewDestroyed scans mPendingExits and mActiveAnimators for ghosts whose visual PARENT was destroyed off-scene (dispatcher:2060-2090), but it does NOT scan mActiveAnimations (spec-mode CHANGE/ENTER). Those entries are keyed by child only and hold no parent reference. They self-heal on-scene because destroying an ancestor scene-disconnects the child too (child-keyed OnViewDestroyed → CancelActiveAnimation). Off-scene, where the child is not itself destroyed, no equivalent parent-scan cleans the child's in-flight spec CHANGE/ENTER.

- **근거 — 일반 프레임워크 기대동작**: Symmetry with the EXIT-ghost orphan scan: any in-flight transition whose owning context is torn down should be cancelled. AndroidX TransitionManager removes pending transitions for a destroyed scene root.

- **근거 — 프레임워크 레퍼런스**: AndroidX TransitionManager.endTransitions on scene-root teardown

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read the cited code and the finding holds on every checkable point.  1. ActiveSpecAnimation has NO parent reference. Header [layout-transition-dispatcher.h:394-400] defines only {Animation animation; Ui::LayoutTransition transition; LayoutTransitionSlot slot; TransientActorState transientState;}. By contrast GhostExit [h:405-413] carries WeakHandle<Ui::View> parent and AnimatorState [h:422-450] carries parentRef [h:433]. So mActiveAnimations [h:467] is keyed by child only with no parent linkage — it is structurally impossible to scan it by parent.  2. OnViewDestroyed's orphan-parent scans cover only EXIT state. [dispatcher.cpp:2032-2090]…

    - `intent`(confirmed [보정:info]): The finding's factual core is verified. OnViewDestroyed (dispatcher 2032-2126) calls CancelActiveAnimation(view) keyed by the destroyed view itself (2045), then orphan-scans by PARENT only mPendingExits (2060-2072) and EXIT-slot mActiveAnimators (2074-2090). It does NOT parent-scan mActiveAnimations (spec CHANGE/ENTER). The prior auditor's cited "2134" mActiveAnimations loop is actually in OnAnimationFinished (2128-2164), a different function, not an orphan scan — so the claim "OnViewDestroyed does not scan mActiveAnimations for parent-orphans" is correct. ActiveSpecAnimation (header 394-400) holds only {animation, transition, slot, transient…

- **재현**: A transition-bearing parent P has child C with an in-flight CHANGE spec animation. P is reparented out of the scene (off-scene) and destroyed while the app retains a handle to C and re-parents C elsewhere. C's old AAnimateTo continues toward stale bounds until it finishes, and OnFinished fires for the old transition on the resurrected child. (Lower confidence: requires C to remain alive and not scene-disconnect.)

- **▶ 수정 방향**: `dispatcher:2032-2090 (orphan scan covers only mPendingExits + EXIT mActiveAnimators, not mActiveAnimations); mActiveAnimations is keyed by child with no parentRef (struct ActiveSpecAnimation, header 394-400)` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


### ScrollView & BoundsEffects (`scroll-bounds-effects`) — 3건


<a id="fix-scroll-bounds-effects-scroll-01"></a>

#### 🟠 HIGH · `scroll-01` — Cross-axis content is measured unbounded; overflow on the non-scroll axis is silently clipped and unreachable

- **위치**: `scroll-view-layout-manager.cpp:78-89 (both axes float::max); scroll-view-impl.cpp:1084-1090 (cross-axis movement zeroed), 1787-1788 (cross axis not counted scrollable), 132 (clip to box)`

- **현상(버그)**: In Measure, the non-MATCH_PARENT branch sets the child constraint to std::numeric_limits<float>::max() on BOTH axes (scroll-view-layout-manager.cpp:81-82), regardless of mScrollDirection. For a Vertical ScrollView with content 800x1200 in a 400x600 viewport (samples/scrollview/scrollview-layout-test.cpp:46-58), maxWidth becomes 800 and SetScrollableWidth(800) is called (line 145). But ScrollViewImpl only treats an axis as scrollable when its direction permits it (scroll-view-impl.cpp:1787-1788 mHasScrollableArea; AdjustScrollPosition clamps X to maxScrollX only via direction-gated callers). With Vertical direction, horizontal pan/wheel is filtered out (AdjustMovement, scroll-view-impl.cpp:1084-1090), so the extra 400px of width is rendered, clipped by CLIP_TO_BOUNDING_BOX (scroll-view-impl.cpp:132), and can never be brought into view.

- **근거 — 일반 프레임워크 기대동작**: A canonical scroll container measures the cross (non-scrolling) axis bounded to the viewport (AT_MOST/exact), so the child lays out to fit the cross dimension (text wraps, MATCH-style children fill). Only the scroll axis is measured unbounded. Android NestedScrollView measures the cross axis with the parent's MeasureSpec (EXACTLY/AT_MOST viewport); CSS overflow-y:scroll keeps the content box width = container width.

- **근거 — 프레임워크 레퍼런스**: Android View.MeasureSpec UNSPECIFIED only along scroll axis (NestedScrollView.measureChildWithMargins); CSS overflow scrollable-overflow region; WPF ScrollViewer CanContentScroll axis-specific infinity

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:high]): Every cited code fact holds in /home/jae/dali/dali-ui-claude.  (1) scroll-view-layout-manager.cpp:81-82 — In Measure, the non-MATCH_PARENT branch sets the child constraint to std::numeric_limits<float>::max() on BOTH axes, and Measure never references mScrollDirection (it has no access to ScrollViewImpl during Measure; the manager only down-casts to ScrollViewImpl in Arrange at line 102). So a Vertical ScrollView measures the cross (horizontal) axis unbounded exactly as the non-scroll axis. Confirmed.  (2) scroll-view-layout-manager.cpp:84-92 + Arrange:121-146 — For the explicit-width (800) content child, effectiveWidth = childSize.width = 80…

    - `intent`(intentional-design [보정:low]): Code matches the observed mechanism. In scroll-view-layout-manager.cpp:81-82 both axes are constrained to float::max() when the child is not MATCH_PARENT, independent of mScrollDirection; Arrange sets BOTH SetScrollableWidth/Height (lines 145-146). On the impl side, with Vertical direction the cross axis is gated out: AdjustMovement zeroes movX (scroll-view-impl.cpp:1084-1086), mHasScrollableArea only counts the vertical axis (1787-1788), and the canvas is clipped via CLIP_TO_BOUNDING_BOX (line 132). So horizontal overflow in a Vertical ScrollView is indeed rendered, clipped, and unreachable — the mechanism is real.  However, the cross-axis-u…

    - `code2`(confirmed [보정:medium]): I re-derived the behavior from the actual worktree source and every load-bearing mechanical claim holds.  (1) Unbounded both-axis measure: /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/scroll-view-layout-manager.cpp:78-89. For a non-MATCH_PARENT child the constraint is std::numeric_limits<float>::max() on BOTH axes regardless of mScrollDirection (lines 81-82). mScrollDirection is not referenced anywhere in this file (grep). So a child with explicit width 800 measures at 800 (line 84), effectiveWidth=800, maxWidth=800 (lines 86-88).  (2) Arrange sets scrollableWidth from the unbounded measured width: scroll-view-layout-ma…

- **재현**: Vertical ScrollView, viewport 400x600, content View with explicit width 800 and height 1200. Result: content laid out at 800 wide, right 400px clipped and unreachable because horizontal scrolling is disabled. A WRAP_CONTENT text child would also be measured at float::max() width and never wrap.

- **▶ 수정 방향**: `scroll-view-layout-manager.cpp:78-89` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-scroll-bounds-effects-scroll-02"></a>

#### 🟡 MEDIUM · `scroll-02` — Manager Arrange double-applies effective scale to the content's scroll offset

- **위치**: `scroll-view-layout-manager.cpp:104 (s = GetEffectiveScale), 123-124 (GetPositionX()*s); view-impl.cpp:665-668 (GetPositionX reads live POSITION); view-impl.cpp:1820-1824 (ApplyScrollPosition writes final visual POSITION)`

- **현상(버그)**: Arrange computes childBounds.x = child.GetPositionX() * s and childBounds.y = child.GetPositionY() * s (scroll-view-layout-manager.cpp:123-124, s = view->GetEffectiveScale()). GetPositionX() returns the LIVE Actor::Property::POSITION_X (view-impl.cpp:665-668), which for the content is the scroll offset written by ApplyScrollPosition as mMaximumStart - scrollPos (scroll-view-impl.cpp:1820-1824) — already a final visual pixel value. Multiplying that already-visual position by s scales it a second time. Everywhere else the layout system positions children from natural-unit requested position times s (e.g. view-impl.cpp:1144 visPadLeft + visMarginStart + GetRequestedPositionX()*s), never from live POSITION times s.

- **근거 — 일반 프레임워크 기대동작**: Position fed to Arrange should be in the same unit space as the rest of the layout (natural requested position * scale, or the bounds-relative content origin). Reading an already-scaled live POSITION and scaling again breaks when effective scale != 1, displacing/compounding the scroll offset by the scale factor on every relayout.

- **근거 — 프레임워크 레퍼런스**: Layout arrange must use a single consistent coordinate/scale space for child placement (WPF Arrange uses the same DIP space throughout; Android layout() uses pixel coordinates uniformly)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I traced the cited code in /home/jae/dali/dali-ui-claude and the "observed" control/data flow holds.  1. scroll-view-layout-manager.cpp:104,123-124: Arrange reads s = view->GetEffectiveScale() and sets childBounds.x = child.GetPositionX() * s, childBounds.y = child.GetPositionY() * s. Confirmed verbatim.  2. child.GetPositionX() (View handle) -> view.cpp:256 GetImpl(*this).GetPositionX() -> view-impl.cpp:665-668 returns the LIVE Actor::Property::POSITION_X. Confirmed it reads the live position, not a requested/natural value.  3. scroll-view-impl.cpp:1816-1824 ApplyScrollPosition writes mContent's POSITION_X = mMaximumStartX - position.x. That…

    - `intent`(confirmed [보정:medium]): Re-verified the cited chain. scroll-view-layout-manager.cpp:104,123-124 computes childBounds.x = child.GetPositionX() * s, and View/ViewImpl::GetPositionX() returns the LIVE Actor::Property::POSITION_X (view-impl.cpp:665-668, view.cpp:254-257) — not a requested/natural position. For the scroll content that live POSITION is written by ApplyScrollPosition as mMaximumStartX - scrollPos (scroll-view-impl.cpp:1816-1824), which is treated as a final visual coordinate: the values bounding it (mViewportWidth from SIZE_WIDTH, scroll-view-impl.cpp:1775; mScrollableWidth from Arrange's childBounds.width which is bounds-derived/visual) are visual. The ge…

- **재현**: Enable UiScalePolicy::ENABLED with scale 2.0 on a ScrollView, scroll content to offset, then trigger a relayout (e.g. content relayout). Arrange repositions content at offset*2, jumping the scroll. With default scale 1.0 the bug is dormant, which is why existing tests pass.

- **▶ 수정 방향**: `scroll-view-layout-manager.cpp:104` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-scroll-bounds-effects-scroll-03"></a>

#### 🟡 MEDIUM · `scroll-03` — Manager Arrange ignores the content-box origin (padding) and depends on an external imperative scroll model for placement

- **위치**: `view-impl.cpp:2471-2477 (content-box bounds with padding origin handed to manager); scroll-view-layout-manager.cpp:123-124 (origin ignored, live POSITION used); scroll-view-impl.cpp:1772-1773,1816-1824 (host re-derives padding and applies offset)`

- **현상(버그)**: The dispatcher passes a content bounds whose origin is the ScrollView padding (view-impl.cpp:2472-2475: x=padding.start*s, y=padding.top*s). The manager ignores bounds.x/bounds.y for placement and instead positions the content at its live POSITION (scroll-view-layout-manager.cpp:123-124). The content's correct resting position (padding offset minus scroll) is supplied out-of-band by ScrollViewImpl::ApplyScrollPosition using mMaximumStartX = padding.start (scroll-view-impl.cpp:1772-1773, 1820-1821). So on the very first Arrange before any UpdateScrollingProperties runs, content sits at POSITION 0 (no padding applied) until SetScrollableWidth/Height fires UpdateScrollingProperties→ApplyScrollPosition. The two padding sources (manager bounds origin vs. mMaximumStart) must stay in agreement but are computed in different files.

- **근거 — 일반 프레임워크 기대동작**: A LayoutManager's Arrange is the single authority for child placement and should place content relative to the bounds it is given (bounds.x/y = content origin), with the scroll offset added as a manager-owned value. Splitting placement between the manager (ignores bounds origin) and the host impl (owns the offset, re-derives padding) is surprising and fragile.

- **근거 — 프레임워크 레퍼런스**: WPF ScrollContentPresenter.ArrangeOverride places content at (-HorizontalOffset, -VerticalOffset) relative to its own arrange rect; Android ScrollView.onLayout uses scrollX/scrollY as the single offset model

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): The core "observed" claim holds against the source. (1) The dispatcher computes a content-box origin from padding: view-impl.cpp:2472-2473 `visContentBounds.x = padding.start*s; .y = padding.top*s`, handed to manager->Arrange at :2477. (2) The ScrollView manager ignores that bounds origin and positions the content from its live actor POSITION: scroll-view-layout-manager.cpp:123-124 `childBounds.x = child.GetPositionX()*s; childBounds.y = child.GetPositionY()*s` — bounds.x/bounds.y are only consulted for MATCH_PARENT width/height (lines 130-135), never for placement. (3) The padding-derived resting offset is supplied out-of-band by the host: s…

    - `intent`(intentional-design [보정:low]): Mechanics confirmed but the defect framing is wrong. ScrollViewLayoutManager::Arrange sets childBounds.x/y from child.GetPositionX/Y()*s (scroll-view-layout-manager.cpp:123-124), ignoring the padding-origin content bounds the dispatcher hands it (view-impl.cpp:2471-2475: x=padding.start*s, y=padding.top*s). Content resting position is instead owned by the scroll model: ApplyScrollPosition sets POSITION = mMaximumStartX/Y - scroll, with mMaximumStartX/Y = padding.start/top (scroll-view-impl.cpp:1772-1773, 1816-1824). This split is INTENTIONAL: the scroll model must move content every frame during pan/fling/animation (scroll-view-impl.cpp:1537-…

- **재현**: Set non-zero padding on a ScrollView whose content fits the viewport (scrollable == viewport so UpdateScrollingProperties' clamp path may early-out when SetScrollableWidth sees < 0.01f change): the manager leaves content at whatever live POSITION it had rather than at the padding origin, so initial padding placement relies entirely on the host's first ApplyScrollPosition.

- **▶ 수정 방향**: `view-impl.cpp:2471-2477` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


### [횡단] MATCH/WRAP 매트릭스 (`x-matchparent-wrapcontent-matrix`) — 6건


<a id="fix-x-matchparent-wrapcontent-matrix-layout-02"></a>

#### 🟠 HIGH · `layout-02` — FlexLayout MATCH_PARENT on the main axis overrides and double-counts flex-grow

- **위치**: `flex-layout-manager.cpp:272-278 (override), :133-159 (grow distribution includes it), :518 Measure adds only min so line.mainSize underestimates`

- **현상(버그)**: In flex Arrange, a main-axis MATCH_PARENT child participates in flex-grow free-space distribution (ApplyFlexGrowShrink uses workingSizes seeded from measured min ~0 and adds grow share, flex-layout-manager.cpp:139-159), but then ArrangeOneFlexLine hard-overwrites its main size to the full available main minus margin (flex-layout-manager.cpp:272-278), discarding the grow result. Meanwhile its allocMain (childMainSize+marginMain) is the full line, so a sibling with flex-grow already consumed the free space computed from line.mainSize that excluded this child's true width. The MATCH_PARENT child overlaps siblings and the grow math is computed against an inconsistent line size.

- **근거 — 일반 프레임워크 기대동작**: MATCH_PARENT main and flex-grow should not both apply; CSS resolves a single used main size per item. Either MATCH_PARENT should be treated as flex-basis:100% feeding the normal grow/shrink algorithm, or excluded from it — not both.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox 9.7 resolve flexible lengths; flex-basis vs definite main size

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:high]): Re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp and the MATCH_PARENT measure path at view-impl.cpp.  The finding's core mechanics hold against the actual source:  1) A non-standalone main-axis MATCH_PARENT child DOES participate in flex-grow distribution. BuildFlexLinesForArrange adds its grow to the line total (flex-layout-manager.cpp:120) and ApplyFlexGrowShrink adds a grow share `extra` into its workingSizes when grow>0 (flex-layout-manager.cpp:144-156). There is no MATCH_PARENT exclusion.  2) The line main size underestimates that child. Arrange seeds workingSizes from …

- **재현**: Row FlexLayout width 300 with child A (width 100) and child B SetRequestedWidth(MATCH_PARENT) SetFlexGrow(1). B is set to 300 (full) and overlaps A; grow free-space was computed from a line.mainSize that treated B as ~0.

- **▶ 수정 방향**: `flex-layout-manager.cpp:272-278` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-x-matchparent-wrapcontent-matrix-layout-03"></a>

#### 🟡 MEDIUM · `layout-03` — ScrollView reports full viewport (not minimum) for a MATCH_PARENT child in Measure, violating the follower contract

- **위치**: `scroll-view-layout-manager.cpp:78-92 vs docs/layout-structure.md:131-139, 162; contrast stack-layout-manager.cpp:96-105`

- **현상(버그)**: ScrollViewLayoutManager::Measure sets effectiveWidth/Height = widthConstraint/heightConstraint for a MATCH_PARENT child and folds it into maxWidth/maxHeight (scroll-view-layout-manager.cpp:86-89). Every other manager and plain View treat a MATCH_PARENT child as contributing its minimum (~0) to the parent's measured size (e.g. stack :98-105, flex :518, absolute :147-152, grid :122-126). So a MATCH_PARENT scroll-content child drives the scroll container's measured size to the full viewport instead of 0/min.

- **근거 — 일반 프레임워크 기대동작**: docs/layout-structure.md:131-139 states a MATCH_PARENT view reports minimum size in Measure and a WRAP_CONTENT parent determines its size exclusively from non-MATCH_PARENT children. ScrollView contradicts that universal claim.

- **근거 — 프레임워크 레퍼런스**: Documented DALi 'MATCH_PARENT follower' rule; Android ScrollView measures child with UNSPECIFIED but child MATCH_PARENT still resolves to viewport only after parent size known

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Re-read scroll-view-layout-manager.cpp:78-92 directly. The code does exactly what the finding claims: for a MATCH_PARENT child it sets effectiveWidth/effectiveHeight = widthConstraint/heightConstraint (lines 86-87) and folds those into maxWidth/maxHeight via std::max (lines 88-89), which become the returned MeasuredSize (line 92). So a MATCH_PARENT content child drives ScrollView's measured size to the full incoming constraint (viewport), not the minimum.  I verified this violates the universal follower contract by checking the child-level Measure at view-impl.cpp:2444-2445 and 2452-2453: a MATCH_PARENT view's own Measure returns GetMinimumWi…

    - `intent`(confirmed [보정:medium]): Re-read scroll-view-layout-manager.cpp:78-92: for a MATCH_PARENT child the manager sets effectiveWidth/Height = widthConstraint/heightConstraint (the full incoming constraint = viewport) and folds it into maxWidth/maxHeight, returning the viewport rather than the child's reported minimum. The contrast is real: stack-layout-manager.cpp:94-104 accumulates childSize (the child's own measured value, ~min for MATCH_PARENT), so MATCH_PARENT contributes ~0 there. The universal contract in docs/layout-structure.md:131-139 states a MATCH_PARENT view reports its minimum (default 0) in Measure and a WRAP_CONTENT parent sizes itself exclusively from non-…

- **재현**: ScrollView (or its content host) measured under a WRAP_CONTENT parent with a single MATCH_PARENT content child: ScrollView measures to the viewport size rather than 0, so a WRAP ancestor expands to the viewport.

- **▶ 수정 방향**: `scroll-view-layout-manager.cpp:78-92` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-x-matchparent-wrapcontent-matrix-layout-04"></a>

#### 🟡 MEDIUM · `layout-04` — GridLayout silently ignores per-child START/CENTER/END alignment for MATCH_PARENT cells

- **위치**: `grid-layout-manager.cpp:300-320, 322-342, guard at :301 and :323; MATCH child measured size 0 from view-impl.cpp:1000-1003`

- **현상(버그)**: ArrangeGridChildrenToCells applies horizontal/vertical alignment only inside the guard `if(childWidth > 0.0f && childWidth < cellWidth)` / `if(childHeight > 0.0f ...)` using the child's measured size (grid-layout-manager.cpp:296-301, 322-323). A MATCH_PARENT child has measured size 0 (follower), so the guard is false and the alignment switch is skipped entirely — the child always fills the cell regardless of a START/CENTER/END GridLayoutParams alignment the developer set.

- **근거 — 일반 프레임워크 기대동작**: Alignment set by the developer should be honored or explicitly documented as no-op for MATCH_PARENT. Docs only say cells are FILL 'by default' (layout-structure.md:159-160), implying an explicit non-FILL alignment would take effect.

- **근거 — 프레임워크 레퍼런스**: WPF Grid HorizontalAlignment/VerticalAlignment honored per-child; Android GridLayout gravity

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): I re-read the cited code and the control flow holds.  grid-layout-manager.cpp:296-298 reads the child's measured size into childWidth/childHeight. The alignment switches are gated at :301 `if(childWidth > 0.0f && childWidth < cellWidth)` and :323 `if(childHeight > 0.0f && childHeight < cellHeight)` (verified :300-342). GetChildHorizontalAlignment/VerticalAlignment (:64-74) return the developer-set GridLayoutParams alignment (default FILL), so the gate is what decides whether a non-FILL alignment is applied.  For a MATCH_PARENT child, Measure returns GetMinimumWidth()/GetMinimumHeight() (view-impl.cpp:1000-1003 in the layout-manager branch, an…

    - `intent`(intentional-design [보정:info]): The mechanism described is real but the behavior is intentional, and the framework "expected" claim does not establish a genuine deviation.  Mechanism (confirmed): A MATCH_PARENT child reports measured size = GetMinimumWidth()/GetMinimumHeight() (default 0) — view-impl.cpp:1000-1003 and 1028-1043. In ArrangeGridChildrenToCells the alignment switch is gated by `if(childWidth > 0.0f && childWidth < cellWidth)` (grid-layout-manager.cpp:301) and the vertical analog at :323. With measured size 0 the guard is false, so START/CENTER/END is skipped and the child fills the cell (it is also re-Measured to the cell bounds at :344-346). So the observed b…

- **재현**: GridLayout cell with a MATCH_PARENT child and GridLayoutParams horizontal alignment CENTER: child fills the cell instead of centering.

- **▶ 수정 방향**: `grid-layout-manager.cpp:300-320` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-x-matchparent-wrapcontent-matrix-layout-05"></a>

#### 🟡 MEDIUM · `layout-05` — FlexBasis > 0 overrides MATCH_PARENT minimum in Measure, so a basis-sized MATCH_PARENT child DOES contribute to a WRAP parent

- **위치**: `flex-layout-manager.cpp:490-501 then :503,:518 accumulate into line.mainSize; default basis is WRAP_CONTENT(-1) so only triggers when basis explicitly set >0 (flex :56-60)`

- **현상(버그)**: In flex Measure, after measuring the child to its min (~0 for MATCH_PARENT), if FlexBasis>0 the working main size is set to basis*scale (flex-layout-manager.cpp:490-501) and that becomes childMainSize contributing to line.mainSize and thus totalMainSize. So a MATCH_PARENT main-axis child with a positive flex-basis no longer reports minimum — it contributes `basis` to a WRAP_CONTENT flex parent, contradicting the follower rule for that child.

- **근거 — 일반 프레임워크 기대동작**: Per docs a MATCH_PARENT view is a follower reporting minimum in Measure; the basis override silently re-introduces it into parent sizing. CSS would use flex-basis as the hypothetical main size but the item's used size is still bounded by the container; under a shrink-to-fit container this is subtle, but DALi's stated invariant is 'MATCH_PARENT contributes min'.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox flex-basis as hypothetical main size

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): The "observed" claim is an accurate reading of the code. In FlexLayoutManager::Measure (flex-layout-manager.cpp:489-501), after measuring the child to its reported size (childSize), the code sets workingSize = childSize, then if GetFlexBasis(childImpl) > 0.0f it overwrites workingSize.width (horizontal main axis) or workingSize.height with basis*childScale. childMainSize is then computed from workingSize (lines 503-504) and accumulated into currentLine.mainSize (line 518), which feeds totalMainSize (line 532) and the returned result (lines 539/545). So a positive flex-basis replaces the child's measured value with basis in the parent's measur…

- **반론(소수의견)**: `intent`=uncertain — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: WRAP_CONTENT row FlexLayout with one child SetRequestedWidth(MATCH_PARENT) and FlexLayoutParams basis=50: parent measures width 50 instead of 0.

- **▶ 수정 방향**: `flex-layout-manager.cpp:490-501` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-x-matchparent-wrapcontent-matrix-layout-06"></a>

#### 🟢 LOW · `layout-06` — Grid implicit (definition-less) track is treated as Star(1) and fills, so a MATCH_PARENT-only implicit grid behaves differently from an explicit AUTO track

- **위치**: `grid-layout-manager.cpp:118-119 (auto-like floor) vs :163-166 and :206-212 (Star fill); comment at :114-119`

- **현상(버그)**: An axis with no definitions is treated as auto-like for the content floor (grid-layout-manager.cpp:118-119) but as Star(1) for sizing (grid-layout-manager.cpp:163-166, 206-212, 221-227), filling the definite available space. An explicitly declared AUTO track instead stays at the content floor. So for a grid with only MATCH_PARENT cells: an implicit track collapses to 0 only because starAvailable is 0 under WRAP (Measure :461-475), but under a definite parent the implicit track fills while an explicit AUTO track stays 0. The two 'auto' notions diverge.

- **근거 — 일반 프레임워크 기대동작**: AUTO and implicit single-track behavior should coincide, or the difference should be documented. Docs say 'an axis with no definitions falls back to a single implicit track' and 'MATCH_PARENT children do not drive AUTO sizing' (layout-structure.md:159) without noting implicit==Star.

- **근거 — 프레임워크 레퍼런스**: WPF Grid: undefined RowDefinition defaults to Star(1*); explicit Auto is content-sized — DALi mirrors WPF here but it conflicts with the doc's 'implicit == auto track' wording

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I traced the full grid sizing flow in /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp.  Implicit (definition-less) track: MeasureGridChildrenAndFillAuto sets rowAutoLike/colAutoLike when rowDefs.Size()==0 (line 118-119) and captures the child content as a floor (lines 122, 126). Then ApplyGridDefinitions treats the implicit track as Star(1): for i>=colDefs.Size() it adds 1.0f to totalStar (lines 163-166, 189-191) and in the star pass grows it to fill remaining space, never below the floor: colWidths[i]=max(colWidths[i], share) (lines 206-212, 221-227).  Explicit AUTO track: rowAutoLike/colAutoLike i…

    - `intent`(intentional-design [보정:info]): The code matches the observed behavior, and it is a deliberate design choice. The implicit (definition-less) track is computed as a content floor in MeasureGridChildrenAndFillAuto via rowAutoLike/colAutoLike when rowDefs.Size()==0u (grid-layout-manager.cpp:118-119, 120-127), then promoted to Star(1) in ApplyGridDefinitions (grid-layout-manager.cpp:163-166, 189-190) and filled while never shrinking below the content floor (:206-212, 221-227). Under WRAP the implicit track collapses because starAvailable accumulation treats it as non-star (:466, :482). An explicit AUTO track stays at the content floor (no star pass). So implicit-Star-fill vs ex…

- **재현**: Grid with no row/col definitions and one MATCH_PARENT child under a definite 200x150 parent: the implicit track fills to 200x150; the same grid with explicit GridLength::Auto() row/col leaves the track at 0.

- **▶ 수정 방향**: `grid-layout-manager.cpp:118-119` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-x-matchparent-wrapcontent-matrix-layout-07"></a>

#### 🟢 LOW · `layout-07` — AbsoluteLayout sizes a WRAP_CONTENT parent from default-positioned children's natural size, unlike CSS absolute positioning

- **위치**: `absolute-layout-manager.cpp:84-176, esp. :145-152 (w=childSize.width when w<0) and :172-173 (maxRight/maxBottom)`

- **현상(버그)**: AbsoluteLayoutManager::Measure accumulates maxRight/maxBottom = x + w + margin over ALL non-standalone children including those with default bounds (x=0,y=0,w=-1,h=-1 -> w=childSize.width) (absolute-layout-manager.cpp:99-102, 145-152, 172-173). So a WRAP_CONTENT AbsoluteLayout grows to bound its (default-positioned, overlapping) children. A MATCH_PARENT child correctly contributes 0 (w becomes childSize=min 0), staying consistent with the follower rule, but ordinary WRAP children all pile onto the same origin and the parent sizes to the largest.

- **근거 — 일반 프레임워크 기대동작**: In CSS, absolutely positioned children do not contribute to their containing block's size. DALi's absolute layout instead behaves like a bounding-box (closer to a Canvas/relative layout). This is a defensible design but surprising for anyone expecting 'absolute' semantics.

- **근거 — 프레임워크 레퍼런스**: CSS position:absolute (out of normal flow, no contribution to containing block); contrast WPF Canvas (also no auto-size from children)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): The "observed" claim holds against the actual source.  1. Default bounds: absolute-layout-manager.cpp:43 GetChildBounds returns LayoutRect(0,0,-1,-1) when params is null, so a default-positioned child has x=0,y=0,w=-1,h=-1 as claimed.  2. Negative width resolves to child natural size: lines 145-152, `if(w < 0) w = childSize.width;` (and h likewise), where childSize = childImpl.Measure(...) at line 143. Confirmed.  3. Bounding-box accumulation: lines 172-173, `maxRight = std::max(maxRight, x + w + marginW)` and `maxBottom = std::max(maxBottom, y + h + marginH)`, over ALL non-standalone children (the only skip is IsStandalone at line 89-92). Me…

    - `intent`(confirmed [보정:low]): Re-read /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/absolute-layout-manager.cpp. The observed behavior is accurate: Measure() loops all non-standalone children (lines 84-92), resolves default bounds (0,0,-1,-1) so for w<0 it sets w=childSize.width / h=childSize.height (lines 145-152), then accumulates maxRight=max(maxRight, x+w+marginW) and maxBottom likewise (lines 172-173), returning MeasuredSize(maxRight, maxBottom) (line 176). With default-positioned children all at x=y=0 this is a bounding box of overlapping children — so a WRAP_CONTENT AbsoluteLayout grows to the largest child and children overlap at origin, exac…

- **재현**: WRAP_CONTENT AbsoluteLayout with two children of size 100x100 and no bounds set: parent measures 100x100 and both children overlap at (0,0).

- **▶ 수정 방향**: `absolute-layout-manager.cpp:84-176` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


### [횡단] Measure 캐시 (`x-measure-cache`) — 5건


<a id="fix-x-measure-cache-cache-01"></a>

#### 🟠 HIGH · `cache-01` — Several Label size-affecting setters never invalidate the measure cache → stale measured size

- **위치**: `dali-ui-foundation/integration-api/label-impl.cpp:230-235,257-273,541-575,767-777; cache gate view-impl.cpp:908-912; natural-size read OnMeasure view-impl.cpp:1034,1047; InvalidateTextMeasure view-impl→InvalidateMeasure label-impl.cpp:1636-1648`

- **현상(버그)**: LabelImpl::SetFontFamily (label-impl.cpp:230-235), SetFontWeight (541-551), SetFontWidth (553-563), SetFontSlant (565-575), SetFontSizeScale (767-777), SetMultiLine (257-262), SetLineWrapMode (269-273), SetFontVariation (815-/822-), SetLetterSpacing (901-) all mutate text metrics that change GetNaturalSize() but do NOT call InvalidateTextMeasure()/InvalidateMeasure(); each carries only a placeholder comment '// InvalidateMeasure() may be called if needed.'. SetText/SetFontSize/SetLineHeight/SetTextFit DO call InvalidateTextMeasure (220/248/353/739).

- **근거 — 일반 프레임워크 기대동작**: Any property that changes a view's natural/measured size must invalidate the measure cache (WPF AffectsMeasure; Android TextView.setTypeface/setText call requestLayout()). Otherwise ViewImpl::Measure (view-impl.cpp:908-912) returns the cached size keyed only on the unchanged constraint.

- **근거 — 프레임워크 레퍼런스**: Android View.requestLayout()/PFLAG_FORCE_LAYOUT; TextView setTypeface/setTextSize call requestLayout(). WPF FrameworkPropertyMetadata.AffectsMeasure.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:high]): I re-read every cited setter in /home/jae/dali/dali-ui-claude/dali-ui-foundation/integration-api/label-impl.cpp and the cache mechanism in .../public-api/view-impl.cpp.  CONFIRMED that the cited size-affecting setters do NOT invalidate the measure cache: SetFontFamily (230-235), SetMultiLine (257-262, calls UpdateMarqueeState only), SetLineWrapMode (269-273), SetFontWeight (541-546), SetFontWidth (553-558), SetFontSlant (565-570), SetFontSizeScale (767-772), SetFontVariation(axes) (815-820) and SetFontVariation(string) which delegates to it (822-839), ClearFontVariation (846-851), SetLetterSpacing (901-905). Each font* setter carries the plac…

    - `intent`(confirmed [보정:high]): Re-verified against /home/jae/dali/dali-ui-claude. The cited setters genuinely fail to invalidate the measure cache, and this is an unintended omission, not intentional design.  Code facts: - label-impl.cpp:230-235 (SetFontFamily), 541-575 (SetFontWeight/Width/Slant), 767-777 (SetFontSizeScale), 257-262 (SetMultiLine), 269-273 (SetLineWrapMode), 815-820 (SetFontVariation) all mutate Text::Controller defaults that change natural size, and each carries only the placeholder comment "// InvalidateMeasure() may be called if needed." with NO call to InvalidateTextMeasure/InvalidateMeasure. By contrast SetText(220), SetFontSize(248), SetLineHeight(3…

    - `code2`(confirmed [보정:high]): Re-derived from source under /home/jae/dali/dali-ui-claude. The cache gate is real: ViewImpl::Measure (public-api/view-impl.cpp:908-912) returns the cached mMeasuredSize whenever mLastMeasuredConstraint.width >= 0 and FloatEqual(cached, effNatW/effNatH). The ONLY thing that resets the cache to a re-measurable state is InvalidateMeasure(), which sets mLastMeasuredConstraint = MEASURE_CACHE_DIRTY (negative sentinel) at view-impl.cpp:1305-1312. For a WRAP_CONTENT label, OnMeasure (integration-api/label-impl.cpp:1465-1533) reads GetNaturalSize() (line 1529 → 1448-1456 → mController->GetNaturalSize()), so any property feeding the controller's text…

- **재현**: WRAP_CONTENT Label with text 'Hi' is measured (cache key = constraint). Call label.SetFontFamily('NotoSans-Bold') or SetMultiLine(true). No invalidation occurs; next layout pass is a cache hit and the Label keeps its old measured size, clipping/mis-sizing the now-larger text. SetFontSize on the same label would fix it (it invalidates), exposing the inconsistency.

- **▶ 수정 방향**: `dali-ui-foundation/integration-api/label-impl.cpp:230-235` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-x-measure-cache-cache-05"></a>

#### 🟡 MEDIUM · `cache-05` — flex-grow / flex-shrink resize a child's main size but the child is NOT re-measured at the grown/shrunk size unless it is MATCH_PARENT

- **위치**: `flex-layout-manager.cpp:147-155 (grow mutates workingSizes), 348-350 (re-measure gated on MATCH_PARENT only), 352 Arrange uses grown childBounds`

- **현상(버그)**: ApplyFlexGrowShrink mutates workingSizes[idx] (flex-layout-manager.cpp:139-159 grow, 160-190 shrink) and the child is arranged at the grown bounds, but the re-measure call at line 348-350 fires only when the child is MATCH_PARENT. A flex-grow child that is WRAP_CONTENT (the normal flex-grow case) is arranged at a larger main size than it was ever measured at, so its own content/children were measured against the pre-grow (smaller) constraint.

- **근거 — 일반 프레임워크 기대동작**: CSS Flexbox resolves flexible lengths and then re-resolves each item's cross size / re-lays-out its content against the used main size (flexbox §9.4 'resolve flexible lengths' → §9.7). After distributing free space, the item must be re-measured at its used main size so nested WRAP_CONTENT content reflows.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox §9.4–§9.7 (resolve flexible lengths, then determine used cross size); Android FlexboxLayout re-measures flex items after distributing free space.

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:low]): The observed behavior is real: in flex-layout-manager.cpp, ApplyFlexGrowShrink mutates workingSizes[idx] for grow (lines 147-155) and shrink (lines 173-186), the child is arranged at the grown/shrunk bounds (line 352), but the re-measure (lines 348-350) is gated on `GetRequestedWidth()==MATCH_PARENT || GetRequestedHeight()==MATCH_PARENT` only. A WRAP_CONTENT flex-grow child is therefore arranged at a main size larger than it was ever measured at, with no re-measure of its content. That fact is accurately described by the finding.  The finding's "expected" CSS/Android claim is also substantively correct: CSS Flexbox §9.4→§9.7 resolves the used…

    - `code`(confirmed [보정:medium]): Re-read /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp. Control flow confirms the finding:  1. ApplyFlexGrowShrink (called at line 584) mutates workingSizes[idx].width/height for any child with flexGrow>0 (lines 145-156) and shrink (lines 173-187). No requested-mode gate here — it applies to WRAP_CONTENT children.  2. ArrangeOneFlexLine (called line 645) derives childMainSize from workingSizes[idx] (line 258) and builds childBounds.width/height from the grown allocMain (lines 323/341).  3. The re-measure call at lines 348-350 fires ONLY if childImpl.GetRequestedWidth()==MATCH_PARENT || GetRequested…

- **재현**: Horizontal Flex, child A flexGrow=1 and WRAP_CONTENT, containing a multiline Label. A is measured at its content width (narrow), grows to fill the row, but is never re-measured at the grown width, so the Label keeps its narrow (taller, wrapped) height even though it now has room to be one line.

- **▶ 수정 방향**: `flex-layout-manager.cpp:147-155` 에서 위 '기대동작'을 만족하도록 수정. (분류: spec-violation)


<a id="fix-x-measure-cache-cache-06"></a>

#### 🟡 MEDIUM · `cache-06` — Correctness depends on manual InvalidateMeasure at every mutation; there is no AffectsMeasure metadata or forceLayout backstop

- **위치**: `view-impl.cpp:1293-1354 (only invalidation reset), 1271-1291 (scale reset), no grep hit for forceLayout/PFLAG_FORCE_LAYOUT in the subsystem; ad-hoc setters view-data-impl.cpp:993-1497`

- **현상(버그)**: There is no per-view forceLayout/PFLAG_FORCE_LAYOUT bypass; the only cache resets are InvalidateMeasure→DIRTY (view-impl.cpp:1293-1354) and ResetEffectiveScaleRecursive→NaN (1271-1291). Whether a mutation re-measures is decided ad hoc per setter (view-data-impl.cpp:993-1497 covers margin/padding/requested/min/max/layoutMode; SetLayoutParams at view-impl.cpp:2170-2174). Anything that affects measurement but is set on a path that forgets InvalidateMeasure (e.g. cache-01) is permanently stale until an unrelated invalidation happens to occur.

- **근거 — 일반 프레임워크 기대동작**: A general UI framework either keys the cache on all measurement inputs or provides a declarative AffectsMeasure/AffectsArrange system plus a forceLayout escape hatch so a missed-invalidation bug degrades to extra work, not stale layout. DALi's cache fails-stale (wrong size) rather than fails-safe (redundant measure).

- **근거 — 프레임워크 레퍼런스**: WPF FrameworkPropertyMetadata.AffectsMeasure; Android View.requestLayout()/forceLayout() + invalidate().

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): All three factual ("observed") claims hold against the actual source at /home/jae/dali/dali-ui-claude.  1) Only cache-reset mechanisms: InvalidateMeasure sets MEASURE_CACHE_DIRTY (view-impl.cpp:1310-1313, full body 1293-1354) and ResetEffectiveScaleRecursive sets quiet_NaN (view-impl.cpp:1284-1285, body 1271-1291). The cited line ranges match exactly (note: actual path is dali-ui-foundation/public-api/view-impl.cpp, not internal, but content is identical to the citation).  2) No forceLayout/PFLAG_FORCE_LAYOUT/AffectsMeasure/AffectsArrange backstop: `grep -rniE 'forceLayout|FORCE_LAYOUT|AffectsMeasure|AffectsArrange|PFLAG'` over the entire dal…

    - `intent`(intentional-design [보정:info]): The mechanism the finding describes is accurate but the "expected" framework framing is wrong, and the design it criticizes is the deliberately documented contract.  Mechanism verified: The Measure cache hit is keyed solely on the incoming constraint (view-impl.cpp:908-912 — `mLastMeasuredConstraint` vs effNatW/effNatH; returns cached `mMeasuredSize`). The only cache resets are InvalidateMeasure→DIRTY (view-impl.cpp:1293-1354) and ResetEffectiveScaleRecursive→NaN (1271-1291). There is no forceLayout/PFLAG_FORCE_LAYOUT/AffectsMeasure anywhere in dali-ui-foundation (grep: zero hits). Setters invalidate ad hoc and only when relevant: MARGIN/PADD…

- **재현**: Any custom ViewImpl subclass whose GetNaturalSize() depends on internal state (e.g. an image whose pixel data is swapped) and which forgets to call InvalidateMeasure will render at the stale measured size with no framework-level recovery, mirroring the Label bug in cache-01.

- **▶ 수정 방향**: `view-impl.cpp:1293-1354` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-x-measure-cache-cache-03"></a>

#### 🟢 LOW · `cache-03` — Measure cache compares constraints with a 0.001 epsilon instead of exact equality

- **위치**: `view-impl.cpp:188-191 (FloatEqual eps=0.001), 908-909 (cache compare), 934-935 (exact store)`

- **현상(버그)**: Cache-hit test uses FloatEqual(stored,effNat) with epsilon 0.001 (view-impl.cpp:188-191, 908-909), while the stored key is the exact effNat (934-935). Two constraints differing by <0.001 are treated as identical and skip re-measure.

- **근거 — 일반 프레임워크 기대동작**: Android/WPF compare the measure spec / availableSize for exact equality; sub-unit constraint deltas re-measure. DALi will return a stale measured size for a constraint that drifted by up to ~0.001 visual units, which can compound across deep trees where each level re-derives child constraints by subtraction.

- **근거 — 프레임워크 레퍼런스**: Android View.mMeasureCache exact long-key compare; WPF previousAvailableSize exact compare.

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:info]): Code facts confirmed at the cited lines (file moved to /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp): FloatEqual(a,b,epsilon=0.001f) at 188-191; cache-hit compare via FloatEqual at 908-909; exact store of effNat at 934-935. So the literal "observed" mechanic is accurate.  However the verdict is intentional-design, not a bug:  1. Epsilon comparison is the established, pervasive project convention, not an oversight. git log -L 908,909 shows the cache compare has used FloatEqual since its INTRODUCTION in commit 0401bc42 ("Implement MATCH_PARENT follower semantics"); it was epsilon-based from day one and survived two …

    - `code`(confirmed [보정:low]): Re-read /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp (note: the file is under public-api/, not internal/, but the cited line numbers match exactly).   - Lines 188-191: `inline bool FloatEqual(float a, float b, float epsilon = 0.001f) { return std::abs(a - b) < epsilon; }` — confirms the 0.001 epsilon helper. - Lines 908-909: cache-hit test is `if(mImpl->mLastMeasuredConstraint.width >= 0.0f && FloatEqual(mImpl->mLastMeasuredConstraint.width, effNatW) && FloatEqual(mImpl->mLastMeasuredConstraint.height, effNatH))` then `return mImpl->mMeasuredSize;` (line 911). This compares the stored constraint to the new effecti…

- **재현**: A parent animates width from 100.0000 to 100.0009 over frames; the child is never re-measured because each step is within epsilon, even though accumulated drift could matter for pixel-snapped content.

- **▶ 수정 방향**: `view-impl.cpp:188-191` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-x-measure-cache-cache-04"></a>

#### ⚪ INFO · `cache-04` — Cache-hit guard checks width>=0 but not height>=0 (asymmetric defensive check)

- **위치**: `view-impl.cpp:908-909; paired writes at 934-935, 1311-1312, 1284-1285`

- **현상(버그)**: The cache-hit guard is `mLastMeasuredConstraint.width >= 0.0f && FloatEqual(width,...) && FloatEqual(height,...)` (view-impl.cpp:908-909) — it sentinel-guards only the width component for the DIRTY(-1)/NaN states, not height.

- **근거 — 일반 프레임워크 기대동작**: For symmetry/robustness both components should be guarded, or a single explicit state flag used. It is currently safe only because width and height are always written together (934-935 valid, 1311-1312 DIRTY, 1284-1285 NaN), so width<0 ⇔ height<0. Any future code path that writes one without the other would let a DIRTY/NaN height pass the FloatEqual on a coincidental match.

- **근거 — 프레임워크 레퍼런스**: Android packs both specs into one long key, eliminating per-component sentinel asymmetry.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:info]): The "observed" claim is an accurate reading of the code. At /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:908-909 the cache-hit guard is:   if(mImpl->mLastMeasuredConstraint.width >= 0.0f && FloatEqual(mImpl->mLastMeasuredConstraint.width, effNatW) &&      FloatEqual(mImpl->mLastMeasuredConstraint.height, effNatH)) The sentinel guard `>= 0.0f` is applied ONLY to the width component; height is checked only via FloatEqual, with no `height >= 0.0f` guard. This is the asymmetry the finding describes.  The supporting facts also check out: - Three-state encoding (NaN initial / DIRTY -1.0f / positive) documented at view-i…

    - `intent`(intentional-design [보정:info]): Re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp. The guard at 908-909 is `mLastMeasuredConstraint.width >= 0.0f && FloatEqual(width,...) && FloatEqual(height,...)`. The width-only sentinel check is a deliberate, documented design, not an oversight:  1. The three-state encoding of mLastMeasuredConstraint is explicitly documented at view-impl.cpp:113-124: NaN = initial, MEASURE_CACHE_DIRTY(-1.0f) = invalidated, positive = valid constraint. The `width >= 0.0f` test rejects BOTH negative sentinel states (NaN fails `>= 0` per IEEE 754; -1.0f fails it numerically), gating entry to the positive-val…

- **재현**: Refactor hazard: a partial cache update that sets width to a valid value but leaves height=NaN would pass the guard (width>=0) and then FloatEqual(NaN,h) is false → miss, OK; but width=valid,height=-1 with incoming height=-1 constraint (never happens today since constraints are clamped >=0) would false-hit. Latent, not currently triggerable.

- **▶ 수정 방향**: `view-impl.cpp:908-909` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


### [횡단] RTL/LayoutDirection (`x-layout-direction-rtl`) — 6건


<a id="fix-x-layout-direction-rtl-rtl-01"></a>

#### 🟠 HIGH · `rtl-01` — RTL mirror axis is the layout's measured width, not a content box enclosing children — under-measured/overflowing custom & absolute layouts mirror wrong

- **위치**: `public-api/view-impl.cpp:1079 (ApplyLayoutDirection(bounds.width)); :1193-1211 (formula, no clamp). samples/customlayout/customlayout-diagonal-layout-direction-example.cpp:56-58 explicitly returns {widthConstraint, totalHeight} with the comment 'Span the full available width so the framework's RTL mirror at bounds.width matches the window width' — a documented workaround proving the footgun.`

- **현상(버그)**: ApplyLayoutDirection mirrors each child with POSITION_X = parentWidth - oldX - childW where parentWidth = the layout's own visual bounds.width (view-impl.cpp:1079 passes bounds.width; 1193,1211). There is no clamping and no relationship enforced between bounds.width and the children's actual extent. If a layout places a child at logicalX+childW > bounds.width (e.g. WRAP_CONTENT layout narrower than content, a custom diagonal layout, or AbsoluteLayout children placed beyond maxRight), the mirrored X goes negative / off-screen.

- **근거 — 일반 프레임워크 기대동작**: RTL mirroring in Android/CSS is performed relative to the container's content/padding box which by construction encloses in-flow children, so flipping never throws a child to a negative coordinate. The mirror reference should be the same box children were laid into (or be clamped), independent of how the manager chose to report its measured size.

- **근거 — 프레임워크 레퍼런스**: Android RTL gravity/layoutDirection resolution mirrors within the resolved padding box; CSS direction:rtl flips within the line box. Neither mirrors against an independently-chosen intrinsic size.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed): Re-read the cited code (paths actually live under dali-ui-foundation/, not public-api/ root, but the lines match). The finding's "observed" behavior holds:  1. /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:1079 — Arrange() calls ApplyLayoutDirection(bounds.width), where bounds is the rect this view was assigned by its parent.  2. view-impl.cpp:1193-1213 — ApplyLayoutDirection mirrors each non-standalone direct child via child.SetProperty(POSITION_X, parentWidth - oldX - childW) with NO clamping and no min(0)/max guard. Verified verbatim.  3. The mirror reference parentWidth IS the manager-chosen measured size, not …

    - `code2`(confirmed [보정:medium]): Re-derived the RTL mirror mechanism from source.  1) The mirror formula is exactly as claimed: ViewImpl::ApplyLayoutDirection (view-impl.cpp:1193-1212) sets each non-standalone child's POSITION_X = parentWidth - oldX - childW, with NO clamping (lines 1208-1211). It is invoked from ViewImpl::Arrange at view-impl.cpp:1079 as ApplyLayoutDirection(bounds.width) — so the mirror reference axis is the view's OWN arranged bounds.width.  2) Critically, bounds.width for a non-MATCH_PARENT view is the view's own MEASURED width, not a content box enclosing children. At the root pump, layout-controller.cpp:479: bounds.width = (layoutWidth == MATCH_PARENT)…

    - `intent`(intentional-design [보정:info]): The observed code is real and accurate: ViewImpl::Arrange calls ApplyLayoutDirection(bounds.width) (view-impl.cpp:1079) and the mirror formula is newX = parentWidth - oldX - childW with no clamp (view-impl.cpp:1193-1211). parentWidth is the layout's own arranged bounds.width. The sample workaround comment is real (customlayout-diagonal-layout-direction-example.cpp:56-58).  However this is a deliberate, documented design choice, not an unintended deviation: 1. Single intentional commit d68a9374 ("Apply RTL mirroring in layout arrange pass") introduced it with the explicit stated rationale "layout managers stay direction-agnostic" — mirror runs…

- **재현**: A Layout with a custom ArrangeCallback (or a WRAP_CONTENT manager) reports measured width = max child width (say 80) but places three children diagonally at x=0,50,150. Under RTL the third child gets POSITION_X = 80 - 150 - 200 = -270, flying off the left edge. Author must artificially inflate measured width to fix it.

- **▶ 수정 방향**: `public-api/view-impl.cpp:1079` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-x-layout-direction-rtl-rtl-02"></a>

#### 🟡 MEDIUM · `rtl-02` — AbsoluteLayout mirrors explicit X bounds under RTL — deviates from Android AbsoluteLayout/FrameLayout and CSS position:absolute (physical left does NOT mirror)

- **위치**: `absolute-layout-manager.cpp:99-103,205-208,290; view-impl.cpp:1193-1211; utc-Dali-AbsoluteLayout.cpp:481-512 (asserts mirroring); samples/absolutelayout/absolutelayout-layout-direction-example.cpp:50-54 places red box at x=50 and states RTL flips it.`

- **현상(버그)**: AbsoluteLayoutManager::Arrange writes the child at the logical x from its explicit SetBounds (absolute-layout-manager.cpp:205,272/277,290) and the central ApplyLayoutDirection then flips it. A box explicitly placed at x=10 under RTL ends up at parentWidth-10-w. This is asserted as intended (utc-Dali-AbsoluteLayout.cpp:502: 200-10-50) and documented in the sample ('RTL flips direct children horizontally', absolutelayout-layout-direction-example.cpp:26-28).

- **근거 — 일반 프레임워크 기대동작**: In Android (AbsoluteLayout/FrameLayout with explicit left or absolute X) and CSS (position:absolute; left:10px) an explicit PHYSICAL X coordinate is NOT mirrored under RTL; only logical start/end offsets mirror. A general-purpose absolute layout treats its coordinates as physical.

- **근거 — 프레임워크 레퍼런스**: Android AbsoluteLayout / View.setX (physical, no RTL mirror); CSS position:absolute left/right (physical offsets, not start/end).

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): The "observed" code claim holds exactly. AbsoluteLayoutManager::Arrange writes the child at the LOGICAL x from its explicit SetBounds with no RTL handling: absolute-layout-manager.cpp:205 (x=childBoundsSpec.x), :276 (x*=childScale for non-proportional), :290 (childBounds.x = bounds.x + x + margin). The manager is direction-agnostic. The mirroring happens centrally in ViewImpl::Arrange: line 1062 dispatches to the layout manager, then line 1079 unconditionally calls ApplyLayoutDirection(bounds.width). ApplyLayoutDirection (view-impl.cpp:1193-1213) early-returns unless RTL (1195), then for every non-standalone child (1203 skips only standalone)…

    - `intent`(intentional-design [보정:info]): The observed mirroring is a deliberately designed, central RTL contract — not an AbsoluteLayout-specific deviation, and it is pinned by both a test and a sample.  CODE (verified): AbsoluteLayoutManager::Arrange writes children at their logical/spec X with no direction awareness (absolute-layout-manager.cpp:205-208, 270-285, 290). The mirroring happens centrally in ViewImpl::Arrange, which calls ApplyLayoutDirection(bounds.width) after EVERY OnArrange variant (view-impl.cpp:1079). ApplyLayoutDirection (view-impl.cpp:1193-1213) flips every non-standalone child: POSITION_X = parentWidth - oldX - childW. So the flip is universal across all layout…

- **재현**: Developer ports an absolute overlay from Android expecting a badge pinned at left:10 to stay on the left in both directions; under DALi RTL it jumps to the right edge.

- **▶ 수정 방향**: `absolute-layout-manager.cpp:99-103` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-x-layout-direction-rtl-rtl-03"></a>

#### 🟡 MEDIUM · `rtl-03` — flex-direction:ROW_REVERSE combined with RTL (the CSS double-negative) is untested

- **위치**: `flex-layout-manager.cpp:311-324,444-447; utc-Dali-FlexLayout.cpp:465-... (ROW_REVERSE, no SetLayoutDirection), :636-669 (RTL, ROW only); only one RIGHT_TO_LEFT occurrence in the file (line 643).`

- **현상(버그)**: FlexLayoutManager performs reverse purely as logical x math (isMainAxisReversed at flex-layout-manager.cpp:311-324) and never reads LayoutDirection; the central pass then mirrors. So ROW_REVERSE+RTL produces visual LTR (double-negative cancels), matching CSS. But no UTC and no sample exercises the combination: the RTL flex test (utc-Dali-FlexLayout.cpp:636-669) uses ROW only; the ROW_REVERSE test (465) and the reverse sample (flexlayout-direction-reverse-example.cpp) are LTR only.

- **근거 — 일반 프레임워크 기대동작**: A general framework's RTL correctness for reversed flex containers should be covered by tests, because it is the single most error-prone RTL interaction (CSS: row-reverse + rtl == row + ltr visually).

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox §5.2 / writing modes: row-reverse under direction:rtl resolves to visual left-to-right.

- **근거 — 검증자 코드분석**:

    - `intent`(confirmed [보정:medium]): This is a genuine missing-coverage finding, and the framework "expected" behavior is stated correctly.  Code-architecture claim verified: - FlexLayoutManager is fully LayoutDirection-agnostic: reverse is pure logical x/y math via isMainAxisReversed (flex-layout-manager.cpp:311-342, IsMainAxisReversed at :444-448). Grep shows NO LayoutDirection/RIGHT_TO_LEFT reference anywhere in flex-layout-manager.cpp. - The RTL mirror is a separate central pass: ViewImpl::ApplyLayoutDirection (view-impl.cpp:1193-1213) mirrors every direct child's POSITION_X via parentWidth - oldX - childW, invoked once per Arrange after every OnArrange variant (view-impl.cp…

    - `code`(confirmed [보정:medium]): I re-read all cited code under /home/jae/dali/dali-ui-claude (note: the flex manager actually lives at dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp, not the path in the evidence string, but it is the same file).  OBSERVED claim verified true: 1. FlexLayoutManager performs reverse as pure logical x/y math: flex-layout-manager.cpp:309-342 branches on isMainAxisReversed and decrements/increments mainOffsetInOut; IsMainAxisReversed() at :444-448 is derived solely from FlexDirection (ROW_REVERSE/COLUMN_REVERSE), not LayoutDirection. A grep for LayoutDirection/RIGHT_TO_LEFT/Mirror over the entire flex-layout-manager.cpp returns zer…

- **재현**: Regression in either the reverse math or the central mirror would silently break ROW_REVERSE under RTL with no failing test.

- **▶ 수정 방향**: `flex-layout-manager.cpp:311-324` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-x-layout-direction-rtl-rtl-04"></a>

#### 🟢 LOW · `rtl-04` — Vertical StackLayout cross-axis START/END alignment mirroring under RTL is untested

- **위치**: `stack-layout-manager.cpp:475-495 (vertical cross align); view-impl.cpp:1193-1211 (X-only flip); utc-Dali-StackLayout.cpp:687 only covers horizontal.`

- **현상(버그)**: In a vertical StackLayout the cross axis is horizontal; LayoutAlignment::START places at crossX=left, END at right (stack-layout-manager.cpp:489-494), and the central pass flips X so START becomes visual-right under RTL (correct, Android-like). Only the HORIZONTAL stack RTL case is tested (utc-Dali-StackLayout.cpp:687-723); no vertical-stack RTL test verifies that logical cross-axis START/END mirror while the vertical main axis does not.

- **근거 — 일반 프레임워크 기대동작**: Cross-axis logical alignment (start/end) should mirror under RTL while the main (vertical) axis must not — both halves of the invariant deserve a test.

- **근거 — 프레임워크 레퍼런스**: Android vertical LinearLayout gravity START/END resolve to right/left under RTL.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): The finding is a missing-coverage claim, and every load-bearing fact holds against the worktree source (files live under public-api/, not internal/, but the cited content matches).  Code behavior verified: - stack-layout-manager.cpp:475-495: in the VERTICAL branch the cross axis is X; LayoutAlignment::END adds crossAvailable-childWidth (line 489-490 -> visual right), START is the default no-op (line 492-494 -> visual left, crossX=currentX+margin.start). Matches "observed". - view-impl.cpp:1193-1211: ApplyLayoutDirection returns early unless RIGHT_TO_LEFT (1195), then flips only POSITION_X via parentWidth-oldX-childW (1211). X-only flip confir…

    - `intent`(confirmed [보정:low]): Re-verified all claims against source. (1) Vertical-stack cross-axis alignment with explicit START/CENTER/END/FILL cases exists at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp:478-495 (END at 489-490 adds crossAvailable-childWidth to crossX; START is the default no-op at 492-494). (2) The RTL mirroring is an X-only flip in ViewImpl::ApplyLayoutDirection at view-impl.cpp:1193-1212 (POSITION_X = parentWidth - oldX - childW; Y untouched), so the vertical main axis is correctly NOT mirrored while the horizontal cross axis is. The 'observed' description is accurate. (3) Test gap is real: grep over ut…

- **재현**: A vertical stack with START-aligned narrow children under RTL should pin them to the visual right; nothing guards this.

- **▶ 수정 방향**: `stack-layout-manager.cpp:475-495` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-x-layout-direction-rtl-rtl-05"></a>

#### 🟢 LOW · `rtl-05` — RTL mirror reads live actor POSITION_X/SIZE_WIDTH and rewrites in place — correctness depends on every Arrange re-running the manager first (fragile idempotency)

- **위치**: `view-impl.cpp:1068 (mArrangedBounds stored logical), :1209-1211 (mirror reads live POSITION_X), :2463 (reset before manager); contrast layout-transition-dispatcher.cpp:346-362 which correctly recomputes visual from stored logical GetArrangedBounds.`

- **현상(버그)**: ApplyLayoutDirection mutates the child actor's POSITION_X using the actor's current POSITION_X and SIZE_WIDTH (view-impl.cpp:1209-1211) rather than a stored logical value. It is safe only because every full Arrange first resets POSITION_X to the logical value via the dispatch path (OnArrange:1098, DispatchArrangeWithLayoutManager:2463) before the flip runs. Any future code path that re-invokes ApplyLayoutDirection (or Arrange) without first re-establishing the logical POSITION_X would mirror an already-mirrored value and flip the child back.

- **근거 — 일반 프레임워크 기대동작**: A mirror pass should derive the visual position from a persisted logical coordinate (the manager already stores logical mArrangedBounds, view-impl.cpp:1068, and the transition dispatcher reuses it) rather than from the mutable live property, to be idempotent by construction.

- **근거 — 프레임워크 레퍼런스**: Android/WPF resolve RTL during layout from logical values each pass; they do not in-place re-flip a physical coordinate.

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:info]): The observed code facts are accurate but the finding describes intentional, test-pinned design rather than a defect. Re-read at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:  1. ApplyLayoutDirection (1193-1213) does read the live actor POSITION_X (1209) and SIZE_WIDTH (1210) and rewrite POSITION_X in place (1211) — observed claim confirmed.  2. mArrangedBounds stores the logical pre-mirror bounds (1068; getter 1664-1666), and the transition dispatcher VisualBoundsOf recomputes the visual x from that stored logical value each call (layout-transition-dispatcher.cpp:346-360: bounds.x = parentBounds.width - bounds.x -…

    - `code`(confirmed [보정:low]): Re-read the cited code under /home/jae/dali/dali-ui-claude. ApplyLayoutDirection (view-impl.cpp:1193-1213) reads the child's LIVE actor properties — POSITION_X (1209) and SIZE_WIDTH (1210) — and rewrites POSITION_X in place as parentWidth - oldX - childW (1211). It does NOT read any persisted logical coordinate at flip time. This confirms the "observed" claim exactly.  The idempotency argument also holds. ApplyLayoutDirection is invoked exactly once, at the tail of ViewImpl::Arrange (1079), after OnArrange/DispatchArrange* have run. Each child's logical POSITION_X is re-established immediately before the flip because the child's own Arrange w…

- **재현**: An optimization that does an 'arrange-only' refresh (re-running the mirror without the manager) would double-mirror; the current architecture has no guard, only the discipline that Arrange always re-runs the manager.

- **▶ 수정 방향**: `view-impl.cpp:1068` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-x-layout-direction-rtl-rtl-06"></a>

#### 🟢 LOW · `rtl-06` — Default OnRelayout (non-layout legacy path) swaps padding for RTL but the main layout pipeline never uses that code; two divergent RTL implementations

- **위치**: `view-impl.cpp:520-556 (OnRelayout RTL with std::swap(padding.start,end)) guarded by 522-525; vs view-impl.cpp:1193-1211 geometric mirror used by Measure/Arrange pipeline.`

- **현상(버그)**: ViewImpl::OnRelayout contains a separate RTL handling that std::swaps padding.start/end and offsets children (view-impl.cpp:538-552), but it only runs for views with NO layout capability and NO parent layout/view (guard at 522-525) — i.e. legacy DALi relayout. The real layout pipeline mirrors geometrically via ApplyLayoutDirection and never swaps padding. So there are two unrelated RTL mechanisms in the same class; the OnRelayout one is effectively dead for any view inside the new layout system.

- **근거 — 일반 프레임워크 기대동작**: A single, consistent RTL strategy. Having a padding-swap RTL path that is unreachable for layout-managed views is a latent inconsistency / maintenance hazard.

- **근거 — 프레임워크 레퍼런스**: Single resolveRtlPropertiesIfNeeded pass in Android; no dual mechanism.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): Both code claims hold against the actual source under /home/jae/dali/dali-ui-claude.  (1) Legacy padding-swap RTL path: view-impl.cpp:520-556. OnRelayout returns early (522-525) if `HasLayoutCapability(*this) || GetParentLayout() || GetParentView()`. Only when ALL three are false does it reach the RTL block, where for RIGHT_TO_LEFT it does `std::swap(padding.start, padding.end)` (540-543) and then offsets children by margin.start+padding.start (548-552). So this RTL mechanism runs only for views with no layout capability and no layout-system parent — i.e. legacy DALi relayout.  (2) Layout-pipeline RTL path: ApplyLayoutDirection (view-impl.cpp…

    - `intent`(intentional-design [보정:info]): The factual code claims are accurate but the "defect" framing is not. Verified at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:520-556 that ViewImpl::OnRelayout has an RTL branch doing std::swap(padding.start,end) (542) guarded by 522-525 (HasLayoutCapability||GetParentLayout||GetParentView), and at 1193-1211 that ApplyLayoutDirection performs a geometric mirror (POSITION_X = parentWidth - oldX - childW) called once per Arrange (1079). These are genuinely mutually exclusive.  However: (1) The padding-swap-under-RTL idiom is NOT unique to OnRelayout. The identical std::swap(padding.start,padding.end) under RIGHT_TO…

- **재현**: A maintainer fixing RTL padding edits OnRelayout assuming it is the active path; the change has no effect on Stack/Flex/Grid/Absolute children.

- **▶ 수정 방향**: `view-impl.cpp:520-556` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


### [횡단] margin/padding/정렬 (`x-margin-padding-alignment`) — 5건


<a id="fix-x-margin-padding-alignment-pad-01"></a>

#### 🟠 HIGH · `pad-01` — Padding double-counted for WRAP_CONTENT leaf View with a background visual

- **위치**: `view-impl.cpp:2983-2984 (GetNaturalSize adds padding, introduced commit c588fc2ec 2026-04-16); view-impl.cpp:1034-1035 (OnMeasure adds pw again); pw defined view-impl.cpp:953-954. GetNaturalSize is the CustomActorImpl override (view-impl.h:1108) so Self().GetNaturalSize() dispatches to it.`

- **현상(버그)**: ViewImpl::GetNaturalSize() adds padding to the visual's natural size (view-impl.cpp:2983-2984: naturalSize.width += padding.start+end). Then ViewImpl::OnMeasure's WRAP_CONTENT leaf branch calls Self().GetNaturalSize() and adds padding AGAIN: size.width = naturalSize.width + pw (view-impl.cpp:1034-1035, with pw = padding.start+end from 953). So a WRAP_CONTENT View whose background visual reports a non-zero natural size is measured as visualNatural + 2*padding.

- **근거 — 일반 프레임워크 기대동작**: Natural/intrinsic content size excludes padding; the layout pass adds padding exactly once when reporting the desired box (CSS: width=content+padding+border, padding added once; Android intrinsic size + padding once). Padding must be added exactly once.

- **근거 — 프레임워크 레퍼런스**: CSS box model: content-box width does not include padding; padding contributes once to the border-box. Android: getMeasuredWidth = intrinsic content + paddingLeft + paddingRight (once).

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:high]): Re-read /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp (path differs from the finding's `internal/controls/control/view-impl.cpp` but it is the only view-impl.cpp with GetNaturalSize; same file). The two cited code sites exist and behave as claimed:  1. ViewImpl::GetNaturalSize() (function at line 2974): when a BACKGROUND visual exists, it gets the visual's natural size and then adds padding: `naturalSize.width += (mImpl->mPadding.start + mImpl->mPadding.end)` (line 2983) and the height analog (line 2984), returning visualNatural + padding.  2. ViewImpl::OnMeasure() leaf branch (no children). `pw = mPadding.start + …

    - `intent`(confirmed [보정:high]): Re-read the cited source at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp. The double-count is real and unintended:  1. ViewImpl::GetNaturalSize() (view-impl.cpp:2982-2985) takes the background visual's natural size and adds padding: naturalSize.width += (mPadding.start + mPadding.end), likewise height. Returns Vector3::ZERO when no background visual (line 2987).  2. ViewImpl::OnMeasure's WRAP_CONTENT leaf branch (view-impl.cpp:1032-1035 width, 1045-1048 height) computes naturalSize = Self().GetNaturalSize() then size.width = naturalSize.width + pw, where pw = mPadding.start+end (line 953). GetNaturalSize is declar…

    - `code2`(confirmed [보정:high]): Re-derived from source at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp (the audit's cited path lacked the public-api/ directory, but line numbers/content match exactly).  1. GetNaturalSize (lines 2974-2988): when a background visual exists, it takes the visual's natural size and adds padding ONCE: naturalSize.width += (mImpl->mPadding.start + mImpl->mPadding.end) (line 2983), same for height (2984). Introduced by commit c588fc2ec (2026-04-16, "Move mPadding to ViewDataImpl"), confirmed via git blame.  2. GetNaturalSize is declared `override` of Dali::CustomActorImpl::GetNaturalSize() at view-impl.h:1108, so Self()…

- **재현**: View v; v.SetBackground(a visual with natural size 80x40); v.SetPadding(Extents(5,5,5,5)); v.SetRequestedWidth(WRAP_CONTENT). Measure -> width = 80 + 10 (in GetNaturalSize) + 10 (in OnMeasure) = 100, expected 90.

- **▶ 수정 방향**: `view-impl.cpp:2983-2984` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-x-margin-padding-alignment-margin-01"></a>

#### 🟡 MEDIUM · `margin-01` — ScrollViewLayoutManager ignores per-child margin entirely (both axes, Measure and Arrange)

- **위치**: `scroll-view-layout-manager.cpp:51-151 — no GetMargin anywhere; contrast stack-layout-manager.cpp:89-93, grid-layout-manager.cpp:107-111, flex-layout-manager.cpp:482-486, absolute-layout-manager.cpp:131-133 which all reserve margin.`

- **현상(버그)**: ScrollViewLayoutManager::Measure and ::Arrange never call childImpl.GetMargin(). Child measure constraints are full width/height or FLT_MAX (scroll-view-layout-manager.cpp:81-84); MATCH_PARENT child is set to the full bounds.width/height with no margin subtraction (scroll 128-135); child position is child.GetPositionX()*s with no margin offset (scroll 123-124).

- **근거 — 일반 프레임워크 기대동작**: Like every other manager in this codebase (and Android MarginLayoutParams), the parent should reserve the child's margin: reduce the child's available space by margin and offset its position by margin.start/top. Margin should be honored uniformly across managers.

- **근거 — 프레임워크 레퍼런스**: Android ScrollView/MarginLayoutParams: parent measures child with measureChildWithMargins and offsets by margins; margins are honored by scroll containers.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Re-read scroll-view-layout-manager.cpp:51-151 in full. The "observed" claim holds exactly:  (1) Measure (lines 81-84): childWidthConstraint = widthIsMatchParent ? widthConstraint : FLT_MAX; childHeightConstraint likewise. No margin is subtracted from either constraint. effectiveWidth/Height (86-89) also do not account for margin.  (2) Arrange MATCH_PARENT path (lines 128-135): childBounds.width = bounds.width and childBounds.height = bounds.height, with no margin subtraction.  (3) Child position (lines 123-124): childBounds.x = child.GetPositionX()*s; childBounds.y = child.GetPositionY()*s — no margin.start/top offset added.  A grep over the …

    - `intent`(confirmed [보정:medium]): Re-read scroll-view-layout-manager.cpp (/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/scroll-view-layout-manager.cpp): GetMargin() count is 0. Measure (lines 81-89) sets childWidth/HeightConstraint to widthConstraint/heightConstraint or FLT_MAX with NO margin subtraction. Arrange (lines 123-135) sets childBounds.x = child.GetPositionX()*s with NO margin offset, and MATCH_PARENT child gets childBounds.width = bounds.width with NO margin subtraction. Contrast is accurate: all four other managers call GetMargin and reserve it in both phases — stack:89-93, grid:107-111 + position offset 289-292, flex:260-263 + position offse…

- **재현**: ScrollView content child with SetMargin(10,10,10,10) and MATCH_PARENT width: child is sized to full viewport width (margin ignored) and placed at x=0 instead of x=10, unlike the same child under Stack/Flex/Grid/Absolute.

- **▶ 수정 방향**: `scroll-view-layout-manager.cpp:51-151` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-x-margin-padding-alignment-align-01"></a>

#### 🟢 LOW · `align-01` — CENTER alignment uses unrounded float midpoint (asymmetric / sub-pixel centering)

- **위치**: `stack-layout-manager.cpp:487,537; grid-layout-manager.cpp:306,328; flex-layout-manager.cpp:210,297.`

- **현상(버그)**: All managers center with a raw float *0.5 / /2.0 and never floor/round: Stack crossX += (crossAvailable - childWidth)*0.5f (stack-layout-manager.cpp:487,537); Grid childBounds.x += (cellWidth-childWidth)*0.5f (grid-layout-manager.cpp:306,328); Flex childCrossOffset += crossSpace/2.0f and justify mainOffset = freeSpace/2.0f (flex-layout-manager.cpp:210,297).

- **근거 — 일반 프레임워크 기대동작**: Pixel-snapping frameworks floor/round the centering offset to avoid half-pixel placement and to keep left/right gaps deterministic; CSS rounds to device pixels at paint, Android centers with integer arithmetic ((parentSize-childSize)/2).

- **근거 — 프레임워크 레퍼런스**: Android Gravity.CENTER uses integer division; CSS aligns then snaps to device pixels.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): Re-read all six cited lines under /home/jae/dali/dali-ui-claude and each matches the "observed" claim verbatim. Stack: stack-layout-manager.cpp:487 `crossX += (crossAvailable - childWidth) * 0.5f;` (CENTER cross-axis horizontal) and :537 `crossY += (crossAvailable - childHeight) * 0.5f;` (CENTER cross-axis vertical), both inside a switch on LayoutAlignment::CENTER, with no floor/round; the result feeds childBounds.x/.y and is passed raw to childImpl.Arrange(childBounds) (line 506). Grid: grid-layout-manager.cpp:306 `childBounds.x += (cellWidth - childWidth) * 0.5f;` and :328 `childBounds.y += (cellHeight - childHeight) * 0.5f;`, both under ca…

    - `intent`(intentional-design [보정:info]): The observed float-centering is real and accurately cited. I verified: stack-layout-manager.cpp:487 and :537 use crossX/crossY += (crossAvailable - childW/H) * 0.5f; grid-layout-manager.cpp:306 and :328 use (cellWidth/Height - childWidth/Height) * 0.5f; flex-layout-manager.cpp:210 uses childCrossOffset += crossSpace / 2.0f and :297 uses out.mainOffset = freeSpace / 2.0f. None floor/round.  But this is intentional design, not a defect, for two independent reasons:  1) The entire layout subsystem is float-native by contract. LayoutRect stores float x/y/width/height (layout-types.h:176-265) and MeasuredSize is float (layout-types.h:98-170). DALi…

- **재현**: Center a child of width 101 in a content box of width 200: offset = 49.5, child spans [49.5,150.5] — fractional, with unequal integer-pixel gaps after rasterization. Repeated across nested centers the fractional drift accumulates.

- **▶ 수정 방향**: `stack-layout-manager.cpp:487` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-x-margin-padding-alignment-stack-fill-01"></a>

#### 🟢 LOW · `stack-fill-01` — Stack cross-axis FILL only stretches WRAP_CONTENT children; fixed-size children silently START-align

- **위치**: `stack-layout-manager.cpp:480-485,530-535 (guard `childImpl.GetRequestedWidth()==WRAP_CONTENT`).`

- **현상(버그)**: In Stack cross-axis alignment, FILL resets the child to crossAvailable ONLY when GetRequestedWidth()==WRAP_CONTENT (stack-layout-manager.cpp:480-485 VERTICAL; 530-535 HORIZONTAL). A child with a fixed cross size and FILL keeps its fixed size and is left at START (crossX unchanged).

- **근거 — 일반 프레임워크 기대동작**: FILL/stretch alignment conventionally forces the child to the cross extent regardless of its requested size, or is explicitly documented as a no-op for fixed sizes. Treating FILL as START for fixed-size children is surprising and undiscoverable.

- **근거 — 프레임워크 레퍼런스**: CSS align-items:stretch only stretches items with auto cross size (so the WRAP-only guard is actually CSS-consistent) — but unlike CSS there is no diagnostic and START is the silent fallback; Android FILL gravity forces fill.

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:info]): The observed code behavior is accurate: in stack-layout-manager.cpp (path is /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp, NOT the path cited in evidence) the FILL case stretches the child to crossAvailable only when the cross-axis size is WRAP_CONTENT — VERTICAL at lines 480-485 (GetRequestedWidth()==WRAP_CONTENT) and HORIZONTAL at lines 530-535 (GetRequestedHeight()==WRAP_CONTENT). A fixed-size child under FILL keeps its size; crossX/crossY are not adjusted (only CENTER/END adjust position, lines 486-491 / 536-544), so it stays at START.  This is deliberately pinned, not a bug. Two dedicated U…

    - `code`(confirmed [보정:low]): Re-read stack-layout-manager.cpp:460-560. The observed code reading is exactly correct.  VERTICAL branch (lines 478-495): the FILL case (480-485) only sets finalWidth=crossAvailable when GetRequestedWidth()==WRAP_CONTENT. For a fixed cross size (e.g. 60), the WRAP_CONTENT guard is false (60 != -1.0f, MATCH_PARENT is -2.0f per layout-types.h:65,73), so finalWidth stays = childWidth = allocations[i].width = 60 (line 477). Critically, the FILL case adds NOTHING to crossX, which was initialized at line 476 to currentX + margin.start (the START base). CENTER (487) and END (490) add offsets; FILL does not. So a fixed-size FILL child is rendered at …

- **재현**: Vertical Stack, child RequestedWidth=60 fixed, alignment FILL, container content width 200: child stays width 60 at x=0 (START), not stretched to 200 — same visual result as START.

- **▶ 수정 방향**: `stack-layout-manager.cpp:480-485` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-x-margin-padding-alignment-neg-margin-01"></a>

#### 🟢 LOW · `neg-margin-01` — Negative margins partially honored but untested across all managers

- **위치**: `extents.h:115-118 (int16_t fields); flex-layout-manager.cpp:343-346; grid-layout-manager.cpp:289-292; stack-layout-manager.cpp:476,549; absolute-layout-manager.cpp:290-291. No negative-margin UTC found under automated-tests/src/dali-ui-foundation/.`

- **현상(버그)**: Extents fields are int16_t (signed), so negative margins are representable. Managers compute marginW=margin.start+margin.end and do childBounds.x += margin.start (can shift outward, negative), then childBounds.width = max(0, width - (start+end)) (a negative total margin would GROW the box). E.g. flex-layout-manager.cpp:343-346, grid 289-292, stack 476/549. No test exercises negative margins (grep for SetMargin(Extents(-...)) returns nothing).

- **근거 — 일반 프레임워크 기대동작**: Define and test a policy: CSS allows negative margins (pulls elements together / overlaps); Android clamps. Here the behavior (position can go negative, total negative margin enlarges the child) is an emergent untested side effect rather than a stated contract.

- **근거 — 프레임워크 레퍼런스**: CSS 2.1 §8.3 negative margins allowed and overlap; Android clamps margins to >= 0 in most ViewGroups.

- **근거 — 검증자 코드분석**:

    - `intent`(confirmed [보정:medium]): The two cited code paths genuinely diverge on min/max enforcement for a MATCH_PARENT standalone child, and both are reachable for the SAME child.  Verified facts (all in /home/jae/dali/dali-ui-claude): - ArrangeStandaloneChild (view-impl.cpp:268-296): for MATCH_PARENT, childW=max(0, parentFullWidth-marginW) at :281. It re-measures via childImpl.Measure(childW,childH) at :289 (Measure DOES clamp natural to min/max at :905-906 and stores a clamped mMeasuredSize), BUT the Arrange bounds at :292-295 use the UNCLAMPED childW, and ViewImpl::Arrange→OnArrange writes bounds.width verbatim to SIZE_WIDTH at :1100 with no clamp. So the arranged SIZE is …

- **반론(소수의견)**: `code`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: child.SetMargin(Extents(-10,-10,0,0)) in a Stack: childBounds.x shifts left by 10 and childBounds.width = measured - (-20) = measured + 20, an undocumented growth/overlap with no test asserting intent.

- **▶ 수정 방향**: `extents.h:115-118` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


### [횡단] 분배/반올림 (`x-distribution-rounding`) — 8건


<a id="fix-x-distribution-rounding-flex-01"></a>

#### 🟠 HIGH · `flex-01` — Flex grow/shrink and stack weight never freeze/respect child min/max or intrinsic min-content; single non-iterative pass overruns

- **위치**: `flex-layout-manager.cpp:139-189; stack-layout-manager.cpp:410,434,438; view-impl.cpp:1098-1101,931`

- **현상(버그)**: ApplyFlexGrowShrink (flex-layout-manager.cpp L139-189) distributes free space in ONE pass: grow adds extra=(grow/totalFlexGrow)*freeSpace (L147), shrink subtracts reduction floored per-child at std::max(0.0f,...) (L181/185). Grown/shrunk non-MATCH_PARENT children are not re-measured; workingSize is used verbatim as the Arrange size (L258->L352). StackLayoutManager weight does the same (share at L410, used as allocation L434/438). ViewImpl::OnArrange writes the size straight to SIZE_WIDTH/HEIGHT with no clamp (view-impl.cpp L1098-1101).

- **근거 — 일반 프레임워크 기대동작**: CSS Flexbox §9.7: freeze any item that would violate its min/max main size, remove it from the flex factor sum, and re-run distribution on the remainder until all fit — so totals still exactly fill and no item is below its min-content/min size or above its max. Android/WPF likewise clamp then re-proportion.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox §9.7 Resolving Flexible Lengths (steps c-e, min/max violations & freezing)

- **근거 — 검증자 코드분석**:

    - `code2`(confirmed [보정:medium]): Re-derived from source under /home/jae/dali/dali-ui-claude. The core mechanics and the manifesting bug are real.  (1) Flex distribution is a single non-iterative pass with no min/max freeze/re-proportion. flex-layout-manager.cpp:139-159 (grow: extra=(grow/totalFlexGrow)*freeSpace, no upper clamp) and :160-190 (shrink: reduction floored only at std::max(0.0f, ...), L181/185). No reference to any per-child min/max main size anywhere in ApplyFlexGrowShrink.  (2) Grown/shrunk non-MATCH_PARENT children are NOT re-measured. In ArrangeOneFlexLine the re-measure guard at flex-layout-manager.cpp:348 fires only when GetRequestedWidth/Height==MATCH_PARE…

    - `code`(confirmed [보정:high]): I attempted to refute but the code behaves as the finding claims.  FLEX (flex-layout-manager.cpp): ApplyFlexGrowShrink (L133-191) distributes free space in a single pass — grow adds extra=(grow/totalFlexGrow)*freeSpace into workingSizes (L147-155), shrink subtracts a per-child reduction floored only at std::max(0.0f,...) (L178-186). There is no min/max clamp, no freezing of violating items, and no iterative re-distribution. The call site loops over lines exactly once: `for(auto& line : lines) ApplyFlexGrowShrink(...)` (L582-585) — no while/freeze loop. In ArrangeOneFlexLine, the grown/shrunk childMainSize comes straight from workingSizes[idx]…

    - `intent`(confirmed [보정:medium]): Re-verified the core mechanics against /home/jae/dali/dali-ui-claude source. (1) Flex grow/shrink is a single non-iterative pass with no per-child min/max read and no freeze/re-loop: ApplyFlexGrowShrink distributes extra=(grow/totalFlexGrow)*freeSpace [flex-layout-manager.cpp:147] and reduction floored only at std::max(0.0f,...) [flex-layout-manager.cpp:181/185]. (2) Grown/shrunk non-MATCH_PARENT children are NOT re-measured — re-measure is gated solely on MATCH_PARENT [flex-layout-manager.cpp:348-351] — and the workingSize is passed verbatim into childImpl.Arrange(childBounds) [flex-layout-manager.cpp:352]. (3) Arrange does not clamp: OnArra…

- **재현**: Flex row, container 300px, two children flex-shrink:1 with basis 200 each (total 400, overflow 100). Each shrinks by 50 -> 150px. Now give child A MinimumWidth=180. Code shrinks A to 150 anyway (no min applied in Arrange); CSS would freeze A at 180 and shrink B to 120. Grow analog: a grown child with MaximumWidth still grows past its max.

- **▶ 수정 방향**: `flex-layout-manager.cpp:139-189` 에서 위 '기대동작'을 만족하도록 수정. (분류: spec-violation)


<a id="fix-x-distribution-rounding-flex-02"></a>

#### 🟡 MEDIUM · `flex-02` — Flex shrink overflow that hits the per-child zero floor is silently lost (line overflows container)

- **위치**: `flex-layout-manager.cpp:172-188`

- **현상(버그)**: In the shrink loop each child is reduced by its computed reduction then floored: workingSizes[idx].width = std::max(0.0f, width - reduction) (flex-layout-manager.cpp L181). The clipped-off deficit (reduction beyond what the child had) is discarded and line.mainSize is unconditionally set to availableMain (L188), so the line claims to fit even though the summed child sizes exceed availableMain.

- **근거 — 일반 프레임워크 기대동작**: CSS §9.7 redistributes the unabsorbed overflow to the remaining shrinkable items; the line must end up exactly availableMain wide with children summing to that. Setting line.mainSize=availableMain while children sum larger is an internal inconsistency.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox §9.7 (distribute remaining free space, total exactly fills)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Re-read /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:160-191 (ApplyFlexGrowShrink) and 247-355 (ArrangeOneFlexLine). The shrink branch performs exactly one single-pass weighted distribution: reduction = (shrink*childMainSize/totalWeightedShrink)*overflow (L178), then floors each child at zero via std::max(0.0f, workingSizes[idx].width - reduction) (L181/L185), and unconditionally sets line.mainSize = availableMain (L188). There is no loop/second pass and no accounting of the clipped-off deficit, so any reduction exceeding a child's current size is silently discarded and never redistributed to the…

    - `intent`(confirmed [보정:medium]): Re-read /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:133-191 (note: the file lives under public-api/layouts/, not the path in the finding's evidence string). The shrink branch (L160-189) is a SINGLE PASS: for each child, reduction = (shrink*childMainSize/totalWeightedShrink)*overflow (L178), then workingSizes[idx] = std::max(0.0f, size - reduction) (L181/L185), then line.mainSize = availableMain unconditionally (L188). There is no freeze-and-redistribute loop, and the only per-child lower bound is the hard 0 floor (no GetMinimumSize clamp exists; L94-105 only sets basis). So when a child's comput…

- **재현**: Container 100px, child A basis 30 shrink 10, child B basis 200 shrink 0.0001. totalWeightedShrink dominated by A; A's reduction (~130*...) exceeds 30 so A floors at 0, but the remaining overflow is not re-applied to B -> children sum > 100, content overflows, yet line.mainSize records 100.

- **▶ 수정 방향**: `flex-layout-manager.cpp:172-188` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-x-distribution-rounding-grid-04"></a>

#### 🟡 MEDIUM · `grid-04` — Negative Star factor and negative flex-basis are not clamped, producing negative/garbage track and item sizes

- **위치**: `layout-types.h:493-499,525; grid-layout-manager.cpp:203-204; flex-layout-params-impl.h:110-113 vs 76,94`

- **현상(버그)**: GridLength::Star(factor) stores factor unclamped (layout-types.h:493-499, GetValue L525). ApplyGridDefinitions uses starValue directly: colWidths[i]=(starValue/totalStarWidth)*remainingWidth (grid L203-204). A negative star makes that column negative AND can make totalStarWidth small/negative, skewing or sign-flipping all sibling shares. FlexBasis is likewise unclamped (flex-layout-params-impl.h:110-113, SetFlexBasis no max), and the 'basis>0' guard (flex L94/491) silently ignores a negative basis but a basis between 0 and 0 edge and large values flow through unchecked.

- **근거 — 일반 프레임워크 기대동작**: Factors should be clamped non-negative (as flex-grow/shrink ARE: flex-layout-params-impl.h:76,94 use std::max(0.0f,...)). The inconsistency — grow/shrink clamped but star factor and basis not — is a convention gap; WPF treats star weights as >=0.

- **근거 — 프레임워크 레퍼런스**: WPF Grid star sizing (non-negative weights); CSS flex-grow/shrink clamping

- **근거 — 검증자 코드분석**:

    - `code`(confirmed): Re-read all cited code under /home/jae/dali/dali-ui-claude and the "observed" claims hold.  1. GridLength::Star(factor) stores mValue=factor with no clamp (layout-types.h:493-499) and GetValue() returns it raw (layout-types.h:525-528). Confirmed unclamped.  2. ApplyGridDefinitions uses starValue directly: starValue = colDefs[i].GetValue(); colWidths[i] = (totalStarWidth > 0) ? (starValue / totalStarWidth) * remainingWidth : 0.0f (grid-layout-manager.cpp:203-204). The only guard is totalStarWidth > 0, not per-factor. For col defs {Star(2), Star(-1)}: totalStarWidth = 2 + (-1) = 1 > 0, so col0 = 2/1*remaining = 2*remaining (overflow), col1 = -1…

    - `intent`(confirmed [보정:medium]): Re-verified every cited fact against /home/jae/dali/dali-ui-claude source.  STAR factor unclamped: layout-types.h:493-499 `Star(float factor)` stores `mValue = factor` with no clamp; `GetValue()` returns it raw (layout-types.h:525). The grid manager uses the raw value: `totalStarWidth += def.GetValue()` (grid-layout-manager.cpp:154; rows :180) and `colWidths[i] = (starValue / totalStarWidth) * remainingWidth` (grid-layout-manager.cpp:203-204; rows :218-219). A negative star factor shrinks/sign-flips totalStarWidth and produces a negative `colWidths[i]`. ComputeGridPositions accumulates `colPositions[i+1] = colPositions[i] + colWidths[i] + spa…

- **재현**: GridLayout column defs {Star(2), Star(-1)}: totalStarWidth=1, col0 = 2*remaining (double width, overflows), col1 = -remaining (negative width -> std::accumulate underflows totalWidthOut, child cell width clamped to 0 but positions corrupt).

- **▶ 수정 방향**: `layout-types.h:493-499` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-x-distribution-rounding-stack-05"></a>

#### 🟡 MEDIUM · `stack-05` — Stack weight is not weighted by basis and can over-fill when remainingMain<0 is clamped, plus weight children ignore min/max in Arrange

- **위치**: `stack-layout-manager.cpp:116-118,395,434,438`

- **현상(버그)**: remainingMain is clamped to >=0 (stack L118, L395) so when non-weight children already exceed the axis, every weight child gets share 0 and is forced to size 0 (allocations[i]=max(0,share-margin) L434/438) regardless of its MinimumWidth/Height. No deficit is propagated and non-weight children are never shrunk. Weight uses raw weight only; there is no per-child basis/min floor.

- **근거 — 일반 프레임워크 기대동작**: Android LinearLayout: when total exceeds bounds it can distribute negative delta (shrinking weighted children toward, but honoring, their measured/min size); a weighted child still respects its minimum. Forcing weighted children to exactly 0 below their min violates min-size contract.

- **근거 — 프레임워크 레퍼런스**: Android LinearLayout weight (negative delta distribution, min respected)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:high]): Finding index 47 = id "layout-02" (finderKey layout-base-callbacks). I traced the real control flow in /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp.  Callback Measure path: ViewImpl::Measure computes effNatW/effNatH by clamping natW/natH only to min/max (view-impl.cpp:905-906), then invokes the measure callback verbatim with the constraint (view-impl.cpp:918-921) and runs only ApplyConstraints on the return (view-impl.cpp:931). ApplyConstraints (view-impl.cpp:1419-1428) applies ONLY min/max (Min/MaximumWidth*s) — it never references mRequestedWidth/RequestedHeight. So the callback's returned size is final; Request…

    - `intent`(intentional-design [보정:low]): All cited code facts verify. Callback measure path: view-impl.cpp:918-921 invokes the callback with the full effVisW/effVisH constraint, and :931 runs only ApplyConstraints, which (view-impl.cpp:1419-1428) enforces ONLY min/max, never RequestedWidth. By contrast the manager path forces resultVisW=requestedVisW (view-impl.cpp:2442-2447) and the default OnMeasure forces size.width=mRequestedWidth (view-impl.cpp:996-998). So the observed asymmetry is real and reproducible. The two callback UTCs (utc-Dali-Layout.cpp:329-350) do set RequestedWidth==constraint==200 with HorizontalLineMeasure returning {widthConstraint, maxHeight} (:308), which coin…

- **재현**: Vertical stack height 100, one fixed child 120px and one weight=1 child with MinimumHeight=40. remainingMain clamps to 0 -> weight child sized 0, ignoring its 40px minimum; container content overflows and the weighted child collapses.

- **▶ 수정 방향**: `stack-layout-manager.cpp:116-118` 에서 위 '기대동작'을 만족하도록 수정. (분류: spec-violation)


<a id="fix-x-distribution-rounding-grid-08"></a>

#### 🟡 MEDIUM · `grid-08` — Grid star distribution lacks per-track min/max clamp+re-proportion; auto content floor only honored on implicit tracks, not explicit Star tracks

- **위치**: `grid-layout-manager.cpp:201-213`

- **현상(버그)**: For an explicit STAR column, colWidths[i] is overwritten to the pure proportional share (grid L203-204), discarding any content floor captured in the auto pass. Only IMPLICIT star tracks keep the floor via std::max (L210-212). There is no MinWidth/MaxWidth per track and no second pass to re-proportion after a track is clamped.

- **근거 — 일반 프레임워크 기대동작**: WPF Grid star resolution clamps each star track to its Min/Max then redistributes the remainder among unconstrained stars so the row still sums to available; a star track also never shrinks below content it must hold if min-sized.

- **근거 — 프레임워크 레퍼런스**: WPF Grid star sizing with MinWidth/MaxWidth and proportional remainder

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:high]): Finding index 50 = "cache-05" (flex-grow/flex-shrink resize a child but it is NOT re-measured unless MATCH_PARENT). I re-read the cited source at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp.  The observed claims hold exactly: - ApplyFlexGrowShrink mutates workingSizes[idx].width/.height for grow (flex-layout-manager.cpp:147-155) and for shrink (181-185), exactly as cited. - The re-measure call is `childImpl.Measure(childBounds.width, childBounds.height)` guarded by `if(childImpl.GetRequestedWidth() == MATCH_PARENT || childImpl.GetRequestedHeight() == MATCH_PARENT)` (flex-layout-manager.cpp:348-3…

    - `intent`(intentional-design [보정:low]): Index 50 is finding "mp-01" — main-axis MATCH_PARENT children each grab the full main extent in Stack and Flex, so two of them (without weight/flex-grow) overlap and overflow.  CODE CLAIM — CONFIRMED ACCURATE. Stack VERTICAL: stack-layout-manager.cpp:464-467 sets childHeight = max(0, availableHeight - marginH) for a MATCH_PARENT-height child, slotHeight = childHeight + marginH (468), and currentY += slotHeight + visSpacing (508). availableHeight is the full container content height, constant across the loop (it is never decremented by prior children — verified at :393 it is the whole availableMain). So two such children both get the full exte…

- **재현**: Columns {Auto, Star(1)} width 100; a child in the Star column needs 80px min-content but the star share computes 40px -> child clipped/overflow, whereas WPF would keep >=content where required by track minimums.

- **▶ 수정 방향**: `grid-layout-manager.cpp:201-213` 에서 위 '기대동작'을 만족하도록 수정. (분류: spec-violation)


<a id="fix-x-distribution-rounding-dist-03"></a>

#### 🟢 LOW · `dist-03` — No rounding-remainder reconciliation in any distributor; accumulated float error means shares do not sum exactly to available

- **위치**: `stack-layout-manager.cpp:133,410; flex-layout-manager.cpp:147; grid-layout-manager.cpp:204,219,230`

- **현상(버그)**: Each distributor computes independent proportional shares: stack share=(weight/totalWeight)*remainingMain (stack L133/410), flex extra=(grow/totalFlexGrow)*freeSpace (flex L147), grid colWidths[i]=(starValue/totalStarWidth)*remainingWidth (grid L204/219). None tracks an accumulated remainder or assigns leftover to a last track. Grid's totalWidthOut is std::accumulate of the independently-rounded shares (grid L230).

- **근거 — 일반 프레임워크 기대동작**: Android LinearLayout and CSS note explicitly handle the pixel/float remainder so distributed children sum EXACTLY to the available space (last child or error-accumulation absorbs the remainder). Here, sum(shares) can drift from available by N*epsilon for N children, leaving a sub-pixel gap or overshoot at the end of the axis.

- **근거 — 프레임워크 레퍼런스**: Android LinearLayout weight remainder handling; CSS Flexbox §9.7 rounding note

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): I re-read all cited lines in the worktree under /home/jae/dali/dali-ui-claude. The "observed" claim holds exactly:  - stack-layout-manager.cpp:133 — `float share = (weight / totalWeight) * remainingMain;` (Measure pass), and :410 — identical `(weight / totalWeight) * remainingMain` (Arrange pass). Each weighted child's share is computed independently; no per-child remainder is tracked and no leftover is assigned to a last child. - flex-layout-manager.cpp:147 — `float extra = (grow / line.totalFlexGrow) * freeSpace;` distributed per-child independently inside ApplyFlexGrowShrink; line.mainSize is then set to availableMain (:158) without reconc…

- **반론(소수의견)**: `intent`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: Grid with 3 columns Star(1) over width 100 -> each 33.333..; sum 99.999.. leaving a fractional gap; with many star tracks the drift accumulates and the final column does not reach the right edge.

- **▶ 수정 방향**: `stack-layout-manager.cpp:133` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-x-distribution-rounding-dist-06"></a>

#### 🟢 LOW · `dist-06` — weightSum/growSum guarded against zero but a child carrying a factor while sum>0 yet target space is zero yields silent collapse; divide-by-zero avoided everywhere (positive finding) — but NaN/Inf from factors can still propagate

- **위치**: `flex-layout-manager.cpp:120-121,139,147; flex public setter std::max(0.0f, grow) at flex-layout-params-impl.h:76`

- **현상(버그)**: All three guard the divisor: stack uses weight/totalWeight only inside totalWeight>0 branches (L236,391); flex guards line.totalFlexGrow>0 (L139) and totalWeightedShrink>0 (L170); grid guards totalStarWidth>0 (L204). So a pure divide-by-zero is avoided. However factors are floats with no upper bound or finiteness check: a user-supplied Inf/NaN grow (flex-layout-params SetFlexGrow std::max(0.0f, grow) does NOT reject NaN since max(0,NaN)=NaN on many libstdc++ paths) makes totalFlexGrow NaN, then extra=NaN propagates into workingSize and into the actor SIZE property.

- **근거 — 일반 프레임워크 기대동작**: A robust distributor validates finiteness of user factors (reject/clamp NaN/Inf) before they enter size math, since a single NaN size silently poisons the whole line/track and the rendered actor size.

- **근거 — 프레임워크 레퍼런스**: General UI framework input validation for distribution factors

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:low]): The factual "observed" claims are all accurate (re-verified in /home/jae/dali/dali-ui-claude): - Callback measure path: view-impl.cpp:918-921 invokes the callback and the return (931) only passes through ApplyConstraints, which applies min/max ONLY (view-impl.cpp:1419-1428). RequestedWidth/Height is never re-applied. - Manager path DOES force RequestedWidth: DispatchMeasureWithLayoutManager sets resultVisW=requestedVisW when requestedVisW>=0 (view-impl.cpp:2442-2443). - Default OnMeasure DOES force RequestedWidth: size.width=mImpl->mRequestedWidth (view-impl.cpp:996-998). - UTC masking is real: HorizontalLineMeasure returns {widthConstraint, …

    - `code`(confirmed): Every code claim in the "observed" field holds against the actual source at /home/jae/dali/dali-ui-claude.  1. Callback measure path applies only min/max, never RequestedWidth/Height: - view-impl.cpp:905-906 clamps the incoming constraint to min/max only (effNatW = clamp(natW, min, max)). - view-impl.cpp:918-921 invokes the measure callback with effVisW/effVisH; the return is stored verbatim into `visual`. - view-impl.cpp:931 runs `visual = ApplyConstraints(visual)`. ApplyConstraints (view-impl.cpp:1419-1428) only does std::max(.,min*s)/std::min(.,max*s) — purely min/max. It never references mRequestedWidth/Height. So a callback's returned si…

- **재현**: child.SetLayoutParams(FlexLayoutParams::New().SetFlexGrow(NAN)); std::max(0.0f,NAN) returns NAN -> totalFlexGrow NAN -> extra NAN -> child width NAN -> actor SIZE_WIDTH NAN.

- **▶ 수정 방향**: `flex-layout-manager.cpp:120-121` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-x-distribution-rounding-flex-07"></a>

#### 🟢 LOW · `flex-07` — Flex Measure pass does NOT apply grow/shrink, so a flex container's measured (desired) size ignores flex sizing — inconsistent with Arrange

- **위치**: `flex-layout-manager.cpp:450-549 (no ApplyFlexGrowShrink) vs 582-585`

- **현상(버그)**: FlexLayoutManager::Measure (L450-549) builds lines and sums mainSize from basis/measured sizes only; it never calls ApplyFlexGrowShrink. Grow/shrink is applied only in Arrange (L582-585). So the size a flex container reports during its own parent's Measure can differ from what it lays out, and a WRAP_CONTENT flex parent's reported main size is the un-grown sum.

- **근거 — 일반 프레임워크 기대동작**: Consistent two-phase frameworks compute the same resolved item sizes in Measure and Arrange (or document that Measure is intrinsic-only). Mixing intrinsic-Measure with resolved-Arrange is a subtle source of one-frame size mismatch.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox hypothetical main size vs used main size; WPF Measure/Arrange size agreement

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:info]): The factual observation is accurate: FlexLayoutManager::Measure (/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:450-549) sums mainSize from basis/measured (intrinsic) child sizes and never calls ApplyFlexGrowShrink, which is invoked only in Arrange (L582-585, via BuildFlexLinesForArrange at L579 then ApplyFlexGrowShrink at L584). However this Measure/Arrange asymmetry is the deliberately documented contract, not an unintended deviation, and the finding's framework claim is mis-stated.  Intent evidence: - docs/layout-structure.md:131-157 explicitly specifies the two phases. Measure phase: a MATCH_PA…

    - `code`(confirmed [보정:high]): Re-read all cited locations under /home/jae/dali/dali-ui-claude and the "observed" claims hold exactly.  Callback Measure path (view-impl.cpp:897-942): natW/natH derive from the incoming constraint (visualW/H), clamped ONLY to min/max at :905-906 (std::min(std::max(natW, GetMinimumWidth()), GetMaximumWidth())) — mRequestedWidth/Height never enter here. The callback is invoked with effVisW/effVisH (:921) and its return is processed only through ApplyConstraints (:931). ApplyConstraints (view-impl.cpp:1419-1429) applies ONLY min (std::max with GetMinimumWidth*s) and max (std::min with GetMaximumWidth*s); it does NOT force RequestedWidth/Height.…

- **재현**: Flex row WRAP_CONTENT width inside a stretch parent: Measure reports sum of basis; Arrange grows children to fill — parent measured one size, children arranged to another.

- **▶ 수정 방향**: `flex-layout-manager.cpp:450-549` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


### [횡단] 생명주기 변형 (`x-lifecycle-mutation`) — 7건


<a id="fix-x-lifecycle-mutation-child-01"></a>

#### 🟠 HIGH · `child-01` — OnChildOrderChanged wholesale-rebuilds mChildren from Actor order, silently discarding PRESERVE / Insert layout-order divergence

- **위치**: `view-impl.cpp:2826-2851 (full-list rebuild from self.GetChildAt); view-impl.cpp:2040-2049 (PRESERVE deliberately leaves mChildren diverged via mSkipChildrenUpdate); view-impl.cpp:1742-1744 (Insert diverges mChildren from actor order)`

- **현상(버그)**: OnChildOrderChanged (view-impl.cpp:2826-2851) rebuilds the ENTIRE mImpl->mChildren by walking the Actor child list (self.GetChildAt(i)) in visual order and re-collecting handles. It is connected to dali-core's ChildOrderChangedSignal (view-impl.cpp:324) and runs for any UPDATE-policy reorder (and any inherited Actor::Raise/Lower call). It does not preserve a previously-established divergent logical order.

- **근거 — 일반 프레임워크 기대동작**: Once layout order has been intentionally decoupled from visual order (via Insert() or a PRESERVE-policy reorder), a later visual-only/UPDATE reorder of a DIFFERENT sibling must not silently reset every other child's layout order. A general framework reconciles the two orderings incrementally (move only the changed child) or keeps them independent.

- **근거 — 프레임워크 레퍼런스**: Android ViewGroup: a single child array is the authoritative order; reordering one child does not rewrite the indices of unrelated children's layout participation.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Re-read grid-layout-manager.cpp:131-232 (ApplyGridDefinitions), :81-128 (MeasureGridChildrenAndFillAuto auto-floor pass), and :448-449/526-527 (vector init to 0.0f). Every "observed" claim holds:  1. Explicit STAR track overwrites with pure proportional share, no floor: line 203-204 is `float starValue = colDefs[i].GetValue(); colWidths[i] = (totalStarWidth > 0) ? (starValue/totalStarWidth)*remainingWidth : 0.0f;` — plain `=` assignment, discarding any prior value. Furthermore, the auto-floor pass (L118-127) only writes `colWidths[col]` when `colAutoLike` is true (`colDefs.Size()==0 || def type==AUTO`), so an explicit STAR track never even ca…

    - `intent`(confirmed [보정:low]): Re-read grid-layout-manager.cpp:88-128 (auto/floor pass), :131-232 (ApplyGridDefinitions), and layout-types.h:442-531 (GridLength API).  OBSERVED behavior is accurately described and verified: - Explicit STAR column: colWidths[i] = (starValue/totalStarWidth)*remainingWidth, a bare overwrite with no content floor (grid-layout-manager.cpp:201-204). An explicit STAR column is also NOT "autoLike" (the colAutoLike guard at :119 is true only for size()==0 or AUTO), so no content floor is ever captured for it during the auto pass either. - Implicit star track: colWidths[i] = std::max(colWidths[i], share) explicitly preserves the auto-pass content fl…

    - `code2`(intentional-design [보정:info]): The finding's factual mechanism is accurate but its "bug" framing is refuted by the documented contract.  Mechanism (all confirmed in /home/jae/dali/dali-ui-claude): - OnChildOrderChanged (view-impl.cpp:2826-2851) does rebuild the entire mImpl->mChildren by walking actor order (loop at :2838 `self.GetChildAt(i)`), keeping per-actor order; skipped only when mImpl->mSkipChildrenUpdate is set (:2828-2831). - Connected to DevelActor::ChildOrderChangedSignal at view-impl.cpp:324. - PRESERVE policy sets mSkipChildrenUpdate via ScopedSkipChildrenUpdate (Raise: view-impl.cpp:2040-2049; Lower 2053-2067, etc.), so PRESERVE reorders skip the rebuild. - …

- **재현**: parent has [a,b,c]. parent.Insert(0,c) -> logical mChildren=[c,a,b], actor stays [a,b,c]. Then a.Raise(LayoutOrderPolicy::UPDATE) -> actor becomes [b,a,c], OnChildOrderChanged fires and rebuilds mChildren=[b,a,c]. The Insert-established 'c first' layout order is lost; a StackLayout now lays children out as b,a,c instead of c,b,a. Same loss occurs if b had earlier been Raise(PRESERVE)'d.

- **▶ 수정 방향**: `view-impl.cpp:2826-2851` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-x-lifecycle-mutation-child-02"></a>

#### 🟠 HIGH · `child-02` — Mutating children during the plain-View Measure/Arrange pass invalidates the live mChildren iterator (UB)

- **위치**: `view-impl.cpp:966 and 1111 (range-for over mImpl->mChildren); view-impl.cpp:1138 (childImpl.Measure inside the loop); view-impl.cpp:918 (Measure invokes app MeasureCallback); OnChildRemove/Insert Erase mImpl->mChildren (view-impl.cpp:2816,1743). No reentrancy guard exists.`

- **현상(버그)**: ViewImpl::OnMeasure and OnArrange iterate mImpl->mChildren directly with a range-for (view-impl.cpp:966, 1111) and, inside that loop, re-measure MATCH_PARENT children via childImpl.Measure(childW,childH) (view-impl.cpp:1138). Measure dispatches to an application MeasureCallback (view-impl.cpp:918). There is no re-entrancy/layout-in-progress guard anywhere in ViewImpl (grep for mInLayout/mIsMeasuring returns nothing).

- **근거 — 일반 프레임워크 기대동작**: A layout pass must be robust against a child callback that adds/removes a sibling — either by snapshotting, by re-reading the count each iteration, or by forbidding mutation during layout. Iterating a live std::vector-like container while a callback Erases from it is undefined behavior.

- **근거 — 프레임워크 레퍼런스**: Android ViewGroup.layout/measure re-reads getChildCount() per iteration and guards against re-entrant layout; CSS/WPF panels snapshot or forbid mid-pass mutation.

- **근거 — 검증자 코드분석**:

    - `intent`(confirmed [보정:high]): Re-verified all load-bearing claims against /home/jae/dali/dali-ui-claude. (1) mChildren is Dali::Vector<Ui::View> — a contiguous array-backed container whose Erase shifts the buffer and invalidates iterators/pointers [integration-api/view-integ.h:54]. (2) Both ViewImpl::OnMeasure and ViewImpl::OnArrange iterate mImpl->mChildren with a live range-for [public-api/view-impl.cpp:966, 1111]. (3) Inside the OnArrange loop, MATCH_PARENT children are re-measured via childImpl.Measure(childW,childH) [view-impl.cpp:1150] (finding cited 1138 — minor line drift, substance correct); OnMeasure re-measures at :983. (4) Measure() invokes the application Mea…

    - `code2`(confirmed [보정:high]): Index 52 = finding "child-02" (finderKey x-lifecycle-mutation), not pad-01: counting the "id" entries 0-based, child-01=51 and child-02=52 (verified via grep of "id": lines, /tmp/layout-unverified.json line 784 = child-02, JSON object at lines 782-796).  The claim — plain-View Measure/Arrange iterates the live mChildren and a re-entrant MeasureCallback that removes a sibling invalidates the iterator (UB), with no re-entrancy guard — is genuine. Re-derived from source under /home/jae/dali/dali-ui-claude:  1. Container type: mChildren is IntegrationView::ChildContainer = Dali::Vector<Ui::View> (integration-api/view-integ.h:54). Dali::Vector is …

    - `code`(confirmed): Finding pad-01 (index 52): "Padding double-counted for WRAP_CONTENT leaf View with a background visual." I re-read the cited source at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp and confirm both halves of the claim.  1. GetNaturalSize adds padding (cited 2983-2984): at view-impl.cpp:2974-2988, ViewImpl::GetNaturalSize() obtains the BACKGROUND visual's natural size via visualImplPtr->GetNaturalSize(naturalSize), then does `naturalSize.width += (mImpl->mPadding.start + mImpl->mPadding.end);` (2983) and the height analog (2984). If no background visual, it returns Vector3::ZERO (2987) — so the double-count only man…

- **재현**: A plain View parent (no LayoutManager) with children [a(MATCH_PARENT), b]. App sets a MeasureCallback on 'a' that calls parent.Remove(b). During OnArrange's MATCH_PARENT re-measure of 'a', the callback Erases b from mChildren, invalidating the range-for iterator mid-loop -> crash / heap corruption.

- **▶ 수정 방향**: `view-impl.cpp:966` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-x-lifecycle-mutation-child-03"></a>

#### 🟡 MEDIUM · `child-03` — Concrete LayoutManagers iterate a pre-pass child SNAPSHOT, not the live list — mid-pass add/remove is silently ignored

- **위치**: `stack-layout-manager.cpp:217-223 and 341-346 (snapshot into std::vector<View>); childImpl.Measure() calls inside the snapshot loop (e.g. stack-layout-manager.cpp:289). Contrast with the live-iteration plain-View path (view-impl.cpp:966).`

- **현상(버그)**: StackLayoutManager::Measure/Arrange copy all children into a std::vector<View> before the pass (stack-layout-manager.cpp:217-223, 341-346) and iterate that snapshot, calling childImpl.Measure() which can re-enter app callbacks. A child removed mid-pass is still measured/arranged (its View handle keeps it alive but its Actor is detached); a child added mid-pass is not seen until the next pass.

- **근거 — 일반 프레임워크 기대동작**: Android's invariant is that a LayoutManager iterates the LIVE, consistent child list. Snapshotting avoids the UB of child-02 but breaks that invariant: removed children are still positioned (briefly visible/animated against a parent they no longer belong to) and newly-added children are skipped for a frame.

- **근거 — 프레임워크 레퍼런스**: Android RecyclerView.LayoutManager / ViewGroup iterate the live child list and observe structural changes immediately within the pass.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Every code-level claim in child-04 holds against /home/jae/dali/dali-ui-claude.  1) Accessors read layout order: view-impl.cpp:2007-2010 (GetChildCount returns mImpl->mChildren.Count()), 2012-2019 (GetChildAt returns mImpl->mChildren[index]), 2021-2035 (IndexOfChild iterates mImpl->mChildren). All three read mChildren, not Actor sibling order. Confirmed.  2) Divergence from Actor order under PRESERVE is real and intentional. Raise(PRESERVE) at view-impl.cpp:2040-2048 wraps self.Raise() in a ScopedSkipChildrenUpdate guard on the parent's mSkipChildrenUpdate flag; OnChildAdd (2640-2645) and the sibling reorder handlers at 2769/2828 early-return…

    - `intent`(intentional-design [보정:info]): Element 53 = align-01 "CENTER alignment uses unrounded float midpoint". The observed code is accurate: stack-layout-manager.cpp:487 (`crossX += (crossAvailable - childWidth) * 0.5f`) and :537 (`crossY += (crossAvailable - childHeight) * 0.5f`); grid-layout-manager.cpp:306 and :328 (`* 0.5f`); flex-layout-manager.cpp:210 (`out.mainOffset = freeSpace / 2.0f`) and :297 (`childCrossOffset += crossSpace / 2.0f`). All verified by direct Read; none floor/round. A grep for std::round/std::floor/roundf/floorf across dali-ui-foundation/public-api/layouts/ returns nothing, and docs/layout-structure.md states no rounding/snap contract.  This is INTENTION…

- **재현**: StackLayout [a,b,c]; a MeasureCallback on 'a' removes 'c' during the manager's measure pass. The manager finished snapshotting [a,b,c], so it still measures and (in Arrange) positions 'c' this frame even though it has been unparented from mChildren.

- **▶ 수정 방향**: `stack-layout-manager.cpp:217-223` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-x-lifecycle-mutation-child-04"></a>

#### 🟡 MEDIUM · `child-04` — GetChildAt/GetChildCount/IndexOfChild return logical (layout) order, which can diverge from Actor visual order, but this is undocumented

- **위치**: `view-impl.cpp:2012-2019 (GetChildAt reads mChildren); view.h:1312-1318 (doc omits ordering); test UtcDaliViewRaisePreserveP (utc-Dali-View.cpp:2225-2237) asserts IndexOfChild != VisualIndexOf, proving divergence is intended yet undocumented on the accessors.`

- **현상(버그)**: GetChildAt/GetChildCount/IndexOfChild read mImpl->mChildren (view-impl.cpp:2007-2035), i.e. layout order. After Insert() or any PRESERVE reorder, this diverges from the Actor sibling order returned by Dali::Actor::GetChildAt. The public doc (view.h:1305-1326) describes GetChildAt only as 'the child view at the index' with no mention of which of the two orderings it reflects.

- **근거 — 일반 프레임워크 기대동작**: When an API maintains two divergent orderings, the accessor's doc must state which order it returns. Callers mixing View::GetChildAt with the inherited Dali::Actor::GetChildAt (still reachable on a View) will get different children for the same index after any Insert/PRESERVE op.

- **근거 — 프레임워크 레퍼런스**: Android getChildAt(i) is unambiguously the single child array; WPF Children indexer is the single visual+layout order.

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:info]): Re-read the cited source. GetChildAt/GetChildCount/IndexOfChild do read mImpl->mChildren, i.e. layout order (view-impl.cpp:2007-2035, verified). The GetChildAt docstring (view.h:1312-1318) indeed says only "The child view at the index" with no ordering qualifier (verified). The divergence between layout order and Actor visual order is real and deliberately pinned: UtcDaliViewRaisePreserveP (utc-Dali-View.cpp:2225-2237) asserts IndexOfChild==1 while VisualIndexOf==2 after Raise(PRESERVE), and the UPDATE/PRESERVE pair is exhaustively tested.  But the divergence is NOT undocumented at the contract level: the entire Raise/Lower(LayoutOrderPolicy)…

    - `code`(confirmed [보정:low]): All cited facts verified against /home/jae/dali/dali-ui-claude.  view.h:1327-1335 re-exposes the inherited reorder API via using-declarations, including line 1330 `using Dali::Actor::Raise;`. view.h:1345 declares `void Raise(LayoutOrderPolicy policy);` with NO default argument (and the same pattern for Lower/RaiseToTop/LowerToBottom/RaiseAbove/LowerBelow at :1355-1395). Because the policy overload requires one argument and has no default, a zero-arg call `view.Raise()` cannot bind to it; overload resolution selects the inherited `Dali::Actor::Raise()` brought in by the using-declaration. The overload-resolution claim holds.  The semantics cla…

- **재현**: After parent.Insert(0,c), parent.GetChildAt(0)==c but static_cast<Actor>(parent).GetChildAt(0)==a. A developer iterating Actor children for hit-testing vs View children for layout sees inconsistent order with no doc warning.

- **▶ 수정 방향**: `view-impl.cpp:2012-2019` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-x-lifecycle-mutation-child-05"></a>

#### 🟢 LOW · `child-05` — Dual reorder API surface: inherited Dali::Actor::Raise() (no-arg) coexists with Raise(LayoutOrderPolicy) and behaves like UPDATE, a footgun

- **위치**: `view.h:1330 (using Dali::Actor::Raise) and view.h:1345 (void Raise(LayoutOrderPolicy) — no default); view-impl.cpp:2037-2051 (policy path) vs the inherited path that only fires the signal.`

- **현상(버그)**: view.h re-exposes the inherited no-arg Dali::Actor::Raise/Lower/RaiseToTop/etc via using-declarations (view.h:1327-1332) alongside the new policy overloads (view.h:1345-1395). The policy overloads have NO default argument, so calling raise without a policy resolves to the inherited Actor version, which fires ChildOrderChangedSignal -> OnChildOrderChanged -> effectively UPDATE semantics, but through a different code path than the explicit UPDATE policy.

- **근거 — 일반 프레임워크 기대동작**: A single, unambiguous reorder API, or a defaulted policy parameter, so that the layout-order side effect of a reorder is never silently determined by overload resolution.

- **근거 — 프레임워크 레퍼런스**: Android exposes a single bringChildToFront/setZ surface; overload-resolution-dependent semantics are avoided.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): Index 55 = finding "child-05" (Dual reorder API surface). I re-read every cited location under /home/jae/dali/dali-ui-claude and the claims hold.  (1) view.h:1327-1332 contains using-declarations re-exposing the inherited no-arg Dali::Actor reorder API, including `using Dali::Actor::Raise;` at view.h:1330 (and Lower/LowerBelow/LowerToBottom/RaiseAbove/RaiseToTop). (2) view.h:1345 declares `void Raise(LayoutOrderPolicy policy);` with NO default argument (the surrounding policy overloads 1345-1395 likewise have no default). So overload resolution: `view.Raise()` (no args) can only match the inherited Dali::Actor::Raise(); `view.Raise(UPDATE/PRE…

    - `intent`(intentional-design [보정:info]): Index 55 = child-05. The structural facts are accurate but the framed defect is not a real deviation; it is documented intended design.  Verified facts: - view.h:1327-1332 re-expose inherited Dali::Actor::Raise/Lower/RaiseAbove/etc via using-declarations, and view.h:1345 declares Raise(LayoutOrderPolicy) with no default argument. So a no-arg view.Raise() does bind to the inherited Actor overload (view.h confirmed). - The using-declarations are an explicit, commented design choice: view.h:1333-1335 documents that re-exposing the inherited overload is required because the new single-arg overload would otherwise hide all inherited overloads via …

- **재현**: Developer writes view.Raise() expecting the new policy-aware API; it silently binds to Dali::Actor::Raise() and mutates layout order (UPDATE-like) even when PRESERVE was intended.

- **▶ 수정 방향**: `view.h:1330` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-x-lifecycle-mutation-child-06"></a>

#### 🟢 LOW · `child-06` — Insert invalidates Measure while OnChildOrderChanged invalidates only Arrange for the same logical-reorder operation

- **위치**: `view-impl.cpp:1771 (Insert -> InvalidateMeasure) vs view-impl.cpp:2868 (OnChildOrderChanged -> InvalidateArrange)`

- **현상(버그)**: A pure logical reorder via Insert() calls InvalidateMeasure() (view-impl.cpp:1771), whereas the actor-driven reorder via OnChildOrderChanged calls InvalidateArrange() (view-impl.cpp:2868). Both represent 'child order changed, sizes unchanged'.

- **근거 — 일반 프레임워크 기대동작**: Equivalent operations (reorder without size change) should use the same invalidation level. A reorder generally needs only Arrange; Insert over-invalidates with Measure (correct but heavier and inconsistent with the sibling path).

- **근거 — 프레임워크 레퍼런스**: WPF distinguishes InvalidateMeasure vs InvalidateArrange; a reorder is an arrange-only concern.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): All cited facts verified in /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp. (1) ViewImpl::Insert (defined at :1699) ends its reorder branch with InvalidateMeasure() at :1771 — confirmed exactly as cited. (2) ViewImpl::OnChildOrderChanged (defined at :2826) ends with InvalidateArrange() at :2868 — confirmed exactly as cited. Both functions handle a child-order change with sizes unchanged: Insert's comment at :1765-1771 says "mChildren order affects layout output ... this is the only invalidation point for the reorder," and OnChildOrderChanged rebuilds mChildren from actor-tree order then arranges. The "observed" clai…

- **반론(소수의견)**: `intent`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: Insert(0,c) on a fixed-size StackLayout forces a full re-measure of the subtree though only positions change; an equivalent UPDATE Raise of the same child triggers only re-arrange.

- **▶ 수정 방향**: `view-impl.cpp:1771` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-x-lifecycle-mutation-child-07"></a>

#### ⚪ INFO · `child-07` — StackLayoutManager defines a private IsChildStandalone instead of the base LayoutManager::IsStandalone helper used by Absolute/ScrollView

- **위치**: `layout-manager.cpp:57-60 (base helper); absolute-layout-manager.cpp:89,195 and scroll-view-layout-manager.cpp:73,116 (use base helper); stack-layout-manager.cpp:52-55 (private duplicate)`

- **현상(버그)**: LayoutManager exposes a protected IsStandalone(ViewImpl*) (layout-manager.cpp:57-60). AbsoluteLayoutManager and ScrollViewLayoutManager use it, but StackLayoutManager defines and uses its own free function IsChildStandalone (stack-layout-manager.cpp:52-55).

- **근거 — 일반 프레임워크 기대동작**: Concrete managers should share one standalone predicate; duplicating it risks divergence if the standalone definition changes.

- **근거 — 프레임워크 레퍼런스**: N/A — internal consistency convention.

- **근거 — 검증자 코드분석**:

    - `code`(confirmed): Index 57 = finding "child-07" (x-lifecycle-mutation): StackLayoutManager defines a private IsChildStandalone instead of the base LayoutManager::IsStandalone helper used by Absolute/ScrollView. I re-read every cited file:  1. Base helper confirmed: /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-manager.cpp:57-60 defines `bool LayoutManager::IsStandalone(ViewImpl* child) const { return child && child->GetLayoutMode() == LayoutMode::STANDALONE; }` — exactly as cited.  2. Stack private duplicate confirmed: stack-layout-manager.cpp:52-55 defines a free function `bool IsChildStandalone(ViewImpl& childImpl) { return child…

    - `intent`(confirmed [보정:low]): Index 57 = finding "num-05" (Stack weight / Flex basis accept arbitrary values; NaN/Inf propagation). Re-verified against the worktree.  CONFIRMED parts: - The validation asymmetry is real: SetFlexGrow/SetFlexShrink clamp via std::max(0.0f, x) (flex-layout-params-impl.h:76, 94) while SetFlexBasis stores raw (flex-layout-params-impl.h:112) and SetWeight stores raw (stack-layout-params-impl.h:69-72). No NaN/Inf/negative rejection on weight or basis. No doc states a validation contract (docs/layout-structure.md has no NaN/validate/clamp mention) and no UTC exercises NaN/Inf weight or basis (utc-Dali-StackLayout.cpp:162-338, utc-Dali-FlexLayout.c…

- **재현**: If standalone semantics gain a second condition (e.g. visibility==GONE), the base helper updates but Stack's private copy silently does not.

- **▶ 수정 방향**: `layout-manager.cpp:57-60` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


### [횡단] 수치/센티넬 경계 (`x-numeric-edge-cases`) — 7건


<a id="fix-x-numeric-edge-cases-num-01"></a>

#### 🟠 HIGH · `num-01` — RequestedWidth/Height, Minimum/Maximum size accept negative/NaN/Inf with no validation; negative Minimum defeats the sentinel firewall

- **위치**: `view-data-impl.cpp:1381-1471 (no clamp); view-impl.cpp:905-906 (sole sentinel firewall depends on minimum>=0); view-data-impl.h:632-647 (defaults)`

- **현상(버그)**: REQUESTED_WIDTH/HEIGHT SetProperty stores the raw float and only checks FloatEqual for change (view-data-impl.cpp:1381-1390, 1400-1410); MINIMUM_/MAXIMUM_WIDTH/HEIGHT likewise store verbatim (:1421-1471). No clamp to >=0, no NaN/Inf rejection. The whole engine's defence against the -1/-2 sentinels leaking into arithmetic is the single line effNatW=min(max(natW,GetMinimumWidth()),GetMaximumWidth()) (view-impl.cpp:905). Default minimum is 0 (view-data-impl.h:634), so max(sentinel,0) scrubs it. But SetMinimumWidth(-5) makes that max() no longer clamp to >=0: a WRAP_CONTENT(-1) or MATCH_PARENT(-2) constraint then survives as a negative effNatW and flows into OnMeasure/managers as a literal negative pixel constraint.

- **근거 — 일반 프레임워크 기대동작**: A general UI framework validates layout inputs: Android masks the size to 30 bits and clamps negatives to 0; CSS clamps negative lengths to 0 and drops NaN/invalid; WPF throws/coerces on negative or NaN Width. Minimum should be clamped to >=0 (and <= Maximum) so the sentinel-scrub invariant cannot be disabled by user input.

- **근거 — 프레임워크 레퍼런스**: Android View.MeasureSpec (30-bit MODE_MASK + getChildMeasureSpec clamping); CSS Values & Units negative-length handling

- **근거 — 검증자 코드분석**:

    - `code2`(confirmed [보정:medium]): Re-derived both arrange paths for a standalone MATCH_PARENT child and confirmed the divergence is real.  ALL citations re-verified under /home/jae/dali/dali-ui-claude:  PATH A (parent's pass): ArrangeStandaloneChildren (view-impl.cpp:1180-1191) calls ArrangeStandaloneChild(childImpl, bounds.width, bounds.height) passing the parent's full arranged width. ArrangeStandaloneChild (view-impl.cpp:268-296) for a MATCH_PARENT width sets childW = std::max(0, parentFullWidth - marginW) with NO min/max clamp (line 281). It then calls childImpl.Measure(childW,...) (line 289) and childImpl.Arrange(bounds{...,childW,...}) (line 295). The Measure return is …

    - `intent`(confirmed [보정:high]): Element 58 = id "num-01". All structural claims verified against the worktree (paths resolve to dali-ui-foundation/internal/views/view/view-data-impl.cpp and public-api/view-impl.cpp; the JSON's bare "view-data-impl.cpp" maps to the internal path).  (1) No input validation: view-data-impl.cpp:1375-1473 — REQUESTED_WIDTH/HEIGHT (1375-1413) and MINIMUM_/MAXIMUM_WIDTH/HEIGHT (1415-1473) each only do FloatEqual change-detection then store the raw float (mRequestedWidth=width; EnsureSizeConstraints().minWidth=width; etc.). No clamp to >=0, no NaN/Inf rejection. Confirmed.  (2) Single sentinel firewall: view-impl.cpp:905-906 is literally the only s…

- **반론(소수의견)**: `code`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: view.SetMinimumWidth(-5.0f); leave RequestedWidth default (WRAP_CONTENT). Parent measures child: natW comes through as -1 (a sentinel), max(-1,-5) = -1, so OnMeasure receives -1 as the visual width constraint and treats it as content space; subsequent max(0, contentWidth*s - margin) zeroes it, silently collapsing the subtree instead of sizing to content.

- **▶ 수정 방향**: `view-data-impl.cpp:1381-1471` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-x-numeric-edge-cases-num-03"></a>

#### 🟡 MEDIUM · `num-03` — MeasuredSize has no measured-state/overflow flag — clamping and overflow are silent and undetectable by parents

- **위치**: `layout-types.h:100-171 (no flag field); view-impl.cpp:1424-1427 (silent clamp); flex shrink std::max(0,...) flex-layout-manager.cpp:181,185`

- **현상(버그)**: MeasuredSize is exactly {float width; float height;} with no mode or state bits (layout-types.h:100-171). ApplyConstraints silently clamps a child's desired size to min/max (view-impl.cpp:1419-1429) and the Stack/Flex/Grid distribution code silently max(0,...)-clamps shares, but none of this is surfaced. A parent cannot tell whether a child was forced smaller than its content.

- **근거 — 일반 프레임워크 기대동작**: Android's measured dimensions carry MEASURED_STATE_TOO_SMALL / MEASURED_STATE_MASK precisely so a parent (e.g. a scroller) can react to children that did not fit. A general framework either reports the constrained state or guarantees DesiredSize is the unclamped need; DALi does neither.

- **근거 — 프레임워크 레퍼런스**: Android View.MEASURED_STATE_TOO_SMALL / combineMeasuredStates

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Finding index 60 is num-03 (finderKey x-numeric-edge-cases). I re-read every cited location at /home/jae/dali/dali-ui-claude and all three code-level claims hold exactly:  1) MeasuredSize is exactly two floats plus accessors — layout-types.h:100-171 shows only `float width; float height;` (169-170), a default ctor and (w,h) ctor, Get/SetWidth, Get/SetHeight, ToVector2. There is NO mode, state, or overflow bit. A grep for MEASURED_STATE/TOO_SMALL/overflow in layout-types.h returns nothing.  2) ApplyConstraints silently clamps to min/max — view-impl.cpp:1419-1429: `constrained.width = std::max(.., min*s)` then `std::min(.., max*s)` (1424-1427),…

- **반론(소수의견)**: `intent`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: A child with content width 300 and MaximumWidth 50 inside a horizontal Stack reports 50; the Stack has no way to know the child was over-constrained, so overflow/scroll affordances cannot be triggered automatically.

- **▶ 수정 방향**: `layout-types.h:100-171` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-x-numeric-edge-cases-num-04"></a>

#### 🟡 MEDIUM · `num-04` — GridLength::Absolute accepts negative pixels and leaks straight into track widths and remaining-space math

- **위치**: `layout-types.h:479-485 (no clamp); grid-layout-manager.cpp:147-151,196-197,234-249,275-276`

- **현상(버그)**: GridLength::Absolute(float pixels) stores the value with no clamp (layout-types.h:479-485). In ApplyGridDefinitions an ABSOLUTE track does colWidths[i] = def.GetValue()*scale and totalAbsoluteWidth += colWidths[i] (grid-layout-manager.cpp:147-151,173-177). A negative absolute length makes colWidths[i] negative and DECREASES totalAbsoluteWidth, so remainingWidth = max(0, available - totalAbsoluteWidth - spacing) (:196-197) grows, and ComputeGridPositions produces a cell whose right edge is left of its left edge (negative width), later max(0,..)-clamped at the child margin step (:291) but only after corrupting neighbouring column positions.

- **근거 — 일반 프레임워크 기대동작**: CSS Grid treats negative track sizes as invalid and clamps to 0/min-content; track sizing never goes negative. Absolute(pixels) should clamp pixels to >=0.

- **근거 — 프레임워크 레퍼런스**: CSS Grid Layout track sizing (negative <track-size> invalid)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Re-read every cited site under /home/jae/dali/dali-ui-claude. (1) GridLength::Absolute(float pixels) at layout-types.h:479-485 stores length.mValue = pixels with NO clamp; GetValue() at :525-528 returns mValue raw. (2) In ApplyGridDefinitions, an ABSOLUTE col does colWidths[i] = def.GetValue()*scale and totalAbsoluteWidth += colWidths[i] (grid-layout-manager.cpp:147-151); rows analogous (:173-177). A negative Absolute therefore makes colWidths[i] negative and DECREASES totalAbsoluteWidth. (3) remainingWidth = std::max(0, availableWidth - totalAbsoluteWidth - totalSpacingWidth) (:196): subtracting a negative totalAbsoluteWidth INFLATES remaini…

    - `intent`(confirmed): Re-verified all cited code in the worktree.  (1) No clamp at source: GridLength::Absolute(float pixels) at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-types.h:479-485 stores mValue = pixels verbatim with no validation; the default ctor (467-471) is the only other producer and yields Absolute 0. No negative guard anywhere in the class.  (2) Leak into track math confirmed at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp:147-151 (ABSOLUTE col): colWidths[i] = def.GetValue()*scale (can be negative) and totalAbsoluteWidth += colWidths[i] (decreases the total). At :196 rem…

- **재현**: GridLayoutManager with column definitions { Absolute(-100), Star(1) }: the negative column inflates the star column by 100px and shifts colPositions, so the star cell is mis-placed and the negative cell overlaps its neighbour.

- **▶ 수정 방향**: `layout-types.h:479-485` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-x-numeric-edge-cases-num-05"></a>

#### 🟡 MEDIUM · `num-05` — Stack weight and Flex basis accept arbitrary values; NaN weight/basis propagates through division and max() poisoning the whole line

- **위치**: `stack-layout-params-impl.h:71 (weight unclamped) vs flex-layout-params-impl.h:76,93 (grow/shrink clamped) — inconsistent; stack-layout-manager.cpp:133,410; flex-layout-manager.cpp:147`

- **현상(버그)**: StackLayoutParamsImpl::SetWeight stores the raw float with no clamp (stack-layout-params-impl.h:71); FlexLayoutParamsImpl::SetFlexBasis stores raw (flex-layout-params-impl.h, SetFlexBasis with no max()). Stack computes share = (weight/totalWeight)*remainingMain (stack-layout-manager.cpp:133,410) and Flex divides by line.totalFlexGrow (flex-layout-manager.cpp:147). A NaN weight makes weight>0.0f false (so it is skipped in counting) yet if a single child has weight=NaN the totalWeight stays 0 and that child is treated as non-weight — inconsistent; an Inf weight makes weight/totalWeight = Inf/Inf = NaN, and NaN then flows into childHeightConstraint and the child's measured size, and via std::max(maxCrossAxis, NaN) poisons the container size (std::max with NaN is order-dependent).

- **근거 — 일반 프레임워크 기대동작**: Flex grow/shrink are correctly clamped with std::max(0.0f,..) (flex-layout-params-impl.h:76,93); weight and basis should be clamped/sanitised the same way. CSS rejects negative flex-basis and treats NaN as invalid; Android LinearLayout weight is a float but layout clamps the resulting size to >=0 and never divides by a poisoned total.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox flex-basis/flex-grow value parsing; Android LinearLayout weight distribution

- **근거 — 검증자 코드분석**:

    - `intent`(confirmed [보정:medium]): Re-verified all cited facts in the worktree. (1) StackLayoutParamsImpl::SetWeight (dali-ui-foundation/internal/layouts/stack-layout-params-impl.h:69-72) stores the raw float with no clamp. (2) FlexLayoutParamsImpl clamps grow and shrink via std::max(0.0f,...) (flex-layout-params-impl.h:76, 94) but SetFlexBasis (lines 110-113) stores raw — the asymmetry the finding asserts is real and confirmed. (3) The two Stack guards genuinely disagree on non-finite input: accumulation uses weight > 0.0f (stack-layout-manager.cpp:83) while distribution uses weight <= 0.0f (line 128). A NaN weight fails BOTH (NaN>0 false; NaN<=0 false), so it is excluded fro…

- **반론(소수의견)**: `code`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: child.SetLayoutParams(StackLayoutParams::New().SetWeight(std::numeric_limits<float>::infinity())) in a vertical Stack: share=Inf/Inf=NaN, child height constraint NaN, maxCrossAxis via std::max(.,NaN) becomes order-dependent, container reports NaN height and SIZE_HEIGHT is set to NaN on the actor.

- **▶ 수정 방향**: `stack-layout-params-impl.h:71` 에서 위 '기대동작'을 만족하도록 수정. (분류: bug)


<a id="fix-x-numeric-edge-cases-num-06"></a>

#### 🟢 LOW · `num-06` — Min/Max conflict resolution is max-wins via order-dependent max-then-min, not a documented invariant

- **위치**: `view-impl.cpp:905,1424-1427; utc-Dali-View.cpp:904-914`

- **현상(버그)**: When MinimumWidth > MaximumWidth, both ViewImpl::Measure (view-impl.cpp:905 min(max(natW,min),max)) and ApplyConstraints (view-impl.cpp:1424-1427 max then min) apply max(x,min) THEN min(x,max), so the Maximum wins (result = max). UtcDaliViewApplyConstraintsMinMaxP asserts exactly this (max wins: width 50 when min 80/max 50) (utc-Dali-View.cpp:904-914).

- **근거 — 일반 프레임워크 기대동작**: WPF clamps with Max(min, Min(max, value)) and documents that MinWidth wins when MinWidth>MaxWidth (the opposite resolution). The DALi choice is internally consistent and tested, but it is the inverse of the common framework convention and is only pinned by a test, not by validation that prevents the inverted range.

- **근거 — 프레임워크 레퍼런스**: WPF FrameworkElement MinWidth/MaxWidth coercion (MinWidth takes precedence)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): All three citations re-read under /home/jae/dali/dali-ui-claude and verified exactly as the finding claims.  view-impl.cpp:905 — `float effNatW = std::min(std::max(natW, mImpl->GetMinimumWidth()), mImpl->GetMaximumWidth());` applies max(natW,min) THEN min(...,max). With min=80, max=50, natW<80: max(natW,80)=80, then min(80,50)=50 → Maximum wins. Line 906 identical for height.  ApplyConstraints view-impl.cpp:1424-1427 — order is max(min) first (1424/1425) then min(max) last (1426/1427), so for an inverted range the final min(...,max) clamps the result down to Maximum. Maximum wins here too. Both the Measure-pass clamp and ApplyConstraints agre…

    - `intent`(intentional-design [보정:low]): Re-verified all citations under /home/jae/dali/dali-ui-claude. The code behaves as the finding describes: view-impl.cpp:905 uses std::min(std::max(natW, GetMinimumWidth()), GetMaximumWidth()) and ApplyConstraints at view-impl.cpp:1424-1427 applies max(.,min) THEN min(.,max). With min>max both yield max-wins. Defaults verified in internal/views/view/view-data-impl.h:642 (max default = numeric_limits<float>::max(), min default 0), so the only clamp in the test comes from the explicit min=80/max=50.  This max-wins resolution is DELIBERATELY pinned by a Positive UTC: UtcDaliViewApplyConstraintsMinMaxP (automated-tests/.../utc-Dali-View.cpp:904-91…

- **재현**: view.SetMinimumWidth(80); view.SetMaximumWidth(50); → measured width 50 (max wins). A developer porting from WPF would expect 80 (min wins).

- **▶ 수정 방향**: `view-impl.cpp:905` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-x-numeric-edge-cases-num-07"></a>

#### 🟢 LOW · `num-07` — Absolute layout cannot express a real -1px/-2px bound and silently reinterprets any negative bound as auto

- **위치**: `absolute-layout-params-impl.h:48,71; absolute-layout-manager.cpp:43,145-152,237-267`

- **현상(버그)**: AbsoluteLayoutParams default bounds are LayoutRect(0,0,-1,-1) (absolute-layout-params-impl.h:48) and the manager treats any w<0 / h<0 as 'use measured size' (absolute-layout-manager.cpp:145-152,237-267). SetBounds stores the rect verbatim (:71). There is no separate 'unset' flag, so a bound of exactly -1 (==WRAP_CONTENT) or -2 (==MATCH_PARENT) is indistinguishable from 'auto', and MATCH_PARENT/WRAP_CONTENT semantics are conflated with the absolute-bounds sentinel.

- **근거 — 일반 프레임워크 기대동작**: An absolute/canvas layout (WPF Canvas, Android FrameLayout) uses a distinct unset marker (NaN/Auto) separate from valid coordinates and does not overload the same negative range that the global WRAP_CONTENT/MATCH_PARENT sentinels use. Negative sizes should clamp to 0, not silently mean auto.

- **근거 — 프레임워크 레퍼런스**: WPF Canvas attached properties (Double.NaN = unset); Android FrameLayout

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Re-read /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:1577-1597. AttachLayoutManager begins with two hard runtime asserts: DALI_ASSERT_ALWAYS(manager...) at :1579 and DALI_ASSERT_ALWAYS(!HasLayoutManager() && "LayoutManager already set. Cannot replace an existing LayoutManager.") at :1580. HasLayoutManager() (:1594-1597) returns GetLayoutManager()!=nullptr, so a second AttachLayoutManager call on a View that already has a manager aborts via DALI_ASSERT_ALWAYS — matching the scenario's claim that switching Stack->Grid at runtime crashes the process.  The header view.h:290-305 documents this exactly: "Only one Layout…

    - `intent`(intentional-design [보정:info]): This is a documented, test-pinned design choice, and the finding's "expected" framework framing is also partly wrong.  1. Documented contract: absolute-layout-params.h:144-147 and 159-162 explicitly document SetWidth/SetHeight: "(-1 means use View's own size)". The default bounds (0,0,-1,-1) at absolute-layout-params-impl.h:48 therefore deliberately mean "no size override; defer to the View's own measured size." This is intentional, not a silent reinterpretation.  2. No semantic conflation: The finding claims a -1/-2 bound conflates WRAP_CONTENT vs MATCH_PARENT with an "absolute-bounds sentinel." This is refuted by absolute-layout-manager.cpp…

- **재현**: A developer sets AbsoluteLayoutParams width to -1 expecting an error or 0; instead the child silently falls back to its measured/natural width as if 'auto', masking the mistake.

- **▶ 수정 방향**: `absolute-layout-params-impl.h:48` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-x-numeric-edge-cases-num-08"></a>

#### 🟢 LOW · `num-08` — NaN constraint defeats the Measure cache (FloatEqual is false for NaN), forcing re-measure every frame

- **위치**: `view-impl.cpp:188-191,908-909; reachable via view-data-impl.cpp:1381 (no NaN rejection) and num-05`

- **현상(버그)**: The measure cache hit test is mLastMeasuredConstraint.width >= 0.0f && FloatEqual(prev,effNatW) (view-impl.cpp:908-909) with FloatEqual = abs(a-b)<eps (view-impl.cpp:188-191). If a NaN size ever reaches a view (e.g. via num-05 weight poisoning or user SetRequestedWidth(NaN) since num-01 allows it), effNatW becomes NaN, FloatEqual(NaN,NaN) is false, and the cache never hits, so the whole subtree re-measures every layout pass even when nothing changed.

- **근거 — 일반 프레임워크 기대동작**: A robust cache either sanitises NaN out of the constraint or treats NaN-vs-NaN as equal (or asserts). Android/WPF never let NaN into the measure constraint in the first place.

- **근거 — 프레임워크 레퍼런스**: WPF Measure caching keyed on availableSize equality (NaN excluded upstream)

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:info]): The mechanism the finding describes is real and verified: FloatEqual is std::abs(a-b)<0.001f (view-impl.cpp:188-191), which is false for NaN-vs-NaN; the cache-hit test (view-impl.cpp:908-909) FloatEquals both width and height, so a NaN effNatW can never hit and forces re-measure every pass; and a NaN requested size is not rejected at view-data-impl.cpp:1381 (only an idempotence FloatEqual + raw store), with no NaN sanitization in layout-controller.cpp or in Measure (view-impl.cpp:897-906). HOWEVER, the NaN-defeats-FloatEqual property is a DELIBERATELY DESIGNED part of the cache state machine, not an oversight. view-impl.cpp:113-123 documents …

    - `code`(confirmed [보정:low]): Finding index 65 is num-06 ("Min/Max conflict resolution is max-wins via order-dependent max-then-min"). I re-read all three cited locations under /home/jae/dali/dali-ui-claude and every observed claim holds exactly:  1. view-impl.cpp:905 — `float effNatW = std::min(std::max(natW, mImpl->GetMinimumWidth()), mImpl->GetMaximumWidth());` (and :906 for height). This applies max(natW,min) FIRST, then min(...,max) outermost, so when MinimumWidth > MaximumWidth the inner max(natW,min) yields >= min, and the outer min(...,max) clamps down to max. Maximum wins. Confirmed.  2. view-impl.cpp:1419-1428 ApplyConstraints — width = max(width, min*s) at :142…

- **재현**: After a NaN size leaks into a deep view, every ProcessLayouts pass re-runs full Measure on that subtree (cache permanently missing), a silent per-frame performance cliff.

- **▶ 수정 방향**: `view-impl.cpp:188-191` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


### [횡단] 테스트 커버리지 (`x-test-coverage-gaps`) — 12건


<a id="fix-x-test-coverage-gaps-flex-01"></a>

#### 🟠 HIGH · `flex-01` — flex-grow / flex-shrink distribution has NO value-asserting test anywhere

- **위치**: `utc-Dali-FlexLayout.cpp:378-398 (MeasureArrange only checks 200x100), :401-463 (justify/align variants assert only container size), :885-918 of utc-Dali-ViewLayoutBoundary.cpp (comment 'we don't need a full FlexLayout type ... runs entirely through the ViewImpl default path'); algorithm under test ApplyFlexGrowShrink flex-layout-manager.cpp:133-191.`

- **현상(버그)**: Every FlexLayout test that exercises grow/shrink only asserts the container's measured/arranged size (e.g. UtcDaliFlexLayoutMeasureArrangeP asserts m=200x100, a=200x100). No test sets FlexGrow/FlexShrink on children AND asserts the resulting child width/height. The boundary 'Flex' test (UtcDaliViewLayoutBoundary_FlexRepeatedPassNoAccumulation_P) explicitly runs the plain ViewImpl path, not FlexLayoutManager.

- **근거 — 일반 프레임워크 기대동작**: A flex test suite must pin the core algorithm: e.g. two children grow=1/grow=2 in a 300px row should arrange to specific widths; shrink with overflow should produce specific reduced widths; the weighted-shrink formula (shrink*size) should be asserted against a hand-computed value.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox 9.7 'Resolving Flexible Lengths'

- **근거 — 검증자 코드분석**:

    - `code2`(confirmed [보정:medium]): Re-derived from scratch. The grow/shrink distribution algorithm ApplyFlexGrowShrink exists at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:133-191 and is reachable (called from Arrange at :584), so it is live code, not dead.  Test-suite census: SetFlexGrow/SetFlexShrink appear in the entire automated-tests tree ONLY in utc-Dali-FlexLayout.cpp at lines 196, 218, 242 — all inside getter/setter round-trip tests (UtcDaliFlexLayoutSetFlexGrowP:190, SetFlexShrinkP:212, NegativeFlexFactorClampedP:234) that assert only GetFlexGrow()/GetFlexShrink() property values, never a laid-out child size.  Every Fle…

    - `code`(confirmed [보정:medium]): Re-read all cited code under /home/jae/dali/dali-ui-claude. The "observed" claim holds on both paths.  Parent-pass / standalone path: ArrangeStandaloneChild (view-impl.cpp:268-296) computes the MATCH_PARENT fill as childW = std::max(0.0f, parentFullWidth - marginW) (line 281) and childH likewise (285) with NO clamp to the child's Minimum/Maximum. It then calls childImpl.Measure(childW, childH) (289) — Measure internally clamps only its constraint (905-906) and its returned MeasuredSize via ApplyConstraints (931) — but the arrange bounds at line 292-294 use the raw childW/childH fill values, NOT GetMeasuredSize(), so any internal clamp is disc…

    - `intent`(confirmed [보정:high]): This is a missing-coverage finding (severity high, category missing-coverage), and the test gap is real. I verified the entire flex test suite at /home/jae/dali/dali-ui-claude/automated-tests/src/dali-ui-foundation/utc-Dali-FlexLayout.cpp (671 lines, 44 tests):  1. The only FlexGrow/FlexShrink references are getter/setter round-trip assertions: SetFlexGrowP (190-197), GetFlexGrowP (201-208), SetFlexShrinkP (212-219), GetFlexShrinkP (223-230), NegativeFlexFactorClampedP (234-244), and FlexBasis equivalents (248-266). Every one asserts only child.GetLayoutParams<FlexLayoutParams>().GetFlexGrow()/GetFlexShrink() returns the stored value — none a…

- **재현**: FlexLayout ROW width=300; childA grow=1 basis=50, childB grow=1 basis=50 → expect each 150px. childA shrink=1 w=200, childB shrink=1 w=200 in 300px → expect each 150px. None asserted.

- **▶ 수정 방향**: `utc-Dali-FlexLayout.cpp:378-398` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-x-test-coverage-gaps-cov-01"></a>

#### 🟠 HIGH · `cov-01` — LayoutController has zero active tests (entire UTC commented out)

- **위치**: `utc-Dali-LayoutController.cpp:37-92 (all four candidate tests are inside // comment blocks); public driver layout-controller.cpp is 18949 bytes of untested logic.`

- **현상(버그)**: utc-Dali-LayoutController.cpp contains only commented-out test bodies (Get, OnWindowResize, ProcessLayouts, same-window-singleton). There are no active assertions, so the per-frame layout driver, pending-root registration, window-resize re-layout, and per-window singleton identity are entirely unverified.

- **근거 — 일반 프레임워크 기대동작**: A layout engine's frame driver is core: tests should verify that InvalidateMeasure registers the root, ProcessLayouts measures+arranges exactly the dirty roots once, OnWindowResize re-lays-out, and Get(window) returns a stable per-window instance.

- **근거 — 프레임워크 레퍼런스**: WPF LayoutManager / Dispatcher layout pass; Android ViewRootImpl.performTraversals

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:high]): I re-read the entire cited file /home/jae/dali/dali-ui-claude/automated-tests/src/dali-ui-foundation/utc-Dali-LayoutController.cpp (all 93 lines). The finding's "observed" claim holds exactly:  - The file contains only the license header (1-16), includes (18-22), and the startup/cleanup hooks utc_dali_layoutcontroller_startup/cleanup (27-35), which merely set test_return_value and contain NO DALI_TEST_* assertions. - All four candidate test functions are commented out with leading `//` on every line: UtcDaliLayoutControllerGetP (37-49), UtcDaliLayoutControllerOnWindowResizeP (51-63), UtcDaliLayoutControllerProcessLayoutsP (65-77), UtcDaliLayo…

    - `intent`(confirmed [보정:medium]): Re-read /home/jae/dali/dali-ui-claude/automated-tests/src/dali-ui-foundation/utc-Dali-LayoutController.cpp:1-93. The literal core claim is CONFIRMED: the file's entire test surface is dead. Lines 37-49, 51-63, 65-77, 79-92 are the four candidate tests (UtcDaliLayoutControllerGetP, OnWindowResizeP, ProcessLayoutsP, GetSameWindowReturnsSameInstanceP) and every one is wrapped in `//` comment blocks. The only active code is the startup/cleanup boilerplate (lines 27-35) which asserts nothing. So utc-Dali-LayoutController.cpp has zero active assertions exercising LayoutController. The driver file size (evidence: 18949 bytes for dali-ui-foundation/p…

    - `code2`(confirmed [보정:medium]): The finding (cov-01, category "missing-coverage") is a verified test-coverage gap, not a runtime layout bug, and the factual claims hold.  Verified against /home/jae/dali/dali-ui-claude: - utc-Dali-LayoutController.cpp:27-35 contains only the startup/cleanup stubs (test_return_value = TET_UNDEF/TET_PASS). Lines 37-92 contain all four candidate tests (UtcDaliLayoutControllerGetP, OnWindowResizeP, ProcessLayoutsP, GetSameWindowReturnsSameInstanceP) entirely inside `//` comment blocks. There are zero active DALI_TEST_* / END_TEST assertions in the file — confirmed by full read. - No active `int Utc...LayoutController` test function exists anywhe…

- **재현**: Resize window → expect roots re-measured against new size; nothing asserts this.

- **▶ 수정 방향**: `utc-Dali-LayoutController.cpp:37-92` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-x-test-coverage-gaps-cov-02"></a>

#### 🟠 HIGH · `cov-02` — View min/max width/height clamping is untested for all layouts

- **위치**: `view-impl.cpp:905-906 (clamp), :1002/:1014/:1030/:1043 (MATCH_PARENT→GetMinimum*); stack-layout-manager.cpp:252-254; grid-layout-manager.cpp:473,489. Test grep for SetMinimum/SetMaximum in layout UTCs: none.`

- **현상(버그)**: ViewImpl::Measure clamps the incoming constraint to GetMinimumWidth/Height and GetMaximumWidth/Height, and Stack/Grid use GetMinimum*() to size WRAP main axes with weight/star; MATCH_PARENT reports GetMinimum*() as desired size. Yet no layout UTC ever calls SetMinimum*/SetMaximum* (grep finds only InputEditor font-scale). The entire min/max pathway — including the MATCH_PARENT-reports-minimum contract the design doc centers on — is unasserted.

- **근거 — 일반 프레임워크 기대동작**: Min/max are first-class constraints; a suite should assert clamping (constraint below min grows result, above max caps it), MATCH_PARENT desired==minimum, and weight/star targetMain==minContent when WRAP.

- **근거 — 프레임워크 레퍼런스**: Android View MeasureSpec min/max; CSS min-width/max-width clamping

- **근거 — 검증자 코드분석**:

    - `code2`(confirmed [보정:low]): All CODE claims in cov-02 are accurate. Verified the clamp at view-impl.cpp:905-906 (effNatW = min(max(natW, GetMinimumWidth), GetMaximumWidth)); MATCH_PARENT returning GetMinimum* at view-impl.cpp:1002/1014 (with-children branch) and :1030/1043 (leaf branch); stack-layout-manager.cpp:252-254 using GetMinimum*() for WRAP main-axis (minMainContent, targetMain=max(wrappedMain,minMainContent)); grid-layout-manager.cpp:473/489 using GetMinimumWidth/Height for STAR available space. The design doc (docs/layout-structure.md:132-133, 181) does center the MATCH_PARENT-reports-minimum contract on SetMinimumWidth/Height, matching the finding's framing. …

    - `code`(confirmed): Re-read all cited code and traced both arrange paths for a standalone MATCH_PARENT child.  Parent pass — ArrangeStandaloneChild (view-impl.cpp:268-296): for MATCH_PARENT width it sets childW = max(0, parentFullWidth - marginW) at :281 with NO clamp to the child's Minimum/MaximumWidth. Line 289 calls childImpl.Measure(childW, childH) — but Measure (view-impl.cpp:881-943) clamps only the *constraint* (effNatW at :905) and the result is never written back into the local childW. Line 294 builds bounds with the unclamped childW and calls childImpl.Arrange(bounds). Arrange (view-impl.cpp:1053-1088) and OnArrange (:1090-1101) write SIZE_WIDTH direct…

- **반론(소수의견)**: `intent`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: View MATCH_PARENT with SetMinimumWidth(50) measured against 0 → desired should be 50; Stack WRAP with weight child and SetMinimumHeight(120) → targetMain 120. Untested.

- **▶ 수정 방향**: `view-impl.cpp:905-906` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-x-test-coverage-gaps-flex-02"></a>

#### 🟡 MEDIUM · `flex-02` — flex-basis of 0 is ignored (treated as WRAP_CONTENT) — deviates from CSS

- **위치**: `flex-layout-params-impl.h:51 (default WRAP_CONTENT), :110-113 SetFlexBasis stores raw value; flex-layout-manager.cpp:95 `if(basis > 0.0f)` and :491 same guard. No test for basis 0 (grep SetFlexBasis(0 returns nothing).`

- **현상(버그)**: GetFlexBasis defaults to WRAP_CONTENT(-1) and the manager only overrides the working main size when basis>0.0f, so an explicit SetFlexBasis(0.0f) is silently treated as content sizing rather than a 0 main-size that grows purely from flex-grow.

- **근거 — 일반 프레임워크 기대동작**: In CSS, `flex-basis:0` is the canonical value for `flex:1` (equal distribution independent of content). 0 must be honored as a real basis distinct from 'auto'/content.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox 7.2.3 flex-basis (0 vs auto)

- **근거 — 검증자 코드분석**:

    - `intent`(confirmed [보정:medium]): Re-verified all code facts under /home/jae/dali/dali-ui-claude. flex-layout-params-impl.h:51 defaults mFlexBasis to WRAP_CONTENT(-1.0f); SetFlexBasis at flex-layout-params-impl.h:110-113 stores the raw value with no clamping, so 0.0f is stored verbatim. In flex-layout-manager.cpp both the Measure path (:490-501) and the Arrange path (:94-105) guard the basis override with `if(basis > 0.0f)`. Since 0.0f > 0.0f is false, an explicit SetFlexBasis(0.0f) never overrides workingSize.width/height, so the child retains its Measure() content/natural size — identical to the WRAP_CONTENT default. GetFlexBasis (:56-59) falls back to WRAP_CONTENT. So the …

    - `code`(confirmed [보정:medium]): Index 67 = finding id "flex-02" (finderKey x-test-coverage-gaps): "flex-basis of 0 is ignored (treated as WRAP_CONTENT)". I traced the code-refute lens against the actual source under /home/jae/dali/dali-ui-claude.  Evidence accuracy: - flex-layout-params-impl.h:51 — default member init `mFlexBasis(WRAP_CONTENT)` confirmed (constructor at lines 47-54, comment line 45 also states "basis=WRAP_CONTENT"). - flex-layout-params-impl.h:110-113 — `SetFlexBasis(float basis){ mFlexBasis = basis; }` stores the raw value with NO clamp, confirmed. - flex-layout-manager.cpp:59 — `GetFlexBasis` returns `params ? params->GetFlexBasis() : WRAP_CONTENT`. - fle…

- **재현**: Two children FlexBasis(0) grow=1 in 300px row: CSS gives 150/150 ignoring content; here each keeps its content/measured width and only shares leftover space.

- **▶ 수정 방향**: `flex-layout-params-impl.h:51` 에서 위 '기대동작'을 만족하도록 수정. (분류: convention-deviation)


<a id="fix-x-test-coverage-gaps-flex-03"></a>

#### 🟡 MEDIUM · `flex-03` — No flex min/max-content clamping during grow/shrink

- **위치**: `flex-layout-manager.cpp:160-189 (shrink loop, only floor is 0.0f, single pass, no freeze/redistribute); grow loop :139-159 similarly single-pass.`

- **현상(버그)**: ApplyFlexGrowShrink distributes free/overflow space purely proportionally with no per-item min-content floor or max-content ceiling, and shrink can drive a child to 0 (std::max(0,...)). There is no concept of a hypothetical main size clamped to min/max content as in CSS, nor re-distribution to remaining flexible items when one hits its limit.

- **근거 — 일반 프레임워크 기대동작**: CSS flex 'Resolving Flexible Lengths' freezes items that hit their min/max-content constraint and re-distributes the remainder among the rest; min-content normally prevents shrinking text/box below its minimum.

- **근거 — 프레임워크 레퍼런스**: CSS Flexbox 9.7 step 4 (freeze inflexible/violating items)

- **근거 — 검증자 코드분석**:

    - `intent`(confirmed [보정:medium]): Re-verified the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp. ApplyFlexGrowShrink (lines 133-191): the grow branch (138-159) distributes free space in a single proportional pass with NO max-content ceiling and NO freeze/redistribute loop; the shrink branch (160-189) is a single pass whose only floor is std::max(0.0f, size - reduction) at lines 181/185 — no per-item min-content floor, no freezing of violating items, no re-distribution to remaining flexible items. A grep for min/max/freeze/clamp over the file (only hits 0.0f floors and unrelated std::max) confirms no min/max-content l…

    - `code`(confirmed [보정:medium]): Index 68 = finding flex-03 ("No flex min/max-content clamping during grow/shrink"). I re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:133-191 (ApplyFlexGrowShrink). The observed claim holds exactly:  - Grow path (lines 139-159): a single pass distributing `extra = (grow/line.totalFlexGrow)*freeSpace` directly into workingSizes[idx].width/height. No max-content ceiling, no freeze, no redistribution to remaining items. - Shrink path (lines 160-189): a single pass with weighted reduction `(shrink*childMainSize/totalWeightedShrink)*overflow`, and the ONLY floor is `std::max(0.0…

- **재현**: Three flex items shrink=1 with one having large min-content: DALi shrinks all proportionally to 0-floor; CSS freezes the min-content item and over-shrinks the others. Untested and unimplemented.

- **▶ 수정 방향**: `flex-layout-manager.cpp:160-189` 에서 위 '기대동작'을 만족하도록 수정. (분류: design-smell)


<a id="fix-x-test-coverage-gaps-grid-01"></a>

#### 🟡 MEDIUM · `grid-01` — Spanned children never contribute to AUTO track sizing

- **위치**: `grid-layout-manager.cpp:120-127; the only span test (UtcDaliGridLayoutRowColumnSpanP utc-Dali-GridLayout.cpp:502-520) uses ABSOLUTE tracks and asserts only the params getters, never the spanned child's measured size.`

- **현상(버그)**: MeasureGridChildrenAndFillAuto only feeds a child's content size into an AUTO/implicit track when rowSpan==1 / colSpan==1 (the `if(rowSpan == 1 && rowAutoLike)` / `if(colSpan == 1 && colAutoLike)` guards). A child spanning 2 AUTO columns contributes nothing to either column's auto size, so AUTO tracks can be sized smaller than the spanned content requires and the child overflows / is clipped to the cell.

- **근거 — 일반 프레임워크 기대동작**: WPF Grid distributes a spanned element's desired size across the spanned auto (and star-after-auto) columns so the union of tracks is at least the element's desired size.

- **근거 — 프레임워크 레퍼런스**: WPF Grid star/auto sizing with ColumnSpan (MeasureOverride spanned distribution)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): Finding num-07 (index 69) accurately describes the code. Verified every citation in the worktree at /home/jae/dali/dali-ui-claude:  1. Default bounds (0,0,-1,-1): absolute-layout-params-impl.h:48 `mBounds(0.0f, 0.0f, -1.0f, -1.0f)`, and the manager's null-params fallback returns the same at absolute-layout-manager.cpp:43 `LayoutRect(0.0f, 0.0f, -1.0f, -1.0f)`.  2. Manager treats any w<0/h<0 as "use measured/natural size" (auto): Measure at absolute-layout-manager.cpp:145-152 (`if(w < 0){ w = childSize.width; } if(h < 0){ h = childSize.height; }`); Arrange at :237-267 (the `if(w < 0 || h < 0)` block recomputes from measured/available). The neg…

    - `intent`(confirmed [보정:medium]): Re-read grid-layout-manager.cpp:81-129. MeasureGridChildrenAndFillAuto measures each child with full available constraint (line 112) but only feeds its content size into an AUTO/implicit track when span==1: `if(rowSpan == 1 && rowAutoLike)` (line 120) and `if(colSpan == 1 && colAutoLike)` (line 124). A child spanning 2+ AUTO/implicit tracks contributes nothing to any of them, so those tracks stay at their floor (0 if no single-span child sits there). ApplyGridDefinitions (131-232) never compensates for spans (it has no span awareness at all), and Arrange's ArrangeGridChildrenToCells (252-350) sizes the cell from the resolved track widths/posi…

- **재현**: Two AUTO columns, single child column=0 columnSpan=2 with content width 200: both columns stay at 0 width (no other children), child cell = 0, content overflows. Untested.

- **▶ 수정 방향**: `grid-layout-manager.cpp:120-127` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


<a id="fix-x-test-coverage-gaps-cov-03"></a>

#### 🟡 MEDIUM · `cov-03` — Params-sharing footgun (same handle on multiple views) is documented but never tested

- **위치**: `docs/layout-structure.md:52-58 (note about shared state / New(other)); grep across utc-Dali-*Layout.cpp shows no reuse of a single Params variable across two SetLayoutParams calls.`

- **현상(버그)**: The design doc warns SetLayoutParams stores the handle as-is, so passing one params handle to multiple Views makes them share mutable state; New(other) is the prescribed copy. No test sets the same StackLayoutParams/GridLayoutParams/FlexLayoutParams/AbsoluteLayoutParams instance on two views to pin either the sharing behavior or the New(other) independence.

- **근거 — 일반 프레임워크 기대동작**: A test should assert that two views given the same handle observe a later mutation on both (sharing), and that New(other) yields independent copies — encoding the intended contract so a future deep-copy change is a deliberate break.

- **근거 — 프레임워크 레퍼런스**: WPF attached-property per-element semantics (no shared mutable layout params)

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): cov-03 is a missing-coverage finding; every factual sub-claim in "observed" holds.  1. Doc warns about sharing + prescribes New(other): confirmed at docs/layout-structure.md:52-58 (New(other) creates an independent copy) and the explicit Note at :58: "SetLayoutParams() stores the handle as-is (no deep copy). Passing the same handle to multiple Views causes them to share state. Use New(other) to create an independent copy when reusing params across Views."  2. SetLayoutParams stores the handle as-is (no deep copy): confirmed at view-impl.cpp:2170-2175 — it takes paramsImpl.GetTraitId() and calls mImpl->SetTrait(..., ToTraitObject(params)), sto…

    - `intent`(confirmed [보정:medium]): This is a missing-coverage finding and the gap is real. Verified the documented contract exists: docs/layout-structure.md:52-58 explicitly warns that SetLayoutParams "stores the handle as-is (no deep copy). Passing the same handle to multiple Views causes them to share state. Use New(other) to create an independent copy." The sharing behavior is real in code: ViewImpl::SetLayoutParams (view-impl.cpp:2170-2175) calls ToTraitObject(params), and ToTraitObject (view-impl.cpp:101-111) returns an IntrusivePtr to the SAME underlying TraitObject body (no clone), so two views given one handle truly share the mutable trait. GridLayoutParams::New(const&…

- **재현**: auto p = GridLayoutParams::New().SetColumn(1); a.SetLayoutParams(p); b.SetLayoutParams(p); b.GetLayoutParams<GridLayoutParams>().SetColumn(2); → does a see column 2? Unpinned.

- **▶ 수정 방향**: `Layout.cpp` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-x-test-coverage-gaps-cov-04"></a>

#### 🟡 MEDIUM · `cov-04` — Weight/star/grow rounding-sum (fill-to-edge) never asserted

- **위치**: `stack-layout-manager.cpp:133; grid-layout-manager.cpp:204,219; flex-layout-manager.cpp:147. The only star test (utc-Dali-GridLayout.cpp:462-479) asserts only the container 200x100, never the two star tracks' widths.`

- **현상(버그)**: Stack weight (share = weight/total*remaining), Grid star (starValue/totalStar*remaining), and Flex grow (grow/total*free) all distribute by float division with no last-item remainder correction. No test verifies that N weighted/star/grow children exactly tile the available main axis without a sub-pixel gap or overshoot.

- **근거 — 일반 프레임워크 기대동작**: Mature layout suites assert that e.g. three star columns over 100px sum to exactly 100 (last track absorbs rounding), guarding against accumulated float drift.

- **근거 — 프레임워크 레퍼런스**: WPF Grid star resolution remainder; Android LinearLayout weight rounding

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Finding index 73 = id "cov-04" (finderKey "x-test-coverage-gaps"), a missing-coverage finding asserting that (a) Stack weight, Grid star, and Flex grow all distribute free space by plain float division with no last-item remainder correction, and (b) no test asserts that N weighted/star/grow children exactly tile the available main axis.  Code citations verified against /home/jae/dali/dali-ui-claude: - Stack: stack-layout-manager.cpp:133 `share = (weight/totalWeight)*remainingMain` — plain division, no remainder absorption. - Grid: grid-layout-manager.cpp:204 `colWidths[i] = (starValue/totalStarWidth)*remainingWidth` and :219 the row equivalen…

    - `intent`(confirmed [보정:low]): Re-verified all citations under /home/jae/dali/dali-ui-claude.  Code claims (distribution by plain float division, no last-item remainder correction) are all accurate: - Stack weight: stack-layout-manager.cpp:133 (Measure) `share = (weight/totalWeight)*remainingMain` and :410 (Arrange) identical formula; sizes written per-child (:147,:434) with no remainder absorbed by a last child. - Grid star: grid-layout-manager.cpp:204 `(starValue/totalStarWidth)*remainingWidth` and :219 for rows; tracks then summed via std::accumulate (:230-231) with no remainder fix-up. - Flex grow: flex-layout-manager.cpp:147 `extra = (grow/line.totalFlexGrow)*freeSpac…

- **재현**: 3 columns Star(1) in 100px → assert col widths are 34/33/33 or 33.33 each summing to 100. Untested.

- **▶ 수정 방향**: `stack-layout-manager.cpp:133` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-x-test-coverage-gaps-stack-01"></a>

#### 🟡 MEDIUM · `stack-01` — Weak/tautological assertions pin questionable Stack & Grid results

- **위치**: `utc-Dali-StackLayout.cpp:345-346; utc-Dali-GridLayout.cpp:497-498, :477-478.`

- **현상(버그)**: Several headline 'Measure/Arrange' tests assert only inequalities or the trivially-true container size, so they pass regardless of the child layout. UtcDaliStackLayoutChildWithWeightP asserts weightChild height `>= 50.0f` (any value passes). UtcDaliGridLayoutAutoDefinitionsP asserts `m.GetWidth() >= 60` / `>= 40`. UtcDaliGridLayoutStarDefinitionsP asserts only container 200x100.

- **근거 — 일반 프레임워크 기대동작**: These should assert the exact computed child size (weightChild fills the remaining main axis = 120-40-5spacing-... ; auto track == child 60x40; each star cell == half).

- **근거 — 프레임워크 레퍼런스**: general: assertions must constrain the value under test, not a superset

- **근거 — 검증자 코드분석**:

    - `intent`(intentional-design [보정:info]): Finding index 74 = child-01 ("OnChildOrderChanged wholesale-rebuilds mChildren from Actor order, silently discarding PRESERVE/Insert layout-order divergence"). The code is real: OnChildOrderChanged at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:2826-2851 rebuilds mImpl->mChildren by walking self.GetChildAt(i) in actor (visual) order, guarded by mSkipChildrenUpdate (2828). Insert diverges mChildren (1742-1744) and PRESERVE diverges by skipping the rebuild via ScopedSkipChildrenUpdate (2040-2051). So the observed mechanics are accurate.  But this is documented, intentional design, not a bug. The LayoutOrderPolicy e…

    - `code`(confirmed [보정:medium]): Index 74 = id "stack-01" (finderKey x-test-coverage-gaps): "Weak/tautological assertions pin questionable Stack & Grid results." I re-read every cited line in the target worktree and all three citations hold exactly:  1. /home/jae/dali/dali-ui-claude/automated-tests/src/dali-ui-foundation/utc-Dali-StackLayout.cpp:346 — `DALI_TEST_CHECK(weightChild.GetSize().height >= 50.0f);`. In UtcDaliStackLayoutChildWithWeightP the vertical Stack is 120 high with a 40px fixed child, 5px spacing, and a weight=1 child whose own RequestedHeight is 50 (lines 329-340). The weight child should fill the remainder 120-40-5 = 75. The test only asserts a non-tight l…

- **재현**: Make UtcDaliStackLayoutChildWithWeightP assert weightChild.GetSize().height == 75 (120-40-5). Currently any positive value passes.

- **▶ 수정 방향**: `utc-Dali-StackLayout.cpp:345-346` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-x-test-coverage-gaps-cov-06"></a>

#### 🟡 MEDIUM · `cov-06` — Mutation-during-layout and nested-manager layouts untested

- **위치**: `grep for 'nested'/AttachLayoutManager in layout UTCs returns nothing; all *Layout UTCs use a single manager with leaf View children; child snapshotting at stack-layout-manager.cpp:218, flex :460, grid :439.`

- **현상(버그)**: No test exercises (a) adding/removing/reordering children or mutating params from within a Measure/Arrange callback (re-entrancy/iterator-invalidation safety), nor (b) a LayoutManager-driven layout nested inside another LayoutManager-driven layout (e.g. FlexLayout containing a GridLayout). Managers snapshot children into a std::vector each pass (e.g. stack-layout-manager.cpp:218-223) which suggests re-entrancy was considered, but it is unverified; nested cases are where MATCH_PARENT re-measure and double-measure interact.

- **근거 — 일반 프레임워크 기대동작**: Tests for nested managers asserting inner child rects, and a test mutating the child set during a custom Measure/Arrange callback to confirm no crash / defined behavior.

- **근거 — 프레임워크 레퍼런스**: WPF/Android layout re-entrancy guards; nested panel measurement

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Index 77 = id "cov-06" (finderKey x-test-coverage-gaps), "Mutation-during-layout and nested-manager layouts untested" (severity medium, category missing-coverage). I re-verified every load-bearing claim against /home/jae/dali/dali-ui-claude:  1. Snapshot-into-std::vector citations all hold: stack-layout-manager.cpp:217-223 (`std::vector<View> children; ... children.push_back(GetChildAt(view,i))` with comment "Collect children once"), flex-layout-manager.cpp:459-465, grid-layout-manager.cpp:437-444. So the "managers snapshot children each pass" premise is accurate.  2. No mutation-during-layout test: SetMeasureCallback/SetArrangeCallback appea…

    - `intent`(intentional-design [보정:low]): The observed mechanics are accurate. view-impl.cpp:918-926 dispatches Measure via GetMeasureCallback() -> GetLayoutManager() -> OnMeasure(), and view-impl.cpp:1056-1062 dispatches Arrange via GetArrangeCallback() -> GetLayoutManager() -> OnArrange(), independently. So a measure-callback can coexist with arrange-via-manager.  However this independence is a documented, deliberate design choice, not an unintended deviation: - view.h:226 and view.h:254 expose SetMeasureCallback and SetArrangeCallback as two separate APIs, each documented (208-211, 230-233) as independently replacing "the default measurement behavior" / "arrangement behavior", eac…

- **재현**: FlexLayout(row) with two GridLayout children each MATCH_PARENT → assert inner grid cells; plus a MeasureCallback that calls layout.Add(...) mid-measure.

- **▶ 수정 방향**: `stack-layout-manager.cpp:218` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-x-test-coverage-gaps-cov-05"></a>

#### 🟢 LOW · `cov-05` — Axis asymmetry: cross-axis MATCH_PARENT/justify/align tested mostly on one axis only

- **위치**: `utc-Dali-FlexLayout.cpp:401-463 (no position/size asserts); RTL tests utc-Dali-FlexLayout.cpp:636 (row only), utc-Dali-StackLayout.cpp:687 (horizontal only).`

- **현상(버그)**: Stack tests cover cross-axis alignment for both orientations (good), but Flex justify/align variant tests (UtcDaliFlexLayoutJustifyContentVariantsP, AlignItemsVariantsP) iterate enum values without asserting any child position/size, so neither the main-axis justify offsets nor the cross-axis align offsets are pinned on either axis. RTL is asserted only for the horizontal/row main axis (Stack horizontal, Flex row, Grid columns, Absolute) — no vertical-axis or wrap-line RTL/mirroring assertion.

- **근거 — 일반 프레임워크 기대동작**: Assert exact child x/y for each justify (FLEX_END, CENTER, SPACE_BETWEEN/AROUND/EVENLY) and each align (FLEX_END/CENTER/STRETCH); test RTL for COLUMN and for WRAP lines.

- **근거 — 프레임워크 레퍼런스**: CSS justify-content/align-items offset computation

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:low]): cov-05 (index 75) makes test-coverage claims that I verified against the source at /home/jae/dali/dali-ui-claude.  Claim 1 — Flex justify/align variant tests assert no child position/size: UtcDaliFlexLayoutJustifyContentVariantsP (utc-Dali-FlexLayout.cpp:401-432) and UtcDaliFlexLayoutAlignItemsVariantsP (435-463) each iterate the enum values calling SetJustifyContent/SetAlignItems + Measure + Arrange, but their only assertions are on the container's measured width/height (m1.GetWidth==200, GetHeight==80; ma.GetWidth==200, GetHeight==100) and GetChildCount==2. No child GetPositionX/Y or GetSize is asserted, so neither main-axis justify offsets…

    - `intent`(confirmed [보정:low]): Element 75 is finding num-08 ("NaN constraint defeats the Measure cache"). I re-verified every cited line in /home/jae/dali/dali-ui-claude.  Mechanism (correct): FloatEqual is `std::abs(a-b) < epsilon` (view-impl.cpp:188-191). The cache-hit gate (view-impl.cpp:908-909) requires `mLastMeasuredConstraint.width >= 0.0f && FloatEqual(width,effNatW) && FloatEqual(height,effNatH)`. effNatW is computed via std::min/std::max with no NaN sanitisation (view-impl.cpp:905-906), and the stored key is effNatW verbatim (view-impl.cpp:934-935). Under a NaN constraint the gate fails twice over: `NaN >= 0.0f` is false AND FloatEqual(NaN,NaN) is false (NaN < ep…

- **재현**: FlexJustify::SPACE_BETWEEN, 2 children 40px in 200px → assert child positions 0 and 160. Untested.

- **▶ 수정 방향**: `utc-Dali-FlexLayout.cpp:401-463` 에서 위 '기대동작'을 만족하도록 수정. (분류: missing-coverage)


<a id="fix-x-test-coverage-gaps-abs-01"></a>

#### 🟢 LOW · `abs-01` — AbsoluteLayout ignores parent padding (inconsistent with other managers)

- **위치**: `absolute-layout-manager.cpp:78-79 (contentWidth=widthConstraint, no padding), :290-291 (childBounds.x = bounds.x + x + margin, no padding); contrast view-impl.cpp:1144 (visPadLeft added) and grid-layout-manager.cpp cell positions start at bounds.x with no padding either — but Grid+Stack subtract padding upstream. No AbsoluteLayout test sets padding with a non-standalone child to pin this.`

- **현상(버그)**: AbsoluteLayoutManager Measure uses widthConstraint/heightConstraint directly as the content area and Arrange positions children at bounds.x + x with no subtraction of the parent's padding, so a padded AbsoluteLayout places child (0,0) at the parent's outer edge, unlike Stack/Grid/Flex which inset by padding, and unlike WPF Canvas-in-a-padded-Border.

- **근거 — 일반 프레임워크 기대동작**: Either honor parent padding as the other managers and the plain View OnArrange do (visPadLeft/Top offsets), or document this as intentional. Current behavior is silently inconsistent across managers.

- **근거 — 프레임워크 레퍼런스**: WPF Canvas honoring container Padding; CSS absolute positioning relative to padding box

- **근거 — 검증자 코드분석**:

    - `code`(confirmed [보정:medium]): Re-read all cited code under /home/jae/dali/dali-ui-claude. The finding (index 76, "standalone-01") accurately describes the code.  PARENT-PASS PATH — ArrangeStandaloneChild (view-impl.cpp:268-296): For a MATCH_PARENT axis, childW = std::max(0, parentFullWidth - marginW) (line 281), with NO clamp to the child's Minimum/Maximum. Line 289 does call childImpl.Measure(childW, childH), and Measure internally clamps effNatW/effNatH to min/max (view-impl.cpp:905-906) — but the clamped MeasuredSize is discarded: the bounds at lines 292-294 are built from the unclamped childW/childH, and childImpl.Arrange(bounds) (view-impl.cpp:1053-1088 → OnArrange:1…

- **반론(소수의견)**: `intent`=refuted — 정적 확인 한계, 런타임 재확인 권장.

- **재현**: AbsoluteLayout padding(20) child bounds (0,0,50,50): child appears at x=0 (outside padding). No assertion exists (only the Standalone+padding test, which is explicitly padding-ignoring by design).

- **▶ 수정 방향**: `absolute-layout-manager.cpp:78-79` 에서 위 '기대동작'을 만족하도록 수정. (분류: unusual-design)


---
## 의도적 설계지만 비관례적 — 재검토 권장 (수정 선택)

> 검증 결과 **의도된 설계**로 판명. 버그는 아니나 Android/WPF/CSS 주류와 달라 이식성·학습비용·문서화 측면에서 검토 권장.


#### 🟡 MEDIUM · `measure-engine/measure-03` — No MEASURED_STATE_TOO_SMALL / overflow propagation (convention gap)

- **위치**: `layout-types.h:100-171 MeasuredSize has only `float width; float height;`; grep MEASURED_STATE/TOO_SMALL -> 0 hits`

- **현재 동작**: MeasuredSize carries only width/height floats (layout-types.h:100-171). There is no state/too-small bit and no combine step. grep for MEASURED_STATE/TOO_SMALL across dali-ui-foundation returns nothing.

- **주류 프레임워크와의 차이**: Android stores measured state alongside size (resolveSizeAndState/combineMeasuredStates/MEASURED_STATE_TOO_SMALL) so ancestors and scroll containers can react to children that didn't fit. (Android View.MEASURED_STATE_TOO_SMALL / getMeasuredState() / combineMeasuredStates())

- **의도 판단 근거**: The finding's literal "observed" code facts are accurate and I confirm them: in /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/layout-types.h the MeasuredSize class (lines 100-171) carries only `float width;` and `float height;` (lines 169-170) plus accessors and ToVector2 — there is no state/too-small bit and no combine step. A grep for MEASURED_STATE/TOO_SMALL/MeasuredState/combineMeasuredStates across dali-ui-foundation/ returns zero hits. So this is NOT factually refuted.  However, framing it as a "convention gap" defect (medium) is wrong; it is intentional design, because MEASURED_STATE_TOO_SMALL is a concept that ex…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟡 MEDIUM · `arrange-engine/arr-03` — Default OnArrange applies no cross-axis alignment; alignment lives only inside managers

- **위치**: `view-impl.cpp:1136-1154 (no alignment branch) vs stack-layout-manager.cpp:475-495.`

- **현재 동작**: The default container OnArrange pins every non-standalone child at origin = padding + leading margin + requested position (view-impl.cpp:1144-1145) and never reads LayoutAlignment; FILL/CENTER/END only exist inside StackLayoutManager (stack-layout-manager.cpp:478-495,528-545), GridLayoutManager (306,328) and FlexLayoutManager. A plain View used as a container (no LayoutManager attached) therefore ignores child LayoutAlignment entirely, while the same children inside a Stack honor it.

- **주류 프레임워크와의 차이**: In Android a ViewGroup's layout_gravity and in WPF a panel's HorizontalAlignment/VerticalAlignment are honored by the base arrange path. Here behavior depends on whether a manager happens to be attached, which is surprising and inconsistent across the two Arrange paths. (Android FrameLayout/ViewGroup layout_gravity; WPF panel child HorizontalAlignment/VerticalAlignment)

- **의도 판단 근거**: I re-read the cited code (line numbers in the finding were stale but the constructs exist). The default container OnArrange does anchor non-standalone children at padding + leading margin + requested position with no alignment branch: view-impl.cpp:1144-1145 compute childX/childY purely from visPadLeft/visMarginStart + GetRequestedPositionX()*s; a grep shows ZERO occurrences of "Alignment" in the entire view-impl.cpp. StackLayoutManager does honor cross-axis CENTER/END/FILL (stack-layout-manager.cpp:475-495 for vertical-stack cross-X, 525-545 for horizontal-stack cross-Y). Dispatch confirms the manager-less path runs OnArrange (view-impl.cpp:…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟢 LOW · `standalone-children/standalone-02` — Standalone child position ignores parent padding, contradicting CSS position:absolute convention

- **위치**: `view-impl.cpp:292-293 and 1174-1175 (no padding) vs view-impl.cpp:1138,1144 (in-flow child adds visPadLeft); doc claim docs/layout-structure.md:183-190; asserted by UtcDaliViewStandaloneIgnoresParentPaddingMatchParentP (utc-Dali-View.cpp:1504-1509).`

- **현재 동작**: ArrangeStandaloneChild positions a standalone child at RequestedPositionX*scale + margin.start*scale with no parent-padding term (view-impl.cpp:292-293), and MeasureStandaloneChildren sizes against the parent full size with padding omitted (view-impl.cpp:1174-1175). A normal in-flow child instead offsets by parent padding: childX = visPadLeft + visMarginStart + pos*s (view-impl.cpp:1144).

- **주류 프레임워크와의 차이**: In CSS, an absolutely positioned child is laid out relative to the padding box of its containing block, so parent padding shifts the child's origin. DALi standalone deliberately drops padding, so PositionX=0 sits at the parent border edge, not inside the padding. (CSS 2.1 10.1 'containing block' for absolutely positioned boxes = padding box of nearest positioned ancestor; WPF Canvas ignores padding (matches DALi).)

- **의도 판단 근거**: The finding's CODE observations are all accurate (I confirmed each against the real source — note the file lives at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp, not the path in the finding's citation, and line numbers are off by ~2, but the code matches):  1. ArrangeStandaloneChild (view-impl.cpp:292-293) positions a standalone child at `GetRequestedPositionX()*childScale + margin.start*childScale` with NO parent-padding term. Confirmed. 2. MeasureStandaloneChildren (view-impl.cpp:1174-1175) sizes the child as `visEffW - visMarginW` / `visEffH - visMarginH` — parent padding omitted. Confirmed. 3. An in-flow child…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟡 MEDIUM · `invalidation-controller/inval-02` — Measure cache HIT skips MeasureStandaloneChildren, decoupling standalone re-measure from parent pass

- **위치**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:908-911 (early return) vs :940 (MeasureStandaloneChildren after the return)`

- **현재 동작**: ViewImpl::Measure returns the cached mMeasuredSize at line 911 (when the incoming constraint equals the cached one) BEFORE reaching MeasureStandaloneChildren at line 940. So when a parent's own measure cache hits, its standalone children are not re-measured by the parent pass at all.

- **주류 프레임워크와의 차이**: In Android/WPF a parent's measure pass, when it runs, measures all participating children; here standalone children are silently dependent on self-registering as their own roots to ever be measured. A standalone child dirtied while the parent's constraint is unchanged is only saved because InvalidateMeasure self-registers it (1335) — the parent pass contributes nothing. (Android ViewGroup.measureChildren / WPF MeasureOverride: a measure pass that executes measures its children; caching short-circuits only when nothing is dirty.)

- **의도 판단 근거**: I re-read every cited line. The finding's raw mechanics are correct but its framing as an "unusual-design" defect is refuted by the code, docs, and tests.  VERIFIED MECHANICS (finding is right here): - view-impl.cpp:908-912 returns mImpl->mMeasuredSize on a cache HIT (incoming effNatW/effNatH equal to mLastMeasuredConstraint) BEFORE reaching MeasureStandaloneChildren at :940. So a parent cache HIT does skip MeasureStandaloneChildren. Correct reading. - The standalone-child self-register path is at InvalidateMeasure :1328-1336: a standalone view sets DIRTY, then RegisterWithLayoutController() and returns WITHOUT propagating to the parent.  WHY…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟡 MEDIUM · `invalidation-controller/inval-03` — InvalidateMeasure dirty-guard invariant depends on Window-gated root registration; off-scene dirtying registers no root

- **위치**: `/home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:1305-1308 (guard), :1403 (window gate), :2716-2739 (OnChildAdd workaround comment + self-invalidate)`

- **현재 동작**: The early-exit guard (view-impl.cpp:1305-1308) assumes 'if this view is DIRTY, the root was already registered.' But RegisterWithLayoutController only registers when DevelWindow::Get(self) returns a window (1403). A view invalidated while off-scene sets its chain to DIRTY but registers NO root. A later InvalidateMeasure on a descendant then short-circuits at this still-DIRTY ancestor (guard fires) and likewise registers nothing.

- **주류 프레임워크와의 차이**: Android/WPF keep the per-view dirty flag across detach and honor it on the next traversal after re-attach; the dirty mark is not conditioned on window attachment. Here the invariant is rescued only indirectly by OnChildAdd self-invalidating the container (2739) and OnSceneConnection re-registering tree-roots/standalone (373-375).

- **의도 판단 근거**: I re-read every cited line under /home/jae/dali/dali-ui-claude. The finding's MECHANISM reading is factually accurate, but its asserted DEFECT is hypothetical and not shown to be reachable, so the actual invariant is sound-by-construction rather than "rescued indirectly."  Verified facts: - Guard: view-impl.cpp:1305-1308 returns early when width==MEASURE_CACHE_DIRTY. Confirmed. - Window gate: RegisterWithLayoutController (1396-1412) only calls controller.RequestLayout when DevelWindow::Get(self) is truthy (1403). So an off-scene InvalidateMeasure marks the chain DIRTY but registers no root. Confirmed. - OnChildAdd: for a non-standalone child,…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟢 LOW · `stack-layout/stack-03` — Weighted shares lose the rounding remainder; total can under/overflow the main axis

- **위치**: `stack-layout-manager.cpp:133 and 410 compute each share in isolation; no accumulation of (sum of shares) vs remainingMain.`

- **현재 동작**: Each weight child gets share=(weight/totalWeight)*remainingMain computed independently in float, both in Measure (stack-layout-manager.cpp:133) and Arrange (410). There is no remainder accumulation or last-child fixup, so with non-divisible weights/sizes the summed shares do not exactly equal remainingMain and a sub-pixel gap or overhang is left at the end of the stack.

- **주류 프레임워크와의 차이**: Frameworks that distribute integer/pixel space track the running remainder and assign leftover pixels (commonly to the last flexible item) so the children exactly tile the container (Android error-distribution; CSS layout rounding). (Android LinearLayout weight remainder handling; CSS flexible-length rounding)

- **의도 판단 근거**: CODE OBSERVATION CONFIRMED, IMPACT/EXPECTATION REFUTED -> net verdict: intentional-design (acceptable float-based choice, not a real defect).  The literal code claim is accurate. In stack-layout-manager.cpp the weighted share is computed independently per child as `share = (weight / totalWeight) * remainingMain`: at line 133 (Measure helper MeasureStackWeightChildren) and line 410 (Arrange re-distribution block). There is indeed no running-remainder accumulation and no last-child fixup; nothing tracks sum(shares) vs remainingMain. Verified by reading the full file (the cited symbols live at exactly those lines; the manager is under public-api…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### ⚪ INFO · `stack-layout/stack-06` — No weightSum analogue: weights always split only leftover, never the full main axis

- **위치**: `stack-layout-manager.cpp:117 and 395 only ever distribute leftover; StackLayoutParams has only SetWeight/SetAlignment (stack-layout-params.cpp:72-92).`

- **현재 동작**: Distribution always uses remainingMain = contentMain − nonWeight − spacing (stack-layout-manager.cpp:117, 395). There is no API to make weights partition the WHOLE main axis (Android weightSum / total-weight-on-full-track), and no clamp/normalization when total weight is small.

- **주류 프레임워크와의 차이**: Android exposes weightSum so weights can distribute the entire dimension rather than only the excess; a general linear layout typically offers both modes. (Android LinearLayout android:weightSum)

- **의도 판단 근거**: I re-read all cited code and verified every "observed" claim holds.  CODE FACTS (all confirmed): - stack-layout-manager.cpp:117 — `float remainingMain = contentMain - mainAxisNonWeight - spacingTotal;` (in MeasureStackWeightChildren). Verified. - stack-layout-manager.cpp:395 — `float remainingMain = std::max(0.0f, availableMain - nonWeightMain - spacingTotal);` (in Arrange). Verified. - Weighted share is computed only from this leftover: line 133 `share = (weight/totalWeight)*remainingMain` and line 410 (Arrange) identical. There is NO branch/parameter anywhere that lets weights partition the full main axis ignoring nonWeight. The Measure pat…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟢 LOW · `flex-layout/flex-08` — SPACE_AROUND / SPACE_EVENLY on overflow center the line instead of CSS safe/distributed fallback

- **위치**: `flex-layout-manager.cpp:215-242 (negative-free-space branches)`

- **현재 동작**: For negative free space, GetFlexJustifyOffsets sets mainOffset = freeSpace/2 for SPACE_AROUND and SPACE_EVENLY (flex-layout-manager.cpp:227-230, 239-241) and zero spacing, i.e. it centers the overflow. SPACE_BETWEEN falls back to start (no offset).

- **주류 프레임워크와의 차이**: Per CSS, with negative free space the distributed alignments collapse: space-between -> flex-start (matches), space-around/space-evenly -> center (this matches the 'unsafe' default). The behavior is defensible, but the comment at 212-214 frames it as overflow handling and it differs from a naive reading; worth noting space-around's CSS overflow fallback is center which matches, so this is mostly a documentation/consistency note rather than a hard bug. (CSS Box Alignment 'safe'/'unsafe' overflow + Flexbox justify-content distributed fallback)

- **의도 판단 근거**: I verified the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:199-245 (the prior auditor's cited path "flex-layout-manager.cpp" resolves here; integration-api copy is a generated/mirror variant). The code is exactly as described: GetFlexJustifyOffsets sets out.mainOffset = freeSpace/2.0f for SPACE_AROUND (line 229) and SPACE_EVENLY (line 240) when freeSpace <= 0, with zero spacing; SPACE_BETWEEN falls back to start (no offset) on overflow (line 216 guard `freeSpace > 0.0f`). The comment at 212-214 frames it as deliberate overflow handling.  This is INTENTIONAL design, not an accidenta…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟡 MEDIUM · `grid-layout/grid-05` — Alignment is skipped for zero-measured and oversized children, causing silent overflow

- **위치**: `grid-layout-manager.cpp:300-342 (both `childWidth>0 && childWidth<cellWidth` / `childHeight>0 && childHeight<cellHeight` guards)`

- **현재 동작**: Per-axis alignment (CENTER/END/START) is applied only inside `if(childWidth > 0.0f && childWidth < cellWidth)`. If a child's measured size is 0 (e.g. a MATCH_PARENT or unmeasured/leaf view reporting min=0) or is larger than the cell, the alignment branch is skipped and the child is left FILL-stretched or overflowing the cell with no clamp to cell bounds.

- **주류 프레임워크와의 차이**: Frameworks clamp the child to the cell (overflow handling) and still apply the requested alignment. A child larger than its cell should be clipped/aligned to the cell, not allowed to silently exceed it; a 0-size child with START/CENTER should still be positioned, not implicitly filled.

- **의도 판단 근거**: Code verified accurate. grid-layout-manager.cpp:301 `if(childWidth > 0.0f && childWidth < cellWidth)` and :323 `if(childHeight > 0.0f && childHeight < cellHeight)` both exist and gate the CENTER/END/START alignment switches. When the branch is skipped, `childBounds.width/height` retains the full cell size set at :291-292 (i.e. FILL). git blame shows both guards (`>0.0f` AND `<cellWidth`) are original to the feature commit 3c874cfe1 (Jaehyun Cho, 2026-02-25); the later 2ae64dc5d ("Update formatting", Jiyun Yang) only touched whitespace, confirmed by diffing — so the guards are deliberate, not an accident.  ZERO-MEASURED case is documented inte…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟡 MEDIUM · `absolute-layout/abs-05` — AbsoluteLayoutParams::SetWidth doc says '-1 means use View's own size' but MATCH_PARENT(-2) bounds are silently treated as WRAP, decoupled from RequestedWidth

- **위치**: `absolute-layout-params.h:144-156; absolute-layout-manager.cpp:145-152 (Measure) and 237-266 (Arrange, only RequestedWidth gates MATCH fill)`

- **현재 동작**: SetWidth doc: 'width (-1 means use View's own size)' (absolute-layout-params.h:148). The manager treats ANY negative bounds.width as 'measure the child and use natural size' in Measure (absolute-layout-manager.cpp:145-148), and in Arrange any w<0 with RequestedWidth!=MATCH_PARENT falls back to GetMeasuredSize (line 251-253). So setting bounds width to MATCH_PARENT (-2) via SetWidth does NOT make the child fill — only the View's own SetRequestedWidth(MATCH_PARENT) is honored, and only in Arrange. Two independent width channels (params bounds vs View RequestedWidth) overlap with unclear precedence: a child with bounds.width=-2 and RequestedWidth=fixed will use natural size from bounds path, ignoring both.

- **주류 프레임워크와의 차이**: A single, documented precedence: either bounds.width fully overrides the View's RequestedWidth, or it defers to it. Here bounds.width>=0 overrides; bounds.width<0 defers to the View's measured/Requested value, but the MATCH_PARENT-fill branch only consults GetRequestedWidth, never the bounds sentinel — so -1 and -2 in bounds are indistinguishable to the fill logic. The -2 sentinel is meaningless in bounds, which is surprising given MATCH_PARENT is a first-class constant. (Android LayoutParams.width = MATCH_PARENT/WRAP_CONTENT as the single sizing channel)

- **의도 판단 근거**: I re-read all cited code under /home/jae/dali/dali-ui-claude.  FACTS CONFIRMED: - Doc comment "width (-1 means use View's own size)" exists at absolute-layout-params.h:146 (and :161 for height). (The finding cited line 148; actual is 146 — the param tag, not the decl.) - Measure: absolute-layout-manager.cpp:141-148 — for w<0 it measures the child against available content space and sets w = childSize.width (the View's resolved size). Confirmed. - Arrange fill branch: absolute-layout-manager.cpp:237-266 — when w<0, it fills (availableWidth - margin) ONLY if childImpl.GetRequestedWidth()==MATCH_PARENT (line 245); otherwise w = childMeasured.wid…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟡 MEDIUM · `layout-base-callbacks/layout-04` — Measure and Arrange dispatch select their source independently — measure-via-callback can coexist with arrange-via-manager

- **위치**: `view-impl.cpp:918-926, view-impl.cpp:1056-1062`

- **현재 동작**: Measure checks only GetMeasureCallback() then GetLayoutManager() (view-impl.cpp:918-926); Arrange independently checks only GetArrangeCallback() then GetLayoutManager() (view-impl.cpp:1056-1062). If only a measure callback is set on a Layout that also has a manager, measurement runs through the callback but arrangement runs through the manager (and vice-versa).

- **주류 프레임워크와의 차이**: In WPF/Android/Compose, measure and arrange of one element are always produced by the same authority; splitting them across two unrelated algorithms (a callback that measured a custom geometry, then a Grid manager that arranges) yields incoherent positions/sizes. (WPF MeasureOverride+ArrangeOverride are one unit; Compose MeasurePolicy.measure does both)

- **의도 판단 근거**: The "observed" code reading is factually accurate but the framing as an anomaly is wrong; this is documented, intentional per-phase dispatch.  Verified the cited code: - view-impl.cpp:918-926 (Measure): checks GetMeasureCallback() then GetLayoutManager() then OnMeasure() — independent per-phase fallback chain. - view-impl.cpp:1056-1067 (Arrange): independently checks GetArrangeCallback() then GetLayoutManager() then OnArrange() — same independent chain. - The two callbacks are independently settable with no coupling: SetMeasureCallback/SetArrangeCallback are separate methods (view-impl.cpp:1553-1561), stored as separate fields mOnMeasure/mOnA…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟠 HIGH · `layout-params/lp-01` — SetLayoutParams shares the params impl by reference across Views (no deep copy) — a footgun absent from every mainstream framework

- **위치**: `view-impl.cpp:101-111 (ToTraitObject, no copy), view-impl.cpp:2170-2175 (SetLayoutParams), stack-layout-params.cpp:36-46 (New vs New(other)), docs/layout-structure.md:58 (explicit warning that this is intentional)`

- **현재 동작**: SetLayoutParams stores the SAME impl object on the View: ToTraitObject(params) (view-impl.cpp:101-111) wraps the existing impl pointer in an IntrusivePtr and SetTrait stores it as-is (view-impl.cpp:2173). Passing one handle to two Views makes both traits point to the identical StackLayoutParamsImpl. Mutating it later (e.g. viewA.GetLayoutParams<StackLayoutParams>().SetWeight(5)) silently changes viewB's layout too, with no guard or copy-on-write.

- **주류 프레임워크와의 차이**: Android ViewGroup.LayoutParams and WPF attached properties are per-child: a child's layout params are never shared by reference with a sibling. setLayoutParams effectively gives each child its own instance, so mutating one child's params cannot corrupt another's. (Android ViewGroup.LayoutParams (per-child ownership); WPF attached properties (per-element local value store))

- **의도 판단 근거**: The finding's mechanical claims are CODE-ACCURATE, but its framing as a "footgun absent from every mainstream framework" / unusual-design defect is wrong; this is the deliberate, documented DALi handle idiom.  Mechanism verified at /home/jae/dali/dali-ui-claude: - view-impl.cpp:101-111 — ToTraitObject(BaseHandle) does NOT copy; it dynamic_casts the existing impl pointer and wraps it in IntrusivePtr<TraitObject>(traitObject) (same object). - view-impl.cpp:2170-2175 — SetLayoutParams stores ToTraitObject(params) into the trait as-is via mImpl->SetTrait(...). - view-impl.cpp:2163-2168 — GetLayoutParams returns BaseHandle(baseObject) re-wrapping …

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟠 HIGH · `layout-params/lp-02` — Mutating a params handle after SetLayoutParams does not invalidate measure (silent stale layout)

- **위치**: `stack-layout-params.cpp:72-76 (SetWeight has no invalidation), view-impl.cpp:2170-2175 (only SetLayoutParams invalidates), view.h:1400-1402 (the GetLayoutParams doc DOES warn to call InvalidateMeasure, but SetLayoutParams path has no equivalent warning)`

- **현재 동작**: Params setters mutate impl fields directly with no View back-reference and no invalidation (stack-layout-params.cpp:72-76; impl fields at stack-layout-params-impl.h:69-72). Only SetLayoutParams calls InvalidateMeasure (view-impl.cpp:2174). So after view.SetLayoutParams(p); a later p.SetWeight(2.0f) updates the data the manager will read but never schedules a relayout — the change applies only on the next unrelated invalidation.

- **주류 프레임워크와의 차이**: In WPF a property change invalidates measure automatically; in Android the idiom is mutate-then-requestLayout, and the framework documents/expects it. Here the most natural call site (mutating the handle you just passed to SetLayoutParams) gives no invalidation and the SetLayoutParams doc (docs/layout-structure.md:45) says nothing about needing a manual InvalidateMeasure after later mutation. (WPF FrameworkPropertyMetadata AffectsMeasure; Android requestLayout contract)

- **의도 판단 근거**: I re-read every cited line. The finding's DATA-FLOW facts are all correct, but its severity/bug framing is wrong: this is the framework's deliberate, documented, tested contract.  Verified facts: 1. Params setters mutate impl fields directly with no View back-reference and no invalidation — stack-layout-params.cpp:72-76 (SetWeight -> GetImpl(*this).SetWeight); impl at stack-layout-params-impl.h:69-72 (just `mWeight = weight`). 2. SetLayoutParams stores the handle's impl object AS-IS, not a copy: view-impl.cpp:2173 calls ToTraitObject(params), and ToTraitObject (view-impl.cpp:101-111) returns an IntrusivePtr to the SAME TraitObject the handle …

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟢 LOW · `layout-transition/lt-04` — No re-entrancy guard against the documented-forbidden tree mutation from animator callbacks; only defensive ordering, not prevention

- **위치**: `dispatcher:1605-1616 (comment acknowledges callbacks 'may still do it', state may dangle); docs layout-transition.md:229-232 state the prohibition with no enforcement in code`

- **현재 동작**: The docs forbid Add/Remove/Unparent/InvalidateMeasure from animator callbacks (layout-transition.md:229-232), but the dispatcher does not assert or guard against it. Instead DispatchOneTick (dispatcher:1605-1616) and TickAnimators step 2 (1738-1761) silently tolerate a callback that erases its own entry and inserts a new one, relying on the freshlyCreated flag. A callback that, e.g., calls Remove on a sibling can still re-enter ScheduleExit / OnViewDestroyed mid-iteration.

- **주류 프레임워크와의 차이**: A framework that hard-forbids an operation typically enforces it (an in-callback flag + DALI_ABORT/log), as it does for lifecycle re-entrancy elsewhere (StartTransitionsForView snapshots and pins handles, 565-601). Silent tolerance means a violating app gets undefined visual results rather than a diagnosable error. (Android ValueAnimator does not forbid this but documents reentrancy; AndroidX guards TransitionManager re-entry)

- **의도 판단 근거**: I re-read all cited code under /home/jae/dali/dali-ui-claude.  OBSERVED CLAIM — accurate. The docs do forbid the mutation: layout-transition.md:229-232 ("No view-tree mutations from animator callbacks. Do not call Add / Remove / Unparent / InvalidateMeasure from inside a LayoutAnimatorCallback"). The dispatcher does NOT enforce this. DispatchOneTick (layout-transition-dispatcher.cpp:1605-1616) invokes cb->Invoke(ctx) with a comment explicitly conceding "The user callback can mutate the view tree (docs forbid this for animator callbacks, but applications may still do it)" and only defends by ordering: it never touches `state` again after the c…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟢 LOW · `layout-transition/lt-05` — ENTER/EXIT animators receive fromBounds==toBounds, so the framework supplies no enter-from/exit-to geometry to interpolate

- **위치**: `dispatcher:1459-1461, 1513-1515; documented as intentional in layout-transition-types.h:395-403 ('fromBounds equals toBounds ... dispatcher has no concept of an enter-from or exit-to position')`

- **현재 동작**: StartAnimatorEnter sets fromBounds=toBounds=bounds (dispatcher:1459-1461); StartAnimatorExit sets fromBounds=toBounds=bounds (1513-1515). DispatchOneTick lerps from==to (1577-1580), so ctx.fromBounds==ctx.toBounds for ENTER/EXIT and the framework's lerp is a no-op. The application callback must fully own bounds interpolation for enter/exit.

- **주류 프레임워크와의 차이**: Android LayoutTransition's APPEARING/DISAPPEARING animators are handed meaningful start/end geometry (the appearing view's pre/post bounds) so a generic slide/scale animator works without the app computing endpoints. Here the bounds-effect channel exists for spec mode, but animator-mode ENTER/EXIT gets nothing. (Android LayoutTransition APPEARING/DISAPPEARING animator staging with bounds)

- **의도 판단 근거**: The mechanical claim is verified accurate. StartAnimatorEnter sets fromBounds=toBounds=bounds (layout-transition-dispatcher.cpp:1459-1461); StartAnimatorExit sets fromBounds=toBounds=bounds (1513-1515, where `bounds` is the current visual bounds from CurrentVisualBoundsForExit at 1502, not necessarily the arranged bounds — but the two are still equal, so DispatchOneTick's Lerp1D at 1577-1580 is a no-op for ENTER/EXIT). So ctx.fromBounds==ctx.toBounds and the framework's lerp contributes zero travel. That part is correct.  But this is explicitly, deliberately specified — not an unintended deviation: 1. Doc note layout-transition-types.h:395-40…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟢 LOW · `layout-transition/lt-06` — CHANGE/SIBLING/REORDER/window-resize cause markers leak across windowless intervals and depend on a global mInWindowResize flag, risking misclassification

- **위치**: `view-impl.cpp:1991-2002, 1857-1863, 1813-1826, 1605-1635 (defensive set/clear guards); dispatcher:610-619, 855-866, 1678-1681 (global resize flag + precedence)`

- **현재 동작**: Cause markers (mPendingChildRemovalForLayoutTransition, mPendingReorderedChildren) are per-ViewImpl boolean/set state consumed only when StartTransitionsForView runs under an attached transition. The code adds many narrow guards (only set when a transition is attached AND a window exists, view-impl.cpp:1999-2002, 1860-1863, 1823-1826; cleared on detach 1618-1634) precisely because a marker set without a consuming layout pass mis-tags a later unrelated CHANGE. WINDOW_RESIZED relies on a single global mInWindowResize per pass (dispatcher:619, 1680) applied to every CHANGE child including inherited SUBTREE descendants (763-764).

- **주류 프레임워크와의 차이**: A robust cause-attribution model ties the cause to the specific mutation event, not to ambient per-view flags that must be defensively gated against leaking. The proliferation of 'only set when window && transition && isCurrentChild' guards indicates the marker lifecycle is fragile. (Android LayoutTransition derives CHANGE_APPEARING/CHANGE_DISAPPEARING directly from the add/remove event)

- **의도 판단 근거**: I re-read every cited line. The finding's factual ("observed") claims about WHAT the code does are all accurate verbatim:  - view-impl.cpp:1999-2002 — `if(transition && window && isCurrentChild) { mPendingChildRemovalForLayoutTransition = true; }` exactly as cited. - view-impl.cpp:1860-1863 (`transition && hadChildren`) and 1823-1826 (`transition && !snapshot.empty()`) — the bulk-remove / RemoveAllChildren guards exist exactly. - view-impl.cpp:1618-1634 — SetLayoutTransition(detach) clears mPendingEnterChildren, mPendingReorderedChildren, mPendingChildRemovalForLayoutTransition, and ClearPendingInheritedEnters, exactly as claimed. - view-impl…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### ⚪ INFO · `layout-transition/lt-07` — Lifecycle cancellation is unconditionally silent with no cancel/finish distinction, unlike canonical animator listeners

- **위치**: `dispatcher:1290-1364 (silent cancels), 2019-2030 (reparent cancels all silently); contract documented at layout-transition.md:211-216,260-289`

- **현재 동작**: Every cancel path (CancelActiveAnimation/CancelActiveAnimator/CancelPendingExit) drops the in-flight callback without firing OnFinished and without any cancelled-reason signal (dispatcher:1290-1364; OnChildReparented 2019-2030). OnFinished fires only on natural completion (FinalizeAnimator 1672-1675, OnAnimationFinished 2158-2208).

- **주류 프레임워크와의 차이**: Android's AnimatorListener splits onAnimationCancel vs onAnimationEnd; AndroidX Transition has TransitionListener.onTransitionCancel. Apps doing cleanup/chaining in OnFinished get no callback at all when a transition is superseded, reparented, or destroyed — they must independently detect those, which the docs admit (layout-transition.md:260-289). (Android Animator.AnimatorListener.onAnimationCancel; AndroidX TransitionListener.onTransitionCancel)

- **의도 판단 근거**: Every factual claim in the finding holds against the source, but the behavior is a deliberate, documented, UTC-asserted contract rather than a defect — hence intentional-design (the finding itself already classifies it as info/convention-deviation and concedes it is documented).  Verified facts: 1. Silent cancels with no OnFinished. CancelActiveAnimation (layout-transition-dispatcher.cpp:1290-1330), CancelPendingExit (1332-1348), and CancelActiveAnimator (1350-1364) each Stop the animation/erase the entry and restore transient/interaction state, but NONE of them call EmitLifecycle. Confirmed no callback fire on any cancel path. 2. OnFinished …

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟢 LOW · `scroll-bounds-effects/scroll-04` — MATCH_PARENT scrollable size uses viewport but is re-measured/arranged at viewport even for the scroll axis (no scroll possible by design, but inconsistent with 'follower' Measure contract)

- **위치**: `scroll-view-layout-manager.cpp:78-89,128-140; docs/layout-structure.md:131-139 (global follower contract) vs 162 (ScrollView override)`

- **현재 동작**: For a MATCH_PARENT child, Measure sets effectiveWidth/Height = the viewport constraint and feeds that into maxWidth/maxHeight (scroll-view-layout-manager.cpp:86-89), so SetScrollableWidth/Height get the viewport size. Arrange then forces childBounds to the viewport and re-measures (lines 128-140). This makes a MATCH_PARENT content exactly fill the viewport and never scroll. That matches docs/layout-structure.md:162 ('ScrollView: MATCH_PARENT children fill the viewport in Arrange') and the sample (samples/scrollview/scrollview-layout-test.cpp:64-75), but it deviates from the documented general MATCH_PARENT 'follower' contract (docs/layout-structure.md:131-139) where MATCH_PARENT must report minimum (0) in Measure and not influence the parent/scrollable size.

- **주류 프레임워크와의 차이**: Either consistent with the global follower rule (MATCH_PARENT contributes 0 to the measured/scrollable extent and is only sized in Arrange), or explicitly documented as a ScrollView-specific override. The current code is the latter and is documented, but the Measure-time use of the viewport as the child's contributed extent is a local special case worth flagging. (Android match_parent inside a scroll container resolves to viewport size along the cross axis and is generally discouraged along the scroll axis; this matches the ScrollView override but not the framework's own follower rule)

- **의도 판단 근거**: I re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/scroll-view-layout-manager.cpp.  Measure (lines 78-92): for a MATCH_PARENT child, childWidth/HeightConstraint = the incoming viewport constraint (81-82), and effectiveWidth/Height = widthConstraint/heightConstraint, contributing to maxWidth/maxHeight returned by Measure (86-89, 92). This is exactly what the finding's "observed" describes — CONFIRMED as factual.  Arrange (lines 95-151): MATCH_PARENT childBounds are forced to bounds.width/height (the viewport) at 130/134, re-measured at 139, then SetScrollableWidth/Height are set to childBounds (= vi…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### ⚪ INFO · `scroll-bounds-effects/bounds-01` — layout-bounds-effects is a transition-animation API, not scroll over-scroll/bounce; factories are internally consistent and tested

- **위치**: `layout-bounds-effects.cpp:76-83,110-116; layout-bounds-effects.h:49-67; docs/layout-transition.md:91-95; utc-Dali-LayoutTransition.cpp:2040-2052,2081-2094`

- **현재 동작**: LayoutBoundsEffects::SlideFrom/SlideTo/ExpandFrom/ShrinkTo build LayoutBoundsEffect descriptors for the LayoutTransition enter/exit channel, not for scroll bounce. SlideTo delegates to SlideFrom (layout-bounds-effects.cpp:76-83) and ShrinkTo delegates to ExpandFrom (110-116) by design ('mirror semantics': ENTER plays endpoint→base, EXIT plays base→endpoint), documented in docs/layout-transition.md:91-95 and the header (layout-bounds-effects.h:49-67), and verified by UTCs (utc-Dali-LayoutTransition.cpp:2040-2094). Over-scroll/bounce/clamp is implemented separately in ScrollViewImpl via EdgeEffect (scroll-view-impl.cpp:1548-1563 OnPull/OnAbsorb) and AdjustScrollPosition clamping (1114-1135), and edge cases (negative position clamped to >=0, content smaller than viewport via std::max(0, scrollable-viewport)) are handled there.

- **주류 프레임워크와의 차이**: No defect: this is the intended factorization. Flagged only to correct the audit premise that bounds-effects governs scroll bounce. SlideTo==SlideFrom and ShrinkTo==ExpandFrom returning identical values is intentional and tested, though a caller reading SlideTo expecting an inverted/opposite direction from SlideFrom could be surprised (the direction is selected by the enter/exit slot, not the factory name). (N/A for scroll; comparable to CSS @keyframes reversed via animation-direction rather than separate keyframe sets)

- **의도 판단 근거**: The finding's "observed" claim is precisely accurate. I re-read every citation in /home/jae/dali/dali-ui-claude:  - layout-bounds-effects.cpp:76-83: SlideTo(edge, distance, timing) body is literally `return SlideFrom(edge, distance, timing);` with the "Mirror semantics" comment — confirmed delegation, identical descriptor. - layout-bounds-effects.cpp:110-116: ShrinkTo(anchor, timing) body is `return ExpandFrom(anchor, timing);` with the mirror-semantics comment — confirmed. - layout-bounds-effects.h:49-67 + 78-119: doc block states the dispatcher uses mirror semantics (ENTER endpoint→base, EXIT base→endpoint) and that SlideFrom/SlideTo and Ex…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟠 HIGH · `x-matchparent-wrapcontent-matrix/layout-01` — Multiple MATCH_PARENT children on the MAIN axis overflow/overlap in StackLayout

- **위치**: `stack-layout-manager.cpp:464-467, 508, 514-517, 558; contribution-0 in Measure at stack-layout-manager.cpp:96-105`

- **현재 동작**: In StackLayoutManager::Arrange a MATCH_PARENT main-axis child is sized to the FULL remaining axis (VERTICAL: childHeight = availableHeight - marginH, stack-layout-manager.cpp:464-467; HORIZONTAL: childWidth = availableWidth - marginW, :514-517) and currentY/currentX is advanced by that full slot (:508,:558). With two or more non-weight MATCH_PARENT main-axis children each takes the entire available main extent, so they overlap at the same origin and the cumulative cursor runs far past the bounds. Measure contributes 0 for each (:98-105), so the container never reserves space for them.

- **주류 프레임워크와의 차이**: A general framework either splits the main axis among MATCH_PARENT siblings (or requires weight for that), or clamps so children do not overlap. Android LinearLayout treats height=MATCH_PARENT on the main axis as 0 unless weight is set; CSS would still apply flex-grow/shrink. Filling 100% per child and stacking them is surprising. (Android LinearLayout weight/MATCH_PARENT on orientation axis; CSS Flexbox 9.7 resolving flexible lengths)

- **의도 판단 근거**: I re-read every cited line. The mechanics are exactly as claimed:  OBSERVED (verified): - Arrange VERTICAL: stack-layout-manager.cpp:464-467 sets childHeight = max(0, availableHeight - marginH) whenever GetRequestedHeight()==MATCH_PARENT (i.e., full remaining main extent, NOT a per-child share); :468 slotHeight = childHeight + marginH; :508 currentY += slotHeight + visSpacing with NO clamp to bounds. HORIZONTAL mirror at :514-517, :518, :558. All citations correct. - Measure contribution: MeasureStackNonWeightChildren measures each non-weight child (:94) and adds childSize.height/width (:98,:103). For a MATCH_PARENT child, View::Measure retur…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟢 LOW · `x-matchparent-wrapcontent-matrix/layout-08` — Plain-View OnMeasure measures MATCH_PARENT children with WRAP-derived content constraint that can be ~0, so nested re-measure in Arrange is the only correct sizing pass

- **위치**: `view-impl.cpp:962-993 (measure children with content constraint), :1136-1151 (override to fill + re-measure in Arrange); managers seed Arrange buffers from GetMeasuredSize (e.g. stack :358, flex :576) before their own re-measure`

- **현재 동작**: In plain-View OnMeasure, every non-standalone child (including MATCH_PARENT) is measured against contentWidth/Height derived from the parent's effective (possibly WRAP-collapsed) size minus padding/margin (view-impl.cpp:981-983). For a WRAP_CONTENT parent containing a MATCH_PARENT child, contentWidth is itself derived from natW which for a WRAP parent is the incoming constraint; nested MATCH_PARENT-in-MATCH_PARENT-in-WRAP chains thus measure each level against a constraint that only becomes real in Arrange (view-impl.cpp:1148-1151 re-measure). The Measure-phase child sizes for MATCH chains are therefore throwaway, relying entirely on the Arrange re-measure. Correct, but means Measure-time GetMeasuredSize() for nested MATCH subtrees is stale until Arrange.

- **주류 프레임워크와의 차이**: Documented as intended (layout-structure.md:141-149 re-measure in Arrange), but the double-pass dependency is fragile: any manager/path that reads a MATCH child's GetMeasuredSize() before the Arrange re-measure gets the minimum, not the final size. (Android two-pass measure for weighted/MATCH children; WPF/UWP Measure caches DesiredSize separate from final ArrangeRect)

- **의도 판단 근거**: The finding describes the plain-View two-pass MATCH_PARENT sizing as a "design-smell" (it self-rates severity=low and admits "Correct, but..."). Re-investigating the real source confirms it is a deliberate, explicitly documented design, not a defect.  Code facts (all under /home/jae/dali/dali-ui-claude): - view-impl.cpp:959-983 — plain-View OnMeasure derives contentWidth/Height from the parent's effective size and measures every non-standalone child against it. Accurate. - view-impl.cpp:1135-1151 — in OnArrange, MATCH_PARENT children get childW/childH = parent content area, then are re-measured with the final visual size before Arrange. This …

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟢 LOW · `x-matchparent-wrapcontent-matrix/layout-09` — Stack weight distribution path diverges between Measure and Arrange (Measure can skip weight children when targetMain<=wrappedMain)

- **위치**: `stack-layout-manager.cpp:236-313 (Measure branch logic), :361-443 (Arrange always redistributes)`

- **현재 동작**: In Stack Measure, when totalWeight>0 but targetMain<=wrappedMain, weight children are measured at the raw constraint and their natural main size is added to mainAxisNonWeight (stack-layout-manager.cpp:268-303), i.e. weight is effectively ignored in that branch. Arrange instead always recomputes weight shares from the actual available main (stack-layout-manager.cpp:391-442). So a weighted MATCH_PARENT child's Measure-time size and Arrange-time size can differ for reasons unrelated to bounds changing, and the measured container size may not match the arranged layout.

- **주류 프레임워크와의 차이**: Measure and Arrange should agree on the model so the cached MeasuredSize predicts the arranged result; the conditional skip is easy to get subtly wrong. (Android LinearLayout measures weighted children twice to keep measure/layout consistent)

- **의도 판단 근거**: I re-read the cited code and the contract doc. The "observed" mechanical reading is accurate: in Stack Measure, when targetMain<=wrappedMain the else branch (stack-layout-manager.cpp:268-303) measures each weight child at the raw constraint and adds its returned (natural/min) size to first.mainAxisNonWeight (lines 289-301), i.e. no weight share is applied; while Arrange always redistributes weight from the actual available main when totalWeight>0 (stack-layout-manager.cpp:391-442). The first pass indeed skips weight children (lines 82-87), and MeasureStackWeightChildren only runs when targetMain>wrappedMain (lines 261-266). So the code does w…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟡 MEDIUM · `x-measure-cache/cache-02` — MATCH_PARENT 'final-bounds re-measure' is silently a cache no-op when final size == measure-pass constraint, so the subtree is not re-laid-out as documented

- **위치**: `docs/layout-structure.md:143-147; re-measure calls flex-layout-manager.cpp:348-350, view-impl.cpp:1148-1151; cache hit view-impl.cpp:908-912`

- **현재 동작**: Every Arrange path re-measures a MATCH_PARENT child at its final bounds (view-impl OnArrange:1148-1151; flex:348-350; stack:502-504,552-554; grid:344-346; absolute:296-298; standalone:287-290). But ViewImpl::Measure caches on the clamped constraint pair (view-impl.cpp:908-912). When the final arranged size equals the constraint the child was measured with during the Measure pass (the common single MATCH_PARENT-fills-parent case), the re-measure is a guaranteed cache hit and runs NOTHING — the child's nested WRAP_CONTENT/text content is never re-measured against 'real' space.

- **주류 프레임워크와의 차이**: Per docs, the re-measure exists so the child's internal state (text layout, nested children) reflects the real available space (layout-structure.md:143-147). The code relies on the cache to skip redundant work, but because the Measure-pass constraint already equals the final size in the fill case, the documented re-layout effect is a no-op there; conversely it only does work when sizes differ — i.e. exactly opposite to the doc's framing of 'prevents redundant work when constraint unchanged'. It is correct numerically (size comes from bounds, not the return value) but the re-measure call is dead in the equal case and load-bearing only in the unequal case. (WPF UpdateLayout re-runs Measure with the dirty flag; Android re-measures in onLayout only via measure() which honors PFLAG_FORCE_LAYOUT.)

- **의도 판단 근거**: The finding's raw code mechanics are accurate, but its central thesis (a doc/code contradiction making the re-measure a broken no-op) is refuted by the source.  Verified code facts (worktree paths): - Cache key: view-impl.cpp:908-912 compares clamped natural constraint mLastMeasuredConstraint against effNatW/effNatH and returns cached mMeasuredSize on a match. - Re-measure at final bounds: view-impl.cpp:1147-1151 (default OnArrange), flex-layout-manager.cpp:348-351, stack-layout-manager.cpp:502-505 and 552-555. All call childImpl.Measure(childW/childH) only for MATCH_PARENT children, immediately before Arrange. Cited lines all hold. - Measure…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟡 MEDIUM · `x-margin-padding-alignment/mp-01` — Main-axis MATCH_PARENT children overlap in Stack and Flex (each grabs full main extent)

- **위치**: `stack-layout-manager.cpp:464-467,508; flex-layout-manager.cpp:272-278,306,319-321. Documented as 'use weight/flex-grow for proportional sharing' in docs/layout-structure.md:152-157.`

- **현재 동작**: On Arrange, a main-axis MATCH_PARENT child is sized to the FULL available main extent minus its own margin, independent of siblings. Stack VERTICAL: childHeight = availableHeight - marginH (stack-layout-manager.cpp:464-467) while currentY still advances by slotHeight (508), so two such children overlap and overflow. Flex: childMainSize = availMain - marginMain where availMain is the whole content main size, not remaining line space (flex-layout-manager.cpp:272-277), and mainOffset advances by allocMain (flex 319-321).

- **주류 프레임워크와의 차이**: In CSS fl/stack layouts a stretched/fill main-axis item shares the main space with siblings (flex default distributes remaining space). Filling the entire main axis per-child and then advancing the cursor produces overlapping, out-of-bounds children. (CSS Flexbox 9.7 (resolving flexible lengths) / align-items:stretch — stretched items share the line, they do not each take the full main size.)

- **의도 판단 근거**: The "observed" code claim is factually accurate, but it describes the documented, intended contract — so this is not a bug.  Stack (VERTICAL), no-weight child path: availableHeight = bounds.height (full container height; stack-layout-manager.cpp:350). For a MATCH_PARENT child, childHeight = availableHeight - marginH (lines 464-467), then currentY advances by slotHeight = childHeight+marginH (line 468, 508). So two no-weight MATCH_PARENT children each get full height and the second is placed past the bottom edge — overlapping/overflowing, exactly as the finding states.  Flex main-axis path: for mainIsMatchParent, childMainSize = availMain - ma…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


#### 🟢 LOW · `x-margin-padding-alignment/grid-align-01` — Grid cell alignment is skipped when child measured size is 0 or >= cell, silently forcing FILL

- **위치**: `grid-layout-manager.cpp:301-320 (hAlign guard), 323-342 (vAlign guard).`

- **현재 동작**: Grid applies horizontal/vertical alignment only inside `if(childWidth > 0.0f && childWidth < cellWidth)` (grid-layout-manager.cpp:301) and the analogous height guard (323). If the child measured 0 (e.g. MATCH_PARENT reporting min 0, or empty) or is >= the cell, START/CENTER/END are all no-ops and the child keeps the full cell size (FILL).

- **주류 프레임워크와의 차이**: START/CENTER/END should still position a zero- or full-size child deterministically; conflating 'measured 0' with 'fill the cell' means a START-aligned MATCH_PARENT/empty child unexpectedly fills and is not left at the start edge. (WPF Grid / Android GridLayout: alignment is applied for all child sizes; FILL is a distinct alignment value, not an implicit fallback for size 0.)

- **의도 판단 근거**: The code reading in "observed" is accurate: grid-layout-manager.cpp:301 guards horizontal alignment with `if(childWidth > 0.0f && childWidth < cellWidth)` and :323 the analogous `childHeight` guard. When childWidth==0 or childWidth>=cellWidth, the switch is skipped and childBounds.width keeps the full cell value set at :291 (effectively FILL). So START/CENTER/END are no-ops in those degenerate cases.  However, the finding's load-bearing scenario — a START-aligned MATCH_PARENT child unexpectedly filling — is the explicitly documented intended contract, not a defect. view-impl.cpp:2444-2445 shows a MATCH_PARENT child's GetMeasuredSize returns G…

- **권장**: 동작 유지 시 문서/샘플에 관례 차이를 명시하거나, 관례 정합이 필요하면 위 차이를 해소.


---
## 요약 표 (수정 필요 confirmed, 서브시스템 × 심각도)

| 서브시스템 | HIGH | MED | LOW | INFO | 합 |
|---|---|---|---|---|---|
| Core Measure 엔진 | 2 | 1 | 1 | 0 | 4 |
| Core Arrange 엔진 | 0 | 2 | 1 | 0 | 3 |
| Standalone 자식 | 0 | 1 | 1 | 1 | 3 |
| 무효화 & LayoutController | 1 | 1 | 3 | 1 | 6 |
| StackLayout | 2 | 2 | 1 | 1 | 6 |
| FlexLayout | 2 | 5 | 2 | 0 | 9 |
| GridLayout | 1 | 3 | 3 | 0 | 7 |
| AbsoluteLayout | 2 | 3 | 1 | 0 | 6 |
| Layout 베이스 & 콜백 | 2 | 1 | 2 | 1 | 6 |
| LayoutParams | 0 | 2 | 2 | 0 | 4 |
| Layout 트랜지션 | 0 | 0 | 2 | 0 | 2 |
| ScrollView & BoundsEffects | 1 | 2 | 0 | 0 | 3 |
| [횡단] MATCH/WRAP 매트릭스 | 1 | 3 | 2 | 0 | 6 |
| [횡단] Measure 캐시 | 1 | 2 | 1 | 1 | 5 |
| [횡단] RTL/LayoutDirection | 1 | 2 | 3 | 0 | 6 |
| [횡단] margin/padding/정렬 | 1 | 1 | 3 | 0 | 5 |
| [횡단] 분배/반올림 | 1 | 4 | 3 | 0 | 8 |
| [횡단] 생명주기 변형 | 2 | 2 | 2 | 1 | 7 |
| [횡단] 수치/센티넬 경계 | 1 | 3 | 3 | 0 | 7 |
| [횡단] 테스트 커버리지 | 3 | 7 | 2 | 0 | 12 |
| **합계** | **24** | **47** | **38** | **6** | **115** |


---
## 부록: 검증으로 폐기된(수정 불필요) 항목

> 초기 감사에서 제기됐으나 검증자 다수가 **반박**(코드 오독 또는 '기대' 자체가 부정확)하여 수정 대상 아님.

- `measure-engine/measure-05` — Measure cache key is the post-clamp magnitude only; it omits requested mode and (after clamp) cannot distinguish WRAP vs MATCH vs a real 0 constraint at the root — 반박근거: The finding's low-level facts are accurate but its conclusion (a real, exploitable cache deviation) is wrong, and its framework claim is half-stated.

VERIFIED FACTS (finding correct here):
- Cache key is the post-clamp magnitude effNatW/ef

- `measure-engine/measure-06` — Visual->natural conversion uses `> 0` in Measure but `>= 0` in OnMeasure — a zero-size constraint is handled asymmetrically — 반박근거: The literal citations hold: view-impl.cpp:897 uses `(visualW > 0.f) ? visualW/s : visualW` and view-impl.cpp:945-950 OnMeasure uses `(widthConstraint >= 0.f && s > 0.f) ? widthConstraint/s : widthConstraint`. At input exactly 0, line 897 ta

- `arrange-engine/arr-01` — MATCH_PARENT re-measure result is discarded; WRAP_CONTENT cross-axis child cannot reflow — 반박근거: I re-derived the algorithm from the cited code (files are under /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/, not /src/internal/ as the finding wrote, but line numbers match). The finding's mechanism is real but its load-bea

- `arrange-engine/arr-06` — WRAP_CONTENT container measure adds both start+end child margin to the right/bottom extent — 반박근거: I re-read both cited sites in /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp.

MEASURE (view-impl.cpp:977-992): margin "natMarginW = margin.start + margin.end" (full margin, line 979/989), childX = childImpl.GetRe

- `absolute-layout/abs-07` — Proportional position uses content box but CSS resolves abspos insets against the padding box — and no clamping for negative/over-1 proportions — 반박근거: I re-read every cited line. The formula is real and accurately quoted: absolute-layout-manager.cpp:157 `x = (contentWidth - w) * bounds.x`, :165 (y), :272 `x = (availableWidth - w) * childBoundsSpec.x`, :280 (y) — no clamping of bounds.x/y 

- `absolute-layout/abs-09` — maxBottom/maxRight bounding box ignores child x/y when negative and mixes margin asymmetrically — 반박근거: File is at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/layouts/absolute-layout-manager.cpp (the finding's cited line numbers are off by ~40, but the code content matches). The finding bundles two claims; one is real, the hea

- `layout-base-callbacks/layout-06` — MATCH_PARENT final-size re-measure during Arrange happens only in default OnArrange, not in the manager or callback arrange paths — 반박근거: The finding claims the MATCH_PARENT final-size re-measure "exists only in ViewImpl::OnArrange" and that the manager/callback arrange paths do not perform it, leaving every custom manager to re-implement it. The narrow factual claim about th

- `layout-transition/lt-01` — Zero-duration animator fires two ticks across two TickAnimators turns, contradicting the documented single-tick/same-turn contract — 반박근거: The finding misreads the freshlyCreated lifecycle. It claims TickAnimators step 2 skips finalization of a freshly-created zero-duration animator, deferring OnFinished to a second turn. The actual control flow refutes this.

TickAnimators ru

- `scroll-bounds-effects/scroll-05` — Measure re-runs full child Measure with float::max() on every layout pass with no scroll-axis caching note; reported scrollable size mixes visual and content spaces — 반박근거: The finding's central claims do not hold against the actual code.

1) "Measure return value is effectively dead for the ScrollView's sizing." REFUTED. The host is the generic DispatchMeasureWithLayoutManager (view-impl.cpp:2421-2457), not a

- `x-margin-padding-alignment/measure-cmp-01` — Inconsistent sentinel/zero handling: Measure uses '> 0' while OnMeasure/DispatchMeasure use '>= 0' for the scale conversion — 반박근거: Re-read the cited code at /home/jae/dali/dali-ui-claude/dali-ui-foundation/public-api/view-impl.cpp:897-898, 905-906, 945-960, 2421-2457, plus layout-controller.cpp:440-471, view-data-impl.h:614-647, and docs/layout-structure.md:105-152.

T

- `x-numeric-edge-cases/num-02` — ScrollView uses FLT_MAX as an unconstrained constraint, but ViewImpl::Measure divides it by effective scale, overflowing to +Inf when scale < 1 — 반박근거: Re-derived the full arithmetic for num-02 (scrollable width = +Inf when scale<1). The cited facts are individually correct but the claimed manifestation never occurs.

Confirmed cited facts:
- scroll-view-layout-manager.cpp:81-82: non-MATCH
