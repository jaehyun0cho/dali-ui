#pragma once

#include <dali/public-api/common/extents.h>
#include <dali/public-api/math/vector4.h>

#include <dali-ui-foundation/public-api/dali-ui-common.h>

#include <algorithm>
#include <limits>

namespace Dali
{
namespace Ui
{
/**
 * @brief Four logical edge insets with floating-point precision.
 *
 * Values are ordered as start, end, top, and bottom.
 */
struct DALI_UI_API Insets
{
  Insets()
  : start(0.0f),
    end(0.0f),
    top(0.0f),
    bottom(0.0f)
  {
  }

  Insets(float start, float end, float top, float bottom)
  : start(start),
    end(end),
    top(top),
    bottom(bottom)
  {
  }

  Insets(const Dali::Extents& extents)
  : start(static_cast<float>(extents.start)),
    end(static_cast<float>(extents.end)),
    top(static_cast<float>(extents.top)),
    bottom(static_cast<float>(extents.bottom))
  {
  }

  operator Vector4() const
  {
    return Vector4(start, end, top, bottom);
  }

  // TODO(DALIUI-XXXX): Remove after applications migrate from Extents to Insets.
  operator Dali::Extents() const
  {
    const auto toExtent = [](float value)
    {
      return static_cast<int16_t>(std::clamp(value,
                                             static_cast<float>(std::numeric_limits<int16_t>::min()),
                                             static_cast<float>(std::numeric_limits<int16_t>::max())));
    };
    return Dali::Extents(toExtent(start), toExtent(end), toExtent(top), toExtent(bottom));
  }

  bool operator==(const Insets& rhs) const
  {
    return start == rhs.start && end == rhs.end && top == rhs.top && bottom == rhs.bottom;
  }

  bool operator!=(const Insets& rhs) const
  {
    return !(*this == rhs);
  }

  float start;
  float end;
  float top;
  float bottom;
};
} // namespace Ui
} // namespace Dali
