### Summary

`Dali::Actor`와 중복되는 `View`의 자식 관리 API를 정리하고(`8dc92478`의 후속), 대체 경로가
완전해지도록 내부 논리 순서 동기화를 보강하는 2개 커밋입니다.
`985efa8a`: `View::Insert`와 무인자 `RemoveAllChildren()`을 삭제하고 상속된
`Actor::InsertAbove`/`InsertBelow`/`RemoveAll()`(@SINCE_2_5.35)로 대체합니다. EXIT 애니메이션을
보장하는 정책 오버로드는 dali-core에 대응물이 없으므로 `View::RemoveAll(RemovePolicy)`로
개명해 유지합니다 — 기존 `Actor::Remove(Actor)` 대 `View::Remove(View, RemovePolicy)`와 같은 구도.
`398bbabc`: append를 가정하던 `OnChildAdd`를 고쳐, 새로 생성한 자식을
`InsertAbove`/`InsertBelow`로 삽입해도 최종 actor 위치로부터 논리 인덱스를 계산해 삽입하므로
논리(layout) 순서가 actor 위치와 일치합니다(`Add()` append는 O(1) fast path 유지).
두 커밋으로 "논리 순서 == actor 순서(non-View·EXIT ghost 제외)" 불변식이 성립합니다.

### Changes

- public API: `Insert(uint32_t, View)`·무인자 `RemoveAllChildren()` 삭제,
  `RemoveAllChildren(RemovePolicy)` → `RemoveAll(RemovePolicy)` 개명(구현 본문 동일),
  `using Dali::Actor::RemoveAll;` 추가(C++ name hiding 해제 — 무인자 호출부 14곳이 의존).
- 내부: `OnChildAdded`의 `PushBack`을 `ComputeLogicalChildIndex()` 기반 삽입으로 교체 —
  판정 조건은 `OnChildOrderChanged` 재구성 필터와 동일(non-View actor·EXIT ghost 건너뜀).
- 호출부 이행: `markdown-component.cpp` 3곳(1회 호출화), reorder/이미지 샘플·매뉴얼 테스트,
  `RemoveAllChildren()` → `RemoveAll()` 12곳.
- 테스트: Layout/LayoutTransition/View-order UTC 개명·복원 + 신규(1회 호출 삽입 양방향,
  ghost 처리 쌍, non-View 건너뜀 등), tct 헤더 재생성.
- 문서/위키(영/한): 자식 관리 인벤토리 갱신, "논리 tail로 배치" 주의 문구를 새 보장 문구로 교체.

### Examples

**일괄 제거**

```cpp
// Before
parent.RemoveAllChildren();                            // 즉시 일괄 제거
parent.RemoveAllChildren(RemovePolicy::ANIMATE_EXIT);  // EXIT 애니메이션 일괄 제거

// After
parent.RemoveAll();                                    // 상속 API — in-flight EXIT ghost까지 강제 unparent
parent.RemoveAll(RemovePolicy::ANIMATE_EXIT);          // 개명만 — 사용법·의미 동일 (ghost 보존)
```

**새 자식을 논리 인덱스 `i`에 삽입 — 1회 호출로 논리·actor 순서가 함께 맞습니다**

```cpp
// Before
parent.Insert(i, child);

// After
View anchor = parent.GetChildViewAt(i); // i가 범위 밖이면 empty handle
if(anchor)
{
  parent.InsertBelow(child, anchor);
}
else
{
  parent.Add(child); // 비었거나 i >= count → append
}
```

**기존 자식을 논리 인덱스 `i`로 이동 — 방향에 따라 호출이 갈립니다**

```cpp
// j = child의 현재 논리 인덱스. dali-core가 child를 먼저 빼고 anchor를 찾기 때문에
// 앞으로 이동(j > i)은 InsertBelow, 뒤로 이동(j < i)은 InsertAbove가 인덱스 i에 안착합니다.
View anchor = parent.GetChildViewAt(i);
if(j > i) { parent.InsertBelow(child, anchor); }
else      { parent.InsertAbove(child, anchor); }
```

### API Changes

| Class | Type | API | Purpose |
|---|---|---|---|
| `Dali::Ui::View` | Removed | `void Insert(uint32_t index, View child)` | `Dali::Actor::InsertAbove`/`InsertBelow`로 대체 |
| `Dali::Ui::View` | Removed | `void RemoveAllChildren()` | `Dali::Actor::RemoveAll()`로 대체 |
| `Dali::Ui::View` | Renamed | `void RemoveAllChildren(RemovePolicy)` → `void RemoveAll(RemovePolicy policy)` | EXIT-aware 일괄 제거; `Actor::RemoveAll()`을 인자로 오버로드(동작·의미 동일) |
| `Dali::Ui::View` | Added | `using Dali::Actor::RemoveAll` | 무인자 상속 버전의 name hiding 해제 |
| `Dali::Ui::ViewImpl` | Removed | `Insert`, `RemoveAllChildren()` | 위 handle API의 미러 |
| `Dali::Ui::ViewImpl` | Renamed | `RemoveAllChildren(RemovePolicy)` → `RemoveAll(Ui::RemovePolicy policy)` | 위 handle API의 미러 |
| `Dali::Ui::View` | Unchanged | `void Remove(View child, RemovePolicy policy)` | 개별 EXIT-aware 제거; `Dali::Actor` 대응물 없음 |
| `Dali::Ui::RemovePolicy` | Unchanged | `IMMEDIATE`, `ANIMATE_EXIT` | `Remove`/`RemoveAll` 오버로드들이 공유 |

(`398bbabc`은 설치되지 않는 internal 헤더에 private non-virtual 멤버 1개만 추가 — public API 변화 없음)

### Notes

**⚠️ 인자 유무가 in-flight EXIT ghost 처리를 가릅니다.** 상속된 무인자 `RemoveAll()`은 진행 중인
EXIT ghost까지 강제로 unparent하며 EXIT를 조용히 취소하고(`OnFinished` 없음), `RemoveAll(RemovePolicy::IMMEDIATE)`는
ghost를 남겨 애니메이션을 끝내게 둡니다(옛 `RemoveAllChildren()`과 동일). 두 형태 모두 논리 자식
목록은 동일하게 유지되며, 각 경로는 UTC 쌍으로 고정되어 있습니다. 무인자 호출로 마이그레이션된
12곳은 모두 `LayoutTransition`이 없어 영향이 없습니다.

**⚠️ 그리기 순서 변화.** 기존 `Insert`는 논리 순서만 바꾸고 actor 위치는 유지하는 라이브러리 내
유일한 API였습니다. `InsertAbove`/`InsertBelow`는 둘 다 바꾸므로, 논리 인덱스 0에 삽입한 자식은
이전에는 형제 위에 그려졌으나 이제 아래에 그려집니다. 겹치지 않는 컨테이너에서는 관측되지 않으며
(이행한 모든 호출부가 해당), 두 순서를 일치시키려는 의도된 통합입니다.

**전환(transition) 의미.** 1회 호출 신규 삽입은 `ChildOrderChangedSignal`을 발생시키지 않으므로
형제들을 `LayoutChangeCause::REORDERED`로 태깅하지 않습니다. 태깅이 필요한 경우에는 종전의
`Add()` + `InsertBelow()` 2단계가 계속 동작합니다(해당 의도를 검증하는 UTC는 2단계를 유지).

**ABI.** `985efa8a`는 의도적·승인된 public API/ABI 파괴입니다(`8dc92478`과 동일 성격,
`rules/public-api-abi.md`의 redesign 조항). 삭제·개명·추가된 멤버는 모두 non-virtual이며
ABI-frozen virtual 영역은 불변, 직렬화 property key 변화 없음. `398bbabc`은 API/ABI 변경이 없습니다.
