# IPlugVisage

An iPlug2 gain effect with the Visage graphics showcase embedded in its editor.
The showcase demonstrates graphics and widgets; its controls are not connected
to the gain parameter (use the host's parameter controls).

## Build

Requires CMake 3.24+, Git, and a C++17 toolchain: Xcode command-line tools on
macOS (10.15+) or Visual Studio 2022 with Desktop development with C++ on Windows.
Ninja is optional. Run from the iPlug2 root:

```sh
cmake -S Examples/IPlugVisage -B build/IPlugVisage -G Ninja -DCMAKE_BUILD_TYPE=Release -DIPLUG_DEPLOY_PLUGINS=OFF
cmake --build build/IPlugVisage --parallel
```

For Visual Studio, omit `-G Ninja` and build with `--config Release`. CMake can
also generate an Xcode project using `-G Xcode`. Generated projects stay in the
build directory; no hand-maintained Xcode or Visual Studio projects are needed.

The supported formats are APP, VST3, CLAP, and AU (AU is macOS only). All four
are enabled by default. Select formats
with e.g. `-DIPLUG_VISAGE_FORMATS="APP;CLAP"`. SDK availability is handled by the
iPlug2 CMake helpers. Outputs are under the build directory. To install to the
usual plugin folders after building, configure with `-DIPLUG_DEPLOY_PLUGINS=ON`.
Linux, iOS, and web builds are not supported by this example.

To include this example in a root iPlug2 CMake build, set
`-DIPLUG2_BUILD_VISAGE=ON` (off by default to avoid extra dependency downloads).

## Visage dependency

CMake FetchContent downloads [Visage](https://github.com/VitalAudio/visage) and
its graphics dependencies on the first configure. No `get_visage.sh`, separate
Visage build, or manually copied libraries are required. Fonts, images, icons,
and shaders are compiled and embedded by Visage's CMake helpers; the plugin
does not need the source tree or a shader compiler at runtime.

The default revision is `828037000d0893647ab29b66ae9c4a241c90f671`, the upstream
`main` HEAD when this example was updated. Override it with
`-DIPLUG_VISAGE_GIT_TAG=<commit-or-tag>` (or `main` to follow upstream). The
showcase sources always come from that same revision to avoid API drift.
For local Visage development, use
`-DFETCHCONTENT_SOURCE_DIR_VISAGE=/absolute/path/to/visage`.

On macOS, `cmake/VisageMacOS.cmake` builds a corrected copy of Visage's window
implementation. It removes MTKView's redundant drawable acquisition so bgfx
alone owns acquisition and presentation, avoiding drawable-pool stalls. The
fetched sources (or a local Visage checkout) are not modified. This workaround
can be removed when upstream no longer acquires a drawable in `drawInMTKView:`.

## Example layout

- `IPlugVisage.cpp` / `.h`: gain DSP and embedded Visage editor lifecycle.
- `config.h`: plugin identity, audio layout, and editor settings.
- `CMakeLists.txt` and `cmake/`: dependency, resource, and target setup.
- `resources/`: desktop bundle metadata, icons, and native standalone menus and
  audio preferences dialog. Showcase graphics assets come from Visage.

Installer templates, the generic manual, and resources for unsupported plugin
formats are intentionally omitted.
