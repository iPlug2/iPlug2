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
{
public:
  using ViewType = IPlugVST3View<IPlugVST3>;
    
  IPlugVST3(const InstanceInfo& info, const Config& config);
  ~IPlugVST3();

  // IPlugAPIBase
  void BeginInformHostOfParamChange(int idx) override;
  void InformHostOfParamChange(int idx, double normalizedValue) override;
  void EndInformHostOfParamChange(int idx) override;
  void InformHostOfProgramChange() override {}
  void InformHostOfParameterDetailsChange() override;
  bool EditorResizeFromDelegate(int viewWidth, int viewHeight) override;

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
  Steinberg::Vst::ParamValue PLUGIN_API getParamNormalized (Steinberg::Vst::ParamID tag) override;
  Steinberg::tresult PLUGIN_API setParamNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
  Steinberg::IPlugView* PLUGIN_API createView(const char* name) override;
  Steinberg::tresult PLUGIN_API setEditorState(Steinberg::IBStream* pState) override;
  Steinberg::tresult PLUGIN_API getEditorState(Steinberg::IBStream* pState) override;
  Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream *state) override;
 
  // IUnitInfo
  Steinberg::int32 PLUGIN_API getUnitCount() override;
  Steinberg::tresult PLUGIN_API getUnitInfo(Steinberg::int32 unitIndex, Steinberg::Vst::UnitInfo& info) override;
  Steinberg::int32 PLUGIN_API getProgramListCount() override;
  Steinberg::tresult PLUGIN_API getProgramListInfo(Steinberg::int32 listIndex, Steinberg::Vst::ProgramListInfo& info) override;
  Steinberg::tresult PLUGIN_API getProgramName(Steinberg::int32 listId, Steinberg::int32 programIndex, Steinberg::Vst::String128 name) override;

  Steinberg::tresult PLUGIN_API getProgramInfo(Steinberg::int32 listId, Steinberg::int32 programIndex, Steinberg::Vst::CString attributeId, Steinberg::Vst::String128 attributeValue) override { return Steinberg::kNotImplemented; }
  Steinberg::tresult PLUGIN_API hasProgramPitchNames(Steinberg::int32 listId, Steinberg::int32 programIndex) override { return Steinberg::kNotImplemented; }
  Steinberg::tresult PLUGIN_API getProgramPitchName(Steinberg::int32 listId, Steinberg::int32 programIndex, Steinberg::int16 midiPitch, Steinberg::Vst::String128 name) override { return Steinberg::kNotImplemented; }
  Steinberg::int32 PLUGIN_API getSelectedUnit() override { return Steinberg::Vst::kRootUnitId; }
  Steinberg::tresult PLUGIN_API selectUnit(Steinberg::int32 unitId) override { return Steinberg::kNotImplemented; }
  Steinberg::tresult PLUGIN_API getUnitByBus(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 busIndex, Steinberg::int32 channel, Steinberg::int32& unitId) override { return Steinberg::kNotImplemented; }
  Steinberg::tresult PLUGIN_API setUnitProgramData(Steinberg::int32 listOrUnitId, Steinberg::int32 programIndex, Steinberg::IBStream* data) override { return Steinberg::kNotImplemented; }
  
  Steinberg::Vst::IComponentHandler* GetComponentHandler() { return componentHandler; }
  ViewType* GetView() { return mView; }
   
  // Interface    
  OBJ_METHODS(IPlugVST3, SingleComponentEffect)
  DEFINE_INTERFACES
  END_DEFINE_INTERFACES(SingleComponentEffect)
  REFCOUNT_METHODS(SingleComponentEffect)
    
private:
  ViewType* mView;
};

IPlugVST3* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif
