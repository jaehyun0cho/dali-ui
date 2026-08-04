# GridLayout: WrapContent MinSize + Star Column

내용에 맞춰 크기가 정해지는 `GridLayout`에 최소 너비를 지정했을 때, 그로 인해 생긴 여유 공간을 `GridLength::Star` 열이 모두 흡수하는지 확인한다.

## 화면 구성

- 배경은 흰색이며, 루트 `GridLayout`은 콘텐츠 영역 좌상단에 놓인다
- 루트 `GridLayout`: 배경 `Color::GAINSBORO`, `SetMinimumWidth(400)`, 요청 높이 200px, padding/spacing 없음
- 루트 열 정의: `GridLength::Auto()`, `GridLength::Star(1)`
- 루트 행 정의: `GridLength::Star(1)`
- child1 `Color::RED`: (0,0) Auto 열, 요청 너비 200px
- child2 `Color::GREEN`: (0,1) Star 열에 놓인 중첩 `GridLayout`
  - 중첩 열 정의: `GridLength::Star(1)`, `GridLength::Star(1)`
  - 중첩 행 정의: `GridLength::Star(1)`
  - grandChild1 `Color::BLUE`: (0,0)
  - grandChild2 `Color::YELLOW`: (0,1)

```
  |<--------------- 400px --------------->|
  [ RED  200px  ][ BLUE 100 ][ YELLOW 100 ]   높이 200px
     Auto 열          Star(1) 열 = 200px
```

## 테스트 1: 최소 너비로 wrap 되는 루트

1. 목록에서 `GridLayout: WrapContent MinSize + Star Column`을 선택한다
2. **기대 결과**: 루트 그리드가 콘텐츠 영역 좌상단에 400x200 크기로 놓인다 (Auto 내용은 200px뿐이지만 `SetMinimumWidth(400)`이 400px을 강제한다)
3. **기대 결과**: `Color::GAINSBORO` 배경은 자식에 완전히 덮여 보이지 않는다

## 테스트 2: Auto 열과 Star 열의 너비 배분

1. 빨강 영역과 그 오른쪽 영역의 너비를 비교한다
2. **기대 결과**: Auto 열은 child1의 요청 너비인 200px가 된다
3. **기대 결과**: Star 열은 400 - 200 = 200px를 받아 `Color::GREEN` 중첩 그리드가 그 200px를 차지한다

## 테스트 3: 중첩 Star 열의 재분배

1. 중첩 그리드 안의 파랑/노랑 영역을 관찰한다
2. **기대 결과**: 두 `GridLength::Star(1)` 열이 200px를 반씩 나눠 `Color::BLUE`와 `Color::YELLOW`가 각각 100px 너비가 된다
3. **기대 결과**: 두 영역 모두 높이 200px로 루트 높이를 가득 채운다

## 통과 기준

- 루트 그리드의 최종 크기가 400x200이어야 한다
- Auto 열 200px, Star 열 200px로 나뉘어야 한다
- 중첩 그리드 안에서 `Color::BLUE`와 `Color::YELLOW`가 각각 100px 너비로 정확히 반씩 나눠 가져야 한다
- 창 크기를 바꿔도 위 수치가 변하지 않아야 한다
