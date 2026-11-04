#ifndef DALI_UI_RADIAL_GRADIENT_H
#define DALI_UI_RADIAL_GRADIENT_H

/*
 * Copyright (c) 2026 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// EXTERNAL INCLUDES
#include <dali/public-api/math/vector2.h>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/gradient/gradient-base.h>

namespace DALI_NAMESPACE
{
namespace Ui
{
namespace Gradient
{

/**
 * @brief Authored radial gradient value.
 */
class DALI_UI_API Radial : public Base
{
public:
  /**
   * @brief Creates a radial gradient value.
   */
  Radial();

  /**
   * @brief Creates a radial gradient value.
   *
   * @param[in] center @UNIT_LOGICAL The center position.
   * Only with Units::USER_SPACE (a normalized coordinate otherwise); a ChartView series gradient uses it as canvas
   * pixels without the effective layout scale.
   * @param[in] radius @UNIT_LOGICAL The radius.
   * Only with Units::USER_SPACE (a normalized coordinate otherwise); a ChartView series gradient uses it as canvas
   * pixels without the effective layout scale.
   */
  Radial(const Vector2& center, float radius);

  /**
   * @brief Sets the radial gradient center and radius.
   *
   * @param[in] center @UNIT_LOGICAL The center position.
   * Only with Units::USER_SPACE (a normalized coordinate otherwise); a ChartView series gradient uses it as canvas
   * pixels without the effective layout scale.
   * @param[in] radius @UNIT_LOGICAL The radius.
   * Only with Units::USER_SPACE (a normalized coordinate otherwise); a ChartView series gradient uses it as canvas
   * pixels without the effective layout scale.
   */
  void SetCenterAndRadius(const Vector2& center, float radius);

  /**
   * @brief Gets the center position.
   *
   * @return @UNIT_LOGICAL The center position.
   * Only with Units::USER_SPACE (a normalized coordinate otherwise); a ChartView series gradient uses it as canvas
   * pixels without the effective layout scale.
   */
  Vector2 GetCenter() const;

  /**
   * @brief Gets the radius.
   *
   * @return @UNIT_LOGICAL The radius.
   * Only with Units::USER_SPACE (a normalized coordinate otherwise); a ChartView series gradient uses it as canvas
   * pixels without the effective layout scale.
   */
  float GetRadius() const;

  /**
   * @brief Returns a radial typed value from a base gradient value.
   *
   * @param[in] base The source gradient value
   * @return A deep-copied radial value, or a value whose GetType() is Type::NONE if the source type does not match
   */
  static Radial DownCast(const Base& base);
};

} // namespace Gradient
} // namespace Ui
} //namespace DALI_NAMESPACE

#endif // DALI_UI_RADIAL_GRADIENT_H
