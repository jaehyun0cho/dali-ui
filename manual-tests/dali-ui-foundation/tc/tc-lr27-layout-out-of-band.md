# LR27. Layout: Out of band layout

이 TC는 신규 Layout suite의 **Out of band layout** 검증입니다. 대응 소스는 `tc-lr27-layout-out-of-band.cpp`입니다. 기존 TC의 실행 결과를 사용하지 않습니다.

## 실행 조건

- foundation manual-test 앱에서 `LR27`를 검색하고 표시명이 일치하는 TC에 진입하십시오.
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

- K14.external-measure는19행입니다. parent200×80의 일반 MATCH_PARENT child를 정상 측정/배치한 다음 parent Measure/Arrange cache가 모두 유효한지 확인합니다. child만70×40으로 외부 Measure한 뒤 parent 두 cache가 모두 무효이고 measureDirty/arrangeDirty/poison은 false여야 합니다. 이것은 cache-only invalidation이며 명시적인 InvalidateMeasure의 dirty 전파와 다릅니다. 외부 normalized constraint70×40, parent 재계산 후200×80, 두 parent cache의 재게시, 실제 rect(0,0,200,80), overflow 없음까지 확인하십시오.
- K14.external-standalone-measure는16행입니다. parent200×80, STANDALONE WRAP_CONTENT child의 natural size300×90, 요청 위치7/9입니다. 최초 parent 배치 후 cache 유효와 slot consumed를 확인합니다. 외부 Measure(70,40)는70×40을 반환하고 child slot을 unconsumed로 바꾸되 parent cache는 유지해야 합니다. parent Measure(200,80)의 cache hit는 child producer를 재실행하지 않아야 합니다. 이어지는 parent Arrange는 child를200×80로 정확히 한 번 재측정하고 slot consumed와 rect(7,9,200,80)을 복원해야 합니다. 다음 동일 Arrange는 child Measure를 더 실행하지 않아야 합니다. 외부 Measure 전 counter를 n이라고 하면 외부 측정 뒤 n+1, parent Measure 뒤 n+1, 보정 Arrange 뒤 n+2, 반복 Arrange 뒤 n+2입니다.
- K14.external-arrange는8행입니다. child를(75,20,10,5)로 직접 이동한 뒤 동일 parent Arrange가 본래(20,0,30,10)를 복원해야 합니다.

필수 행은 일반 child에서 before.measure-cache/arrange-cache, external.owner-measure-cache-cleared/owner-arrange-cache-cleared/no-measure-dirty/no-arrange-dirty/no-poison, external.constraint와 restored.constraint의 두 축, restored.measure-cache/arrange-cache, restored rect4축, capture/no-overflow입니다. standalone에서는 before.parent-cache/standalone-slot-consumed, external.measure-result 두 축/standalone-slot-unconsumed/parent-cache-preserved, parent-measure-hit-no-child-producer, corrected.slot-consumed/corrective-child-producer/slot4축, repeat.no-child-producer, capture/no-overflow를 확인하십시오.

`Rect`는 x/y/width/height의 네 필수 행, `Size`는 width/height의 두 필수 행입니다. 일반 geometry 허용오차는 0.001이며 정수·순서·bool·별도로 지정한 exact 항목은 정확히 일치해야 합니다. NaN/Inf는 일반 geometry 비교에서 실패합니다. expected는 입력 표·식에서 계산하며 candidate 출력으로 갱신하지 마십시오.

## 근거와 검증 경계

parent traversal 외부에서 수행되는 child Measure/Arrange가 cached result와 replay snapshot을 망가뜨리지 않는지 검사합니다. internal/views/view/view-data-impl.cpp의 InvalidateAncestorLayoutCachesForMeasureMiss는 dirty나 poison을 세우지 않고 cache만 내립니다. MeasureStandaloneChildren/ArrangeStandaloneChildren만 standalone slot을 소비하므로 일반 child와 standalone의 bit를 같은 계약으로 비교하지 마십시오.

직접 `Measure`/`Arrange`의 반환과 Actor event-side target을 검사합니다. `Arrange` 반환은 pre-RTL logical이고 child Actor 좌표는 부모의 RTL 적용 결과입니다. Window snapshot은 실제 settle 결과이며 animation 중간 화면과 혼동하지 마십시오. callback 측정 횟수는 명시한 synthetic producer의 횟수입니다.

## 종료와 재현

`< Back`으로 나간 뒤 같은 TC에 다시 진입하고 `Reset run` 이후 첫 scenario를 반복하십시오. run ID·counter가 새로 시작하고 초기 기대값이 같아야 합니다. 실패는 scenario/input/action/actual/expected를 함께 보존하십시오. 기존 Layout TC 실행으로 대체하지 마십시오.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 3 scenario, 3 action, 43 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `K14.external-measure` | `run` : 19 |
| `K14.external-standalone-measure` | `run` : 16 |
| `K14.external-arrange` | `run` : 8 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR27 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
