# XcodeGen for iPlug2

This directory contains XcodeGen templates and scripts for generating Xcode projects for iPlug2 plugins.

## Overview

[XcodeGen](https://github.com/yonaskolb/XcodeGen) is a command-line tool that generates Xcode projects from a YAML specification file. Using XcodeGen for iPlug2 projects offers several advantages:

- **No merge conflicts**: Project specs are human-readable YAML files that diff cleanly
- **Maintainability**: Changes to build settings can be made in one place and regenerated
- **Consistency**: Ensures all projects follow the same structure and settings
- **Automation**: Projects can be generated as part of CI/CD pipelines

## Installation

```bash
# Using Homebrew (recommended)
brew install xcodegen

# Using Mint
mint install yonaskolb/xcodegen
```

## Quick Start

1. Navigate to an example project with a `project.yml` file:
   ```bash
   cd Examples/IPlugEffect
   ```

2. Generate the Xcode project:
   ```bash
   xcodegen generate
   ```

3. Open the generated project:
   ```bash
   open projects/IPlugEffect-macOS.xcodeproj
   ```

Or use the helper script:
```bash
./Scripts/xcodegen/generate_project.sh IPlugEffect
```

## File Structure

```
Scripts/xcodegen/
├── README.md                 # This file
├── iplug2-common.yml         # Shared templates for all iPlug2 projects
└── generate_project.sh       # Helper script to generate projects

Examples/IPlugEffect/
├── project.yml               # Full XcodeGen spec (comprehensive)
├── project-simple.yml        # Simplified spec using shared templates
└── ...
```

## Project Spec Structure

### Basic project.yml

```yaml
name: MyPlugin-macOS

# Include shared iPlug2 templates
include:
  - path: ../../Scripts/xcodegen/iplug2-common.yml

options:
  bundleIdPrefix: com.MyCompany
  deploymentTarget:
    macOS: "10.13"

# Use existing xcconfig files
configFiles:
  Debug: config/MyPlugin-mac.xcconfig
  Release: config/MyPlugin-mac.xcconfig

configs:
  Debug: debug
  Release: release

settings:
  base:
    BINARY_NAME: MyPlugin

targets:
  VST3:
    templates:
      - IPlug2_VST3_macOS
    settings:
      INFOPLIST_FILE: resources/MyPlugin-VST3-Info.plist
    sources:
      # Your plugin sources
      - path: .
        includes: ["*.cpp", "*.h"]
      # ... IPlug/IGraphics sources
```

## Available Target Templates

The `iplug2-common.yml` file provides these templates:

### macOS Targets
| Template | Description |
|----------|-------------|
| `IPlug2_VST3_macOS` | VST3 plugin bundle |
| `IPlug2_CLAP_macOS` | CLAP plugin bundle |
| `IPlug2_AUv2_macOS` | Audio Unit v2 component |
| `IPlug2_AUv3_macOS` | AUv3 app extension |
| `IPlug2_AUv3Framework_macOS` | Framework for AUv3 |
| `IPlug2_APP_macOS` | Standalone application |

### iOS Targets
| Template | Description |
|----------|-------------|
| `IPlug2_AUv3_iOS` | AUv3 app extension |
| `IPlug2_AUv3Framework_iOS` | Framework for AUv3 |
| `IPlug2_APP_iOS` | Host application |

## Required Source Files by Target

Each target type requires specific source files from the iPlug2 framework:

### IPlug Core (all targets)
```yaml
- path: ../../IPlug
  includes:
    - IPlugAPIBase.cpp
    - IPlugParameter.cpp
    - IPlugPluginBase.cpp
    - IPlugProcessor.cpp
    - IPlugPaths.mm      # macOS
    - IPlugTimer.cpp
```

### Format-Specific Sources

**VST3:**
```yaml
- path: ../../IPlug/VST3
  includes:
    - IPlugVST3.cpp
```

**CLAP:**
```yaml
- path: ../../IPlug/CLAP
  includes:
    - IPlugCLAP.cpp
```

**AUv2:**
```yaml
- path: ../../IPlug/AUv2
  includes:
    - IPlugAU.cpp
    - IPlugAU_view_factory.mm
    - dfx-au-utilities.c
    - IPlugAU.r
```

**AUv3:**
```yaml
- path: ../../IPlug/AUv3
  includes:
    - IPlugAUv3.mm
    - IPlugAUAudioUnit.mm
    - IPlugAUViewController.mm
```

**APP (Standalone):**
```yaml
- path: ../../IPlug/APP
  includes:
    - IPlugAPP.cpp
    - IPlugAPP_host.cpp
    - IPlugAPP_main.cpp
    - IPlugAPP_dialog.cpp
```

### IGraphics (NanoVG backend)
```yaml
- path: ../../IGraphics
  includes:
    - IGraphics.cpp
    - IGraphicsEditorDelegate.cpp

- path: ../../IGraphics/Controls
  includes:
    - IControls.cpp
    - ITextEntryControl.cpp
    - IPopupMenuControl.cpp

- path: ../../IGraphics/Platforms
  includes:
    - IGraphicsMac.mm
    - IGraphicsMac_view.mm
    - IGraphicsCoreText.mm

- path: ../../IGraphics/Drawing
  includes:
    - IGraphicsNanoVG.cpp
    - IGraphicsNanoVG_src.m
```

### SWELL (macOS)
```yaml
- path: ../../WDL/swell
  includes:
    - swell.cpp
    - swell-ini.cpp
    - swell-misc.mm
    - swell-gdi.mm
    - swell-kb.mm
    - swell-menu.mm
    - swell-wnd.mm
    - swell-dlg.mm
    - swell-miscdlg.mm
```

## Relationship to xcconfig Files

XcodeGen works alongside your existing xcconfig files. The xcconfig files define:
- `IPLUG2_ROOT` - Path to iPlug2 root
- `BINARY_NAME` - Plugin output name
- `EXTRA_INC_PATHS`, `EXTRA_LNK_FLAGS` - Custom paths/flags
- Format-specific preprocessor definitions

XcodeGen references these via `configFiles:` and the templates use the variables defined in them.

## Adding a New Example

1. Create your plugin source files (Plugin.cpp, Plugin.h, config.h)

2. Create the project.yml:
   ```yaml
   name: MyPlugin-macOS

   include:
     - path: ../../Scripts/xcodegen/iplug2-common.yml

   configFiles:
     Debug: config/MyPlugin-mac.xcconfig
     Release: config/MyPlugin-mac.xcconfig

   configs:
     Debug: debug
     Release: release

   targets:
     VST3:
       templates: [IPlug2_VST3_macOS]
       settings:
         INFOPLIST_FILE: resources/MyPlugin-VST3-Info.plist
       sources:
         # Add your sources here
   ```

3. Generate the project:
   ```bash
   xcodegen generate --spec project.yml --project projects
   ```

## Troubleshooting

### Build Settings Not Applied
Make sure your xcconfig file is referenced correctly and contains the `#include` for `common-mac.xcconfig`.

### Missing Source Files
Check that all source paths are relative to the project.yml location.

### Framework Linking Errors
Verify the dependencies list in your target matches what's needed for your plugin format.

## Migration from Manual .xcodeproj

To migrate an existing project:

1. Compare the manually created project with the template
2. Extract any custom build settings to your xcconfig file
3. Create a project.yml using the templates
4. Generate and test the new project
5. Optionally remove the old .xcodeproj from version control

## Future Improvements

- [ ] Add AAX target template (requires AAXLibrary.xcodeproj dependency)
- [ ] Add VST2 target template (deprecated but still supported)
- [ ] Create iOS project templates
- [ ] Add REAPER extension templates
- [ ] Support for Skia graphics backend
- [ ] Workspace generation combining macOS and iOS projects
