# ViewLayout: LayoutDirection Toggle

순수 `View`(`LayoutManager` 없음)의 `LayoutDirection`을 `RIGHT_TO_LEFT`로 바꾸면 직계 자식이 가로로 미러링되고, `LayoutMode::STANDALONE` 자식은 미러링에서 제외되는지 확인한다.

## 화면 구성

- 배경은 흰색이고, 내용은 `ViewLayout: Margin`과 동일하다
- 루트: 회색(`Color::GRAY`) `View`, 폭 `MATCH_PARENT`, 높이 `MATCH_PARENT`
- 자식 1: 빨간색(`Color::RED`), 폭 `MATCH_PARENT`, 높이 `200px`, margin `Extents(50, 50, 50, 50)`
- 자식 2: 초록색(`Color::GREEN`), 폭 `WRAP_CONTENT`, 높이 `200px`, 요청 위치 `y = 300`, margin `Extents(50, 50, 50, 50)`
  - 손자: 노란색(`Color::YELLOW`), `100x100`
- 자식 3: 파란색(`Color::BLUE`), 폭 `200px`, 높이 `MATCH_PARENT`, 요청 위치 `y = 600`, margin `Extents(50, 50, 50, 50)`
- 토글 버튼: `200x50` 크기의 반투명 검정(`Vector4(0, 0, 0, 0.5)`) `InteractiveView`, 위치 `(0, 0)`, `LayoutMode::STANDALONE`, 자신의 `LayoutDirection`은 `LEFT_TO_RIGHT`로 고정, 흰색 `Change LayoutDirection` 라벨이 중앙에 표시된다

> 상단 헤더(60px)만큼 세로 공간이 줄어들므로 최하단 요소는 창 크기에 따라 일부만 보일 수 있다.

## 테스트 1: 초기 레이아웃 (LEFT_TO_RIGHT)

1. 목록에서 이 TC를 선택해 화면을 관찰한다
2. **기대 결과**: 초록 박스와 파란 박스가 화면 왼쪽에 붙어 있고, 왼쪽 가장자리에서 margin `50px`만큼 떨어져 있다
3. **기대 결과**: 토글 버튼이 좌상단 `(0, 0)`에 `200x50` 크기로 떠 있다

## 테스트 2: RIGHT_TO_LEFT로 전환

1. `Change LayoutDirection` 버튼을 탭한다
2. **기대 결과**: 초록 박스와 파란 박스가 화면 오른쪽으로 미러링되어, 오른쪽 가장자리에서 margin `50px`만큼 떨어져 놓인다
3. **기대 결과**: 빨간 바는 폭이 `MATCH_PARENT`라 좌우 대칭이므로 위치가 그대로다
4. **기대 결과**: 노란 손자는 초록 박스 폭(`100px`)과 크기가 같아 초록 박스 기준 미러링을 해도 위치가 변하지 않는다
5. **기대 결과**: 토글 버튼은 여전히 좌상단 `(0, 0)`에 있고 라벨도 정상으로 읽힌다 (`STANDALONE`이므로 미러링 제외)

## 테스트 3: 다시 LEFT_TO_RIGHT로 복귀

1. `Change LayoutDirection` 버튼을 한 번 더 탭한다
2. **기대 결과**: 테스트 1의 배치로 되돌아온다

## 테스트 4: 재진입 시 초기화

1. `RIGHT_TO_LEFT` 상태로 만든 뒤 `< Back`으로 목록에 나갔다가 다시 이 TC로 들어온다
2. **기대 결과**: 항상 `LEFT_TO_RIGHT` 상태로 시작한다

## 통과 기준

- 탭할 때마다 직계 자식의 좌우 위치가 반전된다
- 미러링되어도 각 자식의 크기와 세로 위치(`y = 300`, `y = 600`)는 변하지 않는다
- 토글 버튼의 위치와 라벨 방향은 어떤 상태에서도 변하지 않는다
- 재진입하면 방향 상태가 `LEFT_TO_RIGHT`로 초기화된다
