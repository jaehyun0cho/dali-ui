### Summary
- 비주얼·텍스트·컴포넌트가 앱·스타일·프레임워크 기본값에서 받는 길이(크기, 오프셋, 거리, 반지름, 두께, 간격)를 natural 단위로 보고, 그 값을 가진 뷰의 effective scale(윈도우별 스케일 × 시스템 스케일)을 **정확히 한 번** 곱하도록 정리합니다.
- 입력·제스처 임계값, 텍스처 픽셀 값, blur radius(`GaussianBlurEffect`·`BackgroundBlurEffect`, 텍스트 shadow·outline), 렌더링 좌표 API(Extension setter, 애니메이션 spec, 스크롤 위치), 윈도우 크기는 물리 픽셀로 유지합니다. blur는 radius가 커질수록 연산량이 늘어나므로 스케일을 따르지 않습니다.
- 같은 규칙의 레이아웃·ScrollView·RecyclerView 수정(`pr-1`~`pr-3`)은 이미 `devel`에 merge되었습니다. 이 브랜치는 `devel`(`022245a1`) 위에 나머지 23개 커밋을 쌓은 것이며, 커밋은 컴포넌트의 기능 하나당 하나입니다.
- 순서대로 PR 4개로 나눌 수 있습니다: 비주얼 7개(`pr-4-visual.md`, 브랜치 `FixVisualForScale`), 텍스트 9개(`pr-5-text.md`), 컴포넌트 6개(`pr-6-components.md`), 문서 1개(`pr-7-docs.md`). 각 파일의 Changes는 항목 하나가 커밋 하나이고 순서도 같습니다. 마지막 커밋은 이 PR 설명들입니다.
- 문서 PR은 다른 세 PR의 동작을 설명하므로 마지막에 올립니다.
- USER_SPACE gradient와 marquee 속도의 스케일은 upstream(`ade525c3`, `ecb3b0bf`, `097fbd44`, `ca94e54f`)이 이미 처리하므로 이 시리즈에서 다루지 않습니다. 샘플의 gradient 확인 키는 그 동작을 보여 줍니다.
- 모든 커밋 메시지의 `Sample:` 줄에 샘플 실행 파일, 누를 키·버튼, 기대 결과(수정 전 값 포함)를 적었습니다.
- 새 virtual, 공개 API 변경, 모든 View에 드는 추가 메모리는 없습니다. 공개 헤더는 주석만 바뀌고, integration 헤더 `chart-view-impl.h`에 private non-virtual 함수 2개가 추가됩니다.

### Changes
- 비주얼(커밋 7개): 이미지 ABSOLUTE corner radius, RELATIVE borderline·inner shadow corner radius, n-patch 고정 테두리, fitting extra size, arc 두께, view-size 이미지 natural size
- 텍스트(커밋 9개): 타이핑·선택 영역 폰트 크기 s² 제거, Label·InputEditor `GetLineCount()`의 폭 clamp, bevel intensity, 자간, 텍스트 핸들 이미지, replacement 기본 폰트와 ellipsis fallback
- 컴포넌트(커밋 6개): Markdown 순서 목록 마커 s² 제거, WindowFrame이 창에 맞게 배치, 기본 decoration 최소 크기를 surface 픽셀로 보고, CanvasView 기본 view box를 natural 크기로 고정, ChartView를 natural 단위 캔버스로
- 문서(커밋 1개): Ui-Scale wiki에 "스케일을 따르는 값" 절 추가(blur radius는 따르지 않는 값으로 명시), Unit & Scale·ScrollView wiki와 공개·integration 헤더 주석에 natural 단위와 렌더링 픽셀 구분
- 샘플: 묶음마다 앞에 샘플 전용 커밋을 두어 스케일 키(UI 스케일 q/w/e/r/t = 0.8/1.0/1.2/1.5/2.0, window-frame은 윈도우 스케일 `1`/`2`)와 확인용 콘텐츠를 추가하므로, 같은 샘플로 수정 전·후를 비교할 수 있습니다. 남는 공간을 받는 content를 확인하는 비주얼 샘플 3개는 root도 스케일을 따릅니다.
- 테스트: 수정 커밋 19개가 모두 회귀 방지 UTC를 추가합니다(새 test case 36개, public·internal·components suite).

### 동작 변경
- 스케일 1.0에서도 달라지는 것: 시스템 폰트 스케일을 쓸 때 타이핑 폰트 크기의 이중 적용이 없어지고, 텍스트 핸들 크기가 마지막으로 지정한 이미지 크기를 따르며, 폰트 없는 END ellipsis fallback이 컨트롤 기본 폰트 크기를 씁니다.
- 스케일 ≠ 1.0에서는 위 길이들이 스케일을 따릅니다. 포커스·접근성 하이라이트 테두리(n-patch)는 s > 1에서 두꺼워지고, 스케일 1 미만에서 view 크기로 다시 로드된 WRAP_CONTENT 이미지가 다음 스케일 변경 때 작아진 크기로 남던 현상이 없어집니다.
- `DefaultWindowDecoration::GetMinimumFrameSize()`는 현재 스케일의 surface 픽셀을 반환하므로, 스케일을 바꾼 뒤 다시 적용해야 합니다.
- 알려진 제한: WindowFrame은 스케일 변경 직후 1프레임 동안 이전 배치로 보일 수 있고, 9-patch 텍스트 핸들의 테두리는 1배로 그려집니다.

### 검증
- `devel` `d0a58a59` 위에 있던 구성의 최종 커밋에서 라이브러리(Release)·public UTC·샘플(text, canvas-view, chart-view, desktop-window, window-frame)을 빌드하고, 텍스트·컴포넌트 커밋이 고친 internal·components UTC 파일 6개를 컴파일했습니다. 그 뒤 `devel` `022245a1` 위로 충돌 없이 rebase했고(커밋별 패치 동일), 그 위에서는 비주얼 브랜치 `FixVisualForScale`의 최종 커밋만 라이브러리·public UTC·샘플(image-view, visual-base)을 빌드하고 internal UTC 파일 6개를 컴파일했습니다. UTC는 실행하지 않았습니다.
- 비주얼·텍스트·컴포넌트·문서 커밋 묶음은 각각 `devel`(`022245a1`) 위에 단독으로 cherry-pick해도 충돌이 없습니다(묶음별 단독 빌드는 하지 않음).
- 이전 구성(`devel` `d2d2a7e2` 기준, 지금은 merge된 레이아웃·ScrollView·RecyclerView 커밋과 이후 뺀 blur 커밋 3개 포함)에서는 public·components·internal UTC를 빌드·실행해 이 시리즈가 추가한 UTC가 모두 통과하고 기존 테스트의 회귀가 없음을 확인했습니다. internal suite의 WidgetView 42건은 설치된 dali-adaptor의 WidgetEngine 경로에서 나는 segfault로 이 시리즈와 무관합니다.
- 같은 구성에서 public UTC를 수정 전 라이브러리로 돌리면 회귀 방지용 2건을 뺀 나머지가 모두 실패해, 테스트가 수정을 실제로 검증함을 확인했습니다(`SetWindowScale` 변형으로 window scale에서도 같은 결과).
- `Sample:` 줄의 기대값은 코드에서 계산한 값입니다. 비주얼 PR의 샘플 root 변경, fitting extra size 값(80 px), view-size 이미지 값(q → w에서 128 px, 수정 전 102 px)만 화면에서 확인했습니다.

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
