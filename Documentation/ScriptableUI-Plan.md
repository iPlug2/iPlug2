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
      "id": "gainKnob",
      "param": "kGain",
      // ABSOLUTE bounds: [x, y, w, h] in pixels
      "bounds": [50, 50, 100, 100],
      "style": "default",
      "label": "Gain"
    },
    {
      "type": "IVSliderControl",
      "id": "mixSlider",
      "param": "kMix",
      // RELATIVE bounds: percentages + offsets (evaluated at runtime)
      "bounds": {
        "x": "80%",           // 80% of parent width
        "y": "10",            // 10px from top
        "w": "40",            // 40px wide
        "h": "parent.h - 20"  // parent height minus 20px
      },
      "style": "minimal",
      "direction": "vertical"
    },
    {
      "type": "ITextControl",
      "id": "title",
      // FRACTIONAL bounds using IRECT-like operations
      "bounds": {
        "from": "parent",
        "pad": -10,
        "fracV": [0.1, true]   // FracRectVertical(0.1, true) = top 10%
      },
      "text": "My Plugin",
      "fontSize": 24,
      "align": "center"
    },
    {
      // FlexBox container - uses Yoga layout engine
      "type": "IFlexBox",
      "id": "mainRow",
      "bounds": { "from": "parent", "pad": -10 },
      "flex": {
        "direction": "row",
        "justify": "space-between",
        "wrap": "wrap",
        "gap": 10
      },
      "children": [
        {
          "type": "IVKnobControl",
          "param": "kOscPitch",
          "flex": { "grow": 1, "basis": 80 },  // flex item properties
          "label": "Pitch"
        },
        {
          "type": "IVKnobControl",
          "param": "kOscShape",
          "flex": { "grow": 1, "basis": 80 },
          "label": "Shape"
        }
      ]
    },
    {
      // Responsive container with breakpoints
      "type": "IVGroupControl",
      "id": "oscillatorGroup",
      "bounds": {
        "from": "parent",
        "fracV": [0.6, false]  // bottom 60%
      },
      "label": "Oscillator",
      // Different layouts at different sizes
      "responsive": {
        "default": { "columns": 4 },
        "maxWidth:400": { "columns": 2 },
        "maxWidth:250": { "columns": 1 }
      },
      "children": [
        { "type": "IVKnobControl", "param": "kOsc1", "label": "Osc 1" },
        { "type": "IVKnobControl", "param": "kOsc2", "label": "Osc 2" },
        { "type": "IVKnobControl", "param": "kOsc3", "label": "Osc 3" },
        { "type": "IVKnobControl", "param": "kOsc4", "label": "Osc 4" }
      ]
    }
  ]
}
```

## Implementation Steps

### Phase 1: Core Parser + Runtime Layout (~400 LOC)

**File: `IGraphics/Extras/IGraphicsJSON.h`**

```cpp
#pragma once
#include "IGraphics.h"
#include "IGraphicsFlexBox.h"
#include <nlohmann/json.hpp>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

using json = nlohmann::json;

// Stored layout definition for a control (evaluated on each resize)
struct LayoutDef
{
  enum class Type { Absolute, Relative, Flex };
  Type type = Type::Absolute;
  json spec;  // original JSON bounds spec
  int parentIdx = -1;  // parent control index, -1 = root
};

class IGraphicsJSON
{
public:
  IGraphicsJSON(IGraphics* pGraphics, IGEditorDelegate* pDelegate);

  // Load UI from JSON file or string
  bool Load(const char* jsonPath);
  bool LoadFromString(const char* jsonStr);

  // RUNTIME LAYOUT: Call on resize to recompute all bounds
  void OnResize(const IRECT& newBounds);

  // Hot reload (debug only)
  void EnableHotReload(bool enable = true);
  void CheckForChanges(); // call from OnIdle

  // Map string IDs to integer tags
  void SetTagMapping(const std::map<std::string, int>& tagMap);
  void SetParamMapping(const std::map<std::string, int>& paramMap);

private:
  bool ParseControls(const json& j);
  bool ParseStyles(const json& j);
  IControl* CreateControl(const json& controlDef, int parentIdx);

  // Layout evaluation
  IRECT EvaluateBounds(const LayoutDef& layout, const IRECT& parentBounds);
  IRECT ParseAbsoluteBounds(const json& bounds, const IRECT& parent);
  IRECT ParseRelativeBounds(const json& bounds, const IRECT& parent);
  float EvaluateExpr(const std::string& expr, const IRECT& parent);

  // FlexBox layout
  void LayoutFlexContainer(int containerIdx, const IRECT& bounds);

  IVStyle ParseStyle(const json& styleDef);
  IVStyle GetStyle(const std::string& name);
  int ResolveParam(const json& param);
  int ResolveTag(const std::string& id);

  IGraphics* mGraphics;
  IGEditorDelegate* mDelegate;
  std::map<std::string, IVStyle> mStyles;
  std::map<std::string, int> mTagMap;
  std::map<std::string, int> mParamMap;

  // Layout state - stored for runtime recalculation
  std::vector<LayoutDef> mLayouts;  // parallel to control indices
  json mRootSpec;  // keep JSON for hot-reload re-parse

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

### Phase 4: Runtime Layout Evaluation (~150 LOC)

```cpp
// Called by IGraphics on resize (via SetLayoutOnResize(true))
void IGraphicsJSON::OnResize(const IRECT& newBounds)
{
  // Iterate controls in order (parents before children)
  for (int i = 0; i < mLayouts.size(); i++) {
    const auto& layout = mLayouts[i];

    // Get parent bounds (root = newBounds, otherwise parent control's bounds)
    IRECT parentBounds = (layout.parentIdx < 0)
      ? newBounds
      : mGraphics->GetControl(layout.parentIdx)->GetRECT();

    // Evaluate this control's bounds
    IRECT bounds = EvaluateBounds(layout, parentBounds);
    mGraphics->GetControl(i)->SetTargetAndDrawRECTs(bounds);
  }

  // Re-layout flex containers (children positions depend on container)
  for (int i = 0; i < mFlexContainers.size(); i++) {
    LayoutFlexContainer(mFlexContainers[i],
      mGraphics->GetControl(mFlexContainers[i])->GetRECT());
  }
}

IRECT IGraphicsJSON::EvaluateBounds(const LayoutDef& layout, const IRECT& parent)
{
  const json& b = layout.spec;

  // Simple array: [x, y, w, h] absolute pixels
  if (b.is_array()) {
    return IRECT(b[0], b[1], b[0] + b[2], b[1] + b[3]);
  }

  // Object with expressions
  IRECT result = parent;

  if (b.contains("from") && b["from"] == "parent") {
    result = parent;
  }

  // Apply IRECT operations (mirrors C++ API)
  if (b.contains("pad")) {
    result = result.GetPadded(b["pad"].get<float>());
  }
  if (b.contains("fracV")) {
    auto fv = b["fracV"];
    result = result.FracRectVertical(fv[0].get<float>(), fv[1].get<bool>());
  }
  if (b.contains("fracH")) {
    auto fh = b["fracH"];
    result = result.FracRectHorizontal(fh[0].get<float>(), fh[1].get<bool>());
  }
  if (b.contains("reduceFromRight")) {
    result = result.ReduceFromRight(b["reduceFromRight"].get<float>());
  }
  if (b.contains("reduceFromBottom")) {
    result = result.ReduceFromBottom(b["reduceFromBottom"].get<float>());
  }

  // Expression-based: "x": "50%", "w": "parent.w * 0.5 - 10"
  if (b.contains("x") || b.contains("y") || b.contains("w") || b.contains("h")) {
    float x = b.contains("x") ? EvaluateExpr(b["x"], parent) : parent.L;
    float y = b.contains("y") ? EvaluateExpr(b["y"], parent) : parent.T;
    float w = b.contains("w") ? EvaluateExpr(b["w"], parent) : parent.W();
    float h = b.contains("h") ? EvaluateExpr(b["h"], parent) : parent.H();
    result = IRECT(x, y, x + w, y + h);
  }

  return result;
}

// Simple expression evaluator: "50%", "parent.w - 20", "100"
float IGraphicsJSON::EvaluateExpr(const json& val, const IRECT& parent)
{
  if (val.is_number()) return val.get<float>();

  std::string expr = val.get<std::string>();

  // Percentage: "50%" = 50% of parent dimension (context-dependent)
  if (expr.back() == '%') {
    return std::stof(expr.substr(0, expr.size()-1)) / 100.0f * parent.W();
  }

  // Simple expressions: "parent.w - 20", "parent.h * 0.5"
  // (For complex expressions, would use a proper parser or Lua)
  // ... minimal expression evaluator ...

  return std::stof(expr);  // fallback: parse as number
}
```

### Phase 5: Hot Reload (~50 LOC, debug only)

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

### Phase 6: Plugin Integration

**Usage in plugin:**

```cpp
// In config.h - map enum names to strings for JSON
#define PARAM_MAP { {"kGain", kGain}, {"kMix", kMix}, {"kOscPitch", kOscPitch} }
#define TAG_MAP { {"gainKnob", kGainKnob}, {"mixSlider", kMixSlider} }

// In Plugin.cpp
mLayoutFunc = [&](IGraphics* pGraphics) {
  const IRECT b = pGraphics->GetBounds();

  // First call: create controls
  if (pGraphics->NControls() == 0) {
    pGraphics->SetLayoutOnResize(true);  // Enable responsive layout
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    mJSONUI = std::make_unique<IGraphicsJSON>(pGraphics, this);
    mJSONUI->SetParamMapping(PARAM_MAP);
    mJSONUI->SetTagMapping(TAG_MAP);

#ifdef _DEBUG
    mJSONUI->Load(PLUG_RESOURCES_PATH "/ui.json");
    mJSONUI->EnableHotReload(true);
#else
    mJSONUI->LoadFromString(UI_JSON_RESOURCE);
#endif
  }

  // On resize: recompute all bounds from JSON layout definitions
  mJSONUI->OnResize(b);
};

// In OnIdle (for hot reload)
void OnIdle() override {
  if (mJSONUI) mJSONUI->CheckForChanges();
}
```

**How it works:**
1. `SetLayoutOnResize(true)` makes IGraphics call `mLayoutFunc` on every resize
2. First call creates controls with initial bounds from JSON
3. Subsequent calls trigger `OnResize()` which re-evaluates all layout expressions
4. Bounds like `"x": "50%"` or `"fracV": [0.25, true]` recalculate based on new window size

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
