## Description of Change
View가 Dali::Actor와 중복으로 제공하던 형제 순서(sibling-order) 정책 API를 제거하고, 호출부를 dali-core의 상속 API로 대체합니다.
`LayoutOrderPolicy::UPDATE`는 상속된 `Dali::Actor::Raise/Lower/RaiseToTop/LowerToBottom/RaiseAbove/LowerBelow` 호출과 동일하게 동작했고(레이아웃 순서는 `ChildOrderChangedSignal`로 이미 동기화됨), `PRESERVE`(그리기 순서만 변경)는 `Actor::Property::DEPTH_INDEX`(@SINCE_2_5.35)로 대체됩니다.

### Summary
- `View`/`ViewImpl`/`ViewDataImpl`의 `LayoutOrderPolicy` 오버로드 6종과 `LayoutOrderPolicy` enum을 제거합니다.
- 정책 구현에만 쓰이던 `ScopedSkipChildrenUpdate`/`mSkipChildrenUpdate`와 가드 3곳을 제거합니다.
- 오버로드 제거로 불필요해진 `using Dali::Actor::Raise/Lower...` 6줄을 제거합니다(`Remove`/`RemoveAll` 재노출은 유지).
- `Remove(View, RemovePolicy)`, `RemoveAll(RemovePolicy)`, 논리 자식 접근자는 중복이 아니므로 유지합니다.
- 호출부 7곳(샘플 4, 수동 테스트 2, UTC 1), UTC 12개, 문서 1곳을 마이그레이션합니다.
- 공개 API 제거이므로 API/ABI 변경(내보낸 비가상 심볼 제거)입니다. 가상 테이블과 프로퍼티 키는 변경이 없습니다.

### Changes
- `public-api/views/view.h`, `view.cpp`, `view-impl.h`, `view-impl.cpp`, `view-types.h`: 오버로드·enum 삭제
- `internal/views/view/view-data-impl.{h,cpp}`: 구현 6개, RAII 가드, 비트필드, `OnChildAdded`/`OnChildRemoved`/`OnChildOrderChanged` 가드 삭제
- `automated-tests`: order-internal UTC 12개 → 상속 Actor 호출 테스트 6개 + `UtcDaliViewDepthIndexDoesNotChangeChildOrderInternalP`; FlexLayout UTC 1곳
- `samples`: text-render-scale 카드 포커스 시 `SetDepthIndex(1)`, 블러 시 `SetDepthIndex(0)`; layout-transition 샘플 3개는 `RaiseToTop()`
- `manual-tests`: tc-80/tc-82 `RaiseToTop()` (막 Add된 자식이라 동작 동일)
- `docs/layout-structure.md` 문구 갱신

### API Changes
| Type | API | Purpose |
|------|-----|---------|
| View | `Raise/Lower/RaiseToTop/LowerToBottom(LayoutOrderPolicy)` | 제거. 상속된 `Dali::Actor` 동명 API를 사용합니다. |
| View | `RaiseAbove/LowerBelow(View, LayoutOrderPolicy)` | 제거. 동작 차이: 제거된 오버로드는 빈 `target`을 조용히 무시했으나, 상속된 `Dali::Actor::RaiseAbove/LowerBelow`는 유효한 형제를 전제(`@pre`)로 하며 빈 핸들이면 assert합니다. 호출 전에 target 핸들을 확인해야 합니다. |
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
