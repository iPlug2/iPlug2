# IPlugJSONUI

Example demonstrating JSON-based declarative UI for IGraphics.

## Features

- **Declarative UI**: Define controls, styles, and layout in `resources/ui.json`
- **Hot Reload**: In debug builds, edit `ui.json` and save - UI updates instantly
- **Runtime Layout**: Bounds recalculate on resize using expressions like `"fracH": [0.33, true]`

## Usage

1. Build in debug mode
2. Run the plugin
3. Edit `resources/ui.json`
4. Save - the UI reloads automatically

## JSON Schema

```json
{
  "styles": {
    "styleName": {
      "colorFG": "#FF6600",
      "roundness": 0.5,
      ...
    }
  },
  "controls": [
    {
      "type": "IVKnobControl",
      "id": "myKnob",
      "param": "kGain",
      "bounds": { "from": "parent", "fracH": [0.5, true] },
      "style": "styleName",
      "label": "Gain"
    }
  ]
}
```

## Supported Bounds Formats

| Format | Example | Description |
|--------|---------|-------------|
| Absolute | `[50, 50, 100, 100]` | `[x, y, w, h]` in pixels |
| Percentage | `"x": "50%"` | Percentage of parent |
| Expression | `"h": "parent.h - 20"` | Simple math |
| IRECT ops | `"fracV": [0.25, true]` | Maps to IRECT methods |

## Supported Controls

- `IVKnobControl`, `IVSliderControl`, `IVButtonControl`
- `IVToggleControl`, `IVSwitchControl`, `IVTabSwitchControl`
- `IVGroupControl`, `IVPanelControl`, `IVLabelControl`
- `ITextControl`, `IPanelControl`

See `IGraphicsJSON.cpp` for the full list.
