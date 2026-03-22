# Smoothing and Callbacks

Parameter smoothing utilities and change notification callbacks.

## LogParamSmooth

Logarithmic one-pole smoothing filter for parameter changes. Prevents clicks/zipper noise from abrupt value changes.

Located in `IPlug/Extras/Smoothers.h`.

### Single-Channel Smoothing

```cpp
#include "Smoothers.h"

// Declare in plugin header
LogParamSmooth<sample, 1> mGainSmoother;

// Configure in OnReset (called when sample rate changes)
void OnReset() override {
  mGainSmoother.SetSmoothTime(20., GetSampleRate());  // 20ms smoothing
}

// Use in ProcessBlock
void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override {
  const double targetGain = GetParam(kGain)->DBToAmp();

  for (int s = 0; s < nFrames; s++) {
    const double smoothedGain = mGainSmoother.Process(targetGain);
    outputs[0][s] = inputs[0][s] * smoothedGain;
    outputs[1][s] = inputs[1][s] * smoothedGain;
  }
}
```

### Multi-Channel Block Smoothing

For smoothing multiple parameters simultaneously with per-sample output:

```cpp
// Template: LogParamSmooth<SampleType, NumChannels>
LogParamSmooth<sample, 4> mParamSmoother;  // 4 smoothed parameters
sample mParamsToSmooth[4];                  // target values array

// Set target values (from OnParamChange or ProcessBlock)
mParamsToSmooth[0] = GetParam(kGain)->DBToAmp();
mParamsToSmooth[1] = GetParam(kAttack)->Value() / 1000.;
mParamsToSmooth[2] = GetParam(kDecay)->Value() / 1000.;
mParamsToSmooth[3] = GetParam(kSustain)->Value() / 100.;

// ProcessBlock: produce per-sample smoothed output
sample* smoothedBuffers[4];  // allocate 4 x nFrames buffers
mParamSmoother.ProcessBlock(mParamsToSmooth, smoothedBuffers, nFrames);

// Now smoothedBuffers[0][s] has the smoothed gain at each sample
```

### API Reference

```cpp
template<typename T, int NC = 1>
class LogParamSmooth {
  LogParamSmooth(double timeMs = 5., T initialValue = 0.);
  T Process(T input);                                          // Single sample, NC=1 only
  void SetValue(T value);                                      // Set all channels to value
  void SetValues(T values[NC]);                                // Set per-channel values
  void SetSmoothTime(double timeMs, double sampleRate);        // Configure time constant
  void ProcessBlock(T inputs[NC], T** outputs, int nFrames, int channelOffset = 0);
};
```

## SmoothedGain

Convenience wrapper for the common case of applying smoothed gain:

```cpp
#include "Smoothers.h"

SmoothedGain<sample> mGainSmoother;

void OnReset() override {
  mGainSmoother.SetSampleRate(GetSampleRate());
}

void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override {
  const double gain = GetParam(kGain)->DBToAmp();
  mGainSmoother.ProcessBlock(inputs, outputs, NOutChansConnected(), nFrames, gain);
}
```

### API Reference

```cpp
template<typename T>
class SmoothedGain {
  SmoothedGain(double smoothingTime = 5.0);
  void ProcessBlock(T** inputs, T** outputs, int nChans, int nFrames, double gainValue);
  void SetSampleRate(double sampleRate);
};
```

## Parameter Change Callbacks

### OnParamChange (Audio Thread)

Called when a parameter value changes. May be called from the audio thread, so must be realtime-safe (no allocations, locks, or file I/O):

```cpp
// Basic version
void OnParamChange(int paramIdx) override {
  switch (paramIdx) {
    case kGain:
      mGain = GetParam(kGain)->DBToAmp();
      break;
    case kFreq:
      mOscillator.SetFreqCPS(GetParam(kFreq)->Value());
      break;
  }
}
```

### OnParamChange (with source)

Version that includes the source of the parameter change:

```cpp
void OnParamChange(int paramIdx, EParamSource source, int sampleOffset = -1) override {
  // source can be: kReset, kHost, kPresetRecall, kUI, kDelegate, kRecompile, kUnknown
  if (source == kPresetRecall) {
    // Preset was loaded -- may want to rebuild state
  }
  OnParamChange(paramIdx);  // call simpler version
}
```

### OnParamChangeUI (UI Thread)

Called on the main/UI thread. Safe to access UI elements:

```cpp
void OnParamChangeUI(int paramIdx, EParamSource source = kUnknown) override {
  if (paramIdx == kFilterMode) {
    if (auto* pGraphics = GetUI()) {
      bool isLowpass = GetParam(kFilterMode)->Int() == 0;
      pGraphics->HideControl(kResonance, !isLowpass);
    }
  }
}
```

Common uses:
- Show/hide controls based on mode parameters
- Update non-parameter UI elements (labels, displays)
- Send control messages to visualization controls

### OnParamReset

Called when multiple parameters change at once (e.g., preset load):

```cpp
void OnParamReset(EParamSource source) override {
  // Default implementation calls OnParamChange + OnParamChangeUI for each param
  // Override for custom bulk-update logic
}
```

## EParamSource

```cpp
enum EParamSource {
  kReset,         // Programmatic reset
  kHost,          // Host automation
  kPresetRecall,  // Preset loaded
  kUI,            // User interaction
  kDelegate,      // Editor delegate
  kRecompile,     // JIT recompile (Faust)
  kUnknown        // Unknown source
};
```

## Thread Safety Notes

- `OnParamChange` may be called on the audio thread. Only set member variables, don't allocate or lock.
- `OnParamChangeUI` is always on the UI thread. Safe to call IGraphics methods.
- `GetParam(idx)->Value()` uses `std::atomic<double>`, so reading is always safe from any thread.
- For complex parameter-dependent state changes, consider updating DSP state in `ProcessBlock` by reading parameter values there, rather than in `OnParamChange`.
