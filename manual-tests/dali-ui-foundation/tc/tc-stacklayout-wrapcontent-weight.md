# StackLayout: MinWidth + MatchParent Weight

최소 폭(`SetMinimumWidth`)으로 크기가 결정된 가로 `StackLayout` 안에서 weight가 주축 폭을, `MATCH_PARENT`가 cross-axis 높이를 채우는지 확인한다.

> 원본 샘플 파일 이름은 `stacklayout-wrapcontent-weight-example.cpp`지만, 실제 코드는 `SetMinimumWidth(400)` + `MATCH_PARENT` 조합을 보여준다. 파일 이름은 추적을 위해 그대로 두고 이 문서는 코드가 실제로 하는 동작을 기준으로 작성한다.

## 화면 구성

- 배경은 흰색이고, 루트는 `StackOrientation::HORIZONTAL` `StackLayout`으로 콘텐츠 영역 좌상단에 놓인다
- 루트 배경은 `Color::GAINSBORO`, 최소 폭은 `400px`, 요청 높이는 `200px` → 실제 크기 `400x200`
- 자식 1: 빨간색(`Color::RED`), 폭 `200px` 고정, 높이 `MATCH_PARENT`
- 자식 2: 초록색(`Color::GREEN`) 가로 `StackLayout`, weight `1.0`, 높이 `MATCH_PARENT` → 폭 `200px`
  - 손자 1: 파란색(`Color::BLUE`), weight `1.0`, 높이 `MATCH_PARENT` → 폭 `100px`
  - 손자 2: 노란색(`Color::YELLOW`), weight `1.0`, 높이 `MATCH_PARENT` → 폭 `100px`

```
  |<--------------- 400px --------------->|
  +-------------------+---------+---------+
  |       RED         |  BLUE   | YELLOW  |  200px
  |      200px        |  100px  |  100px  |
  +-------------------+---------+---------+
```

## 테스트 1: 초기 레이아웃

1. 목록에서 이 TC를 선택해 화면을 관찰한다
2. **기대 결과**: 콘텐츠 영역 좌상단에 `400x200` 크기의 블록이 놓이고, 그 바깥은 흰색이다
3. **기대 결과**: 왼쪽 `200px`가 빨강이고, 오른쪽 `200px`가 파랑 `100px` + 노랑 `100px`로 채워진다
4. **기대 결과**: 회색(`Color::GAINSBORO`) 루트 배경이 보이는 빈 영역은 없다

## 테스트 2: MATCH_PARENT 높이

1. 빨강 / 파랑 / 노랑의 세로 크기를 관찰한다
2. **기대 결과**: 세 색 모두 위아래로 잘리거나 남는 부분 없이 `200px` 높이를 가득 채운다

## 테스트 3: 재진입

1. `< Back`으로 목록에 나갔다가 다시 이 TC로 들어온다
2. **기대 결과**: 첫 진입과 완전히 동일한 화면이 표시된다

## 통과 기준

- 루트 폭이 `MinimumWidth`인 `400px`로 결정된다
- 빨강의 폭은 `200px`이고, weight `1.0` 자식이 남은 `200px`를 모두 가져간다
- 파랑과 노랑의 폭이 서로 같다(각 `100px`)
- 세 색 모두 높이가 `200px`로 동일하다
