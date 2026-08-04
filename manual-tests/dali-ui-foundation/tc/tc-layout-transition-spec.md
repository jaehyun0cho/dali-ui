# LayoutTransition: Declarative Spec

StackLayout에 선언형 `LayoutTransition`을 붙였을 때 ENTER / EXIT / CHANGE 슬롯이 모두
0.4초 `EASE_IN_OUT_SINE`으로 애니메이션되는지 확인한다.

## 화면 구성

- 배경은 흰색이다
- 상단: 높이 60px의 버튼 행(`FlexLayout`, `FlexDirection::ROW`, `FlexAlign::STRETCH`).
  세 버튼은 `FlexGrow 1`로 폭을 균등 분할하고 좌우 margin 5px씩으로 10px 간격이 생긴다
- 버튼은 모두 검정 배경(`Color::BLACK`) + 흰 글씨(`Color::WHITE`)이며 라벨은 왼쪽부터
  `Click to ENTER`, `Click to EXIT`, `Click to CHANGE`이다
- 하단: 세로 `StackLayout`(spacing 10px). 진입 시 자식 3개가 각각 높이 80px, 폭 `MATCH_PARENT`로
  놓이며 색은 `Color::RED`, `Color::GREEN`, `Color::BLUE` 순이다
- 자식 색은 `Color::RED` → `Color::GREEN` → `Color::BLUE` → `Color::YELLOW` → `Color::CYAN` →
  `Color::MAGENTA` 순으로 순환한다

## 테스트 1: ENTER 슬롯

1. `Click to ENTER`를 누른다
2. **기대 결과**: 새 자식이 스택 맨 아래에 추가되면서 높이 0 → 80px로 펼쳐지고
   동시에 opacity 0 → 1로 나타난다. 소요 시간은 0.4초이고 가감속은 `EASE_IN_OUT_SINE`이다
3. 여러 번 누른다
4. **기대 결과**: 자식이 팔레트 순서대로 계속 추가되고, 매번 같은 방식으로 펼쳐진다

## 테스트 2: EXIT 슬롯

1. `Click to EXIT`를 누른다
2. **기대 결과**: 마지막 자식이 높이 80px → 0으로 수축하고 opacity 1 → 0으로 사라진 뒤
   제거된다. 수축은 위쪽 모서리(`LayoutBoundsEdge::TOP`) 기준으로 일어난다
3. **기대 결과**: 자식이 하나도 남지 않은 상태에서 눌러도 아무 일도 일어나지 않는다

## 테스트 3: CHANGE 슬롯

1. `Click to CHANGE`를 누른다
2. **기대 결과**: 모든 자식의 높이가 80px → 160px로 0.4초에 걸쳐 늘어나고,
   아래쪽 자식들이 밀려나는 움직임도 함께 애니메이션된다
3. 다시 누른다
4. **기대 결과**: 모든 자식의 높이가 160px → 80px로 되돌아간다
5. 높이가 160px인 상태에서 `Click to ENTER`를 누른다
6. **기대 결과**: 새로 추가되는 자식도 160px 높이로 펼쳐진다

## 테스트 4: 재진입 초기화

1. 자식을 추가하거나 제거하고 높이를 160px로 바꾼다
2. `< Back`으로 목록에 나갔다가 다시 이 TC로 들어온다
3. **기대 결과**: 자식 3개(빨강/초록/파랑), 높이 80px인 최초 상태로 초기화되어 있다

## 통과 기준

- ENTER / EXIT / CHANGE 세 슬롯이 모두 0.4초 `EASE_IN_OUT_SINE`으로 애니메이션되어야 한다
- ENTER는 높이 확장과 opacity 페이드인이 동시에 일어나야 한다
- EXIT는 높이 수축과 opacity 페이드아웃이 끝난 뒤에 자식이 사라져야 한다
- 한 자식이 추가/제거될 때 나머지 자식의 재배치도 CHANGE 슬롯으로 애니메이션되어야 한다
- 목록으로 나갔다 다시 들어오면 항상 자식 3개 · 높이 80px 상태로 초기화되어야 한다
