# Layout Cache Migration Notes

이 문서는 Measure/Arrange 캐시 도입으로 바뀐 **관찰 가능한 동작**과, 기존 코드에서
확인해야 할 항목을 정리한다. 캐시의 설계와 계약 전문은
[layout-structure.md](layout-structure.md)의 "Layout caching and the producer
contract" 및 "Invalidation" 절에 있다.

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
protected 멤버로 새로 추가되었다. attach 전에 호출해도 안전하다(no-op).

호출이 없는 setter가 실제로 잃는 것:

- **Measure가 읽는 상태**: measure 캐시는 무조건 동작하므로, 무효화 없는 변경은
  다음 무관한 pass가 돌아도 반영되지 않는다(캐시가 이전 결과를 계속 서빙).
  이 위험은 이전부터 있었고 이제 계약으로 명문화되었다.
- **Arrange만 읽는 상태**: 라이브러리 밖 manager는 IMPURE라 매 pass 재실행되므로
  "다음 무관한 pass"에 우연히 반영되지만, 그것은 보장이 아니라 우연이다. pass를
  예약하는 것은 어디에도 없으므로 앱이 idle이면 영영 반영되지 않는다.

라이브러리 내장 manager(Stack/Grid/Flex)는 이번 변경으로 모두 배선되어, **직접
setter 호출만으로** 재배치가 예약되고 반영된다. 이전에는 이 직접 호출이
unspecified였다(아무것도 pass를 예약하지 않음).

### 1.2 커스텀 `OnArrange` / `ArrangeCallback`이 캐시 대상인지 결정한다

arrange 캐시는 **opt-in**이고 기본값은 `ArrangePurity::IMPURE`다. 아무것도 선언하지
않으면 producer는 이전과 똑같이 매 pass 실행된다 — **정확하지만 느리다**.

```cpp
// View 파생 클래스: 그 타입의 New()에서 선언한다 (생성자 금지 — 파생으로 새어 나간다)
MyViewImplPtr MyViewImpl::New()
{
  MyViewImplPtr impl(new MyViewImpl());
  impl->SetArrangePurity(ArrangePurity::PURE);
  return impl;
}

// 콜백: 2-인자 오버로드
view.SetArrangeCallback(ArrangeCallback::New(&MyArrange), ArrangePurity::PURE);
```

다음 중 하나라도 해당하면 **선언하지 말 것**:

- 조상/월드 좌표(`SCREEN_POSITION`, `WORLD_POSITION`, `WORLD_SCALE`, 윈도우 좌표)를
  읽는다
- actor 트리 밖의 표면(네이티브 플레이어, 웹 엔진 등)에 상태를 밀어 넣는다
- 입력이 같아도 배치하는 자식 집합이 달라진다
- `InvalidateArrange()`가 따라붙지 않는 상태에 의존한다

purity는 **정확한 타입 단위**이며 상속되지 않는다. 라이브러리 밖에서 정의한
`LayoutManager` 파생은 purity를 선언할 수 없어 항상 재실행된다. 캐시 가능한 커스텀
레이아웃이 필요하면 `LayoutManager` 대신 View 파생 + `SetArrangePurity` 또는
`SetArrangeCallback(cb, PURE)`를 쓴다.

### 1.3 measure producer가 매 프레임 호출된다고 가정하고 있지 않은가

measure 캐시는 이전부터 **무조건** 동작했지만, 이번 변경으로 무효화 누락이 여러 건
메워지면서 캐시 적중률이 올라갔다. `OnMeasure` / `MeasureCallback` /
`LayoutManager::Measure`를 per-frame tick으로 쓰고 있었다면 지금 드러난다.
producer 밖의 상태를 읽는다면 그 상태를 바꾸는 쪽에서 `InvalidateMeasure()`를
호출해야 한다.

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
| `ArrangePurity` | `layout-types.h` | arrange producer의 순수성 선언값 |
| `View::SetArrangeCallback(cb, purity)` | `view.h` | 콜백 purity 선언 |
| `ViewImpl::SetArrangePurity(purity)` | `view-impl.h` (protected) | `OnArrange` purity 선언 |
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
