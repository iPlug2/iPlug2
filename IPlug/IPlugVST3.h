#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

#include <vector>
#ifdef NO_IGRAPHICS
#include "IPlugBase.h"
typedef IPlugBase IPLUG_BASE_CLASS;
#else
#include "IPlugBaseGraphics.h"
typedef IPlugBaseGraphics IPLUG_BASE_CLASS;
#endif

#undef stricmp
#undef strnicmp
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
//#include "public.sdk/source/vst/vstpresetfile.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivstcontextmenu.h"
//#include "IMidiQueue.h"

struct IPlugInstanceInfo
{
  // not needed
};

class IPlugVST3View;

class IPlugVST3 : public IPLUG_BASE_CLASS
                , public Steinberg::Vst::IUnitInfo
                , public Steinberg::Vst::SingleComponentEffect
{
public:
  IPlugVST3(IPlugInstanceInfo instanceInfo,
            int nParams,
            const char* channelIOStr,
            int nPresets,
            const char* effectName,
            const char* productName,
            const char* mfrName,
            int vendorVersion,
            int uniqueID,
            int mfrID,
            int latency = 0,
            bool plugDoesMidi = false,
            bool plugDoesChunks = false,
            bool plugIsInst = false,
            int plugScChans = 0);

  virtual ~IPlugVST3();

  // AudioEffect
  Steinberg::tresult PLUGIN_API initialize (FUnknown* context) override;
  Steinberg::tresult PLUGIN_API terminate() override;
  Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;
  Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
  Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& newSetup) override;
  Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
//  Steinberg::tresult PLUGIN_API setState(IBStream* state) override;
//  Steinberg::tresult PLUGIN_API getState(IBStream* state) override;
//  Steinberg::tresult PLUGIN_API setComponentState(IBStream *state) override;
  Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
  Steinberg::uint32 PLUGIN_API getLatencySamples () override;
  Steinberg::uint32 PLUGIN_API getTailSamples() override { return GetTailSize(); }
  // IEditController
  Steinberg::IPlugView* PLUGIN_API createView (const char* name) override;
  Steinberg::tresult PLUGIN_API setEditorState (Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API getEditorState (Steinberg::IBStream* state) override;
  Steinberg::tresult PLUGIN_API setParamNormalized (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) override;
  Steinberg::Vst::ParamValue PLUGIN_API getParamNormalized(Steinberg::Vst::ParamID tag) override;
  Steinberg::Vst::ParamValue PLUGIN_API plainParamToNormalized(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue plainValue) override;
  Steinberg::tresult PLUGIN_API getParamStringByValue (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized, Steinberg::Vst::String128 string) override;
  Steinberg::tresult PLUGIN_API getParamValueByString (Steinberg::Vst::ParamID tag, Steinberg::Vst::TChar* string, Steinberg::Vst::ParamValue& valueNormalized) override;

  //IUnitInfo
  Steinberg::int32 PLUGIN_API getUnitCount() override;
  Steinberg::tresult PLUGIN_API getUnitInfo(Steinberg::int32 unitIndex, Steinberg::Vst::UnitInfo& info) override;
  Steinberg::int32 PLUGIN_API getProgramListCount() override;
  Steinberg::tresult PLUGIN_API getProgramListInfo(Steinberg::int32 listIndex, Steinberg::Vst::ProgramListInfo& info) override;
  Steinberg::tresult PLUGIN_API getProgramName(Steinberg::Vst::ProgramListID listId, Steinberg::int32 programIndex, Steinberg::Vst::String128 name) override;

  virtual Steinberg::tresult PLUGIN_API getProgramInfo(Steinberg::Vst::ProgramListID listId, Steinberg::int32 programIndex, Steinberg::Vst::CString attributeId, Steinberg::Vst::String128 attributeValue) override {return Steinberg::kNotImplemented;}
  virtual Steinberg::tresult PLUGIN_API hasProgramPitchNames(Steinberg::Vst::ProgramListID listId, Steinberg::int32 programIndex) override {return Steinberg::kNotImplemented;}
  virtual Steinberg::tresult PLUGIN_API getProgramPitchName(Steinberg::Vst::ProgramListID listId, Steinberg::int32 programIndex, Steinberg::int16 midiPitch, Steinberg::Vst::String128 name) override {return Steinberg::kNotImplemented;}
  virtual Steinberg::Vst::UnitID PLUGIN_API getSelectedUnit () override {return Steinberg::Vst::kRootUnitId;}
  virtual Steinberg::tresult PLUGIN_API selectUnit(Steinberg::Vst::UnitID unitId) override {return Steinberg::kNotImplemented;}
  virtual Steinberg::tresult PLUGIN_API getUnitByBus(Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection dir, Steinberg::int32 busIndex, Steinberg::int32 channel, Steinberg::Vst::UnitID& unitId) override {return Steinberg::kNotImplemented;}
  virtual Steinberg::tresult PLUGIN_API setUnitProgramData(Steinberg::int32 listOrUnitId, Steinberg::int32 programIndex, Steinberg::IBStream* data) override {return Steinberg::kNotImplemented;}
  
  //IPlugBase
  virtual void BeginInformHostOfParamChange(int idx) override;
  virtual void InformHostOfParamChange(int idx, double normalizedValue) override;
  virtual void EndInformHostOfParamChange(int idx) override;
  virtual void InformHostOfProgramChange() override {};
  
  virtual bool IsRenderingOffline() override { return (processSetup.processMode == Steinberg::Vst::kOffline); }

  virtual int GetSamplePos() override;
  virtual double GetTempo() override;
  virtual void GetTimeSig(int& numerator, int& denominator) override;
  virtual void GetTime(ITimeInfo& timeInfo) override;

  virtual void ResizeGraphics(int w, int h) override;
  void SetLatency(int samples) override;

  void PopupHostContextMenuForParam(int param, int x, int y) override;

  enum
  {
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

  OBJ_METHODS (IPlugVST3, SingleComponentEffect)
  DEFINE_INTERFACES
  DEF_INTERFACE (IUnitInfo)
  END_DEFINE_INTERFACES (SingleComponentEffect)
  REFCOUNT_METHODS(SingleComponentEffect)

protected:
  virtual bool SendMidiMsg(IMidiMsg& msg) override { return false; } //TODO:

private:
  void addDependentView (IPlugVST3View* view);
  void removeDependentView (IPlugVST3View* view);
  virtual Steinberg::tresult beginEdit(Steinberg::Vst::ParamID tag) override;
  virtual Steinberg::tresult performEdit(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue valueNormalized) override;
  virtual Steinberg::tresult endEdit(Steinberg::Vst::ParamID tag) override;
  Steinberg::Vst::AudioBus* getAudioInput(Steinberg::int32 index);
  Steinberg::Vst::AudioBus* getAudioOutput(Steinberg::int32 index);
  Steinberg::Vst::SpeakerArrangement getSpeakerArrForChans(Steinberg::int32 chans);

  int mScChans;
  bool mSidechainActive;
//  IMidiQueue mMidiOutputQueue;
  Steinberg::Vst::ProcessContext mProcessContext;
  std::vector <IPlugVST3View*> mViews;

  friend class IPlugVST3View;
};

IPlugVST3* MakePlug();

class IPlugVST3View : public Steinberg::CPluginView
{
public:
  IPlugVST3View(IPlugVST3* pPlug);
  ~IPlugVST3View ();

  // CPluginView overides
  Steinberg::tresult PLUGIN_API attached(void* parent, Steinberg::FIDString type);
  Steinberg::tresult PLUGIN_API removed();

  // IPlugView overides
  Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* newSize);
  Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect* size);
  Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type);

  void resize(int w, int h);

protected:
  IPlugVST3* mPlug;
  bool mExpectingNewSize;
};

#endif
