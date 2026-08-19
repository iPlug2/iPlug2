#pragma once

#include <atomic>

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugARAExample_ARA.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;

class IPlugARAExample final : public Plugin
{
public:
  IPlugARAExample(const InstanceInfo& info);

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

  /** @return the last transport position seen by the audio thread, in seconds */
  double GetPlayheadPos() const { return mPlayheadPos.load(std::memory_order_relaxed); }

  /** @return the last transport running state seen by the audio thread */
  bool GetTransportWasRunning() const { return mTransportRunning.load(std::memory_order_relaxed); }

private:
  std::atomic<double> mPlayheadPos {0.};
  std::atomic<bool> mTransportRunning {false};
};
