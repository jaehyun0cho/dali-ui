### Summary
- 비주얼에서 natural 단위 길이(ABSOLUTE corner radius, RELATIVE borderline 폭, n-patch 고정 테두리, arc 두께)가 픽셀로 쓰이던 문제를 고칩니다.
- view 크기로 로드한 이미지의 natural size를 natural 단위로 보고하도록 고칩니다.
- `GaussianBlurEffect`·`BackgroundBlurEffect`의 blur radius는 바꾸지 않습니다. blur는 radius가 커질수록 연산량이 늘어나므로 스케일을 따르지 않고 픽셀 값으로 둡니다.
- `devel` 위에 올라가며, 다른 effective-scale PR과 코드 의존은 없습니다. borderline 커밋의 `Sample:` 확인에 쓰는 ui-scale 테두리 박스는 이미 `devel`에 있습니다.
- 셰이더는 기존 `viewEffectiveScale` uniform을 사용합니다. 새 virtual, 공개 API 변경, View당 추가 메모리는 없습니다(공개 헤더는 주석만 변경).

### Changes
- 샘플: UI 스케일 키(q/w/e/r/t), USER_SPACE ramp(`4`), WRAP view-size 이미지 추가. 남는 공간을 받는 content를 확인하는 샘플 3개는 root도 스케일을 따름
- 이미지 비주얼 셰이더의 ABSOLUTE corner radius에 `viewEffectiveScale`을 곱함(color·gradient 셰이더와 동일)
- RELATIVE borderline·inner shadow corner radius 계산에서 natural 폭을 스케일하고 스케일 변경 시 다시 계산
- 3x3·일반 n-patch 셰이더가 고정 테두리를 `viewEffectiveScale`로 그림
- arc 비주얼의 cap 셰이더와 CPU 반지름 계산에서 두께에 스케일을 곱함. 스케일은 vertex·fragment 셰이더가 같이 선언한 `SharedBlock`에서 읽고, butt-cap 셰이더의 자기 block 이름을 `FragBlock`으로 고침
- view 크기로 로드한 이미지의 natural size를 요청 당시 스케일로 나눠 보고

### 동작 변경
- 스케일 ≠ 1.0: 위 길이들이 스케일을 따릅니다. 포커스·접근성 하이라이트 테두리(n-patch)는 s > 1에서 두꺼워집니다.
- 스케일 1 미만에서 view 크기로 다시 로드된 WRAP_CONTENT 이미지가 다음 스케일 변경 때 작아진 크기로 남던 현상이 없어집니다(예: q → w에서 128 px가 아닌 102 px).
- arc 비주얼은 공개 API가 없어 integration property map을 쓰는 경우에만 해당합니다. butt-cap arc는 uniform block을 그대로 쓰는 backend(데스크톱 GLES3, Vulkan)에서 셰이더 link가 실패하던 문제(block 이름 중복, 이 PR 이전부터 있던 문제)가 없어집니다.

### 검증
- 커밋 7개는 upstream `devel`(`492e5502`) 바로 위에 있습니다. 이 base로 rebase한 뒤에는 빌드하지 않았습니다.
- 앞의 커밋 6개: 아래 확인은 이 PR에서 뺀 fitting extra size 커밋이 아직 들어 있던 시리즈에서 한 것입니다. `devel` `d2d2a7e2` 기준 시리즈 최종 커밋에서 라이브러리·샘플을 빌드하고 public·components·internal UTC를 빌드·실행해, 이 시리즈가 추가한 UTC가 모두 통과함을 확인했습니다(세부 사항은 `pr-effective-scale-fixes.md`). `devel` `022245a1` 위의 최종 커밋에서는 라이브러리(Release)·public UTC·샘플(image-view, visual-base)을 빌드하고 internal UTC는 이 PR이 고친 파일 6개만 컴파일했습니다. UTC는 실행하지 않았습니다.
- 마지막 커밋(`SharedBlock`): 아래의 셰이더 compile·link 확인과 바뀐 UTC 파일의 syntax check만 했습니다. 이 커밋 상태 그대로는 라이브러리와 UTC를 빌드하지 않았습니다.
- arc 셰이더 쌍은 dali-adaptor shader parser의 출력(GLES2, block을 푸는 GLES3, uniform block을 쓰는 GLES 3.0·3.2, Vulkan GLSL)을 cap 2종에 대해 glslang과 Mesa llvmpipe로 compile·link했습니다. Tizen 실기기와 실제 Vulkan driver는 확인하지 않았습니다.
- 샘플 root 변경은 화면에서 확인했습니다(x1.0에서는 이전과 픽셀 단위로 같고, 스케일이 커지면 확인 대상 content도 함께 커짐). view-size 이미지 커밋의 `Sample:` 값(q → w에서 128 px, 수정 전 102 px)도 화면에서 확인했고, 나머지 `Sample:` 줄의 기대값은 코드에서 계산한 값입니다.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
