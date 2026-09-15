# LR29. Layout: Parked invalidation

이 TC는 신규 Layout suite의 **Parked invalidation** 검증입니다. 대응 소스는 `tc-lr29-layout-park.cpp`입니다. 기존 TC의 실행 결과를 사용하지 않습니다.

## 실행 조건

- foundation manual-test 앱에서 `LR29`를 검색하고 표시명이 일치하는 TC에 진입하십시오.
- 다른 입력·animation을 중지하고 `Reset run`을 한 번 실행하십시오. 첫 scenario와 새로운 `run_id`를 기록하십시오.
- 기본 수치는 scale=1, LTR, parent-local 좌표이며 좌표·extent 단위는 visual unit입니다. 각 scenario가 scale/direction을 바꾸면 아래 명시값을 사용하십시오.
- 수치 actual은 TC의 결과 행 또는 같은 run의 stdout 원문에서 읽으십시오. screenshot에서 위치를 눈대중으로 추정하지 마십시오.
- diagnostics가 필요한 단계는 diagnostics ON build가 필요합니다. capability가 없거나 required row가 누락되면 PASS로 처리하지 마십시오.

## 조작 및 판정

1. 현재 `scenario_id`와 `action_seq`를 기록하고 `Apply next step`을 한 번 누르십시오.
2. 동기 계산은 같은 action에서 끝납니다. Window 단계는 해당 action의 View snapshot과 Window 완료 fence를 기다립니다. 10초 안에 완료되지 않으면 FAIL과 마지막 출력·화면을 기록하십시오. 특별한 quiet/시간 조건은 아래에서 우선합니다.
3. `Read result`를 누르고 해당 action의 전체 결과를 읽으십시오. `Next result page`는 결과 페이지 이동이며 `Next scenario`와 다릅니다.
4. `required_checks`와 실행 검사 수가 같고 0보다 크며, 모든 필수 행의 actual/expected/오차가 일치해야 해당 action을 통과시킵니다. 누적 실패가 하나라도 있으면 전체 TC를 PASS로 기록하지 마십시오.
5. 단계가 더 있으면 `Apply next step`을 누르십시오. scenario 완료 후 `Next scenario`로 이동하여 목록의 모든 scenario를 실행하십시오. 생략한 scenario를 통과로 추정하지 마십시오.
6. 끝에서 run/scenario/action, 필수·실행 행 수, 모든 실패 행, stdout 또는 결과 화면 증거를 저장하십시오.

## 시나리오와 독립 기대값

`K17.passive-park`는 diagnostics ON에서 실행하십시오. PARK의 arm/quiet 구간에는 TC 내부 timer가 없습니다. 복구 후 scheduler 관측에는 아래 명시한 일회성 timer만 사용하며 polling하지 않습니다.

1. `prepare`를 실행하고37×19 snapshot을 확인하십시오.
2. `arm-passive-observation`을 실행하십시오. producer가 자기 Measure를 invalidate하므로 pending work는 남아야 합니다. 이 action은8행입니다. 바깥 InvalidateMeasure는 정상적인 event-time wake를 예약할 수 있고 수동 ProcessLayouts는 그 예약을 소비하지 않으므로 manual-wake-preserved는 처리 전후 wakeArmed가 같음을 검사합니다. 외부 invalidation 뒤부터 수동 처리 종료까지만 trace하여 in-pass-request-parked=true, no-in-pass-wake-request=0, capture.no-overflow=true를 확인하십시오. 이 시점에 wakeArmed를 무조건 false로 요구하지 않습니다. stdout의 `LR29_PARK producer=N`과 `LR29_PARK_ARMED`를 읽으십시오. 이 TC에 한해 producer callback의 최소 stdout·flush를 허용하며 시간 benchmark에는 사용하지 않습니다.
3. 버튼 release를 포함한 입력이 모두 끝난 뒤 stdout만 수동으로 읽어 마지막 byte offset O0와 producer C0를 기록하십시오. 화면 조회·screenshot·pointer 이동·Read result·주기 polling을 하지 않고 **10초** 기다리십시오. OS 로그 파일을 다시 읽어 O1/C1을 기록하십시오. 이 구간에 새 `LR29_PARK producer` 행이 없어야 하며 C1=C0여야 합니다. 다른 앱 또는 서버 동작이 ProcessEvents를 발생시킨 증거가 있으면 해당 구간을 버리고 새 quiet 구간을 측정하십시오.
4. quiet 구간이 끝난 뒤 `external-event-and-stop`을 실행하십시오. 이 버튼 press/release가 만든 pass는 quiet 구간에 포함하지 마십시오. 명시 외부 invalidation과 ProcessLayouts에 대해 after>before여야 합니다. 정확히1 증가를 요구하지 않습니다.
5. 마지막 recover-window-and-drain을 실행하십시오. 이 단계는25행입니다. callback이 disarm/제거된 상태에서 AfterLayout을 먼저 arm하고 InvalidateMeasure만 호출합니다. ProcessLayouts를 수동 호출하지 않으므로 자연 Window 완료가10초 안에 도착해야 합니다. recovery.geometry=(0,0,37,19), Window valid/natural-process, pending roots/completions=0, clean episode, child valid/Measure cache/Arrange cache/clean 상태, PARK producer counter 불변을 확인하십시오.
6. 자연 fence 이후100ms의 일회성 관측에서 drained.pending-roots/pending-completions/process-depth=0, wake-not-armed/not-manual/clean-episode/window-valid/not-saturated=true 및 PARK producer 불변을 확인하십시오. 이 timer는 callback을 제거하고 자연 Window 완료가 도착한 다음에만 생성되며 quiet interval의 무입력 증거에 포함하지 않습니다. Window fence 순간에는 이미 예약된 idle wake가 아직 남아 있을 수 있으므로 최종 wake 해제는 이 후속 관측에서 판정합니다. 누락된 geometry 또는 남은 scheduler 작업을 STOP 로그만으로 PASS 처리하지 마십시오.
7. O0/O1, C0/C1, 서버의 monotonic 시작·끝 시각, 최소10초 길이와 입력 기록, STOP before/after를 저장하십시오. 자동 행의 PASS만으로 전체 PASS를 기록하지 마십시오. 외부 quiet 증거까지 충족한 경우에만 서버가 PASS를 판정합니다. 외부 관측을 수행하지 않았으면 미실행입니다.

필수 step 순서는 prepare(4행) → arm-passive-observation(8행) → external-event-and-stop(5행) → recover-window-and-drain(25행)입니다. 전체 자동 관측42행과 외부 quiet 증거가 모두 필요합니다. 복구 단계의 결과도 Read result로 수집하십시오.

4096 producer 행을 넘으면 saturation FAIL이며 TC를 종료하십시오. `< Back`/Reset은 먼저 callback을 disarm하고 제거합니다.

`Rect`는 x/y/width/height의 네 필수 행, `Size`는 width/height의 두 필수 행입니다. 일반 geometry 허용오차는 0.001이며 정수·순서·bool·별도로 지정한 exact 항목은 정확히 일치해야 합니다. NaN/Inf는 일반 geometry 비교에서 실패합니다. expected는 입력 표·식에서 계산하며 candidate 출력으로 갱신하지 마십시오.

## 근거와 검증 경계

PARK는 in-processing invalidation을 버리지 않으면서 독자 wake를 재등록하지 않는 계약입니다. layout-controller.cpp의 ProcessLayouts()/ManualProcessScope는 이미 예약된 wake를 보존하며, RequestIdleWakeIfAllowed()의 in-pass 분기는 PARKED_REQUEST를 남기고 새 wake 요청을 하지 않습니다. 기존 wake가 자연 처리에서 소비된 뒤의 무입력 동작은10초 quiet interval로 별도 검증합니다. 외부 입력으로 진행하는 것과 자기 wake loop를 구분합니다.

직접 `Measure`/`Arrange`의 반환과 Actor event-side target을 검사합니다. `Arrange` 반환은 pre-RTL logical이고 child Actor 좌표는 부모의 RTL 적용 결과입니다. Window snapshot은 실제 settle 결과이며 animation 중간 화면과 혼동하지 마십시오. callback 측정 횟수는 명시한 synthetic producer의 횟수입니다.

## 종료와 재현

`< Back`으로 나간 뒤 같은 TC에 다시 진입하고 `Reset run` 이후 첫 scenario를 반복하십시오. run ID·counter가 새로 시작하고 초기 기대값이 같아야 합니다. 실패는 scenario/input/action/actual/expected를 함께 보존하십시오. 기존 Layout TC 실행으로 대체하지 마십시오.

arm action은 자동 결과 Label 갱신을 억제합니다. LR29_PARK_ARMED 이후 quiet interval이 끝날 때까지 Read result도 누르지 마십시오. quiet 종료 후 Read result로 해당 action의 모든 행을 수집하십시오.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 1 scenario, 4 action, 42 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `K17.passive-park` | `prepare` : 4 → `arm-passive-observation` : 8 → `external-event-and-stop` : 5 → `recover-window-and-drain` : 25 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR29 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
