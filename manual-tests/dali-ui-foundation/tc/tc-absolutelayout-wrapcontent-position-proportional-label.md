# AbsoluteLayout: WrapContent Parent + Position-Proportional Label

`WRAP_CONTENT` `AbsoluteLayout` 부모가 비례 위치와 `WRAP_CONTENT` 크기를 가진 Label 자식에
맞춰 크기가 결정되는지, 그리고 부모를 고정 크기로 바꾸면 라벨이 중앙으로 재배치되는지 확인한다.

## 화면 구성

- 배경: 흰색(`Color::WHITE`)
- 초록 루트 `View`(`Color::GREEN`): 너비/높이 모두 `MATCH_PARENT`
- 빨간 부모 `AbsoluteLayout`(`Color::RED`): 너비/높이 모두 `WRAP_CONTENT`, padding `Extents(50, 50, 50, 50)`
- 파란 Label(`Color::BLUE`, 흰 글씨): 텍스트 `WRAP Label`, 너비/높이 모두 `WRAP_CONTENT`,
  `LayoutRect(0.5, 0.5, -1.0, -1.0)` + `POSITION_PROPORTIONAL`
- 토글 버튼: 220x50 반투명 검정(알파 0.65) `InteractiveView`, 콘텐츠 영역 상단 중앙(`X_PROPORTIONAL` 0.5),
  가운데 흰색 라벨에 현재 동작이 표시된다

## 테스트 1: 초기 WRAP_CONTENT 상태

1. 목록에서 본 TC에 진입한다
2. **기대 결과**: 빨간 부모의 크기가 파란 Label 크기 + padding 100px(좌우 각 50px, 상하 각 50px)와 같다
3. **기대 결과**: 파란 Label이 빨간 영역의 padding 안쪽에 딱 맞게 놓이며, 남는 여백이 없으므로 비례 위치 계산 결과가 0이 된다
4. **기대 결과**: 토글 버튼 라벨에 `Click to set size`가 표시된다

## 테스트 2: 고정 크기로 전환

1. 상단 중앙의 토글 버튼을 탭한다
2. **기대 결과**: 빨간 부모가 400x200 고정 크기로 커진다
3. **기대 결과**: 파란 Label은 크기 변화 없이 padding 안쪽 영역(300x100)의 정중앙에 배치된다
4. **기대 결과**: 토글 버튼 라벨이 `Click to set wrap`으로 바뀐다

## 테스트 3: WRAP_CONTENT로 복귀

1. 토글 버튼을 다시 탭한다
2. **기대 결과**: 빨간 부모가 테스트 1과 동일한 wrap 크기로 되돌아간다
3. **기대 결과**: 토글 버튼 라벨이 다시 `Click to set size`로 바뀐다

## 테스트 4: 토글 버튼 중앙 정렬

1. 창 너비를 늘리거나 줄인다
2. **기대 결과**: 토글 버튼은 크기 220x50을 유지한 채 항상 콘텐츠 영역 상단 가로 중앙에 놓인다

## 통과 기준

- `WRAP_CONTENT` 부모의 크기가 Label 크기와 padding의 합과 일치해야 한다
- 고정 크기로 전환하면 `POSITION_PROPORTIONAL` Label이 padding 안쪽 영역 중앙으로 이동해야 한다
- 토글은 wrap과 고정 크기 사이를 몇 번을 눌러도 정확히 왕복해야 한다
- 목록으로 나갔다가 다시 진입하면 항상 wrap 상태와 `Click to set size` 라벨로 초기화되어야 한다
