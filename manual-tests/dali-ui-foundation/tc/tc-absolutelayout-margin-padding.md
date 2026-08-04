# AbsoluteLayout: Margin / Padding

`AbsoluteLayout` 컨테이너의 padding, 자식별 margin, 그리고 중첩 `AbsoluteLayout`의
padding이 각각 올바르게 적용되는지 확인한다.

> 상단 헤더(60px)만큼 세로 공간이 줄어들므로 최하단 요소는 창 크기에 따라 일부만 보일 수 있다.

## 화면 구성

- 배경: 흰색(`Color::WHITE`)
- 루트: `AbsoluteLayout`, 너비/높이 모두 `MATCH_PARENT`, padding `Extents(50, 50, 50, 50)`
- 빨간 박스(`Color::RED`): margin 없음, `SetWidth(100).SetHeight(100)` — x/y 기본값 0이므로 padding 경계에 붙음
- 초록 박스(`Color::GREEN`): margin `Extents(50, 50, 50, 50)`, `SetY(100).SetWidth(100).SetHeight(50)`
- 파란 박스(`Color::BLUE`): margin `Extents(50, 50, 50, 50)`, `SetX(100).SetY(200).SetWidth(100).SetHeight(50)`
- 중첩 회색 `AbsoluteLayout`(`Color::GRAY`): padding 50px, margin 50px, `SetY(300).SetWidth(200).SetHeight(200)`
  - 마젠타 박스(`Color::MAGENTA`): `LayoutRect(0, 0, 1.0, 1.0)` + `ALL` — 회색 컨테이너의 padding 안쪽 영역을 채움
  - 노란 박스(`Color::YELLOW`): margin 50px, `SetX(50).SetWidth(50).SetHeight(50)`
  - 시안 박스(`Color::CYAN`): margin 50px, `SetY(50).SetWidth(50).SetHeight(50)`

## 테스트 1: 컨테이너 padding

1. 목록에서 본 TC에 진입한다
2. **기대 결과**: 빨간 박스가 콘텐츠 영역 가장자리가 아니라 padding 50px만큼 안쪽인 (50, 50)에 100x100으로 표시된다
3. **기대 결과**: 어떤 자식도 padding 영역(가장자리 50px) 안으로 들어가지 않는다

## 테스트 2: 자식 margin

1. 빨강(margin 없음)과 초록/파랑(margin 50px)의 위치를 비교한다
2. **기대 결과**: 초록 박스는 지정한 y=100 위치에서 margin 50px만큼 더 밀려 배치된다
3. **기대 결과**: 파란 박스는 지정한 (100, 200) 위치에서 margin 50px만큼 더 밀려 배치된다
4. **기대 결과**: 빨간 박스만 padding 경계에 정확히 붙어 있다

## 테스트 3: 중첩 AbsoluteLayout

1. 화면 아래쪽의 회색 컨테이너를 관찰한다
2. **기대 결과**: 회색 컨테이너는 200x200 크기이며 margin 50px만큼 밀려 배치된다
3. **기대 결과**: 마젠타 박스가 회색 컨테이너 내부에서 padding 50px를 제외한 영역을 채우고, 회색 테두리가 사방 50px 남는다
4. **기대 결과**: 노란 박스와 시안 박스는 각자의 margin 50px가 추가로 적용된 위치에 50x50 크기로 표시된다

## 통과 기준

- 컨테이너 padding은 모든 자식의 기준 원점을 안쪽으로 이동시켜야 한다
- 자식 margin은 해당 자식에게만 적용되어야 한다
- 중첩된 `AbsoluteLayout`의 padding은 그 자식에게만 적용되고 루트 padding과 독립적이어야 한다
- 목록으로 나갔다가 다시 진입해도 동일한 화면이 표시되어야 한다
