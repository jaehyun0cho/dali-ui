# GridLayout: RowSpan / ColumnSpan

`GridLayoutParams`의 `SetRowSpan` / `SetColumnSpan`으로 셀이 여러 행·열에 걸쳐 확장되는지 확인한다.

## 화면 구성

- 배경은 흰색이며, 루트 `GridLayout`은 너비/높이 모두 `MATCH_PARENT`이다
- 루트 padding은 사방 50px, row spacing 10px, column spacing 10px이다
- 3행 3열이며 모든 행·열 정의가 `GridLength::Star(1)`이라 균등하게 나뉜다
- 셀 배치
  - `Color::RED`: (0,0), `SetColumnSpan(2)`
  - `Color::GREEN`: (0,2)
  - `Color::BLUE`: (1,0)
  - `Color::YELLOW`: (1,1), `SetRowSpan(2)` + `SetColumnSpan(2)`
  - `Color::CYAN`: (2,0)

```
  [ RED (2열)      ][GREEN ]
  [BLUE  ][ YELLOW (2행 x 2열) ]
  [CYAN  ][                    ]
```

## 테스트 1: ColumnSpan

1. 목록에서 `GridLayout: RowSpan / ColumnSpan`을 선택한다
2. **기대 결과**: 최상단 `Color::RED` 셀이 0열과 1열을 모두 덮으며, 두 열 사이의 10px 간격까지 채운다
3. **기대 결과**: `Color::RED` 셀의 오른쪽에 `Color::GREEN` 셀이 2열 한 칸만 차지한 채 놓인다

## 테스트 2: RowSpan + ColumnSpan

1. 노란색 셀의 크기를 관찰한다
2. **기대 결과**: `Color::YELLOW` 셀이 1~2행 x 1~2열의 사각 영역을 모두 덮으며, 그 안쪽 spacing까지 채워 하나의 큰 블록으로 보인다
3. **기대 결과**: `Color::BLUE`는 (1,0), `Color::CYAN`은 (2,0)에 각각 한 칸씩만 놓인다

## 통과 기준

- span이 적용된 셀은 병합된 트랙 전체 + 그 사이 spacing까지 덮어야 한다
- span이 없는 셀은 자기 칸을 넘지 않아야 한다
- 3행 3열이 모두 `GridLength::Star(1)`이므로 각 행 높이와 각 열 너비가 서로 같아야 한다
- 창 크기를 바꿔도 행·열 비율과 span 영역의 관계가 유지되어야 한다
