# LR59. Layout: Layout 계산 성능

이 문서는 [tc-lr59-layout-compute-performance.cpp](tc-lr59-layout-compute-performance.cpp)의 실행 절차입니다. 기존 Layout TC 결과는 합산하지 마십시오. 전체 실행 조건과 profile은 [suite 검증 기준](../layout-validation-coverage.md)을 먼저 읽으십시오.

## 실행 및 완료 확인

1. app와 실제 resolve된 library의 build-ID, revision, profile, asset SHA-256을 기록하고 stdout을 저장하십시오. launcher에서 `LR59`를 검색하여 해당 TC에 진입하십시오.
2. `LR59.Reset run`을 한 번 실행하고 새 `run`을 기록하십시오. scenario 내 step 사이에는 Reset하지 마십시오.
3. `Apply next step`을 한 번 누른 뒤 `LR_ACTION`의 scenario/step/action_seq/required_checks를 기록하십시오. 동기 step은 즉시 완료됩니다. Window/resource step은 해당 episode의 완료까지 기다리십시오. 10초 내 완료되지 않으면 PENDING을 PASS로 처리하지 마십시오. 성능 sample step만 최대120초를 허용합니다.
4. `Read result`를 누르고 `Next result page`로 **모든** 행을 읽으십시오. page1만 확인하지 마십시오. 행의 tc/pid/run/scenario/step/action_seq를 연결하여 다른 실행의 값과 혼합하지 마십시오. 같은 행을 다시 읽으면 값이 같아야 합니다.
5. 해당 step의 실제 행 수와 required_checks가 정확히 같아야 합니다. 일반 rect 허용오차는0.001, 정수/bool/ID는 exact입니다. NaN/Inf, 누락/잘림, 중복 읽기로 채운 행 수, unexpected exception, crash, timeout, cleanup error는 FAIL입니다. `passed`만 믿지 말고 actual/expected/tolerance를 재계산하십시오.
6. 모든 step 후 `Next scenario`로 이동하고 모든 scenario를 실행하십시오. 마지막 `LR_RESULT`의 completed_scenarios=required_scenarios, failures=0 및 전체 행 증거가 필요합니다. 실패 후 Reset으로 실패 이력을 지우지 마십시오.

## 입력과 독립 기대값

각 Stack/Flex/Grid/Absolute × N=16/128/512, 총12개 scenario에서 `construction`, `first_measure`, `first_arrange`, `warm_measure`, `warm_arrange`를 순서대로 실행하십시오. Release OFF step당 필수 행 수는 217+4*N입니다.

fixture의 i번째 leaf는 (10*i,0,8,16), root는 (10*N−2,16)입니다. 10회 warm-up 후31개 sample마다 모든 leaf의 finite/수치 오차를 검사하며 마지막 sample은 모든 leaf의 x/y/width/height도 개별 출력합니다. 검사나 출력 시간은 측정 구간에 포함하지 않습니다. 각 Measure의 반환 width/height는 고정 buffer에 저장하여 모두 검증하고, Arrange의 반환 rect 및 직후 root/leaf Actor geometry를 다음 Layout 호출 전에 검증합니다. first/warm Arrange 뒤에는 복구용 Measure/Arrange를 실행하지 않습니다. construction과 Measure의 후속 geometry 검사에만 별도 Arrange를 사용하며, 원래 반환값 검사가 먼저 끝나야 합니다. `sample.immediate.*`의 values/mismatches/maximum_error 세 행도 필수입니다.

ON profile에서는 같은 12개 scenario마다 `work-first_measure`, `work-first_arrange`, `work-warm_measure`, `work-warm_arrange` 4개 step을 실행하십시오. 행 수는 step당39+4*N입니다. warm step은8회 반복하고 timing sample을 만들지 않습니다. 다음 독립 budget을 exact로 검사합니다. 표에 없는 work count는0이어야 합니다.

| phase | Measure enter/producer/hit/publish | Arrange enter/producer/hit/publish | replay | ancestor visit |
|---|---|---|---|---|
| first Measure | N+1/N+1/0/N+1 | 0/0/0/0 |0|N|
| first Arrange | Grid만 N/0/N/0, 나머지0 | N+1/N+1/0/N+1 |0|0|
| warm Measure ×8 |8/0/8/0|0/0/0/0|0|0|
| warm Arrange ×8 |0/0/0/0|8/0/8/0|8*(N+1)|0|

first Arrange geometry write는 실행 전 Actor rect와 독립 expected rect의 exact component 차이 개수입니다. 다른 phase는 write0이며 detached workload에서 wake/root-drain/park는 모두0입니다. storage는 first Measure에서 Stack children+work, Flex children, Grid children+rows+columns, Absolute children 각1회입니다. first Arrange에서 Stack/Flex children+work, Grid/Absolute children 각1회입니다. 각 children/work의 element 합계는N, Grid rows1/columnsN이며 bytes는 해당 C++ element sizeof와의 곱입니다. warm Arrange는 VIEW_REPLAY_CHILDREN8회, elements8*N이고 warm Measure는0입니다. 표시된4개 storage slot의 calls/elements/bytes와 unexpected-sites0을 모두 검증하십시오.

이 storage 검사는 instrumentation이 붙은 명명된 reserve/vector 지점의 요청량입니다. 전체 allocator 호출 횟수·전체 heap bytes라고 해석하지 마십시오. buffer overflow, node ID 누락, epoch/sequence 불일치도 FAIL입니다. ON exact budget 실패를 OFF 시간 차이가 작다는 이유로 무시하지 마십시오.

construction/first_measure/first_arrange는 sample당1회, warm_measure4096회, warm_arrange32회입니다. `nanoseconds/operations`로 계산하십시오. 다른 phase, manager, N의 표본을 합치지 마십시오. 같은 process 내31개 sample을31개의 독립 process pair로 잘못 계산하지 마십시오. 반환값 buffer 저장이 추가된 현재 fixture revision으로 reference를 다시 수집하십시오. 이전에 반환값을 버리던 binary의 baseline과 혼합하지 마십시오.

시간 측정은 반드시 Release OFF app/library에서 실행하십시오. 최종 PASS에는 같은 fixture의 ON exact-budget 결과와 OFF 비교 결과가 모두 필요합니다. diagnostics/coverage/Debug 빌드에서는 시간 회귀를 판정하지 마십시오. 화면 EXTERNAL_REQUIRED는 내부 geometry 검사가 끝났다는 상태입니다. 아래 공통 성능 절차에 따른 기준 대비 검정이 끝나야 PASS를 부여할 수 있습니다.

## 성능 비교

[성능 계획 예시](../layout-validation-performance-plan.example.json)를 실행 전에 복사하여 metric family/seed/alpha/precision/MDE/timer floor를 고정하십시오. 이 값은 candidate 결과를 본 후 완화하지 마십시오. 비교는 같은 device, affinity, governor, 해상도/DPI, resources, compiler flags를 사용한 reference/candidate의 독립 process AB/BA pair 단위입니다. 한 process의31개 내부 sample을 median으로 요약하고31→62→최대93 process pair에서 사전 고정한 family/반복 관측 보정 CI를 계산합니다.

`python3 layout-validation-tools.py compare PLAN.json EXPERIMENT.json`으로 저장된 표본을 분석하십시오. CI 하한>1이면 크기와 무관하게 FAIL입니다. 그 외 CI 상한≤1+MDE이고 half-width가 precision을 만족할 때에만 명시한 해상도에서 regression 미검출 PASS입니다. 최대 pair 이후에도 정밀도가 부족하면 INDETERMINATE입니다. MDE는 허용 slowdown 비율이 아닙니다. significant slowdown을 MDE 이하라고 통과시키지 마십시오.

애플리케이션 변경과 library patch 영향은 같은 workload 계약으로 app/reference-library, app/candidate-library를 비교하고, app 변경도 있다면 old/new app × old/new library의 네 조합을 기록하십시오. unavailable 조합은 분석 누락으로 표시하십시오. 단일 build의 시간만으로 regression PASS를 부여하지 마십시오.

## 종료와 증거

`< Back`으로 나간 뒤 다시 진입하여 첫 step을 재실행하십시오. 이전 timer/callback/fixture/추가 Window가 남아 있으면 FAIL입니다. 결과에는 scenario/input/step, 실제값과 기대값, 최초 실패 행, 전체 stdout, artifact fingerprint를 첨부하십시오. `python3 ../layout-validation-tools.py check-log LOG --tc LR59 --external`은 저장된 행의 누락과 수치를 다시 검사하는 보조 수단입니다. 이 도구는 앱을 실행하거나 서버의 UI 자동 조작을 대체하지 않습니다.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic`: 12 scenario, 48 action, 43856 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `stack-n16` | `work-first_measure` : 103 → `work-first_arrange` : 103 → `work-warm_measure` : 103 → `work-warm_arrange` : 103 |
| `stack-n128` | `work-first_measure` : 551 → `work-first_arrange` : 551 → `work-warm_measure` : 551 → `work-warm_arrange` : 551 |
| `stack-n512` | `work-first_measure` : 2087 → `work-first_arrange` : 2087 → `work-warm_measure` : 2087 → `work-warm_arrange` : 2087 |
| `flex-n16` | `work-first_measure` : 103 → `work-first_arrange` : 103 → `work-warm_measure` : 103 → `work-warm_arrange` : 103 |
| `flex-n128` | `work-first_measure` : 551 → `work-first_arrange` : 551 → `work-warm_measure` : 551 → `work-warm_arrange` : 551 |
| `flex-n512` | `work-first_measure` : 2087 → `work-first_arrange` : 2087 → `work-warm_measure` : 2087 → `work-warm_arrange` : 2087 |
| `grid-n16` | `work-first_measure` : 103 → `work-first_arrange` : 103 → `work-warm_measure` : 103 → `work-warm_arrange` : 103 |
| `grid-n128` | `work-first_measure` : 551 → `work-first_arrange` : 551 → `work-warm_measure` : 551 → `work-warm_arrange` : 551 |
| `grid-n512` | `work-first_measure` : 2087 → `work-first_arrange` : 2087 → `work-warm_measure` : 2087 → `work-warm_arrange` : 2087 |
| `absolute-n16` | `work-first_measure` : 103 → `work-first_arrange` : 103 → `work-warm_measure` : 103 → `work-warm_arrange` : 103 |
| `absolute-n128` | `work-first_measure` : 551 → `work-first_arrange` : 551 → `work-warm_measure` : 551 → `work-warm_arrange` : 551 |
| `absolute-n512` | `work-first_measure` : 2087 → `work-first_arrange` : 2087 → `work-warm_measure` : 2087 → `work-warm_arrange` : 2087 |

Profile `release`: 12 scenario, 60 action, 65500 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `stack-n16` | `construction` : 281 → `first_measure` : 281 → `first_arrange` : 281 → `warm_measure` : 281 → `warm_arrange` : 281 |
| `stack-n128` | `construction` : 729 → `first_measure` : 729 → `first_arrange` : 729 → `warm_measure` : 729 → `warm_arrange` : 729 |
| `stack-n512` | `construction` : 2265 → `first_measure` : 2265 → `first_arrange` : 2265 → `warm_measure` : 2265 → `warm_arrange` : 2265 |
| `flex-n16` | `construction` : 281 → `first_measure` : 281 → `first_arrange` : 281 → `warm_measure` : 281 → `warm_arrange` : 281 |
| `flex-n128` | `construction` : 729 → `first_measure` : 729 → `first_arrange` : 729 → `warm_measure` : 729 → `warm_arrange` : 729 |
| `flex-n512` | `construction` : 2265 → `first_measure` : 2265 → `first_arrange` : 2265 → `warm_measure` : 2265 → `warm_arrange` : 2265 |
| `grid-n16` | `construction` : 281 → `first_measure` : 281 → `first_arrange` : 281 → `warm_measure` : 281 → `warm_arrange` : 281 |
| `grid-n128` | `construction` : 729 → `first_measure` : 729 → `first_arrange` : 729 → `warm_measure` : 729 → `warm_arrange` : 729 |
| `grid-n512` | `construction` : 2265 → `first_measure` : 2265 → `first_arrange` : 2265 → `warm_measure` : 2265 → `warm_arrange` : 2265 |
| `absolute-n16` | `construction` : 281 → `first_measure` : 281 → `first_arrange` : 281 → `warm_measure` : 281 → `warm_arrange` : 281 |
| `absolute-n128` | `construction` : 729 → `first_measure` : 729 → `first_arrange` : 729 → `warm_measure` : 729 → `warm_arrange` : 729 |
| `absolute-n512` | `construction` : 2265 → `first_measure` : 2265 → `first_arrange` : 2265 → `warm_measure` : 2265 → `warm_arrange` : 2265 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR59 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
