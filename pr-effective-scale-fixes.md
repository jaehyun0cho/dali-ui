### Summary
- 앱·스타일·프레임워크 기본값이 주는 길이(크기, 오프셋, 거리, 반지름, 두께, 간격, px/s 속도)를 natural 단위로 보고, 그 값을 가진 뷰의 effective scale(윈도우별 스케일 × 시스템 스케일)을 **정확히 한 번** 곱하도록 정리합니다.
- 입력·제스처 임계값, 텍스처 픽셀 값, 렌더링 좌표 API(Extension setter, 애니메이션 spec, 스크롤 위치), 윈도우 크기는 물리 픽셀로 유지합니다.
- LayoutTransition ENTER/EXIT의 `LayoutBoundsLength::Pixel()` 오프셋도 이제 전환되는 뷰의 스케일을 따릅니다.
- `devel`(`004b14f8`) 위의 레이아웃 수정(`pr-1-layout.md`, 브랜치 `FixLayoutForWindowScale`의 커밋 5개)에 쌓은 후속 PR이며, 커밋은 컴포넌트의 기능 하나당 하나입니다.
- 커밋 순서대로 PR 7개로 나눌 수 있습니다: 레이아웃 5개(`pr-1-layout.md`, `FixLayoutForWindowScale`에 있음), 이어서 이 브랜치의 ScrollView 10개(`pr-2-scrollview.md`), RecyclerView 9개(`pr-3-recyclerview.md`), 비주얼 9개(`pr-4-visual.md`), 텍스트 10개(`pr-5-text.md`), 컴포넌트 6개(`pr-6-components.md`), 문서 1개(`pr-7-docs.md`). 마지막 커밋은 이 PR 설명들입니다. `pr-1`~`pr-6`의 Changes는 항목 하나가 커밋 하나이고 순서도 같습니다.
- 스케일 API가 extension-api로 옮겨졌으므로(`Extension::LayoutScaleManager`, `Extension::View::GetEffectiveLayoutScale()`, `Extension::View::SetUiScalePolicy()`), 이 시리즈의 코드·UTC·샘플·문서도 그 API를 쓰고, 내부 `LinearItemsLayouterImpl`의 스케일 접근자는 `Set/GetEffectiveLayoutScale()`로 이름 붙였습니다.
- USER_SPACE gradient(뷰·Label·InputField/InputEditor·GradientSpan)의 스케일과 marquee 속도는 upstream_devel(`ade525c3`, `ecb3b0bf`, `097fbd44`, `ca94e54f`)이 처리하므로 이 시리즈에서 다루지 않습니다. 샘플의 gradient 확인 키는 그 동작을 보여 줍니다.
- 모든 커밋 메시지의 `Sample:` 줄에 샘플 실행 파일, 누를 키·버튼, 기대 결과(수정 전 값 포함)를 적었습니다.
- 새 virtual, 공개 클래스 레이아웃 변경, View당 추가 메모리는 없습니다(설치 헤더는 주석만 변경).

### Changes
- 레이아웃(기반 브랜치 `FixLayoutForWindowScale`에 포함): transition `Pixel()` × 뷰 스케일, plain View 컨테이너의 `SetRequestedX/Y`는 자식 자신의 스케일, 부모 없는 리프의 즉시 크기·레거시 relayout padding/margin
- ScrollView/PageScrollView: 레이아웃 패스마다 콘텐츠 위치가 s배로 밀리던 문제, padding 오프셋, 레이아웃 전 clamp, 키 스텝, 포커스 peek, 휠, BounceEdgeEffect 한계, 키 edge bump, `SetPageSize`
- RecyclerView: 키 스텝, 포커스 peek, 휠, 키 edge bump, decoration offset·GroupBodyDecoration, 캐시 범위, 아이템 간격·추정 크기, 그룹 body margin; ScrollBar 최소 크기 이중 스케일 제거
- 비주얼: 이미지 ABSOLUTE corner radius, RELATIVE borderline/inner shadow corner, n-patch 고정 테두리, fitting extra size, arc 두께, view-size 이미지 natural size, Gaussian/Background blur 반지름
- 텍스트: 타이핑 폰트 크기 s² 제거, Label/InputEditor `GetLineCount()`, shadow/outline blur, bevel, 자간, 핸들 이미지, replacement 기본 폰트와 ellipsis fallback
- 컴포넌트: Markdown 순서 목록 마커 s² 제거, WindowFrame이 창에 맞게 배치, 기본 decoration 최소 크기를 surface 픽셀로 보고, CanvasView 기본 view box를 natural 크기로 고정, ChartView를 natural 단위 캔버스로
- 샘플: 블록마다 앞에 샘플 전용 커밋을 두어 스케일 키(q/w/e/r/t = 0.8/1.0/1.2/1.5/2.0)와 확인용 콘텐츠를 추가하므로, 같은 샘플로 수정 전·후를 비교할 수 있습니다. scrollview.example은 edge effect 토글을 `e`에서 `d`로 옮겼습니다.
- 문서: Ui-Scale·Unit & Scale wiki에 "스케일을 따르는 값" 절, 기능별 wiki(영/한)·헤더 문서 갱신, 회귀 방지 UTC 추가

### 동작 변경
- 스케일 1.0에서도 달라지는 것: 런타임 `SetPadding`이 ScrollView 콘텐츠 위치에 반영됨, 부모 없는 리프 크기가 즉시 min/max로 clamp됨, 시스템 폰트 스케일 사용 시 타이핑 폰트 크기 이중 적용 해소, 텍스트 핸들 크기가 마지막 지정 이미지 크기로 고정, 폰트 없는 ellipsis fallback이 컨트롤 기본 폰트 크기를 사용
- 스케일 ≠ 1.0에서는 위 값들이 스케일을 따릅니다. 포커스·접근성 하이라이트 테두리(n-patch)는 s > 1에서 두꺼워집니다. `DefaultWindowDecoration::GetMinimumFrameSize()`는 현재 스케일의 surface 픽셀을 반환하므로 스케일 변경 뒤 다시 적용해야 합니다.
- 알려진 제한: 스케일 변경 직후 다음 레이아웃 패스 전의 RecyclerView 스크롤 요청은 이전 지오메트리 기준(헤더 문서화), WindowFrame은 스케일 변경 후 1프레임 동안 이전 배치, 9-patch 텍스트 핸들의 테두리는 1배, window에 붙기 전에 호출한 ScrollView `ScrollTo`는 window scale 항 없이 clamp됨(기존 사전 clamp 한계의 일부)

### 검증
- `devel` `d2d2a7e2` 기준 시리즈 최종 커밋(아래 수정 전)에서 라이브러리(Release)와 샘플 12개 디렉터리를 빌드하고, public·components·internal UTC를 빌드·실행했습니다. 이 시리즈가 추가한 UTC는 모두 통과했고 기존 테스트의 회귀는 없었습니다. internal suite의 WidgetView 42건은 설치된 dali-adaptor의 WidgetEngine 경로에서 나는 segfault로 이 시리즈와 무관하고, 이 suite는 기존 `-Werror=overloaded-virtual` TU 5개와 Release dali-core의 `Log::Filter` 부재를 스크래치 빌드에서 우회해 돌렸습니다.
- 이 시리즈의 public UTC 36건을 수정 전 라이브러리(`FixLayoutForWindowScale`)로 돌리면 34건이 실패해(2건은 회귀 방지용) 테스트가 수정을 실제로 검증합니다. `SetUiScale` 대신 `SetWindowScale`을 쓰는 변형으로 window scale에서도 같은 결과를 확인했습니다.
- 이후 넣은 ChartView MARKER 주석 radius 수정과 샘플(tooltip-shadow, chart-view, window-frame) 수정은 라이브러리·components UTC(컴파일만)·샘플 빌드로 확인했습니다.
- `devel` `004b14f8`로 rebase할 때 `utc-Dali-ImageVisual.cpp`(양쪽이 파일 끝에 테스트 추가)와 `window-frame-impl.h`(양쪽이 같은 위치에 선언 추가)에서 충돌이 나 양쪽을 모두 살렸습니다. rebase 뒤에는 다시 빌드하지 않았고, 충돌을 푼 두 파일만 문법 검사(`g++ -fsyntax-only`)로 확인했습니다.
- `Sample:` 줄의 기대값은 코드에서 계산한 값이며, 샘플을 직접 실행해 확인하지는 않았습니다.

### Examples
```cpp
// 스케일 2.0에서 Pixel(40)은 이제 렌더링 기준 80px 아래에서 시작합니다(이전: 40px).
transition.SetEnterBoundsEffect(LayoutBoundsEffects::SlideFrom(LayoutBoundsEdge::BOTTOM,
                                                               LayoutBoundsLength::Pixel(40.0f),
                                                               timing));
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
