### Summary
- Layout 회귀를 화면에서 재현·판정하는 manual-test suite TC95–TC157(63개 TC)을 2개 커밋으로 추가합니다. 번호는 기존 TC01–TC94에 이어집니다.
- 변경은 `manual-tests/` 아래에만 있습니다. dali-ui-foundation·automated-tests·빌드 파일은 바꾸지 않고 테스트용 hook이나 accessor도 추가하지 않으며, font 같은 license 위험 자원이나 새 binary 파일도 넣지 않습니다.
- 모든 관측은 base에 이미 있는 API로 합니다: `SetMeasureCallback`/`SetArrangeCallback` probe의 producer 호출 수, `GetMeasuredSize()`, `Ui::GetImpl(view).GetArrangedBounds()`, actor current property, View·`LayoutController`의 `LayoutFinishedSignal`, `LayoutTransition::SetOnStart/SetOnFinished`와 `LayoutAnimatorContext`, `Extension::View::GetEffectiveLayoutScale()`, `Label::GetNaturalSize()`/`GetHeightForWidth()`.
- suite는 시간을 측정하지 않습니다. transition은 lifecycle event, 수동 `ProcessLayouts()`가 raw 0으로 한 번 전달하는 첫 context, 자연 완료 뒤의 끝 상태와 불변식(alpha(raw), raw 단조 증가)으로만 판정하고 timeout은 liveness 한도일 뿐입니다. TC152·TC153·TC155·TC156은 같은 workload의 31회 반복 sample 정확성만 봅니다.
- 내부 상태를 읽던 검사는 공개 API의 결과로 바꾸고, 대응하는 공개 결과가 없는 행만 지웠습니다(cache/dirty flag, generation, dependency owner, controller의 root 등록·pending 상태, event 수, transition 내부 상태와 진행 중 geometry).
- 삭제한 scenario: TC124의 K20 replay 4개(별도 PR에서 고친 DEBUG 전용 assert에 걸림), 관측 계약 자체를 검사하던 구 TC155(그래서 구 TC156–158을 TC155–157로 재번호), TC143의 한글 font 고정 scenario와 TC156의 font identity step(대상이 동봉 font의 identity).
- 계약 합계는 63 TC, 647 scenario, 1,484 action, 263,351 checks이며 manifest의 profile은 `release` 하나입니다.
- 이번 재작성 뒤에는 빌드와 실행을 하지 않았습니다. 정적 검토와 `layout-validation-tools.py audit`(inventory-valid, 63 cases)만 수행했으므로 병합 전에 화면 실행으로 다시 검증해야 합니다.

### Changes
- 첫 커밋: `tc/`에 TC95–TC157의 CPP·MD 쌍(`tc-95-…`~`tc-157-…`, launcher 표시명은 `95. Layout: Core constraints` 형식, 그 아래에 TC별 한 줄 설명)과 공통 fixture·support·workload, 판정 도구 `layout-validation-tools.py`(계약·로그 검사, 시간 비교 없음), manifest, coverage·map 문서를 추가하고 README에서 링크합니다.
- 둘째 커밋: measure mode와 STANDALONE 유도식 계약에 맞춰 TC97/105/106/119/120/143/145의 행을 다시 유도하고, 29개 TC에 66개 scenario를 추가합니다(EXACTLY slot, 두 단계 측정, start edge clamp, producer를 실행하지 않는 mode 없는 `Measure`, window 기준 STANDALONE).
- text 기대값은 실행 중 `Label::GetNaturalSize()`/`GetHeightForWidth()`로 구하거나 같은 Label의 값끼리 비교하므로 특정 font나 glyph metric에 의존하지 않습니다.
- image는 기존 `res/heartsframe-plain.png`(247×167)와 `res/dali-logo-anim-001.png`(326×171)를 쓰고, 실패 경로는 존재하지 않는 파일 경로로 만듭니다(`res/`의 image가 아닌 파일은 Lottie JSON이라 정상 load됨).
- TC152·TC153의 work step은 leaf producer 호출 수의 예산으로, TC122의 quiet 구간은 앱 timer와 `LayoutFinishedSignal`·producer 호출 수로 판정합니다.
- `main.cpp`는 TC 번호의 숫자 정렬만 추가합니다(font 환경 변수 설정 없음).
- TC125만 PRESERVE 계약을 검사하려고 deprecated `LayoutOrderPolicy` overload를 씁니다.

### Examples
- `PIN`은 별도로 검토해 고정한 manifest의 SHA-256이며 candidate 로그에서 계산하지 않습니다.

```sh
python3 manual-tests/dali-ui-foundation/layout-validation-tools.py audit
python3 manual-tests/dali-ui-foundation/layout-validation-tools.py check-log LOG --tc TC114 \
  --profile release --manifest MANIFEST --manifest-sha256 PIN
```

```cpp
// TC115 K11 app-hit: for a callback leaf a cache hit is a call that runs no producer.
const uint32_t     before   = s->probe->measures;
const MeasuredSize returned = s->probe->view.Measure(s->constraint.width, s->constraint.height);
r.Size("returned", returned, s->measured);
r.Equal("producer-unchanged", s->probe->measures, before);
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
