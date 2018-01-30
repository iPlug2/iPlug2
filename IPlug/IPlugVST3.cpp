#include <cstdio>

#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"

#include "IPlugVST3.h"

using namespace Steinberg;
using namespace Vst;

class IPlugVST3Parameter : public Parameter
{
public:
  IPlugVST3Parameter (IParam* pParam, ParamID tag, UnitID unitID)
  : mIPlugParam(pParam)
  {
    UString (info.title, str16BufferSize (String128)).assign (pParam->GetNameForHost());
    UString (info.units, str16BufferSize (String128)).assign (pParam->GetLabelForHost());
    
    precision = pParam->GetPrecision();
    
    if (pParam->Type() != IParam::kTypeDouble)
      info.stepCount = pParam->GetRange();
    else
      info.stepCount = 0; // continuous
    
    int32_t flags = 0;

    if (pParam->GetCanAutomate())
    {
      flags |= ParameterInfo::kCanAutomate;
    }
    
    info.defaultNormalizedValue = valueNormalized = pParam->GetDefaultNormalized();
    info.flags = flags;
    info.id = tag;
    info.unitId = unitID;
  }
  
  virtual void toString (ParamValue valueNormalized, String128 string) const override
  {
    WDL_String display;
    mIPlugParam->GetDisplayForHost(valueNormalized, true, display);
    Steinberg::UString(string, 128).fromAscii(display.Get());
  }
  
  virtual bool fromString (const TChar* string, ParamValue& valueNormalized) const override
  {
    String str ((TChar*)string);
    valueNormalized = mIPlugParam->GetNormalized(atof(str.text8()));
    
    return true;
  }
  
  virtual Steinberg::Vst::ParamValue toPlain (ParamValue valueNormalized) const override
  {
    return mIPlugParam->GetNonNormalized(valueNormalized);
  }
  
  virtual Steinberg::Vst::ParamValue toNormalized (ParamValue plainValue) const override
  {
    return mIPlugParam->GetNormalized(valueNormalized);
  }
  
  OBJ_METHODS (IPlugVST3Parameter, Parameter)
  
protected:
  IParam* mIPlugParam;
};

IPlugVST3::IPlugVST3(IPlugInstanceInfo instanceInfo, IPlugConfig c)
: IPLUG_BASE_CLASS(c, kAPIVST3)
{
  SetInputChannelConnections(0, NInChannels(), true);
  SetOutputChannelConnections(0, NOutChannels(), true);
  
  if (NInChannels()) 
  {
    mLatencyDelay = new NChanDelayLine<double>(NInChannels(), NOutChannels());
    mLatencyDelay->SetDelayTime(GetLatency());
  }

  // initialize the bus labels
  SetInputBusLabel(0, "Main Input");

  if (HasSidechainInput())
    SetInputBusLabel(1, "Aux Input");
  
  if (IsInstrument())
  {
    int busNum = 0;
    char label[32]; //TODO: 32!!!

    for (int i = 0; i < NOutChannels(); i+=2) // stereo buses only
    {
      sprintf(label, "Output %i", busNum+1);
      SetOutputBusLabel(busNum++, label);
    }
  }
  else
  {
    SetOutputBusLabel(0, "Output");
  }
    
  // Make sure the process context is predictably initialised in case it is used before process is called
 
  memset(&mProcessContext, 0, sizeof(ProcessContext));
}

IPlugVST3::~IPlugVST3() {}

#pragma mark -
#pragma mark AudioEffect overrides

tresult PLUGIN_API IPlugVST3::initialize (FUnknown* context)
{
  TRACE;

  tresult result = SingleComponentEffect::initialize(context);

  String128 tmpStringBuf;
  char hostNameCString[128];
  FUnknownPtr<IHostApplication>app(context);

  if (app)
  {
    app->getName(tmpStringBuf);
    Steinberg::UString(tmpStringBuf, 128).toAscii(hostNameCString, 128);
    SetHost(hostNameCString, 0); // Can't get version in VST3
  }

  if (result == kResultOk)
  {
    SpeakerArrangement maxInputs = getSpeakerArrForChans(NInChannels()-NSidechainChannels());
    if(maxInputs < 0) maxInputs = 0;

    // add io buses with the maximum i/o to start with

    if (maxInputs)
    {
      Steinberg::UString(tmpStringBuf, 128).fromAscii(GetInputBusLabel(0)->Get(), 128);
      addAudioInput(tmpStringBuf, maxInputs);
    }

    if(!IsInstrument()) // if effect, just add one output bus with max chan count
    {
      Steinberg::UString(tmpStringBuf, 128).fromAscii(GetOutputBusLabel(0)->Get(), 128);
      addAudioOutput(tmpStringBuf, getSpeakerArrForChans(NOutChannels()) );
    }
    else
    {
      for (int i = 0, busIdx = 0; i < NOutChannels(); i+=2, busIdx++)
      {
        Steinberg::UString(tmpStringBuf, 128).fromAscii(GetOutputBusLabel(busIdx)->Get(), 128);
        addAudioOutput(tmpStringBuf, SpeakerArr::kStereo );
      }
    }

    if (HasSidechainInput())
    {
      assert(NSidechainChannels() < 2); // TODO: side-chain input with more than 2 channels?
      Steinberg::UString(tmpStringBuf, 128).fromAscii(GetInputBusLabel(1)->Get(), 128);
      addAudioInput(tmpStringBuf, getSpeakerArrForChans(NSidechainChannels()), kAux, 0);
    }

    if(DoesMIDI())
    {
      addEventInput (STR16("MIDI Input"), 1);
      //addEventOutput(STR16("MIDI Output"), 1);
    }

    if (NPresets())
    {
      parameters.addParameter(new Parameter(STR16("Preset"),
                                            kPresetParam,
                                            STR16(""),
                                            0,
                                            NPresets(),
                                            ParameterInfo::kIsProgramChange));
    }

    if(!IsInstrument())
    {
      StringListParameter * bypass = new StringListParameter(STR16("Bypass"),
                                                            kBypassParam,
                                                            0,
                                                            ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass | ParameterInfo::kIsList);
      bypass->appendString(STR16("off"));
      bypass->appendString(STR16("on"));
      parameters.addParameter(bypass);
    }

    for (int i=0; i<NParams(); i++)
    {
      IParam *p = GetParam(i);

      UnitID unitID = kRootUnitId;
      
      const char* paramGroupName = p->GetParamGroupForHost();

      if (CSTR_NOT_EMPTY(paramGroupName))
      {        
        for(int j = 0; j < mParamGroups.GetSize(); j++)
        {
          if(strcmp(paramGroupName, mParamGroups.Get(j)) == 0)
          {
            unitID = j+1;
          }
        }
        
        if (unitID == kRootUnitId) // new unit, nothing found, so add it
        {
          mParamGroups.Add(paramGroupName);
          unitID = mParamGroups.GetSize();
        }
      }
      
      Parameter* param = new IPlugVST3Parameter(p, i, unitID);
      parameters.addParameter(param);
    }
  }

  OnHostIdentified();
  RestorePreset(0);
  
  return result;
}

tresult PLUGIN_API IPlugVST3::terminate ()
{
  TRACE;

  mViews.empty();
  return SingleComponentEffect::terminate();
}

tresult PLUGIN_API IPlugVST3::setBusArrangements(SpeakerArrangement* inputs, int32_t numIns, SpeakerArrangement* outputs, int32_t numOuts)
{
  TRACE;

  // disconnect all io pins, they will be reconnected in process
  SetInputChannelConnections(0, NInChannels(), false);
  SetOutputChannelConnections(0, NOutChannels(), false);

  int32_t reqNumInputChannels = SpeakerArr::getChannelCount(inputs[0]);  //requested # input channels
  int32_t reqNumOutputChannels = SpeakerArr::getChannelCount(outputs[0]);//requested # output channels

  // legal io doesn't consider sidechain inputs
  if (!LegalIO(reqNumInputChannels, reqNumOutputChannels))
  {
    return kResultFalse;
  }

  // handle input
  AudioBus* bus = FCast<AudioBus>(audioInputs.at(0));

  // if existing input bus has a different number of channels to the input bus being connected
  if (bus && SpeakerArr::getChannelCount(bus->getArrangement()) != reqNumInputChannels)
  {
    audioInputs.erase(std::remove(audioInputs.begin(), audioInputs.end(), bus));
    addAudioInput(USTRING("Input"), getSpeakerArrForChans(reqNumInputChannels));
  }

  // handle output
  bus = FCast<AudioBus>(audioOutputs.at(0));
  // if existing output bus has a different number of channels to the output bus being connected
  if (bus && SpeakerArr::getChannelCount(bus->getArrangement()) != reqNumOutputChannels)
  {
    audioOutputs.erase(std::remove(audioOutputs.begin(), audioOutputs.end(), bus));
    addAudioOutput(USTRING("Output"), getSpeakerArrForChans(reqNumOutputChannels));
  }

  if (!HasSidechainInput() && numIns == 1) // No sidechain, every thing OK
  {
    return kResultTrue;
  }

  if (HasSidechainInput() && numIns == 2) // numIns = num Input BUSes
  {
    int32_t reqNumSideChainChannels = SpeakerArr::getChannelCount(inputs[1]);  //requested # sidechain input channels

    bus = FCast<AudioBus>(audioInputs.at(1));

    if (bus && SpeakerArr::getChannelCount(bus->getArrangement()) != reqNumSideChainChannels)
    {
      audioInputs.erase(std::remove(audioInputs.begin(), audioInputs.end(), bus));
      addAudioInput(USTRING("Sidechain Input"), getSpeakerArrForChans(reqNumSideChainChannels), kAux, 0); // either mono or stereo
    }

    return kResultTrue;
  }

  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::setActive(TBool state)
{
  TRACE;

  OnActivate((bool) state);

  return SingleComponentEffect::setActive(state);
}

tresult PLUGIN_API IPlugVST3::setupProcessing (ProcessSetup& newSetup)
{
  TRACE;

  if ((newSetup.symbolicSampleSize != kSample32) && (newSetup.symbolicSampleSize != kSample64)) return kResultFalse;

  mSampleRate = newSetup.sampleRate;
  mBypassed = false;
  IPlugBase::SetBlockSize(newSetup.maxSamplesPerBlock);
  OnReset();

  processSetup = newSetup;

  return kResultOk;
}

tresult PLUGIN_API IPlugVST3::process(ProcessData& data)
{
  TRACE_PROCESS;

  if(data.processContext)
    memcpy(&mProcessContext, data.processContext, sizeof(ProcessContext));

  GetTimeInfo();
  
  //process parameters
  IParameterChanges* paramChanges = data.inputParameterChanges;
  if (paramChanges)
  {
    int32_t numParamsChanged = paramChanges->getParameterCount();

    //it is possible to get a finer resolution of control here by retrieving more values (points) from the queue
    //for now we just grab the last one

    for (int32_t i = 0; i < numParamsChanged; i++)
    {
      IParamValueQueue* paramQueue = paramChanges->getParameterData(i);
      if (paramQueue)
      {
        int32_t numPoints = paramQueue->getPointCount();
        int32_t offsetSamples;
        double value;

        if (paramQueue->getPoint(numPoints - 1,  offsetSamples, value) == kResultTrue)
        {
          int idx = paramQueue->getParameterId();

          switch (idx)
          {
            case kBypassParam:
            {
               const bool bypassed = (value > 0.5);
              
              if (bypassed != mBypassed)
                mBypassed = bypassed;

              break;
            }
            case kPresetParam:
              RestorePreset(FromNormalizedParam(value, 0, NPresets(), 1.));
              break;
              //TODO: pitch bend, modwheel etc
            default:
              {
                WDL_MutexLock lock(&mParams_mutex);
                if (idx >= 0 && idx < NParams())
                {
                  GetParam(idx)->SetNormalized((double)value);
                  SetParameterInUIFromAPI(idx, (double) value, true);
                  OnParamChange(idx);
                }
              }
              break;
          }

        }
      }
    }
  }

  if(DoesMIDI())
  {
    //process events.. only midi note on and note off?
    IEventList* eventList = data.inputEvents;
    if (eventList)
    {
      int32_t numEvent = eventList->getEventCount();
      for (int32_t i=0; i<numEvent; i++)
      {
        Event event;
        if (eventList->getEvent(i, event) == kResultOk)
        {
          IMidiMsg msg;
          switch (event.type)
          {
            case Event::kNoteOnEvent:
            {
              msg.MakeNoteOnMsg(event.noteOn.pitch, event.noteOn.velocity * 127, event.sampleOffset, event.noteOn.channel);
              ProcessMidiMsg(msg);
              break;
            }

            case Event::kNoteOffEvent:
            {
              msg.MakeNoteOffMsg(event.noteOff.pitch, event.sampleOffset, event.noteOff.channel);
              ProcessMidiMsg(msg);
              break;
            }
          }
        }
      }
    }
  }

#pragma mark process single precision

  if (processSetup.symbolicSampleSize == kSample32)
  {
    if (data.numInputs)
    {
      if (HasSidechainInput())
      {
        if (getAudioInput(1)->isActive()) // Sidechain is active
        {
          mSidechainActive = true;
          SetInputChannelConnections(0, NInChannels(), true);
        }
        else
        {
          if (mSidechainActive)
          {
            ZeroScratchBuffers();
            mSidechainActive = false;
          }

          SetInputChannelConnections(0, NInChannels(), true);
          SetInputChannelConnections(data.inputs[0].numChannels, NInChannels() - NSidechainChannels(), false);
        }

        AttachInputBuffers(0, NInChannels() - NSidechainChannels(), data.inputs[0].channelBuffers32, data.numSamples);
        AttachInputBuffers(NSidechainChannels(), NInChannels() - NSidechainChannels(), data.inputs[1].channelBuffers32, data.numSamples);
      }
      else
      {
        SetInputChannelConnections(0, data.inputs[0].numChannels, true);
        SetInputChannelConnections(data.inputs[0].numChannels, NInChannels() - data.inputs[0].numChannels, false);
        AttachInputBuffers(0, NInChannels(), data.inputs[0].channelBuffers32, data.numSamples);
      }
    }

    for (int outBus = 0, chanOffset = 0; outBus < data.numOutputs; outBus++)
    {
      int busChannels = data.outputs[outBus].numChannels;
      SetOutputChannelConnections(chanOffset, busChannels, (bool) getAudioOutput(outBus)->isActive());
      SetOutputChannelConnections(chanOffset + busChannels, NOutChannels() - (chanOffset + busChannels), false);
      AttachOutputBuffers(chanOffset, busChannels, data.outputs[outBus].channelBuffers32);
      chanOffset += busChannels;
    }

    if (GetBypassed())
      PassThroughBuffers(0.0f, data.numSamples);
    else
      ProcessBuffers(0.0f, data.numSamples); // process buffers single precision
  }

#pragma mark process double precision

  else if (processSetup.symbolicSampleSize == kSample64)
  {
    if (data.numInputs)
    {
      if (HasSidechainInput())
      {
        if (getAudioInput(1)->isActive()) // Sidechain is active
        {
          mSidechainActive = true;
          SetInputChannelConnections(0, NInChannels(), true);
        }
        else
        {
          if (mSidechainActive)
          {
            ZeroScratchBuffers();
            mSidechainActive = false;
          }

          SetInputChannelConnections(0, NInChannels(), true);
          SetInputChannelConnections(data.inputs[0].numChannels, NInChannels() - NSidechainChannels(), false);
        }

        AttachInputBuffers(0, NInChannels() - NSidechainChannels(), data.inputs[0].channelBuffers64, data.numSamples);
        AttachInputBuffers(NSidechainChannels(), NInChannels() - NSidechainChannels(), data.inputs[1].channelBuffers64, data.numSamples);
      }
      else
      {
        SetInputChannelConnections(0, data.inputs[0].numChannels, true);
        SetInputChannelConnections(data.inputs[0].numChannels, NInChannels() - data.inputs[0].numChannels, false);
        AttachInputBuffers(0, NInChannels(), data.inputs[0].channelBuffers64, data.numSamples);
      }
    }

    for (int outBus = 0, chanOffset = 0; outBus < data.numOutputs; outBus++)
    {
      int busChannels = data.outputs[outBus].numChannels;
      SetOutputChannelConnections(chanOffset, busChannels, (bool) getAudioOutput(outBus)->isActive());
      SetOutputChannelConnections(chanOffset + busChannels, NOutChannels() - (chanOffset + busChannels), false);
      AttachOutputBuffers(chanOffset, busChannels, data.outputs[outBus].channelBuffers64);
      chanOffset += busChannels;
    }

    if (mBypassed)
      PassThroughBuffers(0.0, data.numSamples);
    else
      ProcessBuffers(0.0, data.numSamples); // process buffers double precision
  }

// Midi Out
//  if (mDoesMidi) {
//    IEventList eventList = data.outputEvents;
//
//    if (eventList)
//    {
//      Event event;
//
//      while (!mMidiOutputQueue.Empty()) {
//        //TODO: parse events and add
//        eventList.addEvent(event);
//      }
//    }
//  }

  return kResultOk;
}

//TODO: VST3 State needs work

tresult PLUGIN_API IPlugVST3::canProcessSampleSize(int32_t symbolicSampleSize)
{
  tresult retval = kResultFalse;

  switch (symbolicSampleSize)
  {
    case kSample32:
    case kSample64:
      retval = kResultTrue;
      break;
    default:
      retval = kResultFalse;
      break;
  }

  return retval;
}

uint32_t PLUGIN_API IPlugVST3::getLatencySamples ()
{ 
  return mLatency;
} 

#pragma mark -
#pragma mark IEditController overrides

IPlugView* PLUGIN_API IPlugVST3::createView (const char* name)
{
  if (name && strcmp (name, "editor") == 0)
  {
    IPlugVST3View* view = new IPlugVST3View(this);
    addDependentView(view);
    return view;
  }

  return 0;
}

tresult PLUGIN_API IPlugVST3::setEditorState(IBStream* state)
{
  TRACE;

  IByteChunk chunk;
  //InitChunkWithIPlugVer(&chunk); // TODO: IPlugVer should be in chunk!

  SerializeState(chunk); // to get the size

  if (chunk.Size() > 0)
  {
    state->read(chunk.GetBytes(), chunk.Size());
    UnserializeState(chunk, 0);
    
    int32_t savedBypass = 0;
    
    if (state->read (&savedBypass, sizeof (int32_t)) != kResultOk)
    {
      return kResultFalse;
    }
    
    mBypassed = (bool) savedBypass;
    
    RedrawParamControls();
    return kResultOk;
  }

  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::getEditorState(IBStream* state)
{
  TRACE;

  IByteChunk chunk;
  
// TODO: IPlugVer should be in chunk!
//  int pos;
//  GetIPlugVerFromChunk(chunk, pos)
  
  if (SerializeState(chunk))
  {
    state->write(chunk.GetBytes(), chunk.Size());
  }
  else
  {
    return kResultFalse;
  }  
  
  int32_t toSaveBypass = mBypassed ? 1 : 0;
  state->write(&toSaveBypass, sizeof (int32_t));

  return kResultOk;
}

ParamValue PLUGIN_API IPlugVST3::plainParamToNormalized(ParamID tag, ParamValue plainValue)
{
  WDL_MutexLock lock(&mParams_mutex);
  IParam* param = GetParam(tag);

  if (param)
  {
    return param->GetNormalized(plainValue);
  }

  return plainValue;
}

ParamValue PLUGIN_API IPlugVST3::getParamNormalized(ParamID tag)
{
  if (tag == kBypassParam) 
  {
    return (ParamValue) mBypassed;
  }
//   else if (tag == kPresetParam) 
//   {
//     return (ParamValue) ToNormalizedParam(mCurrentPresetIdx, 0, NPresets(), 1.);
//   }

  WDL_MutexLock lock(&mParams_mutex);
  IParam* param = GetParam(tag);

  if (param)
  {
    return param->GetNormalized();
  }

  return 0.0;
}

tresult PLUGIN_API IPlugVST3::setParamNormalized(ParamID tag, ParamValue value)
{
  WDL_MutexLock lock(&mParams_mutex);
  IParam* param = GetParam(tag);

  if (param)
  {
    param->SetNormalized(value);
    return kResultOk;
  }

  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string)
{
  WDL_MutexLock lock(&mParams_mutex);
  IParam* param = GetParam(tag);

  if (param)
  {
    WDL_String display;
    param->GetDisplayForHost(valueNormalized, true, display);
    Steinberg::UString(string, 128).fromAscii(display.Get());
    return kResultTrue;
  }

  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::getParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized)
{
  return SingleComponentEffect::getParamValueByString (tag, string, valueNormalized);
}

void IPlugVST3::addDependentView(IPlugVST3View* view)
{
  mViews.push_back(view);
}

void IPlugVST3::removeDependentView(IPlugVST3View* view)
{
  mViews.erase(std::remove(mViews.begin(), mViews.end(), view));
}

tresult IPlugVST3::beginEdit(ParamID tag)
{
  if (componentHandler)
    return componentHandler->beginEdit(tag);
  return kResultFalse;
}

tresult IPlugVST3::performEdit(ParamID tag, ParamValue valueNormalized)
{
  if (componentHandler)
    return componentHandler->performEdit(tag, valueNormalized);
  return kResultFalse;
}

tresult IPlugVST3::endEdit(ParamID tag)
{
  if (componentHandler)
    return componentHandler->endEdit(tag);
  return kResultFalse;
}

AudioBus* IPlugVST3::getAudioInput (int32_t index)
{
  AudioBus* bus = FCast<AudioBus> (audioInputs.at(index));
  return bus;
}

AudioBus* IPlugVST3::getAudioOutput (int32_t index)
{
  AudioBus* bus = FCast<AudioBus> (audioOutputs.at(index));
  return bus;
}

// TODO: more speaker arrs
SpeakerArrangement IPlugVST3::getSpeakerArrForChans(int32_t chans)
{
  switch (chans)
  {
    case 1:
      return SpeakerArr::kMono;
    case 2:
      return SpeakerArr::kStereo;
    case 3:
      return SpeakerArr::k30Music;
    case 4:
      return SpeakerArr::kAmbi1stOrderACN;
    case 5:
      return SpeakerArr::k50;
    case 6:
      return SpeakerArr::k51;
    default:
      return SpeakerArr::kEmpty;
      break;
  }
}

#pragma mark -
#pragma mark IUnitInfo overrides

int32_t PLUGIN_API IPlugVST3::getUnitCount()
{
  TRACE;
  
  return mParamGroups.GetSize() + 1;
}

tresult PLUGIN_API IPlugVST3::getUnitInfo(int32_t unitIndex, UnitInfo& info)
{
  TRACE;
  
  if (unitIndex == 0) 
  {
    info.id = kRootUnitId;
    info.parentUnitId = kNoParentUnitId;
    UString name(info.name, 128);
    name.fromAscii("Root Unit");
#ifdef VST3_PRESET_LIST
    info.programListId = kPresetParam;
#else
    info.programListId = kNoProgramListId;
#endif
    return kResultTrue;
  }
  else if (unitIndex > 0 && mParamGroups.GetSize()) 
  {
    info.id = unitIndex;
    info.parentUnitId = kRootUnitId;
    info.programListId = kNoProgramListId;
    
    UString name(info.name, 128);
    name.fromAscii(mParamGroups.Get(unitIndex-1));
    
    return kResultTrue;
  }

  return kResultFalse;
}

int32_t PLUGIN_API IPlugVST3::getProgramListCount()
{
#ifdef VST3_PRESET_LIST
  return (NPresets() > 0);
#else
  return 0;
#endif
}

tresult PLUGIN_API IPlugVST3::getProgramListInfo(int32_t listIndex, ProgramListInfo& info /*out*/)
{
  if (listIndex == 0)
  {
    info.id = kPresetParam;
    info.programCount = (int32_t) NPresets();
    UString name(info.name, 128);
    name.fromAscii("Factory Presets");
    return kResultTrue;
  }
  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::getProgramName(ProgramListID listId, int32_t programIndex, String128 name /*out*/)
{
  if (listId == kPresetParam)
  {
    Steinberg::UString(name, 128).fromAscii(GetPresetName(programIndex));
    return kResultTrue;
  }
  return kResultFalse;
}

#pragma mark -
#pragma mark IPlugBase overrides

void IPlugVST3::BeginInformHostOfParamChange(int idx)
{
  Trace(TRACELOC, "%d", idx);
  beginEdit(idx);
}

void IPlugVST3::InformHostOfParamChange(int idx, double normalizedValue)
{
  Trace(TRACELOC, "%d:%f", idx, normalizedValue);
  performEdit(idx, normalizedValue);
}

void IPlugVST3::EndInformHostOfParamChange(int idx)
{
  Trace(TRACELOC, "%d", idx);
  endEdit(idx);
}

void IPlugVST3::GetTimeInfo()
{
  mTimeInfo.mSamplePos = (double) mProcessContext.projectTimeSamples;
  mTimeInfo.mPPQPos = mProcessContext.projectTimeMusic;
  mTimeInfo.mTempo = mProcessContext.tempo;
  mTimeInfo.mLastBar = mProcessContext.barPositionMusic;
  mTimeInfo.mCycleStart = mProcessContext.cycleStartMusic;
  mTimeInfo.mCycleEnd = mProcessContext.cycleEndMusic;
  mTimeInfo.mNumerator = mProcessContext.timeSigNumerator;
  mTimeInfo.mDenominator = mProcessContext.timeSigDenominator;
  mTimeInfo.mTransportIsRunning = mProcessContext.state & ProcessContext::kPlaying;
  mTimeInfo.mTransportLoopEnabled = mProcessContext.state & ProcessContext::kCycleActive;
}

void IPlugVST3::ResizeGraphics(int w, int h, double scale)
{
  if (GetHasUI())
  {
    mViews.at(0)->resize(w, h); // only resize view 0?
  }
}

void IPlugVST3::SetLatency(int latency)
{
  IPlugBase::SetLatency(latency);

  FUnknownPtr<IComponentHandler>handler(componentHandler);
  handler->restartComponent(kLatencyChanged);  
}

#pragma mark -
#pragma mark IPlugVST3View
IPlugVST3View::IPlugVST3View(IPlugVST3* pPlug)
  : mPlug(pPlug)
  , mExpectingNewSize(false)
{
  if (mPlug)
    mPlug->addRef();
}

IPlugVST3View::~IPlugVST3View()
{
  if (mPlug)
  {
    mPlug->removeDependentView (this);
    mPlug->release();
  }
}

tresult PLUGIN_API IPlugVST3View::isPlatformTypeSupported(FIDString type)
{
  if(mPlug->GetHasUI()) // for no editor plugins
  {
#ifdef OS_WIN
    if (strcmp (type, kPlatformTypeHWND) == 0)
      return kResultTrue;

#elif defined OS_MAC
    if (strcmp (type, kPlatformTypeNSView) == 0)
      return kResultTrue;
    else if (strcmp (type, kPlatformTypeHIView) == 0)
      return kResultTrue;
#endif
  }

  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3View::onSize(ViewRect* newSize)
{
  TRACE;

  if (newSize)
  {
    rect = *newSize;

    if (mExpectingNewSize)
    {
      mPlug->OnWindowResize();
      mExpectingNewSize = false;
    }
  }

  return kResultTrue;
}

tresult PLUGIN_API IPlugVST3View::getSize(ViewRect* size)
{
  TRACE;

  if (mPlug->GetHasUI())
  {
    *size = ViewRect(0, 0, mPlug->GetUIWidth(), mPlug->GetUIHeight());

    return kResultTrue;
  }
  else
  {
    return kResultFalse;
  }
}

tresult PLUGIN_API IPlugVST3View::attached (void* parent, FIDString type)
{
  if (mPlug->GetHasUI())
  {
    #ifdef OS_WIN
    if (strcmp (type, kPlatformTypeHWND) == 0)
      mPlug->OpenWindow(parent);
    #elif defined OS_MAC
    if (strcmp (type, kPlatformTypeNSView) == 0)
      mPlug->OpenWindow(parent);
    else // Carbon
      return kResultFalse;
    #endif
    mPlug->OnGUIOpen();

    return kResultTrue;
  }

  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3View::removed()
{
  if (mPlug->GetHasUI())
  {
    mPlug->OnGUIClose();
    mPlug->CloseWindow();
  }

  return CPluginView::removed();
}

void IPlugVST3View::resize(int w, int h)
{
  TRACE;

  ViewRect newSize = ViewRect(0, 0, w, h);
  mExpectingNewSize = true;
  plugFrame->resizeView(this, &newSize);
}
