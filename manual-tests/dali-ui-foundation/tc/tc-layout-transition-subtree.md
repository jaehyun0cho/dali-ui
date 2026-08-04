# LayoutTransition: Reflow Scope (SUBTREE / DIRECT)

루트 컨테이너에 붙인 단 하나의 `LayoutTransition`이 `LayoutReflowScope::SUBTREE`에서는
손자까지 재배치를 애니메이션하고, `LayoutReflowScope::DIRECT_CHILDREN`에서는 직속 자식만
애니메이션하는지 확인한다.

## 화면 구성

- 배경은 흰색이다
- 상단: 높이 60px의 버튼 행(`FlexLayout`, `FlexDirection::ROW`, `FlexAlign::STRETCH`).
  세 버튼은 `FlexGrow 1`로 폭을 균등 분할하고 좌우 margin 5px씩으로 10px 간격이 생긴다
- 버튼은 모두 검정 배경(`Color::BLACK`) + 흰 글씨(`Color::WHITE`)이며 라벨은 왼쪽부터
  `Toggle layout`, `Scope: SUBTREE`, `Add/Remove item`이다
- 그 아래 루트 세로 `StackLayout`(spacing 10px)에 `LayoutTransition`이 붙어 있다
  - 첫 자식: 회색 spacer(`Color::LIGHT_GRAY`), 폭 `MATCH_PARENT`, 높이 40px
  - 둘째 자식: 검정 카드(`Color::BLACK`, `WRAP_CONTENT` 높이, spacing 10px).
    카드 자체에는 트랜지션이 없다
  - 카드 안 항목 3개: `Color::RED`, `Color::GREEN`, `Color::BLUE`, 각각 높이 60px
- CHANGE 타이밍은 0.4초 `EASE_IN_OUT_SINE`, ENTER / EXIT의 opacity 페이드는 0.3초이다

## 테스트 1: SUBTREE 범위에서 토글

1. 진입 직후(`Scope: SUBTREE` 상태에서) `Toggle layout`을 누른다
2. **기대 결과**: spacer 높이가 40px → 160px로, 카드 안 항목 3개의 높이가 60px → 100px로
   **모두** 0.4초에 걸쳐 애니메이션된다. 카드도 spacer에 밀려 내려가는 움직임이 애니메이션된다
3. 다시 누른다
4. **기대 결과**: spacer 160px → 40px, 항목 100px → 60px로 되돌아가며 역시 모두 애니메이션된다

## 테스트 2: DIRECT_CHILDREN 범위에서 토글

1. `Scope: SUBTREE`를 누른다
2. **기대 결과**: 버튼 라벨이 `Scope: DIRECT`로 바뀐다
3. `Toggle layout`을 누른다
4. **기대 결과**: spacer와 카드(루트의 직속 자식)만 0.4초 애니메이션으로 움직이고,
   카드 안 항목 3개의 높이 변화는 애니메이션 없이 즉시 스냅된다
5. `Scope: DIRECT`를 다시 눌러 `Scope: SUBTREE`로 되돌린 뒤 토글한다
6. **기대 결과**: 테스트 1과 같이 안쪽 항목까지 다시 애니메이션된다

## 테스트 3: 손자 항목 추가 / 제거

1. `Scope: SUBTREE` 상태에서 `Add/Remove item`을 누른다
2. **기대 결과**: 카드 안에 마젠타(`Color::MAGENTA`) 항목이 추가되며 opacity 0 → 1 페이드와
   높이 0 → 60px 확장이 함께 일어난다(루트 트랜지션의 ENTER 슬롯이 손자까지 도달)
3. 다시 누른다
4. **기대 결과**: 마젠타 항목이 페이드아웃 + 높이 수축 후 사라진다
5. `Scope: DIRECT`로 바꾼 뒤 `Add/Remove item`을 누른다
6. **기대 결과**: 마젠타 항목이 애니메이션 없이 즉시 나타나고, 다시 누르면 즉시 사라진다

## 테스트 4: 재진입 초기화

1. 스코프를 `Scope: DIRECT`로 바꾸고 레이아웃을 펼친 뒤 마젠타 항목을 추가한다
2. `< Back`으로 목록에 나갔다가 다시 이 TC로 들어온다
3. **기대 결과**: 버튼 라벨이 `Scope: SUBTREE`로 돌아오고, spacer 40px · 항목 60px · 마젠타 항목
   없는 최초 상태로 초기화되어 있다

## 통과 기준

- `SUBTREE` 범위에서는 루트의 트랜지션 하나가 직속 자식과 손자 항목을 모두 애니메이션해야 한다
- `DIRECT_CHILDREN` 범위에서는 손자 항목이 애니메이션 없이 최종 위치/크기로 스냅되어야 한다
- 카드에는 자체 트랜지션이 없는데도 `SUBTREE`에서 안쪽 항목이 재배치 애니메이션되어야 한다
- 손자 추가 / 제거도 스코프에 따라 애니메이션 여부가 달라져야 한다
- 목록으로 나갔다 다시 들어오면 항상 `Scope: SUBTREE` · 접힌 상태로 초기화되어야 한다
