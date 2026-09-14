### Summary

Measure constraint가 작게 바뀌었을 때 이전 line wrapping 결과를 cache reuse하던 문제를 exact comparison으로 수정합니다.
기본 View의 MATCH_PARENT Arrange budget을 기존 Measure arithmetic과 맞추고, actual slot을 만드는 fixed requested extent를 조건부로 사용해 clean child의 반복 cache miss를 줄입니다.
View data member를 추가하지 않고 Stack·Flex의 temporary storage와 Recycler의 fixed item set에 대한 sequential growth 비용을 줄입니다.

### Changes

- normalization·min/max clamping 후 두 Measure constraint를 numeric `==`로 비교하며 valid/dirty/poison·scale·invalidation·cache publication contract는 유지합니다.
- 기본 View의 cold Measure arithmetic은 유지합니다. Arrange에서 finite·nonnegative requested extent의 scaled value가 finite이고 actual slot과 정확히 같을 때만 해당 natural extent를 사용합니다.
- 그 밖의 slot은 기존 normalization을 사용합니다. parent min/max를 다시 적용하지 않으며 outer rect·WRAP/fixed child axis·MP의 opposite-axis slot policy를 유지합니다.
- parent input은 child callback 전에 capture하고 MP child가 있는 axis에서만 content를 계산합니다. 기존 finiteness check와 visual-space fallback을 유지합니다.
- Stack의 write-only buffer를 제거하고 FlexLine을 move하며, Recycler capacity는 현재 item count 등을 상한으로 약 1.5배 growth를 적용합니다.
- public Layout UTC 20개와 기존 public/integration interface를 사용하는 Recycler UTC 4개를 포함합니다.
- public API comment, contract document, English·Korean wiki에 적용 범위·remaining cache miss·fixed request와 clamp 단계·extent cache growth의 한계를 명시합니다.

### Examples

**수정되는 버그: 작은 constraint 변경으로 line wrapping이 필요해져도 이전 Measure 결과를 반환하는 문제**

- 재현 조건: scale 1의 ROW/WRAP Flex는 width·height가 WRAP_CONTENT이고, 50×20인 child 두 개를 가집니다. `Measure(100, 100)` 후 invalidation 없이 `Measure(99.99951171875, 100)`을 호출합니다.
- 수정 전: width 차이가 epsilon 0.001 미만이므로 이전 cache의 100×20을 반환해, 두 줄에 필요한 height 40을 반영하지 못합니다.
- 수정 후: constraint를 exact comparison하여 cache miss로 처리하고, 새 constraint로 다시 계산한 50×40을 반환합니다. 같은 constraint로 처음 계산한 cold 결과와도 일치합니다.

Regression test: `UtcDaliLayoutConstraintCacheWarmMatchesColdWrapP`.

fixed requested extent/scale 조합 414/0.9, 768/0.9, 3840/1.2를 width·height 각각 검사했습니다. padding 합 6, MP child와 변경용 sibling을 둔 여섯 조합 모두 아래와 같았습니다. M/A는 실제 child Measure/Arrange producer execution count입니다.

| 조건 | fixed operand 보정 전 | 보정 후 |
|---|---:|---:|
| 최초 Measure+Arrange | 2/1 | 1/1 |
| child 자체 invalidation 후 한 pass | 2/1 | 1/1 |
| sibling 변경 16 pass 합계 | 32/16 | 0/0 |

반복에서 parent producer count는 양쪽 모두 16/16입니다. 보정 전에도 exact comparison과 기본 arithmetic sharing이 적용되어 있었으므로, 이 표는 fixed operand 보정의 효과를 보여줍니다. 전체 patch 이전과의 비교는 아닙니다. MP child의 Arrange extent는 rounding 결과가 바뀔 수 있으며 outer parent rect는 유지됩니다.

### Validation

- Layout 변경 commit: `4981fbcddfef253d5ed47eccb944320b726697a3`. baseline parent commit: `1f794aa393ecd1016df31f672e89d0ceff08706e`. fixed operand 보정 전 비교 commit: `aba5ba801f116772fa38ea3173cd0cb33252f591`.
- document·comment 보완 이전에 GCC 15.2/CMake 3.31의 기존 Debug build에서 production code의 build/install과 public·internal UTC build를 수행했습니다. production code는 `-g -O0`, UTC는 `-O0 --coverage`이며 해당 checkout을 사용했습니다.
- 이전 실행에서 관련 public regression test 833개와 internal cache·invalidation·scale·Recycler regression test 77개, 합계 910개가 통과했습니다. LayoutConstraintCache UTC 20개는 833개에 포함됩니다.
- fixed operand 보정 전 library에서는 새 regression test 3개가 실패했고, 같은 UTC binary와 보정 후 library에서는 모두 통과했습니다.
- internal 기본 build는 기존 TextVisual overload warning으로 실패했습니다. 변경되지 않은 WindowsWarningCoverage/ViewFittingMode 두 test object에만 `-Wno-error=overloaded-virtual`을 추가해 compile한 뒤 build와 77개 실행을 이어갔습니다. repository의 warning 설정은 바꾸지 않았습니다.
- 실제 전체 view-data translation unit을 AArch64 GCC 15.2의 `-O3`/`-Os`로 code generation했으며 두 budget 경로의 `fnmsub`를 확인했습니다. target에서 실행하지는 않았습니다.
- 마지막 Layout review 보완은 6개 파일의 document·comment에 한정됩니다. C++ 3개 파일의 diff가 comment뿐임을 검사했고, `git diff --check`, clang-format 20 검사와 독립적인 static review를 통과했습니다. 계산 코드와 assertion을 바꾸지 않아 build·test는 재실행하지 않았습니다. 해당 Git tree는 document·comment 변경으로 이전 tree와 다릅니다.
- command·revision·hash·testcase별 output과 exit code는 `~/ai/layout-amend-validation`에 보관했습니다. coverage 비율은 validation 근거로 사용하지 않았습니다.

### Limits

- 기본 View에 대한 보정이며 모든 manager의 arithmetic을 통일하지 않습니다. vertical Stack과 1×1 Star Grid, parent·child width MP, fixed natural height 20, margin 0, slot 922.50830078125×22, scale 1.1의 이전 diagnostic에서 horizontal padding 합 0은 parent의 명시적 invalidation 16회 동안 child M/A 0/0, padding 합 2는 32/16이었습니다. child count는 diagnostic output이며 assertion이 아닙니다.
- producer execution count는 해당 fixture의 결과입니다. 전체 Measure API call attempt, application frame time, allocation size·peak heap은 측정하지 않았습니다. MP-axis lazy calculation의 실행시간 개선도 주장하지 않습니다. 전체 patch 전후의 frame time benchmark는 수행하지 않았습니다.
- View field는 늘리지 않습니다. Recycler vector의 spare capacity와 reallocation 중 동시에 존재하는 old/new buffer는 heap 비용입니다. growth의 count 상한 때문에 매번 item을 하나 추가하고 끝까지 다시 채워 `size == capacity == count`에 도달하면 누적 copy 비용은 Θ(n²)로 남을 수 있습니다. count가 줄어도 기존 capacity는 유지됩니다.
- scale 1/inset 0에서 기본 View의 requested width 100/max width 80은 child budget 100과 parent measured width 80을 만듭니다. Arrange는 actual slot 80 또는 100을 따릅니다. 기존 cold calculation을 유지한 policy이며 parent min/max를 child의 fixed-request budget에 새로 적용하지 않습니다.
- setter·validation epsilon, Flex wrapping boundary, transition threshold, manager budget과 Recycler coordinate model은 추가 review 수정에서 바꾸지 않습니다.
- AArch64는 실제 전체 translation unit의 code generation만 확인했습니다. target UTC 실행·다른 compiler/LTO 설정·GUI manual check·sample 실행은 수행하지 않았습니다.
