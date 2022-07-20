/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugVST3_Processor.h"

#pragma mark - IPlugVST3Processor Constructor/Destructor

using namespace iplug;
using namespace Steinberg;
using namespace Vst;

IPlugVST3Processor::IPlugVST3Processor(const InstanceInfo& info, const Config& config)
: IPlugAPIBase(config, kAPIVST3)
, IPlugVST3ProcessorBase(config, *this)
{
  setControllerClass(info.mOtherGUID);
#ifndef OS_LINUX
  CreateTimer();
#endif
}

IPlugVST3Processor::~IPlugVST3Processor() {}

#pragma mark AudioEffect overrides

tresult PLUGIN_API IPlugVST3Processor::initialize(FUnknown* context)
{
  TRACE
  
  if (AudioEffect::initialize(context) == kResultOk)
  {
    Initialize(this);
    IPlugVST3GetHost(this, context);
    OnHostIdentified();
    OnParamReset(kReset);
    
    return kResultOk;
  }
  
  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3Processor::terminate()
{
  return AudioEffect::terminate();
}

tresult PLUGIN_API IPlugVST3Processor::setBusArrangements(SpeakerArrangement* pInputBusArrangements, int32 numInBuses, SpeakerArrangement* pOutputBusArrangements, int32 numOutBuses)
{
  TRACE
  
  return SetBusArrangements(this, pInputBusArrangements, numInBuses, pOutputBusArrangements, numOutBuses) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API IPlugVST3Processor::setActive(TBool state)
{
  TRACE
  
  OnActivate((bool) state);
  return AudioEffect::setActive(state);
}

tresult PLUGIN_API IPlugVST3Processor::setupProcessing(ProcessSetup& newSetup)
{
  TRACE
  
  return SetupProcessing(newSetup, processSetup) ? kResultOk : kResultFalse;
}

tresult PLUGIN_API IPlugVST3Processor::setProcessing(TBool state)
{
  Trace(TRACELOC, " state: %i", state);
  
  return SetProcessing((bool) state) ? kResultOk : kResultFalse;
}

tresult PLUGIN_API IPlugVST3Processor::process(ProcessData& data)
{
  TRACE
  
  Process(data, processSetup, audioInputs, audioOutputs, mMidiMsgsFromEditor, mMidiMsgsFromProcessor, mSysExDataFromEditor, mSysexBuf);
  return kResultOk;
}

tresult PLUGIN_API IPlugVST3Processor::canProcessSampleSize(int32 symbolicSampleSize)
{
  return CanProcessSampleSize(symbolicSampleSize) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API IPlugVST3Processor::setState(IBStream* pState)
{
  TRACE
  
  return IPlugVST3State::SetState(this, pState) ? kResultOk : kResultFalse;
}

tresult PLUGIN_API IPlugVST3Processor::getState(IBStream* pState)
{
  TRACE
    
  return IPlugVST3State::GetState(this, pState) ? kResultOk :kResultFalse;
}

#pragma mark IEditorDelegate overrides

void IPlugVST3Processor::SendControlValueFromDelegate(int ctrlTag, double normalizedValue)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  message->setMessageID("SCVFD");
  message->getAttributes()->setInt("CT", ctrlTag);
  message->getAttributes()->setFloat("NV", normalizedValue);
  
  sendMessage(message);
}

void IPlugVST3Processor::SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  message->setMessageID("SCMFD");
  message->getAttributes()->setInt("CT", ctrlTag);
  message->getAttributes()->setInt("MT", msgTag);
  message->getAttributes()->setBinary("D", pData, dataSize);
  
  sendMessage(message);
}

void IPlugVST3Processor::SendArbitraryMsgFromDelegate(int msgTag, int dataSize, const void* pData)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  if (dataSize == 0) // allow sending messages with no data
  {
    dataSize = 1;
    uint8_t dummy = 0;
    pData = &dummy;
  }
  
  message->setMessageID("SAMFD");
  message->getAttributes()->setInt("MT", msgTag);
  message->getAttributes()->setBinary("D", pData, dataSize);
  sendMessage(message);
}

#pragma mark IConnectionPoint override

tresult PLUGIN_API IPlugVST3Processor::notify(IMessage* message)
{
  if (!message)
    return kInvalidArgument;
  
  const void* data = nullptr;
  uint32 size;
  
  if (!strcmp(message->getMessageID(), "SMMFUI")) // midi message from UI
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
  else if (!strcmp(message->getMessageID(), "SAMFUI")) // message from UI
  {
    int64 msgTag;
    int64 ctrlTag;

    if (message->getAttributes()->getInt("MT", msgTag) == kResultOk && message->getAttributes()->getInt("CT", ctrlTag) == kResultOk)
    {
      if (message->getAttributes()->getBinary("D", data, size) == kResultOk)
      {
        if(OnMessage((int) msgTag, (int) ctrlTag, size, data))
        {
          return kResultOk;
        }
      }
      
      return kResultFalse;
    }
  }
  
  return AudioEffect::notify(message);
}

#pragma mark Messaging overrides

void IPlugVST3Processor::TransmitMidiMsgFromProcessor(const IMidiMsg& msg)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  message->setMessageID("SMMFD");
  message->getAttributes()->setBinary("D", (void*) &msg, sizeof(IMidiMsg));
  sendMessage(message);
}

void IPlugVST3Processor::TransmitSysExDataFromProcessor(const SysExData& data)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  message->setMessageID("SSMFD");
  message->getAttributes()->setBinary("D", (void*) data.mData, data.mSize);
  message->getAttributes()->setInt("O", data.mOffset);
  sendMessage(message);
}
