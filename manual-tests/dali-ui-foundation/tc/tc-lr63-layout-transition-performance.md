# LR63. Layout: Transition work and timing regression

이 문서는 [tc-lr63-layout-transition-performance.cpp](tc-lr63-layout-transition-performance.cpp)의 신규 TC만 실행하는 절차입니다. 이전 Layout TC의 실행 결과를 이 TC의 증거로 합산하지 마십시오.

## 실행 조건과 조작

1. 검증할 app와 실제로 resolve되는 library의 경로, revision, build-ID 및 diagnostics profile을 기록하십시오. 일반 correctness 단계는 `DALI_UI_LAYOUT_TEST_DIAGNOSTICS`를 켠 app/library를 함께 사용하십시오. 성능 시간 표본은 별도 Release OFF artifact를 사용하십시오.
2. 검증 artifact의 `manual-tests/dali-ui-foundation/bin/manual-test-dali-ui-foundation`을 실행하고 stdout을 손실 없이 저장하십시오. launcher에서 `LR63. Layout: Transition work and timing regression`를 선택하십시오. 격리 build인 경우 해당 artifact 안의 binary를 실행하십시오.
3. `LR63.Reset run`을 한 번 눌러 새 run ID를 기록하십시오. 이후 실패를 지우기 위해 Reset하지 마십시오. 버튼 표시는 `Reset run`이며 Actor name과 accessibility name은 앞의 TC ID를 포함합니다.
4. 각 scenario에서 `LR63.Apply next step`을 한 번씩 누르십시오. `LR_ACTION`의 scenario/step/required_checks가 아래 순서와 일치하는지 먼저 확인하십시오. 다음 step을 누르기 전 `Read result`로 해당 action의 완료와 모든 행을 읽으십시오.
5. scene 단계는 해당 Window의 View target snapshot과 Window fence까지 기다립니다. 일반적으로 3초 안에 완료되어야 합니다. 본문에 더 긴 시간 표본 실행이 명시된 경우를 제외하고 5초가 지나도 PENDING이면 누락된 fence, 실행 환경과 로그를 저장하고 FAIL/실행 실패로 판정하십시오. animation 자연 완료 단계에서는 CPP에 적힌 650~1000ms Delay 관측까지 기다리십시오.
6. `Read result`는 해당 action의 모든 검사 행을 stdout에 출력합니다. stdout으로 행 수와 숫자를 빠짐없이 수집했으면 페이지 버튼을 전부 누를 필요는 없습니다. 화면만으로 관측하는 경우 `LR63.Next result page`로 모든 페이지를 읽고 필요하면 `Previous result page`로 돌아가십시오. `Next scenario`와 결과 페이지 버튼을 구분하십시오. 모든 step이 끝난 뒤에만 `LR63.Next scenario`를 누르십시오. 총 scenario 수는 15개입니다.

## 단계와 필수 관측 수

mode 0~4 × node count N=1/32/128의 15개 scenario를 모두 실행하십시오. profile마다 `mount`를 실행한 뒤 ON에서는 `work-count`, OFF에서는 `release-samples`를 실행하십시오. 필수 행 수는 다음과 같습니다.

| N | `mount` | ON `work-count` | OFF `release-samples` |
| --- | ---: | ---: | ---: |
| 1 | 7 | 10 | 226 |
| 32 | 162 | 165 | 5186 |
| 128 | 642 | 645 | 20546 |

전체 leaf geometry block은 `5×N+2`행입니다. 두 개의 count 행과 각 leaf의 identity/order 한 행, target x/y/width/height 네 행으로 구성됩니다. `mount`는 block 한 개, ON은 block 한 개와 trace 세 행입니다. OFF는 warm-up 한 block과 sample 0~30의 31개 block 및 최종 sample/operation count 두 행입니다. N=128에서도 mount와 OFF action의 누적 21188행이 모두 필요합니다. stdout 전체를 수집하는 방법을 사용하십시오.

`Rect`는 x/y/width/height 네 행, `Size`는 width/height 두 행입니다. 일반 scalar/count/bool 검사 하나는 한 행입니다. `LR_CHECK`의 run/scenario/step/action_seq를 `LR_ACTION`과 대조하고 다음 action 시작 전까지의 로그 구간을 함께 저장하여 같은 이름의 행이 서로 다른 step에서 섞이지 않게 하십시오. `LR_RESULT`의 completed_scenarios/required_scenarios, rows와 page 범위를 함께 보관하십시오.

## 독립 기대값과 관측 의미

- mode 0=transition attachment 없음, 1=붙어 있지만 입력 불변인 dormant, 2=CHANGE disabled, 3=active Spec, 4=active Animator입니다. mode 0은 process 전체에 dispatcher가 한 번도 생성되지 않았다는 주장이 아닙니다.

- `mount`는 첫 child만 관측하지 않습니다. N개 leaf 전부를 required target set으로 등록하고 같은 Window fence에서 snapshot을 받습니다. 각 leaf i(0≤i<N)의 target은 `(x=0, y=i, width=80, height=1)`이고 logical child 수와 저장 handle 수가 N이며 `GetChildViewAt(i)`가 생성한 i번째 leaf와 같아야 합니다.

- ON trace에서 transition begin count는 mode 3/4만 N과 같아야 하고 0/1/2는 0입니다. manual tick 이전 TRANSITION_TICK trace 수=0, trace overflow=false도 필수입니다. trace 수집을 닫은 직후 다음 Layout이나 timer 처리를 하지 않고 전체 leaf target을 검사합니다. update mode 0/2/3/4는 `(0,i,81,1)`, dormant mode 1은 `(0,i,80,1)`이어야 합니다. `performance.work.handles`, `.logical-children`, `.leaf-i.identity-order`와 각 `.target` 네 행을 모두 대조하십시오.

- OFF timing은 화면 조작 지연이 아니라 내부 steady_clock 구간입니다. 10회 warm-up 뒤 모든 leaf의 target `(0,i,80,1)`과 count/identity를 먼저 검사하고 31개 raw sample을 기록합니다. update mode 0/2/3/4는 표본당 9번, dormant mode 1은 표본당 128번 ProcessLayouts를 실행합니다. update의 requested width는 81/80을 왕복합니다. 홀수 9회이므로 sample의 마지막 입력은 직전 sample과 반드시 다릅니다. 업데이트를 전혀 수행하지 않은 구현이 초기 폭 80을 보존하여 통과하는 것을 막습니다. 기존 8회 workload 표본과 섞어 비교하지 마십시오.

- timer 종료 직후 각 `performance.sample-j` block에서 모든 leaf의 독립 기대값을 확인합니다. update mode의 sample j가 짝수(0,2,...,30)이면 `(0,i,81,1)`, 홀수이면 `(0,i,80,1)`입니다. dormant는 모든 sample에서 `(0,i,80,1)`입니다. 각 sample의 `handles=N`, `logical-children=N`, 모든 `.leaf-i.identity-order=1`도 필수입니다. expected width는 sample 순서에서 정하며 candidate가 반환한 width나 상태 counter로 생성하지 않습니다. 검사와 raw sample 출력은 timer 밖에 있고, 다음 Update/ProcessLayouts/Delay 전에 수행됩니다. 뒤의 정상 sample이 앞의 실패를 상쇄하지 않습니다.

- 이 target 검사는 기존 public `ViewImpl::GetArrangedBounds()`의 const getter를 사용합니다. getter는 마지막 Arrange의 logical 결과를 복사하며 Arrange, event flush, animation seek, animation 완료 대기를 수행하지 않습니다. LTR, scale 1인 이 fixture에서는 logical target과 visual target이 같습니다. Spec은 진행 중인 Actor geometry를 되돌려 animate하고, Animator callback은 의도적으로 Actor geometry를 쓰지 않으므로 현재 Actor 좌표를 final Layout target으로 판정하면 잘못된 결과가 됩니다. LR63은 native Spec seek/최종 rendered geometry 성공을 주장하지 않습니다. 해당 기능의 별도 신규 correctness TC는 그대로 필수입니다. sample 뒤 재배치나 seek/완료 대기로 결과를 복구한 다음 값을 읽는 방법은 이 성능 oracle의 증거로 사용할 수 없습니다.

- `LR_SAMPLE`의 tc/pid/run/scenario/step/action_seq/profile을 `LR_ACTION` 및 binary fingerprint와 연결해 보관하십시오. `nanoseconds/operations`로 정규화하되 metric 이름은 `LR63.mode{mode}.n{count}.update.ns` 또는 `.dormant.ns`입니다. 다른 mode/N을 합치지 마십시오. 동일 compiler flags, device, app/library revision 및 build-ID, fixture를 가진 reference/candidate를 AB/BA 순서로 비교하십시오.

- candidate 실행 전에 paired log-ratio CI 방식, alpha와 family multiplicity 보정, 순서 seed, precision/MDE/timer floor, 31→최대93 paired sample 상한을 manifest에 고정하십시오. 유의한 slowdown의 CI 하한>0이면 크기에 관계없이 FAIL입니다. 그 외 quality/precision을 만족하고 CI 상한≤MDE일 때만 명시한 검출력 내 regression 미검출 PASS입니다. 정밀도 부족은 상한까지 재측정 후 미결입니다.

- geometry/identity/count 행이 하나라도 실패한 sample은 빠른 시간값을 성능 PASS 근거로 사용할 수 없습니다. 같은 sample index의 `LR_SAMPLE`과 `performance.sample-j` 전체 검사 행을 연결하십시오. 화면의 EXTERNAL_REQUIRED는 성능 PASS가 아닙니다. baseline 또는 ON work-count 증거 또는 OFF sample 또는 fingerprints가 빠지면 최종 PASS를 부여하지 마십시오. old app×old library/new app×old library/old app×new library/new app×new library의 네 artifact를 별도 실행하여 애플리케이션 구현과 library 변경 영향을 구분하십시오.

## Pass/Fail 판정

- 모든 leaf target은 x=0, y=0~127, width=80 또는 81, height=1 범위이며 절대오차 0.001 이하만 통과합니다. 0.01 이상의 오차는 검출해야 합니다. NaN/Inf는 크기와 무관하게 FAIL입니다. count/identity/order는 정확히 일치해야 합니다. 이 TC는 intermediate rendered animation 좌표나 progress를 판정하지 않습니다.
- `LR_CHECK`의 passed 표시만 믿지 말고 actual/expected/tolerance를 다시 대조하십시오. 같은 action에서 필요한 행 수보다 적거나 값이 잘렸거나 같은 행만 중복 수집되면 PASS로 처리하지 마십시오.
- 모든 필수 action이 실행되고 expected checks와 실제 checks가 일치하며, 전체 15개 scenario가 완료되고 누적 failures=0이어야 합니다. 마지막 페이지 또는 마지막 scenario만 PASS인 것은 TC PASS가 아닙니다. ON/OFF 실행 및 외부 통계 판정까지 모두 충족해야 최종 성능 PASS입니다.
- missing snapshot, diagnostics 미지원, observer overflow, 예상 밖 exception/crash/hang는 PASS가 아닙니다. 현재 제품 오류가 나타나도 expected를 actual로 바꾸거나 xfail로 제외하지 마십시오.
- `Read result`가 화면 layout을 발생시켜도 이전 action의 고정된 결과는 바뀌면 안 됩니다. 같은 run/action을 다시 읽어 값이 바뀌면 observer 오류로 FAIL입니다.

## 정리와 재실행

launcher로 돌아가면 TC가 signal을 끊고 manual clock을 복원하며 animation, 추가 Window, fixture tree를 해제합니다. focus 변경 TC는 저장한 focus를 복원합니다. 다시 진입했을 때 초기 scenario/step과 새 run ID인지 확인하십시오. 이전 transition의 callback, 남은 ghost, 추가 Window, 이전 실패행이 새 run으로 유입되면 FAIL입니다. 전체 suite의 재진입 검증에서는 실제 launcher 진입→전체 실행→나가기 과정을 반복하고, 내부 subtree 재생성을 실제 진입/퇴장으로 대신 세지 마십시오.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic`: 15 scenario, 30 action, 8155 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `TR15.mode-0-n1` | `mount` : 7 → `work-count` : 10 |
| `TR15.mode-0-n32` | `mount` : 162 → `work-count` : 165 |
| `TR15.mode-0-n128` | `mount` : 642 → `work-count` : 645 |
| `TR15.mode-1-n1` | `mount` : 7 → `work-count` : 10 |
| `TR15.mode-1-n32` | `mount` : 162 → `work-count` : 165 |
| `TR15.mode-1-n128` | `mount` : 642 → `work-count` : 645 |
| `TR15.mode-2-n1` | `mount` : 7 → `work-count` : 10 |
| `TR15.mode-2-n32` | `mount` : 162 → `work-count` : 165 |
| `TR15.mode-2-n128` | `mount` : 642 → `work-count` : 645 |
| `TR15.mode-3-n1` | `mount` : 7 → `work-count` : 10 |
| `TR15.mode-3-n32` | `mount` : 162 → `work-count` : 165 |
| `TR15.mode-3-n128` | `mount` : 642 → `work-count` : 645 |
| `TR15.mode-4-n1` | `mount` : 7 → `work-count` : 10 |
| `TR15.mode-4-n32` | `mount` : 162 → `work-count` : 165 |
| `TR15.mode-4-n128` | `mount` : 642 → `work-count` : 645 |

Profile `release`: 15 scenario, 30 action, 133845 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `TR15.mode-0-n1` | `mount` : 7 → `release-samples` : 226 |
| `TR15.mode-0-n32` | `mount` : 162 → `release-samples` : 5186 |
| `TR15.mode-0-n128` | `mount` : 642 → `release-samples` : 20546 |
| `TR15.mode-1-n1` | `mount` : 7 → `release-samples` : 226 |
| `TR15.mode-1-n32` | `mount` : 162 → `release-samples` : 5186 |
| `TR15.mode-1-n128` | `mount` : 642 → `release-samples` : 20546 |
| `TR15.mode-2-n1` | `mount` : 7 → `release-samples` : 226 |
| `TR15.mode-2-n32` | `mount` : 162 → `release-samples` : 5186 |
| `TR15.mode-2-n128` | `mount` : 642 → `release-samples` : 20546 |
| `TR15.mode-3-n1` | `mount` : 7 → `release-samples` : 226 |
| `TR15.mode-3-n32` | `mount` : 162 → `release-samples` : 5186 |
| `TR15.mode-3-n128` | `mount` : 642 → `release-samples` : 20546 |
| `TR15.mode-4-n1` | `mount` : 7 → `release-samples` : 226 |
| `TR15.mode-4-n32` | `mount` : 162 → `release-samples` : 5186 |
| `TR15.mode-4-n128` | `mount` : 642 → `release-samples` : 20546 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR63 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
