# IParam API Reference

Complete reference for the `IParam` class (`IPlug/IPlugParameter.h`).

## Parameter Types

```cpp
enum EParamType { kTypeNone, kTypeBool, kTypeInt, kTypeEnum, kTypeDouble };
```

Set automatically by the Init method used.

## Init Methods

### InitBool

```cpp
void InitBool(const char* name, bool defaultValue,
  const char* label = "", int flags = 0, const char* group = "",
  const char* offText = "off", const char* onText = "on");
```

Custom display text: `InitBool("Bypass", false, "", 0, "", "Disabled", "Enabled")`

### InitEnum (varargs)

```cpp
void InitEnum(const char* name, int defaultValue, int nEnums,
  const char* label = "", int flags = 0, const char* group = "",
  const char* listItems = 0, ...);
```

Example: `InitEnum("Mode", 0, 3, "", 0, "", "Clean", "Overdrive", "Fuzz")`

### InitEnum (initializer_list)

```cpp
void InitEnum(const char* name, int defaultValue,
  const std::initializer_list<const char*>& listItems,
  int flags = 0, const char* group = "");
```

Example: `InitEnum("Shape", 0, {"Sine", "Triangle", "Square", "Saw"})`

### InitInt

```cpp
void InitInt(const char* name, int defaultValue, int minVal, int maxVal,
  const char* label = "", int flags = 0, const char* group = "");
```

### InitDouble

The most flexible Init method:

```cpp
void InitDouble(const char* name, double defaultVal,
  double minVal, double maxVal, double step,
  const char* label = "", int flags = 0, const char* group = "",
  const Shape& shape = ShapeLinear(),
  EParamUnit unit = kUnitCustom,
  DisplayFunc displayFunc = nullptr);
```

### Specialized Init Methods

All have sensible default ranges:

```cpp
void InitGain(name, default=0., min=-70., max=24., step=0.5, flags=0, group="");
void InitFrequency(name, default=1000., min=0.1, max=10000., step=0.1, flags=0, group="");
void InitPercentage(name, default=0., min=0., max=100., flags=0, group="");
void InitSeconds(name, default=1., min=0., max=10., step=0.1, flags=0, group="");
void InitMilliseconds(name, default=1., min=0., max=100., flags=0, group="");
void InitPitch(name, default=60, min=0, max=128, flags=0, group="", middleCisC4=false);
void InitAngleDegrees(name, default=0., min=0., max=360., flags=0, group="");
```

### Init (Clone)

```cpp
void Init(const IParam& p,
  const char* searchStr = "", const char* replaceStr = "",
  const char* newGroup = "");
```

Copies type, range, shape, flags, display text from another parameter. Optionally substitutes strings in the name and changes the group.

## Value Access

| Method | Returns | Notes |
|--------|---------|-------|
| `Value()` | `double` | Raw non-normalized value |
| `Bool()` | `bool` | True if value >= 0.5 |
| `Int()` | `int` | Truncated to integer |
| `DBToAmp()` | `double` | dB to linear amplitude |
| `GetNormalized()` | `double` | 0-1 normalized value |
| `GetDefault(normalized)` | `double` | Default value |
| `GetMin()` | `double` | Minimum value |
| `GetMax()` | `double` | Maximum value |
| `GetRange()` | `double` | Max - Min |
| `GetStep()` | `double` | Step size |

## Value Setting

| Method | Input | Notes |
|--------|-------|-------|
| `Set(value)` | Real value | Constrains to range |
| `SetNormalized(value)` | 0-1 | Converts via shape |
| `SetString(str)` | Text | Parses to value |
| `SetToDefault()` | -- | Resets to default |
| `SetDefault(value)` | Real value | Changes default and sets to it |

## Normalization

```cpp
double ToNormalized(double nonNormalizedValue);   // real → 0-1
double FromNormalized(double normalizedValue);     // 0-1 → real
double Constrain(double value);                    // clip + step
double ConstrainNormalized(double normalizedValue);
```

## Display

```cpp
void GetDisplay(WDL_String& display, bool withDisplayText = true);
void GetDisplayWithLabel(WDL_String& display, bool withDisplayText = true);
void SetDisplayText(double value, const char* str);   // map value → text
void SetDisplayPrecision(int precision);
void SetDisplayFunc(DisplayFunc func);                 // custom lambda
void SetLabel(const char* label);                      // change unit label
```

Custom display function example:

```cpp
GetParam(kFreq)->InitFrequency("Freq", 1000., 20., 20000.);
GetParam(kFreq)->SetDisplayFunc([](double value, WDL_String& display) {
  if (value >= 1000.)
    display.SetFormatted(32, "%.1f kHz", value / 1000.);
  else
    display.SetFormatted(32, "%.0f Hz", value);
});
```

## Display Text Mapping

Map specific values to text labels (useful for enum-like doubles):

```cpp
GetParam(kMode)->InitDouble("Mode", 0, 0, 2, 1);
GetParam(kMode)->SetDisplayText(0, "Off");
GetParam(kMode)->SetDisplayText(1, "Low");
GetParam(kMode)->SetDisplayText(2, "High");
```

Query: `GetDisplayText(value)`, `MapDisplayText(str, &value)`, `NDisplayTexts()`, `GetDisplayTextAtIdx(idx, &value)`.

## Properties

```cpp
const char* GetName();
const char* GetLabel();         // unit suffix
const char* GetGroup();
EParamType Type();
EParamUnit Unit();
EDisplayType DisplayType();     // from shape
int GetFlags();
bool GetCanAutomate();
bool GetStepped();
bool GetNegateDisplay();
bool GetSignDisplay();
bool GetMeta();
EShapeIDs GetShapeID();
double GetShapeValue();
```

## Flags

```cpp
enum EFlags {
  kFlagsNone          = 0,
  kFlagCannotAutomate = 0x1,   // Not automatable by host
  kFlagStepped        = 0x2,   // Discrete stepped values
  kFlagNegateDisplay  = 0x4,   // Display as negative
  kFlagSignDisplay    = 0x8,   // Display with +/- sign
  kFlagMeta           = 0x10,  // Influences other parameters
};
```

## AU Parameter Units

Used by AudioUnit hosts for parameter display. Set via the `unit` arg on `InitDouble`:

```cpp
enum EParamUnit {
  kUnitPercentage, kUnitSeconds, kUnitMilliseconds, kUnitSamples,
  kUnitDB, kUnitLinearGain, kUnitPan, kUnitPhase, kUnitDegrees,
  kUnitMeters, kUnitRate, kUnitRatio, kUnitFrequency, kUnitOctaves,
  kUnitCents, kUnitAbsCents, kUnitSemitones, kUnitMIDINote,
  kUnitMIDICtrlNum, kUnitBPM, kUnitBeats, kUnitCustom
};
```

The specialized Init methods (InitGain, InitFrequency, etc.) set the correct unit automatically.

## Shapes

### ShapeLinear (Default)

Linear mapping from normalized to real values:
```cpp
IParam::ShapeLinear()
```

### ShapePowCurve

Power curve mapping. Higher values = more resolution at low end:
```cpp
IParam::ShapePowCurve(3.)   // exponent 3 → fine control at low values
IParam::ShapePowCurve(0.5)  // exponent 0.5 → fine control at high values
```

Common usage: ADSR times, where ms-level control matters at the low end:
```cpp
GetParam(kAttack)->InitDouble("Attack", 10., 0.1, 5000., 0.1, "ms",
  0, "Env", IParam::ShapePowCurve(3.));
```

### ShapeExp

True exponential mapping (uses ln/exp internally). Suitable for frequency:
```cpp
IParam::ShapeExp()
```

## JSON Export

```cpp
void GetJSON(WDL_String& json, int idx);  // Full param description as JSON
void PrintDetails();                        // Debug print to console
```
