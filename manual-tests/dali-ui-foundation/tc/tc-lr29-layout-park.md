# LR29. Layout: Parked invalidation

이 TC는 신규 Layout suite의 **Parked invalidation** 검증입니다. 대응 소스는 `tc-lr29-layout-park.cpp`입니다. 기존 TC의 실행 결과를 사용하지 않습니다.

## 실행 조건

- foundation manual-test 앱에서 `LR29`를 검색하고 표시명이 일치하는 TC에 진입하십시오.
- 다른 입력·animation을 중지하고 `Reset run`을 한 번 실행하십시오. 첫 scenario와 새로운 `run_id`를 기록하십시오.
- 기본 수치는 scale=1, LTR, parent-local 좌표이며 좌표·extent 단위는 visual unit입니다. 각 scenario가 scale/direction을 바꾸면 아래 명시값을 사용하십시오.
- 수치 actual은 TC의 결과 행 또는 같은 run의 stdout 원문에서 읽으십시오. screenshot에서 위치를 눈대중으로 추정하지 마십시오.
- layout 관측 hook은 모든 빌드에 포함되므로 별도의 capability 준비가 필요하지 않습니다. required row가 누락되면 PASS로 처리하지 마십시오.
- 화면 조건: 앱 window는 480×800 이상이어야 하며 fixture stage는 HUD 아래 fixture 영역에 렌더링됩니다. 큰 fixture는 window 밖으로 이어질 수 있으며 판정은 렌더 좌표(screen extents)로 수행하므로 잘린 화면을 FAIL 근거로 삼지 마십시오.

## 조작 및 판정

1. TC 진입 시 run이 자동으로 시작되며 HUD 첫 줄에 `scenario k/N`, `steps i/M`, 다음 step ID가 표시됩니다. 처음부터 다시 시작하려면 `Reset run`을 누르십시오. `Run scenario`는 현재 scenario의 남은 step을 순서대로 자동 실행하고, `Run step`은 다음 step 하나만 실행하며, `Run all`은 남은 scenario 전체를 자동 실행합니다.
2. 각 step의 관측 경로를 따르십시오. `AfterLayout`은 LayoutController pass와 필수 View의 완료 fence를 기다립니다. `AfterRender`는 그 뒤 새 render task의 완료와 current geometry 반영을 기다립니다. 기본 제한은 layout fence 10초, fresh frame 4초, render settle 4초이며 각각 `observer.timeout`, `render.frame-timeout`, `render.settle-timeout`으로 실패를 기록합니다. 실패 시 마지막 출력·화면을 함께 보존하십시오. 특별한 quiet/시간 조건이 있는 step은 `Run step`으로 하나씩 실행하며 아래 지시가 우선합니다.
3. 각 step이 끝나면 `LR_READY`에 이어 그 step의 `LR_RESULT`와 모든 `LR_CHECK` 행이 stdout에 자동 출력되고 HUD에 요약과 실패 행이 표시됩니다. 별도의 읽기 조작은 없습니다.
4. `required_checks`와 실행 검사 수가 같고 0보다 크며, 모든 필수 행의 actual/expected/오차가 일치해야 해당 step을 통과시킵니다. 누적 실패가 하나라도 있으면 전체 TC를 PASS로 기록하지 마십시오.
5. scenario가 끝나면 `Next scenario`로 이동해 반복하거나 `Run all`로 남은 scenario 전체를 자동 실행하십시오. 생략한 scenario를 통과로 추정하지 마십시오. `Run all` 뒤에는 마지막 `LR_RESULT`의 `completed_scenarios`가 `required_scenarios`와 같아질 때까지 기다리십시오. 그 전의 `FAILING`/`INCOMPLETE`는 종결 verdict가 아니므로 실행을 중단하지 마십시오. 완료 후 `< Back`으로 나가 `LR_CASE_END`의 `completed_scenarios`가 `required_scenarios`와 같은지 확인하십시오.
6. 끝에서 run/scenario/step, 필수·실행 행 수, 모든 실패 행, stdout 증거를 저장하십시오.

## 시나리오와 독립 기대값

`K17.passive-park`는 관측 hook을 사용합니다. PARK의 arm/quiet 구간에는 TC 내부 timer가 없습니다. 복구 후 scheduler 관측에는 아래 명시한 일회성 timer만 사용하며 polling하지 않습니다.

1. `prepare`를 실행하고 37×19 snapshot을 확인하십시오.
2. `arm-passive-observation`을 실행하십시오. producer가 자기 Measure를 invalidate하므로 pending work는 남아야 합니다. 이 action은 8행입니다. 바깥 InvalidateMeasure는 정상적인 event-time wake를 예약할 수 있고 수동 ProcessLayouts는 그 예약을 소비하지 않으므로 manual-wake-preserved는 처리 전후 wakeArmed가 같음을 검사합니다. 외부 invalidation 뒤부터 수동 처리 종료까지만 trace하여 in-pass-request-parked=true, no-in-pass-wake-request=0, capture.no-overflow=true를 확인하십시오. 이 시점에 wakeArmed를 무조건 false로 요구하지 않습니다. stdout의 `LR29_PARK producer=N`과 `LR29_PARK_ARMED`를 읽으십시오. 이 step은 자동 결과 출력을 억제하므로 8행은 quiet 구간이 끝난 뒤 다음 버튼(`Run step`)을 누를 때 `LR_ACTION`보다 먼저 stdout에 나옵니다. 이 TC는 반드시 `Run step`으로 step 하나씩 진행하고 `Run scenario`/`Run all`을 쓰지 마십시오. 이 TC에 한해 producer callback의 최소 stdout·flush를 허용하며 시간 benchmark에는 사용하지 않습니다.
3. 버튼 release를 포함한 입력이 모두 끝난 뒤 stdout만 수동으로 읽어 마지막 byte offset O0와 producer C0를 기록하십시오. 화면 조회·screenshot·pointer 이동·버튼 입력·주기 polling을 하지 않고 **10초** 기다리십시오. OS 로그 파일을 다시 읽어 O1/C1을 기록하십시오. 이 구간에 새 `LR29_PARK producer` 행이 없어야 하며 C1=C0여야 합니다. 다른 앱 또는 서버 동작이 ProcessEvents를 발생시킨 증거가 있으면 해당 구간을 버리고 새 quiet 구간을 측정하십시오.
4. quiet 구간이 끝난 뒤 `external-event-and-stop`을 실행하십시오. 이 버튼 press/release가 만든 pass는 quiet 구간에 포함하지 마십시오. 명시 외부 invalidation과 ProcessLayouts에 대해 after>before여야 합니다. 정확히 1 증가를 요구하지 않습니다.
5. 마지막 recover-window-and-drain을 실행하십시오. 이 단계는 25행입니다. callback이 disarm/제거된 상태에서 AfterLayout을 먼저 arm하고 InvalidateMeasure만 호출합니다. ProcessLayouts를 수동 호출하지 않으므로 자연 Window 완료가 10초 안에 도착해야 합니다. recovery.geometry=(0,0,37,19), Window valid/natural-process, pending roots/completions=0, clean episode, child valid/Measure cache/Arrange cache/clean 상태, PARK producer counter 불변을 확인하십시오.
6. 자연 fence 이후 100ms의 일회성 관측에서 drained.pending-roots/pending-completions/process-depth=0, wake-not-armed/not-manual/clean-episode/window-valid/not-saturated=true 및 PARK producer 불변을 확인하십시오. 이 timer는 callback을 제거하고 자연 Window 완료가 도착한 다음에만 생성되며 quiet interval의 무입력 증거에 포함하지 않습니다. Window fence 순간에는 이미 예약된 idle wake가 아직 남아 있을 수 있으므로 최종 wake 해제는 이 후속 관측에서 판정합니다. 누락된 geometry 또는 남은 scheduler 작업을 STOP 로그만으로 PASS 처리하지 마십시오.
7. O0/O1, C0/C1, 서버의 monotonic 시작·끝 시각, 최소 10초 길이와 입력 기록, STOP before/after를 저장하십시오. 자동 행의 PASS만으로 전체 PASS를 기록하지 마십시오. 외부 quiet 증거까지 충족한 경우에만 서버가 PASS를 판정합니다. 외부 관측을 수행하지 않았으면 미실행입니다.

필수 step 순서는 prepare(4행) → arm-passive-observation(8행) → external-event-and-stop(5행) → recover-window-and-drain(25행)입니다. 전체 자동 관측 42행과 외부 quiet 증거가 모두 필요합니다. 복구 단계의 결과도 완료 시 자동 출력되는 행으로 수집하십시오.

4096 producer 행을 넘으면 saturation FAIL이며 TC를 종료하십시오. `< Back`/Reset은 먼저 callback을 disarm하고 제거합니다.

`Rect`는 x/y/width/height의 네 필수 행, `Size`는 width/height의 두 필수 행입니다. 일반 geometry 허용오차는 0.001이며 정수·순서·bool·별도로 지정한 exact 항목은 정확히 일치해야 합니다. NaN/Inf는 일반 geometry 비교에서 실패합니다. expected는 입력 표·식에서 계산하며 candidate 출력으로 갱신하지 마십시오.

## 근거와 검증 경계

PARK는 in-processing invalidation을 버리지 않으면서 독자 wake를 재등록하지 않는 계약입니다. layout-controller.cpp의 ProcessLayouts()/ManualProcessScope는 이미 예약된 wake를 보존하며, RequestIdleWakeIfAllowed()의 in-pass 분기는 PARKED_REQUEST를 남기고 새 wake 요청을 하지 않습니다. 기존 wake가 자연 처리에서 소비된 뒤의 무입력 동작은 10초 quiet interval로 별도 검증합니다. 외부 입력으로 진행하는 것과 자기 wake loop를 구분합니다.

`Snapshot`은 `LayoutController`가 `LayoutFinished`로 보고한 최종 parent-local target bounds(post-RTL, pre-transition)입니다. `GetArrangedBounds()`의 logical rect(pre-RTL)와 구분하십시오. 행 이름에 `logical`이나 `arranged`가 포함되어도 좌표 의미는 관측 API를 기준으로 판단하십시오. `Rendered` 행(id.x/.y/.width/.height, `*.rendered.*` 포함)은 current scene-graph 상태에서 계산한 화면 사각형을 부모 또는 명시한 base 기준으로 관측한 값이며 RTL 적용 후 좌표입니다. `GetMeasuredSize`는 해당 pass의 측정 결과입니다. controller 완료 snapshot, transition 중간 current geometry, Window의 최종 화면 합성은 서로 다른 관측입니다. callback 측정·배치 횟수는 controller pass가 호출한 synthetic producer의 횟수입니다.

## 종료와 재현

`< Back`으로 나간 뒤 같은 TC에 다시 진입하고 `Reset run` 이후 첫 scenario를 반복하십시오. run ID·counter가 새로 시작하고 초기 기대값이 같아야 합니다. 실패는 scenario/input/action/actual/expected를 함께 보존하십시오. 기존 Layout TC 실행으로 대체하지 마십시오.

arm step은 자동 결과 출력과 HUD 갱신을 억제합니다. LR29_PARK_ARMED 이후 quiet interval이 끝날 때까지 어떤 버튼도 누르지 마십시오. quiet 종료 후 `Run step`을 누르면 억제되었던 arm step의 행이 먼저 출력되고 다음 step이 시작됩니다.

## 고정 실행 계약과 종료 증거

각 step 실행(`Run step`/`Run scenario`/`Run all`) 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 각 step의 전체 stdout 행은 완료 시 자동 출력되므로 그대로 보존하십시오(arm step의 행만 quiet 구간 뒤 다음 `Run step`에서 출력됩니다). stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 1 scenario, 4 action, 42 checks. 이 표에 없는 scenario는 컴파일된 계약에 선언되지 않으며 실행·판정 대상이 아닙니다.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `K17.passive-park` | `prepare` : 4 → `arm-passive-observation` : 8 → `external-event-and-stop` : 5 → `recover-window-and-drain` : 25 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`이고 `completed_scenarios == required_scenarios`이며 모든 count(failures 포함)가 마지막 `LR_RESULT`와 같아야 합니다. PASS는 그 완료 조건과 failures=0이 함께 성립할 때만입니다. `layout-validation-coverage.md`에 알려진 제품 결함 행이 문서화된 TC는 failures>0으로 종결되며, `check-log`가 `observations-valid-with-failures`로 그 행을 **정확히** 나열해야 합니다. 그 밖의 실패 행이나 `invalid-evidence`는 회귀 또는 무효 증거이며 PASS가 아닙니다. END 누락·cleanup 오류도 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR29 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. `--profile`의 `diagnostic`/`release` 두 값은 내용이 동일한 같은 계약을 가리키므로 어느 쪽을 지정해도 같은 결과입니다. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. `status`가 `observations-valid`면 실패 행이 없고, `observations-valid-with-failures`면 증거는 유효하며 `failing_rows`에 나열된 행만 실패했다는 뜻이고(exit 1), `invalid-evidence`면 증거 자체가 무효이므로 재실행해야 합니다(exit 2). `observer.timeout`은 layout 완료 관측 실패, `render.frame-timeout`은 새 render task 또는 입력 반영 완료 관측 실패, `render.settle-timeout`은 current geometry 반영 관측 실패로 분류하십시오. 이 과정에서 필수 관측 행이 누락되거나 실제 행 수가 계약과 달라지면 strict 검증 결과는 `invalid-evidence`(exit 2)입니다. 해당 observer 행, `protocol.required_checks`, 도구의 `error`, 원본 stdout/stderr를 함께 보존하십시오. 재실행은 새 run 증거로 별도 기록하고 첫 실패를 삭제하지 마십시오. timeout, 필수 행 누락, `LR_READY` 또는 END 누락을 **어떤 경우에도 PASS로 처리하지 마십시오**. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
