# TC77 — In-pass text 변경의 자동 continuation

실행 대상은 서버가 제공한 현재 revision의 manual-tests binary입니다. 이 문서에는 빌드 단계가 없습니다. stdout/stderr를 모두 보존하십시오.

1. launcher 목록에서 `77. Layout Invalidation: Automatic Continuation`을 선택합니다. 이름 검색이나 목록 탐색을 사용하십시오.
2. 화면 진입 이후 touch·키·resize를 추가하지 마십시오. 앱 내부 timer·polling·animation을 추가하거나 `ProcessLayouts()`를 호출하지 마십시오.
3. 외부 도구의 timeout 15초 동안 `TC77_ARRANGE count=N mutating=1`과 `TC77_PASS finished=F arrange=N mutating=M`을 수집합니다.
4. `count=30`과 producer 중단 로그, 이후 `mutating=0`인 완료 알림을 관측하십시오. `mutating=1`인 완료 알림도 그 전에 존재해야 합니다.
5. resource 관련 startup event가 끝난 뒤 3초 이상 `TC77_ARRANGE`/`TC77_PASS`가 증가하지 않는지 외부 로그로 확인하십시오. 15초 내 quiet 구간이 없으면 실패 근거로 남기고 별도 TC95를 수행하십시오.
6. 뒤로 이동했다가 재진입하여 arrange 및 finished 번호가 1부터 다시 시작하는지 확인하십시오.

- PASS: 입력 없이 30번의 mutation에 도달하고, pending이 있던 pass도 완료 알림을 전달하며, mutation 중단 뒤 최종 처리를 거쳐 quiet 상태에 도달합니다.
- FAIL: 외부 입력을 추가해야 진행되거나, 정상 pass의 완료 알림이 mutation 중단까지 억제되거나, mutation 종료 뒤에도 계산/알림이 계속 반복되거나, 재진입 시 counter가 초기화되지 않거나 crash가 발생합니다.
- `finished`는 1씩 증가해야 하고 `arrange`는 역행하면 안 됩니다. resource 및 platform event가 추가 pass를 만들 수 있으므로 모든 PC에서 고정된 전체 완료 횟수를 강제하지 마십시오. `count=30` 이후 최종 geometry를 처리하는 pass는 허용합니다.
- 이 TC는 text resource 경로도 사용합니다. 독립 event가 scheduling 유실을 숨길 수 있으므로 자동 진행 보장의 최종 판정에는 TC95 quiet 시나리오도 필요합니다.
- CPU는 보조 관측값입니다. 프로세스 전체 CPU에 고정 임계값을 적용하거나 `LayoutFinished`만으로 안정화·화면 표시 완료를 단정하지 마십시오.
