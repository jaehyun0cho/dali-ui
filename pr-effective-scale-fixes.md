### Summary
- 앱·스타일·프레임워크 기본값이 주는 길이(크기, 오프셋, 거리, 반지름, 두께, 간격, px/s 속도)를 natural 단위로 보고, 그 값을 가진 뷰의 effective scale(윈도우별 스케일 × 시스템 스케일)을 **정확히 한 번** 곱하도록 정리합니다.
- 입력·제스처 임계값, 텍스처 픽셀 값, 렌더링 좌표 API(Extension setter, 애니메이션 spec, 스크롤 위치), 윈도우 크기는 물리 픽셀로 유지합니다.
- LayoutTransition ENTER/EXIT의 `LayoutBoundsLength::Pixel()` 오프셋도 이제 전환되는 뷰의 스케일을 따릅니다.
- `AddWindowScaleFactor-PR`(LayoutScaleManager, 윈도우별 스케일) 위에 쌓은 후속 PR이며, 커밋은 컴포넌트의 기능 하나당 하나입니다.
- 기반 PR이 `ViewImpl::GetEffectiveScale()`을 deprecated하고 `GetEffectiveLayoutScale()`을 추가했으므로, 이 시리즈의 코드·UTC·샘플·문서도 `GetEffectiveLayoutScale()`을 쓰고 내부 `LinearItemsLayouterImpl`의 스케일 접근자도 `Set/GetEffectiveLayoutScale()`로 이름 붙였습니다.
- USER_SPACE gradient(뷰·Label·InputField/InputEditor·GradientSpan)의 스케일과 marquee 속도는 upstream_devel(`ade525c3`, `ecb3b0bf`, `097fbd44`, `ca94e54f`)이 처리하므로 이 시리즈에서 다루지 않습니다. 샘플의 gradient 확인 키는 그 동작을 보여 줍니다.
- 모든 커밋 메시지의 `Sample:` 줄에 샘플 실행 파일, 누를 키·버튼, 기대 결과(수정 전 값 포함)를 적었습니다.
- 새 virtual, 공개 클래스 레이아웃 변경, View당 추가 메모리는 없습니다(설치 헤더는 주석만 변경).

### Changes
- 레이아웃: transition `Pixel()` × 뷰 스케일, plain View 컨테이너의 `SetRequestedX/Y`는 자식 자신의 스케일, 부모 없는 리프의 즉시 크기·레거시 relayout padding/margin
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
- 알려진 제한: 스케일 변경 직후 다음 레이아웃 패스 전의 RecyclerView 스크롤 요청은 이전 지오메트리 기준(헤더 문서화), WindowFrame은 스케일 변경 후 1프레임 동안 이전 배치, 9-patch 텍스트 핸들의 테두리는 1배

### 검증
- 라이브러리 전체 빌드·링크, UTC 링크·실행, 샘플·manual-tests 빌드: 설치된 dali-adaptor에 `media-view-backend.h`가 없어 하지 못함(커밋별 빌드도 하지 않음)
- 오브젝트 컴파일: 최종 커밋에서 이 PR이 바꾼 라이브러리 소스 55개와 UTC 파일 26개가 모두 성공(UTC는 컴파일만 하고 실행하지 않음, 경고는 base에도 있는 `-Woverloaded-virtual` 헤더 경고가 나는 3개 TU뿐), `view-data-impl.cpp`는 그 파일을 바꾸는 커밋 4개에서도 성공. 이 PR이 바꾼 라이브러리 C++ 파일 115개는 최종 커밋에서 clang-format 무변경
- `Sample:` 줄의 기대값은 코드에서 계산한 값이며, 샘플을 직접 실행해 확인하지는 않았습니다.

### Examples
```cpp
// 스케일 2.0에서 Pixel(40)은 이제 렌더링 기준 80px 아래에서 시작합니다(이전: 40px).
transition.SetEnterBoundsEffect(LayoutBoundsEffects::SlideFrom(LayoutBoundsEdge::BOTTOM,
                                                               LayoutBoundsLength::Pixel(40.0f),
                                                               timing));
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
