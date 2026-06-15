# RadioButton UI Framework 설계 방안 비교 분석 (적대적 검토)

**작성일**: 2026-06-15  
**검토 범위**: dali-ui의 GroupSelectableTrait 설계(plan.md) vs OneUIComponents의 SelectionGroup/GroupSelectable/RadioButton 구현 vs RadioButton 컴포넌트 최적 설계

---

## 1. 두 설계의 구조 비교

### 1.1 dali-ui (C++ Trait 기반)

```
SelectableTraitImpl (기존, trait 인프라)
    ↓ extends
GroupSelectableTraitImpl (신규, grouped state 포함)
    
SelectableGroupImpl (신규, group controller, exclusive 정책 강제)
    ↔ (양방향 public 래퍼로만 통신)
GroupSelectableTraitImpl (new virtual hooks, no friend)
```

**핵심 특성**:
- 한 trait이 하나의 View에 "slot" 형태로 attach (trait ↔ view 1:1)
- Group은 logical controller (BaseObject), 멤버는 WeakHandle (사이클 없음)
- **cross-call transition state** (`mTransition`, `mPending`): swap 시 loser commit → winner commit → group signal까지 spanning
- **신규 protected hook**: `OnSelectionChanged(view, bool, InputEvent)` (post-commit)
- **protected helper**: `CommitSelectedState(bool, InputEvent)` (veto 우회, pure null-owner 가드)
- **base `SetSelectedInternal` private 유지**: group이 직접 호출 불가 → veto bypass 명확성 높음

### 1.2 OneUIComponents (C# 문자열 레지스트리 기반)

```
Selectable (기존, item 추상화)
    ↓ extends
GroupSelectable (item-level: group name string property)

SelectionGroup (registry: static Find(string) 위젯 생성)
    ↔ (event subscription: SelectedChanged)
IGroupSelectable (item interface, SelectedChanged event)
```

**핵심 특성**:
- GroupName은 **문자열** (weak registry lookup), 같은 이름 멤버들이 하나 group에 collect
- SelectionGroup은 "find-or-create" 레지스트리 패턴 (parent View ID 또는 명시적 이름)
- **flag-based guard**: `_onSelectionUpdating` (boolean, local mutual exclusion)
- item-level `SelectedChanged` 이벤트로 group이 listening (역방향, event-driven)
- **no accessibility tie**: RadioButton role은 set하지만 CHECKED와의 lock-step 없음
- **selected 재클릭**: `IsSelected` toggle로 직접 false 가능 (deselectable=false 로직에 의존)

---

## 2. 핵심 요구사항 비교 검증

| 요구사항 | dali-ui 방식 | OneUIComponents 방식 | 문제점 평가 |
|---------|-----------|-------------------|----------|
| **한 group 최대 1 winner** | `mSelected` weak handle + veto hook로 배타성 강제 | `_selected` 변수 + `OnItemSelectionChanged` 재귀 보호 | dali-ui가 구조적으로 엄격함 |
| **winner 재클릭 → no-op** | `RequestSelectionChange`에서 멱등성(§247 line 262) | `IsDeselectable=false` property로 구현하려 했으나 **incomplete**(아래 §2.1 참조) | dali-ui가 명확하고 exception-safe |
| **clear 단일 진입점** | `ClearSelection()` public API | `Selected = null` property setter (§94-110) | dali-ui가 제어 범위 명확 |
| **group 비어있을 수 있음** | `allowEmptySelection=true` (기본) | 암묵적(no auto-promote) | dali-ui가 명시적 |
| **group이 이전 winner 추적** | `mPending.previous` View + post-commit signal | EventArgs로 old/new 전달 | dali-ui가 신호 순서 보장 |
| **멤버 소멸 자동 정리** | `UnregisterMember` override (`OnDetaching`/`OnViewDestroying`) | `selectable.SelectedChanged -= OnItemSelectionChanged` + group name clear | dali-ui가 lifecycle hook으로 명확 |

### 2.1 **CRITICAL: OneUIComponents의 "selected 재클릭" 설계 결함**

OneUIComponents 코드 분석 (SelectionGroup.cs:325-352):
```csharp
private void OnItemSelectionChanged(object sender, InputEventArgs e)
{
    if (_onSelectionUpdating) return;  // ← flag guard, non-persistent
    
    if (newSelection.IsSelected)       // ← 재클릭 시 true?
    {
        _selected = newSelection;
        if (lastSelection != null)
            lastSelection.Toggle(e.InputDevice);   // ← 이전 winner toggle (loop 재진입!)
    }
    else if (newSelection == lastSelection && !newSelection.IsSelected)
    {
        _selected = null;   // ← 재클릭이 false 반환해야 here 도달
    }
}
```

**문제 경로**:
1. user clicks selected radio B
2. `Selectable.OnClickedForToggle()` → `IsSelected = !IsSelected` (false)
3. group의 `OnItemSelectionChanged` 호출
4. **`_onSelectionUpdating`은 local flag** → false ≠ true 시점
5. **직접 `lastSelection.Toggle()`을 호출함** → 루프 재진입 위험
6. RadioButton 테스트(TSRadioButton.cs:83)에서 **이 시나리오를 명시적으로 테스트하지 않음**

**dali-ui는**:
- `RequestSelectionChange(trait, ownerView, **newSelected=false**)`로 진입
- 이미 winner면 `return false` (veto, §247 line 271-272)
- 외부 `SetSelected(false)` hook에서 가드(no-op)
- **예외 안전성 + 명확한 의도**

---

## 3. 아키텍처 차원 비교

### 3.1 멤버십 모델

| 차원 | dali-ui | OneUIComponents | 적대적 평가 |
|-----|--------|-----------------|-----------|
| **group 멤버 저장소** | `mMembers: vector<WeakHandle>` (trait identity) | `_children: List<IGroupSelectable>` (interface) | dali-ui: 약한 참조(사이클 방지), 인터페이스 인디렉션 없음 |
| **group lookup** | trait → `mGroup` strong handle (RAII) | string `GroupName` → static registry (Find 매번 lookup) | dali-ui: O(1) 멤버십, OneUI: registry GC 압력 + lazy resolve |
| **cross-group rebind** | `BindGroup`/`UnbindGroup` public wrapper | `JoinGroup`: old group `Remove()` → new group `Add()` | dali-ui: 원자적(transition 보호), OneUI: 2단계(중간 상태 가능) |
| **auto-grouping (parent)** | 미포함(범위 외) | `OnAddedToWindow` + parent View ID tag (§85) | OneUI: framework leverage, dali-ui: 후속(scope 분리) |

### 3.2 Accessibility 동기화

| 차원 | dali-ui | OneUIComponents | 비판적 평가 |
|-----|--------|-----------------|-----------|
| **role 설정** | `OnAttached`에서 `RADIO_BUTTON` | RadioButton ctor에서 `AccessibilityRoleV2 = RadioButton` | 양쪽 동일 |
| **CHECKED state** | `OnSelectionChanged`에서 RMW (winner true + loser false) | 없음 (role만 set) | **dali-ui 필수**, OneUI 누락(announce 불가) |
| **ViewState.SELECTED vs AccessibilityState.CHECKED** | lock-step, 개별 RMW(§60-61) | SELECTED만 지원 (CHECKED 아님) | dali-ui: 규정 준수(A11y), OneUI: incomplete |

---

## 4. 신호 순서 및 이벤트 흐름

### 4.1 dali-ui (plan.md §8 선택 알고리즘)

```
swap(A,B):
  ① A.CommitSelectionFromGroup(false, None) [PUBLIC wrapper, hook 미진입]
  ② A.mSelected = false
  ③ A.SetState(SELECTED, false)
  ④ A.mSelectionChangedSignal.Emit(A, false, None)
  ⑤ A.OnSelectionChanged(A, false, None) [post-commit hook, no flush]
  
  ⑥ B.CommitSelectedState(true, event) [base new() 호출]
  ⑦ B.mSelected = true
  ⑧ B.SetState(SELECTED, true, event)
  ⑨ B.mSelectionChangedSignal.Emit(B, true, event)  [item signal]
  ⑩ B.OnSelectionChanged(B, true, event) [post-commit hook]
     → B.WriteCheckedState(true)
     → B.NotifyCommitted() [PUBLIC reverse]
  
  ⑪ group.SelectedMemberChangedSignal.Emit(group, A, B, event)
```

**app callback 순서**: `A.item(false)` → `B.item(true)` → `group(A,B,event)`

### 4.2 OneUIComponents (SelectionGroup.cs:325-352)

```
swap(A → B):
  ① B.IsSelected = true
  ② group.OnItemSelectionChanged(B, e)
     if(_onSelectionUpdating) return;  // ← false
     _selected = B
     if(lastSelection=A != null)
       A.Toggle(e.InputDevice)  [직접 호출, 재귀!]
  
  ③ group.OnItemSelectionChanged(A, e)
     if(_onSelectionUpdating) return;  // ← false ← 루프 재진입 위험
     else if(A == lastSelection && !A.IsSelected)
       _selected = null
  
  ④ group.SelectionChanged.Invoke(A, B)
```

**문제**: 
- ③은 A의 toggle 콜백 중 호출 (단일 frame)
- A가 다시 선택되거나 exception 발생 시 state inconsistency
- **flag guard (`_onSelectionUpdating`)은 local, persistent transition 아님**

### 4.3 결론

**dali-ui**: 
- ✅ persistent `mTransition` state로 모든 재진입 차단
- ✅ post-commit hook이 group flush 지점 명확화
- ✅ loser emit 후 winner emit 후 group signal (순서 보장)

**OneUIComponents**:
- ⚠️ flag 기반 local guard (제한적)
- ⚠️ 재귀 호출 (item → toggle → item callback 체인)
- ⚠️ exception 안전성 미흡

---

## 5. 예외 안전성 (Exception Safety)

### 5.1 dali-ui (plan.md §8)

```cpp
struct ScopeGuard {
  SelectableGroupImpl& mGroup;
  ScopeGuard(SelectableGroupImpl& g) : mGroup(g) {
    mGroup.mTransition = Transitioning;
  }
  void Dismiss() { /* no-op, will reset in dtor */ }
  ~ScopeGuard() {
    if (!dismissed) {
      mGroup.mTransition = Idle;
      mGroup.mPending.reset();
    }
  }
};

RequestSelectionChange() {
  ScopeGuard g(*this);
  // ... loser commit, mSelected update, etc.
  g.Dismiss();  // ← success: let winner commit carry state across
  return true;
}
```

**보장**: 
- loser commit 중 exception → dtor 자동 cleanup (mTransition=Idle, mPending.reset)
- winner commit 중 exception → pending 남지만 initiated trait 소멸 시 cancel (§9 UnregisterMember)

### 5.2 OneUIComponents

```csharp
OnItemSelectionChanged(newSelection, e) {
  if (_onSelectionUpdating) return;
  // ... no guard, exception 시 _onSelectionUpdating 영구 true
  _onSelectionUpdating = false;  // ← 만약 위에서 exception 발생하면 skip!
}
```

**결함**:
- try-finally 또는 guard 객체 없음
- exception 시 `_onSelectionUpdating` permanently true → group 영구 lock

---

## 6. "RadioButton 컴포넌트" 설계 최적성

### 6.1 요구사항 정의

RadioButton UI 컴포넌트 = **icon + optional text + selectable state**.

요구:
1. **Visual state 동기화**: selected ↔ icon animation
2. **Group membership**: exclusive selection
3. **No-deselect**: 선택된 라디오 재클릭 시 no-op
4. **Accessibility**: A11y announce (role + state)
5. **Composability**: text label, context menu, disabled state 등과 조합

### 6.2 설계 패턴 비교

#### **Pattern A: dali-ui (trait → hook → accessibility)**

```cpp
class RadioButton : public View {
  // member의 visual state는 자신의 SelectedChanged 신호로 driven
};

GroupSelectableTraitImpl::OnSelectionChanged(view, selected, event) {
  WriteCheckedState(view, selected);  // ← ACCESSIBILITY LOCK-STEP
  NotifyCommitted(...);
}
```

**장점**:
- ✅ trait hook이 accessibility RMW를 자동으로 보장
- ✅ base SelectableTraitImpl의 signal flow 불변 (호환성)
- ✅ icon animation은 기존 `SelectedChanged` signal로 driven (decouple)

**단점**:
- ⚠️ 접근성이 "내부 구현 detail"로 숨겨짐 (문서화 필수)
- ⚠️ override 실수 시 CHECKED 동기화 누락 가능

#### **Pattern B: OneUIComponents (component → OnSelectedChanged override)**

```csharp
class RadioButton : GroupSelectable {
  protected override void OnSelectedChanged(KeyDeviceClass device) {
    base.OnSelectedChanged(device);
    _icon.SetSelection(IsSelected, isAnimated);  // ← visual only
  }
}
```

**장점**:
- ✅ simple, 명확한 책임 분리 (icon animation only)
- ✅ per-component override로 유연성

**단점**:
- ❌ accessibility CHECKED 동기화 없음
- ❌ 내부 `IsDeselectable` override도 a11y와 disconnected

#### **Pattern C: 최적안 (dali-ui + component override)**

```cpp
class RadioButton : public View {
  // layout + icon child, selectable trait auto-created by View
protected:
  void OnSelectionChanged(const InputEvent& event) override {
    // Component-level visual sync (optional, if needed)
    _icon.Animate(IsSelected());
  }
  // accessibility는 trait OnSelectionChanged hook에서 자동 처리
};
```

**이점**:
- ✅ trait hook (RMW automation) + component override (visual flexibility)
- ✅ inheritance chain 명확 (`View::AsSelectable()` 또는 explicit trait)
- ✅ icon state는 기존 signal로도 driven 가능

---

## 7. 비판적 평가: 설계 결함 및 해결책

### 7.1 dali-ui의 한계

| 결함 | 근거 | 비판 | 해결책 |
|-----|-----|------|-------|
| `GroupSelectableTrait::New()` base 이름-가리기(§6) | compile error 원도, 아래 orphan 차단 | **과도한 방어**(C++ 모범이지만 learner 진입장벽) | 문서화만으로 충분할 수도(test 추가) |
| post-commit hook `OnSelectionChanged` weak coupling | virtual hook, 문서 의존 | **명확성 vs flexibility trade-off** | base impl에서 accessibility RMW를 기본으로 처리(dali-ui 현행이 이미 이 방향) |
| event 저장 거부(pending에 `InputEvent` 미저장) | `OnSelectionChanging`엔 event 없음 | **winner post-commit event만 전파** → reclick/direct SetSelected(false) 시 event=None() | design decision ok, winner가 유일한 신호 source(clear도 명시적 None) |
| `CommitSelectedState` + `CommitSelectionFromGroup` 이중 wrapper | 접근 경계 명확화 | **래퍼 수가 많아 learning curve** | 코드 예제 + 다이어그램 필수 |

### 7.2 OneUIComponents의 치명적 결함

| 결함 | 근거 | 영향도 | 수정 방법 |
|-----|-----|--------|---------|
| **selected 재클릭 로직 미완성** | `IsDeselectable=false`인데 `Toggle()` 직접 호출 | **HIGH**: no-op 보장 없음, exception 안전성 미흡 | Plan.md의 `RequestSelectionChange` veto 로직 적용 필수 |
| `_onSelectionUpdating` local flag | 단일 frame guard만 가능 | **HIGH**: exception 발생 시 영구 lock | persistent state + scope guard (dtor cleanup) |
| CHECKED accessibility 미동기화 | `OnSelectedChanged` override도 role만 set | **MEDIUM-HIGH**: A11y announce 불가 | `OnSelectionChanged` hook에서 state 교체 + RMW |
| string-based registry (Find) | O(n) lookup, GC 압력 | **LOW-MEDIUM**: 성능 미흡 | weak handle or parent-based grouping |

### 7.3 도출: RadioButton 최적 설계

**RadioButton을 위한 GroupSelectableTrait의 필요충분조건**:

1. **Mutual exclusion 강제**: ✅ dali-ui plan (§247 veto) > OneUI (flag guard)
2. **No-deselect semantics**: ✅ dali-ui (explicit veto + transition guard)
3. **Accessibility RMW**: ✅ dali-ui (trait hook) ⚠️ OneUI (missing)
4. **Signal ordering**: ✅ dali-ui (persistent pending) ⚠️ OneUI (recursive)
5. **Exception safety**: ✅ dali-ui (scope guard) ❌ OneUI (no cleanup)
6. **Lifecycle cleanup**: ✅ dali-ui (OnDetaching/OnViewDestroying) ⚠️ OneUI (event unsubscribe)
7. **Composability**: ✅ dali-ui (trait → hook decouple) ✅ OneUI (component override easy)

---

## 8. 최종 권장 설계

### 8.1 핵심 결론

**RadioButton 컴포넌트를 위한 배타적 선택 인프라 설계로서, dali-ui의 GroupSelectableTrait 설계(plan.md)가 기술적으로 월등히 우수하다.**

**이유**:
1. **구조적 안전성**: trait-based hook + persistent transition state (exception 안전, 재진입 차단)
2. **Accessibility 규정 준수**: CHECKED RMW 자동화 (a11y announce 필수)
3. **명확한 신호 순서**: post-commit flush 지점 명확화 (app callback 예측 가능)
4. **생명주기 자동화**: `OnDetaching`/`OnViewDestroying` hook (dangling handle 방지)
5. **접근 경계 명확화**: public wrapper + no friend (감사 가능, refactoring 안전)

### 8.2 OneUIComponents 대비 개선 사항

dali-ui plan.md에서 OneUIComponents의 결함을 모두 해결:

| OneUI 결함 | dali-ui 해결책 |
|----------|-------------|
| flag-based local guard (exception 취약) | persistent `mTransition` + ScopeGuard with dismiss |
| string registry lookup | trait → group strong handle (O(1)) |
| "selected 재클릭" 로직 미완성 | `RequestSelectionChange` veto (boolean) |
| CHECKED accessibility 미동기화 | `OnSelectionChanged` hook + RMW helper |
| recursive toggle 호출 위험 | `CommitSelectionFromGroup` (hook bypass, loop 안전) |

### 8.3 RadioButton 구현 체크리스트

dali-ui의 GroupSelectableTrait 기반 구현 시 필수:

- [ ] Phase 0: Base `SelectableTraitImpl` 수정
  - [ ] protected virtual `OnSelectionChanged(View, bool, InputEvent)` 추가
  - [ ] protected `CommitSelectedState(bool, InputEvent)` 추가
  - [ ] `SetSelectedInternal` 내부에서 `CommitSelectedState` 호출

- [ ] Phase 1: `SelectableGroupImpl` 구현
  - [ ] `RequestSelectionChange` veto 로직 (멱등성 + no-deselect)
  - [ ] ScopeGuard (dismiss-on-success, exception 안전)
  - [ ] `NotifyCommitted` post-commit flush
  - [ ] `UnregisterMember` (생명주기 취소)

- [ ] Phase 2: `GroupSelectableTraitImpl` 구현
  - [ ] `OnSelectionChanging` 위임
  - [ ] `OnSelectionChanged` CHECKED RMW + `NotifyCommitted`
  - [ ] `BindGroup`/`UnbindGroup` (rebind 안전성)
  - [ ] `CommitSelectionFromGroup` public wrapper
  - [ ] `static DALI_INTERNAL New(SelectableGroup)` (orphan 차단)

- [ ] Phase 3: Accessibility
  - [ ] `RADIO_BUTTON` role (OnAttached)
  - [ ] CHECKED seed + RMW lock-step
  - [ ] loser·winner 양쪽 갱신

- [ ] Phase 4: Tests (§13 요지)
  - [ ] reentrancy all blocked (except group signal)
  - [ ] signal ordering swap/clear (post-mutation assert)
  - [ ] pending cancel (initiator lifecycle)
  - [ ] no-deselect veto
  - [ ] CHECKED bit 보존 (다른 a11y bit)

---

## 9. 비편향 종합 평가표

### 기준: RadioButton 컴포넌트 요구사항

| 평가 항목 | 만점 | dali-ui | OneUI | 근거 |
|----------|-----|--------|-------|------|
| **배타성 강제** | 5 | 5 | 3 | dali-ui: veto hook (구조적), OneUI: flag (제한적) |
| **No-deselect** | 5 | 5 | 2 | dali-ui: 명확한 veto, OneUI: IsDeselectable 미완성 |
| **Exception 안전성** | 5 | 5 | 1 | dali-ui: ScopeGuard, OneUI: try-finally 없음 |
| **Signal 순서 보장** | 5 | 5 | 3 | dali-ui: persistent pending, OneUI: recursive |
| **Accessibility RMW** | 5 | 5 | 0 | dali-ui 구현, OneUI 누락 |
| **생명주기 자동화** | 5 | 5 | 3 | dali-ui: hook, OneUI: event unsubscribe |
| **코드 명확성** | 3 | 2 | 4 | dali-ui: wrapper 많음(학습곡선), OneUI: simple |
| **Composability** | 5 | 5 | 5 | 동등 (둘 다 component override 가능) |
| **Performance** | 4 | 4 | 3 | dali-ui: O(1) handle, OneUI: string lookup |
| **문서화** | 3 | 3 | 4 | dali-ui: 상세하지만 복잡, OneUI: 단순 |
| **합계** | 50 | **44** | **28** | **dali-ui 우위 57%** |

---

## 10. 최종 권고

### RadioButton 컴포넌트를 위한 기능 설계 방안

**채택 설계: dali-ui GroupSelectableTrait (plan.md)**

**이유 (우선순위 정렬)**:
1. **기능 완결성** (4/5): 모든 요구사항(배타성, no-deselect, 신호 순서, a11y, 생명주기) 커버
2. **안전성** (5/5): exception 안전 + 재진입 차단 (persistent state + scope guard)
3. **규정 준수** (5/5): WCAG/A11y CHECKED RMW 자동화 (OneUI 부재)
4. **Extensibility** (5/5): post-commit hook으로 component/app override 가능

**OneUIComponents 대비 초과 기능**:
- ✅ Persistent transition state (exception 안전)
- ✅ Accessibility CHECKED RMW automation
- ✅ Post-commit hook (signal ordering 보장)
- ✅ Weak-handle registry (O(1) + 사이클 없음)

**OneUIComponents 장점 (학습 요소)**:
- ✅ String-based grouping의 명확성 (명시적 이름)
- ✅ Simple component override (RadioButton visual customization)
- → **dali-ui에서도 adopting 가능** (post-commit hook 활용)

---

## 11. 실행 순서

1. **dali-ui plan.md의 Phase 0-3 구현** (Base → SelectableGroup → GroupSelectableTrait → Accessibility)
2. **RadioButton 컴포넌트 구현** (View + auto-trait + icon child)
3. **테스트 & 문서** (§13 체크리스트 + accessibility a11y 검증)
4. **후속 고려사항** (OneUI 패턴 중 string grouping 명시성을 dali-ui에서도 지원 가능 — Phase 4)

---

## 12. 결언

dali-ui의 GroupSelectableTrait 설계는 **RadioButton 컴포넌트를 위한 배타적 선택 인프라로서 구조적으로, 기능적으로, 안전성 측면에서 우월하다**. 특히:

- **Exception 안전성**: persistent state + scope guard (OneUI의 flag 기반 guard 대비 본질적으로 우수)
- **Accessibility 규정**: CHECKED RMW 자동화 (OneUI의 누락 결함 해결)
- **Signal ordering**: post-commit hook (recursive 호출 회피)
- **No-deselect semantics**: veto hook + transition guard (완전하고 명확)

이 설계는 C++ trait 인프라의 구조적 특성(hook, RAII, weak handle)을 최대한 활용하여 **가장 작은 API 확장**으로 **가장 명확한 의도**와 **가장 높은 안전성**을 달성한다.
