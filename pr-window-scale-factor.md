## Description of Change
윈도우별 스케일을 설정할 수 있게 합니다. 이를 위해 시스템 스케일(UI 스케일)과 윈도우별 스케일을 함께 관리하는 `LayoutScaleManager`를 추가합니다. 뷰의 유효 스케일은 그 뷰가 속한 윈도우의 윈도우별 스케일과 `UiScalePolicy`가 선택한 시스템 스케일 항의 곱이며, 레이아웃 패스에서 적용되므로 렌더링도 같은 스케일을 따릅니다.
기존 `UiScaleManager`는 deprecated되며, 릴리스된 심볼을 모두 유지한 채 `LayoutScaleManager`와 같은 상태를 읽고 씁니다.

### Summary
- `LayoutScaleManager`(신규): `Get`/`DownCast`, `SetUiScale`/`GetUiScale`, `SetScalable`/`IsScalable`, `SetWindowScale`/`GetWindowScale`. 시스템 스케일, 스케일링 스위치, 레이아웃 루트 레지스트리, 윈도우별 스케일 기록을 한 곳에서 보유합니다.
- `UiScaleManager` deprecated(문서 태그만): 심볼·시그니처 변경 없이 같은 상태를 공유하므로 섞어 써도 마지막 설정값이 적용됩니다. 두 핸들은 서로 `DownCast`되지 않습니다. 라이브러리 코드는 더 이상 `UiScaleManager`를 호출하지 않습니다.
- 윈도우별 스케일은 `UiScalePolicy`와 직교하고(`DISABLED` 뷰에도 적용) `SetScalable(false)`는 두 항을 모두 1.0으로 만듭니다(저장값 유지). `1.0` 전달이 리셋이며, 파괴된 윈도우의 기록은 자동으로 회수됩니다.
- 윈도우별 스케일을 쓰지 않는 앱은 inline 게이트 덕분에 윈도우 조회 비용이 없고 유효 스케일 계산 결과가 같습니다(DnD 기본 배치 preview는 아래 영향 참고).
- `DragAndDropEvent::GetPreviewRequestedX/Y/Width/Height`로 preview를 다른 View와 같은 유효 스케일로 배치합니다.
- View당 추가 바이트 0, 새 virtual 0, 새 `friend` 0.

### Changes
- `public-api/configuration/layout-scale-manager.{h,cpp}`, `internal/layout-scale-manager-impl.{h,cpp}` 추가, 우산 헤더에 include 추가. `UiScaleManagerImpl`은 자체 상태 없이 `IntrusivePtr<LayoutScaleManagerImpl>` 하나로 위임합니다.
- `view-data-impl.cpp`: 시스템 스케일 항은 `LayoutScaleManager`에서 읽고 루트 등록/해제는 `LayoutScaleManagerImpl`을 직접 호출합니다. `ComputeEffectiveScale()`은 INHERIT/INHERIT 빠른 경로를 유지하고, 나머지 노드는 항별로 조상 정책을 따라 각 항을 구해 한 번 곱합니다(윈도우별 스케일은 체인당 1회).
- 내부 전용 `WindowScalePolicy`(View당 +0 바이트)로, 이미 스케일된 값을 받는 텍스트 커서·핸들·마커와 인라인 치환 레이어를 윈도우별 스케일 항에서 제외합니다(텍스트 로직 무변경).
- DnD: `DragAndDropEvent` getter 4종을 추가하고, 기본 배치는 preview의 `UiScalePolicy`를 바꾸지 않습니다. 레시피·샘플 2종·wiki(EN/KR)를 갱신했습니다.
- UTC: `utc-Dali-LayoutScaleManager.cpp`(신규), `utc-Dali-UiScaleManager.cpp`(deprecated 경로와 상태 공유), 윈도우별 스케일·정책 조합 케이스, DnD 케이스, 기존 UTC의 `LayoutScaleManager` 이전. 이 시리즈는 로컬에서 빌드·실행하지 않았습니다.
- 문서·샘플: wiki Ui-Scale(EN/KR, deprecated 대응표), `docs/devel-integration-history.md`(이전 표), `docs/layout-structure.md`, 샘플 11종 이전, `samples/ui-scale` 윈도우별 스케일 데모와 README.

### API Changes
| Class | Type | API | Purpose |
|---|---|---|---|
| `LayoutScaleManager` | Class | `LayoutScaleManager` (`layout-scale-manager.h`) | 추가. 시스템 스케일·윈도우별 스케일·스케일링 스위치를 보유하는 프로세스 전역 싱글턴 |
| `LayoutScaleManager` | Method | `Get()`, `DownCast()`, `GetUiScale()`, `SetUiScale(float)`, `IsScalable()`, `SetScalable(bool)` | 추가. `UiScaleManager`의 대체 |
| `LayoutScaleManager` | Method | `GetWindowScale(Window)`, `SetWindowScale(Window, float)` | 추가. 윈도우별 스케일 설정·조회 |
| `UiScaleManager` | Class | 클래스와 모든 멤버 | Deprecated(문서 태그만). 심볼 삭제·시그니처 변경 없음 |
| `DragAndDropEvent` | Method | `GetPreviewRequestedX()`, `GetPreviewRequestedY()` | 추가. pointer를 preview의 requested 단위로 제공 |
| `DragAndDropEvent` | Method | `GetPreviewRequestedWidth()`, `GetPreviewRequestedHeight()` | 추가. drag 시작 시 source 크기를 requested 단위로 제공 |

### Examples
```cpp
#include <dali-ui-foundation/public-api/configuration/layout-scale-manager.h>

LayoutScaleManager manager = LayoutScaleManager::Get();

// 이전: UiScaleManager::Get().SetScale(2.0f)
manager.SetUiScale(2.0f);

// 윈도우마다 다른 스케일
manager.SetWindowScale(mainWindow, 1.0f);   // 기본(기록 없음)
manager.SetWindowScale(subWindow, 1.25f);   // subWindow의 뷰: 유효 스케일 1.25 * 2.0 = 2.5

// 정책과 직교: DISABLED 뷰도 윈도우별 스케일은 받는다
view.SetUiScalePolicy(UiScalePolicy::DISABLED);  // subWindow 안이면 유효 스케일 1.25

// 마스터 스위치는 두 항을 모두 끈다
manager.SetScalable(false);  // 모든 뷰 1.0 (GetWindowScale()은 1.25를 계속 반환)
manager.SetScalable(true);   // 저장값 재적용

// 리셋 (파괴될 윈도우에는 필요 없음, 기록이 자동 회수됨)
manager.SetWindowScale(subWindow, 1.0f);
```

> 주의: 각 항이 개별 검증을 통과해도 **곱**이 유한·정상수라는 보장은 없습니다(`1e20 * 1e20` 오버플로, `1e-25 * 1e-25` 언더플로). 곱을 코드에서 검증하면 두 값의 설정 순서에 따라 같은 쌍이 수락/거부되므로, 호출자 계약으로 문서화했습니다.

> 영향(릴리즈된 동작 변경, 2.5.33.11051부터 문서화된 동작): `DragAndDropDetector` 기본 배치(updater·`DragPreviewPositionSignal` 없음)가 더 이상 preview의 `UiScalePolicy`를 `DISABLED`로 바꾸지 않습니다. S≠1에서 자연 단위 preview와 자식은 다른 뷰와 같은 스케일로 그려지고 grab 지점은 pointer 아래에 유지됩니다. 이전 동작을 전제로 픽셀 크기로 만든 preview(reusable 또는 updater 없는 factory preview)는 preview에 `UiScalePolicy::DISABLED`를 직접 설정하면 윈도우별 스케일이 없을 때 이전과 같습니다. 앱이 직접 배치하는 기존 레시피(`DISABLED` + 물리값)는 윈도우별 스케일이 없으면 그대로 동작하며, 윈도우별 스케일과 함께 쓰려면 새 getter로 옮깁니다.

> 진단 로그: deprecated `UiScaleManager::SetScale()`에 잘못된 값을 넘기면 같은 값이 거부되지만, 오류 로그는 `LayoutScaleManagerImpl::SetUiScale`에서 출력됩니다.

## Test Dependencies (optional)

- dali-core: default
- dali-adaptor: default
- dali-toolkit: default

🤖 Generated with [Claude Code](https://claude.com/claude-code)
