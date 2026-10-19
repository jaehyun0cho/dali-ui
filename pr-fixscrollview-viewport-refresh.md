### Summary
- `ScrollView`가 자기 크기(viewport)만 바뀐 layout pass에서도 scroll 범위·clamp·page 정보를 다시 계산하도록 하고(`RefreshViewport()`), 그로 인해 드러난 문제를 작은 commit 세 개로 함께 고칩니다. upstream devel 위 commit 4개입니다.
- (A) layout이 scroll 위치를 다시 적용할 때마다 drag/snap 기준 캐시를 동기화합니다. viewport 확대로 clamp된 뒤 snap이 무시되거나 drag가 손가락을 따라가지 않고 반대쪽 edge를 당기던 문제와, padding 있는 view의 첫 drag가 튀던 문제를 고칩니다.
- (A) 대기 중인 `NotifyPages*` 선택은 진행 중인 scroll에 양보하고(count 변경 시 낡은 page 대신 진행 중인 page 보고), 측정 이후의 빈 layout은 count override를 해제하며 `(-1, 0)`을 한 번 보고합니다.
- (B) `ScrollToPage`가 두 mode 모두 목표 page를 기록해 partial last page를 명시적으로 선택해도 이전 page로 반올림되지 않으며, ScrollFinished가 소비하지 못한 목표(이동 거리 없는 요청, ScrollFinished handler 안의 즉시 요청)는 직접 commit합니다.
- (C) 고정 page size에서 viewport 확대로 정착된 pager의 offset이 clamp되면 page를 다시 계산해 정확히 한 번 보고합니다(todo 항목 `page-scroll-view-fixed-page-size-viewport.md` 해결·제거).
- 공개 API/ABI 변경 없음: 새 virtual 없음, `ScrollViewImpl`/`PageScrollViewImpl` 크기(568/624) 불변, 공개 header 무변경, integration header는 `RefreshViewport()` 외에 문서만 변경, 새 per-instance 상태는 View attachment의 float 하나뿐입니다.
- 이 branch에 올렸던 이전 후속 commit 6개(요청 serial, completion enum, 알림 queue, `std::function` hook 등)는 public UTC 빌드 실패와 재진입·signal 시점 변경을 일으켜 이 세 commit으로 대체했습니다.
- 신규 UTC 37개(PageScrollView 32, ScrollView 5), 변경 라인 커버리지 분석 기준 100%(후속 세 commit 127/127).

### Changes
- `scroll-view-impl.{h,cpp}`: non-virtual `RefreshViewport()` 추가(자기 크기 변경 시에만 재계산·알림), `UpdateScrollingProperties`에서 위치 적용 후 `mCurrentPosition` 동기화, animation 정착 시 clamp된 float 위치 적용, 취소된 animation의 늦은 `Finished` 무시.
- `scroll-view-layout-manager.cpp`: child loop 뒤 `RefreshViewport()` 호출로 수신자가 이번 pass의 scrollable 크기를 읽게 합니다.
- `page-scroll-view-impl.{h,cpp}`: `OnScrollableAreaChanged`에 `selectionKept`·빈 layout 보고·`offsetMoved`(settled offset은 `PAGE_GEOMETRY` attachment에 기록) 추가, page geometry 기반 재계산, snap 중 page 수 변경 시 clamp된 page 보고, `NotifyPagesInserted/Removed`의 `mNotifyInProgress`를 scope 객체로 관리.
- `PageScrollViewImpl::ScrollToPage`: 두 mode 모두 `mSnapTargetPage` 기록, 소비되지 않은 목표는 `count > 0 ? clamp(target) : 0`으로 commit; `OnScrollFinished`는 현재 page의 정착 위치(0.5 이내)에 있으면 그 page를 유지하고 정착 offset을 기록합니다.
- 부수 효과: 즉시 요청이 버린 snap의 page는 보고되지 않고, 이동 거리 없는 animated 요청도 그 page를 선택하며, 명시적으로 선택한 page라도 clamp로 offset이 움직이면 재계산됩니다(반올림된 scroll 캐시의 재clamp는 이동으로 보지 않음).
- `utc-Dali-PageScrollView.cpp`(신규)·`utc-Dali-ScrollView.cpp`: viewport clamp/drag/padding, pending 선택 양보, 빈 content, partial page 선택·retarget·중첩 요청, 고정 page size clamp, half-pixel maximum, content 없는 `RefreshViewport` 등 UTC 추가(기존 테스트 무변경).
- `todo-items/README.md`, `todo-items/page-scroll-view-fixed-page-size-viewport.md`: 해결된 todo 항목 제거.

### Examples
```cpp
// content 500, viewport 100, 고정 page size 100 (todo 시나리오)
pager.SetPageSize(Vector2(100.0f, 80.0f));
pager.ScrollToPage(4, false);        // offset 400, page 4
pager.SetRequestedWidth(350.0f);     // 범위 150 → offset 150으로 clamp
// 이전: page 4 유지, PageChanged 없음 / 이후: page 2, PageChanged(2, 5) 한 번

// viewport 160, content 500 → 4 pages, 마지막 page는 offset 340 (round(340/160) = 2)
pager.ScrollToPage(3, false);        // 이전: page 2 / 이후: page 3

// viewport 확대로 clamp된 뒤의 snap
pager.ScrollToPage(4, false);        // offset 400
pager.SetRequestedWidth(250.0f);     // offset 250으로 clamp (캐시도 250으로 동기화)
pager.SetRequestedWidth(100.0f);
pager.ScrollToPage(4, true);         // 이전: 캐시가 400이라 no-op / 이후: 400까지 snap
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
