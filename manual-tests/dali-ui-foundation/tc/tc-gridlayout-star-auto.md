# GridLayout: Star Sizing + Span

`GridLength::Star`의 비례 계수가 남은 공간을 계수 비율대로 나누는지, 그리고 `ColumnSpan`으로 전체 열을 차지하는 헤더/푸터가 정상 동작하는지 확인한다.

## 화면 구성

- 배경은 흰색이며, 루트 `GridLayout`은 너비/높이 모두 `MATCH_PARENT`이다
- 루트 padding은 사방 50px, row spacing 10px, column spacing 10px이다
- 행 정의(위에서부터): `GridLength::Absolute(100)`, `GridLength::Star(1)`, `GridLength::Absolute(50)`
- 열 정의(왼쪽부터): `GridLength::Absolute(100)`, `GridLength::Star(2)`, `GridLength::Star(1)`
- 셀 배치
  - `Color::RED` 헤더: (0,0), `SetColumnSpan(3)`
  - `Color::GREEN` 사이드바: (1,0)
  - `Color::BLUE` 콘텐츠: (1,1) — `Star(2)` 열
  - `Color::YELLOW` 패널: (1,2) — `Star(1)` 열
  - `Color::CYAN` 푸터: (2,0), `SetColumnSpan(3)`

```
  [ RED  헤더 (3열 전체, 높이 100px) ]
  [GREEN ][ BLUE  Star(2) ][YELLOW S(1)]
  [ CYAN 푸터 (3열 전체, 높이 50px)  ]
   100px
```

## 테스트 1: 고정 행 + 유연 행

1. 목록에서 `GridLayout: Star Sizing + Span`을 선택한다
2. **기대 결과**: `Color::RED` 헤더 행 높이는 100px, `Color::CYAN` 푸터 행 높이는 50px로 고정된다
3. **기대 결과**: 가운데 `Star(1)` 행이 남은 세로 공간을 모두 차지한다

## 테스트 2: Star 계수 비율

1. `Color::BLUE` 콘텐츠 열과 `Color::YELLOW` 패널 열의 너비를 비교한다
2. **기대 결과**: `Star(2)` 열인 `Color::BLUE`가 `Star(1)` 열인 `Color::YELLOW`보다 정확히 2배 넓다
3. **기대 결과**: `Color::GREEN` 사이드바 열은 `Absolute(100)`이므로 항상 100px 너비를 유지한다

## 테스트 3: ColumnSpan 헤더/푸터

1. 최상단과 최하단 셀의 가로 범위를 관찰한다
2. **기대 결과**: `Color::RED` 헤더와 `Color::CYAN` 푸터가 3개 열 전체와 그 사이 column spacing까지 덮는다

## 통과 기준

- 고정 행(100px / 50px)과 고정 열(100px)의 크기가 창 크기와 무관하게 유지되어야 한다
- 창 크기를 바꿔도 `Star(2)` 열과 `Star(1)` 열의 너비 비율이 항상 2:1이어야 한다
- 헤더와 푸터가 3열 전체를 덮어야 한다
