# LR62. Layout: Diagnostics observation contract

이 TC는 새 Layout suite가 사용하는 diagnostics 관측 API 자체를 검증합니다. 대응 source는 `tc-lr62-layout-diagnostics-contract.cpp`입니다. 기존 Layout TC는 실행 전제에 포함하지 않습니다.

## 실행 조건과 준비

- layout 관측 hook은 library와 manual-test executable 모두에 항상 포함됩니다. 별도로 켜야 하는 build 옵션은 없습니다. 실제 로드한 library 경로·revision·빌드 옵션과 executable revision을 증거에 기록하십시오. 기록한 library가 실제로 실행에 resolve된 파일인지 확인하십시오.
- foundation 앱에서 `LR62. Layout: Diagnostics observation contract`에 진입하여 `Reset run`을 한 번 누르십시오. 새 `run_id`를 기록하십시오. 아래 21개 scenario, 총 34개 action, 1513개 필수 assertion을 실행합니다.
- `Run scenario`(또는 step 하나씩은 `Run step`)를 누르고 완료될 때까지 추가 입력을 보내지 마십시오. D05는 실제 View snapshot과 Window 완료 fence를 기다리고, D06 두 번째 action은 복원된 timer의 자연 종료를 800ms 후 관측합니다. 각 action이 10초 내 완료되지 않으면 timeout FAIL입니다.
- 각 step 완료 시 모든 행이 자동 출력됩니다. scenario가 끝나면 `Next scenario`로 이동하거나 `Run all`로 이어서 실행하십시오. `Run all` 뒤에는 마지막 `LR_RESULT`의 `completed_scenarios`가 `required_scenarios`와 같아질 때까지 기다리십시오. 그 전의 `FAILING`/`INCOMPLETE`는 종결 verdict가 아니므로 실행을 중단하지 마십시오. 완료 후 `< Back`으로 나가 `LR_CASE_END`의 `completed_scenarios`가 `required_scenarios`와 같은지 확인하십시오.
- stdout의 `LR_ACTION`·`LR_CHECK`·`LR_RESULT` 또는 같은 run의 화면 결과를 저장하십시오. `run`, `scenario`, `step`, `action_seq`가 일치하는 행끼리만 비교하십시오. screenshot의 위치나 글자 모양을 수치 expected로 사용하지 마십시오.

## Scenario와 pass/fail 기준

| Scenario / action | 필수 행 수 | 독립 invariant와 관측값 |
| --- | ---: | --- |
| `D00.profile-always-on` | 8 | 관측 hook이 모든 빌드에 존재하면서 capture가 열리지 않은 동안에는 아무것도 기록하지 않음을 검증합니다. scenario 시작 시 `GetCaptureStatus()`의 active는 false입니다(overflow는 앞선 scenario가 의도적으로 세워둘 수 있어 절대값으로 판정하지 않고 아래 delta 비교에 포함합니다). 등록하지 않은 View의 snapshot nodeId는 0입니다. capture를 열지 않은 상태로 fixture 생성과 완전한 layout pass를 수행한 뒤 capture status가 시작 시 값과 **모든 필드에서 동일**해야 합니다(counter는 BeginCapture만 초기화하므로 절대 0이 아니라 delta 0을 판정합니다. 따라서 이 scenario는 앞선 scenario 실행 순서와 무관합니다). 작업을 끼우지 않은 Begin/End 왕복은 count=0, totalEvents=0, overflow=false, active=false, epoch=6209, capacity=16으로 끝납니다. InvalidateArrange 한 번을 감싼 capture는 totalEvents>0, count>0을 산출해 hook 분기가 살아 있고 dead-strip되지 않았음을 증명합니다(작은 16 event buffer가 넘칠 수도 있으므로 overflow는 이 판정에 포함하지 않습니다 — overflow 역시 event가 생성되었다는 증거입니다). 이 scenario는 과거 `D00.profile-off`를 대체합니다. |
| `D01.registry` | 20 | empty View와 ID 0은 거부, 서로 다른 View의 같은 ID도 거부합니다. 같은 View/ID 재등록은 허용합니다. ID 101→102 변경 후 다른 View가 101을 사용할 수 있어야 합니다. capture 중 ID 변경은 거부하고 capture metadata와 기존 ID 102를 보존합니다. 마지막 handle이 소멸한 View의 ID 103은 재사용 가능해야 합니다. inactive clear 이후 등록했던 세 View의 ID는 모두 0입니다. |
| `D01.registry-capacity` | 20 | 8192개 detached View를 unique ID로 등록하고 모든 snapshot ID를 대조합니다. 8193번째 등록은 거부되지만 이미 등록한 View의 같은 ID 재등록 및 ID 교체는 full 상태에서도 가능해야 합니다. ID 값만 비워도 slot이 생기지 않습니다. 가운데 View의 마지막 handle을 파괴하면 딱 한 slot을 재사용할 수 있으며 나머지 ID는 보존되어야 합니다. Clear 뒤 8192개 전체를 새 ID로 refill하고 다시 포화를 검사합니다. 모든 node를 정리한 뒤 새 등록·clear도 성공해야 합니다. |
| `D02.capture-validation` | 17 | null buffer, capacity 0, epoch 0은 거부하고 inactive 상태를 유지합니다. epoch 6202/capacity 32로 시작한 capture 중 다른 buffer/epoch 888의 nested Begin은 거부합니다. 원래 capture metadata와 두 번째 buffer의 `0xAABBCCDD` sentinel이 보존되어야 합니다. 한 번의 InvalidateArrange가 원래 buffer에 기록되고 첫 sequence=1, epoch=6202이어야 합니다. 실제 stored count=totalEvents>0이며 overflow=false입니다. |
| `D03.overflow-reset` | 23 | capacity 2에서 InvalidateArrange 8회를 호출하면 stored count=2, totalEvents≥8이고 totalEvents>stored count, overflow=true이어야 합니다. 두 record의 sequence는 1,2, epoch는 6203입니다. buffer 앞뒤 4×64-bit canary는 모두 원래 상수와 같아야 합니다. End 뒤 작업 및 반복 End는 capture metadata를 바꾸지 않습니다. 새 Begin(epoch 6204)은 count/total=0, overflow=false로 초기화하고 새 첫 record sequence=1로 시작합니다. 빈 epoch 6205는 count/total=0으로 종료합니다. |
| `D03.active-registry-clear` | 7 | capture 중 ClearRegisteredNodes는 IDs를 지우지 않고 active=true/overflow=true로 잘못된 사용을 드러냅니다. 임의 event를 만들지 않으므로 count/total=0이고 기존 ID 6208이 유지됩니다. End도 오류를 보존합니다. 그 뒤 inactive clear만 ID를 0으로 바꿉니다. |
| `D04.invalid-readers` | 12 | empty View/Window/Label의 snapshot은 valid=false, Animation은 empty입니다. Window에 속하지 않은 View의 transition snapshot/Animation도 invalid/empty입니다. empty Window의 tick/manual-mode 변경은 false입니다. 이 호출들로 capture metadata 또는 event 수가 변하면 FAIL입니다. |
| `D05.readonly-snapshots` | 15 | 실제 배치가 끝난 View와 Label, Window 및 inactive transition을 관측합니다. Label은 `Hgj`, font size 16이며 ready=true, line/glyph count>0, finite baseline이어야 합니다. 이 scenario는 font의 수치 metric 자체를 oracle로 삼지 않습니다. capture 중 1,000회씩 읽은 모든 snapshot 필드는 첫 값과 exact equality이어야 합니다. 여기에는 cache/dirty/in-progress/generation/owner, 실제 actor scale, geometry, line/glyph/font/baseline, controller queue/work 상태가 포함됩니다. event count/total=0, overflow=false이며 actor geometry도 보존되어야 합니다. End 뒤 다시 1,000회 읽어도 snapshot과 마지막 capture metadata가 같아야 합니다. |
| `D06.clock-restoration` / `mount` | 4 | 새 fixture child의 parent-local bounds가 x=40,y=30,width=80,height=40이어야 합니다. 네 geometry 행 허용오차는 0.001입니다. |
| `D06.clock-restoration` / `manual-and-restore` | 19 | negative/NaN/+Inf delta는 false를 반환하고 Window 상태를 보존합니다. manual mode에서 실제 CHANGE animator를 만들고 0.05s tick을 주면 첫 callback 1회/rawProgress=0이어야 합니다. 그 callback 안의 nested manual tick은 false입니다. manual mode를 false로 복원한 뒤 수동 tick은 false이며 active transition 상태를 바꾸지 않습니다. 실제 timer가 800ms 내 start=1/finish=1, callback>1, 마지막 rawProgress=1, active animator 없음에 도달해야 합니다. 반복 restore도 성공합니다. |

D07은 `spec/animator × change/enter/exit × registered/unregistered`의 12개 scenario입니다. 각 scenario는 `mount` 4행과 `historical-fields` 110행을 실행합니다. mount의 독립 expected는 `(40,30,80,40)`입니다. history action은 실제 Window controller로 transition을 시작하되 같은 event action 안에서만 관측하며, animator의 native timer는 manual mode로 정지합니다. animation seek나 시간 경과를 oracle로 사용하지 않습니다.

- `registered`는 시작 전에 root=11, child=12로 등록합니다. `unregistered`는 capture와 registry가 비어 있는 상태에서 시작합니다. 시작 시 ownerId/role은 각각 `11/2(DIRECT_PARENT)` 또는 `0/0`으로 고정됩니다.
- CHANGE의 from=`(40,30,80,40)`, to=`(140,30,80,40)`, cause=OTHER(4), duration=2, delay=0.25입니다. spec ENTER의 from=`(20,30,80,40)`, to=`(40,30,80,40)`, spec EXIT의 from/to는 그 반대입니다. 두 spec bounds slot은 composite duration=2.25, snapshot delay=0, cause=0이라는 기존 조회 계약을 보존합니다. animator ENTER/EXIT는 from=to=`(40,30,80,40)`, cause=OTHER, duration=2, delay=0.25입니다.
- `snapshot.started`, `snapshot.cleared`, `snapshot.rebound`는 각 34행입니다. 각각 모든 22개 scalar field와 from/to/lastLerped의 12개 성분을 독립 expected와 비교합니다. Clear 후 nodeId/parentId만 0으로 바뀌며, root=201/child=202 재등록 후 nodeId=202/parentId=201이 됩니다. 시작 당시 ownerId/role과 geometry/timing은 세 단계 내내 유지되어야 합니다.
- EXIT만 savedInteractionAvailable=true이고 저장된 sensitive/keyboardFocusable/touchFocusable는 true/true/false입니다. spec ENTER/EXIT만 savedClipAvailable=true, savedClip=DISABLED, currentClip=CLIP_TO_BOUNDING_BOX입니다. animator는 freshAnimator=true, elapsed=0, lastLerped=from이며 spec의 lastLerped는 zero rect입니다. 나머지 미사용 scalar/flag는 기본값이어야 합니다.
- 관측 100회 반복 결과는 모든 snapshot field에서 같아야 하며 capture metadata는 transition 생성과 읽기 전후에 변하지 않아야 합니다. scalar/ID/flags/timing은 exact 비교, geometry는 0.001 오차입니다. getter가 등록되지 않았다는 이유로 historical geometry를 생략하거나 현재 owner ID로 과거 owner ID를 덮어쓰면 FAIL입니다.

`D01.registry-capacity`의 준비·snapshot 반복은 performance sample이 아닙니다. fixture는 Window에 붙이지 않으며 local vector와 handles를 action 안에서 해제합니다. 마지막에는 registry clear가 실행되고 실패·예외 종료도 공통 cleanup으로 End/Clear합니다. 이 action은 준비 비용을 포함하여 최대 60초를 허용하고, 60초 초과·중도 중단은 PASS가 아닙니다. ID 비교는 전체 8192개를 순회한 aggregate bool 행이며, 이 행이 false이면 전체 registry identity 검사는 실패입니다. `capacity.*`의 20행이 모두 필요합니다.

D03의 overflow=true는 의도적으로 잘못된 사용을 넣은 **expected 오류 상태**입니다. 결과 행 `overflow.flag`와 `active-clear.error-visible`가 PASS인지 판정하십시오. 실제 suite buffer가 예기치 않게 overflow한 것을 이 scenario의 expected overflow로 무시하지 마십시오.

모든 bool/ID/count/epoch/sequence 및 read-only equality는 exact 비교입니다. D06의 rawProgress 0/1도 tolerance 0입니다. NaN/Inf가 정상 수치 actual에 섞이면 FAIL입니다. `required_checks`에 미달한 action, 누락된 scenario/action, `PENDING`·`INCOMPLETE`·예외·crash·timeout을 PASS에 합산하지 마십시오. 전체는 21/21 scenario, 34 action, 1513개 필수 assertion, failures=0이어야 합니다. 비교는 독립 ID 규칙·buffer 경계·순서·관측 전후 불변조건·공개 animator lifecycle에 기반합니다. candidate에서 얻은 값으로 expected를 갱신하지 마십시오.

## 항상 켜진 hook의 artifact 판정

`D00.profile-always-on`은 실행 중인 executable 안에서 hook이 존재하면서 비활성임을 보이지만, library ABI나 hot-path 비용 자체를 증명하지는 않습니다. 화면의 `EXTERNAL_REQUIRED`를 전체 PASS로 바꾸지 마십시오.

LLM 실행 agent는 서버가 보존한 Release artifact에서 다음을 확인하고 실행 결과와 별개로 기록하십시오.

1. 실제 executable이 로드한 library 파일을 식별하십시오. 다른 checkout이나 별도로 installed된 library를 대신 조회하지 마십시오.
2. 그 파일에 `nm -D -C --defined-only <actual-library>`를 실행하십시오. `Dali::Ui::Integration::LayoutTestDiagnostics`의 12개 함수가 **모두 존재**해야 합니다. 동시에 `LayoutTransitionDispatcher::GetLayoutTest*`, `SetLayoutTestManualTicks`, `TickLayoutTestAnimators`는 export되지 **않아야** 합니다. 이들은 설치되지 않는 internal header의 멤버이며 hidden visibility로 유지됩니다. `nm` 실패를 symbol 부재로 해석하지 마십시오.
3. 변경 이전 Release artifact와 같은 compiler/flags로 만든 ABI 비교에서 public 객체 크기와 vtable이 동일한지 확인하십시오. 특히 `ViewDataImpl`의 object size가 변하지 않아야 합니다.
4. measure/arrange producer의 disassembly에서 각 hook 자리가 out-of-line 호출로 가는 **load 한 번 + 조건 분기 한 번**인지 확인하십시오. 인자 평가가 분기 앞에 남아 있거나 주변 loop의 register spill이 늘어났다면 기록하십시오.
5. 동일 compiler/flags로 internal entry의 크기도 이전 artifact와 비교하십시오. `ActiveSpecAnimation`, `GhostExit`, `AnimatorState`는 public `TransitionSnapshot` 전체를 각 entry에 내장하지 않으며, animator가 이미 보관하는 geometry/timing은 다시 저장하지 않아야 합니다. start path와 getter의 code inspection 또는 allocation 추적에서 관측용 추가 heap allocation이 없어야 합니다. 크기 수치와 toolchain을 함께 보존하십시오. 이 구조 감소만으로 시간 성능 회귀가 없다고 판정하지 마십시오.
6. 위 artifact 증거와 이 TC의 1513개 assertion 증거가 모두 있어야 LR62 판정이 가능합니다. 한쪽 결과로 다른 쪽을 추정하지 마십시오.

## 관측 부하와 검증 경계

이 TC는 capture의 고정 buffer 경계와 등록 규칙, 관측 중 **Layout 상태 및 work event가 변하지 않는지**를 실행 검증합니다. `events=0`이나 canary 보존은 heap allocation=0의 증거가 아닙니다. 일반 malloc/new, OS memory, text shaping 전체, timing 성능을 이 TC가 검증했다고 보고하지 마십시오.

관측자 allocation 감사에는 capture/snapshot 구간의 정확한 allocation 증거 또는 해당 경로의 심볼/호출 분석을 첨부하십시오. 준비 단계에서 만든 View/Label, callback, result buffer, timer 및 framework 자체의 workload allocation과 관측자 경로를 구별해야 합니다. `StorageSite` event는 명시된 Layout vector storage 지점만 세므로 process allocation 총계로 해석하지 마십시오. 이 증거가 없으면 allocation 검증은 미실행으로 기록하며 LR62의 상태 불변성 PASS로 대체하지 마십시오. capture를 열지 않은 상태의 timing 비교는 별도 performance TC에서 수행합니다.

## 종료 및 재실행

`Reset run` 또는 `< Back`으로 끝내면 capture 종료, node 등록 해제, manual tick mode 복원이 cleanup에서 수행됩니다. D06 완료 후 뒤로 나갔다가 다시 들어와 D01부터 한 번 더 실행하십시오. 기존 run 결과를 새 run에 합치지 마십시오. 실패 시 source/library revision, build type, 입력 action, actual/expected, 전체 해당 action 행, 마지막 stdout 및 timeout/crash 상태를 보존하십시오.

## 고정 실행 계약과 종료 증거

각 step 실행(`Run step`/`Run scenario`/`Run all`) 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 각 step의 전체 stdout 행은 완료 시 자동 출력되므로 그대로 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 21 scenario, 34 action, 1,513 checks. 이 표에 없는 scenario는 컴파일된 계약에 선언되지 않으며 실행·판정 대상이 아닙니다.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `D00.profile-always-on` | `run` : 8 |
| `D01.registry` | `run` : 20 |
| `D01.registry-capacity` | `run` : 20 |
| `D02.capture-validation` | `run` : 17 |
| `D03.overflow-reset` | `run` : 23 |
| `D03.active-registry-clear` | `run` : 7 |
| `D04.invalid-readers` | `run` : 12 |
| `D05.readonly-snapshots` | `run` : 15 |
| `D06.clock-restoration` | `mount` : 4 → `manual-and-restore` : 19 |
| `D07.spec-change-unregistered` | `mount` : 4 → `historical-fields` : 110 |
| `D07.spec-change-registered` | `mount` : 4 → `historical-fields` : 110 |
| `D07.spec-enter-unregistered` | `mount` : 4 → `historical-fields` : 110 |
| `D07.spec-enter-registered` | `mount` : 4 → `historical-fields` : 110 |
| `D07.spec-exit-unregistered` | `mount` : 4 → `historical-fields` : 110 |
| `D07.spec-exit-registered` | `mount` : 4 → `historical-fields` : 110 |
| `D07.animator-change-unregistered` | `mount` : 4 → `historical-fields` : 110 |
| `D07.animator-change-registered` | `mount` : 4 → `historical-fields` : 110 |
| `D07.animator-enter-unregistered` | `mount` : 4 → `historical-fields` : 110 |
| `D07.animator-enter-registered` | `mount` : 4 → `historical-fields` : 110 |
| `D07.animator-exit-unregistered` | `mount` : 4 → `historical-fields` : 110 |
| `D07.animator-exit-registered` | `mount` : 4 → `historical-fields` : 110 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`이고 `completed_scenarios == required_scenarios`이며 모든 count(failures 포함)가 마지막 `LR_RESULT`와 같아야 합니다. PASS는 그 완료 조건과 failures=0이 함께 성립할 때만입니다. `layout-validation-coverage.md`에 알려진 제품 결함 행이 문서화된 TC는 failures>0으로 종결되며, `check-log`가 `observations-valid-with-failures`로 그 행을 **정확히** 나열해야 합니다. 그 밖의 실패 행이나 `invalid-evidence`는 회귀 또는 무효 증거이며 PASS가 아닙니다. END 누락·cleanup 오류도 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR62 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. `--profile`의 `diagnostic`/`release` 두 값은 내용이 동일한 같은 계약을 가리키므로 어느 쪽을 지정해도 같은 결과입니다. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. `status`가 `observations-valid`면 실패 행이 없고, `observations-valid-with-failures`면 증거는 유효하며 `failing_rows`에 나열된 행만 실패했다는 뜻이고(exit 1), `invalid-evidence`면 증거 자체가 무효이므로 재실행해야 합니다(exit 2). `observer.timeout`은 layout 완료 관측 실패, `render.frame-timeout`은 새 render task 또는 입력 반영 완료 관측 실패, `render.settle-timeout`은 current geometry 반영 관측 실패로 분류하십시오. 이 과정에서 필수 관측 행이 누락되거나 실제 행 수가 계약과 달라지면 strict 검증 결과는 `invalid-evidence`(exit 2)입니다. 해당 observer 행, `protocol.required_checks`, 도구의 `error`, 원본 stdout/stderr를 함께 보존하십시오. 재실행은 새 run 증거로 별도 기록하고 첫 실패를 삭제하지 마십시오. timeout, 필수 행 누락, `LR_READY` 또는 END 누락을 **어떤 경우에도 PASS로 처리하지 마십시오**. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
