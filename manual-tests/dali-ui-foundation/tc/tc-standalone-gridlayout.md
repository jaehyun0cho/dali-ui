# StandaloneMode: GridLayout

`GridLayout` 안에서 `LayoutMode::STANDALONE` 자식이 셀 배정에서 제외되고, 부모 좌표계의 지정 위치에 그려지는지 확인한다.

## 화면 구성

- 배경은 흰색이다
- 루트: `GridLayout`, 폭/높이 모두 `MATCH_PARENT`, padding 50, row spacing 10, column spacing 10
- 행 정의: `GridLength::Absolute(50)`, `GridLength::Absolute(100)`, `GridLength::Absolute(200)`
- 열 정의: `GridLength::Absolute(50)`, `GridLength::Absolute(100)`
- 셀 (0,0): 빨강 `Color::RED`
- 셀 (0,1): 초록 `Color::GREEN`
- 파랑 `Color::BLUE`: `LayoutMode::STANDALONE`, 100x100, `RequestedX`/`RequestedY` = (300, 300) — 셀 (1,0)에 들어가지 않는다
- 셀 (1,1): 노랑 `Color::YELLOW`
- 셀 (2,0): 시안 `Color::CYAN`
- 셀 (2,1): 마젠타 `Color::MAGENTA`

## 테스트 1: 초기 레이아웃

1. 목록에서 이 TC를 선택해 진입한다
2. **기대 결과**: 파랑 STANDALONE 자식이 부모 좌표계 (300, 300)에 100x100 크기로 그려진다
3. **기대 결과**: 파랑 자식에는 `GridLayoutParams`가 설정되어 있지 않으며, 위치와 크기는 오직 `RequestedX`/`RequestedY`와 `RequestedWidth`/`RequestedHeight`로 결정된다

## 테스트 2: 나머지 셀의 배치

1. 빨강, 초록, 노랑, 시안, 마젠타 셀의 위치를 확인한다
2. **기대 결과**: 남은 자식들은 파랑 자식이 아예 없는 것처럼 각자의 행/열 좌표에 배치된다
3. **기대 결과**: 빨강은 (0,0), 초록은 (0,1), 노랑은 (1,1), 시안은 (2,0), 마젠타는 (2,1) 셀을 차지한다
4. **기대 결과**: 행 높이는 위에서부터 50, 100, 200이고 열 폭은 왼쪽부터 50, 100이며, 셀 사이 간격은 가로/세로 모두 10이다
5. **기대 결과**: 셀 (1,0)은 비어 있어 흰 배경이 그대로 보인다

## 통과 기준

- 파랑 STANDALONE 자식이 부모 좌표계 (300, 300)에 100x100으로 그려져야 한다
- 나머지 자식들의 셀 배정과 크기가 STANDALONE 자식이 없는 경우와 동일해야 한다 (셀 배정에서 제외)
- STANDALONE 자식은 부모의 padding 영향을 받지 않아야 한다 (padding 50이 있어도 (300, 300)에 그대로 놓인다)
- 목록으로 나갔다가 다시 진입해도 동일한 배치가 나타나야 한다
