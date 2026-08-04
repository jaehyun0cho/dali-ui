# FlexLayout: ROW_REVERSE / COLUMN_REVERSE

`FlexDirection::ROW_REVERSE`와 `FlexDirection::COLUMN_REVERSE`에서 주축 진행 방향과 `FlexJustify::FLEX_START` 기준점이 뒤집히는지 확인한다.

## 화면 구성

- 배경은 흰색(`Color::WHITE`)이다.
- 바깥 컨테이너는 `MATCH_PARENT x MATCH_PARENT` 세로 `StackLayout`이며 spacing 50px, padding `Extents(50, 50, 50, 50)`가 설정된다.
- 그 안에 `FlexLayout` 섹션 2개가 세로로 쌓인다. 두 섹션 모두 `StackLayoutParams::New().SetWeight(1.0f).SetAlignment(LayoutAlignment::FILL)`, `FlexJustify::FLEX_START`, `FlexAlign::CENTER`, padding `Extents(50, 50, 50, 50)`를 가진다.
- 섹션 1(`FlexDirection::ROW_REVERSE`, 배경 `Vector4(0.95, 0.95, 0.95, 1.0)`)의 자식은 추가 순서대로 빨강(`Color::RED`) 100x50, 초록(`Color::GREEN`) 100x100, 파랑(`Color::BLUE`) 100x200이다.
- 섹션 2(`FlexDirection::COLUMN_REVERSE`, 배경 `Vector4(0.9, 0.9, 0.9, 1.0)`)의 자식은 추가 순서대로 노랑(`Color::YELLOW`) 50x100, 시안(`Color::CYAN`) 100x100, 마젠타(`Color::MAGENTA`) 200x100이다.

## 테스트 1: ROW_REVERSE (섹션 1)

1. 목록에서 이 TC를 선택한다
2. **기대 결과**: 추가 순서와 반대로 오른쪽부터 빨강 → 초록 → 파랑 순으로 배치된다(가장 오른쪽이 빨강, 가장 왼쪽이 파랑)
3. **기대 결과**: `FLEX_START` 기준점이 오른쪽이 되어 세 박스가 padding 안쪽 오른쪽 끝에 서로 붙어 몰린다
4. **기대 결과**: 세 박스의 폭은 모두 100px이고 높이는 각각 50 / 100 / 200px이다
5. **기대 결과**: `FlexAlign::CENTER`에 의해 높이가 다른 세 박스의 세로 중심이 같은 선에 놓인다

## 테스트 2: COLUMN_REVERSE (섹션 2)

1. 아래 섹션을 관찰한다
2. **기대 결과**: 추가 순서와 반대로 아래쪽부터 노랑 → 시안 → 마젠타 순으로 쌓인다(가장 아래가 노랑, 가장 위가 마젠타)
3. **기대 결과**: `FLEX_START` 기준점이 아래쪽이 되어 세 박스가 padding 안쪽 아래 끝에 서로 붙어 몰린다
4. **기대 결과**: 세 박스의 높이는 모두 100px이고 폭은 각각 50 / 100 / 200px이다
5. **기대 결과**: `FlexAlign::CENTER`에 의해 폭이 다른 세 박스의 가로 중심이 같은 선에 놓인다

## 통과 기준

- 섹션 1의 좌우 순서가 추가 순서의 역순이어야 한다
- 섹션 2의 상하 순서가 추가 순서의 역순이어야 한다
- 두 섹션 모두 박스 사이에 빈 간격이 없어야 하며, 역방향 시작 지점(오른쪽 / 아래쪽) padding 50px에 붙어야 한다
- 두 섹션의 높이가 서로 같고 섹션 사이에 50px 간격이 있어야 한다
