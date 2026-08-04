# FlexLayout: FlexShrink

항목의 크기 합이 컨테이너보다 클 때 `SetFlexShrink` 값에 따라 축소량이 어떻게 나뉘는지 확인한다.

## 화면 구성

- 배경은 흰색(`Color::WHITE`)이다.
- 바깥 컨테이너는 `MATCH_PARENT x MATCH_PARENT` 세로 `StackLayout`이며 spacing 16px, padding `Extents(50, 50, 50, 50)`가 설정된다.
- 그 안에 `FlexLayout` 행 2개가 세로로 쌓인다. 두 행 모두 `StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL)`, `FlexDirection::ROW`, `FlexWrap::NO_WRAP`, `FlexAlign::STRETCH`, padding `Extents(50, 50, 50, 50)`를 가진다.
- 모든 박스는 `SetFlexBasis(250.0f)`를 가진다. 요청 높이는 100px이지만 두 행 모두 `FlexAlign::STRETCH`이므로 실제 렌더링 높이는 행 안쪽 높이만큼 세로로 늘어난다.
  - 행 1(배경 `Vector4(0.95, 0.95, 0.95, 1.0)`): 빨강(`Color::RED`), 초록(`Color::GREEN`), 파랑(`Color::BLUE`) 모두 shrink 기본값(1.0)
  - 행 2(배경 `Vector4(0.9, 0.9, 0.9, 1.0)`): 노랑(`Color::YELLOW`) `SetFlexShrink(0.0f)`, 시안(`Color::CYAN`) 기본값(1.0), 마젠타(`Color::MAGENTA`) `SetFlexShrink(3.0f)`
- 한 행의 안쪽 폭은 `콘텐츠 영역 폭 - 200`(바깥 padding 100 + 행 padding 100)이다. 이 값이 basis 합 750px보다 작을 때 축소가 발생하므로, 축소를 관찰하려면 창 폭을 950px 미만으로 줄인다. 행 2의 3배 비율 검증은 마젠타 폭이 0으로 클램프되지 않는 창 폭 약 620px 이상에서만 유효하다.

## 테스트 1: 기본 shrink(행 1)

1. 창 폭을 950px 미만으로 맞춘 뒤 행 1을 관찰한다
2. **기대 결과**: 세 박스가 줄바꿈 없이 한 줄에 남고, 행의 안쪽 폭을 넘지 않는다
3. **기대 결과**: 빨강, 초록, 파랑의 폭이 서로 같다(같은 basis, 같은 shrink 이므로 균등 축소)

## 테스트 2: shrink 0 / 1 / 3 비교(행 2)

1. 같은 상태에서 행 2를 관찰한다
2. **기대 결과**: 노랑은 `shrink 0`이므로 basis 그대로 250px를 유지한다
3. **기대 결과**: 시안과 마젠타만 축소되며, 마젠타의 축소량이 시안 축소량의 3배다
4. **기대 결과**: 결과적으로 폭은 노랑 > 시안 > 마젠타 순이다

## 통과 기준

- 두 행 모두 `NO_WRAP`이므로 줄바꿈이 발생하지 않아야 한다
- 행 1의 세 박스 폭이 서로 같아야 한다
- 행 2의 노랑 폭이 250px로 유지되어야 한다
- 행 2에서 `250 - 마젠타 폭`이 `250 - 시안 폭`의 3배여야 한다(창 폭 약 620~940px 범위에서 관찰)
- 창 폭이 충분히 넓어 축소가 필요 없을 때는 모든 박스가 250px여야 한다
