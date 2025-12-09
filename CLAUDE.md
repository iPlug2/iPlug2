# CLAUDE.md

This is the iPlug2 framework repository. Upstream: https://github.com/iPlug2/iPlug2.git

## Instructions

- Be concise. Assume the user knows audio programming concepts but not iPlug2 specifically.
- Check current directory before running scripts; return to repo root after.

## iPlug2 Overview

Cross-platform C++ audio plugin framework with two main components:

1. **IPlug** - Core plugin abstraction layer
2. **IGraphics** - GUI toolkit with multiple backends (NanoVG - lightweight, fast, Skia - heavyweight, better quality)

**Plugin Formats:** CLAP, VST3, AUv2/AUv3, AAX, WAM (VST2 is supported but deprecated)
**Extra Formats:** Standalone App, REAPER extensions (& REAPER plugins - plugins that call the REAPER API)
**Platforms:** macOS, iOS, Windows, Web (WAM/WASM)

## Repo Structure

```
├── IPlug/              # Core plugin abstraction
├── IGraphics/          # UI framework
├── IGraphics/Controls  # UI controls library
├── IGraphics/Platforms # Platform-specific implementations
├── IGraphics/Drawing   # Drawing implementations (NanoVG, Skia)
├── Dependencies/       # Third-party libs/SDKs (don't modify)
├── Examples/           # Template projects
├── Tests/              # Framework test projects
├── WDL/                # Cockos WDL (don't modify)
├── Scripts/            # Build utilities
├── Scripts/ci/         # CI/CD scripts
└── Documentation/      # Docs and wiki
```

## Key Concepts

- Three main files per project: `Plugin.cpp`, `Plugin.h`, `config.h`
- `ProcessBlock` must be realtime-safe (no allocations, locks, file I/O)
- Parameters: fixed count at compile time, indexed by enum, non-normalized values
- Use `GetParam(kIndex)->Value()` for parameter access
- IGraphics UIs typically built in a lambda in the plugin constructor
- IControls link to parameters via parameter ID
- Visualization controls can use an [ISender](@IPlug/ISender.h) to send data from audio thread to UI thread
- Controls can be grouped and tagged when Attached

- IGraphics is only one UI option; other options include SwiftUI, WebView. See examples in [Examples/](@Examples/).
- The default IGraphics backend NanoVG doesn't need the dependencies to be downloaded

## Code Style

- 2-space indentation, no tabs
- Unix line endings (except Windows-specific files)
- Member variables: `mCamelCase`
- Pointer args: `pCamelCase`
- Internal methods: `_methodName`
- Use C++17 (`override`, `final`, `auto`, `std::optional`, `std::string_view`)
- Avoid STL in core code; prefer WDL alternatives

## Resources

- [API Docs](https://iplug2.github.io/docs) | [Wiki](https://github.com/iPlug2/iPlug2/wiki)
- When building standalone app targets only debug builds have the debug menu with screenshot capabilities