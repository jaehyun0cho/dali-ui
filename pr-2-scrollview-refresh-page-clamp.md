### Summary
- `ScrollView` 자신의 크기만 바뀐 layout pass에서도 viewport, clamp 범위, viewport 기반 page 수를 갱신합니다.
- 지금까지는 viewport 변경이 content 크기를 전달하는 `SetScrollableWidth/Height`를 거치지 않아, 이 경우 이전 값에 머물렀습니다.
- animation 도중 scroll 범위가 줄면 완료 시 clamp된 float 위치에 정착하고, 취소된 animation의 `Finished`는 무시합니다.
- `PageScrollView`는 snap 중 page 수가 바뀌면 snap 목표를 새 page 수로 clamp한 page를 보고합니다.
- layout 이전에 `NotifyPagesInserted/Removed`로 선택된 page는 첫 layout에서 scroll 위치에 적용되고 유지됩니다.
- 이 PR은 다른 PR에 의존하지 않습니다.

### Changes
- integration-api에 `ScrollViewImpl::RefreshViewport()`(non-virtual)를 추가합니다. 자기 크기가 바뀐 경우에만 다시 계산하고 알립니다.
- 하위 클래스의 `OnScrollableAreaChanged()` override는 content 크기뿐 아니라 viewport 변경에도 호출됩니다.
- `ScrollViewLayoutManager::Arrange`는 child loop 뒤에 `RefreshViewport()`를 호출해 수신자가 이번 pass의 scrollable 크기를 읽게 합니다.
- `OnScrollAnimationFinished`는 현재 `mScrollAnimation`이 아닌 animation을 무시하고, 범위 밖 완료 위치를 clamp합니다.
- `PageScrollViewImpl::OnScrollableAreaChanged`는 주 축 scrollable 길이나 page 길이가 `1` 미만이면 건너뛰고, page 변경도 알립니다.
- 마지막 page geometry는 새 멤버 대신 View attachment에 저장해 공개된 구현 객체의 크기를 유지합니다.
- `NotifyPages*`의 `mNotifyInProgress`는 예외에도 이전 값을 복원하는 `NotifyInProgressScope`로 관리합니다.
- `ScrollToPage`는 animated scroll이 시작되지 않으면 snap 목표를 지우고, `OnScrollFinished`는 확정 page를 clamp합니다.
- `utc-Dali-PageScrollView.cpp`를 새로 추가하고, `utc-Dali-ScrollView.cpp`에 범위 축소 후 완료 위치 TC를 추가합니다.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
