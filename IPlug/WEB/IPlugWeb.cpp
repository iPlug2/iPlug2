/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IPlugWeb.h"

#include <memory>

#include <emscripten.h>
#include <emscripten/bind.h>

using namespace iplug;
using namespace emscripten;

static const int kNumMsgHeaderBytes = 6;
static const int kNumSPVFUIBytes = 18;
static const int kNumSMMFUIBytes = 9;
static const int kNumSSMFUIBytes = 10; // + data size
static const int kNumSAMFUIBytes = 18; // + data size

IPlugWeb::IPlugWeb(const InstanceInfo& info, const Config& config)
: IPlugAPIBase(config, kAPIWEB)
{
  mSPVFUIBuf.Resize(kNumSPVFUIBytes); memcpy(mSPVFUIBuf.GetData(), "SPVFUI", kNumMsgHeaderBytes);
  mSMMFUIBuf.Resize(kNumSMMFUIBytes); memcpy(mSMMFUIBuf.GetData(), "SMMFUI", kNumMsgHeaderBytes);
  mSSMFUIBuf.Resize(kNumSSMFUIBytes); memcpy(mSSMFUIBuf.GetData(), "SSMFUI", kNumMsgHeaderBytes);
  mSAMFUIBuf.Resize(kNumSAMFUIBytes); memcpy(mSAMFUIBuf.GetData(), "SAMFUI", kNumMsgHeaderBytes);

  mWAMCtrlrJSObjectName.SetFormatted(32, "%s_WAM", GetPluginName());
}

void IPlugWeb::SendParameterValueFromUI(int paramIdx, double value)
{
#if WEBSOCKET_CLIENT
  int pos = kNumMsgHeaderBytes;
  *((int*)(mSPVFUIBuf.GetData() + pos)) = paramIdx; pos += sizeof(int);
  *((double*)(mSPVFUIBuf.GetData() + pos)) = value; pos += sizeof(double);

  EM_ASM({
    var jsbuff = Module.HEAPU8.subarray($0, $0 + $1);
    ws.send(jsbuff);
  }, (int) mSPVFUIBuf.GetData(), kNumSPVFUIBytes);

#else
  val::global(mWAMCtrlrJSObjectName.Get()).call<void>("setParam", paramIdx, value);
#endif
  IPlugAPIBase::SendParameterValueFromUI(paramIdx, value); // call super class in order to make sure OnParamChangeUI() gets triggered
};

void IPlugWeb::SendMidiMsgFromUI(const IMidiMsg& msg)
{
#if WEBSOCKET_CLIENT
  int pos = kNumMsgHeaderBytes;
  mSMMFUIBuf.GetData()[pos] = msg.mStatus; pos++;
  mSMMFUIBuf.GetData()[pos] = msg.mData1; pos++;
  mSMMFUIBuf.GetData()[pos] = msg.mData2; pos++;

  EM_ASM({
    var jsbuff = Module.HEAPU8.subarray($0, $0 + $1);
    ws.send(jsbuff);
  }, (int) mSMMFUIBuf.GetData(), kNumSMMFUIBytes);

#else
  WDL_String dataStr;
  dataStr.SetFormatted(16, "%i:%i:%i", msg.mStatus, msg.mData1, msg.mData2);
  val::global(mWAMCtrlrJSObjectName.Get()).call<void>("sendMessage", std::string("SMMFUI"), std::string(dataStr.Get()));
#endif
}

void IPlugWeb::SendSysexMsgFromUI(const ISysEx& msg)
{
  DBGMSG("TODO: SendSysexMsgFromUI");

//   EM_ASM({
//     window[Module.UTF8ToString($0)]["midiOut"].send(0x90, 0x45, 0x7f);
//   }, mWAMCtrlrJSObjectName.Get());
//  val::global(mWAMCtrlrJSObjectName.Get())["midiOut"].call<void>("send", {0x90, 0x45, 0x7f} );

// #if WEBSOCKET_CLIENT
//   mSSMFUIBuf.Resize(kNumSSMFUIBytes + msg.mSize);
//   int pos = kNumMsgHeaderBytes;
//
//   *((int*)(mSSMFUIBuf.GetData() + pos)) = msg.mSize; pos += sizeof(int);
//   memcpy(mSSMFUIBuf.GetData() + pos, msg.mData, msg.mSize);
//
//   EM_ASM({
//     var jsbuff = Module.HEAPU8.subarray($0, $0 + $1);
//     ws.send(jsbuff);
//   }, (int) mSSMFUIBuf.GetData(), mSSMFUIBuf.Size());
// #else
//   EM_ASM({
//     window[Module.UTF8ToString($0)].sendMessage('SSMFUI', $1, Module.HEAPU8.slice($1, $1 + $2).buffer);
//   }, mWAMCtrlrJSObjectName.Get(), (int) msg.mData, msg.mSize);
// #endif
}

void IPlugWeb::SendArbitraryMsgFromUI(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  mSAMFUIBuf.Resize(kNumSAMFUIBytes + dataSize);
  int pos = kNumMsgHeaderBytes;

  *((int*)(mSAMFUIBuf.GetData() + pos)) = msgTag; pos += sizeof(int);
  *((int*)(mSAMFUIBuf.GetData() + pos)) = ctrlTag; pos += sizeof(int);
  *((int*)(mSAMFUIBuf.GetData() + pos)) = dataSize; pos += sizeof(int);

  memcpy(mSAMFUIBuf.GetData() + pos, pData, dataSize);

#if WEBSOCKET_CLIENT
  EM_ASM({
    var jsbuff = Module.HEAPU8.subarray($0, $0 + $1);
    ws.send(jsbuff);
  }, (int) mSAMFUIBuf.GetData(), mSAMFUIBuf.Size());
#else
  EM_ASM({
    window[Module.UTF8ToString($0)].sendMessage('SAMFUI', "", Module.HEAPU8.slice($1, $1 + $2).buffer);
  }, mWAMCtrlrJSObjectName.Get(), (int) mSAMFUIBuf.GetData() + kNumMsgHeaderBytes, mSAMFUIBuf.Size() - kNumMsgHeaderBytes); // Non websocket doesn't need "SAMFUI" bytes at beginning
#endif
}

extern std::unique_ptr<IPlugWeb> gPlug;

// could probably do this without these extra functions
// https://kripken.github.io/emscripten-site/docs/porting/connecting_cpp_and_javascript/embind.html#deriving-from-c-classes-in-javascript
static void _SendArbitraryMsgFromDelegate(int msgTag, int dataSize, uintptr_t pData)
{
  const uint8_t* pDataPtr = reinterpret_cast<uint8_t*>(pData); // embind doesn't allow us to pass raw pointers
  gPlug->SendArbitraryMsgFromDelegate(msgTag, dataSize, pDataPtr);
}

static void _SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, uintptr_t pData)
{
  const uint8_t* pDataPtr = reinterpret_cast<uint8_t*>(pData); // embind doesn't allow us to pass raw pointers
  gPlug->SendControlMsgFromDelegate(ctrlTag, msgTag, dataSize, pDataPtr);
}

static void _SendControlValueFromDelegate(int ctrlTag, double normalizedValue)
{
  gPlug->SendControlValueFromDelegate(ctrlTag, normalizedValue);
}

static void _SendParameterValueFromDelegate(int paramIdx, double normalizedValue)
{
  gPlug->SendParameterValueFromDelegate(paramIdx, normalizedValue, true);
}

static void _SendMidiMsgFromDelegate(int status, int data1, int data2)
{
  IMidiMsg msg {0, (uint8_t) status, (uint8_t) data1, (uint8_t) data2};
  gPlug->SendMidiMsgFromDelegate(msg);
}

static void _SendSysexMsgFromDelegate(int dataSize, uintptr_t pData)
{
  const uint8_t* pDataPtr = reinterpret_cast<uint8_t*>(pData); // embind doesn't allow us to pass raw pointers
  ISysEx msg(0, pDataPtr, dataSize);
  gPlug->SendSysexMsgFromDelegate(msg);
}

EMSCRIPTEN_BINDINGS(IPlugWeb) {
  function("SPVFD", &_SendParameterValueFromDelegate);
  function("SAMFD", &_SendArbitraryMsgFromDelegate);
  function("SCMFD", &_SendControlMsgFromDelegate);
  function("SCVFD", &_SendControlValueFromDelegate);
  function("SMMFD", &_SendMidiMsgFromDelegate);
  function("SSMFD", &_SendSysexMsgFromDelegate);
}
