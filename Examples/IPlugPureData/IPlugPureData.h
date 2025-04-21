#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "PdWrapper.h"
#include "wdlstring.h"
#include <memory>

// Define MAXPDSTRING if not already defined by libpd
#ifndef MAXPDSTRING
#define MAXPDSTRING 4096
#endif

const int kNumPresets = 1;
const int kMaxParams = 100; // Maximum number of parameters supported

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class IPlugPureData final : public Plugin
{
public:
  IPlugPureData(const InstanceInfo& info);
  ~IPlugPureData();

#if IPLUG_DSP // http://bit.ly/2S64BDd
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif

  // libpd callbacks
  void OnParamChange(int paramIdx, double value);
  void OnMidiMsg(const IMidiMsg& msg);
  
  // libpd specific methods
  void OpenPatch(const char* filename, const char* dir);
  void SetParamValue(int paramIndex, float value);
  void DefineParam(int argc, t_atom* argv);
  void ReadUiDesc(const char* filename);
  void Quit();

  // Getter for PdWrapper
  PdWrapper* GetPdWrapper() { return mPdWrapper.get(); }

  // Resource location
  bool LocatePdPatch(const char* patchName, WDL_String& path);
  
  // File browser for selecting Pd patches
  bool OpenFileBrowser(WDL_String& selectedPath);

private:
  std::unique_ptr<PdWrapper> mPdWrapper;
  
  // Parameter storage
  PdParam mParams[kMaxParams];
  
  // Patch information
  bool mPatchOpen;
  char mPatchFilename[MAXPDSTRING];
  char mPatchDir[MAXPDSTRING];
  
  // libpd hooks
  void* mFromGuiHook;
  void* mFromPdHook;
  
  // Tempo tracking
  double mLastTempo;
  int mTicksSinceTempo;
  int pd_vis = 0;

  // Helper methods
  void InitLibPd();
  void CleanupLibPd();
  void ProcessMidiNote(int channel, int pitch, int velocity);
  void ProcessPdMessage(const char* recv, const char* msg, int argc, t_atom* argv);
};
