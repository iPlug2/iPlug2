#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

#include "IPlugBase.h"
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

using namespace Steinberg;
using namespace Vst;

class IPlugVST3View;

class IPlugVST3 : public IPlugBase
                , public IUnitInfo
                , public SingleComponentEffect
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

  // AudioEffect overrides:
  tresult PLUGIN_API initialize (FUnknown* context);
  tresult PLUGIN_API terminate();
  tresult PLUGIN_API setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts);
  tresult PLUGIN_API setActive(TBool state);
  tresult PLUGIN_API setupProcessing (ProcessSetup& newSetup);
  tresult PLUGIN_API process(ProcessData& data);
//  tresult PLUGIN_API setState(IBStream* state);
//  tresult PLUGIN_API getState(IBStream* state);
//  tresult PLUGIN_API setComponentState(IBStream *state);
  tresult PLUGIN_API canProcessSampleSize(int32 symbolicSampleSize);
  Steinberg::uint32 PLUGIN_API getLatencySamples ();
  
  // IEditController
  IPlugView* PLUGIN_API createView (const char* name);
  tresult PLUGIN_API setEditorState (IBStream* state);
  tresult PLUGIN_API getEditorState (IBStream* state);
  tresult PLUGIN_API setParamNormalized (ParamID tag, ParamValue value);
  ParamValue PLUGIN_API getParamNormalized(ParamID tag);
  ParamValue PLUGIN_API plainParamToNormalized(ParamID tag, ParamValue plainValue);
  tresult PLUGIN_API getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string);
  tresult PLUGIN_API getParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized);

  //IUnitInfo
  int32 PLUGIN_API getUnitCount();
  tresult PLUGIN_API getUnitInfo(int32 unitIndex, UnitInfo& info);
  int32 PLUGIN_API getProgramListCount();
  tresult PLUGIN_API getProgramListInfo(int32 listIndex, ProgramListInfo& info);
  tresult PLUGIN_API getProgramName(ProgramListID listId, int32 programIndex, String128 name);

  virtual tresult PLUGIN_API getProgramInfo(ProgramListID listId, int32 programIndex, Steinberg::Vst::CString attributeId, String128 attributeValue) {return kNotImplemented;}
  virtual tresult PLUGIN_API hasProgramPitchNames(ProgramListID listId, int32 programIndex) {return kNotImplemented;}
  virtual tresult PLUGIN_API getProgramPitchName(ProgramListID listId, int32 programIndex, int16 midiPitch, String128 name) {return kNotImplemented;}
  virtual UnitID PLUGIN_API getSelectedUnit () {return kRootUnitId;}
  virtual tresult PLUGIN_API selectUnit(UnitID unitId) {return kNotImplemented;}
  virtual tresult PLUGIN_API getUnitByBus(MediaType type, BusDirection dir, int32 busIndex, int32 channel, UnitID& unitId) {return kNotImplemented;}
  virtual tresult PLUGIN_API setUnitProgramData(int32 listOrUnitId, int32 programIndex, IBStream* data) {return kNotImplemented;}
  
  //IPlugBase
  virtual void BeginInformHostOfParamChange(int idx);
  virtual void InformHostOfParamChange(int idx, double normalizedValue);
  virtual void EndInformHostOfParamChange(int idx);
  virtual void InformHostOfProgramChange() {};
  
  virtual bool IsRenderingOffline() { return (processSetup.processMode == kOffline); }

  virtual int GetSamplePos();
  virtual double GetTempo();
  virtual void GetTimeSig(int* pNum, int* pDenom);
  virtual void GetTime(ITimeInfo* pTimeInfo);

  virtual void ResizeGraphics(int w, int h);
  void SetLatency(int samples);

  void PopupHostContextMenuForParam(int param, int x, int y);

  // DumpFactoryPresets("/Users/oli/Desktop/",  GUID_DATA1, GUID_DATA2, GUID_DATA3, GUID_DATA4);
//  void DumpFactoryPresets(const char* path, int a, int b, int c, int d);  // TODO

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
  virtual bool SendMidiMsg(IMidiMsg* pMsg) {return false;}  //TODO

private:
  void addDependentView (IPlugVST3View* view);
  void removeDependentView (IPlugVST3View* view);
  virtual tresult beginEdit(ParamID tag);
  virtual tresult performEdit(ParamID tag, ParamValue valueNormalized);
  virtual tresult endEdit(ParamID tag);
  AudioBus* getAudioInput(int32 index);
  AudioBus* getAudioOutput(int32 index);
  SpeakerArrangement getSpeakerArrForChans(int32 chans);

  int mScChans;
  bool mSidechainActive;
//  IMidiQueue mMidiOutputQueue;
  ProcessContext mProcessContext;
  TArray <IPlugVST3View*> viewsArray;

  friend class IPlugVST3View;
};

IPlugVST3* MakePlug();

class IPlugVST3View : public CPluginView
{
public:
  IPlugVST3View(IPlugVST3* pPlug);
  ~IPlugVST3View ();

  // CPluginView overides
  tresult PLUGIN_API attached(void* parent, FIDString type);
  tresult PLUGIN_API removed();

  // IPlugView overides
  tresult PLUGIN_API onSize(ViewRect* newSize);
  tresult PLUGIN_API getSize(ViewRect* size);
  tresult PLUGIN_API isPlatformTypeSupported(FIDString type);

  void resize(int w, int h);

protected:
  IPlugVST3* mPlug;
  bool mExpectingNewSize;
};

#endif