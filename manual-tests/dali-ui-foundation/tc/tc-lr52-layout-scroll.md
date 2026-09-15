# LR52. Layout: Scroll and page layout

이 TC는 ScrollView의 constraint 전달·scroll 보존·축별 padding/margin과 PageScrollView의 layout 기반 page 수·viewport 변경·page 삽입/삭제를 검증합니다. 대응 소스는 `tc-lr52-layout-scroll.cpp`입니다. 기존 TC 실행 결과는 사용하지 않습니다. diagnostics OFF/ON 모두 **40 scenario, 99 action, 2135 required checks**를 실행하십시오.

## 실행 조건

- foundation manual-test 앱에서 `LR52`를 검색하고 `Scroll and page layout`에 진입하십시오.
- `Reset run`을 한 번 누르고 새 `run_id`를 기록하십시오. scale=1, LTR, parent-local 좌표이며 extent 단위는 visual unit입니다.
- pan과 scroll animation은 fixture가 끕니다. 별도 drag, resize 또는 animation을 시작하지 마십시오.
- 이 TC는 diagnostics API를 요구하지 않습니다. 숫자는 결과 행 또는 같은 run의 stdout 원문에서 읽으십시오. screenshot으로 위치를 추정하지 마십시오.

## 조작 및 판정

1. 현재 `scenario_id`, `action_seq`를 기록하고 `Apply next step`을 한 번 누르십시오.
2. 동기 action은 그 호출에서 끝납니다. mount/resize/content 변경 action은 해당 View들의 `LayoutFinishedSignal`과 같은 action의 Window 완료 fence를 기다립니다. 10초 안에 완료되지 않거나 필요한 snapshot이 빠지면 FAIL입니다.
3. `Read result`를 누르고 전체 결과를 읽으십시오. `Next result page`로 같은 action의 남은 행을 읽습니다. 이는 `Next scenario`와 다른 조작입니다.
4. 아래 표의 `required_checks`와 실행 행 수가 정확히 같고 모든 필수 행이 PASS여야 action을 통과시킵니다. `Next scenario`는 현재 scenario의 모든 action이 끝난 후에 누르십시오.
5. 아래 40 scenario, 99 action을 모두 실행하고 2135개 검사 행을 보존하십시오. 실패·누락·timeout이 하나라도 있으면 TC를 PASS로 기록하지 마십시오. 알려진 문제도 xfail로 통과시키지 마십시오.
6. run/scenario/action ID, 필수·실행 행 수, 실패 actual/expected/tolerance, 원문 로그를 저장하십시오. 동일 fixture의 후속 실패가 앞 단계 오류에 따른 것인지도 기록하되 검사를 생략하지 마십시오.

`Rect`는 x/y/width/height 네 행, `Size`는 width/height 두 행입니다. geometry 허용오차는 0.001이며 정수·bool·producer 수는 정확히 일치해야 합니다. explicit constraint 비교는 오차 0입니다. NaN/Inf는 실패합니다. unconstrained callback 입력은 infinity가 아닌 `std::numeric_limits<float>::max()`라는 유한값입니다. candidate 출력으로 expected를 갱신하지 마십시오.

## I05.scroll-preservation — 7 action / 49 checks

| action | 독립 기대값 | 필수 행 수 |
|---|---|---:|
| mount | viewport100×80, content240×200 | 8 |
| scroll-to | scroll(50,60), content(−50,−60,240,200) | 6 |
| settled-layout-replay | 다음 실제 Window layout pass에서도 같은 scroll과 content target 유지 | 6 |
| upper-clamp | max=(240−100,200−80)=(140,120), content(−140,−120,240,200) | 6 |
| resize-viewport | viewport160×120, max=(80,80), 실제 content(−80,−80,240,200) | 10 |
| negative-clamp | scroll(0,0), content(0,0,240,200) | 6 |
| replace-smaller-content | 새 content60×40, old content parent 없음, scroll(0,0) | 7 |

`settled-layout-replay`는 ScrollView의 producer가 현재 scroll 위치를 반영하는지 검사합니다. viewport resize는 geometry가 같은 content에서도 clamp가 갱신되어야 합니다. Window snapshot은 callback 당시 layout target이고 실제 Actor position은 component의 post-layout scroll 적용 결과입니다. 각 행에서 정한 관측 시점을 혼합하지 마십시오.

## SC.match-constraints.0…3 — 각 1 action / 18 checks

각 scenario에서 `run`을 실행하십시오. viewport 요청100×80, ScrollView padding(start,end,top,bottom)=(11,17,7,13), inner constraint=(72,60)입니다. custom content producer의 자연 크기는240×200입니다. requested 크기가 MATCH_PARENT인 축만 inner constraint를 받습니다.

| scenario | width / height 요청 | 최초 producer constraint | measured 및 arranged content 크기 | Arrange 재측정 constraint | 누적 Measure producer 수 |
|---|---|---|---|---|---:|
| SC.match-constraints.0 | WRAP / WRAP | FLT_MAX / FLT_MAX | 240×200 | 재측정 없음 | 1 |
| SC.match-constraints.1 | MATCH / WRAP | 72 / FLT_MAX | 72×200 | 72 / 200 | 2 |
| SC.match-constraints.2 | WRAP / MATCH | FLT_MAX / 60 | 240×60 | 240 / 60 | 2 |
| SC.match-constraints.3 | MATCH / MATCH | 72 / 60 | 72×60 | 72 / 60, cache hit | 1 |

18행은 owner Measure2, 최초 callback constraint2, content measured2, 최초 producer1, owner Arrange 반환4, content geometry4, 최종 callback constraint2, 누적 producer1입니다. owner Arrange 반환은(0,0,100,80), content origin은 padding에 따른(11,7)입니다. callback 카운터는 이 synthetic producer만 계수합니다.

## SC.axis-padding-margin.0…2 — 각 4 action / 76 checks

각 scenario에서 `mount → maximum → resize → padding-change` 순서로 실행하십시오. **각 action 19행**입니다. mode0=Horizontal, mode1=Vertical, mode2=Both입니다. axis는 getter와도 정확히 대조합니다.

| action | viewport | ScrollView padding(start,end,top,bottom) | mode0 content | mode1 content | mode2 content |
|---|---|---|---|---|---|
| mount | 100×80 | 11,17,7,13 | 240×60 | 72×200 | 72×60 |
| maximum | 100×80 | 11,17,7,13 | 240×60 | 72×200 | 72×60 |
| resize | 130×110 | 11,17,7,13 | 240×90 | 102×200 | 102×90 |
| padding-change | 130×110 | 2,4,3,5 | 240×102 | 124×200 | 124×102 |

mount의 scroll은(0,0)입니다. maximum은 `ScrollTo(999,999,false)`이고 이후 resize/padding에서도 유효한 최대 위치로 clamp됩니다. maxScroll=(max(contentWidth−viewportWidth,0), max(contentHeight−viewportHeight,0))이므로 mode0 x는140→110→110, mode1 y는120→90→90, mode2는 항상(0,0)입니다. content origin은 `(padding.start−scroll.x, padding.top−scroll.y)`입니다.

content 내부에는 padding(3,5,7,11)을, 내부 MATCH child에는 margin(2,4,6,8)을 적용합니다. 내부 child의 local rect는 `(5,13,max(contentWidth−14,0),max(contentHeight−32,0))`입니다. 이는 content 내부 기본 Layout의 margin/padding 결합 검사이며 ScrollView가 content 자체의 margin을 처리한다는 별도 계약을 가정하지 않습니다.

19행은 viewport Rect4, content Rect4, 내부 margin child Rect4, scroll2, integration API의 viewport2·scrollable size2, direction1입니다. mount/resize/padding의 실제 Actor event property는 필요한 View snapshot을 모두 받은 Window fence에서 읽습니다.

## PAGE.horizontal / PAGE.vertical — 각 8 action / 156 checks

두 축에서 아래 모든 action을 실행하십시오. cross viewport는80이며 main viewport는 처음100, resize 후125입니다. content는 같은 축의 StackLayout이며 page 길이는100,100,50입니다. 마지막 부분 page를 포함한 실제 content 길이는250입니다. page 크기 기본값(0,0)은 viewport를 사용하므로 pageCount=ceil(contentLength/pageLength)입니다. scroll main은 `[0,max(contentLength−viewportLength,0)]`로 clamp됩니다.

| action | 주요 독립 기대값 | 필수 행 수 |
|---|---|---:|
| mount-default-size | pageCount3, current0, scroll0, content250, 최초 PageChanged(0,3) 1회 | 14 |
| page-boundaries | ScrollToPage(−7,false)→page0/scroll0; ScrollToPage(99,false)→page2/scroll150, 마지막 signal(2,3) | 23 |
| viewport-resize | main viewport125, pageCount2, current1, scroll125, content250, signal(1,2) | 13 |
| explicit-page-size | main page100/cross17, pageCount3, current1, 기존 scroll125 유지, signal(1,3), page size getter 정확 일치 | 15 |
| insert-before-current | 앞에 길이100 page를 추가한 뒤 NotifyPagesInserted(0,1); 즉시 count4/current2/scroll200/signal(2,4) 1회; layout 후 content350, 기존 page identity·local 위치200·viewport 내 위치0 유지 | 30 |
| remove-before-current | 앞 page 제거 후 NotifyPagesRemoved(0,1); 즉시 count3/current1/scroll100/signal(1,3) 1회; layout 후 content250 | 24 |
| remove-all | 실제 page 모두 제거 후 NotifyPagesRemoved(0,3); 즉시 count0/current−1/scroll0/signal(−1,0) 1회; layout 후 content main extent0 | 24 |
| restore-default | page size(0,0) 복원, 길이100 page 추가; pageCount1/current0/scroll0/content100/signal(0,1) | 13 |

각 기본 `Check`는 count/current2, scroll main/cross2, content Rect4, integration API의 scrollable main extent/viewport main extent2로 **10행**입니다. cross scroll은0입니다. `SignalCheck`는 고정64개 event buffer에 overflow가 없고 event가 존재한다는 조건1, 마지막 page/count2로 **3행**입니다. insert에서는 추가로 기존 child identity1, 기존 현재 page local Rect4, viewport 내 위치1을 검사합니다.

Notify 직후에는 예상 page 수가 즉시 반영되지만 실제 content Arrange는 다음 layout에서 이루어집니다. 이 시점을 분리해서 읽으십시오.

| 관측 | insert 직후 | remove 직후 | empty 직후 |
|---|---:|---:|---:|
| 공개 expected page 수 | 4 | 3 | 0 |
| 선반영 scrollable main extent(page100×expectedCount) | 400 | 300 | 0 |
| 아직 재배치 전 Actor content main extent | 250 | 350 | 250 |
| 다음 layout 후 content·scrollable main extent | 350 | 250 | 0 |

부분 마지막 page 때문에 선반영 range와 최종 geometry가 다릅니다. page count·현재 identity 보존을 검사하면서 이 임시 range를 최종 geometry로 오인하지 마십시오. viewport만 바뀌고 content 크기는 그대로인 resize도 count/current/scroll 갱신을 검사하므로 누락된 invalidation을 드러낼 수 있습니다.

## PAGE.notify-horizontal/vertical — 추가 24 scenario / 48 action / 1104 checks

아래 suffix11개를 두 축에서 각각 실행하십시오. 정확한 scenario ID는 `PAGE.notify-horizontal.SUFFIX`, `PAGE.notify-vertical.SUFFIX`입니다. 각 scenario는 독립적인 길이100 page5개로 시작하며 cross extent80, viewport main100입니다. `mount`는14행이고 initial current를 아래 표의 값으로 즉시 이동시킨 뒤 count5·scroll=`100×current`·geometry·signal을 검증합니다.

`notify-and-settle`은 실제 child tree를 먼저 바꾸고 API를 호출합니다. 각 행의 actual index/count는 TC가 tree를 수정할 위치이며 API 인수와 구별됩니다. clamp 기대는 표에 고정되어 있고 candidate getter에서 생성하지 않습니다.

| SUFFIX | initial current | API index/count | 실제 tree index/count | 최종 count/current/scroll | notify-and-settle 필수 행 |
|---|---:|---|---|---|---:|
| insert-at-current | 2 | 2/1 | 2/1 삽입 | 6/3/300 | 31 |
| insert-after | 2 | 4/1 | 4/1 삽입 | 6/2/200 | 31 |
| insert-negative-index | 2 | −7/1 | 0/1 삽입 | 6/3/300 | 31 |
| insert-past-end | 2 | 99/1 | 5/1 삽입 | 6/2/200 | 31 |
| remove-after | 2 | 4/1 | 4/1 삭제 | 4/2/200 | 31 |
| remove-current | 2 | 2/1 | 2/1 삭제 | 4/2/200 | 31 |
| remove-through-current | 2 | 1/3 | 1/3 삭제 | 2/1/100 | 31 |
| remove-last-current | 4 | 4/99 | 4/1 삭제 | 4/3/300 | 31 |
| remove-negative-index | 2 | −7/1 | 0/1 삭제 | 4/1/100 | 31 |
| remove-past-end | 2 | 99/99 | 4/1 삭제 | 4/2/200 | 31 |
| remove-all-clamped | 2 | −7/99 | 0/5 삭제 | 0/−1/0 | 26 |

`notify.immediate.*`10행은 count/current, scroll2, 아직 이전 extent500인 content Rect4, 새 예상 scrollable extent=`100×count`, viewport100을 검사합니다. `page.signal.*`3행은 overflow 없는 buffer와 최종 page/count, `notify.signal-delta=1` 및 `notify.logical-count`가 뒤따릅니다.

Window fence 뒤 `notify.settled.*`10행은 실제 content extent=`100×count`를 검사합니다. 비어 있지 않으면 `notify.visible-identity`, `notify.visible-local` Rect4, `notify.visible-origin=0`도 필수입니다. 현재 page를 삭제한 경우에는 그 자리에 남은 다음 page 또는 마지막 유효 page identity가 기대값입니다. empty는 `notify.empty-children` 한 행을 사용합니다. old page identity를 삭제 후에도 보존하라고 요구하지 않습니다.

두 축에 각각 `PAGE.notify-AXIS.nonpositive-count`도 있습니다. `mount`14행 뒤 `notify-noop`48행을 실행하십시오. 삽입 count0/−3, 삭제 count0/−3 네 호출은 tree 변경 없이 count5/current2/scroll200/content500을 그대로 유지해야 하며 각각 signal delta0입니다. `noop.0…3.*` 각11행, 마지막 `page.signal.*`3행 및 `noop.logical-count=5`가 필요합니다. no-op도 signal을 발생시키는 회귀를 검출합니다.

## PAGE.pre-layout-horizontal/vertical — 추가 6 scenario / 12 action / 370 checks

각 축에서 `insert-before`, `insert-after`, `remove-empty` suffix를 실행하십시오. 정확한 ID는 `PAGE.pre-layout-horizontal.SUFFIX`, `PAGE.pre-layout-vertical.SUFFIX`입니다. 첫 action `notify-before-layout`은 fixture를 Window에 **붙이지 않습니다**. 요청 viewport100과 page100 하나를 구성해도 실제 viewport·scrollable은 아직0이어야 합니다. 다음 action `attach-and-settle`에서만 붙입니다. agent가 그 사이 fixture를 별도로 attach하거나 임의 Measure/Arrange로 준비하지 마십시오.

| SUFFIX | layout 전 실제 tree와 Notify | 즉시 count/current/scroll | attach 이후 기대 | action별 필수 행 |
|---|---|---|---|---|
| insert-before | 기존 page 앞에 page100 추가, NotifyPagesInserted(0,1) | 2/1/0 | count2/current1/scroll100, 기존 page가 viewport origin에 표시 | 41 → 20 |
| insert-after | 기존 page 뒤에 page100 추가, NotifyPagesInserted(1,1) | 2/0/0 | count2/current0/scroll0 | 41 → 20 |
| remove-empty | 유일한 page 삭제, NotifyPagesRemoved(0,1); 이어 빈 상태에서 NotifyPagesRemoved(99,7) | 0/−1/0, 두 번째 호출은 signal 추가 없음 | count0/current−1/scroll0/content main0 | 48 → 15 |

모든 첫 action은 `pending.initial.*`에서 default page size의 보수적 count1/current0을, `pending.explicit.*`에서 main100/cross17의 명시 size를 적용한 count0/current−1을 검사합니다. 실제 scrollable0이므로 이 시점에는 signal이 없어야 합니다. size getter2행도 필요합니다. 같은 size 재설정은 `pending.same-size.*` 상태를 보존해야 합니다. (0,0)으로 복원한 `pending.default.*`는 다시 count1/current0입니다.

각 `CheckUnmeasuredPage` prefix의7행은 count/current2, scroll x/y2, scrollable main/viewport main2, signal-count1입니다. Notify 후 `pending.notified.*`7행, `page.signal.*`3행, `pending.logical-count`1행을 검사합니다. remove-empty는 `pending.already-empty.*`7행이 추가됩니다. insert-before의 layout 전 scroll0은 아직 range가 없기 때문이며, 실제 첫 layout 이후에도 pending current page의 표시 위치를 잃어도 된다는 뜻이 아닙니다.

attach action은 지정 View snapshot과 Window fence 후 `pending.settled.*`10행, `page.signal.*`3행, logical count1행을 읽습니다. 비어 있지 않으면 expected visible identity1, local Rect4, `pending.visible-origin=0`1행, empty이면 `pending.empty-children`1행을 추가합니다. page current index와 실제 보이는 page가 서로 어긋나면 FAIL입니다.

## 근거와 검증 경계

- `public-api/layouts/scroll-view-layout-manager.cpp`의 MATCH 축 constraint, 자연 축 unconstrained Measure, Arrange-owned 재측정, ALWAYS Arrange와 실제 scroll 위치 보존을 검사합니다.
- `integration-api/scroll-view-impl.cpp`의 viewport·scrollable size·padding origin·AdjustScrollPosition·ApplyScrollPosition을 수치로 대조합니다.
- `public-api/views/scroll/page-scroll-view.h`의 page 크기·count·index·Notify 계약과 `integration-api/page-scroll-view-impl.cpp`의 즉시 expected count/다음 layout 갱신 경로를 검사합니다.

이 TC는 scroll과 Layout의 결합을 대상으로 하며 fling 물리·gesture 인식·scrollbar GUI 전체를 검증한다는 의미는 아닙니다. timing은 측정하지 않습니다. `LR59/60/64`의 성능 결과를 이 TC의 정확성 통과로 대체하지 마십시오.

## 종료와 재현

`< Back`으로 나간 뒤 같은 TC에 다시 진입하고 `Reset run` 이후 첫 scenario를 반복하십시오. run ID와 counter가 새로 시작하고 초기 기대값이 같아야 합니다. 반복 실행은 위 2135행 집계와 별도로 기록하십시오.

## 고정 실행 계약과 종료 증거

각 Apply 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 모든 action 뒤 `Read result`로 해당 action의 전체 stdout 행을 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 40 scenario, 99 action, 2135 checks. diagnostics가 필수인 action은 OFF의 capability 실패를 PASS로 처리하지 마십시오.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `I05.scroll-preservation` | `mount` : 8 → `scroll-to` : 6 → `settled-layout-replay` : 6 → `upper-clamp` : 6 → `resize-viewport` : 10 → `negative-clamp` : 6 → `replace-smaller-content` : 7 |
| `SC.match-constraints.0` | `run` : 18 |
| `SC.match-constraints.1` | `run` : 18 |
| `SC.match-constraints.2` | `run` : 18 |
| `SC.match-constraints.3` | `run` : 18 |
| `SC.axis-padding-margin.0` | `mount` : 19 → `maximum` : 19 → `resize` : 19 → `padding-change` : 19 |
| `SC.axis-padding-margin.1` | `mount` : 19 → `maximum` : 19 → `resize` : 19 → `padding-change` : 19 |
| `SC.axis-padding-margin.2` | `mount` : 19 → `maximum` : 19 → `resize` : 19 → `padding-change` : 19 |
| `PAGE.horizontal` | `mount-default-size` : 14 → `page-boundaries` : 23 → `viewport-resize` : 13 → `explicit-page-size` : 15 → `insert-before-current` : 30 → `remove-before-current` : 24 → `remove-all` : 24 → `restore-default` : 13 |
| `PAGE.vertical` | `mount-default-size` : 14 → `page-boundaries` : 23 → `viewport-resize` : 13 → `explicit-page-size` : 15 → `insert-before-current` : 30 → `remove-before-current` : 24 → `remove-all` : 24 → `restore-default` : 13 |
| `PAGE.notify-horizontal.insert-at-current` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-horizontal.insert-after` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-horizontal.insert-negative-index` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-horizontal.insert-past-end` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-horizontal.remove-after` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-horizontal.remove-current` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-horizontal.remove-through-current` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-horizontal.remove-last-current` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-horizontal.remove-negative-index` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-horizontal.remove-past-end` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-horizontal.remove-all-clamped` | `mount` : 14 → `notify-and-settle` : 26 |
| `PAGE.notify-horizontal.nonpositive-count` | `mount` : 14 → `notify-noop` : 48 |
| `PAGE.notify-vertical.insert-at-current` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-vertical.insert-after` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-vertical.insert-negative-index` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-vertical.insert-past-end` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-vertical.remove-after` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-vertical.remove-current` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-vertical.remove-through-current` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-vertical.remove-last-current` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-vertical.remove-negative-index` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-vertical.remove-past-end` | `mount` : 14 → `notify-and-settle` : 31 |
| `PAGE.notify-vertical.remove-all-clamped` | `mount` : 14 → `notify-and-settle` : 26 |
| `PAGE.notify-vertical.nonpositive-count` | `mount` : 14 → `notify-noop` : 48 |
| `PAGE.pre-layout-horizontal.insert-before` | `notify-before-layout` : 41 → `attach-and-settle` : 20 |
| `PAGE.pre-layout-horizontal.insert-after` | `notify-before-layout` : 41 → `attach-and-settle` : 20 |
| `PAGE.pre-layout-horizontal.remove-empty` | `notify-before-layout` : 48 → `attach-and-settle` : 15 |
| `PAGE.pre-layout-vertical.insert-before` | `notify-before-layout` : 41 → `attach-and-settle` : 20 |
| `PAGE.pre-layout-vertical.insert-after` | `notify-before-layout` : 41 → `attach-and-settle` : 20 |
| `PAGE.pre-layout-vertical.remove-empty` | `notify-before-layout` : 48 → `attach-and-settle` : 15 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`, failures=0, 완료 count가 마지막 결과와 같아야 합니다. END 누락·cleanup 오류는 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR52 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. Release에서는 profile을 바꾸십시오. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
