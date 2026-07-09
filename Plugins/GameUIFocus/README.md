# Game UI Focus

Reusable Runtime plugin for controller-friendly UMG focus handling.

## What This Contains

- `UGameUIFocusScreenWidgetBase`
  - Owns focus zones: navigation, content, modal.
  - Handles D-pad/left-stick navigation, Accept, Back, page switching, and next-tick focus restore.
- `UGameUIFocusPageWidgetBase`
  - Registers focusable item widgets.
  - Restores last/default focus.
  - Scrolls focused widgets into view when a `FocusScrollBox` is assigned.
- `UGameUIFocusItemWidgetBase`
  - Base class for focusable buttons, rows, slots, and setting entries.
  - Handles focus highlight, mouse hover, 1D/2D movement, analog repeat, and optional focus identifiers.
- `UGameUIFocusValueRowWidget`
  - Generic row base for Settings-like left/right value changes.
  - Handles D-pad/left-stick horizontal changes, Accept, disabled feedback, optional expanded list navigation, and slider/toggle helper getters.
  - Can build numeric slider rows from `MinValue`, `MaxValue`, `StepSize`, and `CurrentValue`.
  - Emits events instead of applying project-specific settings directly.
- `UGameUIFocusPageInterface`
  - Optional interface for Blueprint or custom C++ pages that should participate in the focus system.
- `EGameUIFocusZone`
  - `Navigation`, `Content`, `Modal`.

## Transfer Checklist

1. Copy `Plugins/GameUIFocus` into the target project's `Plugins` folder.
2. Enable the plugin in the target `.uproject`:

```json
{
	"Name": "GameUIFocus",
	"Enabled": true
}
```

3. Regenerate project files.
4. Build the target editor once so UnrealHeaderTool generates the plugin reflection code.
5. Reparent UI Blueprints:
   - Screen/root settings widget -> `GameUIFocusScreenWidgetBase`
   - Page widgets with selectable content -> `GameUIFocusPageWidgetBase`
   - Focusable rows/buttons/slots -> `GameUIFocusItemWidgetBase`
   - Settings rows with left/right value changes -> `GameUIFocusValueRowWidget`
6. In the screen widget, bind or assign:
   - `FocusWidgetSwitcher`
   - navigation widgets via `RegisterNavigationEntry` or `SetNavigationEntries`
   - call `InitializeFocusScreen` after construction/opening
7. In each page widget:
   - set `DefaultFocusWidget` or register focus items
   - assign `FocusScrollBox` if focused rows should auto-scroll into view
8. In each focus item:
   - make sure the widget is enabled, visible, and focusable
   - implement `OnInteractionHighlightChanged` in Blueprint for visual/audio focus feedback

## What Is Not Included

This plugin intentionally does not include project-specific settings logic, save game code, graphics/audio option data, or Nautilus-specific widget materials. For Settings rows, bind to `UGameUIFocusValueRowWidget::OnValueChanged` in the owning page and apply the value in the target project's own settings subsystem.

## Numeric Value Rows

Use `InitializeNumericValueRow` for sliders such as mouse sensitivity, gamepad sensitivity, brightness, or audio volume. This avoids hard-coded Blueprint option lists like `0.1, 0.2, 0.3`.

Example for sensitivity:

```text
RowIdentifier: Setting.Controls.MouseSensitivity
Label: Mouse Sensitivity
MinValue: 0.1
MaxValue: 2.0
StepSize: 0.1
CurrentValue: CurrentMouseSensitivity
FractionalDigits: 1
Suffix: empty
Enabled: true
```

Bind the owning page to `OnNumericValueChanged`:

```text
OnNumericValueChanged(RowIdentifier, OptionIndex, NumericValue)
```

The page applies `NumericValue` to the project's own settings object, player controller, game instance, or save game. The row only owns focus, input, display text, and value stepping.

For player-facing percentages, use a 0-100 range and apply `/ 100` in the project settings layer:

```text
MinValue: 0
MaxValue: 100
StepSize: 10
FractionalDigits: 0
Suffix: %
```
