
/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

#include "public.sdk/source/vst/vstbus.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include "pluginterfaces/base/ibstream.h"

#include "IPlugStructs.h"
#include "IPlugQueue.h"
#include "IPlugProcessor.h"
#include "IPlugVST3_Parameter.h"

using namespace Steinberg;
using namespace Vst;

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

class IPlugVST3_ProcessorBase : public IPlugProcessor<PLUG_SAMPLE_DST>
{
public:
  IPlugVST3_ProcessorBase(IPlugConfig c) : IPlugProcessor<PLUG_SAMPLE_DST>(c, kAPIVST3)
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
  
  template <class T>
  void Initialize(T* effect)
  {
    String128 tmpStringBuf;
    
//  for(auto configIdx = 0; configIdx < NIOConfigs(); configIdx++)
//  {
    int configIdx = NIOConfigs()-1;
    
    IOConfig* pConfig = GetIOConfig(configIdx);
    
    assert(pConfig);
    for(auto busIdx = 0; busIdx < pConfig->NBuses(ERoute::kInput); busIdx++)
    {
      uint64_t busType = GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kInput, busIdx, pConfig);
      
      int flags = 0; //busIdx == 0 ? flags = Steinberg::Vst::BusInfo::BusFlags::kDefaultActive : flags = 0;
      UString(tmpStringBuf, 128).fromAscii(pConfig->GetBusInfo(ERoute::kInput, busIdx)->mLabel.Get(), 128);
      effect->addAudioInput(tmpStringBuf, busType, (BusTypes) busIdx > 0, flags);
    }
    
    for(auto busIdx = 0; busIdx < pConfig->NBuses(ERoute::kOutput); busIdx++)
    {
      uint64_t busType = GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kOutput, busIdx, pConfig);
      
      int flags = 0; //busIdx == 0 ? flags = Steinberg::Vst::BusInfo::BusFlags::kDefaultActive : flags = 0;
      UString(tmpStringBuf, 128).fromAscii(pConfig->GetBusInfo(ERoute::kOutput, busIdx)->mLabel.Get(), 128);
      effect->addAudioOutput(tmpStringBuf, busType, (BusTypes) busIdx > 0, flags);
    }
//  }
  }
  
  void SetBusArrangments(SpeakerArrangement* pInputBusArrangements, int32 numInBuses, SpeakerArrangement* pOutputBusArrangements, int32 numOutBuses)
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
  
  void DoMidiIn(IEventList* eventList, IPlugQueue<IMidiMsg>& editorQueue, IPlugQueue<IMidiMsg>& processorQueue)
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
  
  void DoMidiOut(IPlugQueue<SysExData>& sysExQueue, SysExData& sysExBuf, IEventList* outputEvents, int32 numSamples)
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
  
  void AttachBuffers(ERoute direction, int idx, int n, AudioBusBuffers& pBus, int nFrames, int32 sampleSize)
  {
    if (sampleSize == kSample32)
      IPlugProcessor::AttachBuffers(direction, idx, n, pBus.channelBuffers32, nFrames);
    else if (sampleSize == kSample64)
        IPlugProcessor::AttachBuffers(direction, idx, n, pBus.channelBuffers64, nFrames);
  }
  
  bool SetupProcessing(const ProcessSetup& setup)
  {
    if ((setup.symbolicSampleSize != kSample32) && (setup.symbolicSampleSize != kSample64))
      return false;
    
    SetSampleRate(setup.sampleRate);
    //SetBypassed(false);   // TODO - why???
    IPlugProcessor::SetBlockSize(setup.maxSamplesPerBlock); // TODO: should IPlugVST3Processor call SetBlockSize in construct unlike other APIs?
    mMidiOutputQueue.Resize(setup.maxSamplesPerBlock);
    OnReset();
    
    return true;
  }
  
  bool IsBusActive(const BusList& list, int32 idx)
  {
    bool exists = false;
    if (idx < static_cast<int32> (list.size()))
      exists = static_cast<bool>(list.at(idx));
    return exists;
  }
  
  void PrepareProcessContext(ProcessData& data, ProcessSetup& setup)
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
  
  template <class T>
  void ProcessParameterChanges(T* plug, ProcessData& data)
  {
    IParameterChanges* paramChanges = data.inputParameterChanges;
    
    if (paramChanges)
    {
      int32 numParamsChanged = paramChanges->getParameterCount();
      
      // it is possible to get a finer resolution of control here by retrieving more values (points) from the queue
      // for now we just grab the last one
      
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
                if (idx >= 0 && idx < plug->NParams())
                {
                  ENTER_PARAMS_MUTEX;
                  plug->GetParam(idx)->SetNormalized((double)value);
                  plug->SendParameterValueFromAPI(idx, (double) value, true);
                  plug->OnParamChange(idx, kHost, offsetSamples);
                  LEAVE_PARAMS_MUTEX;
                }
              }
              break;
            }
          }
        }
      }
    }
  }
  
  void ProcessAudio(ProcessData& data, const BusList& ins, const BusList& outs, ProcessSetup& setup)
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
        if (sampleSize == kSample32)
          ProcessBuffers(0.f, data.numSamples); // single precision
        else
          ProcessBuffers(0.0, data.numSamples); // double precision
      }
    }
  }
  
  template <class T>
  void Process(T* plug, ProcessData& data, ProcessSetup& setup, const BusList& ins, const BusList& outs, IPlugQueue<IMidiMsg>& fromEditor, IPlugQueue<IMidiMsg>& fromProcessor, IPlugQueue<SysExData>& sysExFromEditor, SysExData& sysExBuf)
  {
    PrepareProcessContext(data, setup);
    ProcessParameterChanges(plug, data);
    
    if (DoesMIDIIn())
    {
      DoMidiIn(data.inputEvents, fromEditor, fromProcessor);
    }
    
    ProcessAudio(data, ins, outs, setup);
    
    if (DoesMIDIOut())
    {
      DoMidiOut(sysExFromEditor, sysExBuf, data.outputEvents, data.numSamples);
    }
  }
  
  bool SendMidiMsg(const IMidiMsg& msg) override
  {
    mMidiOutputQueue.Add(msg);
    return true;
  }
  
  bool CanProcessSampleSize(int32 symbolicSampleSize)
  {
    switch (symbolicSampleSize)
    {
      case kSample32:   // fall through
      case kSample64:   return true;
      default:          return false;
    }
  }
  
private:
    
  ProcessContext mProcessContext;
  IMidiQueue mMidiOutputQueue;
  bool mSidechainActive = false;
};

// State

struct IPlugVST3State
{
  template <class T>
  static bool GetState(T* plug, IBStream* state)
  {
    IByteChunk chunk;
    
    // TODO: IPlugVer should be in chunk!
    //  IByteChunk::GetIPlugVerFromChunk(chunk)
    
    if (plug->SerializeState(chunk))
    {
      /*
       int chunkSize = chunk.Size();
       void* data = (void*) &chunkSize;
       state->write(data, (int32) sizeof(int));
       state->write(chunk.GetData(), chunkSize);*/
      state->write(chunk.GetData(), chunk.Size());
    }
    else
    {
      return false;
    }
    
    int32 toSaveBypass = plug->GetBypassed() ? 1 : 0;
    state->write(&toSaveBypass, sizeof (int32));
    
    return true;
  };
  
  template <class T>
  static bool SetState(T* plug, IBStream* state)
  {
    TRACE;
    
    IByteChunk chunk;
    
    const int bytesPerBlock = 128;
    char buffer[bytesPerBlock];
    
    while(true)
    {
      Steinberg::int32 bytesRead = 0;
      auto status = state->read(buffer, (Steinberg::int32) bytesPerBlock, &bytesRead);
      
      if (bytesRead <= 0 || (status != kResultTrue && plug->GetHost() != kHostWaveLab))
        break;
      
      chunk.PutBytes(buffer, bytesRead);
    }
    int pos = plug->UnserializeState(chunk,0);
    
    int32 savedBypass = 0;
    
    state->seek(pos,IBStream::IStreamSeekMode::kIBSeekSet);
    if (state->read (&savedBypass, sizeof (Steinberg::int32)) != kResultOk) {
      return kResultFalse;
    }
    
    if (!plug->IsInstrument())
    {
      plug->getParameterObject(kBypassParam)->setNormalized(savedBypass);
    }
    
    plug->OnRestoreState();
    return kResultOk;
  }
};
