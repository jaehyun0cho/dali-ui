### Summary

`LayoutController`는 dali-core `Integration::Processor`로 등록되며, `Process()`
실행 중 자기 자신을 해제하는 경로(윈도우 소멸 감지, 슬롯에서의 `Remove()`)가
있었습니다. dali-core는 `Process()` 반환 직후 같은 포인터로 `GetProcessorName()`을
호출하므로(`Core::RunProcessors`의 트레이스 기록) 이 해제는 use-after-free이며,
`ENABLE_TRACE` 빌드에서 `DALI_TRACE_PERFORMANCE_MARKER` 활성 시 크래시가 납니다.
dali-ui의 어떤 동기 진입점도(`Get()`/`Remove()` 포함 — 다른 프로세서의 `Process()`
콜스택에서도 호출됨) 프로세서 루프의 바깥임을 증명할 수 없으므로, 소멸을 "즉시
무력화(detach)"와 "지연 해제(free)"로 분리했습니다. detach된 컨트롤러는 처리와
emit을 즉시 멈추고 대기 목록에 보관되며, 해제는 루프 바깥이 구조적으로 보장되는
idle 콜백(백스톱: 프로세스 종료 시 정적 소멸)에서만 수행됩니다.

### Changes

- `Detach()`: 프로세서 등록 2건·윈도우 시그널·트랜지션 tick 해제로 즉시 무력화 (멱등)
- `DetachAndQueueForFree()`: 소유권을 `gDetachedLayoutControllers`로 이전 후 idle 예약.
  멱등 가드 필수 — 슬롯이 `Remove()`→`Get()`으로 만든 새 컨트롤러를 언와인드의
  2차 호출이 오인 회수하지 않도록 차단
- 해제는 idle 콜백과 정적 소멸 둘뿐; `Get()`/`Remove()`의 동기 해제 제거
- idle 예약은 detach마다 무조건 — adaptor 정지 시 대기 idle이 폐기되므로 "예약됨"
  플래그는 고착 위험, 중복 idle은 빈 큐를 보고 종료
- `gGlobalProcessDepth`(RAII): 중첩 이벤트 루프에서 idle이 끼어든 경우 skip+재예약
- `UnregisterFromAll()`이 대기 목록도 스크럽 (detach~해제 사이 View 소멸 대비)
- `LayoutTransitionDispatcher::Shutdown()` 추가(`mTickTimer` 정지); 헤더는 주석만, ABI 무관
