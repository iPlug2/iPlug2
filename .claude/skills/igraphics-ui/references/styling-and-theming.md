# Styling and Theming

## IVStyle

Controls the visual presentation of all IV (vector) controls. Start from `DEFAULT_STYLE` and customize with fluent `With...()` methods.

### Fields

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| showLabel | bool | true | Show control label |
| showValue | bool | true | Show parameter value |
| colorSpec | IVColorSpec | defaults | 9-slot color palette |
| labelText | IText | 12pt center | Label text style |
| valueText | IText | 12pt center | Value text style |
| hideCursor | bool | true | Hide mouse cursor during drag |
| drawFrame | bool | true | Draw border frame |
| drawShadows | bool | true | Draw drop shadows |
| emboss | bool | false | Embossed 3D effect |
| roundness | float | 0.0 | Corner radius factor (0-1) |
| frameThickness | float | 1.0 | Border stroke thickness |
| shadowOffset | float | 3.0 | Shadow distance |
| widgetFrac | float | 1.0 | Fraction of rect the widget occupies (0-1) |
| angle | float | 0.0 | Widget angle in degrees (for knob indicators) |
| labelOrientation | EOrientation | North | Label placement direction |

### Fluent API

All return a new IVStyle:

```
.WithShowLabel(bool)          .WithShowValue(bool)
.WithLabelText(IText)         .WithValueText(IText)
.WithHideCursor(bool)         .WithDrawFrame(bool)
.WithDrawShadows(bool)        .WithEmboss(bool)
.WithRoundness(float)         .WithFrameThickness(float)
.WithShadowOffset(float)      .WithWidgetFrac(float)
.WithAngle(float)             .WithLabelOrientation(EOrientation)
.WithColor(EVColor, IColor)   .WithColors(IVColorSpec)
```

### Example Styles

```cpp
// Dark flat style
const IVStyle darkFlat = DEFAULT_STYLE
  .WithColor(kBG, COLOR_BLACK)
  .WithColor(kFG, IColor(255, 80, 80, 80))
  .WithColor(kPR, IColor(255, 200, 200, 200))
  .WithDrawShadows(false)
  .WithDrawFrame(false)
  .WithRoundness(0.5f);

// Compact style (no label/value)
const IVStyle compact = DEFAULT_STYLE
  .WithShowLabel(false)
  .WithShowValue(false)
  .WithWidgetFrac(0.5f);
```

---

## IVColorSpec

9-slot color palette used by IVStyle. Slots are indexed by `EVColor` enum.

| Slot | Enum | Default | Purpose |
|------|------|---------|---------|
| Background | `kBG` | transparent | Control background |
| Foreground | `kFG` | light gray | Off/inactive state |
| Pressed | `kPR` | blue | On/pressed/active state |
| Frame | `kFR` | black | Border stroke |
| Highlight | `kHL` | translucent | Mouse-over highlight, splash animation |
| Shadow | `kSH` | dark gray | Drop shadow |
| Extra 1 | `kX1` | black | Indicator track (knobs, sliders) |
| Extra 2 | `kX2` | default | User-defined |
| Extra 3 | `kX3` | default | User-defined |

Constructor takes initializer list: `IVColorSpec {kBGColor, kFGColor, kPRColor, kFRColor, kHLColor, kSHColor, kX1Color, kX2Color, kX3Color}`

---

## IText

Text style for labels, values, and standalone text controls.

### Constructor
```cpp
IText(float size, IColor color, const char* font, EAlign align, EVAlign valign, float angle,
      IColor textEntryBGColor, IColor textEntryFGColor)
```

Most arguments are optional. Common patterns:
```cpp
IText(12.f)                                    // 12pt, defaults
IText(24.f, COLOR_WHITE)                       // 24pt white
IText(16.f, COLOR_WHITE, "Roboto-Regular", EAlign::Near, EVAlign::Top)
```

### Fluent API
```
.WithFGColor(IColor)  .WithSize(float)  .WithFont(const char*)
.WithAlign(EAlign)    .WithVAlign(EVAlign)  .WithAngle(float)
```

### Built-in Constants
- `DEFAULT_TEXT` -- 14pt, default color, centered
- `DEFAULT_LABEL_TEXT` -- smaller label text
- `DEFAULT_VALUE_TEXT` -- value display text

### Font Loading
```cpp
pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);  // from resource
pGraphics->LoadFont("ForkAwesome", FORK_AWESOME_FN); // icon font
```

---

## IColor

ARGB color with alpha channel. Constructor: `IColor(a, r, g, b)` -- note alpha first.

### Named Constants
`COLOR_BLACK`, `COLOR_WHITE`, `COLOR_RED`, `COLOR_GREEN`, `COLOR_BLUE`, `COLOR_YELLOW`, `COLOR_ORANGE`, `COLOR_GRAY`, `COLOR_LIGHT_GRAY`, `COLOR_DARK_GRAY`, `COLOR_MID_GRAY`, `COLOR_TRANSPARENT`, `COLOR_TRANSLUCENT`

### Methods
- `WithOpacity(float)` -- return color with modified alpha (0.0-1.0)
- `WithContrast(float)` -- lighten/darken
- `GetRandomColor()` -- static, random color
- `LinearInterpolateBetween(c1, c2, t)` -- static, blend between two colors

---

## IPattern (Gradients)

For gradient fills on controls or custom drawing.

```cpp
// Solid color
IPattern solidPat(COLOR_RED);

// Linear gradient
IPattern linGrad = IPattern::CreateLinearGradient(bounds, EDirection::Vertical,
  {{COLOR_WHITE, 0.f}, {COLOR_BLACK, 1.f}});

// Radial gradient
IPattern radGrad = IPattern::CreateRadialGradient(bounds.MW(), bounds.MH(), bounds.W()/2.f,
  {{COLOR_WHITE, 0.f}, {COLOR_BLACK, 1.f}});
```

Color stops: `{IColor, float offset}` where offset is 0.0 to 1.0.
