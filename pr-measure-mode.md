### Summary
- 이 PR은 두 커밋으로 이루어져 있습니다. (1) `LayoutMode::STANDALONE` 뷰의 measure 제약 규칙을 하나로 통일하고 layout-root 추적을 정리합니다. (2) measure에 모드(AT_MOST / EXACTLY / UNSPECIFIED)를 도입하고, 뷰가 자기 minimum/maximum 안에서만 측정·배치되도록 합니다.
- 배경: `MATCH_PARENT` 자식은 measure 시 최소값(follower)을 보고했고, `WRAP_CONTENT` 컨테이너는 budget 폭으로 측정된 뒤 더 좁은 폭으로 배치되어 줄바꿈 Label의 높이가 어긋났습니다. 또 minimum/maximum이 측정에만 적용되고 배치에는 적용되지 않아 `GetMeasuredSize()`와 실제 배치 크기가 달랐습니다.
- 공개 `View::Measure(w, h)`의 시그니처는 그대로이며, 요청 크기와 부모의 종류에서 모드를 파생합니다. dali-ui 내부 레이아웃은 모드 인자가 있는 내부 overload를 호출하므로 부모를 다시 찾는 비용이 없습니다.
- EXACTLY 축은 부모가 배정한 크기(자기 min/max로 clamp)를 보고하고 그 크기로 측정합니다. `WRAP_CONTENT` 컨테이너는 2단계로 측정합니다(자연 크기 결정 → `MATCH_PARENT`/stretched 자식을 확정된 폭으로 EXACTLY 재측정). 자기 크기가 없는 컨테이너는 이전 수치를 유지합니다.
- Arrange는 slot을 뷰 자신의 minimum/maximum으로 clamp하고 slot의 start edge를 유지합니다(RTL은 end edge). 부모의 형제 배치는 바뀌지 않으며 남는 공간은 재분배하지 않습니다. clamp 순서는 기존대로 max-wins입니다.
- 측정 캐시 키에 모드가 포함되고, 같은 slot에서의 arrange 시점 재측정은 생략됩니다. `RecyclerView` 항목은 cross 축을 EXACTLY로 측정합니다.
- 벤치마크(중앙값): multi-line Label 1000행 리스트의 arrange-only 패스 3.3 → 3.4 ms, 재측정 패스 323 → 3.9 ms(캐시 적중). stretched `MATCH_PARENT` Label 1000행 메뉴의 재측정은 0.9 → 53 ms인데, 올바른 높이를 얻기 위해 행마다 텍스트를 budget 폭과 실제 폭에서 두 번 레이아웃하기 때문이며 문서의 Cost 절에 기록했습니다(이전에는 높이가 틀렸습니다).
- 공개 API와 virtual 함수는 바꾸지 않았고(공개 헤더는 주석만), Label 변경은 EXACTLY 폭 기준으로 높이를 계산하는 한 줄뿐이며, `sizeof(ViewDataImpl)`은 그대로입니다.
- 알려진 한계는 문서의 Limits 절에 있습니다: 재측정 없이 더 넓게 배치되는 custom 컨테이너는 측정 폭 기준 높이를 가지며, 늘어나는 내용이 없는 stretched 컨테이너의 측정값은 자연 크기로 남고, 0.001 px 미만의 slot은 빈 것으로 취급합니다.

### Changes
- 새 내부 헤더 `measure-mode.h`: `MeasureMode`, `MeasureModes`, `MeasureModeCheck`, `MeasureModeParent`, `MAX_EXACT_MEASURE_SIZE`, `EMPTY_SLOT_TOLERANCE`/`SnapEmptySlot`과 용어 정리.
- `view-data-impl.{h,cpp}`: 내부 `Measure(w, h, modeW, modeH, check)`, 모드 파생(`DeriveMeasureModes`, custom 부모 규칙, root/STANDALONE 규칙), 2단계 `MeasureDefault`, `ArrangeImpl`의 `ClampToOwnSizeBounds`, `HasSizeDependentContent`, `CanSkipArrangeRemeasure`, 모드를 포함한 캐시 키, 명시 크기 뷰의 clamp된 content 크기.
- `stack`/`flex`/`grid`/`absolute`/`scroll-view` layout manager: 2단계 측정과 EXACTLY 자식 측정, Flex의 배정 주축 저장과 재분배 규칙, Grid의 셀 초과 자식 재측정, zero-slot fallback, ScrollView의 스크롤 범위를 content의 Arrange 반환 rect로 결정.
- `layout-controller.cpp`, `standalone-bounds-utils.h`: root와 STANDALONE 뷰의 `MATCH_PARENT` 축을 clamp된 slot에서 EXACTLY로 측정.
- `linear-items-layouter-impl.cpp`: RecyclerView 항목을 cross 축 EXACTLY로 측정.
- `label-impl.cpp`: EXACTLY 폭으로 측정될 때 그 폭에서 높이를 계산(한 줄).
- `view.h`, `view-impl.h`: 주석만 변경(측정 계약과 minimum/maximum 설명).
- `docs/layout-structure.md`: Measure modes, Two-step measurement, Requested vs Minimum/Maximum, Lists, Limits, Cost 절 추가·갱신.
- automated-tests(빌드만 수행, 실행하지 않음): `utc-Dali-ViewMeasureMode.cpp` 78건 신규, `utc-Dali-View.cpp`·`utc-Dali-GridLayout.cpp`·internal 테스트 1건 갱신, STANDALONE 관련 25건 추가와 1건 개명.

### Examples
```cpp
// MATCH_PARENT 축의 측정값은 이제 배치될 크기입니다 (이전: 최소값).
View view = View::New();
view.SetRequestedWidth(MATCH_PARENT);
view.SetRequestedHeight(MATCH_PARENT);
view.Measure(200.0f, 150.0f);
// view.GetMeasuredSize() == (200, 150)
```

```cpp
// 자기 maximum을 넘는 slot은 clamp되고 slot의 start edge를 유지합니다.
StackLayout column = StackLayout::New(StackOrientation::VERTICAL); // 300 폭
View item = View::New();
item.SetRequestedWidth(MATCH_PARENT);
item.SetMaximumWidth(100.0f);
column.Add(item);
// 레이아웃 후: item의 폭은 100, x는 slot의 시작점, 형제 배치는 그대로
```

```cpp
// WRAP_CONTENT 컨테이너 안의 줄바꿈 Label은 실제 배치 폭 기준 높이를 가집니다.
StackLayout column = StackLayout::New(StackOrientation::VERTICAL); // WRAP x WRAP
View seed = View::New();
seed.SetRequestedWidth(100.0f);
seed.SetRequestedHeight(0.0f);
Label label = Label::New("The quick brown fox jumps over the lazy dog ...");
label.SetMultiLine(true);
label.SetRequestedWidth(MATCH_PARENT);
column.Add(seed);
column.Add(label);
// 레이아웃 후: column의 높이 == label.GetHeightForWidth(100)
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
