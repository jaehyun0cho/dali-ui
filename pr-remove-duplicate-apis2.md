## Description of Change
deprecated로 표시했던 View/ViewImpl의 `LayoutOrderPolicy` 형제 순서(sibling-order) 오버로드와 `LayoutOrderPolicy` enum을 제거합니다.
**병합 전제 조건:** deprecation PR("Deprecate View sibling-order APIs taking LayoutOrderPolicy")이 먼저 병합되고, 이 API를 쓰던 애플리케이션이 모두 `docs/view-sibling-order-migration.md`에 따라 대체 방식으로 옮긴 뒤에만 병합합니다. 병합 직전 rebase를 하면 가이드 Evidence의 줄 번호를 다시 확인합니다.

### Summary
- `View`/`ViewImpl`/`ViewDataImpl`의 `LayoutOrderPolicy` 오버로드 6종과 `LayoutOrderPolicy` enum을 제거합니다.
- 정책 구현에만 쓰이던 `ScopedSkipChildrenUpdate`/`mSkipChildrenUpdate`와 가드 3곳을 제거합니다.
- 오버로드 제거로 불필요해진 `using Dali::Actor::Raise/Lower...` 6줄을 제거합니다(`Remove`/`RemoveAll` 재노출은 유지).
- `Remove(View, RemovePolicy)`, `RemoveAll(RemovePolicy)`, 논리 자식 접근자는 중복이 아니므로 유지합니다.
- deprecated 오버로드를 검증하던 정책 UTC 12개를 삭제합니다(대체 경로 UTC 7개는 deprecation PR에서 추가됨).
- 가이드와 `docs/layout-structure.md`를 "제거됨" 기준으로 갱신합니다.
- 공개 API 제거이므로 API/ABI 변경입니다(내보낸 비가상 심볼 12개 제거, SONAME 유지). 가상 테이블과 프로퍼티 키는 변경이 없습니다.

### Changes
- `public-api/views/view.h`, `view.cpp`, `view-impl.h`, `view-impl.cpp`, `view-types.h`: 오버로드·enum 삭제
- `internal/views/view/view-data-impl.{h,cpp}`: 구현 6개, RAII 가드, 비트필드, `OnChildAdded`/`OnChildRemoved`/`OnChildOrderChanged` 가드 삭제
- `automated-tests`: order-internal UTC에서 정책 테스트 12개 삭제
- `docs/view-sibling-order-migration.md`, `docs/layout-structure.md`: deprecated → 제거됨 문구, 근거 줄 번호 갱신

### API Changes
| Type | API | Purpose |
|------|-----|---------|
| View | `Raise/Lower/RaiseToTop/LowerToBottom(LayoutOrderPolicy)` | 제거. 상속된 `Dali::Actor` 동명 API를 사용합니다. |
| View | `RaiseAbove/LowerBelow(View, LayoutOrderPolicy)` | 제거. 동작 차이: 제거된 오버로드는 빈 `target`을 조용히 무시했으나, 상속된 `Dali::Actor::RaiseAbove/LowerBelow`는 유효한 형제를 전제(`@pre`)로 하며 빈 핸들이면 assert합니다. 호출 전에 target 핸들을 확인해야 합니다. |
| ViewImpl | 같은 6개 오버로드 | 제거. `Self().Raise()` 등 상속된 호출을 사용합니다. |
| Ui | `enum class LayoutOrderPolicy` | 제거. `PRESERVE` 의도는 `Actor::SetDepthIndex`/`Actor::Property::DEPTH_INDEX`로 표현합니다. |

### Examples
```cpp
// Before
card.RaiseToTop(LayoutOrderPolicy::UPDATE);   // 형제 순서 + 레이아웃 순서 변경
card.RaiseToTop(LayoutOrderPolicy::PRESERVE); // 그리기 순서만 변경

// After
card.RaiseToTop();     // 상속된 Dali::Actor::RaiseToTop: 형제 순서 변경, 레이아웃 순서는 ChildOrderChangedSignal로 자동 동기화
card.SetDepthIndex(1); // 형제/레이아웃 순서는 그대로, 그리기(및 히트) 순서만 위로. 되돌릴 때는 SetDepthIndex(0)
```

## Test Dependencies (optional)

- dali-core: default
- dali-adaptor: default
- dali-toolkit: default

🤖 Generated with [Claude Code](https://claude.com/claude-code)
