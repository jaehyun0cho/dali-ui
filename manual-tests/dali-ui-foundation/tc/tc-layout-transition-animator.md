# LayoutTransition: Animator Callback

`LayoutAnimatorCallback`로 등록한 애니메이터가 프레임마다 호출되어 애플리케이션이 직접
크기와 opacity를 기록할 때, 선언형 스펙 모드와 사실상 같은 결과가 나오는지 확인한다.

## 화면 구성

- 배경은 흰색이다
- 상단: 높이 60px의 버튼 행(`FlexLayout`, `FlexDirection::ROW`, `FlexAlign::STRETCH`).
  세 버튼은 `FlexGrow 1`로 폭을 균등 분할하고 좌우 margin 5px씩으로 10px 간격이 생긴다
- 버튼은 모두 검정 배경(`Color::BLACK`) + 흰 글씨(`Color::WHITE`)이며 라벨은 왼쪽부터
  `Click to ENTER`, `Click to EXIT`, `Click to CHANGE`이다
- 하단: 세로 `StackLayout`(spacing 10px). 진입 시 자식 3개가 각각 높이 80px, 폭 `MATCH_PARENT`로
  놓이며 색은 `Color::RED`, `Color::GREEN`, `Color::BLUE` 순이다
- 세 슬롯 모두 0.4초 `EASE_IN_OUT_SINE` `LayoutAnimatorTiming`을 공유한다
- 프레임별 값은 프레임워크가 아니라 애플리케이션 콜백이 기록한다. ENTER / EXIT 콜백은
  `progress`로 OPACITY와 높이를 직접 쓰고, CHANGE 콜백은 `fromBounds`와 `toBounds`를
  선형 보간해 위치와 크기를 직접 쓴다

## 테스트 1: ENTER 애니메이터

1. `Click to ENTER`를 누른다
2. **기대 결과**: 새 자식이 스택 맨 아래에 추가되면서 높이 0 → 80px로 펼쳐지고
   동시에 opacity 0 → 1로 나타난다. 소요 시간은 0.4초이다
3. 여러 번 누른다
4. **기대 결과**: 자식이 `Color::RED` → `Color::GREEN` → `Color::BLUE` → `Color::YELLOW` →
   `Color::CYAN` → `Color::MAGENTA` 순환 순서로 계속 추가된다

## 테스트 2: EXIT 애니메이터

1. `Click to EXIT`를 누른다
2. **기대 결과**: 마지막 자식이 높이 80px → 0으로 수축하고 opacity 1 → 0으로 사라진 뒤
   제거된다
3. **기대 결과**: 자식이 하나도 남지 않은 상태에서 눌러도 아무 일도 일어나지 않는다

## 테스트 3: CHANGE 애니메이터

1. `Click to CHANGE`를 누른다
2. **기대 결과**: 모든 자식의 높이가 80px → 160px로 0.4초에 걸쳐 늘어나고,
   아래쪽 자식들의 위치 이동도 콜백이 기록한 값으로 함께 애니메이션된다
3. 다시 누른다
4. **기대 결과**: 모든 자식의 높이가 160px → 80px로 되돌아간다

## 테스트 4: 최초 자식의 애니메이션 억제

1. TC에 처음 들어온 직후 화면을 본다
2. **기대 결과**: 처음 놓인 자식 3개는 ENTER 애니메이션 없이 opacity 1, 높이 80px로 바로 보인다
   (최초 배치 패스에서는 애니메이터 ENTER가 호출되지 않는다)

## 테스트 5: 재진입 초기화

1. 자식을 추가하거나 제거하고 높이를 160px로 바꾼다
2. `< Back`으로 목록에 나갔다가 다시 이 TC로 들어온다
3. **기대 결과**: 자식 3개(빨강/초록/파랑), 높이 80px인 최초 상태로 초기화되어 있다

## 통과 기준

- 시각적 결과가 `LayoutTransition: Declarative Spec`과 사실상 동일해야 한다
- ENTER / EXIT / CHANGE 세 슬롯 모두 0.4초 `EASE_IN_OUT_SINE`으로 애니메이션되어야 한다
- 진행 중 프레임이 끊기거나 값이 튀지 않고 매끄럽게 보간되어야 한다
- 목록으로 나갔다 다시 들어오면 항상 자식 3개 · 높이 80px 상태로 초기화되어야 한다
