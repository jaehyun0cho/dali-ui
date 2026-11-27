**Label text가 effective scale ≠ 1에서 현재 devel과 다르게 그려질 것으로 보입니다. 허용 가능한 변경인지 검토 부탁드립니다.**

검토 기준은 이 PR의 `2b5534c5`와, 연계된 dali-core 변경(Change-Id `I5925370335e498290c2dc687d7752924e82b3a1a`)입니다. 비교 대상은 현재 devel(`02a32508`)입니다. 빌드나 실행 없이 코드만 읽고 확인한 내용이므로, 아래 내용이 맞는지 확인 부탁드립니다.

### 1. 무엇이 달라지는지

Label은 text visual의 `SIZE`와 `OFFSET`을 이미 scale이 반영된 pixel 값(ABSOLUTE)으로 계산해서 넘깁니다. 그래서 shader가 scale을 한 번 더 곱하지 않도록 `SetTransformMapUsageForFittingMode(true)`로 `visualTransformUseEffectiveScale`을 0으로 내려 줍니다.

- **devel**: shader가 0을 받습니다. `effectiveScale`이 1이 되어, 넘겨 준 pixel 값 그대로 그립니다.
- **이 PR 적용 후**: shader가 0을 받지 못하고 기본값 1.0을 씁니다. `effectiveScale`이 `viewEffectiveScale`이 되어, `SIZE`와 `OFFSET`에 scale이 한 번 더 곱해집니다.

```glsl
// text-visual-shader.vert
highp float effectiveScale = mix(1.0, viewEffectiveScale, visualTransformUseEffectiveScale);
vec2 visualSize = mix(size * uSize.xy, size * effectiveScale, offsetSizeMode.zw ) + extraSize * effectiveScale;
vec2 visualOffset = mix(offset * uSize.xy, offset * effectiveScale, offsetSizeMode.xy);
```

예를 들어 Label이 400×100 px이고 text visual의 `SIZE`가 (400, 60), `OFFSET`이 (0, 20)이면, text가 그려지는 범위는 다음과 같습니다.

| effective scale | devel | 이 PR 적용 후 |
|---|---|---|
| 1.0 | (0, 20) ~ (400, 80) | (0, 20) ~ (400, 80) |
| 2.0 | (0, 20) ~ (400, 80) | (0, 40) ~ (800, 160) |
| 0.5 | (0, 20) ~ (400, 80) | (0, 10) ~ (200, 40) |

- scale 2.0에서는 글자가 가로·세로 2배로 늘어나 그려지고, Label 영역 밖으로 넘칩니다.
- 영향을 받는 것: Label text(sync / async), 높이가 큰 text의 tile renderer, marquee, Label 안의 inline image
- 달라지지 않는 것: effective scale이 1.0일 때의 결과, Label의 measure / layout 결과, InputField와 InputEditor(TextVisual이 아닌 atlas renderer로 그립니다)
- dali-core는 이 renderer의 partial update 영역을 원래 크기(`× 1`)로 계산하므로, 넘쳐서 그려진 부분은 갱신 영역에 포함되지 않습니다.

### 2. 원인

(1) 이 PR에서 flag를 넘기는 방식이 바뀌었습니다.

```cpp
// devel: renderer의 custom property로 등록 -> uniform으로 전달됨
mImpl->mRenderer.RegisterProperty("visualTransformUseEffectiveScale", mImpl->mTransformMapSetForFittingMode ? 0.0f : 1.0f);

// 이 PR: dali-core의 default property에 설정
mImpl->mRenderer.SetProperty(Dali::VisualRenderer::Property::VISUAL_EFFECTIVE_SCALE_ENABLED, !mImpl->mTransformMapSetForFittingMode);
```

(2) 그런데 dali-core에서 이 property가 uniform으로 연결되지 않는 것으로 보입니다.

- `EnsureEffectiveScaleVisualPropertiesAndCache()`가 `AddUniformMapping(VISUAL_EFFECTIVE_SCALE_ENABLED, "visualTransformUseEffectiveScale")`를 호출합니다.
- `Object::AddUniformMapping()`은 `GetSceneObjectInputProperty(index)`로 scene-graph property를 찾고, 찾지 못하면 아무것도 등록하지 않습니다.
- `VisualRenderer::GetSceneObjectInputProperty()`에는 `TRANSFORM_ORIGIN`, `TRANSFORM_PIVOT`, `TRANSFORM_OFFSET_SIZE_MODE` 계열의 case만 있고, 새로 추가된 `VISUAL_EFFECTIVE_SCALE` / `VISUAL_EFFECTIVE_SCALE_ENABLED`의 case가 없습니다. `default`로 빠지고, 그 뒤의 `GetSceneObjectAnimatableProperty()`도 이 index를 처리하지 않아 nullptr이 반환됩니다.

(3) 결과적으로 shader에는 `shader.RegisterUniqueProperty("visualTransformUseEffectiveScale", 1.0f)`로 등록해 둔 기본값 1.0만 남습니다.

### 3. 검토 요청

- Label text의 결과가 effective scale ≠ 1에서 devel과 달라지는 것이 의도된 변경인지, 허용 가능한 변경인지 추가 검토가 필요합니다.
- 의도된 변경이 아니라면, flag 0이 예전처럼 shader까지 전달되도록 보완이 필요해 보입니다.
- 실행으로 확인한 내용이 아니므로, UI scale을 2.0 등으로 바꾼 상태에서 Label 샘플 확인 부탁드립니다.

참고로 (2)의 case만 추가해서는 해결되지 않을 것 같습니다. `VISUAL_EFFECTIVE_SCALE_ENABLED`가 BOOLEAN이라 uniform buffer에는 `sizeof(bool)`인 1 byte만 기록되는데, shader의 uniform은 `lowp float`입니다. flag의 타입도 함께 봐 주시면 좋겠습니다.
