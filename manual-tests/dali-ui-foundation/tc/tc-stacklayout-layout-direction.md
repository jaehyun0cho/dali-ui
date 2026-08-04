# StackLayout: LayoutDirection Toggle

가로 `StackLayout`의 `LayoutDirection`을 `RIGHT_TO_LEFT`로 바꾸면 직계 자식의 배치가 좌우로 뒤집히고, `LayoutMode::STANDALONE` 자식은 뒤집히지 않는지 확인한다.

## 화면 구성

- 배경은 흰색이고, 루트는 `StackOrientation::HORIZONTAL` `StackLayout`으로 콘텐츠 영역을 가득 채운다 (`MATCH_PARENT` x `MATCH_PARENT`)
- 루트 padding은 `Extents(50, 50, 50, 50)`, 자식 사이 spacing은 `10px`
- 왼쪽 바: 빨간색(`Color::RED`), 폭 `100px` 고정, `LayoutAlignment::FILL`
- 가운데: 초록색(`Color::GREEN`), weight `1.0`, `LayoutAlignment::FILL`
- 오른쪽 바: 파란색(`Color::BLUE`), 폭 `100px` 고정, `LayoutAlignment::FILL`
- 토글 버튼: `200x50` 크기의 반투명 검정(`Vector4(0, 0, 0, 0.5)`) `InteractiveView`, 위치 `(0, 0)`, `LayoutMode::STANDALONE`, 자신의 `LayoutDirection`은 `LEFT_TO_RIGHT`로 고정, 흰색 `Change LayoutDirection` 라벨이 중앙에 표시된다

## 테스트 1: 초기 레이아웃 (LEFT_TO_RIGHT)

1. 목록에서 이 TC를 선택해 화면을 관찰한다
2. **기대 결과**: 왼쪽부터 빨강(`100px`) / 초록(남은 폭) / 파랑(`100px`) 순서로 놓인다
3. **기대 결과**: 토글 버튼이 콘텐츠 영역 좌상단에 `200x50` 크기로 떠 있고, 라벨 글자가 뒤집히지 않고 정상으로 읽힌다

## 테스트 2: RIGHT_TO_LEFT로 전환

1. `Change LayoutDirection` 버튼을 탭한다
2. **기대 결과**: 세 바의 순서가 좌우로 뒤집혀 왼쪽부터 파랑(`100px`) / 초록 / 빨강(`100px`)이 된다
3. **기대 결과**: 토글 버튼은 여전히 좌상단 `(0, 0)`에 그대로 있고 라벨도 그대로 읽힌다 (`STANDALONE`이므로 미러링 제외)

## 테스트 3: 다시 LEFT_TO_RIGHT로 복귀

1. `Change LayoutDirection` 버튼을 한 번 더 탭한다
2. **기대 결과**: 테스트 1의 배치(빨강 / 초록 / 파랑)로 되돌아온다

## 테스트 4: 재진입 시 초기화

1. `RIGHT_TO_LEFT` 상태로 만든 뒤 `< Back`으로 목록에 나갔다가 다시 이 TC로 들어온다
2. **기대 결과**: 항상 `LEFT_TO_RIGHT` 상태(빨강 / 초록 / 파랑)로 시작한다

## 통과 기준

- 탭할 때마다 세 바의 좌우 순서가 반전된다
- 좌우가 반전되어도 빨강과 파랑의 폭은 `100px`, 초록은 남은 폭을 유지한다
- 토글 버튼의 위치와 라벨 방향은 어떤 상태에서도 변하지 않는다
- 재진입하면 방향 상태가 `LEFT_TO_RIGHT`로 초기화된다
