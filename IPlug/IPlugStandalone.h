#ifndef _IPLUGAPI_
#define _IPLUGAPI_

/**
 * @file
 * @copydoc IPlugStandalone
 */


#include "IPlugOSDetect.h"

//TODO: Shouldn't have to do this here, but couldn't make it work in IPlug_include_in_plug_hdr.h
#ifdef NO_IGRAPHICS
#include "IPlugBase.h"
typedef IPlugBase IPLUG_BASE_CLASS;
#else
#include "IPlugBaseGraphics.h"
typedef IPlugBaseGraphics IPLUG_BASE_CLASS;
#endif


#include "RtMidi.h"

#ifdef OS_WIN
  struct IPlugInstanceInfo
  {
    RtMidiOut* mRTMidiOut;
    uint16_t mMidiOutChan; // 0 = any, 1 = midi chan 1
  };

#elif defined OS_OSX
  struct IPlugInstanceInfo
  {
    WDL_String mOSXBundleID;
    RtMidiOut* mRTMidiOut;
    unsigned short mMidiOutChan; // 0 = any, 1 = midi chan 1
  };
#endif

/**  Standalone application base class for an IPlug plug-in, inherits from IPlugBase or IPlugBaseGraphics
*   @ingroup APIClasses
*/
class IPlugStandalone : public IPLUG_BASE_CLASS
{
public:
  IPlugStandalone(IPlugInstanceInfo instanceInfo, IPlugConfig config);

  // these methods aren't needed in standalones but they are pure virtual in IPlugBase so must have a NO-OP here
  void BeginInformHostOfParamChange(int idx) override {};
  void InformHostOfParamChange(int idx, double normalizedValue) override {};
  void EndInformHostOfParamChange(int idx) override {};
  void InformHostOfProgramChange() override {};

  int GetSamplePos() override { return 0; }
  double GetTempo() override { return DEFAULT_TEMPO; }
  void GetTimeSig(int& numerator, int& denominator) override { return; }
  void GetTime(ITimeInfo& timeInfo) override { return; }

  void ResizeGraphics(int w, int h, double scale) override;

protected:
  bool SendMidiMsg(IMidiMsg& msg) override;
  bool SendSysEx(ISysEx& msg) override;

private:
  RtMidiOut* mMidiOut = nullptr;
  uint16_t mMidiOutChan;
};

IPlugStandalone* MakePlug(void* pMidiOutput, uint16_t& midiOutChannel);

#endif
