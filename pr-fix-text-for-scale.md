### Summary
- 텍스트 컨트롤(Label, InputField, InputEditor)에서 effective scale이 두 번 곱해지거나 빠지던 두 곳을 고칩니다: 타이핑·선택 영역의 font size run, `GetLineCount()`의 폭 clamp.
- text 리뷰를 반영해 bevel intensity, letter spacing, text handle image, ellipsis fallback, MarkdownView ordered-list marker 변경은 이 PR에서 뺐습니다. replacement 기본 폰트 변경도 이 PR에서 제외했습니다.
- 샘플, 공개 헤더, integration 헤더는 수정하지 않습니다. 검증은 automated-tests로 합니다.
- 브랜치 `FixTextForScale`은 `devel` `5b9c6243` 위의 커밋 4개입니다.
- 새 virtual, 공개 API 변경, 추가 메모리는 없습니다. 라이브러리 변경은 `.cpp` 4개, 코드 8줄입니다.

### Changes
- 타이핑·선택 영역의 font size run을 FontSpan run처럼 scale을 곱하지 않은 값으로 저장 (font validation이 scale을 한 번 더 곱해 s²가 되던 문제)
- `Label::GetLineCount()`, `InputEditor::GetLineCount()`: 폭 clamp에 쓰는 minimum/maximum width에 마지막 layout의 scale을 곱함
- wiki `Label`, `Label-(kr)`: UI scale 표 아래의 text style TBD 노트 삭제 (underline, shadow, outline은 이미 UI scale을 따름)

### 동작 변경
- scale 1.0에서도: font size scale(system font size 등)이 1이 아닐 때 타이핑·선택 font size가 두 번 곱해지지 않습니다.
- scale ≠ 1.0: `GetLineCount()`는 laid-out 폭을 scale이 곱해진 minimum/maximum width로 clamp한 폭에서 줄 수를 세고, scale이 바뀐 뒤 다음 layout 전까지는 마지막 layout의 scale을 씁니다.
- 이 PR에서 다루지 않는 것(리뷰 반영): bevel intensity, letter spacing, text handle image는 scale을 따르지 않습니다. font defaults가 없는 텍스트의 ImageSpan 전용 줄은 scale을 따르지 않습니다(UI scale 2.0에서 글자 줄 38 px, 이미지 전용 줄 19 px). MarkdownView ordered-list marker는 UI scale ≠ 1.0에서 marker를 다시 표시하면 scale이 한 번 더 곱해집니다(UI scale 2.0에서 행 높이 84 px → 128 px). 두 수치는 rebase 전 빌드에서 이 PC의 기본 폰트로 실측한 값입니다.

### 검증
- `samples/` 아래 파일, 헤더 파일, `text-controller-relayouter.cpp`가 base(`5b9c6243`)와 같음을 스크립트로 확인했습니다.
- 빌드(최종 tree `1b81fb6b`): 라이브러리 foundation + components(Release, `-Werror=shadow`)와 public UTC suite(`dali-ui-foundation`).
- 실행: 이 PR의 public 케이스 2개를 최종 tree의 빌드에서 이름으로 하나씩 실행했고 통과했습니다.
  - `UtcDaliLabelGetLineCountScalesWidthLimitsP`
  - `UtcDaliInputEditorGetLineCountScalesWidthLimitsP`
- `UtcDaliTextDecorationScaleTypingFontSizeP`(internal suite)는 rebase 전 구성(`devel` `492e5502` + visual 커밋 7개 기준, 같은 patch)에서 실행해 통과했습니다. rebase 뒤에는 internal suite를 다시 빌드하지 않았고, 최종 테스트 파일을 suite flags + `-Wshadow`로 syntax check만 했습니다. 그 사이 upstream은 이 PR이 바꾸는 파일과 `dali-ui-foundation/internal/text`를 바꾸지 않았습니다.
- 세 케이스는 rebase 전 구성에서, 해당 수정을 base 코드로 되돌린 빌드로 실행하면 실패함을 확인했습니다(`checking 18 == 8`, `checking 18 == 8`, `checking 2880 == 1440`).
- 테스트는 폰트 크기로 계산된 값(natural size, height-for-width, 고정 줄 수)을 기대값으로 쓰지 않습니다. `GetLineCount()` 케이스는 같은 컨트롤의 `GetLineCount(width)`와 비교하고, 타이핑 font size 케이스는 저장된 run size, 그리고 validation이 고른 font의 point size가 run size × effective text scale인지를 비교합니다.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
