# LR31. Layout: Replay mutation

이 TC는 신규 Layout suite의 **Replay mutation** 검증입니다. 대응 소스는 `tc-lr31-layout-replay-mutation.cpp`입니다. 기존 TC의 실행 결과를 사용하지 않습니다.

## 실행 조건

- foundation manual-test 앱에서 `LR31`를 검색하고 표시명이 일치하는 TC에 진입하십시오.
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

- `K19.add-remove`: a20,b30 사이 gap7인 cached tree에서 a를 제거하고 c40을 추가합니다. 논리 child는 b,c 순서, x0/37이어야 합니다. RemoveAll 후 child 수0, 범위 밖 조회는 empty입니다.
- `K20.mutate-during-producer`: 첫 Measure callback에서 child b를 한 번 추가합니다. callback의 captured tree가 안전하게 끝난 후 외부 invalidation으로 다음 pass를 수행하며 b x20,width30을 확인합니다.


- 추가 K20.replay-property-mutation / replay-external-measure / replay-external-arrange / replay-reentrant-arrange는 diagnostics ON에서 실행합니다. 앞의 세 scenario는26행, 마지막은30행입니다.
- parent Stack200×60, a20×10,b30×10을 정상 Measure/Arrange하여 cache를 만든 후 a의 Actor POSITION_X만99로 바꿉니다. 실제 a.PropertySetSignal을 연결하고 동일 root Arrange가 cache replay로 a.x를0으로 복원할 때 callback을 정확히 한 번 실행합니다. callback에서 root와 a의 replay/arrange-in-progress, processing 상태를 POD snapshot에 저장합니다. root의 ARRANGE_HIT=1, ARRANGE_PRODUCER=0을 함께 확인하므로 일반 producer 호출을 replay evidence로 기록할 수 없습니다.
- callback은 아래 public action만 한 번 수행하고 UI/로그를 갱신하지 않습니다. 이후 root 상태와 자연스러운 다음 Compute 결과를 확인합니다.

| scenario | callback action | after measure cache | after arrange cache | measure/arrange dirty | arrange poison |
|---|---|---|---|---|---|
| replay-property-mutation | b.SetMinimumWidth(45) | false | false | true/true | true |
| replay-external-measure | b.Measure(12,8) | false | false | false/false | false |
| replay-external-arrange | b.Arrange(75,20,10,5) | true | false | false/false | false |
| replay-reentrant-arrange | root.Arrange(0,0,999,999) | true | true | false/false | true |

외부 Measure/Arrange는 cache-only invalidation입니다. 재실행할 producer의 publish 단계가 없는 replay에 arrangePublishBlocked를 새로 남기면 안 됩니다. property mutation은 dirty와 poison을 남겨야 하고, 동일 View Arrange 재진입은 producer를 재귀 호출하지 않고 마지막 완료 rect(0,0,200,60)를 반환해야 합니다. 마지막 scenario의 nested.last-completed-result4행이 이 결과입니다.

각 scenario의 필수 공통 행은 initial.published-cache, capture, observer.calls/root-replaying/child-replaying/processing, root.cache-hit/producer-not-run, replay.flags-unwound, capture.no-overflow, after.measure-cache/arrange-cache/measure-dirty/arrange-dirty/arrange-poison, replay.no-producer-publish-block입니다. 다음 Compute 후 recovered.a=(0,0,20,10), recovered.b=(20,0,45,10)은 minimum을45로 바꾼 property mutation에만 적용하고 나머지는(20,0,30,10)입니다. 두 rect8행과 recovered.caches/clean2행을 확인하십시오.

이 진단은 off-scene 동기 replay의 state 보존과 회복을 검증합니다. Window의 PARK/idle wake 증거는 LR29의 자연 완료·quiet 절차에서 별도로 수집합니다. PropertySetSignal 연결은 Run cleanup에서 해제하며 callback에는 일회성 guard가 있어 재귀적인 mutation을 반복하지 않습니다. 근거는 view-data-impl.cpp의 ReplayPassScope/ReplayNodeScope, InvalidateAncestorLayoutCachesForMeasureMiss, InvalidateParentArrangeCacheForOutOfBandArrange와 ArrangeImpl 재진입 guard입니다.

`Rect`는 x/y/width/height의 네 필수 행, `Size`는 width/height의 두 필수 행입니다. 일반 geometry 허용오차는 0.001이며 정수·순서·bool·별도로 지정한 exact 항목은 정확히 일치해야 합니다. NaN/Inf는 일반 geometry 비교에서 실패합니다. expected는 입력 표·식에서 계산하며 candidate 출력으로 갱신하지 마십시오.

## 근거와 검증 경계

child handle snapshot, replay child set 변경, producer 중 tree mutation의 후속 정상 상태를 검사합니다.

직접 `Measure`/`Arrange`의 반환과 Actor event-side target을 검사합니다. `Arrange` 반환은 pre-RTL logical이고 child Actor 좌표는 부모의 RTL 적용 결과입니다. Window snapshot은 실제 settle 결과이며 animation 중간 화면과 혼동하지 마십시오. callback 측정 횟수는 명시한 synthetic producer의 횟수입니다.

## 종료와 재현

`< Back`으로 나간 뒤 같은 TC에 다시 진입하고 `Reset run` 이후 첫 scenario를 반복하십시오. run ID·counter가 새로 시작하고 초기 기대값이 같아야 합니다. 실패는 scenario/input/action/actual/expected를 함께 보존하십시오. 기존 Layout TC 실행으로 대체하지 마십시오.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 6 scenario, 6 action, 128 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `K19.add-remove` | `run` : 14 |
| `K20.mutate-during-producer` | `run` : 6 |
| `K20.replay-property-mutation` | `run` : 26 |
| `K20.replay-external-measure` | `run` : 26 |
| `K20.replay-external-arrange` | `run` : 26 |
| `K20.replay-reentrant-arrange` | `run` : 30 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR31 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
