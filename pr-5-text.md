### Summary
- 텍스트 컨트롤(Label, InputField, InputEditor)에서 natural 단위 값이 effective scale을 따르지 않거나 두 번 곱해지던 문제를 고칩니다.
- 대상은 타이핑 폰트 크기, `GetLineCount()`의 폭 clamp, shadow/outline blur, bevel, 자간, 텍스트 핸들 이미지, replacement 기본 폰트와 ellipsis fallback입니다.
- 변경은 스케일이 곱해지는 값과 그 값을 고르는 기본 폰트에 한정되며, 텍스트 레이아웃 알고리즘 자체는 그대로입니다.
- USER_SPACE gradient와 marquee 속도의 스케일은 upstream(`ade525c3`, `ecb3b0bf`, `097fbd44`, `ca94e54f`)이 이미 처리하므로 이 PR에 포함하지 않습니다.
- `devel` 위에 올라가며, 다른 effective-scale PR과 코드 의존은 없습니다.
- 새 virtual, 공개 API 변경, View당 추가 메모리는 없습니다(공개 헤더는 주석만 변경).

### Changes
- 샘플: UI 스케일 키(없던 샘플만), line count 출력(`9`), USER_SPACE gradient 확인(`0`, `g`, `9`)
- 타이핑·선택 영역 폰트 run이 size × scale로 저장된 뒤 validation에서 다시 스케일되던 것(s²)을 FontSpan처럼 스케일 전 값으로 저장
- `Label::GetLineCount()`의 폭 clamp에 스케일된 min/max 폭 사용(`OnMeasure()`와 동일)
- `InputEditor::GetLineCount()`도 같은 방식으로 수정
- shadow·outline blur 반지름을 스케일된 유효 값으로 계산
- bevel intensity에 텍스트가 렌더링된 스케일을 곱함(동기·비동기 모두)
- 자간을 스케일된 유효 값으로 계산해 레이아웃·커서·ellipsis·렌더링에 사용
- 텍스트 핸들 이미지 크기를 컨트롤의 스케일로 요청(터치 영역·flip 판정 포함)
- 폰트 속성이 없는 텍스트의 이미지 전용 줄이 스케일된 기본 폰트를 사용
- 폰트 없는 END ellipsis가 font client 기본값(12pt) 대신 레이아웃의 기본 폰트(크기·스케일 포함)를 사용

### 동작 변경
- 스케일 1.0에서도: 시스템 폰트 스케일을 쓸 때 타이핑 폰트 크기의 이중 적용이 없어지고, 텍스트 핸들 크기는 마지막으로 지정한 이미지 크기를 따르며, 폰트 없는 ellipsis fallback은 컨트롤 기본 폰트 크기를 씁니다.
- 스케일 ≠ 1.0: 위 값들이 스케일을 따릅니다.
- 알려진 제한: 9-patch 텍스트 핸들의 테두리는 1배로 그려집니다.

### 검증
- 이 PR의 커밋만 `devel`(`004b14f8`) 위에 cherry-pick해도 충돌이 없음을 확인했습니다.
- `devel` `d2d2a7e2` 기준 시리즈 최종 커밋에서 라이브러리·샘플을 빌드하고 public·components·internal UTC를 빌드·실행해, 이 시리즈가 추가한 UTC가 모두 통과함을 확인했습니다(세부 사항은 `pr-effective-scale-fixes.md`). `devel` `004b14f8`로 rebase한 뒤에는 다시 빌드하지 않았습니다.
- `Sample:` 줄의 기대값은 코드에서 계산한 값이며, 샘플을 직접 실행해 확인하지는 않았습니다.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
