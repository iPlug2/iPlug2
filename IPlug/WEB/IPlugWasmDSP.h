/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef _IPLUG_HYBRID_DSP_
#define _IPLUG_HYBRID_DSP_

#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

BEGIN_IPLUG_NAMESPACE

/** Used to pass various instance info to the API class */
struct InstanceInfo
{};

/** Hybrid DSP class - AudioWorklet processor for split DSP/UI builds.
 * This is a simplified DSP-only class that runs in an AudioWorklet,
 * designed to be compiled with SINGLE_FILE=1 for BASE64 embedding.
 *
 * Unlike IPlugWAM, this doesn't use the WAM SDK. Communication with
 * the UI module is via simple postMessage (SPVFD, SCVFD, etc.).
 *
 * Build flags: -DNO_IGRAPHICS -DIPLUG_DSP=1 -sSINGLE_FILE=1
 *
 * @ingroup APIClasses */
class IPlugWasmDSP : public IPlugAPIBase
                      , public IPlugProcessor
{
public:
  IPlugWasmDSP(const InstanceInfo& info, const Config& config);
  virtual ~IPlugWasmDSP() = default;

  /** Initialize the DSP processor
   * @param sampleRate The sample rate
   * @param blockSize The block size (typically 128 for Web Audio) */
  void Init(int sampleRate, int blockSize);

  /** Process a block of audio
   * @param inputs Array of input channel pointers
   * @param outputs Array of output channel pointers
   * @param nFrames Number of frames to process */
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;

  // Message handlers - called from JS port.onmessage
  void OnParamMessage(int paramIdx, double value);
  void OnMidiMessage(int status, int data1, int data2);
  void OnSysexMessage(const uint8_t* pData, int size);
  void OnArbitraryMessage(int msgTag, int ctrlTag, int dataSize, const void* pData);

  /** Called on idle tick to flush queued messages to UI */
  void OnIdleTick();

  // IPlugProcessor
  void SetLatency(int samples) override {}
  bool SendMidiMsg(const IMidiMsg& msg) override;
  bool SendSysEx(const ISysEx& msg) override;

  // IEditorDelegate - send to UI via postMessage
  void SendControlValueFromDelegate(int ctrlTag, double normalizedValue) override;
  void SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData) override;
  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
  void SendArbitraryMsgFromDelegate(int msgTag, int dataSize = 0, const void* pData = nullptr) override;

  /** Get the number of input channels */
  int GetNumInputChannels() const { return MaxNChannels(ERoute::kInput); }

  /** Get the number of output channels */
  int GetNumOutputChannels() const { return MaxNChannels(ERoute::kOutput); }

  /** Check if plugin is an instrument (synth) */
  bool IsPlugInstrument() const { return IsInstrument(); }
};

IPlugWasmDSP* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif // _IPLUG_HYBRID_DSP_
