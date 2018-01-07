#ifndef _IPLUGAPI_
#define _IPLUGAPI_

#include "IPlugOSDetect.h"

//TODO: Shouldn't have to do this here, but couldn't make it work in IPlug_include_in_plug_hdr.h
#ifdef NO_IGRAPHICS
#include "IPlugBase.h"
typedef IPlugBase IPLUG_BASE_CLASS;
#else
#include "IPlugBaseGraphics.h"
typedef IPlugBaseGraphics IPLUG_BASE_CLASS;
#endif

#ifdef OS_WIN
  #include "RtMidi.h"
  
  struct IPlugInstanceInfo
  {
    RtMidiOut* mRTMidiOut;
    unsigned short* mMidiOutChan; // 0 = any, 1 = midi chan 1
  };

#elif defined OS_OSX
  #include "RtMidi.h"
  
  struct IPlugInstanceInfo
  {
    WDL_String mOSXBundleID;
    RtMidiOut* mRTMidiOut;
    unsigned short* mMidiOutChan; // 0 = any, 1 = midi chan 1
  };

#endif

class IPlugStandalone : public IPLUG_BASE_CLASS
{
public:
  IPlugStandalone(IPlugInstanceInfo instanceInfo,
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

  // these methods aren't needed in standalones but they are pure virtual in IPlugBase so must have a NO-OP here
  void BeginInformHostOfParamChange(int idx) override {};
  void InformHostOfParamChange(int idx, double normalizedValue) override {};
  void EndInformHostOfParamChange(int idx) override {};
  void InformHostOfProgramChange() override {};

  int GetSamplePos() override { return 0; }
  double GetTempo() override { return DEFAULT_TEMPO; }
  void GetTimeSig(int& numerator, int& denominator) override { return; }
  void GetTime(ITimeInfo& timeInfo) override { return; }

  void ResizeGraphics(int w, int h) override;

protected:
  bool SendMidiMsg(IMidiMsg& msg) override;
  bool SendSysEx(ISysEx& msg) override;

private:
  RtMidiOut* mMidiOut = nullptr;
  unsigned short* mMidiOutChan = nullptr; //TODO: sort this out
};

IPlugStandalone* MakePlug(void* pMidiOutput, unsigned short* pMidiOutChan);

#endif
