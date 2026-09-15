# Layout 신규 검증 suite

현재 상태는 작업 중간 저장본입니다. 사용자 요청으로 기존 Layout TC를 복원한 뒤 추가 build/test 없이 commit했습니다. 최신 source와 reviewed contract/MD의 최종 동기화, 일부 fixture 보완 및 실행 검증이 남아 있으므로 전체 suite가 통과한 상태로 취급하지 마십시오.

신규 LR01–LR64만으로 실행하십시오. 기존 Layout TC45–TC94의 C++/MD는 유지합니다. 신규 suite의 판정에 기존 TC 결과를 합산하지 않습니다. 기존 launcher, `tc/*.cpp` 자동 등록, module별 앱 구조는 유지합니다. TC별 MD가 서버 LLM의 실행 명세이며 공통 helper는 버튼·완료 fence·결과 보관만 담당합니다.

## 구성 결정과 검증 계약

| 반드시 만족할 조건 | 적용 방식 |
|---|---|
| 기존 manual-tests 구조 유지 | 같은 foundation 앱에 신규 TC 등록, main 변경 없음 |
| 문제를 재현 가능한 작은 단위로 격리 | core/manager/cache/controller/transition/integration/performance별 TC와 scenario 분리 |
| 기존 Layout TC에 의존하지 않음 | 신규 suite에 default View, custom callback/manager, 4개 manager, standalone, scale/direction, lifecycle 포함 |
| 실제 계산값 판정 | 직접 Measure/Arrange 결과, 실제 Actor target, 완료된 text model, Window fence snapshot, 제한된 내부 trace |
| 구현과 독립적인 expected | 입력 표·합/비율/좌표 식·정해진 상태 전이 및 event 순서 사용 |
| 성능 관측의 간섭 억제 | work-count용 diagnostics ON과 시간용 Release OFF 분리, 측정 구간 밖 검증·출력 |
| LLM이 실행 및 검증 가능 | 고정 버튼 이름, TC/scenario/step/action 식별자, actual/expected/tolerance, exact required_checks, timeout 및 전체 증거 |

하나의 대형 TC로 모든 상태를 공유하면 이전 실패가 다음 fixture에 영향을 줍니다. 따라서 TC는 분리하되 공통 검증 helper와 기존 앱을 공유합니다. 신규 executable이나 서버 runner는 추가하지 않습니다. 단편적인 화면 demo가 아니라 모든 필수 scenario/action/assertion을 실행해야 하는 suite입니다.

유한한 TC 집합으로 가능한 모든 Layout 오류의 부재를 증명할 수는 없습니다. 완료 기준은 아래 지원 경로의 명시적 oracle, 실행 증거, source branch와 assertion 연결, positive-control mutation 및 성능 비교입니다. 단순히 64개 파일이 존재하거나 branch coverage가 높다는 이유로 전체 검증을 통과했다고 기록하지 마십시오.

## 지원 경로와 신규 TC 매핑

| 경로 / 실제 source 기준 | 신규 TC | 주 관측 / oracle |
|---|---|---|
| `internal/views/view/view-data-impl.cpp`: Normalize, requested size/position, min/max, margin/padding, Measure/Arrange dispatch | LR01–07 | 제한값 표, callback 입력·반환, exact/epsilon 경계 |
| effective scale, global scale, inherited LTR/RTL | LR08–09 | scale별 시각 단위, parent-local mirror 식 |
| `public-api/layouts/stack-layout-manager.cpp` | LR10–11 | 고정/weight 잔여 공간, spacing, cross alignment, WRAP/clamp |
| `public-api/layouts/flex-layout-manager.cpp` | LR12–15 | basis/grow/shrink, line 분할, direction/reverse/wrap, justify/align, 실제 glyph baseline |
| `public-api/layouts/grid-layout-manager.cpp` | LR16–18 | absolute/auto/star track, span, clamp/empty/boundary |
| `public-api/layouts/absolute-layout-manager.cpp` | LR19–20 | 고정/비율 축별 독립 식, 음수 위치, wrap |
| standalone / descendant participation | LR21 | parent 계산 제외와 subtree 내부 계산의 분리 |
| Measure cache, Arrange replay/policy | LR22–24 | 실제 producer count, key 경계, 손상된 Actor geometry 복원 |
| generation / dependency owner / out-of-band | LR25–27 | dirty/cache state, ancestor stop 이유, owner stack 정리 |
| View completion, PARK, reentry/exception, replay 중 tree 변경 | LR28–31 | 동일 episode 신호·순서, quiet interval, poison/rollback |
| tree order/reparent/off-scene/Window cleanup | LR32–34 | 각 child target과 새로운 parent 기준값, lifetime 정리 |
| `internal/layouts/layout-transition-dispatcher.cpp` 및 public transition API | LR35–44 | descriptor, scope/cause, bounds/clip, 실제 animation seek·manual tick·natural completion, interruption/exit/focus/lifecycle |
| `public-api/layouts/layout-controller.cpp` | LR45–49 | root constraint, batching, View→Window 완료, lifetime, exception rollback |
| 실제 Label/ImageView resource 및 중첩 manager | LR50–51 | 고정 font/image, resource fence, A→B→C→A 전체 geometry |
| ScrollableBase / ScrollView | LR52 | viewport/content extent, offset/clamp, 실제 scene integration |
| `internal/linear-items-layouter-impl.cpp` | LR53 | visible/cache range, decorated extent, scroll consumption |
| `internal/group-linear-items-layouter-impl.cpp`, GroupAdapter | LR54 | flatten type/group/position, body/header/gap margin |
| 실제 RecyclerView / ItemAdapter | LR55 | stable ID, insert/move/remove/content/size/replacement, Window viewport |
| input coordinate/clip 및 RenderEffect resize | LR56–57 | 실제 touch hit/local position, RenderTask framebuffer texture 크기 |
| seed / metamorphic validation | LR58 | 고정 seed와 독립 arithmetic, 축 교환·RTL·translation |
| construction/cold/warm, sparse/dense/same-value | LR59–60 | 매 표본 전체 geometry + Release raw time |
| 반복 replay/깊이/재진입 | LR61 | 매 cycle 전체 노드, 깊이별 닫힌 식 |
| diagnostics API 자체의 계약 | LR62 | buffer guard/overflow/epoch/ID, snapshot 반복 읽기의 비변이 |
| 복합 tree/reflow workload | LR64 | deep/balanced, Flex wrap/grow/shrink, Grid star/span, variable Recycler, text reflow |
| transition work/time | LR63 | absent/dormant/disabled/active spec/animator의 count 및 Release time |

조건별 source와 실제 assertion 연결은 [core map](layout-validation-core-map.md), [transition/controller map](layout-validation-transition-map.md), [workload/diagnostics map](layout-validation-workload-map.md)에 기록합니다. 이 표는 source 경로와 TC 책임의 매핑입니다. 모든 branch가 실제 실행되었다는 증거표가 아닙니다. `layout-validation-manifest.json`의 execution_evidence는 실행 전 pending입니다. 실행 보고서에는 각 TC/action/assertion의 성공/실패/미실행과 실제 artifact를 별도로 기록하십시오.

## 실행 profile과 build

- **diagnostic**: app와 library 모두 `DALI_UI_LAYOUT_TEST_DIAGNOSTICS`를 정의합니다. library CMake 옵션은 `ENABLE_LAYOUT_TEST_DIAGNOSTICS=ON`입니다. 일반 correctness suite와 LR59/60/63/64의 ON work/geometry 검사를 실행하십시오. Coverage를 켜도 시간 비교에는 사용하지 마십시오.
- **release**: app/library 둘 다 Release이며 diagnostics/coverage를 끕니다. LR59/60/63/64 시간 표본, LR62 OFF artifact 검사를 실행하십시오. 가능한 correctness TC도 같은 profile에서 재실행하되 diagnostics가 필수인 TC의 capability 실패를 계산 회귀로 혼동하지 마십시오.
- app만 macro를 켜고 library는 OFF인 조합은 지원하지 않습니다. `ldd`/process maps의 실제 로딩 경로와 ELF build-ID를 저장하여 설치된 다른 library를 잘못 검증하지 않도록 하십시오.

현재 build는 source 내부에 generated 파일을 만들고 manual binary도 source의 `bin/`에 쓰므로 검증용 복사본에서 빌드하십시오. 공용 install prefix에 설치하거나 `git clean`을 실행할 필요가 없습니다. 다음은 이미 별도로 복사한 `SOURCE`, 쓰기 가능한 `WORK`, dependency prefix가 설정된 환경에서의 예시입니다. 경로는 실행 환경에 맞게 명시하십시오.

```sh
cmake -S "$SOURCE/build/tizen" -B "$WORK/lib-diagnostic" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_DEBUG=OFF \
  -DENABLE_LAYOUT_TEST_DIAGNOSTICS=ON -DENABLE_COVERAGE=OFF
cmake --build "$WORK/lib-diagnostic" --target dali2-ui-foundation dali2-ui-components -j2
cat > "$WORK/source-overlay.cmake" <<'CMAKE'
list(REMOVE_ITEM CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES "${LAYOUT_TEST_SOURCE_ROOT}")
include_directories(BEFORE "${LAYOUT_TEST_SOURCE_ROOT}")
CMAKE
cmake -S "$SOURCE/manual-tests" -B "$WORK/app-diagnostic" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PROJECT_INCLUDE="$WORK/source-overlay.cmake" \
  -DLAYOUT_TEST_SOURCE_ROOT="$SOURCE" \
  -DCMAKE_CXX_FLAGS="-DDALI_UI_LAYOUT_TEST_DIAGNOSTICS" \
  -DCMAKE_EXE_LINKER_FLAGS="-L$WORK/lib-diagnostic/dali-ui-foundation -L$WORK/lib-diagnostic/dali-ui-components -Wl,-rpath,$WORK/lib-diagnostic/dali-ui-foundation:$WORK/lib-diagnostic/dali-ui-components"
cmake --build "$WORK/app-diagnostic" --target manual-test-dali-ui-foundation -j2
```

설치 dependency가 Release인 환경에서는 위처럼 RelWithDebInfo와 ENABLE_DEBUG=OFF를 사용하십시오. Debug build type은 ENABLE_DEBUG를 강제로 켜므로 Release core에 없는 Log::Filter symbol을 참조할 수 있습니다.

생성된 compile command에서 `$SOURCE`가 installed include보다 먼저인지 확인하십시오. Release에서도 같은 source overlay를 해당 복사본 경로로 적용하십시오. `CMAKE_CXX_FLAGS=-I...`만 사용하면 dependency의 명시적 include 뒤에 놓일 수 있습니다.

Release는 별도 source/build 복사본에서 `CMAKE_BUILD_TYPE=Release`, `ENABLE_DEBUG=OFF`, `ENABLE_LAYOUT_TEST_DIAGNOSTICS=OFF`, `ENABLE_COVERAGE=OFF`로 만들고 app CXX flags의 diagnostics macro를 제거하십시오. Coverage ON에서 shader-generator의 coverage runtime link가 필요한 toolchain은 격리 build의 `CMAKE_EXE_LINKER_FLAGS=--coverage`, `CMAKE_SHARED_LINKER_FLAGS=--coverage`도 함께 지정하십시오.

## LLM 실행 절차

1. `python3 layout-validation-tools.py audit`으로 CPP/MD 쌍과 고정 asset hash를 검증하십시오. 이 명령은 앱을 실행하지 않습니다.
2. `FONTCONFIG_FILE`을 실행 source의 `res/layout-validation/fonts.conf` 절대 경로로 설정하십시오. `fc-match`의 실제 font 파일과 hash를 기록하십시오. DPI/scale/해상도/locale도 저장하십시오.
3. 앱 stdout/stderr를 파일로 보존하고 launcher에서 TC ID로 검색하십시오. `Reset run` 한 번 후 모든 scenario를 순서대로 실행하십시오. 버튼 표시는 공통이고 Actor/accessibility name에는 `LRxx.` 접두사가 있습니다.
4. `LR_READY`의 tc/pid/run/scenario/step/action_seq가 현재 action과 일치할 때까지 stdout을 수동적으로 관측하십시오. 이 표시는 action 완료 통지이며 PASS 판정 자체가 아닙니다. timeout은 TC별 MD를 따르십시오. 완료 후 `Read result`를 누르면 **그 action의 모든 행을 stdout으로 출력**합니다. stdout 전체가 확보되면 화면의 모든 페이지를 넘길 필요는 없습니다. stdout을 읽을 수 없으면 `Next result page`로 모든 행을 수집하십시오. 어느 경우든 누락된 행을 통과로 추정하지 마십시오.
5. 직접 계산과 Window snapshot을 구별하십시오. View snapshot의 rect는 parent-local, RTL 적용 후, transition 이전의 target입니다. animation current property는 별도의 settle/seek 관측입니다. screenshot만으로 서브픽셀 geometry를 판정하지 마십시오.
6. 일반 async timeout은 각 TC MD를 따르십시오. PARK의 10초 quiet interval에는 Read result·timer polling·스크린 조작을 하지 마십시오. 수집 시스템이 stdout을 수동적으로 기록하는 것은 가능합니다.
7. 같은 행의 actual/expected/tolerance를 다시 계산하고 required_checks와 실제 행 수의 정확일치, 완료 scenario 수, 누적 실패 수를 확인하십시오. 결과 row는 `(tc,pid,run,scenario,action_seq,row)`로 식별합니다. 반복 출력은 중복 수집이며 추가 검사로 세지 마십시오.
8. 앱이 EXTERNAL_REQUIRED를 내면 external 성능/quiet/artifact 조건이 미완료입니다. missing baseline, unsupported diagnostics, overflow, timeout은 PASS가 아닙니다.
9. `python3 layout-validation-tools.py check-log LOG --tc LRxx --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`로 저장된 수치를 다시 검증할 수 있습니다. 성능 등의 internal evidence에 한해 `--external`을 사용할 수 있으며 이 옵션은 external 판정을 완료하지 않습니다.
10. TC에서 Back으로 나온 뒤 해당 run의 `LR_CASE_END`를 보존하십시오. `cleanup_passed=true`, failures=0, 완료 count가 마지막 `LR_RESULT`와 일치해야 합니다. END 누락·cleanup 예외는 PASS가 아닙니다. stdout와 stderr를 함께 수집하십시오. 재진입하여 초기 상태를 검증하십시오. 전체 suite를 반복할 때 process·Window·timer·signal·font/scale 상태의 누적 영향도 기록하십시오.

`PIN`은 이번 candidate 로그에서 계산하는 값이 아닙니다. 운영자가 별도로 검토한 manifest 파일의 SHA256을 신뢰 저장소에 사전 고정하십시오. `reviewed_contracts.profiles`에는 profile별 TC의 전체 scenario/step/required_checks가 있고, 도구는 app 출력과 exact equality를 요구합니다. 계약 삭제·축소, profile 불일치, hash 불일치, 검토되지 않은 계약은 거부합니다. 계약을 바꾸려면 source/MD 변경을 검토한 뒤 새 pin을 배포하십시오. 파일을 읽어 hash가 같다는 사실만으로 그 파일의 신뢰성을 보장하지 않습니다.

## 독립 oracle, coverage, mutation

- golden expected를 candidate 출력에서 만들지 마십시오. CPP의 arithmetic fixture와 대응 MD의 표/식을 기준으로 판단하십시오. 기존 Layout TC의 PASS를 신규 scenario의 증거로 합산하지 마십시오.
- 새로 추가되는 Layout API/분기는 source→fixture input→action→actual observer→expected 식→assertion→실행 증거로 연결하여 manifest와 TC를 함께 갱신하십시오.
- coverage는 진단 전용 artifact에서 별도 수집하십시오. 관심 범위는 위 표의 Layout/관련 integration source입니다. launcher/HUD가 우연히 실행한 줄은 해당 TC의 oracle이 검증한 분기로 세지 마십시오. 새 source의 미실행 branch에는 재현 fixture 또는 지원하지 않는 계약이라는 근거가 필요합니다.
- positive control은 격리된 복사본에서 한 변이씩 적용하십시오. 예: Stack spacing 항 제거→LR10/LR58, Flex grow 비율 변경→LR12, Grid star 비율 변경→LR16, Absolute `(available-child)*p`를 `available*p`로 변경→LR20, Measure key 비교 제거→LR22, replay geometry write 생략→LR23, RTL mirror 생략→LR09, generation/owner stop 변경→LR25/26, transition progress 적용 생략→LR39/41, recycler spacing 제거→LR53, resource invalidation 누락→LR50.
- 각 변이는 원본의 **같은 assertion이 PASS**하고 변이에서 **예정한 actual 불일치로 FAIL**해야 kill입니다. compile failure·환경 crash·이미 원본에서 실패하던 baseline assertion은 mutation kill로 세지 마십시오. 원본 source는 수정하지 말고 patch/revision/log를 보고서에 저장하십시오.
- LR58의 seed 실패는 seed와 실현된 입력 node 목록을 보존하십시오. `shrink-candidates`는 한 node를 삭제한 후보 JSON을 만들 뿐입니다. 후보를 TC에서 재현하여 동일 assertion 실패가 유지되는지 확인한 뒤에만 축소된 재현이라고 기록하십시오. 실패를 재현하지 않은 후보는 `reproduced=false`입니다.

## 성능과 관측자 비용

LR59/60은 시간 종류를 분리하고 LR63은 transition 상태별 work/time을 분리합니다. reference/candidate의 같은 workload 계약과 매 표본 geometry 검증이 선행되어야 합니다. 여러 manager/N/phase를 합쳐 회귀를 숨기지 마십시오.

[성능 계획 예시](layout-validation-performance-plan.example.json)는 LR59/60의69 metric family입니다. 실행 전에 device의 timer resolution을 측정하여 timer_floor_ns, precision, MDE, family, seed를 고정하십시오. 독립 process pair 31/62/93, AB/BA 균형, family 및 3회 관측 보정 CI를 사용합니다. [LR63 계획 예시](layout-validation-transition-performance-plan.example.json)의 15개 mode/N/phase와 [LR64 계획 예시](layout-validation-complex-performance-plan.example.json)의 16개 family/N도 제공합니다. 각 plan의 TC만 포함하는 별도 실험 로그로 비교하거나, 실행 전에 전체 metric family를 합친 단일 plan을 별도로 고정하십시오. 같은 process의31개 내부 sample은 독립 process pair가 아닙니다.

`compare`의 plan에는 신뢰 저장소의 `contract_manifest` 경로와 `contract_manifest_sha256`도 고정하십시오. fixture 변경 시 reference/candidate 모두 같은 검토된 계약을 실행해야 합니다.

`compare`의 EXPERIMENT JSON에는 `plan_sha256`와 `blocks`를 넣으십시오. 각 block은 `pair_id`, `order`(AB/BA), `reference_log`, `candidate_log`, 동일한 `environment_reference`/`environment_candidate`, 두 artifact fingerprint를 포함합니다. environment에는 device/cpu_affinity/governor/resolution/dpi/assets_sha256/compiler_flags가 필요합니다. artifact에는 app_sha256/library_sha256/fixture_contract_sha256, diagnostics=false, coverage=false, build_type=Release를 넣으십시오. path는 분석 실행 위치에서 resolve되므로 절대 경로를 권장합니다.

CI 하한>1이면 작은 slowdown도 FAIL입니다. 그 외 상한≤1+MDE 및 precision을 만족할 때만 해당 검출 해상도 내 regression 미검출입니다. bounded sample 후에도 불확실하면 INDETERMINATE입니다. 검정 결과를 보고 MDE를 늘리지 마십시오.

진단 OFF는 아래를 별도로 확인하십시오.

- 원본/변경본의 exported dynamic symbol 집합과 ViewImpl 등 ABI 크기, 관련 function disassembly를 같은 compiler/flags에서 비교하십시오. diagnostics symbol/storage/call이 OFF artifact에 없어야 합니다.
- 진단 ON의 고정 POD buffer는 capture 과정에서 동적 allocation을 하지 않도록 설계했습니다. LR62의 snapshot/event 비변이 검증만으로 무할당을 증명하지 마십시오. 별도 검증 process에서 선택한 allocator profiler로 관측자 함수 호출 stack 안의 allocation만 검사하십시오. 관측 전 준비·workload 자체 allocation은 별도 기록하고, profiling 결과를 무계측 timing 표본에 섞지 마십시오. 도구가 없거나 symbol/stack을 식별하지 못하면 미실행입니다. production allocator나 상시 hook을 변경하지 마십시오.
- `SNAPSHOT_ALLOCATION`은 `StorageSite` enum에 명시한 vector 생성/reserve 지점의 element count/requested bytes입니다. process 전체 malloc/RSS 또는 실제 allocator usable size가 아닙니다.
- 일반 Layout hot path에 OFF hook overhead가 없는지 원본과 같은 Release workload로 비교하십시오. ON/OFF 시간 차이를 library 회귀와 혼동하지 마십시오.

## 실행 보고서

보고서에는 fixture/source/asset fingerprint, profile, 전체 scenario/action 목록, 실패와 미실행 구분, 최초 불일치 수치, coverage attribution, mutation kill/미검증 목록, 성능 CI/정밀도/결측값을 남기십시오. 새 suite가 현재 제품 오류를 발견하면 expected를 낮추거나 xfail로 숨기지 마십시오. product fix는 별도 변경으로 추적하고 동일 신규 TC를 다시 실행하십시오.
