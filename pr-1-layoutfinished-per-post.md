### Summary
- `LayoutFinished`(View, Window)를 window layout이 settle될 때가 아니라 완료된 layout pass마다 전달하도록 계약을 바꿉니다.
- 기존에는 park된 작업도 pending으로 취급되어, 그것이 drain될 때까지 신호가 보류되었고 정지 상태 앱에서는 무기한 늦어졌습니다.
- 이제 pending 작업은 완료된 pass의 통지를 억제하거나 늦추지 않으며, park된 작업은 그것을 처리한 pass가 다시 통지합니다.
- slot 안의 무효화는 여전히 wake 없이 park되지만, 전달 중인 통지와 Window 신호를 취소하지 않습니다.
- 전달 시점은 core Relayout 뒤 post-process 단계로 유지되며, 한 번의 전달에서 View는 최신 bounds로 한 번씩, Window는 마지막에 한 번 통지됩니다.
- 수동 `ProcessLayouts()`는 호출 안에서 emit하지 않고, 그 pass는 다음 자동 pre-process 뒤의 post-process가 전달합니다.
- 자동 pre-process 이후(slot 등)에 완료된 pass는 cycle을 요청하지 않으므로 독립적으로 발생한 다음 cycle에서 전달됩니다.
- SVG 같은 fitting-mode visual은 Arrange 시점이 아니라 `LayoutFinished` 전달 시 최신 arrange 대상 크기로 fitting됩니다.
- 그래서 controller pass를 만들지 않는 수동 `View::Arrange()`는 이제 SVG visual을 fitting하지 않습니다.
- 이 PR은 다른 PR에 의존하지 않습니다.

### Changes
- `LayoutControllerImpl`의 settle latch(`mLayoutDirtySinceEmit`, `mEmitScheduled`)를 제거하고 `LayoutCompletionRecord`를 씁니다.
- root마다 구독 View snapshot을 `mPendingCompletion`에 기록하고, 정상 종료한 batch만 Window 통지를 추가합니다.
- 가장 바깥 자동 pre-process만 기록을 `mReadyCompletion`으로 promote하고, post-process에서 View 다음 Window 순으로 전달합니다.
- 새 snapshot이 있으면 건너뛰던 stale-skip을 제거했고, 연결 여부는 emit 시점에 판단합니다.
- slot이 throw하면 시작된 통지는 반복하지 않고 남은 통지는 유지하며, slot의 `LayoutController::Remove()`는 남은 전달을 멈춥니다.
- Arrange·cache replay의 SVG fitting(`FittingModeUpdate::ARRANGE`)을 제거하고, `OnLayoutFinished`는 `mArrangedBounds`로 맞춥니다.
- `view.h`, `layout-controller.h`의 신호·`ProcessLayouts()` 문서와 진단 로그 문구를 새 계약에 맞게 고칩니다.
- `docs/` 세 문서와 `wiki/Layout-Structures*.md`를 갱신하고, `tc-77` manual test와 Lottie probe sample 출력을 바꿉니다.
- 기존 UTC 기대값을 pass별 통지로 바꾸고, 병합·순서·parked work·연결 판정·`Remove()`·실패 batch·전달 전 이탈/구독 해제 UTC를 추가합니다.
- fitting UTC를 전달 시점 fitting, 최신 대상 사용, 수동 `View::Arrange()` 미적용 기준으로 다시 작성합니다.

### Examples
- 자동 cycle 전에 수동 pass를 두 번 실행하면, 다음 cycle에서 View 한 번(최신 bounds)과 Window 한 번으로 합쳐 전달됩니다.
- 아래는 추가된 `UtcDaliLayoutControllerManualPassesMergeIntoOneDeliveryP`를 요약한 것입니다.

```cpp
LayoutController& controller = LayoutController::Get(window);
root.LayoutFinishedSignal().Connect(&application, [](View, LayoutRect bounds)
{ /* called once, bounds.width == 180 */ });
controller.LayoutFinishedSignal().Connect(&application, [](Window) { /* called once, after the View slot */ });

controller.ProcessLayouts();    // pass 1: root at 100; nothing is emitted inside the call
root.SetRequestedWidth(180.0f);
controller.ProcessLayouts();    // pass 2: root at 180 replaces the same item
application.SendNotification(); // next automatic cycle delivers root (180), then the Window
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
