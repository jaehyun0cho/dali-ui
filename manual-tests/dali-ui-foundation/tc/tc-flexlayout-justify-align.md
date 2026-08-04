# FlexLayout: SpaceBetween + AlignSelf

`FlexJustify::SPACE_BETWEEN`의 주축 간격 분배와 자식별 `SetAlignSelf` 교차축 정렬 오버라이드를 확인한다.

## 화면 구성

- 배경은 흰색(`Color::WHITE`)이다.
- 루트는 `MATCH_PARENT x MATCH_PARENT` 크기의 `FlexLayout`이며 `FlexDirection::ROW`, `FlexJustify::SPACE_BETWEEN`, `FlexAlign::CENTER`, padding `Extents(50, 50, 50, 50)`가 설정된다.
- 자식은 모두 50x200 크기이고, 추가 순서와 교차축 정렬은 다음과 같다.
  - 1번 빨강(`Color::RED`): `AlignSelf` 없음 → 컨테이너 기본값 `FlexAlign::CENTER`
  - 2번 초록(`Color::GREEN`): `FlexAlign::FLEX_START`
  - 3번 파랑(`Color::BLUE`): `FlexAlign::CENTER`
  - 4번 노랑(`Color::YELLOW`): `FlexAlign::FLEX_END`
  - 5번 시안(`Color::CYAN`): `FlexAlign::BASELINE`
  - 6번 마젠타(`Color::MAGENTA`): `AlignSelf` 없음 → 컨테이너 기본값 `FlexAlign::CENTER`

## 테스트 1: SPACE_BETWEEN 주축 분배

1. 목록에서 이 TC를 선택한다
2. **기대 결과**: 6개 박스가 왼쪽부터 빨강 → 초록 → 파랑 → 노랑 → 시안 → 마젠타 순으로 배치된다
3. **기대 결과**: 첫 박스(빨강)의 왼쪽 변이 padding 안쪽 시작선에 닿고, 마지막 박스(마젠타)의 오른쪽 변이 padding 안쪽 끝선에 닿는다
4. **기대 결과**: 박스 사이의 다섯 간격이 모두 같은 크기다

## 테스트 2: AlignSelf 교차축 정렬

1. 각 박스의 세로 위치를 관찰한다
2. **기대 결과**: 초록(`FLEX_START`)은 padding 안쪽 영역의 위쪽 끝에 붙는다
3. **기대 결과**: 빨강, 파랑, 마젠타(`CENTER`)는 세로 중앙에 놓인다
4. **기대 결과**: 노랑(`FLEX_END`)은 padding 안쪽 영역의 아래쪽 끝에 붙는다
5. **기대 결과**: 시안(`BASELINE`)은 교차축 오프셋이 적용되지 않아 초록과 같은 위쪽 끝 위치에 놓인다

## 통과 기준

- 여섯 박스의 크기가 모두 50x200으로 유지되어야 한다
- 박스 사이의 다섯 간격이 서로 같아야 하며, 컨테이너 양 끝에는 padding 50px 외의 추가 간격이 없어야 한다
- `AlignSelf`를 지정한 박스는 컨테이너의 `FlexAlign::CENTER`가 아니라 각자의 값에 따라 정렬되어야 한다
- 창 가로 크기를 바꾸면 박스 사이 간격만 변하고 박스 크기는 변하지 않아야 한다
