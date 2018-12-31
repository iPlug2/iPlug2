#include "IWebsocketEditorDelegate.h"
#include "IPlugStructs.h"

IWebsocketEditorDelegate::IWebsocketEditorDelegate(int nParams)
: IGEditorDelegate(nParams)
, mParamChangeFromClients(PARAM_TRANSFER_SIZE)
, mMIDIFromClients(MIDI_TRANSFER_SIZE)
{
  
}

IWebsocketEditorDelegate::~IWebsocketEditorDelegate()
{
}

void IWebsocketEditorDelegate::OnWebsocketReady(int connIdx)
{
  //TODO: need to send serialize state and send it to the client
}

bool IWebsocketEditorDelegate::OnWebsocketText(int connIdx, const char* pStr, size_t dataSize)
{
  return true; // return true to keep the connection open
}

//this method gets called on server connection thread
bool IWebsocketEditorDelegate::OnWebsocketData(int connIdx, void* pData, size_t dataSize)
{
  uint8_t* pByteData = (uint8_t*) pData;
  int pos = 6;

  if (memcmp(pData, "SPVFUI" , 6) == 0) // send parameter value from user interface
  {
    int paramIdx = * ((int*)(pByteData + pos)); pos+= 4;
    double value = * ((double*)(pByteData + pos)); pos += 8;
    
    mParamChangeFromClients.Push(IParamChange { paramIdx, value, true } );
  }
  else if (memcmp(pData, "SMMFUI" , 6) == 0) // send midi message from user interface
  {
    IMidiMsg msg;
    msg.mStatus = * ((uint8_t*)(pByteData + pos)); pos++;
    msg.mData1 = * ((uint8_t*)(pByteData + pos)); pos++;
    msg.mData2 = * ((uint8_t*)(pByteData + pos)); pos++;

    mMIDIFromClients.Push(msg);
  }
  else if (memcmp(pData, "SSMFUI" , 6) == 0) // send sysex message from user interface
  {
    //TODO: how are we going to queue
  }
  else if (memcmp(pData, "SAMFUI" , 6) == 0) // send arbitrary message from user interface
  {
  }
  
  //TODO: should now echo message to other clients
  
  return true;
}

void IWebsocketEditorDelegate::SendMidiMsgFromUI(const IMidiMsg& msg)
{
  IByteChunk data; // FIXME: don't copy
  data.PutStr("SMMFD");
  data.Put(&msg.mStatus);
  data.Put(&msg.mData1);
  data.Put(&msg.mData2);

  // Server side UI edit, send to clients
  SendDataToConnection(-1, data.GetData(), data.Size());
  
  IGEditorDelegate::SendMidiMsgFromUI(msg);
}

void IWebsocketEditorDelegate::SendSysexMsgFromUI(const ISysEx& msg)
{
  IByteChunk data; // FIXME: don't copy
  data.PutStr("SSMFD");
  data.Put(&msg.mSize);
  data.PutBytes(&msg.mData, msg.mSize);
  
  // Server side UI edit, send to clients
  SendDataToConnection(-1, data.GetData(), data.Size());
  
  IGEditorDelegate::SendSysexMsgFromUI(msg);
}

void IWebsocketEditorDelegate::SendArbitraryMsgFromUI(int messageTag, int controlTag, int dataSize, const void* pData)
{
  IByteChunk data; // FIXME: don't copy
  data.PutStr("SSMFD");
  data.Put(&messageTag);
  data.Put(&controlTag);
  data.Put(&dataSize);
  data.PutBytes(pData, dataSize);
  
  // Server side UI edit, send to clients
  SendDataToConnection(-1, data.GetData(), data.Size());
  
  IGEditorDelegate::SendArbitraryMsgFromUI(messageTag, controlTag, dataSize, pData);
}


//void IWebsocketEditorDelegate::BeginInformHostOfParamChangeFromUI(int paramIdx)
//{
//  IGEditorDelegate::BeginInformHostOfParamChangeFromUI(paramIdx);
//}

void IWebsocketEditorDelegate::SendParameterValueFromUI(int paramIdx, double normalizedValue)
{
  IByteChunk data; // FIXME: don't copy
  data.PutStr("SPVFD");
  data.Put(&paramIdx);
  data.Put(&normalizedValue);
  
  // Server side UI edit, send to clients
  SendDataToConnection(-1, data.GetData(), data.Size());

  IGEditorDelegate::SendParameterValueFromUI(paramIdx, normalizedValue);
}

//void IWebsocketEditorDelegate::EndInformHostOfParamChangeFromUI(int paramIdx)
//{
//  IGEditorDelegate::EndInformHostOfParamChangeFromUI(paramIdx);
//}

void IWebsocketEditorDelegate::SendControlValueFromDelegate(int controlTag, double normalizedValue)
{
  IByteChunk data; // FIXME: don't copy
  data.PutStr("SCVFD");
  data.Put(&controlTag);
  data.Put(&normalizedValue);
  
  SendDataToConnection(-1, data.GetData(), data.Size());
  
  IGEditorDelegate::SendControlValueFromDelegate(controlTag, normalizedValue);
}

void IWebsocketEditorDelegate::SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, const void* pData)
{
  IByteChunk data; // FIXME: don't copy
  data.PutStr("SCMFD");
  data.Put(&controlTag);
  data.Put(&messageTag);
  data.Put(&dataSize);
  data.PutBytes(pData, dataSize);
  
  SendDataToConnection(-1, data.GetData(), data.Size());
  
  IGEditorDelegate::SendControlMsgFromDelegate(controlTag, messageTag, dataSize, pData);
}

void IWebsocketEditorDelegate::SendArbitraryMsgFromDelegate(int messageTag, int dataSize, const void* pData)
{
  IByteChunk data; // FIXME: don't copy
  data.PutStr("SAMFD");
  data.Put(&messageTag);
  data.Put(&dataSize);
  data.PutBytes(pData, dataSize);
  
  SendDataToConnection(-1, data.GetData(), data.Size());
  
  IGEditorDelegate::SendArbitraryMsgFromDelegate(messageTag, dataSize, pData);
}

void IWebsocketEditorDelegate::SendMidiMsgFromDelegate(const IMidiMsg& msg)
{
  IByteChunk data; // FIXME: don't copy
  data.PutStr("SMMFD");
  data.Put(&msg.mStatus);
  data.Put(&msg.mData1);
  data.Put(&msg.mData2);

  SendDataToConnection(-1, data.GetData(), data.Size());
  
  IGEditorDelegate::SendMidiMsgFromDelegate(msg);
}

void IWebsocketEditorDelegate::SendSysexMsgFromDelegate(const ISysEx& msg)
{
  IByteChunk data; // FIXME: don't copy
  data.PutStr("SSMFD");
  data.Put(&msg.mSize);
  data.PutBytes(msg.mData, msg.mSize);
  
  SendDataToConnection(-1, data.GetData(), data.Size());
  
  IGEditorDelegate::SendSysexMsgFromDelegate(msg);
}

void IWebsocketEditorDelegate::ProcessWebsocketQueue()
{
  while(mParamChangeFromClients.ElementsAvailable())
  {
    IParamChange p;
    mParamChangeFromClients.Pop(p);
    
    //FIXME: how do params get updated?
//    ENTER_PARAMS_MUTEX;
//    if(p.normalized)
//      GetParam(p.paramIdx)->SetNormalized(p.value);
//    else
//      GetParam(p.paramIdx)->Set(p.value);
//
//    OnParamChange(p.paramIdx, kHost);
//    LEAVE_PARAMS_MUTEX;
    
    SendParameterValueFromDelegate(p.paramIdx, p.value, p.normalized); // TODO:  if the parameter hasn't changed maybe we shouldn't do anything?
  }
  
  while (mMIDIFromClients.ElementsAvailable()) {
    IMidiMsg msg;
    mMIDIFromClients.Pop(msg);
    IGEditorDelegate::SendMidiMsgFromDelegate(msg); // Call the superclass, since we don't want to send another MIDI message to the websocket
    DeferMidiMsg(msg); // can't just call SendMidiMsgFromUI here which would cause a feedback loop
  }
}
