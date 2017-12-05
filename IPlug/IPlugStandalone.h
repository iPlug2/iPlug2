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
  void BeginInformHostOfParamChange(int idx) {};
  void InformHostOfParamChange(int idx, double normalizedValue) {};
  void EndInformHostOfParamChange(int idx) {};
  void InformHostOfProgramChange() {};

  int GetSamplePos() { return 0; }   // Samples since start of project.
  double GetTempo() { return DEFAULT_TEMPO; }
  void GetTimeSig(int& numerator, int& denominator) { return; }
  void GetTime(ITimeInfo& timeInfo) { return; }

  void ResizeGraphics(int w, int h);

protected:
  bool SendMidiMsg(IMidiMsg& msg);
  bool SendSysEx(ISysEx& msg);

private:
  RtMidiOut* mMidiOut;
  unsigned short* mMidiOutChan;
};

IPlugStandalone* MakePlug(void* pMidiOutput, unsigned short* pMidiOutChan);

#endif
