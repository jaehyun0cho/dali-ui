### Summary
- 비주얼과 render effect에서 natural 단위 길이(ABSOLUTE corner radius, RELATIVE borderline 폭, n-patch 고정 테두리, fitting extra size, arc 두께, blur 반지름)가 픽셀로 쓰이던 문제를 고칩니다.
- view 크기로 로드한 이미지의 natural size를 natural 단위로 보고하도록 고칩니다.
- `devel` 위에 올라가며, 다른 effective-scale PR과 코드 의존은 없습니다. 다만 borderline 커밋의 `Sample:` 확인에는 레이아웃 PR의 샘플 커밋이 추가한 ui-scale 테두리 박스가 필요합니다.
- 셰이더는 기존 `viewEffectiveScale` uniform을 사용합니다. 새 virtual, 공개 API 변경, View당 추가 메모리는 없습니다(공개 헤더는 주석만 변경).

### Changes
- 샘플: UI 스케일 키(q/w/e/r/t, gaussian-blur-image는 `r` 제외), USER_SPACE ramp(`4`), extra width가 있는 fitted 이미지(`3`), WRAP view-size 이미지
- 이미지 비주얼 셰이더의 ABSOLUTE corner radius에 `viewEffectiveScale`을 곱함(color·gradient 셰이더와 동일)
- RELATIVE borderline·inner shadow corner radius 계산에서 natural 폭을 스케일하고 스케일 변경 시 다시 계산
- 3x3·일반 n-patch 셰이더가 고정 테두리를 `viewEffectiveScale`로 그림
- fitting mode가 배치하는 비주얼의 `EXTRA_SIZE`에서 스케일된 부분을 SIZE에 더함
- arc 비주얼의 cap 셰이더와 CPU 반지름 계산에서 두께에 스케일을 곱함
- view 크기로 로드한 이미지의 natural size를 요청 당시 스케일로 나눠 보고
- GaussianBlurEffect의 blur pass actor에 owner의 스케일을 등록
- BackgroundBlurEffect도 같은 방식으로 owner의 스케일을 사용

### 동작 변경
- 스케일 ≠ 1.0: 위 길이들이 스케일을 따릅니다. 포커스·접근성 하이라이트 테두리(n-patch)는 s > 1에서 두꺼워집니다.
- WRAP_CONTENT 이미지가 view 크기 로드 후 다시 로드될 때마다 줄어들던 현상이 없어집니다.
- arc 비주얼은 공개 API가 없어 integration property map을 쓰는 경우에만 해당합니다.

### 검증
- 이 PR의 커밋만 `devel`(`004b14f8`) 위에 cherry-pick해도 충돌이 없음을 확인했습니다.
- `devel` `d2d2a7e2` 기준 시리즈 최종 커밋에서 라이브러리·샘플을 빌드하고 public·components·internal UTC를 빌드·실행해, 이 시리즈가 추가한 UTC가 모두 통과함을 확인했습니다(세부 사항은 `pr-effective-scale-fixes.md`). `devel` `004b14f8`로 rebase한 뒤에는 다시 빌드하지 않았습니다.
- `Sample:` 줄의 기대값은 코드에서 계산한 값이며, 샘플을 직접 실행해 확인하지는 않았습니다.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
