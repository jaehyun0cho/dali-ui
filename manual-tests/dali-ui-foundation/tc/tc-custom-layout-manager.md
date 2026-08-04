# CustomLayoutManager: Diagonal (LayoutManager Subclass)

`LayoutManager`를 직접 상속한 레이아웃 매니저를 `View::AttachLayoutManager`로 붙였을 때, 콜백 방식과 동일한 대각선 배치가 나오는지 확인한다.

## 화면 구성

- 배경은 흰색이다
- 루트는 평범한 `View`이며, 여기에 `Dali::MakeUnique<DiagonalLayoutManager>()`가 `AttachLayoutManager`로 부착된다
- 부착 이후 루트는 모든 레이아웃 패스의 `Measure` / `Arrange`를 이 매니저에 위임한다
- 매니저는 `GetChildViewCount` / `GetChildViewAt`로 자식을 순회하고, `IsStandalone`인 자식은 건너뛴다
- 자식 3개가 추가 순서대로 배치된다
  - 자식 1: 50x50, 빨강 `Vector4(0.9, 0.2, 0.2, 1.0)`
  - 자식 2: 100x100, 초록 `Vector4(0.2, 0.7, 0.2, 1.0)`
  - 자식 3: 200x200, 파랑 `Vector4(0.2, 0.3, 0.9, 1.0)`
- 매니저의 `Measure`는 자식들의 폭 합과 높이 합(350x350)을 반환한다

```
  +--+
  |50|
  +--+----+
     |100 |
     |    |
     +----+--------+
          |  200   |
          |        |
          |        |
          +--------+
```

## 테스트 1: 초기 레이아웃

1. 목록에서 이 TC를 선택해 진입한다
2. **기대 결과**: 각 박스의 좌상단 모서리가 이전 박스의 우하단 모서리에 정확히 닿는 대각선 배치가 나타난다
3. **기대 결과**: 빨강 50x50은 (0, 0), 초록 100x100은 (50, 50), 파랑 200x200은 (150, 150)에 그려진다

## 테스트 2: 콜백 방식과의 동등성

1. 목록으로 돌아가 `CustomLayout: Diagonal (Measure/Arrange Callback)` TC에 진입해 화면을 비교한다
2. **기대 결과**: 두 TC의 박스 위치와 크기가 동일하다
3. **기대 결과**: 배치를 얻는 수단만 다를 뿐(콜백 vs `LayoutManager` 서브클래스) 화면에는 차이가 없다

## 통과 기준

- 세 박스의 좌상단 좌표가 각각 (0, 0), (50, 50), (150, 150)이어야 한다
- 결과 배치가 `CustomLayout: Diagonal (Measure/Arrange Callback)`과 픽셀 단위로 동일해야 한다
- 목록으로 나갔다가 다시 진입해도 동일한 배치가 나타나야 한다 (진입할 때마다 `View`와 매니저가 새로 생성된다)
