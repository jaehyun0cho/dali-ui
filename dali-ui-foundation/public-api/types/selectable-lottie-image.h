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

// EXTERNAL INCLUDES
#include <dali/public-api/common/dali-string.h>
#include <cstdint>

// INTERNAL INCLUDES
#include <dali-ui-foundation/public-api/dali-ui-common.h>

namespace Dali
{
namespace Ui
{

/**
 * @brief A Lottie image for a selectable control, bundling the animation url with
 * the explicit integer frame ranges that play on select and on deselect.
 *
 * A selectable control (CheckBox, RadioButton, Switch, ...) drives a single Lottie
 * glyph by playing a "select" segment when it becomes selected and a "deselect"
 * segment when it becomes deselected. Those two segments are inseparable from the
 * url: a url without both ranges is meaningless. SelectableLottieImage therefore has
 * a single construction path that takes the url AND both ranges together — there is
 * no way to register a url without also supplying both frame ranges.
 *
 * The frame ranges are EXPLICIT integer frame numbers (not marker names); they map
 * directly onto LottieAnimationView::SetMinMaxFrame() / JumpToFrame().
 *
 * SelectableLottieImage is plain, copyable, immutable value data (icon description held by
 * a style). The live scene view that renders it is SelectableLottieAnimationView, created
 * per control instance.
 *
 * @code
 *   // url + select segment [0,30] + deselect segment [30,48]
 *   SelectableLottieImage image("checkbox.json",
 *                               SelectableLottieImage::FrameRange(0, 30),
 *                               SelectableLottieImage::FrameRange(30, 48));
 * @endcode
 */
class DALI_UI_API SelectableLottieImage
{
public:
  /**
   * @brief An inclusive integer frame span [startFrame, endFrame].
   *
   * The public members map directly onto the signed-int Lottie frame API, following
   * the Dali::Extents value-struct convention (public members + a plain constructor).
   */
  struct FrameRange
  {
    /**
     * @brief Default constructor which provides an initialized FrameRange(0, 0).
     */
    FrameRange() = default;

    /**
     * @brief Constructor.
     *
     * @param[in] start The first frame of the segment
     * @param[in] end   The last frame of the segment
     */
    FrameRange(int32_t start, int32_t end);

    int32_t startFrame{0}; ///< The first frame of the segment
    int32_t endFrame{0};   ///< The last frame of the segment
  };

  /**
   * @brief Creates a SelectableLottieImage from a url and both frame ranges.
   *
   * This is the ONLY construction path: a url can never be registered without both
   * the select and the deselect frame ranges.
   *
   * @param[in] url           The Lottie animation url
   * @param[in] selectRange   The frame segment played when the control becomes selected
   * @param[in] deselectRange The frame segment played when the control becomes deselected
   */
  SelectableLottieImage(const Dali::String& url,
                        const FrameRange&   selectRange,
                        const FrameRange&   deselectRange);

  /**
   * @brief Deleted default constructor: a url is inseparable from both frame ranges.
   */
  SelectableLottieImage() = delete;

  /**
   * @brief Returns the Lottie animation url.
   *
   * @return The url
   */
  const Dali::String& GetUrl() const;

  /**
   * @brief Returns the frame segment played when the control becomes selected.
   *
   * @return The select frame range
   */
  const FrameRange& GetSelectRange() const;

  /**
   * @brief Returns the frame segment played when the control becomes deselected.
   *
   * @return The deselect frame range
   */
  const FrameRange& GetDeselectRange() const;

private:
  Dali::String mUrl;           ///< The Lottie animation url
  FrameRange   mSelectRange;   ///< The segment played when the control becomes selected
  FrameRange   mDeselectRange; ///< The segment played when the control becomes deselected
};

} // namespace Ui
} // namespace Dali
