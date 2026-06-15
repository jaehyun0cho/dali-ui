# GroupSelectableTrait 최종 설계 및 구현 방안 (확정·자기완결 문서)

> 이 문서 하나만으로 모든 결정·수정·근거를 알 수 있도록 작성되었다. 외부 문서 참조 없음.
> 인용된 `파일:라인`은 dali-ui 소스(`dali-ui-foundation/`)에서 도구로 검증한 사실이다.
> 대상: RadioButton 컴포넌트의 백킹 인프라. 범위: `SelectableGroup` + `GroupSelectableTrait`.
> 도출: 여러 후보 설계안을 무편향 적대적 검토로 수렴시킨 확정안이다. 핵심 난점 4가지를 모두 해소했다 —
> ① C++ 접근 경계(group↔trait의 protected/private 위반), ② cross-call transition 가드(stranding·예외 안전),
> ③ post-commit 시그널 순서·event 전파, ④ `GroupSelectableTrait::New()`의 base 상속 이름-가리기 함정.

---

## 0. 요구사항과 확정 결정

한 group에서 최대 하나의 member만 selected. 한 member가 `selected=true`가 되면 같은 group의 이전 winner는 자동으로 `selected=false`.

- **기본 정책 OPTIONAL**: group은 비어 있을 수 있고 프로그램적으로 비울 수 있다. **단, 사용자가 선택된 라디오를 재클릭해도 절대 해제되지 않는다(no-op).**
- **clear의 단일 진입점은 `SelectableGroup::ClearSelection()`.** grouped 멤버의 상속 `SetSelected(false)`는 winner인 경우 no-op.
- **`OnSelectionChanging`에 `InputEvent`를 추가하지 않는다.** 재클릭 vs clear 구분은 event가 아니라 호출 경로로.
- 제외: `SelectableView`, `GroupSelectableView`, `View::AsGroupSelectable()`, 문자열 group name registry, non-exclusive group, 키보드 내비게이션, `RADIO_GROUP` role, RadioButton 시각 컴포넌트.

---

## 1. 검증된 dali-ui 사실

### 1.1 SelectableTraitImpl 접근 수준 (접근 경계의 근거)
- `SelectableTraitImpl : public TraitObject(=BaseObject), public ConnectionTracker` (selectable-trait-impl.h:48).
- **protected**: `~dtor`, `GetOwner()` (h:92), `OnAttached/OnDetaching/OnViewDestroying`, `OnSelectionChanging(View,bool)` (h:119).
  (이번 설계가 추가하는 `CommitSelectedState`·`OnSelectionChanged`도 protected.)
- **private**: `SetSelectedInternal(bool,InputEvent)` (h:124), `OnClickedForToggle`, 데이터(`mOwner` WeakHandle<View> h:128, `mSelected` 등 h:130-131).
- `GetImpl(SelectableTrait&) → SelectableTraitImpl&` (h:137-139) — 외부는 impl의 **public 멤버만** 호출 가능.
- **`SelectableGroupImpl`은 별도 `BaseObject`이고 trait의 서브클래스도 friend도 아니다.** [class.protected]에 의해 trait의 protected
  `CommitSelectedState`/`GetOwner`를 **어떤 포인터로도 호출 불가**, 또한 trait impl은 group의 private(`pending`/`transitionState`/signal)을 만질 수 없다.
  → 합법 다리는 ① 서브클래스 `GroupSelectableTraitImpl`의 **public 래퍼**(forward), ② `SelectableGroupImpl` 자신의 **public 메서드**(reverse)뿐.

### 1.2 SelectableTraitImpl 동작
- `SetSelectedInternal(bool,InputEvent)` (cpp:70-93): no-op `if(mSelected==selected)`(cpp:72); store-before-attach `if(!owner){mSelected=selected;return;}`(cpp:77-83);
  veto `if(!OnSelectionChanging(owner,selected)) return;`(cpp:85); commit `mSelected=...; IntegrationView::SetState(owner,ViewState::SELECTED,selected); mSelectionChangedSignal.Emit(owner,...)`(cpp:90-92, item Emit이 **마지막 줄**). **post-commit 훅 없음.**
- `OnSelectionChanging` base는 true(cpp:151-154), **in-tree override 0개**. `SetSelected`→`SetSelectedInternal(sel,InputEvent::None())`(cpp:67).
  toggle ON 기본(ctor cpp:43-48); `OnClickedForToggle`→`SetSelectedInternal(!mSelected,event)`(cpp:187-190) → 선택 항목 재클릭 시 false 요청.
  base `OnAttached`가 `mOwner`를 설정(cpp:130), 단일 소유자 assert(`DALI_ASSERT_ALWAYS`, cpp:129).
- **`SelectableTrait`에 `static SelectableTrait New()`가 존재**(selectable-trait.h:74), 반환형은 **base** `SelectableTrait`.
  → 파생 `GroupSelectableTrait`가 `New`를 선언하지 않으면 `GroupSelectableTrait::New()`가 **상속된 base New()로 해소되어 base 타입(plain) 핸들을 반환**한다(이름-가리기 함정, §6).
- `View::AsSelectable()`→`EnsureSelectableTrait()`: `SELECTABLE_TRAIT` 슬롯 1개, 없으면 plain `SelectableTrait::New()` (view-impl.cpp:478-490).

### 1.3 InputEvent / 핸들 컨벤션
- `InputEvent : public BaseHandle` **Rule-of-5** (input-event.h:46,57-87). `InputEvent::None()` 유효 핸들·타입 NONE (input-event.h:108).
  `IntegrationView::SetState(viewImpl, ViewState, bool on, InputEvent cause = InputEvent::None())` (view-integ.h:142).
- 핸들 연산자: **trait 계열 = copy ctor + 비가상 dtor만**(`InteractiveTrait` interactive-trait.h:87,94; `SelectableTrait` selectable-trait.h:93,100).
  **비-trait BaseHandle = Rule-of-5**(`InputEvent`).

### 1.4 WeakHandle / Trait 생명주기
- `WeakHandle<T>`는 모든 BaseObject 기반 핸들에 동작(weak-handle.cpp:40-43, 66-68). `TraitObject:BaseObject` → `WeakHandle<GroupSelectableTrait>` 유효.
- 뷰 소멸: `OnViewDestroying`(view-data-impl.cpp:474-480). 슬롯 교체/제거: `OnDetaching`→`OnAttached`(view-data-impl.cpp:512-526).

### 1.5 접근성 (CHECKED 기록 = RMW 필수)
- `AccessibilityRole`: `CHECK_BOX`(:47), `RADIO_BUTTON`(:65), `TOGGLE_BUTTON`(:71). **`RADIO_GROUP` 없음** (view-accessibility-enums.h).
- `CHECKED`≠`SELECTED`. radio CHECKED announce는 위 3종 role에서 **CHECKED 비트 변화 시에만**(view-accessible.cpp:749-753).
  `ViewState::SELECTED`는 `AccessibilityState::SELECTED`에 매핑(CHECKED 아님, view-accessible.cpp:358-360).
- `Ui::View::Property::ACCESSIBILITY_STATES`는 INTEGER 비트셋(view.h:1710; 등록 view-data-impl.cpp:374). **SetProperty가 비트셋 전체 교체**(view-data-impl.cpp:1218-1219).
  마스크 `1u << static_cast<uint32_t>(AccessibilityState::CHECKED)`(view-accessibility-data.cpp:94), **`ENABLED` 기본 1**(view-accessibility-data.cpp:92) → 마스크만 쓰면 ENABLED 손실. **→ CHECKED 기록 = RMW 필수.**

### 1.6 규약
- 네임스페이스 `Dali::Ui`. **가상 추가 = additive vtable(기존 override 보존)**; 기존 가상 시그니처 변경 = de-override(더 파괴적); **비가상 private→protected = ABI 중립**.
  소스 변경 시 automated-tests/docs/manual-tests/samples 갱신. 타 프레임워크명 금지.

---

## 2. 프레임워크 조사 요약

라디오 그룹 멤버십 4분류: 컨테이너/부모, 명명 키+레지스트리, **명시적 논리 그룹 객체**, 공유 값 바인딩. dali-ui는 trait 기반·`OnSelectionChanging` 확장점이라
**명시적 논리 그룹 객체**가 최적. 핵심 요구: ① 그룹이 배타성 강제 ② **선택 라디오 재클릭 = no-op** ③ 빈 그룹은 API로 가능하나 제스처로 불가
④ 그룹 단위 (이전,현재) 알림 ⑤ 약한 참조 누수 방지 ⑥ 멤버 소멸 시 자동 탈퇴 ⑦ 멤버 radio role + checked.

---

## 3. 최종 결정 요약

| 항목 | 결정 |
|---|---|
| group controller / item trait | `SelectableGroup` / `GroupSelectableTrait : SelectableTrait` (기존 selectable trait slot 공유) |
| View class / `View::AsGroupSelectable` | 만들지 않음 |
| plain selectable 자동 교체 | 금지 (slot에 plain trait 있으면 `Add` false) |
| pre-change hook | `OnSelectionChanging(View,bool)` 시그니처 유지 (event는 veto 미사용, commit/signal 전파만) |
| base commit helper | protected `CommitSelectedState(bool,InputEvent)` (no-op 가드 + **pure null-owner early-return**) |
| `SetSelectedInternal` | private 유지 |
| post-commit hook | protected `OnSelectionChanged(View,bool,InputEvent)` |
| group↔trait 호출 | **양방향 public 래퍼** (forward: `CommitSelectionFromGroup`/`GetOwnerView`/`BindGroup`/`UnbindGroup`; reverse: `RequestSelectionChange`/`NotifyCommitted`/`UnregisterMember`). friend 없음 |
| selected 재클릭 / 직접 `SetSelected(false)` | 항상 veto(no-op) |
| clear | `ClearSelection()`만 허용 |
| `allowEmptySelection` 기본 | `true` |
| group signal | `SelectedMemberChangedSignal(group, previous, current, event)` (previous→current) |
| signal 순서 | previous item false → current item true → group changed |
| pending | previous/current View + initiator trait 저장, **event는 저장 안 함**(winner post-commit event 사용) |
| transition state | `enum {Idle, Transitioning}` persistent group state + **dismiss-on-success scope guard**(예외 안전) |
| 재진입 | transition 중 모든 public 변경 거부, group signal 단계 허용 |
| 생명주기 취소 | initiator 소멸/detach 시 pending 취소 + Idle |
| `GroupSelectableTrait::New` | `static DALI_INTERNAL GroupSelectableTrait New(SelectableGroup)` — base no-arg New() 가림 + Add 전용(public no-arg/orphan 차단) |
| 핸들 연산자 | `SelectableGroup` Rule-of-5 / `GroupSelectableTrait` copy ctor + dtor |
| ownership | trait → group strong, group → member weak |
| accessibility | item role `RADIO_BUTTON`, `CHECKED` bit RMW lock-step |

---

## 4. Public API

```cpp
// public-api/selectable-group.h [NEW] — 비-trait BaseHandle → Rule-of-5
class DALI_UI_API SelectableGroup : public BaseHandle
{
public:
  SelectableGroup();
  static SelectableGroup New();                          // exclusive 고정, allowEmptySelection=true
  static SelectableGroup DownCast(BaseHandle handle);
  SelectableGroup(const SelectableGroup&);
  SelectableGroup(SelectableGroup&&) noexcept;
  SelectableGroup& operator=(const SelectableGroup&) = default;
  SelectableGroup& operator=(SelectableGroup&&) noexcept = default;
  ~SelectableGroup();

  // (group, previousSelected, currentSelected, originating event); 빈 View == 선택 없음. clear의 event는 None().
  Signal<void(SelectableGroup, View, View, InputEvent)>& SelectedMemberChangedSignal();

  bool     Add(View member);     // 슬롯 비었거나 GroupSelectableTrait면 등록 후 true; plain SelectableTrait면 false; transition 중 false
  bool     Remove(View member);  // grouping만 해제(selected bool·CHECKED 보존); transition 중 false
  uint32_t GetMemberCount() const;                       // 살아 있고 등록된 member 수. GetMemberAt 미제공(weak 인덱스 불안정)

  View     GetSelectedMember() const;
  bool     SelectMember(View member);                    // 멤버면 member.SetSelected(true), 아니면 false; transition 중 false
  bool     ClearSelection();                             // allowEmpty면 비우고 true; 아니면/transition 중 false. clear 단일 진입점

  void     SetAllowEmptySelection(bool allow);  bool IsEmptySelectionAllowed() const;     // 기본 true

public: explicit SelectableGroup(Integration::SelectableGroupImpl* impl);
};

// public-api/group-selectable-trait.h [NEW] — SelectableTrait 상속, trait 계열 → copy ctor + dtor
class DALI_UI_API GroupSelectableTrait : public SelectableTrait
{
public:
  GroupSelectableTrait();
  static GroupSelectableTrait DownCast(BaseHandle handle);
  GroupSelectableTrait(const GroupSelectableTrait&);
  ~GroupSelectableTrait();
  SelectableGroup GetGroup() const;                      // 조회용(membership 변경 금지 — 문서화)
  // SetSelected(false)는 grouped winner에서 no-op(§0). 인스턴스 생성은 SelectableGroup::Add(View) 경유.
public: explicit GroupSelectableTrait(Integration::GroupSelectableTraitImpl* impl);
};
```

`SelectableGroup::SelectedMemberChangedSignal`은 group-level이며 item-level `SelectionChangedSignal`(상속)과 **이름을 분리**해 혼동을 줄인다.

---

## 5. Base SelectableTraitImpl 수정 (신규 가상 1 + protected 헬퍼 1, 시그니처 불변)

```cpp
class SelectableTraitImpl : public TraitObject, public ConnectionTracker {
protected:
  virtual bool OnSelectionChanging(View view, bool newSelected);              // 변경 없음
  virtual void OnSelectionChanged(View view, bool selected, InputEvent event); // 신설, base no-op
  void CommitSelectedState(bool selected, InputEvent event);                  // 신설 비가상, hook 미진입
private:
  void SetSelectedInternal(bool selected, InputEvent event);                  // private 유지
};
```
- `OnSelectionChanging(View,bool)` **시그니처 유지**(event 미추가, 오버로드 미추가): veto에 event 불필요 + override 실수·name hiding 위험 회피.
- `CommitSelectedState`:
  ```text
  if(mSelected == selected) return;              // no-op 가드 — 중복 그룹 commit의 phantom 시그널/CHECKED 방지
  owner = mOwner.GetHandle();
  if(!owner) return;                             // ★ PURE early-return(store-and-return 아님) — store-only면 시그널·OnSelectionChanged 미발생인데
                                                 //   pending이 설정돼 Transitioning 영구 stranding. 그룹은 attached 멤버만 commit한다.
  mSelected = selected;
  IntegrationView::SetState(GetImpl(owner), ViewState::SELECTED, selected, event);   // event를 cause로(view-integ.h:142)
  mSelectionChangedSignal.Emit(owner, mSelected, event);          // item 시그널(기존 cpp:92)
  OnSelectionChanged(owner, selected, event);                    // 신규 post-commit, item Emit 직후
  ```
- `SetSelectedInternal`: store-before-attach(store-and-return) 유지; veto 통과 후 `CommitSelectedState(selected, event)` 호출. **private 유지**(group은 직접 호출 안 함).
- `OnSelectionChanged`: base no-op. `GroupSelectableTraitImpl`이 여기서 CHECKED RMW + `NotifyCommitted` 호출.

---

## 6. C++ 접근 경계 (양방향 public 래퍼, friend 없음)

group impl은 trait의 protected/private을 직접 만지지 않고, trait impl도 group의 private을 직접 조작하지 않는다(§1.1).

```text
SelectableGroupImpl → GroupSelectableTraitImpl (forward, GroupSelectableTraitImpl의 public 래퍼):
  CommitSelectionFromGroup(selected, event)   // → 상속 protected CommitSelectedState (veto 우회: sibling/clear/reconcile 해제)
  GetOwnerView()                              // → 상속 protected GetOwner() (h:92)
  BindGroup(group) / UnbindGroup(group)       // group back-pointer 설정/해제(Add/Remove 크로스그룹 rebind)

GroupSelectableTraitImpl → SelectableGroupImpl (reverse, SelectableGroupImpl의 public 메서드):
  RequestSelectionChange(trait, ownerView, newSelected)   // OnSelectionChanging이 위임
  NotifyCommitted(trait, ownerView, selected, event)      // OnSelectionChanged가 호출(post-commit flush)
  UnregisterMember(trait)                                 // OnDetaching/OnViewDestroying이 호출
```

```cpp
class GroupSelectableTraitImpl : public SelectableTraitImpl {
public:   // integration 협업점 (일반 사용 금지 — 헤더 주석 명시; CommitSelectionFromGroup은 veto 우회이므로 group-internal 전용)
  void CommitSelectionFromGroup(bool selected, InputEvent event);
  View GetOwnerView() const;
  void BindGroup(SelectableGroupImpl* group);
  void UnbindGroup(SelectableGroupImpl* group);
  static DALI_INTERNAL GroupSelectableTrait New(SelectableGroup group);   // §아래
protected:
  bool OnSelectionChanging(View, bool) override;
  void OnSelectionChanged(View, bool, InputEvent) override;
  void OnAttached(TraitId, View&) override;
  void OnDetaching(TraitId, View&) override;
  void OnViewDestroying(ViewImpl*) override;
private:
  IntrusivePtr<SelectableGroupImpl> mGroup;   // STRONG (trait→group). group→member는 WEAK → 사이클 없음
};
// 신규 free function (h:137 패턴): Integration::GroupSelectableTraitImpl& GetImpl(GroupSelectableTrait& obj);
```

`CommitSelectionFromGroup()`만 `CommitSelectedState()`를 호출한다(`SetSelectedInternal()` 미호출 → veto hook 미진입). `OnSelectionChanged()`는
group의 private을 직접 조작하지 않고 `NotifyCommitted()`만 호출하며, group impl이 자기 private state를 처리한다. **friend 선언 없이 빌드되고, state machine 소유자가 명확**하다.

### `GroupSelectableTrait::New` — base 상속 이름-가리기 함정 해소 (확정)
base `SelectableTrait`에 `static SelectableTrait New()`가 있으므로(§1.2), 파생이 `New`를 **선언하지 않으면**(이전 후보안 일부) `GroupSelectableTrait::New()`가
**상속된 base New()로 해소되어 base 타입(plain·ungrouped) 핸들을 반환**하는 함정이 남는다. 해소:
- 파생에 `static DALI_INTERNAL GroupSelectableTrait New(SelectableGroup group)`를 **선언**한다. 이름 가리기로 base의 모든 `New` 오버로드를 가려 `GroupSelectableTrait::New()`(무인자)는 **컴파일 에러**가 되고, 의미가 group-bound로 고정된다.
- `DALI_INTERNAL`로 두어 **`Add()` 내부 전용**(public 무인자 New 차단 + group-bound지만 미등록인 orphan handle을 public 표면에서 만들 수 없게). 정상 생성·등록·attach는 `SelectableGroup::Add(View)`가 한 번에 한다.

---

## 7. 내부 구조와 ownership

`SelectableGroupImpl`(BaseObject + ConnectionTracker) 상태:
```text
mMembers: std::vector<WeakHandle<GroupSelectableTrait>>    // WEAK; null 엔트리 compaction
mSelected: WeakHandle<GroupSelectableTrait>                // 현재 winner
mAllowEmptySelection: bool = true
mTransition: enum { Idle, Transitioning }
mPending: { View previous; View current; WeakHandle<GroupSelectableTrait> initiator; bool active=false }   // event 미저장
mSelectedMemberChangedSignal
```
- group은 member trait/View를 strong으로 장기 보유하지 않는다(member lifetime 연장 금지). `mPending`의 previous/current View는 transition 중 signal payload 안정성을 위한 **단기 보관**.
- `pending`에 `InputEvent`를 저장하지 않는다 — `OnSelectionChanging(View,bool)`엔 event가 없다. group signal의 event는 **winner의 `OnSelectionChanged(view,true,event)`가 받은 event**를 그대로 쓴다.
- trait이 group을 STRONG으로 보유 → app이 `SelectableGroup` 핸들을 버려도 member가 살아 있는 동안 group 동작 유지. group→member WEAK → 사이클 없음.

---

## 8. Selection 알고리즘 (option b — 접근-합법, stranding 없음)

**가드 모델**: transition은 `RequestSelectionChange()` stack frame을 넘어 base의 winner commit → winner의 `OnSelectionChanged()`까지 이어진다. 따라서
순수 stack-local guard로는 부족하며, `mTransition`/`mPending`은 **persistent group state**로 두고 `NotifyCommitted()` 또는 생명주기 취소가 완료한다.
다만 `RequestSelectionChange()` 임계 구역(예외 가능)은 **dismiss-on-success scope guard**로 보호한다 — 정상 완료 시 `Dismiss()`하여 플래그를 cross-call로 넘기고,
예외 시 미-dismiss dtor가 `mTransition=Idle` + `mPending.reset()`로 리셋한다.

```text
GroupSelectableTraitImpl::OnSelectionChanging(view, newSelected):
  return mGroup ? mGroup->RequestSelectionChange(*this, GetOwnerView(), newSelected) : true   // *this(서브클래스)·자기 view 전달

SelectableGroupImpl::RequestSelectionChange(GroupSelectableTraitImpl& trait, View ownerView, newSelected):
  if(mTransition == Transitioning) return false              // 외부 재진입 거부
  current = mSelected.GetHandle()
  if newSelected:
    if(current points to &trait) return true                // 멱등(base no-op, cpp:72)
    ScopeGuard g(*this);  // arm Transitioning. dtor:(미-dismiss면) Idle + mPending.reset()
    prevView = current ? GetImpl(current).GetOwnerView() : View()
    if(current) GetImpl(current).CommitSelectionFromGroup(false, InputEvent::None())   // loser 해제(PUBLIC 래퍼, hook 미진입)
    mPending = { prevView, ownerView, initiator: handleOf(trait), active: true }
    mSelected = handleOf(trait)                              // commit 전에 winner 기록
    g.Dismiss()                                              // 정상 완료: 플래그를 base의 winner commit까지 유지
    return true                                             // base가 winner commit → winner.OnSelectionChanged→NotifyCommitted가 flush+Idle
  else:                                                      // 외부 해제(재클릭/직접 SetSelected(false))
    if(current does not point to &trait) return true         // 비-winner: 무해(base no-op)
    return false                                            // winner-deselect → 무조건 veto(no-op), allowEmpty 무관

GroupSelectableTraitImpl::OnSelectionChanged(view, selected, event):   // post-commit (CommitSelectedState 마지막 줄)
  WriteCheckedState(view, selected)                          // ACCESSIBILITY_STATES RMW (winner true / loser false)
  if(mGroup) mGroup->NotifyCommitted(*this, view, selected, event)     // PUBLIC reverse; group private 미접근

SelectableGroupImpl::NotifyCommitted(trait, ownerView, selected, event):   // PUBLIC; group이 자기 상태를 flush
  if(mTransition != Transitioning) return                   // 방어(Idle이면 no flush)
  if(!mPending.active) return
  if(selected == false) return                              // loser/clear false는 flush 안 함
  if(mPending.initiator does not resolve to &trait) return  // initiator 식별(View 비교 아님 — slot 교체/재attach stale 방지)
  prev = mPending.previous; cur = mPending.current
  mPending.reset(); mTransition = Idle                      // 소비·해제 먼저 → emit 직전 Idle로 group 핸들러 재진입 허용
  mSelectedMemberChangedSignal.Emit(SelectableGroup(this), prev, cur, event)

SelectableGroupImpl::ClearSelection():
  if(mTransition == Transitioning) return false
  winner = mSelected.GetHandle(); if(!winner) return true
  if(!mAllowEmptySelection) return false
  prevView = GetImpl(winner).GetOwnerView()
  { ScopeGuard g(*this);                                    // clear는 단일 프레임 동기 완결 → dismiss 불필요; dtor가 Idle 복귀
    mSelected = WeakHandle(); mPending.reset()              // pending 미설정 → winner OnSelectionChanged→NotifyCommitted 게이트 실패(selected==false)
    GetImpl(winner).CommitSelectionFromGroup(false, InputEvent::None())   // winner item(false)+CHECKED=false
  }  // g.dtor → mTransition=Idle
  mSelectedMemberChangedSignal.Emit(SelectableGroup(this), prevView, View(), InputEvent::None())   // 직접 emit: item(false)→group
  return true
```

**불변식 / 순서**
- swap: `A.item(false,None)` → `A.CHECKED=false` → `B.item(true,event)` → `B.CHECKED=true` → `group(A,B,event)`. loser는 pending 설정 전 해제되어 flush 안 함. group emit 정확히 1회.
- clear: `winner.item(false,None)` → `group(prev,empty,None)`. post-commit flush 없음(게이트 실패), 직접 emit 1회.
- 첫 선택(이전 winner 없음): B commit → `group(empty, B, event)`.
- 재진입: item 단계 `Transitioning` → 외부 `SelectMember/SetSelected/Add/Remove/ClearSelection` 거부. group signal 단계 `Idle` → 중첩 fresh swap 허용. `CommitSelectionFromGroup`은 hook 미진입이라 재귀 없음.
- 방어: `NotifyCommitted`는 Idle/pending 불일치/selected==false에서 strict no-op → intervening clear 후 winner post-commit이 이중 emit 못 함.

**WriteCheckedState(view, on)**:
```cpp
const int mask = 1 << static_cast<int>(AccessibilityState::CHECKED);
int raw = view.GetProperty<int>(View::Property::ACCESSIBILITY_STATES);
view.SetProperty(View::Property::ACCESSIBILITY_STATES, on ? (raw | mask) : (raw & ~mask));   // 비트셋 전체 교체이므로 RMW 필수(§1.5)
```

---

## 9. 재진입·생명주기 (두 stranding 벡터 모두 제거)

- **모든 public 변경 거부**: `Transitioning` 중 `SelectMember`/`Add`/`Remove`/`ClearSelection`은 `false`, 외부 `SetSelected`는 `RequestSelectionChange` false 반환으로 veto. 그룹 내부 `CommitSelectionFromGroup`(hook 미진입)만 진행.
- **mid-transition 생명주기 취소**(public-변경 가드의 명시적 예외 — 파괴는 API 요청이 아님):
  ```text
  GroupSelectableTraitImpl::OnDetaching / OnViewDestroying(this):
    mGroup->UnregisterMember(*this)     // group: mMembers에서 제거(compaction); 아래 처리
  SelectableGroupImpl::UnregisterMember(trait):
    erase from mMembers
    if(mPending.active && mPending.initiator resolves to &trait): mPending.reset(); mTransition = Idle   // ★ 아직 commit 안 된 winner 소멸 stranding 차단
    if(mSelected resolves to &trait): mSelected = empty
    // 자동 승격 없음; cleanup 중 group signal emit 없음
  ```
- **stranding 잔여 없음(4 경로)**: 성공→`NotifyCommitted` 리셋 / initiator 소멸→위 취소 리셋 / 임계 구역 예외→미-dismiss ScopeGuard dtor 리셋 / 그 외 winner는 살아 있으면 base가 항상 `CommitSelectedState(true)` 호출→`NotifyCommitted`.
- **post-mutation tripwire**: 모든 top-level public 변경 반환 시 `DALI_ASSERT_DEBUG(mTransition==Idle && !mPending.active)`(debug-only).
- **OnAttached 체인 순서**: `GroupSelectableTraitImpl::OnAttached`는 `SelectableTraitImpl::OnAttached(id,view)`를 **먼저** 호출(cpp:130에서 `mOwner` 설정, 단일 소유자 assert cpp:129) 후 RADIO_BUTTON role·CHECKED seed·store-before-attach reconcile(reconcile이 `GetOwner`/`IsSelected` 읽음).

---

## 10. Add / Remove / Dynamic reconcile

`SelectableGroup::Add(View)` (단일 멤버십 진입점):
```text
empty View 또는 transition 중: false
selectable trait 없음: GroupSelectableTrait::New(group) 생성 → slot에 attach → register → reconcile → true
GroupSelectableTrait 있음: 다른 group이면 이전 group에서 unregister; BindGroup(this) → register → reconcile → true
plain SelectableTrait 있음: 교체 안 함 → false   // 기존 selected state·signal 구독자 유실 방지(자동 migration 금지)
```
`SelectableGroup::Remove(View)`: transition 중 false; member 아니면 false; 맞으면 unregister + `UnbindGroup` + winner였으면 `mSelected=empty`; **selected bool·CHECKED 보존**; true.

**store-before-attach / dynamic Add reconcile**("기존 winner 우선"):
```text
not selected: register only; seed CHECKED=false
selected & group has no winner: mSelected=this; seed CHECKED=true; no group signal
selected & group already has a winner: keep existing winner; this.CommitSelectionFromGroup(false, None) (transition 가드 하에); seed CHECKED=false; no group signal
```
이미 selected인 member를 나중에 추가해도 기존 winner를 자동으로 빼앗지 않는다(winner 변경은 `SelectMember()`만).

---

## 11. Accessibility 동기화

- `OnAttached`(base 체인 후): role `RADIO_BUTTON` 설정; 현재 selected에 맞춰 CHECKED seed; store-before-attach reconcile.
- `OnSelectionChanged`: selected 값이 바뀔 때마다 CHECKED RMW(§7 WriteCheckedState). winner(true)·loser(false) **양쪽** 갱신(loser 누락 시 stale announce).
- `ViewState::SELECTED`는 base commit path가 관리. radio announce엔 `CHECKED` 변화가 필요하므로 두 state를 별도 lock-step.
- 이번 범위 group role 미추가. `RADIO_GROUP`·roving focus·arrow 내비는 RadioButton/RadioGroupView 단계.

---

## 12. 구현 단계

0. **Phase 0 — Base**: protected `virtual OnSelectionChanged`(no-op) + protected `CommitSelectedState`(no-op·pure-null-owner 가드, event를 SetState cause로) +
   `SetSelectedInternal` 리팩터(private·시그니처 그대로). `utc-Dali-SelectableTrait.cpp` 회귀(토글·시그널·store-before-attach·cause 흐름·post-commit이 plain 동작 불변).
1. **Phase 1 — `SelectableGroup`(+Impl)**: Rule-of-5 핸들. New/DownCast, Add/Remove(bool, transition 거부)/GetMemberCount, GetSelectedMember/SelectMember/ClearSelection,
   SetAllowEmptySelection, `SelectedMemberChangedSignal`, 상태(mMembers/mSelected/mTransition/mPending), `RequestSelectionChange`, **public `NotifyCommitted`/`UnregisterMember`**,
   **ScopeGuard(dismiss-on-success)**, compaction, post-mutation assert. `utc-Dali-SelectableGroup.cpp`(swap 순서, 외부 winner-deselect veto, clear, 동적 추가/제거,
   **전체 재진입 거부**, **initiator 소멸 stranding 방지**, 예외 시 가드 리셋, 단일 emit, post-clear 이중 emit 방어).
2. **Phase 2 — `GroupSelectableTrait`(+Impl)**: `SelectableTraitImpl` 상속, STRONG `mGroup`, **`New(SelectableGroup)`은 `DALI_INTERNAL`**(base New() 가림 + Add 전용).
   override `OnSelectionChanging`(위임)/`OnSelectionChanged`(CHECKED RMW + `NotifyCommitted`)/`OnAttached`(**base 먼저** + role + CHECKED seed + reconcile)/`OnDetaching`/`OnViewDestroying`(UnregisterMember).
   **public `CommitSelectionFromGroup`/`GetOwnerView`/`BindGroup`/`UnbindGroup`** + `GetImpl(GroupSelectableTrait&)`. `EnsureGroupSelectableTrait`(Add 내부, 3분기).
   `utc-Dali-GroupSelectableTrait.cpp`(store-before-attach reconcile, CHECKED StateChanged, plain-trait 슬롯 거부, `SetSelected(false)` no-op 계약, **`GroupSelectableTrait::New()` 무인자 컴파일 에러 확인**, 접근 경계 컴파일).
3. **Phase 3 — docs / manual-tests / samples** + ABI 변경 로그(가상 추가 + public 헤더 2). 프레임워크 중립 표현만.
4. **Phase 4 (연기)**: `View::AsGroupSelectable`, in-place 업그레이드, mandatory 모드, GroupSelectableView, 화살표 내비/단일 탭 스톱, `RADIO_GROUP` enum, RadioButton View.

---

## 13. 테스트 계획 (요지)

- **Base 회귀**: plain `SetSelected(true/false)`·click toggle·동일값 no-signal·store-before-attach·post-commit이 plain 동작 불변.
- **Membership**: empty/plain-trait Add=false; trait 없는 View Add=생성+true; 다른 group의 GroupSelectableTrait Add=rebind; transition 중 Add/Remove=false; Remove는 selected bool·CHECKED 보존.
- **Mutual exclusion**: A,B,C 중 B select 시 A=false,B=true,C=false; 다른 group 무영향; `SelectMember(non-member)`=false; 멱등 재선택 no-op.
- **Reclick/clear**: 재클릭·직접 `SetSelected(false)` no-op(allowEmpty 무관); `ClearSelection` allowEmpty true=성공/false=실패.
- **Signal ordering**: swap=A item false(None)→B item true(event)→group(A,B,event); clear=winner item false(None)→group(winner,empty,None); 재클릭 veto=시그널 없음.
- **Reentrancy**: loser/winner item 콜백 내 C select 거부; group signal 콜백 내 C select 허용; transition 중 Add/Remove/Clear=false; pending 단일 emit; initiator detach/destroy 시 pending cancel; swap transition이 `RequestSelectionChange` return 이후 winner post-commit까지 유지.
- **Accessibility**: role RADIO_BUTTON; commit 후 CHECKED true/false; loser·winner 양쪽 CHECKED 갱신; 다른 bit 보존; store-before-attach true도 attach 후 CHECKED seed.
- **Lifecycle**: selected member destroy/detach 시 group selected empty; pending initiator destroy/detach 시 pending·transition 정리; cleanup 중 자동 승격·group signal 없음; group destroy 후 dangling 없음; weak compaction.
- **접근 경계**: `SelectableGroupImpl`이 trait protected/private 직접 호출 안 함; sibling/clear=`CommitSelectionFromGroup`; owner 조회=`GetOwnerView`; flush=`NotifyCommitted`; trait이 group private 직접 수정 안 함; `GroupSelectableTrait::New()` 무인자 컴파일 에러.

---

## 14. 기각한 대안 (근거)

- **`OnSelectionChanging`에 `InputEvent` 추가/오버로드**: 기각. 외부 deselect는 항상 veto, clear는 group 내부가 veto 우회 commit. event는 cause·signal 전파만. 시그니처 변경/오버로드는 override·name hiding 위험.
- **`SetSelectedInternal` protected 이동**: 기각. group이 호출하면 다시 veto hook을 거쳐 별도 bypass flag가 필요. protected `CommitSelectedState`가 승인된 내부 commit만 명확히 수행.
- **per-trait veto 우회 flag(`applyingGroupSelection`)**: 기각. set/reset 누락·예외·재진입 시 veto 영구 비활성/두 winner 실패군. hook을 아예 거치지 않는 `CommitSelectedState`가 구조적으로 안전.
- **pending에 event 저장**: 기각. `OnSelectionChanging`엔 event가 없어 정확히 채울 수 없다. winner post-commit event가 유일하게 자연스럽다.
- **stack-only transition guard**: 기각. swap transition은 `RequestSelectionChange` return 이후 base commit·winner post-commit까지 이어진다. persistent group state + `NotifyCommitted`/cancel 완료 + 임계 구역 dismiss-가드(예외 안전)가 필요.
- **per-item signal slot으로 group flush**: 기각. app 콜백과 group flush 순서를 API로 보장하기 어렵다. post-commit hook이 명확한 지점.
- **trait↔group friend 결합**: 기각. 경계를 흐리고 veto-우회 commit 감사를 어렵게 한다. 명시적 public 래퍼가 testable·경계 명확.
- **plain `SelectableTrait` 자동 migration / group-level View class**: 기각(범위 외/구독자 유실). 후속.

---

## 15. 최종 결론

`SelectableGroup` logical controller + `GroupSelectableTrait : SelectableTrait`(기존 selectable trait slot 공유). base `SelectableTraitImpl`은
`OnSelectionChanging(View,bool)` 시그니처를 유지하고 post-commit `OnSelectionChanged`와 protected `CommitSelectedState`(no-op·pure-null-owner 가드)만 추가하며 `SetSelectedInternal`은 private.
grouped winner의 외부 false(재클릭·직접 `SetSelected(false)`)는 항상 veto, clear는 `ClearSelection()`만. swap은 loser item false → winner item true → group changed 순서를 보장한다.

네 가지 난점을 모두 닫았다:
1. **접근 경계** — group↔trait 모든 cross-class 호출을 양방향 **public 래퍼**(`CommitSelectionFromGroup`/`GetOwnerView`/`BindGroup`/`UnbindGroup`, `RequestSelectionChange`/`NotifyCommitted`/`UnregisterMember`)로만 — friend 없이 컴파일.
2. **transition 가드** — persistent group state(cross-call) + **dismiss-on-success scope guard**(예외 안전) + initiator-소멸 취소 → stranding 4 경로 모두 차단.
3. **event 전파** — pending에 event 미저장, winner post-commit event를 group signal에 전달.
4. **`New` 함정** — `static DALI_INTERNAL GroupSelectableTrait New(SelectableGroup)`로 base `New()` 상속 이름-가리기를 닫고(무인자 `New()`는 컴파일 에러) Add 전용으로 orphan을 차단.

접근성은 `RADIO_BUTTON` role + `CHECKED` bit RMW(winner·loser 양쪽). ownership은 trait→group strong, group→member weak(사이클 없음). event는 commit/signal 전파용으로만 흐른다.
그룹 객체는 계층과 분리되어 있어 추후 `SelectableView`/`GroupSelectableView`·키보드·`RADIO_GROUP`을 비파괴적으로 얹을 수 있다. 이 설계는 RadioButton에 필요한 mutual exclusion,
reclick no-op, explicit clear, signal ordering, accessibility 동기화, C++ 접근 경계, reentrancy 방어, lifecycle cleanup을 가장 작은 API 확장과 가장 명확한 내부 경로로 만족한다.
