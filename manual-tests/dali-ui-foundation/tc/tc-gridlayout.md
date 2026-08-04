# GridLayout: Absolute Rows / Columns

GridLayout이 `GridLength::Absolute`로 정의한 고정 크기 행·열에 자식을 정확히 배치하는지 확인한다.

## 화면 구성

- 배경은 흰색이며, 루트 `GridLayout`은 너비/높이 모두 `MATCH_PARENT`이다
- 루트 padding은 사방 50px, row spacing 10px, column spacing 10px이다
- 행 정의(위에서부터): `GridLength::Absolute(50)`, `GridLength::Absolute(100)`, `GridLength::Absolute(200)`
- 열 정의(왼쪽부터): `GridLength::Absolute(50)`, `GridLength::Absolute(100)`
- 셀 6개는 `GridLayoutParams`로 행·열을 지정하며, 추가 순서는 (0,0) → (0,1) → (1,0) → (1,1) → (2,0) → (2,1)이다

```
  [RED    ][GREEN  ]   행 높이 50px
  [BLUE   ][YELLOW ]   행 높이 100px
  [CYAN   ][MAGENTA]   행 높이 200px
    50px     100px
```

## 테스트 1: 초기 레이아웃

1. 목록에서 `GridLayout: Absolute Rows / Columns`를 선택한다
2. **기대 결과**: 6개 셀이 `Color::RED`, `Color::GREEN`, `Color::BLUE`, `Color::YELLOW`, `Color::CYAN`, `Color::MAGENTA` 순서로 2열 3행에 배치된다
3. **기대 결과**: 좌상단 빨강 셀의 좌상단 모서리가 콘텐츠 영역 좌상단에서 가로 50px, 세로 50px 떨어진 위치에 놓인다
4. **기대 결과**: 행 높이는 위에서부터 50px / 100px / 200px, 열 너비는 왼쪽부터 50px / 100px이다

## 테스트 2: spacing 과 padding

1. 셀과 셀 사이 간격을 관찰한다
2. **기대 결과**: 행 사이와 열 사이에 각각 10px의 흰색 간격이 보인다
3. **기대 결과**: 그리드 바깥쪽 사방으로 50px의 흰색 여백이 남는다

## 통과 기준

- 6개 셀의 색상과 행·열 위치가 위 배치도와 정확히 일치해야 한다
- 행 높이 50/100/200px, 열 너비 50/100px가 창 크기와 무관하게 고정되어야 한다
- 셀 사이 간격 10px와 그리드 padding 50px가 유지되어야 한다
- 배경이 흰색으로 보여야 한다
