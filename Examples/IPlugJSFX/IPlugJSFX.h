#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IPlugStructs.h"

// Forward declaration - SX_Instance is defined in jsfx/sfxui.h
class SX_Instance;

// Number of JSFX sliders we expose as plugin parameters
const int kNumJSFXParams = 64;
const int kNumPresets = 1;

enum EParams
{
  // First parameter is the JSFX effect path selector (as a placeholder)
  kParamStart = 0,
  kNumParams = kNumJSFXParams
};

using namespace iplug;

class IPlugJSFX final : public Plugin
{
public:
  IPlugJSFX(const InstanceInfo& info);
  ~IPlugJSFX();

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnReset() override;
  void OnActivate(bool active) override;

  bool SerializeState(IByteChunk& chunk) const override;
  int UnserializeState(const IByteChunk& chunk, int startPos) override;
#endif

  // JSFX-specific methods
  bool LoadEffect(const char* effectPath);
  void CloseEffect();
  const char* GetEffectName() const;

  // Get/set JSFX root path (directory containing effects/ and data/ subdirs)
  void SetJSFXRootPath(const char* path);
  const char* GetJSFXRootPath() const { return mJSFXRootPath.Get(); }

private:
  void SyncParametersToJSFX();
  void SyncParametersFromJSFX();

  SX_Instance* mJSFXInstance = nullptr;
  WDL_FastString mJSFXRootPath;
  WDL_FastString mEffectName;
  WDL_TypedBuf<double> mInterleaveBuf;

  double mSampleRate = 44100.0;
  bool mNeedsReset = false;
};
