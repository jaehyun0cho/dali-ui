# LR49. Layout: Controller rollback and reentry

이 문서는 [tc-lr49-layout-controller-rollback.cpp](tc-lr49-layout-controller-rollback.cpp)의 신규 TC만 실행하는 절차입니다. 이전 Layout TC의 실행 결과를 이 TC의 증거로 합산하지 마십시오.

## 실행 조건과 조작

1. 검증할 app와 실제로 resolve되는 library의 경로, revision, build-ID 및 diagnostics profile을 기록하십시오. 일반 correctness 단계는 `DALI_UI_LAYOUT_TEST_DIAGNOSTICS`를 켠 app/library를 함께 사용하십시오.
2. 검증 artifact의 `manual-tests/dali-ui-foundation/bin/manual-test-dali-ui-foundation`을 실행하고 stdout을 손실 없이 저장하십시오. launcher에서 `LR49. Layout: Controller rollback and reentry`를 선택하십시오. 격리 build인 경우 해당 artifact 안의 binary를 실행하십시오.
3. `LR49.Reset run`을 한 번 눌러 새 run ID를 기록하십시오. 이후 실패를 지우기 위해 Reset하지 마십시오. 버튼 표시는 `Reset run`이며 Actor name과 accessibility name은 앞의 TC ID를 포함합니다.
4. 각 scenario에서 `LR49.Apply next step`을 한 번씩 누르십시오. `LR_ACTION`의 scenario/step/required_checks가 아래 순서와 일치하는지 먼저 확인하십시오. 다음 step을 누르기 전 `Read result`로 해당 action의 완료와 모든 행을 읽으십시오.
5. scene 단계는 해당 Window의 View target snapshot과 Window fence까지 기다립니다. 일반적으로 3초 안에 완료되어야 합니다. 본문에 더 긴 시간 표본 실행이 명시된 경우를 제외하고 5초가 지나도 PENDING이면 누락된 fence, 실행 환경과 로그를 저장하고 FAIL/실행 실패로 판정하십시오. animation 자연 완료 단계에서는 CPP에 적힌 650~1000ms Delay 관측까지 기다리십시오.
6. `Read result`는 해당 action의 모든 검사 행을 stdout에 출력합니다. stdout으로 행 수와 숫자를 빠짐없이 수집했으면 페이지 버튼을 전부 누를 필요는 없습니다. 화면만으로 관측하는 경우 `LR49.Next result page`로 모든 페이지를 읽고 필요하면 `Previous result page`로 돌아가십시오. `Next scenario`와 결과 페이지 버튼을 구분하십시오. 모든 step이 끝난 뒤에만 `LR49.Next scenario`를 누르십시오. 총 scenario 수는 6개입니다.

## 단계와 필수 관측 수

`CT05.measure-throws/arrange-throws`: `mount` 4개 → `throw-and-recover` 8개 → `recovered-fence` 4개입니다. `CT05.nested-process`: `mount` 4개 → `nested-call` 7개입니다.

`Rect`는 x/y/width/height 네 행, `Size`는 width/height 두 행입니다. 일반 scalar/count/bool 검사 하나는 한 행입니다. `LR_CHECK`의 run/scenario/step/action_seq를 `LR_ACTION`과 대조하고 다음 action 시작 전까지의 로그 구간을 함께 저장하여 같은 이름의 행이 서로 다른 step에서 섞이지 않게 하십시오. `LR_RESULT`의 completed_scenarios/required_scenarios, rows와 page 범위를 함께 보관하십시오.

`CT05.remaining-root-rollback`는 `mount-boundary-roots` 8개 → `abort-earlier-root` 6개 → `drain-restored-batch` 10개입니다. `CT05.lifecycle-throws`는 `mount` 4개 → `throw-lifecycle` 5개 → `recover-lifecycle` 5개입니다.

`CT05.resize-flag-abort`는 `mount` 4개 → `abort-resize-then-change` 13개입니다.

## 독립 기대값과 관측 의미

- 합성 leaf의 Measure 또는 Arrange가 의도한 std::runtime_error를 던집니다. 메시지는 각각 `LR injected Measure failure` / `LR injected Arrange failure`여야 합니다.

- catch 직후 processDepth=0, manualProcessing=false, measureInProgress=false, arrangeInProgress=false이어야 하며 실패한 root의 pending work는 남아 있어야 합니다. throw flag 해제와 재처리 후 arrange cache가 복구되고 새 fence의 child=(65,30,80,40)을 확인하십시오.

- nested scenario는 Measure callback에서 같은 controller의 ProcessLayouts를 한 번 호출합니다. callback은 무한 재귀 없이 한 번 완료되고 logical bounds=(40,30,80,40), processDepth=0으로 복원되어야 합니다.

- remaining-root scenario에서는 먼저 처리되는 ancestor가 예외를 던졌을 때 뒤쪽 standalone root ID=5의 ROLLBACK_ROOT trace가 있어야 합니다. pending root가 2개 이상 유지되고, 복구 뒤 child=(40,30,80,40), boundary=(5,7,160,120), pending=0과 Window fence 내부 depth=1을 검사하십시오. 예외 catch 뒤의 depth=0 검사와 관측 시점이 다릅니다.

- OnStart가 `LR injected lifecycle failure`를 던지는 scenario에서도 depth=0, pending 유지, start=1이어야 합니다. 같은 event 안에서 throw를 해제하고 timing을 disable한 뒤 x90으로 처리하여 기존 animation을 취소합니다. 다음 action에서 x100으로 복구하며 취소된 transition의 finish=0이어야 합니다.

- resize-abort scenario는 공개 OnWindowResize notification 뒤 Measure 예외를 발생시킵니다. 다음 일반 변경의 callback cause는 WINDOW_RESIZED가 아닌 OTHER이어야 합니다. from=(40,30,80,40), to=(90,30,80,40), 아직 finish=0을 대조하십시오. 저장한 Window 크기 notification은 cleanup에서 복원합니다.

## Pass/Fail 판정

- 작은 target 좌표는 절대오차 0.001 이하, 실제 rendered animation 좌표는 0.01 이하입니다. progress는 해당 행의 tolerance(0.0001 또는 0.00001)를 사용하십시오. NaN/Inf는 크기와 무관하게 FAIL입니다. count/ID/bool/exception condition은 정확히 일치해야 합니다.
- `LR_CHECK`의 passed 표시만 믿지 말고 actual/expected/tolerance를 다시 대조하십시오. 같은 action에서 필요한 행 수보다 적거나 값이 잘렸거나 같은 행만 중복 수집되면 PASS로 처리하지 마십시오.
- 모든 필수 action이 실행되고 expected checks와 실제 checks가 일치하며, 전체 6개 scenario가 완료되고 누적 failures=0이어야 합니다. 마지막 페이지 또는 마지막 scenario만 PASS인 것은 TC PASS가 아닙니다.
- missing snapshot, diagnostics 미지원, observer overflow, 예상 밖 exception/crash/hang는 PASS가 아닙니다. 현재 제품 오류가 나타나도 expected를 actual로 바꾸거나 xfail로 제외하지 마십시오.
- `Read result`가 화면 layout을 발생시켜도 이전 action의 고정된 결과는 바뀌면 안 됩니다. 같은 run/action을 다시 읽어 값이 바뀌면 observer 오류로 FAIL입니다.

## 정리와 재실행

launcher로 돌아가면 TC가 signal을 끊고 manual clock을 복원하며 animation, 추가 Window, fixture tree를 해제합니다. focus 변경 TC는 저장한 focus를 복원합니다. 다시 진입했을 때 초기 scenario/step과 새 run ID인지 확인하십시오. 이전 transition의 callback, 남은 ghost, 추가 Window, 이전 실패행이 새 run으로 유입되면 FAIL입니다. 전체 suite의 재진입 검증에서는 실제 launcher 진입→전체 실행→나가기 과정을 반복하고, 내부 subtree 재생성을 실제 진입/퇴장으로 대신 세지 마십시오.

## remaining-root 예외 주입의 범위

`CT05.remaining-root-rollback`의 `abort-earlier-root`는 그 action의 수동 `ProcessLayouts()`에만 Measure 예외를 주입합니다. catch 직후 trace와 Window rollback snapshot을 복사하고 **같은 action에서 `throwMeasure=false`로 해제**합니다. `remaining.pending-retained`와 `remaining.depth.restored`는 이 고정 snapshot의 값을 검사합니다. 이후 `Read result`나 idle event가 복구 작업을 진행해도 주입 예외가 다시 발생해서는 안 됩니다.

다음 `drain-restored-batch`에서는 각 root를 명시적으로 invalidate한 뒤 실제 Window fence에서 두 target과 pending/depth를 검증합니다. 앞 action 종료 후 자연 복구가 이미 진행되었더라도 앞의 rollback snapshot을 새 상태로 덮지 않습니다. uncaught 주입 예외로 app가 종료되면 fixture 실행 FAIL이며, `LR_CASE_END` 없이 crash한 run을 완료로 처리하지 마십시오. action별 필수 개수와 98개 전체 검사는 변경하지 않습니다.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 6 scenario, 16 action, 98 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `CT05.measure-throws` | `mount` : 4 → `throw-and-recover` : 8 → `recovered-fence` : 4 |
| `CT05.arrange-throws` | `mount` : 4 → `throw-and-recover` : 8 → `recovered-fence` : 4 |
| `CT05.nested-process` | `mount` : 4 → `nested-call` : 7 |
| `CT05.remaining-root-rollback` | `mount-boundary-roots` : 8 → `abort-earlier-root` : 6 → `drain-restored-batch` : 10 |
| `CT05.lifecycle-throws` | `mount` : 4 → `throw-lifecycle` : 5 → `recover-lifecycle` : 5 |
| `CT05.resize-flag-abort` | `mount` : 4 → `abort-resize-then-change` : 13 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR49 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
