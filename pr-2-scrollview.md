### Summary
- ScrollView·PageScrollView에서 앱이 주는 natural 길이(키 스크롤 스텝, 포커스 peek, 휠 한 칸, edge 효과 한계, 페이지 크기)가 뷰의 effective scale을 따르지 않던 문제를 고칩니다.
- 이미 물리 픽셀인 스크롤 위치를 다시 스케일해서 레이아웃 패스마다 콘텐츠가 s배씩 밀리던 문제도 함께 고칩니다.
- `devel` 위에 올라가며, 다른 effective-scale PR과 코드 의존은 없습니다.
- 첫 커밋(샘플 전용)은 scrollview·page-scroller·recyclerview·scrollbar 샘플에 스케일 키와 확인용 콘텐츠를 추가합니다. RecyclerView PR이 이 샘플 커밋을 이용하므로 그 PR은 이 PR 위에 쌓습니다.
- 스크롤 위치·제스처 임계값처럼 렌더링 좌표인 값은 그대로 물리 픽셀입니다.
- 새 virtual, 공개 API 변경, View당 추가 메모리는 없습니다(공개 헤더는 주석만 변경).

### Changes
- 샘플: UI 스케일 키(q/w/e/r/t = 0.8~2.0), 목록 padding·bounce, 다시 만들기(`n`), 페이지 크기, 포커스(`f`), margin 출처(`l`), 작은 스크롤바(`m`); scrollview.example은 edge effect 토글을 `e`에서 `d`로 옮김
- ScrollViewLayoutManager가 콘텐츠 actor 위치를 읽어 다시 스케일하던 것을 제거(패스마다 s배로 밀리던 문제)
- ScrollView padding 오프셋을 레이아웃과 같은 렌더링 픽셀로 맞추고, 값이 바뀌면 다시 배치
- 첫 레이아웃 전 스크롤의 clamp를 requested 크기 × 각 뷰의 스케일로 비교
- `SetKeyScrollStep()` 스텝과 포커스/스크롤 판정 거리에 스케일을 곱함
- `SetFocusScrollPeek()`에 스케일을 곱함
- 휠 한 칸(120 natural)에 스케일을 곱함
- BounceEdgeEffect의 당김·밀어내기 한계(100, 80)에 원본 뷰의 스케일을 곱함
- 키보드 edge bump의 흡수 속도가 스케일을 따름(30 natural 길이 유지)
- PageScrollView의 `SetPageSize()`를 natural 단위로 받음

### 동작 변경
- 스케일 1.0에서도: 런타임 `SetPadding()`이 ScrollView 콘텐츠 위치에 반영됩니다.
- 스케일 ≠ 1.0: 위 길이들이 스케일을 따르며, 스크롤된 콘텐츠가 레이아웃 패스마다 밀리지 않습니다.
- 샘플: scrollview.example의 edge effect 토글 키가 `e`에서 `d`로 바뀝니다.

### 검증
- 이 PR의 커밋만 `devel`(`004b14f8`) 위에 cherry-pick해도 충돌이 없음을 확인했습니다.
- `devel` `d2d2a7e2` 기준 시리즈 최종 커밋에서 라이브러리·샘플을 빌드하고 public·components·internal UTC를 빌드·실행해, 이 시리즈가 추가한 UTC가 모두 통과함을 확인했습니다(세부 사항은 `pr-effective-scale-fixes.md`). `devel` `004b14f8`로 rebase한 뒤에는 다시 빌드하지 않았습니다.
- `Sample:` 줄의 기대값은 코드에서 계산한 값이며, 샘플을 직접 실행해 확인하지는 않았습니다.

### Examples
```cpp
// 스텝은 natural 단위입니다. 스케일 1.5에서는 키 한 번에 225px(150 × 1.5)씩 스크롤됩니다(이전: 150px).
scrollView.SetKeyScrollStep(150.0f);
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
