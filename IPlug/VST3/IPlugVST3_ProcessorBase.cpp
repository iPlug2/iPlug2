/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"

#include "IPlugVST3_ProcessorBase.h"

using namespace iplug;
using namespace Steinberg;
using namespace Vst;

#ifndef CUSTOM_BUSTYPE_FUNC
uint64_t iplug::GetAPIBusTypeForChannelIOConfig(int configIdx, ERoute dir, int busIdx, IOConfig* pConfig)
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
#endif

IPlugVST3ProcessorBase::IPlugVST3ProcessorBase(Config c, IPlugAPIBase& plug)
: IPlugProcessor(c, kAPIVST3)
, mPlug(plug)
{
  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
  SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true);
  
  if (MaxNChannels(ERoute::kInput))
  {
    mLatencyDelay = std::unique_ptr<NChanDelayLine<PLUG_SAMPLE_DST>>(new NChanDelayLine<PLUG_SAMPLE_DST>(MaxNChannels(ERoute::kInput), MaxNChannels(ERoute::kOutput)));
    mLatencyDelay->SetDelayTime(GetLatency());
  }
  
  // Make sure the process context is predictably initialised in case it is used before process is called
  memset(&mProcessContext, 0, sizeof(ProcessContext));
}

void IPlugVST3ProcessorBase::ProcessMidiIn(IEventList* eventList, IPlugQueue<IMidiMsg>& editorQueue, IPlugQueue<IMidiMsg>& processorQueue)
{
  IMidiMsg msg;
  
  // Process events.. only midi note on and note off?
  
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
            processorQueue.Push(msg);
            break;
          }
            
          case Event::kNoteOffEvent:
          {
            msg.MakeNoteOffMsg(event.noteOff.pitch, event.sampleOffset, event.noteOff.channel);
            ProcessMidiMsg(msg);
            processorQueue.Push(msg);
            break;
          }
          case Event::kPolyPressureEvent:
          {
            msg.MakePolyATMsg(event.polyPressure.pitch, event.polyPressure.pressure * 127., event.sampleOffset, event.polyPressure.channel);
            ProcessMidiMsg(msg);
            processorQueue.Push(msg);
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
  
  while (editorQueue.Pop(msg))
  {
    ProcessMidiMsg(msg);
  }
}

void IPlugVST3ProcessorBase::ProcessMidiOut(IPlugQueue<SysExData>& sysExQueue, SysExData& sysExBuf, IEventList* outputEvents, int32 numSamples)
{
  // MIDI
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
      else if (msg.StatusMsg() == IMidiMsg::kControlChange)
      {
        toAdd.type = Event::kLegacyMIDICCOutEvent;
        toAdd.midiCCOut.channel = msg.Channel();
        toAdd.midiCCOut.controlNumber = msg.mData1;
        toAdd.midiCCOut.value = msg.mData2;
        toAdd.midiCCOut.value2 = 0;
      }
      else if (msg.StatusMsg() == IMidiMsg::kPitchWheel)
      {
        toAdd.type = Event::kLegacyMIDICCOutEvent;
        toAdd.midiCCOut.channel = msg.Channel();
        toAdd.midiCCOut.value = msg.mData1;
        toAdd.midiCCOut.value = msg.mData2;
      }
      
      mMidiOutputQueue.Remove();
      // don't add any midi messages other than noteon/noteoff
    }
  }
  
  mMidiOutputQueue.Flush(numSamples);
  
  // Output SYSEX from the editor, which has bypassed the processors' ProcessSysEx()
  if (sysExQueue.ElementsAvailable())
  {
    Event toAdd = {0};
    
    while (sysExQueue.Pop(sysExBuf))
    {
      toAdd.type = Event::kDataEvent;
      toAdd.sampleOffset = sysExBuf.mOffset;
      toAdd.data.type = DataEvent::kMidiSysEx;
      toAdd.data.size = sysExBuf.mSize;
      toAdd.data.bytes = (uint8*) sysExBuf.mData; // TODO!  this is a problem if more than one message in this block!
      outputEvents->addEvent(toAdd);
    }
  }
}

void IPlugVST3ProcessorBase::SetBusArrangments(SpeakerArrangement* pInputBusArrangements, int32 numInBuses, SpeakerArrangement* pOutputBusArrangements, int32 numOutBuses)
{
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
}

void IPlugVST3ProcessorBase::AttachBuffers(ERoute direction, int idx, int n, AudioBusBuffers& pBus, int nFrames, int32 sampleSize)
{
  if (sampleSize == kSample32)
    IPlugProcessor::AttachBuffers(direction, idx, n, pBus.channelBuffers32, nFrames);
  else if (sampleSize == kSample64)
    IPlugProcessor::AttachBuffers(direction, idx, n, pBus.channelBuffers64, nFrames);
}

bool IPlugVST3ProcessorBase::SetupProcessing(const ProcessSetup& setup, ProcessSetup& storedSetup)
{
  if ((setup.symbolicSampleSize != kSample32) && (setup.symbolicSampleSize != kSample64))
    return false;
  
  storedSetup = setup;
  
  SetSampleRate(setup.sampleRate);
  IPlugProcessor::SetBlockSize(setup.maxSamplesPerBlock); // TODO: should IPlugVST3Processor call SetBlockSize in construct unlike other APIs?
  mMidiOutputQueue.Resize(setup.maxSamplesPerBlock);
  OnReset();
  
  return true;
}

bool IPlugVST3ProcessorBase::SetProcessing(bool state)
{
  if (!state)
    OnReset();
  
  return true;
}

bool IPlugVST3ProcessorBase::CanProcessSampleSize(int32 symbolicSampleSize)
{
  switch (symbolicSampleSize)
  {
    case kSample32:   // fall through
    case kSample64:   return true;
    default:          return false;
  }
}

static bool IsBusActive(const BusList& list, int32 idx)
{
  bool exists = false;
  if (idx < static_cast<int32> (list.size()))
    exists = static_cast<bool>(list.at(idx));
  return exists;
}

void IPlugVST3ProcessorBase::PrepareProcessContext(ProcessData& data, ProcessSetup& setup)
{
  ITimeInfo timeInfo;
  
  if (data.processContext)
    memcpy(&mProcessContext, data.processContext, sizeof(ProcessContext));
  
  if (mProcessContext.state & ProcessContext::kProjectTimeMusicValid)
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
  const bool offline = setup.processMode == Steinberg::Vst::kOffline;
  SetTimeInfo(timeInfo);
  SetRenderingOffline(offline);
}

void IPlugVST3ProcessorBase::ProcessParameterChanges(ProcessData& data)
{
  IParameterChanges* paramChanges = data.inputParameterChanges;
  
  if (paramChanges)
  {
    int32 numParamsChanged = paramChanges->getParameterCount();
    
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
            default:
            {
              if (idx >= 0 && idx < mPlug.NParams())
              {
#ifdef PARAMS_MUTEX
                mPlug.mParams_mutex.Enter();
#endif
                mPlug.GetParam(idx)->SetNormalized((double)value); // TODO: In VST3 non distributed the same parameter value is also set via IPlugVST3Controller::setParamNormalized(ParamID tag, ParamValue value)
                mPlug.OnParamChange(idx, kHost, offsetSamples);
#ifdef PARAMS_MUTEX
                mPlug.mParams_mutex.Leave();
#endif
              }
            }
              break;
          }
        }
      }
    }
  }
}

void IPlugVST3ProcessorBase::ProcessAudio(ProcessData& data, ProcessSetup& setup, const BusList& ins, const BusList& outs)
{
  int32 sampleSize = setup.symbolicSampleSize;
  
  if (sampleSize == kSample32 || sampleSize == kSample64)
  {
    if (data.numInputs)
    {
      if (HasSidechainInput())
      {
        if (IsBusActive(ins, 1)) // Sidechain is active
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
        
        AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput) - NSidechainChannels(), data.inputs[0], data.numSamples, sampleSize);
        AttachBuffers(ERoute::kInput, NSidechainChannels(), MaxNChannels(ERoute::kInput) - NSidechainChannels(), data.inputs[1], data.numSamples, sampleSize);
      }
      else
      {
        SetChannelConnections(ERoute::kInput, 0, data.inputs[0].numChannels, true);
        SetChannelConnections(ERoute::kInput, data.inputs[0].numChannels, MaxNChannels(ERoute::kInput) - data.inputs[0].numChannels, false);
        AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), data.inputs[0], data.numSamples, sampleSize);
      }
    }
    
    for (int outBus = 0, chanOffset = 0; outBus < data.numOutputs; outBus++)
    {
      int busChannels = data.outputs[outBus].numChannels;
      SetChannelConnections(ERoute::kOutput, chanOffset, busChannels, IsBusActive(outs, outBus));
      SetChannelConnections(ERoute::kOutput, chanOffset + busChannels, MaxNChannels(ERoute::kOutput) - (chanOffset + busChannels), false);
      AttachBuffers(ERoute::kOutput, chanOffset, busChannels, data.outputs[outBus], data.numSamples, sampleSize);
      chanOffset += busChannels;
    }
    
    if (GetBypassed())
    {
      if (sampleSize == kSample32)
        PassThroughBuffers(0.f, data.numSamples); // single precision
      else
        PassThroughBuffers(0.0, data.numSamples); // double precision
    }
    else
    {
#ifdef PARAMS_MUTEX
      mPlug.mParams_mutex.Enter();
#endif
      if (sampleSize == kSample32)
        ProcessBuffers(0.f, data.numSamples); // single precision
      else
        ProcessBuffers(0.0, data.numSamples); // double precision
#ifdef PARAMS_MUTEX
      mPlug.mParams_mutex.Leave();
#endif
    }
  }
}

void IPlugVST3ProcessorBase::Process(ProcessData& data, ProcessSetup& setup, const BusList& ins, const BusList& outs, IPlugQueue<IMidiMsg>& fromEditor, IPlugQueue<IMidiMsg>& fromProcessor, IPlugQueue<SysExData>& sysExFromEditor, SysExData& sysExBuf)
{
  PrepareProcessContext(data, setup);
  ProcessParameterChanges(data);
  
  if (DoesMIDIIn())
  {
    ProcessMidiIn(data.inputEvents, fromEditor, fromProcessor);
  }
  
  ProcessAudio(data, setup, ins, outs);
  
  if (DoesMIDIOut())
  {
    ProcessMidiOut(sysExFromEditor, sysExBuf, data.outputEvents, data.numSamples);
  }
}

bool IPlugVST3ProcessorBase::SendMidiMsg(const IMidiMsg& msg)
{
  mMidiOutputQueue.Add(msg);
  return true;
}
