/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IPlugWasmDSP.h"

#include <emscripten.h>
#include <emscripten/bind.h>

using namespace iplug;
using namespace emscripten;

// Global instance pointer for Emscripten bindings
static IPlugWasmDSP* sInstance = nullptr;

IPlugWasmDSP::IPlugWasmDSP(const InstanceInfo& info, const Config& config)
: IPlugAPIBase(config, kAPIWAM) // Reuse WAM API type for compatibility
, IPlugProcessor(config, kAPIWAM)
{
  sInstance = this;

  int nInputs = MaxNChannels(ERoute::kInput);
  int nOutputs = MaxNChannels(ERoute::kOutput);

  SetChannelConnections(ERoute::kInput, 0, nInputs, true);
  SetChannelConnections(ERoute::kOutput, 0, nOutputs, true);
}

void IPlugWasmDSP::Init(int sampleRate, int blockSize)
{
  DBGMSG("IPlugWasmDSP::Init(%d, %d)\n", sampleRate, blockSize);

  SetSampleRate(sampleRate);
  SetBlockSize(blockSize);

  OnParamReset(kReset);
  OnReset();
}

void IPlugWasmDSP::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), !IsInstrument());
  SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true);
  AttachBuffers(ERoute::kInput, 0, NChannelsConnected(ERoute::kInput), inputs, nFrames);
  AttachBuffers(ERoute::kOutput, 0, NChannelsConnected(ERoute::kOutput), outputs, nFrames);

  ENTER_PARAMS_MUTEX
  ProcessBuffers(0.0f, nFrames);
  LEAVE_PARAMS_MUTEX
}

void IPlugWasmDSP::OnIdleTick()
{
  // Flush queued parameter changes from DSP to UI
  while (mParamChangeFromProcessor.ElementsAvailable())
  {
    ParamTuple p;
    mParamChangeFromProcessor.Pop(p);
    SendParameterValueFromDelegate(p.idx, p.value, false);
  }

  // Flush queued MIDI messages from DSP to UI
  while (mMidiMsgsFromProcessor.ElementsAvailable())
  {
    IMidiMsg msg;
    mMidiMsgsFromProcessor.Pop(msg);
    SendMidiMsgFromDelegate(msg);
  }

  OnIdle();
}

void IPlugWasmDSP::OnParamMessage(int paramIdx, double value)
{
  ENTER_PARAMS_MUTEX
  SetParameterValue(paramIdx, value);
  LEAVE_PARAMS_MUTEX
}

void IPlugWasmDSP::OnMidiMessage(int status, int data1, int data2)
{
  IMidiMsg msg = {0, (uint8_t)status, (uint8_t)data1, (uint8_t)data2};
  ProcessMidiMsg(msg);
}

void IPlugWasmDSP::OnSysexMessage(const uint8_t* pData, int size)
{
  ISysEx sysex = {0, pData, size};
  ProcessSysEx(sysex);
}

void IPlugWasmDSP::OnArbitraryMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  OnMessage(msgTag, ctrlTag, dataSize, pData);
}

bool IPlugWasmDSP::SendMidiMsg(const IMidiMsg& msg)
{
  // Queue MIDI message to be sent to UI on idle tick
  mMidiMsgsFromProcessor.Push(msg);
  return true;
}

bool IPlugWasmDSP::SendSysEx(const ISysEx& msg)
{
  // Post SysEx to UI via Module.port (set by processor)
  EM_ASM({
    var data = new Uint8Array($1);
    data.set(HEAPU8.subarray($0, $0 + $1));
    Module.port.postMessage({
      verb: 'SSMFD',
      data: data.buffer
    });
  }, reinterpret_cast<intptr_t>(msg.mData), msg.mSize);
  return true;
}

void IPlugWasmDSP::SendControlValueFromDelegate(int ctrlTag, double normalizedValue)
{
  // Try SAB first for low-latency visualization data
  bool usedSAB = EM_ASM_INT({
    if (Module.processor && Module.processor.sabBuffer) {
      // Pack value as float
      var ptr = Module._malloc(4);
      Module.HEAPF32[ptr >> 2] = $1;
      var result = Module.processor._writeSABMessage(0, $0, 0, ptr, 4);
      Module._free(ptr);
      return result ? 1 : 0;
    }
    return 0;
  }, ctrlTag, normalizedValue);

  // Fallback to postMessage
  if (!usedSAB) {
    EM_ASM({
      Module.port.postMessage({
        verb: 'SCVFD',
        ctrlTag: $0,
        value: $1
      });
    }, ctrlTag, normalizedValue);
  }
}

void IPlugWasmDSP::SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData)
{
  // Try SAB first for low-latency visualization data
  bool usedSAB = false;
  if (dataSize > 0 && pData)
  {
    usedSAB = EM_ASM_INT({
      if (Module.processor && Module.processor.sabBuffer) {
        return Module.processor._writeSABMessage(1, $0, $1, $2, $3) ? 1 : 0;
      }
      return 0;
    }, ctrlTag, msgTag, reinterpret_cast<intptr_t>(pData), dataSize);
  }

  // Fallback to postMessage
  if (!usedSAB)
  {
    if (dataSize > 0 && pData)
    {
      EM_ASM({
        var data = new Uint8Array($3);
        data.set(HEAPU8.subarray($2, $2 + $3));
        Module.port.postMessage({
          verb: 'SCMFD',
          ctrlTag: $0,
          msgTag: $1,
          data: data.buffer
        });
      }, ctrlTag, msgTag, reinterpret_cast<intptr_t>(pData), dataSize);
    }
    else
    {
      EM_ASM({
        Module.port.postMessage({
          verb: 'SCMFD',
          ctrlTag: $0,
          msgTag: $1,
          data: null
        });
      }, ctrlTag, msgTag);
    }
  }
}

void IPlugWasmDSP::SendParameterValueFromDelegate(int paramIdx, double value, bool normalized)
{
  EM_ASM({
    Module.port.postMessage({
      verb: 'SPVFD',
      paramIdx: $0,
      value: $1
    });
  }, paramIdx, value);
}

void IPlugWasmDSP::SendArbitraryMsgFromDelegate(int msgTag, int dataSize, const void* pData)
{
  // Try SAB first for low-latency visualization data
  bool usedSAB = false;
  if (dataSize > 0 && pData)
  {
    usedSAB = EM_ASM_INT({
      if (Module.processor && Module.processor.sabBuffer) {
        return Module.processor._writeSABMessage(2, 0, $0, $1, $2) ? 1 : 0;
      }
      return 0;
    }, msgTag, reinterpret_cast<intptr_t>(pData), dataSize);
  }

  // Fallback to postMessage
  if (!usedSAB)
  {
    if (dataSize > 0 && pData)
    {
      EM_ASM({
        var data = new Uint8Array($2);
        data.set(HEAPU8.subarray($1, $1 + $2));
        Module.port.postMessage({
          verb: 'SAMFD',
          msgTag: $0,
          data: data.buffer
        });
      }, msgTag, reinterpret_cast<intptr_t>(pData), dataSize);
    }
    else
    {
      EM_ASM({
        Module.port.postMessage({
          verb: 'SAMFD',
          msgTag: $0,
          data: null
        });
      }, msgTag);
    }
  }
}

// Forward declaration for MakePlug
BEGIN_IPLUG_NAMESPACE
extern IPlugWasmDSP* MakePlug(const InstanceInfo& info);
END_IPLUG_NAMESPACE

// Static wrapper functions for Emscripten bindings
static void _init(int sampleRate, int blockSize)
{
  // Create the plugin instance if it doesn't exist yet
  if (!sInstance)
  {
    iplug::MakePlug(iplug::InstanceInfo());
    // Constructor sets sInstance = this
  }

  if (sInstance) sInstance->Init(sampleRate, blockSize);
}

static void _processBlock(uintptr_t inputPtrs, uintptr_t outputPtrs, int nFrames)
{
  if (sInstance)
  {
    sample** inputs = reinterpret_cast<sample**>(inputPtrs);
    sample** outputs = reinterpret_cast<sample**>(outputPtrs);
    sInstance->ProcessBlock(inputs, outputs, nFrames);
  }
}

static void _onParam(int paramIdx, double value)
{
  if (sInstance) sInstance->OnParamMessage(paramIdx, value);
}

static void _onMidi(int status, int data1, int data2)
{
  if (sInstance) sInstance->OnMidiMessage(status, data1, data2);
}

static void _onSysex(uintptr_t pData, int size)
{
  if (sInstance)
  {
    const uint8_t* pDataPtr = reinterpret_cast<const uint8_t*>(pData);
    sInstance->OnSysexMessage(pDataPtr, size);
  }
}

static void _onArbitraryMsg(int msgTag, int ctrlTag, int dataSize, uintptr_t pData)
{
  if (sInstance)
  {
    const void* pDataPtr = reinterpret_cast<const void*>(pData);
    sInstance->OnArbitraryMessage(msgTag, ctrlTag, dataSize, pDataPtr);
  }
}

static void _onIdleTick()
{
  if (sInstance) sInstance->OnIdleTick();
}

static int _getNumInputChannels()
{
  return sInstance ? sInstance->GetNumInputChannels() : 0;
}

static int _getNumOutputChannels()
{
  return sInstance ? sInstance->GetNumOutputChannels() : 0;
}

static bool _isInstrument()
{
  return sInstance ? sInstance->IsPlugInstrument() : false;
}

static int _getNumParams()
{
  return sInstance ? sInstance->NParams() : 0;
}

static double _getParamDefault(int paramIdx)
{
  if (sInstance && paramIdx >= 0 && paramIdx < sInstance->NParams())
    return sInstance->GetParam(paramIdx)->GetDefault(true);
  return 0.0;
}

static std::string _getParamName(int paramIdx)
{
  if (sInstance && paramIdx >= 0 && paramIdx < sInstance->NParams())
    return sInstance->GetParam(paramIdx)->GetName();
  return "";
}

static std::string _getParamLabel(int paramIdx)
{
  if (sInstance && paramIdx >= 0 && paramIdx < sInstance->NParams())
    return sInstance->GetParam(paramIdx)->GetLabel();
  return "";
}

static double _getParamMin(int paramIdx)
{
  if (sInstance && paramIdx >= 0 && paramIdx < sInstance->NParams())
    return sInstance->GetParam(paramIdx)->GetMin();
  return 0.0;
}

static double _getParamMax(int paramIdx)
{
  if (sInstance && paramIdx >= 0 && paramIdx < sInstance->NParams())
    return sInstance->GetParam(paramIdx)->GetMax();
  return 1.0;
}

static double _getParamStep(int paramIdx)
{
  if (sInstance && paramIdx >= 0 && paramIdx < sInstance->NParams())
    return sInstance->GetParam(paramIdx)->GetStep();
  return 0.001;
}

static double _getParamValue(int paramIdx)
{
  if (sInstance && paramIdx >= 0 && paramIdx < sInstance->NParams())
    return sInstance->GetParam(paramIdx)->Value();
  return 0.0;
}

static std::string _getPluginName()
{
  return sInstance ? sInstance->GetPluginName() : "";
}

static std::string _getPluginInfoJSON()
{
  if (!sInstance) return "{}";

  std::string json = "{";
  json += "\"name\":\"" + std::string(sInstance->GetPluginName()) + "\",";
  json += "\"numInputChannels\":" + std::to_string(sInstance->GetNumInputChannels()) + ",";
  json += "\"numOutputChannels\":" + std::to_string(sInstance->GetNumOutputChannels()) + ",";
  json += "\"isInstrument\":" + std::string(sInstance->IsPlugInstrument() ? "true" : "false") + ",";
  json += "\"params\":[";

  int nParams = sInstance->NParams();
  for (int i = 0; i < nParams; i++)
  {
    IParam* pParam = sInstance->GetParam(i);
    if (i > 0) json += ",";
    json += "{";
    json += "\"idx\":" + std::to_string(i) + ",";
    json += "\"name\":\"" + std::string(pParam->GetName()) + "\",";
    json += "\"label\":\"" + std::string(pParam->GetLabel()) + "\",";
    json += "\"min\":" + std::to_string(pParam->GetMin()) + ",";
    json += "\"max\":" + std::to_string(pParam->GetMax()) + ",";
    json += "\"default\":" + std::to_string(pParam->GetDefault()) + ",";
    json += "\"step\":" + std::to_string(pParam->GetStep()) + ",";
    json += "\"value\":" + std::to_string(pParam->Value());
    json += "}";
  }

  json += "]}";
  return json;
}

EMSCRIPTEN_BINDINGS(IPlugWasmDSP) {
  function("init", &_init);
  function("processBlock", &_processBlock);
  function("onParam", &_onParam);
  function("onMidi", &_onMidi);
  function("onSysex", &_onSysex);
  function("onArbitraryMsg", &_onArbitraryMsg);
  function("onIdleTick", &_onIdleTick);
  function("getNumInputChannels", &_getNumInputChannels);
  function("getNumOutputChannels", &_getNumOutputChannels);
  function("isInstrument", &_isInstrument);
  function("getNumParams", &_getNumParams);
  function("getParamDefault", &_getParamDefault);
  function("getParamName", &_getParamName);
  function("getParamLabel", &_getParamLabel);
  function("getParamMin", &_getParamMin);
  function("getParamMax", &_getParamMax);
  function("getParamStep", &_getParamStep);
  function("getParamValue", &_getParamValue);
  function("getPluginName", &_getPluginName);
  function("getPluginInfoJSON", &_getPluginInfoJSON);
}
