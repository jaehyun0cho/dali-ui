#pragma once

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

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/dali-ui-common.h>

/*
 * Length unit tags of the API documentation
 *
 * The API documentation marks a length or coordinate in pixels, a speed in pixels per unit of time, or a font size,
 * with one of the two tags below, written as the first token of its description. The tag says whether the framework
 * still multiplies the value by the effective layout scale or whether the scale is already in it.
 *
 * @UNIT_LOGICAL Logical value. The layout multiplies it by the effective layout scale of the view it belongs to, the
 * UI scale times the window scale, every time it is used, so a later scale change applies automatically. Getters
 * return it as set. It equals pixels only at scale 1.0. The _dp, _sdp and _spx literals produce logical
 * values.
 *
 * @UNIT_PIXEL Pixel value. Pixels in the units dali-core uses, such as Actor::Property::SIZE, POSITION and
 * touch coordinates, with the effective layout scale already applied. A pixel value the framework stores, such as
 * a scroll position, keeps its number when the scale changes, so it can then point at different content.
 *
 * pixel = logical x effective layout scale, unless the text after the tag says otherwise
 *
 * On a function without @param or @return text for a length, a tag right after @brief covers the lengths it takes
 * and returns. A tag in a class description covers every length of the class, for example MeasuredSize is in pixels.
 * When a value follows another factor, or a policy makes it a fraction of a size, the text after the tag says so. A
 * font size in points stays in points; its tag only says whether the scale is in it. Unitless values carry no tag:
 * ratios, weights, alignment, opacity, normalized anchors, angles, durations, counts, indices, scale factors and
 * chart data coordinates. WRAP_CONTENT and MATCH_PARENT are sentinels, not lengths. A length that no tag covers does
 * not say whether it is logical or in pixels, for example in the generated *.autogen.h headers. In other text, logical
 * pixels are logical values, but logical can also mean reading direction (a logical edge) or layout order (the
 * logical child list).
 *
 * The Length Units guide of the API documentation, docs/length-units.md in the dali-ui repository, has examples.
 */

namespace DALI_NAMESPACE
{

namespace Ui
{

/**
 * @brief Converts an integer spx value to a float pixel value.
 * @details The input value is multiplied by a scaling-factor configured via UiConfig.
 *
 * @pre UiConfig::Apply() must have been called before using this operator.
 *      An assertion failure is triggered if UiConfig has not been initialized.
 * @warning Do NOT use this operator to initialize static or global variables.
 *          Static initialization order is undefined across translation units,
 *          so UiConfig::Apply() may not have been called yet at that point.
 *          Use lazy initialization or compute values at first access instead.
 *
 * @param v The value in spx (e.g. 35_spx)
 * @return @UNIT_LOGICAL The converted value as a float.
 */
DALI_UI_API float operator"" _spx(unsigned long long int v);

/**
 * @brief Converts a floating-point spx value to a float pixel value.
 * @details The input value is multiplied by a scaling-factor configured via UiConfig.
 *
 * @pre UiConfig::Apply() must have been called before using this operator.
 *      An assertion failure is triggered if UiConfig has not been initialized.
 * @warning Do NOT use this operator to initialize static or global variables.
 *          Static initialization order is undefined across translation units,
 *          so UiConfig::Apply() may not have been called yet at that point.
 *          Use lazy initialization or compute values at first access instead.
 *
 * @param v The value in spx (e.g. 35.0_spx)
 * @return @UNIT_LOGICAL The converted value as a float.
 */
DALI_UI_API float operator"" _spx(long double v);

/**
 * @brief Converts an integer dp value to a float pixel value.
 * @details The input value is computed with dpi and baseline-dpi configured via UiConfig.
 *
 * @pre UiConfig::Apply() must have been called before using this operator.
 *      An assertion failure is triggered if UiConfig has not been initialized.
 * @warning Do NOT use this operator to initialize static or global variables.
 *          Static initialization order is undefined across translation units,
 *          so UiConfig::Apply() may not have been called yet at that point.
 *          Use lazy initialization or compute values at first access instead.
 *
 * @param v The value in dp (e.g. 35_dp)
 * @return @UNIT_LOGICAL The converted value as a float.
 */
DALI_UI_API float operator"" _dp(unsigned long long int v);

/**
 * @brief Converts a floating-point dp value to a float pixel value.
 * @details The input value is computed with dpi and baseline-dpi configured via UiConfig.
 *
 * @pre UiConfig::Apply() must have been called before using this operator.
 *      An assertion failure is triggered if UiConfig has not been initialized.
 * @warning Do NOT use this operator to initialize static or global variables.
 *          Static initialization order is undefined across translation units,
 *          so UiConfig::Apply() may not have been called yet at that point.
 *          Use lazy initialization or compute values at first access instead.
 *
 * @param v The value in dp (e.g. 35.0_dp)
 * @return @UNIT_LOGICAL The converted value as a float.
 */
DALI_UI_API float operator"" _dp(long double v);

/**
 * @brief Converts an integer sdp value to a float pixel value.
 * @details The input value is computed with dpi, baseline-dpi and scaling-factor
 *          configured via UiConfig.
 *
 * @pre UiConfig::Apply() must have been called before using this operator.
 *      An assertion failure is triggered if UiConfig has not been initialized.
 * @warning Do NOT use this operator to initialize static or global variables.
 *          Static initialization order is undefined across translation units,
 *          so UiConfig::Apply() may not have been called yet at that point.
 *          Use lazy initialization or compute values at first access instead.
 *
 * @param v The value in sdp (e.g. 35_sdp)
 * @return @UNIT_LOGICAL The converted value as a float.
 */
DALI_UI_API float operator"" _sdp(unsigned long long int v);

/**
 * @brief Converts a floating-point sdp value to a float pixel value.
 * @details The input value is computed with dpi, baseline-dpi and scaling-factor
 *          configured via UiConfig.
 *
 * @pre UiConfig::Apply() must have been called before using this operator.
 *      An assertion failure is triggered if UiConfig has not been initialized.
 * @warning Do NOT use this operator to initialize static or global variables.
 *          Static initialization order is undefined across translation units,
 *          so UiConfig::Apply() may not have been called yet at that point.
 *          Use lazy initialization or compute values at first access instead.
 *
 * @param v The value in sdp (e.g. 35.0_sdp)
 * @return @UNIT_LOGICAL The converted value as a float.
 */
DALI_UI_API float operator"" _sdp(long double v);

} // namespace Ui

} //namespace DALI_NAMESPACE
