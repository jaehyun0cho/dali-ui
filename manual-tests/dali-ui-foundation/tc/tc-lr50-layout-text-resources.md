# LR50. Layout: Text and image resources

이 문서는 [tc-lr50-layout-text-resources.cpp](tc-lr50-layout-text-resources.cpp)의 실행 절차입니다. 기존 Layout TC 결과는 합산하지 마십시오. 전체 실행 조건은 [suite 검증 기준](../layout-validation-coverage.md)을 먼저 읽으십시오.

## 실행 및 완료 확인

1. app와 실제 resolve된 library의 build-ID, revision, build type, asset SHA-256을 기록하고 stdout을 저장하십시오. launcher에서 `LR50`를 검색하여 해당 TC에 진입하십시오.
2. `LR50.Reset run`을 한 번 실행하고 새 `run`을 기록하십시오. scenario 내 step 사이에는 Reset하지 마십시오.
3. `Run scenario`(step 하나씩 진행하려면 `Run step`)를 누른 뒤 `LR_ACTION`의 scenario/step/action_seq/required_checks를 기록하십시오. 동기 step은 즉시 완료됩니다. Window/resource step은 해당 episode의 완료까지 기다리십시오. 10초 내 완료되지 않으면 PENDING을 PASS로 처리하지 마십시오. 성능 sample step만 최대 120초를 허용합니다.
4. 각 step 완료 시 자동 출력되는 **모든** `LR_CHECK` 행을 읽으십시오. 행의 tc/pid/run/scenario/step/action_seq를 연결하여 다른 실행의 값과 혼합하지 마십시오. 같은 행을 다시 읽으면 값이 같아야 합니다.
5. 해당 step의 실제 행 수와 required_checks가 정확히 같아야 합니다. 일반 rect 허용오차는 0.001, 정수/bool/ID는 exact입니다. NaN/Inf, 누락/잘림, 중복 읽기로 채운 행 수, unexpected exception, crash, timeout, cleanup error는 FAIL입니다. `passed`만 믿지 말고 actual/expected/tolerance를 재계산하십시오.
6. 모든 step 후 `Next scenario`로 이동하고 모든 scenario를 실행하십시오. 마지막 `LR_RESULT`의 completed_scenarios=required_scenarios, failures=0 및 전체 행 증거가 필요합니다. 그 전의 `FAILING`/`INCOMPLETE`는 종결 verdict가 아니므로 실행을 중단하지 마십시오. 완료 후 `< Back`으로 나가 `LR_CASE_END`의 `completed_scenarios`가 `required_scenarios`와 같은지 확인하십시오. 실패 후 Reset으로 실패 이력을 지우지 마십시오.

## 입력과 독립 기대값

| scenario / step | 실행 후 필수 관측값 | 검사 수 |
|---|---|---:|
| text-model / single-A | 24px DejaVu Sans, `A`, 1 line/1 glyph, glyph index 36 | 18 |
| text-model / two-lines | `A\nA`, 2 lines/3 glyphs(줄바꿈 glyph 포함) | 18 |
| text-model / font-size | 36px로 변경, 2 lines/3 glyphs | 18 |
| text-model / return-A | 24px 단일 A로 복귀, 1 line/1 glyph | 18 |
| image-37 / run | 원본 37×19, 너비 고정 시 74×38, 높이 고정 시 111×57, 양축 고정 80×30 | 11 |
| image-73 / run | 원본 73×41, 너비 고정 시 146×82, 높이 고정 시 219×123, 양축 고정 80×30 | 11 |
| image-async / load-37 → replace-73 → return-37 | 각 ResourceReady 1회, READY, 실제 Window target=(0,0,37,19)→(0,0,73,41)→(0,0,37,19), scene 연결 | 각 12 |

먼저 `FONTCONFIG_FILE`을 `res/layout-validation/fonts.conf`의 절대 경로로 설정하고 `fc-match -f '%{file}\n' 'DejaVu Sans'`가 동봉 TTF인지 확인하십시오. asset SHA-256 검사도 필수입니다. system font로 대체한 결과를 허용하지 마십시오. font model은 관측 hook으로 완료된 실제 glyph/line을 읽습니다. `text.target`은 (0,0,100,120), control=100×120입니다. A advance는 `1401/2048*fontSize`, ascender=`1901/2048*fontSize`, descender=`-483/2048*fontSize`이며 hinting 차이는 각 1px 이내여야 합니다. 좌표 허용오차 0.001과 font metric의 1px 오차를 혼용하지 마십시오. baseline은 finite이며 0<baseline<120입니다. 다른 font/metric 값에 맞춰 expected를 갱신하지 마십시오.

비동기 image TC는 URL 변경 전에 action별 ResourceReady count와 child/Window snapshot 상태를 초기화합니다. 요청 이후 child LayoutFinished를 받은 동일 Window fence만 유효한 episode로 기록하고, 다음 fence와 child 관측을 혼합하지 않습니다. ResourceReady와 유효 episode의 도착 순서는 어느 쪽이 먼저여도 허용합니다. 둘 다 관측한 뒤 일회성 100ms event에서 최신 완료 snapshot(image.async.target)과 그 시점의 실제 Actor target(image.current.target)을 각각 새 이미지의 독립 수치와 비교합니다. 대기 중 새 자연 Window fence가 오면 최신 완료 snapshot을 반영하고, 결과를 확정한 뒤에는 overwrite하지 않습니다. image.request.window-observed=true, ready.count=1, status=READY, scene 연결도 필수입니다. 각 action은 12행입니다. ResourceReady 이후 새 fence가 반드시 온다고 가정하지 않으며 TC가 InvalidateMeasure/Measure/Arrange를 추가로 호출하지 않습니다. 10초 내 양쪽 완료를 확보하지 못하거나 frozen/current 값이 새 expected와 다르면 실패 원인을 기록하고 PASS하지 마십시오. 자연 크기 getter는 image visual 생성을 수행할 수 있으므로 이 TC의 수치 조회를 비변이 observer 검증으로 해석하지 마십시오.

## Fixed text의 실제 완료 관측

text-model의 fixed100×120 Label은 SetText/SetFontSize 뒤 새 LayoutFinished를 요구하지 않습니다. LabelImpl::InvalidateTextMeasure는 WRAP 축이 있을 때만 Measure를 무효화하며 fixed text 내용은 core Relayout 경로로 갱신될 수 있습니다. TC는 setter보다 먼저 action을 arm하고 DevelActor::OnRelayoutSignal에서 fresh count와 준비된 TextSnapshot, Actor rect를 복사합니다. 이 공개 signal은 core ActorSizer::SetNegotiatedSize가 LabelImpl::OnRelayout을 호출한 뒤 emit합니다.

첫 mount와 fixed-korean-font는 자연 Window fence도 함께 기다립니다. 이후 text-model step은 새 core Relayout 완료만 요구합니다. model/target을 고정한 뒤 일회성 1ms event에서 그 복사본을 검사하며 font 파일 조회도 이 시점에 수행합니다. timer는 완료를 만들어내는 방법이 아니며 fresh signal과 ready model이 없으면 검사 자체를 시작하지 않습니다. text.relayout.after-action=true와 해당 action의 새 line/glyph/font advance/ascender/descender가 모두 필요하므로 이전 model을 단순 지연 후 승인하지 않습니다. text-model은각 18행, fixed-korean-font는 10행입니다. 강제 InvalidateMeasure/Arrange 또는 text 렌더링 모드 변경은 하지 않습니다.

## 종료와 증거

`< Back`으로 나간 뒤 다시 진입하여 첫 step을 재실행하십시오. 이전 timer/callback/fixture/추가 Window가 남아 있으면 FAIL입니다. 결과에는 scenario/input/step, 실제값과 기대값, 최초 실패 행, 전체 stdout, artifact fingerprint를 첨부하십시오. `python3 layout-validation-tools.py check-log LOG --tc LR50`은 저장된 행의 누락과 수치를 다시 검사하는 보조 수단입니다. 이 도구는 앱을 실행하거나 서버의 UI 자동 조작을 대체하지 않습니다.

## 추가 intrinsic·고정 한글 font scenario

- `text-intrinsic/run`은 14행입니다. 24px A의 WRAP natural width=1401/2048*24±1,height29±1입니다. CHARACTER wrap의 A6개를 MATCH_PARENT width constraint25에 측정하면 width0,height174±6(6줄), constraint40이면 width0,height87±3(3줄)입니다. requested width25는 width25/height174±6입니다. text를 A로 바꾸면 25×(29±1), WRAP+font36이면 width=1401/2048*36±2(glyph bearing·정수 반올림이 크기에 비례해 커지므로 폭만 ±2),height43±1, empty text는 0×0입니다. 서로 다른 font hinting의 누적 허용오차는 한 줄당 1px이며 줄 수 차이를 허용한다는 뜻이 아닙니다.
- `fixed-korean-font/run`은 10행입니다. 동봉 NanumGothic의 `가`(U+AC00)는 cmap glyph1086, advance=940/1000*24±1입니다. 실제 model ready, glyph count1, Window target(0,0,100,60)을 확인하십시오. `fc-match NanumGothic`의 실제 파일도 기록하십시오.

`image-failure-recovery/run`은 9행입니다. invalid bytes resource에서 fixed Measure80×30, statusFAILED, Arrange(3,5,80,30), 유효한 73×41 이미지로 교체하고 WRAP으로 변경한 Measure73×41을 확인하십시오.

FontClient::GetDescription으로 실제 glyph font ID의 파일 경로를 조회하여 canonical path가 동봉 TTF 경로와 같은지도 검사합니다. 이 조회는 timing/capture 구간 밖에서 수행하며 실제 로드 경로의 파일 SHA-256도 assets.json과 일치해야 합니다.

## 단계 구성

- `text-intrinsic`은 `natural`(300×300 stage: A 24px 폭 1401/2048×24±1, 높이 29±1) → `narrow`(AAAAAA, CHARACTER wrap, MATCH_PARENT, stage 폭 25: measured 폭 0, 높이 174±6, 렌더 (0,0,25,174)±6) → `wide`(stage 폭 40: 87±3) → `fixed-width`(요청 폭 25, stage 300) → `changed-text`(A: 25×29) → `font-size`(WRAP, 36px: 폭 1401/2048×36±2, 높이 43±1) → `empty`(0×0)의 7 step이며 각 step은 measured 2행 + 렌더 4행입니다.
- `image-37`/`image-73`은 `wrap`(natural 2행 + measured + 렌더) → `fixed-width` → `fixed-height` → `both-fixed`(+status)의 4 step이고, `image-failure-recovery`는 `invalid`(80×30, margin (3,0,5,0): 렌더 (3,5,80,30), status FAILED) → `recovered`(73×41)입니다.

## 고정 실행 계약과 종료 증거

각 step 실행(`Run step`/`Run scenario`/`Run all`) 후 같은 action의 `LR_READY`를 stdout에서 수동적으로 기다리십시오. timeout 내 도착하지 않으면 FAIL이며, 이 완료 통지만으로 PASS를 판정하지 마십시오. 완료 뒤에 결과를 읽으십시오.

아래 목록은 검토된 manifest의 정확한 ID와 필수 행 수입니다. 설명의 축약 이름보다 이 목록을 우선하십시오. 각 step의 전체 stdout 행은 완료 시 자동 출력되므로 그대로 보존하십시오. stdout와 stderr를 함께 수집하십시오.

Profile `diagnostic / release`: 7 scenario, 25 action, 227 checks. 이 표에 없는 scenario는 컴파일된 계약에 선언되지 않으며 실행·판정 대상이 아닙니다.

| Scenario ID | 순서대로 실행할 action ID : required_checks |
|---|---|
| `text-model` | `single-A` : 18 → `two-lines` : 18 → `font-size` : 18 → `return-A` : 18 |
| `text-intrinsic` | `natural` : 6 → `narrow` : 6 → `wide` : 6 → `fixed-width` : 6 → `changed-text` : 6 → `font-size` : 6 → `empty` : 6 |
| `fixed-korean-font` | `run` : 10 |
| `image-37` | `wrap` : 8 → `fixed-width` : 6 → `fixed-height` : 6 → `both-fixed` : 7 |
| `image-73` | `wrap` : 8 → `fixed-width` : 6 → `fixed-height` : 6 → `both-fixed` : 7 |
| `image-async` | `load-37` : 12 → `replace-73` : 12 → `return-37` : 12 |
| `image-failure-recovery` | `invalid` : 7 → `recovered` : 6 |

마지막 결과 수집 후 `< Back`으로 나가 같은 `(tc,pid,run)`의 `LR_CASE_END`를 수집하십시오. `reason=exit`, `cleanup_passed=true`이고 `completed_scenarios == required_scenarios`이며 모든 count(failures 포함)가 마지막 `LR_RESULT`와 같아야 합니다. PASS는 그 완료 조건과 failures=0이 함께 성립할 때만입니다. `layout-validation-coverage.md`에 알려진 제품 결함 행이 문서화된 TC는 failures>0으로 종결되며, `check-log`가 `observations-valid-with-failures`로 그 행을 **정확히** 나열해야 합니다. 그 밖의 실패 행이나 `invalid-evidence`는 회귀 또는 무효 증거이며 PASS가 아닙니다. END 누락·cleanup 오류도 PASS가 아닙니다.

서버가 사전 고정한 manifest와 SHA256으로 `python3 layout-validation-tools.py check-log LOG --tc LR50 --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN`을 실행하십시오. `--profile`의 `diagnostic`/`release` 두 값은 내용이 동일한 같은 계약을 가리키므로 어느 쪽을 지정해도 같은 결과입니다. `PIN`을 candidate 출력에서 즉석 생성하지 마십시오. 계약이 다르면 별도 검토가 필요합니다. `status`가 `observations-valid`면 실패 행이 없고, `observations-valid-with-failures`면 증거는 유효하며 `failing_rows`에 나열된 행만 실패했다는 뜻이고(exit 1), `invalid-evidence`면 증거 자체가 무효이므로 재실행해야 합니다(exit 2). `observer.timeout`은 layout 완료 관측 실패, `render.frame-timeout`은 새 render task 또는 입력 반영 완료 관측 실패, `render.settle-timeout`은 current geometry 반영 관측 실패로 분류하십시오. 이 과정에서 필수 관측 행이 누락되거나 실제 행 수가 계약과 달라지면 strict 검증 결과는 `invalid-evidence`(exit 2)입니다. 해당 observer 행, `protocol.required_checks`, 도구의 `error`, 원본 stdout/stderr를 함께 보존하십시오. 재실행은 새 run 증거로 별도 기록하고 첫 실패를 삭제하지 마십시오. timeout, 필수 행 누락, `LR_READY` 또는 END 누락을 **어떤 경우에도 PASS로 처리하지 마십시오**. 외부 검증이 필요한 TC의 `--external` 결과는 pending이며 exit 1입니다. 해당 MD의 외부 증거가 없으면 전체 PASS가 아닙니다.
