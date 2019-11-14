 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import <AudioToolbox/AudioToolbox.h>
#include <CoreMIDI/CoreMIDI.h>

#include "IPlugAUv3.h"
#import "IPlugAUAudioUnit.h"

using namespace iplug;

IPlugAUv3::IPlugAUv3(const InstanceInfo& instanceInfo, const Config& config)
: IPlugAPIBase(config, kAPIAUv3)
, IPlugProcessor(config, kAPIAUv3)
{
  Trace(TRACELOC, "%s", config.pluginName);
}

void IPlugAUv3::SetAUAudioUnit(void* pAUAudioUnit)
{
  mAUAudioUnit = pAUAudioUnit;
}

void IPlugAUv3::InformHostOfParamChange(int paramIdx, double normalizedValue)
{
  const AUParameterAddress address = GetParamAddress(paramIdx);

  [(__bridge IPlugAUAudioUnit*) mAUAudioUnit informHostOfParamChange:address :(float) GetParam(paramIdx)->FromNormalized(normalizedValue)];
}

bool IPlugAUv3::SendMidiMsg(const IMidiMsg& msg)
{
  uint8_t data[3] = { msg.mStatus, msg.mData1, msg.mData2 };
  
  int64_t sampleTime = mLastTimeStamp.mSampleTime + msg.mOffset;
  
  return [(__bridge IPlugAUAudioUnit*) mAUAudioUnit sendMidiData: sampleTime : sizeof(data) : data];
}

//bool IPlugAUv3::SendMidiMsgs(WDL_TypedBuf<IMidiMsg>& msgs)
//{
//  return false;
//}

bool IPlugAUv3::SendSysEx(const ISysEx& msg)
{
  int64_t sampleTime = mLastTimeStamp.mSampleTime + msg.mOffset;

  return [(__bridge IPlugAUAudioUnit*) mAUAudioUnit sendMidiData: sampleTime : msg.mSize : msg.mData];
}

//void IPlugAUv3::HandleOneEvent(AURenderEvent const *event, AUEventSampleTime startTime)
//{
//  switch (event->head.eventType)
//  {
//    case AURenderEventParameter:
//    case AURenderEventParameterRamp:
//    {
//      AUParameterEvent const& paramEvent = event->parameter;
//      const int paramIdx = GetParamIdx(paramEvent.parameterAddress);
//      const double value = (double) paramEvent.value;
//      const int sampleOffset = (int) (paramEvent.eventSampleTime - startTime);
//      ENTER_PARAMS_MUTEX
//      GetParam(paramIdx)->Set(value);
//      LEAVE_PARAMS_MUTEX
//      OnParamChange(paramIdx, EParamSource::kHost, sampleOffset);
//      break;
//    }
//
//    case AURenderEventMIDI:
//    {
//      IMidiMsg msg;
//      msg.mStatus = event->MIDI.data[0];
//      msg.mData1 = event->MIDI.data[1];
//      msg.mData2 = event->MIDI.data[2];
//      msg.mOffset = (int) (event->MIDI.eventSampleTime - startTime);
//      ProcessMidiMsg(msg);
//      mMidiMsgsFromProcessor.Push(msg);
//      break;
//    }
//    default:
//      break;
//  }
//}
//
//void IPlugAUv3::PerformAllSimultaneousEvents(AUEventSampleTime now, AURenderEvent const *&event)
//{
//  do {
//    HandleOneEvent(event, now);
//
//    // Go to next event.
//    event = event->head.next;
//
//    // While event is not null and is simultaneous (or late).
//  } while (event && event->head.eventSampleTime <= now);
//}

void IPlugAUv3::ProcessWithEvents(AudioTimeStamp const* pTimestamp, uint32_t frameCount, AURenderEvent const* pEvents, ITimeInfo& timeInfo)
{
  SetTimeInfo(timeInfo);
  
  IMidiMsg midiMsg;
  while (mMidiMsgsFromEditor.Pop(midiMsg))
  {
    ProcessMidiMsg(midiMsg);
  }
  
  mLastTimeStamp = *pTimestamp;
  AUEventSampleTime now = AUEventSampleTime(pTimestamp->mSampleTime);
  uint32_t framesRemaining = frameCount;
  
  for (const AURenderEvent* pEvent = pEvents; pEvent != nullptr; pEvent = pEvent->head.next)
  {
    switch (pEvent->head.eventType)
    {
      case AURenderEventMIDI:
      {
        const AUMIDIEvent& midiEvent = pEvent->MIDI;

        midiMsg = {static_cast<int>(midiEvent.eventSampleTime - now), midiEvent.data[0], midiEvent.data[1], midiEvent.data[2] };
        ProcessMidiMsg(midiMsg);
        mMidiMsgsFromProcessor.Push(midiMsg);
      }
      break;

      case AURenderEventParameter:
      case AURenderEventParameterRamp:
      {
        const AUParameterEvent& paramEvent = pEvent->parameter;
        const int paramIdx = GetParamIdx(paramEvent.parameterAddress);
        const double value = (double) paramEvent.value;
        const int sampleOffset = (int) (paramEvent.eventSampleTime - now);
        ENTER_PARAMS_MUTEX
        GetParam(paramIdx)->Set(value);
        LEAVE_PARAMS_MUTEX
        OnParamChange(paramIdx, EParamSource::kHost, sampleOffset);
        break;
      }
      break;

      default:
        break;
    }
  }

  ENTER_PARAMS_MUTEX;
  ProcessBuffers(0.f, framesRemaining); // what about bufferOffset
  LEAVE_PARAMS_MUTEX;
    
  //Output SYSEX from the editor, which has bypassed ProcessSysEx()
  while (mSysExDataFromEditor.Pop(mSysexBuf))
  {
    ISysEx smsg {mSysexBuf.mOffset, mSysexBuf.mData, mSysexBuf.mSize};
    SendSysEx(smsg);
  }
  

//  while (framesRemaining > 0) {
//    // If there are no more events, we can process the entire remaining segment and exit.
//    if (event == nullptr) {
////      uint32_t const bufferOffset = frameCount - framesRemaining;
// TODO - ProcessBuffers should be within param mutex lock
//      ProcessBuffers(0.f, framesRemaining); // what about bufferOffset
//      return;
//    }
//
//    // **** start late events late.
//    auto timeZero = AUEventSampleTime(0);
//    auto headEventTime = event->head.eventSampleTime;
//    uint32_t const framesThisSegment = uint32_t(std::max(timeZero, headEventTime - now));
//
//    // Compute everything before the next event.
//    if (framesThisSegment > 0)
//    {
////      uint32_t const bufferOffset = frameCount - framesRemaining;
// TODO - ProcessBuffers should be within param mutex lock
//      ProcessBuffers(0.f, framesThisSegment); // what about bufferOffset
//
//      // Advance frames.
//      framesRemaining -= framesThisSegment;
//
//      // Advance time.
//      now += AUEventSampleTime(framesThisSegment);
//    }
//
//    PerformAllSimultaneousEvents(now, event);
//  }
}

// this is called on a secondary thread (not main thread, not audio thread)
void IPlugAUv3::SetParameterFromValueObserver(uint64_t address, float value)
{
  const int paramIdx = GetParamIdx(address);

  ENTER_PARAMS_MUTEX
  IParam* pParam = GetParam(paramIdx);
  assert(pParam);
  pParam->Set((double) value);
  LEAVE_PARAMS_MUTEX  
  OnParamChange(paramIdx, kHost, -1);
}

void IPlugAUv3::SendParameterValueFromObserver(uint64_t address, float value)
{
  const int paramIdx = GetParamIdx(address);
  
  SendParameterValueFromAPI(paramIdx, value, false); // will trigger OnParamChangeUI()
}

float IPlugAUv3::GetParameter(uint64_t address)
{
  const int paramIdx = GetParamIdx(address);

  ENTER_PARAMS_MUTEX
  const float val = (float) GetParam(paramIdx)->Value();
  LEAVE_PARAMS_MUTEX
  return val;
}

const char* IPlugAUv3::GetParamDisplayForHost(uint64_t address, float value)
{
  const int paramIdx = GetParamIdx(address);

  ENTER_PARAMS_MUTEX
  GetParam(paramIdx)->GetDisplayForHost(value, false, mParamDisplayStr);
  LEAVE_PARAMS_MUTEX
  return (const char*) mParamDisplayStr.Get();
}

float IPlugAUv3::GetParamStringToValue(uint64_t address, const char* str)
{
  const int paramIdx = GetParamIdx(address);
  
  ENTER_PARAMS_MUTEX
  float val = (float) GetParam(paramIdx)->StringToValue(str);
  LEAVE_PARAMS_MUTEX  
  return val;
}

void IPlugAUv3::SetBuffers(AudioBufferList* pInBufList, AudioBufferList* pOutBufList)
{
  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), false);
  SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), false);

  int chanIdx = 0;
  if(pInBufList)
  {
    for(int i = 0; i < pInBufList->mNumberBuffers; i++)
    {
      int nConnected = pInBufList->mBuffers[i].mNumberChannels;
      SetChannelConnections(ERoute::kInput, chanIdx, nConnected, true);
      AttachBuffers(ERoute::kInput, chanIdx, nConnected, (float**) &(pInBufList->mBuffers[i].mData), GetBlockSize());
      chanIdx += nConnected;
    }
  }
  
  chanIdx = 0;
  
  if(pOutBufList)
  {
    for(int i = 0; i < pOutBufList->mNumberBuffers; i++)
    {
      int nConnected = pOutBufList->mBuffers[i].mNumberChannels;
      SetChannelConnections(ERoute::kOutput, chanIdx, nConnected, true);
      AttachBuffers(ERoute::kOutput, chanIdx, nConnected, (float**) &(pOutBufList->mBuffers[i].mData), GetBlockSize());
      chanIdx += nConnected;
    }
  }
}

void IPlugAUv3::Prepare(double sampleRate, uint32_t blockSize)
{
  SetBlockSize(blockSize);
  SetSampleRate(sampleRate);
}
