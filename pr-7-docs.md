### Summary
- effective-scale 수정 PR들(레이아웃, ScrollView, RecyclerView, 비주얼, 텍스트, 컴포넌트)이 정리한 규칙을 문서로 묶는 PR입니다. 코드 변경은 없습니다.
- 규칙: 단위 리터럴과 레이아웃 길이는 natural 단위이고, 스크롤 위치는 렌더링 픽셀입니다. 직접 작성한 셰이더는 `viewEffectiveScale`을 적용해야 합니다.
- 위 6개 PR이 모두 반영된 뒤에 올리는 마지막 PR입니다. ScrollView·RecyclerView·View 헤더 문서를 함께 고치므로 텍스트상으로도 레이아웃·ScrollView·RecyclerView PR 뒤에 와야 합니다.

### Changes
- wiki Ui-Scale(EN/KR)에 "스케일을 따르는 값" 절 추가(레이아웃, 스크롤, 비주얼·render effect, 텍스트, 컴포넌트·canvas)
- wiki Unit & Scale(EN/KR)에 natural 단위와 렌더링 픽셀 구분 추가
- wiki ScrollView(EN/KR)에 스크롤 위치·스텝·peek의 단위 설명 추가
- 공개 헤더 주석: `unit.h`, `view.h`, `scroll-view.h`, `layout-types.h`, `absolute-layout.h`, `stack-layout.h`, `visual-base.h`, `visual-types.h`
- integration 헤더 주석: `recycler/recycler-view.h`의 스크롤 거리 단위, `visual-properties-integ.h`의 셰이더 `viewEffectiveScale` 적용 안내

### 검증
- `devel`(`004b14f8`)에 올린 레이아웃·ScrollView·RecyclerView PR의 커밋 위에 이 PR의 커밋을 cherry-pick해도 충돌이 없음을 확인했습니다(그 PR들 없이 단독으로는 `scroll-view.h`에서 충돌).
- 주석·문서만 바꾸므로 동작 변경은 없습니다. 공개 헤더는 `devel` `d2d2a7e2` 기준 시리즈 최종 커밋의 라이브러리·UTC 빌드에서 컴파일됐고, `devel` `004b14f8`로 rebase한 뒤에는 다시 빌드하지 않았습니다.

🤖 Generated with [Claude Code](https://claude.com/claude-code)
