# LR37. Layout: Transition CHANGE causes and precedence

이 문서는 [tc-lr37-layout-transition-change-causes.cpp](tc-lr37-layout-transition-change-causes.cpp)의 신규 TC만 실행하는 절차입니다. 이전 Layout TC의 실행 결과를 이 TC의 증거로 합산하지 마십시오.

## 실행 조건과 조작

1. 검증할 app와 실제로 resolve되는 library의 경로, revision, build-ID 및 diagnostics profile을 기록하십시오. 일반 correctness 단계는 `DALI_UI_LAYOUT_TEST_DIAGNOSTICS`를 켠 app/library를 함께 사용하십시오.
2. 검증 artifact의 `manual-tests/dali-ui-foundation/bin/manual-test-dali-ui-foundation`을 실행하고 stdout을 손실 없이 저장하십시오. launcher에서 `LR37. Layout: Transition CHANGE causes and precedence`를 선택하십시오. 격리 build인 경우 해당 artifact 안의 binary를 실행하십시오.
3. `LR37.Reset run`을 한 번 눌러 새 run ID를 기록하십시오. 이후 실패를 지우기 위해 Reset하지 마십시오. 버튼 표시는 `Reset run`이며 Actor name과 accessibility name은 앞의 TC ID를 포함합니다.
4. 각 scenario에서 `LR37.Apply next step`을 한 번씩 누르십시오. `LR_ACTION`의 scenario/step/required_checks가 아래 순서와 일치하는지 먼저 확인하십시오. 다음 step을 누르기 전 `Read result`로 해당 action의 완료와 모든 행을 읽으십시오.
5. scene 단계는 해당 Window의 View target snapshot과 Window fence까지 기다립니다. 일반적으로 3초 안에 완료되어야 합니다. 본문에 더 긴 시간 표본 실행이 명시된 경우를 제외하고 5초가 지나도 PENDING이면 누락된 fence, 실행 환경과 로그를 저장하고 FAIL/실행 실패로 판정하십시오. animation 자연 완료 단계에서는 CPP에 적힌 650~1000ms Delay 관측까지 기다리십시오.
6. `Read result`는 해당 action의 모든 검사 행을 stdout에 출력합니다. stdout으로 행 수와 숫자를 빠짐없이 수집했으면 페이지 버튼을 전부 누를 필요는 없습니다. 화면만으로 관측하는 경우 `LR37.Next result page`로 모든 페이지를 읽고 필요하면 `Previous result page`로 돌아가십시오. `Next scenario`와 결과 페이지 버튼을 구분하십시오. 모든 step이 끝난 뒤에만 `LR37.Next scenario`를 누르십시오. 총 scenario 수는 11개입니다.

## 단계와 필수 관측 수

`TR03.cause-0`~`cause-6`: `mount` 4개 → `change` 14개입니다. `TR14.threshold-0.490000/0.500000/0.510000`: `mount` 4개 → `threshold` 5개입니다. `TR01.timing-enable-override`: `mount` 4개 → `default-disabled` 5개 → `override-enabled` 3개 → `override-cleared` 6개입니다.

`Rect`는 x/y/width/height 네 행, `Size`는 width/height 두 행입니다. 일반 scalar/count/bool 검사 하나는 한 행입니다. `LR_CHECK`의 run/scenario/step/action_seq를 `LR_ACTION`과 대조하고 다음 action 시작 전까지의 로그 구간을 함께 저장하여 같은 이름의 행이 서로 다른 step에서 섞이지 않게 하십시오. `LR_RESULT`의 completed_scenarios/required_scenarios, rows와 page 범위를 함께 보관하십시오.

## 독립 기대값과 관측 의미

- cause 0/1/2/3/4는 SIBLING_ADDED/SIBLING_REMOVED/REORDERED/WINDOW_RESIZED/OTHER이며 5는 resize opt-out, 6은 add/remove/reorder 동시 발생입니다. 순서 우선순위는 REORDERED가 가장 높습니다.

- 기존 child는 (0,20,80,40)이며 변경 폭은 120입니다. target y는 cause 1/2에서 0, cause 6에서 120, 나머지는 20입니다. context의 slot/cause/from/to를 독립 숫자표와 대조하십시오. 이 scenario의 resize는 공개 OnWindowResize(640,480) notification 경로이며 실제 OS Window resize는 LR45에서 따로 실행합니다.

- CHANGE threshold는 0.49/0.5 이동에서 start 0, 0.51에서 start 1입니다. target은 diagnostic logical bounds로 관측하며 Animator가 되돌린 event Actor bounds와 혼동하지 마십시오.

- default clear 뒤 x=80으로 즉시 settle합니다. OTHER override만 설정하면 실제 Animation duration=.6초/start=1이어야 합니다. override clear 후 x=160, duration=0/delay=10 후 x=200으로 즉시 settle하며 취소된 animation의 finish는 0입니다.

## Pass/Fail 판정

- 작은 target 좌표는 절대오차 0.001 이하, 실제 rendered animation 좌표는 0.01 이하입니다. progress는 해당 행의 tolerance(0.0001 또는 0.00001)를 사용하십시오. NaN/Inf는 크기와 무관하게 FAIL입니다. count/ID/bool/exception condition은 정확히 일치해야 합니다.
- `LR_CHECK`의 passed 표시만 믿지 말고 actual/expected/tolerance를 다시 대조하십시오. 같은 action에서 필요한 행 수보다 적거나 값이 잘렸거나 같은 행만 중복 수집되면 PASS로 처리하지 마십시오.
- 모든 필수 action이 실행되고 expected checks와 실제 checks가 일치하며, 전체 11개 scenario가 완료되고 누적 failures=0이어야 합니다. 마지막 페이지 또는 마지막 scenario만 PASS인 것은 TC PASS가 아닙니다.
- missing snapshot, diagnostics 미지원, observer overflow, 예상 밖 exception/crash/hang는 PASS가 아닙니다. 현재 제품 오류가 나타나도 expected를 actual로 바꾸거나 xfail로 제외하지 마십시오.
- `Read result`가 화면 layout을 발생시켜도 이전 action의 고정된 결과는 바뀌면 안 됩니다. 같은 run/action을 다시 읽어 값이 바뀌면 observer 오류로 FAIL입니다.

## 정리와 재실행

launcher로 돌아가면 TC가 signal을 끊고 manual clock을 복원하며 animation, 추가 Window, fixture tree를 해제합니다. focus 변경 TC는 저장한 focus를 복원합니다. 다시 진입했을 때 초기 scenario/step과 새 run ID인지 확인하십시오. 이전 transition의 callback, 남은 ghost, 추가 Window, 이전 실패행이 새 run으로 유입되면 FAIL입니다. 전체 suite의 재진입 검증에서는 실제 launcher 진입→전체 실행→나가기 과정을 반복하고, 내부 subtree 재생성을 실제 진입/퇴장으로 대신 세지 마십시오.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 11 scenario, 24 action, 171 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `TR03.cause-0` | `mount` : 4 → `change` : 14 |
| `TR03.cause-1` | `mount` : 4 → `change` : 14 |
| `TR03.cause-2` | `mount` : 4 → `change` : 14 |
| `TR03.cause-3` | `mount` : 4 → `change` : 14 |
| `TR03.cause-4` | `mount` : 4 → `change` : 14 |
| `TR03.cause-5` | `mount` : 4 → `change` : 14 |
| `TR03.cause-6` | `mount` : 4 → `change` : 14 |
| `TR14.threshold-0.490000` | `mount` : 4 → `threshold` : 5 |
| `TR14.threshold-0.500000` | `mount` : 4 → `threshold` : 5 |
| `TR14.threshold-0.510000` | `mount` : 4 → `threshold` : 5 |
| `TR01.timing-enable-override` | `mount` : 4 → `default-disabled` : 5 → `override-enabled` : 3 → `override-cleared` : 6 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR37 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
