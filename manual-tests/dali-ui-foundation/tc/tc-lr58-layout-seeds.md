# LR58. Layout: Deterministic seeds

이 TC는 신규 Layout suite의 **Deterministic seeds** 검증입니다. 대응 소스는 `tc-lr58-layout-seeds.cpp`입니다. 기존 TC의 실행 결과를 사용하지 않습니다.

## 실행 조건

- foundation manual-test 앱에서 `LR58`를 검색하고 표시명이 일치하는 TC에 진입하십시오.
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

- `R01.seed.1/17/73/257/65537`: LCG `state=state*1664525+1013904223`를 unsigned32 wrap으로 갱신하여 width=1+state%47, height=1+state%31의16개 child를 만듭니다. seed는 실행 전 고정됩니다.
- intrinsic width=Σwidth+3×15, height=max(height)입니다. i번째 x=앞선 width의 합+3i입니다. vertical tree는 입력 두 축을 교환하며 y가 같은 합이어야 합니다. 각 seed 필수130행을 모두 읽으십시오.
- R06–R08의 실패 축소·의도적 결함 주입·source coverage는 suite 보조 도구의 별도 실행 증거입니다. 이 TC의 PASS가 보조 검증까지 수행했다는 뜻은 아닙니다. 원본이 같은 assertion을 PASS하고 mutant가 예상 사유로 FAIL한 경우만 검출력 증거로 집계하십시오.

`Rect`는 x/y/width/height의 네 필수 행, `Size`는 width/height의 두 필수 행입니다. 일반 geometry 허용오차는 0.001이며 정수·순서·bool·별도로 지정한 exact 항목은 정확히 일치해야 합니다. NaN/Inf는 일반 geometry 비교에서 실패합니다. expected는 입력 표·식에서 계산하며 candidate 출력으로 갱신하지 마십시오.

추가 manager별 고정 seed는1/73/65537이며 LCG 식은 동일합니다. 다음 9개 scenario도 각각 실행하십시오.

- `R02.flex-seed.*`: 8개 basis width=1+state%47, height=1+state%31, grow1입니다. container main=Σbasis+120이므로 각 child width=basis+15입니다. forward prefix sum, ROW_REVERSE, ROW_REVERSE+RTL의 이중 반전, parent(31,17) 이동 후 local 좌표 보존, COLUMN으로 축 교환을 각각32행씩 검사합니다. 필수160행입니다.
- `R04.grid-seed.*`: rows4개=20+state%20, columns3개=15+state%20, row gap3/column gap5입니다. 각 cell 좌표는 prefix track sum+앞선 gap입니다. 12 cell의 LTR/RTL/parent 이동/행열 전치 결과를 각각48행씩 검사합니다. 필수192행입니다.
- `R05.absolute-seed.*`: 12 child의 width1..47/height1..31과 고정식 위치를 생성합니다. 홀수 child만 X_PROPORTIONAL이며 p=(state%7−1)/4이고 x=(200−width)p입니다. 짝수는 x=state%50−10, 모든 y=state%30−5입니다. LTR/parent 이동/RTL x=200−x−width/축 전치의 네 묶음48행씩, 필수192행입니다. 전치에서는 X_PROPORTIONAL을 Y_PROPORTIONAL로 교환합니다.

이 seed 입력은 각 manager의 명시한 유효 domain을 검증합니다. 실행하지 않은 seed나 다른 입력 domain의 conformance까지 일반화하지 마십시오.

## 근거와 검증 경계

독립 prefix sum oracle과 axis 교환 관계를 고정 seed로 재현합니다. candidate 출력이나 현재 배치 결과를 expected로 저장하지 않습니다.

직접 `Measure`/`Arrange`의 반환과 Actor event-side target을 검사합니다. `Arrange` 반환은 pre-RTL logical이고 child Actor 좌표는 부모의 RTL 적용 결과입니다. Window snapshot은 실제 settle 결과이며 animation 중간 화면과 혼동하지 마십시오. callback 측정 횟수는 명시한 synthetic producer의 횟수입니다.

## 종료와 재현

`< Back`으로 나간 뒤 같은 TC에 다시 진입하고 `Reset run` 이후 첫 scenario를 반복하십시오. run ID·counter가 새로 시작하고 초기 기대값이 같아야 합니다. 실패는 scenario/input/action/actual/expected를 함께 보존하십시오. 기존 Layout TC 실행으로 대체하지 마십시오.

## 축소 후보의 별도 재현 절차

이 TC는 JSON을 런타임에 불러오지 않습니다. 자동 suite의 PASS 조건과 실패 분석용 fixture 변경을 구분하십시오. LLM agent가 축소를 수행하려면 원본을 보존한 별도 source/build 복사본을 사용하십시오.

1. 실패한 seed를 위 LCG로 다시 전개하여 실제 입력만 `{"tc":"LR58","scenario":"R01.seed.73","assertion":"horizontal.3.x","seed":73,"nodes":[{"id":0,"width":12,"height":7}]}` 형태로 저장하십시오. 예시 node 수치는 설명용이며 실패 seed의 입력으로 교체해야 합니다. actual Layout 출력은 입력이나 expected에 넣지 마십시오.
2. `python3 layout-validation-tools.py shrink-candidates failure.json > candidates.json`을 실행하십시오. 도구가 만든 `reproduced=false`는 미검증 후보입니다.
3. 별도 복사본의 해당 scenario에서 LCG 생성 loop만 후보 nodes의 명시적 width/height 목록으로 교체하십시오. 두 tree에 같은 목록을 쓰고 node의 원래 id를 assertion 이름으로 보존하십시오. R01의 `total += 45`는 `3*max(N-1,0)`, 두 `i<16`은 `i<N`, required_checks는 `2+8*N`으로 함께 바꾸십시오. 독립 prefix 식과 axis 변환은 유지하십시오. 다른 manager는 해당 MD의 basis/track/proportional 입력과 공식을 같은 방식으로 보존하십시오.
4. 별도 source를 빌드하고 같은 scenario를 UI로 실행하십시오. 보존한 원래 node의 **동일 의미 assertion**이 같은 수치 오류로 실패해야 그 후보를 채택할 수 있습니다. node가 삭제되었거나 실패 의미가 달라지면 동일 재현으로 세지 마십시오. compile error·crash·row-count mismatch는 축소 성공이 아닙니다.
5. 채택한 후보를 다시 입력으로 줄이고 더 삭제할 수 없으면 입력 JSON·fixture patch·binary/source hash·전체 result와 cleanup 증거를 남기십시오. 변경한 분석 fixture는 원래 regression suite manifest에 자동 등록하거나 원본 PASS 결과와 합치지 마십시오.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 14 scenario, 14 action, 2282 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `R01.seed..1` | `run` : 130 |
| `R01.seed..17` | `run` : 130 |
| `R01.seed..73` | `run` : 130 |
| `R01.seed..257` | `run` : 130 |
| `R01.seed..65537` | `run` : 130 |
| `R02.flex-seed..1` | `run` : 160 |
| `R04.grid-seed..1` | `run` : 192 |
| `R05.absolute-seed..1` | `run` : 192 |
| `R02.flex-seed..73` | `run` : 160 |
| `R04.grid-seed..73` | `run` : 192 |
| `R05.absolute-seed..73` | `run` : 192 |
| `R02.flex-seed..65537` | `run` : 160 |
| `R04.grid-seed..65537` | `run` : 192 |
| `R05.absolute-seed..65537` | `run` : 192 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR58 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
