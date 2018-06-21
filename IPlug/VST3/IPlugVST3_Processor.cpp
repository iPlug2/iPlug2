/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

#include "IPlugVST3_Processor.h"

#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"

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

IPlugVST3Processor::IPlugVST3Processor(IPlugInstanceInfo instanceInfo, IPlugConfig c)
: IPlugAPIBase(c, kAPIVST3)
, IPlugProcessor<PLUG_SAMPLE_DST>(c, kAPIVST3)
{
  setControllerClass(instanceInfo.mOtherGUID);

  _SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
  _SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true);
  
  if (MaxNChannels(ERoute::kInput))
  {
    mLatencyDelay = new NChanDelayLine<PLUG_SAMPLE_DST>(MaxNChannels(ERoute::kInput), MaxNChannels(ERoute::kOutput));
    mLatencyDelay->SetDelayTime(GetLatency());
  }
  
  // Make sure the process context is predictably initialised in case it is used before process is called
  memset(&mProcessContext, 0, sizeof(ProcessContext));
  
  CreateTimer();
}

IPlugVST3Processor::~IPlugVST3Processor() {}

#pragma mark AudioEffect overrides

tresult PLUGIN_API IPlugVST3Processor::initialize(FUnknown* context)
{
  TRACE;
  
  tresult result = AudioEffect::initialize(context);
  
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
      
      int flags = 0; //busIdx == 0 ? flags = Steinberg::Vst::BusInfo::BusFlags::kDefaultActive : flags = 0;
      Steinberg::UString(tmpStringBuf, 128).fromAscii(pConfig->GetBusInfo(ERoute::kInput, busIdx)->mLabel.Get(), 128);
      addAudioInput(tmpStringBuf, busType, (BusTypes) busIdx > 0, flags);
    }
    
    for(auto busIdx = 0; busIdx < pConfig->NBuses(ERoute::kOutput); busIdx++)
    {
      uint64_t busType = GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kOutput, busIdx, pConfig);
      
      int flags = 0; //busIdx == 0 ? flags = Steinberg::Vst::BusInfo::BusFlags::kDefaultActive : flags = 0;
      Steinberg::UString(tmpStringBuf, 128).fromAscii(pConfig->GetBusInfo(ERoute::kOutput, busIdx)->mLabel.Get(), 128);
      addAudioOutput(tmpStringBuf, busType, (BusTypes) busIdx > 0, flags);
    }
    //    }
    
    
    if(DoesMIDI())
    {
      addEventInput(STR16("MIDI Input"), 1);
      //addEventOutput(STR16("MIDI Output"), 1);
    }
    
  
    OnHostIdentified();
  }
  
  return result;
}

//tresult PLUGIN_API IPlugVST3Processor::terminate()
//{
//  return AudioEffect::terminate();
//}

tresult PLUGIN_API IPlugVST3Processor::setBusArrangements(SpeakerArrangement* pInputBusArrangements, int32 numInBuses, SpeakerArrangement* pOutputBusArrangements, int32 numOutBuses)
{
  TRACE;
  
  // disconnect all io pins, they will be reconnected in process
  _SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), false);
  _SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), false);
  
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

tresult PLUGIN_API IPlugVST3Processor::setActive(TBool state)
{
  OnActivate((bool) state);
  return AudioEffect::setActive(state);
}

tresult PLUGIN_API IPlugVST3Processor::setupProcessing(ProcessSetup& newSetup)
{
  TRACE;
  
  if ((newSetup.symbolicSampleSize != kSample32) && (newSetup.symbolicSampleSize != kSample64)) return kResultFalse;
  
  _SetSampleRate(newSetup.sampleRate);
  _SetBypassed(false);
  IPlugProcessor::_SetBlockSize(newSetup.maxSamplesPerBlock); // TODO: should IPlugVST3Processor call SetBlockSizein construct unlike other APIs?
  OnReset();
  
  processSetup = newSetup;
  
  return kResultOk;
}

tresult PLUGIN_API IPlugVST3Processor::process(ProcessData& data)
{
  TRACE;
  
  if(data.processContext)
    memcpy(&mProcessContext, data.processContext, sizeof(ProcessContext));
  
  PrepareProcessContext();
  
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
                _SetBypassed(bypassed);
              
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
                OnParamChange(idx, kHost);
                LEAVE_PARAMS_MUTEX;
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
          _SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
        }
        else
        {
          if (mSidechainActive)
          {
            _ZeroScratchBuffers();
            mSidechainActive = false;
          }
          
          _SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
          _SetChannelConnections(ERoute::kInput, data.inputs[0].numChannels, MaxNChannels(ERoute::kInput) - NSidechainChannels(), false);
        }
        
        _AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput) - NSidechainChannels(), data.inputs[0].channelBuffers32, data.numSamples);
        _AttachBuffers(ERoute::kInput, NSidechainChannels(), MaxNChannels(ERoute::kInput) - NSidechainChannels(), data.inputs[1].channelBuffers32, data.numSamples);
      }
      else
      {
        _SetChannelConnections(ERoute::kInput, 0, data.inputs[0].numChannels, true);
        _SetChannelConnections(ERoute::kInput, data.inputs[0].numChannels, MaxNChannels(ERoute::kInput) - data.inputs[0].numChannels, false);
        _AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), data.inputs[0].channelBuffers32, data.numSamples);
      }
    }
    
    for (int outBus = 0, chanOffset = 0; outBus < data.numOutputs; outBus++)
    {
      int busChannels = data.outputs[outBus].numChannels;
      _SetChannelConnections(ERoute::kOutput, chanOffset, busChannels, (bool) getAudioOutput(outBus)->isActive());
      _SetChannelConnections(ERoute::kOutput, chanOffset + busChannels, MaxNChannels(ERoute::kOutput) - (chanOffset + busChannels), false);
      _AttachBuffers(ERoute::kOutput, chanOffset, busChannels, data.outputs[outBus].channelBuffers32, data.numSamples);
      chanOffset += busChannels;
    }
    
    if (GetBypassed())
      _PassThroughBuffers(0.0f, data.numSamples);
    else
      _ProcessBuffers(0.0f, data.numSamples); // process buffers single precision
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
          _SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
        }
        else
        {
          if (mSidechainActive)
          {
            _ZeroScratchBuffers();
            mSidechainActive = false;
          }
          
          _SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
          _SetChannelConnections(ERoute::kInput, data.inputs[0].numChannels, MaxNChannels(ERoute::kInput) - NSidechainChannels(), false);
        }
        
        _AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput) - NSidechainChannels(), data.inputs[0].channelBuffers64, data.numSamples);
        _AttachBuffers(ERoute::kInput, NSidechainChannels(), MaxNChannels(ERoute::kInput) - NSidechainChannels(), data.inputs[1].channelBuffers64, data.numSamples);
      }
      else
      {
        _SetChannelConnections(ERoute::kInput, 0, data.inputs[0].numChannels, true);
        _SetChannelConnections(ERoute::kInput, data.inputs[0].numChannels, MaxNChannels(ERoute::kInput) - data.inputs[0].numChannels, false);
        _AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), data.inputs[0].channelBuffers64, data.numSamples);
      }
    }
    
    for (int outBus = 0, chanOffset = 0; outBus < data.numOutputs; outBus++)
    {
      int busChannels = data.outputs[outBus].numChannels;
      _SetChannelConnections(ERoute::kOutput, chanOffset, busChannels, (bool) getAudioOutput(outBus)->isActive());
      _SetChannelConnections(ERoute::kOutput, chanOffset + busChannels, MaxNChannels(ERoute::kOutput) - (chanOffset + busChannels), false);
      _AttachBuffers(ERoute::kOutput, chanOffset, busChannels, data.outputs[outBus].channelBuffers64, data.numSamples);
      chanOffset += busChannels;
    }
    
    if (GetBypassed())
      _PassThroughBuffers(0.0, data.numSamples);
    else
      _ProcessBuffers(0.0, data.numSamples); // process buffers double precision
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

tresult PLUGIN_API IPlugVST3Processor::canProcessSampleSize(int32 symbolicSampleSize)
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

tresult PLUGIN_API IPlugVST3Processor::setState(IBStream* state)
{
  TRACE;
  IByteChunk chunk;
  int chunkSize = 0;
  state->read(&chunkSize, sizeof(int));
  chunk.Resize(chunkSize);
  state->read(chunk.GetBytes(), chunk.Size());
  
  int readPos = 0;
  IByteChunk::GetIPlugVerFromChunk(chunk, readPos);
  int bypassState;
  readPos = chunk.Get(&bypassState, readPos);
  //TODO: do something with bypass state
  UnserializeState(chunk, readPos);
  
  return kResultTrue;
}

tresult PLUGIN_API IPlugVST3Processor::getState(IBStream* state)
{
  TRACE;
  IByteChunk chunk;
  IByteChunk::InitChunkWithIPlugVer(chunk);
  int bypassState = GetBypassed();
  chunk.Put(&bypassState);

  if (SerializeState(chunk))
  {
    int chunkSize = chunk.Size();
    void* data = (void*) &chunkSize;
    state->write(data, (int32) sizeof(int));
    state->write(chunk.GetBytes(), chunkSize);
  }
  else
    return kResultFalse;
  
  return kResultTrue;
}


void IPlugVST3Processor::PrepareProcessContext()
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
  _SetTimeInfo(timeInfo);
  _SetRenderingOffline(offline);
}

void IPlugVST3Processor::SendControlValueFromDelegate(int controlTag, double normalizedValue)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  message->setMessageID("SCVFD");
  message->getAttributes()->setInt("CT", controlTag);
  message->getAttributes()->setFloat("NV", normalizedValue);
  
  sendMessage(message);
}

void IPlugVST3Processor::SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, const void* pData)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  message->setMessageID("SCMFD");
  message->getAttributes()->setInt("CT", controlTag);
  message->getAttributes()->setInt("MT", messageTag);
  message->getAttributes()->setBinary("D", pData, dataSize);
  
  sendMessage(message);
}

void IPlugVST3Processor::SendArbitraryMsgFromDelegate(int messageTag, int dataSize, const void* pData)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  if(dataSize == 0) // allow sending messages with no data
  {
    dataSize = 1;
    uint8_t dummy = 0;
    pData = &dummy;
  }
  
  message->setMessageID("SAMFD");
  message->getAttributes()->setInt("MT", messageTag);
  message->getAttributes()->setBinary("D", pData, dataSize);
  sendMessage(message);
}

tresult PLUGIN_API IPlugVST3Processor::notify(IMessage* message)
{
  if (!message)
    return kInvalidArgument;
  
  const void* data = nullptr;
  uint32 size;
  
  if (!strcmp (message->getMessageID(), "SMMFUI")) // midi message from UI
  {
    if (message->getAttributes()->getBinary("D", data, size) == kResultOk)
    {
      if (size == sizeof(IMidiMsg))
      {
        IMidiMsg msg;
        memcpy(&msg, data, size);
        mMidiMsgsFromEditor.Push(msg);
        return kResultOk;
      }
      
      return kResultFalse;
    }
  }
  else if (!strcmp (message->getMessageID(), "SAMFUI")) // message from UI
  {
    int64 val;
    
    if (message->getAttributes()->getInt("MT", val) == kResultOk)
    {
      if (message->getAttributes()->getBinary("D", data, size) == kResultOk)
      {
        if(OnMessage((int) val, size, data))
        {
          return kResultOk;
        }
      }
      
      return kResultFalse;
    }
  }
  
  return AudioEffect::notify(message);
}

void IPlugVST3Processor::_TransmitMidiMsgFromProcessor(const IMidiMsg& msg)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  message->setMessageID("SMMFD");
  message->getAttributes()->setBinary("D", (void*) &msg, sizeof(IMidiMsg));
  sendMessage(message);
}
