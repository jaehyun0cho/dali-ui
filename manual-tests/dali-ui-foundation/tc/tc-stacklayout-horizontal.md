# StackLayout: Horizontal Rows + Alignment

가로 `StackLayout`에서 cross-axis(세로) 정렬과 weight가 함께 동작하는지 확인한다.

## 화면 구성

- 배경은 흰색이고, 바깥 루트는 `StackOrientation::VERTICAL` `StackLayout`으로 콘텐츠 영역을 가득 채운다 (`MATCH_PARENT` x `MATCH_PARENT`)
- 바깥 루트 padding은 `Extents(50, 50, 50, 50)`, 행 사이 spacing은 `10px`
- 행 4개는 모두 `StackOrientation::HORIZONTAL` `StackLayout`이며 weight `1.0` + `LayoutAlignment::FILL`, 자식 사이 spacing `10px`
- 행 배경은 번갈아 밝은 회색: 행 1 / 행 3은 `#F2F2F2`, 행 2 / 행 4는 `#E6E6E6`
- 행 1~3의 자식: 빨강(`Color::RED`) `50x50`, 초록(`Color::GREEN`) `50x50`, 파랑(`Color::BLUE`) 폭 `WRAP_CONTENT` + weight `1.0` + 높이 `50px`
- 행 1은 세 자식 모두 `LayoutAlignment::START`, 행 2는 `CENTER`, 행 3은 `END`
- 행 4의 자식: 빨강 폭 `50px`, 초록 폭 `50px`, 파랑 weight `1.0`이며 세 자식 모두 `LayoutAlignment::FILL`(높이 지정 없음)

## 테스트 1: 초기 레이아웃

1. 목록에서 이 TC를 선택해 화면을 관찰한다
2. **기대 결과**: 밝은 회색 행 4개가 위에서 아래로 쌓이고, 행 사이 간격은 `10px`이다
3. **기대 결과**: 네 행의 높이가 서로 같다(모두 weight `1.0`)
4. **기대 결과**: 각 행 안에서 빨강 / 초록 / 파랑이 왼쪽부터 이 순서로 놓이고 박스 사이 간격은 `10px`이다

## 테스트 2: 행별 cross-axis 정렬

1. 행 1의 빨강 / 초록 박스 세로 위치를 관찰한다
2. **기대 결과**: 행의 위쪽 변에 붙어 있다 (`START`)
3. 행 2를 관찰한다
4. **기대 결과**: 박스가 행 높이의 세로 중앙에 놓인다 (`CENTER`)
5. 행 3을 관찰한다
6. **기대 결과**: 박스가 행의 아래쪽 변에 붙어 있다 (`END`)
7. 행 4를 관찰한다
8. **기대 결과**: 세 박스 모두 행 높이를 세로로 가득 채운다 (`FILL`)

## 테스트 3: weight로 남는 폭 차지

1. 각 행의 파란 박스 폭을 관찰한다
2. **기대 결과**: 파랑이 빨강 `50px` + 초록 `50px` + spacing `20px`를 뺀 나머지 폭을 모두 차지한다

## 테스트 4: 재진입

1. `< Back`으로 목록에 나갔다가 다시 이 TC로 들어온다
2. **기대 결과**: 첫 진입과 완전히 동일한 화면이 표시된다

## 통과 기준

- 행 1 / 행 2 / 행 3에서 빨강과 초록의 크기는 `50x50`으로 동일하고 세로 위치만 정렬 값에 따라 달라진다
- 행 4에서만 세 박스의 높이가 행 높이와 같아진다
- 각 행의 파란 박스 오른쪽 변이 행의 오른쪽 변과 일치한다
