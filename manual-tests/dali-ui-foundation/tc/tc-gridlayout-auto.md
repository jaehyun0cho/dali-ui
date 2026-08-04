# GridLayout: Auto Sizing + Batch Definitions

`GridLength::Auto`가 내용에 맞춰 행·열 크기를 정하고, `GridLength::Star`가 남은 공간을 가져가는지 확인한다. 행·열 정의는 `SetRowDefinitions` / `SetColumnDefinitions` 일괄 API로 설정한다.

## 화면 구성

- 배경은 흰색이며, 루트 `GridLayout`은 너비/높이 모두 `MATCH_PARENT`이다
- 루트 padding은 사방 50px, row spacing 10px, column spacing 10px이다
- 행 정의(위에서부터): `GridLength::Auto()`, `GridLength::Star(1)`, `GridLength::Auto()`
- 열 정의(왼쪽부터): `GridLength::Auto()`, `GridLength::Star(1)`
- 셀 배치
  - (0,0) `Color::RED`: 요청 크기 100x50
  - (0,1) `#FF9999`(`Vector4(1.0, 0.6, 0.6, 1.0)`): 요청 높이 50px
  - (1,0) `Color::GREEN`: 요청 너비 100px
  - (1,1) `Color::BLUE`: 크기 요청 없음
  - (2,0) `Color::YELLOW`: 요청 크기 100x50
  - (2,1) `Color::CYAN`: 요청 높이 50px

```
  [RED  ][ #FF9999      ]   Auto 행 (높이 50px)
  [GREEN][ BLUE         ]   Star(1) 행 (남은 높이)
  [YELLO][ CYAN         ]   Auto 행 (높이 50px)
   Auto      Star(1)
   100px     남은 너비
```

## 테스트 1: Auto 행·열 크기

1. 목록에서 `GridLayout: Auto Sizing + Batch Definitions`를 선택한다
2. **기대 결과**: 첫 번째 열은 내용(너비 100px 자식)에 맞춰 100px 너비가 된다
3. **기대 결과**: 첫 번째 행과 세 번째 행은 내용(높이 50px 자식)에 맞춰 각각 50px 높이가 된다

## 테스트 2: Star 행·열의 남은 공간 흡수

1. 가운데 행과 두 번째 열의 크기를 관찰한다
2. **기대 결과**: 두 번째 열은 padding, spacing, 첫 번째 열을 제외한 남은 가로 공간을 모두 차지한다
3. **기대 결과**: 가운데 `Color::BLUE` 행은 padding, spacing, 위아래 Auto 행(각 50px)을 제외한 남은 세로 공간을 모두 차지한다

## 통과 기준

- 일괄 API(`SetRowDefinitions` / `SetColumnDefinitions`)로 설정한 3행 2열 구성이 그대로 반영되어야 한다
- Auto 행/열의 크기가 그 안의 자식 요청 크기(50px / 100px)와 같아야 한다
- Star 행/열이 남은 공간을 모두 차지하며, 창 크기를 바꾸면 Star 트랙만 따라 커지거나 작아져야 한다
- 6개 셀 색상이 `Color::RED`, `#FF9999`, `Color::GREEN`, `Color::BLUE`, `Color::YELLOW`, `Color::CYAN`으로 표시되어야 한다
