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

#include "IPlugBase_select.h"

/** Used to pass various instance info to the API class, where needed */
struct IPlugInstanceInfo {};

class IPlugVST3View;

/**  VST3 base class for an IPlug plug-in, inherits from IPlugBase or IPlugBaseGraphics
*   @ingroup APIClasses
*/
class IPlugVST3 : public IPLUG_BASE_CLASS
                , public IPlugProcessor<PLUG_SAMPLE_DST>
                , public Steinberg::Vst::SingleComponentEffect
{
public:
  IPlugVST3(IPlugInstanceInfo instanceInfo, IPlugConfig config);
  ~IPlugVST3();

  //IPlugBase
  void BeginInformHostOfParamChange(int idx) override;
  void InformHostOfParamChange(int idx, double normalizedValue) override;
  void EndInformHostOfParamChange(int idx) override;
  void InformHostOfProgramChange() override {};
  
  //IPlugProcessor
  void ResizeGraphics(int w, int h, double scale) override;
  void SetLatency(int samples) override;
  bool SendMidiMsg(IMidiMsg& msg) override { return false; } //TODO: SendMidiMsg
  
  // AudioEffect
  Steinberg::tresult PLUGIN_API initialize(FUnknown* context) override;
  Steinberg::tresult PLUGIN_API terminate() override;
  Steinberg::tresult PLUGIN_API setBusArrangements(uint64_t* inputs, int32_t numIns, uint64_t* outputs, int32_t numOuts) override;
  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
  Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& newSetup) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
//  Steinberg::tresult PLUGIN_API setState(IBStream* state) override;
//  Steinberg::tresult PLUGIN_API getState(IBStream* state) override;
//  Steinberg::tresult PLUGIN_API setComponentState(IBStream *state) override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(int32_t symbolicSampleSize) override;
  uint32_t PLUGIN_API getLatencySamples () override { return GetLatency(); }
  uint32_t PLUGIN_API getTailSamples() override { return GetTailSize(); }
  
  // IEditController
  Steinberg::IPlugView* PLUGIN_API createView (const char* name) override;
  Steinberg::tresult PLUGIN_API setEditorState (Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API getEditorState (Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API setParamNormalized (uint32_t tag, double value) override;
  double PLUGIN_API getParamNormalized(uint32_t tag) override;
  double PLUGIN_API plainParamToNormalized(uint32_t tag, double plainValue) override;
  Steinberg::tresult PLUGIN_API getParamStringByValue (uint32_t tag, double valueNormalized, Steinberg::Vst::String128 string) override;
  Steinberg::tresult PLUGIN_API getParamValueByString (uint32_t tag, Steinberg::Vst::TChar* string, double& valueNormalized) override;

  //IUnitInfo
  int32_t PLUGIN_API getUnitCount() override;
  Steinberg::tresult PLUGIN_API getUnitInfo(int32_t unitIndex, Steinberg::Vst::UnitInfo& info) override;
  int32_t PLUGIN_API getProgramListCount() override;
  Steinberg::tresult PLUGIN_API getProgramListInfo(int32_t listIndex, Steinberg::Vst::ProgramListInfo& info) override;
  Steinberg::tresult PLUGIN_API getProgramName(int32_t listId, int32_t programIndex, Steinberg::Vst::String128 name) override;

  Steinberg::tresult PLUGIN_API getProgramInfo(int32_t listId, int32_t programIndex, Steinberg::Vst::CString attributeId, Steinberg::Vst::String128 attributeValue) override {return Steinberg::kNotImplemented;}
  Steinberg::tresult PLUGIN_API hasProgramPitchNames(int32_t listId, int32_t programIndex) override {return Steinberg::kNotImplemented;}
  Steinberg::tresult PLUGIN_API getProgramPitchName(int32_t listId, int32_t programIndex, Steinberg::int16 midiPitch, Steinberg::Vst::String128 name) override {return Steinberg::kNotImplemented;}
  int32_t PLUGIN_API getSelectedUnit () override {return Steinberg::Vst::kRootUnitId;}
  Steinberg::tresult PLUGIN_API selectUnit(int32_t unitId) override {return Steinberg::kNotImplemented;}
  Steinberg::tresult PLUGIN_API getUnitByBus(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, int32_t busIndex, int32_t channel, int32_t& unitId) override {return Steinberg::kNotImplemented;}
  Steinberg::tresult PLUGIN_API setUnitProgramData(int32_t listOrUnitId, int32_t programIndex, Steinberg::IBStream* data) override {return Steinberg::kNotImplemented;}
  
  
  Steinberg::Vst::IComponentHandler* GetComponentHandler() { return componentHandler; }
  IPlugVST3View* GetView() { return mViews.at(0); }
  
  
private:
  /** Called prior to rendering a block of audio in order to update processing context data such as transport info */
  void PreProcess();

  enum
  {
//    TODO: add missing parameters
    kBypassParam = 'bpas',
    kPresetParam = 'prst',
//    kModWheelParam = 'modw',
//    kBreathParam = 'brth',
//    kCtrler3Param = 'ct03',
//    kExpressionParam = 'expr',
//    kPitchBendParam = 'pitb',
//    kSustainParam = 'sust',
//    kAftertouchParam = 'aftt',
  };

  OBJ_METHODS(IPlugVST3, SingleComponentEffect)
  DEFINE_INTERFACES
  END_DEFINE_INTERFACES(SingleComponentEffect)
  REFCOUNT_METHODS(SingleComponentEffect)

  void addDependentView (IPlugVST3View* view);
  void removeDependentView (IPlugVST3View* view);
  Steinberg::tresult beginEdit(uint32_t tag) override;
  Steinberg::tresult performEdit(uint32_t tag, double valueNormalized) override;
  Steinberg::tresult endEdit(uint32_t tag) override;
  Steinberg::Vst::AudioBus* getAudioInput(int32_t index);
  Steinberg::Vst::AudioBus* getAudioOutput(int32_t index);
  uint64_t getSpeakerArrForChans(int32_t chans);

  bool mSidechainActive = false;
//  IMidiQueue mMidiOutputQueue;
  Steinberg::Vst::ProcessContext mProcessContext;
  std::vector <IPlugVST3View*> mViews;
  
  friend class IPlugVST3View;
};

IPlugVST3* MakePlug();

/** IPlug VST3 View  */
class IPlugVST3View : public Steinberg::CPluginView
{
public:
  IPlugVST3View(IPlugVST3* pPlug);
  ~IPlugVST3View();

  // CPluginView overides
  Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type) override;
  Steinberg::tresult PLUGIN_API removed() override;

  // IPlugView overides
  Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* newSize) override;
  Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect* size) override;
  Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override;

  void resize(int w, int h);

  IPlugVST3* mPlug;
  bool mExpectingNewSize;
};

#endif
