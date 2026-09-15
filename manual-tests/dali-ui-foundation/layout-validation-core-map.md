# Layout core 의사결정과 신규 assertion 연결표

이 문서는 LR01–LR34 및 LR58의 현재 C++ fixture와 source 의사결정을 연결합니다. 파일 존재, syntax 검사, coverage 진입만으로 runtime PASS를 뜻하지 않습니다. 전체 source branch의 완료 목록이 아니며, 표에 없는 조건을 검증했다고 판단하지 마십시오. 전체 구성·실행 profile·coverage·mutation 절차는 [suite 실행 명세](layout-validation-coverage.md)를 함께 사용하십시오.

## 읽는 방법과 실행 증거

- Source 기호는 아래 경로와 함수명을 함께 식별합니다. 줄 번호는 변경 시 쉽게 어긋나므로 식별자로 사용하지 않습니다. 조건식이나 함수가 바뀌면 fixture 입력·oracle·check도 함께 검토하십시오.
- 별도 action을 적지 않은 Single scenario의 step ID는 run입니다. 순차 scenario에는 실제 step ID를 적었습니다. Apply next step으로 실행하고 Read result로 전체 결과를 수집하십시오.
- R은 직접 Measure 반환·GetMeasuredSize·Arrange 반환·Actor event-side rect, W는 필수 View들의 callback snapshot을 동일 Window 완료 fence에서 고정한 값, D는 diagnostics ON의 제한된 POD snapshot/등록 node trace, E는 별도 외부 관측입니다. 직접 Arrange 결과를 render 완료나 Window 완료의 증거로 해석하지 마십시오.
- Size("p")는 p.width/p.height, Rect("p")는 p.x/p.y/p.width/p.height 네 check입니다. Id("a.", i)는 점을 하나 더 붙이므로 실제 ID는 a..0입니다. 아래 ..{i}는 이 실제 형식을 표시합니다. float를 std::to_string으로 붙이는 scenario는 예를 들어 C09.max-wins.20.000000입니다.
- 표의 check는 대표 연결 ID입니다. 완료 조건은 해당 build의 LR_CASE_CONTRACT에 있는 모든 scenario/step, 필수 assertion ID, required_checks와 실제 행 수의 정확일치입니다. 대표 ID 몇 개나 다른 assertion의 중복으로 빠진 검사를 대체하지 마십시오.
- 결과는 (tc, pid, run, scenario, action_seq, row)와 source hash, app/library build-ID, 실제 로딩 경로, profile, asset hash, 계약 로그를 함께 보존하십시오. 같은 행의 재출력은 추가 검사가 아닙니다.
- R/W expected는 입력과 합·비율·좌표식으로 계산합니다. TC가 지정한 절대 허용오차를 사용하며, 큰 좌표에서 오차를 확대하는 전역 relative tolerance로 대체하지 마십시오. NaN/Inf, overflow, observer 미준비, timeout, 검사 누락은 PASS가 아닙니다.
- D의 producer count는 synthetic producer 또는 지정 event count입니다. 전체 allocation/private cache access 횟수로 확대하지 마십시오. TLS dependency owner는 producer callback 안에서만 owner 증거로 읽으며 scope 밖 0은 정상 복원입니다. union의 scale key는 cache-valid 상태에서만 읽고 invalid 상태의 propagation generation과 혼합하지 마십시오.

## Source 기호

| 기호 | Source와 함수/계약 |
|---|---|
| V | [view-data-impl.cpp](../../dali-ui-foundation/internal/views/view/view-data-impl.cpp): ViewDataImpl의 Measure, MeasureDefault, ArrangeImpl, ArrangeDefault, ApplyConstraints, cache/invalidation/child 관리 |
| API | [view.h](../../dali-ui-foundation/public-api/views/view.h), [view.cpp](../../dali-ui-foundation/public-api/views/view.cpp), [view-impl.cpp](../../dali-ui-foundation/public-api/views/view-impl.cpp): setter·callback·manager·standalone·order 계약과 전달 경로 |
| P | [Stack params](../../dali-ui-foundation/public-api/layouts/stack-layout-params.cpp), [Flex params](../../dali-ui-foundation/public-api/layouts/flex-layout-params.cpp), [Grid params](../../dali-ui-foundation/public-api/layouts/grid-layout-params.cpp), [Absolute params](../../dali-ui-foundation/public-api/layouts/absolute-layout-params.cpp), V의 SetLayoutParams/GetLayoutParams |
| S | [stack-layout-manager.cpp](../../dali-ui-foundation/public-api/layouts/stack-layout-manager.cpp): MeasureStackNonWeightChildren, MeasureStackWeightChildren, StackLayoutManager::Measure/Arrange |
| F | [flex-layout-manager.cpp](../../dali-ui-foundation/public-api/layouts/flex-layout-manager.cpp): BuildFlexLinesForArrange, ApplyFlexGrowShrink, GetFlexJustifyOffsets, ArrangeOneFlexLine, FlexLayoutManager::Measure/Arrange |
| G | [grid-layout-manager.cpp](../../dali-ui-foundation/public-api/layouts/grid-layout-manager.cpp): MeasureGridChildrenAndFillAuto, ApplyGridDefinitions, ComputeGridPositions, ArrangeGridChildrenToCells |
| A | [absolute-layout-manager.cpp](../../dali-ui-foundation/public-api/layouts/absolute-layout-manager.cpp): GetChildBounds, GetChildFlags, AbsoluteLayoutManager::Measure/Arrange |
| ST | [standalone-bounds-utils.h](../../dali-ui-foundation/internal/layouts/standalone-bounds-utils.h): SnapshotStandaloneSlotInputs, ResolveStandaloneExtent, DeriveStandaloneRootBounds; V의 MeasureStandaloneChildren/ArrangeStandaloneChild/ArrangeStandaloneChildren |
| SC | [ui-scale-manager-impl.cpp](../../dali-ui-foundation/internal/ui-scale-manager-impl.cpp), [UiScaleManager public API](../../dali-ui-foundation/public-api/configuration/ui-scale-manager.cpp); V의 GetEffectiveScale와 scale cache/property 동기화 |
| DEP | [layout-dependency-scope.cpp](../../dali-ui-foundation/internal/layouts/layout-dependency-scope.cpp), [layout-invalidation-generation.cpp](../../dali-ui-foundation/internal/layouts/layout-invalidation-generation.cpp); V의 ancestor invalidation/owner stop |
| CT | [layout-controller.cpp](../../dali-ui-foundation/public-api/layouts/layout-controller.cpp): ProcessLayouts, Process, EmitPendingViewLayoutFinishedSignals, Get, Remove, idle wake/완료 queue |
| TXT | [label-impl.cpp](../../dali-ui-foundation/integration-api/label-impl.cpp)의 준비된 text model 진단 reader와 [layout-types.h](../../dali-ui-foundation/public-api/layouts/layout-types.h)의 FlexAlign::BASELINE 계약 |
| OBS | [layout-test-diagnostics.h](../../dali-ui-foundation/integration-api/layout-test-diagnostics.h), [공통 fixture](tc/layout-validation-fixtures.h), [공통 support](tc/layout-validation-support.h) |

## 기본 constraint, 값, dispatch, setter

| Source 의사결정 | TC / scenario, action | 입력 경계와 독립 oracle | 실제 check ID / 관측 |
|---|---|---|---|
| V MeasureDefault 및 S/F/G/A Measure: 기여 child 없는 WRAP와 padding | LR01 C01.empty..{kind}, kind=0..4 | empty=0×0; padding start/end=3/5,top/bottom=7/11 →8×18 | empty.*, padded.* / R |
| V Measure/ApplyConstraints: 고정 요청, min/max clamp | LR01 C02.fixed..{kind}..., C07.clamp..{kind}... | 요청0/1/37.5/120; min40,max100에39/40/41/99/100/101 →clamp 결과 | measured.*, arranged.*, clamp.* / R |
| V Measure: normalized constraint와 min=max | LR01 C08.equal-bounds..{kind} | min=max60×30; incoming1×1과400×400 모두60×30 | small-budget.*, large-budget.* / R |
| V callback의 producer constraint clamp | LR01 C03.intrinsic | intrinsic37×19; 작은 constraint20×10; 큰 constraint로 복귀37×19 | free.*, limited.*, restored.* / R |
| API/V requested/min/max setter의 유한값·음수 거부 | LR02 C09.reject..{0..3} | -3,NaN,+Inf,-Inf를 요청/최소/최대 양축에 설정; 이전37×19,min3×2,max100×80 유지 | requested-width, requested-height, minimum-*, maximum-*, preserved-measure.* / R |
| API max-wins; V ApplyConstraints의 clamp 순서 | LR02 C09.max-wins.{20,70,150}.000000 | min100/max40,min90/max30; 요청이 최소 아래/사이/위여도40×30 | maximum-wins.* / R |
| requested sentinel canonicalization | LR02 C09.sentinel.* | WRAP_CONTENT와 MATCH_PARENT 주변±0.0005 →정확한 sentinel | canonical-width, canonical-height / R, tolerance=0 |
| P SetFlexGrow/SetFlexShrink: 음수 factor | LR02 R03.negative-flex-factors | grow=-1,shrink=-2 →0/0 | grow, shrink / R, tolerance=0 |
| V ArrangeImpl: custom 반환 rect 안전성/복구 | LR02 R03.returned-rect..{0..2} | 음수width,NaN x,+Inf height. Debug예외/Release전체inputfallback;Actor의유효provisional입력7,9,53,29유지;후속11,13,61,31복구 | invalid-result-not-published, safe-actor.*, recovered.* / R. 한 종류의 예외 정책 전체를 강제하는 검사는 아닙니다. |
| V ArrangeDefault: parent 이동과 child local 좌표 분리 | LR03 C10.C12.local-coordinate | parent70,90→100,120; child는 padding+요청+margin=16,26,37,19 유지 | parent.*, child.*, child-after-parent-move.* / R |
| V MeasureDefault: child far edge의 max | LR03 C04.wrap-bounds | 겹친 child extent70×35→80×35 | wrap.*, changed.* / R |
| V MeasureDefault: WRAP에 MATCH가 기여하는 최소값, 최종 MATCH budget | LR03 C05.C11.match-budget, C06.wrap-match | MATCH 최소기여0×0/최종slot5,13,86,28; childmin30×20→10×20 | minimum-contribution.*, match-slot.*, wrap-minimum.*, reduced-minimum.* / R |
| V MeasureDefault/ArrangeDefault: padding 후 음수 잔여 clamp | LR03 C11.exhausted-padding | content budget가 padding보다 작을 때0 extent | zero-measure.*, zero-content.* / R |
| P/V SetLayoutParams/GetLayoutParams: 값 복사/명시 재할당 | LR04 C16.stack-copy, C16.flex-copy, C16.grid-copy, C16.absolute-copy | 두 View에 같은 params 설정 후 원본/읽은 복사본 변경; 재설정만 반영. Flex17/2/3,Grid row2,col3,span4/5,Absolute1,2,30,40 | read-a, read-b, a-original-copy, b-original-copy, snapshot-is-copy, explicit-reassign, basis, grow, shrink, align, snapshot-independent, row, column, row-span, column-span, alignment, bounds.*, flags / R |
| V Measure: callback > attached manager > default | LR05 C17.callback-manager-default | default max30×15→한 번 manager attach 후 callback7×9→callback제거 후 Stack20+7+30=57×15 | default-before-attach.*, callback-priority.*, manager-after-remove.* / R. null attach나 detach를 사용하지 않습니다. |
| V AttachLayoutManager; S/F/G/A public container equivalence | LR05 C18.attached-manager..{1..4} | container와 같은 manager를 붙인 View 각각 child20×10; Flexcross100,Grid200×100,나머지20×10 | container.*, attached.* / R. actual끼리만 비교하지 않고 각각 숫자와 비교합니다. |
| V tracked setters의 changed/no-op 값 비교와 invalidation | LR06 K05.tracked..{0..11} | requested W/H,padding,margin,min/max양축,Stack/Flex/Grid/Absolute params 변경→producer+1; 동일값→+0 | changed-producer, same-producer, measured-width / R. index→setter 순서는 C++ changes 배열입니다. |
| manager child participation과 visibility | LR06 C19.hidden-participates | A20,gap7,B30; A visible=false 뒤에도 B.x27 | visible-b.*, hidden-b.* / R |
| V Measure normalized key FloatEqual | LR07 C20.measure-tolerance | key128→128.0005 hit,128.002 miss; min200에서 raw120/130은 같은normalized200 | below-epsilon-hit, above-epsilon-miss, normalized-hit, normalized-producer-input / R |
| V requested setter 값 비교 및 P weight mode 변화 | LR07 C20.pixel-setter, C20.exact-params | setter0.0005무시/0.002반영; weight0→0.0005는 nonweight→weight로 바뀜 | below-epsilon, above-epsilon, mode-change-producer, params-present, exact-stored-weight / R |
| V CanServeArrangeFromCache: exact rect key | LR07 C20.exact-arrange | Measure 후x128→128.0005는 Arrange miss,같은 rect 반복 hit | changed-producer, exact-position, unchanged-producer / R |
| V CanServeArrangeFromCache: 유효 Measure 전제 | LR07 C20.unmeasured-arrange | Measure 전 Arrange 두 번은 producer1→2; 유효 Measure 후3→3 | unmeasured.first-producer, unmeasured.repeat-producer, first-valid-measure.*, measured.first-producer, measured.repeat-cache / R |

## Scale와 direction

| Source 의사결정 | TC / scenario, action | 입력 경계와 독립 oracle | 실제 check ID / 관측 |
|---|---|---|---|
| SC/V effective scale 정책과 global scalable gate | LR08 C13.scale.*, C14.K12.master-switch | scale1/1.25/1.5/2; DISABLED parent200×100,INHERIT child20×10,ENABLED child30s×15s; master ON/OFF/ON은74×38/37×19/74×38 | parent-disabled.*, child-inherit.*, child-enabled.*, enabled.*, disabled.*, restored.* / R |
| SC invalid scale setter 보존 | LR08 R03.scale-finite | NaN,-1,+Inf에서 이전scale1 유지 | nan-rejected, negative-rejected, infinity-rejected / R |
| V Measure scale key exact 비교 및 invalidation | LR08 C20.scale-exact-key | sub-epsilon scale 변화 후 producer 재계산/key/effective 일치; 같은scale 반복 hit | producer-recomputed, key-valid, exact-key, exact-effective, same-scale-hit / R+D. setter가 cache를 먼저 무효화하므로 방어용 key 비교 단독 branch의 도달 증거와 구분하십시오. |
| V Measure: mEffectiveScaleActorSynced=false일 때 복구 | LR08 C13.actor-scale-repair | observer가 준 property index에 외부값 설정; 다음Measure는 producer hit이어도 actor scale 복구 | property-index, sync-invalidated, actor-scale, sync-restored, producer-hit / R+D |
| V ApplyLayoutDirection와 replay direction key | LR09 C15.LTR-RTL-LTR | parent200,A20,B30,gap7; x(0,27)→(180,143)→(0,27), x'=W-x-w를 한 번 적용 | a..{0..2}.*, b..{0..2}.* / R |
| V explicit/inherited direction의 subtree 경계 | LR09 C15.explicit-child | RTL root300,explicitLTR child100은x200,그 안 leaf는x0 | child-mirrored-by-parent.*, leaf-explicit-ltr.* / R |

## Stack와 Flex

| Source 의사결정 | TC / scenario, action | 입력 경계와 독립 oracle | 실제 check ID / 관측 |
|---|---|---|---|
| S nonweight/weight pass, spacing 차감, 축 분기 | LR10 S01.horizontal, S01.vertical | main300,fixed60,gap10×2,remaining220,weight1:2; slots60/220÷3/440÷3; 마지막 시작80+220÷3,끝300 | fixed.*, weight1.*, weight2.* / R |
| S Arrange cross START/CENTER/END/FILL | LR10 S04.cross..{0..3} | parent100×60,child20×10 →y0/25/50/0,fill height60 | aligned.* / R |
| S margin accumulation과 main overflow | LR10 S05.margin-gap-overflow | margin2/3/4/5,width30의slot35; gap7 후B.x42,parentwidth40밖에도 요청 보존 | margin.*, overflow.* / R |
| S weighted Measure clamp와 Arrange allocation 분리 | LR11 S08.minimum, S08.maximum | weight1/1,parent200에서allocation각100; measured min120 또는max60은 별도 결과 | measured.*, allocated-a.*, allocated-b.* / R |
| S WRAP의 weight intrinsic 기여와 parent min | LR11 S06.wrap-weight | 20+gap10+30=60; parentmin100이면100 | wrap.*, minimum.* / R |
| S unweighted MATCH와 weighted MATCH 선택 | LR11 S07.match-overflow | fixed20 뒤 unweighted MATCH100은overflow; weight1이면remaining80 | unweighted.*, weighted.* / R |
| F ApplyFlexGrowShrink: positive/negative free space | LR12 F06.grow, F07.shrink | grow basis20/40,free90,1:2→50/100; shrink basis100/200,parent250,weights100:400→90/160 | a.*, b.* / R |
| F explicit basis와 measured min/max, allocation 분리 | LR12 F10.maximum, F11.minimum | measured60/80을 별도 확인하고 growallocation100 또는shrinkallocation50 확인 | measured-a.*, allocated-a.*, allocated-b.* / R. 최종 min/max 재분배 전체를 요구하는 fixture로 해석하지 마십시오. |
| F basis0 대 intrinsic, MATCH grow 선택 | LR12 F08.zero-basis, F09.match-grow | intrinsic40/30이어도 explicitbasis0; grow1child100. fixed50뒤MATCHgrow150 | zero.*, growing.*, match.* / R |
| F shrink 후 개별0 clamp, 재분배 없음 | LR12 F15.shrink-clamp | basis1/100,shrink100/1,parent10; deficit91을 같은100가중치로 차감 →max(0,1-45.5)=0,100-45.5=54.5 | clamped.*, remaining.* / R. 이 clamp 정책에서 합계=parent10을 강제하지 않습니다. |
| F BuildFlexLinesForArrange: exact fit/초과/wrap reverse | LR13 F02.wrap.* | main100/99,widths40/60/30;100은 첫두child같은line,99는첫child뒤break; reverse는 line cross순서 반전 | a.*, b.*, c.* / R |
| F Arrange alignContent START/CENTER/END/STRETCH | LR13 F13.content..{0..3} | lineheight10/20,parentcross100,free70;첫y0/35/70/0,stretch둘째y45 | line0.*, line1.* / R |
| F GetFlexJustifyOffsets: 여섯 justify | LR14 F04.justify..{0..5} | parent140,width20/40,free80; START(0,20),END(80,100),CENTER(40,60),BETWEEN(0,100),AROUND(20,80),EVENLY(80/3,20+160/3) | a.*, b.* / R |
| F GetFlexJustifyOffsets: freeSpace<=0 | LR14 F12.overflow..{3..5} | SPACE_BETWEEN/AROUND/EVENLY에서 overflow로 음수 분산gap이 생기지 않음 | a.*, b.* / R; 구체 main/width는 해당 CPP/MD 입력을 사용합니다. |
| F ArrangeOneFlexLine: alignSelf AUTO/override | LR14 F05.align-self..{0..4} | parentEND,child20×10,cross60; AUTO/START/END/CENTER/STRETCH y50/0/50/25/0,stretchheight60 | self.*, inherited.* / R |
| F IsMainAxisHorizontal/IsMainAxisReversed | LR14 F01.direction..{0..3} | ROW/ROW_REVERSE/COLUMN/COLUMN_REVERSE;main100,child20/30;reverse80/50 | a.*, b.* / R |
| F ArrangeOneFlexLine BASELINE와 public alignment 계약 | LR15 F14.align-items, F14.align-self | 고정font Hx,18/36pixel;실제 localbaseline은 달라야 하고child.y+baseline은 같아야 함 | text-ready, font-resolved, same-glyph, larger-advance, a.line-spacing, b.line-spacing, a.local-baseline, b.local-baseline, different-local-baselines, shared-rendered-baseline, first-x, second-x / W+D |

LR15는 baseline fallback을 expected로 승인하지 않습니다. 준비된 line/glyph/render offset의 const bounded observer로 읽고, 관측 중 재계산·Resize·elision 변경을 호출하지 않습니다. 실제 baseline 차이를 보정하지 않는 제품 구현은 shared-rendered-baseline에서 실패해야 합니다. 원본부터 실패한 assertion을 mutation kill로 세지 마십시오.

## Grid와 Absolute

| Source 의사결정 | TC / scenario, action | 입력 경계와 독립 oracle | 실제 check ID / 관측 |
|---|---|---|---|
| G ApplyGridDefinitions/ComputeGridPositions: ABSOLUTE와 spacing | LR16 G01.absolute | rows20/30,gap5;columns40/50,gap7;각prefix에spacing포함 | a.*, b.*, c.* / R |
| G STAR 잔여 공간과 weight | LR16 G02.star | parent330,absolute50,spacing합20,remaining260을1:2분배 | a.*, b.*, c.* / R |
| G MeasureGridChildrenAndFillAuto: AUTO maximum | LR16 G03.auto | AUTO40,gap10,parent200 →STAR150 | auto.*, star.* / R |
| G STAR weight0와 음수 잔여 clamp | LR16 G08.zero-star-overflow | parent40,absolute60,gap7 →STARwidth0,start67 | absolute.*, zero.* / R |
| G implicit cell/명시 definition 선택 | LR16 G04.implicit | parent100×60에서fullcell;explicitcolumn30,row20으로변경 | implicit.*, explicit.* / R |
| G span extent와 내부 spacing | LR17 G05.span-spacing | span2width30+7+50=87,height20+5+30=55;둘째spanx37,y25,127×75 | span00.*, span11.* / R |
| G multi-span AUTO deficit 배분 | LR17 G06.auto-deficit | track20/30,gap10→60;span100의deficit40을둘에20씩→40/50 | auto-a.*, auto-b.* / R |
| G ArrangeGridChildrenToCells: alignment, margin/padding | LR17 G09.align..{0..3}, G10.padding-margin | cell100×60,child20×10 →START0,0/CENTER40,25/END80,50/FILL100×60;별도padding/margin slot5,13,106,48 | aligned.*, cell.* / R |
| G row/column index clamp; P span minimum | LR18 G12.index..{0,1,UINT_MAX}, G12.span..{0,1,2,UINT_MAX} | first0,0,30,20;last/초과37,25,50,30;span0→1,span2/초과→남은유효2track87×55 | cell.*, params, normalized-span, span.* / R |
| G definition clear와 implicit fallback | LR18 G13.definition-clear | 명시definition제거 후parent100×60fullcell | defined.*, cleared.* / R |
| A explicit local bounds와 parent이동 | LR19 A01.explicit-local | parent70,90→100,120에도child11,13,37,19유지 | child.*, moved-parent.* / R |
| A explicit extent경계와 음수position | LR19 A02.extent.*, A07.wrap-negative-position | width0/1/37/250,x=-20,y7;WRAP far edge -20+100=80,-5+20=15 | explicit.*, wrap.* / R |
| A intrinsic extent sentinel과 MATCH delegate | LR19 A03.intrinsic-match | sentinel에서요청37×19;MATCH로바꾸면100×60 | intrinsic.*, match.* / R |
| A 네 proportional flag 독립 선택 | LR20 A04.flags..{0..15}, A05.all-flags | parent200×100,bounds(.25,.5,.5,.25);비율width100/height25,position=(parent-child)*p;ALL(.5,.5,.5,.5)→50,25,100,50 | flags.*, all.* / R |
| A position비율 [0,1] 바깥과RTL | LR20 A08.position.* | p=-.25/0/.5/1/1.25,parent300,child100 →x=200p;RTL=200-200p | ltr.*, rtl.* / R |
| A Measure: WRAP의 circular contribution 제외 | LR20 A06.wrap-circularity | position만비율+width40 →WRAP40×20;width비율이면해당축기여0 →0×20 | position-only.*, size-circular.* / R |

## Standalone, cache, invalidation, ownership

| Source 의사결정 | TC / scenario, action | 입력 경계와 독립 oracle | 실제 check ID / 관측 |
|---|---|---|---|
| V/S/F/G/A normal/STANDALONE contribution | LR21 S10.standalone..{0..4}, G14.mode-change | standalone은parentWRAP/spacing/index계산제외;요청7,9는임시Actor77,99와독립;mode변경포함/제외재계산 | parent-measured.*, independent.*, logical-count, ordinary.*, excluded.* / R |
| ST DeriveStandaloneRootBounds/ArrangeStandaloneChild MATCH양축 | LR21 S10.match-margin-clamp..{0..4} | parent200×100,padding20/30/40/10무시;ownmargin7/11/13/17,request5/9 →12,22,182,70;ownminW190/max195,minH20/max60 →12,22,190,60 | full-parent-minus-own-margin.*, own-minmax-after-match.* / R |
| V Measure cold/hit,height key 변화 | LR22 K01.same-key | 같은100×50producer1;height51에서producer2 | cold.*, hit.*, producer-once, height-key / R |
| V normalized min/max key | LR22 K02.normalized-key | minW120,maxH30;raw40×100/50×200모두120×30;measured120×19 | producer-once, normalized-width, normalized-height, clamped-result.* / R |
| V explicit invalidation과 producer 내부값 변화 구분 | LR22 K03.explicit-invalidation | intrinsic37→53만바꾸면37hit;InvalidateMeasure뒤53,다음hit | cached.*, unchanged-producer, invalidated.*, fresh-producer, fresh-cache / R |
| V ancestor invalidation과 clean sibling cache | LR22 K04.sibling-isolation | A20→40,B30;root70;Aproducer증가/B유지 | root.*, dirty-producer, clean-producer / R |
| V ApplySelfBoundsIfChanged/ReplayArrangeSubtreeFromCache | LR23 K08.actor-repair, K09.subtree-replay | 유효Measure/Arrange후Actor손상→hit에서rect복구;PIVOT.x=.5유지,POSITION_USES_PIVOT=false | initial.*, repaired.*, producer-once, pivot-x, placement-ignores-pivot, origin-x, a.*, b.*, a-producer, b-producer / R |
| V ArrangeImpl의 callback 반환 publication/hit | LR23 K10.returned-bounds | 유효Measure후입력과다른finite반환rect;동일입력hit는반환target복원 | returned.*, hit.*, producer / R |
| V CanServeArrangeFromCache의IF_CHANGED/ALWAYS/subtree gate | LR24 K15.if-changed, K15.always, K16.always-descendant, K16.policy-change | Measure선행;반복3회IF_CHANGED1/ALWAYS3;mixed의ALWAYS실행/clean sibling hit;policy교체3→4 | producer, bounds.*, always-runs, clean-sibling-hit, a.*, b.*, always-total, if-changed-total / R |
| DEP generation,V InvalidateMeasure dirty/cache propagation | LR25 K06.generation | 캐시root/child에서child무효화→rootdirty+cacheclear;다음outermostMeasure새generation | capture, initial-cache, propagated-dirty, cache-invalidated, new-generation, new-cache, ancestor-visited, no-overflow / D |
| V InvalidateArrange: Measurecache유지 | LR25 K07.arrange-only | Arrange만무효화;다음Arrange후measureproducer1/arrangeproducer2 | measure-preserved, arrange-invalid, dirty, settled, measure-producer, arrange-producer / R+D |
| DEP ArrangeOwnedMeasureScope,V cache-miss ancestor stop | LR26 K13.owner..{0..4} | parentMeasure120×60→Arrange200×80;MATCHchild최종slotMeasure안PODcapture;owner=root등록ID1,kindArrange1 | owned-producer-observed, owner-in-measure, owner-kind-in-measure, parent-arranging, child-measuring, owner-measure-cache-preserved, final-measure-constraint.*, final-slot.* / R+D |
| DEP ownerRAII종료와 explicit invalidation 예외 | LR26 같은scenario후반 | callback밖owner0/kind0,push/pop균형;childexplicitInvalidateMeasure는owner까지dirty전달 | owner-after-scope, owner-kind-after-scope, owner-cache-published, explicit-invalidation-reaches-owner, owned-scope, balanced-scopes, no-overflow / D |
| V InvalidateAncestorLayoutCachesForMeasureMiss: out-of-band | LR27 K14.external-measure | 최종slot200×80뒤외부70×40;parentcache둘만drop,dirty/poison없음;다음parent계산200×80복원 | before.*, external.owner-*-cache-cleared, external.no-measure-dirty, external.no-arrange-dirty, external.no-poison, external.constraint.*, restored.constraint.*, restored.* / R+D |
| ST unconsumed slot corrective Measure와 standalone boundary | LR27 K14.external-standalone-measure | intrinsic300×90,parent200×80;외부70×40뒤parentMeasurehit,parentArrange는child한번재측정7,9,200,80;반복+0 | before.standalone-slot-consumed, external.standalone-slot-unconsumed, external.parent-cache-preserved, parent-measure-hit-no-child-producer, corrected.slot-consumed, corrective-child-producer, corrected.slot.*, repeat.no-child-producer / R+D |
| V InvalidateParentArrangeCacheForOutOfBandArrange | LR27 K14.external-arrange | 외부childArrange로owner기록slot변경;다음parentArrange정상slot재생성 | external.*, restored.* / R |

normal child의 measuredSlotUnconsumed를 false로 요구하지 않습니다. bit의 소비 계약은 standalone 경로입니다. LR26 callback 안 TLS owner와 LR27 cache 상태도 서로 다른 관측 증거입니다.

## 완료, PARK, reentry, replay 변경, tree/Window lifetime

| Source 의사결정 | TC / scenario, action | 입력 경계와 독립 oracle | 실제 check ID / 관측 |
|---|---|---|---|
| CT View완료queue→Windowfence;V LayoutFinished delivery | LR28 L03.fenced-snapshots / mount, resize-child | A20×10,B30×15→Awidth40;같은episode에서A0,0,40,10/B40,0,30,15 | a.*, b.*, a-notified, b-notified, changed.*, shifted.* / W |
| API setter no-op와 directArrange 완료신호 | LR28 같은scenario / no-op-setter, direct-arrange | 같은width40설정및directArrange7,9,40,10은동기완료count불변 | a-no-synchronous-signal, b-no-synchronous-signal, no-manual-completion, manual-target.* / R. 동기비교만으로장시간신호부재를주장하지마십시오. |
| CT 처리중 재무효화 pending보존/PARK | LR29 K17.passive-park / prepare, arm-passive-observation |37×19정착후producer자체Measure무효화;수동처리후pending>0,wake예약전후보존;in-pass PARKED_REQUEST>0/WAKE_REQUEST=0;UI입력없이10초stdout관측 | settled.*, producer-entered, pending-retained, manual-wake-preserved, in-pass-request-parked, no-in-pass-wake-request, capture.no-overflow, not-saturated; LR29_PARK_ARMED/LR29_PARK producer / W+D+E |
| CT 외부event진행과 producerdisarm | LR29 같은scenario / external-event-and-stop | action입구before와Invalidate+Process후after비교;callback제거 | external-progress, disarmed, captured-before-positive, captured-after-positive, LR29_PARK_STOP / D+E. 이입력의pass는앞quietinterval에포함하지않습니다. |
| CT 자연episode완료와 idlewake퇴장 | LR29 같은scenario / recover-window-and-drain | disarm뒤AfterLayoutarm→Invalidate,수동Process없이37×19/queue0/clean;자연fence뒤에만one-shot100ms로wake/depth0확인 | recovery.geometry.*, recovery.natural-process, recovery.pending-roots, recovery.pending-completions, recovery.child-clean, recovery.park-producer-stopped, drained.* / W+D |
| V MeasurePassGuard/Arrange pass rollback | LR30 K18.measure-throw, K18.arrange-throw | 완료37×19또는1,2,37,19후producerthrow;완료record보존. Arrange Actor에는provisional7,9,53,29가보임;retry후producer3,동일retrycache에서도3 | exception-propagated, provisional-input.*, last-completed.*, retry.*, retry-producer, retry-cache.*, retry-cache-producer / R. Arrange의last-completed는ViewImpl::GetArrangedBounds()로읽습니다. |
| V Measure 같은View reentry poison guard | LR30 K18.measure-reentry | callback안같은View Measure;무한producer재진입없이정상37×19복구 | entered, bounded, recovered.* / R |
| V child snapshot/Add/Remove/RemoveAll invalidation | LR31 K19.add-remove, K20.mutate-during-producer | A제거+C추가후B.x0,C.x37;RemoveAlllogical0;Measure중B추가후후속passB.x20 | child-count, first, b.*, c.*, empty.*, empty-count, out-of-range, children, added, later-child.* / R |
| V 실제replay PropertySetSignal observer와 tracked변경 | LR31 K20.replay-property-mutation | 캐시A.x를99로손상,rootArrangehit가0복구하는observer안B.minimumWidth45설정 | observer.calls, observer.root-replaying, observer.child-replaying, observer.processing, root.cache-hit, root.producer-not-run, after.*, recovered.b.* / R+D |
| V replay중 Measure miss와 ancestorcachedrop | LR31 K20.replay-external-measure | 같은observer안B.Measure12×8;rootcache둘false,dirty/poisonfalse;다음계산B20,0,30,10 | 공통observer/cache check; after.measure-cache, after.arrange-cache, after.measure-dirty, after.arrange-dirty, after.arrange-poison, recovered.* / R+D |
| V replay중 out-of-band Arrange의 parentcachedrop | LR31 K20.replay-external-arrange | observer안B.Arrange75,20,10,5;rootMeasurecachetrue/Arrangecachefalse,dirty/poisonfalse | 공통observer/cache check; after.*, recovered.* / R+D |
| V ArrangeImpl 같은View reentry가 hit보다 먼저poison | LR31 K20.replay-reentrant-arrange | observer안root.Arrange999×999;반환lastcompleted0,0,200,60;poisontrue/flags복원/정상재계산복구 | nested.last-completed-result.*, after.arrange-poison, replay.flags-unwound, replay.no-producer-publish-block, recovered.caches, recovered.clean / R+D |
| V RaiseAbove UPDATE/PRESERVE의logicalorder | LR32 ORDER.update, ORDER.preserve | A20,B30,C40에서A.RaiseAbove(C);UPDATE B,C,A/A.x70,PRESERVE A,B,C/A.x0 | first, last, a.*, b.* / R |
| V Raise/Lower/LowerBelow의ScopedSkipChildrenUpdate | LR32 ORDER.preserve-raise, ORDER.preserve-lower, ORDER.preserve-lower-below | 실제Actor순서B,A,C / A,C,B / C,A,B;logicalA,B,C와x0/20/50유지 | logical.count, actor.count, logical.order.{0..2}, actor.order.{0..2}, preserved.a.*, preserved.b.*, preserved.c.* / R |
| V child filtering: non-ViewActor | LR32 ORDER.non-view | 장식Actor1+View2;actor3/logical2,logicalindex2empty,B.x20 | logical, actor, out-of-range, b.* / R |
| V parent변경과 old/newrootcache invalidation | LR33 L01.reparent | leftA20+gap7+B30,rightC40;B이동뒤left20×10,right40+5+30=75×20,B.x45 | old-root.*, new-root.*, old-count, new-count, moved.*, attached-to-root / R |
| V detached/off-scene 계산 | LR33 L02.detached-measure | child37×19요청7,9;root44×28→detachroot0×0,child37×19→reattach44×28 | empty.*, detached-child.*, reattached.* / R |
| CT per-Window Get/Remove/재생성 | LR34 L04.window-recreate / secondary-window, remove-and-recreate-controller | secondary220×140의child37×19,Remove/Get뒤child53×19를새fence로확인;main생존 | secondary-distinct, secondary-child.*, recreated-child.*, main-window-alive / W |
| CT Windowrootdetach와 publichandle정리 | LR34 같은scenario / detach-close | rootunparented,secondaryhandleReset,main유지 | root-unparented, window-handle-released, main-window-retained / R. publichandle해제를모든내부객체의즉시destruction증거로확대하지마십시오. |

LR31의 네 replay scenario는 실제 ARRANGE_HIT=1, ARRANGE_PRODUCER=0 trace와 callback 안 replay flag를 함께 요구합니다. producer 중 변경을 replay 검사로 대체하지 않습니다. 이 fixture는 off-scene 동기 replay이며, Window wake/PARK 전체의 증거는 LR29 및 별도 controller TC에서 확보하십시오.

## 고정 seed와 변환 불변식

| Source 의사결정 | TC / scenario, action | 입력 경계와 독립 oracle | 실제 check ID / 관측 |
|---|---|---|---|
| S nonweight prefix와 축교환 | LR58 R01.seed..{1,17,73,257,65537} | uint32 LCG s=s*1664525+1013904223;16child w=1+s%47,h=1+s%31,gap3;intrinsic(Σw+45,maxh),각prefix,verticaltranspose | intrinsic.*, horizontal..{0..15}.*, vertical..{0..15}.* / R |
| F equalgrow와reverse/RTL/translation/transpose | LR58 R02.flex-seed..{1,73,65537} |8childbasis=width,grow1,total=Σwidth+120→각width+15;reverse W-prefix-width;reverse+RTL두번반전;parent31,17이동local불변 | flex.forward..*, flex.reverse..*, flex.reverse-rtl..*, flex.translated-parent..*, flex.transposed..* / R |
| G ABSOLUTEprefix와RTL/translation/transpose | LR58 R04.grid-seed..{1,73,65537} |4rows20..39,3cols15..34,rowgap3,colgap5,12cell;widthΣcol+10,heightΣrow+9 | grid.ltr..*, grid.rtl..*, grid.translated-parent..*, grid.transposed..* / R |
| A finiteabsolute/proportional과RTL/translation/transpose | LR58 R05.absolute-seed..{1,73,65537} |12child,parent200×120;짝수absolutex=-10..39,홀수p=-.25..1.25에서x=(200-w)p;y=-5..24;transposeparent120×200 | absolute.ltr..*, absolute.translated-parent..*, absolute.rtl..*, absolute.transposed..* / R |

seed는 위의 제한된 유효 입력 영역을 재현합니다. 무작위 미실행 입력, 모든 params 조합, invalid-domain 전체, failure shrink의 성공을 이 TC의 PASS로 주장하지 마십시오. R06–R08의 축소·mutation·coverage 보조 작업은 별도 artifact와 재실행 증거가 필요합니다.

## 검토와 실행 결과를 연결하는 절차

1. 아래 CPP/MD 쌍에서 scenario의 실제 입력과 check를 읽으십시오. table 패턴을 계약 로그의 구체 ID로 펼치고 action의 모든 assertion을 검사하십시오.
2. 관심 source 함수·조건에 대해 해당 action 구간 coverage/trace를 수집하십시오. 가능하면 action 시작 전 counter reset, 완료 후 dump 경계를 두고 등록 node ID와 root/child 역할을 기록하십시오. 지원되지 않는 reset/dump를 수행했다고 적지 마십시오.
3. launcher/HUD가 우연히 밟은 branch는 제외하십시오. 진입 coverage에 더해 fixture 입력, node/action, actual observer, oracle, 실패 가능한 assertion을 함께 연결해야 합니다.
4. trace의 overflow check를 포함하여 검증하십시오. callback 밖 TLS owner나 invalid union key로 성공/실패를 만들지 마십시오.
5. positive-control mutation은 원본의 같은 assertion PASS를 먼저 확보하고, 격리 mutant에서 그 assertion이 예상 이유로 FAIL하는지 대조하십시오. compile failure, crash, timeout, 원본부터 FAIL은 kill이 아닙니다.
6. 표에 없는 source 조건은 함수·조건·입력·필요 observer·후보 assertion을 누락 목록에 기록하십시오. 기존 행의 의미를 늘려 미실행 branch를 채우거나 coverage 비율만으로 완료 처리하지 마십시오.

전역 scale의 모든 reparent 조합, manager setter의 모든 route 조합, RaiseToTop/LowerToBottom 개별 경계, 깊은 ancestor stop 조합, observer가 바꾸는 모든 tree 형태는 위 대표 fixture만으로 각각 검증되었다고 볼 수 없습니다. 별도 신규 TC의 실제 assertion 연결이 있으면 교차 참조로 추가하고, 없으면 추가 fixture가 필요한 차이로 기록하십시오. Text/resource, Scroll/Recycler, hit/clip, RenderEffect, transition, controller 추가 경로와 성능은 [suite 책임표](layout-validation-coverage.md)의 대응 LR35–57/LR59–64 및 각 MD에서 추적하십시오.

## CPP/MD 찾아가기

- LR01: [tc-lr01-layout-core-constraints.cpp](tc/tc-lr01-layout-core-constraints.cpp) · [실행 명세](tc/tc-lr01-layout-core-constraints.md)
- LR02: [tc-lr02-layout-core-invalid-inputs.cpp](tc/tc-lr02-layout-core-invalid-inputs.cpp) · [실행 명세](tc/tc-lr02-layout-core-invalid-inputs.md)
- LR03: [tc-lr03-layout-default-view.cpp](tc/tc-lr03-layout-default-view.cpp) · [실행 명세](tc/tc-lr03-layout-default-view.md)
- LR04: [tc-lr04-layout-params-values.cpp](tc/tc-lr04-layout-params-values.cpp) · [실행 명세](tc/tc-lr04-layout-params-values.md)
- LR05: [tc-lr05-layout-dispatch.cpp](tc/tc-lr05-layout-dispatch.cpp) · [실행 명세](tc/tc-lr05-layout-dispatch.md)
- LR06: [tc-lr06-layout-tracked-setters.cpp](tc/tc-lr06-layout-tracked-setters.cpp) · [실행 명세](tc/tc-lr06-layout-tracked-setters.md)
- LR07: [tc-lr07-layout-numeric-boundaries.cpp](tc/tc-lr07-layout-numeric-boundaries.cpp) · [실행 명세](tc/tc-lr07-layout-numeric-boundaries.md)
- LR08: [tc-lr08-layout-scale.cpp](tc/tc-lr08-layout-scale.cpp) · [실행 명세](tc/tc-lr08-layout-scale.md)
- LR09: [tc-lr09-layout-direction.cpp](tc/tc-lr09-layout-direction.cpp) · [실행 명세](tc/tc-lr09-layout-direction.md)
- LR10: [tc-lr10-layout-stack-allocation.cpp](tc/tc-lr10-layout-stack-allocation.cpp) · [실행 명세](tc/tc-lr10-layout-stack-allocation.md)
- LR11: [tc-lr11-layout-stack-wrap-clamp.cpp](tc/tc-lr11-layout-stack-wrap-clamp.cpp) · [실행 명세](tc/tc-lr11-layout-stack-wrap-clamp.md)
- LR12: [tc-lr12-layout-flex-allocation.cpp](tc/tc-lr12-layout-flex-allocation.cpp) · [실행 명세](tc/tc-lr12-layout-flex-allocation.md)
- LR13: [tc-lr13-layout-flex-lines.cpp](tc/tc-lr13-layout-flex-lines.cpp) · [실행 명세](tc/tc-lr13-layout-flex-lines.md)
- LR14: [tc-lr14-layout-flex-alignment.cpp](tc/tc-lr14-layout-flex-alignment.cpp) · [실행 명세](tc/tc-lr14-layout-flex-alignment.md)
- LR15: [tc-lr15-layout-flex-baseline.cpp](tc/tc-lr15-layout-flex-baseline.cpp) · [실행 명세](tc/tc-lr15-layout-flex-baseline.md)
- LR16: [tc-lr16-layout-grid-tracks.cpp](tc/tc-lr16-layout-grid-tracks.cpp) · [실행 명세](tc/tc-lr16-layout-grid-tracks.md)
- LR17: [tc-lr17-layout-grid-spans.cpp](tc/tc-lr17-layout-grid-spans.cpp) · [실행 명세](tc/tc-lr17-layout-grid-spans.md)
- LR18: [tc-lr18-layout-grid-boundaries.cpp](tc/tc-lr18-layout-grid-boundaries.cpp) · [실행 명세](tc/tc-lr18-layout-grid-boundaries.md)
- LR19: [tc-lr19-layout-absolute-bounds.cpp](tc/tc-lr19-layout-absolute-bounds.cpp) · [실행 명세](tc/tc-lr19-layout-absolute-bounds.md)
- LR20: [tc-lr20-layout-absolute-proportions.cpp](tc/tc-lr20-layout-absolute-proportions.cpp) · [실행 명세](tc/tc-lr20-layout-absolute-proportions.md)
- LR21: [tc-lr21-layout-standalone.cpp](tc/tc-lr21-layout-standalone.cpp) · [실행 명세](tc/tc-lr21-layout-standalone.md)
- LR22: [tc-lr22-layout-measure-cache.cpp](tc/tc-lr22-layout-measure-cache.cpp) · [실행 명세](tc/tc-lr22-layout-measure-cache.md)
- LR23: [tc-lr23-layout-arrange-replay.cpp](tc/tc-lr23-layout-arrange-replay.cpp) · [실행 명세](tc/tc-lr23-layout-arrange-replay.md)
- LR24: [tc-lr24-layout-arrange-policy.cpp](tc/tc-lr24-layout-arrange-policy.cpp) · [실행 명세](tc/tc-lr24-layout-arrange-policy.md)
- LR25: [tc-lr25-layout-invalidation-generation.cpp](tc/tc-lr25-layout-invalidation-generation.cpp) · [실행 명세](tc/tc-lr25-layout-invalidation-generation.md)
- LR26: [tc-lr26-layout-dependency-ownership.cpp](tc/tc-lr26-layout-dependency-ownership.cpp) · [실행 명세](tc/tc-lr26-layout-dependency-ownership.md)
- LR27: [tc-lr27-layout-out-of-band.cpp](tc/tc-lr27-layout-out-of-band.cpp) · [실행 명세](tc/tc-lr27-layout-out-of-band.md)
- LR28: [tc-lr28-layout-view-completion.cpp](tc/tc-lr28-layout-view-completion.cpp) · [실행 명세](tc/tc-lr28-layout-view-completion.md)
- LR29: [tc-lr29-layout-park.cpp](tc/tc-lr29-layout-park.cpp) · [실행 명세](tc/tc-lr29-layout-park.md)
- LR30: [tc-lr30-layout-reentry-exceptions.cpp](tc/tc-lr30-layout-reentry-exceptions.cpp) · [실행 명세](tc/tc-lr30-layout-reentry-exceptions.md)
- LR31: [tc-lr31-layout-replay-mutation.cpp](tc/tc-lr31-layout-replay-mutation.cpp) · [실행 명세](tc/tc-lr31-layout-replay-mutation.md)
- LR32: [tc-lr32-layout-tree-order.cpp](tc/tc-lr32-layout-tree-order.cpp) · [실행 명세](tc/tc-lr32-layout-tree-order.md)
- LR33: [tc-lr33-layout-reparent-offscene.cpp](tc/tc-lr33-layout-reparent-offscene.cpp) · [실행 명세](tc/tc-lr33-layout-reparent-offscene.md)
- LR34: [tc-lr34-layout-window-cleanup.cpp](tc/tc-lr34-layout-window-cleanup.cpp) · [실행 명세](tc/tc-lr34-layout-window-cleanup.md)
- LR58: [tc-lr58-layout-seeds.cpp](tc/tc-lr58-layout-seeds.cpp) · [실행 명세](tc/tc-lr58-layout-seeds.md)
