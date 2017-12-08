#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

//TODO: Shouldn't have to do this here, but couldn't make it work in IPlug_include_in_plug_hdr.h
#ifdef NO_IGRAPHICS
#include "IPlugBase.h"
typedef IPlugBase IPLUG_BASE_CLASS;
#else
#include "IPlugBaseGraphics.h"
typedef IPlugBaseGraphics IPLUG_BASE_CLASS;
#endif

#include "aeffectx.h"

struct IPlugInstanceInfo
{
  audioMasterCallback mVSTHostCallback;
};

class IPlugVST : public IPLUG_BASE_CLASS
{
public:
  IPlugVST(IPlugInstanceInfo instanceInfo,
           int nParams,
           const char* channelIOStr,
           int nPresets,
           const char* effectName,
           const char* productName,
           const char* mfrName,
           int vendorVersion,
           int uniqueID,
           int mfrID,
           int latency = 0,
           bool plugDoesMidi = false,
           bool plugDoesChunks = false,
           bool plugIsInst = false,
           int plugScChans = 0);

  // ----------------------------------------
  // See IPlugBase for the full list of methods that your plugin class can implement.

  void BeginInformHostOfParamChange(int idx) override;
  void InformHostOfParamChange(int idx, double normalizedValue) override;
  void EndInformHostOfParamChange(int idx) override;

  void InformHostOfProgramChange() override;

  int GetSamplePos() override;   // Samples since start of project.
  double GetTempo() override;
  void GetTimeSig(int& numerator, int& denominator) override;
  void GetTime(ITimeInfo& timeInfo) override;
  EHost GetHost() override;

  void ResizeGraphics(int w, int h) override;
  bool IsRenderingOffline() override;

  virtual void OnGUICreated() override;

  audioMasterCallback mHostCallback;

protected:
  void HostSpecificInit() override;
  void SetLatency(int samples) override;
  bool SendMidiMsg(IMidiMsg& msg) override;
  bool SendSysEx(ISysEx& msg) override;
  audioMasterCallback GetHostCallback();

private:
  template <class SAMPLETYPE>
  void VSTPrepProcess(SAMPLETYPE** inputs, SAMPLETYPE** outputs, VstInt32 nFrames);

  ERect mEditRect;

  bool SendVSTEvent(VstEvent& event);
  bool SendVSTEvents(WDL_TypedBuf<VstEvent>* pEvents);

  VstSpeakerArrangement mInputSpkrArr, mOutputSpkrArr;

  bool mHostSpecificInitDone;

  enum { VSTEXT_NONE=0, VSTEXT_COCKOS, VSTEXT_COCOA }; // list of VST extensions supported by host
  int mHasVSTExtensions;

  ByteChunk mState;     // Persistent storage if the host asks for plugin state.
  ByteChunk mBankState; // Persistent storage if the host asks for bank state.

public:
  static VstIntPtr VSTCALLBACK VSTDispatcher(AEffect *pEffect, VstInt32 opCode, VstInt32 idx, VstIntPtr value, void *ptr, float opt);
  static void VSTCALLBACK VSTProcess(AEffect *pEffect, float **inputs, float **outputs, VstInt32 nFrames);  // Deprecated.
  static void VSTCALLBACK VSTProcessReplacing(AEffect *pEffect, float **inputs, float **outputs, VstInt32 nFrames);
  static void VSTCALLBACK VSTProcessDoubleReplacing(AEffect *pEffect, double **inputs, double **outputs, VstInt32 nFrames);
  static float VSTCALLBACK VSTGetParameter(AEffect *pEffect, VstInt32 idx);
  static void VSTCALLBACK VSTSetParameter(AEffect *pEffect, VstInt32 idx, float value);
  AEffect mAEffect;
};

IPlugVST* MakePlug();

#endif
