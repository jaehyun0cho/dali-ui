# 코드 리뷰 (확정 최종본 · 전수 재검증) — `f63fdc13 "Initial patch for navigator and dialog"`

- **대상 커밋**: `f63fdc13` (Author: dongsug.song, 2026-06-25) — branch `claude` / `pr-430` / `codex` 공통 HEAD
- **범위**: navigator + dialog (34 files, 4,546 insertions)
- **검증 원칙**: 선행 리뷰의 *어떤 주장도 사실이 아닐 수 있다*고 가정하고 현재 소스로 전수 재검증했다. 부재("없다")류는 `grep`으로 0건 확인, 핵심 함수와 foundation 훅은 라인 위치를 직접 정독해 인용했다. 본 라운드에서 H5(모달 lifecycle 정확성 버그)의 코드 체인(`PushModal`→`UpdateVisibility`→`FinishTransition`)을 끝까지 추적해 확정했다. 과장·오류 주장은 §4에 정정 사유와 함께 명시한다.
- **검증 소스**: `dali-ui-components/{public-api,integration-api}/{navigator,dialog}/*`, `automated-tests/src/dali-ui-components/utc-Dali-*.cpp`, `samples/{navigator,dialog}/*`, foundation `dali-ui-foundation/public-api/{view.h,view-impl.h,layouts/absolute-layout-manager.cpp}`, dali-core `dali/public-api/signals/base-signal.cpp`, `dali/public-api/actors/actor.h`. 빌드/실행은 하지 않음(정적 코드 리뷰).
- 본 문서는 다른 파일 참조 없이 단독으로 완결됨.

---

## 총평

이 커밋은 "동작하는 prototype"이지 "navigation/dialog system"이 아니다. 핸들/impl 보일러플레이트와 ABI 골격은 리포 관례를 대체로 따른다. 핵심 문제는 코드 양이 아니라 **contract의 부재**다: modal isolation, focus/back integration, lifecycle 정확성, transition semantics, dialog result/default/cancel, accessibility, ownership policy가 public API 수준으로 정의되어 있지 않다. 어려운 부분(중앙 정렬, dismiss, 결과 처리, key 통합)은 sample/caller에게 떠넘겼다.

**판정: 현 상태로 merge 불가. Skeleton은 있고 system은 없다.**

선행 리뷰의 결함 다수가 사실로 확인되었고, 모달 lifecycle 정확성 버그(H5)를 코드 끝까지 추적해 확정했다. 일부 단정(scrim 중복 연결, 항상 double-emit, "cross-fade 깨짐", FinishedSignal dangling)은 과장·거짓이라 정정했다(§4).

---

## 1. 높음 — 컴포넌트의 존재 이유를 깨는 결함 (전부 소스 확인)

### H1. 모달이 실제 modal로 격리되지 않음 (focus·key·a11y)
모달을 push해도 가려진 navigation top page는 `VISIBLE=true`로 남고 포커스/키/제스처로 접근·활성화된다. 유일한 장벽은 scrim `InteractiveView`의 *포인터* hit-test뿐이다.
- 근거: `navigator-impl.cpp:239-280`(PushModal은 push/AddChildFill/RestackModals/scrim connect/UpdateVisibility/emit/transition만), `:422-432`(UpdateVisibility는 각 스택 top의 `VISIBLE`만 토글; nav top은 모달 유무와 무관하게 VISIBLE 유지).
- 재검증(grep, components 양 impl 디렉터리): `SetDescendantFocusBlocked` 0건, `RequestFocus` 0건, `OnKeyEvent` 0건. 프레임워크는 제공 — `view-impl.h:983`/`:1032`/`:1069`.
- 영향: DALi 주 타깃인 TV/리모컨 5-way에서 "보이지 않는 포커스"로 다이얼로그 뒤 버튼이 포커스를 받고 Enter로 실행된다. 포커스를 가두지 못하는 모달은 모달이 아니다.

### H2. 모달 진입 시 포커스 이동 없음, 해제 시 복원 없음
`PushModal`/`PopModal`이 `RequestFocus`로 다이얼로그(또는 기본 버튼)에 포커스를 옮기지 않고, 닫을 때 직전 포커스를 복원하지 않는다.
- 근거: `navigator-impl.cpp:239-317`. grep으로 `RequestFocus` 0건.
- 영향: 다이얼로그를 연 채 Enter→뒤의 숨은 컨트롤 실행, 닫은 뒤 포커스 유실.

### H3. 접근성 전무 — 프레임워크가 도구를 주는데 안 씀
`Dialog`/`AlertDialog`/`DialogContainer` 어디에도 accessibility role·name·modal 플래그가 설정되지 않는다.
- 재검증: foundation `view.h`는 a11y property 5종 제공 — `ACCESSIBILITY_NAME`(:1713)/`DESCRIPTION`(:1719)/`ROLE`(:1735)/`HIDDEN`(:1762)/`IS_MODAL`(:1806); `view-impl.h`는 `CreateAccessibleObject`(:162)/`OnAccessibilityActivated`(:133) 제공. components 양 impl 디렉터리에서 `ACCESSIBILITY`/`CreateAccessibleObject` grep 0건. `alert-dialog-impl.cpp:71-104`는 시각 `Label`만 만들고 a11y name/description 미설정.
- 영향: 스크린리더가 다이얼로그 개방을 인지/낭독하지 못함. (개념적으로 medium 성격이나 public ABI로 굳으면 retrofit이 어렵고 모달 격리(H1)와 직결되므로 High.)

### H4. Will/Did lifecycle 시그널이 상태 변경 *후*에 방출됨 (타이밍 계약 위반)
네 mutator 모두 스택을 변경하고 `UpdateVisibility()`로 가시성을 바꾼 *뒤*에 `Will*` 시그널을 emit한다.
- 근거(재정독): `Push` `:100`push_back→`:103`UpdateVisibility(prev 숨김)→`:107`WillDisappear; `Pop` `:131`pop_back→`:134`UpdateVisibility→`:139`WillDisappear; `PushModal` `:250`push_back→`:261`UpdateVisibility→`:268`WillDisappear; `PopModal` `:292`pop_back→`:295`UpdateVisibility→`:304`WillDisappear.
- 정밀화(공정성): `Pop`의 outgoing(top)은 `:131`에서 pop_back되어 `UpdateVisibility`가 더 이상 건드리지 않으므로 `WillDisappear(top)`(:139) 시점에 아직 VISIBLE=true다. 반면 `Push`의 outgoing(prev)은 WillDisappear 이전에 이미 VISIBLE=false다. 타이밍이 일관되지도 않다.
- 영향: 이름은 "will"(사전 hook)인데 실제로는 상태가 이미 바뀐 뒤의 after-state notification이다. → 가시성 변경 *전*에 emit하거나 `Changed`/`Did` 계열로 개명해야 함.

### H5. 모달 push/pop이 "사라지지 않는" 페이지에 disappear/appear 시그널을 방출함 (lifecycle 정확성 버그)
`PushModal`은 가려질 뷰를 `disappearing = prevModal ? prevModal : NavTop()`(`:265`)로 잡고 `PageWillDisappear`(`:268`) 및 transition 종료 시 `PageDidDisappear`를 emit한다. 그러나 **그 nav top page는 실제로 사라지지 않는다**:
- `UpdateVisibility`(`:261`)는 nav top을 `VISIBLE=true`로 유지(`:422-432`).
- `mTxRemoveOutgoing=false`(`:278`)이므로 `FinishTransition`의 제거 블록(`:478-485`)이 건너뛰어진다 → nav top은 제거되지 않는다.
- `RunTransition(animated, /*fadeIncoming=*/true)`(`:279`)은 incoming(모달)만 페이드한다 → nav top은 fade도 되지 않는다.
- 그럼에도 `FinishTransition`은 `outgoing = mTxOutgoing`(=nav top)에 대해 `mPageDidDisappearSignal.Emit(handle, outgoing, …)`(`:501-503`)을 실행 → **숨김도·fade도·제거도 되지 않은, 여전히 화면에 보이는 페이지에 `DidDisappear`가 방출된다.**
- `PopModal`도 대칭: `appearing = newModalTop ? newModalTop : NavTop()`(`:300`), `mTxIncoming=appearing`(`:311`) → 마지막 모달 pop 시 `DidAppear(navTop)`(`:507`)가 방출되지만 그 nav top은 모달이 떠 있는 동안 한 번도 숨겨진 적이 없다.
- 영향: `DidDisappear`에서 리소스 해제/구독 해지를 하는 사용자 코드는 **여전히 보이는 페이지**를 정리하게 된다. iOS는 overlay/formSheet presentation에서 presenting VC에 `viewDidDisappear`를 보내지 않음으로써 이를 구분하지만, 여기서는 모달 종류와 무관하게 항상 disappear/appear를 방출한다 → lifecycle 시그널이 의미적으로 거짓.

### H6. 전환 엔진에 재진입 가드가 없음
모든 mutator가 시작부에서 `SettlePendingTransition()`을 호출하고, 이것이 진행 중 애니메이션을 abort한 뒤 `FinishTransition()`으로 `Did*`/`TransitionFinished`를 **동기 방출**한다(샘플 자신이 `TransitionFinishedSignal`에 연결, navigator-example.cpp:79).
- 근거: `navigator-impl.cpp:95,127,215,245,288`(prologue), `:523-530`(Settle), `:470-511`(Finish가 `:503/:507/:509` emit).
- 정밀화(중요): `FinishTransition`은 `mTx*`/`mTransition`을 emit *전에* 리셋(`:487-497`)하므로 `Did*`/`TransitionFinished`에서의 재진입은 대체로 안전하다. **그러나 `Will*`은 mutation 도중(mTx* 세팅 전, `:107-118`)에 방출**되므로, 그 슬롯에서 `Push`를 재진입하면 외부 호출의 후속 라인(`:114-118`)이 내부 호출의 transition 상태를 덮어써 깨진다. iOS `UINavigationController`가 명시적으로 직렬화하는 함정. → transition queue / cancellation / 재진입 가드 중 하나 필요.

---

## 2. 중간 — 기능 / 계약 결함 (전부 소스 확인)

### M1. 전환이 한쪽만 페이드되고 outgoing은 한 프레임에 사라짐, 설정 불가
`Push`는 페이드 전 `UpdateVisibility()`로 이전 page를 즉시 `VISIBLE=false`로 만들고(`:103`) 들어오는 page만 페이드인(`:443-458`). `Pop`은 새 top을 즉시 visible로 만들고 나가는 top만 페이드아웃. 비대칭이며 type/duration/easing 설정 불가(`TRANSITION_DURATION=0.25f` 하드코딩, `:39`).
- 정밀화: 헤더는 "optional fade transitions"만 약속하므로 *cross-fade 계약 위반은 아니다*. 다만 효과가 거칠고 navigation component의 전환 표면으로는 빈약하다.

### M2. `Remove(유일 페이지)`가 조용히 무시됨 — 문서 계약 위반
헤더는 "Removes a page from either stack. Removing the top page pops it."(navigator.h:84-88)라고 명시. 그러나 nav page가 하나뿐일 때 `Remove`→`Pop(false)`로 위임되고, `Pop`은 `size<=1`이면 무동작 후 빈 핸들 반환.
- 근거: `navigator-impl.cpp:181-184`, `:123-126`.
- 영향: 유일 page를 `Remove`로 제거 불가(`Clear`만 가능). silent no-op. (2개 이상일 때 top remove는 정상.)

### M3. `DialogContainer` 문서가 구현과 다름 (centered)
헤더가 모달 콘텐츠를 "centered above the scrim"이라 두 번 명시(dialog-container.h:41,62)하지만, `SetModalContent`은 `Self().Add()`+`RaiseToTop()`만 한다(dialog-container-impl.cpp:93-109). 실제 centering은 sample이 `AbsoluteLayoutParams`로 직접 처리(navigator-example.cpp:129-137).

### M4. 취소 불가 모달 옵션 없음 + scrim 콜백이 source 검증을 안 함
scrim 탭과 Back은 *항상* dismiss한다(`dialog-container-impl.cpp:87,148-155`, `navigator-impl.cpp:258,532-535`). 샘플의 "Delete? 되돌릴 수 없음" 경고조차 바깥 탭으로 사라진다(navigator-example.cpp:124-127). 또한 `OnScrimClicked(Ui::DialogContainer /*container*/)`는 전달된 container 인자를 무시하고 무조건 `PopModal(true)`만 호출(`:532-535`)하여, 어떤 컨테이너가 트리거했는지·그것이 현재 top modal인지 검증하지 않는다. `setCancelable(false)` 등가물 부재.

### M5. `AlertDialog`는 하드코딩 sample UI 수준, dialog 결과 계약 없음
- 하드코딩: 버튼을 `InteractiveView`+`Label`로 라벨 위치 `(16,18)` 고정, 행 높이 64px 고정, 색 하드코딩 조립(`alert-dialog-impl.cpp:121-145`; title/message 색·폰트 `:79-83,:99-103`). 라벨 중앙 정렬·측정·생략(…)·포커스 불가. 헤더 주석도 "temporary ... until a dedicated Button component exists"라고 인정(alert-dialog.h).
- 계약 부재: `SetActionButtons`는 `(label, std::function<void()>)` pair만 받는다(alert-dialog.h:124, `.cpp:111-160`). 어떤 버튼 선택됐는지 결과 반환·default/cancel/destructive role·auto-dismiss·Return/Esc 처리 없음. sample은 콜백에서 수동 `PopModal()`(navigator-example.cpp:126-127).

### M6. 긴 본문 스크롤 / 최대 크기 없음
`Dialog`는 세로 `StackLayoutManager`만 붙이고(`dialog-impl.cpp:73`) 스크롤·max-height가 없다. `AlertDialog` 본문은 단일 `Label`(무제한 높이)이라 긴 메시지가 화면을 넘치거나 잘린다.

### M7. 등록된 property가 0개 — 테마/애니메이션/스타일 불가
4개 컨트롤 모두 property index 범위만 예약하고 enumerator는 비었다(`navigator-properties.h:37-46`, `dialog-properties.h:39-72`). 재검증(grep): `PROPERTY_REGISTRATION` 0건. spacing/alignment/scrim color/transition duration이 전부 C++ setter 전용 → `UiStyle`/테마·property 애니메이션 불가. 같은 리포 `chart-view`는 빈 `Create()`를 쓰면서도 property를 등록(chart-view-impl.cpp:64-84).

### M8. 백핸들러 self-retain 누수 위험 (조건부)
`mBackHandlers`가 `std::function<bool()>`를 값으로 보관(`navigator-impl.h:129`, `SetBackHandler` `.cpp:383-394`, invoke/remove `:537-559`). `nav.SetBackHandler(page, [nav]{...})`처럼 Navigator 핸들을 캡처하면 impl→function→Navigator 강참조 순환으로 refcount가 0에 도달하지 못한다. 무조건 발생이 아니라 strong 캡처 시의 foot-gun이며, `WeakHandle`은 리포 기존 패턴인데 가드도 문서 경고도 없다(`navigator.h:140`).

### M9. scrim 시그널 미해제 (cross-navigator 재사용 시 stale)
`PushModal`이 `ScrimClickedSignal`을 연결(`navigator-impl.cpp:258`)하지만 `PopModal/Remove/Clear` 어디서도 `Disconnect` 안 함(grep: `Disconnect` 0건; Connect 사이트는 scrim `:258`·애니메이션 `:456` 둘뿐).
- 영향: 동일 `DialogContainer`를 *다른* `Navigator`에 재사용하면 옛 Navigator 슬롯이 살아 있어 scrim 탭이 두 Navigator의 `PopModal`을 모두 호출, 무관한 모달이 닫힐 수 있다. (같은 Navigator 재push는 dedup으로 안전 — §4.)

### M10. ownership 정책 부재
- 같은 `View`를 여러 슬롯에 넣는 것을 막지 않음 — `DialogImpl`은 `mHeaderView==mBodyView`를 reject/clear하지 않는다(`dialog-impl.cpp:76-117,158-169`). `Actor::Add` reparent 덕에 crash는 아니나 슬롯 불변식 없음.
- parent 있는 view 전달 시 transfer 미문서화 — `AddChildFill`/`AddSection`/`SetModalContent`가 parent 검사 없이 `Self().Add(view)`(`navigator-impl.cpp:415-420` 등). `Actor::Add`는 기존 parent에서 자동 제거·reparent(dali-core actor.h:843-849)하므로 caller 기존 tree에서 view가 사라지는 side effect.
- 외부 제거 hook 없음 — child가 외부에서 parent 제거/파괴될 때 스택을 회복하는 `OnChildRemove` 처리 없음(grep 0건).

---

## 3. 낮음 / 사소 (소스 확인)

- **L1. 숨은 page가 레이아웃 트리에 상주** — `AbsoluteLayoutManager::Measure`가 standalone만 제외하고 *모든* 자식을 measure(가시성 무관, `absolute-layout-manager.cpp:103-162`, 주석 "Always measure children"). `ViewImpl::Measure`는 constraint 기준 캐시라 상시 비용은 작으나, 모든 page의 view 트리가 메모리에 상주하고 navigator resize 등 `InvalidateMeasure` 시 숨은 스택 전체가 재측정된다. 네이티브 off-screen detach 모델과 대비.
- **L2. Navigator가 Back/Escape를 스스로 처리 안 함** — `OnKeyEvent` 미오버라이드(grep 0건; navigator-impl.h 오버라이드는 `OnInitialize`뿐). 앱이 window key를 수동 연결해야 함(navigator-example.cpp:183-195). (`NavigateBack()` API 자체는 동작하므로 낮음.)
- **L3. appear 시그널이 scene 연결과 무관하게 방출** — grep: `OnSceneConnection`/`IsOnScene` 0건. Navigator가 scene에 없어도 `PageWillAppear/DidAppear` 방출.
- **L4. Navigator가 page의 OPACITY property를 덮어씀** — `FinishTransition`이 incoming opacity를 1로 강제 복구(`:476`). page가 opacity를 의미 있게 쓰면 덮인다.
- **L5. custom scrim 교체 시 이전 scrim 시그널 정리 불명확** — `SetScrim`은 새 scrim에 connect만(`dialog-container-impl.cpp:116-141`), 이전 scrim 시그널 해제는 destruction에 의존.
- **L6. scrim 색·dim 양 하드코딩** (`0x000000`/0.5, `dialog-container-impl.cpp:84`), 테마/속성 훅 없음.
- **L7. `InsertBefore` 및 비-top `Remove`가 in-flight 전환을 settle 안 함** (`navigator-impl.cpp:154-211`).
- **L8. `~NavigatorImpl`이 `Stop()`만, `Clear()/Reset()` 안 함** (`navigator-impl.cpp:69-75`).
- **L9. DownCast가 프로젝트 헬퍼를 안 씀** — `ChartView`는 `Ui::View::DownCast<ChartView, Integration::ChartViewImpl>(handle)`(chart-view.cpp:71-74; 헬퍼 view.h:2194-2221)를 쓰는데, 새 4개 클래스는 동일 `dynamic_cast` 로직을 수동 반복(`navigator.cpp:67-81` 등). 버그 아님, 보일러플레이트·drift 위험.
- **사소**: 빈 `Create()` 반환→TypeRegistry/builder 인스턴스화 불가(단, 리포 관례); `GetCurrentView`가 nav/modal 두 스택 혼재; `Pop()` 빈 핸들이 "1개"와 "0개" 구분 불가; 공개 헤더 `virtual ~Impl()`(대신 `override`); leaf impl 미-`final`; Navigator 헤더 문서가 Dialog 대비 빈약; 샘플 미사용 `mWindowW/mWindowH`.

---

## 4. 정정 / 반증 — 과장이거나 거짓으로 판명된 주장

선행 리뷰들의 다음 주장은 소스 재검증 결과 부정확하므로 정정한다(본 문서 결함 목록은 정정된 형태만 채택).

1. **"같은 DialogContainer 재push 시 scrim 연결이 누적되어 한 탭에 여러 모달이 닫힌다" → 거짓.** dali-core `BaseSignal::OnConnect`가 (함수포인터, 객체포인터) 동일 슬롯을 `FindCallback`으로 찾아 중복 연결을 막는다(base-signal.cpp:172-197, 동등성 `:49-64`). 같은 Navigator 재push는 단일 연결. 실제 결함은 cross-navigator stale 연결로 한정(M9).
2. **"`FinishTransition`이 settle 후 항상 double-emit 한다" → 거짓.** `mTxIncoming`/`mTxOutgoing`/`mTransition`을 emit *전에* 모두 리셋(`navigator-impl.cpp:487-497`). 실제 문제는 동기 settle·재진입 계약 부재(H6).
3. **"animated path가 전혀 실행되지 않는다" → 부정확.** `NavigateBack`이 내부적으로 `Pop(true)`/`PopModal(true)` 호출(`:360-379`)하므로 animated 분기가 호출될 수 있다. 다만 테스트가 completion·signal·order를 검증하지 않으므로 *animated 동작 보장*은 여전히 비어 있다(Navigator UTC 18개 모두 `animated=false`).
4. **"Push의 cross-fade가 깨졌다" → 표현 정정.** 헤더는 cross-fade를 약속하지 않는다("optional fade"). 정확히는 *한쪽만 페이드*하는 거친 전환(M1).
5. **"`RestackModals`가 애니메이션 중 트리를 재정렬한다" → 거짓.** 모든 mutator가 시작부에서 `SettlePendingTransition()`을 먼저 호출해 트리를 정착시킨 뒤 진행.
6. **"AlertDialog 버튼 라벨 고정 위치가 RTL을 깬다" → 거짓.** 리포가 RTL 미러링을 중앙 처리(고정 픽셀이 자동 미러).
7. **"애니메이션 FinishedSignal 슬롯이 dangling 한다" → 거짓.** `ViewImpl`이 `ConnectionTrackerInterface`(view-impl.h:94)라 객체 파괴 시 자동 해제.

---

## 5. 설계 격차 — C/C++ 및 네이티브 프레임워크 비교

모델 자체가 성숙한 navigation 시스템에 한 세대 뒤처져 있다. "stack vector + fade + scrim"은 navigation의 출발점일 뿐이다.

### 라우팅 · 상태 (치명적 격차)
- **선언적/네임드 라우트 없음.** `Push(View)`로 만들어진 인스턴스만 받는다. SwiftUI `NavigationStack.navigationDestination(for:)`, Jetpack `navController.navigate("route")`, Flutter named routes, React/Vue Router, WinUI `Frame.Navigate(typeof(Page), param)` 전부 데이터/문자열 구동.
- **딥링킹/URL 복원 없음.** iOS `onOpenURL`, Android `<deepLink>`, Flutter `RouteInformationParser` 대비 부재.
- **상태 저장/복원 없음.** 스택이 `std::vector<Ui::View>` 라이브 핸들(navigator-impl.h:127-128)뿐 — 프로세스 종료·회전 시 소실. iOS state restoration, Android `SavedStateHandle`/`saveState()`, SwiftUI `NavigationPath` Codable 대비 부재.

### 모달 · 다이얼로그 (치명적 격차)
- **결과/응답 코드 없음 — 가장 흔한 다이얼로그 관용구.** Qt `QDialog::exec()`→int, GTK `gtk_dialog_run()`/`AlertDialog.choose()`→response, WinUI `ContentDialog.ShowAsync()`→`ContentDialogResult`, Flutter `showDialog<T>()`→`Future<T?>`. 여기는 `std::function<void()>`(반환 없음).
- **기본/취소 버튼, Esc 닫기, close-policy 없음.** Qt `setDefault`, GTK `default-button`/`cancel-button`, WinUI `DefaultButton`, web `<dialog>` Esc 자동 취소, QML `closePolicy` 전부 없음.
- **프레젠테이션 스타일·비취소 모달 없음.** iOS sheet/fullScreen/formSheet/popover+detents, `interactiveDismissDisabled()`; Android `BottomSheetDialogFragment`/`setCancelable(false)`. 여기는 "scrim 위 카드 페이드인" 단 하나.

### 스택 조작 (높음 격차)
- **`CanGoBack`/`PopToRoot`/`PopTo`/`Replace`/`SetRoot`/`pop-N` 없음.** 단발 `Pop`과 전체 `Clear`뿐. iOS `popToRootViewController`/`setViewControllers`, Android `popBackStack(id, inclusive)`/`launchSingleTop`, Qt `StackView.replace`/`popToIndex`, web `replaceState`.
- **forward 히스토리 없음**(WinUI `Frame.CanGoBack/GoBack/ForwardStack`, web `history.forward()`), **탭/중첩 그래프(다중 백스택) 없음**(iOS `UITabBarController`+per-tab `NavigationStack`, Android nested graphs).

### 전환 · 애니메이션 (높음 격차)
- **유일한 전환이 고정 0.25s 불투명도 페이드.** 타입/지속/이징/방향 per-call 지정 불가. **인터랙티브 제스처 백(edge-swipe), Android 13+ predictive back, 공유요소(hero) 전환** 전무. QML `StackView` 방향별 `Transition`, WinUI `NavigationTransitionInfo`, iOS `UIViewControllerAnimatedTransitioning` 대비 큰 격차.

### 관찰성 · 자동 chrome · ownership
- **자동 내비게이션 바/백버튼/타이틀 없음** — Navigator는 맨 `AbsoluteLayout` 컨테이너. iOS/SwiftUI/Android `TopAppBar` 기본 제공과 대비.
- **현재 라우트 관찰 불가** — Android `addOnDestinationChangedListener`, Flutter `NavigatorObserver` 대비, 시그널이 `View` 핸들만 실음.
- **ownership/lifecycle 계약 없음** — UIKit presenting/presented relationship, Qt `StackView` item ownership(pop 시 destroy) 대비, 여기는 핸들 vector add/remove뿐.

---

## 6. 테스트 / 샘플 부실 (소스 확인)

- **애니메이션 경로가 직접 검증되지 않음** — Navigator UTC 18개가 명시적으로 `animated=false`(완료·시그널·순서 무검증).
- **5개 시그널 전부 미검증** — `PageWill*`/`PageDid*`/`TransitionFinished`의 emit 순서·시점·`byPop`·page 인자 무검증(H4·H5의 결함이 테스트로 잡히지 않는 이유).
- **`Remove` 전 분기 미테스트** (top/middle/modal/not-found, 특히 root remove 무동작).
- **`AlertDialog` 버튼 클릭 핸들러 호출 미검증** — `cancelCalls`/`okCalls`를 만들고 콜백 등록하지만 click을 발생시키거나 카운터를 단언하지 않음(utc-Dali-AlertDialog.cpp:156-174).
- **`ScrimClickedSignal` 테스트가 무의미** — 연결 수 0만 확인(utc-Dali-DialogContainer.cpp:157-163).
- spacing/alignment는 getter만 검증; 재진입·null 핸들·double-pop·InsertBefore 엣지·중복 섹션·focus/a11y 격리·cross-navigator scrim 무검증.
- **샘플이 API의 빈 부분을 메움** — centering을 `AbsoluteLayoutParams`로 직접, action 콜백에서 수동 `PopModal()`, 고정 픽셀/색/라벨 위치(navigator-example.cpp:121-140, dialog-example.cpp:75-84,150-164), key를 window에서 직접 처리(:183-195).

---

## 7. 우선 수정 권고

**Merge 전 반드시:**
1. **Modal contract 정의·구현 (H1·H2·H3)**: push 시 가려진 subtree `SetDescendantFocusBlocked(true)`, 모달에 `RequestFocus`, pop 시 포커스 복원, `OnKeyEvent`에서 Back/Esc 소비, `DIALOG`/`ALERT` role·name·modal 플래그.
2. **Lifecycle 시그널 교정 (H4·H5)**: `Will*`을 상태 변경 *전*에 emit하거나 개명; 모달로 *가려지기만* 하고 사라지지 않는 nav 페이지에는 disappear/appear 시그널을 보내지 않거나 "covered/uncovered"용 별도 시그널로 구분. **재진입 가드 (H6)** 추가.
3. **Transition state machine 명시 (M1)**: 진행 중 operation 허용/queue/reject, cancellation, enter/exit 양쪽 애니메이션, duration/easing/type 커스터마이즈.
4. **Scrim dismiss 경로 (M4·M9)**: connect/disconnect lifecycle, `OnScrimClicked`에서 source가 current top modal인지 검증, cross-navigator 방어, `cancelable=false` policy.
5. **`Remove(유일 page)` 버그 (M2)**, **`DialogContainer` 문서/구현 일치 (M3)**, **property 등록으로 테마/애니메이션 가능화 (M7)**.
6. **`AlertDialog` 재설계 (M5)**: result/default/cancel/destructive role, auto-dismiss policy, Return/Esc 처리, theme/property 통합, 실제 Button 컴포넌트 의존 명확화.
7. **테스트를 invariant 중심으로 재작성**: animated 경로·시그널 순서/시점(H4·H5)·focus trap/restore·scrim dismiss·root remove·버튼 클릭 디스패치·a11y.
8. **프로젝트 패턴 준수**: `View::DownCast<T, Impl>()` 헬퍼(L9), property enum/registration 일관성, public docs↔구현 일치.

**후속 설계:** `CanGoBack`/`PopToRoot`/`PopTo`/`Replace`/`SetRoot`, route/key 기반 navigation, state save/restore, nested/per-tab stack, dialog result passing, forward 히스토리, public property registration, style/theme 통합.

---

## 최종 판정

"initial prototype"으로는 이해 가능하나 "view navigation including dialog"라는 public component로 merge하기엔 부족하다. 가장 심각한 결함은 **modal이 modal이 아니고(H1·H2·H3), lifecycle 시그널이 이름·실제 동작과 어긋나며(H4·H5), dialog가 result/default/cancel/focus/accessibility 계약을 갖지 않는다(M4·M5·H3)**는 점이다. 특히 H5(가려지기만 한 페이지에 disappear/appear를 방출)는 사용자 코드의 리소스 정리 로직을 직접 오작동시키는 lifecycle 정확성 버그로, 코드 체인을 끝까지 추적해 확정했다. 이 상태로 들어가면 이후 ABI 호환성 때문에 고치기 더 어려워진다. 지금은 merge보다 contract 재정의와 테스트 보강이 먼저다. (선행 리뷰의 scrim 중복·double-emit·cross-fade·dangling 단정은 §4에서 정정했다.)
