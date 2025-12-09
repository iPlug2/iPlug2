# IPlugVisage

A minimal gain effect using `VisageEditorDelegate`. Based on the build setup in
commit `467236fe60c2114337954d0ea0904f798512a936`, with a parameter-connected UI
instead of the upstream showcase.

`UI VISAGE` in `iplug_add_plugin()` selects the delegate and propagates the
required includes, sources, libraries, and definitions. The plugin only supplies
`OnDraw()` and mouse callbacks: the delegate owns the editor/window, attaches it
to the host, and handles resizing and closing. No IGraphics code is required.

The slider sends normalized gain values through `SendParameterValueFromUI()`,
with balanced begin/end host gestures. `OnParamChangeUI()` requests redraws for
host automation and preset changes. Closing during a drag ends the gesture;
the destructor closes the editor while the plugin callbacks are still alive.
Gain defaults to 100% (unity), with a range of 0–100%.

## Build

Requires CMake 3.24+, Git, and a C++17 compiler. Supported desktop formats are
APP, VST3, CLAP, and AU (macOS only); SDK availability is handled by iPlug2.
From the repository root:

```sh
cmake -S Examples/IPlugVisage -B build/IPlugVisage -G Ninja -DCMAKE_BUILD_TYPE=Release -DIPLUG_DEPLOY_PLUGINS=OFF
cmake --build build/IPlugVisage --parallel
```

For Visual Studio, omit `-G Ninja` and build with `--config Release`. Xcode
projects can be generated with `-G Xcode`. Select formats with
`-DIPLUG_VISAGE_FORMATS="APP;CLAP"`. Outputs stay in the build directory when
`IPLUG_DEPLOY_PLUGINS=OFF`. To include the example in the root examples build,
set `-DIPLUG2_BUILD_VISAGE=ON` (off by default).

CMake fetches Visage revision `828037000d0893647ab29b66ae9c4a241c90f671` and its
dependencies. Override with `-DIPLUG_VISAGE_GIT_TAG=<revision>`, or use a local
checkout with `-DFETCHCONTENT_SOURCE_DIR_VISAGE=/absolute/path/to/visage`.
The Roboto font from IPlugEffect is embedded into the binary.

macOS requires 10.15+. `cmake/VisageMacOS.cmake` builds a patched copy of the
pinned Visage window source to avoid redundant MTKView drawable acquisition
while bgfx owns presentation. It leaves the dependency checkout unchanged.
Windows uses native pixel dimensions for host window sizing. Linux, iOS, and
web builds are not supported by this example.

## Manual checks

- Drag the slider and confirm gain follows; verify 0% mutes and 100% passes audio.
- Automate Gain in a host and confirm the slider/value follows.
- Resize the editor and check drawing and hit testing remain aligned.
- Close and reopen the editor, including during a drag; verify the gain persists
  and the host gesture ends.
