# Length Unit Rules

These rules keep logical and physical lengths apart in the dali-ui API documentation.

## Rule: Tag Every Documented Length With Its Unit

- Status: required
- Scope: public-api, extension-api, integration-api, docs
- Applies To: Every parameter, return value, out parameter, data member, constant, registered property, and
  signal or callback parameter that is a pixel length or a pixel coordinate in the headers of
  `dali-ui-foundation/public-api`, `dali-ui-foundation/extension-api`, `dali-ui-foundation/integration-api`,
  `dali-ui-components/public-api` and `dali-ui-components/integration-api`.

### Intent

dali-core is physical everywhere: Actor size and position, touch coordinates and every parameter, return value and
property. dali-ui mixes those values with logical values that the layout multiplies by the effective layout scale
later. A tag on each length keeps people and tools from passing one where the other is expected.

- Logical value. The layout multiplies it by the effective layout scale of the view it belongs to, the UI scale times
  the window scale, every time it is used, so a later scale change applies automatically. Getters return it as set.
  It equals physical pixels only at scale 1.0. The _dp, _sdp and _spx literals produce logical values.
- Physical value. Pixels in the units dali-core uses, such as Actor::Property::SIZE, POSITION and touch coordinates,
  with the effective layout scale already applied. A physical value the framework stores, such as a scroll position,
  keeps its number when the scale changes, so it can then point at different content.

The tag follows what the code does, not what it should do: a configured length that the code uses without the scale
is physical until the code changes.

### Preferred

```cpp
/**
 * @brief Sets the requested width.
 * @param[in] width @UNIT_LOGICAL The requested width, or WRAP_CONTENT / MATCH_PARENT
 */
void SetRequestedWidth(float width);

/**
 * @brief Gets the scroll position.
 * @return @UNIT_PHYSICAL The current scroll position
 */
Vector2 GetScrollPosition() const;

float x; ///< @UNIT_PHYSICAL X position

/// @brief @UNIT_LOGICAL Title font size. Must be finite and non-negative.
float GetTitleFontSize() const;
```

- Exactly one tag, written as the first token of the description it qualifies.
- A declaration documented only by a brief carries the tag right after `@brief`; it then covers the lengths the
  declaration takes and returns.
- A Set/Get pair uses the same tag.
- One class-description line tags a type whose lengths have the same unit in every use (MeasuredSize).
- When the value follows another factor (the parent's scale, a fallback scaled twice, a policy that makes it a
  fraction), add one sentence after the tag saying exactly which.

### Avoid

```cpp
// No tag: the reader cannot tell which unit to pass.
void SetKeyScrollStep(float step);

// Two tags, or a tag that contradicts the text:
/// @param[in] width @UNIT_LOGICAL @UNIT_PHYSICAL The width in rendered pixels
```

- A tag on a unitless value: ratios, weights, alignment, opacity, normalized anchors, angles, durations, counts,
  indices, scale factors, chart data coordinates.
- A third tag or a free-text unit instead of a tag.
- Hand edits in generated `*.autogen.h` headers.

### Exceptions

- Overrides documented with `@copydoc`, and undocumented overrides, inherit the base tag.
- A type used with both units (LayoutRect) gets no class tag; each use is tagged.
- False positives of the checker, and lengths deliberately left untagged for now, live in its allowlist with one
  reason each. An undocumented property index that only mirrors another index of the same name is allowlisted; the
  documented index carries the unit.
- Private members, `@cond` blocks, `Internal` namespaces and `*.autogen.h` headers are not part of the API
  documentation and are not checked.

### Validation

```sh
python3 scripts/check-length-unit-tags.py --all
python3 scripts/check-length-unit-tags.py --diff origin/devel
```

The definitions and examples are in `docs/length-units.md`.
