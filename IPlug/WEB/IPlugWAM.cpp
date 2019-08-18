/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugWAM.h"

using namespace iplug;

IPlugWAM::IPlugWAM(const InstanceInfo& info, const Config& config)
: IPlugAPIBase(config, kAPIWAM)
, IPlugProcessor(config, kAPIWAM)
{
  int nInputs = MaxNChannels(ERoute::kInput), nOutputs = MaxNChannels(ERoute::kOutput);

  SetChannelConnections(ERoute::kInput, 0, nInputs, true);
  SetChannelConnections(ERoute::kOutput, 0, nOutputs, true);
}

const char* IPlugWAM::init(uint32_t bufsize, uint32_t sr, void* pDesc)
{
  DBGMSG("init\n");

  SetSampleRate(sr);
  SetBlockSize(bufsize);

  DBGMSG("%i %i\n", sr, bufsize);

  WDL_String json;
  json.Set("{\n");
  json.AppendFormatted(8192, "\"audio\": { \"inputs\": [{ \"id\":0, \"channels\":%i }], \"outputs\": [{ \"id\":0, \"channels\":%i }] },\n", MaxNChannels(ERoute::kInput), MaxNChannels(ERoute::kOutput));
  json.AppendFormatted(8192, "\"parameters\": [\n");

  for (int idx = 0; idx < NParams(); idx++)
  {
    IParam* pParam = GetParam(idx);
    pParam->GetJSON(json, idx);

    if(idx < NParams()-1)
      json.AppendFormatted(8192, ",\n");
    else
      json.AppendFormatted(8192, "\n");
  }

  json.Append("]\n}");

  //TODO: correct place? - do we need a WAM reset message?
  OnReset();
  OnParamReset(kReset);

  return json.Get();
}

void IPlugWAM::onProcess(WAM::AudioBus* pAudio, void* pData)
{
  const int blockSize = GetBlockSize();
  
  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), !IsInstrument()); //TODO: go elsewhere
  SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true); //TODO: go elsewhere
  AttachBuffers(ERoute::kInput, 0, NChannelsConnected(ERoute::kInput), pAudio->inputs, blockSize);
  AttachBuffers(ERoute::kOutput, 0, NChannelsConnected(ERoute::kOutput), pAudio->outputs, blockSize);
  ProcessBuffers((float) 0.0f, blockSize);
  
  //emulate IPlugAPIBase::OnTimer - should be called on the main thread - how to do that in audio worklet processor?
  if(mBlockCounter == 0)
  {
    while(mParamChangeFromProcessor.ElementsAvailable())
    {
      ParamTuple p;
      mParamChangeFromProcessor.Pop(p);
      SendParameterValueFromDelegate(p.idx, p.value, false);
    }
    
    while (mMidiMsgsFromProcessor.ElementsAvailable())
    {
      IMidiMsg msg;
      mMidiMsgsFromProcessor.Pop(msg);
      SendMidiMsgFromDelegate(msg);
    }
        
    OnIdle();
    
    mBlockCounter = 8; // 8 * 128 samples = 23ms @ 44100 sr
  }
  
  mBlockCounter--;
}

void IPlugWAM::onMessage(char* verb, char* res, double data)
{
  if(strcmp(verb, "SMMFUI") == 0)
  {
    uint8_t data[3];
    char* pChar = strtok(res, ":");
    int i = 0;
    while (pChar != nullptr)
    {
      data[i++] = atoi(pChar);
      pChar = strtok(nullptr, ":");
    }
    
    IMidiMsg msg = {0, data[0], data[1], data[2]};
    ProcessMidiMsg(msg); // TODO: should queue to mMidiMsgsFromEditor?
  }
}

void IPlugWAM::onMessage(char* verb, char* res, char* str)
{
}

void IPlugWAM::onMessage(char* verb, char* res, void* pData, uint32_t size)
{
  if(strcmp(verb, "SAMFUI") == 0)
  {
    int pos = 0;
    IByteStream stream(pData, size);
    int messageTag;
    int controlTag;
    int dataSize;
    pos = stream.Get(&messageTag, pos);
    pos = stream.Get(&controlTag, pos);
    pos = stream.Get(&dataSize, pos);
    
    OnMessage(messageTag, controlTag, dataSize, stream.GetData() + (sizeof(int) * 3));
  }
  else if(strcmp(verb, "SSMFUI") == 0)
  {
    //TODO
  }
  else
  {
    DBGMSG("onMessageA not handled\n");
  }
}

void IPlugWAM::onMidi(byte status, byte data1, byte data2)
{
//   DBGMSG("onMidi\n");
  IMidiMsg msg = {0, status, data1, data2};
  ProcessMidiMsg(msg); // onMidi is not called on HPT. We could queue things up, but just process the message straightaway for now
  //mMidiMsgsFromProcessor.Push(msg);
  
  WDL_String dataStr;
  dataStr.SetFormatted(16, "%i:%i:%i", msg.mStatus, msg.mData1, msg.mData2);
  
  // TODO: in the future this will be done via shared array buffer
  // if onMidi ever gets called on HPT, should defer via queue
  postMessage("SMMFD", dataStr.Get(), "");
}

void IPlugWAM::onParam(uint32_t idparam, double value)
{
//  DBGMSG("IPlugWAM:: onParam %i %f\n", idparam, value);
  SetParameterValue(idparam, value);
}

void IPlugWAM::onSysex(byte* pData, uint32_t size)
{
  ISysEx sysex = {0 /* no offset */, pData, (int) size };
  ProcessSysEx(sysex);
  
  WDL_String dataStr;
  dataStr.SetFormatted(16, "%i", size);
  
  // TODO: in the future this will be done via shared array buffer
  // if onSysex ever gets called on HPT, should defer via queue
  postMessage("SSMFD", dataStr.Get(), "");
}

void IPlugWAM::SendControlValueFromDelegate(int controlTag, double normalizedValue)
{
  WDL_String propStr;
  WDL_String dataStr;

  propStr.SetFormatted(16, "%i", controlTag);
  dataStr.SetFormatted(16, "%f", normalizedValue);

  // TODO: in the future this will be done via shared array buffer
  postMessage("SCVFD", propStr.Get(), dataStr.Get());
}

void IPlugWAM::SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, const void* pData)
{
  WDL_String propStr;
  propStr.SetFormatted(16, "%i:%i", controlTag, messageTag);
  
  // TODO: in the future this will be done via shared array buffer
  postMessage("SCMFD", propStr.Get(), pData, (uint32_t) dataSize);
}

void IPlugWAM::SendParameterValueFromDelegate(int paramIdx, double value, bool normalized)
{
  WDL_String propStr;
  WDL_String dataStr;
  propStr.SetFormatted(16, "%i", paramIdx);
  dataStr.SetFormatted(16, "%f", value);

  // TODO: in the future this will be done via shared array buffer
  postMessage("SPVFD", propStr.Get(), dataStr.Get());
}

void IPlugWAM::SendArbitraryMsgFromDelegate(int messageTag, int dataSize, const void* pData)
{
  WDL_String propStr;
  propStr.SetFormatted(16, "%i", messageTag);
  
  // TODO: in the future this will be done via shared array buffer
  postMessage("SAMFD", propStr.Get(), pData, (uint32_t) dataSize);
}

