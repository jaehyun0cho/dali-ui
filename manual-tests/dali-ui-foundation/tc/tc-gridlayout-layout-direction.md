# GridLayout: LayoutDirection Toggle

`SetLayoutDirection`을 `RIGHT_TO_LEFT`로 바꿨을 때 `GridLayout`의 셀이 좌우로 미러링되고, `LayoutMode::STANDALONE` 자식은 미러링에서 제외되는지 확인한다.

## 화면 구성

- 배경은 흰색이며, 루트 `GridLayout`은 너비/높이 모두 `MATCH_PARENT`이다
- 루트 padding은 사방 50px, row spacing 10px, column spacing 10px이다
- 행 정의(위에서부터): `GridLength::Absolute(50)`, `GridLength::Absolute(100)`, `GridLength::Absolute(200)`
- 열 정의(왼쪽부터): `GridLength::Absolute(50)`, `GridLength::Absolute(100)`
- 셀 6개: (0,0) `Color::RED`, (0,1) `Color::GREEN`, (1,0) `Color::BLUE`, (1,1) `Color::YELLOW`, (2,0) `Color::CYAN`, (2,1) `Color::MAGENTA`
- 토글 버튼: 200x50 크기의 반투명 검정(`Vector4(0.0, 0.0, 0.0, 0.5)`) `InteractiveView`
  - `SetLayoutMode(LayoutMode::STANDALONE)`, 요청 좌표 (0,0), `SetLayoutDirection(LEFT_TO_RIGHT)`
  - 가운데 정렬된 흰색 라벨 `Change LayoutDirection`

## 테스트 1: 초기 LEFT_TO_RIGHT 배치

1. 목록에서 `GridLayout: LayoutDirection Toggle`을 선택한다
2. **기대 결과**: 왼쪽 열이 50px, 오른쪽 열이 100px이며 (0,0) 빨강이 좌상단에 놓인다
3. **기대 결과**: 반투명 검정 토글 버튼이 좌상단에 겹쳐 보이고 라벨 `Change LayoutDirection`이 가운데 정렬된다

## 테스트 2: RIGHT_TO_LEFT 전환

1. 토글 버튼을 탭한다
2. **기대 결과**: 그리드 6개 셀이 좌우 반전되어, 0열(50px 열)이 오른쪽에 오고 1열(100px 열)이 그 왼쪽에 온다
3. **기대 결과**: 빨강이 우상단, 초록이 그 왼쪽으로 이동하며 행 순서(위→아래)는 그대로 유지된다
4. **기대 결과**: 토글 버튼은 `LayoutMode::STANDALONE`이므로 좌상단 그대로 남고 라벨 글자도 뒤집히지 않는다

## 테스트 3: LEFT_TO_RIGHT 복귀

1. 토글 버튼을 한 번 더 탭한다
2. **기대 결과**: 테스트 1의 초기 배치로 정확히 되돌아간다

## 테스트 4: 재진입 초기화

1. RIGHT_TO_LEFT 상태에서 뒤로 가기로 목록에 나갔다가 다시 이 TC로 들어온다
2. **기대 결과**: 항상 `LEFT_TO_RIGHT` 상태로 시작한다

## 통과 기준

- 탭할 때마다 `LEFT_TO_RIGHT`와 `RIGHT_TO_LEFT`가 번갈아 적용되어야 한다
- `RIGHT_TO_LEFT`에서 그리드 열 순서만 좌우 반전되고 행 순서와 각 셀 크기(50/100px 열, 50/100/200px 행)는 그대로여야 한다
- `LayoutMode::STANDALONE` 토글 버튼은 방향이 바뀌어도 위치가 변하지 않아야 한다
- 목록으로 나갔다 다시 들어오면 항상 `LEFT_TO_RIGHT` 초기 상태여야 한다
