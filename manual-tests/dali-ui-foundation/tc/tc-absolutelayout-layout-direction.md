# AbsoluteLayout: LayoutDirection Toggle

`RIGHT_TO_LEFT` 레이아웃 방향에서 `AbsoluteLayout`의 직계 자식이 좌우로 미러링되고,
`LayoutMode::STANDALONE` 자식은 미러링에서 제외되는지 확인한다.

## 화면 구성

- 배경: 흰색(`Color::WHITE`)
- 루트: `AbsoluteLayout`, 너비/높이 모두 `MATCH_PARENT`
- 빨간 박스(`Color::RED`): `LayoutRect(50, 50, 50, 50)`
- 초록 박스(`Color::GREEN`): `LayoutRect(100, 100, 100, 100)`
- 파란 박스(`Color::BLUE`): `LayoutRect(200, 200, 100, 50)`
- 토글 버튼: 200x50 반투명 검정(알파 0.5) `InteractiveView`, 좌상단 (0, 0),
  `LayoutMode::STANDALONE` + `LayoutDirection::LEFT_TO_RIGHT`,
  가운데 흰색 라벨 `Change LayoutDirection`

## 테스트 1: 초기 LTR 배치

1. 목록에서 본 TC에 진입한다
2. **기대 결과**: 빨강 (50, 50) 50x50, 초록 (100, 100) 100x100, 파랑 (200, 200) 100x50이 왼쪽 기준으로 배치된다
3. **기대 결과**: 토글 버튼이 좌상단 (0, 0)에 200x50으로 표시된다

## 테스트 2: RIGHT_TO_LEFT로 전환

1. `Change LayoutDirection` 버튼을 탭한다
2. **기대 결과**: 세 박스가 루트 너비를 기준으로 좌우 반전되어, 각 박스의 오른쪽 가장자리가 원래 왼쪽 가장자리와 같은 거리만큼 오른쪽 끝에서 떨어진다
3. **기대 결과**: 세 박스의 크기와 세로 위치는 변하지 않는다
4. **기대 결과**: 토글 버튼은 `LayoutMode::STANDALONE`이므로 미러링되지 않고 좌상단에 그대로 남는다
5. **기대 결과**: 토글 버튼의 라벨 텍스트도 좌우가 뒤집히지 않는다

## 테스트 3: LEFT_TO_RIGHT로 복귀

1. 토글 버튼을 다시 탭한다
2. **기대 결과**: 세 박스가 테스트 1과 동일한 위치로 복귀한다

## 통과 기준

- RTL 전환 시 직계 자식만 좌우 미러링되어야 한다
- `LayoutMode::STANDALONE` 자식은 방향 변경과 무관하게 항상 좌상단에 고정되어야 한다
- 토글을 여러 번 눌러도 LTR/RTL 상태가 정확히 왕복해야 한다
- 목록으로 나갔다가 다시 진입하면 항상 `LEFT_TO_RIGHT` 상태로 시작해야 한다
