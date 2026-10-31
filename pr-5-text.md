### Summary
- 텍스트 컨트롤(Label, InputField, InputEditor)과 MarkdownView에서 natural 단위 값이 effective scale을 따르지 않거나 두 번 곱해지던 문제를 고칩니다.
- 대상은 타이핑 폰트 크기, `GetLineCount()`의 폭 clamp, bevel, 자간, 텍스트 핸들 이미지, replacement 기본 폰트와 ellipsis fallback, Markdown 순서 목록 마커입니다.
- dali-core·dali-adaptor에는 이 값들에 UI 스케일을 적용하는 로직이 없으므로, 이 PR의 스케일 적용과 겹치지 않습니다.
- shadow·outline의 blur radius는 스케일을 따르지 않습니다. blur는 radius가 커질수록 연산량이 늘어나기 때문입니다.
- USER_SPACE gradient와 marquee 속도의 스케일은 upstream(`ade525c3`, `ecb3b0bf`, `097fbd44`, `ca94e54f`)이 이미 처리하므로 포함하지 않습니다.
- 브랜치 `FixTextForScale`은 `FixVisualForScale`(`1b31e14a`, `devel` `492e5502` 위 커밋 7개) 위에 있고, 그 위의 커밋 10개가 이 PR의 내용입니다. 비주얼 PR과 코드 의존은 없습니다.
- 새 virtual과 공개 API 변경은 없습니다(공개 헤더는 주석만 변경). 추가 메모리는 InputField·InputEditor의 decorator당 120 byte(핸들 이미지 크기 표)뿐입니다.

### Changes
- 샘플: text 샘플에 UI 스케일 키·line count(`9`)·USER_SPACE gradient(`0`, `g`, `9`) 추가
- 타이핑·선택 영역 폰트 run을 FontSpan처럼 스케일 전 값으로 저장(validation에서 한 번 더 곱해지던 s² 제거)
- `Label`·`InputEditor::GetLineCount()`의 폭 clamp에 마지막 layout의 스케일로 곱한 min/max 폭 사용
- bevel intensity에 텍스트가 렌더링된 스케일을 곱함(동기·비동기 모두)
- 자간을 스케일된 유효 값으로 계산해 레이아웃·커서·ellipsis·렌더링에 사용
- 텍스트 핸들 view와 터치 영역을 표시 중인 이미지(pressed/released, flip된 쪽) 크기 × 스케일로 지정하고, 크기를 읽을 수 없는 이미지(SVG 등)는 natural 크기로 둠
- 폰트 속성이 없는 텍스트의 이미지 전용 줄이 레이아웃의 기본 폰트(크기·스케일 포함)를 사용
- 이미지만 있는 한 줄의 END ellipsis 계획과 fallback이 12 pt 기본 폰트 대신 레이아웃의 기본 폰트를 사용해, 줄 높이 계산에서 뺄 이미지를 그려지는 "…"와 같은 폭으로 정함
- MarkdownView 순서 목록 마커를 다시 표시할 때 Label의 `GetNaturalSize()`(마지막 layout 스케일의 픽셀)를 requested 크기로 넣어 s²가 되던 것을, Label을 effective scale에서 `Measure()`한 값을 그 스케일로 나눈 natural 단위 크기로 바꿈(마커와 host의 고정 크기 구조는 그대로)

### 동작 변경
- 스케일 1.0에서도: 시스템 폰트 스케일을 쓸 때 타이핑 폰트 크기의 이중 적용이 없어집니다. 텍스트 핸들은 pressed/released·좌우 이미지 크기가 서로 다를 때 표시 중인 이미지 크기로 그려집니다.
- 스케일 ≠ 1.0: 위 값들이 스케일을 따릅니다. `GetLineCount()`는 스케일이 바뀐 뒤 다음 layout 전까지 마지막 layout을 기준으로 셉니다.
- 이미지만 여러 개 있는 한 줄의 END ellipsis는 줄 높이 계산에서 뺄 이미지를 그려지는 "…"와 같은 폰트 폭으로 정합니다. 그려지는 "…"는 이전과 같고, 이 차이를 보여 주는 샘플은 없습니다.
- 알려진 제한: 핸들 flip 판정은 핸들 자신의 released 이미지 크기를 기준으로 하므로, 더 큰 pressed 이미지를 쓰면 누르는 동안 경계 밖으로 최대 (크기 차이 × 스케일)만큼 나갈 수 있습니다. 9-patch 텍스트 핸들의 테두리는 1배로 그려집니다. Markdown 순서 목록 마커의 크기는 마커를 표시할 때 잰 값이고, 새 항목은 창에 추가되기 전에(윈도우 스케일이 빠진 스케일에서) 잽니다. 따라서 마커 텍스트가 열 폭 추정값보다 넓은 폰트에서만, 저장된 폭이 나중에 적용되는 스케일(윈도우 스케일이 있는 창의 첫 layout부터, 또는 스케일만 바뀐 뒤)에서의 텍스트 폭과 반올림 오차(스케일 비율만큼 커질 수 있음)만큼 다를 수 있습니다.

### 검증
- 이 PR의 커밋 10개만 `devel`(`492e5502`) 위에 cherry-pick해도 충돌이 없음을 확인했습니다.
- `a602046e` 위에 비주얼 커밋 7개를 두고 쌓은 텍스트 커밋 11개(당시에는 컴포넌트 샘플 커밋 포함)의 최종 커밋에서 라이브러리(Release, `-Werror=shadow`)와 public UTC suite를 빌드했고, 바뀐 internal UTC 파일과 `text-decorator.cpp` object를 컴파일했습니다. UTC는 실행하지 않았으며, 새 테스트의 기대값은 코드를 추적해 정했습니다.
- 그 뒤 `FixVisualForScale`(`devel` `492e5502`) 위로 충돌 없이 rebase했고(bevel 커밋은 include 문맥만 다름), 그 위에서는 빌드하지 않았습니다. 대신 upstream 변경(`5fc3af26` text cutout, `cdf69ea7`, `3490783f`, `926abc3e`)과 겹치는 곳을 코드로 대조했습니다. bevel uniform을 쓰는 곳과 자간을 읽는 곳은 그대로이고, 제거된 deprecated API는 쓰지 않습니다. 이전 구성의 빌드·실행 결과는 `pr-effective-scale-fixes.md`에 있습니다.
- 그 뒤 window-frame·tooltip-shadow·chart-view 샘플 커밋을 컴포넌트 PR(`pr-6-components.md`)로 옮겼습니다. 남은 커밋의 패치는 그대로이고(ellipsis fallback 커밋은 메시지만 고침), 다시 빌드하지 않았습니다.
- 그 뒤 Markdown 순서 목록 마커 커밋을 다시 썼습니다(Changes의 마지막 항목). 이 브랜치에서 그 커밋의 `markdown-component.cpp` object(Release, `-Werror=shadow`)와 `utc-Dali-MarkdownView.cpp`(`-Wshadow -Werror=shadow`)를 컴파일했고, UTC는 실행하지 않았습니다.
- `Sample:` 줄의 기대값은 코드에서 계산한 값이고, 그중 두 가지를 화면에서 확인했습니다. `Label::GetLineCount()` 커밋의 값(1440×900 창, scale 2에서 2줄, 수정 전 4줄)은 `a602046e` 위 같은 커밋의 빌드로 확인했습니다(기본 480×800 창에서는 이 Label이 최대 폭보다 좁아 수정 전후 값이 같음). Markdown 순서 목록 마커 커밋의 `Sample:` 줄은 이 커밋의 `markdown-component.cpp` object로 다시 링크한 components 라이브러리와, 샘플의 키 동작을 같은 순서로 실행하는 샘플 사본으로 1920×800 창(`DALI_WINDOW_WIDTH=1920`)에서 확인했습니다: `e`(2.0), `N`, `L` 두 번 뒤에도 번호 항목 행의 높이가 84 px로 유지되고(수정 전: 첫 `L` 뒤 128 px), `q`(1.0)로 돌아가면 42 px입니다(수정 전: 64 px). 기본 480×800 창에서는 스케일 2.0에서 왼쪽 패널(natural 360 → 720 px)이 창을 다 채워 Markdown 영역이 보이지 않습니다. replacement 기본 폰트·ellipsis fallback 커밋은 샘플로 보일 수 있는 차이가 없어 `Sample: none`입니다.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
