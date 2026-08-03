# View Measure/Arrange 성능 개선 최종 수정 방안

## 1. 목적

이 문서의 목적은 layout 계산 결과를 올바르게 유지하면서 반복적인 Measure와 Arrange 호출 비용을 최소화하는 것이다.

주요 목표는 다음과 같다.

- 동일한 Measure constraint에 대한 callback, LayoutManager, `OnMeasure()` 및 자식 순회 생략
- 동일한 Arrange bounds에 대한 actor geometry 접근, producer, 자식 Arrange, 방향 해석 및 lifecycle 처리 생략
- pass 중 발생한 재무효화가 종료부의 cache publish에 의해 유실되는 문제 방지
- 직접 Measure, Arrange 내부 Measure 및 recycler item Measure를 정확히 구분
- LTR/RTL 반복 적용과 전환을 결정론적으로 처리
- 내부 geometry write와 동기 callback의 외부 write를 정확히 구분
- fitting과 natural-size 갱신을 Arrange 실행 여부에서 분리

핵심 원칙은 다음과 같다.

> dirty 여부를 cache key의 특수값으로 추측하지 않고 명시적인 상태로 관리하며, pass가 끝난 시점에도 계산 전제가 유효할 때만 결과를 publish한다.

## 2. 허용되는 동작 변화

다음 변화는 일반적인 retained layout 시스템의 동작으로 허용한다.

- clean Measure cache hit에서 Measure callback, LayoutManager 및 `OnMeasure()`가 다시 호출되지 않음
- clean Arrange cache hit에서 Arrange callback, LayoutManager, `OnArrange()` 및 자식 Arrange가 다시 호출되지 않음
- Arrange cache hit에서 LayoutFinished가 발생하지 않음
- 외부에서 직접 설정한 actor geometry가 다음 declarative layout 전까지 유지되고, 다음 layout에서 계산 결과로 덮임
- 자식 View만 직접 Arrange했을 때 그 자식 자신의 parent-relative RTL X는 부모 Arrange에서 확정됨
- layout 결과에 영향을 주는 hidden mutable state를 변경한 callback, producer 또는 LayoutManager가 명시적으로 invalidation을 요청해야 함

다음은 허용하지 않는다.

- stale measured result 또는 stale arranged result 재사용
- pass 중 invalidation 유실
- RTL 반복 적용에 따른 위치 왕복 또는 누적 오차
- LTR 복원 누락
- resource-ready 이후 natural size 또는 fitting 갱신 누락
- 내부 write 오분류에 따른 필수 invalidation 누락

## 3. 정확성 불변조건

### 3.1 Measure 불변조건

1. effective scale 적용 순서를 유지한다.
2. min/max constraint 정규화 순서를 유지한다.
3. 정규화된 constraint의 기존 동치 의미를 유지한다.
4. callback, LayoutManager, 기본 producer의 우선순위를 유지한다.
5. 부모가 소비한 child measured result가 독립적인 child Measure로 바뀌면 부모 관련 cache를 무효화한다.
6. 정상 부모 Measure가 현재 pass에서 새 child 결과를 소비한 경우에는 그 child miss만으로 부모를 다시 dirty로 만들지 않는다.
7. child Measure 중 실제 invalidation이 발생하면 활성 부모 pass의 publish를 막는다.
8. full Measure는 자기 Arrange cache를 항상 무효화한다.
9. Measure보다 먼저 실행된 Arrange 결과는 Measure cache가 유효해지기 전까지 cache hit 대상으로 만들지 않는다.

### 3.2 Arrange 불변조건

1. Arrange input bounds와 producer가 반환한 final bounds를 분리한다.
2. 네 축의 final bounds를 authoritative geometry로 유지한다.
3. pass 중 invalidation, reparent 또는 scale-context 변경이 cache publish로 덮이지 않게 한다.
4. logical bounds와 physical direction 결과를 분리한다.
5. 부모가 direct child의 parent-relative physical X를 소유한다.
6. standalone 및 physical-owned subtree 경계를 유지한다.
7. clean hit에서는 actor property를 읽거나 쓰지 않는다.

### 3.3 Property write 불변조건

1. 내부 write 판정은 실제 target과 property index에 연결한다.
2. setter 반환까지 유지되는 ambient boolean이나 depth를 내부 write 근거로 사용하지 않는다.
3. 내부 setter 직후 동기 signal callback이 수행하는 write는 외부 write로 처리한다.
4. setter 실패, hook 미발화, 조기 반환 또는 중첩 write가 token을 남기지 않게 한다.

## 4. 현재 병목과 정확성 문제

### 4.1 Measure hit 전 Trait 순회는 없다

Measure는 effective scale을 구하고 actor property와 동기화한 뒤 constraint를 scale 변환하고 min/max로 정규화한다. 그 후 cached constraint를 비교하며, callback, LayoutManager 및 `OnMeasure()`는 cache miss에서만 실행된다.

min/max getter는 이미 보유한 constraint storage를 조회할 뿐 Trait tree를 순회하지 않는다.

따라서 Measure hit 비용 개선의 1차 대상은 다음이다.

- 매 Measure의 effective-scale actor property read
- dirty sentinel과 invalidation 처리의 결합
- 불필요한 조상 cache 무효화

raw constraint key는 correctness 필수항목이 아니다.

### 4.2 invalidation early return이 새 전파를 숨길 수 있다

이미 dirty라는 이유로 `InvalidateMeasure()` 또는 `InvalidateArrange()`가 즉시 반환하면 다음 작업이 누락될 수 있다.

- pass 중 다시 발생한 invalidation 표시
- 바뀐 조상 체인으로의 전파
- active pass poison
- layout controller 등록

로컬 dirty 상태와 “조상 전파 또는 controller 등록이 이미 끝났다”는 상태를 같은 값으로 표현해서는 안 된다.

### 4.3 Arrange는 동일 입력에도 전체 작업을 반복한다

현재 Arrange는 동일 bounds에서도 다음 작업을 다시 수행할 수 있다.

- provisional self geometry 적용
- Arrange producer 실행
- final self geometry 적용
- standalone child 처리
- direct child RTL 처리
- lifecycle 및 LayoutFinished 후보 처리

또한 RTL 처리가 현재 physical X를 다시 반전하는 방식이면 반복 호출이 멱등적이지 않고 LTR로 복원할 근거도 없다.

### 4.4 Measure 호출의 소유권이 서로 다르다

다음 호출은 같은 규칙으로 처리할 수 없다.

1. 부모 Measure가 child를 측정하고 현재 결과를 소비하는 정상 재귀
2. 외부에서 child의 public Measure를 직접 호출
3. Arrange producer가 child를 Measure한 뒤 현재 Arrange에서 소비
4. recycler layouter가 표준 View 재귀 밖에서 item을 직접 Measure/Arrange

이들을 모두 direct Measure로 취급하면 과잉 무효화가 발생하고, 모두 정상 재귀로 취급하면 stale ancestor cache가 발생한다.

## 5. Measure 상태 모델

각 View에 다음 명시 상태를 둔다.

- `measureCacheValid`
- `measureDirty`
- `measureInProgress`
- `measurePassPoisoned`
- `lastMeasureConstraint`
- 마지막으로 정상 완료된 measured result
- `effectiveScaleSyncRequired`

기본 상태에는 범용 invalidation generation을 두지 않는다. 현재 layout 호출은 event thread에서 동기적으로 중첩되며, invalidation early return을 제거하고 pass 진입에서 dirty를 소비하면 실제 재무효화는 dirty와 pass poison으로 관찰할 수 있다.

generation은 recycler처럼 별도의 비표준 owner scope에서 중첩·중단을 구분해야 할 때만 제한적으로 사용한다.

### 5.1 sentinel 제거

정상 constraint 값과 cache validity를 같은 필드에 저장하지 않는다.

명시 상태를 사용하면 다음을 구별할 수 있다.

- 최초 Measure 전
- valid cache 보유
- invalidation된 상태
- Measure 진행 중
- 진행 중 다시 invalidation됨
- 마지막 완료 결과는 있지만 현재 cache는 invalid

기존 정규화된 constraint 비교 의미는 유지한다.

## 6. Measure cache hit

다음 조건을 모두 만족할 때만 Measure hit로 처리한다.

1. `measureCacheValid == true`
2. `measureDirty == false`
3. `measureInProgress == false`
4. `measurePassPoisoned == false`
5. 정규화된 effective constraint가 저장된 constraint와 기존 비교 의미상 동일
6. effective scale cache가 현재 상속 context에 대해 유효

`effectiveScaleSyncRequired`가 false인 정상 hit에서는 actor property를 읽거나 쓰지 않는다.

`effectiveScaleSyncRequired`가 true이면 full Measure를 수행하기 전에 내부 cached scale을 actor property에 token write로 동기화한다. 이 동기화만으로 measured result의 의미가 바뀌지 않았다면 Measure producer는 다시 실행하지 않고 기존 measured result를 반환할 수 있다.

hit에서는 다음을 생략한다.

- Measure callback 조회와 호출
- LayoutManager Measure
- `OnMeasure()`
- child traversal
- actor effective-scale property read

## 7. Measure miss transaction

Measure miss는 다음 순서로 수행한다.

1. release build에서도 same-View 재진입을 검사한다.
2. RAII pass guard를 생성한다.
3. `measureInProgress = true`로 설정한다.
4. `measureDirty = false`로 현재 dirty를 소비한다.
5. `measurePassPoisoned = false`로 pass-local 상태를 초기화한다.
6. `measureCacheValid = false`로 만들어 미완료 결과 재사용을 막는다.
7. 자기 Arrange cache를 무효화한다.
8. callback, LayoutManager 또는 기본 `OnMeasure()`를 실행한다.
9. 결과를 min/max와 scale 규칙에 따라 확정한다.
10. pass 중 새 dirty, poison, 재진입 또는 계산 실패가 없는 경우에만 constraint와 measured result를 publish한다.
11. publish가 불가능하면 마지막 정상 완료 결과를 보존하고 후속 layout을 등록한다.
12. RAII guard가 progress 상태를 복구한다.

pass 진입 시 dirty를 소비했으므로 수행 중 `InvalidateMeasure()`가 발생하면 `measureDirty`가 다시 true가 된다. 종료 시 dirty를 무조건 false로 덮지 않는다.

### 7.1 Measure 재진입

same-View Measure 재진입은 debug assertion에만 의존하지 않는다.

재진입 시:

- 현재 Measure pass를 poison한다.
- Measure cache를 invalid 상태로 유지한다.
- 후속 layout을 한 번 등록한다.
- 마지막 정상 완료 measured result가 있으면 반환한다.
- 완료 결과가 없으면 안전한 기본값을 반환한다.
- 재진입 결과를 publish하지 않는다.

## 8. Measure miss와 조상 cache dependency

full Measure miss와 실제 `InvalidateMeasure()` 호출은 서로 다른 사건이다.

### 8.1 정상 부모-자식 Measure

부모 Measure가 child를 동기적으로 측정하는 경우 child miss는 다음만 수행한다.

- child 자신의 Measure cache를 새로 계산
- child 자신의 Arrange cache 무효화
- cache-only 조상 전파 중 실제 조상 체인에서 첫 `measureInProgress` View 직전에 중단

활성 Measure 판정은 target View의 실제 parent chain에서만 수행한다. 다른 tree에서 Measure가 진행 중이라는 이유로 전파를 중단하지 않는다.

정상 child miss 자체는 active parent를 dirty로 만들지 않는다. 부모가 현재 pass에서 새 child result를 소비하기 때문이다.

### 8.2 child Measure 중 실제 invalidation

child Measure 수행 중 `InvalidateMeasure()`가 실제로 호출되면 cache-only miss 전파와 달리 정상 invalidation 전파를 수행한다.

- child를 dirty로 설정
- active child pass를 poison
- standalone 경계가 아니면 실제 조상 chain을 따라 parent까지 전파
- active parent Measure를 dirty 또는 poison 상태로 만들어 현재 publish 차단
- root 또는 boundary layout을 후속 등록

첫 active parent에서 단순히 중단해서는 안 된다. “clean child miss”와 “pass 중 재무효화”를 구분해야 한다.

### 8.3 scope 없는 public direct Measure

target의 실제 조상 chain에 active Measure owner가 없는 full Measure는 direct Measure로 처리한다.

1. target 자기 Arrange cache를 무효화한다.
2. target이 standalone이면 자기 경계에서 중단하고 자신만 layout root로 취급한다.
3. 일반 target이면 standalone 또는 명시적인 ownership 경계까지 조상 Measure cache를 cache-only invalidation한다.
4. 같은 범위의 조상 Arrange cache도 cache-only invalidation한다.
5. 외부 geometry를 즉시 변경하거나 자동으로 declarative Arrange를 강제하지 않는다.

조상 Measure cache를 함께 무효화해야 하는 이유는 부모 Arrange가 non-`MATCH_PARENT` child를 다시 Measure하지 않고 저장된 measured result를 사용할 수 있기 때문이다.

### 8.4 unrelated View Measure

Measure callback 또는 LayoutManager가 현재 owner와 무관한 tree의 View를 직접 Measure하면 그 View는 direct Measure 규칙을 적용한다.

thread-local Measure depth나 “어딘가에 Measure가 진행 중”이라는 전역 상태로 정상 재귀를 판정하지 않는다.

## 9. Arrange-owned child Measure

known first-party Arrange producer가 child를 Measure하고 같은 Arrange pass에서 결과를 소비하는 경우에만 경량 owner scope를 사용할 수 있다.

scope의 목적은 다음과 같다.

- clean child Measure가 조상 Measure cache 전체를 불필요하게 무효화하지 않음
- child 결과가 현재 Arrange에서 소비되었음을 표시
- child pass 중 재무효화·재진입·실패 시 active Arrange owner를 poison
- 실패 시 후속 Arrange 등록

scope는 Arrange callback 전체를 무조건 내부 작업으로 취급하지 않는다. framework가 소유하는 확인된 child Measure 호출만 감싼다.

외부 callback이 hidden state에 따라 child를 측정하거나 layout 결과를 바꾸면 callback 작성자가 명시적으로 Measure 또는 Arrange invalidation을 요청해야 한다.

경량 owner scope를 1차 구현에서 생략해도 보수적인 direct Measure 전파를 사용하면 correctness는 유지할 수 있다. 이 경우 불필요한 조상 Measure miss가 증가할 수 있으므로 계측 후 scope를 활성화한다.

## 10. Recycler 전용 layout owner scope

recycler item은 표준 View tree Measure/Arrange recursion과 다른 소유권을 가진다. item의 measured result와 physical bounds는 recycler layouter와 scroller가 직접 관리한다.

일반 direct Measure 전파를 그대로 적용하면 item miss마다 scroller와 recycler 조상 Measure cache를 불필요하게 무효화할 수 있다.

### 10.1 scope 설치 위치

scope는 특정 built-in layouter의 `LayoutChunk()` 내부에만 설치하지 않는다. recycler가 ItemsLayouter virtual API를 호출하는 중앙 경계에서 설치한다.

다음과 같이 item을 생성·측정·배치할 수 있는 모든 진입을 포함한다.

- 전체 child layout
- 수직 scroll
- 수평 scroll
- cache window 확장
- recycle/rebind 후 layout
- custom layouter 호출

scope 진입 시 현재 recycler owner와 제한적인 pass generation을 기록하고, 종료 시 owner 상태를 검증한다.

### 10.2 scope 규칙

- 대상 item이 실제 recycler dependency subtree에 속하는지 확인한다.
- item 자신의 Measure/Arrange cache는 정상적으로 처리한다.
- 일반 조상 cache-only 전파는 recycler ownership 경계에서 멈춘다.
- nested recycler에서는 가장 가까운 실제 owner를 선택한다.
- item dirty 잔존, 재진입, 미완료, 예외, 조기 반환 또는 owner generation 변경을 failure로 처리한다.
- failure 시 recycler Arrange를 poison하고 후속 recycler layout을 scope당 한 번 등록한다.
- clean 완료에서만 scope generation을 확정한다.

범용 View Measure cache에는 이 generation을 확장하지 않는다.

## 11. Arrange-before-Measure 처리

public Arrange는 Measure보다 먼저 호출될 수 있다. 이 호출은 현재 동작처럼 full Arrange를 수행할 수 있지만 다음 규칙을 적용한다.

- `measureCacheValid == false`이면 Arrange cache hit를 허용하지 않는다.
- Measure가 valid하지 않은 상태에서 실행한 Arrange 결과는 Arrange cache로 publish하지 않는다.
- producer와 child 배치는 현재 available measured state를 사용해 실행한다.
- 이후 full Measure가 완료되면 자기 Arrange cache를 반드시 무효화한다.

이 규칙은 미측정 기본값으로 계산된 child bounds 또는 producer side effect가 반복 cache hit로 고착되는 것을 막는다.

controller-managed layout은 항상 Measure 후 Arrange 순서를 유지한다.

## 12. effective scale 최적화

### 12.1 generic invalidation과 scale reset 분리

일반 `InvalidateMeasure()`에서 effective scale을 항상 reset하지 않는다.

effective scale은 다음 실제 원인에서만 subtree reset한다.

- UI scale 변경
- scale policy 변경
- parent 변경 또는 reparent
- scale 상속 context 변경

child removal에서도 제거된 subtree의 inherited scale cache를 재귀적으로 reset한다.

scale reset은 다음 상태도 함께 갱신한다.

- `effectiveScaleSyncRequired = true`
- subtree Measure cache invalid
- subtree Arrange cache invalid
- logical bounds context invalid
- active pass라면 context poison

### 12.2 actor property 동기화

effective scale actor property를 매 Measure마다 읽지 않는다.

다음 상태를 사용한다.

- `effectiveScaleSyncRequired`
- 내부 cached effective scale

내부 동기화 절차:

1. cached effective scale을 확정한다.
2. `effectiveScaleSyncRequired = false`를 setter 전에 consume한다.
3. `(target, effectiveScalePropertyIndex)` one-shot token을 push한다.
4. actor property setter를 호출한다.
5. 가장 이른 registered-property handler에서 matching token을 즉시 consume한다.
6. 기존 fitting, render-effect 및 decoration 갱신은 계속 수행한다.
7. 이후 동기 property signal callback이 같은 property를 다시 쓰면 token이 없으므로 외부 write로 인식하고 `effectiveScaleSyncRequired = true`로 되돌린다.
8. setter 실패 또는 handler 미발화 시 RAII destructor가 token을 회수하고 sync-required를 다시 설정한다.

animation이 registered effective-scale property를 바꾸는 경로에서는 animation hook이 sync-required를 설정해야 한다.

## 13. raw Measure fast key

raw constraint key는 초기 구현에 포함하지 않는다.

현재 min/max 정규화는 여러 raw 음수 constraint를 동일한 effective constraint로 collapse할 수 있다. 기존 cache 의미는 정규화된 constraint와 기존 float 비교 의미를 기준으로 한다.

프로파일에서 정규화 전 비용이 실제 병목으로 확인된 경우에만 다음 2단계 구조를 검토한다.

- raw key: exact 호출을 찾는 선택적인 alias
- normalized key: correctness를 결정하는 canonical key

raw key 불일치만으로 miss를 확정하지 않는다. normalized key 비교를 계속 수행한다. raw key는 bit-exact 또는 canonical key보다 엄격한 동치만 허용한다.

## 14. Arrange 상태 모델

각 View에 다음 상태를 둔다.

- `arrangeCacheValid`
- `arrangeDirty`
- `arrangeInProgress`
- `arrangePassPoisoned`
- `lastArrangeInput`
- 마지막으로 완료된 final arranged bounds
- logical arranged bounds
- `logicalContextValid`
- `logicalContextPoisonedDuringPass`
- `directionResolveRequired`
- `directionResolveInProgress`
- cached effective direction
- automatic direction mirroring enabled 여부

기본 Arrange cache key에 다음을 넣지 않는다.

- actor에서 읽은 현재 parent width
- actor에서 읽은 current direction
- 범용 arrange generation

필요한 변경은 invalidation과 `directionResolveRequired`로 표현한다.

## 15. Arrange cache hit

다음 조건을 모두 만족할 때만 hit로 처리한다.

1. `measureCacheValid == true`
2. `arrangeCacheValid == true`
3. `arrangeDirty == false`
4. `arrangeInProgress == false`
5. `arrangePassPoisoned == false`
6. `logicalContextValid == true`
7. `directionResolveRequired == false`
8. 네 축의 input bounds가 `lastArrangeInput`과 exact match

hit에서는 cached final arranged bounds를 즉시 반환한다.

다음 작업을 전혀 수행하지 않는다.

- actor geometry read/write
- provisional self bounds 적용
- Arrange callback 조회와 호출
- LayoutManager Arrange
- `OnArrange()`
- child Arrange
- standalone child traversal
- direction resolver
- lifecycle callback
- LayoutFinished 후보 생성

NaN 또는 invalid bounds는 hit 대상이 아니다.

## 16. Arrange miss transaction

Arrange miss는 다음 순서로 수행한다.

1. release 재진입 검사
2. RAII pass guard 생성
3. `arrangeInProgress = true`
4. `arrangeDirty = false`로 현재 dirty consume
5. `arrangePassPoisoned = false`
6. `logicalContextPoisonedDuringPass = false`
7. `arrangeCacheValid = false`
8. 변경된 provisional self bounds만 internal-write token으로 적용
9. callback, LayoutManager 또는 `OnArrange()` 실행
10. producer가 반환한 네 축 final bounds 검증
11. 변경된 final self bounds만 internal-write token으로 적용
12. logical bounds candidate 저장
13. 이전 final width와 새 final width가 다르면 direction resolver rearm
14. standalone child 처리
15. required인 경우에만 direction resolver 실행
16. lifecycle 처리 및 LayoutFinished 후보 생성
17. Measure valid, dirty 없음, poison 없음, logical context 유효, direction pending 없음일 때만 input key와 final result publish
18. RAII guard로 progress 복구

pass 중 reparent 또는 scale-context reset이 발생하면 `logicalContextPoisonedDuringPass = true`로 설정한다.

종료부에서 logical context를 무조건 valid로 만들지 않는다. context poison이 없을 때만 candidate를 publish한다.

## 17. Arrange 재진입

same-View Arrange 재진입은 debug assertion에만 의존하지 않는다.

재진입 시:

- 현재 Arrange pass poison
- Arrange cache invalid 유지
- direction pending 보존
- 후속 layout 등록
- 마지막 정상 완료 arranged bounds가 있으면 반환
- 없으면 입력 bounds 또는 안전한 fallback 반환
- 재진입 결과 publish 금지

재진입 검사보다 cache hit 검사를 먼저 수행하지 않는다.

## 18. 결정론적 LTR/RTL resolver

현재 physical X를 다시 반전하지 않는다. 저장된 logical bounds를 기준으로 physical X를 계산한다.

- LTR: `physicalX = logicalX`
- RTL: `physicalX = parentFinalWidth - logicalX - logicalWidth`

resolver는 direct child를 대상으로 하며 다음 child만 처리한다.

- non-standalone
- logical context valid
- automatic direction mirroring enabled

target X가 actor의 event-side 값과 exact하게 다를 때만 internal-write token으로 쓴다.

### 18.1 direction required 원인

다음 사건은 `directionResolveRequired = true`로 만든다.

- 최초 Arrange
- effective layout direction 변경
- parent final width 변경
- child add/remove/reparent
- child layout mode 변경
- child full Arrange
- child logical X 또는 width 변경
- 외부 child physical X 또는 width write
- automatic mirroring ownership 변경

resolver 실행 직전에 required를 false로 선소비한다. resolver 중 새 required가 발생하면 종료부가 이를 덮어쓰지 않는다.

### 18.2 공통 direction signal

모든 View가 반드시 거치는 공통 초기화에서 direction signal을 연결한다. derived `OnInitialize()`가 base 구현을 호출한다고 가정하지 않는다.

handler는 다음을 수행한다.

- cached effective direction 갱신
- `directionResolveRequired = true`
- Arrange invalidation
- direction-dependent fitting 요청

hit 경로에서 direction 또는 parent width를 actor property로 다시 읽지 않는다.

### 18.3 active parent Arrange 단계

- parent idle: required 설정, parent Arrange invalidation 및 후속 등록
- parent producer 또는 pre-resolver: required만 유지하여 현재 pass resolver가 처리
- parent resolver 실행 중: current resolver pass poison, required 재설정, 후속 pass 등록

현재 resolver가 child Arrange를 재귀 호출하지 않고 LayoutFinished가 pass 이후 발신되는 구조에서는 `arrangeInProgress`와 `directionResolveInProgress` 두 상태면 충분하다.

## 19. 직접 View Arrange의 방향 보장

View 하나에 full Arrange를 직접 호출하면 다음을 확정한다.

- 그 View 자신의 logical/final bounds
- 그 View가 소유한 direct child의 physical LTR/RTL 위치
- 그 pass에서 재귀적으로 Arrange된 subtree

그 View 자신의 parent-relative RTL X는 부모 resolver가 소유한다. child Arrange만 호출했을 때 자기 parent-relative X까지 즉시 고치기 위해 부모 geometry를 읽거나 쓰지 않는다.

## 20. Internal property write token

내부 geometry와 effective-scale write는 scoped LIFO token stack으로 구분한다.

token은 최소한 다음 정보를 가진다.

- token id
- target View identity
- property index
- consumed 여부
- 선택적인 owner/write category

### 20.1 처리 절차

1. framework-owned setter 직전에 token push
2. 가장 이른 matching property handler에서 즉시 consume
3. nonmatching target/index handler는 token을 소비하지 않고 외부 write 처리
4. nested internal setter는 새 token을 push하고 LIFO 처리
5. setter 반환 시 token이 미소비이면 RAII destructor가 제거
6. 실패 시 필요한 invalidation 또는 sync-required를 복구

token stack이 thread-local storage를 사용하더라도 ambient depth처럼 판정하지 않는다. 항상 target과 index가 정확히 일치해야 한다.

### 20.2 적용 범위

- provisional/final self bounds
- direction resolver의 child X
- transition geometry
- scroll content/scroller geometry
- recycler item/scroller geometry
- framework-owned component layer geometry
- effective-scale actor property

custom Arrange callback이 직접 호출하는 setter는 framework token으로 감싸지 않는다.

## 21. 외부 geometry write

중앙 property hook에서 다음 index를 모두 감시한다.

- position vector
- position X/Y
- size vector
- size width/height

matching internal token이 없으면 외부 write다.

외부 write 처리:

1. 자기 Arrange cache cache-only invalidation
2. active self Arrange pass poison
3. ownership 또는 standalone 경계까지 관련 조상 Arrange cache cache-only invalidation
4. X/width 또는 composite write이면 direct parent direction resolver rearm
5. 즉시 declarative layout scheduling은 하지 않음

현재 hook은 setter 이후 호출되고 이전 값을 제공받지 않는다. 따라서 별도 shadow state 또는 pre-set interception이 없으면 실제 값 변경 여부를 정확히 판단할 수 없다.

1차 구현에서는 모든 성공한 외부 geometry setter를 보수적으로 invalidation한다. 내부 hot path는 setter 전 exact 비교로 동일 값 write를 제거한다.

geometry animation의 매 frame이 일반 property hook을 통과한다고 가정하지 않는다. 기본적으로 animation은 transient physical geometry를 소유하고, 다음 명시적 declarative layout에서 계산 결과가 다시 권위를 가진다.

### 21.1 base property hook 계약

first-party derived View는 geometry index를 common base hook으로 전달해야 한다.

외부 derived View가 `OnPropertySet()`을 override하면서 base를 호출하지 않으면 중앙 geometry 감시를 보장할 수 없다. 다음 중 하나가 필요하다.

- public extension 계약에 geometry index의 base delegation을 명시
- 장기적으로 override 우회가 불가능한 더 낮은 공통 hook 제공

초기 구현은 first-party delegation을 전수 감사하고 외부 계약을 문서화한다.

## 22. Automatic direction mirroring 경계

모든 non-standalone child를 자동으로 mirroring하지 않는다.

- 자체 RTL 배치를 하던 component는 LTR logical bounds만 생성하도록 변경한다.
- common resolver와 component 자체 resolver의 이중 mirroring을 제거한다.
- scroll offset이 physical geometry를 소유하는 content는 automatic mirroring에서 opt-out한다.
- recycler scroller와 item은 recycler owner가 physical position을 소유한다.
- standalone child는 기존 경계를 유지한다.
- transition은 logical context validity와 mirroring opt-out을 존중한다.

opt-out은 타입 추론이 아니라 명시적인 내부 ownership state로 표현한다.

## 23. Fitting과 natural-size 갱신

fitting을 LayoutFinished에 의존시키지 않는다. Arrange cache hit에서는 LayoutFinished가 발생하지 않으며 resource-ready가 항상 UI layout pass를 예약하는 것도 아니다.

### 23.1 source-driven fitting

다음 원인에서 fitting update를 직접 요청한다.

- visual 등록
- visual 교체
- visual 제거
- resource-ready
- fitting mode 변경
- padding 변경
- effective scale 변경
- effective direction 변경
- size 변경

processor는 실행 시작 전에 registered flag를 먼저 clear한다. 처리 중 새 요청이 발생하면 다시 등록할 수 있어야 한다.

persistent pending logical size는 두지 않는다. 실행 시점의 현재 size를 사용하고 이후 size 변경이 다시 요청한다.

### 23.2 natural-size Measure invalidation

resource-ready visual이 natural size에 기여하고 relayout이 필요하면 scene 연결 여부와 무관하게 Measure를 invalidation한다.

다음 경로를 포함한다.

- background 등록
- background 교체
- background 제거
- background resource-ready
- generic natural-size visual resource-ready

root와 child/leaf plain View가 같은 source-driven 경로를 사용한다.

## 24. LayoutManager invalidation 계약

공개 virtual API를 변경하지 않는다.

내부 implementation storage에 owner backpointer를 두고 protected non-virtual helper를 제공한다.

- `InvalidateOwnerMeasure()`
- `InvalidateOwnerArrange()`

attach에서 owner를 연결한다. manager 교체가 허용되지 않는 동안 replacement lifecycle을 추가하지 않는다.

built-in manager setter는:

- equality guard
- layout 영향에 따른 owner invalidation
- 영향 분류가 불확실하면 보수적 Measure invalidation

외부 LayoutManager도 hidden mutable state가 결과에 영향을 주면 대응 helper를 호출해야 한다.

## 25. Producer 정비

범용 cache 활성화 전에 모든 Measure/Arrange producer를 분류한다.

### 25.1 순수 계산 producer

clean hit에서 안전하게 생략한다.

### 25.2 geometry 변화 시 side effect producer

full Arrange miss에서만 실행하고 독립적인 world/window/animation notification이 필요한 경우 별도 source 경로를 유지한다.

### 25.3 fitting producer

fitting 작업을 source-driven processor로 이전한다.

### 25.4 상태성 producer

internal flag 또는 model 변경이 layout 결과를 바꾸면 해당 setter에서 Measure 또는 Arrange를 invalidation한다. Core relayout 요청만으로 UI cache freshness를 표현하지 않는다.

### 25.5 직접 child layout producer

child를 직접 Measure/Arrange하는 producer는 다음을 명시한다.

- Measure ownership
- Arrange ownership
- logical/physical position ownership
- direction mirroring opt-out
- failure 시 owner poison

### 25.6 불필요한 hot-path log

결과를 그대로 반환하는 no-op Arrange에 남아 있는 release log는 제거하거나 debug 수준으로 낮춘다. 모든 text 계열 no-op Arrange를 함께 감사한다.

### 25.7 외부 producer 계약

callback 또는 custom producer는 다음 계약을 따른다.

- 동일 input과 clean state에서 결과가 결정적
- hidden state 변경 시 명시 invalidation
- 반복 호출 자체에 필수 side effect를 두지 않음
- child 직접 배치 시 direction과 cache ownership 명시
- callback 등록·교체 시 invalidation

계약을 만족하지 않는 producer를 위해 영구적으로 범용 cache를 끄지 않는다.

## 26. Transition과 LayoutFinished

transition은 다음을 준수한다.

- logical context가 유효한 child만 logical-to-physical 변환
- automatic mirroring opt-out
- 모든 framework-owned geometry setter에 internal-write token
- direction resolver 완료 후 snapshot 수집

LayoutFinished 의미는 “Arrange API 호출”이 아니라 “실제 full Arrange 작업 완료”다.

- full Arrange 완료 View만 후보 생성
- cache hit에서는 후보 미생성
- 같은 layout episode의 복수 full Arrange는 마지막 완료 결과 사용
- controller 밖의 수동 Arrange는 기존 수집 계약 유지
- fitting은 이 signal에 의존하지 않음

## 27. 폐기하거나 수정해야 할 의견

### 27.1 Measure hit 전에 Trait traversal이 수행된다는 의견

폐기한다. hit 판정 전에 callback과 LayoutManager lookup/dispatch가 실행되지 않으며 min/max getter는 보유 storage를 읽는다.

### 27.2 raw Measure key를 초기 필수 최적화로 도입

폐기한다. hit 전 지배 비용을 먼저 제거해야 하며 raw 입력은 기존 normalized constraint 동치보다 엄격해야 한다. 이득도 계측되지 않았다.

### 27.3 raw key를 canonical key로 사용

폐기한다. 서로 다른 음수 raw constraint가 min/max 정규화 후 같은 effective constraint가 될 수 있다.

### 27.4 dirty sentinel을 계속 상태 모델로 사용

수정한다. 현재 정상 constraint와 충돌하지 않아 동작 가능하지만 valid/dirty/in-progress/poison 구분이 어렵고 새 cache 상태와 결합하면 유지보수 위험이 커진다. 명시 상태로 이관한다.

### 27.5 이미 dirty이면 invalidation 즉시 반환

폐기한다. pass 중 재무효화, 조상 전파 및 controller 등록을 유실할 수 있다. pending set 또는 별도 propagation 상태로 중복을 줄인다.

### 27.6 모든 View에 invalidation generation 도입

폐기한다. early-return 제거와 dirty consume/publish만으로 정상 동기 pass의 재무효화를 관찰할 수 있다. generation은 recycler 같은 비표준 owner scope에만 제한한다.

### 27.7 모든 부모 Measure에 범용 dependency scope 도입

폐기한다. 정상 child miss는 첫 active Measure 조상 직전에서 cache-only 전파를 중단하면 충분하고, 실제 재무효화는 일반 invalidation 전파가 parent publish를 막는다. 범용 scope는 상태와 비용을 늘린다.

### 27.8 global Measure depth로 direct/recursive 판정

폐기한다. callback이 무관한 tree의 View를 측정하면 잘못된 owner로 분류한다. target의 실제 ancestor chain만 본다.

### 27.9 모든 child full Measure에서 조상 Measure를 무조건 무효화

폐기한다. 정상 부모 Measure, Arrange-owned Measure 및 recycler item 측정에서 과잉 invalidation을 일으킨다.

### 27.10 direct child Measure에서 조상 Arrange만 무효화

폐기한다. 부모 Arrange가 저장된 child measured result를 사용할 수 있으므로 조상 Measure cache도 무효화해야 한다.

### 27.11 Measure 전에 실행한 Arrange 결과를 즉시 cache

폐기한다. 미측정 기본값으로 계산된 child geometry가 cache hit로 고착될 수 있다. Measure valid 전에는 full Arrange를 허용하되 cache publish하지 않는다.

### 27.12 effective-scale sync-in-progress boolean

폐기한다. property signal이 setter 반환 전에 동기 실행되므로 callback의 외부 rewrite를 내부 write로 오인할 수 있다.

### 27.13 per-View acting-depth로 internal write 판정

폐기한다. 부모 resolver와 recycler가 다른 target View의 geometry를 쓸 수 있다.

### 27.14 thread-local/global ambient write depth

폐기한다. 내부 setter 중 동기 callback이 수행한 외부 write까지 억제할 수 있다.

### 27.15 Arrange 전체를 internal-write scope로 처리

폐기한다. custom callback의 외부성 geometry write를 내부 write로 숨긴다.

### 27.16 post-set hook에서 실제 값 변경 여부 판정

폐기한다. 이전 값이 없으므로 정확한 비교가 불가능하다. shadow state 또는 pre-set interception 전에는 보수적으로 invalidation한다.

### 27.17 current physical X를 다시 RTL 반전

폐기한다. 반복 적용이 멱등적이지 않고 LTR 복원 근거가 없다.

### 27.18 Arrange마다 무조건 direction resolver 실행

폐기한다. 안정 상태에서 O(children) actor read/write를 반복하여 Arrange cache 이득을 훼손한다.

### 27.19 direction 또는 parent width를 actor에서 읽어 hit key 구성

폐기한다. zero-touch hit를 깨고, 필요한 변경은 common direction signal·Arrange input·final width 비교·required bit로 표현할 수 있다.

### 27.20 별도 parent-width cache key를 항상 유지

폐기한다. resolver가 사용하는 parent final width는 cached Arrange result에 포함되고 normal width 변경은 Arrange miss다. 별도 key는 중복이다.

### 27.21 범용 direction generation을 hit 필수조건으로 사용

기본안에서는 폐기한다. 같은 direction event가 required bit와 Arrange invalidation을 설정하면 generation은 중복이다. 이벤트 유실을 generation 자체가 해결하지도 못한다.

### 27.22 Arrange 종료 시 logical context를 무조건 valid로 설정

폐기한다. pass 중 reparent 또는 scale-context reset을 덮어쓸 수 있다.

### 27.23 모든 non-standalone child 자동 mirroring

폐기한다. scroll content, recycler item/scroller 및 component-owned physical geometry와 충돌한다.

### 27.24 child Arrange만으로 자기 parent-relative RTL X까지 확정

폐기한다. parent-local 위치는 부모 resolver의 책임이다.

### 27.25 fitting을 LayoutFinished에 계속 연결

폐기한다. cache hit와 resource-ready 경로에서 필수 fitting이 누락될 수 있다.

### 27.26 fitting을 위한 persistent pending logical size

폐기한다. 현재 size를 사용하고 source 변경이 새 요청을 등록하는 방식이 stale pending 값을 피한다.

### 27.27 4-state Arrange phase enum

현재 제어 흐름에서는 폐기한다. pass 전체와 resolver 구간의 두 progress 상태, dirty, poison 및 required로 필요한 상태를 표현할 수 있다.

### 27.28 공개 LayoutManager virtual API 변경

폐기한다. ABI 위험 없이 내부 owner helper로 해결할 수 있다.

### 27.29 외부 producer 영구 no-cache

폐기한다. external descendant 하나가 전체 조상 subtree cache를 계속 막을 수 있다. 명시 invalidation 계약을 사용한다.

### 27.30 전환기 no-cache 상태를 monotonic bit로 유지

폐기한다. detach/reparent 뒤 복구되지 않아 cache가 영구 비활성화될 수 있다. 필요하면 exact ref-counted blocker만 임시 사용한다.

### 27.31 고정 cycle 수 또는 개선율 보장

폐기한다. 플랫폼, compiler, tree 구조 및 producer에 따라 달라지므로 작업 횟수 계측과 benchmark로 판단한다.

## 28. 구현 단계

### 단계 0: 계측 기반선

- Measure/Arrange 호출 수
- cache hit/miss 수
- actor property read/write 수
- producer 및 child traversal 수
- invalidation propagation 깊이
- 후속 layout 등록 수
- direction resolver 실행/write 수

기존 layout result와 주요 call count를 기록한다.

### 단계 1: 명시 상태와 invalidation 안전성

- Measure/Arrange valid, dirty, in-progress, pass-poison 도입
- sentinel 상태 의존 제거
- invalidation early-return 제거
- pass 진입 dirty consume
- clean 종료 conditional publish
- release 재진입 및 RAII guard
- Measure-before-Arrange cache publish 조건

이 단계에서는 새로운 Arrange hit를 아직 활성화하지 않아도 된다.

### 단계 2: Measure dependency

- full Measure의 self Arrange invalidation
- target ancestor chain 기반 active Measure 판정
- 정상 재귀의 첫 active parent 직전 cache-only 전파 중단
- 실제 invalidation의 active parent dirty/poison 전파
- scope 없는 direct Measure의 조상 Measure+Arrange cache-only invalidation
- standalone 경계 처리

### 단계 3: effective scale과 property token

- generic Measure invalidation에서 scale reset 분리
- child removal subtree reset
- `effectiveScaleSyncRequired`
- earliest-handler target/index token
- geometry composite/per-axis hook
- 외부 geometry 보수적 cache invalidation

### 단계 4: 결정론적 direction

- logical bounds 저장
- common direction signal 연결
- required-bit resolver
- LTR 복원
- component-owned mirroring opt-out
- context poison

### 단계 5: Arrange cache

- input/final bounds 분리
- exact zero-touch hit
- Measure valid gate
- full Arrange transaction
- full-Arrange-only LayoutFinished

### 단계 6: Recycler와 producer

- recycler 중앙 owner scope
- 모든 layouter item-layout 진입 감사
- 제한 generation과 failure follow-up
- known Arrange-owned Measure scope
- LayoutManager owner invalidation
- producer hidden state 및 side effect 정비
- no-op release log 정리

### 단계 7: fitting과 transition

- source-driven fitting
- processor flag 선소비
- natural-size resource invalidation
- transition token과 direction ownership

### 단계 8: 선택적 미세 최적화

- raw alias key
- propagation pending token
- Arrange-owned scope 확대

각 항목은 프로파일에서 이득이 확인된 경우에만 적용한다.

## 29. 검증 계획

### 29.1 Measure

- 동일 constraint 반복 hit
- min/max 변경
- effective scale 정책별 변경
- 음수 raw constraint의 normalized 동치
- callback, LayoutManager, 기본 producer 우선순위
- pass 중 self invalidation
- child pass 중 invalidation과 active parent publish 차단
- clean recursive child miss에서 parent 재-dirty 없음
- 무관 tree 직접 Measure
- scope 없는 direct child Measure 후 조상 cache miss
- standalone direct Measure
- same-View 재진입
- Measure-before-Arrange 및 Arrange-before-Measure

### 29.2 Arrange

- 동일 네 축 input의 zero-touch hit
- 축별 단독 변경 miss
- input과 final bounds가 다른 callback
- pass 중 self/child invalidation
- reparent 및 scale-context poison
- same-View 재진입
- Measure invalid 상태에서 cache publish 안 함
- hit에서 actor read/write, producer, child traversal 및 signal 후보 0회

### 29.3 Direction

- 최초 LTR/RTL
- 동일 RTL 반복
- LTR→RTL→LTR
- parent final width 변경
- child logical X/width 변경
- direction signal 중 동기 geometry write
- child add/remove/reparent
- producer/pre-resolver/resolver 단계별 재무효화
- standalone child
- scroll/recycler opt-out
- component 이중 mirroring 제거
- transition snapshot

### 29.4 Property token

- same View/same property internal write
- parent→child cross-View write
- nested internal write
- nonmatching target/index
- token consume 후 synchronous callback external rewrite
- setter 실패
- hook 미발화
- 조기 반환
- token stack 불일치
- effective-scale registered handler consume

### 29.5 Recycler

- 전체 layout
- 수직 scroll 중 새 item fill
- 수평 scroll 중 새 item fill
- cache extent 변경
- recycle/rebind
- custom layouter
- nested recycler
- item Measure/Arrange 재진입
- item dirty 또는 미완료
- owner generation 변경
- scope당 후속 layout 1회
- 일반 조상 Measure cache의 불필요한 무효화 없음

### 29.6 Fitting과 resource

- root plain View
- child/leaf plain View
- visual 등록·교체·제거
- background 변경
- scene 연결 전후 resource-ready
- natural-size Measure invalidation
- fitting mode/padding/scale/direction 변경
- processor 실행 중 재요청
- Arrange hit 상태의 fitting

### 29.7 Producer와 manager

- built-in manager setter 동일 값/변경 값
- callback 등록·교체
- hidden state 변경 후 explicit invalidation
- side-effect producer full miss
- no-op producer hit 생략
- 직접 child layout ownership
- physical-owned child opt-out

## 30. 성능 완료 조건

다음 구조적 지표를 우선 완료 조건으로 사용한다.

1. steady Measure hit에서 actor effective-scale property read 0회
2. steady Measure hit에서 callback, manager, producer, child traversal 0회
3. steady Arrange hit에서 actor property read/write 0회
4. steady Arrange hit에서 producer, child Arrange, resolver 및 lifecycle 처리 0회
5. leaf invalidation 시 clean sibling subtree Arrange 호출 수 증가 없음
6. stable direction 상태에서 resolver 실행 0회
7. recycler item layout이 일반 조상 Measure cache를 반복 무효화하지 않음
8. pass 중 재무효화로 인한 stale publish 0건
9. token 오소비 또는 미회수 0건
10. fitting 요청 중복은 coalesce되며 재진입 요청은 유실되지 않음

wall-clock과 cycle 수는 별도 benchmark로 측정하며 고정 수치를 사전에 약속하지 않는다.

## 31. 최종 완료 기준

다음 조건을 모두 만족해야 한다.

1. 정상 Measure 결과와 final Arrange bounds가 의도한 계산 의미를 유지한다.
2. pass 중 invalidation이 cache publish로 덮이지 않는다.
3. 직접 child Measure가 stale ancestor Measure/Arrange cache를 남기지 않는다.
4. 정상 recursive Measure는 불필요하게 active parent를 dirty로 만들지 않는다.
5. Arrange-owned Measure와 recycler item Measure가 일반 조상을 과도하게 무효화하지 않는다.
6. Arrange-before-Measure 결과가 cache로 고착되지 않는다.
7. LTR/RTL이 logical bounds로부터 결정론적으로 계산되고 왕복 가능하다.
8. internal write와 synchronous callback external write가 정확히 구별된다.
9. external geometry no-op을 알 수 없는 경우 안전한 방향으로 invalidation한다.
10. resource-ready와 fitting이 LayoutFinished에 의존하지 않는다.
11. manager와 모든 producer가 명시 invalidation·ownership 계약을 가진다.
12. 공개 ABI를 변경하지 않는다.
13. 선택적 generation과 scope가 실제 필요한 경로에만 존재한다.
14. Arrange hit 판정을 위해 actor parent width 또는 direction을 읽지 않는다.
15. raw Measure fast key는 프로파일 근거 전까지 도입하지 않는다.

## 32. 결론

Measure/Arrange 성능 개선의 중심은 raw 입력 비교를 앞당기거나 모든 pass에 generation을 추가하는 것이 아니다.

우선해야 할 수정은 다음이다.

- 명시적인 cache validity와 progress 상태
- invalidation early-return 제거
- pass 진입 dirty consume과 clean 종료 publish
- 정상 recursive Measure와 direct Measure의 구분
- full Measure의 self Arrange invalidation
- exact-input zero-touch Arrange cache
- logical bounds 기반 방향 resolver
- target/property scoped write token
- recycler의 명시적인 layout ownership
- source-driven fitting과 natural-size invalidation
- manager와 producer의 invalidation 계약

일반 Measure는 실제 ancestor chain과 dirty 전파만으로 정확성을 확보할 수 있으므로 범용 generation과 복잡한 dependency scope를 도입하지 않는다. owner scope와 generation은 Arrange-owned 측정 및 recycler처럼 표준 재귀를 벗어난 경로에만 제한한다.

이 구조는 layout 결과의 정확성을 유지하면서 steady Measure와 Arrange hit를 상수 시간의 상태·key 검사로 줄인다. 동시에 외부 geometry, 방향 전환, resource-ready, custom producer 및 recycler 같은 비정상 hot path도 명시적인 ownership과 invalidation 계약 아래에서 안전하게 처리한다.
