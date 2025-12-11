# IPlugJSFX

An iPlug2 plugin that embeds the JSFX scripted audio effect engine, allowing you to run JSFX effects within any plugin format supported by iPlug2.

## Overview

JSFX is a real-time programmable effects system originally developed by Cockos for REAPER. This example demonstrates how to embed the JSFX engine in an iPlug2 plugin, providing cross-platform support for JSFX effects.

## Features

- Load and run any JSFX effect
- Cross-platform support (macOS, Windows, Linux via iPlug2)
- All standard plugin formats (VST3, AU, CLAP, AAX, standalone)
- Parameter automation for JSFX sliders
- State save/restore for effect and parameters

## Building

This example requires the JSFX source files to be compiled and linked. The JSFX sources are located in the `jsfx/` directory at the repository root.

### Required JSFX source files:
- `jsfx/effectload.cpp`
- `jsfx/effectproc.cpp`
- `jsfx/sfxui.cpp`
- `jsfx/miscfunc.cpp`
- WDL/EEL2 compiler sources

### Dependencies:
- WDL (included in iPlug2)
- EEL2 scripting engine (part of WDL)

## Usage

1. Set the JSFX root path using `SetJSFXRootPath()` - this should point to a directory containing `effects/` and `data/` subdirectories
2. Load an effect using `LoadEffect("path/to/effect.jsfx")`
3. The plugin parameters will automatically map to the JSFX sliders

## API

```cpp
// Set the root directory for JSFX effects
void SetJSFXRootPath(const char* path);

// Load a JSFX effect by path (relative to root/effects/)
bool LoadEffect(const char* effectPath);

// Close the currently loaded effect
void CloseEffect();

// Get the name of the loaded effect
const char* GetEffectName() const;
```

## Notes

- This is a prototype/example implementation
- The JSFX native UI is not currently integrated - a simple IGraphics UI is provided
- MIDI support is enabled but requires additional implementation for full functionality
- For production use, you may want to add effect browsing UI and more robust error handling
