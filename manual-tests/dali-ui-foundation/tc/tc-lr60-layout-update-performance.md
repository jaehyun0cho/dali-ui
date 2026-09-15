# LR60. Layout: 변경 유형별 성능

이 문서는 [tc-lr60-layout-update-performance.cpp](tc-lr60-layout-update-performance.cpp)의 실행 절차입니다. 기존 Layout TC 결과는 합산하지 마십시오. 전체 실행 조건과 profile은 [suite 검증 기준](../layout-validation-coverage.md)을 먼저 읽으십시오.

## 실행 및 완료 확인

1. app와 실제 resolve된 library의 build-ID, revision, profile, asset SHA-256을 기록하고 stdout을 저장하십시오. launcher에서 `LR60`를 검색하여 해당 TC에 진입하십시오.
2. `LR60.Reset run`을 한 번 실행하고 새 `run`을 기록하십시오. scenario 내 step 사이에는 Reset하지 마십시오.
3. `Apply next step`을 한 번 누른 뒤 `LR_ACTION`의 scenario/step/action_seq/required_checks를 기록하십시오. 동기 step은 즉시 완료됩니다. Window/resource step은 해당 episode의 완료까지 기다리십시오. 10초 내 완료되지 않으면 PENDING을 PASS로 처리하지 마십시오. 성능 sample step만 최대120초를 허용합니다.
4. `Read result`를 누르고 `Next result page`로 **모든** 행을 읽으십시오. page1만 확인하지 마십시오. 행의 tc/pid/run/scenario/step/action_seq를 연결하여 다른 실행의 값과 혼합하지 마십시오. 같은 행을 다시 읽으면 값이 같아야 합니다.
5. 해당 step의 실제 행 수와 required_checks가 정확히 같아야 합니다. 일반 rect 허용오차는0.001, 정수/bool/ID는 exact입니다. NaN/Inf, 누락/잘림, 중복 읽기로 채운 행 수, unexpected exception, crash, timeout, cleanup error는 FAIL입니다. `passed`만 믿지 말고 actual/expected/tolerance를 재계산하십시오.
6. 모든 step 후 `Next scenario`로 이동하고 모든 scenario를 실행하십시오. 마지막 `LR_RESULT`의 completed_scenarios=required_scenarios, failures=0 및 전체 행 증거가 필요합니다. 실패 후 Reset으로 실패 이력을 지우지 마십시오.

## 입력과 독립 기대값

N=16/128/512의3개 scenario에서 `sparse`, `dense`, `same-value`를 실행하십시오. Release OFF 필수 행 수는 step당217+4*N입니다.

Stack horizontal, i번째 x=10*i,width8,y0입니다. sparse는 중앙 item 한 개의 height만15/16으로 왕복하고 dense는 모든 item을 변경합니다. same-value는 이미16인 값을16으로 설정합니다. root Arrange height16에서 각 leaf의 요청 height가 유지되어야 합니다. 다른 leaf가 변경되면 FAIL입니다. 각 sample의 Measure 반환값, Arrange 반환값, measured slots와 최초 Actor geometry를 다음 Layout 호출 전에 검사합니다. `sample.immediate.*`의 scalar values는12+6*N, mismatch와 maximum_error는0이어야 합니다. 복구 Arrange 후의 결과로 최초 오류를 덮지 마십시오.

ON profile에서는 `work-sparse`, `work-dense`, `work-same-value`를 실행하고 step당42+4*N행을 확인하십시오. 초기 settle 후 변경 leaf 수 M은 sparse1, denseN, same-value0입니다. M>0일 때 Measure와 Arrange 각각 enter=N+1, producer=publish=M+1, hit=N−M입니다. replay=N−M, geometry write=M, invalidate-measure=2*M, ancestor-visit=2*M+1입니다. Stack Measure children/work와 Arrange children/work는 각각1회, 각N개이며 bytes는 N*sizeof(View) 또는 N*sizeof(MeasuredSize)입니다. M=0이면 Measure enter=hit1, Arrange enter=hit1, replay=N+1, VIEW_REPLAY_CHILDREN1회/요소N이며 producer/publish/write/invalidation/ancestor-visit는0입니다. detached workload의 wake/root-drain/park는 항상0입니다.

capture buffer overflow, 잘못된 node/epoch/sequence, 알려지지 않은 storage site는 FAIL입니다. storage 값은 계측된 명명 지점의 요청 횟수/요소/bytes이며 전체 heap allocation을 뜻하지 않습니다. ON budget과 OFF timing은 별도 실행하며 둘 다 있어야 최종 PASS입니다.

각 sample은 setter+Measure+Arrange를 포함하며 fixture 생성, geometry 검증, 로그는 시간 구간 밖입니다. 10회 warm-up 후31개 sample을 원문 보존하십시오. 같은 값 재설정의 불필요한 invalidation으로 시간이 늘어나는 경우도 별도 metric에서 검출하십시오. 시간은 Release OFF에서 실행하고 아래 비교 절차를 따르십시오. 반환값을 저장하는 현재 fixture revision으로 reference를 다시 수집하며, 이전 binary와 표본을 혼합하지 마십시오.

## 성능 비교

[성능 계획 예시](../layout-validation-performance-plan.example.json)를 실행 전에 복사하여 metric family/seed/alpha/precision/MDE/timer floor를 고정하십시오. 이 값은 candidate 결과를 본 후 완화하지 마십시오. 비교는 같은 device, affinity, governor, 해상도/DPI, resources, compiler flags를 사용한 reference/candidate의 독립 process AB/BA pair 단위입니다. 한 process의31개 내부 sample을 median으로 요약하고31→62→최대93 process pair에서 사전 고정한 family/반복 관측 보정 CI를 계산합니다.

`python3 layout-validation-tools.py compare PLAN.json EXPERIMENT.json`으로 저장된 표본을 분석하십시오. CI 하한>1이면 크기와 무관하게 FAIL입니다. 그 외 CI 상한≤1+MDE이고 half-width가 precision을 만족할 때에만 명시한 해상도에서 regression 미검출 PASS입니다. 최대 pair 이후에도 정밀도가 부족하면 INDETERMINATE입니다. MDE는 허용 slowdown 비율이 아닙니다. significant slowdown을 MDE 이하라고 통과시키지 마십시오.

애플리케이션 변경과 library patch 영향은 같은 workload 계약으로 app/reference-library, app/candidate-library를 비교하고, app 변경도 있다면 old/new app × old/new library의 네 조합을 기록하십시오. unavailable 조합은 분석 누락으로 표시하십시오. 단일 build의 시간만으로 regression PASS를 부여하지 마십시오.

## 종료와 증거

`< Back`으로 나간 뒤 다시 진입하여 첫 step을 재실행하십시오. 이전 timer/callback/fixture/추가 Window가 남아 있으면 FAIL입니다. 결과에는 scenario/input/step, 실제값과 기대값, 최초 실패 행, 전체 stdout, artifact fingerprint를 첨부하십시오. `python3 ../layout-validation-tools.py check-log LOG --tc LR60 --external`은 저장된 행의 누락과 수치를 다시 검사하는 보조 수단입니다. 이 도구는 앱을 실행하거나 서버의 UI 자동 조작을 대체하지 않습니다.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic`: 3 scenario, 9 action, 8250 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `n16` | `work-sparse` : 106 → `work-dense` : 106 → `work-same-value` : 106 |
| `n128` | `work-sparse` : 554 → `work-dense` : 554 → `work-same-value` : 554 |
| `n512` | `work-sparse` : 2090 → `work-dense` : 2090 → `work-same-value` : 2090 |

Profile `release`: 3 scenario, 9 action, 9825 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `n16` | `sparse` : 281 → `dense` : 281 → `same-value` : 281 |
| `n128` | `sparse` : 729 → `dense` : 729 → `same-value` : 729 |
| `n512` | `sparse` : 2265 → `dense` : 2265 → `same-value` : 2265 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR60 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
