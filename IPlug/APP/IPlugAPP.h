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

struct IPlugInstanceInfo
{
  void* pAppHost;
};

class IPlugAPPHost;

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
  void ResizeGraphics(int viewWidth, int viewHeight, float scale) override;

  //IEditorDelegate
  // SendSysexMsgFromUI overridden here, to avoid unnecessarily queueing sysex data up
  void SendSysexMsgFromUI(const ISysEx& msg) override;
  
  //IPlugProcessor
  bool SendMidiMsg(const IMidiMsg& msg) override;
  bool SendSysEx(ISysEx& msg) override;
  
  //IPlugAPP
  void AppProcess(double** inputs, double** outputs, int nFrames);

private:
  IPlugAPPHost* mAppHost = nullptr;
  IPlugQueue<IMidiMsg> mMidiMsgsFromCallback {MIDI_TRANSFER_SIZE};
  
  friend class IPlugAPPHost;
};

IPlugAPP* MakePlug(void* pAPPHost);

#endif
