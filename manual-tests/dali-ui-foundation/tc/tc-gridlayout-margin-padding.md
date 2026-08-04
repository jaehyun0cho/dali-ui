# GridLayout: Margin / Padding

그리드 컨테이너의 `SetPadding`과 셀별 `SetMargin`이 각각 어떻게 적용되는지 확인한다. row/column spacing을 0으로 두어 margin 효과만 드러나게 한다.

> 상단 헤더(60px)만큼 세로 공간이 줄어들므로 최하단 요소는 창 크기에 따라 일부만 보일 수 있다.

## 화면 구성

- 배경은 흰색이고, 루트 `GridLayout`의 배경은 `Color::GRAY`이며 너비/높이 모두 `MATCH_PARENT`이다
- 루트 padding은 사방 50px(start, end, top, bottom), row spacing 0px, column spacing 0px이다
- 행 정의: `GridLength::Absolute(200)` 3개 (총 600px)
- 열 정의: `GridLength::Star(1)` 2개 (남은 너비를 균등 분할)
- 셀 배치
  - (0,0) `Color::RED`: margin 없음
  - (0,1) `Color::GREEN`: margin 사방 50px
  - (1,0) `Color::BLUE`: margin 사방 50px
  - (1,1) `Color::YELLOW`: margin 없음
  - (2,0) `Color::CYAN`: `SetColumnSpan(2)`, margin 사방 50px

```
  [RED (margin 0) ][GREEN (margin 50)]
  [BLUE (margin 50)][YELLOW (margin 0)]
  [ CYAN 2열 span (margin 50)        ]
```

## 테스트 1: 컨테이너 padding

1. 목록에서 `GridLayout: Margin / Padding`을 선택한다
2. **기대 결과**: 루트의 `Color::GRAY` 배경이 콘텐츠 영역 왼쪽/위/오른쪽 가장자리를 따라 50px 띠로 보인다. 아래쪽은 세 행(총 600px) 아래로 남는 공간 전체가 회색으로 이어진다
3. **기대 결과**: margin이 없는 `Color::RED` 셀의 좌상단이 회색 padding 띠에 딱 붙는다

## 테스트 2: 셀별 margin

1. margin이 있는 셀과 없는 셀을 비교한다
2. **기대 결과**: `Color::RED`와 `Color::YELLOW`는 자기 셀을 빈틈없이 채운다
3. **기대 결과**: `Color::GREEN`과 `Color::BLUE`는 자기 셀 안에서 사방 50px씩 줄어들고, 그 자리에 `Color::GRAY` 배경이 드러난다
4. **기대 결과**: spacing이 0이므로 margin이 없는 셀끼리는 서로 맞닿는다

## 테스트 3: ColumnSpan 셀의 margin

1. 최하단 `Color::CYAN` 셀을 관찰한다
2. **기대 결과**: `Color::CYAN`이 2개 열 전체 폭을 차지하되, 그 영역 안에서 사방 50px margin만큼 줄어들어 그려진다

## 통과 기준

- 루트 `Color::GRAY` 배경이 padding 50px 영역, margin이 적용된 셀 주변, 그리고 세 행(총 600px) 아래 남는 영역에서 보여야 한다
- margin 50px가 적용된 셀은 셀 경계에서 사방 50px 안쪽으로 축소되어야 한다
- margin이 없는 셀은 셀 경계에 정확히 맞아야 하고, spacing 0이므로 인접 셀과 틈이 없어야 한다
- 각 행 높이가 200px로 고정되고, 두 열 너비가 서로 같아야 한다
