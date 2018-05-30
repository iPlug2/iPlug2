/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

#ifndef _IPLUGAPI_
#define _IPLUGAPI_

/**
 * @file
 * @copydoc IPlugAPP
 */


#include "IPlugPlatform.h"
#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

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

/**  Standalone application base class for an IPlug plug-in, inherits from IPlugAPIBase
*   @ingroup APIClasses
*/
class IPlugAPP : public IPlugAPIBase
               , public IPlugProcessor<PLUG_SAMPLE_DST>
{
public:
  IPlugAPP(IPlugInstanceInfo instanceInfo, IPlugConfig config);

  //IPlugAPIBase
  void BeginInformHostOfParamChange(int idx) override {};
  void InformHostOfParamChange(int idx, double normalizedValue) override {};
  void EndInformHostOfParamChange(int idx) override {};
  void InformHostOfProgramChange() override {};
  void ResizeGraphics() override;

  //IPlugProcessor
  bool SendMidiMsg(const IMidiMsg& msg) override;
  bool SendSysEx(ISysEx& msg) override;

private:
  RtMidiOut* mMidiOut = nullptr;
  uint16_t mMidiOutChan;
};

IPlugAPP* MakePlug(void* pMidiOutput, uint16_t& midiOutChannel);

#endif
