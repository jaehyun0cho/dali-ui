# AbsoluteLayout: WrapContent MinSize + Proportional Child

최종 크기가 minWidth로 결정되는 `WRAP_CONTENT` 부모에 대해 비례 크기 자식이
올바르게 계산되는지 확인한다.

## 화면 구성

- 배경: 흰색(`Color::WHITE`)
- 루트: `AbsoluteLayout`, 배경 `Color::GAINSBORO`, `SetMinimumWidth(400)`, 높이 200px 고정
  (너비는 `WRAP_CONTENT`이지만 자식 폭 합이 200px뿐이므로 minWidth 400px가 최종 너비가 된다)
- 루트는 콘텐츠 영역 좌상단에 배치된다
- child1 빨간 박스(`Color::RED`): `LayoutRect(0, 0, 200, 200)` — 고정 200x200
- child2 초록 `AbsoluteLayout`(`Color::GREEN`): `LayoutRect(200, 0, 0.5, 1.0)` + `SIZE_PROPORTIONAL`
  — x=200, 너비는 루트의 50% = 200px, 높이는 루트의 100% = 200px
  - grandChild1 파란 박스(`Color::BLUE`): `LayoutRect(0, 0, 50, 100)` — 고정 50x100
  - grandChild2 노란 박스(`Color::YELLOW`): `LayoutRect(50, 0, 0.5, 1.0)` + `SIZE_PROPORTIONAL`
    — x=50, 너비는 child2의 50% = 100px, 높이는 child2의 100% = 200px

```
  +----------------------+----------------------+
  |                      | [파랑] [  노랑  ]    |
  |  빨강 200x200        |  50     100          |
  |                      |  (초록 200x200)      |
  +----------------------+----------------------+
   <-------- 루트 400x200 (minWidth로 결정) ---->
```

## 테스트 1: 초기 레이아웃

1. 목록에서 본 TC에 진입한다
2. **기대 결과**: 연회색 루트가 좌상단에 400x200 크기로 표시된다
3. **기대 결과**: 빨간 박스가 (0, 0)에 200x200으로 표시되어 루트 왼쪽 절반을 채운다
4. **기대 결과**: 초록 영역이 x=200에서 시작해 200x200 크기로 루트 오른쪽 절반을 채운다
5. **기대 결과**: 초록 영역 안에서 파란 박스가 (0, 0)에 50x100으로 표시된다
6. **기대 결과**: 초록 영역 안에서 노란 박스가 x=50에서 100x200 크기로 표시된다
7. **기대 결과**: 초록 영역의 오른쪽 끝 50px 구간은 초록색 그대로 남는다

## 통과 기준

- `WRAP_CONTENT` 너비가 minWidth 400px로 결정되어야 한다
- child2의 비례 너비는 자식 폭 합(200px)이 아니라 확정된 부모 너비 400px 기준으로 200px가 되어야 한다
- grandChild2의 비례 너비는 child2의 확정 너비 200px 기준으로 100px가 되어야 한다
- 목록으로 나갔다가 다시 진입해도 동일한 화면이 표시되어야 한다
