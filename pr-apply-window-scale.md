## Description of Change
윈도우별 스케일 기능을 다시 적용하고, UI 스케일·윈도우 스케일 설정 API를 앱 개발자용 공개 API 목록이 아닌 extension API로 제공합니다.
devel에서 revert된 5개 커밋(`95de0b2d`..`82749540` → revert `c0c51c03`..`369ae41a`)과 **같은 패치**를 다시 적용하고(누적 패치 동일: 97 files, +7443/-1007), 그 위에 extension API 이전 4개 커밋을 쌓았습니다.
뷰의 유효 스케일은 그 뷰가 속한 윈도우의 윈도우 스케일과 `UiScalePolicy`가 선택한 UI 스케일 항의 곱이며, 레이아웃 패스에서 적용되므로 렌더링과 requested↔픽셀 변환도 같은 스케일을 따릅니다.
최종 상태에서 `LayoutScaleManager`와 `UiScalePolicy`는 `Dali::Ui::Extension`이 제공하며 공개 umbrella 헤더에는 없습니다. 릴리스된 `UiScaleManager`, `UiScalePolicy` 열거형, `View`/`ViewImpl`의 `SetUiScalePolicy()`/`GetUiScalePolicy()`, `ViewImpl::GetEffectiveScale()`은 심볼·시그니처·동작을 그대로 둔 채 문서 태그만 deprecated 처리했습니다.
extension API 헤더는 공개 API와 같은 devel 패키지로 설치되므로 플랫폼 통합 개발자와 모듈 제공자는 그대로 사용할 수 있습니다(사용을 막는 변경이 아닙니다).
검증: 재적용 5개 커밋의 누적 패치가 revert된 upstream 커밋들과 완전히 동일함을 확인했습니다. 마지막 커밋은 바뀐 라이브러리 TU 6개, `utc-Dali-View.cpp` 오브젝트, 샘플 TU 2개를 컴파일했고(경고 0), 릴리스된 `View`/`ViewImpl::Set/GetUiScalePolicy` 심볼 4개가 설치된 lib과 같음을 `nm`으로 확인했습니다. UTC는 빌드만 하고 실행하지 않았으며, 이 베이스(`e45e501c`)에서 전체 빌드와 링크를 다시 수행하지는 않았습니다.

### Summary
- 윈도우별 스케일: `Extension::LayoutScaleManager::SetWindowScale(Window, float)`/`GetWindowScale(Window)`를 추가합니다. 윈도우마다 다른 스케일을 주면 그 윈도우의 모든 뷰가 `windowScale × uiScale`로 레이아웃됩니다.
- 스케일 보유자 이전: 시스템 스케일(UI 스케일), 스케일링 스위치, 레이아웃 루트 레지스트리를 `Extension::LayoutScaleManager`(`extension-api/layout-scale-manager.h`)가 보유합니다. `GetScale`/`SetScale`은 `GetUiScale`/`SetUiScale`이 되고, `RegisterLayoutRoot`/`UnregisterLayoutRoot`은 공개 API 없이 라이브러리가 내부에서 처리합니다.
- `UiScaleManager`는 deprecated(문서 태그만)이며 두 클래스가 한 상태를 공유하므로 섞어 써도 마지막 설정값이 적용됩니다. 두 핸들은 서로 다른 객체라서 `DownCast`되지 않습니다.
- 정책: 새 `Extension::UiScalePolicy`(`extension-api/ui-scale-policy.h`, 열거자·값·`uint8_t` 기반 타입 동일한 별개 타입)와 이를 받고 반환하는 `Extension::View::SetUiScalePolicy(Ui::View, Extension::UiScalePolicy)`/`GetUiScalePolicy(Ui::View)`를 추가합니다. `Ui::UiScalePolicy`와 `View`/`ViewImpl`의 정책 접근자는 deprecated이며, 변환은 deprecated `ViewImpl` 멤버 두 곳에서만 일어나고 값 일치는 `static_assert`로 고정합니다.
- 유효 스케일 조회는 `Extension::View::GetEffectiveLayoutScale(const ViewImpl&)`로 제공합니다(이벤트 스레드 assert 없이 읽음). deprecated `ViewImpl::GetEffectiveScale()`은 같은 값을 계속 반환합니다. `ViewImpl` 하위 클래스 안에서는 `Ui::Extension::...`로 호출해야 합니다(`Extension::`만 쓰면 `CustomActorImpl::Extension`).
- 윈도우 스케일은 `UiScalePolicy`와 직교해 `DISABLED` 뷰에도 적용되고, `SetScalable(false)`는 두 항을 모두 1.0으로 만듭니다(저장값은 유지·재적용). `1.0` 전달이 리셋이며, 파괴된 윈도우의 기록은 자동으로 회수됩니다.
- 씬 밖의 뷰는 유효 스케일을 캐시하지 않고, 윈도우에서 분리된 스케일 루트는 하위 트리의 캐시를 버립니다. 그래서 분리된 뷰는 윈도우 항 없이 현재 UI 스케일과 스위치를 바로 따르고, 다시 추가하면 그 윈도우의 스케일이 적용됩니다.
- DnD: `DragAndDropEvent::GetPreviewRequestedX/Y/Width/Height`로 preview를 다른 View와 같은 유효 스케일로 배치하고, 기본 배치는 더 이상 preview의 `UiScalePolicy`를 `DISABLED`로 바꾸지 않습니다(아래 영향 참고).
- 비용: View당 추가 바이트 0(내부 전용 `WindowScalePolicy`는 기존 패딩에 들어감), 새 virtual 0, 새 `friend` 0. 윈도우 스케일을 쓰지 않는 앱은 inline 게이트로 윈도우 조회 비용이 없습니다.

### Changes
- `Add LayoutScaleManager and deprecate UiScaleManager`: `layout-scale-manager.{h,cpp}`, `internal/layout-scale-manager-impl.{h,cpp}` 추가. `UiScaleManagerImpl`은 자체 상태 없이 `LayoutScaleManagerImpl`에 위임하고, 라이브러리·UTC·샘플·문서 호출부를 이전합니다.
- `Add a per-window scale`: 윈도우별 스케일 기록·회수, 체인당 1회 곱하는 `ComputeEffectiveScale()`, 씬 밖 캐시 금지와 분리 시 하위 트리 리셋, 이미 스케일된 텍스트 커서·핸들·치환 레이어를 제외하는 내부 전용 `WindowScalePolicy`(텍스트 로직 무변경).
- `Add window scale controls and a two-window scenario to the ui-scale sample`: 프리셋 행과 `System × Window = Final` 상태 라벨, 다른 윈도우 스케일로 두 번째 Window를 띄우는 Zone I, `samples/ui-scale/README.md`.
- `Add preview requested X, Y, width and height to DragAndDropEvent`: getter 4종(포인터를 preview 자신의 유효 스케일로, source 렌더 크기를 렌더 당시 스케일로 나눈 값)과 앱 레시피·샘플 2종·wiki(EN/KR) 갱신.
- `Stop forcing scale policies on the default drag preview`: 기본 배치가 `UiScalePolicy`/`WindowScalePolicy`를 `DISABLED`로 설정하지 않고 `GetPreviewRequestedX()`/`GetPreviewRequestedY()`로 배치합니다.
- `Move LayoutScaleManager to the extension API`: `Dali::Ui::Extension::LayoutScaleManager`로 옮기고 공개 umbrella 헤더에서 제외합니다(동작 불변, `UiScaleManager` forwarding 유지).
- `Deprecate View UiScalePolicy accessors for Extension::View`: `Extension::View` 접근자 추가, `View`/`ViewImpl` 멤버에 `@deprecated`, 라이브러리·샘플 호출부 이전. UTC와 Public API만 쓰는 text manual test는 deprecated 멤버를 계속 검증·사용합니다.
- `Move GetEffectiveLayoutScale to Extension::View`: 미릴리스 `ViewImpl::GetEffectiveLayoutScale()`을 extension 함수로 대체하고 내부 헬퍼 `ViewDataImpl::GetEffectiveLayoutScaleOf()`를 통해 레이아웃 매니저·integration·컴포넌트·샘플 호출부를 이전합니다.
- `Deprecate UiScalePolicy for Extension::UiScalePolicy`: `extension-api/ui-scale-policy.h` 추가(extension umbrella 포함), `Extension::View`와 `ViewDataImpl`이 새 타입을 쓰고 `ViewImpl` 멤버가 변환, `Ui::UiScalePolicy`에 `@deprecated`. text decorator·inline replacement·샘플 2종은 타입 표기만 바뀝니다.
- UTC·문서: `utc-Dali-LayoutScaleManager.cpp`(신규 25 TC), `utc-Dali-UiScaleManager.cpp`(신규 15 TC), `Extension::View` 12 TC(값 일치·세 값 양방향 변환 포함), DnD 10 TC와 기존 UTC 이전. wiki Ui-Scale·In-Scene-Drag-and-Drop·Layout-Structures(각 EN/KR), `docs/devel-integration-history.md` 이전 표 5개, `docs/layout-structure.md`, `docs/project/index.html`, 샘플 13종.

### API Changes
| Class | Type | API | Purpose |
|---|---|---|---|
| `Extension::LayoutScaleManager` | Class | `extension-api/layout-scale-manager.h` | 추가. UI 스케일·윈도우 스케일·스케일링 스위치를 보유하는 프로세스 전역 싱글턴 |
| `Extension::LayoutScaleManager` | Method | `Get()`, `DownCast()`, `GetUiScale()`, `SetUiScale(float)`, `IsScalable()`, `SetScalable(bool)` | 추가. `UiScaleManager`의 대체 |
| `Extension::LayoutScaleManager` | Method | `GetWindowScale(Window)`, `SetWindowScale(Window, float)` | 추가. 윈도우별 스케일 조회·설정 |
| `Extension::UiScalePolicy` | Enum | `INHERIT`, `ENABLED`, `DISABLED` (`extension-api/ui-scale-policy.h`) | 추가. `Ui::UiScalePolicy`의 대체(값·기반 타입 동일, 별개 타입) |
| `Extension::View` | Function | `SetUiScalePolicy(Ui::View, Extension::UiScalePolicy)`, `GetUiScalePolicy(Ui::View)` | 추가. deprecated 정책 멤버의 대체 |
| `Extension::View` | Function | `GetEffectiveLayoutScale(const ViewImpl&)` | 추가. 뷰의 유효 스케일(윈도우 스케일 × UI 스케일 항) |
| `DragAndDropEvent` | Method | `GetPreviewRequestedX()`, `GetPreviewRequestedY()` | 추가. 포인터를 preview의 requested 단위로 제공 |
| `DragAndDropEvent` | Method | `GetPreviewRequestedWidth()`, `GetPreviewRequestedHeight()` | 추가. drag 시작 시 source 크기를 requested 단위로 제공 |
| `UiScaleManager` | Class | 클래스와 모든 멤버 | Deprecated(문서 태그만). 심볼 삭제·시그니처 변경 없음 |
| `UiScalePolicy` | Enum | `Ui::UiScalePolicy` (`public-api/configuration/ui-scale-policy.h`) | Deprecated(문서 태그만). deprecated 정책 멤버가 계속 받고 반환 |
| `View`, `ViewImpl` | Method | `SetUiScalePolicy()`, `GetUiScalePolicy()` | Deprecated(문서 태그만). `Ui::UiScalePolicy` 시그니처·심볼·동작 유지 |
| `ViewImpl` | Method | `GetEffectiveScale()` | Deprecated(문서 태그만). `Extension::View::GetEffectiveLayoutScale()`과 같은 값 |

### Examples
```cpp
#include <dali-ui-foundation/extension-api/layout-scale-manager.h>
#include <dali-ui-foundation/extension-api/ui-scale-policy.h>
#include <dali-ui-foundation/extension-api/view.h>

namespace Extension = Dali::Ui::Extension;

auto manager = Extension::LayoutScaleManager::Get();

// 이전: UiScaleManager::Get().SetScale(2.0f) (deprecated)
manager.SetUiScale(2.0f);

// 윈도우마다 다른 스케일
manager.SetWindowScale(mainWindow, 1.0f); // 기본(기록 없음)
manager.SetWindowScale(subWindow, 1.25f); // subWindow의 뷰: 유효 스케일 1.25 * 2.0 = 2.5

// 이전: GetImpl(view).GetEffectiveScale() (deprecated)
float scale = Extension::View::GetEffectiveLayoutScale(GetImpl(view)); // subWindow의 INHERIT 뷰: 2.5
subWindow.Remove(view);   // 분리된 뷰는 윈도우 항이 없다: 2.0
manager.SetUiScale(3.0f); // 분리된 뷰도 바로 따른다: 3.0
subWindow.Add(view);      // 다시 윈도우 스케일 적용: 1.25 * 3.0

// 정책과 직교: DISABLED 뷰도 윈도우 스케일은 받는다
// 이전: panel.SetUiScalePolicy(Dali::Ui::UiScalePolicy::DISABLED) (deprecated)
Extension::View::SetUiScalePolicy(panel, Extension::UiScalePolicy::DISABLED); // subWindow 안이면 1.25

// deprecated 멤버로 읽은 값은 static_cast로 변환한다 (값이 같은 별개 타입, 암묵 변환 없음)
auto policy = static_cast<Extension::UiScalePolicy>(panel.GetUiScalePolicy());

// 마스터 스위치는 두 항을 모두 끈다 (저장값은 유지되고 다시 켜면 재적용)
manager.SetScalable(false); // 모든 뷰 1.0, GetWindowScale()은 1.25를 계속 반환
manager.SetScalable(true);

// ViewImpl 하위 클래스 안에서는 Ui:: 한정자를 붙인다
float mine = Dali::Ui::Extension::View::GetEffectiveLayoutScale(*this);
```

```cpp
// DnD preview: UiScalePolicy::DISABLED + 물리 단위 대신 requested 단위로 배치한다
preview.SetRequestedWidth(event.GetPreviewRequestedWidth());
preview.SetRequestedHeight(event.GetPreviewRequestedHeight());
preview.SetRequestedX(event.GetPreviewRequestedX());
preview.SetRequestedY(event.GetPreviewRequestedY());
```

> 영향(릴리스된 동작 변경): `DragAndDropDetector` 기본 배치(updater·`DragPreviewPositionSignal` 없음)가 더 이상 preview의 `UiScalePolicy`를 `DISABLED`로 바꾸지 않습니다. 스케일 ≠ 1에서 자연 단위 preview와 그 자식은 다른 뷰와 같은 스케일로 그려지고 grab 지점은 포인터 아래에 유지됩니다. 이전 동작을 전제로 픽셀 크기로 만든 preview는 preview에 `Extension::UiScalePolicy::DISABLED`를 직접 지정하면 윈도우 스케일이 없을 때 이전과 같습니다.

> 두 `UiScalePolicy`: 서로 암묵 변환되지 않으므로 섞어 쓰면 컴파일 오류로 드러나고 동작이 조용히 바뀌지 않습니다. `using namespace Dali::Ui;`와 `using namespace Dali::Ui::Extension;`을 함께 쓰면 한정자 없는 `UiScalePolicy`가 모호해지고, `namespace Dali::Ui::Extension` 안에서 deprecated 멤버에 값을 넘길 때는 `Ui::UiScalePolicy::...`로 한정해야 합니다.

> 주의: 각 항이 개별 검증을 통과해도 **곱**이 유한·정상수라는 보장은 없습니다(`1e20 * 1e20` 오버플로, `1e-25 * 1e-25` 언더플로). 곱을 코드에서 검증하면 설정 순서에 따라 같은 쌍이 수락/거부되므로 호출자 계약으로 문서화했습니다.

> 빌드: 이 브랜치를 빌드하는 기존 빌드 디렉터리는 cmake를 다시 실행하고, 설치 경로에 남은 `public-api/configuration/layout-scale-manager.h`를 지워야 합니다.

> 진단 로그: deprecated `UiScaleManager::SetScale()`에 잘못된 값을 넘기면 같은 값이 거부되지만, 오류 로그는 `LayoutScaleManagerImpl::SetUiScale`에서 출력됩니다.

## Test Dependencies (optional)

- dali-core: default
- dali-adaptor: default
- dali-toolkit: default

🤖 Generated with [Claude Code](https://claude.com/claude-code)
