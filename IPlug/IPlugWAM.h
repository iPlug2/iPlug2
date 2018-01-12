#ifndef _IPLUGAPI_
#define _IPLUGAPI_

#include "IPlugOSDetect.h"
#include "IPlugBase.h"

// #include "Processor.h"

/** Used to pass various instance info to the API class */
struct IPlugInstanceInfo {};

/**  Web Audio Module (WAM) API base class for an IPlug plug-in, inherits from IPlugBase or IPlugBaseGraphics
*   @ingroup APIClasses
*/
class IPlugWAM : public IPLUG_BASE_CLASS
{
public:
  IPlugWAM(IPlugInstanceInfo instanceInfo, IPlugConfig config);

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
