# LR59. Layout: Compute performance

이 문서는 [tc-lr59-layout-compute-performance.cpp](tc-lr59-layout-compute-performance.cpp)의 실행 절차입니다. 기존 Layout TC 결과는 합산하지 마십시오. 전체 실행 조건은 [suite 검증 기준](../layout-validation-coverage.md)을 먼저 읽으십시오.

## 실행 및 완료 확인

1. app와 실제 resolve된 library의 build-ID, revision, build type, asset SHA-256을 기록하고 stdout을 저장하십시오. launcher에서 `LR59`를 검색하여 해당 TC에 진입하십시오.
2. `LR59.Reset run`을 한 번 실행하고 새 `run`을 기록하십시오. scenario 내 step 사이에는 Reset하지 마십시오.
3. `Run scenario`(step 하나씩 진행하려면 `Run step`)를 누른 뒤 `LR_ACTION`의 scenario/step/action_seq/required_checks를 기록하십시오. 동기 step은 즉시 완료됩니다. Window/resource step은 해당 episode의 완료까지 기다리십시오. 10초 내 완료되지 않으면 PENDING을 PASS로 처리하지 마십시오. 성능 sample step만 최대 120초를 허용합니다.
4. 각 step 완료 시 자동 출력되는 **모든** `LR_CHECK` 행을 읽으십시오. 행의 tc/pid/run/scenario/step/action_seq를 연결하여 다른 실행의 값과 혼합하지 마십시오. 같은 행을 다시 읽으면 값이 같아야 합니다.
5. 해당 step의 실제 행 수와 required_checks가 정확히 같아야 합니다. 일반 rect 허용오차는 0.001, 정수/bool/ID는 exact입니다. NaN/Inf, 누락/잘림, 중복 읽기로 채운 행 수, unexpected exception, crash, timeout, cleanup error는 FAIL입니다. `passed`만 믿지 말고 actual/expected/tolerance를 재계산하십시오.
6. 모든 step 후 `Next scenario`로 이동하고 모든 scenario를 실행하십시오. 마지막 `LR_RESULT`의 completed_scenarios=required_scenarios, failures=0 및 전체 행 증거가 필요합니다. 그 전의 `FAILING`/`INCOMPLETE`는 종결 verdict가 아니므로 실행을 중단하지 마십시오. 완료 후 `< Back`으로 나가 `LR_CASE_END`의 `completed_scenarios`가 `required_scenarios`와 같은지 확인하십시오. 실패 후 Reset으로 실패 이력을 지우지 마십시오.

## 입력과 독립 기대값

각 Stack/Flex/Grid/Absolute × N=16/128/512, 총 12개 scenario에서 timing step `construction`, `first_pass`, `warm_pass`, `idle_pass`를 먼저 순서대로 실행하십시오. step당 필수 행 수는 220+4*N입니다. 시간 판정에는 Release 빌드가 필요합니다.

fixture의 i번째 leaf는 (10*i,0,8,16), root는 (10*N−2,16)이며 workload는 화면의 stage에 부착되고 형제 leaf 하나가 같은 stage에 있습니다. 모든 layout은 `LayoutController::ProcessLayouts()`(응용이 사용하는 controller 경로)로 수행합니다. phase의 측정 구간은 다음과 같습니다.

| phase | 측정 구간(operations) | 의미 |
|---|---|---|
| `construction` | `FlatWorkload` 생성 1회 | View/params 생성 비용. 이후 부착·pass·검사는 구간 밖 |
| `first_pass` | 부착 직후 `ProcessLayouts()` 1회 | 첫 measure/arrange producer 경로 |
| `warm_pass` | (형제 폭 변경 + `ProcessLayouts()`) 32회 | root measure/arrange cache hit과 subtree replay |
| `idle_pass` | dirty 없는 `ProcessLayouts()` 4096회 | controller 진입 오버헤드 |

10회 warm-up 후 31개 sample마다 `sample.immediate.*`(root measured/arranged와 모든 leaf의 measured slot·event-side rect, values=12+6*N), `sample.timer_positive`, `sample.geometry.*`(leaf 집계 3행)을 검사하고, step 끝에서 화면 부착된 워크로드의 새 `REFRESH_ONCE` render task 완료 뒤 current scene geometry로 `rendered.*`(집계 3행 + leaf별 4행)을 검사합니다. 검사·출력 시간은 측정 구간에 포함하지 않습니다. `nanoseconds/operations`로 계산하십시오.

이어서 같은 12개 scenario마다 work-budget step `work-first_pass`, `work-warm_pass`, `work-idle_pass`를 실행하십시오. 관측 hook은 모든 빌드에 포함되므로 두 step 집합은 하나의 빌드에서 연속 실행합니다. 행 수는 step당 42+4*N입니다. capture는 등록한 workload node(root=1, leaf=2..N+1)의 event만 계수하며 stage·launcher view의 event는 무시합니다. 다음 독립 budget을 exact로 검사하고 표에 없는 work count는 0이어야 합니다.

| phase | Measure enter/producer/hit/publish | Arrange enter/producer/hit/publish | replay | ancestor visit | root drain |
|---|---|---|---|---|---|
| first pass | N+1/N+1/0/N+1 (Grid는 enter 2N+1, hit N) | N+1/N+1/0/N+1 | 0 | N | 1 |
| warm pass ×32 | 32/0/32/0 | 32/0/32/0 | 32*(N+1) | 0 | 32 |
| idle pass ×4096 | 0 | 0 | 0 | 0 | 0 |

first pass의 geometry write는 부착 전 Actor rect와 독립 expected rect의 exact component 차이 개수입니다. wake는 부착 시 capture 밖에서 이미 무장되고 수동 `ProcessLayouts()`는 이를 소비하지 않으므로 모든 phase에서 0, park는 0입니다. storage는 first pass에서 Measure/Arrange 명명 site를 각 1회(Stack children+work ×2, Flex children+children/work, Grid children/rows/columns+children, Absolute children ×2), warm pass는 VIEW_REPLAY_CHILDREN 32회/요소 32*N입니다. element 합계는 N(Grid rows 1)이며 bytes는 해당 C++ element sizeof와의 곱입니다. 표시된 storage slot의 calls/elements/bytes와 unexpected-sites 0을 모두 검증하십시오.

이 storage 검사는 instrumentation이 붙은 명명된 reserve/vector 지점의 요청량입니다. 전체 allocator 호출 횟수·전체 heap bytes라고 해석하지 마십시오. buffer overflow, node ID 누락, epoch/sequence 불일치도 FAIL입니다. work-budget의 exact 실패를 시간 차이가 작다는 이유로 무시하지 마십시오.

시간 측정은 반드시 capture를 열지 않는 Release app/library에서 실행하십시오. 최종 PASS에는 같은 fixture의 work-budget exact 결과와 짝지은 timing 비교 결과가 모두 필요합니다. coverage/Debug 빌드에서는 시간 회귀를 판정하지 마십시오. 화면 EXTERNAL_REQUIRED는 내부 geometry 검사가 끝났다는 상태입니다. 아래 공통 성능 절차에 따른 기준 대비 검정이 끝나야 PASS를 부여할 수 있습니다. 큰 N의 workload는 window 폭을 넘을 수 있습니다. 최종 `rendered.*`는 timing 구간 밖에서 `AfterFrame`으로 새 render task 완료를 받은 뒤 모든 leaf의 current scene geometry를 독립 식과 비교합니다. 화면 밖 leaf까지 OS Window에서 실제 보였다는 의미는 아닙니다. frame 실패 시 event-side 값으로 대체하지 마십시오.

## 성능 비교

[성능 계획 예시](../layout-validation-performance-plan.example.json)를 실행 전에 복사하여 metric family/seed/alpha/precision/MDE/timer floor를 고정하십시오. 이 값은 candidate 결과를 본 후 완화하지 마십시오. 비교는 같은 device, affinity, governor, 해상도/DPI, resources, compiler flags를 사용한 reference/candidate의 독립 process AB/BA pair 단위입니다. 한 process의 31개 내부 sample을 median으로 요약하고 31→62→최대 93 process pair에서 사전 고정한 family/반복 관측 보정 CI를 계산합니다.

`python3 layout-validation-tools.py compare PLAN.json EXPERIMENT.json`으로 저장된 표본을 분석하십시오. CI 하한>1이면 크기와 무관하게 FAIL입니다. 그 외 CI 상한≤1+MDE이고 half-width가 precision을 만족할 때에만 명시한 해상도에서 regression 미검출 PASS입니다. 최대 pair 이후에도 정밀도가 부족하면 INDETERMINATE입니다. MDE는 허용 slowdown 비율이 아닙니다. significant slowdown을 MDE 이하라고 통과시키지 마십시오.

애플리케이션 변경과 library patch 영향은 같은 workload 계약으로 app/reference-library, app/candidate-library를 비교하고, app 변경도 있다면 old/new app × old/new library의 네 조합을 기록하십시오. unavailable 조합은 분석 누락으로 표시하십시오. 단일 build의 시간만으로 regression PASS를 부여하지 마십시오.

## 종료와 증거

`< Back`으로 나간 뒤 다시 진입하여 첫 step을 재실행하십시오. 이전 timer/callback/fixture/추가 Window가 남아 있으면 FAIL입니다. 결과에는 scenario/input/step, 실제값과 기대값, 최초 실패 행, 전체 stdout, artifact fingerprint를 첨부하십시오. `python3 layout-validation-tools.py check-log LOG --tc LR59 --profile release --manifest MANIFEST --manifest-sha256 PIN --external`은 저장된 행의 누락과 수치를 다시 검사하는 보조 수단입니다. 이 도구는 앱을 실행하거나 서버의 UI 자동 조작을 대체하지 않습니다.

## 고정 실행 계약과 종료 증거

각 step 실행(`Run step`/`Run scenario`/`Run all`) 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 각 step의 전체 stdout 행은 완료 시 자동 출력되므로 그대로 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 12 scenario, 84 action, 85544 checks. 이 표에 없는 scenario는 컴파일된 계약에 선언되지 않으며 실행·판정 대상이 아닙니다.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `stack-n16` | `construction` : 284 → `first_pass` : 284 → `warm_pass` : 284 → `idle_pass` : 284 → `work-first_pass` : 106 → `work-warm_pass` : 106 → `work-idle_pass` : 106 |
| `stack-n128` | `construction` : 732 → `first_pass` : 732 → `warm_pass` : 732 → `idle_pass` : 732 → `work-first_pass` : 554 → `work-warm_pass` : 554 → `work-idle_pass` : 554 |
| `stack-n512` | `construction` : 2268 → `first_pass` : 2268 → `warm_pass` : 2268 → `idle_pass` : 2268 → `work-first_pass` : 2090 → `work-warm_pass` : 2090 → `work-idle_pass` : 2090 |
| `flex-n16` | `construction` : 284 → `first_pass` : 284 → `warm_pass` : 284 → `idle_pass` : 284 → `work-first_pass` : 106 → `work-warm_pass` : 106 → `work-idle_pass` : 106 |
| `flex-n128` | `construction` : 732 → `first_pass` : 732 → `warm_pass` : 732 → `idle_pass` : 732 → `work-first_pass` : 554 → `work-warm_pass` : 554 → `work-idle_pass` : 554 |
| `flex-n512` | `construction` : 2268 → `first_pass` : 2268 → `warm_pass` : 2268 → `idle_pass` : 2268 → `work-first_pass` : 2090 → `work-warm_pass` : 2090 → `work-idle_pass` : 2090 |
| `grid-n16` | `construction` : 284 → `first_pass` : 284 → `warm_pass` : 284 → `idle_pass` : 284 → `work-first_pass` : 106 → `work-warm_pass` : 106 → `work-idle_pass` : 106 |
| `grid-n128` | `construction` : 732 → `first_pass` : 732 → `warm_pass` : 732 → `idle_pass` : 732 → `work-first_pass` : 554 → `work-warm_pass` : 554 → `work-idle_pass` : 554 |
| `grid-n512` | `construction` : 2268 → `first_pass` : 2268 → `warm_pass` : 2268 → `idle_pass` : 2268 → `work-first_pass` : 2090 → `work-warm_pass` : 2090 → `work-idle_pass` : 2090 |
| `absolute-n16` | `construction` : 284 → `first_pass` : 284 → `warm_pass` : 284 → `idle_pass` : 284 → `work-first_pass` : 106 → `work-warm_pass` : 106 → `work-idle_pass` : 106 |
| `absolute-n128` | `construction` : 732 → `first_pass` : 732 → `warm_pass` : 732 → `idle_pass` : 732 → `work-first_pass` : 554 → `work-warm_pass` : 554 → `work-idle_pass` : 554 |
| `absolute-n512` | `construction` : 2268 → `first_pass` : 2268 → `warm_pass` : 2268 → `idle_pass` : 2268 → `work-first_pass` : 2090 → `work-warm_pass` : 2090 → `work-idle_pass` : 2090 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`이고 `completed_scenarios == required_scenarios`이며 모든 count(failures 포함)가 마지막 `LR_RESULT`와 같아야 합니다. PASS는 그 완료 조건과 failures=0이 함께 성립할 때만입니다. `layout-validation-coverage.md`에 알려진 제품 결함 행이 문서화된 TC는 failures>0으로 종결되며, `check-log`가 `observations-valid-with-failures`로 그 행을 **정확히** 나열해야 합니다. 그 밖의 실패 행이나 `invalid-evidence`는 회귀 또는 무효 증거이며 PASS가 아닙니다. END 누락·cleanup 오류도 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR59 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. `--profile`의 `diagnostic`/`release` 두 값은 내용이 동일한 같은 계약을 가리키므로 어느 쪽을 지정해도 같은 결과입니다. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. `status`가 `observations-valid`면 실패 행이 없고, `observations-valid-with-failures`면 증거는 유효하며 `failing_rows`에 나열된 행만 실패했다는 뜻이고(exit 1), `invalid-evidence`면 증거 자체가 무효이므로 재실행해야 합니다(exit 2). `observer.timeout`은 layout 완료 관측 실패, `render.frame-timeout`은 새 render task 또는 입력 반영 완료 관측 실패, `render.settle-timeout`은 current geometry 반영 관측 실패로 분류하십시오. 이 과정에서 필수 관측 행이 누락되거나 실제 행 수가 계약과 달라지면 strict 검증 결과는 `invalid-evidence`(exit 2)입니다. 해당 observer 행, `protocol.required_checks`, 도구의 `error`, 원본 stdout/stderr를 함께 보존하십시오. 재실행은 새 run 증거로 별도 기록하고 첫 실패를 삭제하지 마십시오. timeout, 필수 행 누락, `LR_READY` 또는 END 누락을 **어떤 경우에도 PASS로 처리하지 마십시오**. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
