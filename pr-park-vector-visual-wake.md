### Summary
AnimatedVectorImageVisual이 Measure/Arrange pass 안에서도 `Adaptor::RequestProcessEventsAndUpdate()`를
직접 호출해 LayoutController의 "처리 창 안에서는 스스로 idle wake를 걸지 않는다" 규칙을 우회했고,
`LottieAnimationView::SetResourceUrl()`은 같은 URL 재설정을 리로드로 처리해 매 pass visual을
재생성했습니다. 두 결함이 겹치면 wake가 ProcessEvents를 계속 재기동해 busy main loop이 됩니다.
D2: pass가 스택에 있는 동안 wake만 생략하고 매니저 post-processor 등록은 유지합니다. pre phase에
등록된 once post-processor는 같은 ProcessEvents 사이클에서 실행되고 래스터 결과는 벡터 스레드의
자체 트리거로 돌아오므로 작업은 유실되지 않습니다. LayoutFinished 절반은 게이트하지 않습니다.
D1: `SetResourceUrl()`은 같은 URL이면 no-op가 되고, 명시적 리로드는 새 `Reload()`로 분리해
`ImageView`/`AnimatedImageView`와 계약을 통일합니다 (공개 동작 변경, 저장소 내 호출처는 모두 이관).

### Changes
- `animated-vector-image-visual.cpp`: `IsLayoutPassOnStack()`이면 어댑터 wake만 생략, 콜백 등록은 유지.
- `lottie-animation-view{,-impl}.{h,cpp}`: 같은 URL 가드 + 비가상 `Reload()` 추가(ABI 가산),
  공개 문서의 잘못된 `@return` 제거.
- 호출처 이관: manual test tc-10/11/14, `lottie-animation-view-example`, `image-loading-policy-example`.
- 공개 UTC 2개(같은 URL 재설정 no-op, `Reload()`), 내부 UTC는 같은 URL 테스트를 "visual 유지"로
  뒤집고 `Reload()` 2개 + measure 중 같은 URL headline 1개 + D2 3개 추가.
- `docs/layout-structure.md`: 프레임워크 내부 wake도 처리 창 규칙을 따른다는 문단 추가.
- `samples/lottie-measure-reload-probe`: measure pass 안에서 자식을 건드리는 진단 샘플 추가
  (키 5로 no-op `SetResourceUrl()` ↔ 명시적 `Reload()` 전환).
- 빌드 및 UTC 실행은 이 작업에서 수행하지 않았습니다.

### Examples
```cpp
LottieAnimationView view = LottieAnimationView::New("walker.json");

// 같은 URL 재설정은 이제 no-op다: visual을 다시 만들지 않는다.
view.SetResourceUrl("walker.json");

// 강제 리로드는 명시적으로 요청한다. 재생은 자동으로 재개되지 않으므로 Play()를 함께 호출한다.
view.Reload();
view.Play();
```
