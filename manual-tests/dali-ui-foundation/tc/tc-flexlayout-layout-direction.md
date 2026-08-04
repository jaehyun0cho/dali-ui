# FlexLayout: LayoutDirection Toggle

`SetLayoutDirection`으로 `RIGHT_TO_LEFT`를 지정했을 때 `FlexLayout`의 직계 자식이 좌우로 미러링되고, `LayoutMode::STANDALONE` 자식은 미러링에서 제외되는지 확인한다.

## 화면 구성

- 배경은 흰색(`Color::WHITE`)이다.
- 루트는 `MATCH_PARENT x MATCH_PARENT` 크기의 `FlexLayout`이며 `FlexDirection::ROW`, `FlexAlign::STRETCH`, padding `Extents(50, 50, 50, 50)`가 설정된다.
- 루트의 자식은 추가 순서대로 다음과 같다.
  - 빨강(`Color::RED`): 폭 100px 고정
  - 초록(`Color::GREEN`): 폭 `WRAP_CONTENT` + `SetFlexGrow(1.0f)`
  - 파랑(`Color::BLUE`): 폭 100px 고정
  - 토글 버튼: 200x50 `InteractiveView`, 배경 `Vector4(0.0, 0.0, 0.0, 0.5)`(반투명 검정), 흰색 `Change LayoutDirection` 라벨, `SetRequestedX(0)` / `SetRequestedY(0)`, `LayoutMode::STANDALONE`, 자신은 `LayoutDirection::LEFT_TO_RIGHT` 고정
- 토글 버튼은 `STANDALONE`이므로 flex 배치 계산에서 제외되고 루트의 좌상단 (0, 0)에 놓인다.

## 테스트 1: 초기 LTR 배치

1. 목록에서 이 TC를 선택한다
2. **기대 결과**: 왼쪽부터 빨강 → 초록 → 파랑 순으로 배치된다
3. **기대 결과**: 빨강과 파랑의 폭은 100px, 초록이 남은 공간을 차지한다
4. **기대 결과**: 반투명 검정 토글 버튼이 좌상단에 보이고 라벨 `Change LayoutDirection`이 가운데 정렬로 표시된다

## 테스트 2: RTL 전환

1. `Change LayoutDirection` 버튼을 탭한다
2. **기대 결과**: 루트가 `LayoutDirection::RIGHT_TO_LEFT`가 되어 세 박스가 좌우 반전된다. 즉 왼쪽부터 파랑 → 초록 → 빨강 순이다
3. **기대 결과**: 빨강과 파랑의 폭은 여전히 100px이고 초록이 남은 공간을 차지한다
4. **기대 결과**: 토글 버튼은 미러링되지 않고 계속 좌상단에 머무르며, 라벨도 좌우 반전되지 않는다

## 테스트 3: LTR 복귀

1. 버튼을 한 번 더 탭한다
2. **기대 결과**: 다시 `LEFT_TO_RIGHT`가 되어 테스트 1의 배치(빨강 → 초록 → 파랑)로 돌아온다

## 테스트 4: 재진입 초기화

1. RTL 상태에서 뒤로 가기로 목록으로 나간 뒤 이 TC에 다시 들어온다
2. **기대 결과**: 항상 LTR 상태(빨강 → 초록 → 파랑)로 시작한다

## 통과 기준

- 탭할 때마다 LTR과 RTL이 번갈아 전환되어야 한다
- RTL에서 직계 자식 세 개의 좌우 순서가 정확히 반전되어야 하고 각 박스의 폭은 유지되어야 한다
- `STANDALONE` 토글 버튼은 두 방향 모두에서 좌상단 (0, 0)에 고정되어야 한다
- 목록으로 나갔다 다시 들어오면 항상 LTR 상태로 초기화되어야 한다
