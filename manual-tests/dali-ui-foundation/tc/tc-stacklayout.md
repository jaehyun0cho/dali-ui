# StackLayout: Vertical Weight

세로 `StackLayout`에서 고정 높이 자식과 weight 자식이 주축(세로) 공간을 나누어 갖는지 확인한다.

## 화면 구성

- 배경은 흰색이고, 루트는 `StackOrientation::VERTICAL` `StackLayout`으로 콘텐츠 영역을 가득 채운다 (`MATCH_PARENT` x `MATCH_PARENT`)
- 루트 padding은 `Extents(50, 50, 50, 50)`, 자식 사이 spacing은 `10px`
- 상단 바: 빨간색(`Color::RED`), 높이 `100px` 고정, `LayoutAlignment::FILL`
- 중간: 초록색(`Color::GREEN`), `StackLayoutParams` weight `1.0`, `LayoutAlignment::FILL`
- 하단 바: 파란색(`Color::BLUE`), 높이 `100px` 고정, `LayoutAlignment::FILL`

```
  [ RED   : 100px 고정   ]
  [ GREEN : weight 1.0   ]
  [ BLUE  : 100px 고정   ]
```

## 테스트 1: 초기 레이아웃

1. 목록에서 이 TC를 선택해 화면을 관찰한다
2. **기대 결과**: 위에서 아래로 빨강 / 초록 / 파랑 순서로 세 개의 바가 놓인다
3. **기대 결과**: 세 바 모두 콘텐츠 영역의 좌우 가장자리에서 `50px` 떨어져 있고, 빨강 위쪽과 파랑 아래쪽에도 `50px` 여백이 있다
4. **기대 결과**: 빨강과 파랑의 높이는 각각 `100px`이고, 초록이 남은 세로 공간을 모두 차지한다

## 테스트 2: 자식 사이 간격

1. 빨강-초록 경계와 초록-파랑 경계를 관찰한다
2. **기대 결과**: 두 경계마다 `10px`의 흰색 간격이 보인다

## 테스트 3: 재진입

1. `< Back`으로 목록에 나갔다가 다시 이 TC로 들어온다
2. **기대 결과**: 첫 진입과 완전히 동일한 화면이 표시된다

## 통과 기준

- 빨강 바와 파랑 바의 높이가 항상 `100px`로 유지된다
- 초록 영역의 높이는 (콘텐츠 영역 높이 - padding `100px` - spacing `20px` - `200px`)와 같다
- 세 자식 모두 `LayoutAlignment::FILL`이므로 좌우 폭이 서로 동일하다
- 배경이 회색이 아니라 흰색으로 보인다
