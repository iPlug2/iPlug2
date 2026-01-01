# Scriptable IGraphics UI - Implementation Plan

## Overview

Add JSON-based declarative UI definitions to iPlug2, enabling hot-reload during development without recompilation. Uses existing `nlohmann::json` (already in Dependencies/Extras/).

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  ui.json                                                    │
│  { "controls": [...], "styles": {...} }                     │
└─────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  IGraphicsJSON (new class ~500 LOC)                         │
│  - Parse JSON → IControl instances                          │
│  - Style registry                                           │
│  - Hot-reload via file watcher (debug only)                 │
└─────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│  IGraphics (existing)                                       │
│  AttachControl(), RemoveAllControls(), etc.                 │
└─────────────────────────────────────────────────────────────┘
```

## JSON Schema

```jsonc
{
  // Optional: inherit/extend another JSON file
  "extends": "base-ui.json",

  // Named styles (reusable)
  "styles": {
    "default": {
      "colorFG": "#FF6600",
      "colorBG": "#333333",
      "roundness": 0.5,
      "drawFrame": true,
      "drawShadows": true,
      "shadowOffset": 3.0
    },
    "minimal": {
      "colorFG": "#FFFFFF",
      "colorBG": "#222222",
      "roundness": 0.0,
      "drawFrame": false
    }
  },

  // Named fonts
  "fonts": {
    "main": { "name": "Roboto-Regular", "resource": "ROBOTO_FN" }
  },

  // Control definitions
  "controls": [
    {
      "type": "IVKnobControl",
      "id": "gainKnob",           // becomes ctrlTag (hashed or mapped)
      "param": "kGain",           // param enum name or index
      "bounds": [50, 50, 150, 150], // [x, y, w, h] or LTRB
      "style": "default",         // reference to styles
      "label": "Gain"
    },
    {
      "type": "IVSliderControl",
      "id": "mixSlider",
      "param": "kMix",
      "bounds": { "x": 200, "y": 50, "w": 40, "h": 150 },
      "style": "minimal",
      "direction": "vertical"
    },
    {
      "type": "ITextControl",
      "id": "title",
      "bounds": [10, 10, 300, 30],
      "text": "My Plugin",
      "fontSize": 24,
      "align": "center"
    },
    {
      // Container with children
      "type": "IVGroupControl",
      "id": "oscillatorGroup",
      "bounds": [10, 200, 300, 200],
      "label": "Oscillator",
      "children": [
        { "type": "IVKnobControl", "param": "kOscPitch", "bounds": [10, 30, 80, 80] }
      ]
    }
  ],

  // Optional: layout helpers
  "layout": {
    "type": "flex",  // or "grid", "manual"
    "direction": "row",
    "gap": 10
  }
}
```

## Implementation Steps

### Phase 1: Core Parser (~300 LOC)

**File: `IGraphics/Extras/IGraphicsJSON.h`**

```cpp
#pragma once
#include "IGraphics.h"
#include <nlohmann/json.hpp>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

using json = nlohmann::json;

class IGraphicsJSON
{
public:
  IGraphicsJSON(IGraphics* pGraphics, IGEditorDelegate* pDelegate);

  // Load UI from JSON file or string
  bool Load(const char* jsonPath);
  bool LoadFromString(const char* jsonStr);

  // Hot reload (debug only)
  void EnableHotReload(bool enable = true);
  void CheckForChanges(); // call from OnIdle

  // Map string IDs to integer tags
  void SetTagMapping(const std::map<std::string, int>& tagMap);
  void SetParamMapping(const std::map<std::string, int>& paramMap);

private:
  bool ParseControls(const json& j);
  bool ParseStyles(const json& j);
  IControl* CreateControl(const json& controlDef);
  IRECT ParseBounds(const json& bounds);
  IVStyle ParseStyle(const json& styleDef);
  IVStyle GetStyle(const std::string& name);
  int ResolveParam(const json& param);
  int ResolveTag(const std::string& id);

  IGraphics* mGraphics;
  IGEditorDelegate* mDelegate;
  std::map<std::string, IVStyle> mStyles;
  std::map<std::string, int> mTagMap;
  std::map<std::string, int> mParamMap;

  // Hot reload state
  std::string mLoadedPath;
  time_t mLastModified = 0;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
```

### Phase 2: Control Factory (~200 LOC)

```cpp
IControl* IGraphicsJSON::CreateControl(const json& def)
{
  std::string type = def["type"];
  IRECT bounds = ParseBounds(def["bounds"]);
  int paramIdx = def.contains("param") ? ResolveParam(def["param"]) : kNoParameter;
  IVStyle style = def.contains("style") ? GetStyle(def["style"]) : DEFAULT_STYLE;

  IControl* pControl = nullptr;

  // Vector controls
  if (type == "IVKnobControl") {
    pControl = new IVKnobControl(bounds, paramIdx,
      def.value("label", ""), style);
  }
  else if (type == "IVSliderControl") {
    EDirection dir = def.value("direction", "vertical") == "vertical"
      ? EDirection::Vertical : EDirection::Horizontal;
    pControl = new IVSliderControl(bounds, paramIdx,
      def.value("label", ""), style, true, dir);
  }
  else if (type == "IVButtonControl") {
    pControl = new IVButtonControl(bounds, nullptr,
      def.value("label", ""), style);
  }
  else if (type == "ITextControl") {
    IText textStyle;
    if (def.contains("fontSize")) textStyle.mSize = def["fontSize"];
    // ... parse other text properties
    pControl = new ITextControl(bounds, def.value("text", "").c_str(), textStyle);
  }
  // ... ~15 more control types

  // Common properties
  if (pControl && def.contains("id")) {
    int tag = ResolveTag(def["id"]);
    // tag is set during AttachControl
  }

  return pControl;
}
```

### Phase 3: Style Parsing (~100 LOC)

```cpp
IVStyle IGraphicsJSON::ParseStyle(const json& j)
{
  IVStyle style = DEFAULT_STYLE;

  if (j.contains("colorFG")) style.colorSpec.mFGColor = ParseColor(j["colorFG"]);
  if (j.contains("colorBG")) style.colorSpec.mBGColor = ParseColor(j["colorBG"]);
  if (j.contains("roundness")) style.roundness = j["roundness"];
  if (j.contains("drawFrame")) style.drawFrame = j["drawFrame"];
  if (j.contains("drawShadows")) style.drawShadow = j["drawShadows"];
  if (j.contains("shadowOffset")) style.shadowOffset = j["shadowOffset"];
  if (j.contains("frameThickness")) style.frameThickness = j["frameThickness"];
  // ...

  return style;
}

IColor ParseColor(const json& c)
{
  if (c.is_string()) {
    // Parse "#RRGGBB" or "#RRGGBBAA"
    std::string s = c;
    // ... hex parsing
  }
  else if (c.is_array()) {
    // [r, g, b] or [r, g, b, a]
    return IColor(
      c.size() > 3 ? c[3].get<int>() : 255,
      c[0], c[1], c[2]
    );
  }
  return COLOR_WHITE;
}
```

### Phase 4: Hot Reload (~50 LOC, debug only)

```cpp
void IGraphicsJSON::EnableHotReload(bool enable)
{
#ifndef NDEBUG
  mHotReloadEnabled = enable;
  if (enable && !mLoadedPath.empty()) {
    mLastModified = GetFileModTime(mLoadedPath.c_str());
  }
#endif
}

void IGraphicsJSON::CheckForChanges()
{
#ifndef NDEBUG
  if (!mHotReloadEnabled || mLoadedPath.empty()) return;

  time_t currentMod = GetFileModTime(mLoadedPath.c_str());
  if (currentMod > mLastModified) {
    mLastModified = currentMod;
    mGraphics->RemoveAllControls();
    Load(mLoadedPath.c_str());
  }
#endif
}
```

### Phase 5: Plugin Integration

**Usage in plugin:**

```cpp
// In config.h - map enum names to strings for JSON
#define PARAM_MAP { {"kGain", kGain}, {"kMix", kMix}, {"kOscPitch", kOscPitch} }
#define TAG_MAP { {"gainKnob", kGainKnob}, {"mixSlider", kMixSlider} }

// In Plugin.cpp
mLayoutFunc = [&](IGraphics* pGraphics) {
  pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
  pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

  auto jsonUI = std::make_unique<IGraphicsJSON>(pGraphics, this);
  jsonUI->SetParamMapping(PARAM_MAP);
  jsonUI->SetTagMapping(TAG_MAP);

#ifdef _DEBUG
  // Load from file path for hot reload
  jsonUI->Load(PLUG_RESOURCES_PATH "/ui.json");
  jsonUI->EnableHotReload(true);
  mJSONUI = std::move(jsonUI); // keep alive
#else
  // Load from embedded resource
  jsonUI->LoadFromString(UI_JSON_RESOURCE);
#endif
};

// In OnIdle (for hot reload)
void OnIdle() override {
  if (mJSONUI) mJSONUI->CheckForChanges();
}
```

## File Structure

```
IGraphics/
├── Extras/
│   ├── IGraphicsJSON.h        # Header
│   └── IGraphicsJSON.cpp      # Implementation (~500 LOC)
│
Examples/
├── IPlugEffectJSON/           # New example project
│   ├── IPlugEffectJSON.cpp
│   ├── IPlugEffectJSON.h
│   ├── config.h
│   └── resources/
│       └── ui.json
```

## Supported Controls (Phase 1)

| Control Type | JSON Properties |
|-------------|-----------------|
| `IVKnobControl` | bounds, param, style, label |
| `IVSliderControl` | bounds, param, style, label, direction |
| `IVButtonControl` | bounds, style, label |
| `IVToggleControl` | bounds, param, style, labelOn, labelOff |
| `IVSwitchControl` | bounds, param, style, numStates |
| `ITextControl` | bounds, text, fontSize, align, color |
| `IVGroupControl` | bounds, label, children |
| `IVPanelControl` | bounds, style |
| `IPanelControl` | bounds, color |

## Future Extensions (Phase 2+)

1. **Expressions for bounds**: `"bounds": "parent.width * 0.5"`
2. **FlexBox layout**: `"layout": { "type": "flex", "direction": "row" }`
3. **Lua/JS for actions**: `"onValueChange": "lua:updateMeter(value)"`
4. **Bitmap/SVG controls**: `"type": "IBKnobControl", "bitmap": "knob.png"`
5. **Presets**: `"preset": "dark-theme"` loading from separate JSON

## Maintenance Considerations

**Low maintenance:**
- JSON schema is stable (additive changes only)
- Control factory pattern isolates changes
- ~500 LOC total, single file

**When IGraphics API changes:**
- Add new control types to factory (1-5 lines each)
- Add new style properties to parser (1 line each)
- Breaking API changes: update CreateControl signatures

**Testing:**
- Unit test: parse sample JSON, verify control count/types
- Visual test: example project with all control types

## Alternatives Considered

| Approach | Pros | Cons |
|----------|------|------|
| **JSON (chosen)** | Zero runtime, hot-reload, simple | No logic/scripting |
| **Lua** | Full scripting | +200KB binary, binding maintenance |
| **YAML** | More readable | Needs parser, no gain over JSON |
| **XML** | Industry standard | Verbose, needs parser |

## Timeline Estimate

- Phase 1-3 (Core): 1-2 days
- Phase 4 (Hot reload): 0.5 day
- Phase 5 (Example): 0.5 day
- Testing/polish: 1 day

**Total: ~4 days of focused work**

## Open Questions

1. **Bounds format**: `[x, y, w, h]` or `[L, T, R, B]`? Recommend x/y/w/h as more intuitive.
2. **Tag resolution**: Auto-generate from string hash, or require explicit mapping?
3. **Resource embedding**: Use existing resource system or custom embedding?
4. **Error handling**: Silent fallback or assert on parse errors?
