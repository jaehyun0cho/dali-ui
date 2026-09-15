# LR64. 복합 Layout 계산 성능

[tc-lr64-layout-complex-performance.cpp](tc-lr64-layout-complex-performance.cpp)의 신규 fixture만 실행합니다. 기존 Layout TC 실행 결과로 대체하지 마십시오. 공통 profile·증거 규칙은 [suite 검증 기준](../layout-validation-coverage.md), baseline 비교 방법은 [LR59](tc-lr59-layout-compute-performance.md)의 성능 비교 절차를 사용합니다.

## 실행 방법

1. 실행 전에 reference/candidate app와 resolve된 library의 revision/build-ID, compiler flags, diagnostic 옵션, asset SHA-256, device/affinity/governor/해상도/DPI를 저장하십시오. 현재 fixture revision이 같아야 합니다. [LR64 성능 계획 예시](../layout-validation-complex-performance-plan.example.json)의 16 metric family를 사용하고 alpha, precision, MDE, timer floor, AB/BA pair 순서를 candidate 실행 전에 고정하십시오.
2. launcher에서 `LR64`에 진입하고 `Reset run`을 한 번 실행합니다. 새 run ID를 기록하십시오. `Apply next step`을 한 번 누르고 LR_ACTION의 scenario/step/action_seq/required_checks를 기록하십시오.
3. Release OFF에서는 scenario별 `timing` step을 실행합니다. ON에서는 `workload-correctness`를 실행하고, text-reflow의 두 scenario는 먼저 `font-identity`를 실행합니다. ON의 측정값은 timing sample로 출력하지 않습니다. 일반 manager의 producer/hit/replay/named-storage exact gate는 LR59/60의 ON 실행 증거를 별도로 결합하십시오. LR64의 ON geometry 검사만으로 작업량 검사를 수행했다고 표시하지 마십시오.
4. OFF sample step은 최대120초, ON 동기 step은10초, font-identity는 해당 Label의 실제 Layout completion을 기다립니다. 시간 초과·PENDING·unexpected exception·crash는 PASS가 아닙니다.
5. `Read result`와 `Next result page`로 모든 행을 읽고 stdout 원문과 연결합니다. actual/expected/tolerance를 직접 대조하며 NaN/Inf, 누락·잘림, 다른 run의 행 혼합, 행 수 부족, cleanup 오류는 FAIL입니다. 같은 행을 반복 읽어 required_checks를 채우지 마십시오.
6. `Next scenario`로 아래16개 scenario를 모두 실행합니다. ON은 font-identity2개를 포함하여18 step, OFF는16 step입니다. 종료 후 다시 진입하여 첫 step을 재실행하고 이전 fixture/callback이 남지 않는지 확인하십시오.

## 독립 입력과 기대값

각 family를 N=16/128에서 실행합니다. deep의 N은 root 아래 depth, balanced의 N은 leaf 수입니다. 계산 트리의 모든 중간 container도 검증 대상입니다. expected 배열 생성, fixture 생성, 검증, 로그는 측정 구간 밖이고, 측정 구간은 setter 또는 cross constraint 변경과 실제 Measure/Arrange 또는 ItemsLayouter::OnLayoutChildren입니다. 첫10회 warm-up 뒤31개 sample을 저장하며, 각 sample은1 operation입니다.

| family | 왕복 입력 | 수치 oracle |
|---|---|---|
| deep-dirty | 마지막 leaf height20↔21 | root를 포함한 N+1개 node의 width120, height20/21, local x=y=0 |
| balanced-dirty | 오른쪽 끝 leaf height20↔21 | binary vertical Stack, 간격2. k leaf subtree 높이22*k−2에 변경 branch만+1. 두 번째 child local y는 첫 subtree 높이+2. root height22*N−2 또는+1 |
| flex-wrap | root width40↔50 | basis10, height16, grow/shrink0. columns4/5, item i의 x=10*(i%columns), y=16*floor(i/columns), width10/height16 |
| flex-grow | root width12*N↔16*N | basis8, grow weight1/3 교대, 총 weight2*N. item width=8+2*weight 또는8+4*weight, x는 앞 item width의 누적 합 |
| flex-shrink | root width12*N↔8*N | basis16, shrink weight1/3 교대. item width=16−2*weight 또는16−4*weight, x는 누적 합 |
| grid-star-span | root width250↔375 | columns STAR1/2/1/1, N개의 STAR1 row, root height20*N. unit=width/5. 각 row의 첫 item=(0,20*row,3*unit,20), 둘째=(3*unit,20*row,unit,20), 마지막 item=(4*unit,0,unit,20*N)이며 전체 row span |
| variable-recycler | cross120↔160 | 실제 LinearItemsLayouter integration API, item height10/20/30 반복, decoration left3/right7/top2/bottom4, spacing5. item x3,width=cross−10,y=앞 item들의(height+11) 합+2. range=각(height+6)의 합+5*(N−1), offset0, first0,lastN−1. content bounds도 decoration 포함한 별도 rect로 검사 |
| text-reflow | fixed width25↔40 | 고정 DejaVu Sans24px, 문자열 A를 N회, CHARACTER wrap. 한 line에1개/2개 glyph, hinted ascender23+descender6=29px, height=29*ceil(N/columns). requested height WRAP_CONTENT. Measure 반환값과 즉시 Arrange rect를 검사 |

각 sample의 `sample.immediate.*`는 scalar values 개수, mismatch0, maximum_error0의3행입니다. 모든 Measure/Arrange 반환값과 즉시 Actor geometry를 다음 Layout 호출 전에 읽습니다. 보정용 Layout을 실행한 뒤 오류를 숨기는 방법은 허용하지 않습니다. 마지막 sample에는 모든 node rect의 x/y/width/height를 개별 행으로 출력합니다. root가 없는 Recycler는 item과 content bounds를 직접 검증합니다.

| family | 최종 detail 행 D | ON step required_checks | OFF step required_checks |
|---|---:|---:|---:|
| deep-dirty |4*(N+1)|93+D|124+D|
| balanced-dirty |4*(2*N−1)|93+D|124+D|
| Flex 세 family |4*(N+1)|93+D|124+D|
| grid-star-span |4*(2*N+2)|93+D|124+D|
| variable-recycler |4*N|93+D|124+D|
| text-reflow |4|97|128|

Text ON `font-identity`는5행입니다. completed model valid/ready, glyph index36, 실제 resolved font의 canonical path가 `res/layout-validation/DejaVuSans.ttf`, ascender23와 descender−6을 확인합니다. path/hash가 다른 font로 바뀌면 성능 baseline을 재사용하지 마십시오. text shaping 엔진 전체 기능의 검증 결과로 확대하지 마십시오.

## 성능 판정

OFF stdout에는 `LR_SAMPLE`의 metric `LR64.<family>.n<N>.update`, iteration0..30, operations1과 raw nanoseconds가 있어야 합니다. 진단·coverage 빌드 시간을 Release baseline과 섞지 마십시오. 이 TC는 Layout 계산 비용을 측정하며 window presentation latency나 Recycler 전체 adapter/render throughput을 측정하지 않습니다. variable Recycler의 모든 item이 viewport100000 안에 있어 계산 대상이 고정됩니다.

같은 environment와 logical fixture의 reference/candidate 독립 process AB/BA pair를31→62→최대93개 수집하고 공통 compare 도구로 family/반복 관측 보정 CI를 계산합니다. 유의한 slowdown(CI 하한>1)은 작은 값이어도 먼저 FAIL입니다. CI 상한≤1+사전 MDE이고 half-width가 precision을 충족할 때만 해당 검출 정밀도에서 regression 미검출 PASS입니다. MDE는 허용 slowdown 비율이 아닙니다. timer floor 이하 sample, 5%를 넘는 noise 등 사전 환경 기준 위반은 환경을 정리해 bounded 재측정하며 최대 횟수 후에는 INDETERMINATE입니다. reference 누락·조건 불일치·미완료 외부 검정을 내부 geometry PASS에 합산하지 마십시오.

최종 PASS에는 LR64 전체 ON 수치/font 증거, LR59/60 ON exact work-count 증거, LR64 OFF의 모든 metric에 대한 유효한 reference/candidate 비교 증거가 필요합니다. 앱의 `EXTERNAL_REQUIRED`는 외부 비교가 아직 필요하다는 뜻입니다. `python3 ../layout-validation-tools.py check-log LOG --tc LR64 --external`은 저장된 행을 재검사하는 보조 수단이며 UI 실행을 대신하지 않습니다.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic`: 16 scenario, 18 action, 6730 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `deep-dirty-n16` | `workload-correctness` : 161 |
| `deep-dirty-n128` | `workload-correctness` : 609 |
| `balanced-dirty-n16` | `workload-correctness` : 217 |
| `balanced-dirty-n128` | `workload-correctness` : 1113 |
| `flex-wrap-n16` | `workload-correctness` : 161 |
| `flex-wrap-n128` | `workload-correctness` : 609 |
| `flex-grow-n16` | `workload-correctness` : 161 |
| `flex-grow-n128` | `workload-correctness` : 609 |
| `flex-shrink-n16` | `workload-correctness` : 161 |
| `flex-shrink-n128` | `workload-correctness` : 609 |
| `grid-star-span-n16` | `workload-correctness` : 229 |
| `grid-star-span-n128` | `workload-correctness` : 1125 |
| `variable-recycler-n16` | `workload-correctness` : 157 |
| `variable-recycler-n128` | `workload-correctness` : 605 |
| `text-reflow-n16` | `font-identity` : 5 → `workload-correctness` : 97 |
| `text-reflow-n128` | `font-identity` : 5 → `workload-correctness` : 97 |

Profile `release`: 16 scenario, 16 action, 7216 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `deep-dirty-n16` | `timing` : 192 |
| `deep-dirty-n128` | `timing` : 640 |
| `balanced-dirty-n16` | `timing` : 248 |
| `balanced-dirty-n128` | `timing` : 1144 |
| `flex-wrap-n16` | `timing` : 192 |
| `flex-wrap-n128` | `timing` : 640 |
| `flex-grow-n16` | `timing` : 192 |
| `flex-grow-n128` | `timing` : 640 |
| `flex-shrink-n16` | `timing` : 192 |
| `flex-shrink-n128` | `timing` : 640 |
| `grid-star-span-n16` | `timing` : 260 |
| `grid-star-span-n128` | `timing` : 1156 |
| `variable-recycler-n16` | `timing` : 188 |
| `variable-recycler-n128` | `timing` : 636 |
| `text-reflow-n16` | `timing` : 128 |
| `text-reflow-n128` | `timing` : 128 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR64 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
