### Summary
- `LayoutOrderPolicy::PRESERVE`를 대체하는 `Dali::Ui::Extension::View::...PreservingLayout()` 함수 6종을 추가합니다. actor children 순서(그리기·hit-test 순서)만 바꾸고 부모의 layout 순서(view children 순서)는 유지합니다.
- 두 순서의 규칙을 하나로 정리했습니다. 상속된 `Dali::Actor`의 Raise/Lower 계열과 `InsertAbove()`/`InsertBelow()`는 layout 순서를 actor 순서로 동기화하고, `Add()`처럼 마지막에 붙는 삽입과 제거는 동기화하지 않습니다.
- deprecated `LayoutOrderPolicy` 오버로드와 열거형은 시그니처와 동작을 유지하고, `@deprecated` 안내만 새 함수로 바꿨습니다.
- 빌드와 테스트 실행은 하지 않았습니다.

### Changes
- `extension-api/view.h/.cpp`: `RaisePreservingLayout`, `LowerPreservingLayout`, `RaiseToTopPreservingLayout`, `LowerToBottomPreservingLayout`, `RaiseAbovePreservingLayout`, `LowerBelowPreservingLayout` 추가
- `view-data-impl`: PRESERVE 구현을 한 곳으로 모으고, deprecated 오버로드가 같은 구현을 호출
- 새 자식을 `InsertAbove()`/`InsertBelow()`로 마지막이 아닌 위치에 넣으면 layout 순서 전체를 actor 순서로 동기화 (`SyncChildrenWithActorOrder()`). 두 순서가 같던 부모에서는 결과가 이전과 같음
- `SyncChildrenWithActorOrder()`를 자식 수에 선형으로 개선: 두 순서가 이미 같은지 할당 없이 한 번 순회로 먼저 확인하고, 다를 때만 hash set으로 재구성. 첫 자식 앞에 새 자식을 `InsertBelow()`로 넣는 호출이 O(n²)에서 O(n)이 되며 동작은 같음
- PRESERVE 가드가 순서 변경 처리만 막도록 수정: 호출 중 `ChildOrderChangedSignal` 콜백에서 추가·제거한 자식이 layout 자식 목록에 반영됨
- 부모에 붙는 도중 콜백이 다시 제거한 자식은 layout 자식 목록에 넣지 않음
- 문서: 공개 헤더 주석, `docs/view-sibling-order-migration.md` 재작성, `docs/layout-structure.md`, `docs/devel-integration-history.md`, `wiki/Layout-Structures.md`, `wiki/Layout-Structures-(kr).md`
- UTC: `utc-Dali-View-order-internal.cpp` 31건, `utc-Dali-View.cpp` 2건 추가
- manual test: TC125의 deprecated 호출을 `...PreservingLayout()`과 상속된 `Dali::Actor` 호출로 교체하고, `RaiseToTopPreservingLayout`/`LowerToBottomPreservingLayout`/`RaiseAbovePreservingLayout`과 layout 순서 동기화 규칙을 검증하는 시나리오 9개 추가 (manifest revision 16)

### Examples
```cpp
// Before (deprecated)
view.RaiseToTop(LayoutOrderPolicy::PRESERVE);
view.RaiseAbove(target, LayoutOrderPolicy::PRESERVE);

// After
Dali::Ui::Extension::View::RaiseToTopPreservingLayout(view);
Dali::Ui::Extension::View::RaiseAbovePreservingLayout(view, target);

// parent에 a, b, c 순서로 추가한 상태에서
Dali::Ui::Extension::View::RaiseToTopPreservingLayout(a); // actor: b, c, a / layout: a, b, c
parent.InsertAbove(d, b); // 새 자식 d 삽입 → actor: b, d, c, a / layout: b, d, c, a (동기화)
// c.Lower() 같은 Actor 순서 변경도 같은 방식으로 layout 순서를 actor 순서에 맞춥니다.
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
