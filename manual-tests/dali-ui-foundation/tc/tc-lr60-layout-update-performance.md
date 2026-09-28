# LR60. Layout: Update performance

이 문서는 [tc-lr60-layout-update-performance.cpp](tc-lr60-layout-update-performance.cpp)의 실행 절차입니다. 기존 Layout TC 결과는 합산하지 마십시오. 전체 실행 조건은 [suite 검증 기준](../layout-validation-coverage.md)을 먼저 읽으십시오.

## 실행 및 완료 확인

1. app와 실제 resolve된 library의 build-ID, revision, build type, asset SHA-256을 기록하고 stdout을 저장하십시오. launcher에서 `LR60`를 검색하여 해당 TC에 진입하십시오.
2. `LR60.Reset run`을 한 번 실행하고 새 `run`을 기록하십시오. scenario 내 step 사이에는 Reset하지 마십시오.
3. `Run scenario`(step 하나씩 진행하려면 `Run step`)를 누른 뒤 `LR_ACTION`의 scenario/step/action_seq/required_checks를 기록하십시오. 동기 step은 즉시 완료됩니다. Window/resource step은 해당 episode의 완료까지 기다리십시오. 10초 내 완료되지 않으면 PENDING을 PASS로 처리하지 마십시오. 성능 sample step만 최대 120초를 허용합니다.
4. 각 step 완료 시 자동 출력되는 **모든** `LR_CHECK` 행을 읽으십시오. 행의 tc/pid/run/scenario/step/action_seq를 연결하여 다른 실행의 값과 혼합하지 마십시오. 같은 행을 다시 읽으면 값이 같아야 합니다.
5. 해당 step의 실제 행 수와 required_checks가 정확히 같아야 합니다. 일반 rect 허용오차는 0.001, 정수/bool/ID는 exact입니다. NaN/Inf, 누락/잘림, 중복 읽기로 채운 행 수, unexpected exception, crash, timeout, cleanup error는 FAIL입니다. `passed`만 믿지 말고 actual/expected/tolerance를 재계산하십시오.
6. 모든 step 후 `Next scenario`로 이동하고 모든 scenario를 실행하십시오. 마지막 `LR_RESULT`의 completed_scenarios=required_scenarios, failures=0 및 전체 행 증거가 필요합니다. 그 전의 `FAILING`/`INCOMPLETE`는 종결 verdict가 아니므로 실행을 중단하지 마십시오. 완료 후 `< Back`으로 나가 `LR_CASE_END`의 `completed_scenarios`가 `required_scenarios`와 같은지 확인하십시오. 실패 후 Reset으로 실패 이력을 지우지 마십시오.

## 입력과 독립 기대값

N=16/128/512의 3개 scenario에서 timing step `sparse`, `dense`, `same-value`를 먼저 실행하십시오. 필수 행 수는 step당 220+4*N이며, 시간 판정에는 Release 빌드가 필요합니다.

Stack horizontal(i번째 x=10*i, width 8, y 0)을 화면의 stage에 부착하고 초기 `ProcessLayouts()`로 settle합니다. sparse는 중앙 item 한 개의 height만 15/16으로 왕복하고 dense는 모든 item을 변경합니다. same-value는 이미 16인 값을 16으로 설정하며, 아무것도 invalidate되지 않으므로 형제 leaf의 폭을 바꿔 pass를 유도합니다. 각 sample의 측정 구간은 setter(+same-value의 형제 변경)와 `ProcessLayouts()` 1회입니다. root 16 높이에서 각 leaf의 요청 height가 유지되어야 하며 다른 leaf가 변경되면 FAIL입니다. `sample.immediate.*`의 scalar values는 12+6*N, mismatch와 maximum_error는 0이어야 하고, step 끝의 `rendered.*`는 화면 부착된 워크로드에 대해 LayoutController가 배치한 event-side leaf별 사각형입니다(렌더 프레임 아님).

이어서 같은 scenario에서 work-budget step `work-sparse`, `work-dense`, `work-same-value`를 실행하고 step당 45+4*N행을 확인하십시오. 관측 hook은 모든 빌드에 포함되므로 두 step 집합은 하나의 빌드에서 연속 실행합니다. 등록한 node(root=1, leaf=2..N+1)의 event만 계수합니다. 초기 settle 후 변경 leaf 수 M은 sparse 1, dense N, same-value 0입니다. M>0일 때 Measure와 Arrange 각각 enter=N+1, producer=publish=M+1, hit=N−M입니다. replay=N−M, geometry write=M, invalidate-measure=2*M, ancestor-visit=2*M+1, root drain=1입니다. Stack Measure children/work와 Arrange children/work는 각각 1회, 각 N개이며 bytes는 N*sizeof(View) 또는 N*sizeof(MeasuredSize)입니다. M=0이면 Measure enter=hit 1, Arrange enter=hit 1, replay=N+1, VIEW_REPLAY_CHILDREN 1회/요소 N, root drain 1이며 producer/publish/write/invalidation/ancestor-visit는 0입니다. wake는 부착 시 capture 밖에서 무장되므로 0, park는 0입니다.

capture buffer overflow, 잘못된 node/epoch/sequence, 알려지지 않은 storage site는 FAIL입니다. storage 값은 계측된 명명 지점의 요청 횟수/요소/bytes이며 전체 heap allocation을 뜻하지 않습니다. work budget과 timing 비교는 둘 다 있어야 최종 PASS입니다.

fixture 생성, 부착, geometry 검증, 로그는 시간 구간 밖입니다. 10회 warm-up 후 31개 sample을 원문 보존하십시오. 같은 값 재설정의 불필요한 invalidation으로 시간이 늘어나는 경우도 별도 metric에서 검출하십시오. 시간은 capture를 열지 않는 Release 빌드에서 실행하고 아래 비교 절차를 따르십시오. controller 경로로 바뀐 현재 fixture revision으로 reference를 다시 수집하며, 이전 binary와 표본을 혼합하지 마십시오.

## 성능 비교

[성능 계획 예시](../layout-validation-performance-plan.example.json)를 실행 전에 복사하여 metric family/seed/alpha/precision/MDE/timer floor를 고정하십시오. 이 값은 candidate 결과를 본 후 완화하지 마십시오. 비교는 같은 device, affinity, governor, 해상도/DPI, resources, compiler flags를 사용한 reference/candidate의 독립 process AB/BA pair 단위입니다. 한 process의 31개 내부 sample을 median으로 요약하고 31→62→최대 93 process pair에서 사전 고정한 family/반복 관측 보정 CI를 계산합니다.

`python3 layout-validation-tools.py compare PLAN.json EXPERIMENT.json`으로 저장된 표본을 분석하십시오. CI 하한>1이면 크기와 무관하게 FAIL입니다. 그 외 CI 상한≤1+MDE이고 half-width가 precision을 만족할 때에만 명시한 해상도에서 regression 미검출 PASS입니다. 최대 pair 이후에도 정밀도가 부족하면 INDETERMINATE입니다. MDE는 허용 slowdown 비율이 아닙니다. significant slowdown을 MDE 이하라고 통과시키지 마십시오.

애플리케이션 변경과 library patch 영향은 같은 workload 계약으로 app/reference-library, app/candidate-library를 비교하고, app 변경도 있다면 old/new app × old/new library의 네 조합을 기록하십시오. unavailable 조합은 분석 누락으로 표시하십시오. 단일 build의 시간만으로 regression PASS를 부여하지 마십시오.

## 종료와 증거

`< Back`으로 나간 뒤 다시 진입하여 첫 step을 재실행하십시오. 이전 timer/callback/fixture/추가 Window가 남아 있으면 FAIL입니다. 결과에는 scenario/input/step, 실제값과 기대값, 최초 실패 행, 전체 stdout, artifact fingerprint를 첨부하십시오. `python3 layout-validation-tools.py check-log LOG --tc LR60 --profile release --manifest MANIFEST --manifest-sha256 PIN --external`은 저장된 행의 누락과 수치를 다시 검사하는 보조 수단입니다. 이 도구는 앱을 실행하거나 서버의 UI 자동 조작을 대체하지 않습니다.

## 고정 실행 계약과 종료 증거

각 step 실행(`Run step`/`Run scenario`/`Run all`) 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 각 step의 전체 stdout 행은 완료 시 자동 출력되므로 그대로 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 3 scenario, 18 action, 18129 checks. 이 표에 없는 scenario는 컴파일된 계약에 선언되지 않으며 실행·판정 대상이 아닙니다.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `n16` | `sparse` : 284 → `dense` : 284 → `same-value` : 284 → `work-sparse` : 109 → `work-dense` : 109 → `work-same-value` : 109 |
| `n128` | `sparse` : 732 → `dense` : 732 → `same-value` : 732 → `work-sparse` : 557 → `work-dense` : 557 → `work-same-value` : 557 |
| `n512` | `sparse` : 2268 → `dense` : 2268 → `same-value` : 2268 → `work-sparse` : 2093 → `work-dense` : 2093 → `work-same-value` : 2093 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`이고 `completed_scenarios == required_scenarios`이며 모든 count(failures 포함)가 마지막 `LR_RESULT`와 같아야 합니다. PASS는 그 완료 조건과 failures=0이 함께 성립할 때만입니다. `layout-validation-coverage.md`에 알려진 제품 결함 행이 문서화된 TC는 failures>0으로 종결되며, `check-log`가 `observations-valid-with-failures`로 그 행을 **정확히** 나열해야 합니다. 그 밖의 실패 행이나 `invalid-evidence`는 회귀 또는 무효 증거이며 PASS가 아닙니다. END 누락·cleanup 오류도 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR60 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. `--profile`의 `diagnostic`/`release` 두 값은 내용이 동일한 같은 계약을 가리키므로 어느 쪽을 지정해도 같은 결과입니다. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. `status`가 `observations-valid`면 실패 행이 없고, `observations-valid-with-failures`면 증거는 유효하며 `failing_rows`에 나열된 행만 실패했다는 뜻이고(exit 1), `invalid-evidence`면 증거 자체가 무효이므로 재실행해야 합니다(exit 2). `observer.timeout`은 layout 완료 관측 실패, `render.frame-timeout`은 새 render task 또는 입력 반영 완료 관측 실패, `render.settle-timeout`은 current geometry 반영 관측 실패로 분류하십시오. 이 과정에서 필수 관측 행이 누락되거나 실제 행 수가 계약과 달라지면 strict 검증 결과는 `invalid-evidence`(exit 2)입니다. 해당 observer 행, `protocol.required_checks`, 도구의 `error`, 원본 stdout/stderr를 함께 보존하십시오. 재실행은 새 run 증거로 별도 기록하고 첫 실패를 삭제하지 마십시오. timeout, 필수 행 누락, `LR_READY` 또는 END 누락을 **어떤 경우에도 PASS로 처리하지 마십시오**. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
