# Improve layout performance by caching measure and arrange results

### Summary

매 layout pass마다 tree 전체를 다시 계산하던 Measure/Arrange에 cache를 도입합니다. 입력(bounds·effective layout direction·effective scale)이 그대로면 producer(`OnMeasure`/`OnArrange`/callback/`LayoutManager`) 호출을 생략합니다.

원칙은 **cache는 계산을 생략할 뿐 결과를 바꾸지 않는다**입니다. cache hit도 subtree를 순회하며 actor geometry reconcile·RTL mirror·LayoutFinished 발신을 그대로 수행하므로, layout 이후의 Position/Size는 이전과 동일합니다. 달라지는 것은 pre-existing bug 수정뿐입니다.

새 public API는 전부 additive(new virtual 0개, public header에 `std::` 0개)라 ABI가 유지되고, 기존 앱은 코드 수정 없이 그대로 동작합니다. regression: internal 409/409 · foundation 1989/1998(실패 9건은 pre-existing CanvasView headless) · components 191/191, 신규 UTC 142개.

### Changes

- **Arrange cache**: 입력이 같으면 이전 arrange 결과를 재사용합니다. hit는 subtree prune이 아니라 replay라서 관찰 가능한 동작이 miss와 동일합니다.
- **`ArrangePolicy`**: producer 실행 정책입니다. 기본값 `ARRANGE_IF_CHANGED`는 cache 적격이고, world 좌표·native surface 등 layout invalidation이 추적하지 않는 상태에 의존하는 producer만 `ARRANGE_ALWAYS`로 opt-out합니다(in-library는 VideoView·WebView·RecyclerView·ScrollViewLayoutManager). policy는 instance에 저장되고 subclass에 상속됩니다.
- **Invalidation 정비**: pass 중 재invalidation 유실, out-of-band `Measure()`/`Arrange()`, reparent, effective scale, natural size(resource-ready), fitting, child order, layout direction 변경이 모두 올바르게 cache를 무효화하도록 고쳤습니다.
- **LayoutManager 계약**: manager 자기 state를 바꾸는 setter는 신설된 protected `InvalidateOwnerMeasure()`/`InvalidateOwnerArrange()`를 호출해야 합니다. built-in Stack/Grid/Flex setter는 배선을 마쳤습니다.
- **RTL mirror**: actor 좌표를 다시 반전하던 비멱등 방식을 logical arranged bounds 기반의 결정적 계산으로 바꿨습니다. arrange된 적 없는 child는 건드리지 않습니다.
- **결과가 달라지는 bug fix**: CheckBox가 RTL에서 이중 mirror로 LTR처럼 그려지던 문제 등 — 전체 목록과 migration 안내는 docs/layout-cache-migration.md 참고.

### Examples

```cpp
// ViewImpl 파생: OnArrange가 layout invalidation이 추적하지 않는 상태에
// 의존하면 constructor에서 opt-out한다. 기본값은 ARRANGE_IF_CHANGED.
MyViewImpl::MyViewImpl()
: ViewImpl()
{
  SetArrangePolicy(ArrangePolicy::ARRANGE_ALWAYS);
}
```

```cpp
// callback도 설치할 때 policy를 지정할 수 있다. 1-arg overload는 ARRANGE_IF_CHANGED.
view.SetArrangeCallback(ArrangeCallback::New(&MyArrange), ArrangePolicy::ARRANGE_ALWAYS);
```

```cpp
// LayoutManager가 자기 state를 가지면 setter가 owner를 invalidate해야 한다.
void MyManager::SetGap(float gap)
{
  if(mGap == gap) { return; }
  mGap = gap;
  InvalidateOwnerMeasure(); // 배치만 바뀐다면 InvalidateOwnerArrange()
}
```
