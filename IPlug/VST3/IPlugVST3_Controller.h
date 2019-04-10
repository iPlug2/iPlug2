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

#include "IPlugAPIBase.h"

#include "IPlugVST3_View.h"
#include "IPlugVST3_ControllerBase.h"
#include "IPlugVST3_Common.h"

using namespace Steinberg;
using namespace Vst;

/**  VST3 Controller API-base class for a distributed IPlug VST3 plug-in
 *   @ingroup APIClasses */
class IPlugVST3Controller : public EditControllerEx1
                          , public IMidiMapping
                          , public IPlugAPIBase
                          , public IPlugVST3ControllerBase
{
public:
  typedef IPlugVST3View<IPlugVST3Controller> ViewType;
    
  struct IPlugInstanceInfo
  {
    Steinberg::FUID mOtherGUID;
  };
  
  IPlugVST3Controller(IPlugInstanceInfo instanceInfo, IPlugConfig c);
  virtual ~IPlugVST3Controller();

  // IEditController
  tresult PLUGIN_API initialize(FUnknown* context) override;
  IPlugView* PLUGIN_API createView(FIDString name) override;
  tresult PLUGIN_API setComponentState(IBStream* pState) override; // receives the processor's state
  tresult PLUGIN_API setState(IBStream* pState) override;
  tresult PLUGIN_API getState(IBStream* pState) override;
  
  tresult PLUGIN_API setParamNormalized(ParamID tag, ParamValue value) override;
  // ComponentBase
  tresult PLUGIN_API notify(IMessage* message) override;

  // IMidiMapping
  tresult PLUGIN_API getMidiControllerAssignment(int32 busIndex, int16 channel, CtrlNumber midiControllerNumber, ParamID& tag) override;

  // IEditControllerEx
  tresult PLUGIN_API getProgramName(ProgramListID listId, int32 programIndex, String128 name /*out*/) override;
  
  DELEGATE_REFCOUNT(EditControllerEx1)
  tresult PLUGIN_API queryInterface(const char* iid, void** obj) override;
  
  // IPlugAPIBase
  void BeginInformHostOfParamChange(int idx) override { beginEdit(idx); }
  void InformHostOfParamChange(int idx, double normalizedValue) override  { performEdit(idx, normalizedValue); }
  void EndInformHostOfParamChange(int idx) override  { endEdit(idx); }
  void InformHostOfProgramChange() override  { /* TODO: */}
  void EditorPropertiesChangedFromDelegate(int viewWidth, int viewHeight, const IByteChunk& data) override;
  void DirtyParametersFromUI() override;
  
  // IEditorDelegate
  void SendMidiMsgFromUI(const IMidiMsg& msg) override;
  void SendSysexMsgFromUI(const ISysEx& msg) override;
  void SendArbitraryMsgFromUI(int messageTag, int controlTag = kNoTag, int dataSize = 0, const void* pData = nullptr) override;

  Vst::IComponentHandler* GetComponentHandler() const { return componentHandler; }
  ViewType* GetView() const { return mView; }

private:
  ViewType* mView = nullptr;
  bool mPlugIsInstrument;
  Steinberg::FUID mProcessorGUID;
};

IPlugVST3Controller* MakeController();

#endif // _IPLUGAPI_
