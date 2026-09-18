# Layout validation 도구 판정 계약

이 도구는 TC를 실행하지 않습니다. 서버의 실행 에이전트는 TC별 MD에 따라 앱을 조작하고 원본 stdout/stderr, 종료 상태, artifact hash를 보존한 뒤 이 도구로 판정합니다. JSON 출력과 process exit code를 함께 확인하십시오. 화면에 표시된 PASS나 timing ratio만으로 최종 PASS를 결정하지 마십시오.

## 실행 전 계약 검사

저장소 root에서 다음 명령을 실행하십시오.

```sh
python3 manual-tests/dali-ui-foundation/layout-validation-tools.py audit
python3 manual-tests/dali-ui-foundation/layout-validation-tools.py self-test
```

각 명령의 exit code가 0이고 `status`가 각각 `inventory-valid`, `self-test-pass`여야 합니다. `audit`는 TC/MD, asset hash, MD의 scenario/action/check 수, performance plan의 manifest pin 및 전체 metric family 일치를 검사합니다. 이 결과는 실제 앱의 실행 성공이나 시간 성능을 뜻하지 않습니다.

`layout-validation-manifest.json`의 `reviewed_performance_metrics`는 실행 코드 및 candidate 로그와 별도로 검토하는 계약입니다. 각 plan의 `metric_family`와 `metrics`는 아래 전체 집합을 선택해야 합니다.

| metric_family | TC | metric 수 |
| --- | --- | ---: |
| `compute_update` | LR59, LR60 | 57 |
| `transition` | LR63 | 15 |
| `complex` | LR64 | 16 |

LR59는 layout family 4종 × item 수 3종 × `construction`, `first_pass`, `warm_pass`, `idle_pass`의 48개 metric입니다. LR60은 item 수 3종 × `sparse`, `dense`, `same_value`의 9개 metric입니다. 이전 `first_measure`, `first_arrange`, `warm_measure`, `warm_arrange`와는 같은 metric으로 비교할 수 없습니다.

phase 또는 TC 전체를 plan에서 빼거나, 알 수 없는 metric을 넣거나, 중복 metric을 넣으면 invalid evidence입니다. Candidate 결과에 맞춰 expected metric, mandatory check 수, pin을 자동으로 재생성하지 마십시오. 계약 변경은 source와 독립된 expected 및 검증 범위를 검토한 뒤 이루어져야 하며, 승인된 plan과 manifest pin은 측정 전에 서버 설정에 고정해야 합니다.

## 성능 비교 실행과 판정

TC별 성능 MD의 조건에 따라 준비한 plan 및 experiment를 사용하십시오.

```sh
python3 manual-tests/dali-ui-foundation/layout-validation-tools.py compare /absolute/path/approved-plan.json /absolute/path/experiment.json
```

에이전트는 전체 JSON 출력을 보존하고 다음 순서로 판정하십시오.

1. exit 2 또는 `status: invalid-evidence`이면 FAIL로 취급하고 evidence 수집/계약 오류로 분류합니다. 누락·중복·잘못된 identity·cleanup 실패·서로 다른 artifact/environment 등의 원인을 `error`에서 기록합니다. 성능을 판정할 증거가 부족하므로 PASS로 전환하지 마십시오.
2. exit 1 및 `status: correctness-failed`이면 correctness 또는 work-budget regression입니다. `correctness_failures`의 모든 `pair_id`, `side`, `log`, TC별 `failing_rows`를 보고하십시오. `results`는 비어 있으며 timing PASS는 발행하지 않습니다. Reference 쪽 실패도 비교 기준이 성립하지 않으므로 candidate PASS의 근거가 될 수 없습니다.
3. timing `results`가 있는 경우 모든 metric의 verdict를 확인하십시오. 하나라도 `FAIL`, `MORE_PAIRS_REQUIRED`, `INDETERMINATE`이면 최종 PASS가 아닙니다. `MORE_PAIRS_REQUIRED`는 승인된 31/62/93 pair 절차의 다음 look까지 수집하고, `INDETERMINATE`는 정해진 수집 한도 내에서 결론을 내리지 못한 결과로 보고하십시오.
4. exit 0이고 모든 reviewed metric이 `PASS_AT_DECLARED_RESOLUTION`일 때만 승인된 해상도에서 timing PASS입니다. 이는 별도 TC MD에서 요구하는 외부 관측·allocator stack·coverage evidence까지 충족했다는 뜻은 아닙니다.

`compare`는 correctness 실패를 발견해도 모든 pair와 양쪽 로그의 구조, sample, operation count, scope, process 독립성 검증을 계속합니다. 뒤의 pair가 malformed이면 exit 2가 우선합니다. 정상 구조의 실패 로그는 exit 1로 반환하며, 동일한 시간이거나 candidate 시간이 짧아도 geometry/work-budget 실패를 PASS로 만들 수 없습니다.

`compare`가 검사하는 TC는 plan의 `metric_family`가 지정한 TC뿐입니다. 따라서 timing 로그에 그 family에 없는 TC의 기록이 하나라도 있으면 `invalid-evidence`입니다. 그 TC의 계약과 실패 행은 검사되지 않은 채 남고, 같은 process에서 계획되지 않은 작업이 실행됐다는 뜻이므로 timing 자체도 신뢰할 수 없습니다. 통과하는 TC든 실패하는 TC든 같게 취급하며, `error`에 planned/unplanned TC 목록이 남습니다.

`observer.timeout`, `render.settle-timeout`, `render.frame-timeout` 또는 `render.unsupported-parent` 이후 필수 관측이 누락되거나 행 수가 맞지 않는 경우에도 strict 검증은 `invalid-evidence`를 유지합니다. 원본 관측 실패 행과 `error`를 함께 보존하고 frame/layout 관측 실패로 보고하십시오. 재실행했다면 첫 실패 기록과 새 실행의 identity를 모두 보존해야 하며, timeout을 무시하거나 허용 오차를 넓혀 성공으로 바꾸지 마십시오.

## Frame fence와 `LR_FRAME`

`AfterFrame`은 window에 있는 1x1 sentinel actor를 source로 하는 새 `REFRESH_ONCE` render task의 완료를 기다립니다. framebuffer가 없으므로 pixel readback도 GPU sync도 없고, 개별 시도마다 만료되는 별도 timeout도 없습니다. task는 event-side 쓰기가 모두 큐에 들어간 뒤에 생성되므로, 완료는 update 한 번과 render 한 번이 그 쓰기를 모두 소비했다는 뜻입니다. OS window의 presentation이나 compositor 출력이 아니라 fresh scene state의 증거입니다. 전체 fence의 유일한 한도는 `FRAME_DEADLINE_MS`(4000 ms)이며, 이는 fence 이후의 scene-graph settle watchdog인 `DEFAULT_SETTLE_MS`와 독립된 상수입니다.

정상 완료는 아무 기록도 남기지 않습니다. 비정상 완료는 `render.frame-timeout` 실패 행과 그 action의 `protocol.required_checks` 실패 행을 만들고, 동시에 stdout과 stderr에 기계가 읽을 수 있는 `LR_FRAME` 레코드를 한 줄 남깁니다.

```
LR_FRAME {"tc":"LR59","pid":123,"run":1,"scenario":"…","step":"…","action_seq":15,"result":"timeout","reason":"…","attempts":3,"elapsed_ms":4001.2,"deadline_ms":4000}
```

`result`는 `timeout`, `disconnected`, `predicate-exception` 중 하나입니다. `LR_FRAME`은 행이 아니므로 어떤 계약 수치도 바꾸지 않지만, `check-log`는 이 레코드를 검사 대상으로 삼습니다. 형식이 어긋나면 `malformed frame record`로 `invalid-evidence`이고, 같은 action에 `render.frame-timeout` 실패 행이 없으면 `frame failure record without its render.frame-timeout row`로 `invalid-evidence`입니다. 행 수가 맞지 않는 action에 frame 레코드가 있으면 그 원인을 메시지에 함께 표시합니다. `check-log` 결과의 `frame_failures`에 TC별로 `action_seq`, `scenario`, `step`, `result`, `reason`, `attempts`, `elapsed_ms`가 남으므로 이 값을 그대로 보고하십시오. `compare`는 `frame_failures`가 비어 있지 않은 로그를 `failing_rows`와 같게 `correctness-failed`로 처리합니다.

`LR_FRAME`이 나왔다면 deadline을 늘리거나 재시도를 추가해 통과시키지 마십시오. fence가 만료됐다는 것은 그 action이 관측하려던 frame이 실제로 오지 않았다는 뜻이며, 원인을 규명해 보고해야 합니다.
