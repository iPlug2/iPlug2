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

Fast, cross-platform. Works on macOS and Windows.

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
cmake -G Ninja ..                              # Ninja (macOS/Windows)
cmake -G "Visual Studio 17 2022" -A x64 ..     # Visual Studio
cmake -G Xcode ..                              # Xcode

# Build
cmake --build . --config Release
```

## Supported Platforms & Targets

| Format | Target Suffix | macOS | Windows | iOS | visionOS | Web |
|--------|---------------|-------|---------|-----|----------|-----|
| Standalone App | `-app` | ✓ | ✓ | ✓ | ✓ | - |
| VST2 | `-vst2` | ✓ | ✓ | - | - | - |
| VST3 | `-vst3` | ✓ | ✓ | - | - | - |
| CLAP | `-clap` | ✓ | ✓ | - | - | - |
| AAX | `-aax` | ✓ | ✓ | - | - | - |
| AUv2 | `-au` | ✓ | - | - | - | - |
| AUv3 | `AU-framework`, `AUv3-appex` | ✓ | - | ✓ | ✓ | - |
| WAM | `-wam`, `-web` | - | - | - | - | ✓ |

**Notes:**
- VST2 requires the VST2 SDK (deprecated, no longer distributed by Steinberg)
- VST3 requires the VST3 SDK
- CLAP requires the CLAP SDK
- AAX requires the AAX SDK from Avid
- AUv3 is embedded in the standalone APP on macOS
- Linux is not yet supported

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

# Deployment target (default: 10.13)
cmake -G Ninja -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 ..
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
| `RESOURCES` | Bundle resources (fonts, images) - goes into `Resources/` | None |
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
| `APP` | Standalone application | macOS, Windows |
| `VST3` | VST3 plugin | macOS, Windows |
| `CLAP` | CLAP plugin | macOS, Windows |
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

**Windows:**
- VST2: `%PROGRAMFILES%\VstPlugins`
- VST3: `%LOCALAPPDATA%\Programs\Common\VST3` (per-user, no admin required)
- CLAP: `%LOCALAPPDATA%\Programs\Common\CLAP` (per-user, no admin required)
- AAX: `%COMMONPROGRAMFILES%\Avid\Audio\Plug-Ins`

> **Note:** VST3/CLAP use per-user paths by default (spec-compliant, hosts scan both locations). For system-wide installation, use `-DIPLUG_VST3_DEPLOY_PATH="%COMMONPROGRAMFILES%\VST3"`.

### Deployment Options

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

#### AUv3 with CMake (Xcode Generator)

AUv3 requires proper code signing with a Development Team to register with the system. CMake handles this automatically when you provide your Development Team ID:

```bash
cmake -G Xcode -DIPLUG2_DEVELOPMENT_TEAM=YOUR_TEAM_ID ..
cmake --build . --config Debug --target MyPlugin-app
```

The `IPLUG2_DEVELOPMENT_TEAM` variable enables:
- **Hardened runtime** (`-o runtime`) on all binaries
- **Entitlements** for the appex (sandbox, microphone, etc.)
- **Proper signing order** (appex with entitlements, then app container)
- **Launch Services registration** for AUv3 discovery

**Finding your Team ID:**

```bash
# List available signing identities
security find-identity -v -p codesigning

# Team ID is the 10-character code in parentheses, e.g.:
# "Apple Development: Your Name (XXXXXXXX)" YYYYYYYYYY
#                                            ^^^^^^^^^^ Team ID
```

**Requirements for AUv3 registration:**

1. **Entitlements file**: `projects/MyPlugin-macOS.entitlements` must exist with at minimum:
   ```xml
   <key>com.apple.security.app-sandbox</key>
   <true/>
   ```

2. **Hardened runtime**: Required for system plugin discovery

3. **Matching bundle identifiers**: The appex's `AudioComponentBundle` must match the framework's `CFBundleIdentifier`

**Verifying AUv3 is registered:**

```bash
# List all registered Audio Units
auval -a

# Check plugin-specific registration
auval -v aumu <subtype> <manufacturer>  # Instruments
auval -v aufx <subtype> <manufacturer>  # Effects
```

If your AUv3 doesn't appear in `auval -a`, check:
- App has been launched at least once
- Signatures are valid: `codesign --verify --deep --strict MyPlugin.app`
- Appex has entitlements: `codesign -d --entitlements - MyPlugin.app/Contents/PlugIns/MyPlugin.appex`

#### Manual Code Signing (without Development Team)

For local testing without a Development Team:

```bash
# Sign from innermost to outermost
codesign -s - out/MyPlugin.app/Contents/Frameworks/MyPluginAU.framework
codesign -s - out/MyPlugin.app/Contents/PlugIns/MyPlugin.appex
codesign -s - out/MyPlugin.app

# Run app to register AUv3
open out/MyPlugin.app
```

> **Warning:** Ad-hoc signing (`-s -`) is for **local development only**. Ad-hoc signed apps cannot be distributed, will not run on other machines, and are blocked by Gatekeeper. For distribution, you must sign with a valid Developer ID certificate and notarize the app (see below).

#### Distribution Signing

```bash
codesign -s "Developer ID Application: Your Name (TEAM_ID)" out/MyPlugin.app
xcrun notarytool submit out/MyPlugin.app --apple-id your@email.com --team-id TEAM_ID --wait
xcrun stapler staple out/MyPlugin.app
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

Linux is not yet supported. CMake configuration will show an error message.

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
