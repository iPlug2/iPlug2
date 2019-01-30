
#pragma once

#include "IPlugStructs.h"
#include "pluginterfaces/vst/ivstevents.h"

using namespace Steinberg;
using namespace Vst;

class IPlugVST3_ProcessorBase : public IPlugProcessor<PLUG_SAMPLE_DST>
{
public:
  IPlugVST3_ProcessorBase(IPlugConfig c) : IPlugProcessor<PLUG_SAMPLE_DST>(c, kAPIVST3)
  {
    // Make sure the process context is predictably initialised in case it is used before process is called
    memset(&mProcessContext, 0, sizeof(ProcessContext));
  }
  
  void DoMIDIn(IEventList* eventList, IPlugQueue<IMidiMsg>& editorQueue, IPlugQueue<IMidiMsg>& processorQueue)
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
  
  void DoMIDIOut(IMidiQueue& queue, IPlugQueue<SysExData>& sysExQueue, SysExData& sysExBuf, IEventList* outputEvents, int32 numSamples)
  {
    // MIDI
    if (!queue.Empty() && outputEvents)
    {
      Event toAdd = {0};
      IMidiMsg msg;
      
      while (!queue.Empty())
      {
        IMidiMsg& msg = queue.Peek();
        
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
        
        queue.Remove();
        // don't add any midi messages other than noteon/noteoff
      }
    }
    
    queue.Flush(numSamples);
    
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
  
  bool SetupProcessing(ProcessSetup& processSetup)
  {
    if ((processSetup.symbolicSampleSize != kSample32) && (processSetup.symbolicSampleSize != kSample64))
      return false;
    
    SetSampleRate(processSetup.sampleRate);
    SetBypassed(false);   // TODO - why???
    IPlugProcessor::SetBlockSize(processSetup.maxSamplesPerBlock); // TODO: should IPlugVST3Processor call SetBlockSize in construct unlike other APIs?
    mMidiOutputQueue.Resize(processSetup.maxSamplesPerBlock);
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
  
  void PrepareProcessContext(ProcessData& data, ProcessSetup& processSetup)
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
    const bool offline = processSetup.processMode == Steinberg::Vst::kOffline;
    SetTimeInfo(timeInfo);
    SetRenderingOffline(offline);
  }
  
  void ProcessAudio(ProcessData& data, const BusList& ins, const BusList& outs, ProcessSetup& processSetup)
  {
    int32 sampleSize = processSetup.symbolicSampleSize;
    
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
  
  bool SendMidiMsg(const IMidiMsg& msg) override
  {
    mMidiOutputQueue.Add(msg);
    return true;
  }

private:
  
  ProcessContext mProcessContext;
  IMidiQueue mMidiOutputQueue;
  bool mSidechainActive = false;
};
