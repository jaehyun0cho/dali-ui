# LR53. Layout: LinearItemsLayouter

이 문서는 [tc-lr53-layout-linear-recycler.cpp](tc-lr53-layout-linear-recycler.cpp)의 실행 절차입니다. 기존 Layout TC 결과는 합산하지 마십시오. 전체 실행 조건과 profile은 [suite 검증 기준](../layout-validation-coverage.md)을 먼저 읽으십시오.

## 실행 및 완료 확인

1. app와 실제 resolve된 library의 build-ID, revision, profile, asset SHA-256을 기록하고 stdout을 저장하십시오. launcher에서 `LR53`를 검색하여 해당 TC에 진입하십시오.
2. `LR53.Reset run`을 한 번 실행하고 새 `run`을 기록하십시오. scenario 내 step 사이에는 Reset하지 마십시오.
3. `Apply next step`을 한 번 누른 뒤 `LR_ACTION`의 scenario/step/action_seq/required_checks를 기록하십시오. 동기 step은 즉시 완료됩니다. Window/resource step은 해당 episode의 완료까지 기다리십시오. 10초 내 완료되지 않으면 PENDING을 PASS로 처리하지 마십시오. 성능 sample step만 최대120초를 허용합니다.
4. `Read result`를 누르고 `Next result page`로 **모든** 행을 읽으십시오. page1만 확인하지 마십시오. 행의 tc/pid/run/scenario/step/action_seq를 연결하여 다른 실행의 값과 혼합하지 마십시오. 같은 행을 다시 읽으면 값이 같아야 합니다.
5. 해당 step의 실제 행 수와 required_checks가 정확히 같아야 합니다. 일반 rect 허용오차는0.001, 정수/bool/ID는 exact입니다. NaN/Inf, 누락/잘림, 중복 읽기로 채운 행 수, unexpected exception, crash, timeout, cleanup error는 FAIL입니다. `passed`만 믿지 말고 actual/expected/tolerance를 재계산하십시오.
6. 모든 step 후 `Next scenario`로 이동하고 모든 scenario를 실행하십시오. 마지막 `LR_RESULT`의 completed_scenarios=required_scenarios, failures=0 및 전체 행 증거가 필요합니다. 실패 후 Reset으로 실패 이력을 지우지 마십시오.

## 입력과 독립 기대값

| scenario | 주요 독립 기대값 | 검사 수 |
|---|---|---:|
| vertical, horizontal | item 10개, extent20, gap2 → range218, viewport50 | 각각 35 |
| cache-window | before/after22, 초기 materialized0..3, offset44에서1..5, range218 | 8 |
| variable-decoration | heights10/30/20, offsets3/7/2/4, gap5 → range88 | 22 |
| empty-zero-viewport | range0, offset0, scroll consumption0 및 빈 경계 | 6 |

각 scenario의 `run`를 실행하십시오. fixed vertical은 initial item0..2의 rect=(0,0/22/44,120,20), horizontal은 (0/22/44,0,20,120)입니다. offset26에서 first1/last3, 큰 양의 scroll의 추가 소비량142 및 offset168/first7/last9, 경계에서 추가 소비0, 큰 음의 scroll 소비−168입니다. 다른 축의 scroll 소비는0입니다. item4 content bounds main offset은88입니다. 마지막은 first0/last2입니다.

variable-decoration에서 item rect는 (3,2,110,10),(3,23,110,30),(3,64,110,20), item1 slot=(0,21,120,36)입니다. 음수 item extent는1, 음수 spacing은0으로 제한됩니다. 방향 변경 후 offset0과 first0을 확인하십시오.

이 TC는 built-in layouter를 통제된 Recycler bridge로 검증합니다. 실제 RecyclerView의 adapter binding·viewport lifecycle 증거는 LR55에서 별도로 수집하십시오. 여기서 GetView/Recycle callback을 검사했다는 이유로 실제 RecyclerView lifecycle도 통과했다고 기록하지 마십시오.

## 종료와 증거

`< Back`으로 나간 뒤 다시 진입하여 첫 step을 재실행하십시오. 이전 timer/callback/fixture/추가 Window가 남아 있으면 FAIL입니다. 결과에는 scenario/input/step, 실제값과 기대값, 최초 실패 행, 전체 stdout, artifact fingerprint를 첨부하십시오. `python3 ../layout-validation-tools.py check-log LOG --tc LR53`은 저장된 행의 누락과 수치를 다시 검사하는 보조 수단입니다. 이 도구는 앱을 실행하거나 서버의 UI 자동 조작을 대체하지 않습니다.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 5 scenario, 5 action, 106 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `vertical` | `run` : 35 |
| `horizontal` | `run` : 35 |
| `cache-window` | `run` : 8 |
| `variable-decoration` | `run` : 22 |
| `empty-zero-viewport` | `run` : 6 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR53 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
