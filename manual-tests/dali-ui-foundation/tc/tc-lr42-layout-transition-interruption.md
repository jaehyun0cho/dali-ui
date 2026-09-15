# LR42. Layout: Transition interruption and continuity

이 문서는 [tc-lr42-layout-transition-interruption.cpp](tc-lr42-layout-transition-interruption.cpp)의 신규 TC만 실행하는 절차입니다. 이전 Layout TC의 실행 결과를 이 TC의 증거로 합산하지 마십시오.

## 실행 조건과 조작

1. 검증할 app와 실제로 resolve되는 library의 경로, revision, build-ID 및 diagnostics profile을 기록하십시오. 일반 correctness 단계는 `DALI_UI_LAYOUT_TEST_DIAGNOSTICS`를 켠 app/library를 함께 사용하십시오.
2. 검증 artifact의 `manual-tests/dali-ui-foundation/bin/manual-test-dali-ui-foundation`을 실행하고 stdout을 손실 없이 저장하십시오. launcher에서 `LR42. Layout: Transition interruption and continuity`를 선택하십시오. 격리 build인 경우 해당 artifact 안의 binary를 실행하십시오.
3. `LR42.Reset run`을 한 번 눌러 새 run ID를 기록하십시오. 이후 실패를 지우기 위해 Reset하지 마십시오. 버튼 표시는 `Reset run`이며 Actor name과 accessibility name은 앞의 TC ID를 포함합니다.
4. 각 scenario에서 `LR42.Apply next step`을 한 번씩 누르십시오. `LR_ACTION`의 scenario/step/required_checks가 아래 순서와 일치하는지 먼저 확인하십시오. 다음 step을 누르기 전 `Read result`로 해당 action의 완료와 모든 행을 읽으십시오.
5. scene 단계는 해당 Window의 View target snapshot과 Window fence까지 기다립니다. 일반적으로 3초 안에 완료되어야 합니다. 본문에 더 긴 시간 표본 실행이 명시된 경우를 제외하고 5초가 지나도 PENDING이면 누락된 fence, 실행 환경과 로그를 저장하고 FAIL/실행 실패로 판정하십시오. animation 자연 완료 단계에서는 CPP에 적힌 650~1000ms Delay 관측까지 기다리십시오.
6. `Read result`는 해당 action의 모든 검사 행을 stdout에 출력합니다. stdout으로 행 수와 숫자를 빠짐없이 수집했으면 페이지 버튼을 전부 누를 필요는 없습니다. 화면만으로 관측하는 경우 `LR42.Next result page`로 모든 페이지를 읽고 필요하면 `Previous result page`로 돌아가십시오. `Next scenario`와 결과 페이지 버튼을 구분하십시오. 모든 step이 끝난 뒤에만 `LR42.Next scenario`를 누르십시오. 총 scenario 수는 6개입니다.

## 단계와 필수 관측 수

`TR08.same-slot-0`~`3`: `mount` 4개 → `first-quarter` 6개 → `replace` 11개 → `complete-replacement` 3개입니다. `TR08.enter-to-exit/change-to-exit`: `mount` 4개 → `cross-slot` 7개 → `exit-finish` 3개입니다.

`Rect`는 x/y/width/height 네 행, `Size`는 width/height 두 행입니다. 일반 scalar/count/bool 검사 하나는 한 행입니다. `LR_CHECK`의 run/scenario/step/action_seq를 `LR_ACTION`과 대조하고 다음 action 시작 전까지의 로그 구간을 함께 저장하여 같은 이름의 행이 서로 다른 step에서 섞이지 않게 하십시오. `LR_RESULT`의 completed_scenarios/required_scenarios, rows와 page 범위를 함께 보관하십시오.

## 독립 기대값과 관측 의미

- mode의 bit0는 최초 Animator, bit1는 replacement Animator입니다. 나머지는 실제 Spec입니다. 최초 x40→140의 .25 지점은 x65입니다. replacement x240의 from은 x65, to는 x240이어야 합니다.

- Animator callback은 좌표를 쓰지 않으므로 replacement가 engine lastLerped를 사용했는지 판정할 수 있습니다. Spec은 actual rendered x65를 pause/seek 후 관측합니다. replacement start 누계=2, 취소된 finish=0, 자연 완료 뒤 finish 누계=1이어야 합니다.

- ENTER→EXIT의 current base는 (40,30,80,40), CHANGE→EXIT는 (65,30,80,40)입니다. 이전 slot finish는 끝까지 0, EXIT finish=1, 최종 parent는 비어야 합니다.

## Pass/Fail 판정

- 작은 target 좌표는 절대오차 0.001 이하, 실제 rendered animation 좌표는 0.01 이하입니다. progress는 해당 행의 tolerance(0.0001 또는 0.00001)를 사용하십시오. NaN/Inf는 크기와 무관하게 FAIL입니다. count/ID/bool/exception condition은 정확히 일치해야 합니다.
- `LR_CHECK`의 passed 표시만 믿지 말고 actual/expected/tolerance를 다시 대조하십시오. 같은 action에서 필요한 행 수보다 적거나 값이 잘렸거나 같은 행만 중복 수집되면 PASS로 처리하지 마십시오.
- 모든 필수 action이 실행되고 expected checks와 실제 checks가 일치하며, 전체 6개 scenario가 완료되고 누적 failures=0이어야 합니다. 마지막 페이지 또는 마지막 scenario만 PASS인 것은 TC PASS가 아닙니다.
- missing snapshot, diagnostics 미지원, observer overflow, 예상 밖 exception/crash/hang는 PASS가 아닙니다. 현재 제품 오류가 나타나도 expected를 actual로 바꾸거나 xfail로 제외하지 마십시오.
- `Read result`가 화면 layout을 발생시켜도 이전 action의 고정된 결과는 바뀌면 안 됩니다. 같은 run/action을 다시 읽어 값이 바뀌면 observer 오류로 FAIL입니다.

## 정리와 재실행

launcher로 돌아가면 TC가 signal을 끊고 manual clock을 복원하며 animation, 추가 Window, fixture tree를 해제합니다. focus 변경 TC는 저장한 focus를 복원합니다. 다시 진입했을 때 초기 scenario/step과 새 run ID인지 확인하십시오. 이전 transition의 callback, 남은 ghost, 추가 Window, 이전 실패행이 새 run으로 유입되면 FAIL입니다. 전체 suite의 재진입 검증에서는 실제 launcher 진입→전체 실행→나가기 과정을 반복하고, 내부 subtree 재생성을 실제 진입/퇴장으로 대신 세지 마십시오.

## Native interruption 관측과 HUD 분리

모든 LR42 fixture는 결과 Label의 View subtree 밖에 있는 같은 실제 Window의 최상위 root입니다. root의 `SENSITIVE=false`로 겹치는 영역의 입력을 받지 않으며, focus를 바꾸지 않습니다. `Read result`의 Label invalidation이 paused CHANGE 좌표를 재배치하여 다음 replacement의 from을 바꾸지 않도록 합니다. 이 TC의 6개 scenario와 124개 assertion, quarter-point expected x=65는 그대로 유지합니다. 공통 `NewState`/`Mount` 및 focus TC의 전제는 변경하지 않습니다.

HUD와 같은 조상에서 실제 replay가 paused transition을 덮는 원래 상호작용은 LR39 `TR14.ancestor-replay-during-enter`에서 명시적으로 재현합니다. LR42의 독립 fixture 통과를 그 integration 문제의 해결로 간주하지 마십시오.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 6 scenario, 22 action, 124 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `TR08.same-slot-0` | `mount` : 4 → `first-quarter` : 6 → `replace` : 11 → `complete-replacement` : 3 |
| `TR08.same-slot-1` | `mount` : 4 → `first-quarter` : 6 → `replace` : 11 → `complete-replacement` : 3 |
| `TR08.same-slot-2` | `mount` : 4 → `first-quarter` : 6 → `replace` : 11 → `complete-replacement` : 3 |
| `TR08.same-slot-3` | `mount` : 4 → `first-quarter` : 6 → `replace` : 11 → `complete-replacement` : 3 |
| `TR08.change-to-exit` | `mount` : 4 → `cross-slot` : 7 → `exit-finish` : 3 |
| `TR08.enter-to-exit` | `mount` : 4 → `cross-slot` : 7 → `exit-finish` : 3 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR42 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
