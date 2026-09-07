### Summary
- 기존 LayoutTransition은 부모에 부착되어 모든 직계 자식(SUBTREE scope면 자손까지)에 일괄 적용되어, 자식별 제외·재정의가 불가능했습니다.
- 자식이 스스로에게 부착하는 self transition을 추가합니다: `View::SetSelfLayoutTransition(t)` — 우선순위는 자신 > 직계 부모 > SUBTREE 조상이며, 각 레벨은 부착 여부만으로 종결됩니다(도매 적용, 슬롯 병합 없음). 빈 핸들은 해제(상속 복귀)입니다.
- 뷰별 적용 정책 `LayoutTransitionMode { AUTO, PASS_THROUGH, ISOLATE_SUBTREE }`와 `View::SetLayoutTransitionMode/GetLayoutTransitionMode`를 추가합니다. 정책은 핸들 해석보다 먼저 평가됩니다(정책 > 값).
- `PASS_THROUGH`: transition이 이 뷰를 통과합니다 — 자신은 어떤 transition의 대상도 아니되(자기 self 핸들 포함), 상속은 자손으로 계속 흐르고 자기 children-role transition도 유지됩니다.
- `ISOLATE_SUBTREE`: 이 뷰와 서브트리 전체를 게이트 이상(자기 children-role 포함)의 모든 owner로부터 격리합니다. 게이트 아래에서 자손이 스스로 선언한 transition은 유효합니다.
- 핸들은 mode 변경으로 해제되지 않고(`AUTO` 복귀 시 복원), mode 변경은 in-flight transition을 취소하지 않습니다.
- 공개 API는 비가상 추가만으로 구성되어 기존 ABI를 유지하며, 뷰당 메모리 증가는 0바이트입니다(2비트 필드가 기존 패딩에 흡수).

### Changes
- `View::SetSelfLayoutTransition/GetSelfLayoutTransition`, `LayoutTransitionMode` enum, `View::SetLayoutTransitionMode/GetLayoutTransitionMode` 추가(비가상).
- 판정 단일화: `ResolveGoverningTransition`(레벨 0 정책 게이트 → self → 직계 부모 → SUBTREE 조상) — ENTER/CHANGE 디스패치, EXIT 유예, EXIT 효과 소스, pending ENTER 라우팅이 동일 규칙을 공유.
- 디스패처: 자식 키 self 스냅샷·pending ENTER 맵, owner 패스의 디스패치 시점 self/mode skip(이중 애니메이션 방지), `ISOLATE_SUBTREE` 캡처 경계, 순회를 강한 핸들로 고정(콜백 재진입 안전), 해제·게이트 시 pending 상태 정리.
- `ViewDataImpl::Remove`를 판정 1회 호출 구조로 재구성하고 무-transition fast path(`HasAnyInstance` 게이트)를 유지. EXIT ghost/geometry는 직계 부모 기준 불변.
- 자동화 테스트 38케이스, 위키(EN/KR) "Per-view override"·"Per-view policy" 절, A/B/C 비교용 self-override 샘플 추가.

### Examples
```cpp
parent.SetLayoutTransition(t);                                         // 자식 일괄 적용(기존)

child.SetSelfLayoutTransition(mine);                                   // 이 자식만 다른 transition (부모보다 우선)
child.SetSelfLayoutTransition(LayoutTransition());                     // 해제 → 부모/조상 규칙으로 복귀

child.SetLayoutTransitionMode(LayoutTransitionMode::PASS_THROUGH);     // 이 자식만 통과(대상 제외, 자손 상속·핸들은 유지)
child.SetLayoutTransitionMode(LayoutTransitionMode::ISOLATE_SUBTREE);  // 이 자식+자손 전체를 위쪽 상속에서 격리
child.SetLayoutTransitionMode(LayoutTransitionMode::AUTO);             // 정책 해제 → 부착된 핸들 효과 복원
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
