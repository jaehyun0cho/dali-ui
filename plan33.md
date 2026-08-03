# DALi UI — View Measure/Arrange 성능 개선 최종 수정 방안 (근거·폐기근거 포함)

- 대상: `~/dali/dali-ui-claude` (branch `claude`, HEAD `0b1476d0`).
- 상태: 검토 확정, **구현 보류**.
- 검증 방법: 본 문서의 모든 사실 주장은 실제 소스(dali-ui-foundation, dali-ui-components,
  dali-core)를 직접 열람하고, 핵심 명제는 선행 결론을 전제하지 않는 제로베이스 재검증(병렬
  독립검증·적대적 반증 포함)으로 재확인했다. 빌드·실행 테스트는 미수행(§15). 인용의 line
  번호는 검토 시점 기준이므로 구현 착수 시 재확인이 필요하다. dali-core 인용은 별도 표기하며,
  그 외는 `internal/views/view/view-data-impl.cpp` 기준이다.
- **정책 전제(승인된 결정)**: Layout 계산값과 actor target geometry는 현재 결과와 동일하게
  유지한다. 다만 외부 관찰 동작이 바뀌더라도 **일반적인 UI framework layout 시스템 관점에서
  정상으로 판단되는 수준이면 허용**한다. **잘못된 geometry나 필수 갱신 누락은 허용 밖**이다.

---

## 0. 핵심 결론

**Arrange 캐시의 최종 형태는 "표준 무효화 계약을 만족하는 모든 View에 적용되는 보편 캐시"**
이며, 이를 켜기 전에 무효화 정확성 수정·Measure 히트 비용 제거·결정적 LTR/RTL·직접 geometry
write 분리·fitting 분리·first-party producer 정비를 완료한다. 정확성의 핵심은 **다섯 상태를 각각
독립적으로 관리**하는 것이다 — ① 계산 결과의 존재, ② 캐시 최신성(freshness), ③ declarative dirty,
④ parent 좌표 문맥 유효성(context validity), ⑤ 물리 방향 해석. 특히 ②freshness와 ④context validity를
하나의 비트로 합치지 않는다. 상태 표현은 **필요 최소**로 유지한다 — dirty/poison 비트가 pass 내
단조(monotone)이므로 **invalidation generation 카운터 같은 잉여 상태는 도입하지 않고**(§3.3), 대신
"이 Measure/Arrange를 소유한 논리적 owner가 누구인가"는 트리 구조로 유도 불가하므로 **명시적 owner
scope**로 표현한다(§5.3).

---

## 1. 수정안 요약 (한 줄 근거)

| # | 수정 | 왜 (근거) |
|---|---|---|
| P0-1 | dirty를 pass 진입 시 소비 + 종료부 조건부 publish | pass 중 재무효화가 조상 DIRTY 조기반환에 스왈로우 + 종료부 무조건 valid 저장 → 오늘도 존재하는 sticky stale (§3.1) |
| P0-2 | 무효화 early-return 제거(무조건 root 전파) | 콜백 부모가 dirty 자식 Arrange 미호출 시 dirty 미소비 → 재무효화 "이미 등록" 오판 → 변경 영구 유실 (§3.2) |
| P0-3 | **Measure·Arrange 양 축에 release 재진입 가드**(nested 진입은 dirty 미소비·조기반환·바깥 pass poison) | `LayoutController::ProcessLayouts()`는 공개 API + 중첩 지원 → 앱 콜백이 진행 중 View를 재진입시켜 조건부 publish가 stale publish(ABA). 오늘 Measure는 가드 전무, Arrange는 debug assert뿐 (§3.3·§11.1) |
| C1 | effective-scale 동기화 → sync-required 비트(+애니메이션 훅, VIEW_EFFECTIVE_SCALE case) + sync-in-progress 가드 | 히트 지배 비용(actor read)을 비트로 대체. 외부 write 자기교정 유지엔 property case에서 bit set 필수 (§4) |
| C1-b | 일반 `InvalidateMeasure`에서 effective-scale 리셋 분리 (INHERIT 정책 한정 이득) | INHERIT 정책 View만 부모체인 재탐색; size/content 무효화마다 재탐색은 낭비 (§4.3) |
| C2 | Arrange 입력 캐시 + zero-touch hit (보편, 정비-선행 게이트) | 국소 변경 시 Measure는 O(dirty path)인데 Arrange만 O(N). 조상 hit는 자손 Arrange 완전 생략 (단 §2.3 recycler 예외) (§5) |
| C2-a | Measure miss ⟹ 조상 Measure+Arrange 캐시 무효화, **owner scope 경계에서 중단** | Arrange가 non-MATCH_PARENT 자식을 저장 measuredSize로 배치 → Arrange만 무효화 시 외부 직접 child.Measure 후 오배치. 중단 판정은 "measure-in-progress 조상"만으로 불충분(§5.3) |
| C2-owner | **dependency-owner scope**(owner 포인터 스택) 도입 — generation 카운터 불채택 | 5 stock 매니저·recycler가 **Arrange 안에서 child.Measure**를 호출(owner는 arrange-in-progress이지 measure-in-progress 아님) → 부모체인+in-progress 비트로 owner 귀속 표현 불가. dirty는 pass 내 단조라 generation은 잉여 (§3.3·§5.3) |
| C2-b | 재귀 스케일 리셋 + child-remove 시 제거 서브트리 재귀 리셋 | 리셋이 measure만 초기화하면 스케일-불변 입력 자식 hit로 손자 stale; child-remove는 오늘 미호출 (§5.4) |
| C2-c | freshness(poison/dirty)와 context validity 분리 + **context 조건부 publish**(pass 중 reparent/scale-context 오염 시 재유효화 금지) | dirty/physical write는 context 미접촉; 그러나 pass 중 reparent/scale 리셋은 context 무효화, scale 리셋은 descendant dirty 미설정 → 무조건 재유효화는 버그 (§5.5) |
| C2-d | direction을 required-bit 결정적 resolver로 | 현 토글은 involution — 캐시 hit가 OnArrange LTR 재설정 건너뛰면 이중 반전 노출 (§6) |
| C2-e | direction 변경 시그널을 공통 non-virtual 초기화에서 연결 | base View 미연결 — 방향만 바뀌면 pass 미예약 (§6.2) |
| C2-f | 모든 직접 geometry write를 중앙 `OnPropertySet` + target/index 일회성 token으로 구별, **외부 write는 보수적 무효화** | typed setter·SetProperty 모두 hook 발화; depth는 동기 signal 콜백 과억제; **hook은 post-set이라 값 변경 판정 불가 → 보수적 무효화**(SIZE는 OnSizeSet 우회로 정밀도 회복) (§7) |
| C2-g | fitting을 Arrange에서 분리 + natural-size 기여 visual의 resource-ready·SetBackground 시 InvalidateMeasure | plain View 크기 불변 후속 resource-ready 재적용 부재 + natural size 변화 시 measure 무효화 부재 (§8) |
| C2-h | View 단위 신호를 "full-Arrange 완료 신호"로 재정의 | per-view 구독 체크로는 구독 자손이 clean 조상 hit에 가려짐 (§9) |
| Pre-1 | built-in manager owner back-pointer + 전 setter 무효화 | built-in setter 무효화 미호출 + raw 접근 (§10.1) |
| Pre-2 | Chart·**RecyclerView(item Measure+Arrange)** 등 상태성 정비 | flag writer가 RelayoutRequest뿐; recycler layouter가 item을 **직접 Measure·Arrange**(스크롤 경로 포함) (§10.2) |
| Pre-3 | Label 릴리즈 로그 제거/강등 | `return bounds;`뿐인데 매 호출 릴리즈 로그 (§10.3) |
| Pre-4 | 이중 RTL 제거(CheckBox 등, 별도 커밋) | 자체 mapX + 공통 미러 재반전 — 비대칭 padding 오배치 (§10.4) |
| Keep-1 | 음수 constraint의 min-bound 정규화 동치 유지 | 유한 음수는 `max(neg, min≥0)`로 저장 min에 collapse (§2.2) |
| Defer-1 | raw Measure fast key 연기(측정 후) | sync-bit로 actor read 제거 후 잔여는 스칼라(<0.1%) + FloatEqual 단위 불일치 위험 (§12.Y) |
| Defer-2 | **invalidation generation 카운터 불채택** | dirty/poison이 pass 내 단조(유일 mid-pass clear=재진입, 재진입은 1비트로 관측) → generation은 잉여 (§3.3·§12.Gen) |
| Roll-1 | (전환기) exact subtree no-cache counter + scoped guard | 단계 배포 시 legacy descendant를 조상 hit가 삼키는 것 방지 (§5.6) |

---

## 2. 현재 구조 (제로베이스 재검증 확정)

### 2.1 구동 모델
`ProcessLayouts`는 pending root(unordered_set)를 swap 드레인 후 처리(layout-controller.cpp:696-698),
root마다 `Measure`→`Arrange` 재귀(:803-865). `RequestLayout`은 등록+스케줄만, dirty 미생성.
**`ProcessLayouts()`는 공개 API(layout-controller.h:128)이며 중첩을 명시 지원한다**(:384 `++mProcessDepth`,
:411 `if(mProcessDepth==1)`, :486 감소, :756 `ProcessLayoutRoot`→:847/:864 재-Measure/Arrange). ⟹
앱 콜백이 진행 중 pass를 재진입시킬 수 있다(§3.3의 ABA 근거).

### 2.2 Measure — 캐시 있음, 히트 전 비용
순서(:2778-2809): `GetEffectiveScale()`(지연 캐시 멤버 :1420-1427) → **actor read+비교+조건부 write**
(:2786-2792) → 단위 변환(:2794, `visualW > 0.f`일 때만 scale 나눗셈) → min/max 클램프(:2802; 저장 min은
`IsValidSizeBound` :330-332로 항상 `>= 0`, max 기본 FLOAT_MAX) → 비교·조기 반환(:2805-2809) → miss일
때만 dispatch·저장·standalone. **재진입 가드 없음**(mMeasureInProgress 부재).
- **히트 전 Trait 순회는 없다**: callback/manager 조회는 조기 반환 이후. 히트 낭비의 지배항은 actor
  animatable property read(:2786)이며 sync-bit(§4)가 이를 제거. 제거 후 잔여 pre-hit는 스칼라(§12.Y).
- **음수 constraint 동치**: `effNat = std::min(std::max(natRaw, min≥0), max)`이므로 유한 음수 모두 저장
  min으로 collapse → 같은 슬롯·히트. `:2805`의 `>= 0.f`는 dirty sentinel(DIRTY=-1 :137 / NaN :691) 차단용.

### 2.3 Arrange — 캐시 없음, O(N) 전파 (개선 본질)
매 호출(:2842-2908, early-return 없음; 재진입 가드는 debug assert :2850 + `mArrangeInProgress` :2851):
`ApplySelfBoundsIfChanged` ×2 + dispatch(trait 2회+dynamic_cast) + `mArrangedBounds=finalBounds`(:2878) +
standalone + `ApplyLayoutDirection`(:2889) + 신호 후보(:2902-2904).
- **조상 hit는 자손 Arrange를 완전 생략**한다(`ArrangeDefault` :940, 전 built-in manager가 자식
  `Arrange()`를 부모 Arrange 본문에서 호출).
- **⚠️ Arrange/Measure는 표준 재귀 밖의 진입점을 갖는다(재검증 확정)**:
  - **5 stock 매니저 전부가 자기 Arrange 안에서 MATCH_PARENT 자식을 재-Measure**한다 — absolute-layout
    -manager.cpp:358 / scroll-view-layout-manager.cpp:139 / flex-layout-manager.cpp:354 / grid-layout
    -manager.cpp:477 / stack(Arrange :329 내 동형). `ArrangeDefault`도 :937에서 재-Measure. 이 시점 owner는
    **arrange**-in-progress이지 measure-in-progress가 아니다.
  - **RecyclerView는 item view를 직접 Measure·Arrange**한다 — `LinearItemsLayouter`가
    `view.Measure(kUnconstrained, mCrossExtent)`(linear-items-layouter-impl.cpp:288/295; sentinel
    `kUnconstrained = FLT_MAX*0.25` :282)와 `view.Arrange(...)`(:291/298)를 item에 직접 호출한다. item은
    `mScroller`의 자식이며(recycler-view-impl.cpp:765/779) recycler는 mScroller를 Measure/Arrange하지
    않는다. 이 호출은 **스크롤 경로**(ScrollBy→Fill/LayoutChunk)에서도 발생해 어떤 layout pass도 스택에
    없다. 결과는 이중 캐시(layouter mExtentCache + item 자기 mMeasuredSize :2829-2832).
  - 공개 `View::Measure`(view.h:175)·`View::Arrange`(view.h:185, view.cpp:82)도 무가드 외부 진입점이다.
  ⟹ "조상 hit가 모든 자손 prune"은 recycler item에 적용되지 않으며, **owner 귀속은 부모 체인+in-progress
  비트로 유도 불가**하다(§5.3의 owner scope 근거).
- **`mArrangedBounds`는 그 View의 자기 parent-local *논리*(unmirrored) rect**이며 runtime writer는 오직
  1곳(:2878, `ResolveReturnedBounds(inputBounds, returnedBounds)` :2875). manager 경로 `returnedBounds=
  bounds`(:2866), `ArrangeDefault` `return bounds`(:944)로 입력 에코 → 자손 재측정은 이 rect를 안 바꿈.
- **부모 Arrange는 non-MATCH_PARENT 자식을 저장 measuredSize로 배치**(`GetMeasuredSize` :920; managers
  absolute:302/grid:427). ⟹ 자식 measuredSize 슬롯이 바뀌면 부모 Arrange 결과가 바뀐다(§5.3 근거).
- **RTL에서 actor 물리 X ≠ `mArrangedBounds.x`**: `ApplyLayoutDirection`(:2946-2966)이 미러 후 자식
  POSITION_X를 덮으므로(:2964). `mArrangedBounds`는 방향-무관 논리 캐시.
- **현 RTL 미러 involution은 오늘 캐시 부재로 은폐** — 캐시 hit가 OnArrange LTR 재설정을 건너뛰면 노출됨(§6).

### 2.4 무효화 모델
`mArrangeDirty` 생성-후 writer 3곳(:1449/:1503/:2879; ctor 별도). `mLastMeasuredConstraint`도 self writer만
(:691/:1447-1448/:2831-2832/:3009-3010) — **cross-view writer 0개**(§3.3 근거). 상향 전파는 재귀
`InvalidateMeasure` 호출(:1467-1487). geometry 입력 setter는 모두 `InvalidateMeasure` 경유.

### 2.5 확장점 제약과 geometry write 경로 (재검증 확정)
- `LayoutManager`는 ABI-frozen virtual + protected ctor, `GetLayoutManager()` raw 반환, built-in setter
  무효화 미호출. ⟹ 캐시 안전성은 타입이 아니라 무효화 계약으로 확보.
- **공개 `Dali::Actor` position/size setter는 전부 `SetProperty(index)` 경유**(dali-core actor.cpp:142-150,
  237-300). `Object::SetProperty`는 값 적용 후 `OnPropertySet`→`PropertySetSignal.Emit`을 **동기** 호출
  (object-impl.cpp:479-486). **값 변경 여부를 비교하지 않으며, 거부된 write에도 hook을 호출한다(TODO
  :477-478)**. hook은 post-set으로 `(index, newValue)`만 받아 이전 값을 알 수 없다(§7 근거).

---

## 3. Phase 0 — 무효화·재진입 안전성 (전제, 독립 landable 버그 수정)

### 3.1 유실 A: pass 중 재무효화 스왈로우 (sticky)
mid-pass 무효화가 조상 early-return(:1441-1444, "이미 DIRTY ⟹ 등록 완료" 가정)에 스왈로우 + 종료부
무조건 저장(:2831-2832) → stale·고착.
**수정**: miss 확정 직후 dirty 소비(Measure `=NaN` / Arrange `=false`), 종료부는 재오염 없을 때만 publish.

### 3.2 유실 B: 미소비 dirty의 early-return
커스텀 부모가 자식 Arrange 미호출 시 자식 dirty 미소비(clear는 자기 Arrange :2879 유일) → 재무효화가
early-return(:1498-1501)로 영구 유실.
**수정**: dirty 조기반환 제거 — 무조건 root까지 전파·등록(pending set coalesce).

### 3.3 release 재진입 가드 = generation 카운터를 잉여로 만드는 전제 (P0-3)
**병리**: 조건부 publish 모델의 정확성은 "pass 진입~종료 구간에 자기 dirty를 지우는 주체가 자기 pass
entry뿐"이라는 불변식에 의존한다. `mArrangeDirty` clear는 자기 publish(:2879), Measure 캐시키 clear는
자기 publish(:2831-2832)뿐이고 cross-view writer가 없으므로(§2.4), 이 불변식은 **자기 재진입만 깨뜨린다**.
그런데 오늘 재진입 가드는 Arrange의 debug assert(:2850)뿐이고 **Measure는 가드가 전무**하며, 공개
`ProcessLayouts()`가 중첩을 지원하므로(§2.1) 앱 콜백(Arrange callback :2859)이 `ProcessLayouts()`를
재진입 → `ProcessLayoutRoot`(:756)가 진행 중 View를 재-Measure/Arrange(:847/:864) → **중첩 pass가 A.dirty를
consume·publish하고 빠지면 바깥 pass가 clean을 보고 stale publish**(진짜 ABA).
**수정**: Measure·Arrange **양 축에 release 재진입 가드**를 둔다. 의미론: 같은 View의 pass가 이미 진행
중이면 **dirty를 consume하지 않고** 마지막 valid 결과(없으면 검증된 입력)로 **조기 반환**하며, **바깥
pass를 poison**하고 후속 layout을 등록한다.
**결과(§12.Gen 근거)**: 이 가드가 있으면 dirty/poison은 pass 내 **단조**(false(entry)→true(무효화)→유지)가
되어, "invalidation generation 스냅샷 ≠ 현재"가 "dirty‖poisoned"와 항상 동값이 된다 → **monotonic
generation 카운터는 순수 잉여**다. (이 가드를 성능 이유로 거절하면 generation 불채택 판정도 철회해야 한다.)

### 3.4 revision 카운터 불필요 논증
수정 후 모든 무효화가 root 등록에 도달, mid-pass 재오염은 조건부 publish가 감지, §5.3 무효화는 후속 pass가
즉시 소비, 신호 slot 무효화는 emit이 settle 시점(pass 밖), 재진입은 P0-3 가드가 차단. ⟹ uint64 revision·
active-root·invalidation generation 기계 불필요.

---

## 4. Change 1 — effective-scale sync-required 비트 + 무효화 분리

### 4.1 sync-required 비트 + 재진입 가드
**수정**: 동기화 블록(:2786-2792)을 `mEffScaleSyncRequired`로 조건부화. **set 지점**: ctor(true) / 스케일
-컨텍스트 원인(§4.3) / **`VIEW_EFFECTIVE_SCALE_PROPERTY_INDEX` 외부 write case(:4198)** — sync-bit로 매
Measure의 actor read를 없애므로 이 배선 없으면 외부 write 미반영 / `OnAnimateAnimatableProperty` 훅(:2609)
해당 인덱스. 내부 push 재발화는 `mEffScaleSyncInProgress` 가드로 감싸고 write 후 clear. actor property
동기화 write 자체도 §7의 target/index 일회성 token으로 수행한다.
**근거**: 히트 지배 비용(actor read :2786)을 비트로 대체. 애니메이션은 event-side를 setter 우회 bake
(object-impl.cpp:877)하므로 훅에서 bit set 필수.

### 4.2 안전 불변식
`mEffectiveScale` writer 2곳: 양수 재계산(:1424), ResetEffectiveScaleRecursive(:2998). 캐시 valid 저장은
miss 내부 유일 → 캐시 valid ⟹ 스케일 불변 ⟹ 히트 시 동기화 no-op.

### 4.3 `InvalidateMeasure`에서 effective-scale 리셋 분리 (이득은 INHERIT 한정)
**병리**: `InvalidateMeasure`가 오늘 `mEffectiveScale=-1`을 리셋(:1446)해, INHERIT 정책 View는 다음
`GetEffectiveScale`에서 부모 체인 재탐색을 유발. (DISABLED/ENABLED는 상수 반환 → 이득은 INHERIT 한정.)
**수정**: :1446 리셋을 generic `InvalidateMeasure`에서 제거하고, 스케일-컨텍스트 원인(SetUiScalePolicy /
UiScaleManagerImpl::SetScale / reparent OnChildAdd :2355 / child-remove §5.4)에서만 `ResetEffective
ScaleRecursive`(:2998) + `mEffScaleSyncRequired=true`. **§5.4 child-remove 리셋 추가가 필수 동반 조건**이다.

---

## 5. Change 2 — Arrange 캐시 (보편, 정비-선행)

### 5.1 상태 모델
```cpp
LayoutRect                  mLastArrangeInput;
Dali::LayoutDirection::Type mCachedEffectiveLayoutDirection;
bool mArrangeCacheValid : 1;              // 초기 false
bool mArrangeCachePoisoned : 1;           // §5.5 — physical write·자손 measure (freshness, cache-only)
bool mMeasureInProgress : 1;              // §3.3 release 재진입 가드
bool mArrangeInProgress  : 1;             // §3.3 release 재진입 가드 + §6.4 phase
bool mEffScaleSyncRequired : 1;           // §4
bool mEffScaleSyncInProgress : 1;         // §4.1
bool mChildHorizontalResolveRequired : 1; // §6 — 초기 true
bool mDirectionResolveInProgress : 1;     // §6.4
bool mLogicalContextValid : 1;            // §5.5/§6.1 — 초기 false; clean in-context full Arrange 시 true; reparent/scale-context 시 false
bool mLogicalContextPoisonedDuringPass : 1; // §5.5 — pass 중 reparent/scale-context 리셋 시 true(재유효화 차단)
bool mAutomaticDirectionMirroringEnabled : 1; // §6.3
bool mDirectionSignalConnected : 1;       // §6.2
// (전환기 전용) uint32_t mUncacheableArrangeProducerCountInSubtree  // §5.6
// invalidation generation 필드는 두지 않는다 (§3.3, §12.Gen)
```
그리고 per-View 필드가 아닌 **thread-local dependency-owner scope 스택**(§5.3):
```cpp
struct DependencyOwner { ViewImpl* owner; OwnerKind kind; bool poisonedDuringScope; };
thread_local std::vector<DependencyOwner> gLayoutOwnerStack;  // RAII push/pop
```
`mArrangedBounds`(유일 writer :2878)를 **논리(unmirrored) 결과**로 재사용. `mInitialLayoutDone`은 lifecycle
용도로만(§12.G). **freshness(②)와 context validity(④)를 별개 비트로 유지**(§12.F).

### 5.2 최종 hit 조건과 동작 (보편 — 자격 항 없음)
```cpp
if(mArrangeCacheValid && !mArrangeDirty && !mArrangeCachePoisoned &&
   mLogicalContextValid && !mChildHorizontalResolveRequired && exact(mLastArrangeInput, bounds))
{
  return mArrangedBounds;   // zero-touch (논리 rect 반환)
}
```
exact 비교(NaN miss), 키는 input. `mArrangeCacheValid`·`mLogicalContextValid` 초기 false가 첫 pass 보호.
hit에서 self bounds 복원·actor 확인 없음(§12.K).

### 5.3 dependency-owner scope + Measure miss 무효화 (generation 없이, scope로)
**두 병리**:
1. **in-pass constraint 경유 자손 재측정**: 부모 miss → 자식 miss → 손자 크기 변경, 집계 동일 가능 →
   부모가 자식에 동일 input → 자식 hit → 손자 재배치 누락.
2. **외부 직접 `child.Measure(C2)`**: 자식 measuredSize 슬롯이 C2로 덮임. 부모 Arrange는 non-MATCH_PARENT
   자식을 저장 슬롯(:920)으로 배치하므로, 조상 Arrange만 무효화하면 C2로 오배치. 부모가 자식을 자기
   constraint로 재측정하려면 부모 **Measure**도 다시 돌아야 한다.
**수정(무효화)**: full Measure miss 시 자기부터 조상으로 **Measure+Arrange 캐시를 둘 다** cache-only 무효화
(dirty·scheduling 없음), in-progress-Arrange 조상은 `mArrangeCachePoisoned=true`. **어디서 멈추는가가 핵심**:

**⚠️ "첫 measure-in-progress 조상에서 중단"은 불충분하다.** 5 stock 매니저와 recycler가 **Arrange 안에서**
child.Measure를 호출하므로(§2.3), 그 시점 owner는 arrange-in-progress이지 measure-in-progress가 아니다.
"measure-in-progress 조상 중단" 규칙은 이 경우 발화하지 않아 **모든 MATCH_PARENT 자식·recycler item이 매
프레임 "외부 직접 Measure"로 오분류 → root까지 캐시 무효화** → 도입하려는 Arrange 캐시가 상시 무력화된다.

**owner scope로 대체**: 정상 재귀·arrange-owned 재측정·recycler item 측정은 producer가 자식을 측정하기
직전 `gLayoutOwnerStack`에 `DependencyOwner{owner, kind}`를 push한다(RAII). full Measure miss는 stack top의
owner를 본다:
- **top owner가 이 자식의 실제 layout owner**(부모 체인의 measure/arrange-in-progress owner, 또는 recycler
  item의 recycler/layouter — §5.4)이면, 무효화 walk를 **그 owner에서 중단**한다(owner의 pass가 새 자식
  결과를 소비하므로 owner 위로 전파 불필요). 그 owner의 pass는 clean 완료 시 재publish한다.
- **owner가 없거나(top empty) 무관한 View를 콜백이 측정**(top owner의 dependency subtree가 아님)하면 walk를
  **root까지**(외부 직접 Measure).
**scope 실패 판정은 scope-local이어야 한다(잉여 오탐 방지)**: 자식 pass가 pass 중 재무효화·미publish·재진입·
실패로 끝나면 `poisonedDuringScope=true`로 그 scope의 owner pass만 poison한다. **owner의 raw 전역 dirty를
읽어 판정하지 않는다** — 형제 자식이 owner를 정당하게 dirty시킨 경우 현재 자식 scope를 오탐하기 때문이다.
(이 scope-local 스냅샷이 generation의 유일한 유효 용도이며, per-scope bool로 충분해 monotonic uint64
카운터는 불필요 — §3.3·§12.Gen.)
**핵심**: 이 사건은 조상 자기 논리결과(context)를 무효화하지 않는다(§5.5) — freshness만.

### 5.4 recycler item owner scope
recycler item은 부모 체인으로 owner를 유도할 수 없다 — item은 `mScroller`의 자식이고 recycler는 mScroller를
Measure/Arrange하지 않으며(§2.3), item 배치는 **스크롤 경로에서 layout pass 밖**에서도 일어난다. ⟹
`RecyclerLayoutOwnerScope(recycler)`를 item layout cycle(LayoutChunk/Fill/ScrollBy) 전체에 걸쳐 열되,
**parenthood가 아니라 recycler/layouter 쌍에 keying**한다. 이 scope 안의 item Measure·Arrange miss는 walk를
recycler에서 중단(recycler 크기는 viewport이지 item 합이 아니므로 item Measure가 recycler Measure를
무효화하지 않는다). item 자체 캐시(layouter mExtentCache + item mMeasuredSize)는 정상 갱신하고, recycle/
rebind 시 해당 item을 무효화·재layout하는 계약을 명시한다. `mScroller.Add/Remove`가 OnChildAdd/Removed →
InvalidateMeasure(:2359/:2454)로 recycler를 자기 Arrange 도중 재-dirty시키는 것은 이 scope와 조건부 publish가
정확히 흡수한다(오늘은 :2879 무조건 clear가 은폐).

### 5.5 재귀 스케일 리셋·child-remove 리셋 / freshness·context 분리·조건부 publish
- 스케일 리셋(:2996-3016)이 노드별로 `mArrangeCacheValid=false`+`mEffScaleSyncRequired=true`+
  `mLogicalContextValid=false` 설정. **child-remove 갭**: `OnChildRemoved`(:2403-2463)는 오늘
  `ResetEffectiveScaleRecursive` 미호출 → §4.3 분리 시 stale → 제거 서브트리 재귀 리셋 추가.
- **freshness vs context**: physical write·자손 measure는 조상 자기 논리 rect(actor lockstep :2876/:2878)
  미접촉 → context 유효 → cache만 차단(`mArrangeCachePoisoned`). 자기 declarative 재무효화는 dirty(freshness).
  **pass 중 reparent/scale-context 리셋은 context 무효** → `mLogicalContextPoisonedDuringPass=true`.
- **종료부 조건부 publish**:
```cpp
if(!mLogicalContextPoisonedDuringPass) mLogicalContextValid = true;   // 무조건 true 금지
mArrangeCacheValid = !mArrangeDirty && !mArrangeCachePoisoned &&
                     !mChildHorizontalResolveRequired && mLogicalContextValid;
```
**근거**: context와 freshness는 소비자가 다른 별개 축(§12.F). dirty를 context에 접으면 소비처(dispatcher
:352/:362)가 정상 위치 자식을 fallback 처리→오배치. 그러나 pass 중 reparent/scale 리셋은 context를 실제
무효화하는데(특히 `ResetEffectiveScaleRecursive`는 descendant `mArrangeDirty`를 안 세움 → dirty로 안 잡힘),
무조건 true는 이를 되살린다.

### 5.6 자격 기계는 전환기 도구 (최종 구조 아님)
조상 hit는 자손 Arrange를 완전 생략하므로, 정비 전 dirty-없는 producer(ChartView flag writer가
RelayoutRequest만)가 남으면 조상 hit 시 잘못된 계산. §10 정비 완료 시 불필요. 전환기: exact subtree
no-cache counter + scoped guard + instrumentation, 이행 후 제거(monotonic 비트 §12.C·local-only §12.D 불채택).

### 5.7 트랜지션 — 비활성화 불필요
캡처가 읽는 `GetArrangedBounds`(논리 rect)는 hit clean 뷰에서도 불변, ENTER는 valid=false 보호, LayoutFinished
스냅샷은 RTL 미러 후. frame별 write는 §7 token.

---

## 6. direction — required-bit 결정적 resolver

resolver(required일 때만, full Arrange 종료부):
```cpp
const bool rtl = (mCachedEffectiveLayoutDirection == RIGHT_TO_LEFT);
for(non-standalone View 자식) {
  if(!childImpl.mLogicalContextValid) continue;
  if(!childImpl.mAutomaticDirectionMirroringEnabled) continue;
  const LayoutRect cb = childImpl.GetArrangedBounds();      // 논리(LTR) 기록
  const float target = rtl ? parentWidth - cb.x - cb.width : cb.x;
  if(child.GetProperty<float>(POSITION_X) != target)
    InternalGeometryWrite(child, POSITION_X, target);       // §7 token
}
```
**required 선소비**(중간 재요청 보존). **근거**: (1) 논리 절대식은 멱등 — 현행 involution(:2962-2964)은 캐시
hit가 OnArrange LTR 재설정을 건너뛰면 이전 반전값 재반전. (2) LTR도 처리해야 explicit-direction 자식
(actor-parent-impl.cpp:546 시그널 중단)이 RTL→LTR·hit에서 복구. (3) 안정 상태 read·순회·write 0.

### 6.1 스킵 판별은 `mLogicalContextValid`
`mArrangedBounds`·`mInitialLayoutDone`은 reparent 시 미갱신 → `mInitialLayoutDone` 기준 스킵은 reparter된
자식 stale 미러. `mLogicalContextValid`(reparent·scale-context에서 false)로 판별. dirty이지만 재배치 대기
자식도 현재 프레임 rect 유효하면 미러 대상(freshness ⊥ context).

### 6.2 direction 변경 배선
base View 미연결(ScrollView/Label/InputEditor만 자체 연결). **수정**: 핸들러(cached direction + required +
`InvalidateArrange`)를 **non-virtual `ViewImpl::Initialize()`**(view-impl.h:176, `View::New`가 전 경로 호출
view.cpp:48, 본문이 :731에서 derived `OnInitialize` 호출) 안 derived OnInitialize 이전에 연결. 멱등 가드
`mDirectionSignalConnected`. custom producer가 direction 읽어 다른 배치 가능하므로 direction 변경은 full
Arrange 허용.

### 6.3 component-owned child 명시 opt-out
scroll content·recycler scroller·자체 RTL 소유 child는 `mAutomaticDirectionMirroringEnabled=false`. scroller는
context-valid이나 X가 scroll offset 섞인 physical이라 mirror 금지 — 명시 opt-out이 정확.

### 6.4 child full Arrange ↔ parent resolver 타이밍 (2-boolean)
`mArrangeInProgress` + `mDirectionResolveInProgress`로 idle/producer-pre-resolver/resolver-중 구분. resolver는
child.Arrange 미호출 + LayoutFinished 지연 발신(layout-controller.cpp:558-573→pass 후 emit)이라 4-state enum
불필요.

---

## 7. 직접 geometry write — 중앙 `OnPropertySet` + target/index token + 보수적 무효화

**병리**: 공개 `Dali::Actor` setter와 `SetProperty(index)` 모두 `OnPropertySet`을 발화(§2.5). ⟹ 중앙
`OnPropertySet`(:2548)이 유일 완전 포착점. geometry 케이스(POSITION/POSITION_X/Y/SIZE/SIZE_WIDTH/HEIGHT,
composite+per-axis 모두) 추가.

### 7.1 외부 write는 보수적 무효화 (post-set no-op 판정 불가)
**hook은 post-set이며 이전 값을 알 수 없다(§2.5)** — `Object::SetProperty`는 값 변경을 비교하지 않고 동일값
write에도 hook을 발화한다. ⟹ **"값이 실제 바뀐 경우에만 무효화"는 hook에서 달성 불가**. 성공한 외부
geometry setter는 **보수적으로 무효화**한다:
```text
self Arrange cache invalid + traversal ancestor Arrange cache-only invalid
active Arrange ancestor는 mArrangeCachePoisoned=true (freshness; context 유지)
X 또는 width 변경 시 direct parent direction resolve required
dirty·Layout scheduling 없음
```
**정밀도 회복 2건**: (1) **SIZE 무효화는 이미 변경-필터된 `OnSizeSet` 경로로 라우팅**한다 —
`actor-sizer.cpp:120` 가드가 동일 size write를 걸러 `OnSizeSet`(:2593)에만 도달하므로, SIZE는 무비용으로
"실제 변경 시에만" 무효화된다. (2) POSITION 동일값 write가 측정상 hot으로 확인되면, 기존 `mSize` shadow
(view-data-impl.h:898)처럼 **shadow x/y를 ViewDataImpl에 추가**해 hook에서 비교한다 — 단 이는 hook의 내재
성질이 아니라 **명시적 added state**로 문서화한다. 직접 설정 geometry는 다음 declarative Layout 전까지 유지.

### 7.2 엔진 자기 write 구별 — target/index 일회성 suppression token (depth 아님)
엔진 write(self bounds, resolver 자식 X, transition, Scroll/Recycler frame·scroller, recycler item Arrange,
effective-scale sync, component layer)는 공개 setter라 `OnPropertySet`을 재발화. **단순 depth는 부정확**:
전역/스레드 depth는 동기 `PropertySetSignal` 콜백의 다른-View 외부 write까지 억제(over-suppression); acting
-View depth는 cross-View write(다른 View에 발화) 미감지; 본문 전체 스코프는 custom OnArrange 외부성 write
은폐. **해법(target/index 일회성 token)**:
```text
InternalGeometryWrite(targetView, index, value):
    (targetView, index) one-shot token push (LIFO stack)
    targetView 공개 setter로 write   // OnPropertySet(index) 동기 발화
    hook 미소비 시 token pop
OnPropertySet(index, value):
    if stack top이 (this, index) 정확 일치: pop; return   // 내부 write
    else: 외부 write 처리(§7.1)
```
per-call 무장·hook 진입 즉시 소비 → 이후 동기 signal 콜백 write는 외부 감지. write 대상에 무장 → cross-View
정확. nested는 LIFO. setter 실패·미발화·target 소멸 시 RAII cleanup. token stack 불일치는 debug assert+계측.
**fail-safe**: token 누락 시 불필요 pass 1회(perf); depth 과억제는 stale(정확성) — token은 안전 방향 실패.

---

## 8. fitting·natural-size — Arrange/Layout-신호에서 분리

**현황**: fitting은 (a) 공개 `LayoutFinishedSignal` 구독(OnLayoutFinished→ApplyFittingMode :4838), (b) visual
processor(Process→ApplyFittingMode :5343, `SizeOrUiScaleChanged` :5192 경유). Measure는 visual natural size를
읽는다(MeasureDefault :862/:880 → `GetNaturalSize` :2662-2672).
**두 갭(재검증 확정)**:
1. **plain View fitting 재적용 부재**: plain View는 기본 ViewImpl이 `DISABLE_SIZE_NEGOTIATION`
   (view-impl.cpp:108-111) → core relayout이 OnRelayout에 root/child 무관 미도달. 초기 fitting은
   `OnSizeSet→Process`로 적용되나 크기 불변 후속 resource-ready에서는 재적용 경로 없음(resource-ready는 core
   `RelayoutRequest` view-visual-data.cpp:381 + 공개 ResourceReady만).
2. **natural-size 변화 시 Measure 무효화 부재**: image/SVG는 load 후 intrinsic size 변화 가능한데
   resource-ready·`SetBackground`(:4855)는 measure cache를 dirty로 안 만듦.
**수정**: fitting을 입력 mutation에 직접 연결(source-driven; visual 등록·교체·제거, resource-ready, fitting
-mode, `OnSizeSet`, padding, effective scale·direction). one-shot processor는 진입 직후 registered 선소비.
영구 pending 불필요. **natural-size 기여 visual이 resource-ready·SetBackground되면 `InvalidateMeasure`도** —
특정 image View만이 아니라 공통 visual owner 경로에서 판정해 plain-View background 포함(ImageView/Lottie/
AnimatedImage는 이미 컴포넌트 레벨 처리; 잔여 갭=generic background). scene 연결로 DALi-UI 무효화 생략 금지.

---

## 9. View 단위 신호 — "full-Arrange 완료 신호"
full Arrange를 실제 수행한 View만, 전체 정착 후 1회 발신(full이면 bounds 동일해도 발신 / hit면 미발신 /
latest-wins / window 신호와 구분). 스냅샷은 direction resolver 이후·transition이 actor 덮기 전. per-view
구독 체크로는 구독 자손이 clean 조상 hit에 가려짐. **fitting은 §8로 분리했으므로 비종속.**

---

## 10. 선행 정비

### 10.1 built-in LayoutManager
setter 무효화 부재 + raw 접근. manager Impl(PIMPL)에 owner back-pointer, attach에서 연결, 전 setter가 값
변경 시 `InvalidateMeasure/InvalidateArrange`(동일 값 생략). protected non-virtual helper. manager 교체
미허용 동안 replacement 인프라 제외.

### 10.2 상태성 producer·recycler·fitting writer
- **Chart**: flag writer가 RelayoutRequest만 → 조상 hit stale. 배치 상태면 즉시 Rebuild, 재배치는
  `InvalidateArrange`, measured 영향은 `InvalidateMeasure`, data-only는 processor. 라벨 직접 배치 유지 시
  `mAutomaticDirectionMirroringEnabled=false`.
- **RecyclerView(item Measure+Arrange)**: layouter가 item을 **직접 Measure(linear-items-layouter-impl.
  cpp:288/295, sentinel kUnconstrained :282) + Arrange(:291/298)**하며 표준 재귀 밖·스크롤 경로 포함. §5.4의
  `RecyclerLayoutOwnerScope`(recycler/layouter keying, parenthood 아님)로 item Measure·Arrange를 한 carve-out
  으로 감싼다. item 이중 캐시(layouter mExtentCache + item mMeasuredSize) 정합 유지, recycle/rebind 시 item
  무효화. scroller/content는 component-owned direction + frame write는 §7 token.
- **Canvas/Video/Web/Lottie**: 부수효과를 실제 state transition에 연결. Video/Web display area는 world
  position/scale/window 의존 → 독립 갱신 경로.

### 10.3 Label 릴리즈 로그
`return bounds;`뿐인데 `DALI_LOG_RELEASE_INFO` 매 호출 — 제거/DEBUG 강등.

### 10.4 이중 RTL 제거 (별도 커밋)
CheckBox 등 자체 mapX + 공통 미러 재반전 → 비대칭 padding 오배치. "파생은 LTR 논리만, 방향은 공통 resolver".

---

## 11. 재진입·외부 producer 계약

### 11.1 same-View 재진입 (release 가드 — P0-3 상세)
debug assert만으로 막지 않는다. Measure·Arrange 재진입 시: producer 재호출 금지, 현재 pass poison, context
-valid logical result 있으면 반환·없으면 검증된 input fallback, dirty **미소비**(바깥 pass가 관측), 후속 layout
등록. 이 가드가 조건부 publish의 ABA를 닫고 generation을 잉여로 만든다(§3.3).

### 11.2 외부 producer 공개 계약 (영구 no-cache 대신)
external manager·callback·파생 View도 clean 상태에서 생략. migration guide 명시: hidden mutable state가
결과에 영향 시 owner/View invalidation / 동일 input clean 결과 결정적 / 반복 호출 필수 부수효과 금지 / child는
논리 bounds로 Arrange, 직접 배치 시 방향·cache 무효화 책임 / callback 등록·교체 경로 무효화. 계약 위반은
자동 감지 불가(§12.B)라 표준 계약을 공개 의미로 확정(Android requestLayout 미호출=onLayout 미실행).

---

## 12. 폐기한 대안과 폐기 근거

**Gen. invalidation generation 카운터 도입** — 폐기(§3.3). dirty/poison을 지우는 유일한 mid-pass 주체는
자기 재진입뿐이고(clear 지점 :2831-2832/:2879, cross-view writer 0), P0-3 release 재진입 가드가 이를 닫으면
dirty/poison은 pass 내 단조 → "generation 스냅샷 ≠ 현재"가 "dirty‖poisoned"와 항상 동값 → uint64 카운터는
순수 잉여. **단 dependency-scope의 실패 판정은 scope-local이어야** 하며(형제 자식이 owner를 dirty시킨 경우
raw 비트 읽기는 오탐), 그 scope-local 스냅샷은 per-scope bool로 충분해 monotonic 카운터가 필요 없다(§5.3).

**Owner. "Measure miss 조상 무효화를 measure-in-progress 조상에서 중단"** — 폐기. 5 stock 매니저(absolute
:358/scroll-view :139/flex :354/grid :477/stack)와 recycler가 **Arrange 안에서** child.Measure를 호출하므로
그 시점 owner는 arrange-in-progress이지 measure-in-progress가 아니다 → 규칙 미발화 → 모든 MATCH_PARENT
자식·recycler item이 매 프레임 외부 직접 Measure로 오분류 → root 무효화 → 캐시 무력화. **dependency-owner
scope(owner 포인터 스택)로 대체**(§5.3). owner는 트리 구조로 유도 불가(recycler item은 mScroller 자식).

**A. 정비 전 전 View 즉시 캐시** — 폐기. dirty-없는 부수효과 producer(Chart)로 잘못된 계산.
**B. 외부/파생 producer 영구 no-cache** — 폐기. external descendant 하나로 조상 가지치기 차단. 공개 계약(§11.2).
**C. no-cache subtree monotonic 비트** — 폐기. detach·reparent 후 못 내림. delta counter만 복원 가능.
**D. local no-cache 비트만** — 폐기. 조상 먼저 hit하면 도달 못 함.
**E. subtree counter 영구 유지** — 폐기(전환기 도구로만).
**F. context validity를 freshness와 같은 비트로** — 폐기. dirty를 context에 접으면 dispatcher fallback→오배치.
단 pass 중 reparent/scale-context는 실제 context 무효화라 **무조건 true도 금지**(§5.5 조건부 publish).
**G. `mInitialLayoutDone`을 context 근거로** — 폐기. reparent 미갱신 → stale rect 유효 오인.
**H. current-X RTL 토글 + 자식 hit** — 폐기. involution → 재반전 오염.
**I. `ApplyLayoutDirection` 매 Arrange 무조건 실행** — 폐기. 불변이면 결과 동일. required-bit로.
**J. direction을 캐시 키에서 actor read 비교** — 폐기. zero-touch 아님.
**K. hit에서 self bounds 복원/actor 확인** — 폐기. 전체 hit에 actor read 부과.
**L. measured size 같으면 Arrange 캐시 유지** — 폐기. 자식별 measure·부수효과 상이. §5.3대로 무효화.
**M. full Measure마다 context 무효 / Measure 실행됐다고 logical publication 금지** — 폐기. Measure는 freshness만.
**N. 내부 write에 전역/스레드 depth** — 폐기. 동기 signal 콜백 외부 write 억제.
**N-2. acting-View depth** — 폐기. cross-View write 미감지.
**N-3. target depth를 setter 반환까지 유지** — 폐기. 같은 target signal 콜백 write 은폐. hook 진입 즉시 소비.
**N-4. Arrange 본문 전체 스코프** — 폐기. custom OnArrange 외부성 write 은폐.
**NoOp. "외부 geometry write는 값 실제 변경 시에만 무효화"** — 폐기. hook은 post-set이라 이전 값 없음;
`Object::SetProperty`는 값 비교 없이(거부된 write에도 :477-478) hook 발화. 보수적 무효화 + SIZE는 OnSizeSet
우회 + POSITION은 선택적 shadow(명시 added state)(§7.1).
**O. typed setter는 hook 우회** — 폐기. 공개 setter는 SetProperty 경유(actor.cpp:142-150).
**P. dirty early-return / 종료부 무조건 clear** — 폐기(§3).
**Q. hit에서도 per-View LayoutFinished** — 폐기. full-Arrange 완료 의미로 재정의(§9).
**R. fitting을 public LayoutFinished에 연결** — 폐기. hit 미발생 → 누락. 입력 mutation 직접 연결(§8).
**S. transition subtree 영구 no-cache** — 폐기. 논리/물리 분리.
**T. fitting 영구 pending bit** — 폐기(조건부). 직접 연결 가능하면 불필요.
**U. 이중 RTL 유지** — 폐기. 비대칭 padding 오배치.
**V. child.Arrange만으로 부모-기준 RTL 즉시 확정** — 폐기. 다음 parent resolver가 확정.
**W. input/output 한 rect 캐시** — 폐기. callback input≠output 가능.
**X. Arrange bounds epsilon 비교** — 폐기. exact.
**Y. raw Measure fast key 초기 도입** — 폐기(연기). sync-bit로 actor read 제거 후 잔여 pre-hit는 스칼라
(<0.1% 프레임) + raw `FloatEqual`(visual 단위)는 s<1시 정규화(natural 단위)보다 느슨→오히트 + 센티넬 이중화
위험. 후속 도입 시 비트 정확 비교·센티넬 비공유·dirty 이관.
**Z. Measure 비교 앞 Trait 조회/cache** — 폐기. 히트 전 Trait 순회 없음.
**Z-1. Measure→조상 무효화를 전역 depth로** — 폐기. 무관 트리 오인. owner scope 판정(§5.3).
**AB. generic InvalidateMeasure 항상 scale 리셋** — 폐기(성능·INHERIT 한정). child-remove 리셋(§5.4) 동반.
**AC. child-remove parent 무효화만으로 충분** — 폐기. 제거 서브트리 재귀 리셋 필요.
**AD. 4-state arrange phase enum** — 폐기. 2 boolean으로 충분(§6.4).
**AE. root plain View는 resource-ready fitting 자동 안전 / fitting만 요청** — 폐기. DISABLE_SIZE_NEGOTIATION
으로 root/child 무관 미도달 + natural-size 기여 visual은 Measure 무효화도(§8).
**AF. same-View 재진입을 debug assert에만 의존** — 폐기. release 가드 필수 — generation 불채택의 전제(§3.3·§11.1).
**AG. recycler는 item Arrange만 (Measure 없음)** — 폐기. layouter가 item을 직접 **Measure**도 한다
(linear-items-layouter-impl.cpp:288/295, sentinel kUnconstrained). carve-out은 Measure+Arrange 쌍을 덮고
recycler/layouter에 keying, 스크롤 경로에서 유효해야 한다(§5.4·§10.2).
**AH. 정적 cycle·개선율 보장 / manager replacement 필수 / Scroll base 즉시 일괄 전환** — 폐기. 계측 판단 /
미존재 lifecycle / component별 검증 전환.

---

## 13. 단계별 적용 순서

| Phase | 내용 | 성격 |
|---|---|---|
| **0** | §3 무효화 유실 2종 + **P0-3 release 재진입 가드(양 축)** + UTC | 선행 필수, 독립 landable(버그 수정) |
| **1** | §4 sync-required(+애니메이션 훅, VIEW_EFFECTIVE_SCALE case, sync token) + §4.3 scale 리셋 분리 + §5.5 child-remove 리셋 | 독립 landable |
| **2** | §6 resolver + §6.2 배선 + §7 중앙 OnPropertySet + target token + §7.1 보수적 무효화·SIZE OnSizeSet 우회 + **§5.3 owner scope(generation 없이) + Measure→조상 Measure+Arrange 무효화** + §5.4 recycler scope(item Measure+Arrange) + §5.5 freshness/context 분리·조건부 publish + §10.1 manager + §10.2 정비 + §8 fitting·natural-size + §10.3 로그 + §10.4 이중 RTL | 캐시 전 정비 = 활성화 게이트 |
| **3** | §5 Arrange 캐시 활성화(보편 hit) + §9 신호 문서 + §11.2 공개 계약 | 정비 완료 후. 원자 랜딩이면 자격 기계 없이; 단계 배포면 §5.6 전환기 후 제거 |
| **4** | 프로파일 후속: pending-token, raw fast key(연기), 전환기 도구 제거. **generation·POSITION shadow는 측정으로 필요 입증 시에만** | 측정 근거 후 |

---

## 14. 자동화 테스트 (근거 조항별 회귀 고정)

1. **Phase 0**: 콜백 중 형제 무효화 → 미publish+재등록(sticky) / dirty 자식 미호출 후 재무효화 → 새 pass /
   **앱 콜백에서 ProcessLayouts 재진입 시 바깥 pass가 stale publish 안 함(P0-3 양 축 가드)** / release 재진입
   시 dirty 미소비·바깥 poison
2. **Measure**: 동일 constraint 히트 / 유한 음수 동치 / NaN miss / 애니메이션 Play·Stop·Pause 후 교정 /
   INHERIT만 scale 재탐색 / VIEW_EFFECTIVE_SCALE 외부 write 반영
3. **§5.3 owner scope**: 정상 재귀 부모 과무효화 없음 / **매니저 arrange-owned MATCH_PARENT 재측정이 조상을
   root까지 무효화하지 않음** / **recycler item Measure·Arrange(스크롤 포함)가 조상 Measure/Arrange 캐시를
   무효화하지 않음** / 외부 직접 child.Measure(C2) 후 부모 Arrange가 C2로 오배치 안 함(조상 Measure+Arrange
   무효화) / 무관 트리 콜백 Measure는 external / scope-local 실패 판정(형제 dirty가 현재 자식 scope 오탐 안 함)
4. **Arrange/context**: 동일 bounds 히트(OnArrange 수 불변) / hit에서 actor·Trait·producer·순회 0 / **pass 중
   reparent/scale-context 리셋 시 종료부 context 재유효화 안 함** / physical write·자기 dirty에도 context 유지
5. **§6**: RTL 반복 좌표 불변 / 캐시 hit가 LTR 재설정 건너뛰어도 이중 반전 없음 / LTR→RTL→LTR / never-arranged·
   reparent-후 자식 스킵 / mirroring=false child 제외
6. **§7**: 전 경로 외부 write 재적용 / 엔진 자기 write 자기-무효화 안 함(token) / 내부 write의 PropertySetSignal
   콜백이 다른 View 외부 write 시 무효화 보존 / matching token만 소비·nested LIFO / setter 실패·target 소멸
   cleanup / **동일값 SIZE는 OnSizeSet 필터로 무효화 안 함 / 동일값 POSITION은 (shadow 없으면) 보수적 무효화**
7. **§8**: root·child plain View 초기 + 크기 불변 후속 resource-ready fitting 재적용 / natural-size 기여 visual
   resource-ready·SetBackground 후 Measure miss / off-scene resource-ready 후 scene 추가
8. **§9/§10**: full이면 bounds 동일해도 발신·hit면 미발신 / Chart 플래그 소비 / **RecyclerView item recycle·
   rebind·scroll** / 트랜지션 ENTER/CHANGE/reparent
9. **구조적 성능**: leaf 1 무효화 시 clean 서브트리 OnArrange 수 불변; hit 차단·owner-scope 중단 원인별 카운터

---

## 15. 성능·메모리·잔여 리스크

- **Measure 히트**: actor read(지배항) → 비트 검사. + INHERIT scale 재탐색 제거. raw fast key 연기(§12.Y).
- **Arrange 히트**: Trait 2+dynamic_cast+actor 8-read+힙 2회+자식 재귀+방향 처리 제거 — O(N)→O(dirty path+
  fanout). 내부 노드 방문 없음(recycler item은 owner scope 계약).
- **direction**: 안정 상태 read·순회·write 0; 무변경 RTL pass 메시지 2N→0.
- **직접 write 구별**: token은 write당 (View,index) 1건 push/pop. over-suppression 없음. 외부 write 보수적
  무효화는 SIZE를 OnSizeSet으로 우회해 대부분 회복.
- **메모리**: View당 LayoutRect 1 + direction enum 1 + 비트 ~12(기존 bitfield 흡수) + manager Impl당 포인터 1
  + geometry token 저장소 + thread-local owner scope 스택 1개. **generation 카운터·subtree counter·revision
  없음**(전환기에만 counter). (POSITION shadow는 측정 후 선택.)
- **관찰 동작 변경(허용 근거)**: clean 뷰 producer 미호출(표준 계약) / full-Arrange 완료 신호 / 직접 설정
  geometry가 다음 declarative Layout 전까지 유지 후 덮임 / 동일값 POSITION 보수적 무효화(불필요 pass 1회, 정확성
  무해) — 전부 릴리즈 노트·migration guide 대상.
- **실행 검증 필요(정적 확정 불가)**: P0-3 재진입 가드가 중첩 ProcessLayouts를 정확히 흡수하는지 / owner scope가
  정상 재귀·arrange-owned·recycler(스크롤 포함)·external·무관-콜백을 정확히 가르는지, scope-local 실패 판정 /
  OnPropertySet token이 엔진 write·동기 signal·외부·nested 구별 / `mLogicalContextValid` drop·pass-중 poison
  전수 배선 / D4 조상 Measure 무효화 over-inval 비용 / recycler item 이중 캐시 정합 / plain-View fitting·
  natural-size 전수(root·child, 초기·후속, SetBackground) / 무조건 조상 전파 비용 / wall-clock. 각 항목은
  "문제 없음"을 가정하지 않고 시험 gate로 통과.
