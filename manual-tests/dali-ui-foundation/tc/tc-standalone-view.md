# StandaloneMode: View

평범한 `View` 안에서 `LayoutMode::STANDALONE` 자식이 부모의 `WRAP_CONTENT` 누적에서 제외되고, 부모 좌표계의 지정 위치에 그려지는지 확인한다.

## 화면 구성

- 배경은 흰색이다
- 루트: `View`, `Color::GRAY`, 폭/높이 모두 `MATCH_PARENT`
- 자식 1: 빨강 `Color::RED`, 폭 `MATCH_PARENT`, 높이 200, margin 50
- 자식 2: 초록 `Color::GREEN`, 폭 `WRAP_CONTENT`, 높이 200, `RequestedY` 300, margin 50
  - 자식 2의 자식: 노랑 `Color::YELLOW`, 100x100
- 자식 3: 파랑 `Color::BLUE`, `LayoutMode::STANDALONE`, 100x100, `RequestedX`/`RequestedY` = (300, 300)

## 테스트 1: 초기 레이아웃

1. 목록에서 이 TC를 선택해 진입한다
2. **기대 결과**: 파랑 STANDALONE 자식이 부모(루트) 좌표계 (300, 300)에 100x100 크기로 그려진다
3. **기대 결과**: 파랑 자식의 위치와 크기는 오직 `RequestedX`/`RequestedY`와 `RequestedWidth`/`RequestedHeight`로만 결정된다

## 테스트 2: 나머지 자식의 배치

1. 빨강 자식과 초록 자식(및 그 안의 노랑 손자)의 위치를 확인한다
2. **기대 결과**: 남은 자식들은 파랑 자식이 아예 없는 것처럼 배치된다
3. **기대 결과**: 빨강 자식은 margin 50이 적용된 채 폭이 루트를 채우고 높이는 200이다
4. **기대 결과**: 초록 자식은 `WRAP_CONTENT`이므로 노랑 손자 100x100을 감싸는 폭이 되고, `RequestedY` 300에 margin 50이 더해진 y=350에 놓인다
5. **기대 결과**: 루트는 파랑 자식을 일반 배치 대상에서 제외하므로, 빨강/초록의 위치와 크기에 파랑이 아무 영향도 주지 않는다

## 통과 기준

- 파랑 STANDALONE 자식이 부모 좌표계 (300, 300)에 100x100으로 그려져야 한다
- 나머지 자식들의 배치가 STANDALONE 자식이 없는 경우와 동일해야 한다 (루트가 파랑을 일반 자식 배치에서 제외)
- 목록으로 나갔다가 다시 진입해도 동일한 배치가 나타나야 한다
