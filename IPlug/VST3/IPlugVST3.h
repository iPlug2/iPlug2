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

using namespace Steinberg;
using namespace Vst;

/**  VST3 base class for a non-distributed IPlug VST3 plug-in
*   @ingroup APIClasses */
class IPlugVST3 : public IPlugAPIBase
                , public IPlugVST3ProcessorBase
                , public IPlugVST3ControllerBase
                , public Vst::SingleComponentEffect
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
  tresult PLUGIN_API initialize(FUnknown* context) override;
  tresult PLUGIN_API terminate() override;
  tresult PLUGIN_API setBusArrangements(Vst::SpeakerArrangement* pInputs, int32 numIns, Vst::SpeakerArrangement* pOutputs, int32 numOuts) override;
  tresult PLUGIN_API setActive(TBool state) override;
  tresult PLUGIN_API setupProcessing(Vst::ProcessSetup& newSetup) override;
  tresult PLUGIN_API process(Vst::ProcessData& data) override;
  tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;
  uint32 PLUGIN_API getLatencySamples() override { return GetLatency(); }
  uint32 PLUGIN_API getTailSamples() override { return GetTailSize(); } //TODO - infinite tail
  tresult PLUGIN_API setState(IBStream* pState) override;
  tresult PLUGIN_API getState(IBStream* pState) override;
    
  // IEditController
  ParamValue PLUGIN_API getParamNormalized (ParamID tag) override;
  tresult PLUGIN_API setParamNormalized(ParamID tag, ParamValue value) override;
  IPlugView* PLUGIN_API createView(const char* name) override;
  tresult PLUGIN_API setEditorState(IBStream* pState) override;
  tresult PLUGIN_API getEditorState(IBStream* pState) override;
  tresult PLUGIN_API setComponentState(IBStream *state) override;
 
  // IUnitInfo
  int32 PLUGIN_API getUnitCount() override;
  tresult PLUGIN_API getUnitInfo(int32 unitIndex, Vst::UnitInfo& info) override;
  int32 PLUGIN_API getProgramListCount() override;
  tresult PLUGIN_API getProgramListInfo(int32 listIndex, Vst::ProgramListInfo& info) override;
  tresult PLUGIN_API getProgramName(int32 listId, int32 programIndex, Vst::String128 name) override;

  tresult PLUGIN_API getProgramInfo(int32 listId, int32 programIndex, Vst::CString attributeId, Vst::String128 attributeValue) override { return kNotImplemented; }
  tresult PLUGIN_API hasProgramPitchNames(int32 listId, int32 programIndex) override { return kNotImplemented; }
  tresult PLUGIN_API getProgramPitchName(int32 listId, int32 programIndex, int16 midiPitch, Vst::String128 name) override { return kNotImplemented; }
  int32 PLUGIN_API getSelectedUnit() override { return Vst::kRootUnitId; }
  tresult PLUGIN_API selectUnit(int32 unitId) override { return kNotImplemented; }
  tresult PLUGIN_API getUnitByBus(Vst::MediaType type, Vst::BusDirection dir, int32 busIndex, int32 channel, int32& unitId) override { return kNotImplemented; }
  tresult PLUGIN_API setUnitProgramData(int32 listOrUnitId, int32 programIndex, IBStream* data) override { return kNotImplemented; }
  
  Vst::IComponentHandler* GetComponentHandler() { return componentHandler; }
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
