## Description of Change
View/ViewImpl의 `LayoutOrderPolicy` 형제 순서(sibling-order) 오버로드와 `LayoutOrderPolicy` enum을 deprecated로 표시하고, 헤더 주석과 마이그레이션 가이드로 대체 방법을 안내합니다.
API, 동작, 내보낸 심볼은 그대로 유지합니다. 실제 제거는 애플리케이션이 모두 대체 방식으로 옮긴 뒤 별도 PR("Remove View sibling-order APIs duplicating Dali::Actor")에서 진행합니다.
병합 직전 rebase를 하면 가이드 Evidence의 dali-ui 줄 번호를 다시 확인합니다.

### Summary
- `View`/`ViewImpl`의 `Raise/Lower/RaiseToTop/LowerToBottom(LayoutOrderPolicy)`, `RaiseAbove/LowerBelow(View, LayoutOrderPolicy)` 12개와 enum에 `@deprecated`를 추가합니다.
- dali-ui 기존 관례대로 문서 전용 deprecation입니다(컴파일 경고 없음, `-Werror` 빌드 영향 없음).
- 각 주석에 대체 방법을 적습니다: `UPDATE` → 상속된 `Dali::Actor` 호출, `PRESERVE` → 부모 종류에 따라 상속 호출 또는 `Actor::Property::DEPTH_INDEX`.
- `RecyclerView`/`ScrollView`/`Navigator`/`DialogContainer`의 직계 자식에는 `DEPTH_INDEX`를 쓰지 않는 예외, RecyclerView 아이템의 음수 값 금지, 부모 없는 호출, 빈 target과 형제가 아닌 target 처리도 적습니다.
- `docs/view-sibling-order-migration.md` 가이드를 추가하고 `docs/layout-structure.md`에서 연결합니다.
- 트리 안의 호출부(샘플 4, 수동 테스트 2, UTC 1)를 대체 방식으로 옮깁니다.
- 기존 정책 UTC 12개는 유지하고(제거 전까지 동작 보장), 대체 경로 UTC 7개를 추가합니다.
- 선언·심볼·가상 테이블 변경이 없으므로 API/ABI 변경은 없습니다.

### Changes
- `public-api/views/view.h`, `view-impl.h`, `view-types.h`: `@deprecated`와 대체 방법 주석만 추가 (코드 변경 없음)
- `docs/view-sibling-order-migration.md`(신규), `docs/layout-structure.md`: 가이드와 링크
- `automated-tests`: order-internal에 상속 Actor 호출 테스트 6개 + `UtcDaliViewDepthIndexDoesNotChangeChildOrderInternalP`; FlexLayout UTC 1곳 `Raise()`
- `samples`: text-render-scale 카드 포커스 시 `SetDepthIndex(1)`, 해제 시 `SetDepthIndex(0)`; layout-transition 샘플 3개 `RaiseToTop()`
- `manual-tests`: tc-80/tc-82 `RaiseToTop()` (막 Add된 자식이라 동작 동일)

### API Changes
| Type | API | Purpose |
|------|-----|---------|
| View | `Raise/Lower/RaiseToTop/LowerToBottom(LayoutOrderPolicy)`, `RaiseAbove/LowerBelow(View, LayoutOrderPolicy)` | Deprecated. 상속된 `Dali::Actor` 동명 API 또는 `DEPTH_INDEX`로 대체합니다. 동작은 그대로입니다. |
| ViewImpl | 같은 6개 오버로드 | Deprecated. `Self().Raise()` 등 또는 `Self().SetDepthIndex()`로 대체합니다. |
| Ui | `enum class LayoutOrderPolicy` | Deprecated. 위 오버로드와 함께 제거될 예정입니다. |

### Examples
```cpp
// Before
card.RaiseToTop(LayoutOrderPolicy::UPDATE);   // 형제 순서 + 레이아웃 순서 변경
card.RaiseToTop(LayoutOrderPolicy::PRESERVE); // 앱이 소유한 View 부모: 그리기 순서만 변경

// After
card.RaiseToTop();     // 상속된 Dali::Actor::RaiseToTop: 레이아웃 순서는 ChildOrderChangedSignal로 자동 동기화
card.SetDepthIndex(1); // 형제/레이아웃 순서는 그대로, 그리기(및 히트) 순서만 위로. 되돌릴 때는 SetDepthIndex(0)
```

## Test Dependencies (optional)

- dali-core: default
- dali-adaptor: default
- dali-toolkit: default

🤖 Generated with [Claude Code](https://claude.com/claude-code)
