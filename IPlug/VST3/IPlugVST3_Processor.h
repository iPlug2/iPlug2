#ifndef _IPLUGAPI_
#define _IPLUGAPI_

#undef stricmp
#undef strnicmp
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#include "IPlugBase.h"
#include "IPlugProcessor.h"

/** Used to pass various instance info to the API class, where needed */
struct IPlugInstanceInfo {};

using namespace Steinberg;
using namespace Vst;

class IPlugVST3Processor : public AudioEffect
                         , public IPlugBase
                         , public IPlugProcessor<PLUG_SAMPLE_DST>
{
public:
  IPlugVST3Processor(IPlugInstanceInfo instanceInfo, IPlugConfig c);
  virtual ~IPlugVST3Processor();

  // AudioEffect overrides:
  tresult PLUGIN_API initialize(FUnknown* context) override;
//  tresult PLUGIN_API terminate() override;
  tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts) override;
  tresult PLUGIN_API setActive(TBool state) override;
  tresult PLUGIN_API setupProcessing(ProcessSetup& newSetup) override;
  tresult PLUGIN_API process(ProcessData& data) override;
  tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;
  uint32 PLUGIN_API getLatencySamples() override {}; //TODO:
  tresult PLUGIN_API setState(IBStream* state) override;
  tresult PLUGIN_API getState(IBStream* state) override;
  
  //IPlugProcessor
  bool SendMidiMsg(const IMidiMsg& msg) override { return false; } //TODO: SendMidiMsg
  
private:
  bool mSidechainActive = false;
  //IConnectionPoint
  tresult PLUGIN_API notify(IMessage* message) override {}; //TODO:

  /** Called prior to rendering a block of audio in order to update processing context data such as transport info */
  void PrepareProcessContext();
  
  ProcessContext mProcessContext;
  ParameterChanges mOutputParamChanges;
//   spsc_queue<IMidiMsg, capacity<32> > mMidiMsgsFromController; // a queue of midi messages received from the controller, by clicking keyboard UI etc
//   spsc_queue<IMidiMsg, capacity<32> > mMidiMsgsFromProcessor; // a queue of midi messages that might be triggered by IPlug's SendMidiMsg during process to go out the midi output at high priority
//   spsc_queue<IMidiMsg, capacity<1024> > mMidiMsgsFromProcessorToController; // a queue of midi messages to send to the controller
//   spsc_queue<SysExChunk, capacity<2> > mSysExMsgsFromController; // // a queue of SYSEX messages recieved from the controller
//   spsc_queue<SysExChunk, capacity<2> > mSysExMsgsFromProcessorToController; // a queue of SYSEX messages to send to the controller
};

IPlugVST3Processor* MakePlug();

#endif //_IPLUGAPI_
