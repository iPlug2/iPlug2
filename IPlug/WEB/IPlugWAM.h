/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifndef _IPLUGAPI_
#define _IPLUGAPI_

#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"
#include "processor.h"

using namespace WAM;

BEGIN_IPLUG_NAMESPACE

/** Used to pass various instance info to the API class */
struct InstanceInfo
{};

/** WebAudioModule (WAM) API base class. This is used for the DSP processor side of a WAM, which is sandboxed and lives in the AudioWorkletGlobalScope
 * @ingroup APIClasses */
class IPlugWAM : public IPlugAPIBase
               , public IPlugProcessor
               , public Processor
{
public:
  IPlugWAM(const InstanceInfo& info, const Config& config);

  //WAM
  const char* init(uint32_t bufsize, uint32_t sr, void* pDesc) override;
  void terminate() override { DBGMSG("terminate"); }
  void resize(uint32_t bufsize) override { DBGMSG("resize"); }

  void onProcess(WAM::AudioBus* pAudio, void* pData) override;
  void onMidi(byte status, byte data1, byte data2) override;
  void onSysex(byte* pData, uint32_t size) override;
  void onMessage(char* verb, char* res, double data) override;
  void onMessage(char* verb, char* res, char* data) override;
  void onMessage(char* verb, char* res, void* data, uint32_t size) override;
  void onParam(uint32_t idparam, double value) override;

  //IPlugProcessor
  void SetLatency(int samples) override {};
  bool SendMidiMsg(const IMidiMsg& msg) override { return false; }
  bool SendSysEx(const ISysEx& msg) override { return false; }
  
  //IEditorDelegate - these are overwritten because we need to use WAM messaging system
  void SendControlValueFromDelegate(int ctrlTag, double normalizedValue) override;
  void SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData) override;
  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
  void SendArbitraryMsgFromDelegate(int msgTag, int dataSize = 0, const void* pData = nullptr) override;
  
private:
  /** Called repeatedly to emulate IPlugAPIBase::OnTimer() */
  void OnEditorIdleTick();
};

IPlugWAM* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif
