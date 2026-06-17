# group-selectable-trait sample

Demonstrates `SelectableGroup` / `GroupSelectableTrait` single-selection
(radio-button style) using plain circular Views, stacked vertically with a
status label on top.

- Four options are ordinary `View`s made round with a relative corner radius
  (`SetCornerRadiusPolicyRelative()` + `SetCornerRadius(0.5f)`).
- Each option joins a `SelectableGroup` via `Add()`, which attaches a
  `GroupSelectableTrait` and enables toggle-by-click.
- Tapping an option selects it and deselects the previously selected one;
  re-tapping the selected option is a no-op (radio semantics).
- `SelectedMemberChangedSignal(previous, current, event)` recolours the previous
  option grey and the current option blue, and updates the top `Label`
  (e.g. "1st View is selected").

The group is not a visual radio-button component — it is the backing
single-selection controller; the colour/shape here is just sample styling.

Press Escape or Back to quit.
