/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IPlugEmAudioWorklet.h"
#include <emscripten.h>
#include <emscripten/bind.h>

using namespace iplug;
using namespace emscripten;

// Global instance pointer for callbacks
static IPlugEmAudioWorklet* sInstance = nullptr;

IPlugEmAudioWorklet::IPlugEmAudioWorklet(const InstanceInfo& info, const Config& config)
: IPlugAPIBase(config, kAPIWAM) // Reuse WAM API type for compatibility
, IPlugProcessor(config, kAPIWAM)
{
  sInstance = this;

  int nInputs = MaxNChannels(ERoute::kInput);
  int nOutputs = MaxNChannels(ERoute::kOutput);

  SetChannelConnections(ERoute::kInput, 0, nInputs, true);
  SetChannelConnections(ERoute::kOutput, 0, nOutputs, true);
}

IPlugEmAudioWorklet::~IPlugEmAudioWorklet()
{
  StopAudio();
  sInstance = nullptr;
}

bool IPlugEmAudioWorklet::InitAudioWorklet(int sampleRate)
{
  DBGMSG("IPlugEmAudioWorklet::InitAudioWorklet(%d)\n", sampleRate);

  mRequestedSampleRate = sampleRate;

  // Create the audio context
  EmscriptenWebAudioCreateAttributes attrs = {
    .latencyHint = "interactive",
    .sampleRate = static_cast<uint32_t>(sampleRate)
  };

  mAudioContext = emscripten_create_audio_context(&attrs);
  if (!mAudioContext)
  {
    DBGMSG("Failed to create audio context\n");
    return false;
  }

  // Store the actual AudioContext JS object in Module for JS access
  // Emscripten stores audio objects in EmAudio array
  EM_ASM({
    Module.audioContext = emscriptenGetAudioObject($0);
  }, mAudioContext);

  // Start the worklet thread - this shares the WASM module with the audio thread
  emscripten_start_wasm_audio_worklet_thread_async(
    mAudioContext,
    mWorkletStack,
    kWorkletStackSize,
    OnWorkletThreadInitialized,
    this
  );

  return true;
}

void IPlugEmAudioWorklet::OnWorkletThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void* userData)
{
  IPlugEmAudioWorklet* pPlugin = static_cast<IPlugEmAudioWorklet*>(userData);

  if (!success)
  {
    DBGMSG("Failed to initialize audio worklet thread\n");
    return;
  }

  DBGMSG("Audio worklet thread initialized successfully\n");
  pPlugin->mWorkletThreadReady = true;

  // Create the audio worklet processor
  // The processor name must be unique per plugin type
  WebAudioWorkletProcessorCreateOptions opts = {
    .name = "iplug-processor"
  };

  emscripten_create_wasm_audio_worklet_processor_async(
    audioContext,
    &opts,
    OnProcessorCreated,
    userData
  );
}

void IPlugEmAudioWorklet::OnProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void* userData)
{
  IPlugEmAudioWorklet* pPlugin = static_cast<IPlugEmAudioWorklet*>(userData);

  if (!success)
  {
    DBGMSG("Failed to create audio worklet processor\n");
    return;
  }

  DBGMSG("Audio worklet processor created successfully\n");
  pPlugin->mProcessorCreated = true;

  // Get channel counts from plugin config
  int numInputChannels = pPlugin->MaxNChannels(ERoute::kInput);
  int numOutputChannels = pPlugin->MaxNChannels(ERoute::kOutput);

  // Create output channel count array
  int outputChannelCounts[] = { numOutputChannels };

  EmscriptenAudioWorkletNodeCreateOptions nodeOpts = {
    .numberOfInputs = numInputChannels > 0 ? 1 : 0,
    .numberOfOutputs = 1,
    .outputChannelCounts = outputChannelCounts,
  };

  // Create the audio worklet node
  pPlugin->mWorkletNode = emscripten_create_wasm_audio_worklet_node(
    audioContext,
    "iplug-processor",
    &nodeOpts,
    ProcessAudio,
    userData
  );

  if (!pPlugin->mWorkletNode)
  {
    DBGMSG("Failed to create audio worklet node\n");
    return;
  }

  // Store the actual WorkletNode JS object in Module for JS access
  EM_ASM({
    Module.workletNode = emscriptenGetAudioObject($0);
  }, pPlugin->mWorkletNode);

  DBGMSG("Audio worklet node created successfully\n");

  // Initialize the plugin processing
  // Use Emscripten's C API to get the sample rate
  int sr = emscripten_audio_context_sample_rate(audioContext);
  pPlugin->SetSampleRate(sr);
  pPlugin->SetBlockSize(128); // Web Audio uses fixed 128 sample frames

  pPlugin->OnParamReset(kReset);
  pPlugin->OnReset();

  // Notify JS that we're ready
  EM_ASM({
    if (Module.onAudioWorkletReady) {
      Module.onAudioWorkletReady();
    }
  });
}

bool IPlugEmAudioWorklet::ProcessAudio(int numInputs, const AudioSampleFrame* inputs,
                                       int numOutputs, AudioSampleFrame* outputs,
                                       int numParams, const AudioParamFrame* params,
                                       void* userData)
{
  IPlugEmAudioWorklet* pPlugin = static_cast<IPlugEmAudioWorklet*>(userData);
  if (!pPlugin)
    return true;

  const int blockSize = 128; // Fixed in Web Audio API

  // Create channel pointer arrays for planar audio data
  // AudioSampleFrame stores data as: data[channelIndex * samplesPerChannel + sampleIndex]
  static float* inputChannelPtrs[16];
  static float* outputChannelPtrs[16];

  // Set up input channel pointers
  int nInputChans = 0;
  if (numInputs > 0 && inputs && inputs[0].data)
  {
    nInputChans = std::min(inputs[0].numberOfChannels, pPlugin->MaxNChannels(ERoute::kInput));
    for (int i = 0; i < nInputChans; i++)
    {
      inputChannelPtrs[i] = &inputs[0].data[i * blockSize];
    }
  }

  // Set up output channel pointers
  int nOutputChans = 0;
  if (numOutputs > 0 && outputs && outputs[0].data)
  {
    nOutputChans = std::min(outputs[0].numberOfChannels, pPlugin->MaxNChannels(ERoute::kOutput));
    for (int i = 0; i < nOutputChans; i++)
    {
      outputChannelPtrs[i] = &outputs[0].data[i * blockSize];
    }
  }

  // Attach buffers
  if (nInputChans > 0)
    pPlugin->AttachBuffers(ERoute::kInput, 0, nInputChans, inputChannelPtrs, blockSize);

  if (nOutputChans > 0)
    pPlugin->AttachBuffers(ERoute::kOutput, 0, nOutputChans, outputChannelPtrs, blockSize);

  // Process audio
  ENTER_PARAMS_MUTEX
  pPlugin->ProcessBuffers(0.0f, blockSize);
  LEAVE_PARAMS_MUTEX

  return true; // Return true to keep processing
}

void IPlugEmAudioWorklet::StartAudio()
{
  if (mAudioStarted || !mWorkletNode || !mAudioContext)
    return;

  // Connect the worklet node to the audio context destination
  emscripten_audio_node_connect(mWorkletNode, mAudioContext, 0, 0);
  mAudioStarted = true;

  DBGMSG("Audio started\n");
}

void IPlugEmAudioWorklet::StopAudio()
{
  if (!mAudioStarted)
    return;

  // Disconnect would go here if needed
  mAudioStarted = false;

  DBGMSG("Audio stopped\n");
}

void IPlugEmAudioWorklet::OnEditorIdleTick()
{
  while (mParamChangeFromProcessor.ElementsAvailable())
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
}

bool IPlugEmAudioWorklet::SendMidiMsg(const IMidiMsg& msg)
{
  // Queue MIDI message to be sent to UI on idle tick
  mMidiMsgsFromProcessor.Push(msg);
  return true;
}

bool IPlugEmAudioWorklet::SendSysEx(const ISysEx& msg)
{
  // TODO: Implement SysEx
  return false;
}

void IPlugEmAudioWorklet::SetParameterValueFromUI(int paramIdx, double value)
{
  ENTER_PARAMS_MUTEX
  SetParameterValue(paramIdx, value);
  LEAVE_PARAMS_MUTEX
}

void IPlugEmAudioWorklet::SendMidiMsgFromUI(const IMidiMsg& msg)
{
  // Process MIDI message directly (same as IPlugWAM)
  ProcessMidiMsg(msg);
}

// IEditorDelegate methods - delegate to base class (IGEditorDelegate) for IGraphics control updates
void IPlugEmAudioWorklet::SendControlValueFromDelegate(int ctrlTag, double normalizedValue)
{
  EDITOR_DELEGATE_CLASS::SendControlValueFromDelegate(ctrlTag, normalizedValue);
}

void IPlugEmAudioWorklet::SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData)
{
  EDITOR_DELEGATE_CLASS::SendControlMsgFromDelegate(ctrlTag, msgTag, dataSize, pData);
}

void IPlugEmAudioWorklet::SendParameterValueFromDelegate(int paramIdx, double value, bool normalized)
{
  EDITOR_DELEGATE_CLASS::SendParameterValueFromDelegate(paramIdx, value, normalized);
}

void IPlugEmAudioWorklet::SendArbitraryMsgFromDelegate(int msgTag, int dataSize, const void* pData)
{
  EDITOR_DELEGATE_CLASS::SendArbitraryMsgFromDelegate(msgTag, dataSize, pData);
}

// Emscripten bindings for JS to call C++ functions
extern std::unique_ptr<IPlugEmAudioWorklet> gPlugAudioWorklet;

static void _SetParameterValue(int paramIdx, double value)
{
  if (sInstance) sInstance->SetParameterValueFromUI(paramIdx, value);
}

static void _SendMidiMsg(int status, int data1, int data2)
{
  if (sInstance)
  {
    IMidiMsg msg{0, (uint8_t)status, (uint8_t)data1, (uint8_t)data2};
    sInstance->SendMidiMsgFromUI(msg);
  }
}

static void _StartAudio()
{
  if (sInstance) sInstance->StartAudio();
}

static void _StopAudio()
{
  if (sInstance) sInstance->StopAudio();
}

static void _OnIdleTick()
{
  if (sInstance) sInstance->OnEditorIdleTick();
}

static int _GetAudioContext()
{
  return sInstance ? sInstance->GetAudioContext() : 0;
}

static int _GetWorkletNode()
{
  return sInstance ? sInstance->GetWorkletNode() : 0;
}

EMSCRIPTEN_BINDINGS(IPlugEmAudioWorklet) {
  function("setParam", &_SetParameterValue);
  function("sendMidi", &_SendMidiMsg);
  function("startAudio", &_StartAudio);
  function("stopAudio", &_StopAudio);
  function("onIdleTick", &_OnIdleTick);
  function("getAudioContext", &_GetAudioContext);
  function("getWorkletNode", &_GetWorkletNode);
}
