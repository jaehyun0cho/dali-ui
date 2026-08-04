# StandaloneMode: StackLayout

`StackLayout` 안에서 `LayoutMode::STANDALONE` 자식이 스택 누적과 spacing 모두에서 제외되고, 부모 좌표계의 지정 위치에 그려지는지 확인한다.

## 화면 구성

- 배경은 흰색이다
- 루트: 세로 `StackLayout`(`StackOrientation::VERTICAL`), 폭/높이 모두 `MATCH_PARENT`, spacing 10, padding 50
- 상단 바: 빨강 `Color::RED`, 높이 100, `StackLayoutParams`의 `SetAlignment(LayoutAlignment::FILL)`
- 중간: 초록 `Color::GREEN`, `StackLayoutParams`의 `SetWeight(1.0f)` + `SetAlignment(LayoutAlignment::FILL)`
- 하단 바: 파랑 `Color::BLUE`, `LayoutMode::STANDALONE`, 100x100, `RequestedX`/`RequestedY` = (300, 300)

## 테스트 1: 초기 레이아웃

1. 목록에서 이 TC를 선택해 진입한다
2. **기대 결과**: 파랑 STANDALONE 자식이 부모 좌표계 (300, 300)에 100x100 크기로 그려진다
3. **기대 결과**: 파랑 자식의 위치와 크기는 오직 `RequestedX`/`RequestedY`와 `RequestedWidth`/`RequestedHeight`로 결정된다

## 테스트 2: 나머지 자식의 배치

1. 빨강 상단 바와 초록 중간 영역의 위치와 크기를 확인한다
2. **기대 결과**: 남은 자식들은 파랑 자식이 아예 없는 것처럼 세로로 쌓인다
3. **기대 결과**: 빨강 상단 바는 높이 100이고 `LayoutAlignment::FILL`로 padding 50을 뺀 폭을 채운다
4. **기대 결과**: 초록 중간 영역은 `SetWeight(1.0f)`로 남은 세로 공간을 모두 차지하며, 아래쪽 padding 50까지 이어진다
5. **기대 결과**: 빨강과 초록 사이에만 spacing 10이 적용되고, 파랑 자식 때문에 추가되는 spacing은 없다

## 통과 기준

- 파랑 STANDALONE 자식이 부모 좌표계 (300, 300)에 100x100으로 그려져야 한다
- 나머지 자식들의 스택 배치가 STANDALONE 자식이 없는 경우와 동일해야 한다 (누적과 spacing 모두에서 제외)
- 초록 영역의 높이가 파랑 자식의 높이 100이나 spacing 10만큼 줄어들지 않아야 한다
- STANDALONE 자식은 부모의 padding 영향을 받지 않아야 한다 (padding 50이 있어도 (300, 300)에 그대로 놓인다)
- 목록으로 나갔다가 다시 진입해도 동일한 배치가 나타나야 한다
