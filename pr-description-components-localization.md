### Summary
- screen-reader의 `res/po` 52개 로케일 카탈로그를 dali-ui-components 모듈로 가져와 msgfmt로 `.mo`를 빌드하고, `dali2-ui-components` 패키지에 포함해 설치합니다.
- gettext 도메인은 `dali-ui-components`이며, 설치 경로는 dali-ui-foundation 카탈로그와 동일한 표준 locale 디렉터리(`%{_datadir}/locale`, Tizen: `/usr/share/locale`)입니다. 즉 `dali-ui-foundation.mo`와 `dali-ui-components.mo`가 같은 디렉터리에 나란히 설치됩니다.
- `Components::Localization` 공개 API로 카탈로그 조회와 positional(`%1$d`, `%1$s`) 포맷을 제공합니다. 카탈로그가 실제로 쓰는 인자 조합(d, dd, s, ss, sd) 전부를 오버로드 6종으로 지원하며, 번역 문자열은 인자 타입과 변환 지정자가 일치해야 snprintf에 전달됩니다(불일치 시 원문 폴백).
- Windows(vcpkg msgfmt 탐색 + libintl snprintf)와 Linux/Tizen에서 동일 소스로 동작합니다.
- 커밋 3edbd152 없이도 적용·빌드 가능하도록 무의존으로 구성했습니다.

### Changes
- `dali-ui-components/po/*.po` 52개 추가 (`res/po` 원본 그대로, `po_da` 제외)
- `build/tizen/dali-ui-gettext.cmake` 신규: msgfmt 탐색 공용 매크로
- `build/tizen/dali-ui-components/CMakeLists.txt`: 도메인 `dali-ui-components` 정의, `<prefix>/share/locale`로 po→mo 빌드·설치
- `dali-ui-components/public-api/localization/components-localization.{h,cpp}`: 조회·포맷 API (mutex로 직렬화된 성공 시 래치)
- `samples/components-localization/`: `IDS_ACCS_POP_PAGE_P1SD_OF_P2SD_M_DESCRIPTIVE_FORM_TTS`에 (1, 5)를 삽입해 Label로 표시 ("Page 1 of 5.")
- spec: foundation `%files`의 locale 글롭을 `dali-ui-foundation.mo`로 한정하고, components `%files`에 `dali-ui-components.mo`를 추가 (두 패키지가 같은 디렉터리를 나눠 소유)
- `automated-tests` UTC 14케이스(문자열 인자·타입 불일치 포함), wiki(en/kr) 문서, `BuildRequires: gettext-tools` 갱신

### Examples
```cpp
#include <dali-ui-components/dali-ui-components.h>

Dali::String text = Dali::Ui::Components::Localization::GetLocalizedString(
  "IDS_ACCS_POP_PAGE_P1SD_OF_P2SD_M_DESCRIPTIVE_FORM_TTS", 1, 5);
// en_US: "Page 1 of 5."  /  ko_KR: "5페이지 중 1페이지."

label.SetText(text);
```
