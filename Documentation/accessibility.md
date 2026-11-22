# IGraphics accessibility

IGraphics can expose its controls to assistive technology (VoiceOver on macOS; Narrator, NVDA and JAWS on Windows) using [AccessKit](https://accesskit.dev). It is opt-in, and currently supported on macOS and Windows.

Keyboard navigation is part of the same work, but doesn't depend on AccessKit and can be used on its own.

## Enabling it

1. Download the prebuilt AccessKit libraries (once):

   ```bash
   ./Dependencies/IGraphics/download-accesskit.sh
   ```

2. Define `IGRAPHICS_ACCESSKIT` and link the library:

   | Build system | What to do |
   |---|---|
   | CMake | Configure with `-DIGRAPHICS_ACCESSKIT=ON` |
   | Xcode | In `MyPlugin-mac.xcconfig`, add `IGRAPHICS_ACCESSKIT` to `EXTRA_ALL_DEFS` and `$(ACCESSKIT_LNK_FLAGS)` to `EXTRA_LNK_FLAGS` |
   | Visual Studio | In `MyPlugin-win.props`, add `IGRAPHICS_ACCESSKIT` to `EXTRA_ALL_DEFS` (the library is linked automatically) |

No source files need adding to your project. The static library adds several MB to each binary.

## What you get

- The base controls report a role, label and value: knobs and sliders are sliders, `ISwitchControlBase` is a checkbox or a pop-up button, `IButtonControlBase` is a button, tab/radio switches are radio groups, text controls are labels, and multi-sliders, range sliders and XY pads are groups of sliders.
- Assistive technology can press, increment, decrement, set the value of, and focus controls.
- Labels come from `SetAccessibleLabel()`, then the IVControl label, then the name of the linked parameter, then the tooltip. Values use the parameter's display text.
- Nothing is built until assistive technology actually asks for the tree.

## Keyboard navigation

Enabled by default when `IGRAPHICS_ACCESSKIT` is defined, otherwise off. Use `IGraphics::EnableKeyboardNavigation()` to change that.

| Key | Action |
|---|---|
| Tab / Shift+Tab | Move the focus to the next / previous control |
| Arrow keys | Adjust a slider, or change the selection in a list of options |
| Page Up / Page Down | Adjust a slider by 10 steps |
| Home / End | Set a slider to its minimum / maximum |
| Space / Return | Press a button, toggle a switch, open a menu |
| Escape | Clear the focus |

Keys that aren't used for navigation still reach the control under the mouse and the function set with `SetKeyHandlerFunc()`, as before. `SetFocusRingColor()` changes the colour of the focus ring.

## Custom controls

Controls that inherit from one of the base classes usually need nothing. Otherwise:

```cpp
// Simple case: pick a role and a label
pGraphics->AttachControl(new MyDial(bounds, kParamGain))
  ->SetAccessibleRole(EAccessibleRole::Slider)
  ->SetAccessibleLabel("Gain");
```

For controls with several parts, override `IControl::GetAccessibleNodes()` (the first node is the control, any others are its children) and `IControl::OnAccessibleAction()`. `IVTabSwitchControl` and `IVXYPadControl` are examples. `InitAccessibleNode()` fills in a node from a value's parameter.

To hide a control from assistive technology, call `SetAccessibleRole(EAccessibleRole::None)`.

## Limitations

- Linux, iOS and Web are not supported yet.
- The contents of `IPopupMenuControl` and `ITextEntryControl` (the menus and text entries drawn by IGraphics) are not exposed. Platform menus and text entries are.
- Meters are exposed without a value, and `IVKeyboardControl` is not exposed.
- Whether the Tab and arrow keys reach a plug-in's UI depends on the host.
