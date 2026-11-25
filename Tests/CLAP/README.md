# Linux CLAP X11 floating editor smoke test

Build `IPlugEffect-clap` for Linux with its editor enabled. From the repository root, compile the test and run it on a private Xvfb server (no window manager):

```sh
c++ -std=c++17 -Wall -Wextra -Werror \
  -IDependencies/IPlug/CLAP_SDK/include Tests/CLAP/x11-floating.cpp \
  -lX11 -ldl -o build/test-clap-x11
xvfb-run -a build/test-clap-x11 "$PWD/build/x11-floating/out/IPlugEffect.clap"
```

The test checks explicit X11 and empty API strings, reuse of the same X11
window across `hide()` / `show()`, and one `closed(true)` notification when
receiving `WM_DELETE_WINDOW`. It also checks that the editor can be created
again after `destroy()` acknowledges the close.

The CLAP Helpers revision used for this test assigns the API pointer to a
`std::string` before calling the plug-in, so a null API pointer currently
crashes in the helper. The test uses an empty string for the omitted API.
