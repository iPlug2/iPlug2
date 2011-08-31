#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

#include "IPlugBase.h"
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
//#include "pluginterfaces/vst/ivstplugview.h"
#include "IMidiQueue.h"

struct IPlugInstanceInfo
{
};

using namespace Steinberg;
using namespace Vst;

class IPlugVST3View;

class IPlugVST3 :  public IPlugBase
                  ,public SingleComponentEffect
{
public:
  
  IPlugVST3(IPlugInstanceInfo instanceInfo, int nParams, const char* channelIOStr, int nPresets,
        const char* effectName, const char* productName, const char* mfrName,
        int vendorVersion, int uniqueID, int mfrID, int latency = 0, 
        bool plugDoesMidi = false, bool plugDoesChunks = false, 
        bool plugIsInst = false, int plugScChans = 0);
  
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
  virtual tresult PLUGIN_API setState(IBStream* state)  {return kNotImplemented;}
  virtual tresult PLUGIN_API getState(IBStream* state)  {return kNotImplemented;}
  
  // IEditController
  IPlugView* PLUGIN_API createView (const char* name);
  tresult PLUGIN_API setEditorState (IBStream* state);
  tresult PLUGIN_API getEditorState (IBStream* state);
  tresult PLUGIN_API setParamNormalized (ParamID tag, ParamValue value);
  ParamValue PLUGIN_API getParamNormalized(ParamID tag);
  ParamValue PLUGIN_API plainParamToNormalized(ParamID tag, ParamValue plainValue);
  tresult PLUGIN_API getParamStringByValue (ParamID tag, ParamValue valueNormalized, String128 string);
  tresult PLUGIN_API getParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized);
  
  // helper
  void addDependentView (IPlugVST3View* view);
  void removeDependentView (IPlugVST3View* view);
  virtual tresult beginEdit(ParamID tag);
  virtual tresult performEdit(ParamID tag, ParamValue valueNormalized);
  virtual tresult endEdit(ParamID tag);
  AudioBus* getAudioInput(int32 index);
  AudioBus* getAudioOutput(int32 index);
  
  virtual void BeginInformHostOfParamChange(int idx);
  virtual void InformHostOfParamChange(int idx, double normalizedValue);
  virtual void EndInformHostOfParamChange(int idx);
  virtual void InformHostOfProgramChange() {};
  
  virtual int GetSamplePos();
  virtual double GetTempo();
  virtual void GetTimeSig(int* pNum, int* pDenom);
  virtual void GetTime(ITimeInfo* pTimeInfo);

  virtual void ResizeGraphics(int w, int h) {} //TODO

protected:
  virtual void HostSpecificInit() {} //TODO
  virtual bool SendMidiMsg(IMidiMsg* pMsg);
  virtual bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs) {return true;} //TODO
  
  virtual void OnActivate(bool active) { TRACE;  IMutexLock lock(this); }
private:
  int mScChans;
  bool mDoesMidi;
  IMidiQueue mMidiOutputQueue;
  ProcessContext mProcessContext;
  TArray <IPlugVST3View*> viewsArray;
};

IPlugVST3* MakePlug();

class IPlugVST3View:  
   public CPluginView
  //,public IParameterFinder
{
public:
  IPlugVST3View(IPlugVST3* pPlug);
  ~IPlugVST3View ();
  
  // CPluginView overides
  tresult PLUGIN_API attached (void* parent, FIDString type);
  tresult PLUGIN_API removed ();

  // for steinberg hardware that has a wheel
  //tresult PLUGIN_API findParameter (int32 xPos, int32 yPos, ParamID& resultTag /*out*/);
  
  // IPlugView overides
  tresult PLUGIN_API onSize (ViewRect* newSize);
  tresult PLUGIN_API getSize (ViewRect* size);
  tresult PLUGIN_API isPlatformTypeSupported (FIDString type);
  
protected:
  IPlugVST3* mPlug;
};

#endif