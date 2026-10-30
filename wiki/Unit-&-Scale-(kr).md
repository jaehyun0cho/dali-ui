# Unit & Scale

dali-ui는 다양한 화면 밀도(DPI)를 가진 디바이스에서 일관된 UI를 구현하기 위해 픽셀 독립적인 단위 시스템을 제공합니다.
모든 단위는 [Configuration](https://github.sec.samsung.net/NUI/dali-ui/wiki/Configuration-(kr)#configuration-options)에서 설정한 값을 기반으로 런타임에 픽셀 값으로 변환됩니다. 이 값은 자연 단위이며, layout이 그 길이를 소유한 View의 유효 스케일을 곱합니다([UI Scale](https://github.sec.samsung.net/NUI/dali-ui/wiki/Ui-Scale-(Kr)) 참고).

<br/>

## 단위 종류

| 단위 | 리터럴 | 변환 공식 | 설명 |
|---|---|---|---|
| `px` | 없음 (plain `float`) | — | 자연 단위. 유효 스케일 1.0에서 디바이스 픽셀과 같으며, layout이 View의 유효 스케일을 곱합니다 |
| `spx` | `100_spx` | `value × ScalingFactor` | ScalingFactor만 반영. 폰트 크기 등 사용자 설정에 따라 스케일 |
| `dp` | `100_dp` | `value × (DPI / BaselineDPI)` | 화면 밀도를 반영한 밀도 독립 픽셀 |
| `sdp` | `100_sdp` | `value × (DPI / BaselineDPI) × ScalingFactor` | 밀도와 ScalingFactor를 모두 반영 |

<br/>

## Scaling Factor 설정

ScalingFactor는 [Configuration](https://github.sec.samsung.net/NUI/dali-ui/wiki/Configuration-(kr)#configuration-options)에서 설정합니다.

```cpp
UiConfig config = UiConfig::New();
config.SetScalingFactor(1.5f);
config.SetDpi(320);
config.SetBaselineDpi(160);
config.Apply();
```

위 설정에서 각 단위의 변환 예시:

| 표현식 | 계산 | 결과 |
|---|---|---|
| `100_spx` | `100 × 1.5` | `150.0f px` |
| `100_dp` | `100 × (320 / 160)` | `200.0f px` |
| `100_sdp` | `100 × (320 / 160) × 1.5` | `300.0f px` |

<br/>

## 사용 예시

```cpp
view.SetRequestedWidth(200_dp);
view.SetRequestedHeight(100_spx);
view.SetRequestedX(50_sdp);
```

<br/>

## 단위와 유효 스케일

단위 리터럴은 `UiConfig`로 한 번 계산되는 plain `float`입니다. 동적 스케일은 그 뒤에 layout이 적용합니다. 요청 크기와 요청 위치, margin, padding, spacing, `GridLength::Absolute()`, `AbsoluteLayoutParams` bounds, `LayoutTransition`의 `LayoutBoundsLength::Pixel()` offset 등 layout이 사용하는 모든 길이에는 그 길이를 소유한 View의 유효 스케일이 곱해집니다. 위 설정에서 유효 스케일이 `2.0`이면 `view.SetRequestedWidth(100_spx)`는 자연 단위로 `150`이고, 렌더링 폭은 `300` pixel입니다.

렌더링 geometry를 받는 API는 유효 스케일을 적용하지 않으므로, 같은 리터럴이 그 배율만큼 작게 렌더링됩니다: `Dali::Ui::Extension::View`의 geometry setter, `ViewAnimationSpec`의 position / size entry, `LayoutAnimatorContext`의 bounds, `ScrollView`의 scroll position.

<br/>

## 주의사항

> [!WARNING]
> **`Apply()` 호출 전 사용 금지**
> 단위 리터럴은 `UiConfig::Apply()` 이후에만 사용할 수 있습니다.
> `Apply()` 전에 사용하면 assertion 실패가 발생합니다.

> [!WARNING]
> **전역/정적 변수 초기화에 사용 금지**
> 단위 리터럴을 전역 또는 정적 변수의 초기값으로 사용하지 마세요.
> C++의 정적 초기화 순서는 translation unit 간에 보장되지 않으므로,
> `UiConfig::Apply()`가 호출되기 전에 단위 변환이 실행될 수 있습니다.
>
> ```cpp
> // ❌ 잘못된 사용
> static float kWidth = 100_dp;
>
> // ✅ 올바른 사용: 필요한 시점에 계산
> float GetWidth() { return 100_dp; }
> ```

<br/>

---

[← Back to list](https://github.sec.samsung.net/NUI/dali-ui/wiki/Home-(kr)#development-guides)
