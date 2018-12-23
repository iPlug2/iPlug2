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

/** Used to pass various instance info to the API class, where needed */
struct IPlugInstanceInfo {};

class IPlugVST3View;

using namespace Steinberg;

#pragma mark - IPlugVST3 constructor
/**  VST3 base class for a non-distributed IPlug VST3 plug-in
*   @ingroup APIClasses */
class IPlugVST3 : public IPlugAPIBase
                , public IPlugProcessor<PLUG_SAMPLE_DST>
                , public Vst::SingleComponentEffect
{
public:
  IPlugVST3(IPlugInstanceInfo instanceInfo, IPlugConfig config);
  ~IPlugVST3();

  //IPlugAPIBase
  void BeginInformHostOfParamChange(int idx) override;
  void InformHostOfParamChange(int idx, double normalizedValue) override;
  void EndInformHostOfParamChange(int idx) override;
  void InformHostOfProgramChange() override {}
  void InformHostOfParameterDetailsChange() override;
  
  //IEditorDelegate
  void DirtyParametersFromUI() override;
  
  //IPlugProcessor
  void ResizeGraphics(int viewWidth, int viewHeight, float scale) override;
  void SetLatency(int samples) override;
  bool SendMidiMsg(const IMidiMsg& msg) override;
  
  // AudioEffect
  tresult PLUGIN_API initialize(FUnknown* context) override;
  tresult PLUGIN_API terminate() override;
  tresult PLUGIN_API setBusArrangements(Vst::SpeakerArrangement* pInputs, int32 numIns, Vst::SpeakerArrangement* pOutputs, int32 numOuts) override;
  tresult PLUGIN_API setActive(TBool state) override;
  tresult PLUGIN_API setupProcessing(Vst::ProcessSetup& newSetup) override;
  tresult PLUGIN_API process(Vst::ProcessData& data) override;
//  tresult PLUGIN_API setState(IBStream* state) override;
//  tresult PLUGIN_API getState(IBStream* state) override;
//  tresult PLUGIN_API setComponentState(IBStream *state) override;
  tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize) override;
  uint32 PLUGIN_API getLatencySamples () override { return GetLatency(); }
  uint32 PLUGIN_API getTailSamples() override { return GetTailSize(); }
  
  // IEditController
  IPlugView* PLUGIN_API createView (const char* name) override;
  tresult PLUGIN_API setEditorState (IBStream* state) override;
  tresult PLUGIN_API getEditorState (IBStream* state) override;
  tresult PLUGIN_API setState(IBStream* state) override;
  tresult PLUGIN_API getState(IBStream* state) override;
  tresult PLUGIN_API setComponentState(IBStream *state) override;
  tresult PLUGIN_API setParamNormalized (uint32 tag, double value) override;
  double PLUGIN_API getParamNormalized(uint32 tag) override;
  double PLUGIN_API plainParamToNormalized(uint32 tag, double plainValue) override;
  tresult PLUGIN_API getParamStringByValue (uint32 tag, double valueNormalized, Vst::String128 string) override;
  tresult PLUGIN_API getParamValueByString (uint32 tag, Vst::TChar* string, double& valueNormalized) override;

  //IUnitInfo
  int32 PLUGIN_API getUnitCount() override;
  tresult PLUGIN_API getUnitInfo(int32 unitIndex, Vst::UnitInfo& info) override;
  int32 PLUGIN_API getProgramListCount() override;
  tresult PLUGIN_API getProgramListInfo(int32 listIndex, Vst::ProgramListInfo& info) override;
  tresult PLUGIN_API getProgramName(int32 listId, int32 programIndex, Vst::String128 name) override;

  tresult PLUGIN_API getProgramInfo(int32 listId, int32 programIndex, Vst::CString attributeId, Vst::String128 attributeValue) override {return kNotImplemented;}
  tresult PLUGIN_API hasProgramPitchNames(int32 listId, int32 programIndex) override {return kNotImplemented;}
  tresult PLUGIN_API getProgramPitchName(int32 listId, int32 programIndex, int16 midiPitch, Vst::String128 name) override {return kNotImplemented;}
  int32 PLUGIN_API getSelectedUnit () override {return Vst::kRootUnitId;}
  tresult PLUGIN_API selectUnit(int32 unitId) override {return kNotImplemented;}
  tresult PLUGIN_API getUnitByBus(Vst::MediaType type, Vst::BusDirection dir, int32 busIndex, int32 channel, int32& unitId) override {return kNotImplemented;}
  tresult PLUGIN_API setUnitProgramData(int32 listOrUnitId, int32 programIndex, IBStream* data) override {return kNotImplemented;}
  
  Vst::IComponentHandler* GetComponentHandler() { return componentHandler; }
  IPlugVST3View* GetView() { return mViews.at(0); }
  
private:
  /** Called prior to rendering a block of audio in order to update processing context data such as transport info */
  void PreProcess();

  OBJ_METHODS(IPlugVST3, SingleComponentEffect)
  DEFINE_INTERFACES
  END_DEFINE_INTERFACES(SingleComponentEffect)
  REFCOUNT_METHODS(SingleComponentEffect)

  void addDependentView (IPlugVST3View* view);
  void removeDependentView (IPlugVST3View* view);
  tresult beginEdit(uint32 tag) override;
  tresult performEdit(uint32 tag, double valueNormalized) override;
  tresult endEdit(uint32 tag) override;
  Vst::AudioBus* getAudioInput(int32 index);
  Vst::AudioBus* getAudioOutput(int32 index);
  uint64_t getSpeakerArrForChans(int32 chans);

  bool mSidechainActive = false;
  IMidiQueue mMidiOutputQueue;
  Vst::ProcessContext mProcessContext;
  std::vector <IPlugVST3View*> mViews;
  
  friend class IPlugVST3View;
};

IPlugVST3* MakePlug();

/** IPlug VST3 View  */
class IPlugVST3View : public CPluginView
{
public:
  IPlugVST3View(IPlugVST3* pPlug);
  ~IPlugVST3View();

  // CPluginView overides
  tresult PLUGIN_API attached(void* parent, FIDString type) override;
  tresult PLUGIN_API removed() override;

  // IPlugView overides
  tresult PLUGIN_API onSize(ViewRect* newSize) override;
  tresult PLUGIN_API getSize(ViewRect* size) override;
  tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override;

  void resize(int w, int h);

  IPlugVST3* mPlug;
  bool mExpectingNewSize;
};

#endif
