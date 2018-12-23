/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifndef _IPLUGAPI_
#define _IPLUGAPI_

#undef stricmp
#undef strnicmp
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"

#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

/**
 * @file
 * @copydoc IPlugVST3Processor
 */

using namespace Steinberg;
using namespace Vst;

/**  VST3 Processor base class for a distributed IPlug VST3 plug-in
 *   @ingroup APIClasses */
class IPlugVST3Processor : public AudioEffect
                         , public IPlugAPIBase
                         , public IPlugProcessor<PLUG_SAMPLE_DST>
{
public:
  struct IPlugInstanceInfo
  {
    Steinberg::FUID mOtherGUID;
  };
  
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
  uint32 PLUGIN_API getLatencySamples() override { return 0; } //TODO:
  tresult PLUGIN_API setState(IBStream* state) override;
  tresult PLUGIN_API getState(IBStream* state) override;
  
  //IEditorDelegate - these methods are overridden because we need to hook into VST3 messaging system
  void SendControlValueFromDelegate(int controlTag, double normalizedValue) override;
  void SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, const void* pData) override;
  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override {} // NOOP in VST3 processor -> param change gets there via IPlugVST3Controller::setParamNormalized
  void SendArbitraryMsgFromDelegate(int messageTag, int dataSize = 0, const void* pData = nullptr) override;
  
  //IPlugProcessor
  bool SendMidiMsg(const IMidiMsg& msg) override;
  
private:
  void _TransmitMidiMsgFromProcessor(const IMidiMsg& msg) override;
  void _TransmitSysExDataFromProcessor(const SysExData& data) override;

  bool mSidechainActive = false;

  //IConnectionPoint
  tresult PLUGIN_API notify(IMessage* message) override;

  /** Called prior to rendering a block of audio in order to update processing context data such as transport info */
  void PrepareProcessContext();
  
  ProcessContext mProcessContext;
  ParameterChanges mOutputParamChanges;
  IMidiQueue mMidiOutputQueue;
};

IPlugVST3Processor* MakeProcessor();

#endif //_IPLUGAPI_
