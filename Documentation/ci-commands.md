# CI Commands for Pull Requests

This document describes how to use special CI commands in your pull requests to control which builds and tests run.

## CI Architecture

iPlug2 uses two separate CI systems with distinct command prefixes:

| Command | CI System | Build System | What It Builds |
|---------|-----------|--------------|----------------|
| `/ci-azp` | Azure Pipelines | Xcode / Visual Studio | Native IDE projects (`.xcodeproj`, `.vcxproj`) |
| `/ci-cmake` | GitHub Actions | CMake | CMake-based builds (`CMakeLists.txt`) |

## Azure Pipelines: `/ci-azp` Commands

These commands control builds of Xcode and Visual Studio projects.

### Basic Commands

```
/ci-azp projects=IPlugControls,IPlugInstrument  # Build specific projects
/ci-azp platforms=mac,win,ios,web               # Build on specific platforms
/ci-azp formats=vst3,app,clap                   # Build specific plugin formats
/ci-azp graphics=skia                           # Use specific graphics backend
/ci-azp test                                    # Run plugin validation tests
/ci-azp skip                                    # Skip CI entirely
```

### Shortcut Commands

```
/ci-azp full      # Build ALL projects on all enabled platforms
/ci-azp quick     # Build only IPlugEffect (default behavior)
/ci-azp mac       # Build only on macOS
/ci-azp win       # Build only on Windows
/ci-azp ios       # Build only on iOS
/ci-azp web       # Build only for Web (WAM)
```

### Combined Commands

You can combine multiple options on a single line:

```
/ci-azp projects=IPlugControls platforms=mac formats=vst3 test
```

Or use multiple lines:

```
/ci-azp projects=IPlugControls,IPlugInstrument
/ci-azp platforms=mac,win
/ci-azp graphics=both
/ci-azp test
```

### Available Options

#### Projects

Specify which example or test projects to build:

**Examples:**
- `IPlugEffect` (default)
- `IPlugInstrument`
- `IPlugMidiEffect`
- `IPlugControls`
- `IPlugResponsiveUI`
- `IPlugChunks`
- `IPlugSideChain`
- `IPlugSurroundEffect`
- `IPlugDrumSynth`
- `IPlugConvoEngine`
- `IPlugOSCEditor`
- `IPlugVisualizer`
- `IPlugReaperPlugin`
- `IPlugCocoaUI`
- `IPlugSwiftUI`
- `IPlugWebUI`
- `IPlugP5js`
- `IPlugSvelteUI`

**Tests:**
- `IGraphicsStressTest`
- `IGraphicsTest`
- `MetaParamTest`

Use `all` to build everything: `/ci-azp projects=all`

#### Platforms

- `mac` - macOS (arm64, x86_64)
- `win` - Windows (x64, arm64ec)
- `ios` - iOS
- `web` - Web Audio Module (Emscripten)

#### Plugin Formats

- `app` - Standalone application
- `vst3` - VST3 plugin
- `clap` - CLAP plugin
- `auv2` - Audio Unit v2 (macOS/iOS only)
- `aax` - AAX plugin (requires AAX SDK)
- `vst2` - VST2 plugin (deprecated, requires VST2 SDK)

#### Graphics Backends

- `nanovg` - NanoVG (default, lightweight)
- `skia` - Skia (heavyweight, better quality)
- `both` - Build with both backends

## CMake Builds: `/ci-cmake` Commands

These commands control CMake-based builds on GitHub Actions.

### Basic Commands

```
/ci-cmake                        # Build IPlugEffect with default settings
/ci-cmake examples=IPlugControls # Build specific examples
/ci-cmake all                    # Full build with all generators and platforms
```

### Platform/Feature Commands

```
/ci-cmake all-generators   # Test all CMake generators (Ninja, Make, Xcode, VS2022)
/ci-cmake generators       # Same as all-generators
/ci-cmake ios              # Include iOS build
/ci-cmake visionos         # Include visionOS build
/ci-cmake wam              # Include WAM/Emscripten build
/ci-cmake universal        # Include macOS Universal (arm64+x86_64)
/ci-cmake arm64ec          # Include Windows ARM64EC build
```

### Combined Commands

```
/ci-cmake examples=IPlugControls,IGraphicsTest ios wam
```

### Automatic Triggers

CMake CI also runs automatically when these files change:
- `CMakeLists.txt`
- `iPlug2.cmake`
- `CMakePresets.json`
- `Scripts/cmake/**`
- `IPlug/**`
- `IGraphics/**`
- `Examples/**/CMakeLists.txt`
- `Tests/**/CMakeLists.txt`

## Examples

### Build Xcode/VS project on macOS only

```
/ci-azp projects=IPlugControls platforms=mac
```

### Build CMake with iOS support

```
/ci-cmake examples=IPlugEffect ios
```

### Full Azure Pipelines build with testing

```
/ci-azp full test
```

### Full CMake build with all generators

```
/ci-cmake all
```

### Skip Azure Pipelines for documentation-only changes

```
/ci-azp skip
```

## Manual Triggering

### Azure Pipelines (via Azure DevOps UI)

1. Go to the Azure Pipeline
2. Click "Run pipeline"
3. Set parameters:
   - **Build all example projects**: Build everything
   - **Specific projects to build**: Comma-separated project names
   - **Build macOS/Windows/iOS/Web**: Toggle platforms
   - **Build Skia graphics backend**: Enable Skia
   - **Run plugin validation tests**: Enable testing

### CMake CI (via GitHub Actions UI)

1. Go to Actions → "CMake CI (GitHub Actions)"
2. Click "Run workflow"
3. Set parameters:
   - **Examples to build**: Comma-separated or "all"
   - **Test all generators**: Enable to test Ninja, Make, Xcode, VS2022
   - **Include iOS/visionOS/WAM builds**: Toggle platforms

## Tips

1. **Be specific**: Build only what you need to verify your changes
2. **Use the right prefix**: `/ci-azp` for Xcode/VS projects, `/ci-cmake` for CMake
3. **Combine wisely**: Multiple options on one line are processed together
4. **Test first**: Use `/ci-azp test` when changing core functionality
5. **Check comments**: The CI bot will confirm what's being built
