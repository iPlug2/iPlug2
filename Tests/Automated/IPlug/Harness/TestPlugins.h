/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief Plug-in classes shared by the IPlug integration tests.
 *
 * They are written the way a real iPlug2 plug-in is written (derive from Plugin, initialise parameters in the
 * constructor, override ProcessBlock() etc.), and additionally record every callback the framework makes so that the
 * tests can assert on what the plug-in was told.
 */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "IPlugTestAPI.h"
#include "MidiSynth.h"

BEGIN_IPLUG_NAMESPACE

/** A plug-in that records the callbacks it receives. The other test plug-ins derive from it. */
class RecordingPlug : public Plugin
{
public:
  struct ParamChange
  {
    int mIdx;
    EParamSource mSource;
    double mValue; // the non-normalized value of the parameter when the callback was made
  };

  struct ParamValueToUI
  {
    int mIdx;
    double mValue;
    bool mNormalized;
  };

  struct ControlValue
  {
    int mCtrlTag;
    double mNormalizedValue;
  };

  struct Message
  {
    int mMsgTag;
    int mCtrlTag;
    std::vector<uint8_t> mData;
  };

  RecordingPlug(const InstanceInfo& info, const TestConfig& config)
  : Plugin(info, config.Make())
  {
  }

#pragma mark - IPlugProcessor hooks

  void OnReset() override
  {
    mNumResets++;
    mSampleRateAtReset = GetSampleRate();
    mBlockSizeAtReset = GetBlockSize();
  }

  void OnActivate(bool active) override { mActivations.push_back(active); }

  void ProcessMidiMsg(const IMidiMsg& msg) override
  {
    mMidiIn.push_back(msg);

    if (mEchoMidi)
      SendMidiMsg(msg);
  }

  void ProcessSysEx(const ISysEx& msg) override
  {
    mSysExIn.emplace_back(msg.mData, msg.mData + msg.mSize);
    mSysExInOffsets.push_back(msg.mOffset);
  }

#pragma mark - IEditorDelegate/IPlugAPIBase hooks

  void OnParamChange(int paramIdx, EParamSource source, int sampleOffset = -1) override
  {
    mParamChanges.push_back({paramIdx, source, GetParam(paramIdx)->Value()});
  }

  void OnParamChangeUI(int paramIdx, EParamSource source = kUnknown) override
  {
    mParamChangesUI.push_back({paramIdx, source, GetParam(paramIdx)->Value()});
  }

  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override
  {
    mParamValuesToUI.push_back({paramIdx, value, normalized});
    Plugin::SendParameterValueFromDelegate(paramIdx, value, normalized);
  }

  void SendControlValueFromDelegate(int ctrlTag, double normalizedValue) override
  {
    mControlValuesToUI.push_back({ctrlTag, normalizedValue});
  }

  bool OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData) override
  {
    const uint8_t* pBytes = static_cast<const uint8_t*>(pData);
    Message msg {msgTag, ctrlTag, {}};

    if (dataSize > 0 && pBytes)
      msg.mData.assign(pBytes, pBytes + dataSize);

    mMessages.push_back(msg);
    return true;
  }

  void OnMidiMsgUI(const IMidiMsg& msg) override { mMidiToUI.push_back(msg); }
  void OnSysexMsgUI(const ISysEx& msg) override { mSysExToUI.emplace_back(msg.mData, msg.mData + msg.mSize); }
  void OnIdle() override { mNumIdleCalls++; }
  void OnHostIdentified() override { mNumHostIdentifiedCalls++; }
  void OnPresetsModified() override { mNumPresetsModifiedCalls++; }

  void OnRestoreState() override
  {
    mNumRestoreStateCalls++;
    Plugin::OnRestoreState();
  }

  void OnUIOpen() override
  {
    mNumUIOpenCalls++;
    Plugin::OnUIOpen();
  }

  void OnUIClose() override
  {
    mNumUICloseCalls++;
    Plugin::OnUIClose();
  }

  /** Forget everything recorded so far, both by the plug-in and by the "host" */
  void ClearAll()
  {
    ClearRecorded();
    mParamChanges.clear();
    mParamChangesUI.clear();
    mParamValuesToUI.clear();
    mControlValuesToUI.clear();
    mMessages.clear();
    mMidiIn.clear();
    mMidiToUI.clear();
    mSysExIn.clear();
    mSysExInOffsets.clear();
    mSysExToUI.clear();
    mActivations.clear();
    mNumResets = 0;
    mNumIdleCalls = 0;
    mNumHostIdentifiedCalls = 0;
    mNumPresetsModifiedCalls = 0;
    mNumRestoreStateCalls = 0;
    mNumUIOpenCalls = 0;
    mNumUICloseCalls = 0;
  }

  /** @return How many OnParamChange() calls were recorded for a parameter from a particular source */
  int CountParamChanges(int paramIdx, EParamSource source) const
  {
    return static_cast<int>(std::count_if(mParamChanges.begin(), mParamChanges.end(), [&](const ParamChange& change) {
      return change.mIdx == paramIdx && change.mSource == source;
    }));
  }

  std::vector<ParamChange> mParamChanges;         // OnParamChange()
  std::vector<ParamChange> mParamChangesUI;       // OnParamChangeUI()
  std::vector<ParamValueToUI> mParamValuesToUI;   // SendParameterValueFromDelegate()
  std::vector<ControlValue> mControlValuesToUI;   // SendControlValueFromDelegate()
  std::vector<Message> mMessages;                 // OnMessage()
  std::vector<IMidiMsg> mMidiIn;                  // ProcessMidiMsg()
  std::vector<IMidiMsg> mMidiToUI;                // OnMidiMsgUI()
  std::vector<std::vector<uint8_t>> mSysExIn;     // ProcessSysEx()
  std::vector<int> mSysExInOffsets;
  std::vector<std::vector<uint8_t>> mSysExToUI;   // OnSysexMsgUI()
  std::vector<bool> mActivations;                 // OnActivate()
  int mNumResets = 0;
  double mSampleRateAtReset = 0.;
  int mBlockSizeAtReset = 0;
  int mNumIdleCalls = 0;
  int mNumHostIdentifiedCalls = 0;
  int mNumPresetsModifiedCalls = 0;
  int mNumRestoreStateCalls = 0;
  int mNumUIOpenCalls = 0;
  int mNumUICloseCalls = 0;
  bool mEchoMidi = false;
};

/** A gain effect. Every output channel is the same-numbered input channel (or silence) scaled by the gain. */
class GainPlug : public RecordingPlug
{
public:
  enum EParams
  {
    kGain = 0,
    kMute,
    kMode,
    kNumParams
  };

  static TestConfig MakeConfig(const char* channelIO = "2-2", int latency = 0)
  {
    TestConfig config;
    config.nParams = kNumParams;
    config.channelIO = channelIO;
    config.latency = latency;
    return config;
  }

  GainPlug(const InstanceInfo& info = InstanceInfo{}, const TestConfig& config = MakeConfig())
  : RecordingPlug(info, config)
  {
    GetParam(kGain)->InitDouble("Gain", 100., 0., 200., 0.01, "%");
    GetParam(kMute)->InitBool("Mute", false);
    GetParam(kMode)->InitEnum("Mode", 0, {"Clean", "Warm", "Hot"});

    mInputPeaks.resize(static_cast<size_t>(MaxNChannels(ERoute::kInput)), 0.);
  }

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override
  {
    mNumProcessCalls++;
    mLastNFrames = nFrames;

    const double gain = GetParam(kMute)->Bool() ? 0. : GetParam(kGain)->Value() / 100.;
    const int nIn = MaxNChannels(ERoute::kInput);
    const int nOut = MaxNChannels(ERoute::kOutput);

    for (int c = 0; c < nIn; c++)
    {
      double peak = 0.;

      for (int s = 0; s < nFrames; s++)
        peak = std::max(peak, std::fabs(static_cast<double>(inputs[c][s])));

      mInputPeaks[static_cast<size_t>(c)] = peak;
    }

    for (int c = 0; c < nOut; c++)
    {
      for (int s = 0; s < nFrames; s++)
        outputs[c][s] = (c < nIn ? inputs[c][s] : sample(0)) * static_cast<sample>(gain);
    }
  }

  std::vector<double> mInputPeaks; // the peak of each input channel (connected or not) in the last block
  int mNumProcessCalls = 0;
  int mLastNFrames = 0;
};

/** An effect with a sidechain input bus, which is added to the main input when the host has connected it */
class SidechainPlug : public RecordingPlug
{
public:
  static TestConfig MakeConfig()
  {
    TestConfig config;
    config.channelIO = "2-2 2.2-2";
    return config;
  }

  SidechainPlug(const InstanceInfo& info = InstanceInfo{})
  : RecordingPlug(info, MakeConfig())
  {
  }

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override
  {
    for (int c = 0; c < 2; c++)
    {
      const bool sidechainConnected = IsChannelConnected(ERoute::kInput, 2 + c);

      for (int s = 0; s < nFrames; s++)
        outputs[c][s] = inputs[c][s] + (sidechainConnected ? inputs[2 + c][s] : sample(0));
    }
  }
};

/** An effect whose state has custom data after the parameters */
class CustomStatePlug : public RecordingPlug
{
public:
  enum EParams
  {
    kParamA = 0,
    kParamB,
    kParamC,
    kNumParams
  };

  static constexpr int kTableSize = 4;

  static TestConfig MakeConfig()
  {
    TestConfig config;
    config.nParams = kNumParams;
    return config;
  }

  CustomStatePlug(const InstanceInfo& info = InstanceInfo{})
  : RecordingPlug(info, MakeConfig())
  {
    GetParam(kParamA)->InitDouble("A", 1., 0., 10., 0.001);
    GetParam(kParamB)->InitDouble("B", 2., 0., 10., 0.001);
    GetParam(kParamC)->InitInt("C", 3, 0, 10);
  }

  bool SerializeState(IByteChunk& chunk) const override
  {
    bool savedOK = SerializeParams(chunk);
    savedOK &= chunk.PutStr(mText.c_str()) > 0;
    savedOK &= chunk.Put(&mCounter) > 0;

    for (int i = 0; i < kTableSize; i++)
      savedOK &= chunk.Put(&mTable[i]) > 0;

    return savedOK;
  }

  int UnserializeState(const IByteChunk& chunk, int startPos) override
  {
    int pos = UnserializeParams(chunk, startPos);

    if (pos < 0)
      return pos;

    // Only commit the custom data once all of it has been read successfully
    WDL_String text;
    int counter = 0;
    double table[kTableSize] = {};

    pos = chunk.GetStr(text, pos);

    if (pos < 0)
      return pos;

    pos = chunk.Get(&counter, pos);

    for (int i = 0; i < kTableSize && pos >= 0; i++)
      pos = chunk.Get(&table[i], pos);

    if (pos < 0)
      return pos;

    mText = text.Get();
    mCounter = counter;
    std::copy(table, table + kTableSize, mTable);

    return pos;
  }

  std::string mText = "default";
  int mCounter = 0;
  double mTable[kTableSize] = {0., 0., 0., 0.};
};

/** A MIDI effect that transposes notes and forwards everything else unchanged */
class MidiFxPlug : public RecordingPlug
{
public:
  enum EParams
  {
    kTranspose = 0,
    kNumParams
  };

  static TestConfig MakeConfig()
  {
    TestConfig config;
    config.nParams = kNumParams;
    config.channelIO = "0-0";
    config.type = 2;
    config.midiIn = true;
    config.midiOut = true;
    return config;
  }

  MidiFxPlug(const InstanceInfo& info = InstanceInfo{})
  : RecordingPlug(info, MakeConfig())
  {
    GetParam(kTranspose)->InitInt("Transpose", 0, -24, 24, "st");
  }

  void ProcessMidiMsg(const IMidiMsg& msg) override
  {
    mMidiIn.push_back(msg);

    const IMidiMsg::EStatusMsg status = msg.StatusMsg();

    if (status == IMidiMsg::kNoteOn || status == IMidiMsg::kNoteOff)
    {
      IMidiMsg transposed = msg;
      transposed.mData1 = static_cast<uint8_t>(std::min(127, std::max(0, msg.NoteNumber() + GetParam(kTranspose)->Int())));
      SendMidiMsg(transposed);
    }
    else
      SendMidiMsg(msg);
  }

  void ProcessSysEx(const ISysEx& msg) override
  {
    RecordingPlug::ProcessSysEx(msg);
    SendSysEx(msg);
  }

  void ProcessBlock(sample**, sample**, int) override
  {
    mNumMidiMsgsSeenAtBlockStart.push_back(static_cast<int>(mMidiIn.size()));
  }

  std::vector<int> mNumMidiMsgsSeenAtBlockStart; // one entry per ProcessBlock() call
};

/** An instrument built on MidiSynth. Each voice outputs a DC level which follows the note's gate sample-accurately:
 *  it jumps to the note velocity (0-1) at the note-on and ramps down linearly over kReleaseSamples after the note-off. */
class SynthPlug : public RecordingPlug
{
public:
  static constexpr int kNumVoices = 4;
  static constexpr int kReleaseSamples = 16;

  class GateVoice : public SynthVoice
  {
  public:
    GateVoice() { Resize(DEFAULT_BLOCK_SIZE); }

    bool GetBusy() const override { return mInputs[kVoiceControlGate].IsNonzero() || mLevel > 0.; }

    void Trigger(double level, bool isRetrigger) override { mNumTriggers++; }
    void Release() override { mNumReleases++; }

    void ProcessSamplesAccumulating(sample** inputs, sample** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
    {
      mInputs[kVoiceControlGate].Write(mGate.data(), startIdx, nFrames);

      for (int s = startIdx; s < startIdx + nFrames; s++)
      {
        const double gate = static_cast<double>(mGate[static_cast<size_t>(s)]);

        if (gate > 0.)
          mLevel = gate;
        else
          mLevel = std::max(0., mLevel - (1. / kReleaseSamples));

        for (int c = 0; c < nOutputs; c++)
          outputs[c][s] += static_cast<sample>(mLevel);
      }
    }

    void SetSampleRateAndBlockSize(double sampleRate, int blockSize) override { Resize(blockSize); }

    int mNumTriggers = 0;
    int mNumReleases = 0;

  private:
    // ControlRamp::Write() can write up to one MidiSynth block past the frames it was asked for
    void Resize(int blockSize) { mGate.assign(static_cast<size_t>(blockSize + MidiSynth::kDefaultBlockSize + 1), 0.f); }

    std::vector<float> mGate;
    double mLevel = 0.;
  };

  static TestConfig MakeConfig()
  {
    TestConfig config;
    config.channelIO = "0-2";
    config.type = 1;
    config.midiIn = true;
    return config;
  }

  SynthPlug(const InstanceInfo& info = InstanceInfo{})
  : RecordingPlug(info, MakeConfig())
  {
    for (int v = 0; v < kNumVoices; v++)
    {
      mVoices.push_back(std::make_unique<GateVoice>());
      mSynth.AddVoice(mVoices.back().get(), 0);
    }
  }

  void OnReset() override
  {
    RecordingPlug::OnReset();
    mSynth.SetSampleRateAndBlockSize(GetSampleRate(), GetBlockSize());
  }

  void ProcessMidiMsg(const IMidiMsg& msg) override
  {
    mMidiIn.push_back(msg);
    mSynth.AddMidiMsgToQueue(msg);
  }

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override
  {
    const int nOut = MaxNChannels(ERoute::kOutput);

    for (int c = 0; c < nOut; c++)
      std::fill(outputs[c], outputs[c] + nFrames, sample(0));

    mSynth.ProcessBlock(inputs, outputs, 0, nOut, nFrames);
  }

  std::vector<std::unique_ptr<GateVoice>> mVoices; // MidiSynth does not own its voices
  MidiSynth mSynth {VoiceAllocator::kPolyModePoly, MidiSynth::kDefaultBlockSize};
};

END_IPLUG_NAMESPACE
