# Batch Parameter Operations

Methods on `IPluginBase` (`IPlug/IPlugPluginBase.h`) for operating on multiple parameters at once.

## Randomization

### RandomiseParamValues

Randomize parameter values uniformly across each parameter's range:

```cpp
// Randomize all parameters
RandomiseParamValues();

// Randomize a range (inclusive)
RandomiseParamValues(kOsc1Freq, kOsc1Detune);

// Randomize a group
RandomiseParamValues("Oscillator");
```

For custom randomization logic (e.g., weighted, constrained, or with a specific RNG), use `ForParamInRange` or `ForParamInGroup`:

```cpp
// Randomize only continuous parameters, skip bools/enums
ForParamInRange(0, NParams() - 1, [](int idx, IParam& param) {
  if (param.Type() == IParam::kTypeDouble) {
    double range = param.GetRange();
    param.Set(param.GetMin() + range * (std::rand() / (double)RAND_MAX));
  }
});

// Randomize with gaussian distribution centered on default
ForParamInGroup("Filter", [](int idx, IParam& param) {
  double center = param.GetDefault();
  double spread = param.GetRange() * 0.3;
  double value = center + spread * ((std::rand() / (double)RAND_MAX) - 0.5);
  param.Set(value);  // Set() constrains to [min, max] automatically
});
```

## Resetting to Defaults

```cpp
// Reset all parameters to their default values
DefaultParamValues();

// Reset a range
DefaultParamValues(kFilterFreq, kFilterQ);

// Reset a group
DefaultParamValues("Filter");
```

## Copying Values

### Between Ranges

```cpp
// Copy nParams values from srcIdx to destIdx
CopyParamValues(kOsc1Freq, kOsc2Freq, 4);  // copy 4 params
```

### Between Groups

```cpp
// Copy all values from one group to another (groups must have same param count)
CopyParamValues("Channel A", "Channel B");
```

Useful for "copy channel" or "copy oscillator" features.

## Iterating Parameters

### ForParamInRange

Apply a lambda to each parameter in an index range (inclusive):

```cpp
ForParamInRange(int startIdx, int endIdx,
  std::function<void(int paramIdx, IParam& param)> func);
```

```cpp
// Mute all gain parameters
ForParamInRange(kGain1, kGain8, [](int idx, IParam& param) {
  param.Set(param.GetMin());
});

// Print all parameter values in a range
ForParamInRange(0, NParams() - 1, [](int idx, IParam& param) {
  WDL_String display;
  param.GetDisplayWithLabel(display);
  DBGMSG("Param %i (%s): %s\n", idx, param.GetName(), display.Get());
});
```

### ForParamInGroup

Apply a lambda to each parameter in a named group:

```cpp
ForParamInGroup(const char* paramGroup,
  std::function<void(int paramIdx, IParam& param)> func);
```

```cpp
// Set all envelope params to fast attack
ForParamInGroup("ADSR", [](int idx, IParam& param) {
  if (strcmp(param.GetName(), "Attack") == 0)
    param.Set(0.5);
});
```

## Bulk Initialization

### InitParamRange

Initialize multiple parameters with the same settings. Use `%i` in the name format string to insert the count:

```cpp
void InitParamRange(int startIdx, int endIdx, int countStart,
  const char* nameFmtStr, double defaultVal,
  double minVal, double maxVal, double step,
  const char* label = "", int flags = 0, const char* group = "",
  const IParam::Shape& shape = IParam::ShapeLinear(),
  IParam::EParamUnit unit = IParam::kUnitCustom,
  IParam::DisplayFunc displayFunc = nullptr);
```

```cpp
// Initialize 16 step sequencer parameters
InitParamRange(kStep0, kStep15, 1, "Step %i", 0.5, 0., 1., 0.01, "", 0, "Sequencer");
// Creates: "Step 1", "Step 2", ..., "Step 16"

// Initialize 8 band gains
InitParamRange(kBand0, kBand7, 1, "Band %i", 0., -12., 12., 0.1, "dB", 0, "EQ");
```

### CloneParamRange

Clone parameters from one range to another, with optional name substitution:

```cpp
void CloneParamRange(int cloneStartIdx, int cloneEndIdx, int startIdx,
  const char* searchStr = "", const char* replaceStr = "",
  const char* newGroup = "");
```

```cpp
// Define Oscillator 1 parameters
GetParam(kOsc1Freq)->InitFrequency("Osc 1 Freq", 440.);
GetParam(kOsc1Fine)->InitDouble("Osc 1 Fine", 0., -100., 100., 0.1, "cents", 0, "Osc 1");
GetParam(kOsc1Wave)->InitEnum("Osc 1 Wave", 0, {"Sine", "Saw", "Square"}, 0, "Osc 1");

// Clone to Oscillator 2 with name/group substitution
CloneParamRange(kOsc1Freq, kOsc1Wave, kOsc2Freq, "Osc 1", "Osc 2", "Osc 2");
// Creates: "Osc 2 Freq", "Osc 2 Fine", "Osc 2 Wave" in group "Osc 2"
```

## Debug Utilities

```cpp
// Print all parameter values to debug console
PrintParamValues();
```

## Parameter Groups API

```cpp
int NParamGroups();                      // Number of unique groups
const char* GetParamGroupName(int idx);  // Group name at index
void InformHostOfParameterDetailsChange(); // Notify host after modifying param labels/details
```

Groups are registered automatically when parameters are initialized with a group name. They appear in host automation lists for organizing parameters.
