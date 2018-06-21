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
#include <emscripten/bind.h>

using namespace emscripten;

IPlugWeb::IPlugWeb(IPlugInstanceInfo instanceInfo, IPlugConfig config)
: IPlugAPIBase(config, kAPIWEB)
{
  mWAMCtrlrJSObjectName.SetFormatted(32, "%s_WAM", GetPluginName());
}

void IPlugWeb::SendParameterValueFromUI(int paramIdx, double value)
{
#ifdef WEBSOCKET_CLIENT
//  val buffer = val::global("Function").new_(std::string("Uint8Array"), 12 ); // 12 bytes
//  buffer.call<void>("setInt32", paramIdx);
//  buffer.call<void>("setFloat64", value);
#else
  val::global(mWAMCtrlrJSObjectName.Get()).call<void>("setParam", paramIdx, value);
#endif
  IPlugAPIBase::SendParameterValueFromUI(paramIdx, value); // call super class in order to make sure OnParamChangeUI() gets triggered
};

void IPlugWeb::SendMidiMsgFromUI(const IMidiMsg& msg)
{
  WDL_String dataStr;
  dataStr.SetFormatted(16, "%i:%i:%i", msg.mStatus, msg.mData1, msg.mData2);

#ifdef WEBSOCKET_CLIENT
#else
  val::global(mWAMCtrlrJSObjectName.Get()).call<void>("sendMessage", std::string("SMMFUI"), std::string(dataStr.Get()));
#endif
}

void IPlugWeb::SendArbitraryMsgFromUI(int messageTag, int dataSize, const void* pData)
{
#ifdef WEBSOCKET_CLIENT
  WDL_String dataStr;
  dataStr.SetFormatted(16, "%i:%i", messageTag, dataSize);
#else
  // TODO: pData not sent!
  val::global(mWAMCtrlrJSObjectName.Get()).call<void>("sendMessage", std::string("SAMFUI"), std::string(dataStr.Get()));
#endif
}

void IPlugWeb::SendSysexMsgFromUI(const ISysEx& msg)
{
#ifdef WEBSOCKET_CLIENT
#else
  WDL_String dataStr;
  dataStr.SetFormatted(16, "%i", msg.mSize);
  // TODO: msg.mData not sent!
  val::global(mWAMCtrlrJSObjectName.Get()).call<void>("sendMessage", std::string("SSMFUI"), std::string(dataStr.Get()));
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
