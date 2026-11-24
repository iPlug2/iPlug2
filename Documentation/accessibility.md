# IGraphics accessibility

IGraphics can expose its controls to assistive technology (VoiceOver on macOS and iOS; Narrator, NVDA and JAWS on Windows; Orca and other AT-SPI clients on Linux) using [AccessKit](https://accesskit.dev). It is opt-in, and currently supported on macOS, iOS, Windows and Linux.

Keyboard navigation is part of the same work, but doesn't depend on AccessKit and can be used on its own.

## Enabling it

1. Download the prebuilt AccessKit libraries (once):

   ```bash
   ./Dependencies/IGraphics/download-accesskit.sh
   ```

2. Define `IGRAPHICS_ACCESSKIT` and link the library:

   | Build system | What to do |
   |---|---|
   | CMake (all platforms, the only option on Linux) | Configure with `-DIGRAPHICS_ACCESSKIT=ON` |
   | Xcode | In `MyPlugin-mac.xcconfig` and/or `MyPlugin-ios.xcconfig`, add `IGRAPHICS_ACCESSKIT` to `EXTRA_ALL_DEFS` and `$(ACCESSKIT_LNK_FLAGS)` to `EXTRA_LNK_FLAGS` |
   | Visual Studio | In `MyPlugin-win.props`, add `IGRAPHICS_ACCESSKIT` to `EXTRA_ALL_DEFS` (the library is linked automatically) |

No source files need adding to your project. The static library adds several MB to each binary.

The libraries are built from [iPlug2's fork of accesskit-c](https://github.com/iPlug2/accesskit-c), not from the upstream release. Upstream registers its Objective-C classes with fixed names (`AccessKitNode` etc.), and class names are global to a process, so on macOS the second plug-in that links AccessKit to be loaded by a host aborts the host. The fork gives each copy of AccessKit its own class names (`AccessKitNode_<random>`). If you build AccessKit yourself, build the tag that `download-accesskit.sh` names (the fork's `main` branch is unmodified upstream).

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

## iOS

VoiceOver, Switch Control and Speak Screen get the same tree as on macOS, as elements of the IGraphics view.

- Double tap presses a button, toggles a switch or opens a menu, and swiping up or down adjusts a slider. Other touches don't reach the controls while VoiceOver is on, as for any iOS view.
- The tree is built when the view appears if assistive technology is running, or when it is turned on later, and dropped when it is turned off.
- VoiceOver's cursor is separate from the IGraphics keyboard focus, so no focus ring is drawn for it.
- The AccessKit iOS adapter is newer than the others. visionOS is not supported, since there are no AccessKit libraries for it.

## Linux

The Linux implementation uses AT-SPI over D-Bus, so it needs no extra libraries at build time or run time. If there is no accessibility bus, or no assistive technology is running, it does nothing.

- A plug-in appears to assistive technology as its own application, named after the host's process, rather than as part of the host's window. The UI is a window called after the plug-in.
- AccessKit runs a thread that lasts for as long as the process does, so binaries are linked with `-z nodelete`: once a plug-in has been loaded, the host can't unload it. AccessKit's symbols are not exported.
- The position of the window on the screen comes from X11, so it may be wrong under XWayland. It is polled a few times a second while assistive technology is in use, since an embedded window isn't told when its host's window moves.
- The window is reported as active while it has the X11 keyboard focus, which it takes when it is clicked.
- The AccessKit release only has x86_64 and x86 libraries for Linux. For other architectures, build that tag of [accesskit-c](https://github.com/iPlug2/accesskit-c) and put `libaccesskit.a` in `Dependencies/Build/accesskit/lib/linux/<arch>/static`.

To try it without a screen reader, [Accerciser](https://gitlab.gnome.org/GNOME/accerciser) shows the tree and can perform actions. AccessKit only builds the tree while the `IsEnabled` property of `org.a11y.Status` on the session bus is true, which a screen reader or the desktop's accessibility settings take care of.

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

A node that shows or hides something, such as a button that opens a section, can set `IAccessibleNode::expanded` (`-1` if it doesn't expand, otherwise `0` or `1`).

## Dialogs and overlays

`IGraphics::SetModalControl()` makes a control, such as a dialog or an overlay, modal. Until you call `SetModalControl(nullptr)`, only it and its children can take the keyboard focus, receive key presses or be reached by assistive technology, which is told that it is modal. Mouse input isn't affected. Give a control in it the focus when it is shown, and give the focus back when it is dismissed:

```cpp
void MyDialog::Show()
{
  Hide(false);
  mFocusBeforeShow = GetUI()->GetKeyboardFocus();
  GetUI()->SetModalControl(this);
  GetUI()->SetKeyboardFocus(mpOKButton, 0, false);
}

void MyDialog::Dismiss()
{
  Hide(true);
  GetUI()->SetModalControl(nullptr);
  GetUI()->SetKeyboardFocus(mFocusBeforeShow, 0, false); // if it still exists
}
```

## Limitations

- visionOS and Web are not supported yet.
- The contents of `IPopupMenuControl` and `ITextEntryControl` (the menus and text entries drawn by IGraphics) are not exposed. Platform menus and text entries are.
- Meters are exposed without a value, and `IVKeyboardControl` is not exposed.
- Whether the Tab and arrow keys reach a plug-in's UI depends on the host.
