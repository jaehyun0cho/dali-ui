### Summary
- upstream_devel `83abc271` 위의 다섯 커밋으로 구성됩니다. (1) `6d085cdc` `LayoutMode::STANDALONE` 뷰의 measure 제약 통일과 layout-root 추적 정리, (2) `819a9847` 자식의 wrapped 높이를 실제 배치 폭에서 해석(height-for-width), (3) `cf0a7adb` `LabelImpl::IsTextFitActive()` 접근자, (4) `cc2638ff` Label 텍스트 높이 메모(trait) + 생략 규칙, (5) `a1588644` 폭 비의존 서브트리 가지치기(2비트 판정).
- STANDALONE 축별 규칙: `MATCH_PARENT` 축은 부모 extent − 자기 margin, `WRAP_CONTENT` 축은 윈도우 − 자기 margin(윈도우가 없으면 부모 extent), 명시 크기는 `requested × scale`. 부모 measure·부모 placement·자기 root 패스가 같은 도출 함수를 씁니다.
- height-for-width: 일반 `Measure(w,h)`(at-most)는 그대로 두고, 폭이 확정되는 지점(지원 컨테이너의 Arrange, 루트/standalone slot 도출, ScrollView content slot)에서만 내부 비측정·비발행 `ResolveHeightAtWidth`로 "폭 w에서의 높이"를 유도해 소비합니다. 지원 producer는 exact 타입(plain View, multi-line·non-text-fit Label, Stack/Grid/Flex/Absolute manager, callback 없음)이며 그 외는 오늘 값입니다.
- 안전장치: 해석 높이가 예산을 넘으면 미해석(오늘 값), 컨테이너 Arrange는 해석 배치가 bounds에 들어갈 때만 채택(fit check), MATCH 폭 컨테이너의 WRAP 폭 자식은 재측정될 폭으로 예측, 의존 축은 높이만.
- 성능: 메모 trait는 해석된 multi-line Label에만 붙고(키: wrap mode·letter spacing·overflow·direction·outline 폭·locale 세대), 같은 폭에서 clamp 없이 측정된 높이는 재계산 없이 재사용합니다. 2비트 판정으로 텍스트 없는 서브트리는 걷지 않습니다(`sizeof(ViewDataImpl)` 312 불변).
- 벤치마크(1000행 multi-line 리스트, 중앙값): arrange-only 3.3 → 4.1 ms(해석만 적용 시 107 ms), measure 패스 325 → 329 ms(해석만 적용 시 636 ms). WRAP×WRAP 팝업 measure는 +0.1 ms(루트 단계가 재측정 전에 본문을 한 번 해석).
- 공개 API·가상 함수 변경 없음. Label 변경은 비가상 접근자 1개뿐이며 텍스트 로직은 그대로입니다.
- 알려진 한계: custom producer(callback·커스텀 manager·서브클래스) 아래는 오늘 동작, text-fit·single-line Label 미해석, 폭→높이 단방향, `SetMultiLine`/`SetLineWrapMode`/`SetLetterSpacing`/`SetTextOverflowMode`/`SetTextOutline`/locale 변경이 레이아웃을 무효화하지 않는 기존 Label 동작(메모는 키 필드로 우회, 별도 수정 검토 중).

### Changes
- `internal/layouts/height-for-width-resolution.{h,cpp}` (신규): 지원 판정, `ResolveHeightAtWidth`, Default 유도, 루트 helper `ApplyRootHeightForWidth`, WRAP 폭 예측, 예산 초과 guard, Label 메모 trait·skip rule·locale observer, 서브트리 판정 walk.
- `internal/views/view/view-data-impl.{h,cpp}`: `ArrangeDefault` 유도+fit check, `ArrangeStandaloneChild` 루트 helper, `GetLastMeasureConstraint`, `InvalidateLayoutCaches`에서 메모·판정 폐기, 2비트 `HeightForWidthState`와 `ResetHeightForWidthStateUpward`(`SetArrangeCallback`, producer trait 변경), B의 STANDALONE 등록·해제·disconnect 정리.
- `public-api/layouts/{stack,grid,flex,absolute,scroll-view}-layout-manager.cpp`: Arrange와 공유하는 유도 함수, fit check, 해석 진입점; Grid `ApplyGridDefinitions` 축별 분리(문장 동일), ScrollView content slot 해석(무한 budget, ALWAYS 유지).
- `public-api/layouts/layout-controller.cpp`: 두 루트 분기에 `ApplyRootHeightForWidth`; B의 `ReleaseLayoutRootInternal`, 부모 arranged bounds 입력, 리사이즈 키 스냅샷.
- `internal/layouts/standalone-bounds-utils.h`, `public-api/views/view.h`, `public-api/layouts/layout-types.h`: B의 축별 규칙·off-scene 규칙·추적 계약 문서.
- `integration-api/label-impl.{h,cpp}`: `IsTextFitActive()` 비가상 접근자만 추가.
- `docs/layout-structure.md`: "Height-for-width (wrapped content)" 절 신설, MATCH_PARENT·Grid·Flex·STANDALONE 문단 보강, 비용·한계 기술.
- automated-tests(빌드만, 실행 안 함): `utc-Dali-ViewHeightForWidth.cpp` 신규 41건(원 문제, Default/Grid/Flex/Absolute/ScrollView/standalone, FILL 무과대, custom 부모 무회귀, fit fallback, 예산 초과, 좁은 셀 카드 5종, scale 2, 메모 hit/drop/outline/locale, 가지치기 reset), `utc-Dali-Label.cpp` +1, `ui-adaptor.cpp`에 `LocaleChangedSignal` stub; B의 25건 + 개명 1건.
- 저장소 외: 벤치마크 앱과 base/C/D/E 라이브러리는 `~/dali-ui-bench/`에 보관.

### Examples
```cpp
// 세로 WRAP×WRAP StackLayout: 100px 형제와 MATCH_PARENT 폭의 multi-line Label.
// 이전: Label 높이는 budget 480에서 측정된 값으로 배치되어 폭 100에서 잘림.
// 이후: 루트 slot과 Stack의 Arrange가 폭 100에서의 높이를 해석해 배치.
StackLayout stack = StackLayout::New(StackOrientation::VERTICAL);
stack.SetRequestedWidth(WRAP_CONTENT);
stack.SetRequestedHeight(WRAP_CONTENT);

View seed = View::New();
seed.SetRequestedWidth(100.0f);
seed.SetRequestedHeight(0.0f);
stack.Add(seed);

Label label = Label::New("The quick brown fox jumps over the lazy dog ...");
label.SetMultiLine(true);
label.SetRequestedWidth(MATCH_PARENT);
label.SetRequestedHeight(WRAP_CONTENT);
stack.Add(label);

window.Add(stack);
// 레이아웃 후: stack 높이 == label.GetHeightForWidth(100), label 배치 높이도 같음.
```

```cpp
// 명시 크기 STANDALONE 자식은 어느 패스에서도 요청 크기로 측정됩니다.
View parent = View::New();
View boundary = View::New();
boundary.SetLayoutMode(LayoutMode::STANDALONE);
boundary.SetRequestedWidth(200.0f);   // 측정 제약 = 200 × scale (부모·윈도우 무관)
boundary.SetRequestedHeight(WRAP_CONTENT); // 측정 제약 = 윈도우 높이 − margin; 배치 높이는 폭 200에서 해석
parent.Add(boundary);
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
