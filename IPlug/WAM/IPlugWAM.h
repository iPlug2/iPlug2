#ifndef _IPLUGAPI_
#define _IPLUGAPI_

#include "IPlugBase_select.h"

#include "processor.h"

using namespace WAM;

/** Used to pass various instance info to the API class */
struct IPlugInstanceInfo {};

/** WebAudioModule (WAM) API base class for an IPlug plug-in, inherits from IPlugBase or IPlugBaseGraphics
 * @ingroup APIClasses */
class IPlugWAM : public IPLUG_BASE_CLASS
               , public IPlugProcessor<PLUG_SAMPLE_DST>
               , public Processor
{
public:
  IPlugWAM(IPlugInstanceInfo instanceInfo, IPlugConfig config);

  //WAM
  virtual const char* init(uint32_t bufsize, uint32_t sr, void* pDesc) override;
//  virtual void terminate() override {}
//  virtual void resize(uint32_t bufsize) override {}
  
  virtual void onProcess(WAM::AudioBus* pAudio, void* pData) override;
//  virtual void onMidi(byte status, byte data1, byte data2) override {}
//  virtual void onSysex(byte* msg, uint32_t size) override {}
//  virtual void onMessage(char* verb, char* res, double data) override {}
//  virtual void onMessage(char* verb, char* res, char* data) override {}
//  virtual void onMessage(char* verb, char* res, void* data, uint32_t size) override {}
//  virtual void onParam(uint32_t idparam, double value) override {}  // todo: other datatypes

  //IPlugBase
  void BeginInformHostOfParamChange(int idx) override {};
  void InformHostOfParamChange(int idx, double normalizedValue) override {};
  void EndInformHostOfParamChange(int idx) override {};
  void InformHostOfProgramChange() override {};
  EHost GetHost() override {};
  void ResizeGraphics() override {};
  void HostSpecificInit() override {};
  
  //IPlugProcessor
  void SetLatency(int samples) override {};
  bool SendMidiMsg(const IMidiMsg& msg) override { return false; }
  bool SendSysEx(ISysEx& msg) override { return false; }
};

IPlugWAM* MakePlug();

#endif
