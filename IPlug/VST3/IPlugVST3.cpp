/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include <cstdio>

#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"

#include "IPlugVST3.h"

using namespace Steinberg;
using namespace Vst;

#include "IPlugVST3_Parameter.h"

#ifndef CUSTOM_BUSTYPE_FUNC
static uint64_t GetAPIBusTypeForChannelIOConfig(int configIdx, ERoute dir, int busIdx, IOConfig* pConfig)
{
  assert(pConfig != nullptr);
  assert(busIdx >= 0 && busIdx < pConfig->NBuses(dir));

  int numChans = pConfig->GetBusInfo(dir, busIdx)->mNChans;

  switch (numChans)
  {
    case 0: return SpeakerArr::kEmpty;
    case 1: return SpeakerArr::kMono;
    case 2: return SpeakerArr::kStereo;
    case 3: return SpeakerArr::k30Cine; // CHECK - not the same as protools
    case 4: return SpeakerArr::kAmbi1stOrderACN;
    case 5: return SpeakerArr::k50;
    case 6: return SpeakerArr::k51;
    case 7: return SpeakerArr::k70Cine;
    case 8: return SpeakerArr::k71CineSideFill; // CHECK - not the same as protools
    case 9: return SpeakerArr::kAmbi2cdOrderACN;
    case 10:return SpeakerArr::k71_2; // aka k91Atmos
    case 16:return SpeakerArr::kAmbi3rdOrderACN;
    default:
      DBGMSG("do not yet know what to do here\n");
      assert(0);
      return SpeakerArr::kEmpty;
  }
}
#else
extern uint64_t GetAPIBusTypeForChannelIOConfig(int configIdx, ERoutingDir dir, int busIdx, IOConfig* pConfig);
#endif //CUSTOM_BUSTYPE_FUNC


#pragma mark - IPlugVST3 Constructor

IPlugVST3::IPlugVST3(IPlugInstanceInfo instanceInfo, IPlugConfig c)
: IPlugAPIBase(c, kAPIVST3)
, IPlugProcessor<PLUG_SAMPLE_DST>(c, kAPIVST3)
{
  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
  SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true);

  if (MaxNChannels(ERoute::kInput))
  {
    mLatencyDelay = new NChanDelayLine<PLUG_SAMPLE_DST>(MaxNChannels(ERoute::kInput), MaxNChannels(ERoute::kOutput));
    mLatencyDelay->SetDelayTime(GetLatency());
  }

  // Make sure the process context is predictably initialised in case it is used before process is called

  memset(&mProcessContext, 0, sizeof(ProcessContext));
  
  CreateTimer();
}

IPlugVST3::~IPlugVST3() {}

#pragma mark AudioEffect overrides

tresult PLUGIN_API IPlugVST3::initialize(FUnknown* context)
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
//    for(auto configIdx = 0; configIdx < NIOConfigs(); configIdx++)
//    {
      int configIdx = NIOConfigs()-1;

      IOConfig* pConfig = GetIOConfig(configIdx);

      assert(pConfig);
      for(auto busIdx = 0; busIdx < pConfig->NBuses(ERoute::kInput); busIdx++)
      {
        uint64_t busType = GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kInput, busIdx, pConfig);

        int flags = 0; busIdx == 0 ? flags = Steinberg::Vst::BusInfo::BusFlags::kDefaultActive : flags = 0;
        Steinberg::UString(tmpStringBuf, 128).fromAscii(pConfig->GetBusInfo(ERoute::kInput, busIdx)->mLabel.Get(), 128);
        addAudioInput(tmpStringBuf, busType, (BusTypes) busIdx > 0, flags);
      }

      for(auto busIdx = 0; busIdx < pConfig->NBuses(ERoute::kOutput); busIdx++)
      {
        uint64_t busType = GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kOutput, busIdx, pConfig);

        int flags = 0; busIdx == 0 ? flags = Steinberg::Vst::BusInfo::BusFlags::kDefaultActive : flags = 0;
        Steinberg::UString(tmpStringBuf, 128).fromAscii(pConfig->GetBusInfo(ERoute::kOutput, busIdx)->mLabel.Get(), 128);
        addAudioOutput(tmpStringBuf, busType, (BusTypes) busIdx > 0, flags);
      }
//    }


    if(DoesMIDIIn())
      addEventInput(STR16("MIDI Input"), 1);
    
    if(DoesMIDIOut())
      addEventOutput(STR16("MIDI Output"), 1);

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

      const char* paramGroupName = p->GetGroupForHost();

      if (CStringHasContents(paramGroupName))
      {
        for(int j = 0; j < NParamGroups(); j++)
        {
          if(strcmp(paramGroupName, GetParamGroupName(j)) == 0)
          {
            unitID = j+1;
          }
        }

        if (unitID == kRootUnitId) // new unit, nothing found, so add it
        {
          unitID = AddParamGroup(paramGroupName);
        }
      }

      Parameter* pVST3Parameter = new IPlugVST3Parameter(p, i, unitID);
      parameters.addParameter(pVST3Parameter);
    }
  }

  OnHostIdentified();
  RestorePreset(0);

  return result;
}

tresult PLUGIN_API IPlugVST3::terminate()
{
  TRACE;

  mViews.empty();
  return SingleComponentEffect::terminate();
}

tresult PLUGIN_API IPlugVST3::setBusArrangements(SpeakerArrangement* pInputBusArrangements, int32 numInBuses, SpeakerArrangement* pOutputBusArrangements, int32 numOutBuses)
{
  TRACE;

  // disconnect all io pins, they will be reconnected in process
  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), false);
  SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), false);

  //TODO: setBusArrangements !!!
  //const int maxNInputChans = MaxNBuses(ERoute::kInput);
  //const int NInputChannelCountOnBuses[maxNInputChans];
  //memset(NInputChannelCountOnBuses, 0, MaxNBuses(ERoute::kInput) * sizeof(int));

  //const int maxNOutputChans = MaxNBuses(ERoute::kOutput);
  //const int NOutputChannelCountOnBuses[maxNOutputChans];
  //memset(NOutputChannelCountOnBuses, 0, MaxNBuses(ERoute::kOutput) * sizeof(int));
  //
//  for(auto busIdx = 0; busIdx < numIns; busIdx++)
//  {
//    AudioBus* pBus = FCast<AudioBus>(audioInputs.at(busIdx));
//    const int NInputsRequired = SpeakerArr::getChannelCount(inputs[busIdx]);
//    // if existing input bus has a different number of channels to the input bus being connected
//    if (pBus && SpeakerArr::getChannelCount(pBus->getArrangement()) != NInputsRequired)
//    {
//      int flags = 0;
//      busIdx == 0 ? flags = Steinberg::Vst::BusInfo::BusFlags::kDefaultActive : flags = 0;
//      audioInputs.erase(std::remove(audioInputs.begin(), audioInputs.end(), pBus));
//      addAudioInput(USTRING("Input"), (SpeakerArrangement) GetAPIBusTypeForChannelIOConfig(-1, -1, NInputsRequired), (BusTypes) busIdx > 0, flags);
//
//    }
//  }
//
//  for(auto busIdx = 0; busIdx < numOuts; busIdx++)
//  {
//    AudioBus* pBus = FCast<AudioBus>(audioOutputs.at(busIdx));
//    const int NOutputsRequired = SpeakerArr::getChannelCount(outputs[busIdx]);
//    // if existing input bus has a different number of channels to the input bus being connected
//    if (pBus && SpeakerArr::getChannelCount(pBus->getArrangement()) != NOutputsRequired)
//    {
//      int flags = 0;
//      busIdx == 0 ? flags = Steinberg::Vst::BusInfo::BusFlags::kDefaultActive : flags = 0;
//      audioOutputs.erase(std::remove(audioOutputs.begin(), audioOutputs.end(), pBus));
//      addAudioOutput(USTRING("Output"), (SpeakerArrangement) GetAPIBusTypeForChannelIOConfig(-1, -1, NOutputsRequired), (BusTypes) busIdx > 0, flags);
//    }
//  }

  return kResultTrue;
}

tresult PLUGIN_API IPlugVST3::setActive(TBool state)
{
  TRACE;

  OnActivate((bool) state);

  return SingleComponentEffect::setActive(state);
}

tresult PLUGIN_API IPlugVST3::setupProcessing(ProcessSetup& newSetup)
{
  TRACE;

  if ((newSetup.symbolicSampleSize != kSample32) && (newSetup.symbolicSampleSize != kSample64)) return kResultFalse;

  SetSampleRate(newSetup.sampleRate);
  SetBypassed(false);
  IPlugProcessor::SetBlockSize(newSetup.maxSamplesPerBlock); // TODO: should IPlugVST3 call SetBlockSizein construct unlike other APIs?
  mMidiOutputQueue.Resize(newSetup.maxSamplesPerBlock);
  OnReset();

  processSetup = newSetup;

  return kResultOk;
}

tresult PLUGIN_API IPlugVST3::process(ProcessData& data)
{
  TRACE;

  if(data.processContext)
    memcpy(&mProcessContext, data.processContext, sizeof(ProcessContext));

  PreProcess();

  //process parameters
  IParameterChanges* paramChanges = data.inputParameterChanges;
  if (paramChanges)
  {
    int32 numParamsChanged = paramChanges->getParameterCount();

    //it is possible to get a finer resolution of control here by retrieving more values (points) from the queue
    //for now we just grab the last one

    for (int32 i = 0; i < numParamsChanged; i++)
    {
      IParamValueQueue* paramQueue = paramChanges->getParameterData(i);
      if (paramQueue)
      {
        int32 numPoints = paramQueue->getPointCount();
        int32 offsetSamples;
        double value;

        if (paramQueue->getPoint(numPoints - 1,  offsetSamples, value) == kResultTrue)
        {
          int idx = paramQueue->getParameterId();

          switch (idx)
          {
            case kBypassParam:
            {
             const bool bypassed = (value > 0.5);

              if (bypassed != GetBypassed())
                SetBypassed(bypassed);

              break;
            }
            case kPresetParam:
              //RestorePreset((int)round(FromNormalizedParam(value, 0, NPresets(), 1.))); // TODO
              break;
              //TODO: pitch bend, modwheel etc
            default:
              {
                if (idx >= 0 && idx < NParams())
                {
                  ENTER_PARAMS_MUTEX;
                  GetParam(idx)->SetNormalized((double)value);
                  SendParameterValueFromAPI(idx, (double) value, true);
                  OnParamChange(idx, kHost, offsetSamples);
                  LEAVE_PARAMS_MUTEX;
                }
              }
              break;
          }

        }
      }
    }
  }

  if(DoesMIDIIn())
  {
    IMidiMsg msg;

    //process events.. only midi note on and note off?
    IEventList* eventList = data.inputEvents;
    if (eventList)
    {
      int32 numEvent = eventList->getEventCount();
      for (int32 i=0; i<numEvent; i++)
      {
        Event event;
        if (eventList->getEvent(i, event) == kResultOk)
        {
          switch (event.type)
          {
            case Event::kNoteOnEvent:
            {
              msg.MakeNoteOnMsg(event.noteOn.pitch, event.noteOn.velocity * 127, event.sampleOffset, event.noteOn.channel);
              ProcessMidiMsg(msg);
              mMidiMsgsFromProcessor.Push(msg);
              break;
            }
            case Event::kNoteOffEvent:
            {
              msg.MakeNoteOffMsg(event.noteOff.pitch, event.sampleOffset, event.noteOff.channel);
              ProcessMidiMsg(msg);
              mMidiMsgsFromProcessor.Push(msg);
              break;
            }
            case Event::kPolyPressureEvent:
            {
              msg.MakePolyATMsg(event.polyPressure.pitch, event.polyPressure.pressure * 127., event.sampleOffset, event.polyPressure.channel);
              ProcessMidiMsg(msg);
              mMidiMsgsFromProcessor.Push(msg);
              break;
            }
            case Event::kDataEvent:
            {
              ISysEx syx = ISysEx(event.sampleOffset, event.data.bytes, event.data.size);
              ProcessSysEx(syx);
              //mSysexMsgsFromProcessor.Push
              break;
            }
          }
        }
      }
    }
    
    while (mMidiMsgsFromEditor.Pop(msg))
    {
      ProcessMidiMsg(msg);
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
          SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
        }
        else
        {
          if (mSidechainActive)
          {
            ZeroScratchBuffers();
            mSidechainActive = false;
          }

          SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
          SetChannelConnections(ERoute::kInput, data.inputs[0].numChannels, MaxNChannels(ERoute::kInput) - NSidechainChannels(), false);
        }

        AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput) - NSidechainChannels(), data.inputs[0].channelBuffers32, data.numSamples);
        AttachBuffers(ERoute::kInput, NSidechainChannels(), MaxNChannels(ERoute::kInput) - NSidechainChannels(), data.inputs[1].channelBuffers32, data.numSamples);
      }
      else
      {
        SetChannelConnections(ERoute::kInput, 0, data.inputs[0].numChannels, true);
        SetChannelConnections(ERoute::kInput, data.inputs[0].numChannels, MaxNChannels(ERoute::kInput) - data.inputs[0].numChannels, false);
        AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), data.inputs[0].channelBuffers32, data.numSamples);
      }
    }

    for (int outBus = 0, chanOffset = 0; outBus < data.numOutputs; outBus++)
    {
      int busChannels = data.outputs[outBus].numChannels;
      SetChannelConnections(ERoute::kOutput, chanOffset, busChannels, (bool) getAudioOutput(outBus)->isActive());
      SetChannelConnections(ERoute::kOutput, chanOffset + busChannels, MaxNChannels(ERoute::kOutput) - (chanOffset + busChannels), false);
      AttachBuffers(ERoute::kOutput, chanOffset, busChannels, data.outputs[outBus].channelBuffers32, data.numSamples);
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
          SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
        }
        else
        {
          if (mSidechainActive)
          {
            ZeroScratchBuffers();
            mSidechainActive = false;
          }

          SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
          SetChannelConnections(ERoute::kInput, data.inputs[0].numChannels, MaxNChannels(ERoute::kInput) - NSidechainChannels(), false);
        }

        AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput) - NSidechainChannels(), data.inputs[0].channelBuffers64, data.numSamples);
        AttachBuffers(ERoute::kInput, NSidechainChannels(), MaxNChannels(ERoute::kInput) - NSidechainChannels(), data.inputs[1].channelBuffers64, data.numSamples);
      }
      else
      {
        SetChannelConnections(ERoute::kInput, 0, data.inputs[0].numChannels, true);
        SetChannelConnections(ERoute::kInput, data.inputs[0].numChannels, MaxNChannels(ERoute::kInput) - data.inputs[0].numChannels, false);
        AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), data.inputs[0].channelBuffers64, data.numSamples);
      }
    }

    for (int outBus = 0, chanOffset = 0; outBus < data.numOutputs; outBus++)
    {
      int busChannels = data.outputs[outBus].numChannels;
      SetChannelConnections(ERoute::kOutput, chanOffset, busChannels, (bool) getAudioOutput(outBus)->isActive());
      SetChannelConnections(ERoute::kOutput, chanOffset + busChannels, MaxNChannels(ERoute::kOutput) - (chanOffset + busChannels), false);
      AttachBuffers(ERoute::kOutput, chanOffset, busChannels, data.outputs[outBus].channelBuffers64, data.numSamples);
      chanOffset += busChannels;
    }

    if (GetBypassed())
      PassThroughBuffers(0.0, data.numSamples);
    else
      ProcessBuffers(0.0, data.numSamples); // process buffers double precision
  }

  if (DoesMIDIOut())
  {
    IEventList* outputEvents = data.outputEvents;
    
    //MIDI
    if (!mMidiOutputQueue.Empty() && outputEvents)
    {
      Event toAdd = {0};
      IMidiMsg msg;
      
      while (!mMidiOutputQueue.Empty())
      {
        IMidiMsg& msg = mMidiOutputQueue.Peek();
        
        if (msg.StatusMsg() == IMidiMsg::kNoteOn)
        {
          toAdd.type = Event::kNoteOnEvent;
          toAdd.noteOn.channel = msg.Channel();
          toAdd.noteOn.pitch = msg.NoteNumber();
          toAdd.noteOn.tuning = 0.;
          toAdd.noteOn.velocity = (float) msg.Velocity() * (1.f / 127.f);
          toAdd.noteOn.length = -1;
          toAdd.noteOn.noteId = -1; // TODO ?
          toAdd.sampleOffset = msg.mOffset;
          outputEvents->addEvent(toAdd);
        }
        else if (msg.StatusMsg() == IMidiMsg::kNoteOff)
        {
          toAdd.type = Event::kNoteOffEvent;
          toAdd.noteOff.channel = msg.Channel();
          toAdd.noteOff.pitch = msg.NoteNumber();
          toAdd.noteOff.velocity = (float) msg.Velocity() * (1.f / 127.f);
          toAdd.noteOff.noteId = -1; // TODO ?
          toAdd.sampleOffset = msg.mOffset;
          outputEvents->addEvent(toAdd);
        }
        else if (msg.StatusMsg() == IMidiMsg::kPolyAftertouch)
        {
          toAdd.type = Event::kPolyPressureEvent;
          toAdd.polyPressure.channel = msg.Channel();
          toAdd.polyPressure.pitch = msg.NoteNumber();
          toAdd.polyPressure.pressure = (float) msg.PolyAfterTouch() * (1.f / 127.f);
          toAdd.polyPressure.noteId = -1; // TODO ?
          toAdd.sampleOffset = msg.mOffset;
          outputEvents->addEvent(toAdd);
        }
        
        mMidiOutputQueue.Remove();
        // don't add any midi messages other than noteon/noteoff
      }
    }
    
    mMidiOutputQueue.Flush(data.numSamples);
    
    //Output SYSEX from the editor, which has bypassed the processors' ProcessSysEx()
    if(mSysExDataFromEditor.ElementsAvailable())
    {
      Event toAdd = {0};
      
      while (mSysExDataFromEditor.Pop(mSysexBuf))
      {
        toAdd.type = Event::kDataEvent;
        toAdd.sampleOffset = mSysexBuf.mOffset;
        toAdd.data.type = DataEvent::kMidiSysEx;
        toAdd.data.size = mSysexBuf.mSize;
        toAdd.data.bytes = (uint8*) mSysexBuf.mData; // TODO!  this is a problem if more than one message in this block!
        outputEvents->addEvent(toAdd);
      }
    }
  }

  return kResultOk;
}

//TODO: VST3 State needs work

tresult PLUGIN_API IPlugVST3::canProcessSampleSize(int32 symbolicSampleSize)
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

#pragma mark IEditController overrides

IPlugView* PLUGIN_API IPlugVST3::createView(const char* name)
{
  if (name && strcmp(name, "editor") == 0)
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
  
  const int bytesPerBlock = 128;
  char buffer[bytesPerBlock];
  
  while(true)
  {
    Steinberg::int32 bytesRead = 0;
    auto status = state->read (buffer, (Steinberg::int32) bytesPerBlock, &bytesRead);
    
    if (bytesRead <= 0 || (status != kResultTrue && GetHost() != kHostWaveLab))
      break;
    
    chunk.PutBytes(buffer, bytesRead);
  }
  int pos = UnserializeState(chunk,0);
  
  int32 savedBypass = 0;
  
  state->seek(pos,IBStream::IStreamSeekMode::kIBSeekSet);
  if (state->read (&savedBypass, sizeof (Steinberg::int32)) != kResultOk) {
    return kResultFalse;
  }
  
  SetBypassed((bool) savedBypass);
  
  OnRestoreState();
  return kResultOk;
}

tresult PLUGIN_API IPlugVST3::getEditorState(IBStream* state)
{
  TRACE;

  IByteChunk chunk;

// TODO: IPlugVer should be in chunk!
//  int pos;
//  IByteChunk::GetIPlugVerFromChunk(chunk, pos)

  if (SerializeState(chunk))
  {
    state->write(chunk.GetData(), chunk.Size());
  }
  else
  {
    return kResultFalse;
  }

  int32 toSaveBypass = GetBypassed() ? 1 : 0;
  state->write(&toSaveBypass, sizeof (int32));

  return kResultOk;
}

tresult PLUGIN_API IPlugVST3::setComponentState(IBStream* state)
{
  return setEditorState(state);
}

tresult PLUGIN_API IPlugVST3::setState(IBStream* state)
{
  return setEditorState(state);
}

tresult PLUGIN_API IPlugVST3::getState(IBStream* state)
{
  return getEditorState(state);
}

ParamValue PLUGIN_API IPlugVST3::plainParamToNormalized(ParamID tag, ParamValue plainValue)
{
  ENTER_PARAMS_MUTEX;
  IParam* pParam = GetParam(tag);
  if (pParam)
    plainValue = pParam->ToNormalized(plainValue);
  LEAVE_PARAMS_MUTEX;

  return plainValue;
}

ParamValue PLUGIN_API IPlugVST3::getParamNormalized(ParamID tag)
{
  ParamValue returnVal = 0.;
  if (tag == kBypassParam)
    returnVal = (ParamValue) GetBypassed();
//   else if (tag == kPresetParam)
//   {
//     return (ParamValue) ToNormalizedParam(mCurrentPresetIdx, 0, NPresets(), 1.);
//   }
  else
  {
    ENTER_PARAMS_MUTEX;
    IParam* pParam = GetParam(tag);
    if (pParam)
      returnVal = pParam->GetNormalized();
    LEAVE_PARAMS_MUTEX;
  }

  return returnVal;
}

tresult PLUGIN_API IPlugVST3::setParamNormalized(ParamID tag, ParamValue value)
{
  tresult result = kResultFalse;

  ENTER_PARAMS_MUTEX;
  IParam* pParam = GetParam(tag);
  if (pParam)
  {
    pParam->SetNormalized(value);
    result = kResultOk;
  }
  LEAVE_PARAMS_MUTEX;

  return result;
}

tresult PLUGIN_API IPlugVST3::getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string)
{
  tresult result = kResultFalse;
  ENTER_PARAMS_MUTEX;
  IParam* pParam = GetParam(tag);
  if (pParam)
  {
    pParam->GetDisplayForHost(valueNormalized, true, mParamDisplayStr);
    Steinberg::UString(string, 128).fromAscii(mParamDisplayStr.Get());
    result = kResultTrue;
  }
  LEAVE_PARAMS_MUTEX;

  return result;
}

tresult PLUGIN_API IPlugVST3::getParamValueByString(ParamID tag, TChar* string, ParamValue& valueNormalized)
{
  return SingleComponentEffect::getParamValueByString(tag, string, valueNormalized);
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

AudioBus* IPlugVST3::getAudioInput (int32 index)
{
  AudioBus* bus = FCast<AudioBus>(audioInputs.at(index));
  return bus;
}

AudioBus* IPlugVST3::getAudioOutput (int32 index)
{
  AudioBus* bus = FCast<AudioBus>(audioOutputs.at(index));
  return bus;
}

#pragma mark IUnitInfo overrides

int32 PLUGIN_API IPlugVST3::getUnitCount()
{
  TRACE;

  return NParamGroups() + 1;
}

tresult PLUGIN_API IPlugVST3::getUnitInfo(int32 unitIndex, UnitInfo& info)
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
  else if (unitIndex > 0 && NParamGroups())
  {
    info.id = unitIndex;
    info.parentUnitId = kRootUnitId;
    info.programListId = kNoProgramListId;

    UString name(info.name, 128);
    name.fromAscii(GetParamGroupName(unitIndex-1));

    return kResultTrue;
  }

  return kResultFalse;
}

int32 PLUGIN_API IPlugVST3::getProgramListCount()
{
#ifdef VST3_PRESET_LIST
  return (NPresets() > 0);
#else
  return 0;
#endif
}

tresult PLUGIN_API IPlugVST3::getProgramListInfo(int32 listIndex, ProgramListInfo& info /*out*/)
{
  if (listIndex == 0)
  {
    info.id = kPresetParam;
    info.programCount = (int32) NPresets();
    UString name(info.name, 128);
    name.fromAscii("Factory Presets");
    return kResultTrue;
  }
  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::getProgramName(ProgramListID listId, int32 programIndex, String128 name /*out*/)
{
  if (listId == kPresetParam)
  {
    Steinberg::UString(name, 128).fromAscii(GetPresetName(programIndex));
    return kResultTrue;
  }
  return kResultFalse;
}

#pragma mark IPlugAPIBase overrides

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

void IPlugVST3::InformHostOfParameterDetailsChange()
{
  FUnknownPtr<IComponentHandler>handler(componentHandler);
  handler->restartComponent(kParamTitlesChanged);
}

void IPlugVST3::EditorPropertiesChangedFromDelegate(int viewWidth, int viewHeight, const IByteChunk& data)
{
  if (HasUI() && (viewWidth != GetEditorWidth() || viewHeight != GetEditorHeight()))
  {
    mViews.at(0)->resize(viewWidth, viewHeight);
    IPlugAPIBase::EditorPropertiesChangedFromDelegate(viewWidth, viewHeight, data);
  }
}

void IPlugVST3::DirtyParametersFromUI()
{
  startGroupEdit();
  
  IPlugAPIBase::DirtyParametersFromUI();
  
  finishGroupEdit();
}

void IPlugVST3::SetLatency(int latency)
{
  IPlugProcessor::SetLatency(latency);

  FUnknownPtr<IComponentHandler>handler(componentHandler);
  handler->restartComponent(kLatencyChanged);
}

#pragma mark IPlugVST3
void IPlugVST3::PreProcess()
{
  ITimeInfo timeInfo;

  if(mProcessContext.state & ProcessContext::kProjectTimeMusicValid)
    timeInfo.mSamplePos = (double) mProcessContext.projectTimeSamples;
  timeInfo.mPPQPos = mProcessContext.projectTimeMusic;
  timeInfo.mTempo = mProcessContext.tempo;
  timeInfo.mLastBar = mProcessContext.barPositionMusic;
  timeInfo.mCycleStart = mProcessContext.cycleStartMusic;
  timeInfo.mCycleEnd = mProcessContext.cycleEndMusic;
  timeInfo.mNumerator = mProcessContext.timeSigNumerator;
  timeInfo.mDenominator = mProcessContext.timeSigDenominator;
  timeInfo.mTransportIsRunning = mProcessContext.state & ProcessContext::kPlaying;
  timeInfo.mTransportLoopEnabled = mProcessContext.state & ProcessContext::kCycleActive;
  const bool offline = processSetup.processMode == Steinberg::Vst::kOffline;
  SetTimeInfo(timeInfo);
  SetRenderingOffline(offline);
}

bool IPlugVST3::SendMidiMsg(const IMidiMsg& msg)
{
  mMidiOutputQueue.Add(msg);
  return true;
}

#pragma mark - IPlugVST3View
IPlugVST3View::IPlugVST3View(IPlugVST3* pPlug)
  : mPlug(pPlug)
{
  if (mPlug)
    mPlug->addRef();
}

IPlugVST3View::~IPlugVST3View()
{
  if (mPlug)
  {
    mPlug->removeDependentView(this);
    mPlug->release();
  }
}

tresult PLUGIN_API IPlugVST3View::isPlatformTypeSupported(FIDString type)
{
  if(mPlug->HasUI()) // for no editor plugins
  {
#ifdef OS_WIN
    if (strcmp(type, kPlatformTypeHWND) == 0)
      return kResultTrue;

#elif defined OS_MAC
    if (strcmp (type, kPlatformTypeNSView) == 0)
      return kResultTrue;
#endif
  }

  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3View::onSize(ViewRect* newSize)
{
  TRACE;

  if (newSize)
    rect = *newSize;

  return kResultTrue;
}

tresult PLUGIN_API IPlugVST3View::getSize(ViewRect* size)
{
  TRACE;

  if (mPlug->HasUI())
  {
    *size = ViewRect(0, 0, mPlug->GetEditorWidth(), mPlug->GetEditorHeight());

    return kResultTrue;
  }
  else
  {
    return kResultFalse;
  }
}

tresult PLUGIN_API IPlugVST3View::attached(void* parent, FIDString type)
{
  if (mPlug->HasUI())
  {
    #ifdef OS_WIN
    if (strcmp(type, kPlatformTypeHWND) == 0)
      mPlug->OpenWindow(parent);
    #elif defined OS_MAC
    if (strcmp (type, kPlatformTypeNSView) == 0)
      mPlug->OpenWindow(parent);
    else // Carbon
      return kResultFalse;
    #endif
    mPlug->OnUIOpen();

    return kResultTrue;
  }

  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3View::removed()
{
  if (mPlug->HasUI())
    mPlug->CloseWindow();

  return CPluginView::removed();
}

void IPlugVST3View::resize(int w, int h)
{
  TRACE;

  ViewRect newSize = ViewRect(0, 0, w, h);
  plugFrame->resizeView(this, &newSize);
}
