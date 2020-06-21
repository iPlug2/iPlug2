/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

/**
 * @file
 * @copydoc IPlugVST3
 */

#include <vector>

#undef stricmp
#undef strnicmp

#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivstcontextmenu.h"
#include "pluginterfaces/vst/ivstchannelcontextinfo.h"

#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

#include "IPlugVST3_Common.h"
#include "IPlugVST3_ProcessorBase.h"
#include "IPlugVST3_View.h"

BEGIN_IPLUG_NAMESPACE

/** Used to pass various instance info to the API class, where needed */
struct InstanceInfo {};

/**  VST3 base class for a non-distributed IPlug VST3 plug-in
*   @ingroup APIClasses */
class IPlugVST3 : public IPlugAPIBase
                , public IPlugVST3ProcessorBase
                , public IPlugVST3ControllerBase
                , public Steinberg::Vst::SingleComponentEffect
                , public Steinberg::Vst::IMidiMapping
                , public Steinberg::Vst::ChannelContext::IInfoListener
{
public:
  using ViewType = IPlugVST3View<IPlugVST3>;
    
  IPlugVST3(const InstanceInfo& info, const Config& config);
  ~IPlugVST3();

  // IPlugAPIBase
  void BeginInformHostOfParamChange(int idx) override;
  void InformHostOfParamChange(int idx, double normalizedValue) override;
  void EndInformHostOfParamChange(int idx) override;
  void InformHostOfPresetChange() override {}
  void InformHostOfParameterDetailsChange() override;
  bool EditorResize(int viewWidth, int viewHeight) override;

  // IEditorDelegate
  void DirtyParametersFromUI() override;
  
  // IPlugProcessor
  void SetLatency(int samples) override;
  
  // AudioEffect
  Steinberg::tresult PLUGIN_API initialize(FUnknown* context) override;
  Steinberg::tresult PLUGIN_API terminate() override;
  Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* pInputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* pOutputs, Steinberg::int32 numOuts) override;
  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
  Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) override;
  Steinberg::tresult PLUGIN_API setProcessing (Steinberg::TBool state) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
  Steinberg::uint32 PLUGIN_API getLatencySamples() override { return GetLatency(); }
  Steinberg::uint32 PLUGIN_API getTailSamples() override { return GetTailSize(); } //TODO - infinite tail
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* pState) override;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* pState) override;
    
  // IEditController
  Steinberg::Vst::ParamValue PLUGIN_API getParamNormalized(Steinberg::Vst::ParamID tag) override;
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
  Steinberg::IPlugView* PLUGIN_API createView(const char* name) override;
  Steinberg::tresult PLUGIN_API setEditorState(Steinberg::IBStream* pState) override;
  Steinberg::tresult PLUGIN_API getEditorState(Steinberg::IBStream* pState) override;
  Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream *state) override;
 
  // IMidiMapping
  Steinberg::tresult PLUGIN_API getMidiControllerAssignment(Steinberg::int32 busIndex, Steinberg::int16 channel, Steinberg::Vst::CtrlNumber midiCCNumber, Steinberg::Vst::ParamID& tag) override;
  
  // IInfoListener
  Steinberg::tresult PLUGIN_API setChannelContextInfos(Steinberg::Vst::IAttributeList* list) override;

  /** Get the color of the track that the plug-in is inserted on */
  virtual void GetTrackColor(int& r, int& g, int& b) override { r = (mChannelColor>>16)&0xff; g = (mChannelColor>>8)&0xff; b = mChannelColor&0xff; };

  /** Get the name of the track that the plug-in is inserted on */
  virtual void GetTrackName(WDL_String& str) override { str = mChannelName; };

  /** Get the index of the track that the plug-in is inserted on */
  virtual int GetTrackIndex() override { return mChannelIndex; };

  /** Get the namespace of the track that the plug-in is inserted on */
  virtual void GetTrackNamespace(WDL_String& str) override { str = mChannelNamespace; };

  /** Get the namespace index of the track that the plug-in is inserted on */
  virtual int GetTrackNamespaceIndex() override { return mChannelNamespaceIndex; };

  Steinberg::Vst::IComponentHandler* GetComponentHandler() { return componentHandler; }
  ViewType* GetView() { return mView; }
  
  Steinberg::Vst::AudioBus* getAudioInput(Steinberg::int32 index)
  {
    Steinberg::Vst::AudioBus* bus = nullptr;
    if (index < static_cast<Steinberg::int32> (audioInputs.size ()))
      bus = Steinberg::FCast<Steinberg::Vst::AudioBus> (audioInputs.at (index));
    return bus;
  }

  Steinberg::Vst::AudioBus* getAudioOutput(Steinberg::int32 index)
  {
    Steinberg::Vst::AudioBus* bus = nullptr;
    if (index < static_cast<Steinberg::int32> (audioOutputs.size ()))
      bus = Steinberg::FCast<Steinberg::Vst::AudioBus> (audioOutputs.at (index));
    return bus;
  }
  
  void removeAudioInputBus(Steinberg::Vst::AudioBus* pBus)
  {
    audioInputs.erase(std::remove(audioInputs.begin(), audioInputs.end(), pBus));
  }
   
  void removeAudioOutputBus(Steinberg::Vst::AudioBus* pBus)
  {
    audioOutputs.erase(std::remove(audioOutputs.begin(), audioOutputs.end(), pBus));
  }
   
  // Interface    
  OBJ_METHODS(IPlugVST3, SingleComponentEffect)
  DEFINE_INTERFACES
    DEF_INTERFACE(IMidiMapping)
    DEF_INTERFACE(IInfoListener)
  END_DEFINE_INTERFACES(SingleComponentEffect)
  REFCOUNT_METHODS(SingleComponentEffect)

private:
  ViewType* mView;
};

IPlugVST3* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif
