# 77. Layout Invalidation: Park

Layout pass 안에서 발생한 invalidation은 pending으로 보존하고, 정상적으로 끝난 pass의 `LayoutFinished`는 post-process에서 전달하는 계약을 검증합니다. 이 TC는 외부 처리 기회를 기다리는 scheduling 정책을 대상으로 합니다.

## 실행 준비

- 서버가 해당 commit에서 준비한 `manual-test-dali-ui-foundation` 실행 파일을 사용합니다. 이 문서의 실행 과정에서 빌드하지 않습니다.
- stdout/stderr를 파일로 수집하며 앱을 실행하고 목록에서 `77. Layout Invalidation: Park`를 선택합니다. 목록 검색 기능을 사용할 수 있습니다.
- 화면에는 arrange 횟수 라벨과 파란 box가 표시됩니다. 진입 완료 뒤에는 지시된 입력 외의 터치·키·window 이동·크기 변경을 하지 않습니다.
- 서버에서 timeout과 로그를 관측합니다. 앱 내부 polling timer, 자동 클릭, 수동 `ProcessLayouts()` 호출을 추가하지 않습니다.

## 관측값

- `TC77_ARRANGE count=N mutating=1`: box Arrange에서 라벨을 변경했습니다. 이후 재계산할 작업이 남을 수 있습니다.
- `TC77_PASS finished=F arrange=N mutating=M`: 정상 window pass의 post-process 알림입니다. `F`는 전달 순번이며 `M=1`이면 producer가 계속 변경하도록 설정되어 있습니다.
- `producer stopped mutating`: 30번째 변경 후 producer가 추가 변경을 중단했습니다.
- signal handler는 로그만 기록합니다. 신호 자체를 최종 geometry 또는 rendering 완료로 해석하지 않습니다.

## 실행 절차와 판정

1. TC 진입 직후 로그를 수집합니다. `0 < arrange < 30`, `mutating=1`인 `TC77_PASS`가 있어야 합니다. 변경이 계속 설정된 상태에서도 pass 알림이 발생하는 것이 핵심 조건입니다.
2. 마지막 초기 로그 이후 3초 동안 입력을 중단합니다. 초기 자원 처리 중 로그가 더 생겼다면 마지막 로그부터 다시 관측합니다. quiet 구간에서 arrange가 자율적으로 계속 증가하면 이 commit의 park 정책에 대해 FAIL입니다. 10초 내 quiet 구간을 확보하지 못하면 환경의 추가 이벤트 여부를 기록하고 재실행합니다.
3. 파란 box를 한 번 터치한 뒤 로그를 읽습니다. 이전보다 arrange 횟수가 증가하고, 그 결과에 해당하는 `TC77_PASS`가 뒤따라야 합니다. 하나의 터치가 여러 platform 이벤트를 만들 수 있으므로 터치 수와 pass 수가 정확히 같아야 한다는 조건은 사용하지 않습니다.
4. 입력 사이에 quiet 구간을 두면서 3번을 반복합니다. `finished`는 관측된 `TC77_PASS`마다 정확히 1 증가하고 `arrange`는 감소하지 않아야 합니다. `mutating=1`인 알림을 최소 두 번 확인합니다.
5. `producer stopped mutating`을 관측한 뒤 한 번 더 입력합니다. 남은 계산이 처리된 pass 알림을 확인하고, 다시 3초 동안 로그가 더 증가하지 않는지 확인합니다.
6. Back으로 나갔다가 재진입합니다. 로그 순번이 초기화되고 같은 절차가 가능해야 합니다.

## PASS / FAIL

- PASS: 변경 중인 정상 pass에서도 알림이 발생하고, 외부 이벤트로 후속 계산이 진행하며, 변경 중단 후 조용한 상태가 유지됩니다.
- FAIL: `LayoutFinished`가 변경 중단 때까지 억제되거나, 로그 순번이 누락·역행하거나, producer 실행 뒤 완료 알림이 누락되거나, 조용한 상태에서 계산/알림이 계속 증가하거나, 이탈·재진입 중 crash가 발생합니다.
- 최초 진입 후 자원 처리나 서버의 반복 입력이 지속되어 park 관측 조건을 만들지 못한 실행을 PASS로 처리하지 않습니다. 실행 조건과 원본 로그를 함께 보존합니다.
- CPU 수치는 보조 관측값입니다. PC별 고정 CPU 백분율을 기능 PASS / FAIL 기준으로 사용하지 않습니다.
