# LR39. Layout: Transition bounds units and edges

이 문서는 [tc-lr39-layout-transition-bounds.cpp](tc-lr39-layout-transition-bounds.cpp)의 신규 TC만 실행하는 절차입니다. 이전 Layout TC의 실행 결과를 이 TC의 증거로 합산하지 마십시오.

## 실행 조건과 조작

1. 검증할 app와 실제로 resolve되는 library의 경로, revision, build-ID 및 build type(Debug/Release)을 기록하십시오. layout 관측 hook은 모든 빌드에 항상 포함되므로 별도의 diagnostics profile 전환은 없습니다.
2. 검증 artifact의 `manual-tests/dali-ui-foundation/bin/manual-test-dali-ui-foundation`을 실행하고 stdout을 손실 없이 저장하십시오. launcher에서 `LR39. Layout: Transition bounds units and edges`를 선택하십시오. 격리 build인 경우 해당 artifact 안의 binary를 실행하십시오.
3. `LR39.Reset run`을 한 번 눌러 새 run ID를 기록하십시오. 이후 실패를 지우기 위해 Reset하지 마십시오. 버튼 표시는 `Reset run`이며 Actor name과 accessibility name은 앞의 TC ID를 포함합니다.
4. 각 scenario에서 `LR39.Run scenario`를 누르면 남은 step이 순서대로 자동 실행됩니다(step 하나씩 진행하려면 `LR39.Run step`). `LR_ACTION`의 scenario/step/required_checks가 아래 순서와 일치하는지 먼저 확인하십시오. 각 step이 끝나면 `LR_READY`와 그 step의 모든 행이 자동 출력됩니다.
5. scene 단계는 해당 Window의 View target snapshot과 Window fence까지 기다립니다. 일반적으로 3초 안에 완료됩니다. 완료되지 않아도 조작하지 말고 framework의 자체 timeout을 기다리십시오: layout fence 10초 후 `observer.timeout`, fresh frame 4초 후 `render.frame-timeout`, render settle 4초 후 `render.settle-timeout` 행으로 실패가 기록됩니다. `LR_READY`가 도착하더라도 필수 행 누락이나 행 수 불일치는 PASS가 아닙니다. 그 전에 버튼을 누르면 `LR_NOOP`(step still running)만 남고 step은 계속 실행 중입니다. timeout 행이 나오면 실행 환경과 로그를 함께 보존하십시오. animation 자연 완료 단계에서는 CPP에 적힌 650~1000ms Delay 관측까지 기다리십시오.
6. 각 step의 모든 검사 행은 완료 시 stdout에 자동 출력되며 HUD에는 요약과 실패 행만 표시되므로 행 수와 숫자는 stdout에서 빠짐없이 수집하십시오. 모든 step이 끝난 뒤에만 `LR39.Next scenario`를 누르거나, `LR39.Run all`로 남은 scenario를 이어서 자동 실행하십시오. 총 scenario 수는 34개입니다. `Run all` 뒤에는 마지막 `LR_RESULT`의 `completed_scenarios`가 `required_scenarios`와 같아질 때까지 기다리십시오. 그 전의 `FAILING`/`INCOMPLETE`는 종결 verdict가 아니므로 실행을 중단하지 마십시오. 완료 후 `< Back`으로 나가 `LR_CASE_END`의 `completed_scenarios`가 `required_scenarios`와 같은지 확인하십시오.

## 단계와 필수 관측 수

각 scenario에서 `mount-parent` 4개 → `enter-seek-0/1/2/3` 각각 6개 → `enter-natural-finish` 6개 → `exit-seek-0/1/2/3` 각각 8개 → `exit-natural-finish` 4개 순서로 실행하십시오.

`Rect`는 x/y/width/height 네 행, `Size`는 width/height 두 행입니다. 일반 scalar/count/bool 검사 하나는 한 행입니다. `LR_CHECK`의 run/scenario/step/action_seq를 `LR_ACTION`과 대조하고 다음 action 시작 전까지의 로그 구간을 함께 저장하여 같은 이름의 행이 서로 다른 step에서 섞이지 않게 하십시오. `LR_RESULT`의 completed_scenarios/required_scenarios, rows와 page 범위를 함께 보관하십시오.

## 독립 기대값과 관측 의미

- slide는 edge 0/1/2/3=TOP/BOTTOM/LEFT/RIGHT, unit 0/1/2=PIXEL/SELF_FRACTION/PARENT_FRACTION, sign=-1/+1의 24조합입니다. PIXEL 거리는 17, fraction은 .25입니다. base B=(40,30,80,40), parent P=(300,200)입니다.

- offset의 양은 PIXEL=17, SELF=축 child 크기×.25, PARENT=축 parent 크기×.25입니다. TOP/LEFT 부호는 음, BOTTOM/RIGHT는 양이며 sign=-1이면 뒤집습니다. base에 offset만 더하여 endpoint를 직접 계산하십시오.

- Expand endpoint는 TOP (40,30,80,0), BOTTOM (40,70,80,0), LEFT (40,30,0,40), RIGHT (120,30,0,40)입니다. factor .5/1/1.5는 Y factor=.5, 중앙 anchor를 함께 사용합니다. mixed는 size(.5,1.5), anchor(1,.5), offset(PARENT .1, SELF -.5)이며 endpoint=(110,0,40,60)입니다.

- 실제 Animation을 pause/seek한 뒤 `Run::AfterFrame`이 해당 action의 새 `REFRESH_ONCE` render task 완료와 요청 progress 반영을 확인합니다. 그 다음 rendered 좌표를 독립 expected와 비교합니다. expected geometry 일치는 대기 조건이 아니므로 계산 오류를 기다림으로 숨기지 않습니다. 이 완료 신호는 scene 상태의 freshness를 확인하며 Window의 최종 화면 합성을 보장하지 않습니다. t=0/.25/.5/.75에서 ENTER는 endpoint+(B−endpoint)t, EXIT는 B+(endpoint−B)t입니다. 각 seek에 actual handle과 progress도 필수입니다. seek 뒤 진행률을 0으로 되돌려 실제 0.4초 animation을 재생하고 850ms 뒤 자연 완료를 관측합니다.

- EXIT 중 logical child count=1(남은 anchor sibling), Actor parent는 root를 유지해야 합니다. 자연 완료 뒤 unparent, start/finish 각 1, finish callback의 parent 존재 값=0이어야 합니다.

- `TR14.physical-left-rtl`은 RTL에서도 물리 LEFT를 사용합니다. visual base=(180,30,80,40), endpoint=(100,30,80,40), anchor sibling x=240입니다. 이 scenario는 ENTER/EXIT가 RTL을 두 번 적용하거나 논리 START로 바꾸는 오류를 검사합니다.

## Pass/Fail 판정

- 작은 target 좌표는 절대오차 0.001 이하, 실제 rendered animation 좌표는 0.01 이하입니다. progress는 해당 행의 tolerance(0.0001 또는 0.00001)를 사용하십시오. NaN/Inf는 크기와 무관하게 FAIL입니다. count/ID/bool/exception condition은 정확히 일치해야 합니다.
- `LR_CHECK`의 passed 표시만 믿지 말고 actual/expected/tolerance를 다시 대조하십시오. 같은 action에서 필요한 행 수보다 적거나 값이 잘렸거나 같은 행만 중복 수집되면 PASS로 처리하지 마십시오.
- 모든 필수 action이 실행되고 expected checks와 실제 checks가 일치하며, 전체 34개 scenario가 완료되고 누적 failures=0이어야 합니다. 마지막 페이지 또는 마지막 scenario만 PASS인 것은 TC PASS가 아닙니다.
- missing snapshot, observer overflow, 예상 밖 exception/crash/hang는 PASS가 아닙니다. 현재 제품 오류가 나타나도 expected를 actual로 바꾸거나 xfail로 제외하지 마십시오.
- HUD 갱신이 화면 layout을 발생시켜도 이전 step의 고정된 결과는 바뀌면 안 됩니다. 같은 run/step의 행이 다시 출력될 때 값이 바뀌면 observer 오류로 FAIL입니다.

## 정리와 재실행

launcher로 돌아가면 TC가 signal을 끊고 manual clock을 복원하며 animation, 추가 Window, fixture tree를 해제합니다. focus 변경 TC는 저장한 focus를 복원합니다. 다시 진입했을 때 초기 scenario/step과 새 run ID인지 확인하십시오. 이전 transition의 callback, 남은 ghost, 추가 Window, 이전 실패행이 새 run으로 유입되면 FAIL입니다. 전체 suite의 재진입 검증에서는 실제 launcher 진입→전체 실행→나가기 과정을 반복하고, 내부 subtree 재생성을 실제 진입/퇴장으로 대신 세지 마십시오.

## 관측 fixture와 HUD의 분리

bounds fixture root는 결과 Label의 View subtree가 아닌 **같은 실제 Window의 별도 최상위 root**입니다. 결과 Label 갱신의 invalidation이 paused ENTER child에 Arrange를 재실행하여 측정 대상을 바꾸지 않도록 합니다. 부모 300×200, child와 endpoint, native Animation 및 Window fence는 그대로 사용합니다. 독립 root는 `Run::AttachToWindow`로 fixture host의 현재 화면 위치(HUD 아래)에 배치되고, `SENSITIVE=false`여서 겹치는 화면 영역의 입력을 받지 않으며 focus를 바꾸지 않습니다. root는 TC cleanup에서 해제됩니다. geometry expected를 현재 값으로 수정하지 마십시오. HUD subtree에서 재현된 추가 reflow/transition 상호작용은 별도 integration 실패 증거로 보존하며, 이 분리만으로 그 상호작용이 해결되었다고 보고하지 마십시오.

## 조상 replay와 ENTER의 명시적 integration 재현

마지막 `TR14.ancestor-replay-during-enter`는 의도적으로 HUD와 같은 View tree에 root를 둡니다. TOP distance=-17의 endpoint는 `(40,47,80,40)`, 최종 base는 `(40,30,80,40)`입니다. `enter-seek-0` 다음에 `replay-ancestor`를 실행하십시오. 이 단계는 host의 Arrange를 invalidate하고 실제 Window controller를 수동 drain합니다. `ancestor-replay.capture.complete=true`, `ancestor-replay.child-arranged=true`로 대상 child의 Arrange/replay 도달을 확인하고 `Run::AfterFrame`의 새 render task 완료 후 `ancestor-replay.progress=0`과 `ancestor-replay.rendered.{x,y,width,height}=(40,47,80,40)`을 검사합니다. 필수 행 수는 7입니다.

이어서 원래 `enter-seek-1.3`, natural finish, EXIT의 네 seek와 natural finish를 그대로 실행하십시오. paused ENTER가 조상 reflow로 base에 덮이면 FAIL로 보존하십시오. 기본 33개 scenario를 독립 root로 분리한 결과가 이 integration FAIL을 면제하지 않습니다. 원래 bounds 기대값을 바꾸지 않습니다.

## 고정 실행 계약과 종료 증거

각 step 실행(`Run step`/`Run scenario`/`Run all`) 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 각 step의 전체 stdout 행은 완료 시 자동 출력되므로 그대로 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 34 scenario, 375 action, 2387 checks. 이 표에 없는 scenario는 컴파일된 계약에 선언되지 않으며 실행·판정 대상이 아닙니다.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `TR05.slide-e0-u0-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e0-u0-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e0-u1-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e0-u1-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e0-u2-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e0-u2-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e1-u0-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e1-u0-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e1-u1-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e1-u1-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e1-u2-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e1-u2-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e2-u0-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e2-u0-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e2-u1-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e2-u1-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e2-u2-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e2-u2-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e3-u0-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e3-u0-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e3-u1-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e3-u1-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e3-u2-s-1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.slide-e3-u2-s1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.expand-e0` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.expand-e1` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.expand-e2` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.expand-e3` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.factor-0.500000` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.factor-1.000000` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.factor-1.500000` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR05.mixed-units-anchor` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR14.physical-left-rtl` | `mount-parent` : 4 → `enter-seek-0` : 6 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |
| `TR14.ancestor-replay-during-enter` | `mount-parent` : 4 → `enter-seek-0` : 6 → `replay-ancestor` : 7 → `enter-seek-1` : 6 → `enter-seek-2` : 6 → `enter-seek-3` : 6 → `enter-natural-finish` : 6 → `exit-seek-0` : 8 → `exit-seek-1` : 8 → `exit-seek-2` : 8 → `exit-seek-3` : 8 → `exit-natural-finish` : 4 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`이고 `completed_scenarios == required_scenarios`이며 모든 count(failures 포함)가 마지막 `LR_RESULT`와 같아야 합니다. PASS는 그 완료 조건과 failures=0이 함께 성립할 때만입니다. `layout-validation-coverage.md`에 알려진 제품 결함 행이 문서화된 TC는 failures>0으로 종결되며, `check-log`가 `observations-valid-with-failures`로 그 행을 **정확히** 나열해야 합니다. 그 밖의 실패 행이나 `invalid-evidence`는 회귀 또는 무효 증거이며 PASS가 아닙니다. END 누락·cleanup 오류도 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR39 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. `--profile`의 `diagnostic`/`release` 두 값은 내용이 동일한 같은 계약을 가리키므로 어느 쪽을 지정해도 같은 결과입니다. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. `status`가 `observations-valid`면 실패 행이 없고, `observations-valid-with-failures`면 증거는 유효하며 `failing_rows`에 나열된 행만 실패했다는 뜻이고(exit 1), `invalid-evidence`면 증거 자체가 무효이므로 재실행해야 합니다(exit 2). `observer.timeout`은 layout 완료 관측 실패, `render.frame-timeout`은 새 render task 또는 입력 반영 완료 관측 실패, `render.settle-timeout`은 current geometry 반영 관측 실패로 분류하십시오. 이 과정에서 필수 관측 행이 누락되거나 실제 행 수가 계약과 달라지면 strict 검증 결과는 `invalid-evidence`(exit 2)입니다. 해당 observer 행, `protocol.required_checks`, 도구의 `error`, 원본 stdout/stderr를 함께 보존하십시오. 재실행은 새 run 증거로 별도 기록하고 첫 실패를 삭제하지 마십시오. timeout, 필수 행 누락, `LR_READY` 또는 END 누락을 **어떤 경우에도 PASS로 처리하지 마십시오**. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
