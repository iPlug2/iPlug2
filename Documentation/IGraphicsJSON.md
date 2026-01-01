# IGraphicsJSON - Declarative JSON-Based UI for iPlug2

## Overview

IGraphicsJSON enables defining IGraphics UIs declaratively in JSON format with hot-reload support during development. It provides a separation between UI structure/styling and plugin code, allowing rapid UI iteration without recompilation.

**Status**: Experimental / Work in Progress

## Architecture

```
┌─────────────────┐     ┌──────────────┐     ┌─────────────────┐
│   ui.json       │────▶│ IGraphicsJSON│────▶│ IGraphics       │
│   (declarative) │     │ (parser)     │     │ (controls)      │
└─────────────────┘     └──────────────┘     └─────────────────┘
                              │
                              ▼
                        ┌──────────────┐
                        │ mLayouts[]   │  Stored for OnResize
                        └──────────────┘
```

### Key Components

- **IGraphicsJSON**: Main class that parses JSON, creates controls, and handles layout
- **LayoutDef**: Stores the JSON bounds specification for each control (evaluated on resize)
- **nlohmann/json**: JSON parsing library (in Dependencies/Extras/nlohmann)

### Lifecycle

1. Plugin creates IGraphicsJSON instance in `mLayoutFunc`
2. `Load()` or `LoadFromString()` parses JSON and creates all IControl instances
3. Controls are created with placeholder bounds (0,0,100,100)
4. `OnResize()` is called to evaluate actual bounds from layout specs
5. On window resize, `OnResize()` recalculates all bounds
6. In debug builds, `CheckForChanges()` monitors file for hot-reload

## What Currently Works

### Control Types

| JSON Type | IGraphics Control | Notes |
|-----------|------------------|-------|
| `IVKnobControl` | IVKnobControl | Vector knob |
| `IVSliderControl` | IVSliderControl | Supports `direction` |
| `IVButtonControl` | IVButtonControl | Uses SplashClickActionFunc |
| `IVToggleControl` | IVToggleControl | Supports `offText`, `onText` |
| `IVSwitchControl` | IVSwitchControl | Supports `numStates` |
| `IVTabSwitchControl` | IVTabSwitchControl | Supports `labels` array |
| `IVGroupControl` | IVGroupControl | Visual grouping |
| `IVPanelControl` | IVPanelControl | Styled panel |
| `IVLabelControl` | IVLabelControl | Styled label |
| `ITextControl` | ITextControl | Basic text |
| `IPanelControl` | IPanelControl | Basic colored panel |

### Layout System

**Absolute Bounds** (array format):
```json
"bounds": [10, 20, 100, 50]  // [x, y, width, height] relative to parent
```

**Relative/IRECT Operations** (object format):
```json
"bounds": {
  "from": "parent",           // Start from parent bounds
  "pad": -20,                 // GetPadded(-20)
  "fracV": [0.5, true],       // FracRectVertical(0.5, true)
  "fracH": [0.33, true],      // FracRectHorizontal(0.33, true)
  "reduceFromTop": 60,        // ReduceFromTop(60)
  "reduceFromBottom": 20,
  "reduceFromLeft": 10,
  "reduceFromRight": 10,
  "vShift": 50,               // GetVShifted(50) - NUMBER ONLY
  "hShift": -30,              // GetHShifted(-30) - NUMBER ONLY
  "centredInside": [200, 100] // GetCentredInside(200, 100)
}
```

**Expression-Based Bounds**:
```json
"bounds": {
  "x": "10%",                 // 10% of parent width
  "y": 20,                    // 20 pixels from parent top
  "w": "parent.w - 40",       // Parent width minus 40
  "h": "50%"                  // 50% of parent height
}
```

### Styling

**Named Styles**:
```json
{
  "styles": {
    "myStyle": {
      "colorBG": "#333333",
      "colorFG": "#FF6600",
      "roundness": 0.3,
      "drawFrame": true
    }
  },
  "controls": [
    { "type": "IVKnobControl", "style": "myStyle" }
  ]
}
```

**Inline Styles**:
```json
{
  "type": "IVKnobControl",
  "style": {
    "colorBG": "#333333",
    "colorFG": "#FF6600"
  }
}
```

**Available Style Properties**:
- `showLabel`, `showValue`, `drawFrame`, `drawShadows`, `emboss` (bool)
- `roundness`, `frameThickness`, `shadowOffset`, `widgetFrac`, `angle` (float)
- `colorBG`, `colorFG`, `colorPR`, `colorFR`, `colorHL`, `colorSH`, `colorX1`, `colorX2`, `colorX3` (color)
- `labelText`, `valueText` (text style object with `size`, `color`, `align`, `valign`)

**Color Formats**:
- Hex: `"#RRGGBB"` or `"#RRGGBBAA"`
- Array: `[255, 128, 0]` or `[255, 128, 0, 200]`

### Parameter and Tag Mapping

```cpp
// In plugin code, before Load():
mJSONUI->SetParamMapping({
  {"kGain", kGain},
  {"kPan", kPan}
});

mJSONUI->SetTagMapping({
  {"gainKnob", kCtrlTagGain}
});
```

```json
{
  "type": "IVKnobControl",
  "id": "gainKnob",
  "param": "kGain"
}
```

### Hot-Reload

Hot-reload only works in **Debug builds** with `IPLUG_SRCDIR` defined:

```cmake
target_compile_definitions(_${PROJECT_NAME}-base INTERFACE
  $<$<CONFIG:Debug>:IPLUG_SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}">
)
```

The plugin must call `CheckForChanges()` from `OnIdle()`:
```cpp
void MyPlugin::OnIdle()
{
  if (mJSONUI)
    mJSONUI->CheckForChanges();
}
```

---

## What Does NOT Work / Limitations

### Critical Issues

1. **`hShift` and `vShift` Only Accept Numbers**
   ```json
   // CRASHES - string expression not supported:
   "hShift": "33%"

   // Works:
   "hShift": 50
   ```

2. **Layout Operation Order is Fixed**
   Operations are applied in this order regardless of JSON key order:
   `from` → `pad` → `fracV` → `fracH` → `reduceFrom*` → `vShift` → `hShift` → `centredInside` → `x/y/w/h`

3. **Child Layout Timing**
   Children reference parent bounds, but parent bounds may not be finalized on the first layout pass. This can cause incorrect positioning.

4. **Control Index Coupling**
   `mLayouts` indices must match `IGraphics` control indices. Adding controls outside of JSON breaks the mapping.

### Missing Features

| Feature | Status |
|---------|--------|
| Bitmap controls (IBitmapControl, etc.) | Not implemented |
| SVG controls (ISVGKnob, etc.) | Not implemented |
| Meter/visualizer controls | Not implemented |
| Custom action handlers | Hardcoded to SplashClickActionFunc |
| Image/bitmap loading | Not implemented |
| SVG loading | Not implemented |
| Font loading from JSON | Must be loaded in plugin code |
| Flexbox layout | Stub exists, not implemented |
| Complex expressions | Only single operator supported |
| Web/WAM builds | Untested, likely broken |

### Expression Parser Limitations

The expression parser is very basic:
```json
// Supported:
"50%"
"parent.w"
"parent.h"
"parent.w - 20"
"parent.w * 0.5"

// NOT Supported:
"parent.w - 20 + 10"     // Multiple operators
"(parent.w - 20) * 0.5"  // Parentheses
"parent.w / 2"           // Division
"min(parent.w, 200)"     // Functions
```

---

## Resource Loading

### How It Works

| Build | Source | Mechanism |
|-------|--------|-----------|
| Debug + IPLUG_SRCDIR | Source directory | Direct file read |
| Debug (no IPLUG_SRCDIR) | Bundle | LocateResource() |
| Release | Bundle | pGraphics->LoadResource() |

### Platform Specifics

**macOS Bundles**:
- APP: `IPlugJSONUI.app/Contents/Resources/ui.json`
- VST3: `IPlugJSONUI.vst3/Contents/Resources/ui.json`
- AU: `IPlugJSONUI.component/Contents/Resources/ui.json`

**Windows**:
- VST3 bundle: `IPlugJSONUI.vst3/Contents/Resources/ui.json`
- Other formats: Embedded in binary via .rc file (requires resource ID)

**Web/WAM**:
- **Does not work** with file loading
- Must embed JSON as string: `LoadFromString(jsonStr)`

### Adding Resources to Build

**CMake**:
```cmake
iplug_add_plugin(${PROJECT_NAME}
  SOURCES ...
  RESOURCES
    resources/ui.json
)
```

The JSON file is copied to the bundle's Resources folder during build.

---

## Runtime Considerations

### Memory

- Layout specs stored in `mLayouts` vector (one per control)
- Original JSON stored in `mRootSpec` (for potential future use)
- Styles stored in `mStyles` map
- Controls are standard IGraphics controls with normal memory behavior

### Performance

- JSON parsing happens once at load time
- Layout evaluation on every resize (fast - just IRECT math)
- Hot-reload check on every OnIdle (just a stat() call)
- Full reload on file change (removes and recreates all controls)

### State Preservation

Hot-reload **does not preserve**:
- Control values (reset to parameter defaults)
- Animation state
- User interaction state (hover, drag, etc.)

---

## Best Practices

### 1. Keep Plugin Code Minimal
```cpp
mLayoutFunc = [&](IGraphics* pGraphics) {
  if (pGraphics->NControls() == 0) {
    pGraphics->SetLayoutOnResize(true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    mJSONUI = std::make_unique<IGraphicsJSON>(pGraphics, this);
    mJSONUI->SetParamMapping({{"kGain", kGain}});
    mJSONUI->Load(...);
  }
  mJSONUI->OnResize(pGraphics->GetBounds());
};
```

### 2. Use Named Styles
Define reusable styles in the `"styles"` section rather than repeating inline.

### 3. Use Expression Bounds for Responsiveness
```json
"bounds": { "x": "10%", "w": "80%", "h": 100 }
```

### 4. Test with Window Resizing
Layout issues often only appear at certain sizes.

### 5. Debug with IPLUG_SRCDIR
Always define `IPLUG_SRCDIR` in debug builds for hot-reload.

---

## Example: Complete UI

```json
{
  "styles": {
    "knob": {
      "colorBG": "#2A2A2A",
      "colorFG": "#FF6600",
      "colorPR": "#FF8833",
      "roundness": 0.5,
      "drawShadows": true
    }
  },

  "controls": [
    {
      "type": "IPanelControl",
      "bounds": { "from": "parent" },
      "color": "#1A1A1A"
    },
    {
      "type": "ITextControl",
      "bounds": {
        "from": "parent",
        "fracV": [0.1, true],
        "pad": -20
      },
      "text": "My Plugin",
      "fontSize": 24,
      "align": "center",
      "color": "#FFFFFF"
    },
    {
      "type": "IVKnobControl",
      "id": "gainKnob",
      "param": "kGain",
      "bounds": {
        "centredInside": [120, 120]
      },
      "label": "Gain",
      "style": "knob"
    }
  ]
}
```

---

## Future Improvements Needed

1. **Expression Parser**: Support multiple operators, parentheses, functions
2. **Two-Pass Layout**: Position parents before children
3. **Bitmap/SVG Support**: Load images from JSON
4. **Action System**: Callbacks for buttons and controls
5. **Web Support**: Embed JSON or fetch from URL
6. **Partial Reload**: Only recreate changed controls
7. **Flexbox**: Proper flex container implementation
8. **Validation**: JSON schema validation with helpful errors
