#include "IWebsocketEditorDelegate.h"
#include "IPlugStructs.h"

IWebsocketEditorDelegate::IWebsocketEditorDelegate(int nParams)
: IGraphicsEditorDelegate(nParams)
{
}

IWebsocketEditorDelegate::~IWebsocketEditorDelegate()
{
}

void IWebsocketEditorDelegate::OnWebsocketReady(int idx)
{
}

bool IWebsocketEditorDelegate::OnWebsocketText(int idx, void* pData, size_t dataSize)
{
  return true; // return true to keep the connection open
}

bool IWebsocketEditorDelegate::OnWebsocketData(int idx, void* pData, size_t dataSize)
{
  return true; // return true to keep the connection open
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


