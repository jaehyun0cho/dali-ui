## Description of Change
UI 스케일과 윈도우별 스케일을 설정하고 조회하는 API를 앱 개발자용 공식 공개 API 목록에서 빼고 extension-api로 제공합니다.
사용을 막는 변경은 아닙니다. extension-api 헤더는 공개 API와 같은 devel 패키지로 설치되므로 플랫폼 통합 개발자와 모듈 제공자는 그대로 사용할 수 있습니다.
`LayoutScaleManager`와 `ViewImpl::GetEffectiveLayoutScale()`은 아직 릴리스되지 않았으므로(v2.5.45.11629에 없음) deprecate 없이 옮기고, 릴리스된 `View`/`ViewImpl`의 `SetUiScalePolicy()`/`GetUiScalePolicy()`는 문서 전용으로 deprecated 처리합니다.
AddWindowScaleFactor-PR(8574064d) 위에 쌓은 후속 변경입니다. 루트의 `pr-window-scale-factor.md`는 그 PR 시점의 설명이라 수정하지 않았습니다.
검증: 라이브러리, UTC 3종(빌드만, 실행 안 함), 샘플, manual test를 빌드했고 결과가 8574064d 빌드와 모두 같습니다.

### Summary
- `LayoutScaleManager`를 `Dali::Ui::Extension::LayoutScaleManager`(`extension-api/layout-scale-manager.h`)로 옮기고 공개 umbrella 헤더에서 뺍니다. 동작은 같고 `UiScaleManager`는 계속 같은 상태로 forwarding합니다.
- `Extension::View::SetUiScalePolicy(Ui::View, UiScalePolicy)`와 `GetUiScalePolicy(Ui::View)`를 추가하고 `View`/`ViewImpl`의 같은 멤버에 `@deprecated`를 답니다. 선언, 심볼, 동작은 그대로입니다.
- 미릴리스 `ViewImpl::GetEffectiveLayoutScale()`을 `Extension::View::GetEffectiveLayoutScale(const ViewImpl&)`로 바꿉니다. 이벤트 스레드 assert 없이 읽으며, deprecated `ViewImpl::GetEffectiveScale()`은 같은 값을 계속 반환합니다.
- `LayoutScaleManagerImpl`은 internal에 두고, `UiScalePolicy` enum은 deprecated 멤버가 쓰므로 공개 API에 둡니다.
- 라이브러리, 컴포넌트, 샘플 호출부를 새 API로 옮깁니다. UTC와 Public API만 쓰는 text manual test는 deprecated 멤버를 계속 사용합니다.
- `ViewImpl` 하위 클래스 안에서는 `Ui::Extension::...`로 호출합니다. `Extension::`만 쓰면 `CustomActorImpl::Extension`을 가리킵니다.
- 새 API용 UTC 10개를 추가합니다(정책 설정/조회, deprecated 멤버와의 일치, relayout, 같은 값 no-op, 빈 핸들, 유효 스케일 계산, ViewImpl 내부 호출, 이벤트 스레드 없이 읽기).
- 이 브랜치를 빌드하는 기존 빌드 디렉터리는 cmake를 다시 실행하고, 설치 경로에 남은 `public-api/configuration/layout-scale-manager.h`를 지워야 합니다.

### Changes
- Move LayoutScaleManager to the extension API: `extension-api/layout-scale-manager.{h,cpp}` 추가, 공개 헤더/소스 삭제, umbrella 헤더, internal impl의 handle 타입, UTC, 샘플, 문서, 위키
- Deprecate View UiScalePolicy accessors for Extension::View: `extension-api/view.{h,cpp}`, `view.h`/`view-impl.h` deprecated 주석, `ui-scale-policy.h`, text decorator/replacement/샘플 호출부, UTC 6개, 위키, 드래그 앤 드롭 문서, 이력 문서
- Move GetEffectiveLayoutScale to Extension::View: `view-impl.{h,cpp}`, internal 헬퍼 `ViewDataImpl::GetEffectiveLayoutScaleOf()`, 레이아웃 매니저, integration, 컴포넌트, 샘플, UTC 호출부, UTC 4개, 위키, 문서

### API Changes
| Type | API | Purpose |
|------|-----|---------|
| Moved (unreleased) | `Dali::Ui::LayoutScaleManager` → `Dali::Ui::Extension::LayoutScaleManager` | 스케일 설정은 플랫폼 통합 개발자와 모듈 제공자용 extension API |
| Added (extension) | `Extension::View::SetUiScalePolicy(Ui::View, UiScalePolicy)`, `Extension::View::GetUiScalePolicy(Ui::View)` | deprecated 정책 멤버 대체 |
| Added (extension) | `Extension::View::GetEffectiveLayoutScale(const ViewImpl&)` | `ViewImpl::GetEffectiveLayoutScale()` 대체 |
| Removed (unreleased) | `ViewImpl::GetEffectiveLayoutScale()` | 위 extension 함수로 대체 |
| Deprecated (doc only) | `View::SetUiScalePolicy()`, `View::GetUiScalePolicy()`, `ViewImpl::SetUiScalePolicy()`, `ViewImpl::GetUiScalePolicy()` | 심볼과 동작 유지 |
| Deprecated note | `ViewImpl::GetEffectiveScale()` | 대체 안내를 extension 함수로 변경 |

### Examples
```cpp
#include <dali-ui-foundation/extension-api/layout-scale-manager.h>
#include <dali-ui-foundation/extension-api/view.h>

// Platform side: UI scale and per-window scale
Dali::Ui::Extension::LayoutScaleManager::Get().SetUiScale(1.5f);
Dali::Ui::Extension::LayoutScaleManager::Get().SetWindowScale(window, 2.0f);

// Before: panel.SetUiScalePolicy(Dali::Ui::UiScalePolicy::DISABLED);
Dali::Ui::Extension::View::SetUiScalePolicy(panel, Dali::Ui::UiScalePolicy::DISABLED);

// Inside a ViewImpl subclass. Before: GetEffectiveLayoutScale()
float scale = Dali::Ui::Extension::View::GetEffectiveLayoutScale(*this);
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
