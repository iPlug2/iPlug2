/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IPlugWasmDSP.h"

#include <atomic>
#include <unordered_map>
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace iplug;
using namespace emscripten;

// Instance registry for multi-instance support
// Each AudioWorkletProcessor gets its own IPlugWasmDSP instance
static std::unordered_map<int, IPlugWasmDSP*> sInstances;
static std::atomic<int> sNextInstanceId{1};

// Helper to get instance by ID
static IPlugWasmDSP* GetInstance(int instanceId)
{
  auto it = sInstances.find(instanceId);
  return (it != sInstances.end()) ? it->second : nullptr;
}

IPlugWasmDSP::IPlugWasmDSP(const InstanceInfo& info, const Config& config)
: IPlugAPIBase(config, kAPIWAM) // Reuse WAM API type for compatibility
, IPlugProcessor(config, kAPIWAM)
, mInstanceId(0)
{
  int nInputs = MaxNChannels(ERoute::kInput);
  int nOutputs = MaxNChannels(ERoute::kOutput);

  SetChannelConnections(ERoute::kInput, 0, nInputs, true);
  SetChannelConnections(ERoute::kOutput, 0, nOutputs, true);
}

IPlugWasmDSP::~IPlugWasmDSP()
{
  // Remove from registry if registered
  if (mInstanceId != 0)
  {
    sInstances.erase(mInstanceId);
  }
}

void IPlugWasmDSP::SetInstanceId(int instanceId)
{
  mInstanceId = instanceId;
  if (instanceId != 0)
  {
    sInstances[instanceId] = this;
  }
}

void IPlugWasmDSP::Init(int sampleRate, int blockSize)
{
  DBGMSG("IPlugWasmDSP::Init(%d, %d) instance=%d\n", sampleRate, blockSize, mInstanceId);

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

  // ENTER_PARAMS_MUTEX/LEAVE_PARAMS_MUTEX: In single-threaded Emscripten builds
  // (without pthreads), these are no-ops. With pthreads enabled, they protect
  // parameter access during ProcessBuffers. The AudioWorklet runs on a separate
  // thread from the main thread, but parameter messages arrive via postMessage
  // which is serialized, so the mutex primarily guards against concurrent
  // parameter changes from within ProcessBuffers itself (e.g., meta-parameters).
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
  // Post SysEx to UI via instance-specific port
  EM_ASM({
    var instances = Module._instancePorts;
    if (instances && instances[$0]) {
      var data = new Uint8Array($2);
      data.set(HEAPU8.subarray($1, $1 + $2));
      instances[$0].postMessage({
        verb: 'SSMFD',
        data: data.buffer
      });
    }
  }, mInstanceId, reinterpret_cast<intptr_t>(msg.mData), msg.mSize);
  return true;
}

void IPlugWasmDSP::SendControlValueFromDelegate(int ctrlTag, double normalizedValue)
{
  // Try SAB first for low-latency visualization data
  bool usedSAB = EM_ASM_INT({
    var processors = Module._instanceProcessors;
    if (processors && processors[$0] && processors[$0].sabBuffer) {
      var proc = processors[$0];
      // Pack value as float
      var ptr = Module._malloc(4);
      Module.HEAPF32[ptr >> 2] = $2;
      var result = proc._writeSABMessage(0, $1, 0, ptr, 4);
      Module._free(ptr);
      return result ? 1 : 0;
    }
    return 0;
  }, mInstanceId, ctrlTag, normalizedValue);

  // Fallback to postMessage
  if (!usedSAB) {
    EM_ASM({
      var instances = Module._instancePorts;
      if (instances && instances[$0]) {
        instances[$0].postMessage({
          verb: 'SCVFD',
          ctrlTag: $1,
          value: $2
        });
      }
    }, mInstanceId, ctrlTag, normalizedValue);
  }
}

void IPlugWasmDSP::SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData)
{
  // Try SAB first for low-latency visualization data
  bool usedSAB = false;
  if (dataSize > 0 && pData)
  {
    usedSAB = EM_ASM_INT({
      var processors = Module._instanceProcessors;
      if (processors && processors[$0] && processors[$0].sabBuffer) {
        return processors[$0]._writeSABMessage(1, $1, $2, $3, $4) ? 1 : 0;
      }
      return 0;
    }, mInstanceId, ctrlTag, msgTag, reinterpret_cast<intptr_t>(pData), dataSize);
  }

  // Fallback to postMessage
  if (!usedSAB)
  {
    if (dataSize > 0 && pData)
    {
      EM_ASM({
        var instances = Module._instancePorts;
        if (instances && instances[$0]) {
          var data = new Uint8Array($4);
          data.set(HEAPU8.subarray($3, $3 + $4));
          instances[$0].postMessage({
            verb: 'SCMFD',
            ctrlTag: $1,
            msgTag: $2,
            data: data.buffer
          });
        }
      }, mInstanceId, ctrlTag, msgTag, reinterpret_cast<intptr_t>(pData), dataSize);
    }
    else
    {
      EM_ASM({
        var instances = Module._instancePorts;
        if (instances && instances[$0]) {
          instances[$0].postMessage({
            verb: 'SCMFD',
            ctrlTag: $1,
            msgTag: $2,
            data: null
          });
        }
      }, mInstanceId, ctrlTag, msgTag);
    }
  }
}

void IPlugWasmDSP::SendParameterValueFromDelegate(int paramIdx, double value, bool normalized)
{
  EM_ASM({
    var instances = Module._instancePorts;
    if (instances && instances[$0]) {
      instances[$0].postMessage({
        verb: 'SPVFD',
        paramIdx: $1,
        value: $2
      });
    }
  }, mInstanceId, paramIdx, value);
}

void IPlugWasmDSP::SendMidiMsgFromDelegate(const IMidiMsg& msg)
{
  EM_ASM({
    var instances = Module._instancePorts;
    if (instances && instances[$0]) {
      instances[$0].postMessage({
        verb: 'SMMFD',
        status: $1,
        data1: $2,
        data2: $3
      });
    }
  }, mInstanceId, msg.mStatus, msg.mData1, msg.mData2);
}

void IPlugWasmDSP::SendArbitraryMsgFromDelegate(int msgTag, int dataSize, const void* pData)
{
  // Try SAB first for low-latency visualization data
  bool usedSAB = false;
  if (dataSize > 0 && pData)
  {
    usedSAB = EM_ASM_INT({
      var processors = Module._instanceProcessors;
      if (processors && processors[$0] && processors[$0].sabBuffer) {
        return processors[$0]._writeSABMessage(2, 0, $1, $2, $3) ? 1 : 0;
      }
      return 0;
    }, mInstanceId, msgTag, reinterpret_cast<intptr_t>(pData), dataSize);
  }

  // Fallback to postMessage
  if (!usedSAB)
  {
    if (dataSize > 0 && pData)
    {
      EM_ASM({
        var instances = Module._instancePorts;
        if (instances && instances[$0]) {
          var data = new Uint8Array($3);
          data.set(HEAPU8.subarray($2, $2 + $3));
          instances[$0].postMessage({
            verb: 'SAMFD',
            msgTag: $1,
            data: data.buffer
          });
        }
      }, mInstanceId, msgTag, reinterpret_cast<intptr_t>(pData), dataSize);
    }
    else
    {
      EM_ASM({
        var instances = Module._instancePorts;
        if (instances && instances[$0]) {
          instances[$0].postMessage({
            verb: 'SAMFD',
            msgTag: $1,
            data: null
          });
        }
      }, mInstanceId, msgTag);
    }
  }
}

// Forward declaration for MakePlug
BEGIN_IPLUG_NAMESPACE
extern IPlugWasmDSP* MakePlug(const InstanceInfo& info);
END_IPLUG_NAMESPACE

// Static wrapper functions for Emscripten bindings
// All functions now take instanceId as first parameter for multi-instance support

/** Create a new plugin instance. Returns instance ID (>0) or 0 on failure. */
static int _createInstance()
{
  IPlugWasmDSP* pInstance = iplug::MakePlug(iplug::InstanceInfo());
  if (!pInstance) return 0;

  int instanceId = sNextInstanceId.fetch_add(1);
  pInstance->SetInstanceId(instanceId);

  // Initialize port/processor registries if needed
  EM_ASM({
    if (!Module._instancePorts) Module._instancePorts = {};
    if (!Module._instanceProcessors) Module._instanceProcessors = {};
  });

  return instanceId;
}

/** Destroy a plugin instance by ID. */
static void _destroyInstance(int instanceId)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance)
  {
    // Clean up JS references
    EM_ASM({
      if (Module._instancePorts) delete Module._instancePorts[$0];
      if (Module._instanceProcessors) delete Module._instanceProcessors[$0];
    }, instanceId);

    delete pInstance;
  }
}

static void _init(int instanceId, int sampleRate, int blockSize)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance) pInstance->Init(sampleRate, blockSize);
}

static void _processBlock(int instanceId, uintptr_t inputPtrs, uintptr_t outputPtrs, int nFrames)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance)
  {
    sample** inputs = reinterpret_cast<sample**>(inputPtrs);
    sample** outputs = reinterpret_cast<sample**>(outputPtrs);
    pInstance->ProcessBlock(inputs, outputs, nFrames);
  }
}

static void _onParam(int instanceId, int paramIdx, double value)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance) pInstance->OnParamMessage(paramIdx, value);
}

static void _onMidi(int instanceId, int status, int data1, int data2)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance) pInstance->OnMidiMessage(status, data1, data2);
}

static void _onSysex(int instanceId, uintptr_t pData, int size)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance)
  {
    const uint8_t* pDataPtr = reinterpret_cast<const uint8_t*>(pData);
    pInstance->OnSysexMessage(pDataPtr, size);
  }
}

static void _onArbitraryMsg(int instanceId, int msgTag, int ctrlTag, int dataSize, uintptr_t pData)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance)
  {
    const void* pDataPtr = reinterpret_cast<const void*>(pData);
    pInstance->OnArbitraryMessage(msgTag, ctrlTag, dataSize, pDataPtr);
  }
}

static void _onIdleTick(int instanceId)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance) pInstance->OnIdleTick();
}

static int _getNumInputChannels(int instanceId)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  return pInstance ? pInstance->GetNumInputChannels() : 0;
}

static int _getNumOutputChannels(int instanceId)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  return pInstance ? pInstance->GetNumOutputChannels() : 0;
}

static bool _isInstrument(int instanceId)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  return pInstance ? pInstance->IsPlugInstrument() : false;
}

static int _getNumParams(int instanceId)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  return pInstance ? pInstance->NParams() : 0;
}

static double _getParamDefault(int instanceId, int paramIdx)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance && paramIdx >= 0 && paramIdx < pInstance->NParams())
    return pInstance->GetParam(paramIdx)->GetDefault(true);
  return 0.0;
}

static std::string _getParamName(int instanceId, int paramIdx)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance && paramIdx >= 0 && paramIdx < pInstance->NParams())
    return pInstance->GetParam(paramIdx)->GetName();
  return "";
}

static std::string _getParamLabel(int instanceId, int paramIdx)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance && paramIdx >= 0 && paramIdx < pInstance->NParams())
    return pInstance->GetParam(paramIdx)->GetLabel();
  return "";
}

static double _getParamMin(int instanceId, int paramIdx)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance && paramIdx >= 0 && paramIdx < pInstance->NParams())
    return pInstance->GetParam(paramIdx)->GetMin();
  return 0.0;
}

static double _getParamMax(int instanceId, int paramIdx)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance && paramIdx >= 0 && paramIdx < pInstance->NParams())
    return pInstance->GetParam(paramIdx)->GetMax();
  return 1.0;
}

static double _getParamStep(int instanceId, int paramIdx)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance && paramIdx >= 0 && paramIdx < pInstance->NParams())
    return pInstance->GetParam(paramIdx)->GetStep();
  return 0.001;
}

static double _getParamValue(int instanceId, int paramIdx)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (pInstance && paramIdx >= 0 && paramIdx < pInstance->NParams())
    return pInstance->GetParam(paramIdx)->Value();
  return 0.0;
}

static std::string _getPluginName(int instanceId)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  return pInstance ? pInstance->GetPluginName() : "";
}

static std::string _getPluginInfoJSON(int instanceId)
{
  IPlugWasmDSP* pInstance = GetInstance(instanceId);
  if (!pInstance) return "{}";

  std::string json = "{";
  json += "\"instanceId\":" + std::to_string(instanceId) + ",";
  json += "\"name\":\"" + std::string(pInstance->GetPluginName()) + "\",";
  json += "\"numInputChannels\":" + std::to_string(pInstance->GetNumInputChannels()) + ",";
  json += "\"numOutputChannels\":" + std::to_string(pInstance->GetNumOutputChannels()) + ",";
  json += "\"isInstrument\":" + std::string(pInstance->IsPlugInstrument() ? "true" : "false") + ",";
  json += "\"params\":[";

  int nParams = pInstance->NParams();
  for (int i = 0; i < nParams; i++)
  {
    IParam* pParam = pInstance->GetParam(i);
    if (i > 0) json += ",";
    json += "{";
    json += "\"idx\":" + std::to_string(i) + ",";
    json += "\"id\":" + std::to_string(i) + ",";
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
  function("createInstance", &_createInstance);
  function("destroyInstance", &_destroyInstance);
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
