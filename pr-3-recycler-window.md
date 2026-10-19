### Summary
- `LinearItemsLayouter` 기반 RecyclerView의 window 관리를 두 commit으로 고칩니다: (1) disjoint jump 재구성, (2) 추정 크기(estimate)와 실제 크기가 다른 item의 seam/drift 제거.
- (1) 기존 활성 window와 겹치지 않는 위치로 점프하면 이전 window 전체를 recycle하고 새 범위만 bind하며, anchor는 새 첫 item 자신의 offset으로 계산합니다(이전에는 `oldLast + 1`부터 채워 건너뛴 item이 bind된 채 남고 위치가 어긋났습니다).
- (2) 위로 들어오는 item은 추정치로 시작점을 잡지 않고 첫 item의 윗변에서 측정값으로 위로 붙입니다(glue) → 겹침/틈이 생기지 않습니다.
- 측정값−추정값 차이(보정)만큼 window·scroll offset·실행 중 animation을 함께 옮겨 보이는 item은 움직이지 않고, anchor 좌표와 0부터 걸은 offset이 항상 같습니다(edge에서만 clamp).
- content 길이는 placement와 같은 항목별 규칙(측정값 + 미측정×추정값 + spacing, O(1))이라 끝이 마지막 item과 맞고, 끝/최상단을 향하는 animation은 측정으로 움직이는 edge를 누적 거리 기준으로 따라갑니다.
- 동작 변화: window가 측정 크기 기준으로 정확히 채워짐, 크기 0 item은 1px 전진, 추정 범위 안 요청은 consumed 전량 보고, animation 중 layout pass가 scroller 위치를 쓰지 않음, scrollbar 범위는 측정 전까지 항목별 추정값을 따름.
- 속도를 유지하므로 추정보다 큰 item이 많으면 edge까지의 animation이 명목 시간보다 길어질 수 있고, 추정이 실제의 약 2배를 넘거나 cache 0이면 끝을 한 frame 넘었다 돌아올 수 있습니다(0.5px 미만 오차 제외).
- 다른 PR에 의존하지 않습니다. layout 회귀 manual-test suite PR이 이 PR 위에 쌓입니다.

### Changes
- `linear-items-layouter-impl.cpp`(commit 1): `ScrollBy` 재구성 분기 `newLast < mFirstActive || newFirst > mLastActive` → `RecycleAllViews()` 뒤 `GetItemOffsetFrom(anchor…)`로 새 범위만 `Fill`, 활성 item이 없으면 `GetItemOffset(newFirst)`.
- `linear-items-layouter-impl.{h,cpp}`(commit 2): 비가상 private `SettleBy`/`LayoutWindow`/`RecycleLeading`/`MeasureSlot`/`ClampScrollOffset` 추가, 위쪽 glue와 좌표 보정, 끝 너머 요청 시 마지막 item까지 측정, O(1) 항목별 content 길이, `FindLastVisiblePosition` 삭제.
- `recycler-view-impl.cpp`(header·객체 크기 무변경, 새 virtual 없음): animation 중 보정·edge 이동 시 재타깃(거리 보존, top 목표는 0 유지, end 목표는 측정된 끝 추종, 속도 연속을 위한 시간 비례 조정, 끝난 frame에서 끝이 늘면 이어서 이동), animation 중 `ApplyScrollerPosition` 쓰기 생략.
- 교체된 animation의 지연 Finished 무시, `ScrollToPosition`/`ScrollToItemMakeVisible`의 중복 동기화 제거, 비animation 위치·가시화 요청의 재조준(가시화는 처음 정한 edge만), scroll-to-top 마지막 frame 보정 시 0 정렬, 구조적 data change 시 animation 취소.
- `utc-Dali-RecyclerView.cpp`: commit 1 `ScrollByDisjointJumpP`/`ScrollByWithoutRealisedWindowP`(+`gViewsByPosition`), commit 2 UTC 28개(glue/보정/끝/재타깃/수평/decoration/재측정/연속 animation/끝 추종/가시화 aim/data change).

🤖 Generated with [Claude Code](https://claude.com/claude-code)
