# Length Units

Every length in the dali-ui API documentation carries one of two tags. The tag says whether the framework still
multiplies the value by the effective layout scale or whether the scale is already in it.

## Definitions

- **Logical** (`@UNIT_LOGICAL`, shown as **Logical:**): Logical value. The layout multiplies it by the effective layout
  scale of the view it belongs to, the UI scale times the window scale, every time it is used, so a later scale change
  applies automatically. Getters return it as set. It equals physical pixels only at scale 1.0. The _dp, _sdp and _spx
  literals produce logical values.
- **Physical** (`@UNIT_PHYSICAL`, shown as **Physical:**): Physical value. Pixels in the units dali-core uses, such as
  Actor::Property::SIZE, POSITION and touch coordinates, with the effective layout scale already applied. A physical
  value the framework stores, such as a scroll position, keeps its number when the scale changes, so it can then
  point at different content.

The effective layout scale of a view is `Extension::View::GetEffectiveLayoutScale()`:

```
physical = logical x Extension::View::GetEffectiveLayoutScale(view)
```

## Reading a tag

- `@param[in] width @UNIT_LOGICAL The requested width`: pass a logical value.
- `@return @UNIT_PHYSICAL The scroll position`: the value comes back with the scale in it.
- A tag right after `@brief` of a function that has no `@param` or `@return` text for the length applies to the
  lengths it takes and returns.
- A tag on a class description applies to every length of the class, for example MeasuredSize is physical.
- A tag never appears on unitless values: ratios, weights, alignment, opacity, normalized anchors, angles, durations,
  counts, indices, scale factors and chart data coordinates.
- WRAP_CONTENT and MATCH_PARENT are sentinels, not lengths. A proportional AbsoluteLayout value and a RELATIVE corner
  radius are fractions of a size; the API text says when that applies.
- When a value follows another factor (for example the scale of the parent instead of its own), its documentation
  says so in one sentence after the tag.

## Typical logical values

- View requested width, height, x and y; minimum and maximum sizes; padding and margin.
- Layout spacing, FlexLayout basis, GridLength::Absolute() tracks, AbsoluteLayout bounds.
- Corner radius (ABSOLUTE), borderline width, the View Shadow offset, extents and blur radius, and the InnerShadow
  insets and blur radius.
- Font sizes, in pixels or in points, wherever a text control multiplies them by its text UI scale. A font size can
  also follow the system font size scale.
- Values written with the _dp, _sdp and _spx literals.

## Typical physical values

- View::Measure() constraints, the MeasuredSize result, View::Arrange() bounds and LayoutFinishedSignal() rects.
- Extension::View::SetPositionX(), SetSizeWidth() and the other geometry setters.
- Scroll positions, scroll offsets and drag deltas.
- Touch, drag-and-drop and accessibility coordinates.
- Some configured lengths that the framework does not scale yet, such as the Text::Shadow and Text::Outline blur
  radius; their documentation is tagged physical.

## Example

```cpp
using namespace Dali;
using namespace Dali::Ui;

View card = View::New();
card.SetRequestedWidth(200.0_dp);      // logical: 200 dp at scale 1.0, twice as wide at 2.0
card.SetPadding(Insets(16, 16, 8, 8)); // logical

ScrollView scroll = ScrollView::New();
scroll.ScrollTo(Vector2(0.0f, 600.0f)); // physical: 600 rendered pixels

// Turn a physical size into the logical value a requested size needs:
float scale = Extension::View::GetEffectiveLayoutScale(GetImpl(card));
float width = card.GetMeasuredSize().width / scale;
```

The example needs `#include <dali-ui-foundation/extension-api/view.h>` for GetEffectiveLayoutScale().

## Stored physical values

A physical value that the framework keeps, such as the scroll position of a ScrollView, is not converted when the
effective layout scale changes. After a scale change the same number can point at different content. Convert it with
the old and new scales if the content must stay in place.
