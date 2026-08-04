# StackLayout: Cross-axis Alignment

세로 `StackLayout`에서 `StackLayoutParams`의 cross-axis 정렬(`START` / `CENTER` / `END` / `FILL`)이 자식의 가로 위치를 어떻게 결정하는지 확인한다.

## 화면 구성

- 배경은 흰색이고, 루트는 `StackOrientation::VERTICAL` `StackLayout`으로 콘텐츠 영역을 가득 채운다 (`MATCH_PARENT` x `MATCH_PARENT`)
- 루트 padding은 `Extents(50, 50, 50, 50)`, 자식 사이 spacing은 `50px`
- 행 1: 빨간색(`Color::RED`), `100x50`, `LayoutAlignment::START`
- 행 2: 초록색(`Color::GREEN`), `100x50`, `LayoutAlignment::CENTER`
- 행 3: 파란색(`Color::BLUE`), `100x50`, `LayoutAlignment::END`
- 행 4: 시안색(`Color::CYAN`), 높이 `50px`(폭 지정 없음), `LayoutAlignment::FILL`

## 테스트 1: 초기 레이아웃

1. 목록에서 이 TC를 선택해 화면을 관찰한다
2. **기대 결과**: 네 개의 바가 위에서 아래로 빨강 / 초록 / 파랑 / 시안 순서로 놓이고, 각 바의 높이는 `50px`이다
3. **기대 결과**: 바 사이 간격은 `50px`, 콘텐츠 영역 가장자리와의 여백도 `50px`이다

## 테스트 2: cross-axis 정렬

1. 각 바의 가로 위치를 관찰한다
2. **기대 결과**: 빨강(`START`)은 padding 안쪽 왼쪽 끝에 붙는다
3. **기대 결과**: 초록(`CENTER`)은 padding 안쪽 영역의 가로 중앙에 놓인다
4. **기대 결과**: 파랑(`END`)은 padding 안쪽 오른쪽 끝에 붙는다
5. **기대 결과**: 시안(`FILL`)은 padding 안쪽 폭을 좌우로 가득 채운다

## 테스트 3: 재진입

1. `< Back`으로 목록에 나갔다가 다시 이 TC로 들어온다
2. **기대 결과**: 첫 진입과 완전히 동일한 화면이 표시된다

## 통과 기준

- 빨강 / 초록 / 파랑의 폭은 모두 `100px`로 동일하고 가로 위치만 서로 다르다
- 초록의 좌우 여백이 서로 같다
- 시안만 padding 안쪽 폭 전체를 차지한다
