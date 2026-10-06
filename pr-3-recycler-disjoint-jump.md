### Summary
- `LinearItemsLayouterImpl::ScrollBy`가 기존 활성 window와 겹치지 않는 위치로 한 번에 스크롤하는 경우를 처리합니다.
- 뒤쪽 item으로 점프하면 기존에는 이전 window 끝(`oldLast + 1`)부터 채워서, 건너뛴 구간의 item까지 bind된 채 남았습니다.
- 이 경우 새 첫 item이 이전 window 끝의 offset에 anchor되어, 그 anchor에서 계산하는 item 위치가 어긋났습니다.
- 이제 이전 window 전체를 recycle하고 새 범위만 bind하며, anchor는 새 첫 item 자신의 offset으로 계산합니다.
- 다른 PR에 의존하지 않으며 단독으로 적용됩니다.

### Changes
- `linear-items-layouter-impl.cpp`: `ScrollBy`의 재구성 분기 조건에 `newLast < mFirstActive || newFirst > mLastActive`를 추가했습니다.
- 활성 item이 있으면 `GetItemOffsetFrom(mFirstActive, mFirstActiveOffset, newFirst)`로 시작 offset을 구합니다.
- 이어서 `RecycleAllViews()`로 이전 window를 비우고 `newFirst`..`newLast` 범위만 `Fill`합니다.
- 시작 offset은 0부터 다시 더하지 않고 이전 anchor에서 걸어가며 구하고, 측정된 extent cache는 지우지 않습니다.
- 활성 item이 없으면 기존처럼 `GetItemOffset(newFirst)`를 쓰며, 분기 끝의 `mHasActiveItems`는 `newFirst <= newLast`로 둡니다.
- `utc-Dali-RecyclerView.cpp`: `UtcDaliRecyclerViewScrollByDisjointJumpP`와 `gViewsByPosition` 기록을 추가했습니다.
- 이 UTC는 offset 1500으로 점프하면 24..39만 bind되고 이전 14개가 recycle되며 scroller child가 16개인지 확인합니다.
- 이어서 -50 스크롤로 item 23만 새로 bind되고 그 `POSITION_Y`가 재구성된 anchor 기준 1150인지 확인합니다.
- `UtcDaliRecyclerViewScrollByWithoutRealisedWindowP`: window 없이 들어온 첫 scroll이 range를 처음부터 만들고 이후 anchor로 쓰이는지 확인합니다.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
