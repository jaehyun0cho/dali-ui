# LR62. Layout: Diagnostics observation contract

이 TC는 새 Layout suite가 사용하는 diagnostics 관측 API 자체를 검증합니다. 대응 source는 `tc-lr62-layout-diagnostics-contract.cpp`입니다. 기존 Layout TC는 실행 전제에 포함하지 않습니다.

## 실행 profile과 준비

- ON 실행에서는 library를 `ENABLE_LAYOUT_TEST_DIAGNOSTICS=ON`으로 빌드하고 manual-test executable에도 `DALI_UI_LAYOUT_TEST_DIAGNOSTICS`를 정의하십시오. 실제 로드한 library 경로·revision·빌드 옵션과 executable revision을 증거에 기록하십시오. 헤더만 ON인 조합을 정상 profile로 취급하지 마십시오.
- foundation 앱에서 `LR62. Layout: Diagnostics observation contract`에 진입하여 `Reset run`을 한 번 누르십시오. 새 `run_id`를 기록하십시오. ON에서는 아래 7개 scenario, 총 8개 action, 137개 필수 assertion을 실행합니다.
- 각 action의 `Apply next step`을 한 번 누르고 완료될 때까지 추가 입력을 보내지 마십시오. D05는 실제 View snapshot과 Window 완료 fence를 기다리고, D06 두 번째 action은 복원된 timer의 자연 종료를 800ms 후 관측합니다. 각 action이 10초 내 완료되지 않으면 timeout FAIL입니다.
- 완료 후 `Read result`를 누르십시오. `Next result page`로 모든 결과 페이지를 읽으십시오. `Next scenario`는 scenario 이동이며 결과 페이지 이동과 다릅니다.
- stdout의 `LR_ACTION`·`LR_CHECK`·`LR_RESULT` 또는 같은 run의 화면 결과를 저장하십시오. `run`, `scenario`, `step`, `action_seq`가 일치하는 행끼리만 비교하십시오. screenshot의 위치나 글자 모양을 수치 expected로 사용하지 마십시오.

## ON scenario와 pass/fail 기준

| Scenario / action | 필수 행 수 | 독립 invariant와 관측값 |
| --- | ---: | --- |
| `D01.registry` | 20 | empty View와 ID 0은 거부, 서로 다른 View의 같은 ID도 거부합니다. 같은 View/ID 재등록은 허용합니다. ID 101→102 변경 후 다른 View가 101을 사용할 수 있어야 합니다. capture 중 ID 변경은 거부하고 capture metadata와 기존 ID 102를 보존합니다. 마지막 handle이 소멸한 View의 ID 103은 재사용 가능해야 합니다. inactive clear 이후 등록했던 세 View의 ID는 모두 0입니다. |
| `D01.registry-capacity` | 20 | 8192개 detached View를 unique ID로 등록하고 모든 snapshot ID를 대조합니다. 8193번째 등록은 거부되지만 이미 등록한 View의 같은 ID 재등록 및 ID 교체는 full 상태에서도 가능해야 합니다. ID 값만 비워도 slot이 생기지 않습니다. 가운데 View의 마지막 handle을 파괴하면 딱 한 slot을 재사용할 수 있으며 나머지 ID는 보존되어야 합니다. Clear 뒤 8192개 전체를 새 ID로 refill하고 다시 포화를 검사합니다. 모든 node를 정리한 뒤 새 등록·clear도 성공해야 합니다. |
| `D02.capture-validation` | 17 | null buffer, capacity 0, epoch 0은 거부하고 inactive 상태를 유지합니다. epoch 6202/capacity 32로 시작한 capture 중 다른 buffer/epoch 888의 nested Begin은 거부합니다. 원래 capture metadata와 두 번째 buffer의 `0xAABBCCDD` sentinel이 보존되어야 합니다. 한 번의 InvalidateArrange가 원래 buffer에 기록되고 첫 sequence=1, epoch=6202이어야 합니다. 실제 stored count=totalEvents>0이며 overflow=false입니다. |
| `D03.overflow-reset` | 23 | capacity 2에서 InvalidateArrange 8회를 호출하면 stored count=2, totalEvents≥8이고 totalEvents>stored count, overflow=true이어야 합니다. 두 record의 sequence는 1,2, epoch는 6203입니다. buffer 앞뒤 4×64-bit canary는 모두 원래 상수와 같아야 합니다. End 뒤 작업 및 반복 End는 capture metadata를 바꾸지 않습니다. 새 Begin(epoch 6204)은 count/total=0, overflow=false로 초기화하고 새 첫 record sequence=1로 시작합니다. 빈 epoch 6205는 count/total=0으로 종료합니다. |
| `D03.active-registry-clear` | 7 | capture 중 ClearRegisteredNodes는 IDs를 지우지 않고 active=true/overflow=true로 잘못된 사용을 드러냅니다. 임의 event를 만들지 않으므로 count/total=0이고 기존 ID 6208이 유지됩니다. End도 오류를 보존합니다. 그 뒤 inactive clear만 ID를 0으로 바꿉니다. |
| `D04.invalid-readers` | 12 | empty View/Window/Label의 snapshot은 valid=false, Animation은 empty입니다. Window에 속하지 않은 View의 transition snapshot/Animation도 invalid/empty입니다. empty Window의 tick/manual-mode 변경은 false입니다. 이 호출들로 capture metadata 또는 event 수가 변하면 FAIL입니다. |
| `D05.readonly-snapshots` | 15 | 실제 배치가 끝난 View와 Label, Window 및 inactive transition을 관측합니다. Label은 `Hgj`, font size 16이며 ready=true, line/glyph count>0, finite baseline이어야 합니다. 이 scenario는 font의 수치 metric 자체를 oracle로 삼지 않습니다. capture 중 1,000회씩 읽은 모든 snapshot 필드는 첫 값과 exact equality이어야 합니다. 여기에는 cache/dirty/in-progress/generation/owner, 실제 actor scale, geometry, line/glyph/font/baseline, controller queue/work 상태가 포함됩니다. event count/total=0, overflow=false이며 actor geometry도 보존되어야 합니다. End 뒤 다시 1,000회 읽어도 snapshot과 마지막 capture metadata가 같아야 합니다. |
| `D06.clock-restoration` / `mount` | 4 | 새 fixture child의 parent-local bounds가 x=40,y=30,width=80,height=40이어야 합니다. 네 geometry 행 허용오차는 0.001입니다. |
| `D06.clock-restoration` / `manual-and-restore` | 19 | negative/NaN/+Inf delta는 false를 반환하고 Window 상태를 보존합니다. manual mode에서 실제 CHANGE animator를 만들고 0.05s tick을 주면 첫 callback 1회/rawProgress=0이어야 합니다. 그 callback 안의 nested manual tick은 false입니다. manual mode를 false로 복원한 뒤 수동 tick은 false이며 active transition 상태를 바꾸지 않습니다. 실제 timer가 800ms 내 start=1/finish=1, callback>1, 마지막 rawProgress=1, active animator 없음에 도달해야 합니다. 반복 restore도 성공합니다. |

`D01.registry-capacity`의 준비·snapshot 반복은 performance sample이 아닙니다. fixture는 Window에 붙이지 않으며 local vector와 handles를 action 안에서 해제합니다. 마지막에는 registry clear가 실행되고 실패·예외 종료도 공통 cleanup으로 End/Clear합니다. 이 action은 준비 비용을 포함하여 최대60초를 허용하고, 60초 초과·중도 중단은 PASS가 아닙니다. ID 비교는 전체8192개를 순회한 aggregate bool 행이며, 이 행이 false이면 전체 registry identity 검사는 실패입니다. `capacity.*`의20행이 모두 필요합니다.

D03의 overflow=true는 의도적으로 잘못된 사용을 넣은 **expected 오류 상태**입니다. 결과 행 `overflow.flag`와 `active-clear.error-visible`가 PASS인지 판정하십시오. 실제 suite buffer가 예기치 않게 overflow한 것을 이 scenario의 expected overflow로 무시하지 마십시오.

모든 bool/ID/count/epoch/sequence 및 read-only equality는 exact 비교입니다. D06의 rawProgress 0/1도 tolerance 0입니다. NaN/Inf가 정상 수치 actual에 섞이면 FAIL입니다. `required_checks`에 미달한 action, 누락된 scenario/action, `PENDING`·`INCOMPLETE`·예외·crash·timeout을 PASS에 합산하지 마십시오. ON 전체는 8/8 scenario, 9 action, 137개 필수 assertion, failures=0이어야 합니다. 비교는 독립 ID 규칙·buffer 경계·순서·관측 전후 불변조건·공개 animator lifecycle에 기반합니다. candidate에서 얻은 값으로 expected를 갱신하지 마십시오.

## OFF profile 판정

동일 revision을 diagnostics OFF로 빌드한 앱에서는 `D00.profile-off` 한 action/한 행만 존재합니다. `profile.compiled-capability` actual=OFF/expected=OFF를 확인하십시오. 이 행은 executable의 compile branch를 확인할 뿐 실제 library ABI 또는 hot-path 비용을 증명하지 않습니다. 화면의 `EXTERNAL_REQUIRED`를 전체 PASS로 바꾸지 마십시오.

LLM 실행 agent는 서버가 보존한 OFF 빌드 artifact에서 다음을 확인하고 ON 결과와 별개로 기록하십시오.

1. 실제 executable이 로드한 library 파일을 식별하고 그 빌드의 CMake 옵션이 OFF인지 확인하십시오. 다른 checkout이나 installed library를 대신 조회하지 마십시오.
2. 그 파일에 `nm -D -C --defined-only <actual-library>`를 실행하십시오. `LayoutTestDiagnostics` 또는 `GetLayoutTest`를 포함하는 진단 export가 없어야 합니다. `nm` 실패를 symbol 부재로 해석하지 마십시오.
3. strip 이전 같은 OFF artifact의 symbol table에서도 collector storage/helper가 제거되었는지 확인하십시오. 원본 Release와 OFF candidate를 같은 compiler/flags로 만든 ABI·disassembly 비교 결과에서 진단 때문에 public 객체 크기, vtable, production 호출/분기/storage가 추가되지 않았는지 확인하십시오. 진단 source가 없어도 원래 포함된 debug/assert source-location 차이는 기능 변화와 구별하십시오.
4. OFF artifact 증거와 이 TC의 ON 137개 assertion 증거가 모두 있어야 LR62의 profile 결합 판정이 가능합니다. 한 profile의 결과로 다른 profile을 추정하지 마십시오.

## 관측 부하와 검증 경계

이 TC는 capture의 고정 buffer 경계와 등록 규칙, 관측 중 **Layout 상태 및 work event가 변하지 않는지**를 실행 검증합니다. `events=0`이나 canary 보존은 heap allocation=0의 증거가 아닙니다. 일반 malloc/new, OS memory, text shaping 전체, timing 성능을 이 TC가 검증했다고 보고하지 마십시오.

ON 관측자 allocation 감사에는 별도 instrumented artifact의 정확한 capture/snapshot 구간 allocation 증거 또는 해당 경로의 심볼/호출 분석을 첨부하십시오. 준비 단계에서 만든 View/Label, callback, result buffer, timer 및 framework 자체의 workload allocation과 관측자 경로를 구별해야 합니다. `StorageSite` event는 명시된 Layout vector storage 지점만 세므로 process allocation 총계로 해석하지 마십시오. 이 증거가 없으면 allocation 검증은 미실행으로 기록하며 LR62의 상태 불변성 PASS로 대체하지 마십시오. 무계측 timing 비교는 별도 performance TC에서 수행합니다.

## 종료 및 재실행

`Reset run` 또는 `< Back`으로 끝내면 capture 종료, node 등록 해제, manual tick mode 복원이 cleanup에서 수행됩니다. D06 완료 후 뒤로 나갔다가 다시 들어와 D01부터 한 번 더 실행하십시오. 기존 run 결과를 새 run에 합치지 마십시오. 실패 시 source/library revision, profile, 입력 action, actual/expected, 전체 해당 action 행, 마지막 stdout 및 timeout/crash 상태를 보존하십시오.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic`: 8 scenario, 9 action, 137 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `D01.registry` | `run` : 20 |
| `D01.registry-capacity` | `run` : 20 |
| `D02.capture-validation` | `run` : 17 |
| `D03.overflow-reset` | `run` : 23 |
| `D03.active-registry-clear` | `run` : 7 |
| `D04.invalid-readers` | `run` : 12 |
| `D05.readonly-snapshots` | `run` : 15 |
| `D06.clock-restoration` | `mount` : 4 → `manual-and-restore` : 19 |

Profile `release`: 1 scenario, 1 action, 1 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `D00.profile-off` | `run` : 1 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR62 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
