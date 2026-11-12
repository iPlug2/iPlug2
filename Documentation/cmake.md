# Building iPlug2 Projects with CMake

This document covers building iPlug2 plugin projects using CMake on all supported platforms.

## Prerequisites

### macOS

- **CMake** 3.14 or later
- **Ninja** (recommended) or Xcode
- **Xcode Command Line Tools**

```bash
# Install via Homebrew
brew install cmake ninja
```

### Windows

- **CMake** 3.14 or later
- **Visual Studio 2019 or 2022** with "Desktop development with C++" workload
- **Ninja** (optional, recommended for faster builds)

CMake and Ninja can be installed via:
- Visual Studio Installer (includes CMake)
- [cmake.org](https://cmake.org/download/)
- `winget install Kitware.CMake` / `winget install Ninja-build.Ninja`

### Linux

- **CMake** 3.14 or later
- **Ninja** (recommended) or Make
- **GCC** or **Clang** with C++17 support
- **Development libraries:**

```bash
# Ubuntu/Debian
sudo apt-get install -y \
  ninja-build pkg-config \
  libfreetype-dev libfontconfig-dev \
  libx11-dev libx11-xcb-dev libxcb1-dev libxi-dev libxext-dev libxrender-dev \
  libxrandr-dev libxfixes-dev libxcursor-dev \
  libgl-dev libgtk-3-dev \
  libasound2-dev libjack-jackd2-dev libpulse-dev
```

### Web (Emscripten)

- **Emscripten SDK** for WebAssembly builds
- **WAM SDK** (downloaded via `Dependencies/download-iplug-sdks.sh`)

```bash
# Install Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh
```

## Generators

CMake supports multiple build system generators:

### Ninja (Recommended)

Fast, cross-platform. Works on macOS, Windows, and Linux.

```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
```

### Visual Studio (Windows)

Multi-configuration generator. Supports x64 and ARM64EC architectures.

```bash
# x64 (default)
cmake -G "Visual Studio 17 2022" -A x64 ..

# ARM64EC (ARM64 with x64 emulation compatibility)
cmake -G "Visual Studio 17 2022" -A ARM64EC ..

# Build from command line
cmake --build . --config Release
```

### Xcode (macOS/iOS/visionOS)

Multi-configuration generator for Apple platforms.

```bash
cmake -G Xcode ..
cmake --build . --config Release
# Or open the .xcodeproj in Xcode
```

## Quick Start

```bash
# From your plugin project directory
mkdir build && cd build

# Configure (choose one)
cmake -G Ninja ..                              # Ninja (macOS/Windows/Linux)
cmake -G "Visual Studio 17 2022" -A x64 ..     # Visual Studio (Windows)
cmake -G Xcode ..                              # Xcode (macOS)
cmake -G "Unix Makefiles" ..                   # Make (macOS/Linux)

# Build
cmake --build . --config Release
```

## Supported Platforms & Targets

| Format | Target Suffix | macOS | Windows | Linux | iOS | visionOS | Web |
|--------|---------------|-------|---------|-------|-----|----------|-----|
| Standalone App | `-app` | ✓ | ✓ | ✓ | ✓ | ✓ | - |
| VST2 | `-vst2` | ✓ | ✓ | - | - | - | - |
| VST3 | `-vst3` | ✓ | ✓ | ✓ | - | - | - |
| CLAP | `-clap` | ✓ | ✓ | ✓ | - | - | - |
| AAX | `-aax` | ✓ | ✓ | - | - | - | - |
| AUv2 | `-au` | ✓ | - | - | - | - | - |
| AUv3 | `AU-framework`, `AUv3-appex` | ✓ | - | - | ✓ | ✓ | - |
| WAM | `-wam`, `-web` | - | - | - | - | - | ✓ |

**Notes:**
- VST2 requires the VST2 SDK (deprecated, no longer distributed by Steinberg)
- VST3 requires the VST3 SDK
- CLAP requires the CLAP SDK
- AAX requires the AAX SDK from Avid
- AUv3 is embedded in the standalone APP on macOS
- Linux uses X11, NanoVG/GL3, and ALSA/JACK for audio

An absent or incomplete VST3 SDK disables automatic VST3 target creation, without
preventing standalone builds. CMake reports the first missing SDK file and the
download command. To install it, run `./download-vst3-sdk.sh` from
`Dependencies/IPlug/`, then configure again.

### Linux runtime regression tests

These tests use a separate build directory and never deploy plugins to user/system
scan paths or use an audio device. Deployment fixtures copy only inside the test
build directory. The tests require `xvfb`, `xauth`, and Mesa's software renderer
(`libgl1-mesa-dri` on Ubuntu/Debian), in addition to the Linux build dependencies.
The host tests require the VST3 SDK, CLAP SDK, and CLAP helpers headers.
The standard build targets below also build the GTK clipboard and core timer
regression executables through target dependencies.

```bash
cmake -S Tests/Linux -B build/linux-regression -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux-regression --parallel 2 \
  --target IPlugEffect-app LinuxGuiTest LinuxStartupHooks \
  LinuxDialogTest LinuxRuntimeTest LinuxClapHostTest LinuxVst3HostTest LinuxOscTest
ctest --test-dir build/linux-regression --output-on-failure
```

The CLAP/VST3 host fixtures also process known stereo input without an audio
device, save and restore the gain parameter, and check output samples and buffer
boundaries at 1, 64, 127 and 512 frames and 44.1/48 kHz. VST3 covers both 32-bit
and 64-bit sample buffers; CLAP covers 32-bit buffers and partial state-stream
reads/writes. Processing runs on a separate audio thread.

Host-scale tests cover notification before attachment (and before the VST3
frame), visible scale changes, fractional factors, invalid values, environment
override priority, and CLAP hide/show. Click-focus tests route key events through
the X server's current input focus rather than directly invoking a key handler.
The GTK clipboard test owns the selection on the same UI thread as the editor
and requests editor shutdown during a pending paste.

Linux Ninja CI covers GCC x86_64, GCC ARM64 and Clang with ASan/UBSan. GL/GTK
process-wide caches are excluded from leak checking; the core timer lifecycle
test runs separately with leak detection enabled. Clang's Linux byte-swap
compatibility is supplied by `IPlug/IPlugLinuxHeaders.h`, without changing WDL.
Four timer-entry-point fault-injection cases run only in the GCC jobs: ASan
substitutes its own pthread entry point, which prevents the injection shim from
identifying the timer callback. The timer lifecycle tests still run under ASan.

The suite checks repeated GUI startup, rendering and normal shutdown,
main-loop timer/display initialization failures, ALSA error
visibility, and missing/placeholder/incomplete/complete VST3 SDK detection.
It also drives real GTK dialogs to check all message button sets, file open/save
and directory selection, cancellation, reference outputs, callback results, and
space-separated extension filters (with optional leading dots). A separate process
checks safe cancellation when the optional GTK runtime cannot be loaded.
Additional tests cover editor shutdown while the graphics mutex is held, core
timer startup/failure and stopping, self-owned and external UTF-8/empty clipboards,
and automatic/deferred deployment. A minimal CLAP host exercises absent, partial,
and rejected timer/fd extensions, callback coalescing, GUI hide/show/destroy, and
window/fallback-timer initialization failures.
The deployment fixture also checks Linux REAPER extension `reaper_*.so` naming
and deployment of web resources alongside the extension using an isolated path.
Actual resource lookup is checked in separate processes for both COPY and SYMLINK
deployment, including VST3 bundles and two flat plugins with different contents
under the same resource filename. Missing resources must not resolve to another
plugin, the host executable, or the legacy shared resource directory.
Native X11 close events are tested with internal and host-driven event
loops. Clipboard tests include two editors sharing one UI thread, an independent
client pasting while the owner's main thread is blocked, and service startup failures.
They also destroy requestor windows during transfers under both the default and
a host-installed X11 error handler, then verify later replies and unrelated host
errors still work.
GLX probes keep a separate host context with distinct draw/read drawables current
during editor initialization, rendering, resizing, reopening and teardown, and
check restoration after failed activation. They also cover no current context
and nested context scopes across two editors.
Keyboard probes check host-delivered keys at normal, HiDPI and fractional UI
scales, Latin-1/Unicode keys, local XIM Compose sequences, input-context teardown
and reopening, and multi-character UTF-8 commits (including buffer-overflow retries).
IME commits are split into code points to match `IKeyPress`; preedit UI is left to
the input method. XIM uses the host's locale and modifiers without changing these
process-wide settings. A UTF-8 host locale is recommended for Compose and IME.
When no input context is available, a diagnostic is logged and basic key input
still converts Latin-1/Unicode keysyms to UTF-8, without Compose or IME.
Parent-window validation uses checked XCB replies and never replaces the host's
process-wide X11 error handler. Tests cover valid, destroyed and out-of-range
parent IDs, fallback parenting, and continued delivery of unrelated host errors.
The VST3 host tests check real knob gestures and rendering on the host UI thread,
run loops supplied by the frame or initialization context, missing/rejected
registrations, frame replacement, native close, and editor reopening.
Dedicated idle probes check main-thread callbacks before opening an editor, after
closing it, and in builds without IGraphics. They also check queued UI messages,
callback coalescing, timer initialization failures, and teardown.
Close lifecycle probes check that `OnUIClose()` runs once on the main thread with
live controls, including native close, later host teardown, reopening, and
standalone graphics-timer initialization failure.
Standalone input and drawing probes check the owning main thread, including a
GTK dialog opened from an X11 button event and reentrant main-loop dispatch.
A mock REAPER host uses real SWELL windows and timers to check idle delivery
before/after editor use, main-thread drawing and host API calls, deferred docking
and hiding from control callbacks, and timer teardown. It also checks the actual
X11 parent chain and client-area geometry in floating and docked windows, after
resizing, and recovery after bridge/editor initialization failures.
Linux IGraphics REAPER extensions embed through `SWELL_CreateXBridgeWindow`,
passing its X11 Window ID rather than a SWELL HWND to the editor. The bridge
tracks the container's client area and supports docked SWELL children without
their own native window. Bridge or editor initialization failure closes the
container and logs an error instead of opening an unrelated top-level editor.
Fault-injection hooks and exported executable symbols are confined to this test
build. The Linux Ninja CI job runs this suite with software OpenGL under Xvfb.

Linux file/directory dialogs update their reference outputs before invoking the
completion handler and clear them on cancellation or failure. Selected directory
paths end in `/`; directory callbacks leave the filename argument empty and put
the result in the path argument. Message boxes return `kCANCEL` and log an error
when GTK is unavailable, rather than approving the action.

Standalone and REAPER extension graphics timers use the SWELL main loop at the
editor's configured frame rate. Input, drawing, GTK dialogs and control actions
therefore run on the owning main thread rather than a graphics worker.
Nested main-loop dispatch skips graphics callbacks already in progress.
Standalone SWELL is built with GTK event forwarding so opening a GTK dialog does
not replace the application's event handler. Container close/quit is completed
by the outer standalone main loop, never inside a modal callback.
Editor close requests made inside input/drawing callbacks are completed only
after dispatch returns; REAPER docking/hiding actions are deferred the same way.
REAPER idle dispatch starts only after host API imports are complete, uses a
SWELL main-loop timer even without an editor, and stops before extension teardown.

Linux drawing skips a timer tick when the graphics mutex is busy, so editor
teardown can join the drawing thread without a lock inversion. Core timer creation
logs an error and returns `nullptr` if the worker cannot be created.
Linux graphics context scopes restore the calling thread's previous GLX display,
context and draw/read drawables, including nested resource creation and teardown,
instead of clearing the host's current context.

VST3 editors use the host's Linux `IRunLoop` for X11 input and drawing. The run loop
must be available through the frame or initialization context and accept a GUI
timer; otherwise editor attachment fails explicitly rather than using a worker
thread for VST3 UI notifications. FD registration is optional because timer ticks
also poll X11. Registrations are released before the editor or frame is replaced.
The core idle/message timer is separate from the editor timer and remains active
until plugin termination. It uses the initialization context's run loop, or retains
the first usable frame run loop if the context does not provide one. If neither
provides a usable run loop, idle dispatch is unavailable and a diagnostic is logged;
it never falls back to calling UI or host code on a worker thread.

CLAP uses host timers when available. If timer support is missing or registration
fails, a private timer requests coalesced `on_main_thread` callbacks; X11 event
handling and rendering still run on the host's main thread. Host fd support is
optional because GUI ticks also poll X11. All registrations and fallback workers
are released when the GUI is hidden or destroyed and recreated when shown again.
Native close events also release VST3/CLAP GUI registrations before reopening.
X11 close events only record a request and stop drawing. VST3/CLAP complete it
after graphics dispatch returns, unregistering GUI callbacks before closing through
the editor delegate. Standalone completes the request on its SWELL idle timer.
REAPER extensions use their SWELL idle timer to complete the request and
close their container on the host main thread.
This keeps controls alive during `OnUIClose()` and avoids destroying a graphics
object from its own worker or event-processing stack. Repeated host close calls
do not repeat the callback.
CLAP core idle/message delivery uses separate coalesced `on_main_thread` requests,
including when the editor is hidden, destroyed, or absent. Its worker only requests
host callbacks and is joined before the CLAP host proxy is destroyed. Initialization
fails explicitly if this worker cannot be created. Standalone idle/message delivery
uses the SWELL main-loop timer rather than a worker.

Clipboard ownership uses a dedicated X11 connection and worker, so selection
requests do not depend on either plugin's UI loop being dispatched. Text remains
available after an editor closes while its module is loaded. Failure to open the
clipboard connection or start its worker is logged and reported to the caller.
Clipboard replies use checked XCB requests on that dedicated connection. A paste
target disappearing during a transfer cancels the reply without invoking the
host's X11 error handler or replacing that handler process-wide.
Synchronous paste also services pending GTK events so that a host clipboard
owner on the same UI thread can answer. Editor close requests during this wait
are deferred until paste returns; a close cancels the wait.
Linux resource lookup resolves module symlinks before searching adjacent resources.
Flat binaries use an adjacent, binary-specific directory such as
`MyPlugin.clap.resources/ttf/Roboto-Regular.ttf` or
`MyPlugin.resources/ttf/Roboto-Regular.ttf` for a standalone app. COPY deployment
copies only that binary's resource directory; SYMLINK deployment finds resources
beside the resolved binary. When installing manually, keep the binary and its
`.resources` directory together. VST3 keeps the existing `Contents/Resources/`
bundle layout. The old shared `resources/` directory is no longer searched:
rebuild and redeploy Linux binaries to populate the isolated directories.
Resource inputs are link dependencies on Linux and macOS, so a resource-only
edit also runs post-build deployment. Linux deployment tests exercise both
Ninja and Make, flat files and VST3 bundles, and COPY/SYMLINK deployment, using
mutable inputs inside the test build directory.

For interactive standalone testing, `IPlugEffect --no-io` opens only the editor.
Linux preserves ALSA diagnostics; audio startup failures leave the editor available
for changing audio settings, while window/editor startup failures return a nonzero
exit status. Native Linux requires a working ALSA device or JACK server. On WSLg,
an installed JACK client library alone does not provide a server or a route to
Windows audio; that routing must be configured separately.

#### Validation before an upstream merge

Xvfb and the small host fixtures do not replace interactive DAW testing. Record
the host/version, display server, GPU/driver and display scale when checking:

- Multiple instances, repeated editor opening, docking, and project reload.
- Host-to-editor and editor-to-host keyboard focus, Compose/IME input, and GTK
  dialog focus restoration.
- XWayland and movement between monitors with different display scales.
- Real ALSA/JACK audio, device changes, processing restart, and state restore.

These interactive cases, Windows execution and hardware GPU behavior have not
been established by the headless Linux regression suite. Native Wayland, Skia
and WebView remain outside the implemented Linux support scope.

The Linux-support diff also contains two existing vendored patches that should
be reviewed separately from the platform implementation: SWELL INI writes now
report file/flush/rename failures through their return values, and NanoVG's
packed depth/stencil fallback uses the combined framebuffer attachment. Keep
their reproduction evidence and macOS/Windows validation with those patches;
Linux software-GL success alone does not establish cross-platform correctness.

### Linux OSC polling

Linux OSC does not create a worker timer. Call `ProcessOSC()` regularly from the
owning main thread, typically from the plugin's `OnIdle()`. When inheriting both
OSC interfaces, poll both:

```cpp
void MyPlugin::OnIdle()
{
  OSCSender::ProcessOSC();
  OSCReceiver::ProcessOSC();
}
```

All OSC operations, including sending, changing destinations or receive ports,
setting log callbacks, and destruction must use that same thread. Received
messages call `OnOSCMessage()` synchronously during polling; no receive callback runs
during construction or after destruction. A receive callback may change ports or
destinations, but must not destroy its OSC interface. Recursive polling is ignored
until the current tick finishes. Polling and socket operations are not
realtime-safe and must not run in `ProcessBlock()`. macOS and Windows retain their
existing automatic main-thread timers; they do not require manual polling.

The socket regression tests check explicit send/receive polling, callback thread
identity, device replacement/removal, recursive polling, and destruction/reopening.
They can also run without the GUI dependencies:

```bash
cmake -S Tests/Linux/OSC -B build/linux-osc -DCMAKE_BUILD_TYPE=Release
cmake --build build/linux-osc
ctest --test-dir build/linux-osc --output-on-failure
```

On macOS this exercises the Linux OSC polling code using macOS sockets, and also
checks that the native automatic timer still delivers messages. This is not a
substitute for running the suite on Linux.

## Build Commands

### Build All Targets

```bash
# Ninja
ninja

# Visual Studio / Xcode
cmake --build . --config Release
```

### Build Specific Targets

```bash
# Ninja
ninja MyPlugin-app       # Standalone app
ninja MyPlugin-vst2      # VST2
ninja MyPlugin-vst3      # VST3
ninja MyPlugin-clap      # CLAP
ninja MyPlugin-aax       # AAX
ninja MyPlugin-au        # AUv2 (macOS)

# Visual Studio / Xcode
cmake --build . --config Release --target MyPlugin-vst3
```

## Build Options

### Common Options

```bash
# Debug build (default)
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..

# Enable tracer/profiling
cmake -G Ninja -DIPLUG2_TRACER=ON ..
```

### macOS Options

```bash
# Universal binaries (Intel + Apple Silicon)
cmake -G Ninja -DIPLUG2_UNIVERSAL=ON ..

# Or set architectures directly
cmake -G Ninja -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" ..

# Deployment target (default: 13.0)
cmake -G Ninja -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0 ..
```

### Windows Options

Architecture is set via the `-A` flag with Visual Studio generator:

```bash
cmake -G "Visual Studio 17 2022" -A x64 ..      # 64-bit Intel/AMD
cmake -G "Visual Studio 17 2022" -A ARM64EC ..  # ARM64 with x64 emulation compatibility
```

### Debugging Options

Set a host application for debugging plugins:

```bash
# Windows (Visual Studio)
cmake -G "Visual Studio 17 2022" -A x64 -DIPLUG2_DEBUG_HOST="C:/Program Files/REAPER (x64)/reaper.exe" ..

# macOS (Xcode)
cmake -G Xcode -DIPLUG2_DEBUG_HOST="/Applications/REAPER.app" ..
```

| Variable | Description |
|----------|-------------|
| `IPLUG2_DEBUG_HOST` | Path to host application (REAPER, Ableton, etc.) |
| `IPLUG2_DEBUG_HOST_ARGS` | Command line arguments for the host |

**Windows:** Sets `VS_DEBUGGER_COMMAND` for plugin targets, allowing F5 to launch host and attach debugger. If REAPER is installed at `C:/Program Files/REAPER (x64)/reaper.exe`, it is automatically used as the default.

**macOS:** Generates Xcode schemes for plugin targets. If `IPLUG2_DEBUG_HOST` is set, schemes use that executable. Otherwise, schemes are generated with no executable set, allowing you to choose "Ask on Launch" or configure manually in Xcode.

### iOS Options

```bash
# iOS Device
cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DIPLUG2_IOS_PLATFORM=OS ..

# iOS Simulator
cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DIPLUG2_IOS_PLATFORM=SIMULATOR ..

# Deployment target (default: 14)
cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DIPLUG2_IOS_DEPLOYMENT_TARGET=15 ..
```

### visionOS Options

Requires Xcode 15+ with visionOS SDK.

```bash
# visionOS Device
cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DIPLUG2_IOS_PLATFORM=VISIONOS ..

# visionOS Simulator
cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DIPLUG2_IOS_PLATFORM=VISIONOS_SIMULATOR ..
```

| `IPLUG2_IOS_PLATFORM` | SDK | Description |
|-----------------------|-----|-------------|
| `OS` | iphoneos | iOS device (default) |
| `SIMULATOR` | iphonesimulator | iOS Simulator |
| `VISIONOS` | xros | visionOS device |
| `VISIONOS_SIMULATOR` | xrsimulator | visionOS Simulator |

### Web/Emscripten Options

```bash
# Configure with Emscripten toolchain
emcmake cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..

# Build WAM and Web targets
ninja MyPlugin-wam       # Audio processor (WASM)
ninja MyPlugin-web       # UI controller (WASM)
ninja MyPlugin-wam-dist  # Complete distribution package
```

## IGraphics Backends

iPlug2 provides `${IGRAPHICS_LIB}` which is set based on cache variables, allowing backend switching at configure time.

### Cache Variables

| Variable | Options | Default |
|----------|---------|---------|
| `IGRAPHICS_BACKEND` | `NANOVG`, `SKIA` | `NANOVG` |
| `IGRAPHICS_RENDERER` | `GL2`, `GL3`, `METAL`, `CPU` | `GL2` (Windows), `METAL` (macOS/iOS) |

### Configure at Build Time

```bash
# Default: NanoVG with GL2 (Windows) or Metal (macOS)
cmake -G Ninja ..

# Skia with OpenGL 3
cmake -G Ninja -DIGRAPHICS_BACKEND=SKIA -DIGRAPHICS_RENDERER=GL3 ..

# NanoVG with Metal (macOS)
cmake -G Ninja -DIGRAPHICS_BACKEND=NANOVG -DIGRAPHICS_RENDERER=METAL ..
```

### Available Targets

```cmake
# NanoVG (lightweight, included in iPlug2)
iPlug2::IGraphics::NanoVG        # GL2 (default)
iPlug2::IGraphics::NanoVG::GL3   # OpenGL 3
iPlug2::IGraphics::NanoVG::Metal # Metal (macOS/iOS)

# Skia (feature-rich, requires downloading dependencies)
iPlug2::IGraphics::Skia::GL3     # OpenGL 3
iPlug2::IGraphics::Skia::Metal   # Metal (macOS/iOS)
iPlug2::IGraphics::Skia::CPU     # Software rendering
```

## Project Structure

A minimal CMake-enabled iPlug2 project:

```
MyPlugin/
├── CMakeLists.txt
├── MyPlugin.cpp
├── MyPlugin.h
├── config.h
└── resources/
    ├── MyPlugin-macOS-Info.plist
    ├── MyPlugin-VST3-Info.plist
    ├── MyPlugin-CLAP-Info.plist
    ├── fonts/
    └── ...
```

## The `iplug_add_plugin` Macro

The `iplug_add_plugin()` macro simplifies plugin project configuration, reducing ~190 lines of boilerplate to ~15 lines.

### Basic Usage

```cmake
cmake_minimum_required(VERSION 3.14)
project(MyPlugin VERSION 1.0.0)

if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." CACHE PATH "iPlug2 root directory")
endif()

include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    MyPlugin.cpp
    MyPlugin.h
    resources/resource.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
)
```

### Syntax

```cmake
iplug_add_plugin(<name>
  SOURCES <file1> [file2 ...]
  [RESOURCES <file1> [file2 ...]]
  [WEB_RESOURCES <file1> [file2 ...]]
  [FORMATS <format1> [format2 ...]]
  [EXCLUDE_FORMATS <format1> [format2 ...]]
  [LINK <library1> [library2 ...]]
  [DEFINES <def1> [def2 ...]]
  [UI <IGRAPHICS|WEBVIEW|NONE>]
  [WAM_SITE_ORIGIN <origin>]
)
```

### Parameters

| Parameter | Description | Default |
|-----------|-------------|---------|
| `SOURCES` | Source files (.cpp, .h, .mm) | Required |
| `RESOURCES` | Fonts/images in bundle `Resources/`, or a binary-specific `.resources/` directory for flat Linux binaries | None |
| `WEB_RESOURCES` | Web UI files (HTML, JS, CSS) - goes into `Resources/web/` | None |
| `FORMATS` | Target formats to build | `ALL` |
| `EXCLUDE_FORMATS` | Formats to exclude | None |
| `LINK` | Additional libraries to link | None |
| `DEFINES` | Additional compile definitions | None |
| `UI` | UI framework: `IGRAPHICS`, `WEBVIEW`, or `NONE` | `IGRAPHICS` |
| `WAM_SITE_ORIGIN` | WAM site origin for CORS | `"/"` |

### Format Names

| Format | Description | Platforms |
|--------|-------------|-----------|
| `APP` | Standalone application | macOS, Windows, Linux |
| `VST3` | VST3 plugin | macOS, Windows, Linux |
| `CLAP` | CLAP plugin | macOS, Windows, Linux |
| `AAX` | AAX plugin (requires SDK) | macOS, Windows |
| `AU` | Audio Unit v2 | macOS only |
| `AUV3` | Audio Unit v3 | macOS, iOS |
| `WAM` | Web Audio Module | Emscripten only |

### Format Groups

| Group | Expands To | Notes |
|-------|------------|-------|
| `ALL` | APP, VST2, VST3, CLAP, AAX, AU, AUV3, WAM | Default. All formats |
| `ALL_PLUGINS` | VST2, VST3, CLAP, AAX, AU, AUV3, WAM | All plugin formats (no APP) |
| `ALL_DESKTOP` | APP, VST2, VST3, CLAP, AAX, AU, AUV3 | All desktop formats (no WAM) |
| `MINIMAL_PLUGINS` | VST3, CLAP, AU | Core plugin formats only |
| `DESKTOP` | APP, VST3, CLAP, AAX, AU | Desktop without VST2/AUv3 |
| `WEB` | WAM | Web formats only |

### Examples

**Standard IGraphics plugin:**

```cmake
iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    MyPlugin.cpp
    MyPlugin.h
    resources/resource.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
)
```

**Desktop plugin with AUv3 (no WAM):**

```cmake
iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    MyPlugin.cpp
    MyPlugin.h
    resources/resource.h
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
  FORMATS ALL_DESKTOP
)
```

**Minimal plugin (VST3, CLAP, AU only):**

```cmake
iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    MyPlugin.cpp
    MyPlugin.h
    resources/resource.h
  FORMATS MINIMAL_PLUGINS
)
```

**Plugin with extra libraries:**

```cmake
iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    MyPlugin.cpp
    MyPlugin.h
    resources/resource.h
  LINK iPlug2::Extras::Synth iPlug2::Extras::HIIR
)
```

**Plugin without UI (DSP-only):**

```cmake
iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    MyPlugin.cpp
    MyPlugin.h
    resources/resource.h
  UI NONE
  DEFINES NO_IGRAPHICS
  EXCLUDE_FORMATS WAM
)
```

**WebView UI plugin:**

```cmake
iplug_add_plugin(${PROJECT_NAME}
  SOURCES
    MyPlugin.cpp
    MyPlugin.h
    resources/resource.h
  WEB_RESOURCES
    resources/web/index.html
    resources/web/script.js
  UI WEBVIEW
  EXCLUDE_FORMATS WAM
)
```

Note: `WEB_RESOURCES` preserves subdirectory structure. Files in `resources/web/assets/` will be placed in `Resources/web/assets/` in the bundle.

See `Examples/IPlugEffect/CMakeLists.txt` for a complete working example.

## Build Output

### macOS

```
build/out/
├── MyPlugin.app/                    # Standalone (with embedded AUv3)
│   └── Contents/
│       ├── Frameworks/
│       │   └── MyPluginAU.framework/
│       └── PlugIns/
│           └── MyPlugin.appex/
├── MyPlugin.vst3/
├── MyPlugin.clap/
├── MyPlugin.aaxplugin/
├── MyPlugin.component/              # AUv2
├── MyPluginAU.framework/            # AUv3 framework
└── MyPlugin.appex/                  # AUv3 appex
```

### Windows

```
build/out/
├── MyPlugin.exe                     # Standalone app
├── MyPlugin.vst3/
│   └── Contents/
│       └── x86_64-win/              # or arm64ec-win
│           └── MyPlugin.vst3
├── MyPlugin.clap
└── MyPlugin.aaxplugin/
    └── Contents/
        └── x64/                     # or ARM64EC
            └── MyPlugin.aaxplugin
```

### Linux

```
build/out/
├── MyPlugin                         # Standalone app (executable)
├── MyPlugin.vst3/
│   └── Contents/
│       └── x86_64-linux/
│           └── MyPlugin.so
└── MyPlugin.clap
```

### Web

```
build/out/MyPlugin/
├── scripts/
│   ├── MyPlugin-wam.js     # WAM processor loader
│   ├── MyPlugin-wam.wasm   # Audio processing module
│   ├── MyPlugin-web.js     # Web controller loader
│   └── MyPlugin-web.wasm   # UI module
├── index.html              # Test page
├── descriptor.json         # WAM metadata
└── styles/                 # CSS assets
```

## Deployment

### Default Deployment Paths

Plugins are automatically deployed (symlinked or copied) to standard locations:

**macOS:**
- VST2: `~/Library/Audio/Plug-Ins/VST`
- VST3: `~/Library/Audio/Plug-Ins/VST3`
- CLAP: `~/Library/Audio/Plug-Ins/CLAP`
- AUv2: `~/Library/Audio/Plug-Ins/Components`
- AAX: `/Library/Application Support/Avid/Audio/Plug-Ins`
- APP: `~/Applications`

**Linux:**
- VST3: `~/.vst3`
- CLAP: `~/.clap`
- APP: remains in build directory (no default deploy path)

**Windows:**
- VST2: `%PROGRAMFILES%\VstPlugins`
- VST3: `%LOCALAPPDATA%\Programs\Common\VST3` (per-user, no admin required)
- CLAP: `%LOCALAPPDATA%\Programs\Common\CLAP` (per-user, no admin required)
- AAX: `%COMMONPROGRAMFILES%\Avid\Audio\Plug-Ins`

> **Note:** VST3/CLAP use per-user paths by default (spec-compliant, hosts scan both locations). For system-wide installation, use `-DIPLUG_VST3_DEPLOY_PATH="%COMMONPROGRAMFILES%\VST3"`.

### Deployment Options

`iplug_configure_target()` retains automatic deployment for directly configured
targets. When adding resources after configuration, pass `DEFER_DEPLOYMENT` and
call `iplug_deploy_target()` after adding the resources. `iplug_add_plugin()` does
this automatically so deployment runs once, after resource copying.

```bash
# Disable automatic deployment
cmake -G Ninja -DIPLUG_DEPLOY_PLUGINS=OFF ..

# Use symlink instead of copy (default is COPY)
cmake -G Ninja -DIPLUG_DEPLOY_METHOD=SYMLINK ..

# Custom deployment paths
cmake -G Ninja -DIPLUG_VST3_DEPLOY_PATH=/custom/path ..
```

## Platform-Specific Notes

### macOS

**Code Signing (required for AUv3):**

```bash
# Sign from innermost to outermost
codesign -s - out/MyPlugin.app/Contents/Frameworks/MyPluginAU.framework
codesign -s - out/MyPlugin.app/Contents/PlugIns/MyPlugin.appex
codesign -s - out/MyPlugin.app

# Run app to register AUv3
open out/MyPlugin.app
```

> **Warning:** Ad-hoc signing (`-s -`) is for **local development only**. Ad-hoc signed apps cannot be distributed, will not run on other machines, and are blocked by Gatekeeper. For distribution, you must sign with a valid Developer ID certificate and notarize the app (see below).

**Distribution signing:**

```bash
codesign -s "Developer ID Application: Your Name (TEAM_ID)" out/MyPlugin.app
xcrun notarytool submit out/MyPlugin.app --apple-id your@email.com --team-id TEAM_ID --wait
xcrun stapler staple out/MyPlugin.app
```

**Validating Audio Units:**

```bash
auval -a                              # List all registered AUs
auval -v aufx <subtype> <manufacturer>  # Validate effect
auval -v aumu <subtype> <manufacturer>  # Validate instrument
```

### Windows

- MSVC uses static runtime (`/MT` for Release, `/MTd` for Debug)
- Multi-config generators (Visual Studio) require `--config` flag when building
- Use ARM64EC for ARM64 Windows (provides x64 plugin compatibility)

### iOS

- Minimum deployment target: iOS 14 (configurable)
- Device builds require code signing
- AUv3 is embedded in the standalone app

### visionOS

- Requires Xcode 15+ with visionOS SDK installed
- Minimum deployment target: 1.0

### Web/Emscripten

WAM plugins consist of two WASM modules:
1. **WAM Processor** (`-wam`) - DSP/audio (runs in AudioWorklet)
2. **Web Controller** (`-web`) - UI/graphics (runs in main thread)

**Testing:**

WAM requires HTTPS with specific headers for SharedArrayBuffer support. Use [mkcert](https://github.com/FiloSottile/mkcert) to create local certificates:

```bash
# Install mkcert and create local CA
brew install mkcert  # macOS
mkcert -install
mkcert localhost

# Serve with HTTPS (requires a server that sets COOP/COEP headers)
```

Alternatively, use `emrun` which handles the required headers:

```bash
cd build/out/wam
emrun --no_emrun_detect index.html
```

**Notes:**
- WAM SDK must be downloaded via `Dependencies/download-iplug-sdks.sh`

### Linux

- Uses X11 for windowing, NanoVG/GL3 for graphics, ALSA/JACK/PulseAudio for audio
- GCC and Clang both supported; requires C++17
- Supported formats: APP (standalone), VST3, CLAP
- Plugins are deployed to `~/.vst3/` (VST3) and `~/.clap/` (CLAP) by default
- Both Ninja and Unix Makefiles generators work (`linux-ninja` and `linux-make` presets)
- `pkg-config` is used to find system libraries (freetype, fontconfig, X11, GTK3, ALSA, etc.)
- The Skia backend is not yet supported on Linux; use NanoVG
- WebView is not implemented on Linux, including embedded `IWebViewControl`.
  The examples build skips IPlugWebUI, IPlugP5js, IPlugSvelteUI,
  IPlugReaperExtensionWebUI, and IPlugOSCEditor with a status message.
  Explicit `UI WEBVIEW` requests (including independent WebView example builds)
  fail at configure time. `iPlug2::Extras::IWebViewControl` is not provided on
  Linux; linking it is a configuration error. Use `UI IGRAPHICS` or `UI NONE`.
  No WebKitGTK backend is enabled by installing GTK or WebKitGTK packages.
  The Linux regression suite verifies these configuration restrictions and checks
  CLAP attach/show failures with a non-IGraphics editor that cannot open a window.

## Troubleshooting

### General

- **Missing symbols**: Verify all required frameworks/libraries are linked
- **Source files not found**: Check paths in `target_sources()`

### macOS

- **AUv3 not registering**: Ensure app is signed and has been run at least once
- **"Bundle format unrecognized"**: Check PkgInfo files exist; use `cp -R` for frameworks

### Windows

- **Link errors with runtime**: Ensure all libraries use same runtime (`/MT` vs `/MD`)
- **Missing DLLs**: Use static runtime or deploy required DLLs

## Examples

The following examples include CMakeLists.txt and are built automatically from the root:

**Standard IGraphics:**
IPlugEffect, IPlugInstrument, IPlugControls, IPlugConvoEngine, IPlugChunks, IPlugDrumSynth, IPlugMidiEffect, IPlugOSCEditor, IPlugResponsiveUI, IPlugSideChain, IPlugSurroundEffect, IPlugVisualizer

**WebView:**
IPlugWebUI, IPlugP5js, IPlugSvelteUI

**Swift/Cocoa (Xcode generator only):**
IPlugSwiftUI, IPlugCocoaUI - Automatically included when using `-G Xcode` on macOS.

**REAPER:**
IPlugReaperExtension, IPlugReaperPlugin

**Tests:**
IGraphicsTest, IGraphicsStressTest, MetaParamTest

## See Also

- [iPlug2 Documentation](https://iplug2.github.io/docs)
- [CMake Documentation](https://cmake.org/documentation/)
- [Examples/IPlugEffect/CMakeLists.txt](../Examples/IPlugEffect/CMakeLists.txt) - Reference CMake template
