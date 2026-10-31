### Summary
- 레이아웃 경로에서 natural 단위 길이에 뷰의 effective scale(윈도우별 스케일 × 시스템 스케일)이 빠지거나 다른 뷰의 스케일이 곱해지던 4곳을 고칩니다.
- 브랜치 `FixLayoutForWindowScale`의 커밋 5개이며, LayoutScaleManager와 윈도우별 스케일이 반영된 `devel` 위에 올라갑니다. 다른 effective-scale PR과 코드 의존은 없습니다.
- 첫 커밋은 샘플 전용입니다. 이후 커밋의 `Sample:` 줄이 가리키는 키와 확인용 콘텐츠를 추가하며, 여기서 추가한 ui-scale 샘플의 테두리 박스는 비주얼 PR의 borderline 확인에도 쓰입니다.
- 모든 커밋 메시지의 `Sample:` 줄에 샘플, 누를 키·버튼, 기대 결과(수정 전 값 포함)를 적었습니다.
- 새 virtual, 공개 API 변경, View당 추가 메모리는 없습니다.

### Changes
- 샘플: layout-transition-spec에 UI 스케일 키(q/w/e/r/t = 0.8/1.0/1.2/1.5/2.0)와 `p`(`Pixel(80)` slide-in), ui-scale에 plain View 행·"Leaf size" 버튼·두꺼운 테두리 박스 추가
- LayoutTransition ENTER/EXIT의 `LayoutBoundsLength::Pixel()` 오프셋에 전환되는 뷰의 스케일을 곱함(margin과 같은 natural 길이)
- plain View 컨테이너가 자식의 `SetRequestedX/Y`를 부모가 아닌 자식 자신의 스케일로 배치
- 부모 없는 리프 View가 `SetRequestedWidth/Height()` 직후 다음 레이아웃 패스와 같은 크기(min/max로 clamp한 요청값 × 스케일)를 바로 받음
- 레거시 relayout(`RelayoutDefault`)이 actor 자식을 옮길 때 padding/margin에 스케일을 곱함

### 동작 변경
- 스케일 1.0에서도: 부모 없는 리프 View의 크기가 요청 직후 min/max로 clamp됩니다.
- 스케일 ≠ 1.0: transition `Pixel()` 오프셋, plain View 컨테이너 자식의 RequestedX/Y, 레거시 relayout의 padding/margin이 스케일을 따릅니다.

### 검증
- 이 PR의 커밋만 `devel`(`004b14f8`) 위에 cherry-pick해도 충돌이 없음을 확인했습니다.
- `devel` `d2d2a7e2` 기준 시리즈 최종 커밋에서 라이브러리·샘플을 빌드하고 public·components·internal UTC를 빌드·실행해, 이 시리즈가 추가한 UTC가 모두 통과함을 확인했습니다(세부 사항은 `pr-effective-scale-fixes.md`). `devel` `004b14f8`로 rebase한 뒤에는 다시 빌드하지 않았습니다.
- `Sample:` 줄의 기대값은 코드에서 계산한 값이며, 샘플을 직접 실행해 확인하지는 않았습니다.

### Examples
```cpp
// 스케일 2.0에서 Pixel(40)은 이제 렌더링 기준 80px 아래에서 시작합니다(이전: 40px).
transition.SetEnterBoundsEffect(LayoutBoundsEffects::SlideFrom(LayoutBoundsEdge::BOTTOM,
                                                               LayoutBoundsLength::Pixel(40.0f),
                                                               timing));
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
