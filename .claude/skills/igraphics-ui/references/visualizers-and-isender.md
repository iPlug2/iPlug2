# Visualizers and ISender

## Architecture

The ISender pattern safely transfers audio-thread data to the UI thread via a lock-free queue:

```
Audio Thread                    Main Thread                    UI
─────────────                   ───────────                    ──
ProcessBlock()                  OnIdle()
  mSender.ProcessBlock() ──→ queue ──→ mSender.TransmitData()
                                         ──→ SendControlMsgFromDelegate()
                                                ──→ Control.OnMsgFromDelegate()
```

All sender types live in `IPlug/ISender.h`.

## Setup Checklist

### 1. Declare sender in plugin `.h`

```cpp
IPeakSender<2> mMeterSender;           // 2-channel peak meter
IBufferSender<1> mScopeSender;          // 1-channel scope
ISpectrumSender<2> mSpectrumSender;     // 2-channel FFT
ISender<1> mGenericSender;              // 1-channel raw data
```

### 2. Define control tags

```cpp
enum ECtrlTags { kCtrlTagMeter = 0, kCtrlTagScope, kCtrlTagSpectrum };
```

### 3. Attach control with tag in `mLayoutFunc`

```cpp
pGraphics->AttachControl(new IVMeterControl<2>(bounds, "Meter", style), kCtrlTagMeter);
pGraphics->AttachControl(new IVScopeControl<1>(bounds, "Scope", style), kCtrlTagScope);
pGraphics->AttachControl(new IVSpectrumAnalyzerControl<2>(bounds, "Spectrum", style), kCtrlTagSpectrum);
```

### 4. Feed data in `ProcessBlock`

```cpp
void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override {
  // ... DSP processing ...
  mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
  mScopeSender.ProcessBlock(outputs, nFrames, kCtrlTagScope, 1 /* nChans */);
  mSpectrumSender.ProcessBlock(inputs, nFrames, kCtrlTagSpectrum, NInChansConnected());
}
```

### 5. Transmit in `OnIdle`

```cpp
void OnIdle() override {
  mMeterSender.TransmitData(*this);
  mScopeSender.TransmitData(*this);
  mSpectrumSender.TransmitData(*this);
}
```

### 6. Reset in `OnReset` (for senders that need it)

```cpp
void OnReset() override {
  mMeterSender.Reset(GetSampleRate());
}
```

## Sender-to-Control Matching

| Sender | Template Args | Control | Use Case |
|--------|--------------|---------|----------|
| `IPeakSender<N>` | N=channels | `IVMeterControl<N>` | Simple peak meters |
| `IPeakAvgSender<N>` | N=channels | `IVLEDMeterControl<N>` or `IVPeakAvgMeterControl<N>` | LED meters with peak hold + RMS |
| `IBufferSender<N>` | N=channels | `IVScopeControl<N>` | Oscilloscope waveform |
| `ISpectrumSender<N>` | N=channels | `IVSpectrumAnalyzerControl<N>` | FFT spectrum display |
| `ISender<N,Q,T>` | N=channels, Q=queue, T=type | `IVDisplayControl` or custom | Generic value display |

## Single-Value Sender

For sending individual values (e.g., LFO output for visualization):

```cpp
// In plugin .h:
ISender<1> mLFOVisSender;

// In ProcessBlock (or wherever the value is computed):
mLFOVisSender.PushData({kCtrlTagLFOVis, {float(lfoOutput)}});

// In OnIdle:
mLFOVisSender.TransmitData(*this);
```

Pair with `IVDisplayControl` for a rolling graph:
```cpp
pGraphics->AttachControl(new IVDisplayControl(bounds, "", style,
  EDirection::Horizontal, 0.f, 1.f, 0.f, 1024), kCtrlTagLFOVis);
```

## Spectrum Analyzer Bidirectional Messaging

`IVSpectrumAnalyzerControl` communicates bidirectionally:
- **Control -> Plugin**: The control sends FFT config changes via `SendArbitraryMsgFromUI`, arriving at `OnMessage()`.
- **Plugin -> Control**: The plugin syncs state via `SendControlMsgFromDelegate()` (e.g., in `OnUIOpen`).

Handle control-to-plugin messages in `OnMessage`:

```cpp
bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override {
  if (msgTag == IVSpectrumAnalyzerControl<>::kMsgTagFFTSize) {
    mSpectrumSender.SetFFTSize(*reinterpret_cast<const int*>(pData));
    return true;
  }
  else if (msgTag == IVSpectrumAnalyzerControl<>::kMsgTagOverlap) {
    int overlap = *reinterpret_cast<const int*>(pData);
    mSpectrumSender.SetFFTSizeAndOverlap(mSpectrumSender.GetFFTSize(), overlap);
    return true;
  }
  else if (msgTag == IVSpectrumAnalyzerControl<>::kMsgTagWindowType) {
    int idx = *reinterpret_cast<const int*>(pData);
    mSpectrumSender.SetWindowType(static_cast<ISpectrumSender<2>::EWindowType>(idx));
    return true;
  }
  return false;
}
```

Sync UI on open with `SendControlMsgFromDelegate`:
```cpp
void OnUIOpen() override {
  auto sr = GetSampleRate();
  SendControlMsgFromDelegate(kCtrlTagSpectrum,
    IVSpectrumAnalyzerControl<>::kMsgTagSampleRate, sizeof(double), &sr);
}
```

## TransmitDataToControlsWithTags

To send the same data to multiple controls:
```cpp
mSender.TransmitDataToControlsWithTags(*this, {kCtrlTag1, kCtrlTag2});
```

## Custom Receiver Control

To receive ISender data in a custom control, override `OnMsgFromDelegate`:

```cpp
void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override {
  if (!IsDisabled() && msgTag == ISender<>::kMsgTagData) {
    auto d = reinterpret_cast<const ISenderData<2, float>*>(pData);
    // d->vals[0], d->vals[1] contain channel data
    // d->nChans is the number of active channels
    SetDirty(false);
  }
}
```
