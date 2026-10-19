### Summary
- Layout 회귀를 화면에서 재현하고 판정하는 manual-test suite LR01–LR64(64개 TC)와 그 관측 hook을 추가합니다.
- hook은 integration-api `LayoutTestDiagnostics`로 제공되며, 모든 빌드에 컴파일되지만 `BeginCapture()` 전에는 event를 기록하지 않습니다.
- 이 header는 설치되며, enum, aggregate, 12개 함수는 v2.5.x 동안 제거하거나 형태를 바꿀 수 없는 지원 표면입니다.
- 설치 class인 `LabelImpl`에 `GetLayoutTestTextSnapshot()`이 새 export로 추가되며, 같은 약속이 적용됩니다.
- LR30과 같은 형태에서 나던 arrange cache replay의 DEBUG 전용 assert 오탐도 함께 고칩니다.
- `Deliver LayoutFinished for every completed layout pass`와 비유한 입력 거부 commit은 upstream devel에 이미 있습니다(`WindowSnapshot`을 채우는 controller 코드가 그 `mPendingCompletion`, `mReadyCompletion`을 읽습니다).
- suite의 기대 결과는 64개 TC 모두 실패 행 0이며, ScrollView·PageScrollView PR과 RecyclerView PR(2 commits)의 머지를 전제합니다(LR51, LR52–LR54 등).
- 예를 들어 LR08은 subnormal scale 거부를, LR34는 NaN·Inf transition 설정 거부를, LR51은 `RefreshViewport()`를 검사합니다.

### Changes
- `layout-test-diagnostics.h`: event capture, View·Window·Text·Transition snapshot, animator 수동 tick 함수를 추가합니다.
- hook macro는 인자를 평가하기 전에 capture 여부를 검사하며, 추가한 snapshot 멤버는 non-virtual이라 vtable과 객체 layout이 그대로입니다.
- `ViewDataImpl`, Absolute·Flex·Grid·Stack manager, `LayoutController`, transition dispatcher 등에 hook을 넣습니다.
- `LabelImpl::GetLayoutTestTextSnapshot()`과 `Text::Controller::GetNumberOfRenderGlyphPositions()`는 text model을 읽기만 합니다.
- replay DEBUG assert를 `!mArrangeCacheValid || mEffectiveScaleValid`로 완화합니다. replay 중 실행된 코드가 미방문 node를 무효화할 수 있습니다.
- `utc-Dali-ArrangeCacheHit-internal.cpp`에 replay 도중 형제 minimum width 변경, 형제 제거, scale policy reset UTC를 추가합니다.
- `manual-tests/dali-ui-foundation/tc`에 LR01–LR64의 CPP·MD 절차 쌍과 공통 fixture·support를 추가합니다.
- 판정 도구 `layout-validation-tools.py`, manifest, coverage·map 문서, 성능 plan 예시를 추가하고 README에서 coverage 문서를 링크합니다.
- 고정 font(DejaVuSans, NanumGothic)와 각 license, image resource를 `res/layout-validation`에 동봉합니다.
- `main.cpp`는 앱 전체의 `FONTCONFIG_FILE` 기본값을 동봉 font 디렉터리만 쓰는 `fonts.conf`로 정하며, 미리 설정된 값이 우선합니다.

### Examples
- 앱을 실행하기 전의 계약 검사와 저장된 로그의 재판정은 저장소 root에서 다음과 같이 실행합니다.
- `PIN`은 별도로 검토해 고정한 manifest의 SHA256이며, candidate 로그에서 계산하지 않습니다.

```sh
python3 manual-tests/dali-ui-foundation/layout-validation-tools.py audit
python3 manual-tests/dali-ui-foundation/layout-validation-tools.py self-test
python3 manual-tests/dali-ui-foundation/layout-validation-tools.py check-log LOG --tc LR01 \
  --profile diagnostic --manifest MANIFEST --manifest-sha256 PIN
```

🤖 Generated with [Claude Code](https://claude.com/claude-code)
