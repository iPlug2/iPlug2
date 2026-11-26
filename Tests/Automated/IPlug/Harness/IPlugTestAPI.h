/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief A headless "API" class for the automated tests.
 *
 * IPlugTestAPI plays the role that IPlugVST3, IPlugCLAP, IPlugAPP etc. play in a real build: a plug-in class derives
 * from it and the test acts as the host, driving the plug-in through its lifecycle and recording everything the
 * plug-in tells the host. There is no window, audio device, timer thread or plug-in SDK involved, so the tests are
 * deterministic and run on every platform.
 */

#include <vector>
#include <cstdint>

#include "IPlugPlatform.h"
#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

BEGIN_IPLUG_NAMESPACE

/** Passed to the plug-in constructor, as with the real API classes */
struct InstanceInfo
{
  /** The API the plug-in should believe it is running as */
  EAPI mAPI = kAPIAPP;
};

/** Build a Config without needing a config.h per test plug-in */
struct TestConfig
{
  int nParams = 0;
  int nPresets = 1;
  const char* channelIO = "2-2";
  const char* name = "TestPlug";
  int latency = 0;
  bool midiIn = false;
  bool midiOut = false;
  bool mpe = false;
  bool chunks = true;
  int type = 0; // 0 = effect, 1 = instrument, 2 = MIDI effect
  bool hasUI = false;
  int width = 300;
  int height = 200;
  bool hostResize = false;
  int minWidth = 100;
  int maxWidth = 2000;
  int minHeight = 100;
  int maxHeight = 2000;
  int version = 0x00010203;
  int uniqueID = 'Tst1';
  int mfrID = 'Acme';

  Config Make() const
  {
    return Config(nParams, nPresets, channelIO, name, name, "AcmeInc", version, uniqueID, mfrID, latency,
                  midiIn, midiOut, mpe, chunks, type, hasUI, width, height, hostResize,
                  minWidth, maxWidth, minHeight, maxHeight, "com.AcmeInc.app.TestPlug", "group.com.AcmeInc");
  }
};

/** Headless API class: the test is the host */
class IPlugTestAPI : public IPlugAPIBase
                   , public IPlugProcessor
{
public:
  /** A parameter gesture/value notification sent from the plug-in to the host */
  struct ParamEvent
  {
    enum class EType { Begin, Value, End };
    EType mType;
    int mIdx;
    double mNormalizedValue;
  };

  IPlugTestAPI(const InstanceInfo& info, const Config& config)
  : IPlugAPIBase(config, info.mAPI)
  , IPlugProcessor(config, info.mAPI)
  {
    SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
    SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true);
    SetBlockSize(DEFAULT_BLOCK_SIZE);
    InitLatencyDelay(); // as IPlugVST3/IPlugAU/IPlugAAX do, so that a bypassed plug-in still reports the right latency
  }

#pragma mark - Host -> plug-in

  /** What a host does before starting processing */
  void HostActivate(double sampleRate, int blockSize)
  {
    SetBlockSize(blockSize);
    SetSampleRate(sampleRate);
    OnActivate(true);
    OnReset();
  }

  void HostDeactivate() { OnActivate(false); }

  /** Connect the first nIn/nOut channels and disconnect the rest */
  void HostSetChannels(int nIn, int nOut)
  {
    // Disconnected inputs read from the scratch buffers, which hold stale audio if the host was providing single
    // precision buffers: clear them when inputs go away, as IPlugVST3 does when a sidechain bus is deactivated
    if (nIn < NChannelsConnected(ERoute::kInput))
      ZeroScratchBuffers();

    SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), false);
    SetChannelConnections(ERoute::kInput, 0, nIn, true);
    SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), false);
    SetChannelConnections(ERoute::kOutput, 0, nOut, true);
  }

  /** Host automation: set a parameter from a normalized value, as the plug-in API classes do */
  void HostSetParameterNormalized(int paramIdx, double normalizedValue)
  {
    ENTER_PARAMS_MUTEX
    GetParam(paramIdx)->SetNormalized(normalizedValue);
    SendParameterValueFromAPI(paramIdx, normalizedValue, true);
    OnParamChange(paramIdx, kHost);
    LEAVE_PARAMS_MUTEX
  }

  void HostSetBypassed(bool bypassed) { SetBypassed(bypassed); }
  void HostSetRenderingOffline(bool offline) { SetRenderingOffline(offline); }
  void HostSetTimeInfo(const ITimeInfo& timeInfo) { SetTimeInfo(timeInfo); }

  /** Queue a MIDI message to be delivered at the start of the next HostProcess() call */
  void HostSendMidi(const IMidiMsg& msg) { mPendingMidi.push_back(msg); }

  /** Queue a SysEx message to be delivered at the start of the next HostProcess() call. The data is copied. */
  void HostSendSysEx(const ISysEx& msg)
  {
    mPendingSysEx.push_back({msg.mOffset, std::vector<uint8_t>(msg.mData, msg.mData + msg.mSize)});
  }

  /** @return The label the plug-in gave a channel with SetChannelLabel(), as IPlugVST2 reports it to the host */
  const char* HostGetChannelLabel(ERoute direction, int chIdx) { return GetChannelLabel(direction, chIdx).Get(); }

  /** Process one block of audio, using the connected channel counts
   * @tparam T float or double: the sample type of the host buffers */
  template <typename T>
  void HostProcess(T** inputs, T** outputs, int nFrames)
  {
    // Incoming MIDI/SysEx goes to the processor and is also queued for the editor, as in IPlugCLAP/IPlugAPP/IPlugVST2
    for (auto& msg : mPendingMidi)
    {
      ProcessMidiMsg(msg);
      mMidiMsgsFromProcessor.Push(msg);
    }
    mPendingMidi.clear();

    for (auto& pending : mPendingSysEx)
    {
      ISysEx sysEx {pending.mOffset, pending.mData.data(), static_cast<int>(pending.mData.size())};
      ProcessSysEx(sysEx);
      mSysExDataFromProcessor.PushFromArgs(sysEx.mOffset, sysEx.mSize, sysEx.mData);
    }
    mPendingSysEx.clear();

    IMidiMsg msg;
    while (mMidiMsgsFromEditor.Pop(msg))
      ProcessMidiMsg(msg);

    // SysEx from the editor bypasses ProcessSysEx() and goes straight to the host, as in IPlugCLAP/IPlugVST2
    while (mSysExDataFromEditor.Pop(mSysExFromEditorBuf))
      SendSysEx(ISysEx(mSysExFromEditorBuf.mOffset, mSysExFromEditorBuf.mData, mSysExFromEditorBuf.mSize));

    AttachBuffers(ERoute::kInput, 0, NChannelsConnected(ERoute::kInput), inputs, nFrames);
    AttachBuffers(ERoute::kOutput, 0, NChannelsConnected(ERoute::kOutput), outputs, nFrames);

    if (GetBypassed())
      PassThroughBuffers(T(0), nFrames);
    else
      ProcessBuffers(T(0), nFrames);
  }

  /** Stands in for the main-thread idle timer: flushes the processor -> editor queues and calls OnIdle() */
  void HostIdle() { OnTimer(); }

  /** Save the complete plug-in state, as a host would when saving a project */
  bool HostSaveState(IByteChunk& chunk) const { return SerializeState(chunk); }

  /** Restore plug-in state, as a host would when loading a project
   * @return The new chunk position, or a negative value on failure */
  int HostLoadState(const IByteChunk& chunk)
  {
    int pos = UnserializeState(chunk, 0);
    OnRestoreState();
    return pos;
  }

#pragma mark - Plug-in -> host (recorded)

  void BeginInformHostOfParamChange(int idx) override { mParamEvents.push_back({ParamEvent::EType::Begin, idx, 0.}); }
  void InformHostOfParamChange(int idx, double normalizedValue) override { mParamEvents.push_back({ParamEvent::EType::Value, idx, normalizedValue}); }
  void EndInformHostOfParamChange(int idx) override { mParamEvents.push_back({ParamEvent::EType::End, idx, 0.}); }
  void InformHostOfPresetChange() override { mNumPresetChangeNotifications++; }
  void InformHostOfParameterDetailsChange() override { mNumParameterDetailsChanges++; }

  bool EditorResize(int viewWidth, int viewHeight) override
  {
    mNumEditorResizeRequests++;

    if (!mHostAcceptsResize)
      return false;

    SetEditorSize(viewWidth, viewHeight);
    return true;
  }

  bool SendMidiMsg(const IMidiMsg& msg) override
  {
    mMidiOut.push_back(msg);
    return true;
  }

  bool SendSysEx(const ISysEx& msg) override
  {
    mSysExOut.emplace_back(msg.mData, msg.mData + msg.mSize);
    return true;
  }

  void SetLatency(int latency) override
  {
    mNumLatencyChanges++;
    IPlugProcessor::SetLatency(latency);
  }

  void SetTailSize(int tailSize) override
  {
    mNumTailSizeChanges++;
    IPlugProcessor::SetTailSize(tailSize);
  }

  void ClearRecorded()
  {
    mParamEvents.clear();
    mMidiOut.clear();
    mSysExOut.clear();
    mNumPresetChangeNotifications = 0;
    mNumParameterDetailsChanges = 0;
    mNumTailSizeChanges = 0;
    mNumEditorResizeRequests = 0;
    mNumLatencyChanges = 0;
  }

  std::vector<ParamEvent> mParamEvents;
  std::vector<IMidiMsg> mMidiOut;
  std::vector<std::vector<uint8_t>> mSysExOut;
  int mNumPresetChangeNotifications = 0;
  int mNumEditorResizeRequests = 0;
  int mNumLatencyChanges = 0;
  int mNumParameterDetailsChanges = 0;
  int mNumTailSizeChanges = 0;
  bool mHostAcceptsResize = true;

private:
  struct PendingSysEx
  {
    int mOffset;
    std::vector<uint8_t> mData;
  };

  std::vector<IMidiMsg> mPendingMidi;
  std::vector<PendingSysEx> mPendingSysEx;
  SysExData mSysExFromEditorBuf;
};

/** Plug-in classes in the tests derive from Plugin, as they do in a real project */
using Plugin = IPlugTestAPI;

END_IPLUG_NAMESPACE
