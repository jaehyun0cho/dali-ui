# StackLayout: Margin / Padding / Nested

`StackLayout`의 루트 padding, 자식별 margin, 그리고 세로 스택 안에 중첩된 가로 스택이 함께 동작하는지 확인한다.

## 화면 구성

- 배경은 흰색이고, 루트는 `StackOrientation::VERTICAL` `StackLayout`으로 콘텐츠 영역을 가득 채운다 (`MATCH_PARENT` x `MATCH_PARENT`)
- 루트 padding은 `Extents(50, 50, 50, 50)`이고 spacing은 설정하지 않는다(기본값 `0`)
- 자식 1: 빨간색(`Color::RED`), 높이 `50px`, margin 없음, `LayoutAlignment::FILL`
- 자식 2: 초록색(`Color::GREEN`), 높이 `50px`, margin `Extents(50, 50, 0, 0)`(start / end 만 `50px`), `FILL`
- 자식 3: 파란색(`Color::BLUE`), 높이 `50px`, margin `Extents(0, 0, 50, 50)`(top / bottom 만 `50px`), `FILL`
- 자식 4: 시안색(`Color::CYAN`), 높이 `50px`, margin `Extents(50, 50, 50, 50)`(사방 `50px`), `FILL`
- 자식 5: 중첩 가로 `StackLayout`, 배경 `Color::GRAY`, spacing `10px`, margin 사방 `50px`, weight `1.0` + `FILL`
  - 마젠타(`Color::MAGENTA`): weight `1.0`, `LayoutAlignment::FILL`
  - 노랑(`Color::YELLOW`): 높이 `50px`, weight `1.0`, `LayoutAlignment::START`
  - 빨강(`Color::RED`): 높이 `50px`, weight `1.0`, `LayoutAlignment::CENTER`
  - 초록(`Color::GREEN`): 높이 `50px`, weight `1.0`, `LayoutAlignment::END`

## 테스트 1: 루트 padding

1. 빨간 바의 위치를 관찰한다
2. **기대 결과**: 콘텐츠 영역의 좌 / 우 / 상단 가장자리에서 `50px` 안쪽에 놓인다

## 테스트 2: 자식별 margin

1. 빨강 / 초록 / 파랑 / 시안 바의 좌우 변과 상하 간격을 관찰한다
2. **기대 결과**: 빨강은 padding 안쪽 폭을 그대로 가득 채운다
3. **기대 결과**: 초록은 좌우가 빨강보다 `50px`씩 좁고, 빨강과 초록 사이에는 세로 간격이 없다
4. **기대 결과**: 파랑은 좌우 폭이 빨강과 같지만 위아래로 각각 `50px` 떨어져 있다
5. **기대 결과**: 시안은 좌우가 `50px`씩 좁고 위아래로도 `50px` 떨어져 있다

## 테스트 3: 중첩 가로 스택

1. 화면 아래쪽의 회색 행을 관찰한다
2. **기대 결과**: 회색 행이 사방으로 `50px` margin을 두고, 남은 세로 공간을 모두 차지한다 (weight `1.0`)
3. **기대 결과**: 회색 행 안에 마젠타 / 노랑 / 빨강 / 초록이 왼쪽부터 이 순서로 놓이고 폭이 서로 같다(모두 weight `1.0`), 박스 사이 간격은 `10px`이다
4. **기대 결과**: 마젠타는 행 높이를 가득 채우고(`FILL`), 노랑은 위쪽(`START`), 빨강은 세로 중앙(`CENTER`), 초록은 아래쪽(`END`)에 높이 `50px`로 놓인다

## 테스트 4: 재진입

1. `< Back`으로 목록에 나갔다가 다시 이 TC로 들어온다
2. **기대 결과**: 첫 진입과 완전히 동일한 화면이 표시된다

## 통과 기준

- margin이 서로 다른 네 개의 바가 각각 지정된 방향으로만 `50px` 밀려 있다
- 회색 중첩 행의 네 자식 폭이 모두 동일하다
- 노랑 / 빨강 / 초록의 높이는 `50px`로 같고 세로 위치만 정렬 값에 따라 다르다
