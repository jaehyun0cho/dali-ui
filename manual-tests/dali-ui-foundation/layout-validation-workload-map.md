# Layout diagnostics·workload 검증 연결표

이 문서는 신규 suite의 diagnostics bridge와 LR52·LR59·LR60·LR62·LR64를 중심으로 **source decision → scenario/action → 실제 assertion ID → 독립 기대값**을 연결합니다. LR15·LR25–27·LR61·LR63은 해당 관측값을 소비하는 인접 신규 TC로만 함께 표시합니다. 기존 Layout TC 실행에 의존하지 않습니다.

이는 소스와 선언된 검사 계약을 대조한 표입니다. 아래 행은 실행·통과·branch coverage 달성의 증거가 아닙니다. 실행 증거는 동일 artifact의 `run_id/scenario_id/action_seq`, 원문 CHECK/RESULT, 완료 fence, 검토·고정한 contract fingerprint로 별도 연결하십시오. gcov branch가 실행되었다는 사실만으로 해당 결정이 독립 assertion으로 검증되었다고 기록하지 마십시오.

## 표기와 범위

- `F/`는 저장소의 `dali-ui-foundation/`, `T/`는 이 문서 옆 `tc/`입니다. `파일:행`은 작성 당시 탐색 기준점이며 변경 후에는 함수·predicate 이름으로 재확인하십시오.
- `Rect(prefix)`는 실제 ID `prefix.x/.y/.width/.height` 네 행, `Size(prefix)`는 `.width/.height` 두 행입니다. 구현은 `T/layout-validation-support.cpp:201`입니다.
- geometry 기본 tolerance=0.001, integer/bool/count/epoch/ID는 exact입니다. `ImmediateWorkResult::Check`는 `.values`를 실제 비교 scalar 수와 대조하고 `.mismatches=0`, `.maximum_error=0`을 검사합니다. tolerance 밖 또는 nonfinite scalar는 mismatch에 포함됩니다. 마지막 sample의 개별 geometry 행도 유지됩니다.
- named storage는 계측한 reserve/vector 생성 접점의 요청 element/byte 수입니다. `SNAPSHOT_ALLOCATION`이라는 event 이름이 process 전체 malloc 수, allocator 성공 횟수 전체, capacity 변화 전체를 의미하지 않습니다.
- `RequireExternalVerification`이 있는 action의 로컬 CHECK 통과는 전체 performance PASS가 아닙니다. 일치하는 ON work 검사·OFF timing·고정 reference·환경·반복 raw sample 증거가 필요합니다.
- timing 대상은 Release, diagnostics OFF, coverage OFF입니다. ON/coverage/profiler 실행 시간은 Release baseline에 합치지 않습니다.

## 현재 선언된 계약 크기

다음 diagnostic 수치는 최신 앱 metadata와 TC 소스의 scenario/action/required_checks 선언을 대조한 값입니다. metadata는 실행 결과가 아닙니다. Release 값은 아래 source 식으로 검산합니다.

| TC | profile | scenario / action / required checks | 검사 계약 |
|---|---|---|---|
| LR52 | OFF/ON 공통 | 40 / 99 / 2135 | 기존661 + Notify 경계/no-op1104 + pre-layout370 |
| LR59 | ON | 12 / 48 / 43856 | manager4×N{16,128,512}; action마다39+4N |
| LR59 | OFF | 12 / 60 / 65500 | action마다217+4N; construction 포함5 phase |
| LR60 | ON | 3 / 9 / 8250 | N{16,128,512}, sparse/dense/same-value; action마다42+4N |
| LR60 | OFF | 3 / 9 / 9825 | action마다217+4N |
| LR62 | ON | 8 / 9 / 137 | registry/capture/readers/manual tick |
| LR62 | OFF | 1 / 1 / 1 | compiled-capability 문자열과 별도 OFF binary 증거 요구 |
| LR64 | ON | 16 / 18 / 6730 | 8 family×N{16,128}, text font-identity2 action 포함 |
| LR64 | OFF | 16 / 16 / 7216 | 124+D; D는 family별 개별 geometry 행 수 |

## Diagnostics bridge 결정과 검사

| source function / decision | 신규 scenario → action | 실제 assertion ID / 독립 불변조건 | 관측과 범위 |
|---|---|---|---|
| `F/internal/views/view/view-data-impl.cpp:9061` `RegisterNode`: empty/ID0/duplicate ID 거부, 같은 handle 재등록, ID 교체 | LR62 `D01.registry → run` | `id.empty-rejected`, `id.zero-rejected`, `id.duplicate-rejected`, `id.same-object-idempotent`, `id.a=101`, `id.replaced-value=102`, `id.released-value-reusable` | snapshot nodeId와 명시 입력101/102 대조; ID를 object 주소에서 계산하지 않음 |
| 같은 함수의 active capture guard; `ClearRegisteredNodes` active/inactive 분기 | LR62 `D01.registry`, `D03.active-registry-clear → run` | `id.active-change-rejected`, `id.rejection-preserves-capture`, `id.rejection-preserves-id`; `active-clear.error-visible`, `.no-invented-events`, `.id-preserved=6208`, `.end-preserves-error`, `.inactive-clear=0` | active clear는 overflow로 오류를 드러내면서 ID를 보존; 새 event를 만들어 정상 처리한 척하지 않음 |
| `UnregisterNode` 파괴 시 해제; inactive clear | LR62 `D01.registry → run` | `id.destroyed-registration-released`, `id.clear-a/b/replacement=0`, `id.ended` | temporary handle 수명을 끝낸 다음 같은 ID를 새 handle에 부여 |
| `BeginCapture` active/null/zero capacity/zero epoch 거부, 새 epoch 초기화 | LR62 `D02.capture-validation → run` | `capture.null-rejected`, `.zero-capacity-rejected`, `.zero-epoch-rejected`, `.invalid-remains-inactive`, `.nested-rejected`, `.nested-state-preserved`, `.nested-buffer-untouched=0xAABBCCDD` | 두 개의 독립 caller buffer; nested 실패가 기존 buffer·capacity·epoch를 바꾸면 FAIL |
| `Record` inactive return / capacity full / 정상 append (`view-data-impl.cpp:8932`) | LR62 `D03.overflow-reset → run` | `overflow.flag`, `.canaries`, `.capacity=2`, `.stored-count=2`, `.total-not-truncated`, `.sequence-first=1`, `.sequence-second=2`, `.epoch-first/second=6203` | 앞뒤 canary와 붙어 있는2 event buffer; totalEvents는 stored count보다 커야 함 |
| `EndCapture`, 새 `BeginCapture`, 빈 capture | LR62 `D03.overflow-reset → run` | `overflow.end-stops-recording`, `.end-idempotent`; `reset.count=0`, `.total=0`, `.epoch=6204`, `.overflow-cleared`, `.sequence=1`, `.event-epoch=6204`, `.empty-status`, `.original-canaries` | End 뒤 layout 호출에도 저장된 status가 변하지 않는지 직접 비교 |
| reader의 invalid handle/no registered Window 반환 (`view-data-impl.cpp:9134` 이후) | LR62 `D04.invalid-readers → run` | `invalid.view/window/text/transition/animation`, `.detached-transition`, `.detached-animation`, `.tick`, `.manual-mode`, `.capture-state`, `.events=0` | empty View/Window/Label, detached View 입력; controller를 생성하는 getter로 대체하지 않음 |
| `ViewDataImpl::GetLayoutTestSnapshot`: cache-valid 분기에 따른 key/propagation union 읽기, progress/poison/scale/owner 복사 | LR62 `D05.readonly-snapshots → run` | `readonly.view-fields`, `.text-view-fields`, `.leaf-geometry`, `.label-geometry`, `.after-end-fields` | 완료된 Absolute fixture에서 capture 중1000회, End 후1000회 반복; `SameView`의 모든 열거 필드와 두 Rect를 exact 비교 |
| `LayoutControllerImpl::GetLayoutTestSnapshot` (`layout-controller.cpp:445`), registered Window lookup | LR62 `D05.readonly-snapshots → run` | `readonly.window-fields`, `.valid-fixture`, `.capture-empty`, `.after-end-capture` | pending roots/completions, processDepth, generation, wake 및 transition count가 read로 변하면 FAIL; 별도 controller 수명 분기는 LR34/47–49 |
| `LayoutTransitionDispatcher::GetLayoutTestSnapshot/GetLayoutTestAnimation` (`layout-transition-dispatcher.cpp:2815` 이후) | LR62 `D05.readonly-snapshots → run` | `readonly.transition-fields`, `.animation-absent` | LR62는 정지 상태의 반복 read를 검사; active Spec/EXIT/Animator 분기의 값 자체는 LR35–44의 개별 TC가 소비하며 이 행만으로 active 분기 검증을 주장하지 않음 |
| `LabelImpl::GetLayoutTestTextSnapshot` (`label-impl.cpp:4484`): 완료 model의 line/glyph/position/offset const 읽기 | LR62 `D05.readonly-snapshots → run` | `readonly.text-ready`, `.text-fields`, `.capture-empty` | `Text::View::GetGlyphs`를 호출하지 않음. dirty/empty model 반환의 모든 경우가 이 positive fixture에 포함되는 것은 아님 |
| 같은 reader의 raw line-local glyph 위치를 rendered local baseline으로 변환 | LR15 `F14.align-items/align-self → run` | `a/b.line-spacing=0`; `a/b.local-baseline = renderedOffsetY+firstAscender` tolerance.01; `different-local-baselines`; `shared-rendered-baseline: rectA.y+baselineA = rectB.y+baselineB` | 18/36 font, 단일 line `Hx`. `rawY+yBearing≈0`만 baseline으로 읽으면 unequal baseline guard에서 드러남. alignment 실패를 동일 top edge로 expected 변경하지 않음 |
| font/glyph identity를 reader가 그대로 반환 | LR64 `text-reflow-n16/n128 → font-identity` | `font.model.ready`; `font.glyph.A=36`; `font.path`=고정 asset canonical path; `font.ascender=23`, `font.descender=-6` 각각 tolerance1 | reader 밖에서 FontClient description을 조회; capture 중 font load/query를 하지 않음. asset hash는 별도 고정 |
| `TickAnimatorsForTesting`: invalid Window/nonfinite/negative delta; dispatcher manual/recursive guard (`:9183`, dispatcher`:2929`) | LR62 `D06.clock-restoration → manual-and-restore` | `clock.negative-rejected`, `.nan-rejected`, `.inf-rejected`, `.invalid-state-preserved`, `.nested-rejected`, `.manual-disabled`, `.disabled-tick-preserves-state` | 실제 callback 안에서 nested tick을 시도; 실패 후 상태 불변 |
| `SetLayoutTestManualTicks`: 실제 timer 중지/재개; `TickAnimators`의 처음 tick elapsed 미증가 | 같은 action | `clock.active-before-tick`, `.first-tick`, `.first-callback-count=1`, `.first-raw=0`, `.restore`, `.natural-start-once=1`, `.natural-finish-once=1`, `.natural-callbacks`, `.natural-last-raw=1`, `.natural-drained`, `.restore-idempotent` | 수동 첫 tick 뒤 timer 복원,800ms 후 실제 callback/종료 검사. generic core clock 변경 없음 |
| `LayoutInvalidation::AdvanceGeneration` (`layout-invalidation-generation.cpp:62`), Measure/Arrange cache dirty 전환 | LR25 `K06.generation`, `K07.arrange-only → run` | `new-generation`, `cache-invalidated`, `new-cache`; `measure-preserved`, `arrange-invalid`, `settled`, producer1/2 | current generation 차이 및 cache 상태+synthetic producer를 대조. uint32 wrap-to-1 분기는 이 행으로 실행되었다고 주장하지 않음 |
| `ArrangeOwnedMeasureScope` push/pop (`layout-dependency-scope.cpp:55`), 현재 transient owner 읽기 | LR26 `K13.owner..0…2 → run` | `owner-in-measure=1`, `owner-kind-in-measure=1`, `parent-arranging`, `child-measuring`, `owner-after-scope=0`, `owner-kind-after-scope=0`, `balanced-scopes` | child producer 안에서 snapshot; 완료 뒤 owner가 남아 있어야 한다는 잘못된 가정 배제 |
| `InvalidateAncestorLayoutCachesForMeasureMiss`와 standalone unconsumed slot (`view-data-impl.cpp:4551` 이후) | LR27 `K14.external-measure`, `K14.external-standalone-measure → run` | `external.owner-*-cache-cleared`, `external.no-*-dirty`; `external.parent-cache-preserved`, `external.standalone-slot-unconsumed`, `corrected.slot-consumed`, `corrective-child-producer` | out-of-band 결과가 ancestor cache/standalone 소비 상태에 미치는 영향을 구분; geometry assertion과 함께 판단 |

### OFF 비용과 ON observer allocation 증거

| 결정/접점 | 연결된 검사 | 증거의 정확한 의미 |
|---|---|---|
| `build/tizen/CMakeLists.txt:60` option 기본OFF; public/internal diagnostics header의 `#if`; OFF macro `((void)0)` | LR62 `D00.profile-off → run / profile.compiled-capability`; LR62 MD의 OFF symbol/ABI/disassembly 절차 | TC 문자열 한 행은 library OFF 증명이 아님. 동일 Release source의 OFF binary에서 diagnostics export·gNodes/gWindows·testObservation/tick 필드가 없고 일반 경로에 Record 호출이 없는지 binary artifact를 별도 보존 |
| ON reader/capture 함수 실행 구간만 allocator interposition | LR62 MD의 observer allocation 보조 실험 | valid/invalid readers와 Begin/End 호출의 함수별 calls/completed/allocator entry를 별도 profiler로 관측. dlsym/로그/TC UI allocation은 scope 밖. 재귀·중첩 scope·hook binding positive control 필요 |
| `readonly.capture-empty` 및 named storage event가0 | LR62 D05, LR59/60 work rows | Layout event가 없다는 것만 보장; malloc0을 증명하지 않음. direct mmap/custom arena/asynchronous thread 등 profiler 미관측 경로까지 no allocation이라고 일반화하지 않음 |

node8192 registry 포화와 재사용은 아래 추가 행으로 연결됩니다. Window64 registry 포화, 고의 stale pointer, observer 내부 예외의 모든 경로는 LR62의 위 fixture가 직접 검사하는 범위가 아닙니다. 실제 source decision inventory에서 필요한 경우 별도 신규 fixture와 실행 증거로 닫아야 하며, 이 표에 있는 정상/invalid handle 검사를 포화 branch의 PASS로 합산하지 마십시오.

## LR59: cold·warm 계산 결정

구현: `T/tc-lr59-layout-compute-performance.cpp:33` `ObserveImmediate`, `:67` `CheckWork`, `:124` `Benchmark`; 공통 `T/layout-validation-workloads.h`의 `FlatWorkload/ImmediateWorkResult/WorkCapture`입니다. scenario ID는 `stack/flex/grid/absolute-n16/n128/n512`이며 실제 결합은 예를 들어 `stack-n16`입니다. `N`은 child 수, `W=10N−2`, `H=16`, child i의 expected rect=`(10i,0,8,16)`입니다. 각 manager를 같은 logical geometry로 비교하지만 서로 다른 layout 알고리즘의 실행 시간이 같아야 한다는 가정은 없습니다.

| source decision / action | 즉시 assertion | 독립 work budget(ON) | 관측 경계 |
|---|---|---|---|
| `ViewDataImpl::Measure` miss→producer→publish (`:4726/4838/4860/4882`); `work-first_measure` / OFF `first_measure` | `immediate.values=2O+2+2N`, `.mismatches=0`, `.maximum_error=0` | `work.measure.enter/producer/publish=N+1`, hit0, `work.ancestor.visit=N`; manager별 Measure storage 아래 표 | O=1. 반환 MeasuredSize와 root/child measured slot을 **후속 Arrange 전** 비교 |
| `ViewDataImpl::ArrangeImpl` miss→producer→publish (`:5190/5349/5401/5430`); `work-first_arrange` / `first_arrange` | `immediate.values=4O+4+4N`, mismatch0 | `work.arrange.enter/producer/publish=N+1`, hit0; Grid만 `work.measure.enter/hit=N`; 나머지 Measure0; geometry write=실행 전 실제 Actor와 독립 expected의 다른 scalar 수 | Measure 준비는 capture/timing 밖. 최초 Arrange 반환·actual geometry를 **repair Arrange 없이** 비교 |
| 같은 normalized constraint/scale key Measure hit; `work-warm_measure` / `warm_measure` | 위 Measure values 식 | ON O=8, `work.measure.enter/hit=8`, producer/publish0, storage0 | OFF O=4096. 실행 전 settle은 밖이며 각 반환을 fixed buffer에 저장 |
| Arrange hit→`ReplayCachedArrange` (`:5058/5141`); `work-warm_arrange` / `warm_arrange` | 위 Arrange values 식 | ON O=8, `work.arrange.enter/hit=8`, producer/publish0, `work.replay.visit=8(N+1)`, root replay child storage8회/8N elements | OFF O=32. warm replay도 전체 child geometry 확인. 같은 bounds라고 호출 자체를 생략하지 않음 |
| app `FlatWorkload` 구성; OFF `construction` | `sample.immediate.j.values=3+2N`: root requested W/H/child count, 모든 child requested8/16 | ON construction work budget 없음 | 생성 시간과 그 외 계산 phase를 분리. construction 뒤 geometry용 settle은 interval 밖이며 앞서 요청/child 수를 검사 |

`WorkCapture::Check`의 실제33행은 protocol4(`work.capture.started/complete/epoch-sequence/node-identities`), work event15, storage totals2, storage slot4×3입니다. 위 표에서 명시하지 않은 event budget은0입니다. 모든 workload는 detached 직접 호출이므로 `work.wake.request/root.drain/park.request=0`이며, 이것으로 Window idle wake 전체를 검증했다고 해석하지 마십시오. `work.storage.unexpected-sites=0`이므로 예상하지 않은 계측 storage 접점도 실패합니다.

| storage site / source 접점 | FIRST_MEASURE: calls/elements/bytes | FIRST_ARRANGE: calls/elements/bytes |
|---|---|---|
| Stack `Measure :244/:254`, `Arrange :370/:383` | site20 `1/N/N×sizeof(View)`, site22 `1/N/N×sizeof(MeasuredSize)` | site21 `1/N/N×sizeof(View)`, site23 `1/N/N×sizeof(MeasuredSize)` |
| Flex `Measure :523`, `Arrange :625/:637` | site30 `1/N/N×sizeof(View)` | site31 `1/N/N×sizeof(View)`, site32 `1/N/N×sizeof(MeasuredSize)` |
| Grid `Measure :598/:607/:609`, `Arrange :680` | site40 `1/N/N×sizeof(View)`, site42 `1/1/sizeof(float)`, site43 `1/N/N×sizeof(float)` | site41 `1/N/N×sizeof(View)` |
| Absolute `Measure :88`, `Arrange :249` | site10 `1/N/N×sizeof(View)` | site11 `1/N/N×sizeof(View)` |
| View replay `view-data-impl.cpp:5141` | 없음 | WARM만 site2 `O/O×N/O×N×sizeof(View)` |

실제 row ID는 `work.storage.slot{0…3}.site{numericSite}.calls/.elements/.bytes`입니다. slot 번호는 위 순서로 budget에 등록한 순서입니다. 비사용 slot은 site0의0값을 검사합니다. 이 고정 fixture의 budget은 현재 source 접점으로부터 검토한 work 계약이며, 변경 candidate의 계수를 읽어 expected를 자동 생성하지 않습니다. 의도한 알고리즘 변경이 work 계약을 바꾸면 geometry·성능 증거와 함께 계약 변경을 별도로 검토하십시오.

## LR60: sparse·dense·same-value invalidation

구현: `T/tc-lr60-layout-update-performance.cpp` `UpdateWork/UpdateBenchmark`; source는 `ViewDataImpl::SetRequestedHeight → ApplyRequestedHeight`의 동일값 guard와 `InvalidateMeasure` (`view-data-impl.cpp:2275`), Measure hit/miss, Arrange hit/replay입니다. N={16,128,512}; Stack geometry는 LR59와 같고 height16→15를 바꿉니다.

| scenario/action | 독립 변경 입력 | 실제 assertion ID / 식 |
|---|---|---|
| `nN → work-sparse` / OFF `sparse` | 가운데 child 하나만 변경, C=1 | `work.measure.enter=N+1`, producer/publish=C+1, hit=N−C; `work.arrange.enter=N+1`, producer/publish=C+1, hit=N−C; `work.replay.visit=N−C`; `work.geometry.write=C`; `work.invalidate.measure=2C`; `work.ancestor.visit=2C+1` |
| `nN → work-dense` / `dense` | 전체N child 변경, C=N | 같은 C식; cache hit/replay0. Stack Measure/Arrange storage 각각 한 번 |
| `nN → work-same-value` / `same-value` | 모든 child height16 재설정, C=0 | `work.measure.enter/hit=1`, `work.arrange.enter/hit=1`, producer/publish/invalidate/write/ancestor0; `work.replay.visit=N+1`; site2 replay child storage1회/N elements |
| 모든 ON action | 반환과 현재 geometry | `output.measured.width/height=W/16`, `output.arranged`=(0,0,W,16), `geometry.i`의 height만 해당 input15/16, `geometry.visited=N`, `.mismatches=0`, `.maximum_error=0` |
| 모든 OFF action | sample마다15↔16 변경; same-value는16 고정 | `sample.immediate.j.values=12+6N`, `.mismatches=0`, `.maximum_error=0`; `sample.timer_positive.j`; `sample.geometry.j.visited/mismatches/maximum_error`; 마지막 sample의 `sample.geometry.30.i` 전체 Rect |

OFF 각 sample의 setter+Measure+Arrange만 timing에 포함합니다. expected height 갱신·검사·로그는 interval 뒤이며 반환·geometry를 복구하는 추가 layout은 호출하지 않습니다. 불필요한 producer/storage 증가가 timing에서 작게 보이더라도 ON exact budget 실패를 무시하지 마십시오.

## LR64: 복합 계산 workload의 독립 oracle

구현: `T/tc-lr64-layout-complex-performance.cpp`의 `ComplexFixture::Prepare/Mutate/Calculate/Observe`, `BenchmarkComplex`입니다. 각 family는 N16/N128 두 scenario이고 ON action=`workload-correctness`, OFF=`timing`입니다. state s는0/1을 번갈아 바꾸며 expected 구성은 timing 밖, setter+계산은 안, 관측은 직후입니다.

모든 sample j=0…30의 실제 assertion은 `sample.immediate.j.values/.mismatches/.maximum_error`입니다. OFF에만 `sample.timer.j`와 raw sample이 추가됩니다. 마지막 sample은 `final.root`와 `final.node.i`를 개별 Rect로 남깁니다. **LR64는 producer/hit/storage exact budget을 추가로 계수하지 않으며**, family별 geometry와 OFF paired timing을 검사합니다. 단순 manager exact work budget은 LR59/60이 담당합니다.

| family / source decision | 독립 fixture·식 | scalar 수 / 개별 행 범위 |
|---|---|---|
| `deep-dirty-nN`: Stack `Measure/Arrange`, child invalidation의 ancestor 경로 | depth N, 폭120, leaf height20+s; 모든 N ancestor WRAP height20+s; 각 local origin0 | root+N descendant, values=10+4N, D=4(N+1) |
| `balanced-dirty-nN`: 2-child Stack 합산·spacing·한쪽 branch invalidation | N leaves의 완전 이진 tree, spacing2; root H=22N−2+s. rightmost leaf와 해당 ancestor만 height+ s, sibling geometry 고정 | descendants2N−2, values=10+4(2N−2), D=4(2N−1) |
| `flex-wrap-nN`: `FlexLayoutManager::Measure/Arrange`, wrap line 경계와 `ArrangeOneFlexLine` | basis10,height16; containerW=40/50. columns=4/5; rect i=`(10(i mod columns),16 floor(i/columns),10,16)` | values=10+4N, D=4(N+1). ROW/WRAP/FLEX_START의 여러 line; reverse/다른 align을 이 workload가 포괄하지 않음 |
| `flex-grow-nN`: `ApplyFlexGrowShrink :135`의 positive free space | basis8, 반복 weight1/3. containerW=12N/16N, sumGrow=2N; child width=`8+(2 또는4)×weight`, x=이전 폭 prefix sum | values=10+4N, D=4(N+1); full available width 보존 |
| `flex-shrink-nN`: 같은 helper의 negative free space와 scaled shrink | basis16, weight1/3, containerW=12N/8N; sum(shrink×basis)=32N; width=`16−(2 또는4)×weight`, x=prefix sum | values=10+4N, D=4(N+1); 이 fixture는 음수 floor나 min/max 재분배 정책을 검사하지 않음 |
| `grid-star-span-nN`: `ApplyGridDefinitions :264`, `ComputeGridPositions :367`, `ArrangeGridChildrenToCells :387` | columns STAR1:2:1:1, N개 동일STAR row; W250/375, unit=W/5, H=20N. 매 row child `(0,20i,3unit,20)` 및 `(3unit,20i,unit,20)`, 마지막 full row span `(4unit,0,unit,20N)` | child2N+1, values=10+4(2N+1), D=4(2N+2) |
| `variable-recycler-nN`: `LinearItemsLayouterImpl::OnLayoutChildren :65`, `LayoutChunk :288`, `MeasureUpdate :340`, `GetItemBounds :174`, `GetEstimatedContentExtent :417` | VERTICAL, 실제 h_i=10(1+i mod3), decoration L/R/T/B=3/7/2/4, spacing5, cross120/160. outer y_i=Σ(k<i)(h_k+11); actor rect=(3,y_i+2,cross−10,h_i); item bound=(0,y_i,cross,h_i+6); range=Σ(h_i+11)−5 | values=4+8N, D=4N; range/offset0/visible first0/lastN−1와 모든 actor/item bound. viewport100000의 전체 표시 integration fixture이며 실제 RecyclerView virtualization·Group layouter 전체를 이 행으로 대신하지 않음 |
| `text-reflow-nN`: `LabelImpl::OnMeasure`의 width-dependent height, `GetHeightForWidth :2363`, 완료 text model | 고정 DejaVu Sans24, A N개, CHARACTER wrap, W25/40에서1/2 glyph per line; H=29×ceil(N/columns). 고정 asset font identity는 ON 앞 action | values10, D4. text engine shaping 전체·다국어 전체 또는 async image decode 성능을 이 workload의 범위로 주장하지 않음 |

## 인접 soak·transition 성능 연결

| source decision | 신규 scenario/action | 실제 assertion ID / 범위 |
|---|---|---|
| `ReplayCachedArrange`가 변조된 Actor position/size를 cached result로 복구 | LR61 `stack/flex/grid/absolute → run` | 128child,256cycle; index=(cycle×37) mod128에 x=−123.25,width1을 쓴 뒤 `cycle.j.visited=128`, `.mismatches=0`, `.maximum_error=0`;16cycle마다 명시 invalidate. heap leak 판정은 TC MD의10번 실제 재진입·cleanup/RSS/PSS/profile 증거가 별도 필요 |
| 깊은 WRAP ancestor의 padding 합산 | LR61 `depth-8/64/192 → run` | `deep.root`=(8+2D,16+2D), `deep.child.i`=(1,1,8+2i,16+2i). deep tree timing은 LR64 |
| transition 미설정/사용하지 않는 handle/disabled/active Spec/active Animator | LR63 `TR15.mode-{0…4}-n{1,32,128} → mount/work-count` | `performance.fixture/work.handles`, `.logical-children`, `.leaf-i.identity-order`, `.leaf-i.target` 모든 node; `performance.transition.begin-count`=mode≥3 ? N:0, `performance.tick-count=0`, `.trace.complete` |
| 같은 transition workload의 OFF 계산 비용 | LR63 같은 scenario `release-samples` | `performance.warmup.*`, `performance.sample-j.*` 모든 logical target, samples31, operations128(dormant) 또는9(update). active animation Actor 중간값 대신 완료 logical target을 읽음; 실제 timer tick 비용 전체를 dormant work-count로 대신하지 않음 |

## LR52: ScrollView·PageScrollView source 결정

구현: `T/tc-lr52-layout-scroll.cpp`. 실제 조작·관측 순서는 [LR52 MD](tc/tc-lr52-layout-scroll.md)를 따르십시오. Window action은 지정 View snapshot과 Window fence를 모두 요구합니다. 아래에서 `actual geometry`는 fence 후 Actor event property이며 animation pixel 추정값이 아닙니다.

| source function / decision | scenario → action | 실제 assertion ID / 독립 식 |
|---|---|---|
| `ScrollViewLayoutManager::Measure :64`의 MATCH/non-MATCH 축 constraint | `SC.match-constraints.{0…3} → run` | `owner.measure`100×80, `measure.constraint.width/height`=(MATCH ?72/60:FLT_MAX), `content.measure`=(MATCH ?72/60:240/200), `measure.producer=1` |
| `ScrollViewLayoutManager::Arrange :106`의 MATCH 대입/Arrange-owned 재측정 | 같은 scenario | `owner.arrange`=(0,0,100,80), `content.arranged`=(11,7,w,h); `arrange.constraint.width/height`; `arrange.measure.producers`=mixed 축만2, 양축MATCH/양축WRAP1. natural 축 재측정 constraint도240/200으로 exact 대조 |
| Scroll manager `ArrangePolicy::ALWAYS`가 current Actor position을 다시 읽음 | `I05.scroll-preservation → scroll-to/settled-layout-replay` | `scroll-x/y`50/60, `scrolled-content` 및 `preserved`=(−50,−60,240,200), `preserved-scroll-x/y`50/60 |
| `ScrollViewImpl::AdjustScrollPosition :1558`, `SetScrollPosition :230` | 같은 scenario `upper-clamp/negative-clamp` | `maximum-x/y`140/120 및 `clamped-content`; `minimum-x/y`0 및 `origin-content` |
| `SetScrollableWidth/Height :245/:271`, `UpdateScrollingProperties :2258`의 viewport 재계산 | 같은 scenario `resize-viewport` | `larger-viewport`160×120, `new-maximum-x/y`80, `actual-content`=(−80,−80,240,200). content 크기가 같다는 이유로 viewport update를 건너뛰면 검출 |
| `SetContent` 교체 후 range/parent 갱신 | 같은 scenario `replace-smaller-content` | `small-content`60×40, `old-detached`, `small-scroll-x/y=0` |
| padding origin과 MATCH inner extent, axis별 scroll clamp | `SC.axis-padding-margin.{0…2} → mount/maximum/resize/padding-change` | `viewport`, `content`, `scroll.x/y`, `viewport.width/height`, `scrollable.width/height`, `direction`. content width=Horizontal ?240:viewportW−start−end; height=Vertical ?200:viewportH−top−bottom; origin=(start−scrollX,top−scrollY) |
| content 내부 default View의 padding+child margin 합성 | 같은 scenario 모든 action | `nested-margin`=(5,13,max(contentW−14,0),max(contentH−32,0)); 내부 padding3/5/7/11, margin2/4/6/8. ScrollView가 content 자체 margin을 처리한다는 미정 계약을 가정하지 않음 |
| `PageScrollViewImpl::GetPageCount :124`, default page viewport | `PAGE.horizontal/vertical → mount-default-size` | `initial.count=3`, `.current=0`, `.scroll.main/cross=0`, `.content`250×80(axis별 전치), `.scrollable=250`, `.viewport=100`; `initial.signal-count=1`, `page.signal.page/count=0/3` |
| `ScrollToPage :147`, page index clamp와 부분 마지막 page | 같은 scenario `page-boundaries` | `negative.*`:count3/current0/scroll0; `last-partial.*`:count3/current2/scroll150; 마지막 signal2/3 |
| viewport 변경에 따른 default page 수/현재 index 갱신 | 같은 scenario `viewport-resize` | `resized.count=2`, `.current=1`, `.scroll.main=125`, `.viewport=125`, `.scrollable=250`, content origin−125; signal1/2 |
| `SetPageSize :85`와 `GetEffectivePageSize`, axis component 선택 | 같은 scenario `explicit-page-size` | `explicit.count=3`, `.current=1`, `.scroll.main=125`; `explicit.width/height`=main100/cross17; signal1/3 |
| `NotifyPagesInserted :375`: index≤current, 즉시 expected count/range, notification suppression | 같은 scenario `insert-before-current` | `immediate-insert.count/current=4/2`, `.scroll.main=200`, `.scrollable=400`, `.content`의 extent는이전250; `insert.signal-delta=1`; `settled-insert.*` 실제 extent350; `insert.order`, `old-current-local` main200/extent100, `old-current-visible=0` |
| `NotifyPagesRemoved :429`: before-current 이동, 즉시/다음 layout 구분 | 같은 scenario `remove-before-current` | `immediate-remove.count/current=3/1`, `.scroll.main=100`, `.scrollable=300`, 이전 Actor extent350; `remove.signal-delta=1`; `settled-remove.*` extent250 |
| 같은 함수의 zero page/current−1, default page 복원 | 같은 scenario `remove-all/restore-default` | `immediate-empty/settled-empty.count=0`, `.current=−1`, scroll0; Actor extent250→0, `empty.signal-delta=1`; `restored.count/current=1/0`, content100, scroll0, signal0/1 |

Page signal buffer 검사 ID는 각 action의 `page.signal.buffer`(고정64 event, overflow없음), `page.signal.page`, `page.signal.count`입니다. Notify의 임시 range(page100×expected count400/300)는 실제 마지막 부분 page를 포함하는 최종 extent350/250과 다릅니다. source 내부 임시값을 최종 geometry oracle로 사용하지 않습니다.

Scroll fling/gesture 물리·scrollbar appearance 전체는 여기의 Layout 계산 분모가 아닙니다. Page의 current/after/overlap·index/count clamp·pre-layout notification은 아래 추가 행으로 연결됩니다. positive effect가 다른 before-current fixture를 실행했다는 이유만으로 이 분기까지 실행되었다고 주장하지 마십시오.

## 추가 registry·Page decision 연결

| source decision | 신규 scenario/action | 실제 assertion ID / 독립 기대 |
|---|---|---|
| `RegisterNode`8192 slot 포화, 기존 object 재등록/ID 교체는 빈 slot 없이 수행 | LR62 `D01.registry-capacity → run` | `capacity.fill-all-accepted`, `.ids-exact`, `.overflow-rejected`, `.overflow-extra-unregistered=0`, `.full-idempotent`, `.full-rebind`, `.full-rebind-id=90001`, `.full-duplicate-rejected`, `.released-id-alone-no-slot` |
| 파괴에 따른 slot 해제, clear 후 전체 용량 재사용 | 같은 action | `.destroyed-slot-reusable`, `.reused-slot-id=4097`, `.full-again`, `.unaffected-identities`, `.clear-releases-all`, `.refill-all-accepted`, `.refill-identities`, `.refill-full`, `.final-slot-reusable`, `.final-id=6209`, `.final-clear`; 준비·8192개 순회는 timing 밖 |
| `NotifyPagesInserted`: index=current의 shift, index>current의 유지, 음수/과도 index clamp | LR52 `PAGE.notify-{horizontal/vertical}.{insert-at-current/insert-after/insert-negative-index/insert-past-end} → mount/notify-and-settle` | initial5pages/current2; 최종(count,current)=(6,3)/(6,2)/(6,3)/(6,2). `notify.immediate.*`, `notify.signal-delta=1`, `notify.logical-count`, `notify.settled.*`, `notify.visible-identity/local/origin` |
| `NotifyPagesRemoved`: after/current/overlap/마지막 current, index clamp, excess count clamp | 같은 prefix의 `remove-after/current/through-current/last-current/negative-index/past-end/all-clamped` | initial5pages/current2(마지막 current만4), 최종(count,current)=(4,2)/(4,2)/(2,1)/(4,3)/(4,1)/(4,2)/(0,−1). content extent500→100×count, current<0이면 scroll0 아니면100×current; empty는 `notify.empty-children` |
| nonpositive insert/remove count early return | `PAGE.notify-AXIS.nonpositive-count → notify-noop` | `noop.0…3` 각 count5/current2/scroll200/content500 유지, `.signal-delta=0`; 마지막 signal과 logical count도 유지 |
| `SetPageSize` primary<1 return, same-size return; default pageLen<1 count fallback | `PAGE.pre-layout-AXIS.{insert-before/insert-after/remove-empty} → notify-before-layout` | `pending.initial/explicit/same-size/default.*`; default count1/current0, explicit count0/current−1; viewport/scrollable0, signal0, page size getter100/17(axis별) |
| Notify의 pre-layout pgLen0 및 expected count, oldTotal0 remove no-op | 같은 action | `pending.notified.*`: before2/1,after2/0,empty0/−1; scroll0/range0; signal-count1. empty 재호출 `pending.already-empty.*` 동일값/signal1 |
| 첫 실제 layout에서 expected page와 표시 위치 일치 | 같은 scenario `attach-and-settle` | `pending.settled.*`, `pending.visible-identity/local/origin`; before current1/scroll100,after0/0,empty−1/0. pending navigation 유실은 expected를0으로 바꾸지 않고 FAIL로 유지 |

## Release reference/candidate와 실제 증거 연결

실행 방법과 데이터 schema는 [LR59 performance 절차](tc/tc-lr59-layout-compute-performance.md) 및 각 TC MD를 따르십시오. source 판정은 `layout-validation-tools.py:266` `sample_medians`, `:310` `paired_ci`, `:322` `compare`에 있습니다.

| 판단 결정 | 실제 검사/식 | 요구 증거 |
|---|---|---|
| 각 phase의 계산 정확성 | LR59/60/64 sample immediate mismatch0, 개별 마지막 geometry; LR63 모든 sample logical geometry | timer가 정상이어도 geometry 실패면 performance experiment를 통과시키지 않음 |
| measurement window 분리 | LR59/60/64 sample41중 앞10 제외,31 raw sample; phase별 operations를 함께 기록 | parser가 timer_floor_ns 이상 유한 duration, 동일 operations/scenario/action을 검사 |
| 독립 reference pin·조건 일치 | `compare`의 app/library/fixture hash, assets, CPU affinity/governor, resolution/DPI, compiler flags, diagnostics=false/coverage=false/Release | 같은 fixture revision, 동일 기기/환경. baseline 부재·조건 불일치는 PASS가 아님. app×library2×2 실험도 logical workload가 같아야 함 |
| 반복과 다중 비교 | 독립 process pairs31/62/93, AB/BA 균형, metric family×3 look에 alpha 보정; 각 pair의 log(medianCandidate/medianReference) bootstrap | 로그/PID 재사용 금지. 사전에 plan·manifest SHA 고정, 충분한 bootstrap tail resolution |
| 유의한 slowdown 우선 FAIL | CI lower(log ratio)>0이면 `FAIL` | 작은 slowdown도 유의하면 허용 percentage로 면제하지 않음; regression_margin=0 |
| 정상 성능의 정밀도 내 통과 | lower≤0이고 upper≤log(1+mde), halfWidth≤log(1+precision)이면 `PASS_AT_DECLARED_RESOLUTION` | precision/MDE/timer floor는 사전 고정; 뜻은 그 해상도에서 regression 미검출이며 무한 정밀도 동일성 보장이 아님 |
| precision 부족 | 31/62쌍에서는 MORE_PAIRS_REQUIRED,93쌍에서는 INDETERMINATE | 더 긴 실행이나 조건 개선 후 새 사전 plan으로 측정; 미결을 PASS에 합산하지 않음 |
| work regression | LR59/60 ON exact event/storage budget 중 하나라도 다르면 FAIL | OFF timing의 통계 결과와 독립적으로 보고; allocation 원인 분석은 별도 profiler 증거 |

각 map 행을 완료 증거와 연결할 때는 `source revision`, `app/library SHA`, `profile`, `TC/scenario/action`, `assertion IDs`, `actual/expected`, `run/log SHA`, `coverage artifact(있을 경우)`를 함께 기록하십시오. mutant 검출은 먼저 같은 baseline TC가 통과한 뒤 하나의 source predicate/formula만 바꾸고 지정 assertion이 실패한 경우로 한정합니다. compile 실패·앱 시작 실패·관측 누락은 수치 oracle의 mutant 검출 성공으로 집계하지 마십시오.
