# StandaloneMode: FlexLayout

`FlexLayout` 안에서 `LayoutMode::STANDALONE` 자식이 flex line에서 제외되고, 부모 좌표계의 지정 위치에 그려지는지 확인한다.

## 화면 구성

- 배경은 흰색이다
- 루트: `FlexLayout`, 폭/높이 모두 `MATCH_PARENT`, `FlexDirection::ROW`, `FlexAlign::STRETCH`, padding 50
- 빨강 박스: `Color::RED`, 폭 100 고정
- 초록 박스: `Color::GREEN`, 폭 `WRAP_CONTENT`, `FlexLayoutParams`의 `SetFlexGrow(1.0f)`
- 파랑 박스: `Color::BLUE`, `LayoutMode::STANDALONE`, 100x100, `RequestedX`/`RequestedY` = (300, 300)

## 테스트 1: 초기 레이아웃

1. 목록에서 이 TC를 선택해 진입한다
2. **기대 결과**: 파랑 STANDALONE 박스가 부모 좌표계 (300, 300)에 100x100 크기로 그려진다
3. **기대 결과**: 파랑 박스의 위치와 크기는 오직 `RequestedX`/`RequestedY`와 `RequestedWidth`/`RequestedHeight`로 결정된다

## 테스트 2: 나머지 자식의 배치

1. 빨강 박스와 초록 박스의 위치와 크기를 확인한다
2. **기대 결과**: 남은 자식들은 파랑 박스가 아예 없는 것처럼 flex row로 배치된다
3. **기대 결과**: 빨강 박스는 폭 100을 유지하고, 초록 박스는 `SetFlexGrow(1.0f)`로 padding 50을 뺀 나머지 폭을 모두 차지한다
4. **기대 결과**: `FlexAlign::STRETCH`에 의해 빨강과 초록 박스의 높이는 padding 50을 뺀 콘텐츠 높이를 채운다
5. **기대 결과**: 초록 박스가 차지하는 잔여 폭 계산에 파랑 박스는 전혀 포함되지 않는다

## 통과 기준

- 파랑 STANDALONE 박스가 부모 좌표계 (300, 300)에 100x100으로 그려져야 한다
- 나머지 자식들의 flex 배치가 STANDALONE 자식이 없는 경우와 동일해야 한다 (flex line에서 제외)
- STANDALONE 자식은 부모의 padding 영향을 받지 않아야 한다 (padding 50이 있어도 (300, 300)에 그대로 놓인다)
- 목록으로 나갔다가 다시 진입해도 동일한 배치가 나타나야 한다
