---
name: parameters
description: This skill should be used when the user asks to "add a parameter", "define parameters", "create an enum parameter", "add a gain control", "add a frequency parameter", "use parameter groups", "randomize parameters", "reset to defaults", "smooth a parameter", "use LogParamSmooth", "create presets", "serialize state", "handle OnParamChange", "copy parameter values", "use InitDouble", "use InitEnum", "use InitBool", "use parameter shapes", "use ShapePowCurve", "parameter flags", or discusses parameter definition, grouping, batch operations, smoothing, presets, or state serialization in an iPlug2 plugin.
---

# Parameter System

Guidance for defining, grouping, manipulating, smoothing, and serializing parameters in iPlug2 plugins.

## Core Pattern

Parameters are defined as an enum in the plugin header and initialized in the constructor:

```cpp
// Plugin.h
enum EParams {
  kGain = 0,
  kFreq,
  kNumParams
};

// Plugin.cpp constructor
GetParam(kGain)->InitGain("Gain", 0., -70., 24., 0.5);
GetParam(kFreq)->InitFrequency("Frequency", 1000., 20., 20000.);
```

`kNumParams` is passed to the plugin config and determines the fixed parameter count. Parameters are accessed by index throughout the plugin.

## Choosing an Init Method

Use the most specific Init method available -- it sets sensible defaults for range, step, label, unit, and shape:

| Need | Method | Defaults |
|------|--------|----------|
| General continuous value | `InitDouble(name, default, min, max, step, label)` | Linear shape |
| On/off toggle | `InitBool(name, default)` | "off"/"on" display text |
| List of choices | `InitEnum(name, default, {"A", "B", "C"})` | Stepped, auto-indexed |
| Integer range | `InitInt(name, default, min, max)` | Stepped |
| Volume/gain in dB | `InitGain(name, default)` | -70 to 24 dB, 0.5 step |
| Frequency in Hz | `InitFrequency(name, default)` | 0.1 to 10000 Hz, log shape |
| 0-100% | `InitPercentage(name, default)` | 0 to 100 |
| Time in seconds | `InitSeconds(name, default)` | 0 to 10 s |
| Time in ms | `InitMilliseconds(name, default)` | 0 to 100 ms |
| MIDI note number | `InitPitch(name, default)` | 0 to 128 |
| Angle | `InitAngleDegrees(name, default)` | 0 to 360 |
| Clone another param | `Init(otherParam, search, replace, newGroup)` | Copies all settings |

All Init methods accept optional `flags` and `group` arguments. `InitDouble` additionally takes a `shape`, `unit`, and `displayFunc`.

For complete Init method signatures and all IParam methods, consult **`references/iparam-api.md`**.

## Parameter Shapes

Shapes control how normalized (0-1) values map to real values. This affects both UI control feel and automation curves:

| Shape | Use Case | Example |
|-------|----------|---------|
| `ShapeLinear()` | Default, uniform mapping | Gain, percentage |
| `ShapePowCurve(3.)` | Skew toward low end | ADSR times, where fine control at small values matters |
| `ShapePowCurve(0.5)` | Skew toward high end | Rarely used |
| `ShapeExp()` | Exponential (true log) | Frequency sweeps |

```cpp
GetParam(kAttack)->InitDouble("Attack", 10., 1., 1000., 0.1, "ms",
  IParam::kFlagsNone, "ADSR", IParam::ShapePowCurve(3.));
```

## Parameter Groups

Group parameters for organization and batch operations. Pass the group name as the `group` argument to any Init method:

```cpp
GetParam(kAttack)->InitDouble("Attack", 10., 1., 1000., 0.1, "ms", 0, "ADSR", IParam::ShapePowCurve(3.));
GetParam(kDecay)->InitDouble("Decay", 10., 1., 1000., 0.1, "ms", 0, "ADSR", IParam::ShapePowCurve(3.));
GetParam(kSustain)->InitPercentage("Sustain", 50., 0., 100., 0, "ADSR");
GetParam(kRelease)->InitDouble("Release", 10., 2., 1000., 0.1, "ms", 0, "ADSR");
```

Groups enable batch operations:

```cpp
ForParamInGroup("ADSR", [](int idx, IParam& param) { /* ... */ });
RandomiseParamValues("ADSR");
DefaultParamValues("ADSR");
CopyParamValues("ADSR Input", "ADSR Output");
```

Groups are also used by hosts to organize automation lists and by IGraphics for `IVGroupControl` visual grouping.

## Batch Operations and Randomization

Operate on parameters in bulk by range or group:

| Method | Purpose |
|--------|---------|
| `RandomiseParamValues()` | Randomize all parameters |
| `RandomiseParamValues(start, end)` | Randomize a range |
| `RandomiseParamValues("group")` | Randomize a group |
| `DefaultParamValues()` | Reset all to defaults |
| `DefaultParamValues(start, end)` | Reset a range |
| `DefaultParamValues("group")` | Reset a group |
| `CopyParamValues(srcIdx, destIdx, n)` | Copy values between ranges |
| `CopyParamValues("srcGroup", "destGroup")` | Copy values between groups |
| `ForParamInRange(start, end, func)` | Apply lambda to range |
| `ForParamInGroup("group", func)` | Apply lambda to group |
| `InitParamRange(start, end, countStart, fmt, ...)` | Init many params at once |
| `CloneParamRange(srcStart, srcEnd, destStart)` | Clone with name substitution |
| `PrintParamValues()` | Debug-print all values |

```cpp
// Custom randomization with constraints
ForParamInGroup("ADSR", [](int idx, IParam& param) {
  double range = param.GetRange();
  param.Set(param.GetMin() + range * std::rand() / RAND_MAX);
});

// Initialize 16 similar parameters at once
InitParamRange(kStep0, kStep15, 0, "Step %i", 0., 0., 1., 0.01, "", 0, "Sequencer");

// Clone a set of parameters with name substitution
CloneParamRange(kOsc1Freq, kOsc1Detune, kOsc2Freq, "Osc 1", "Osc 2", "Oscillator 2");
```

For the complete batch operations API, consult **`references/batch-operations.md`**.

## Parameter Access in ProcessBlock

Reading parameter values must be realtime-safe. Use `Value()`, `Bool()`, `Int()`, or `DBToAmp()`:

```cpp
void MyPlugin::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->DBToAmp();
  const bool bypass = GetParam(kBypass)->Bool();
  const int mode = GetParam(kMode)->Int();

  for (int s = 0; s < nFrames; s++) {
    outputs[0][s] = bypass ? inputs[0][s] : inputs[0][s] * gain;
  }
}
```

**Warning:** `GetParam(idx)->Value()` returns the raw (non-normalized) value. For gain parameters initialized with `InitGain`, use `DBToAmp()` to convert dB to linear amplitude.

## Parameter Smoothing

Abrupt parameter changes cause clicks. Use `LogParamSmooth` for smooth transitions:

```cpp
// Plugin.h
#include "Smoothers.h"
LogParamSmooth<sample, 1> mGainSmoother;

// Plugin.cpp constructor or OnReset
mGainSmoother.SetSmoothTime(20., GetSampleRate());

// ProcessBlock
for (int s = 0; s < nFrames; s++) {
  const double smoothedGain = mGainSmoother.Process(gain);
  outputs[0][s] = inputs[0][s] * smoothedGain;
}
```

For multi-parameter smoothing (e.g., ADSR envelope), use the block-based API:

```cpp
LogParamSmooth<sample, 4> mParamSmoother;  // 4 smoothed params
sample mParamsToSmooth[4];                  // target values

// In OnParamChange or SetParam:
mParamsToSmooth[kModGain] = GetParam(kGain)->DBToAmp();

// In ProcessBlock:
sample* smoothedOutputs[4];
// ... allocate per-sample buffers
mParamSmoother.ProcessBlock(mParamsToSmooth, smoothedOutputs, nFrames);
```

For a convenience gain smoother, use `SmoothedGain`:

```cpp
SmoothedGain<sample> mGainSmoother;
mGainSmoother.SetSampleRate(GetSampleRate());
mGainSmoother.ProcessBlock(inputs, outputs, nChans, nFrames, gainValue);
```

For smoothing details, consult **`references/smoothing-and-callbacks.md`**.

## OnParamChange and OnParamChangeUI

Two callbacks handle parameter changes:

```cpp
// Called on the audio thread (or main thread depending on API) -- must be realtime-safe
void OnParamChange(int paramIdx) override
{
  switch (paramIdx) {
    case kGain: mDSP.SetGain(GetParam(kGain)->DBToAmp()); break;
    case kFreq: mDSP.SetFreq(GetParam(kFreq)->Value()); break;
  }
}

// Called on the UI thread -- safe to update UI elements
void OnParamChangeUI(int paramIdx, EParamSource source) override
{
  if (paramIdx == kMode) {
    if (auto* pGraphics = GetUI()) {
      bool showAdvanced = GetParam(kMode)->Int() > 0;
      pGraphics->HideControl(kAdvancedParam, !showAdvanced);
    }
  }
}
```

`OnParamChange` has a version with `EParamSource` to distinguish automation, UI, preset recall, etc.

## Presets

Create factory presets in the constructor after parameter initialization:

```cpp
// Positional: values in parameter order
MakePreset("Clean", 0., 1000., 50.);
MakePreset("Drive", -6., 2000., 80.);

// Named: specify only the params you want to set (rest get defaults)
MakePresetFromNamedParams("Bass Boost", 2, kGain, 6., kFreq, 200.);

// Fill remaining slots with defaults
MakeDefaultPreset("Init", 10);
```

For preset debugging, use `DumpMakePresetSrc()` or `DumpMakePresetFromNamedParamsSrc()` to generate preset code from the current state.

For preset and serialization details, consult **`references/presets-and-state.md`**.

## Key Source Files

| Concept | File |
|---------|------|
| IParam class (all Init methods, shapes, types) | `IPlug/IPlugParameter.h` |
| Batch operations, randomization, presets | `IPlug/IPlugPluginBase.h` |
| Parameter change callbacks, delegation | `IPlug/IPlugEditorDelegate.h` |
| Host communication (automation) | `IPlug/IPlugAPIBase.h` |
| Smoothers (LogParamSmooth, SmoothedGain) | `IPlug/Extras/Smoothers.h` |
| Constants (kNoParameter, MAX_PARAM_*) | `IPlug/IPlugConstants.h` |
| Serialization (IByteChunk) | `IPlug/IPlugStructs.h` |
| Minimal example | `Examples/IPlugEffect/` |
| Groups + smoothing example | `Examples/IPlugInstrument/` |
| Comprehensive parameter types | `Examples/IPlugControls/` |
