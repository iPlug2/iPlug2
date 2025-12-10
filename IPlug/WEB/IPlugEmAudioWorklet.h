/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef _IPLUG_EM_AUDIOWORKLET_
#define _IPLUG_EM_AUDIOWORKLET_

#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

#include <emscripten/webaudio.h>

BEGIN_IPLUG_NAMESPACE

/** Used to pass various instance info to the API class */
struct InstanceInfo
{};

/** Emscripten AudioWorklet API class.
 * This is a simplified web audio backend using Emscripten's native AudioWorklet support
 * instead of the WAM SDK. It provides direct audio processing via the WebAudio API.
 *
 * Requires build flags: -sAUDIO_WORKLET=1 -sWASM_WORKERS=1
 * Requires COOP/COEP headers for SharedArrayBuffer support.
 *
 * @ingroup APIClasses */
class IPlugEmAudioWorklet : public IPlugAPIBase
                          , public IPlugProcessor
{
public:
  IPlugEmAudioWorklet(const InstanceInfo& info, const Config& config);
  virtual ~IPlugEmAudioWorklet();

  // Emscripten AudioWorklet initialization
  /** Initialize the audio context and worklet thread
   * @param sampleRate The sample rate to use (0 = use default)
   * @return true on success */
  bool InitAudioWorklet(int sampleRate = 0);

  /** Start audio processing by connecting the worklet node to the destination */
  void StartAudio();

  /** Stop audio processing by disconnecting the node */
  void StopAudio();

  // IPlugProcessor overrides
  void SetLatency(int samples) override {}
  bool SendMidiMsg(const IMidiMsg& msg) override;
  bool SendSysEx(const ISysEx& msg) override;

  // IEditorDelegate - communication to UI thread
  void SendControlValueFromDelegate(int ctrlTag, double normalizedValue) override;
  void SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData) override;
  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
  void SendArbitraryMsgFromDelegate(int msgTag, int dataSize = 0, const void* pData = nullptr) override;

  // Called from UI thread
  void SetParameterValueFromUI(int paramIdx, double value);
  void SendMidiMsgFromUI(const IMidiMsg& msg) override;

  // Get the audio context handle
  EMSCRIPTEN_WEBAUDIO_T GetAudioContext() const { return mAudioContext; }

  // Get the worklet node handle
  EMSCRIPTEN_AUDIO_WORKLET_NODE_T GetWorkletNode() const { return mWorkletNode; }

  /** Called on idle tick to flush queued messages */
  void OnEditorIdleTick();

private:
  // Static callbacks for Emscripten AudioWorklet API
  static void OnWorkletThreadInitialized(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void* userData);
  static void OnProcessorCreated(EMSCRIPTEN_WEBAUDIO_T audioContext, bool success, void* userData);

  /** The main audio processing callback - called on the audio thread
   * @return true to keep processing, false to stop */
  static bool ProcessAudio(int numInputs, const AudioSampleFrame* inputs,
                          int numOutputs, AudioSampleFrame* outputs,
                          int numParams, const AudioParamFrame* params,
                          void* userData);

  // Emscripten handles
  EMSCRIPTEN_WEBAUDIO_T mAudioContext = 0;
  EMSCRIPTEN_AUDIO_WORKLET_NODE_T mWorkletNode = 0;

  // Worklet thread stack (allocated once)
  static constexpr int kWorkletStackSize = 4096;
  uint8_t mWorkletStack[kWorkletStackSize];

  // Initialization state
  bool mWorkletThreadReady = false;
  bool mProcessorCreated = false;
  bool mAudioStarted = false;
  int mRequestedSampleRate = 0;
};

IPlugEmAudioWorklet* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif // _IPLUG_EM_AUDIOWORKLET_
