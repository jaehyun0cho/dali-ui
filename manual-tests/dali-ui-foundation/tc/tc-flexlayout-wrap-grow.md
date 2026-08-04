# FlexLayout: Wrap + FlexBasis / FlexGrow

`FlexWrap::WRAP` 상태에서 `SetFlexBasis`가 기본 크기를, `SetFlexGrow`가 잔여 공간 분배 비율을 결정하는지 확인한다.

## 화면 구성

- 배경은 흰색(`Color::WHITE`)이다.
- 바깥 컨테이너는 `MATCH_PARENT x MATCH_PARENT` 세로 `StackLayout`이며 padding `Extents(50, 50, 50, 50)`가 설정된다(spacing 없음).
- 그 안에 `FlexLayout` 섹션 2개가 세로로 쌓인다. 각 섹션은 `StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL)`, `FlexDirection::ROW`, `FlexWrap::WRAP`, `FlexJustify::FLEX_START`, `FlexAlign::CENTER`(AlignContent)로 설정된다.
- 모든 박스의 높이는 100px이며 폭은 basis와 grow로 결정된다.
  - 섹션 1: 빨강(`Color::RED`) basis 50 / grow 1, 초록(`Color::GREEN`) basis 50 / grow 1, 파랑(`Color::BLUE`) basis 100 / grow 2
  - 섹션 2: 노랑(`Color::YELLOW`) basis 100 / grow 1, 시안(`Color::CYAN`) basis 100 / grow 1, 마젠타(`Color::MAGENTA`) basis 200 / grow 2

## 테스트 1: FlexGrow 잔여 공간 분배

1. 목록에서 이 TC를 선택한다
2. **기대 결과**: 섹션 1의 세 박스가 한 줄에 왼쪽부터 빨강 → 초록 → 파랑 순으로 배치되고 줄 전체를 빈틈없이 채운다
3. **기대 결과**: 섹션 1에서 빨강과 초록의 폭이 서로 같고, 파랑이 두 배의 잔여 공간을 받아 가장 넓다
4. **기대 결과**: 섹션 2의 세 박스도 한 줄을 빈틈없이 채우고, 노랑과 시안의 폭이 같으며 마젠타가 가장 넓다
5. **기대 결과**: 모든 박스의 높이가 100px이다

## 테스트 2: FlexWrap 줄바꿈

1. 창의 가로 크기를 줄여 한 줄의 폭이 basis 합보다 작아지도록 만든다(섹션 1은 basis 합 200px, 섹션 2는 400px 기준)
2. **기대 결과**: 폭이 부족한 섹션의 박스가 다음 줄로 넘어간다
3. **기대 결과**: 줄바꿈이 일어난 섹션에서 `FlexAlign::CENTER`(AlignContent)에 의해 줄 묶음 전체가 섹션 높이의 세로 중앙에 놓인다
4. **기대 결과**: 각 줄 안에서는 그 줄에 남은 공간이 다시 grow 비율대로 분배된다

## 통과 기준

- 각 줄에 남는 빈 공간 없이 박스들이 줄 폭을 가득 채워야 한다
- grow 2인 박스가 받는 추가 폭이 grow 1인 박스가 받는 추가 폭의 2배여야 한다
- 박스 높이가 항상 100px로 유지되어야 한다
- 줄바꿈이 발생해도 박스의 추가 순서(빨강 → 초록 → 파랑, 노랑 → 시안 → 마젠타)가 유지되어야 한다
