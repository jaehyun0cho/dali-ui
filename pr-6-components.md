### Summary
- WindowFrame, DefaultWindowDecoration, ChartView(dali-ui-components)와 CanvasView에서 natural 단위와 픽셀이 섞여 스케일이 빠지거나 두 번 곱해지던 문제를 고칩니다.
- WindowFrame은 이제 윈도우별 스케일과 UI 스케일이 어떤 값이든 윈도우를 정확히 채우고, decoration 내용물과 inset·리사이즈 핸들·shadow outset만 스케일을 따릅니다.
- 브랜치에서는 텍스트 PR(`pr-5-text.md`) 다음에 쌓여 있지만, 다른 effective-scale PR과 코드 의존은 없습니다. 이 PR의 `Sample:` 줄이 쓰는 window-frame·tooltip-shadow·chart-view 스케일 키는 이 PR의 첫 커밋(샘플 전용)이 추가합니다.
- 새 virtual, 공개 API 변경, View당 추가 메모리는 없습니다(공개 헤더는 주석만 변경).

### Changes
- 샘플: window-frame에 윈도우 스케일 키(`1`/`2`), tooltip-shadow에 UI 스케일 키(`q`–`t`)와 새 canvas(`n`), chart-view에 UI 스케일 키(`q`–`t`) 추가. canvas·chart는 가운데 정렬 유지
- WindowFrame: surface 픽셀 영역을 frame 스케일로 나눠 넘기고, inset·핸들·outset을 스케일하며, 스케일이 바뀌면 다시 배치
- `DefaultWindowDecoration::GetMinimumFrameSize()`가 surface 픽셀 setter에 맞게 현재 스케일의 surface 픽셀을 반환(window-frame 샘플은 `1`/`2` 키 뒤에 다시 적용)
- view box가 0인 CanvasView가 처음 배치된 크기를 스케일로 나눈 natural 크기로 고정
- ChartView가 padding·선 굵기·반지름(MARKER 주석 포함)·대시·gradient 좌표를 natural 단위로 그림

### 동작 변경
- 스케일 ≠ 1.0: WindowFrame이 윈도우를 넘치거나 모자라지 않고, 위 길이들이 스케일을 따릅니다.
- `DefaultWindowDecoration::GetMinimumFrameSize()`는 현재 스케일의 surface 픽셀을 반환하므로 스케일을 바꾼 뒤 다시 적용해야 합니다.
- 알려진 제한: WindowFrame은 스케일이 바뀐 직후 1프레임 동안 이전 배치로 보일 수 있습니다(다음 이벤트 루프 틱에 다시 배치).

### 검증
- 이 PR의 커밋 5개만 `devel`(`492e5502`) 위에 cherry-pick해도 충돌이 없음을 확인했습니다.
- `devel` `d2d2a7e2` 기준 시리즈 최종 커밋에서 라이브러리·샘플을 빌드하고 public·components·internal UTC를 빌드·실행해, 이 시리즈가 추가한 UTC가 모두 통과함을 확인했습니다(세부 사항은 `pr-effective-scale-fixes.md`). `devel` `d0a58a59` 위에 다시 쌓은 시리즈 최종 커밋에서는 라이브러리(Release)·public UTC·샘플(canvas-view, chart-view, desktop-window, window-frame)을 빌드했고, components UTC는 바뀐 파일만 컴파일했습니다. UTC는 실행하지 않았습니다.
- ChartView MARKER 주석 radius 수정과 추가 UTC는 그 뒤에 넣었고 라이브러리·components UTC(컴파일만)·샘플 빌드로 확인했습니다. 이후 `devel` `022245a1`, 이어서 텍스트 PR 다음(`devel` `492e5502`)으로 충돌 없이 rebase했고, 그 뒤에는 다시 빌드하지 않았습니다. 샘플 커밋을 텍스트 PR에서 이 PR 맨 앞으로 옮긴 뒤에도(패치 동일) 빌드하지 않았습니다.
- `Sample:` 줄의 기대값은 코드에서 계산한 값이며, 샘플을 직접 실행해 확인하지는 않았습니다.

### Examples
```cpp
// GetMinimumFrameSize()는 현재 스케일의 surface 픽셀이므로, 스케일을 바꾼 뒤 다시 적용합니다.
Ui::Extension::LayoutScaleManager::Get().SetUiScale(2.0f);
windowFrame.SetMinimumFrameSize(decoration.GetMinimumFrameSize());
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
