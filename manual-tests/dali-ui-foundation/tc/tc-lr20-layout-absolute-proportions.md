# LR20. Layout: Absolute proportions

이 TC는 신규 Layout suite의 **Absolute proportions** 검증입니다. 대응 소스는 `tc-lr20-layout-absolute-proportions.cpp`입니다. 기존 TC의 실행 결과를 사용하지 않습니다.

## 실행 조건

- foundation manual-test 앱에서 `LR20`를 검색하고 표시명이 일치하는 TC에 진입하십시오.
- 다른 입력·animation을 중지하고 `Reset run`을 한 번 실행하십시오. 첫 scenario와 새로운 `run_id`를 기록하십시오.
- 기본 수치는 scale=1, LTR, parent-local 좌표이며 좌표·extent 단위는 visual unit입니다. 각 scenario가 scale/direction을 바꾸면 아래 명시값을 사용하십시오.
- 수치 actual은 TC의 결과 행 또는 같은 run의 stdout 원문에서 읽으십시오. screenshot에서 위치를 눈대중으로 추정하지 마십시오.
- diagnostics가 필요한 단계는 diagnostics ON build가 필요합니다. capability가 없거나 required row가 누락되면 PASS로 처리하지 마십시오.

## 조작 및 판정

1. 현재 `scenario_id`와 `action_seq`를 기록하고 `Apply next step`을 한 번 누르십시오.
2. 동기 계산은 같은 action에서 끝납니다. Window 단계는 해당 action의 View snapshot과 Window 완료 fence를 기다립니다. 10초 안에 완료되지 않으면 FAIL과 마지막 출력·화면을 기록하십시오. 특별한 quiet/시간 조건은 아래에서 우선합니다.
3. `Read result`를 누르고 해당 action의 전체 결과를 읽으십시오. `Next result page`는 결과 페이지 이동이며 `Next scenario`와 다릅니다.
4. `required_checks`와 실행 검사 수가 같고 0보다 크며, 모든 필수 행의 actual/expected/오차가 일치해야 해당 action을 통과시킵니다. 누적 실패가 하나라도 있으면 전체 TC를 PASS로 기록하지 마십시오.
5. 단계가 더 있으면 `Apply next step`을 누르십시오. scenario 완료 후 `Next scenario`로 이동하여 목록의 모든 scenario를 실행하십시오. 생략한 scenario를 통과로 추정하지 마십시오.
6. 끝에서 run/scenario/action, 필수·실행 행 수, 모든 실패 행, stdout 또는 결과 화면 증거를 저장하십시오.

## 시나리오와 독립 기대값

- `A04.flags.0..15`: parent200×100, raw bounds(.25,.5,.5,.25)입니다. W bit이면 width100, 아니면.5, H bit이면 height25, 아니면.25입니다. X bit이면 x=(200−width)×.25, Y bit이면 y=(100−height)×.5입니다. 16조합을 전부 실행하십시오.
- `A08.position.*`: parent300, child100이므로 x=(300−100)×p=200p입니다. p−.25/0/.5/1/1.25는 유효 입력이며 RTL은200−x입니다.
- `A05.all-flags`: ALL은 모든 현재 proportional bit를 적용하여(50,25,100,50)입니다.

`Rect`는 x/y/width/height의 네 필수 행, `Size`는 width/height의 두 필수 행입니다. 일반 geometry 허용오차는 0.001이며 정수·순서·bool·별도로 지정한 exact 항목은 정확히 일치해야 합니다. NaN/Inf는 일반 geometry 비교에서 실패합니다. expected는 입력 표·식에서 계산하며 candidate 출력으로 갱신하지 마십시오.

- `A06.wrap-circularity`: position만 proportional이면 determinate width40을 WRAP에 기여합니다. width 자체를 parent의.5로 바꾸면 순환하는 width 기여만 제외하여0×20입니다.

## 근거와 검증 경계

proportional 위치는 전체 parent 크기가 아니라 child extent를 뺀 남은 공간의 비율입니다. 유효 domain 밖으로 오인하여 음수 위치를 clamp하지 않습니다.

직접 `Measure`/`Arrange`의 반환과 Actor event-side target을 검사합니다. `Arrange` 반환은 pre-RTL logical이고 child Actor 좌표는 부모의 RTL 적용 결과입니다. Window snapshot은 실제 settle 결과이며 animation 중간 화면과 혼동하지 마십시오. callback 측정 횟수는 명시한 synthetic producer의 횟수입니다.

## 종료와 재현

`< Back`으로 나간 뒤 같은 TC에 다시 진입하고 `Reset run` 이후 첫 scenario를 반복하십시오. run ID·counter가 새로 시작하고 초기 기대값이 같아야 합니다. 실패는 scenario/input/action/actual/expected를 함께 보존하십시오. 기존 Layout TC 실행으로 대체하지 마십시오.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 23 scenario, 23 action, 112 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `A04.flags..0` | `run` : 4 |
| `A04.flags..1` | `run` : 4 |
| `A04.flags..2` | `run` : 4 |
| `A04.flags..3` | `run` : 4 |
| `A04.flags..4` | `run` : 4 |
| `A04.flags..5` | `run` : 4 |
| `A04.flags..6` | `run` : 4 |
| `A04.flags..7` | `run` : 4 |
| `A04.flags..8` | `run` : 4 |
| `A04.flags..9` | `run` : 4 |
| `A04.flags..10` | `run` : 4 |
| `A04.flags..11` | `run` : 4 |
| `A04.flags..12` | `run` : 4 |
| `A04.flags..13` | `run` : 4 |
| `A04.flags..14` | `run` : 4 |
| `A04.flags..15` | `run` : 4 |
| `A08.position.-0.250000` | `run` : 8 |
| `A08.position.0.000000` | `run` : 8 |
| `A08.position.0.500000` | `run` : 8 |
| `A08.position.1.000000` | `run` : 8 |
| `A08.position.1.250000` | `run` : 8 |
| `A05.all-flags` | `run` : 4 |
| `A06.wrap-circularity` | `run` : 4 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR20 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
