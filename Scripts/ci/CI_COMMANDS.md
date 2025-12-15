# CI Commands for Pull Requests

This document describes how to use special `/ci` commands in your pull requests to control which builds and tests run.

## Overview

By default, PRs only build **IPlugEffect** with **NanoVG** graphics on **macOS and Windows**. This keeps CI fast for simple changes. When you need more control, use `/ci` commands in your PR description or comments.

## Command Syntax

Commands are placed in the PR description (body) or in PR comments. Each command starts with `/ci` followed by options.

### Basic Commands

```
/ci projects=IPlugControls,IPlugInstrument  # Build specific projects
/ci platforms=mac,win,ios,web               # Build on specific platforms
/ci formats=vst3,app,clap                   # Build specific plugin formats
/ci graphics=skia                           # Use specific graphics backend
/ci test                                    # Run plugin validation tests
/ci skip                                    # Skip CI entirely
```

### Shortcut Commands

```
/ci full      # Build ALL projects on all enabled platforms
/ci quick     # Build only IPlugEffect (default behavior)
/ci mac       # Build only on macOS
/ci win       # Build only on Windows
/ci ios       # Build only on iOS
/ci web       # Build only for Web (WAM)
```

### Combined Commands

You can combine multiple options on a single line:

```
/ci projects=IPlugControls platforms=mac formats=vst3 test
```

Or use multiple `/ci` lines:

```
/ci projects=IPlugControls,IPlugInstrument
/ci platforms=mac,win
/ci graphics=both
/ci test
```

## Available Options

### Projects

Specify which example or test projects to build:

**Examples:**
- `IPlugEffect` (default, always included unless explicitly excluded)
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

Use `all` to build everything:
```
/ci projects=all
```

### Platforms

- `mac` - macOS (arm64, x86_64)
- `win` - Windows (x64, arm64ec)
- `ios` - iOS
- `web` - Web Audio Module (Emscripten)

### Plugin Formats

- `app` - Standalone application
- `vst3` - VST3 plugin
- `clap` - CLAP plugin
- `auv2` - Audio Unit v2 (macOS/iOS only)
- `aax` - AAX plugin (requires AAX SDK)
- `vst2` - VST2 plugin (deprecated, requires VST2 SDK)

### Graphics Backends

- `nanovg` - NanoVG (default, lightweight)
- `skia` - Skia (heavyweight, better quality)
- `both` - Build with both backends

## Examples

### Build a specific project on macOS only

```
/ci projects=IPlugControls platforms=mac
```

### Build multiple projects with Skia graphics

```
/ci projects=IGraphicsTest,IPlugControls graphics=skia
```

### Full CI build with testing

```
/ci full test
```

### Build for web deployment

```
/ci projects=IPlugEffect,IPlugInstrument platforms=web
```

### Skip CI for documentation-only changes

```
/ci skip
```

## Using Azure Pipeline Parameters (Manual Trigger)

The Azure Pipeline also supports manual triggering with parameters through the Azure DevOps UI:

1. Go to the Azure Pipeline
2. Click "Run pipeline"
3. Set the parameters:
   - **Build all example projects**: Build everything
   - **Specific projects to build**: Comma-separated project names
   - **Build macOS/Windows/iOS/Web**: Toggle platforms
   - **Build Skia graphics backend**: Enable Skia
   - **Run plugin validation tests**: Enable testing

## GitHub Actions Integration

The `/ci` command system also works with GitHub Actions. When you add a `/ci` command to a PR comment:

1. The GitHub Action parses your command
2. Posts an acknowledgment comment
3. Triggers builds for the specified configuration
4. Posts a summary when builds complete

### Required Secrets (for Azure Pipeline triggering)

If you want GitHub Actions to trigger Azure Pipelines directly, configure these secrets:
- `AZURE_DEVOPS_PAT` - Azure DevOps Personal Access Token
- `AZURE_DEVOPS_ORG` - Your Azure DevOps organization name
- `AZURE_DEVOPS_PROJECT` - Project name (default: iPlug2)
- `AZURE_PIPELINE_ID` - Pipeline ID number

Without these secrets, the GitHub Action will run builds directly on GitHub runners.

## Tips

1. **Be specific**: Build only what you need to verify your changes
2. **Use shortcuts**: `/ci mac` is faster than typing `/ci platforms=mac`
3. **Combine wisely**: Multiple options on one line are processed together
4. **Test first**: Use `/ci test` when changing core functionality
5. **Check comments**: The CI bot will confirm what's being built
