---
name: screenshot-cli
description: Build a headless CLI version of an iPlug2 plugin and render UI screenshots (works on CI/cloud without display)
---

# CLI Screenshot Rendering

Use this skill to build a command-line version of a plugin that can render UI screenshots headlessly. This works without a display server, making it ideal for CI pipelines, cloud environments, and automated documentation.

## Prerequisites

- CMake 3.14+
- Skia dependencies downloaded via `/setup-deps`
- Plugin must have IGraphics UI (uses Skia CPU backend)

## Quick Start

**1. Add CLI format to CMakeLists.txt:**
```cmake
iplug_add_plugin(${PROJECT_NAME}
  SOURCES ${SOURCES}
  RESOURCES ${RESOURCES}
  FORMATS APP CLI    # Add CLI here
)
```

**2. Build the CLI target:**
```bash
cd [ProjectFolder]
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target [PluginName]-cli
```

**3. Generate screenshot:**
```bash
./build/out/[PluginName] --screenshot ui.png
```

## Screenshot Commands

| Command | Description |
|---------|-------------|
| `--screenshot <file.png>` | Render UI to PNG file |
| `--scale <factor>` | Scale factor (1.0 = standard, 2.0 = retina) |
| `--resources <path>` | Path to resources directory (fonts, images) |
| `--set <idx> <val>` | Set parameter before rendering |
| `--load-state <file.bin>` | Load preset before rendering |

## Examples

**Standard resolution screenshot:**
```bash
./MyPlugin --screenshot ui.png
```

**Retina/HiDPI screenshot:**
```bash
./MyPlugin --screenshot ui@2x.png --scale 2.0
```

**Screenshot with specific preset:**
```bash
./MyPlugin --load-state mypreset.bin --screenshot preset.png
```

**Screenshot with parameter values:**
```bash
./MyPlugin --set 0 75 --set 1 50 --screenshot configured.png
```

**Specify resources path (for fonts/images):**
```bash
./MyPlugin --resources ./resources --screenshot ui.png
```

## CI/Automation Example

```bash
#!/bin/bash
# Generate screenshots for documentation

# Build CLI target
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target MyPlugin-cli

# Generate screenshots at different scales
./build/out/MyPlugin --screenshot docs/ui.png
./build/out/MyPlugin --screenshot docs/ui@2x.png --scale 2.0

# Generate preset screenshots
for preset in presets/*.bin; do
  name=$(basename "$preset" .bin)
  ./build/out/MyPlugin --load-state "$preset" --screenshot "docs/${name}.png"
done
```

## How It Works

The CLI target with IGraphics uses:
- **IPlugCLI_ScreenshotRenderer** - Extends CLI with screenshot capability
- **IGraphicsHeadless** - Skia CPU backend without window/display
- **Skia PNG encoder** - High-quality image output

No X11, Wayland, or display server required. Runs fully headless.

## Build Output

| Platform | Output Location |
|----------|-----------------|
| macOS | `build/out/[PluginName]` |
| Linux | `build/out/[PluginName]` |
| Windows | `build/[PluginName]-cli/[PluginName]-cli.exe` |

## Troubleshooting

**"Error: Graphics is not headless"**
- Plugin was built without CLI-Graphics support
- Ensure `FORMATS` includes `CLI` and plugin has IGraphics

**Missing fonts in screenshot**
- Use `--resources <path>` to specify resources directory
- On Linux, ensure fontconfig is installed

**Build fails linking Skia**
- Run `/setup-deps` to download Skia libraries
- Ensure CMake finds Skia: `-DSKIA_DIR=/path/to/skia`

## Workflow

1. **Check plugin has IGraphics UI** - CLI-Graphics requires IGraphics
2. **Add CLI to FORMATS** in CMakeLists.txt
3. **Build CLI target** using CMake
4. **Run with --screenshot** to generate PNG
5. **Use --scale 2.0** for retina displays or high-quality docs
