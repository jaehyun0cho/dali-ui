# Layout Cache Migration Notes

이 문서는 Measure/Arrange 캐시 도입으로 바뀐 **관찰 가능한 동작**과, 기존 코드에서
확인해야 할 항목을 정리한다. 캐시의 설계와 계약 전문은
[layout-structure.md](layout-structure.md)의 "Layout caching and the
measure/arrange contract" 및 "Invalidation" 절에 있다.

이 문서에서 **producer**는 measure/arrange 구현을 가리키는 약칭이다. 즉
`OnMeasure` / `OnArrange` override, attach된 `LayoutManager`, 그리고
`MeasureCallback` / `ArrangeCallback`이다.

원칙은 하나다. **캐시는 계산을 생략할 뿐 결과를 바꾸지 않는다.** 아래 목록에서
"결과가 달라진다"고 표시된 항목은 모두 기존 버그의 수정이며, 그 외에는 layout이
적용된 뒤의 Position/Size가 이전과 동일하다.

---

## 1. 반드시 확인해야 할 항목

### 1.1 `LayoutManager` 파생 클래스가 자기 상태를 가지고 있는가

manager에 보관된 상태(orientation, spacing, 정의 테이블 등)는 owner의 measure
캐시 키에도 arrange 캐시 키에도 들어가지 않는다. 해당 상태를 바꾸는 setter는
반드시 다음 중 하나를 호출해야 한다.

```cpp
void MyManager::SetGap(float gap)
{
  if(mGap == gap) { return; }   // 같은 값이면 아무것도 예약하지 않는다
  mGap = gap;
  InvalidateOwnerMeasure();     // 배치만 바뀐다면 InvalidateOwnerArrange()
}
```

`InvalidateOwnerMeasure()` / `InvalidateOwnerArrange()`는 `LayoutManager`의
protected 멤버로 새로 추가되었다. attach 전에 호출해도 안전하다(no-op). 다만
**상태를 바꾸는 setter에서** 호출하는 것이 기본이다. producer 안의 조건부 요청도
보존되고 자동 후속 처리되지만, 현재 pass의 결과를 즉시 바꾸는 수단으로 의존해서는
안 된다. 무조건 반복하는 producer는 수렴하지 않는다(§1.4).

호출이 없는 setter가 실제로 잃는 것:

- **Measure가 읽는 상태**: measure 캐시는 무조건 동작하므로, 무효화 없는 변경은
  이 view에 무효화가 닿거나 이 view의 키 입력(정규화된 constraint, 유효
  스케일)이 바뀌기 전까지 반영되지 않는다. 무관한 pass가 도는 것만으로는
  회복되지 않는다. 조상이 miss해도 조상 구현이 이 view를 같은 입력으로 다시
  측정하므로 이 view는 여전히 hit하고, 형제의 무효화는 위로만 전파되어 이
  view에 닿지 않는다. 이 위험은 이전부터 있었고 이제 계약으로 명문화되었다.
- **Arrange만 읽는 상태**: 기본 정책인 `ArrangePolicy::IF_CHANGED`에서는 이전
  결과가 재사용될 수 있으므로 무효화 없는 변경이 반영되지 않을 수 있다.
  `ArrangePolicy::ALWAYS`도
  pass 자체를 예약하지는 않으므로 setter의 무효화 호출은 어느 정책에서나 필요하다.

라이브러리 내장 manager(Stack/Grid/Flex)는 이번 변경으로 모두 배선되어, **직접
setter 호출만으로** 재배치가 예약되고 반영된다. 이전에는 이 직접 호출이
unspecified였다(아무것도 pass를 예약하지 않음).

### 1.2 커스텀 arrange producer의 실행 정책을 확인한다

arrange producer의 기본값은 `ArrangePolicy::IF_CHANGED`다. 이전 결과를
재사용할 수 있으면 `OnArrange()`, 1인자 `SetArrangeCallback(callback)`, 그리고
`LayoutManager::Arrange()`가 호출되지 않을 수 있다.

다음과 같이 layout invalidation이 추적하지 않는 상태를 읽거나 매 pass 외부 작업을
수행해야 하는 producer만 명시적으로 opt-out한다.

```cpp
// ViewImpl 파생 클래스
MyViewImpl::MyViewImpl()
{
  SetArrangePolicy(ArrangePolicy::ALWAYS);
}

// 콜백
view.SetArrangeCallback(ArrangeCallback::New(&MyArrange),
                        ArrangePolicy::ALWAYS);

// LayoutManager 파생 클래스
MyManager::MyManager()
{
  SetArrangePolicy(ArrangePolicy::ALWAYS);
}
```

`ArrangePolicy::ALWAYS`가 필요한 경우:

- 조상/월드 좌표(`SCREEN_POSITION`, `WORLD_POSITION`, `WORLD_SCALE`, 윈도우 좌표)를
  읽는다
- actor 트리 밖의 표면(네이티브 플레이어, 웹 엔진 등)에 상태를 밀어 넣는다
- 입력이 같아도 배치하는 자식 집합이나 외부 작업이 달라진다
- `InvalidateArrange()`가 따라붙지 않는 상태에 의존한다

정책은 구현 인스턴스에 저장되고 파생 클래스에도 상속된다. 파생 클래스는 생성자에서
다시 정책을 설정할 수 있다. 기존 1인자 `SetArrangeCallback(callback)`은 이제
`ArrangePolicy::IF_CHANGED`를 사용하므로, callback 호출 횟수나 외부 부수 효과에
의존하던 코드는 2인자 overload로 `ArrangePolicy::ALWAYS`를 지정해야 한다.

### 1.3 measure producer가 매 프레임 호출된다고 가정하고 있지 않은가

measure 캐시는 이전부터 **무조건** 동작했으므로 이 위험 자체는 새로 생긴 것이
아니다. 이번 변경으로 달라진 것은 캐시가 **언제 비워지는지**다. 누락되어 있던
무효화가 메워지고 조상 캐시를 정리하는 경로가 새로 들어오면서, measure producer는
이전보다 **더 자주** 호출된다(적중률은 올라가지 않고 내려간다). 대신 우연한
무효화에 기대어 producer가 다시 호출되던 코드는 그 보장을 잃는다. `OnMeasure` /
`MeasureCallback` / `LayoutManager::Measure`를 per-frame tick으로 쓰고 있었다면
지금 드러난다. producer 밖의 상태를 읽는다면 그 상태를 바꾸는 쪽에서
`InvalidateMeasure()`를 호출해야 한다. 상태를 바꾸는 이벤트 시점의 setter에서
요청하는 것이 기본이며, 처리 중 조건부 후속 요청은 §1.4의 정책을 따른다.

### 1.4 레이아웃 처리 중에 무효화를 호출하고 있지 않은가

Measure/Arrange 또는 `LayoutFinished` callback 안의 invalidation도 보존됩니다.
cache 유효성, dirty, 조상 chain, 진행 중 producer의 cache publish 제한은 그대로
적용됩니다. 현재 batch가 소비하지 못한 pending은 controller가 후속 처리 기회를
예약하므로, 앱이 별도 timer나 입력으로 Layout을 깨울 필요가 없습니다.

연속 요청에는 one-shot timer와 monotonic deadline을 사용합니다. 다음 자동 처리는
직전 작업 종료 후 `max(1초 / 60, 실제 계산 및 완료 callback 비용)`만큼 기다립니다.
이는 실제 VSYNC 동기화가 아니며, 작업 시간이 포함되므로 초당 정확히 60회가 아닙니다.
다른 event가 먼저 발생해도 deadline 전에는 계산과 새 완료 기록 승격을 진행하지
않습니다. 중복 요청은 기존 예약을 공유하며 deadline을 계속 늦추지 않습니다.

`LayoutFinished`는 안정화 신호가 아니라 정상 pass 완료 신호입니다. pending이 남아도
post-process에서 View별 snapshot, Window 알림 순서로 전달됩니다. 수동·중첩 pass는
각자의 기록을 유지하므로 과거 snapshot과 현재 Actor 값이 다를 수 있습니다.
callback 안의 수동 `ProcessLayouts()`는 동기 계산하되 새 완료 알림은 다음 허용된
pre/core Relayout/post 기회까지 기다립니다.

유한한 조건부 요청은 지원하지만 무조건 반복하는 invalidation은 안정화되지 않습니다.
producer는 현재 입력의 결과를 계산하도록 작성하고, 상태 setter는 같은 값의 대입을
no-op으로 유지하십시오. 긴 callback 하나의 선점이나 전체 프로세스 CPU 상한을 이
정책이 보장하지는 않습니다.

producer가 throw한 batch는 pending을 복구하되 자동 재실행을 차단합니다. 이전 정상
pass의 완료 기록은 보존하고, 명시적 수동 처리 또는 processing 밖의 새로운 요청으로
복구할 수 있습니다. callback 예외 후 controller의 기록 보존과 실제 core의 처리 재개는
별개이며 core 수준의 예외 복구 검증이 필요합니다.

cache 철회는 즉시 재계산을 뜻하지 않으므로 후속 pass 전까지 `GetMeasuredSize()`나
Actor bounds에 직전 완료 결과가 보일 수 있습니다. 현재 pass의 정확성을 그 pass가
새로 만든 요청의 즉시 처리에 의존하지 마십시오.

---

## 2. 결과가 달라지는 항목 (버그 수정)

### 2.1 CheckBox의 RIGHT_TO_LEFT 배치

`CheckBox`는 자체적으로 한 번 미러링하고 프레임워크가 다시 한 번 미러링해서, 결과가
서로 상쇄되어 **RIGHT_TO_LEFT에서도 LEFT_TO_RIGHT처럼** 그려지고 있었다. 자체
미러링을 제거해 이제 아이콘이 오른쪽 가장자리에서 `padding.start`만큼 떨어진 곳에
놓인다(= "start"의 정의).

RTL 화면에서 CheckBox를 쓰고 있었다면 스크린샷 기준 테스트를 갱신해야 한다.

### 2.2 자식 `POSITION_X`를 직접 쓴 뒤의 RTL 미러링

RTL 미러링은 이제 자식의 **논리(logical) arranged bounds**에서 계산된다. 이전에는
자식 actor의 현재 `POSITION_X`를 다시 반전하는 방식이었고, 그것은 멱등이 아니라
같은 값에 두 번 적용하면 원래대로 돌아갔다.

결과 차이는 하나뿐이다. 부모가 자식을 arrange한 **뒤에** 그 자식의 `POSITION_X`를
바깥에서 덮어썼다면, RTL에서 그 덮어쓴 값이 더 이상 미러링 입력이 되지 않는다.
논리 좌표로 배치하고 방향 처리는 프레임워크에 맡기면 된다.

아직 한 번도 arrange된 적 없는 자식(= 논리 bounds가 없는 자식)은 이제 미러링
대상에서 제외되어 **그대로 두어진다**. 이전에는 actor의 현재 물리 좌표를 다시
반전하는 방식이라 같은 입력의 반복 pass에서 좌표가 좌우로 진동했는데(멱등이
아님), 프레임워크가 소유한 논리 좌표가 없는 자식을 건드리지 않는 것이 결정적인
동작이다. 자식을 직접 배치하고 arrange하지 않는 producer가 RTL을 원한다면
producer 자신이 방향을 처리해야 한다.

### 2.3 그 외 무효화 누락 수정

다음은 모두 "원래 갱신되었어야 하는데 갱신되지 않던" 경우다. 화면이 늦게 갱신되던
증상이 사라지는 방향이다.

- pass 도중 발생한 재무효화가 pass 종료 시 덮여 사라지던 문제
- 부모가 arrange하지 않는 자식의 재무효화가 영구히 삼켜지던 문제
- 자식 제거 / 오프스크린 루트 재연결 시 subtree의 effective scale이 낡은 값으로
  남던 문제(스케일이 섞여 보이던 증상)
- resource-ready / background visual 등록·교체·해제 후 natural size가 반영되지
  않던 문제
- fitting mode 갱신 요청이 처리 중에 다시 들어오면 유실되던 문제
- `RaiseToTop` / `LowerBelow` 등 child order 변경이 일부 View에서 누락되던 문제
- 레이아웃 방향 변경이 아무 재배치도 일으키지 않던 문제

---

## 3. 새로 추가된 API (전부 additive)

| API | 위치 | 용도 |
|---|---|---|
| `ArrangePolicy` | `layout-types.h` | arrange producer의 실행 정책 |
| `View::SetArrangeCallback(cb, policy)` | `view.h` | callback 실행 정책 지정 |
| `ViewImpl::SetArrangePolicy(policy)` | `view-impl.h` (protected) | `OnArrange` 실행 정책 지정 |
| `ViewImpl::GetArrangePolicy()` | `view-impl.h` (protected) | `OnArrange` 실행 정책 조회(설정값 미러) |
| `LayoutManager::SetArrangePolicy(policy)` | `layout-manager.h` (protected) | manager arrange 실행 정책 지정 |
| `LayoutManager::InvalidateOwnerMeasure()` | `layout-manager.h` (protected) | manager 자기 상태 변경 시 owner 무효화 |
| `LayoutManager::InvalidateOwnerArrange()` | `layout-manager.h` (protected) | 위와 같되 배치 축만 |

가상 함수는 추가되지 않았고, 공개 시그니처에 표준 라이브러리 타입은 사용하지
않았다.

---

## 4. 참고: 동작이 바뀌지 않는 것

- clean 상태에서 producer가 호출되지 않는 것은 캐시의 정상 동작이며, 그때도 actor
  geometry는 매 pass 재조정된다. 레이아웃 밖에서 덮어쓴 좌표는 캐시 적중 시에도
  똑같이 복구된다.
- `LayoutFinishedSignal()`은 pass 기준이다. 캐시 적중으로 처리된 View도 통지를
  받으며, 이는 "bounds가 바뀌었다"는 신호가 아니다.
- 무효화 전파의 coalescing은 ancestor chain을 얼마나 자주 걷는지만 바꾼다. 한 pass
  전에 쌓인 무효화는 전부 그 pass에서 처리된다.
