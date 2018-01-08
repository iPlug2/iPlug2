#ifndef _IPLUGAPI_
#define _IPLUGAPI_

#include "IPlugOSDetect.h"
#include "IPlugBase.h"

// #include "Processor.h"

/*! Used to pass various instance info to the API class */
struct IPlugInstanceInfo {};

/*! Web Audio Module API base class for an IPlug plug-in */
class IPlugWAM : public IPLUG_BASE_CLASS
{
public:
  IPlugWAM(IPlugInstanceInfo instanceInfo,
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

  void BeginInformHostOfParamChange(int idx) override;
  void InformHostOfParamChange(int idx, double normalizedValue) override;
  void EndInformHostOfParamChange(int idx) override;
  void InformHostOfProgramChange() override;
  int GetSamplePos() override;
  double GetTempo() override;
  void GetTimeSig(int& numerator, int& denominator) override;
  void GetTime(ITimeInfo& timeInfo) override;
  EHost GetHost() override;
  void ResizeGraphics(int w, int h) override;
  bool IsRenderingOffline() override;
  
protected:
  void HostSpecificInit() override;
  void SetLatency(int samples) override;
  bool SendMidiMsg(IMidiMsg& msg) override;
  bool SendSysEx(ISysEx& msg) override;
};

IPlugWAM* MakePlug();

#endif
