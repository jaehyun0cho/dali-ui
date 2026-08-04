# FlexLayout: Margin / Padding

`FlexDirection::COLUMN` 컨테이너의 padding, 자식별 margin, 그리고 중첩된 `FlexDirection::ROW` 컨테이너의 padding + margin이 함께 적용되는 결과를 확인한다.

> 상단 헤더(60px)만큼 세로 공간이 줄어들므로 최하단 요소는 창 크기에 따라 일부만 보일 수 있다.

## 화면 구성

- 배경은 흰색(`Color::WHITE`)이다.
- 루트는 `MATCH_PARENT x MATCH_PARENT` 크기의 `FlexLayout`이며 `FlexDirection::COLUMN`, padding `Extents(50, 50, 50, 50)`(start, end, top, bottom)가 설정된다.
- 루트의 자식은 추가 순서대로 다음과 같다.
  - 빨강(`Color::RED`): 높이 50px, margin 없음
  - 초록(`Color::GREEN`): 높이 50px, margin `Extents(50, 50, 50, 50)`
  - 파랑(`Color::BLUE`): 높이 50px, margin 없음
  - 중첩 행: 회색(`Color::GRAY`) `FlexLayout`, 높이 400px, `FlexDirection::ROW`, `FlexAlign::STRETCH`, padding `Extents(50, 50, 50, 50)`, margin `Extents(50, 50, 50, 50)`
- 중첩 행의 자식은 모두 `SetFlexGrow(1.0f)`이며 추가 순서대로 마젠타(`Color::MAGENTA`, margin 없음), 노랑(`Color::YELLOW`, margin `Extents(50, 50, 50, 50)`), 시안(`Color::CYAN`, margin 없음)이다.

```
  y=50   [red    h50]
  y=150  [green  h50]      <- 위/아래 margin 50
  y=250  [blue   h50]
  y=350  +-- gray row h400 (padding 50, margin 50) --+
         | [magenta] [ yellow ] [cyan]              |
         +------------------------------------------+
```

## 테스트 1: 루트 padding과 자식 margin

1. 목록에서 이 TC를 선택한다
2. **기대 결과**: 빨강의 위쪽 변이 콘텐츠 영역 위에서 50px 아래(루트 padding)에 놓이고, 좌우로도 50px씩 안쪽에 놓인다
3. **기대 결과**: 초록은 빨강 아래로 50px 떨어져 시작하고, 좌우도 빨강보다 50px씩 더 안쪽으로 들어간다
4. **기대 결과**: 파랑은 초록 아래로 50px 떨어져 시작하고, 좌우 폭은 빨강과 같다
5. **기대 결과**: 세 박스의 높이는 각각 50px이다

## 테스트 2: 중첩 행의 padding과 margin

1. 파랑 아래의 회색 영역을 관찰한다
2. **기대 결과**: 회색 행은 파랑 아래로 50px 떨어져 시작하고, 좌우도 빨강보다 50px씩 안쪽이며 높이는 400px이다
3. **기대 결과**: 회색 행 안쪽에서 마젠타/노랑/시안이 가로로 배치되고, 회색 padding 50px이 사방으로 보인다
4. **기대 결과**: 세 자식은 `SetFlexGrow(1.0f)`가 같으므로 폭이 서로 같다
5. **기대 결과**: 노랑은 margin 50px 때문에 좌우로 회색 띠가 보이고, 위아래로도 50px씩 들어가 마젠타/시안보다 높이가 100px 작다

## 통과 기준

- 루트 padding 50px이 사방에서 유지되어야 한다
- margin이 있는 자식(초록, 노랑)만 인접 요소와 50px 간격을 가져야 하고, margin이 없는 자식은 padding 경계에 붙어야 한다
- 중첩 회색 행의 높이가 400px이고 안쪽 사방에 50px 회색 padding이 보여야 한다
- 마젠타와 시안의 높이가 300px(400 - padding 100), 노랑의 높이가 200px여야 한다
- 마젠타/노랑/시안의 폭이 서로 같아야 한다
