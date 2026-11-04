## Description of Change

**한 줄 요약: 스케일 설정 API를 "앱 개발자용 공개 API" 자리에서 "플랫폼 통합 개발자용 extension API" 자리로 옮깁니다. 동작은 하나도 바뀌지 않습니다.**

UI 스케일과 윈도우별 스케일은 앱이 직접 만지는 값이 아니라 플랫폼이 정해주는 값입니다.
그런데 지금은 `LayoutScaleManager`, `View::SetUiScalePolicy()` 처럼 앱 개발자가 보는 공개 API 목록에 섞여 있습니다.
이 PR은 그 API들을 `extension-api/`(= `Dali::Ui::Extension`)로 옮겨, API 목록만 보고도 "이건 플랫폼/모듈 쪽 API"임이 드러나게 합니다.

**사용을 막는 변경이 아닙니다.** extension-api 헤더는 공개 API와 **같은 devel 패키지**로 설치되므로,
플랫폼 통합 개발자와 모듈 제공자는 include 경로만 바꿔 그대로 쓸 수 있습니다.

처리 방식은 "릴리스되었는지"로 갈립니다.

| 대상 | 릴리스 여부 | 처리 | 이유 |
|------|------------|------|------|
| `LayoutScaleManager` | 미릴리스 (v2.5.45.11629에 없음) | 바로 이동 | 쓰는 앱이 없으므로 deprecate 단계 불필요 |
| `ViewImpl::GetEffectiveLayoutScale()` | 미릴리스 | 바로 교체 | 위와 같음 |
| `View`/`ViewImpl`의 `Set/GetUiScalePolicy()` | 릴리스됨 | **문서 전용 deprecate** | 심볼·동작 그대로 유지해 기존 코드가 깨지지 않음 |

`AddWindowScaleFactor-PR`(`8574064d`) 위에 쌓은 후속 변경입니다.
루트의 `pr-window-scale-factor.md`는 그 PR 시점 설명이라 수정하지 않았습니다.

### Summary
- `LayoutScaleManager` → `Dali::Ui::Extension::LayoutScaleManager`(`extension-api/layout-scale-manager.h`)로 이동하고 공개 umbrella 헤더에서 제거했습니다. 상태는 하나라서 `UiScaleManager`로 설정한 값과 서로 그대로 보입니다.
- `Extension::View::SetUiScalePolicy(view, policy)` / `GetUiScalePolicy(view)`를 추가하고, `View`/`ViewImpl`의 같은 멤버에는 `@deprecated` 주석만 달았습니다(선언·심볼·동작 유지).
- 미릴리스 `ViewImpl::GetEffectiveLayoutScale()`은 삭제하고 `Extension::View::GetEffectiveLayoutScale(const ViewImpl&)`로 대체했습니다. deprecated `ViewImpl::GetEffectiveScale()`은 계속 같은 값을 반환합니다.
- 새 함수는 이벤트 스레드 assert 없이 읽습니다. 화면에 올라간 뷰는 캐시를 쓰고, 화면 밖 뷰는 매번 계산해 항상 현재 스케일을 반영합니다.
- `LayoutScaleManagerImpl`은 internal에 그대로 두고, `UiScalePolicy` enum은 deprecated 멤버가 쓰므로 공개 API에 남깁니다.
- 라이브러리·컴포넌트·샘플 호출부를 새 API로 옮겼습니다. UTC와 Public API만 쓰는 text manual test는 deprecated 멤버를 계속 호출해 하위 호환을 지킵니다.
- 새 API용 UTC 10개를 추가했습니다(정책 설정/조회, deprecated 멤버와 값 일치, relayout 유발, 같은 값 no-op, 빈 핸들 2건, 유효 스케일 계산, ViewImpl 내부 호출, 이벤트 스레드 없이 읽기).

### Changes
- `Move LayoutScaleManager to the extension API`(`5ee2ad7e`): 헤더/소스를 `extension-api/`로 이동, umbrella 헤더 교체, internal impl의 handle 타입·UTC·샘플·문서·위키 갱신 (53 files)
- `Deprecate View UiScalePolicy accessors for Extension::View`(`705a8782`): `extension-api/view.{h,cpp}`에 2개 함수 추가, `view.h`/`view-impl.h`에 deprecate 주석, text decorator/replacement/샘플 호출부 이전, UTC 6개 (17 files)
- `Move GetEffectiveLayoutScale to Extension::View`(`50326fa1`): `view-impl.{h,cpp}`에서 제거, internal 헬퍼 `ViewDataImpl::GetEffectiveLayoutScaleOf()` 추가, 레이아웃 매니저·integration·컴포넌트·비주얼·샘플 호출부 전면 이전, UTC 4개 (50 files)

### API Changes
| 구분 | API | 설명 |
|------|-----|------|
| 이동(미릴리스) | `Dali::Ui::LayoutScaleManager` → `Dali::Ui::Extension::LayoutScaleManager` | 스케일 설정은 플랫폼 통합/모듈 제공자용 |
| 추가(extension) | `Extension::View::SetUiScalePolicy(Ui::View, UiScalePolicy)` | deprecated 멤버 대체 |
| 추가(extension) | `Extension::View::GetUiScalePolicy(Ui::View)` | 기본값 `UiScalePolicy::INHERIT` |
| 추가(extension) | `Extension::View::GetEffectiveLayoutScale(const ViewImpl&)` | 삭제된 멤버 대체, 이벤트 스레드 assert 없음 |
| 삭제(미릴리스) | `ViewImpl::GetEffectiveLayoutScale()` | 위 함수로 대체 |
| deprecate(문서 전용) | `View::Set/GetUiScalePolicy()`, `ViewImpl::Set/GetUiScalePolicy()` | 심볼·동작 유지 |
| deprecate 문구 변경 | `ViewImpl::GetEffectiveScale()` | 대체 안내를 extension 함수로 교체 |

### Examples
```cpp
#include <dali-ui-foundation/extension-api/layout-scale-manager.h>
#include <dali-ui-foundation/extension-api/view.h>

// 1) 플랫폼 쪽: UI 스케일과 윈도우별 스케일
//    Before: Dali::Ui::LayoutScaleManager::Get().SetUiScale(1.5f);
Dali::Ui::Extension::LayoutScaleManager::Get().SetUiScale(1.5f);
Dali::Ui::Extension::LayoutScaleManager::Get().SetWindowScale(window, 2.0f);

// 2) 뷰 단위 정책: 멤버 호출 → 핸들을 넘기는 자유 함수
//    Before: panel.SetUiScalePolicy(Dali::Ui::UiScalePolicy::DISABLED);
Dali::Ui::Extension::View::SetUiScalePolicy(panel, Dali::Ui::UiScalePolicy::DISABLED);

// 3) 커스텀 뷰 안에서 유효 스케일 읽기
//    Before: float scale = GetEffectiveLayoutScale();
float scale = Dali::Ui::Extension::View::GetEffectiveLayoutScale(*this);
```

> **주의:** `ViewImpl` 하위 클래스 안에서는 반드시 `Ui::Extension::...`로 적어야 합니다.
> `Extension::`만 쓰면 `CustomActorImpl::Extension`을 가리켜 컴파일이 깨집니다.

### Verification
- 라이브러리, UTC 3종(**빌드만, 실행 안 함**), 샘플, manual test 빌드 결과가 기반 커밋 `8574064d` 빌드와 모두 동일합니다.
- 이 브랜치를 기존 빌드 디렉터리에서 빌드할 때는 **cmake를 다시 실행**하고, 설치 경로에 남은 `public-api/configuration/layout-scale-manager.h`를 **직접 삭제**해야 합니다. 남아 있으면 옛 헤더가 먼저 잡힙니다.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
