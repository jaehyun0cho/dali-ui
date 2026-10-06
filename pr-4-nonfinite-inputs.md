### Summary
- `UiScaleManager::SetScale`가 NaN과 0 이하 값에 더해 무한대와 subnormal(`FLT_MIN` 미만) scale도 거부합니다.
- 무한대 scale은 모든 View의 effective scale에 곱해져 측정 크기와 배치 rect를 무한대로 만듭니다.
- layout은 effective scale로 나누기도 하므로, subnormal scale은 역수가 무한대로 overflow됩니다.
- layout transition의 timing, animator timing, bounds effect 값은 setter에서 유한한지 검사하고 아니면 `DALI_ABORT`합니다.
- NaN은 `duration <= 0`이나 범위 검사 같은 관계 비교를 모두 통과해 `Animation::New`와 배치 geometry까지 전달되었습니다.
- `Deliver LayoutFinished for every completed layout pass` PR 뒤에 적용하는 것을 권장하며, 그 경우 그대로 적용됩니다.
- 먼저 적용하면 `utc-Dali-LayoutController.cpp`의 `#include` 블록과 파일 끝에서, 그 PR이 추가한 줄이 문맥이라 충돌합니다.
- 이 충돌은 `#include <limits>`와 새 테스트 함수만 추가하면 풀리는 단순 충돌입니다.

### Changes
- `ui-scale-manager-impl.cpp`: 검사를 `!std::isfinite(scale) || scale < std::numeric_limits<float>::min()`로 바꿉니다.
- 거부된 scale은 기존처럼 오류 로그만 남기고 무시되며, 현재 scale이 유지됩니다.
- `ui-scale-manager.h`: `SetScale` 문서에 양수, 유한, normal(`>= FLT_MIN`) 조건과 그 외 값의 처리를 적습니다.
- `layout-transition-validation`: duration·delay 검사용 `AbortIfNonFiniteTiming`, `AbortIfNonFiniteAnimatorTiming`을 추가합니다.
- `AbortIfInvalidBoundsEffect`는 범위 검사 전에 sizeFactor, anchor, offset(`hasOffset`일 때), timing이 유한한지 검사합니다.
- 이 검사는 `SetEnterBoundsEffect`, `SetExitBoundsEffect` 등록 시점과 기존 apply 시점 재검증에 모두 적용됩니다.
- `layout-transition-impl.cpp`: `SetChangeTiming` 두 overload와 `SetEnter/Exit/ChangeAnimator`가 alpha 검사 전에 새 검사를 호출합니다.
- `utc-Dali-LayoutController.cpp`: subnormal·NaN scale 거부를 보는 `UtcDaliUiScaleManagerSetScaleSubnormalRejectedN`을 추가합니다.
- `utc-Dali-LayoutTransition.cpp`: NaN timing과 무한대 animator timing이 abort되는지 보는 테스트를 추가합니다.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
