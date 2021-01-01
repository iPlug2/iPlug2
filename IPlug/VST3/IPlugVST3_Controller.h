/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifndef _IPLUGAPI_
#define _IPLUGAPI_

/**
 * @file
 * @copydoc IPlugVST3Controller
 */

#undef stricmp
#undef strnicmp
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstchannelcontextinfo.h"

#include "IPlugAPIBase.h"

#include "IPlugVST3_View.h"
#include "IPlugVST3_ControllerBase.h"
#include "IPlugVST3_Common.h"

BEGIN_IPLUG_NAMESPACE

/**  VST3 Controller API-base class for a distributed IPlug VST3 plug-in
 *   @ingroup APIClasses */
class IPlugVST3Controller : public Steinberg::Vst::EditControllerEx1
                          , public Steinberg::Vst::IMidiMapping
                          , public Steinberg::Vst::ChannelContext::IInfoListener
                          , public IPlugAPIBase
                          , public IPlugVST3ControllerBase
{
public:
  using ViewType = IPlugVST3View<IPlugVST3Controller>;
  
  struct InstanceInfo
  {
    Steinberg::FUID mOtherGUID;
  };
  
  IPlugVST3Controller(const InstanceInfo& info, const Config& config);
  virtual ~IPlugVST3Controller();

  // IEditController
  Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
  Steinberg::IPlugView* PLUGIN_API createView(Steinberg::FIDString name) override;
  Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* pState) override; // receives the processor's state
  Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* pState) override;
  Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* pState) override;
  
  Steinberg::Vst::ParamValue PLUGIN_API getParamNormalized (Steinberg::Vst::ParamID tag) override;
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
  // ComponentBase
  Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) override;

  // IMidiMapping
  Steinberg::tresult PLUGIN_API getMidiControllerAssignment(Steinberg::int32 busIndex, Steinberg::int16 channel, Steinberg::Vst::CtrlNumber midiCCNumber, Steinberg::Vst::ParamID& tag) override;

  // IEditControllerEx
  Steinberg::tresult PLUGIN_API getProgramName(Steinberg::Vst::ProgramListID listId, Steinberg::int32 programIndex, Steinberg::Vst::String128 name /*out*/) override;
  
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

  // Interface
  OBJ_METHODS(IPlugVST3Controller, EditControllerEx1)
  DEFINE_INTERFACES
    DEF_INTERFACE(IMidiMapping)
    DEF_INTERFACE(IInfoListener)
  END_DEFINE_INTERFACES(EditControllerEx1)
  REFCOUNT_METHODS(EditControllerEx1)
  
  // IPlugAPIBase
  void BeginInformHostOfParamChange(int idx) override { beginEdit(idx); }
  void InformHostOfParamChange(int idx, double normalizedValue) override  { performEdit(idx, normalizedValue); }
  void EndInformHostOfParamChange(int idx) override  { endEdit(idx); }
  void InformHostOfPresetChange() override  { /* TODO: */}
  bool EditorResize(int viewWidth, int viewHeight) override;
  void DirtyParametersFromUI() override;
  
  // IEditorDelegate
  void SendMidiMsgFromUI(const IMidiMsg& msg) override;
  void SendSysexMsgFromUI(const ISysEx& msg) override;
  void SendArbitraryMsgFromUI(int msgTag, int ctrlTag = kNoTag, int dataSize = 0, const void* pData = nullptr) override;
  void SendParameterValueFromUI(int paramIdx, double normalisedValue) override;

  Steinberg::Vst::IComponentHandler* GetComponentHandler() const { return componentHandler; }
  ViewType* GetView() const { return mView; }

private:
  ViewType* mView = nullptr;
  bool mPlugIsInstrument;
  bool mDoesMidiIn;
  Steinberg::FUID mProcessorGUID;
};

END_IPLUG_NAMESPACE

#endif // _IPLUGAPI_
