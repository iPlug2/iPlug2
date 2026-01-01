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

**Vector Controls**:
| JSON Type | IGraphics Control | Notes |
|-----------|------------------|-------|
| `IVKnobControl` | IVKnobControl | Vector knob, supports `style`, `styleOverrides` |
| `IVSliderControl` | IVSliderControl | Supports `direction` |
| `IVButtonControl` | IVButtonControl | Supports `action` |
| `IVToggleControl` | IVToggleControl | Supports `offText`, `onText` |
| `IVSwitchControl` | IVSwitchControl | Supports `numStates` |
| `IVTabSwitchControl` | IVTabSwitchControl | Supports `labels` array |
| `IVGroupControl` | IVGroupControl | Container, supports `children` |
| `IVPanelControl` | IVPanelControl | Container, supports `children` |
| `IVTabbedPagesControl` | IVTabbedPagesControl | Tabbed container, supports `pages` |
| `IVLabelControl` | IVLabelControl | Styled label |
| `ITextControl` | ITextControl | Basic text |
| `IPanelControl` | IPanelControl | Basic colored panel |

**Bitmap Controls** (require resource mapping):
| JSON Type | IGraphics Control | Notes |
|-----------|------------------|-------|
| `IBKnobControl` | IBKnobControl | `bitmap`, `direction` |
| `IBSliderControl` | IBSliderControl | `trackBitmap`, `handleBitmap`, `direction` |
| `IBButtonControl` | IBButtonControl | `bitmap`, `action` |
| `IBSwitchControl` | IBSwitchControl | `bitmap` |

**SVG Controls** (require resource mapping):
| JSON Type | IGraphics Control | Notes |
|-----------|------------------|-------|
| `ISVGKnobControl` | ISVGKnobControl | `svg` |
| `ISVGSliderControl` | ISVGSliderControl | `trackSVG`, `handleSVG`, `direction` |
| `ISVGButtonControl` | ISVGButtonControl | `offSVG`, `onSVG`, `action` |
| `ISVGToggleControl` | ISVGToggleControl | `offSVG`, `onSVG` |
| `ISVGSwitchControl` | ISVGSwitchControl | `svgs` array (2 SVGs) |

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
  "vShift": 50,               // GetVShifted(50) - supports expressions
  "hShift": "33%",            // GetHShifted() - supports expressions
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

---

## Property Reference

### Common Control Properties

All controls support these properties:

| Property | Type | Description |
|----------|------|-------------|
| `type` | string | **Required.** Control type name (e.g., `"IVKnobControl"`) |
| `bounds` | array/object | Position and size (see Bounds Properties) |
| `id` | string | Control identifier for `GetControlById()` and tag mapping |
| `group` | string | Control group name |
| `param` | string/int | Parameter name (from mapping) or index |
| `label` | string | Label text for vector controls |
| `style` | string/object | Named style or inline style object |
| `styleOverrides` | object | Properties to override from named style |

### Control-Specific Properties

**IVSliderControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `direction` | string | `"vertical"` | `"vertical"` or `"horizontal"` |

**IVButtonControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `action` | string | splash | Action name from mapping |

**IVToggleControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `offText` | string | `"OFF"` | Text when off |
| `onText` | string | `"ON"` | Text when on |

**IVSwitchControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `numStates` | int | 2 | Number of switch states |

**IVTabSwitchControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `labels` | array | - | Array of label strings |
| `direction` | string | `"horizontal"` | `"horizontal"` or `"vertical"` |
| `labelOffset` | float | 10 | Label offset distance |

**IVTabbedPagesControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `tabBarHeight` | float | 20 | Height of tab bar |
| `tabBarFrac` | float | 0.5 | Tab bar width fraction |
| `tabsAlign` | string | `"near"` | `"near"`, `"center"`, `"far"` |
| `pages` | object | - | Map of page name to page definition |

**IVTabbedPagesControl page definition**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `padding` | float | 10 | Page content padding |
| `children` | array | - | Controls on this page |

**ITextControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `text` | string | `""` | Display text |
| `fontSize` | float | - | Font size |
| `align` | string | `"center"` | `"near"`, `"center"`, `"far"` |
| `color` | color | - | Text color |
| `bgColor` | color | - | Background color |

**IPanelControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `color` | color | - | Panel fill color |
| `drawFrame` | bool | false | Draw border frame |

**IVGroupControl / IVPanelControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `children` | array | - | Child controls (see Container Controls) |

**IBKnobControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `bitmap` | string | - | Bitmap name from mapping |
| `direction` | string | `"vertical"` | Drag direction |

**IBSliderControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `trackBitmap` | string | - | Track bitmap name |
| `handleBitmap` | string | - | Handle bitmap name |
| `direction` | string | `"vertical"` | Slider direction |

**IBButtonControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `bitmap` | string | - | Bitmap name from mapping |
| `action` | string | splash | Action name from mapping |

**IBSwitchControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `bitmap` | string | - | Bitmap name from mapping |

**ISVGKnobControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `svg` | string | - | SVG name from mapping |

**ISVGSliderControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `trackSVG` | string | - | Track SVG name |
| `handleSVG` | string | - | Handle SVG name |
| `direction` | string | `"vertical"` | Slider direction |

**ISVGButtonControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `offSVG` | string | - | Off state SVG |
| `onSVG` | string | - | On state SVG |
| `action` | string | splash | Action name from mapping |

**ISVGToggleControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `offSVG` | string | - | Off state SVG |
| `onSVG` | string | - | On state SVG |

**ISVGSwitchControl**:
| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `svgs` | array | - | Array of 2 SVG names |

### Bounds Properties

Bounds can be specified as an array or object:

**Array format**: `[x, y, width, height]` - absolute position relative to parent

**Object format** - operations applied in this order:

| Property | Type | Description |
|----------|------|-------------|
| `from` | string | `"parent"` to start from parent bounds |
| `pad` | float | `GetPadded()` - shrink/grow all edges |
| `fracV` | array | `[fraction, fromTop]` - `FracRectVertical()` |
| `fracH` | array | `[fraction, fromLeft]` - `FracRectHorizontal()` |
| `reduceFromTop` | float | `ReduceFromTop()` |
| `reduceFromBottom` | float | `ReduceFromBottom()` |
| `reduceFromLeft` | float | `ReduceFromLeft()` |
| `reduceFromRight` | float | `ReduceFromRight()` |
| `centredInside` | array | `[width, height]` - `GetCentredInside()` |
| `midVPadded` | float | `GetMidVPadded()` - center vertically with padding |
| `midHPadded` | float | `GetMidHPadded()` - center horizontally with padding |
| `vShift` | float/string | `GetVShifted()` - vertical offset (supports expressions) |
| `hShift` | float/string | `GetHShifted()` - horizontal offset (supports expressions) |
| `x` | float/string | X position (expression supported) |
| `y` | float/string | Y position (expression supported) |
| `w` | float/string | Width (expression supported) |
| `h` | float/string | Height (expression supported) |

**Expression values**: `"50%"`, `"parent.w"`, `"parent.h"`, `"parent.w - 20"`, `"parent.h * 0.5"`

### Style Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `showLabel` | bool | true | Show control label |
| `showValue` | bool | true | Show control value |
| `drawFrame` | bool | true | Draw border frame |
| `drawShadows` | bool | true | Draw drop shadows |
| `emboss` | bool | false | Embossed appearance |
| `roundness` | float | 0 | Corner roundness (0-1) |
| `frameThickness` | float | 1 | Frame line thickness |
| `shadowOffset` | float | 3 | Shadow offset distance |
| `widgetFrac` | float | 0.75 | Widget size as fraction of bounds |
| `angle` | float | 0 | Rotation angle |
| `colorBG` | color | - | Background color |
| `colorFG` | color | - | Foreground/fill color |
| `colorPR` | color | - | Pressed state color |
| `colorFR` | color | - | Frame color |
| `colorHL` | color | - | Highlight color |
| `colorSH` | color | - | Shadow color |
| `colorX1` | color | - | Extra color 1 |
| `colorX2` | color | - | Extra color 2 |
| `colorX3` | color | - | Extra color 3 |
| `labelText` | object | - | Label text style (see Text Properties) |
| `valueText` | object | - | Value text style (see Text Properties) |

### Text Properties

Used in `labelText` and `valueText` style objects:

| Property | Type | Description |
|----------|------|-------------|
| `size` | float | Font size in points |
| `color` | color | Text color |
| `align` | string | Horizontal: `"near"`, `"center"`, `"far"` |
| `valign` | string | Vertical: `"top"`, `"middle"`, `"bottom"` |

### Color Formats

Colors can be specified as:
- Hex string: `"#RRGGBB"` or `"#RRGGBBAA"`
- RGB array: `[255, 128, 0]`
- RGBA array: `[255, 128, 0, 200]`

---

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

**Style Overrides** (combine named style with inline tweaks):
```json
{
  "type": "IVKnobControl",
  "style": "myStyle",
  "styleOverrides": {
    "colorFG": "#00FF00",
    "roundness": 0.8
  }
}
```
This applies `myStyle` first, then overrides specific properties.

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

### Bitmap and SVG Resource Mapping

Bitmaps and SVGs must be loaded in plugin code and mapped by name before JSON loading:

```cpp
// Load bitmaps and SVGs first
IBitmap knobBitmap = pGraphics->LoadBitmap("knob.png", 60);  // 60 frames
ISVG knobSVG = pGraphics->LoadSVG("knob.svg");

// Map by name for JSON to reference
mJSONUI->SetBitmapMapping({
  {"knobBG", knobBitmap},
  {"sliderHandle", pGraphics->LoadBitmap("handle.png")}
});

mJSONUI->SetSVGMapping({
  {"knobSVG", knobSVG}
});

// Now Load() can reference these by name
mJSONUI->Load(...);
```

```json
{
  "type": "IBKnobControl",
  "bitmap": "knobBG",
  "param": "kGain",
  "bounds": { "centredInside": [80, 80] }
}
```

### Action Function Mapping

Custom action functions for buttons:

```cpp
mJSONUI->SetActionMapping({
  {"savePreset", [this](IControl* pControl) {
    // Custom save logic
    mDelegate->SavePreset();
  }},
  {"randomize", [this](IControl* pControl) {
    // Randomize parameters
  }}
});
```

```json
{
  "type": "IVButtonControl",
  "action": "savePreset",
  "label": "Save",
  "bounds": [10, 10, 80, 30]
}
```

If no action is specified, buttons use `SplashClickActionFunc` (visual splash feedback).

### Container Controls

Controls that inherit from `IContainerBase` (IVGroupControl, IVPanelControl) can have `children`:

```json
{
  "type": "IVGroupControl",
  "label": "Oscillator",
  "bounds": { "from": "parent", "fracH": [0.5, true] },
  "style": "main",
  "children": [
    {
      "type": "IVKnobControl",
      "param": "kFreq",
      "bounds": { "from": "parent", "pad": -10, "fracH": [0.5, true] }
    },
    {
      "type": "IVKnobControl",
      "param": "kAmp",
      "bounds": { "from": "parent", "pad": -10, "fracH": [0.5, false] }
    }
  ]
}
```

Children use `"from": "parent"` to reference the container's bounds. The parent-child relationship is properly established via `IContainerBase::AddChildControl()`.

### IVTabbedPagesControl

For tabbed interfaces with multiple pages:

```json
{
  "type": "IVTabbedPagesControl",
  "bounds": { "from": "parent", "pad": -20 },
  "label": "Settings",
  "tabBarHeight": 25,
  "tabBarFrac": 0.5,
  "tabsAlign": "near",
  "style": "main",
  "pages": {
    "Main": {
      "padding": 10,
      "children": [
        { "type": "IVKnobControl", "param": "kGain", "bounds": { "centredInside": [100, 100] } }
      ]
    },
    "FX": {
      "children": [
        { "type": "IVSliderControl", "param": "kMix", "bounds": { "from": "parent", "pad": -20 } }
      ]
    }
  }
}
```

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `tabBarHeight` | float | 20 | Height of the tab bar |
| `tabBarFrac` | float | 0.5 | Fraction of width for tab bar |
| `tabsAlign` | string | "near" | Tab alignment: "near", "center", "far" |
| `pages` | object | - | Map of page names to page definitions |
| `pages.*.padding` | float | 10 | Padding inside each page |
| `pages.*.children` | array | - | Controls on this page |

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

1. **Layout Operation Order is Fixed**
   Operations are applied in this order regardless of JSON key order:
   `from` → `pad` → `fracV` → `fracH` → `reduceFrom*` → `centredInside` → `midVPadded` → `midHPadded` → `vShift` → `hShift` → `x/y/w/h`

2. **Child Layout Timing**
   Children reference parent bounds, but parent bounds may not be finalized on the first layout pass. This can cause incorrect positioning.

3. **Control Index Coupling**
   `mLayouts` indices must match `IGraphics` control indices. Adding controls outside of JSON breaks the mapping.

### Missing Features

| Feature | Status |
|---------|--------|
| Bitmap controls (IBitmapControl, etc.) | ✅ Implemented via resource mapping |
| SVG controls (ISVGKnob, etc.) | ✅ Implemented via resource mapping |
| Meter/visualizer controls | Not implemented |
| Custom action handlers | ✅ Implemented via action mapping |
| Image/bitmap loading from JSON | Plugin pre-loads, maps by name |
| SVG loading from JSON | Plugin pre-loads, maps by name |
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
3. ~~**Bitmap/SVG Support**~~: ✅ Implemented via resource mapping
4. ~~**Action System**~~: ✅ Implemented via action mapping
5. **Web Support**: Embed JSON or fetch from URL
6. **Partial Reload**: Only recreate changed controls
7. **Flexbox**: Proper flex container implementation
8. **Validation**: JSON schema validation with helpful errors
9. **JSON-based Resource Loading**: Load bitmaps/SVGs directly from JSON without plugin code
