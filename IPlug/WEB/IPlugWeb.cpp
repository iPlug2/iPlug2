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

#include "IPlugWeb.h"
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace emscripten;

const int kNumMsgTagBytes = 6;
const int kNumSPVFUIBytes = 18;
const int kNumSMMFUIBytes = 9;
const int kNumSSMFUIBytes = 10; // + data size
const int kNumSAMFUIBytes = 14; // + data size

IPlugWeb::IPlugWeb(IPlugInstanceInfo instanceInfo, IPlugConfig config)
: IPlugAPIBase(config, kAPIWEB)
{
#if WEBSOCKET_CLIENT
  mSPVFUIBuf.Resize(kNumSPVFUIBytes); memcpy(mSPVFUIBuf.GetBytes(), "SPVFUI", kNumMsgTagBytes);
  mSMMFUIBuf.Resize(kNumSMMFUIBytes); memcpy(mSMMFUIBuf.GetBytes(), "SMMFUI", kNumMsgTagBytes);
  mSSMFUIBuf.Resize(kNumSSMFUIBytes); memcpy(mSSMFUIBuf.GetBytes(), "SSMFUI", kNumMsgTagBytes);
  mSAMFUIBuf.Resize(kNumSAMFUIBytes); memcpy(mSAMFUIBuf.GetBytes(), "SAMFUI", kNumMsgTagBytes);
#endif
  
  mWAMCtrlrJSObjectName.SetFormatted(32, "%s_WAM", GetPluginName());
}

void IPlugWeb::SendParameterValueFromUI(int paramIdx, double value)
{
#if WEBSOCKET_CLIENT
  int pos = kNumMsgTagBytes;
  *((int*)(mSPVFUIBuf.GetBytes() + pos)) = paramIdx; pos += sizeof(int);
  *((double*)(mSPVFUIBuf.GetBytes() + pos)) = value; pos += sizeof(double);
  
  EM_ASM({
    var jsbuff = Module.HEAPU8.subarray($0, $0 + $1);
    ws.send(jsbuff);
  }, (int) mSPVFUIBuf.GetBytes(), kNumSPVFUIBytes);
  
#else
  val::global(mWAMCtrlrJSObjectName.Get()).call<void>("setParam", paramIdx, value);
#endif
  IPlugAPIBase::SendParameterValueFromUI(paramIdx, value); // call super class in order to make sure OnParamChangeUI() gets triggered
};

void IPlugWeb::SendMidiMsgFromUI(const IMidiMsg& msg)
{
#if WEBSOCKET_CLIENT
  int pos = kNumMsgTagBytes;
  mSMMFUIBuf.GetBytes()[pos] = msg.mStatus; pos++;
  mSMMFUIBuf.GetBytes()[pos] = msg.mData1; pos++;
  mSMMFUIBuf.GetBytes()[pos] = msg.mData2; pos++;

  EM_ASM({
    var jsbuff = Module.HEAPU8.subarray($0, $0 + $1);
    ws.send(jsbuff);
  }, (int) mSMMFUIBuf.GetBytes(), kNumSMMFUIBytes);
  
#else
  WDL_String dataStr;
  dataStr.SetFormatted(16, "%i:%i:%i", msg.mStatus, msg.mData1, msg.mData2);
  val::global(mWAMCtrlrJSObjectName.Get()).call<void>("sendMessage", std::string("SMMFUI"), std::string(dataStr.Get()));
#endif
}

void IPlugWeb::SendSysexMsgFromUI(const ISysEx& msg)
{
#if WEBSOCKET_CLIENT
  mSSMFUIBuf.Resize(kNumSSMFUIBytes + msg.mSize);
  int pos = kNumMsgTagBytes;
  
  *((int*)(mSSMFUIBuf.GetBytes() + pos)) = msg.mSize; pos += sizeof(int);
  memcpy(mSSMFUIBuf.GetBytes() + pos, msg.mData, msg.mSize);

  EM_ASM({
    var jsbuff = Module.HEAPU8.subarray($0, $0 + $1);
    ws.send(jsbuff);
  }, (int) mSSMFUIBuf.GetBytes(), mSSMFUIBuf.Size());
#else
  EM_ASM({
    var jsbuff = Module.HEAPU8.subarray($2, $2 + $1);
    $0.sendMessage(jsbuff, 'SSMFUI', $1, jsbuf);
  }, mWAMCtrlrJSObjectName.Get(), msg.mSize, (int) msg.mData);
#endif
}

void IPlugWeb::SendArbitraryMsgFromUI(int messageTag, int dataSize, const void* pData)
{
#if WEBSOCKET_CLIENT
  mSAMFUIBuf.Resize(kNumSAMFUIBytes + dataSize);
  int pos = kNumMsgTagBytes;
  
  *((int*)(mSAMFUIBuf.GetBytes() + pos)) = messageTag; pos += sizeof(int);
  *((int*)(mSAMFUIBuf.GetBytes() + pos)) = dataSize; pos += sizeof(int);

  memcpy(mSAMFUIBuf.GetBytes() + pos, pData, dataSize);
  
  EM_ASM({
    var jsbuff = Module.HEAPU8.subarray($0, $0 + $1);
    ws.send(jsbuff);
  }, (int) mSAMFUIBuf.GetBytes(), mSAMFUIBuf.Size());
#else
  EM_ASM({
    var jsbuff = Module.HEAPU8.subarray($3, $3 + $2);
    $0.sendMessage(jsbuff, 'SAMFUI', $1, $2, jsbuf);
  }, mWAMCtrlrJSObjectName.Get(), messageTag, dataSize, (int) pData);
#endif
}

extern IPlugWeb* gPlug;

// could probably do this without these extra functions
// https://kripken.github.io/emscripten-site/docs/porting/connecting_cpp_and_javascript/embind.html#deriving-from-c-classes-in-javascript
void _SendArbitraryMsgFromDelegate(int messageTag, int dataSize, uintptr_t pData)
{
  const uint8_t* pDataPtr = reinterpret_cast<uint8_t*>(pData); // embind doesn't allow us to pass raw pointers
  gPlug->SendArbitraryMsgFromDelegate(messageTag, dataSize, pDataPtr);
}

void _SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, uintptr_t pData)
{
  const uint8_t* pDataPtr = reinterpret_cast<uint8_t*>(pData); // embind doesn't allow us to pass raw pointers
  gPlug->SendControlMsgFromDelegate(controlTag, messageTag, dataSize, pDataPtr);
}

void _SendControlValueFromDelegate(int controlTag, double normalizedValue)
{
  gPlug->SendControlValueFromDelegate(controlTag, normalizedValue);
}

void _SendParameterValueFromDelegate(int paramIdx, double normalizedValue)
{
  gPlug->SendParameterValueFromDelegate(paramIdx, normalizedValue, true);
}

void _SendMidiMsgFromDelegate(int status, int data1, int data2)
{
  IMidiMsg msg {0, (uint8_t) status, (uint8_t) data1, (uint8_t) data2};
  gPlug->SendMidiMsgFromDelegate(msg);
}

void _SendSysexMsgFromDelegate(int messageTag, int dataSize, uintptr_t pData)
{
  const uint8_t* pDataPtr = reinterpret_cast<uint8_t*>(pData); // embind doesn't allow us to pass raw pointers
  ISysEx msg(0, pDataPtr, dataSize);
  gPlug->SendSysexMsgFromDelegate(msg);
}

EMSCRIPTEN_BINDINGS(IPlugWeb) {
  function("SPVFD", &_SendParameterValueFromDelegate);
  function("SMAFD", &_SendArbitraryMsgFromDelegate);
  function("SCMFD", &_SendControlMsgFromDelegate);
  function("SCVFD", &_SendControlValueFromDelegate);
  function("SMMFD", &_SendMidiMsgFromDelegate);
  function("SSMFD", &_SendSysexMsgFromDelegate);
}
