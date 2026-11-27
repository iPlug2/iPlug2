# X11 host resize regression

This test embeds the real `IGraphicsLinux` editor in an X11 window. The test host
changes the parent without a plug-in-format size callback or a child resize.
It checks that the delegate receives the parent dimensions once and resizes the
child, including for a change on just one axis. It also checks position-only
moves, responses to UI size requests, reparenting, closing, and disabled host
resizing.

Run from the iPlug2 root on Linux with the normal X11/NanoVG build dependencies
and a running X11 or XWayland display with GLX support:

```sh
cmake -S Tests/IGraphicsLinuxResize -B build/x11-resize -G Ninja \
  -DIGRAPHICS_BACKEND=NANOVG -DIGRAPHICS_RENDERER=GL3
cmake --build build/x11-resize
ctest --test-dir build/x11-resize --output-on-failure
```

No plug-in SDK, DAW, or audio device is needed.
