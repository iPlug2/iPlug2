---
name: build-cmake
description: Build an iPlug2 plugin project using CMake with Ninja, Xcode, or Visual Studio generators
---

# Build iPlug2 Plugin with CMake

Use this skill when the user wants to build their plugin project using CMake.

## Prerequisites

- CMake 3.14+
- Ninja (recommended) or Xcode/Visual Studio
- Plugin SDKs downloaded (VST3, CLAP, etc.) via `/setup-deps`

## Quick Start

```bash
cd [ProjectFolder]
mkdir -p build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

## Generators

| Generator | Command | Platform | Notes |
|-----------|---------|----------|-------|
| Ninja | `-G Ninja` | macOS, Windows | Fast, recommended |
| Xcode | `-G Xcode` | macOS, iOS, visionOS | Multi-config, good for debugging |
| Visual Studio | `-G "Visual Studio 17 2022" -A x64` | Windows | Multi-config |

## Build Commands

**Configure (choose one):**
```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..          # Ninja
cmake -G Xcode ..                                      # Xcode
cmake -G "Visual Studio 17 2022" -A x64 ..             # VS 2022
```

**Build all targets:**
```bash
cmake --build . --config Release
```

**Build specific target:**
```bash
cmake --build . --config Release --target [PluginName]-vst3
```

## Target Suffixes

| Format | Target Suffix |
|--------|---------------|
| Standalone App | `-app` |
| VST2 | `-vst2` |
| VST3 | `-vst3` |
| CLAP | `-clap` |
| AAX | `-aax` |
| AUv2 | `-au` |

## Common Options

```bash
# Debug build
-DCMAKE_BUILD_TYPE=Debug

# Release build
-DCMAKE_BUILD_TYPE=Release

# Universal binaries (macOS)
-DIPLUG2_UNIVERSAL=ON

# IGraphics backend
-DIGRAPHICS_BACKEND=SKIA -DIGRAPHICS_RENDERER=METAL

# Debug host for plugins
-DIPLUG2_DEBUG_HOST="/Applications/REAPER.app"
```

## Platform-Specific

**iOS Device:**
```bash
cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DIPLUG2_IOS_PLATFORM=OS ..
```

**iOS Simulator:**
```bash
cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DIPLUG2_IOS_PLATFORM=SIMULATOR ..
```

**visionOS:**
```bash
cmake -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DIPLUG2_IOS_PLATFORM=VISIONOS ..
```

**Web (Emscripten):**
```bash
emcmake cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target [PluginName]-wam
```

## Build Output

Output goes to `build/out/`:
- `[PluginName].app` - Standalone
- `[PluginName].vst3/` - VST3
- `[PluginName].clap/` - CLAP
- `[PluginName].component/` - AUv2
- `[PluginName].aaxplugin/` - AAX

## Workflow

1. **Ask for build preferences:**
   - Generator (Ninja/Xcode/VS)
   - Configuration (Debug/Release)
   - Specific target or all
   - IGraphics backend if relevant

2. **Configure if needed** (only once per build directory)

3. **Build the requested target(s)**

4. **Report output location** from `build/out/`

## Tips

- Ninja is fastest for iterative development
- Xcode generator required for Swift examples (IPlugSwiftUI, IPlugCocoaUI)
- Use `--target` to build specific formats and avoid SDK-related failures
- Check [Documentation/cmake.md](../../../Documentation/cmake.md) for full reference
