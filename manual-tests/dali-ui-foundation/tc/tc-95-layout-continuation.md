# TC95 — 외부 입력 없는 Layout 진행과 연속 작업 제한

## 실행 조건

서버가 제공하는 현재 revision의 manual-tests binary를 사용하고 stdout/stderr를 수집하십시오. 이 문서에는 빌드 단계가 없습니다. launcher에서 `95. Layout Continuation: Quiet and Cost`를 선택하십시오.

`TC95_READY` 뒤 3초간 입력하지 않아 startup/resource 처리가 끝나도록 하십시오. 그다음 아래 시나리오별 키를 한 번만 누르고 놓습니다. 자동 키 반복을 사용하지 마십시오. 로그의 `TC95_START run=R`로 실행 구간을 구분합니다. 한 구간이 끝나고 3초간 로그가 멈춘 뒤 다음 구간을 시작하십시오.

이 TC는 timer, idle callback, animation, watchdog, polling, `SendNotification()` 또는 별도 event pump를 만들지 않습니다. `m` 모드의 명시적 수동 계산만 예외입니다. 서버가 timeout과 로그 관측을 수행해야 하며 관측을 위해 앱에 입력하거나 속성을 바꾸면 안 됩니다. 외부 screenshot 획득은 앱 event를 발생시키지 않는 방식만 사용하십시오.

## 로그 해석

모든 `at_us`는 동일 프로세스의 monotonic clock을 마이크로초로 변환한 값입니다. 서버 수신 시각 대신 이 값을 사용하십시오. stdout pipe를 계속 읽어 로그 출력 자체가 막히지 않도록 하십시오.

| 로그 | 관측값 |
|---|---|
| `TC95_START` | run, mode, 목표 limit(0이면 연속), 시작 시각 |
| `TC95_ARRANGE_BEGIN/END` | 실제 Arrange callback 번호와 구간 |
| `TC95_FINISH_BEGIN/END` | View의 pass 완료 callback 번호, 시각, snapshot width |
| `TC95_DONE` | 목표 완료 횟수에 도달한 시점의 arranged/finished |
| `TC95_BURST` | 외부 키로 같은 View에 100회 invalidation 요청 |
| `TC95_STOP` | 연속 producer 중단 요청 |
| `TC95_EXIT` | TC 연결 해제 및 이탈 |

각 번호는 run 안에서 1부터 연속 증가해야 합니다. BEGIN/END는 같은 번호로 짝지으십시오. width는 160입니다(프로젝트의 UI scale이 1이 아닌 환경은 그 scale이 적용된 값으로 검증하십시오). snapshot width가 같아도 pass 알림은 생깁니다.

## 시나리오 및 판정

### A. 유한 요청의 자동 진행

키 `1`, `2`, `3`, `4`를 각각 별도 run으로 수행합니다. 기대 limit은 각각 1, 2, 3, 12입니다.

- 각 run 시작 뒤 추가 입력 없이 10초 내 DONE이 나와야 합니다.
- DONE의 arranged와 finished가 모두 limit과 같아야 합니다.
- 완료 전에도 각 정상 pass에 FINISH가 있어야 합니다. 마지막 안정화 시점에만 알림이 몰리는 것은 FAIL입니다.
- DONE 뒤 3초간 추가 ARRANGE/FINISH가 없어야 합니다.
- START 이전 또는 구간 중 다른 입력·resize가 있었으면 해당 구간은 무효이며 다시 수행합니다. 단순히 count가 다르다는 이유로 허용 범위를 늘리지 마십시오. 반복해도 count가 다르면 로그를 첨부하여 FAIL로 보고합니다.

### B. 완료 callback에서의 요청

키 `h`는 완료 callback에서 다음 invalidation을 만들고, 키 `m`은 그 callback이 `ProcessLayouts()`까지 동기 호출합니다. 각각 limit=8입니다.

- 입력 없이 DONE에 도달하고 arranged=finished=8이어야 합니다. 이후 3초 quiet를 확인합니다.
- `m`은 FINISH_BEGIN(n)과 FINISH_END(n) 사이에 ARRANGE(n+1)가 나타나는 것이 정상입니다.
- `m`에서도 FINISH(n+1)는 FINISH_END(n) 이후의 새 처리 기회에서 발생해야 합니다. callback 안에서 다음 FINISH가 중첩되거나 즉시 연쇄되는 것은 FAIL입니다.
- pending root가 수동 처리로 비었어도 completion 전달이 멈추면 FAIL입니다.

### C. 기본 간격과 비용 반영

A의 `4`, B의 `h`와 `m`, 키 `p`(Arrange 안에서 40ms 작업), 키 `w`(완료 callback 안에서 40ms 작업)를 수행합니다. p/w의 limit=6입니다.

- 일반 모드(a/h/p/w): 이전 FINISH_END(n)부터 다음 ARRANGE_BEGIN(n+1)까지 간격을 계산합니다.
- `m`: 이전 FINISH_END(n)부터 다음 FINISH_BEGIN(n+1)까지 간격을 계산합니다. 동기 manual Arrange 간격에는 제한을 적용하지 마십시오.
- 기본 하한은 16,000us입니다. 구현의 약 16,667us보다 작은 판정값으로 clock 변환·관측 오차를 허용하지만, 평균만 검사하지 말고 모든 인접 쌍을 검사하십시오.
- p/w는 직전 pass의 ARRANGE_END−BEGIN과 FINISH_END−BEGIN을 더한 값을 `observedCost`로 구합니다. 다음 간격은 `max(16,000, observedCost−1,000)`us 이상이어야 합니다. 관측 callback 밖의 추가 비용은 controller가 더 측정할 수 있으므로 더 긴 대기는 허용합니다.
- p의 Arrange, w의 완료 callback 구간이 각각 40,000us 이상인지 확인하십시오. 이를 만족하지 않으면 비용 경로를 검증한 것으로 판단하지 마십시오.
- 상한 timing은 OS 부하에 의존합니다. 개별 16.667ms tick 정확도나 정확히 60회/초를 요구하지 않습니다. 대신 각 유한 run의 전체 10초 timeout으로 liveness를 판정합니다.

### D. 계속되는 요청과 외부 event의 우회 방지

키 `c`로 시작하여 3초 관측합니다. 추가 입력 없이 count가 증가하고 C의 간격 하한을 지켜야 합니다. 그다음 `b`를 5회, 각 30ms 이상 간격으로 누릅니다. 이 단계만 외부 입력을 허용합니다.

- BURST 로그 5개를 확인합니다. 다른 event가 생겨도 인접 pass의 기본 간격 하한은 유지돼야 합니다.
- 마지막 BURST 뒤 입력 없이 2초간 count가 계속 증가해야 합니다. 중복 요청이 deadline을 계속 연장하여 진행이 멈추면 FAIL입니다.
- `x`를 한 번 누릅니다. 이미 pending인 최종 pass는 허용하되 STOP 뒤 1초가 지난 구간부터 3초간 ARRANGE/FINISH가 없어야 합니다.
- x/뒤로 입력에 반응하지 못하거나 지연 없는 반복 로그가 쌓이면 FAIL입니다. 전체 CPU 값만으로 PASS/FAIL을 대체하지 마십시오.

### E. 이탈 및 재진입

`c`로 시작한 뒤 뒤로 이동하여 TC를 종료합니다. `TC95_EXIT` 이후 해당 run의 로그가 재발생하거나 crash하면 FAIL입니다. 다시 TC95에 들어가 READY와 quiet 구간을 확인한 뒤 `3`을 실행하여 arranged=finished=3을 확인합니다.

## 결과 보관

revision, backend, window 크기, UI scale, 각 run의 전체 로그, DONE 횟수, 최소 인접 간격, p/w의 observedCost와 timeout 여부를 저장하십시오. 입력 없는 구간에 다른 window/app event가 개입했다면 그 사실을 기록하고 깨끗한 조건에서 재시도하십시오. 실패를 없애기 위해 실행 중 event pump나 지연 timer를 추가하지 마십시오.

이 TC는 실제 backend의 자동 진행·간격·정지를 검증합니다. cache/geometry의 모든 조합, 여러 Window의 공정성, exception 이후 core 복구, 전체 프로세스 CPU 상한까지 검증하는 것은 아닙니다. 해당 경계는 별도 UTC와 검토 자료에서 다룹니다.
