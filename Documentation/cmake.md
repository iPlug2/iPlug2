# Building iPlug2 Projects with CMake

This document covers building iPlug2 plugin projects using CMake, supporting both Ninja and Xcode generators.

## Prerequisites

- **CMake** 3.14 or later
- **Ninja** (recommended) or Xcode
- **Xcode Command Line Tools** (macOS)

```bash
# Install via Homebrew
brew install cmake ninja
```

## Quick Start

```bash
# From your plugin project directory
mkdir build && cd build
cmake -G Ninja ..
ninja
```

## Supported Plugin Formats

| Format | Target Suffix | Platform | Notes |
|--------|---------------|----------|-------|
| Standalone App | `-app` | macOS, Windows | Hosts AUv3 on macOS |
| VST3 | `-vst3` | macOS, Windows | |
| CLAP | `-clap` | macOS, Windows | |
| AUv2 | `-au` | macOS | Audio Unit v2 |
| AUv3 | `AU-framework`, `AUv3-appex` | macOS | Embedded in APP |

## Build Commands

### Build All Targets
```bash
ninja                    # Build everything
```

### Build Specific Targets
```bash
ninja MyPlugin-app       # Standalone app (includes AUv3 on macOS)
ninja MyPlugin-vst3      # VST3 plugin
ninja MyPlugin-clap      # CLAP plugin
ninja MyPlugin-au        # AUv2 plugin (macOS)
```

### Using Xcode Generator
```bash
cmake -G Xcode ..
cmake --build . --config Release
# Or open the .xcodeproj in Xcode
```

## Build Options

### Universal Binaries (macOS)

Build for both Intel and Apple Silicon:

```bash
# Using the IPLUG2_UNIVERSAL option
cmake -G Ninja -DIPLUG2_UNIVERSAL=ON ..

# Or set architectures directly
cmake -G Ninja -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" ..
```

### Deployment Target (macOS)

Set minimum macOS version (default: 10.13):

```bash
cmake -G Ninja -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 ..
```

### Build Type

```bash
# Debug build (default)
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
```

## Project Structure

A minimal CMake-enabled iPlug2 project:

```
MyPlugin/
├── CMakeLists.txt           # CMake configuration
├── MyPlugin.cpp             # Plugin implementation
├── MyPlugin.h               # Plugin header
├── config.h                 # Plugin configuration
└── resources/
    ├── MyPlugin-macOS-Info.plist
    ├── MyPlugin-VST3-Info.plist
    ├── MyPlugin-CLAP-Info.plist
    ├── MyPlugin-AU-Info.plist
    ├── MyPlugin-macOS-AUv3-Info.plist
    ├── MyPlugin-macOS-AUv3Framework-Info.plist
    ├── AUv3Framework.h      # Or MyPluginAU.h
    ├── IPlugAUViewController_vMyPlugin.xib
    └── fonts/
        └── Roboto-Regular.ttf
```

## CMakeLists.txt Template

```cmake
cmake_minimum_required(VERSION 3.14)
project(MyPlugin VERSION 1.0.0)

# Find iPlug2
if(NOT DEFINED IPLUG2_DIR)
  set(IPLUG2_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../iPlug2" CACHE PATH "iPlug2 root")
endif()
include(${IPLUG2_DIR}/iPlug2.cmake)
find_package(iPlug2 REQUIRED)

# Project paths
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PLUG_RESOURCES_DIR ${PROJECT_DIR}/resources)

# Source files
set(SOURCE_FILES
  MyPlugin.cpp
  MyPlugin.h
  resources/resource.h
)

# Base library for shared code
add_library(_${PROJECT_NAME}-base INTERFACE)
iplug_add_target(_${PROJECT_NAME}-base INTERFACE
  INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources
  LINK iPlug2::IPlug
)

# Resource files
set(FONT_FILES ${PLUG_RESOURCES_DIR}/fonts/Roboto-Regular.ttf)

function(iplug_add_plugin_resources target)
  target_sources(${target} PRIVATE ${FONT_FILES})
  set_source_files_properties(${FONT_FILES} PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
  )
endfunction()

# ============================================================================
# APP Target (Standalone Application)
# ============================================================================
add_executable(${PROJECT_NAME}-app ${SOURCE_FILES})
iplug_add_target(${PROJECT_NAME}-app PUBLIC
  LINK iPlug2::APP ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
)
iplug_configure_target(${PROJECT_NAME}-app APP ${PROJECT_NAME})
target_sources(${PROJECT_NAME}-app PRIVATE ${FONT_FILES})
set_source_files_properties(${FONT_FILES} PROPERTIES
  MACOSX_PACKAGE_LOCATION Resources
)

# ============================================================================
# VST3 Target
# ============================================================================
add_library(${PROJECT_NAME}-vst3 MODULE ${SOURCE_FILES})
iplug_add_target(${PROJECT_NAME}-vst3 PUBLIC
  LINK iPlug2::VST3 ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
)
iplug_configure_target(${PROJECT_NAME}-vst3 VST3 ${PROJECT_NAME})
iplug_add_plugin_resources(${PROJECT_NAME}-vst3)

# ============================================================================
# CLAP Target
# ============================================================================
add_library(${PROJECT_NAME}-clap MODULE ${SOURCE_FILES})
iplug_add_target(${PROJECT_NAME}-clap PUBLIC
  LINK iPlug2::CLAP ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
)
iplug_configure_target(${PROJECT_NAME}-clap CLAP ${PROJECT_NAME})
iplug_add_plugin_resources(${PROJECT_NAME}-clap)

# ============================================================================
# AUv2 Target (macOS only)
# ============================================================================
if(APPLE)
  add_library(${PROJECT_NAME}-au MODULE ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}-au PUBLIC
    LINK iPlug2::AUv2 ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}-au AUv2 ${PROJECT_NAME})
  iplug_add_plugin_resources(${PROJECT_NAME}-au)
endif()

# ============================================================================
# AUv3 Targets (macOS only) - Framework + Appex embedded in APP
# ============================================================================
if(APPLE)
  # Framework containing AUv3 plugin code
  add_library(${PROJECT_NAME}AU-framework SHARED ${SOURCE_FILES})
  iplug_add_target(${PROJECT_NAME}AU-framework PUBLIC
    LINK iPlug2::AUv3 ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
  )
  iplug_configure_target(${PROJECT_NAME}AU-framework AUv3Framework ${PROJECT_NAME})
  iplug_add_plugin_resources(${PROJECT_NAME}AU-framework)

  # App Extension (appex)
  iplug_get_auv3appex_source(${PROJECT_NAME} APPEX_SOURCE)
  add_executable(${PROJECT_NAME}AUv3-appex ${APPEX_SOURCE})
  iplug_configure_target(${PROJECT_NAME}AUv3-appex AUv3Appex ${PROJECT_NAME})

  # Embed AUv3 in APP
  iplug_embed_auv3_in_app(${PROJECT_NAME}-app ${PROJECT_NAME})
endif()
```

## Build Output

Built plugins are placed in the `build/out/` directory:

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
├── MyPlugin.component/              # AUv2
├── MyPluginAU.framework/            # AUv3 framework (standalone copy)
└── MyPlugin.appex/                  # AUv3 appex (standalone copy)
```

## Code Signing

CMake builds require manual code signing for AUv3 registration:

```bash
# Sign from innermost to outermost
codesign -s - out/MyPlugin.app/Contents/Frameworks/MyPluginAU.framework
codesign -s - out/MyPlugin.app/Contents/PlugIns/MyPlugin.appex
codesign -s - out/MyPlugin.app

# Run app to register AUv3
open out/MyPlugin.app

# Verify registration
auval -a | grep MyPlugin
```

For distribution, use a valid Developer ID instead of `-s -` (ad-hoc).

## Validating Audio Units

```bash
# List all registered AUs
auval -a

# Validate specific AU (effect)
auval -v aufx <subtype> <manufacturer>

# Validate instrument
auval -v aumu <subtype> <manufacturer>

# Example
auval -v aufx Ipef Acme
```

## IGraphics Backends

iPlug2 provides a convenience variable `${IGRAPHICS_LIB}` that is automatically set based on cache variables. This allows switching backends at configure time without modifying CMakeLists.txt.

### Using the Convenience Variable

```cmake
# In your CMakeLists.txt - uses whatever backend is configured
LINK iPlug2::APP ${IGRAPHICS_LIB} _${PROJECT_NAME}-base
```

### Configuring at Build Time

```bash
# Default: NanoVG with GL2 (Windows) or Metal (macOS)
cmake -G Ninja ..

# Use Skia with GL3
cmake -G Ninja -DIGRAPHICS_BACKEND=SKIA -DIGRAPHICS_RENDERER=GL3 ..

# Use NanoVG with Metal (macOS)
cmake -G Ninja -DIGRAPHICS_BACKEND=NANOVG -DIGRAPHICS_RENDERER=METAL ..
```

### Cache Variables

| Variable | Options | Default |
|----------|---------|---------|
| `IGRAPHICS_BACKEND` | `NANOVG`, `SKIA` | `NANOVG` |
| `IGRAPHICS_RENDERER` | `GL2`, `GL3`, `METAL`, `CPU` | `GL2` (Windows), `METAL` (macOS/iOS) |

### Available Targets

You can also link directly to specific backend targets:

```cmake
# NanoVG backends
iPlug2::IGraphics::NanoVG        # GL2 (default)
iPlug2::IGraphics::NanoVG::GL3   # OpenGL 3
iPlug2::IGraphics::NanoVG::Metal # Metal (macOS/iOS)

# Skia backends (requires dependencies)
iPlug2::IGraphics::Skia::GL3     # OpenGL 3
iPlug2::IGraphics::Skia::Metal   # Metal (macOS/iOS)
iPlug2::IGraphics::Skia::CPU     # Software rendering
```

### Backend Notes

- **NanoVG**: Lightweight, fast, included in iPlug2. Best for simple UIs.
- **Skia**: Feature-rich, requires downloading dependencies. Better for complex graphics.

## Troubleshooting

### AUv3 not registering
1. Ensure app is properly signed
2. Run the app at least once to register extensions
3. Check `auval -a` output
4. Verify bundle identifiers are hierarchical (app > appex > framework)

### "Bundle format unrecognized"
- Ensure PkgInfo files exist in bundles
- Framework symlinks must be preserved (use `cp -R`, not `copy_directory`)

### Missing symbols
- Check that all required frameworks are linked
- Verify source files are added to the target

## Platform Notes

### macOS
- Minimum deployment target: 10.13 (configurable)
- AUv3 requires the host app to be run for registration
- Framework uses versioned bundle structure (Versions/A/Current)

### Windows
- Use Visual Studio generator or Ninja with MSVC
- VST3 and CLAP supported
- No AU support

## Examples

The following examples include CMakeLists.txt:

- `iPlug2/Examples/IPlugEffect/`
- `iPlug2/Examples/IPlugInstrument/`
- `iPlug2/Examples/IPlugControls/`

## See Also

- [iPlug2 Documentation](https://iplug2.github.io/docs)
- [CMake Documentation](https://cmake.org/documentation/)
