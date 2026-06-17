# DALI UI Layout Review 2

## 대상
- Worktree: `/home/jae/dali/dali-ui-codex`
- Branch: `codex`
- 확인한 sentinel: `README.md`, `dali-ui.manifest`, `dali-ui-foundation`, `dali-ui-components`
- 현재 변경 파일:
  - `dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp`
  - `dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp`
  - `dali-ui-foundation/public-api/view-impl.cpp`
  - `automated-tests/src/dali-ui-foundation/utc-Dali-FlexLayout.cpp`
  - `automated-tests/src/dali-ui-foundation/utc-Dali-StackLayout.cpp`

## 확인된 문제와 조치

### 1. FlexLayout basis 0 처리
- 문제: `FlexLayoutParams::SetFlexBasis(0.0f)`가 실제 layout 계산에서 auto처럼 취급될 수 있었습니다.
- 근거: public 문서는 `WRAP_CONTENT`를 auto로 설명하지만, 기존 manager 조건은 `basis > 0.0f`만 적용했습니다.
- 조치: measure/arrange line 구성 양쪽에서 `basis >= 0.0f`를 적용하도록 변경했습니다.
- 근거 라인:
  - `dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:94`
  - `dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:95`
  - `dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:490`
  - `dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp:491`
- 회귀 UTC:
  - `automated-tests/src/dali-ui-foundation/utc-Dali-FlexLayout.cpp:270`
  - basis 0, grow 1, shrink 0인 두 child가 100px row에서 50px씩 배분되는지 확인합니다.

### 2. StackLayout weight와 main-axis MATCH_PARENT 충돌
- 문제: weight child의 main-axis requested size는 무시되어야 하지만, arrange 후반에서 main-axis `MATCH_PARENT`가 weight allocation을 덮어쓸 수 있었습니다.
- 근거: `StackLayoutParams` 문서는 weight child의 main-axis requested size를 무시한다고 설명합니다. 기존 arrange 경로는 vertical height/horizontal width에서 `MATCH_PARENT`를 다시 parent size로 확장했습니다.
- 조치: weight child에 대해서는 main-axis `MATCH_PARENT` 덮어쓰기를 막고, cross-axis `MATCH_PARENT` 동작은 유지했습니다.
- 근거 라인:
  - `dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp:458`
  - `dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp:459`
  - `dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp:466`
  - `dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp:516`
  - `dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp:554`
- 회귀 UTC:
  - `automated-tests/src/dali-ui-foundation/utc-Dali-StackLayout.cpp:350`
  - vertical/horizontal 양쪽에서 fixed child 40px, spacing 5px, weighted child가 남은 75px만 차지하는지 확인합니다.

### 3. Child order 변경 후 measure cache stale 가능성
- 문제: `Insert()` reorder 경로는 `InvalidateMeasure()`를 호출하지만, actor child order 변경 신호 경로는 `mChildren`를 재정렬한 뒤 `InvalidateArrange()`만 호출했습니다.
- 영향: Flex wrap처럼 child order가 measured size를 바꾸는 layout에서 기존 measure cache가 재사용될 수 있습니다.
- 조치: `ViewImpl::OnChildOrderChanged()`의 invalidation을 `InvalidateMeasure()`로 올렸습니다.
- 근거 라인:
  - `dali-ui-foundation/public-api/view-impl.cpp:2860`
  - `dali-ui-foundation/public-api/view-impl.cpp:2868`
- 회귀 UTC:
  - `automated-tests/src/dali-ui-foundation/utc-Dali-FlexLayout.cpp:702`
  - wrap row에서 순서 변경 전 measured height 200, 변경 후 101이 되어야 함을 확인합니다.

## 검증 결과
- `git diff --check`: 통과했습니다.
- `flex-layout-manager.cpp` 단독 `g++ -c`: 통과했습니다.
- `stack-layout-manager.cpp` 단독 `g++ -c`: 통과했습니다.
- automated-tests 실행 파일 `automated-tests/build/src/dali-ui-foundation/tct-dali-ui-foundation-core`: 현재 생성되어 있지 않습니다.
- 현재 환경에서 다음 헤더가 없습니다:
  - `/home/jae/dali/dali-core/dali-env/opt/include/dali/public-api/adaptor-framework/input-method-context.h`
  - `/home/jae/dali/dali-core/dali-env/opt/include/dali/integration-api/adaptor-framework/input-method-context-integ.h`
- 현재 환경에서 다음 헤더는 존재합니다:
  - `/home/jae/dali/dali-core/dali-env/opt/include/dali/devel-api/adaptor-framework/input-method-context.h`

## 추가 감사에서 남은 후보
- `FlexAlign::BASELINE`은 public enum/API에 있지만 실제 arrange 동작은 no-op입니다. 의도된 미구현인지 정책 확인이 필요합니다.
- Grid explicit definition 밖 row/column placement는 implicit track 생성이 아니라 clamp처럼 동작하는 후보가 있습니다. 정책 확인과 UTC가 필요합니다.
- Grid auto track sizing은 span 1 child만 반영하는 후보가 있습니다. spanning child 기여 정책 확인이 필요합니다.
- ScrollView RTL oversized content는 post-mirror와 scroll position 계산이 충돌할 가능성이 있습니다. 재현 UTC가 필요합니다.
- LayoutTransition SUBTREE ENTER 후보는 add/remove/re-add 같은 동일 pass 시나리오에서 중복될 가능성이 있습니다. 재현 UTC가 필요합니다.

## 현재 상태
- 변경은 commit하지 않았습니다.
- tracked 변경은 5개 파일입니다.
- generated header `automated-tests/src/dali-ui-foundation/tct-dali-ui-foundation-core.h`에는 새 UTC가 등록되지만, 이 파일은 git tracked 대상이 아닙니다.
