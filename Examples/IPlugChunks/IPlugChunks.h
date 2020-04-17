#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"
#include "IPlugChunks_controls.h"

/*
 
 IPlugChunks - an example of storing data in chunks, and custom IControl classes
 
 A step sequenced volume control / trance gate
 
 Using chunks allows you to store arbitary data (e.g. a hidden, non-automatable parameter, a filepath etc) in the plugin's state,
 i.e. when you save a preset to a file or when you save the project in your host
 
 You need to override SerializeState / UnserializeState and set PLUG_DOES_STATE_CHUNKS 1 in config.h
 
 // WARNING - I'm not happy with how the multislider data is shared with the high priority thread
 // need to rethink that
 
 */
#define NUM_SLIDERS 16
#define BEAT_DIV 4 // semiquavers

const int kNumPrograms = 128;

enum EParams
{
  kGain = 0,
  kNumParams
};

// Parameter names for use by preset file menu to dump/ append preset source from named parameters.
const char* pParamNames[] =
{
  "kGain"
};

//SubMenu names for use by PresetSubMenu control. 8 submenus max.
const char* pSubMenuNames[] =
{
  "Factory Presets 1",
  "Factory Presets 2",
  "User Presets 1",
  "User Presets 2",
  "User Presets 3",
  "User Presets 4",
  "User Presets 5",
  "User Presets 6"
};

const int kDummyParamForMultislider = -2;

using namespace iplug;
using namespace igraphics;

class IPlugChunks final : public Plugin
{
public:
  IPlugChunks(const InstanceInfo& info);
  ~IPlugChunks();

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void Reset();
  void OnParamChange(int paramIdx) override;
  void OnIdle() override;
  void AddDumpedPresets(int* n) override;
  bool SerializeState(IByteChunk &chunk) const override;
  int UnserializeState(const IByteChunk &chunk, int startPos) override;
  bool CompareState(const uint8_t* pIncomingState, int startPos) const override;
  void PresetsChangedByHost();
private:
  double mSteps[NUM_SLIDERS];
  double mGain;
  unsigned long mCount, mPrevCount;
  bool mUIJustOpened;
#endif
#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  MultiSliderControlV *mMSlider;
  IVButtonControl* mIncButton;
  IVButtonControl* mDecButton;
  IAboutControl* mAboutControl;
#endif
};
