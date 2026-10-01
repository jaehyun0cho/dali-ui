# LayoutTransition sample

This sample demonstrates the use of **LayoutTransition** to animate views
between layout-pass results: ENTER on add, EXIT on remove, and CHANGE on
bounds change.

## Examples

- **layout-transition-fade-slide.example**: spec 기반으로 카드 하나에
  fade와 아래쪽 80px 이동을 적용합니다. ENTER / EXIT 버튼과 Up / Down으로
  반복 실행하며, `SetSelfLayoutTransition()` 적용 예제도 포함합니다.
- **layout-transition-spec.example**: declarative spec mode. The
  framework drives interpolation; the application supplies a
  `ViewAnimationSpec` for ENTER / EXIT and a `LayoutTransitionTiming`
  for CHANGE.
- **layout-transition-animator.example**: animator-callback mode. The
  application owns the per-frame interpolation and writes properties
  every frame, mirroring the spec-mode height-expand + opacity-fade
  contract from a callback.
- **layout-transition-reorder.example**: edit-mode reorder sample. The
  dragged item is represented by an overlay proxy while the original item
  stays in the list at opacity 0, leaving a moving empty slot. Sibling
  reflow is animated by the CHANGE slot with the same timing as the other
  examples. The list is wrapped in a vertical ScrollView; in edit mode,
  dragging an item near the top / bottom edge of the viewport
  auto-scrolls the list in that direction so items beyond the visible
  area become reachable without releasing the drag.
  The card dropped into place is lifted above its siblings until its settle
  transition completes.
- **layout-transition-subtree.example**: reflow-scope sample. A single
  transition on the root container reflows the whole subtree under
  `LayoutReflowScope::SUBTREE` — a nested card with no transition of its
  own still has its inner items reflow. Toggle the scope back to
  `DIRECT_CHILDREN` to see the inner items snap while only the card
  animates.
- **layout-transition-self-override.example**: per-view override sample. One
  transition on the root governs all three items, but each item declares its
  own policy with `View::SetSelfLayoutTransition`: A inherits the root's
  0.25s timing, B overrides it with a slow 0.8s transition plus its own
  ENTER / EXIT effects, and C opts out entirely with
  `View::SetLayoutTransitionMode(LayoutTransitionMode::PASS_THROUGH)` —
  transitions pass through C, so it snaps on every layout change and is
  unparented instantly even though the root carries an EXIT effect. Toggling
  B's override off passes an uninitialized handle, which returns B to the
  root's rules rather than silencing it. Each item has its own add / remove
  button, so the three ENTER / EXIT policies can be compared side by side.
- **layout-transition-grid-reorder.example**: grid reorder sample. A
  white root holds a translucent rounded panel at (80, 80) with a
  right-aligned notification / edit row, a Wi-Fi / Bluetooth button row,
  and a scrollable 3-column `GridLayout` of SVG icons (each with its
  file-name label). Long-pressing a cell keeps the original in the grid as
  an invisible proxy while a non-interactive preview floats under the
  window; dragging moves the hidden original to the cell under the finger
  and reassigns every cell's `Row` / `Column`, so
  the CHANGE slot animates the reflow. Dragging near the top / bottom of
  the grid auto-scrolls. Because the cell captures the touch stream once a
  drag can start, free scrolling uses the empty margins between cells; the
  auto-scroll keeps off-screen cells reachable during a reorder.

## Controls

- **Tap "Click to ENTER"**: append a new colored child (ENTER expands
  height 0 → child height and fades opacity 0 → 1).
- **Tap "Click to EXIT"**: remove the last child (EXIT shrinks height
  to 0 and fades opacity to 0, then unparents).
- **Tap "Click to CHANGE"**: in the spec and animator examples, toggle
  every child's requested height between 80 and 160 (CHANGE slot
  animates width / height / position).
- **Tap "Click to Edit"**: in the reorder example, toggle edit mode. In
  edit mode, press and drag a child to move the empty slot through the
  sibling order; release to drop. Drag near the top or bottom of the
  ScrollView viewport to auto-scroll the list and continue reordering
  items beyond the visible area.
- **Tap "Toggle layout" / "Scope: ..."**: in the subtree example, toggle
  the nested sizes, or switch the root transition between `SUBTREE` and
  `DIRECT_CHILDREN`.
- **Long-press + drag a grid cell**: in the grid reorder example, hold a
  cell to pick it up, then drag to move it through the grid order; release
  to drop. Drag near the top or bottom of the grid to auto-scroll.
- **Up arrow**: same as the ENTER button (Toggle layout in the subtree
  example).
- **Down arrow**: same as the EXIT button (Toggle layout in the subtree
  example).
- **Esc / Back**: quit.

In the original spec and animator examples, the remaining children also
reflow on every add / remove; their position and size animate via the CHANGE
slot. Those two examples share a single 0.4s EASE_IN_OUT_SINE timing across
all three slots. The fade-slide example instead disables CHANGE and uses
0.3s EASE_IN_OUT for ENTER and EXIT.

In the animator sample the application owns the properties written from
callbacks. ENTER and EXIT both write opacity and height, while CHANGE
writes layout bounds only. If an animator transition is cancelled by a
successor slot, `OnFinished` is not emitted for the cancelled slot; any
non-layout properties written by the cancelled callback remain
application-owned and should be reset by the successor callback or by
lifecycle code when needed.

The Tizen package manifest launches the spec-mode sample entry.

## Build

### Ubuntu

Requires DALi environment to be set up first.

```bash
# From dali-ui root
cd samples/layout-transition
cmake -DCMAKE_INSTALL_PREFIX=$DESKTOP_PREFIX
make -j
```

Run:

```bash
./bin/layout-transition-fade-slide.example
./bin/layout-transition-spec.example
./bin/layout-transition-animator.example
./bin/layout-transition-reorder.example
./bin/layout-transition-subtree.example
./bin/layout-transition-self-override.example
./bin/layout-transition-grid-reorder.example
```

## Spec fade / slide sample

`layout-transition-fade-slide-example.cpp`의 `CreateFadeSlideTransition()`은
아래 효과를 하나의 spec 기반 transition으로 구성합니다.

| Slot | Opacity | Y position |
| --- | --- | --- |
| ENTER | 0 → 1 | 최종 Y + 80px → 최종 Y |
| EXIT | 1 → 0 | 최종 Y → 최종 Y + 80px |

두 효과는 모두 0.3초 `EASE_IN_OUT`입니다. opacity는 `ViewAnimationSpec`,
position은 `SlideFrom/SlideTo(BOTTOM, Pixel(80.0f), timing)`으로 선언합니다.
프레임별 animator callback은 사용하지 않습니다. `ClearChangeTiming()`은
기본 CHANGE를 끄며, ENTER/EXIT duration을 초기화하는 호출이 아닙니다.

`Create()`에서 카드에 `SetSelfLayoutTransition()`을 적용하고, `Enter()`에서
opacity를 0으로 설정한 뒤 host에 추가합니다. `Exit()`는
`Remove(card, RemovePolicy::ANIMATE_EXIT)`로 완료 후 제거를 요청합니다.
`OnTransitionFinished()`는 상태 표시만 갱신하며 보간을 수행하지 않습니다.

### 조작 및 수동 확인

1. 시작 화면의 host는 비어 있습니다. **ENTER** 버튼 또는 **Up**을 누르시면
   카드가 아래쪽 80px 지점에서 올라오면서 나타납니다.
2. `Present: press EXIT`가 표시된 뒤 **EXIT** 버튼 또는 **Down**을 누르시면
   카드가 80px 내려가며 사라지고, 완료 후 host에서 제거됩니다.
3. 전환 중 ENTER/EXIT를 빠르게 반복해서 누르셔도 추가 요청은 무시됩니다.
   상태는 `ABSENT → ENTERING → PRESENT → EXITING → ABSENT` 순서입니다.
4. EXIT 완료 후 ENTER를 다시 눌러 반복하실 수 있습니다. EXIT 중인 카드를
   같은 host에 다시 추가하지 않습니다.
5. 창 크기를 바꾸면서 ENTER/EXIT를 반복하여 상태가 계속 복귀하는지
   확인하실 수 있습니다. root·host·card와 상태 표시 영역은 고정 크기를
   사용하며, 360×400 이상의 창에서 전체 내용을 보실 수 있습니다.
6. **Esc / Back**으로 종료합니다.

`SetEnterOnInitialMount(true)`를 사용하므로 첫 layout 이전에 ENTER 입력을
받아도 초기 ENTER가 생략되지 않습니다. controller 소멸 시에는 member
lifecycle callback을 해제합니다. 기존 Tizen package manifest의 기본 실행
대상은 변경하지 않으며, 위의 새 executable을 직접 실행하시면 됩니다.
