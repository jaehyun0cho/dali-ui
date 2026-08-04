# FlexLayout: JustifyContent Modes

`FlexJustify::FLEX_END`, `CENTER`, `SPACE_AROUND`, `SPACE_EVENLY` 네 가지 주축 분배 모드의 차이를 한 화면에서 비교한다.

## 화면 구성

- 배경은 흰색(`Color::WHITE`)이다.
- 바깥 컨테이너는 `MATCH_PARENT x MATCH_PARENT` 세로 `StackLayout`이며 spacing 50px, padding `Extents(50, 50, 50, 50)`가 설정된다.
- 그 안에 `FlexLayout` 행 4개가 세로로 쌓인다. 각 행은 `StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL)`을 사용하므로 남은 세로 공간을 1:1:1:1로 나눠 가지며 가로는 가득 채운다.
- 각 행은 `FlexDirection::ROW`, `FlexAlign::CENTER`이고 50x50 박스 3개를 담는다.
  - 행 1: `FlexJustify::FLEX_END`, 배경 `Vector4(0.95, 0.95, 0.95, 1.0)`, 박스는 빨강(`Color::RED`)/초록(`Color::GREEN`)/파랑(`Color::BLUE`)
  - 행 2: `FlexJustify::CENTER`, 배경 `Vector4(0.9, 0.9, 0.9, 1.0)`, 박스는 노랑(`Color::YELLOW`)/시안(`Color::CYAN`)/마젠타(`Color::MAGENTA`)
  - 행 3: `FlexJustify::SPACE_AROUND`, 배경 `Vector4(0.95, 0.95, 0.95, 1.0)`, 박스는 빨강/초록/파랑
  - 행 4: `FlexJustify::SPACE_EVENLY`, 배경 `Vector4(0.9, 0.9, 0.9, 1.0)`, 박스는 노랑/시안/마젠타

## 테스트 1: 행별 주축 분배

1. 목록에서 이 TC를 선택한다
2. **기대 결과**: 행 1은 세 박스가 서로 붙은 채로 행의 오른쪽 끝에 몰려 있다
3. **기대 결과**: 행 2는 세 박스가 서로 붙은 채로 행의 가로 중앙에 놓인다
4. **기대 결과**: 행 3은 각 박스의 양옆에 같은 여백이 배정되어, 양 끝 여백이 박스 사이 여백의 절반이다
5. **기대 결과**: 행 4는 양 끝 여백과 박스 사이 여백이 모두 같다

## 테스트 2: 행 크기와 교차축 정렬

1. 네 행의 배경 영역과 박스의 세로 위치를 관찰한다
2. **기대 결과**: 네 행의 높이가 서로 같고, 행 사이에 50px 간격이 보인다
3. **기대 결과**: 각 행의 50x50 박스가 행 높이의 세로 중앙에 놓인다(`FlexAlign::CENTER`)
4. **기대 결과**: 행 배경이 밝은 회색과 조금 더 진한 회색으로 번갈아 나타난다

## 통과 기준

- 박스 크기는 모든 행에서 50x50으로 동일해야 한다
- 네 행의 박스 분포가 각각 FLEX_END / CENTER / SPACE_AROUND / SPACE_EVENLY로 서로 구분되어야 한다
- 행 3의 양 끝 여백이 박스 사이 여백의 정확히 절반이어야 한다
- 행 4의 여백 4개(양 끝 2개 + 사이 2개)가 모두 같아야 한다
- 창 크기를 바꿔도 각 모드의 분배 규칙이 유지되어야 한다
