---
name: screenshot
description: Take a screenshot of the plugin UI using the standalone app CLI for debugging and documentation
---

# Screenshot Plugin UI

Use this skill to capture screenshots of the plugin UI. This is useful for:

- **Debugging UI design** - Quickly verify control layouts, colors, and positioning without launching a DAW
- **Documentation** - Generate consistent screenshots for README files and wikis
- **CI/CD pipelines** - Automated visual regression testing or asset generation

## Prerequisites

Build the standalone app (APP target) first:
```bash
cd [ProjectFolder]
xcodebuild -project "./projects/[ProjectName]-macOS.xcodeproj" -target APP -configuration Debug
```

The app will be built to `~/Applications/[PluginName].app`

## Take Screenshot

### macOS

```bash
~/Applications/[PluginName].app/Contents/MacOS/[PluginName] --screenshot /path/to/output.png
```

### Example

```bash
# Take screenshot of IPlugEffect
~/Applications/IPlugEffect.app/Contents/MacOS/IPlugEffect --screenshot ./screenshot.png
```

The app will:
1. Launch and display the UI (audio/MIDI I/O is automatically disabled)
2. Wait ~500ms for the UI to fully render
3. Capture the window to a PNG file
4. Exit automatically

## Options

- `--screenshot <path>` - Path to save the PNG screenshot (required)
- `--no-io` - Explicitly disable audio/MIDI (implicit when using --screenshot)

## Tips

- Screenshots are high-DPI (Retina) resolution on supported displays
- The screenshot captures the plugin content area
- For iterative UI debugging, rebuild the APP target and re-run the screenshot command
- Combine with the `build` skill: build first, then screenshot
