# StandaloneMode: AbsoluteLayout

`AbsoluteLayout` 안에서 `LayoutMode::STANDALONE` 자식이 `AbsoluteLayoutParams` 대신 자신의 `RequestedX`/`RequestedY`로 배치되는지 확인한다.

## 화면 구성

- 배경은 흰색이다
- 루트: `AbsoluteLayout`, 폭/높이 모두 `MATCH_PARENT`
- 빨강 박스: `Color::RED`, `AbsoluteLayoutParams`의 `SetBounds(LayoutRect(50, 50, 50, 50))`
- 초록 박스: `Color::GREEN`, `AbsoluteLayoutParams`의 `SetBounds(LayoutRect(100, 100, 100, 100))`
- 파랑 박스: `Color::BLUE`, `LayoutMode::STANDALONE`, 100x100, `RequestedX`/`RequestedY` = (300, 300)

## 테스트 1: 초기 레이아웃

1. 목록에서 이 TC를 선택해 진입한다
2. **기대 결과**: 파랑 STANDALONE 박스가 부모 좌표계 (300, 300)에 100x100 크기로 그려진다
3. **기대 결과**: 파랑 박스에는 `AbsoluteLayoutParams`가 설정되어 있지 않으며, 위치와 크기는 오직 `RequestedX`/`RequestedY`와 `RequestedWidth`/`RequestedHeight`로 결정된다

## 테스트 2: 나머지 자식의 배치

1. 빨강 박스와 초록 박스의 위치를 확인한다
2. **기대 결과**: 남은 자식들은 파랑 박스가 아예 없는 것처럼 배치된다
3. **기대 결과**: 빨강 박스는 (50, 50)에 50x50, 초록 박스는 (100, 100)에 100x100으로 그려진다
4. **기대 결과**: 세 박스가 서로 겹치지 않는다

## 통과 기준

- 파랑 STANDALONE 박스가 부모 좌표계 (300, 300)에 100x100으로 그려져야 한다
- 나머지 자식들의 `AbsoluteLayoutParams` 기반 배치가 STANDALONE 자식이 없는 경우와 동일해야 한다
- STANDALONE 자식에게는 `AbsoluteLayoutParams`가 무시되어야 한다
- 목록으로 나갔다가 다시 진입해도 동일한 배치가 나타나야 한다
