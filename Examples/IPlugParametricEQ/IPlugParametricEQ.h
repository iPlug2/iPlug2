#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "ISender.h"
#include "Extras/SVF.h"

const int kNumPresets = 1;

// Parameter indices for each band: Freq, Gain, Q, Type, Enable
enum EParams
{
  // Band 0 (Low - default LowShelf at 80Hz)
  kBand0Freq = 0,
  kBand0Gain,
  kBand0Q,
  kBand0Type,
  kBand0Enable,

  // Band 1 (Low-Mid - default Bell at 250Hz)
  kBand1Freq,
  kBand1Gain,
  kBand1Q,
  kBand1Type,
  kBand1Enable,

  // Band 2 (Mid - default Bell at 1kHz)
  kBand2Freq,
  kBand2Gain,
  kBand2Q,
  kBand2Type,
  kBand2Enable,

  // Band 3 (High-Mid - default Bell at 4kHz)
  kBand3Freq,
  kBand3Gain,
  kBand3Q,
  kBand3Type,
  kBand3Enable,

  // Band 4 (High - default HighShelf at 12kHz)
  kBand4Freq,
  kBand4Gain,
  kBand4Q,
  kBand4Type,
  kBand4Enable,

  kNumParams
};

// Control tags
enum ECtrlTags
{
  kCtrlTagEQAnalyzer = 0
};

// Messages from control to DSP
enum EMsgTags
{
  kMsgTagFFTSize = 100,
  kMsgTagOverlap,
  kMsgTagWindowType,
  kMsgTagSampleRate
};

// Helper to get parameter index for a band
inline int GetBandParamIdx(int band, int paramOffset)
{
  return band * 5 + paramOffset;
}

// Offsets within each band's parameter group
enum EBandParamOffsets
{
  kBandFreq = 0,
  kBandGain,
  kBandQ,
  kBandType,
  kBandEnable
};

using namespace iplug;
using namespace igraphics;

class IPlugParametricEQ final : public Plugin
{
public:
  IPlugParametricEQ(const InstanceInfo& info);

#if IPLUG_DSP
  void OnReset() override;
  void OnIdle() override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnParamChange(int paramIdx) override;
  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

  ISpectrumSender<2> mSender;

private:
  void UpdateFilter(int band);

  // 5 SVF filters for stereo (2 channels)
  SVF<double, 2> mFilters[NUM_EQ_BANDS];

  // Cached parameter values for quick access
  double mFreq[NUM_EQ_BANDS];
  double mGain[NUM_EQ_BANDS];
  double mQ[NUM_EQ_BANDS];
  int mType[NUM_EQ_BANDS];
  bool mEnable[NUM_EQ_BANDS];
#endif

#if IPLUG_EDITOR
  void OnParamChangeUI(int paramIdx, EParamSource source) override;
  bool SerializeState(IByteChunk& chunk) const override;
  int UnserializeState(const IByteChunk& chunk, int startPos) override;
  void OnUIOpen() override;

private:
  void SyncUIControl();
#endif
};
