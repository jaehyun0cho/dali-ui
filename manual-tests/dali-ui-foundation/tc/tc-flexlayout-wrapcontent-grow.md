# FlexLayout: WrapContent MinSize + FlexGrow

`WRAP_CONTENT` 폭에 `SetMinimumWidth`가 걸린 `FlexLayout`에서, 최소 크기 때문에 생긴 여유 공간을 `SetFlexGrow` 자식이 흡수하는지 확인한다.

## 화면 구성

- 배경은 흰색(`Color::WHITE`)이다.
- 루트는 `FlexLayout`(`FlexDirection::ROW`)이며 배경은 게인즈버러(`Color::GAINSBORO`), `SetMinimumWidth(400.0f)`, 높이 200px이다. 폭은 지정하지 않으므로 `WRAP_CONTENT`로 동작한다.
- 루트는 콘텐츠 영역의 좌상단에 놓인다.
- 자식 구성은 다음과 같다.
  - child1 빨강(`Color::RED`): 200x200 고정
  - child2 초록(`Color::GREEN`): `FlexLayout`(`FlexDirection::ROW`), 높이 200px, `SetFlexGrow(1.0f)`
    - grandChild1 파랑(`Color::BLUE`): 높이 200px, `SetFlexGrow(1.0f)`
    - grandChild2 노랑(`Color::YELLOW`): 높이 200px, `SetFlexGrow(1.0f)`

```
  +------------- root 400x200 (min width 400) -------------+
  | [red 200x200] | [blue 100] [yellow 100]  <- green 200  |
  +--------------------------------------------------------+
```

## 테스트 1: 초기 레이아웃

1. 목록에서 이 TC를 선택한다
2. **기대 결과**: 루트의 폭이 콘텐츠 폭 200px이 아니라 `SetMinimumWidth`가 정한 400px이다
3. **기대 결과**: 빨강이 왼쪽 200x200 영역을 차지한다
4. **기대 결과**: 초록이 `400 - 200 = 200`px의 잔여 공간을 모두 받아 오른쪽 절반을 차지한다
5. **기대 결과**: 초록 안에서 파랑과 노랑이 각각 100px씩 폭을 나눠 가진다
6. **기대 결과**: 루트 높이는 200px이며, 게인즈버러 배경이 보이는 빈 영역이 없다

## 통과 기준

- 루트의 폭이 400px여야 한다
- 빨강 200px, 초록 200px로 가로가 정확히 나뉘어야 한다
- 초록 내부의 파랑과 노랑이 각각 100px로 같아야 한다
- 루트 좌상단이 콘텐츠 영역의 좌상단(0, 0)에 놓여야 한다
- 창 크기를 바꿔도 루트 크기가 400x200으로 유지되어야 한다
