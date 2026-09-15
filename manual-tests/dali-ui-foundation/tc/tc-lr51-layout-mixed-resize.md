# LR51. Layout: 혼합 Layout resize

이 문서는 [tc-lr51-layout-mixed-resize.cpp](tc-lr51-layout-mixed-resize.cpp)의 실행 절차입니다. 기존 Layout TC 결과는 합산하지 마십시오. 전체 실행 조건과 profile은 [suite 검증 기준](../layout-validation-coverage.md)을 먼저 읽으십시오.

## 실행 및 완료 확인

1. app와 실제 resolve된 library의 build-ID, revision, profile, asset SHA-256을 기록하고 stdout을 저장하십시오. launcher에서 `LR51`를 검색하여 해당 TC에 진입하십시오.
2. `LR51.Reset run`을 한 번 실행하고 새 `run`을 기록하십시오. scenario 내 step 사이에는 Reset하지 마십시오.
3. `Apply next step`을 한 번 누른 뒤 `LR_ACTION`의 scenario/step/action_seq/required_checks를 기록하십시오. 동기 step은 즉시 완료됩니다. Window/resource step은 해당 episode의 완료까지 기다리십시오. 10초 내 완료되지 않으면 PENDING을 PASS로 처리하지 마십시오. 성능 sample step만 최대120초를 허용합니다.
4. `Read result`를 누르고 `Next result page`로 **모든** 행을 읽으십시오. page1만 확인하지 마십시오. 행의 tc/pid/run/scenario/step/action_seq를 연결하여 다른 실행의 값과 혼합하지 마십시오. 같은 행을 다시 읽으면 값이 같아야 합니다.
5. 해당 step의 실제 행 수와 required_checks가 정확히 같아야 합니다. 일반 rect 허용오차는0.001, 정수/bool/ID는 exact입니다. NaN/Inf, 누락/잘림, 중복 읽기로 채운 행 수, unexpected exception, crash, timeout, cleanup error는 FAIL입니다. `passed`만 믿지 말고 actual/expected/tolerance를 재계산하십시오.
6. 모든 step 후 `Next scenario`로 이동하고 모든 scenario를 실행하십시오. 마지막 `LR_RESULT`의 completed_scenarios=required_scenarios, failures=0 및 전체 행 증거가 필요합니다. 실패 후 Reset으로 실패 이력을 지우지 마십시오.

## 입력과 독립 기대값

두 scenario를 각각 순서대로 실행하십시오.

| scenario | step 순서 | 각 step 검사 수 |
|---|---|---|
| mixed-roundtrip | width-160 → width-263 → width-512 → return-160 | 62,62,62,124 |
| window-roundtrip | mount-160 → resize-263 → resize-512 → resize-160 | 60,60,60,60 |

입력 root width W=160,263,512,160, padding(start,end,top,bottom)=(7,11,5,9), spacing=13입니다. C=W−18이며 전체 높이는 5+90+13+60+13+70+9=260입니다. direct root Measure=(W,260), Arrange=(5,7,W,260), Window root=(0,0,W,260)입니다.

- Grid=(7,5,C,90), Flex=(7,108,C,60), Absolute=(7,181,C,70)입니다.
- Grid의 U=(C−48)/3, 열 x=(0,44,48+U), width=(40,U,2U), 행 y=(0,23), height=(20,67)입니다. cell 0~5 전부 검증하십시오.
- Flex의 G=(C−90)/2, item rect는 (0,25,20,10), (20+G,25,30,10), (C−40,25,40,10)입니다.
- Absolute proportional=((C−20)/2,60,20,10), negative=(-7,4,30,12)입니다.
- return-160은 기존 tree와 fresh tree를 각각 검사합니다. width roundtrip에서 누적 drift가 없어야 합니다.

Window 단계는 root 및 14개 descendants의 동일 완료 episode snapshot을 수집합니다. child 값은 parent-local이므로 root offset을 더하지 마십시오. screenshot 좌표로 숫자를 대신하지 마십시오.

## 종료와 증거

`< Back`으로 나간 뒤 다시 진입하여 첫 step을 재실행하십시오. 이전 timer/callback/fixture/추가 Window가 남아 있으면 FAIL입니다. 결과에는 scenario/input/step, 실제값과 기대값, 최초 실패 행, 전체 stdout, artifact fingerprint를 첨부하십시오. `python3 ../layout-validation-tools.py check-log LOG --tc LR51`은 저장된 행의 누락과 수치를 다시 검사하는 보조 수단입니다. 이 도구는 앱을 실행하거나 서버의 UI 자동 조작을 대체하지 않습니다.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 2 scenario, 8 action, 550 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `mixed-roundtrip` | `width-160` : 62 → `width-263` : 62 → `width-512` : 62 → `return-160` : 124 |
| `window-roundtrip` | `mount-160` : 60 → `resize-263` : 60 → `resize-512` : 60 → `resize-160` : 60 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR51 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
