### Summary
- RecyclerView와 기본 linear layouter에서 앱이 주는 natural 길이(키 스크롤 스텝, 포커스 peek, 휠 한 칸, decoration offset, 캐시 범위, 아이템 크기·간격, 그룹 body margin)가 뷰의 effective scale을 따르지 않던 문제를 고칩니다.
- ScrollBar의 최소 크기 fallback이 두 번 스케일되던 문제도 고칩니다.
- ScrollView/PageScrollView PR 위에 쌓는 PR입니다. 코드 의존은 없지만, 커밋들의 `Sample:` 줄이 그 PR의 샘플 커밋이 추가한 recyclerview·scrollbar 샘플 키를 사용합니다.
- 기본 linear layouter는 RecyclerView에서 스케일을 전달받고, 스케일이 바뀌면 다시 배치합니다. 이 경로의 내부 접근자 이름은 `Set/GetEffectiveLayoutScale()`입니다.
- 새 virtual, 공개 API 변경, View당 추가 메모리는 없습니다(설치되는 public·integration 헤더는 주석만 변경).

### Changes
- `SetKeyScrollStep()` 스텝과 포커스/스크롤 판정 거리에 스케일을 곱함
- `SetFocusScrollPeek()`에 스케일을 곱함
- 휠 한 칸(120 natural)에 스케일을 곱함
- 키보드 edge bump의 흡수 속도가 스케일을 따름(ScrollView와 동일)
- ItemDecoration offset에 RecyclerView의 스케일을 곱하고, GroupBodyDecoration도 같은 방식으로 카드를 들여 놓음
- `SetCacheExtent()`를 natural 단위로 받아 layouter에 스케일을 곱해 전달
- 기본 linear layouter가 아이템 extent·간격에 스케일을 곱하고, 스케일 변경 시 다시 배치
- GroupLinearItemsLayouter의 body margin에 스케일을 곱함(아이템 bounds와 측정 cross inset 모두)
- ScrollBar의 `SetBarMinSize(<= 0)` fallback을 natural 단위로 변환해 두 번 스케일되지 않게 함

### 동작 변경
- 스케일 ≠ 1.0: 위 길이들이 스케일을 따릅니다. ScrollBar의 작은 thumb는 의도한 최소 길이로 그려집니다.
- 알려진 제한: 스케일을 바꾼 직후 다음 레이아웃 패스 전에 요청한 스크롤은 이전 지오메트리 기준으로 처리됩니다(헤더 문서에 명시).

### 검증
- ScrollView/PageScrollView PR의 커밋 위에 이 PR의 커밋을 cherry-pick해도 충돌이 없음을 확인했습니다.
- 라이브러리 전체 빌드·UTC 실행은 하지 못했습니다(설치된 dali-adaptor에 `media-view-backend.h` 없음). 변경 소스의 오브젝트 컴파일은 전체 effective-scale 시리즈의 최종 커밋 기준으로 확인했고, UTC는 컴파일만 했습니다.
- `Sample:` 줄의 기대값은 코드에서 계산한 값이며, 샘플을 직접 실행해 확인하지는 않았습니다.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
