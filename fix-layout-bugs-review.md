# Layout bug fix commit review

대상 커밋: `36bf5f669fbe8a4f1434cc7d378e6c37d71b81b4`

대상 브랜치: `codex`

커밋 제목: `Fix layout sizing, measurement, and caching defects`

대상 worktree: `/home/jae/dali/dali-ui-codex`

## 결론

Layout 관련 버그 수정 방향은 전반적으로 일반적인 Layout 동작과 대체로 맞습니다. 특히 `WRAP_CONTENT` budget clamp, leaf padding 1회 계산, main-axis `MATCH_PARENT`보다 flex grow / stack weight 분배 우선, `GridLayout`의 span-to-AUTO 처리, ScrollView cross-axis bound, reentrant child list mutation snapshot 등은 커밋 의도와 테스트가 잘 맞물려 있습니다.

다만 merge 전에 반드시 정리해야 할 불일치가 있습니다. 첫째, 새 문서/주석의 negative constraint 설명과 실제 `ViewImpl::Measure()` 구현이 맞지 않습니다. 둘째, requested size validation이 special value 근처 값을 허용하지만 이후 판정은 exact equality라 경계값이 의도와 다르게 동작할 수 있습니다. 셋째, 이번 커밋이 추가한 public header/source/docs 주석에 프로젝트 규칙상 피해야 하는 외부 UI framework 이름이 남아 있습니다.

## Findings

### 1. 중요: negative constraint의 "unbounded" 계약이 실제 구현과 맞지 않습니다

근거:

- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view-impl.cpp:1078`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view-impl.cpp:1086`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view-impl.cpp:1096`
- `/home/jae/dali/dali-ui-codex/docs/layout-structure.md:127`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view-impl.cpp:1239`

문서와 새 주석은 `WRAP_CONTENT` / `MATCH_PARENT` 같은 음수 constraint가 unbounded sentinel로 `OnMeasure()`까지 전달된다고 설명합니다. 하지만 실제 `ViewImpl::Measure()`는 `natW` / `natH`를 계산한 뒤 `std::max(natW, minimum)`을 적용합니다. 기본 minimum은 `0`이므로 음수 constraint는 `0`으로 바뀌고, 그 결과 `effVisW` / `effVisH`도 `0`으로 전달됩니다.

따라서 현재 상태에서는 "negative constraint means unbounded"라는 새 문서 계약이 구현과 다릅니다. 의도한 계약이 unbounded라면 min/max pre-clamp에서 음수 sentinel을 보존해야 합니다. 반대로 현재 구현이 맞다면 문서와 주석에서 negative constraint unbounded 설명을 제거하거나 실제 동작에 맞게 고쳐야 합니다.

영향:

- LayoutManager나 custom `MeasureCallback` 작성자가 문서를 믿고 음수 constraint를 unbounded로 기대하면 실제로는 0 constraint를 받게 됩니다.
- `WRAP_CONTENT` clamp 설명도 "negative budget이면 clamp하지 않음"이라고 되어 있지만, 현재 top-level `Measure()` 경유에서는 음수가 먼저 0으로 바뀌어 해당 경로가 사실상 성립하지 않습니다.

### 2. 중간: requested size validation이 epsilon으로 special value 근처 값을 허용하지만 이후 판정은 exact equality입니다

근거:

- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/internal/views/view/view-data-impl.cpp:112`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/internal/views/view/view-data-impl.cpp:114`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view-impl.cpp:1224`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/scroll-view-layout-manager.cpp:87`

`IsValidRequestedSize()`는 `FloatEqual(v, WRAP_CONTENT)` 또는 `FloatEqual(v, MATCH_PARENT)`이면 valid로 인정합니다. 하지만 저장할 때 값을 `WRAP_CONTENT` / `MATCH_PARENT`로 normalize하지 않고 그대로 저장합니다. 이후 `ViewImpl::OnMeasure()`나 각 LayoutManager는 `GetRequestedWidth() == MATCH_PARENT`처럼 exact equality로 분기합니다.

예를 들어 `-1.9995f`가 들어오면 validation은 `MATCH_PARENT` 근처 값으로 통과할 수 있지만, 이후 exact equality에서는 `MATCH_PARENT`가 아니므로 `WRAP_CONTENT` 계열의 음수 요청처럼 처리될 수 있습니다.

권장 수정:

- special value를 허용할 때 저장 전에 정확히 `WRAP_CONTENT` 또는 `MATCH_PARENT`로 normalize합니다.
- 또는 validation을 exact sentinel만 허용하도록 바꿉니다.
- 관련 UTC는 NaN/Inf/잘못된 음수뿐 아니라 special value 근처 float도 포함하는 것이 좋습니다.

### 3. 낮음: 이번 커밋이 추가한 repository-facing 설명에 외부 UI framework 이름이 남아 있습니다

근거:

- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view.h:602`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view.h:621`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view.h:1789`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp:100`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp:163`
- `/home/jae/dali/dali-ui-codex/docs/layout-structure.md:127`
- `/home/jae/dali/dali-ui-codex/docs/layout-structure.md:205`
- `/home/jae/dali/dali-ui-codex/docs/layout-structure.md:227`

DALI UI 프로젝트 규칙에는 Layout 작업 시 외부 UI layout 모델 비교는 private analysis에서 하되, source code, comments, commit message, PR description, repository-facing description에는 외부 framework 이름을 남기지 말라고 되어 있습니다. 이번 커밋은 public header, source comment, docs에 해당 이름을 추가했습니다.

동작 버그는 아니지만 merge 전에 일반화된 표현으로 바꾸는 것이 좋습니다.

## 동작 검토 메모

### View 측정/배치

검토 근거:

- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view-impl.cpp:1081`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view-impl.cpp:1112`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view-impl.cpp:1147`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view-impl.cpp:1191`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view-impl.cpp:1236`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/view-impl.cpp:1376`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-View.cpp:2867`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-View.cpp:2892`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-View.cpp:2943`

확인한 내용:

- `WRAP_CONTENT` leaf padding collapse fix는 `GetNaturalSize()`가 padding을 포함하는 경우와 no visual 경우를 모두 고려해 padding을 한 번만 반영하려는 방향입니다.
- `MATCH_PARENT` child re-measure의 반환값을 배치 크기로 채택하지 않는 변경은 collapse 회귀를 막는 방향이며, 관련 UTC가 있습니다.
- child snapshot은 reentrant add/remove 중 live container iterator invalidation을 피하는 데 유효합니다.
- 단, 위 finding 1처럼 negative constraint 문서 계약은 실제 pre-clamp 동작과 맞지 않습니다.

### AbsoluteLayout

검토 근거:

- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/absolute-layout-manager.cpp:84`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/absolute-layout-manager.cpp:189`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-AbsoluteLayout.cpp:590`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-AbsoluteLayout.cpp:612`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-AbsoluteLayout.cpp:630`

확인한 내용:

- WRAP axis에서 size-proportional child를 제외하고, position-only proportional child는 determinate extent만 intrinsic size에 반영하는 방향은 circularity를 피하면서 content contribution을 보존합니다.
- 관련 UTC가 width/height 양쪽을 다룹니다.

### FlexLayout

검토 근거:

- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:94`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:274`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-FlexLayout.cpp:791`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-FlexLayout.cpp:830`

확인한 내용:

- `flex-basis: 0`에 해당하는 값이 기존 `basis > 0` 조건 때문에 무시되던 문제는 `basis >= 0`으로 고친 것이 맞습니다.
- main-axis `MATCH_PARENT`와 flex-grow가 같이 있는 경우 grow share가 우선하도록 한 것은 sibling overlap을 막는 방향입니다.
- grow가 없는 main-axis `MATCH_PARENT`는 기존 fill-line 동작을 유지하는 UTC가 있습니다.

### StackLayout

검토 근거:

- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp:391`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp:464`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp:514`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-StackLayout.cpp:725`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-StackLayout.cpp:766`

확인한 내용:

- weighted child가 main-axis `MATCH_PARENT`도 가지고 있을 때 full parent size로 overwrite하지 않고 weight allocation을 유지하는 변경은 올바른 방향입니다.
- vertical/horizontal 양쪽 UTC가 있습니다.

### GridLayout

검토 근거:

- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp:91`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp:139`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp:162`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp:262`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-GridLayout.cpp:521`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-GridLayout.cpp:564`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-GridLayout.cpp:727`

확인한 내용:

- spanning child가 AUTO track을 키울 때 ABSOLUTE track을 0으로 보고 과대 분배하던 문제는 ABSOLUTE track seed로 완화됩니다.
- pure AUTO span과 mixed AUTO+ABSOLUTE UTC는 의도와 맞습니다.
- mixed AUTO+STAR는 현재 테스트가 "STAR는 span pass에서 0으로 둔다"는 프로젝트 내부 계약을 문서화하고 있습니다. 일반 layout model과 1:1 대응한다기보다는 현재 구현 계약으로 명확히 고정한 형태입니다.

### ScrollView

검토 근거:

- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/scroll-view-layout-manager.cpp:60`
- `/home/jae/dali/dali-ui-codex/dali-ui-foundation/public-api/layouts/scroll-view-layout-manager.cpp:90`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-ScrollView.cpp:1343`
- `/home/jae/dali/dali-ui-codex/automated-tests/src/dali-ui-foundation/utc-Dali-ScrollView.cpp:1378`

확인한 내용:

- scrollable axis만 unbounded로 측정하고 non-scrollable cross axis는 parent constraint를 넘기는 변경은 overflow/clip side effect를 줄이는 방향입니다.
- cross-axis clamp와 scroll-axis unbounded 양쪽 UTC가 있습니다.

## 검증

수행한 확인:

- `git -C /home/jae/dali/dali-ui-codex status --short --branch`
- `git -C /home/jae/dali/dali-ui-codex log -1 --oneline --decorate`
- `git -C /home/jae/dali/dali-ui-codex show --stat --oneline --decorate --no-renames HEAD`
- `git -C /home/jae/dali/dali-ui-codex show --no-ext-diff --unified=... HEAD -- <changed files>`
- `rg` / `awk`로 관련 구현과 UTC 라인 확인
- `git -C /home/jae/dali/dali-ui-codex diff --check HEAD^ HEAD`

결과:

- `git diff --check HEAD^ HEAD`는 출력 없이 통과했습니다.
- 자동 테스트는 실행하지 않았습니다. `/home/jae/dali/dali-ui-codex/automated-tests/build`가 없어 기존 테스트 바이너리를 확인할 수 없었고, 리뷰 중 새 빌드는 수행하지 않았습니다.

## 권장 조치

1. negative constraint 계약을 결정하십시오.
   - 음수 sentinel을 unbounded로 유지하려면 `ViewImpl::Measure()`의 min/max pre-clamp가 음수를 0으로 바꾸지 않도록 수정하십시오.
   - 현재 0 clamp가 의도라면 `docs/layout-structure.md`와 `view-impl.cpp` 주석의 unbounded 설명을 실제 동작에 맞게 고치십시오.

2. requested size special value 처리 방식을 하나로 맞추십시오.
   - `FloatEqual()`로 허용한 값을 sentinel로 normalize하거나,
   - exact `WRAP_CONTENT` / `MATCH_PARENT`만 valid로 인정하십시오.

3. 이번 커밋에서 추가한 public header/source/docs의 외부 UI framework 이름을 repository-facing 설명에서 제거하십시오.

4. 빌드 가능한 환경에서 변경된 UTC 묶음을 실행하십시오.
   - View, AbsoluteLayout, FlexLayout, GridLayout, StackLayout, ScrollView, LayoutController 테스트가 주요 대상입니다.
