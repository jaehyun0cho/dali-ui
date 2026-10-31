### Summary
- 비주얼·텍스트·컴포넌트가 앱·스타일·프레임워크 기본값에서 받는 길이(크기, 오프셋, 거리, 반지름, 두께, 간격)를 natural 단위로 보고, 그 값을 가진 뷰의 effective scale(윈도우별 스케일 × 시스템 스케일)을 **정확히 한 번** 곱하도록 정리합니다.
- 입력·제스처 임계값, 텍스처 픽셀 값, blur radius(`GaussianBlurEffect`·`BackgroundBlurEffect`, 텍스트 shadow·outline), 렌더링 좌표 API(Extension setter, 애니메이션 spec, 스크롤 위치), 윈도우 크기는 물리 픽셀로 유지합니다. blur는 radius가 커질수록 연산량이 늘어나므로 스케일을 따르지 않습니다. fitting mode가 배치하는 비주얼의 extra size도 `devel`과 같이 픽셀 값으로 둡니다.
- 같은 규칙의 레이아웃·ScrollView·RecyclerView 수정(`pr-1`~`pr-3`)은 이미 `devel`에 merge되었습니다. 이 브랜치는 `devel`(`492e5502`) 위에 나머지 23개 커밋을 쌓은 것이며, 커밋은 컴포넌트의 기능 하나당 하나입니다.
- 순서대로 PR 4개로 나눌 수 있습니다: 비주얼 7개(`pr-4-visual.md`, 브랜치 `FixVisualForScale`), 텍스트 10개(`pr-5-text.md`, 브랜치 `FixTextForScale`), 컴포넌트 5개(`pr-6-components.md`), 문서 1개(`pr-7-docs.md`). Changes 항목은 커밋 순서를 따르며, 텍스트 PR은 비슷한 커밋을 한 항목으로 묶었습니다. 마지막 커밋은 이 PR 설명들입니다.
- 브랜치는 비주얼 → 텍스트 → 컴포넌트 → 문서 순서로 쌓여 있습니다. 비주얼·텍스트·컴포넌트 PR 사이에는 코드 의존이 없고, 문서 PR은 다른 세 PR의 동작을 설명하므로 마지막에 올립니다.
- USER_SPACE gradient와 marquee 속도의 스케일은 upstream(`ade525c3`, `ecb3b0bf`, `097fbd44`, `ca94e54f`)이 이미 처리하므로 이 시리즈에서 다루지 않습니다. 샘플의 gradient 확인 키는 그 동작을 보여 줍니다.
- 커밋 메시지의 `Sample:` 줄에 샘플 실행 파일, 누를 키·버튼, 기대 결과(수정 전 값 포함)를 적었고, 샘플로 보일 수 없는 수정은 `Sample: none`과 그 이유를 적었습니다.
- 새 virtual, 공개 API 변경, 모든 View에 드는 추가 메모리는 없습니다. 공개 헤더는 주석만 바뀌고, integration 헤더 `chart-view-impl.h`에 private non-virtual 함수 2개가 추가됩니다.

### Changes
- 비주얼(커밋 7개): 이미지 ABSOLUTE corner radius, RELATIVE borderline·inner shadow corner radius, n-patch 고정 테두리, arc 두께, view-size 이미지 natural size, arc 셰이더가 스케일을 읽는 `SharedBlock` uniform block
- 텍스트(커밋 10개): 타이핑·선택 영역 폰트 크기 s² 제거, Label·InputEditor `GetLineCount()`의 폭 clamp, bevel intensity, 자간, 텍스트 핸들 이미지, replacement 기본 폰트와 ellipsis fallback, Markdown 순서 목록 마커 s² 제거
- 컴포넌트(커밋 5개): WindowFrame이 창에 맞게 배치, 기본 decoration 최소 크기를 surface 픽셀로 보고, CanvasView 기본 view box를 natural 크기로 고정, ChartView를 natural 단위 캔버스로
- 문서(커밋 1개): Ui-Scale wiki에 "스케일을 따르는 값" 절 추가(blur radius와 fitting mode가 배치하는 비주얼의 extra size는 따르지 않는 값으로 명시), Unit & Scale·ScrollView wiki와 공개·integration 헤더 주석에 natural 단위와 렌더링 픽셀 구분
- 샘플: 비주얼·텍스트·컴포넌트 묶음 맨 앞의 샘플 전용 커밋이 스케일 키(UI 스케일 q/w/e/r/t = 0.8/1.0/1.2/1.5/2.0, window-frame은 윈도우 스케일 `1`/`2`)와 확인용 콘텐츠를 추가하므로, 같은 샘플로 수정 전·후를 비교할 수 있습니다. 남는 공간을 받는 content를 확인하는 비주얼 샘플 3개는 root도 스케일을 따릅니다.
- 테스트: 수정 커밋 19개 중 18개가 회귀 방지 UTC를 추가합니다(새 test case 38개, public·internal·components suite). arc `SharedBlock` 커밋은 기존 internal UTC를 고칩니다.

### 동작 변경
- 스케일 1.0에서도 달라지는 것: 시스템 폰트 스케일을 쓸 때 타이핑 폰트 크기의 이중 적용이 없어지고, 텍스트 핸들은 pressed/released·좌우 이미지 크기가 서로 다를 때 표시 중인 이미지 크기로 그려집니다.
- 스케일 ≠ 1.0에서는 위 길이들이 스케일을 따릅니다. 포커스·접근성 하이라이트 테두리(n-patch)는 s > 1에서 두꺼워지고, 스케일 1 미만에서 view 크기로 다시 로드된 WRAP_CONTENT 이미지가 다음 스케일 변경 때 작아진 크기로 남던 현상이 없어집니다. `GetLineCount()`는 스케일이 바뀐 뒤 다음 layout 전까지 마지막 layout을 기준으로 셉니다.
- 이미지만 여러 개 있는 한 줄의 END ellipsis는 줄 높이 계산에서 뺄 이미지를 그려지는 "…"와 같은 폰트 폭으로 정합니다(그려지는 "…"는 같고, 이를 보여 주는 샘플은 없음).
- butt-cap arc는 uniform block을 그대로 쓰는 backend(데스크톱 GLES3, Vulkan)에서 셰이더 link가 실패하던 문제(block 이름 중복, 이 시리즈 이전부터 있던 문제)가 없어집니다.
- `DefaultWindowDecoration::GetMinimumFrameSize()`는 현재 스케일의 surface 픽셀을 반환하므로, 스케일을 바꾼 뒤 다시 적용해야 합니다.
- 알려진 제한: WindowFrame은 스케일 변경 직후 1프레임 동안 이전 배치로 보일 수 있고, 9-patch 텍스트 핸들의 테두리는 1배로 그려집니다. 텍스트 핸들 flip 판정은 핸들 자신의 released 이미지 크기를 기준으로 하므로, 더 큰 pressed 이미지를 쓰면 누르는 동안 경계 밖으로 최대 (크기 차이 × 스케일)만큼 나갈 수 있습니다. Markdown 순서 목록 마커의 크기는 마커를 표시할 때 잰 값이고, 새 항목은 창에 추가되기 전에(윈도우 스케일이 빠진 스케일에서) 잽니다. 따라서 마커 텍스트가 열 폭 추정값보다 넓은 폰트에서만, 저장된 폭이 나중에 적용되는 스케일(윈도우 스케일이 있는 창의 첫 layout부터, 또는 스케일만 바뀐 뒤)에서의 텍스트 폭과 반올림 오차(스케일 비율만큼 커질 수 있음)만큼 다를 수 있습니다.

### 검증
- `devel` `d0a58a59` 위에 있던 구성의 최종 커밋에서 라이브러리(Release)·public UTC·샘플(text, canvas-view, chart-view, desktop-window, window-frame)을 빌드하고, 텍스트·컴포넌트 커밋이 고친 internal·components UTC 파일 6개를 컴파일했습니다. 그 뒤 `devel` `022245a1` 위로 충돌 없이 rebase했고(커밋별 패치 동일), 그 위에서는 비주얼 커밋 7개(fitting extra size 커밋 포함)의 최종 커밋만 라이브러리·public UTC·샘플(image-view, visual-base)을 빌드하고 internal UTC 파일 6개를 컴파일했습니다. UTC는 실행하지 않았습니다.
- 텍스트 핸들·`GetLineCount()` 수정은 `a602046e` 위에 비주얼 커밋 7개를 두고 쌓은 텍스트 커밋 11개(당시에는 컴포넌트 샘플 커밋 포함)의 최종 커밋에서 라이브러리(Release)·public UTC를 빌드하고 바뀐 internal UTC 파일을 컴파일해 확인했습니다(UTC는 실행하지 않음).
- 지금 구성(`devel` `492e5502` 위 `FixVisualForScale`의 비주얼 커밋 7개 다음에 텍스트·컴포넌트·문서 커밋을 다시 쌓고, 문서의 extra size 문장을 고치고, 컴포넌트 샘플 커밋을 컴포넌트 묶음 맨 앞으로 옮김)으로는 빌드하지 않았습니다. 비주얼 PR의 확인 내용은 `pr-4-visual.md`에 있고, 텍스트 커밋은 upstream 변경과 겹치는 곳을 코드로 대조했습니다(`pr-5-text.md`). 그 뒤 다시 쓴 Markdown 순서 목록 마커 커밋만 `markdown-component.cpp` object와 `utc-Dali-MarkdownView.cpp`를 이 구성에서 컴파일했습니다.
- 비주얼·텍스트·컴포넌트·문서 커밋 묶음은 각각 `devel`(`492e5502`) 위에 단독으로 cherry-pick해도 충돌이 없습니다(묶음별 단독 빌드는 하지 않음).
- 이전 구성(`devel` `d2d2a7e2` 기준, 지금은 merge된 레이아웃·ScrollView·RecyclerView 커밋과 이후 뺀 blur 커밋 3개·fitting extra size 커밋 포함)에서는 public·components·internal UTC를 빌드·실행해 이 시리즈가 추가한 UTC가 모두 통과하고 기존 테스트의 회귀가 없음을 확인했습니다. internal suite의 WidgetView 42건은 설치된 dali-adaptor의 WidgetEngine 경로에서 나는 segfault로 이 시리즈와 무관합니다.
- 같은 구성에서 public UTC를 수정 전 라이브러리로 돌리면 회귀 방지용 2건을 뺀 나머지가 모두 실패해, 테스트가 수정을 실제로 검증함을 확인했습니다(`SetWindowScale` 변형으로 window scale에서도 같은 결과).
- `Sample:` 줄의 기대값은 코드에서 계산한 값입니다. 비주얼 PR의 샘플 root 변경, view-size 이미지 값(q → w에서 128 px, 수정 전 102 px), `Label::GetLineCount()` 값(1440×900 창에서 2줄, 수정 전 4줄), Markdown 순서 목록 마커 값(1920×800 창, `e`·`N`·`L`·`L` 뒤 번호 항목 행 84 px, 수정 전 128 px)만 화면에서 확인했습니다.

### Examples
```cpp
// UI 스케일 2.0, natural 80 x 80 box: borderline은 40 px로 그려지고, 띠의 바깥 모서리
// radius는 x1.0의 48 px의 두 배인 96 px입니다(수정 전: 80 px).
box.SetCornerRadius(0.25f);
box.SetCornerRadiusPolicy(CornerRadiusPolicy::RELATIVE);
box.SetBorderlineWidth(20.0f);
box.SetBorderlineOffset(1.0f);

// GetMinimumFrameSize()는 현재 스케일의 surface 픽셀이므로, 스케일을 바꾼 뒤 다시 적용합니다.
Ui::Extension::LayoutScaleManager::Get().SetUiScale(2.0f);
windowFrame.SetMinimumFrameSize(decoration.GetMinimumFrameSize());
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
