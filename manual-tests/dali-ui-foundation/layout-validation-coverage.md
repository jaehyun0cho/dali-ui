# Layout 신규 검증 suite

이 문서는 `manual-tests/dali-ui-foundation/tc/tc-lr01…tc-lr65` Layout 회귀 suite의 구성 결정과 서버 LLM의 실행·판정 기준입니다. 관측은 controller의 event-side 결과, 새 render task 완료 후의 current scene geometry, 별도 subtree render의 pixel, 직접 호출한 계산 결과로 구분합니다. 같은 geometry 수치라도 관측 경로가 다르므로 서로의 증거로 대체하지 마십시오.

## 현재 상태

- 실행 상태: 아래 내용은 수정된 source의 검증 계약입니다. 현재 변경본의 최종 전체 실행, 독립 반복 실행과 reference/candidate 시간 비교 결과는 별도 실행 보고서로 판정하십시오. 이전 artifact의 실행 합계나 일부 TC의 통과를 현재 변경본의 전체 통과로 사용하지 마십시오. 서버 실행 전 manifest의 `execution_evidence`는 `pending`입니다.
- 기존 Layout TC45–TC94의 C++/MD는 유지하지만 신규 suite의 판정에 합산하지 않습니다. 기존 launcher, `tc/*.cpp` 자동 등록과 module별 앱 구조를 사용합니다. TC별 MD의 검토된 scenario/action/required_checks가 실행 계약입니다.
- LR45 resize 관측은 `SetPositionSize()` 전에 `AfterLayout()` listener를 연결합니다. resize 호출 안에서 layout 완료 signal이 동기 발생하면 호출 후 연결한 listener가 이를 놓치는 것이 이전 timeout의 원인이었습니다. 수정 후에도 timeout이 생기면 새 원본 로그를 보존하고 관측 실패로 처리하십시오.
- transition seek는 고정 80ms 대기 대신 `AfterFrame()`의 새 render task 완료와 요청 progress 반영을 기다립니다. readiness는 입력 progress만 판별하며 geometry는 이후 독립 expected와 비교합니다. LR39 ancestor replay 검사는 같은 방식으로 fresh scene state를 읽습니다. 재실행에서 통과했다고 최초 실패를 삭제하거나 tolerance를 늘리지 마십시오.
- LR59/60은 `StageOnWindow`로 독립 layout root에 workload를 부착하여 HUD의 weighted stack이 측정 횟수에 개입하는 것을 줄입니다. LR61과 LR64의 controller workload는 일반 stage를 사용합니다. sample 구간의 event-side geometry와 시간 측정 밖 `VerifyFinalGeometry`의 current scene geometry는 별도 관측입니다. 큰 workload의 fresh frame timeout도 FAIL이며 event-side 값으로 대신 통과시키지 않습니다.
- LR53의 기존 `FixedRecycler` 직접 호출은 layouter 계산 단위 검증으로 유지합니다. 추가 `window-*` scenario는 실제 `RecyclerView`를 controller에 부착하여 양축, cache 범위, variable extent/decoration, scroll 경계, empty/zero viewport를 검사합니다. LR64의 `variable-recycler` 시간은 직접 layouter 호출 범위이며, 별도 `window-variable-recycler-n16/n128` scenario가 실제 controller와 current geometry를 검사합니다. 두 범위를 합쳐 모두 화면 렌더 시간이라고 설명하지 마십시오.
- LR65는 GLES `Capture`의 별도 subtree render 결과 pixel을 검사합니다. 기본 manager 5종과 geometry를 유지한 color 변경을 다룹니다. OS가 제시한 Window 전체 framebuffer, 다른 Window의 occlusion, source 밖 조상 clipping이나 다른 render task와의 합성을 검증하는 TC는 아닙니다.
- `compare`는 reference 또는 candidate의 geometry/work-budget 실패가 하나라도 있으면 `correctness-failed`, exit 1을 반환하고 timing PASS를 만들지 않습니다. plan은 manifest의 reviewed metric family 전체와 일치해야 합니다. 상세 명령과 결과 JSON 판정은 [도구 계약](layout-validation-tools.md)을 따르십시오.
- 제품 결함의 출처는 결함별로 분류해서 보고하십시오. suite가 드러낸 결함 중 `ScrollView` animation 종료 시 축소된 범위 밖 종점 복원, `PageScrollView` snap 목표 page의 무clamp commit, `RecyclerView` 비중첩 scroll 점프의 gap 항목 잔류는 이 suite보다 앞선 upstream 결함입니다. 부분 마지막 page 선택의 강등은 같은 branch의 제품 수정 commit이 만들고 이어서 고친 것이며, viewport 전용 resize 경로의 도달 가능성도 그 commit이 만든 것입니다. 출처를 분류하지 않으면 회귀와 기존 결함을 구분할 수 없습니다.

아래 표의 행은 suite가 검사하는 계약이며 실패가 남아 있는 것이 정상입니다. 각 행이 어떤 구현 결함을 가리키는지는 행 ID와 함께 기록하십시오.

이전 리뷰에서 확인한 아래 제품 결함은 expected를 낮추거나 검사 행을 삭제하지 않고 유지합니다. 현재 artifact에서 실제로 남아 있는지는 같은 ID의 실행 증거로 판단하십시오. 예상 실패 개수를 맞추는 것은 PASS 기준이 아닙니다.

| TC / 관측 행 | 유지하는 계약과 알려진 증상 |
|---|---|
| LR15 `F14.*` / `shared-rendered-baseline` | baseline 정렬 계약을 검사합니다. 기존 구현의 BASELINE 경로는 glyph baseline 정렬을 수행하지 않아 실패가 보고되었습니다. |
| LR27 `K14.external-standalone-measure` / `before.standalone-slot-consumed` | standalone root 측정 뒤 consumed 상태를 검사합니다. 기존 root pass 뒤 bit가 남아 부모 보정 측정이 반복되는 증상이 보고되었습니다. |
| LR39 `TR14.ancestor-replay-during-enter` | `ancestor-replay.rendered.y`와 `enter-seek-1..3`의 4개 geometry 행을 유지합니다. 조상 arrange replay가 pause된 ENTER 위치를 layout target으로 덮는 증상을 검사합니다. seek progress 전달 실패와 geometry 불일치를 구분하십시오. `enter-seek-1..3`의 3개 행은 매 실행에서 결정적으로 실패하지만 `ancestor-replay.rendered.y`는 replay 직후 frame을 읽어 render thread와 경합하므로 실행에 따라 통과할 수 있습니다. 이 행의 통과를 수정의 증거로 쓰지 말고, 실패 행 수를 3 또는 4로 함께 기록하십시오. |
| LR50 `text-model` / `text.control.height` | 고정 Label frame 높이 120과 text control 높이의 일치를 검사합니다. 기존 text model이 content 기반 높이를 보고하는 4개 행의 실패가 보고되었습니다. actor rect만 맞는다고 이 행을 통과시키지 마십시오. |

LR50/LR64의 동봉 font 경로는 `FONTCONFIG_FILE`을 `res/layout-validation/fonts.conf` 절대 경로로 지정하여 검증합니다. system font로 대체된 실행은 같은 fixture 조건이 아닙니다. LR64 text stage는 실제 reference line height를 얻은 후 충분히 크게 설정하며, 최종 geometry는 다른 controller family와 함께 fresh frame 뒤의 current 값을 읽습니다. LR57 effect 관측은 지속되는 render task를 사용하는 fixture이며, 단일 render 후 해제되는 resource와 혼동하지 마십시오.

## 구성 결정과 검증 계약

| 반드시 만족할 조건 | 적용 방식 |
|---|---|
| 기존 manual-tests 구조 유지 | 같은 foundation 앱에 신규 TC 등록, main 변경 없음 |
| 문제를 재현 가능한 작은 단위로 격리 | core/manager/cache/controller/transition/integration/performance별 TC와 scenario 분리 |
| 기존 Layout TC에 의존하지 않음 | 신규 suite에 default View, custom callback/manager, 4개 manager, standalone, scale/direction, lifecycle 포함 |
| 실제 관측 경로 구분 | controller target, current scene geometry, subtree pixel, 직접 계산 결과를 각각 명시 |
| 구현과 독립적인 expected | 입력 표·합/비율/좌표 식·정해진 상태 전이 및 event 순서 사용 |
| 성능 관측의 간섭 억제 | work-count step과 timing step의 분리, frame 관측·검증·출력은 측정 구간 밖 수행 |
| LLM이 실행 및 검증 가능 | 고정 버튼 이름, TC/scenario/step/action 식별자, actual/expected/tolerance, exact required_checks, timeout 및 전체 증거 |

하나의 대형 TC로 모든 상태를 공유하면 이전 실패가 다음 fixture에 영향을 줍니다. 따라서 TC는 분리하되 공통 검증 helper와 기존 앱을 공유합니다. 신규 executable이나 서버 runner는 추가하지 않습니다.

유한한 TC 집합으로 가능한 모든 Layout 오류의 부재를 증명할 수는 없습니다. 완료 기준은 아래 지원 경로의 명시적 oracle, 실행 증거, source branch와 assertion 연결, positive-control mutation 및 성능 비교입니다.

## 검증 모델: 화면 부착과 렌더 결과

화면 fixture를 사용하는 scenario는 다음 관측을 조합합니다. 직접 layouter 호출, detached/out-of-band 측정 및 event-side 전용 행에는 해당 관측 범위를 별도로 적용하십시오.

1. `Run::Stage(width, height)`는 HUD 아래 host에 고정 크기 stage를 부착합니다. 배경 없는 fixture View에는 구별 가능한 색이 부여됩니다. 독립 layout root는 `Run::AttachToWindow` 또는 `StageOnWindow`를 사용합니다. 화면이나 screenshot은 fixture 부착의 보조 증거이며, 서브픽셀 geometry의 수치 판정을 대신하지 않습니다.
2. `Run::AfterRender(views, action)`는 View와 Window의 layout 완료 signal을 먼저 관측합니다(기본 watchdog 10초). 이후 `AfterFrame()`이 window의 1×1 sentinel actor를 source로 하는 새 `REFRESH_ONCE` render task의 완료를 기다립니다(`FRAME_DEADLINE_MS` 4초). 이 task에는 framebuffer가 없으므로 pixel readback도 GPU sync도 없고, 시도별로 만료되는 내부 timeout도 없습니다. 마지막으로 `IsSettled()`가 current extents와 event-side target의 일치를 확인합니다(16ms polling, 최대 `DEFAULT_SETTLE_MS` 4초). 두 상수는 서로 독립입니다. `IsSettled()` 자체는 geometry 동등성 검사이며 frame identity를 증명하지 않습니다. 새 frame 증거는 앞선 task 완료에서 얻습니다.
3. layout이 바뀌지 않는 color 변경이나 animation seek는 `AfterFrame()`을 직접 사용합니다. optional readiness는 요청 progress 등 입력 전달만 확인하고, expected geometry를 만족할 때까지 기다리는 용도로 사용하지 않습니다. readiness는 task 요청 전과 완료 후 모두 성립해야 하며, 완료 후 성립하지 않으면 새 task를 다시 요청합니다. frame callback은 action epoch, fence sequence, attempt 번호로 구분합니다. Reset/Back은 이전 callback을 차단하고 sentinel task를 window의 task list에서 제거합니다. 비정상 완료는 `render.frame-timeout` 행과 `LR_FRAME` 레코드를 남깁니다([도구 계약](layout-validation-tools.md) 참조).
4. frame fence는 scene-graph가 새 render task를 소비했다는 증거이며 OS Window presentation 완료가 아닙니다. pixel을 읽는 관측은 별도로 `Run::CapturePixels`의 `Capture`이며 현재 dependency에서 GLES가 필요합니다(source subtree를 별도 camera/framebuffer에 렌더한 완료 증거). 큰 workload가 Window 밖으로 이어져도 current scene extents는 읽을 수 있지만, 그 노드의 화면 가시성이나 실제 pixel을 검사했다는 뜻은 아닙니다. frame timeout을 event-side fallback으로 통과시키지 마십시오.
5. 시간 TC의 pass/sample 구간에는 controller 결과를 event-side로 검사합니다. LR59–61 및 LR64의 controller family는 timing 밖의 fresh task 뒤에 current geometry를 별도로 검사합니다. LR64 직접 recycler timing은 controller timing과 구분하고 대응 integration scenario의 결과를 함께 보존하십시오.

| 관측 | 실제 의미 |
|---|---|
| `Rect(View, ...)`, `Bounds`, 일부 state hook | event-side 속성 또는 해당 hook의 계산 상태. 렌더 완료 증거가 아님 |
| `Snapshot` | View LayoutFinished로 받은 parent-local 최종 actor target. post-RTL, transition 적용 전 |
| `GetViewSnapshot().arrangedBounds` | pre-RTL logical rect |
| `Rendered` / `RenderedBounds` | current scene-graph screen extents를 부모 또는 지정 base의 current origin 기준으로 환산한 값. post-RTL. pixel sampling은 아님 |
| `GetMeasuredSize` / producer count | 측정 결과 및 Measure/Arrange callback 실행 횟수 |
| LR65 `*.pixel` | 별도 subtree Capture buffer의 표본점 RGB. 기대색과 채널별 ±3 이내 |

## 기존 Layout TC45–TC94와의 대응

기존 TC의 통과 기준을 라이브러리 코드 경로 기준으로 대조한 결과입니다. 판정에 합산하지 않는 기존 TC는 그대로 두되, 라이브러리 경로가 비어 있던 항목은 신규 scenario로 옮겼습니다.

| 기존 TC | 신규 대응 |
|---|---|
| 45–52 Absolute | LR19 `A01/A02/A07`, LR20 `A04–A08`, LR09 `C15.standalone-replay.4`, LR03 `match-budget` |
| 53–55 custom callback/manager | LR23/LR24/LR27(콜백), LR05 `C19.custom-manager`(앱 정의 LayoutManager와 `InvalidateOwnerMeasure`), LR09 `C15.standalone-replay.0` |
| 56–64 Flex | LR12 `F06/F07/F12/F15/F16/F17`, LR13 `F02.*/F03/F13`, LR14 `F01/F04/F05`, LR15(BASELINE), LR09 `C15.standalone-replay.2` |
| 65–71 Grid | LR16 `G01–G04/G08/G14/G15`, LR17 `G05/G06/G09/G10`, LR18, LR09 `C15.standalone-replay.3` |
| 72–76 Standalone | LR21 `S10.*`, `G14.mode-change` |
| 77 PARK | LR29 |
| 78–82 Transition | LR36, LR38, LR39, LR40, LR41, LR42, LR44; reorder는 LR37의 REORDERED cause까지(TC 내부 drag/proxy/auto-scroll/SVG는 라이브러리 layout 범위 밖) |
| 83–88 Stack | LR10 `S01/S04/S05`, LR11 `S06/S07`, LR09 `C15.LTR-RTL-LTR`·`C15.standalone-replay.1` |
| 89–94 View | LR03 `match-budget/wrap-bounds/wrap-match/C11`, LR01 `C01.padded`, LR45 `CT01`, LR09 `explicit-child`·`C15.standalone-replay.0` |

대응이 없는 것은 TC47의 add-order Z-order(layout 계산이 아닌 actor 그리기 순서)와 TC80/82의 gesture 구현부뿐입니다. 창 크기 변경에 따른 manager별 재계산은 root 제약 변경 경로(LR45 `CT01.real-window-resize`, LR37 WINDOW_RESIZED)와 stage 크기 변경 step이 같은 코드를 지나므로 별도 scenario를 두지 않았습니다.

## 지원 경로와 신규 TC 매핑

| 경로 / 실제 source 기준 | 신규 TC | 주 관측 / oracle |
|---|---|---|
| `internal/views/view/view-data-impl.cpp`: Normalize, requested size/position, min/max, margin/padding, Measure/Arrange dispatch, 앱 정의 LayoutManager(`InvalidateOwnerMeasure`) | LR01–07 | 제한값 표, callback 입력·반환, exact/epsilon 경계, 렌더 사각형 |
| effective scale, global scale, inherited LTR/RTL | LR08–09 | scale별 시각 단위, parent-local mirror 식(logical vs 렌더) |
| `public-api/layouts/stack-layout-manager.cpp` | LR10–11 | 고정/weight 잔여 공간, spacing, cross alignment(FILL과 고정 크기), WRAP/clamp |
| `public-api/layouts/flex-layout-manager.cpp` | LR12–15 | basis/grow/shrink, line 분할과 wrap measure, direction/reverse/wrap/RTL, justify/align, padding/margin, 실제 glyph baseline |
| `public-api/layouts/grid-layout-manager.cpp` | LR16–18 | absolute/auto/star track과 wrap measure, span과 auto deficit, clamp/empty/boundary |
| `public-api/layouts/absolute-layout-manager.cpp` | LR19–20 | 고정/비율 축별 독립 식, 음수 위치, wrap, RTL |
| standalone / descendant participation | LR21 | parent 계산 제외와 subtree 내부 계산의 분리 |
| Measure cache, Arrange replay/policy | LR22–24 | 형제 pass를 통한 producer count, key 경계, 손상된 Actor geometry 복원 |
| generation / dependency owner / out-of-band | LR25–27 | dirty/cache state, ancestor stop 이유, owner stack 정리, out-of-band 호출 뒤 복원 |
| View completion, PARK, reentry/exception, replay 중 tree 변경 | LR28–31 | 동일 episode 신호·순서, quiet interval, poison/rollback |
| tree order/reparent/off-scene/Window cleanup | LR32–34 | 각 child target과 새로운 parent 기준값, lifetime 정리 |
| `internal/layouts/layout-transition-dispatcher.cpp` 및 public transition API | LR35–44 | descriptor, scope/cause, bounds/clip, 실제 animation seek·manual tick·natural completion, interruption/exit/focus/lifecycle |
| `public-api/layouts/layout-controller.cpp` | LR45–49 | root constraint(실제 window 크기), batching, View→Window 완료, lifetime, exception rollback |
| 실제 Label/ImageView resource 및 중첩 manager | LR50–51 | 고정 font/image, resource fence, A→B→C→A 전체 geometry(snapshot + 렌더) |
| ScrollableBase / ScrollView | LR52 | viewport/content extent, offset/clamp, 실제 scene integration |
| `internal/linear-items-layouter-impl.cpp` | LR53 | 직접 layouter 계산과 실제 RecyclerView/controller의 visible/cache range, decorated extent, current geometry |
| `internal/group-linear-items-layouter-impl.cpp`, GroupAdapter | LR54 | flatten type/group/position, body/header/gap margin |
| 실제 RecyclerView / ItemAdapter | LR55 | stable ID, insert/move/remove/content/size/replacement, Window viewport |
| input coordinate/clip 및 RenderEffect resize | LR56–57 | 실제 touch hit/local position, RenderTask framebuffer texture 크기 |
| seed / metamorphic validation | LR58 | 고정 seed와 독립 arithmetic, 축 교환·RTL·translation |
| construction/first pass/warm pass/idle pass | LR59 | 매 표본 전체 geometry + Release raw time, work-budget step |
| sparse/dense/same-value 갱신 | LR60 | 매 표본 전체 geometry + Release raw time, work-budget step |
| 반복 pass/깊이/손상 복원 | LR61 | 매 cycle 전체 노드, 깊이별 닫힌 식, 최종 렌더 집계 |
| diagnostics API 자체의 계약 | LR62 | buffer guard/overflow/epoch/ID, snapshot 반복 읽기의 비변이 |
| 복합 tree/reflow workload | LR64 | deep/balanced, Flex wrap/grow/shrink, Grid star/span, variable Recycler, text reflow |
| transition work/time | LR63 | absent/dormant/disabled/active spec/animator의 count 및 Release time |
| subtree 렌더 픽셀 | LR65 | manager별 solid box 및 동일 geometry color 변경의 pixel과 current 사각형 |

조건별 source와 실제 assertion 연결은 [core map](layout-validation-core-map.md), [transition/controller map](layout-validation-transition-map.md), [workload/diagnostics map](layout-validation-workload-map.md)에 기록합니다. 이 표는 source 경로와 TC 책임의 매핑이며 모든 branch가 실제 실행되었다는 증거표가 아닙니다.

## 실행 조건과 build

layout 관측 hook은 library와 manual-tests app 모두에 **항상** 컴파일됩니다. 켜고 끄는 build 옵션은 없습니다. event 기록은 `BeginCapture()`로 활성화되며, transition owner 관측은 등록 node가 있거나 capture가 열린 동안 활성화됩니다. 등록 전에 시작한 transition의 historical geometry/timing 조회 계약은 별도로 유지됩니다. 따라서 모든 TC의 계약은 단 하나이고, `reviewed_contracts`의 `diagnostic`/`release` 두 profile key는 내용이 동일합니다. 남은 구분은 build type뿐입니다.

- **build type**: 시간 표본(LR59/60/63/64)은 Release 빌드에서만 판정합니다. Debug 빌드에서는 `DEBUG_ENABLED`가 정의되어 성능 step이 의도적으로 `profile.release` 실패를 냅니다. `Run::Sample`이 출력하는 `profile` 필드도 Debug에서 `debug`, 그 외에는 `release`입니다.
- 계약표에 없는 scenario를 실행하거나 판정하지 마십시오. `cases[]`는 TC id와 소스 stem만 나열하며, 어느 TC가 Release 빌드를 요구하는지는 각 TC의 MD가 직접 기술합니다.
- `ldd`/process maps의 실제 로딩 경로와 ELF build-ID를 저장하여 다른 library를 잘못 검증하지 않도록 하십시오.

manual-tests CMake는 설치된 `dali2-core`, `dali2-adaptor`, `dali2-ui-foundation`, `dali2-ui-components`를 pkg-config로 요구합니다(`PKG_CONFIG_PATH`를 설치 prefix로 지정). 표준 절차는 library를 설치한 뒤 manual-tests를 구성하는 것입니다. integration-api 헤더(`layout-test-diagnostics.h` 포함)는 디렉터리 단위로 설치되며, 그 선언에 대응하는 symbol은 모든 library 빌드에 존재합니다.

```sh
# 1) library: 설치 prefix에 설치
cmake -S "$SOURCE/build/tizen" -B "$WORK/lib-release" -DCMAKE_INSTALL_PREFIX="$DESKTOP_PREFIX" \
  -DCMAKE_BUILD_TYPE=Release -DENABLE_DEBUG=OFF -DENABLE_PKG_CONFIGURE=ON
cmake --build "$WORK/lib-release" --target dali2-ui-foundation -j4 && cmake --install "$WORK/lib-release"
# 2) app
cmake -S "$SOURCE/manual-tests" -B "$WORK/app-release" -DCMAKE_BUILD_TYPE=Release
cmake --build "$WORK/app-release" --target manual-test-dali-ui-foundation -j4
```

manual-tests의 실행 파일은 어느 build tree에서 빌드하든 source의 `manual-tests/dali-ui-foundation/bin/`에 쓰이므로, 여러 구성을 만들 때는 순서대로 빌드하고 각 바이너리를 따로 보관하십시오. app을 Debug로 구성하면 TC 소스에 `DEBUG_ENABLED`가 정의되어 성능 step이 `profile.release` 실패를 내므로 시간 측정 app은 Release로 구성하십시오. library를 `CMAKE_BUILD_TYPE=Debug`로 구성하면 `ENABLE_DEBUG`가 켜져 Release core에 없는 Log::Filter symbol을 참조할 수 있으므로 Release/RelWithDebInfo를 사용하십시오. Debug library에서는 `DALI_ASSERT_DEBUG`가 DaliException을 던지므로, 무효 입력을 일부러 주입하는 scenario(LR02 `R03.returned-rect`, LR35, LR49 등)는 주입을 step 안의 **동기** `ProcessLayouts()`/setter 호출로만 수행하고 그 호출을 try/catch로 감싼 뒤 다음 pass 전에 주입을 제거합니다. controller의 비동기 pass에서 예외가 나면 앱이 abort되므로 그런 abort는 TC 결함으로 취급하십시오. `nm -D --defined-only -C libdali2-ui-foundation.so | grep LayoutTest`로 `Dali::Ui::Integration::LayoutTestDiagnostics`의 12개 함수가 모두 export되어 있는지 확인하십시오. 같은 명령의 출력에 `LayoutTransitionDispatcher::GetLayoutTest*`, `SetLayoutTestManualTicks`, `TickLayoutTestAnimators`가 나타나면 안 됩니다. 이들은 설치되지 않는 internal header의 멤버로 hidden visibility를 유지해야 합니다.

## stdout 레코드 문법

앱은 아래 레코드를 stdout에 한 줄씩 출력합니다. JSON 본문은 `LR_READY`와 `LR_EXTERNAL_REQUIRED`를 제외하고 모두 단일 JSON object입니다. 숫자 형식은 항상 `.` 소수점(LC_NUMERIC과 무관)입니다.

| 레코드 | 시점 | 필드 |
|---|---|---|
| `LR_CASE_CONTRACT` | TC 진입/`Reset run` | `tc, pid, run, scenarios[{id, steps[{id, required_checks}]}]` — manifest와 exact 일치해야 함 |
| `LR_ACTION` | step 시작(`Run all`/`Run scenario`/`Run step`) | `tc, pid, run, scenario, step, action_seq, required_checks` |
| `LR_READY` | action 완료 | `tc= pid= run= scenario= step= action_seq= checks= failures=` (key=value) |
| `LR_RESULT` | step 완료 직후(자동), scenario 이동, `Reset run` | `tc, pid, run, scenario, scenario_index, scenario_count, step, step_index, step_count, completed_steps, completed_scenarios, required_scenarios, checks, failures, verdict(PASS/FAIL/EXTERNAL_REQUIRED/FAILING/INCOMPLETE/PENDING), rows` — `PASS`/`FAIL`/`EXTERNAL_REQUIRED`는 **종결** verdict이며 `completed_scenarios == required_scenarios`일 때만 출력됩니다. 완료 전의 상태는 실패가 있으면 `FAILING`, 없으면 `INCOMPLETE`이고, step 실행 중에는 `PENDING`입니다. `FAILING`/`INCOMPLETE`/`PENDING`을 종결 판정으로 읽지 마십시오 |
| `LR_CHECK` | `LR_RESULT` 직후 해당 step의 전체 행(자동) | `tc, pid, run, scenario, row, step, action_seq, kind(numeric/integer/text/color/failure), id, actual, expected, tolerance, passed` |
| `LR_SAMPLE` | 성능 표본 | `tc, pid, run, scenario, step, action_seq, profile(release 또는 debug), metric, iteration, nanoseconds, operations` |
| `LR_EXTERNAL_REQUIRED` | 외부 검증이 필요한 action | 사유 문자열 |
| `LR_NOOP` | 순서에 맞지 않는 버튼 입력 | `tc, pid, run, button, reason` — 버튼이 무시되었음을 뜻하며 검사 행을 만들지 않음 |
| `LR_CLEANUP_ERROR` | scenario cleanup 예외 | `tc, pid, run, scenario` (stdout과 stderr 모두) |
| `LR_CASE_END` | `< Back`/`Reset run` | `tc, pid, run, completed_scenarios, required_scenarios, completed_steps, failures, cleanup_passed, reason(exit/reset)` — `completed_scenarios == required_scenarios`인지로 그 run이 완료되었는지 판정합니다 |

결과 행은 `(tc, pid, run, scenario, action_seq, row)`로 식별합니다. 같은 행의 재출력은 추가 검사가 아닙니다.

## LLM 실행 절차

1. `python3 layout-validation-tools.py audit`으로 CPP/MD 쌍, 고정 asset hash, MD 계약 합계, 예시 plan의 manifest pin과 reviewed metric family 전체 일치를 검증하십시오. 이 명령은 앱을 실행하지 않습니다.
2. 환경: `LC_ALL=C`(또는 최소 `LC_NUMERIC=C`)로 앱을 실행하고, `FONTCONFIG_FILE`을 실행 source의 `res/layout-validation/fonts.conf` 절대 경로로 설정하십시오. `fc-match`의 실제 font 파일과 hash, DPI/scale/해상도/locale, window 크기(480×800 이상)를 기록하십시오.
3. 앱 stdout/stderr를 파일로 보존하고 launcher에서 TC ID로 검색하십시오. TC 진입 시 run이 자동으로 시작되고(HUD에 `scenario k/N`, `steps i/M` 표시), `Run all`은 남은 scenario 전체를, `Run scenario`는 현재 scenario의 남은 step을, `Run step`은 다음 step 하나만 실행합니다. 각 step 완료 시 `LR_READY`, `LR_RESULT`, 그 step의 모든 `LR_CHECK`가 자동 출력됩니다. 버튼 표시는 공통이고 Actor/accessibility name에는 `LRxx.` 접두사(`LR01.Run all` 등)가 있습니다. quiet/시간 조건이 있는 TC(LR29)는 MD가 지시하는 대로 `Run step`으로 진행하십시오.
4. `LR_READY`의 tc/pid/run/scenario/step/action_seq가 현재 step과 일치할 때까지 stdout을 수동적으로 관측하십시오. 이 표시는 step 완료 통지이며 PASS 판정 자체가 아닙니다. timeout은 TC별 MD를 따르십시오. 공통 경로는 layout fence 10초, fresh task 4초, geometry settle 4초의 단계별 제한이며, pixel Capture에는 별도 4초 제한이 있습니다. 완료 직후 그 step의 `LR_RESULT`와 **모든 `LR_CHECK` 행이 자동으로 stdout에 출력**되며 HUD에는 요약과 실패 행만 표시됩니다. 누락된 행을 통과로 추정하지 마십시오. `LR_RESULT`의 verdict는 `completed_scenarios == required_scenarios`가 되기 전에는 종결 판정이 아니므로, `FAILING`/`INCOMPLETE`를 보고 실행을 중단하지 마십시오.
5. `LR_NOOP`가 출력되면 그 버튼은 무시된 것입니다(예: 이전 step이 아직 실행 중인데 `Run step`을 누름, 자동 실행 중 `Next scenario`를 누름). 사유를 확인하고 절차를 이어가되 검사 수에 포함하지 마십시오.
6. 렌더 행(`Rendered`), controller snapshot 행, measured 행을 구별하십시오. 렌더 행은 부모 기준 화면 사각형(RTL 적용 후)이고 snapshot 행은 controller가 완료 signal로 보고한 parent-local 최종 bounds(역시 RTL 적용 후)이며, pre-RTL logical rect는 관측 hook의 `arrangedBounds` 행입니다. animation current property는 별도의 settle/seek 관측입니다. screenshot만으로 서브픽셀 geometry를 판정하지 마십시오. screenshot은 stage 안에 fixture가 그려졌는지의 보조 증거로만 사용하십시오.
7. 일반 async timeout은 각 TC MD를 따르십시오. PARK의 10초 quiet interval에는 버튼 입력·timer polling·스크린 조작을 하지 마십시오. 수집 시스템이 stdout을 수동적으로 기록하는 것은 가능합니다.
8. 같은 행의 actual/expected/tolerance를 다시 계산하고 required_checks와 실제 행 수의 정확일치, 완료 scenario 수, 누적 실패 수를 확인하십시오.
9. 앱이 EXTERNAL_REQUIRED를 내면 external 성능/quiet/artifact 조건이 미완료입니다. missing baseline, observer overflow, timeout은 PASS가 아닙니다.
10. `python3 layout-validation-tools.py check-log LOG --tc LRxx --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`으로 저장된 수치를 다시 검증할 수 있습니다(`--profile`의 두 값은 내용이 동일한 같은 계약을 가리킵니다). 성능 등의 internal evidence에 한해 `--external`을 추가할 수 있으며 이 옵션은 external 판정을 완료하지 않습니다. 결과 `status`는 세 가지입니다: `observations-valid`(증거가 유효하고 실패 행 없음, exit 0), `observations-valid-with-failures`(증거는 유효하며 실패 행을 `failing_rows`에 TC별로 나열, exit 1), `invalid-evidence`(증거 자체를 검증할 수 없음 — 누락 scenario/행, 중복 id, 합계 불일치, cleanup 예외, 잘린 레코드 등, exit 2). 실패 행의 존재와 증거의 무효를 구분하십시오: 알려진 제품 결함은 `observations-valid-with-failures`이고 `failing_rows`가 그 행과 정확히 일치해야 하며, `invalid-evidence`는 재실행 대상입니다. layout/frame timeout으로 필수 관측이 누락되거나 settle 실패 행이 추가되어 exact 행 수가 깨지면 `invalid-evidence`가 됩니다. 원본 `observer.timeout`, `render.frame-timeout`, `render.settle-timeout` 및 `protocol.required_checks` 행, frame fence가 남긴 `LR_FRAME` 레코드(도구 결과의 `frame_failures`)와 도구의 `error`를 보존하고 관측 실패로 보고하십시오. 이것은 timeout을 무시하는 예외가 아니며 **어떤 경우에도 PASS로 처리하지 않습니다**. 증거를 맞추려고 watchdog이나 계약을 바꾸지 마십시오.
11. TC에서 Back으로 나온 뒤 해당 run의 `LR_CASE_END`를 보존하십시오. `cleanup_passed=true`이고 `completed_scenarios == required_scenarios`이며 모든 count(failures 포함)가 마지막 `LR_RESULT`와 일치해야 합니다. 알려진 제품 결함 TC는 failures>0으로 종결되며, 그 수가 실패 행 수와 같아야 합니다. END 누락·cleanup 예외는 PASS가 아닙니다. 재진입하여 초기 상태를 검증하십시오.

`PIN`은 이번 candidate 로그에서 계산하는 값이 아닙니다. 운영자가 별도로 검토한 manifest 파일의 SHA256을 신뢰 저장소에 사전 고정하십시오. `reviewed_contracts.profiles`에는 TC별 전체 scenario/step/required_checks가 있고, 도구는 app 출력과 exact equality를 요구합니다. 두 profile key(`diagnostic`/`release`)는 내용이 동일하므로 어느 값으로 검사해도 같은 결과입니다. 계약을 바꾸려면 source/MD 변경을 검토한 뒤 새 pin을 배포하십시오. 예시 plan 파일의 `contract_manifest_sha256`은 저장소의 manifest에 맞춰져 있으며 `audit`이 이를 검사합니다.

## 독립 oracle, coverage, mutation

- golden expected를 candidate 출력에서 만들지 마십시오. CPP의 arithmetic fixture와 대응 MD의 표/식을 기준으로 판단하십시오. 기존 Layout TC의 PASS를 신규 scenario의 증거로 합산하지 마십시오.
- 새로 추가되는 Layout API/분기는 source→fixture input→action→actual observer→expected 식→assertion→실행 증거로 연결하여 manifest와 TC를 함께 갱신하십시오. 계약(manifest·MD 표)은 source의 독립 expected와 필수 검사 범위를 검토한 뒤 갱신하고, 컴파일된 앱의 `BuildScenarios()` 출력과 exact 대조합니다. candidate 출력에 맞춰 계약을 자동 승인하거나 pin을 바꾸지 마십시오.
- coverage는 coverage 전용 artifact에서 별도 수집하십시오. launcher/HUD가 우연히 실행한 줄은 해당 TC의 oracle이 검증한 분기로 세지 마십시오.
- positive control은 격리된 복사본에서 한 변이씩 적용하십시오. 예: Stack spacing 항 제거→LR10/LR58, Flex grow 비율 변경→LR12, Grid star 비율 변경→LR16, Absolute `(available-child)*p`를 `available*p`로 변경→LR20, Measure key 비교 제거→LR22, replay geometry write 생략→LR23, RTL mirror 생략→LR09/LR14, generation/owner stop 변경→LR25/26, transition progress 적용 생략→LR39/41, recycler spacing 제거→LR53, resource invalidation 누락→LR50.
- 각 변이는 원본의 **같은 assertion이 PASS**하고 변이에서 **예정한 actual 불일치로 FAIL**해야 kill입니다. compile failure·환경 crash·이미 원본에서 실패하던 baseline assertion(위 알려진 제품 결함 포함)은 mutation kill로 세지 마십시오.
- LR58의 seed 실패는 seed와 실현된 입력 node 목록을 보존하십시오. `shrink-candidates`는 한 node를 삭제한 후보 JSON을 만들 뿐이며, 후보를 TC에서 재현하여 동일 assertion 실패가 유지되는지 확인한 뒤에만 축소된 재현이라고 기록하십시오.

## 성능과 관측자 비용

LR59/60은 controller pass 단위로 시간 종류를 분리하고 LR63은 transition 상태별 work/time을 분리합니다. reference/candidate의 같은 workload 계약과 매 표본 geometry 검증이 선행되어야 합니다. 여러 manager/N/phase를 합쳐 회귀를 숨기지 마십시오.

[성능 계획 예시](layout-validation-performance-plan.example.json)는 reviewed `compute_update` family의 57개 metric(LR59 48개 + LR60 9개)을 선택합니다. LR59 phase는 `construction`, `first_pass`, `warm_pass`, `idle_pass`이며 이전 measure/arrange 분리 metric과 같은 baseline으로 비교할 수 없습니다. 실행 전에 device의 timer resolution을 측정하여 timer_floor_ns, precision, MDE, family, seed를 고정하십시오(예시 값 1000ns는 N=16 단일 pass 표본이 floor 아래로 떨어지지 않도록 낮춘 값이며 device에 맞게 조정). 독립 process pair 31/62/93, AB/BA 균형, family 및 3회 관측 보정 CI를 사용합니다. [LR63 계획 예시](layout-validation-transition-performance-plan.example.json)의 15개 mode/N/phase와 [LR64 계획 예시](layout-validation-complex-performance-plan.example.json)의 16개 family/N도 제공합니다. 같은 process의 31개 내부 sample은 독립 process pair가 아닙니다.

`compare`의 plan에는 `metric_family`, 전체 `metrics`, 신뢰 저장소의 `contract_manifest` 경로와 `contract_manifest_sha256`도 고정하십시오. family에서 phase나 TC를 빼면 invalid evidence입니다. reference 또는 candidate의 유효한 geometry/work-budget 실패가 있으면 모든 pair의 evidence 구조를 검사한 뒤 `correctness-failed`(exit 1)를 반환하며, `correctness_failures`에 pair/side/TC별 실패를 보존하고 timing `results`는 비웁니다. 뒤의 pair라도 malformed이면 exit 2가 우선합니다. 같거나 더 빠른 timing으로 정확성 실패를 상쇄할 수 없습니다. `compare`의 EXPERIMENT JSON에는 `plan_sha256`와 `blocks`를 넣으십시오. 각 block은 `pair_id`, `order`(AB/BA), `reference_log`, `candidate_log`, 동일한 `environment_reference`/`environment_candidate`, 두 artifact fingerprint를 포함합니다. 판정: `ci_ratio` 하한>1이면 작은 slowdown도 FAIL, 상한≤1+MDE 및 precision을 만족할 때만 해당 검출 해상도 내 regression 미검출, bounded sample 후에도 불확실하면 INDETERMINATE입니다. 검정 결과를 보고 MDE를 늘리지 마십시오.

알려진 한계와 관측 실패 판정:

1. 저장 baseline·절대 예산 모드는 없으며 시간 판정은 독립 reference/candidate process pair 실험으로 수행합니다. 같은 process의 내부 sample을 독립 pair로 세지 마십시오.
2. LR64는 geometry를 검사하지만 work-count budget을 계수하지 않습니다. 직접 recycler timing과 실제 RecyclerView integration action도 별도 범위입니다.
3. LR63 active transition 시간에는 진행 중 transition의 영향이 있으므로 work-count와 함께 판정하십시오. 성능 로그 전체와 측정 구간 밖 관측 결과를 보존해야 합니다.
4. layout/frame timeout은 continuation을 실행하지 못해 필수 행이 부족할 수 있습니다. settle timeout은 실패 행을 추가한 뒤 continuation을 실행하므로 행 수가 초과할 수 있습니다. `check-log`는 두 경우 모두 exact 계약을 유지하여 `invalid-evidence`로 판정합니다. 최초 원인 행과 도구 오류를 함께 기록하고 PASS로 바꾸지 마십시오. LR65 pixel capture 실패는 각 선언 probe에 실패 행을 기록합니다.
5. frame 완료는 sentinel actor를 source로 한 새 render task의 완료이며 OS Window 합성 결과가 아닙니다. 그 task가 그린 것이 아니라 task가 생성된 시점 이후의 update/render 한 쌍이 증거입니다. offscreen 노드의 current geometry 관측도 그 노드의 pixel 가시성을 입증하지 않습니다.
6. `Run::IsSettled`만으로는 동일 geometry의 색·text·clip 변경을 판별할 수 없습니다. fresh task 경로를 유지하고, 실제 색 검증은 LR65 `P02.same-geometry-color`의 pixel 행을 사용하십시오.
7. LR59 `work-first_pass`의 capture 전 HUD drain 범위와 observer 간섭은 실제 로그와 별도 work-budget 검증으로 확인해야 합니다. capture를 열지 않았다는 사실만으로 모든 관측 비용이 없다고 결론 내리지 마십시오.
8. 종료 후 `Append`가 failure count만 증가시키면 실패 행 합계와 다르므로 `invalid-evidence`입니다. 종료·cleanup·재실행의 identity를 함께 보존하십시오.

항상 켜진 hook에 대해서는 아래를 별도로 확인하십시오.

- 원본/변경본의 exported dynamic symbol 집합과 ViewImpl/ViewDataImpl 등 ABI 크기, 관련 function disassembly를 같은 compiler/flags에서 비교하십시오. `Dali::Ui::Integration::LayoutTestDiagnostics`의 12개 함수는 두 artifact 모두에서 export되어야 하며, internal header의 dispatcher 멤버는 어느 쪽에서도 export되지 않아야 합니다.
- capture용 고정 POD buffer는 capture 과정에서 동적 allocation을 하지 않도록 설계했습니다. node registry는 첫 `RegisterNode` 성공 시에만 할당되므로 등록이 없는 실행에서는 BSS 비용이 포인터 하나입니다. `SNAPSHOT_ALLOCATION`은 `StorageSite` enum에 명시한 vector 생성/reserve 지점의 element count/requested bytes이며 process 전체 malloc/RSS가 아닙니다. capture는 등록 node의 event만 계수하고 stage·launcher view의 event는 무시합니다.
- event 기록 hook의 비활성 분기와 transition ownership 관측 분기를 구분하여 같은 Release workload로 비용을 비교하십시오. transition entry는 전체 public `TransitionSnapshot` 대신 재구성할 수 없는 historical 값만 저장하지만 추가 비용이 0이라는 뜻은 아닙니다. 같은 compiler/flags에서 entry 크기와 transition 생성·교체·tick 비용을 비교하십시오. 분기 전에 불필요한 관측 인자를 평가하지 않는지도 확인하십시오.

## 실행 보고서

보고서에는 fixture/source/asset fingerprint, build type, 전체 scenario/action 목록, 실패와 미실행 구분, 최초 불일치 수치, coverage attribution, mutation kill/미검증 목록, 성능 CI/정밀도/결측값을 남기십시오. 새 suite가 현재 제품 오류를 발견하면 expected를 낮추거나 xfail로 숨기지 마십시오. product fix는 별도 변경으로 추적하고 동일 신규 TC를 다시 실행하십시오.
