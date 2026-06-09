# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

This is the iPlug2 framework repository. Upstream: https://github.com/iPlug2/iPlug2.git

## Instructions

- Be concise. Assume the user knows audio programming concepts but not iPlug2 specifically.
- Check current directory before running scripts; return to repo root after.

## iPlug2 Overview

Cross-platform C++ audio plugin framework with two main components:

1. **IPlug** - Core plugin abstraction layer
2. **IGraphics** - GUI toolkit with multiple backends (NanoVG - lightweight/fast, Skia - higher quality)

**Plugin Formats:** CLAP, VST3, AUv2/AUv3, AAX, WAM (VST2 deprecated)
**Extra Formats:** Standalone App, REAPER extensions
**Platforms:** macOS (Metal), Windows (GL2), Linux (GL3/X11), iOS/visionOS, Web (WASM/emscripten)

## Build

CMake ≥3.21 required. Uses `CMakePresets.json` — outputs to `build/<presetName>/`.

```bash
# Configure + build (pick a preset)
cmake --preset macos-ninja          # macOS (NanoVG/Metal, Ninja)
cmake --preset macos-xcode          # macOS (Xcode project)
cmake --preset macos-xcode-universal  # macOS universal arm64+x86_64
cmake --preset linux-ninja          # Linux (NanoVG/GL3, Ninja)
cmake --preset linux-make           # Linux (Unix Makefiles)
cmake --preset windows-vs2022       # Windows x64 (NanoVG/GL2)
cmake --preset windows-vs2022-arm64ec  # Windows ARM64EC
cmake --preset ios-xcode            # iOS device
cmake --preset wam                  # WebAssembly (emscripten)

cmake --build --preset macos-ninja
cmake --build --preset linux-ninja
```

Build a single format target (Ninja generator):
```bash
ninja MyPlugin-app    # standalone app
ninja MyPlugin-vst3
ninja MyPlugin-clap
ninja MyPlugin-au     # AUv2 (macOS)
ninja MyPlugin-wam    # WebAssembly DSP module
# Or: cmake --build . --config Release --target MyPlugin-vst3
```

Key CMake variables:
- `IGRAPHICS_BACKEND`: `NANOVG` (default) or `SKIA`
- `IGRAPHICS_RENDERER`: `METAL` (macOS), `GL2` (Windows), `GL3` (Linux)
- `IPLUG2_UNIVERSAL`: `ON` for arm64+x86_64 fat binaries (macOS)
- `IPLUG2_DEBUG_HOST` / `IPLUG2_DEBUG_HOST_ARGS`: host app path so F5/Xcode scheme launches the DAW and attaches the debugger (REAPER auto-detected on Windows)

Skia requires pre-built deps; NanoVG does not. Optional SDKs (WAM, VST2, AAX) download via `Dependencies/download-iplug-sdks.sh`. See `Scripts/ci/build_deps.yml` for the CI flow.

**Linux build deps (Ubuntu/Debian):**
```bash
sudo apt-get install -y ninja-build pkg-config \
  libfreetype-dev libfontconfig-dev \
  libx11-dev libxi-dev libxext-dev libxrender-dev \
  libxrandr-dev libxfixes-dev libxcursor-dev \
  libgl-dev libgtk-3-dev \
  libasound2-dev libjack-jackd2-dev libpulse-dev
```

### Defining a Plugin: `iplug_add_plugin`

A plugin's `CMakeLists.txt` includes `iPlug2.cmake`, calls `find_package(iPlug2 REQUIRED)`, then:
```cmake
iplug_add_plugin(<name>
  SOURCES <files...>                # .cpp/.h/.mm (required)
  [RESOURCES <files...>]            # fonts/images → bundle Resources/
  [WEB_RESOURCES <files...>]        # HTML/JS/CSS → Resources/web/ (subdirs preserved)
  [FORMATS <group|format...>]       # default ALL
  [EXCLUDE_FORMATS <format...>]
  [LINK <libs...>]                  # e.g. iPlug2::Extras::Synth, iPlug2::Extras::HIIR
  [DEFINES <defs...>]
  [UI <IGRAPHICS|WEBVIEW|NONE>]     # default IGRAPHICS
  [WAM_SITE_ORIGIN <origin>])
```

Format names: `APP VST3 CLAP AAX AU AUV3 WAM` (AU = AUv2, macOS only; AUV3 = macOS/iOS; AAX needs SDK).
Format groups: `ALL` (default), `ALL_PLUGINS` (no APP), `ALL_DESKTOP` (no WAM), `MINIMAL_PLUGINS` (VST3+CLAP+AU), `DESKTOP`, `WEB`.

IGraphics backend link targets (pass to `LINK`):
```
iPlug2::IGraphics::NanoVG / ::NanoVG::GL3 / ::NanoVG::Metal
iPlug2::IGraphics::Skia::GL3 / ::Skia::Metal / ::Skia::CPU
```

### Deployment

Builds auto-deploy (symlink/copy) to standard scan paths. Per-user by default (no admin):
- **macOS:** `~/Library/Audio/Plug-Ins/{VST3,CLAP,Components}`, AUv2 = `Components`, APP = `~/Applications`
- **Linux:** `~/.vst3`, `~/.clap`; APP stays in build dir
- **Windows:** `%LOCALAPPDATA%\Programs\Common\{VST3,CLAP}`

Override with `-DIPLUG_VST3_DEPLOY_PATH=...` (and `IPLUG_CLAP_DEPLOY_PATH`, etc.) for system-wide installs.

## Code Formatting

```bash
Scripts/run_clang_format.sh         # formats IPlug/ and IGraphics/ in-place
```

`.clang-format` is at repo root. Style: 2-space indent, allman braces, 100-char column limit.

## Testing & Validation

Tests in `Tests/` are standalone plugin projects (not unit tests) — build and run them like any plugin:
- `IGraphicsTest` — exercises IGraphics rendering controls
- `IGraphicsStressTest` — stress tests rendering performance
- `MetaParamTest` — tests parameter meta-system

Each test project has its own `CMakeLists.txt` and follows the same `Plugin.cpp/Plugin.h/config.h` structure.

Plugin validation (after building):
```bash
Scripts/validate_audiounit.sh <path/to/config.h>           # auval (AUv2)
Scripts/validate_audiounit.sh <path/to/config.h> leaks     # leak test
Scripts/validate_audiounit.sh <path/to/config.h> rtsafe    # real-time safety
# For VST3: use pluginval manually
# For AUv2: clear caches if plugin doesn't load: Scripts/clear_audiounit_caches.command
```

## Architecture

### IPlug Class Hierarchy

```
IPlugAPIBase          — parameter storage, state serialization, host communication
  └── IPlugPluginBase  — common plugin lifecycle (init, presets, MIDI config)
        └── IPlugProcessor  — audio processing (ProcessBlock, transport, I/O layout)
```

Format-specific implementations live in `IPlug/<FORMAT>/` (CLAP, VST3, AUv2, AUv3, AAX, APP, WEB, ReaperExt). Each format subdir has `IPlug<FORMAT>.h/.cpp` that inherits from IPlugProcessor and wires up the host API.

`IPlugDelegate_select.h` — compile-time macro selects the right concrete class based on `IPLUG_EDITOR` define, bridging processor and editor.

### Including iPlug2 in a Plugin

Every plugin's `Plugin.h` must include:
```cpp
#include "IPlug_include_in_plug_hdr.h"
```
Every plugin's `Plugin.cpp` must include:
```cpp
#include "IPlug_include_in_plug_src.h"
```
These macros pull in format-specific glue depending on the active build target.

### Thread Model

- **Audio thread**: `ProcessBlock()` — realtime-safe, no allocations/locks/I/O
- **UI thread**: IGraphics callbacks, timer callbacks
- **Audio → UI**: Use `ISender<T>` (`IPlug/ISender.h`) — lock-free queue, `TransmitData()` on audio thread, `ProcessTimer()` on UI thread
- **UI → audio**: `IPlugQueue<T>` for lock-free parameter change messages

### IGraphics

`IGraphics.h/.cpp` is the main drawing surface. `IControl` (`IGraphics/IControl.h`) is the base for all UI widgets. Controls attach to parameters via param index; layout is done in a lambda passed to `GetUI()->LoadLayout()` (or inline in the plugin constructor).

`IGraphics_select.h` selects the backend (NanoVG or Skia) at compile time via `IGRAPHICS_NANOVG` / `IGRAPHICS_SKIA` defines.

Platform-specific windowing: `IGraphics/Platforms/` (macOS/iOS in ObjC, Windows Win32, Linux X11/GTK).

### IPlug/Extras — DSP Building Blocks

Ready-to-use DSP utilities in `IPlug/Extras/`:
- `ADSREnvelope.h`, `LFO.h`, `Oscillator.h` — generators
- `SVF.h`, `DCBlocker.h`, `NoiseGate.h` — filters/dynamics
- `Oversampler.h`, `LanczosResampler.h`, `RealtimeResampler.h` — resampling
- `Smoothers.h`, `Easing.h` — parameter smoothing
- `NChanDelay.h` — multichannel delay line
- `Synth/` — voice/note management helpers

### WASM / Web Builds

Web builds use a **split DSP/UI architecture** (not the legacy WAM SDK):
- **DSP module** (`MyPlugin-wam`) runs in an AudioWorklet (real-time audio thread), embedded as BASE64
- **UI module** (`MyPlugin-web`) runs on the main thread (IGraphics), loads async into a Shadow DOM
- Communication: `postMessage` for params/MIDI; `SharedArrayBuffer` for low-latency viz data (needs COOP/COEP headers — falls back to `postMessage` without them)
- Supports multiple instances in one AudioContext
- Build with `emcmake cmake -G Ninja ...` then `ninja MyPlugin-wam MyPlugin-web`; full docs in `Documentation/wasm.md`

### ARM64EC (Windows on ARM)

Native ARM64 perf while staying x64-host compatible. Use `windows-vs2022-arm64ec` preset. **Skia is not supported on ARM64EC — use NanoVG.** ARM64EC binaries can't run on x64 Windows (need ARM64 hardware/emulation to test). See `Documentation/arm64ec.md`.

### Out-of-Source Project Layout

A typical plugin project (see `Examples/`) keeps source at root and splits the rest into `config/` (build configuration), `projects/` (generated VS/Xcode projects), `resources/` (fonts, images, platform UI files, icons), and `scripts/`. The CMake build is the canonical entry point; per-platform IDE projects are generated/secondary.

### Key Concepts

- Three main files per project: `Plugin.cpp`, `Plugin.h`, `config.h`
- Parameters: fixed count at compile time, indexed by enum, non-normalized values; access via `GetParam(kIndex)->Value()`
- IGraphics UIs typically built in a lambda in the plugin constructor
- Controls grouped and tagged on `Attach()`
- IGraphics is only one UI option — SwiftUI, WebView variants exist in `Examples/`
- NanoVG backend needs no external deps; Skia requires downloaded prebuilt libs

## Code Style

- 2-space indentation, no tabs
- Unix line endings (except Windows-specific files)
- Member variables: `mCamelCase`
- Pointer args: `pCamelCase`
- Internal methods: `_methodName`
- C++17: `override`, `final`, `auto`, `std::optional`, `std::string_view`
- Avoid STL in core code; prefer WDL alternatives (`WDL_String`, `WDL_PtrList`, etc.)

## Resources

- [API Docs](https://iplug2.github.io/docs) | [Wiki](https://github.com/iPlug2/iPlug2/wiki)
- [iPlug2OOS](https://github.com/iPlug2/iPlug2OOS) — recommended out-of-source project template (2025+)
- Standalone app debug builds include a debug menu with screenshot capability
