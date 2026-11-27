# Cocoa AUv2 resize regression

This standalone executable uses the real `IGraphicsMac` view with `AU_API`
enabled and a minimal editor delegate. It needs macOS, Xcode command-line tools
and a working graphics backend; it does not require a DAW, an installed plug-in
or an audio device. Build and run from the repository root:

```sh
cmake -S Tests/IGraphicsMacResize -B build/mac-resize -DCMAKE_BUILD_TYPE=Debug
cmake --build build/mac-resize --target IGraphicsMacResize --parallel 4
ctest --test-dir build/mac-resize --output-on-failure
```

The default NanoVG/Metal backend needs no downloaded graphics libraries. Other
supported backends can be selected with the usual `IGRAPHICS_BACKEND` and
`IGRAPHICS_RENDERER` options.

The first check requires a parent-view resize to update IGraphics dimensions;
the preceding implementation has no notification path to do so. The test also exercises direct editor-view resizing,
UI scale requests with delayed parent acknowledgement, fitting a fixed-aspect
canvas without notification recursion, position-only changes, disabled host
resizing, reparenting, closing and an editor used as the window content view.

This checks Cocoa notification delivery and editor geometry. It does not certify
AU host negotiation. For that, build `IPlugResponsiveUI` as AUv2 with
`PLUG_HOST_RESIZE=1`, open it in a host that offers window-frame resizing, and
confirm that dragging the frame relays out its controls. Repeat UI-initiated
resizing with an example that attaches the corner resizer, including changing
scale before the host changes its container. Check closing/reopening and moving
between Retina and non-Retina displays.
