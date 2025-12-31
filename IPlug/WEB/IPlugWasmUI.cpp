/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IPlugWasmUI.h"

#include <memory>

#include <emscripten.h>
#include <emscripten/bind.h>

using namespace iplug;
using namespace emscripten;

IPlugWasmUI::IPlugWasmUI(const InstanceInfo& info, const Config& config)
: IPlugAPIBase(config, kAPIWEB)
{
  mControllerName.SetFormatted(32, "%s_Hybrid", GetPluginName());
}

void IPlugWasmUI::SendParameterValueFromUI(int paramIdx, double value)
{
  // Send parameter value to DSP via controller
  EM_ASM({
    if (typeof window.iPlugController !== 'undefined') {
      window.iPlugController.setParam($0, $1);
    } else {
      console.warn('IPlugWasmUI: controller not ready');
    }
  }, paramIdx, value);

  // Call super class to trigger OnParamChangeUI()
  IPlugAPIBase::SendParameterValueFromUI(paramIdx, value);
}

void IPlugWasmUI::SendMidiMsgFromUI(const IMidiMsg& msg)
{
  EM_ASM({
    if (typeof window.iPlugController !== 'undefined') {
      window.iPlugController.sendMidi($0, $1, $2);
    } else {
      console.warn('IPlugWasmUI: controller not ready');
    }
  }, msg.mStatus, msg.mData1, msg.mData2);
}

void IPlugWasmUI::SendSysexMsgFromUI(const ISysEx& msg)
{
  EM_ASM({
    if (typeof window.iPlugController !== 'undefined') {
      var data = new Uint8Array($1);
      data.set(HEAPU8.subarray($0, $0 + $1));
      window.iPlugController.sendSysex(data);
    } else {
      console.warn('IPlugWasmUI: controller not ready');
    }
  }, reinterpret_cast<intptr_t>(msg.mData), msg.mSize);
}

void IPlugWasmUI::SendArbitraryMsgFromUI(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if (dataSize > 0 && pData)
  {
    EM_ASM({
      if (typeof window.iPlugController !== 'undefined') {
        var data = new Uint8Array($2);
        data.set(HEAPU8.subarray($3, $3 + $2));
        window.iPlugController.sendArbitraryMsg($0, $1, data);
      } else {
        console.warn('IPlugWasmUI: controller not ready');
      }
    }, msgTag, ctrlTag, dataSize, reinterpret_cast<intptr_t>(pData));
  }
  else
  {
    EM_ASM({
      if (typeof window.iPlugController !== 'undefined') {
        window.iPlugController.sendArbitraryMsg($0, $1, null);
      }
    }, msgTag, ctrlTag);
  }
}

void IPlugWasmUI::SendDSPIdleTick()
{
  EM_ASM({
    if (typeof window.iPlugController !== 'undefined') {
      window.iPlugController.sendIdleTick();
    }
  });
}

// Global instance for Emscripten bindings
extern std::unique_ptr<iplug::IPlugWasmUI> gPlug;

// Callback functions called by JavaScript controller when DSP sends messages
static void _SendParameterValueFromDelegate(int paramIdx, double normalizedValue)
{
  gPlug->SendParameterValueFromDelegate(paramIdx, normalizedValue, true);
}

static void _SendControlValueFromDelegate(int ctrlTag, double normalizedValue)
{
  gPlug->SendControlValueFromDelegate(ctrlTag, normalizedValue);
}

static void _SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, uintptr_t pData)
{
  const uint8_t* pDataPtr = reinterpret_cast<uint8_t*>(pData);
  gPlug->SendControlMsgFromDelegate(ctrlTag, msgTag, dataSize, pDataPtr);
}

static void _SendArbitraryMsgFromDelegate(int msgTag, int dataSize, uintptr_t pData)
{
  const uint8_t* pDataPtr = reinterpret_cast<uint8_t*>(pData);
  gPlug->SendArbitraryMsgFromDelegate(msgTag, dataSize, pDataPtr);
}

static void _SendMidiMsgFromDelegate(int status, int data1, int data2)
{
  IMidiMsg msg{0, (uint8_t)status, (uint8_t)data1, (uint8_t)data2};
  gPlug->SendMidiMsgFromDelegate(msg);
}

static void _SendSysexMsgFromDelegate(int dataSize, uintptr_t pData)
{
  const uint8_t* pDataPtr = reinterpret_cast<uint8_t*>(pData);
  ISysEx msg(0, pDataPtr, dataSize);
  gPlug->SendSysexMsgFromDelegate(msg);
}

static void _StartIdleTimer()
{
  gPlug->CreateTimer();
}

static void _OnParentWindowResize(int width, int height)
{
  if (gPlug)
    gPlug->OnParentWindowResize(width, height);
}

EMSCRIPTEN_BINDINGS(IPlugWasmUI) {
  function("SPVFD", &_SendParameterValueFromDelegate);
  function("SCVFD", &_SendControlValueFromDelegate);
  function("SCMFD", &_SendControlMsgFromDelegate);
  function("SAMFD", &_SendArbitraryMsgFromDelegate);
  function("SMMFD", &_SendMidiMsgFromDelegate);
  function("SSMFD", &_SendSysexMsgFromDelegate);
  function("StartIdleTimer", &_StartIdleTimer);
  function("OnParentWindowResize", &_OnParentWindowResize);
}
