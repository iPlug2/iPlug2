#ifndef _IPLUGAPI_
#define _IPLUGAPI_

/**
 * @file
 * @copydoc IPlugAPP
 */


#include "IPlugPlatform.h"
#include "IPlugBase_select.h"

#include "RtMidi.h"

#ifdef OS_WIN
  struct IPlugInstanceInfo
  {
    RtMidiOut* mRTMidiOut;
    uint16_t mMidiOutChan; // 0 = any, 1 = midi chan 1
  };

#elif defined OS_MAC
  struct IPlugInstanceInfo
  {
    WDL_String mBundleID;
    RtMidiOut* mRTMidiOut;
    uint16_t mMidiOutChan; // 0 = any, 1 = midi chan 1
  };
#endif

/**  Standalone application base class for an IPlug plug-in, inherits from IPlugBase or IPlugBaseGraphics
*   @ingroup APIClasses
*/
class IPlugAPP : public IPLUG_BASE_CLASS
               , public IPlugProcessor<PLUG_SAMPLE_DST>
{
public:
  IPlugAPP(IPlugInstanceInfo instanceInfo, IPlugConfig config);

  //IPlugBase
  void BeginInformHostOfParamChange(int idx) override {};
  void InformHostOfParamChange(int idx, double normalizedValue) override {};
  void EndInformHostOfParamChange(int idx) override {};
  void InformHostOfProgramChange() override {};
  void ResizeGraphics(int w, int h, double scale) override;

  //IPlugProcessor
  bool SendMidiMsg(IMidiMsg& msg) override;
  bool SendSysEx(ISysEx& msg) override;

private:
  RtMidiOut* mMidiOut = nullptr;
  uint16_t mMidiOutChan;
};

IPlugAPP* MakePlug(void* pMidiOutput, uint16_t& midiOutChannel);

#endif
