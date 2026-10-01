# StackLayout sample

This sample demonstrates the use of **StackLayout** to arrange child views in a single column (vertical stack).

## Features

- **Root**: A vertical `StackLayout` that fills the window, with padding and spacing between children.
- **Top bar**: Fixed-height red view (100 px).
- **Middle**: Green view with layout weight 1, taking the remaining space.
- **Bottom bar**: Fixed-height blue view (100 px).

## Build

### Ubuntu

Requires DALi environment to be set up first.

```bash
# From dali-ui root
cd samples/stacklayout
cmake -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX
make -j
```

Run:

```bash
./bin/stacklayout.example
```

**Cross-axis alignment sample** (StackLayout 자식의 cross-axis alignment 테스트):

```bash
./bin/stacklayout-alignment.example
```

- 세로 스택에서 좁은 박스 4개: Start(왼쪽), Center(가운데), End(오른쪽), Fill(전체 너비).

**Margin, padding, nested StackLayout sample** (사용성: margin / padding / StackLayout in StackLayout):

```bash
./bin/stacklayout-margin-padding-nested.example
```

- **Padding**: 루트 스택에 `SetPadding`으로 창 가장자리와의 여백.
- **Margin**: 자식 View에 `SetMargin`으로 서로 다른 여백(좌우/하단) 적용.
- **Nested**: 세로 스택 안에 가로 `StackLayout` 한 행(여러 박스)을 자식으로 추가.

### GBS build (Tizen)

```bash
# From dali-ui root
gbs build -A armv7l --include-all --packaging-dir samples/stacklayout/packaging
```

Output: `com.samsung.dali.stacklayout-2.0.0-1.armv7l.rpm`

## Controls

- **Escape** or **Back**: Quit the application.

## LayoutDirectionChangedSignal sample

```bash
# samples/stacklayout에서 실행
./bin/stacklayout-layout-direction-signal.example
```

`stacklayout-layout-direction-signal-example.cpp`는 부모의 방향 변경이
단일 자식의 `LayoutDirectionChangedSignal()`로 전달되는 모습을 보여드립니다.
자식은 `LayoutDirection::INHERIT`와 `LayoutMode::STANDALONE`을 사용합니다.

`ObserveDirection()` 안의 `Connect(+[](Actor, LayoutDirection::Type) {...})`는
캡처 없는 lambda를 함수 포인터로 연결합니다. callback은
`UpdateObservedPosition()`에서 `SetRequestedX()`와 `SetRequestedY()`를
호출하고, Label과 stdout에 effective direction, signal 횟수,
requested X/Y를 표시합니다. 좌표는 다음 layout pass에서 배치에 반영됩니다.

- **LTR / RTL**: 관찰 parent에 해당 방향을 명시적으로 설정합니다.
- **INHERIT**: parent가 다시 조상의 방향을 상속하도록 합니다.
- **Inherited child**: parent의 유일한 관찰 자식입니다.
- **Esc / Back**: 종료합니다.

`Policy`는 `GetLayoutDirection()`, `Effective`는
`GetEffectiveLayoutDirection()` 결과입니다. 초기 상태는 조회한 방향으로
좌표를 설정하며, `Signals`는 0부터 시작합니다. RTL 시스템에서 시작해도
초기 좌표에 RTL 계산을 적용합니다.

### 좌표 계산

parent와 child의 requested width는 각각 368, 166으로 고정되어 있습니다.
초기 Arrange 전에도 사용할 수 있도록 `GetRequestedWidth()`로 너비를 읽습니다.

- LTR: `x = 50`, `y = 12`.
- RTL: `x = parentWidth - childWidth - 50 = 152`, `y = 12`.
- RTL에서 자식의 오른쪽 끝은 `x + childWidth = 318 = parentWidth - 50`입니다.

`STANDALONE` 자식은 부모의 padding 및 자동 RTL 위치 반전에서 제외됩니다.
자식 자체의 margin은 기본값 0을 유지하므로 위 좌표는 부모의 왼쪽·위쪽
경계를 기준으로 합니다. `SetRequestedY(12)`로 기존 위쪽 간격을 유지합니다.

### 수동 확인

1. LTR를 누르시면 자식의 requested 좌표가 `(50, 12)`이고,
   다음 layout pass에서 부모 왼쪽으로부터 50 떨어져 배치됩니다.
2. RTL를 누르시면 effective direction과 callback 횟수가 바뀌고,
   requested 좌표가 `(152, 12)`가 됩니다. 자식의 오른쪽 끝과
   부모 오른쪽 경계 사이의 간격은 50입니다.
3. RTL를 다시 누르시면 실제 방향 변화가 없어 횟수와 좌표가 유지됩니다.
4. 다시 LTR를 누르시면 `(50, 12)`로 돌아갑니다.
5. INHERIT를 누르시면 parent policy가 INHERIT로 돌아갑니다. effective
   direction이 같다면 policy 변경만으로 callback이 발생하지 않습니다.
6. 플랫폼이 실행 중인 앱에 system locale 변경을 전달하는 환경에서는
   INHERIT 상태에서 LTR 언어와 RTL 언어 사이를 전환하여 자동 상속 및
   위치 변경을 관찰하실 수 있습니다. outer와 parent는 상속 체인을 유지하며,
   sample 자체는 OS 설정을 변경하지 않습니다. LTR에서 다른 LTR 언어로
   변경하는 경우에는 방향 변경 신호가 발생하지 않습니다.

400×500 이상의 창 크기에서 전체 설명과 버튼을 보실 수 있습니다.
