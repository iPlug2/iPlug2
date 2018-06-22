#include "IWebsocketEditorDelegate.h"
#include "IPlugStructs.h"

IWebsocketEditorDelegate::IWebsocketEditorDelegate(int nParams)
: IGraphicsEditorDelegate(nParams)
, mParamChangeFromClients(512) // TODO: constant
{
  
}

IWebsocketEditorDelegate::~IWebsocketEditorDelegate()
{
}

void IWebsocketEditorDelegate::OnWebsocketReady(int connIdx)
{
}

bool IWebsocketEditorDelegate::OnWebsocketText(int connIdx, void* pData, size_t dataSize)
{
  return true; // return true to keep the connection open
}

bool IWebsocketEditorDelegate::OnWebsocketData(int connIdx, void* pData, size_t dataSize)
{
  if (memcmp(pData, "SPVFUI" , 6) == 0) // send parameter value from user interface
  {
    int pos = 6;
    uint8_t* pByteData = (uint8_t*) pData;
    int paramIdx = * ((int*)(pByteData + pos)); pos+= 4;
    double value = * ((double*)(pByteData + pos)); pos += 8;
    mParamChangeFromClients.Push(IParamChange { paramIdx, value, true } );
  }
  else if (memcmp(pData, "SMMFUI" , 6) == 0) // send midi message from user interface
  {
  }
  else if (memcmp(pData, "SSMFUI" , 6) == 0) // send sysex message from user interface
  {
  }
  else if (memcmp(pData, "SAMFUI" , 6) == 0) // send arbitrary message from user interface
  {
  }
  
  //TODO: should now echo message to other clients
  
  return true;
}


//void IWebsocketEditorDelegate::BeginInformHostOfParamChangeFromUI(int paramIdx)
//{
//  IGraphicsEditorDelegate::BeginInformHostOfParamChangeFromUI(paramIdx);
//}

void IWebsocketEditorDelegate::SendParameterValueFromUI(int paramIdx, double normalizedValue)
{
  IByteChunk data; // TODO: this is dumb allocating/copying memory
  data.PutStr("SPVFD");
  data.Put(&paramIdx);
  data.Put(&normalizedValue);
  
  // Server side UI edit, send to clients
  SendDataToConnection(-1, data.GetBytes(), data.Size());

  IGraphicsEditorDelegate::SendParameterValueFromUI(paramIdx, normalizedValue);
}

//void IWebsocketEditorDelegate::EndInformHostOfParamChangeFromUI(int paramIdx)
//{
//  IGraphicsEditorDelegate::EndInformHostOfParamChangeFromUI(paramIdx);
//}

void IWebsocketEditorDelegate::SendControlValueFromDelegate(int controlTag, double normalizedValue)
{
  IByteChunk data; // TODO: this is dumb allocating/copying memory
  data.PutStr("SCVFD");
  data.Put(&controlTag);
  data.Put(&normalizedValue);
  
  SendDataToConnection(-1, data.GetBytes(), data.Size());
  
  IGraphicsEditorDelegate::SendControlValueFromDelegate(controlTag, normalizedValue);
}

void IWebsocketEditorDelegate::SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, const void* pData)
{
  IByteChunk data; // TODO: this is dumb allocating/copying memory
  data.PutStr("SCMFD");
  data.Put(&controlTag);
  data.Put(&messageTag);
  data.Put(&dataSize);
  data.PutBytes(pData, dataSize);
  
  SendDataToConnection(-1, data.GetBytes(), data.Size());
  
  IGraphicsEditorDelegate::SendControlMsgFromDelegate(controlTag, messageTag, dataSize, pData);
}

void IWebsocketEditorDelegate::SendArbitraryMsgFromDelegate(int messageTag, int dataSize, const void* pData)
{
  IByteChunk data; // TODO: this is dumb allocating/copying memory
  data.PutStr("SAMFD");
  data.Put(&messageTag);
  data.Put(&dataSize);
  data.PutBytes(pData, dataSize);
  
  SendDataToConnection(-1, data.GetBytes(), data.Size());
  
  IGraphicsEditorDelegate::SendArbitraryMsgFromDelegate(messageTag, dataSize, pData);
}

void IWebsocketEditorDelegate::SendMidiMsgFromDelegate(const IMidiMsg& msg)
{
  IByteChunk data; // TODO: this is dumb allocating/copying memory
  data.PutStr("SMMFD");
  data.Put(&msg.mStatus);
  data.Put(&msg.mData1);
  data.Put(&msg.mData2);

  SendDataToConnection(-1, data.GetBytes(), data.Size());
  
  IGraphicsEditorDelegate::SendMidiMsgFromDelegate(msg);
}

void IWebsocketEditorDelegate::SendSysexMsgFromDelegate(const ISysEx& msg)
{
  IByteChunk data; // TODO: this is dumb allocating/copying memory
  data.PutStr("SSMFD");
  data.Put(&msg.mSize);
  data.PutBytes(msg.mData, msg.mSize);
  
  SendDataToConnection(-1, data.GetBytes(), data.Size());
  
  IGraphicsEditorDelegate::SendSysexMsgFromDelegate(msg);
}

void IWebsocketEditorDelegate::ProcessWebsocketQueue()
{
  while(mParamChangeFromClients.ElementsAvailable())
  {
    IParamChange p;
    mParamChangeFromClients.Pop(p);
    SendParameterValueFromDelegate(p.paramIdx, p.value, p.normalized); // TODO:  if the parameter hasn't changed maybe we shouldn't do anything?
  }
}

