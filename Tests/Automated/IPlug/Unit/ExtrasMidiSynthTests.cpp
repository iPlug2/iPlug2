/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for MidiSynth (IPlug/Extras/Synth/MidiSynth.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "MidiSynth.h"

using namespace iplug;
using Catch::Approx;

namespace
{
constexpr double kSampleRate = 44100.;
constexpr int kHostBlockSize = 512;
constexpr int kSynthBlockSize = MidiSynth::kDefaultBlockSize; // the synth's internal processing block (32)

// a full scale pitch wheel message is 8191/8192 of the range
constexpr double kPitchWheelMax = 8191. / 8192.;

/** A voice whose audio output is its gate signal, rendered sample accurately from the control ramp,
 *  so that the output of the synth shows exactly when notes start and stop and at what level.
 *  It also records everything else the synth sends it. */
class GateVoice : public SynthVoice
{
public:
  bool GetBusy() const override { return mBusy; }

  void Trigger(double level, bool isRetrigger) override
  {
    mBusy = true;
    mReleased = false;
    mTriggers++;
    mLastLevel = level;
    mWaitingForFirstProcess = true;
  }

  void Release() override
  {
    mReleased = true;
    mReleases++;
  }

  void ProcessSamplesAccumulating(sample** inputs, sample** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
  {
    const ControlRamp& gate = mInputs[kVoiceControlGate];

    if (mWaitingForFirstProcess)
    {
      mStartIdxOfFirstProcessAfterTrigger = startIdx;
      mWaitingForFirstProcess = false;
    }

    // the ramp is documented as describing a change within the block
    if (gate.startValue != gate.endValue && (gate.transitionStart >= nFrames || gate.transitionEnd > nFrames))
      mSawTransitionOutsideBlock = true;

    for (int i = 0; i < nFrames; i++)
    {
      double value;

      if (i < gate.transitionStart)
        value = gate.startValue;
      else if (i < gate.transitionEnd)
        value = gate.startValue + (gate.endValue - gate.startValue) * (i - gate.transitionStart + 1) / (gate.transitionEnd - gate.transitionStart);
      else
        value = gate.endValue;

      for (int c = 0; c < nOutputs; c++)
        outputs[c][startIdx + i] += static_cast<sample>(value * mGain);
    }

    mFramesProcessed += nFrames;

    // the voice is finished once the gate has closed
    if (mReleased && gate.endValue == 0.)
      mBusy = false;
  }

  void SetSampleRateAndBlockSize(double sampleRate, int blockSize) override
  {
    mSampleRate = sampleRate;
    mBlockSize = blockSize;
  }

  void SetProgramNumber(int pgm) override { mPrograms.push_back(pgm); }
  void SetControl(int controlNumber, float value) override { mControls.push_back({controlNumber, value}); }

  double Input(int idx) const { return mInputs[static_cast<size_t>(idx)].endValue; }
  double Pitch() const { return Input(kVoiceControlPitch); }
  double Bend() const { return Input(kVoiceControlPitchBend); }
  double Pressure() const { return Input(kVoiceControlPressure); }
  double Timbre() const { return Input(kVoiceControlTimbre); }
  int Key() const { return mKey; }
  int Channel() const { return mChannel; }
  double Gain() const { return mGain; }

  bool mBusy = false;
  bool mReleased = false;
  bool mWaitingForFirstProcess = false;
  bool mSawTransitionOutsideBlock = false;
  int mStartIdxOfFirstProcessAfterTrigger = -1;
  int mTriggers = 0;
  int mReleases = 0;
  int mFramesProcessed = 0;
  double mLastLevel = 0.;
  double mSampleRate = 0.;
  int mBlockSize = 0;
  std::vector<int> mPrograms;
  std::vector<std::pair<int, float>> mControls;
};

/** A MidiSynth with a set of gate voices */
struct Rig
{
  explicit Rig(int nVoices, VoiceAllocator::EPolyMode mode = VoiceAllocator::kPolyModePoly, uint8_t zone = 0)
  : mSynth(mode)
  {
    for (int i = 0; i < nVoices; i++)
    {
      mVoices.push_back(std::make_unique<GateVoice>());
      mSynth.AddVoice(mVoices.back().get(), zone);
    }

    mSynth.SetSampleRateAndBlockSize(kSampleRate, kHostBlockSize);
    mSynth.SetControlGlideTime(0.); // controller changes take effect immediately, unless a test says otherwise
  }

  GateVoice& Voice(int idx) { return *mVoices[static_cast<size_t>(idx)]; }

  /** Render a block of mono output. mSilent is what ProcessBlock() returned */
  std::vector<sample> Render(int nFrames, sample fill = 0.)
  {
    std::vector<sample> out(static_cast<size_t>(nFrames), fill);
    sample* outputs[1] = {out.data()};
    mSilent = mSynth.ProcessBlock(nullptr, outputs, 0, 1, nFrames);
    return out;
  }

  void NoteOn(int key, int velocity = 127, int offset = 0, int channel = 0)
  {
    IMidiMsg msg;
    msg.MakeNoteOnMsg(key, velocity, offset, channel);
    mSynth.AddMidiMsgToQueue(msg);
  }

  void NoteOff(int key, int offset = 0, int channel = 0)
  {
    IMidiMsg msg;
    msg.MakeNoteOffMsg(key, offset, channel);
    mSynth.AddMidiMsgToQueue(msg);
  }

  void PitchWheel(double value, int channel = 0, int offset = 0)
  {
    IMidiMsg msg;
    msg.MakePitchWheelMsg(value, channel, offset);
    mSynth.AddMidiMsgToQueue(msg);
  }

  /** A control change with a raw 7 bit value */
  void CC(int controller, int value, int channel = 0, int offset = 0)
  {
    mSynth.AddMidiMsgToQueue(IMidiMsg(offset, static_cast<uint8_t>((IMidiMsg::kControlChange << 4) | channel), static_cast<uint8_t>(controller), static_cast<uint8_t>(value)));
  }

  /** A registered parameter number, with a 7 bit value sent as the data entry MSB */
  void RPN(int parameter, int value, int channel)
  {
    CC(101, (parameter >> 7) & 0x7F, channel);
    CC(100, parameter & 0x7F, channel);
    CC(6, value, channel);
  }

  void ChannelPressure(int pressure, int channel = 0, int offset = 0)
  {
    IMidiMsg msg;
    msg.MakeChannelATMsg(pressure, offset, channel);
    mSynth.AddMidiMsgToQueue(msg);
  }

  void PolyPressure(int key, int pressure, int channel = 0, int offset = 0)
  {
    IMidiMsg msg;
    msg.MakePolyATMsg(key, pressure, offset, channel);
    mSynth.AddMidiMsgToQueue(msg);
  }

  void ProgramChange(int program, int channel = 0, int offset = 0)
  {
    IMidiMsg msg;
    msg.MakeProgramChange(program, channel, offset);
    mSynth.AddMidiMsgToQueue(msg);
  }

  int NumBusy() const
  {
    int n = 0;
    for (const auto& voice : mVoices)
      n += voice->GetBusy() ? 1 : 0;
    return n;
  }

  GateVoice* VoiceForKey(int key)
  {
    for (auto& voice : mVoices)
    {
      if (voice->GetBusy() && voice->Key() == key)
        return voice.get();
    }

    return nullptr;
  }

  std::vector<std::unique_ptr<GateVoice>> mVoices; // the test owns the voices, and they outlive the synth
  MidiSynth mSynth;
  bool mSilent = false;
};

/** @return true if every sample in [begin, end) is (approximately) value */
bool AllEqual(const std::vector<sample>& v, int begin, int end, double value)
{
  for (int s = begin; s < end; s++)
  {
    if (std::fabs(static_cast<double>(v[static_cast<size_t>(s)]) - value) > 1e-6)
      return false;
  }

  return true;
}

double PitchForKey(int key)
{
  return (key - 69) / 12.;
}
} // namespace

TEST_CASE("MidiSynth: voices can be added and accessed", "[unit][extras][synth][midisynth]")
{
  Rig rig(3);

  REQUIRE(rig.mSynth.NVoices() == 3);
  for (int i = 0; i < 3; i++)
    REQUIRE(rig.mSynth.GetVoice(i) == &rig.Voice(i));

  int nVisited = 0;
  rig.mSynth.ForEachVoice([&](SynthVoice& voice) {
    REQUIRE(&voice == &rig.Voice(nVisited));
    nVisited++;
  });
  REQUIRE(nVisited == 3);
}

TEST_CASE("MidiSynth: sample rate and block size are passed on to the voices", "[unit][extras][synth][midisynth]")
{
  Rig rig(2);

  for (auto& voice : rig.mVoices)
  {
    REQUIRE(voice->mSampleRate == kSampleRate);
    REQUIRE(voice->mBlockSize == kHostBlockSize);
  }

  rig.mSynth.SetSampleRateAndBlockSize(96000., 64);

  for (auto& voice : rig.mVoices)
  {
    REQUIRE(voice->mSampleRate == 96000.);
    REQUIRE(voice->mBlockSize == 64);
  }
}

TEST_CASE("MidiSynth: silent and idle until a note arrives", "[unit][extras][synth][midisynth]")
{
  Rig rig(4);

  for (int i = 0; i < 3; i++)
  {
    const auto out = rig.Render(128, 0.25);
    REQUIRE(rig.mSilent);
    REQUIRE(AllEqual(out, 0, 128, 0.25)); // the output buffer is not touched
  }

  for (auto& voice : rig.mVoices)
  {
    REQUIRE(voice->mFramesProcessed == 0);
    REQUIRE(voice->mTriggers == 0);
  }
}

TEST_CASE("MidiSynth: notes start and stop on the sample of their MIDI messages", "[unit][extras][synth][midisynth]")
{
  // not on the boundaries between the synth's internal blocks: see the hidden test below
  const int onOffset = GENERATE(0, 1, 31, 33, 50, 100, 127);
  const int offOffset = GENERATE(0, 17, 95, 127);
  constexpr int nFrames = 128;

  Rig rig(4);
  CAPTURE(onOffset, offOffset);

  rig.NoteOn(60, 127, onOffset);
  const auto first = rig.Render(nFrames);

  REQUIRE_FALSE(rig.mSilent);
  REQUIRE(AllEqual(first, 0, onOffset, 0.));
  REQUIRE(AllEqual(first, onOffset, nFrames, 1.));
  REQUIRE(rig.NumBusy() == 1);

  // held
  const auto held = rig.Render(nFrames);
  REQUIRE_FALSE(rig.mSilent);
  REQUIRE(AllEqual(held, 0, nFrames, 1.));

  rig.NoteOff(60, offOffset);
  const auto last = rig.Render(nFrames);

  REQUIRE(AllEqual(last, 0, offOffset, 1.));
  REQUIRE(AllEqual(last, offOffset, nFrames, 0.));
  REQUIRE(rig.NumBusy() == 0);

  // and once the voices have finished the synth reports silence again
  const auto after = rig.Render(nFrames, 0.25);
  REQUIRE(rig.mSilent);
  REQUIRE(AllEqual(after, 0, nFrames, 0.25));
}

TEST_CASE("MidiSynth: a note on and off in the same host block", "[unit][extras][synth][midisynth]")
{
  constexpr int nFrames = 256;
  Rig rig(4);

  rig.NoteOn(60, 127, 10);
  rig.NoteOff(60, 150);
  rig.NoteOn(72, 127, 200);
  const auto out = rig.Render(nFrames);

  REQUIRE(AllEqual(out, 0, 10, 0.));
  REQUIRE(AllEqual(out, 10, 150, 1.));
  REQUIRE(AllEqual(out, 150, 200, 0.));
  REQUIRE(AllEqual(out, 200, nFrames, 1.));
}

TEST_CASE("MidiSynth: messages can be queued in any order", "[unit][extras][synth][midisynth]")
{
  constexpr int nFrames = 256;
  Rig rig(4);

  rig.NoteOff(60, 150);
  rig.NoteOn(60, 127, 10);
  const auto out = rig.Render(nFrames);

  REQUIRE(AllEqual(out, 0, 10, 0.));
  REQUIRE(AllEqual(out, 10, 150, 1.));
  REQUIRE(AllEqual(out, 150, nFrames, 0.));
}

TEST_CASE("MidiSynth: any host block size", "[unit][extras][synth][midisynth]")
{
  const int nFrames = GENERATE(1, 7, 31, 32, 33, 100, 512);

  Rig rig(2);
  CAPTURE(nFrames);

  // start half way through one block, stop half way through a later one
  const int offset = nFrames / 2;
  rig.NoteOn(60, 127, offset);
  const auto first = rig.Render(nFrames);
  REQUIRE(AllEqual(first, 0, offset, 0.));
  REQUIRE(AllEqual(first, offset, nFrames, 1.));

  for (int i = 0; i < 3; i++)
  {
    const auto held = rig.Render(nFrames);
    REQUIRE(AllEqual(held, 0, nFrames, 1.));
  }

  rig.NoteOff(60, offset);
  const auto last = rig.Render(nFrames);
  REQUIRE(AllEqual(last, 0, offset, 1.));
  REQUIRE(AllEqual(last, offset, nFrames, 0.));

  // the voice was run for (at least) every sample between the note on and the note off
  REQUIRE(rig.Voice(0).mFramesProcessed + rig.Voice(1).mFramesProcessed >= 4 * nFrames);
}

TEST_CASE("MidiSynth: messages timed beyond the block wait for a later one", "[unit][extras][synth][midisynth]")
{
  constexpr int nFrames = 100;
  Rig rig(2);

  rig.NoteOn(60, 127, 150);
  rig.NoteOff(60, 270);

  const auto first = rig.Render(nFrames);
  REQUIRE(AllEqual(first, 0, nFrames, 0.));

  const auto second = rig.Render(nFrames);
  REQUIRE(AllEqual(second, 0, 50, 0.));
  REQUIRE(AllEqual(second, 50, nFrames, 1.));

  const auto third = rig.Render(nFrames);
  REQUIRE(AllEqual(third, 0, 70, 1.));
  REQUIRE(AllEqual(third, 70, nFrames, 0.));
}

TEST_CASE("MidiSynth: velocity sets the level of the note", "[unit][extras][synth][midisynth]")
{
  const int velocity = GENERATE(1, 32, 64, 100, 127);

  Rig rig(2);
  rig.NoteOn(60, velocity);
  const auto out = rig.Render(64);

  CAPTURE(velocity);
  REQUIRE(AllEqual(out, 0, 64, velocity / 127.));
  REQUIRE(rig.VoiceForKey(60)->mLastLevel == Approx(velocity / 127.));
}

TEST_CASE("MidiSynth: a note on with zero velocity is a note off", "[unit][extras][synth][midisynth]")
{
  Rig rig(2);
  rig.NoteOn(60, 127);
  rig.Render(64);
  REQUIRE(rig.NumBusy() == 1);

  rig.NoteOn(60, 0, 20);
  const auto out = rig.Render(64);

  REQUIRE(AllEqual(out, 0, 20, 1.));
  REQUIRE(AllEqual(out, 20, 64, 0.));
  REQUIRE(rig.NumBusy() == 0);
  REQUIRE(rig.Voice(0).mTriggers + rig.Voice(1).mTriggers == 1);
}

TEST_CASE("MidiSynth: voices are summed, up to the polyphony available", "[unit][extras][synth][midisynth]")
{
  constexpr int nFrames = 128;
  Rig rig(3);

  rig.NoteOn(60, 127, 10);
  rig.NoteOn(64, 127, 40);
  rig.NoteOn(67, 127, 80);
  const auto chord = rig.Render(nFrames);

  REQUIRE(AllEqual(chord, 0, 10, 0.));
  REQUIRE(AllEqual(chord, 10, 40, 1.));
  REQUIRE(AllEqual(chord, 40, 80, 2.));
  REQUIRE(AllEqual(chord, 80, nFrames, 3.));
  REQUIRE(rig.NumBusy() == 3);

  // a fourth note has to steal a voice (the oldest), so the level stays at three voices' worth
  rig.NoteOn(72, 127, 0);
  const auto stolen = rig.Render(nFrames);

  REQUIRE(AllEqual(stolen, 0, nFrames, 3.));
  REQUIRE(rig.NumBusy() == 3);
  REQUIRE(rig.VoiceForKey(60) == nullptr);
  REQUIRE(rig.VoiceForKey(72) != nullptr);

  // releasing one leaves two
  rig.NoteOff(64, 5);
  const auto released = rig.Render(nFrames);
  REQUIRE(AllEqual(released, 0, 5, 3.));
  REQUIRE(AllEqual(released, 5, nFrames, 2.));
}

TEST_CASE("MidiSynth: key sets the voice pitch", "[unit][extras][synth][midisynth]")
{
  Rig rig(4);

  SECTION("default tuning")
  {
    rig.NoteOn(69);
    rig.NoteOn(81);
    rig.NoteOn(33);
    rig.Render(64);

    REQUIRE(rig.VoiceForKey(69)->Pitch() == Approx(0.).margin(1e-9));
    REQUIRE(rig.VoiceForKey(81)->Pitch() == Approx(1.));
    REQUIRE(rig.VoiceForKey(33)->Pitch() == Approx(-3.));
  }

  SECTION("note offset")
  {
    rig.mSynth.SetNoteOffset(-12.);
    rig.NoteOn(69);
    rig.Render(64);
    REQUIRE(rig.VoiceForKey(69)->Pitch() == Approx(-1.));
  }

  SECTION("custom key to pitch function")
  {
    // quarter tones
    rig.mSynth.SetKeyToPitchFn([](int key) { return static_cast<float>(key - 69) / 24.f; });
    rig.NoteOn(93);
    rig.Render(64);
    REQUIRE(rig.VoiceForKey(93)->Pitch() == Approx(1.));
  }

  SECTION("note glide time")
  {
    rig.mSynth.SetPolyMode(VoiceAllocator::kPolyModeMono);
    rig.mSynth.SetNoteGlideTime(0.1);
    rig.NoteOn(69);
    rig.Render(64);
    rig.NoteOn(81);
    rig.Render(64);

    // on its way, but not there yet
    REQUIRE(rig.Voice(0).Pitch() > 0.);
    REQUIRE(rig.Voice(0).Pitch() < 0.1);

    for (int i = 0; i < 100; i++)
      rig.Render(64);

    REQUIRE(rig.Voice(0).Pitch() == Approx(1.));
  }
}

TEST_CASE("MidiSynth: mono mode plays one note on all the voices", "[unit][extras][synth][midisynth]")
{
  constexpr int nFrames = 64;
  const bool setAfterConstruction = GENERATE(false, true);

  Rig rig(3, setAfterConstruction ? VoiceAllocator::kPolyModePoly : VoiceAllocator::kPolyModeMono);
  if (setAfterConstruction)
    rig.mSynth.SetPolyMode(VoiceAllocator::kPolyModeMono);

  rig.NoteOn(60, 127, 8);
  const auto first = rig.Render(nFrames);
  REQUIRE(AllEqual(first, 0, 8, 0.));
  REQUIRE(AllEqual(first, 8, nFrames, 3.));

  // a second note does not add to the level, it replaces the first
  rig.NoteOn(64, 127);
  const auto second = rig.Render(nFrames);
  REQUIRE(AllEqual(second, 0, nFrames, 3.));
  for (auto& voice : rig.mVoices)
    REQUIRE(voice->Pitch() == Approx(PitchForKey(64)));

  // release it and the first, still held, comes back
  rig.NoteOff(64);
  const auto third = rig.Render(nFrames);
  REQUIRE(AllEqual(third, 0, nFrames, 3.));
  for (auto& voice : rig.mVoices)
    REQUIRE(voice->Pitch() == Approx(PitchForKey(60)));

  rig.NoteOff(60, 30);
  const auto last = rig.Render(nFrames);
  REQUIRE(AllEqual(last, 0, 30, 3.));
  REQUIRE(AllEqual(last, 30, nFrames, 0.));
}

TEST_CASE("MidiSynth: pitch bend is scaled by the pitch bend range", "[unit][extras][synth][midisynth]")
{
  Rig rig(2);
  rig.NoteOn(60);
  rig.Render(64);
  GateVoice* pVoice = rig.VoiceForKey(60);

  SECTION("default range of an octave")
  {
    rig.PitchWheel(1.);
    rig.Render(64);
    REQUIRE(pVoice->Bend() == Approx(kPitchWheelMax).margin(1e-6));

    rig.PitchWheel(-1.);
    rig.Render(64);
    REQUIRE(pVoice->Bend() == Approx(-1.).margin(1e-6));

    rig.PitchWheel(0.5);
    rig.Render(64);
    REQUIRE(pVoice->Bend() == Approx(0.5).margin(1e-3));

    rig.PitchWheel(0.);
    rig.Render(64);
    REQUIRE(pVoice->Bend() == Approx(0.).margin(1e-6));
  }

  SECTION("SetPitchBendRange")
  {
    const int semitones = GENERATE(2, 7, 24);
    rig.mSynth.SetPitchBendRange(semitones);

    rig.PitchWheel(1.);
    rig.Render(64);
    REQUIRE(pVoice->Bend() == Approx(kPitchWheelMax * semitones / 12.).margin(1e-6));

    rig.PitchWheel(-1.);
    rig.Render(64);
    REQUIRE(pVoice->Bend() == Approx(-semitones / 12.).margin(1e-6));
  }

  // the pitch of the note itself is separate
  REQUIRE(pVoice->Pitch() == Approx(PitchForKey(60)));
  REQUIRE(pVoice->mTriggers == 1);
}

TEST_CASE("MidiSynth: pitch bend range can be set per channel with RPN 0", "[unit][extras][synth][midisynth]")
{
  Rig rig(4);
  rig.NoteOn(60, 127, 0, 0);
  rig.NoteOn(64, 127, 0, 1);
  rig.Render(64);

  GateVoice* pChannel0 = rig.VoiceForKey(60);
  GateVoice* pChannel1 = rig.VoiceForKey(64);

  // two octaves on channel 0 only
  rig.RPN(0, 24, 0);
  rig.PitchWheel(-1., 0);
  rig.PitchWheel(-1., 1);
  rig.Render(64);

  REQUIRE(pChannel0->Bend() == Approx(-2.).margin(1e-6));
  REQUIRE(pChannel1->Bend() == Approx(-1.).margin(1e-6));

  // the RPN messages themselves are not passed on as controllers
  REQUIRE(pChannel0->mControls.empty());
  REQUIRE(pChannel1->mControls.empty());
}

TEST_CASE("MidiSynth: channel messages only affect voices playing on that channel", "[unit][extras][synth][midisynth]")
{
  Rig rig(4);
  rig.NoteOn(60, 127, 0, 3);
  rig.NoteOn(64, 127, 0, 5);
  rig.Render(64);

  GateVoice* pChannel3 = rig.VoiceForKey(60);
  GateVoice* pChannel5 = rig.VoiceForKey(64);
  REQUIRE(pChannel3->Channel() == 3);
  REQUIRE(pChannel5->Channel() == 5);

  rig.PitchWheel(-1., 5);
  rig.ChannelPressure(127, 5);
  rig.CC(IMidiMsg::kCutoffFrequency, 127, 5);
  rig.CC(IMidiMsg::kModWheel, 127, 5);
  rig.ProgramChange(9, 5);
  rig.Render(64);

  REQUIRE(pChannel5->Bend() == Approx(-1.).margin(1e-6));
  REQUIRE(pChannel5->Pressure() == Approx(1.));
  REQUIRE(pChannel5->Timbre() == Approx(1.));
  REQUIRE(pChannel5->mControls.size() == 1);
  REQUIRE(pChannel5->mPrograms == std::vector<int>{9});

  REQUIRE(pChannel3->Bend() == 0.);
  REQUIRE(pChannel3->Pressure() == 0.);
  REQUIRE(pChannel3->Timbre() == 0.);
  REQUIRE(pChannel3->mControls.empty());
  REQUIRE(pChannel3->mPrograms.empty());

  // note offs too
  rig.NoteOff(60, 0, 5);
  rig.Render(64);
  REQUIRE(rig.NumBusy() == 2);

  rig.NoteOff(60, 0, 3);
  rig.Render(64);
  REQUIRE(rig.NumBusy() == 1);
}

TEST_CASE("MidiSynth: aftertouch drives the pressure input", "[unit][extras][synth][midisynth]")
{
  Rig rig(4);
  rig.NoteOn(60);
  rig.NoteOn(64);
  rig.Render(64);

  GateVoice* pLow = rig.VoiceForKey(60);
  GateVoice* pHigh = rig.VoiceForKey(64);

  SECTION("channel aftertouch reaches every note on the channel")
  {
    const int pressure = GENERATE(0, 1, 64, 127);
    rig.ChannelPressure(pressure);
    rig.Render(64);
    REQUIRE(pLow->Pressure() == Approx(pressure / 127.));
    REQUIRE(pHigh->Pressure() == Approx(pressure / 127.));
  }

  SECTION("polyphonic aftertouch reaches the one key")
  {
    rig.PolyPressure(64, 100);
    rig.Render(64);
    REQUIRE(pHigh->Pressure() == Approx(100. / 127.));
    REQUIRE(pLow->Pressure() == 0.);

    rig.PolyPressure(60, 50);
    rig.Render(64);
    REQUIRE(pHigh->Pressure() == Approx(100. / 127.));
    REQUIRE(pLow->Pressure() == Approx(50. / 127.));
  }

  REQUIRE(pLow->mTriggers == 1);
  REQUIRE(pLow->Bend() == 0.);
  REQUIRE(pLow->Timbre() == 0.);
}

TEST_CASE("MidiSynth: controllers", "[unit][extras][synth][midisynth]")
{
  Rig rig(4);
  rig.NoteOn(60);
  rig.Render(64);
  GateVoice* pVoice = rig.VoiceForKey(60);

  SECTION("CC74 drives the timbre input")
  {
    rig.CC(IMidiMsg::kCutoffFrequency, 64);
    rig.Render(64);
    REQUIRE(pVoice->Timbre() == Approx(64. / 127.));
    REQUIRE(pVoice->mControls.empty());
  }

  SECTION("other controllers are handed to SetControl")
  {
    rig.CC(IMidiMsg::kModWheel, 127);
    rig.CC(7, 0);
    rig.CC(11, 64, 0, 20);
    rig.Render(64);

    REQUIRE(pVoice->mControls.size() == 3);
    REQUIRE(pVoice->mControls[0].first == 1);
    REQUIRE(pVoice->mControls[0].second == Approx(1.f));
    REQUIRE(pVoice->mControls[1].first == 7);
    REQUIRE(pVoice->mControls[1].second == Approx(0.f).margin(1e-9));
    REQUIRE(pVoice->mControls[2].first == 11);
    REQUIRE(pVoice->mControls[2].second == Approx(64.f / 127.f));
    REQUIRE(pVoice->Timbre() == 0.);
  }

  SECTION("controller changes glide over the control glide time")
  {
    rig.mSynth.SetControlGlideTime(0.1);
    rig.CC(IMidiMsg::kCutoffFrequency, 127);
    rig.Render(64);

    REQUIRE(pVoice->Timbre() > 0.);
    REQUIRE(pVoice->Timbre() < 0.1);

    for (int i = 0; i < 100; i++)
      rig.Render(64);

    REQUIRE(pVoice->Timbre() == Approx(1.));
  }

  REQUIRE(pVoice->mTriggers == 1);
}

TEST_CASE("MidiSynth: sustain pedal", "[unit][extras][synth][midisynth]")
{
  constexpr int nFrames = 64;
  Rig rig(4);

  rig.CC(IMidiMsg::kSustainOnOff, 127);
  rig.NoteOn(60);
  rig.NoteOn(64);
  rig.Render(nFrames);

  rig.NoteOff(60);
  rig.NoteOff(64);
  const auto sustained = rig.Render(nFrames);
  REQUIRE(AllEqual(sustained, 0, nFrames, 2.));

  rig.CC(IMidiMsg::kSustainOnOff, 0, 0, 40);
  const auto released = rig.Render(nFrames);
  REQUIRE(AllEqual(released, 0, 40, 2.));
  REQUIRE(AllEqual(released, 40, nFrames, 0.));
  REQUIRE(rig.NumBusy() == 0);

  // the pedal is not passed on as a controller
  for (auto& voice : rig.mVoices)
    REQUIRE(voice->mControls.empty());
}

TEST_CASE("MidiSynth: all notes off", "[unit][extras][synth][midisynth]")
{
  constexpr int nFrames = 64;
  Rig rig(4);

  rig.NoteOn(60, 127, 0, 0);
  rig.NoteOn(64, 127, 0, 1);
  rig.NoteOn(67, 127, 0, 2);
  const auto chord = rig.Render(nFrames);
  REQUIRE(AllEqual(chord, 0, nFrames, 3.));

  rig.CC(IMidiMsg::kAllNotesOff, 0);
  rig.Render(nFrames);
  REQUIRE(rig.NumBusy() == 0);

  const auto after = rig.Render(nFrames, 0.25);
  REQUIRE(rig.mSilent);
  REQUIRE(AllEqual(after, 0, nFrames, 0.25));

  // voices are allowed to finish their release, not cut dead
  for (auto& voice : rig.mVoices)
  {
    if (voice->mTriggers > 0)
      REQUIRE(voice->Gain() == 1.);
  }
}

TEST_CASE("MidiSynth: program change", "[unit][extras][synth][midisynth]")
{
  Rig rig(2);
  rig.NoteOn(60);
  rig.NoteOn(64);
  rig.Render(64);

  rig.ProgramChange(17);
  rig.Render(64);

  for (auto& voice : rig.mVoices)
  {
    REQUIRE(voice->mPrograms == std::vector<int>{17});
    REQUIRE(voice->mTriggers == 1);
    REQUIRE(voice->GetBusy());
  }
}

TEST_CASE("MidiSynth: Reset silences the synth immediately", "[unit][extras][synth][midisynth]")
{
  constexpr int nFrames = 64;
  Rig rig(3);

  rig.CC(IMidiMsg::kSustainOnOff, 127);
  rig.NoteOn(60);
  rig.NoteOn(64);
  const auto playing = rig.Render(nFrames);
  REQUIRE(AllEqual(playing, 0, nFrames, 2.));

  const bool viaSampleRateChange = GENERATE(false, true);
  if (viaSampleRateChange)
    rig.mSynth.SetSampleRateAndBlockSize(48000., 256);
  else
    rig.mSynth.Reset();

  // a hard kill: gain to zero and gates closed
  for (auto& voice : rig.mVoices)
    REQUIRE(voice->Gain() == 0.);

  const auto afterReset = rig.Render(nFrames);
  REQUIRE(AllEqual(afterReset, 0, nFrames, 0.));
  REQUIRE(rig.NumBusy() == 0);

  rig.Render(nFrames);
  REQUIRE(rig.mSilent);

  // and it plays normally afterwards, with the pedal forgotten
  rig.NoteOn(72, 127, 16);
  const auto again = rig.Render(nFrames);
  REQUIRE(AllEqual(again, 0, 16, 0.));
  REQUIRE(AllEqual(again, 16, nFrames, 1.));

  rig.NoteOff(72);
  rig.Render(nFrames);
  REQUIRE(rig.NumBusy() == 0);
}

TEST_CASE("MidiSynth: SetVoicesActive forces a block to be processed", "[unit][extras][synth][midisynth]")
{
  Rig rig(2);

  // a voice started behind the synth's back
  rig.Voice(0).Trigger(1., false);
  rig.Render(64);
  REQUIRE(rig.mSilent);
  REQUIRE(rig.Voice(0).mFramesProcessed == 0);

  rig.mSynth.SetVoicesActive(true);
  rig.Render(64);
  REQUIRE_FALSE(rig.mSilent);
  REQUIRE(rig.Voice(0).mFramesProcessed == 64);
  REQUIRE(rig.Voice(1).mFramesProcessed == 0);

  // it carries on for as long as a voice is busy...
  rig.Render(64);
  REQUIRE_FALSE(rig.mSilent);
  REQUIRE(rig.Voice(0).mFramesProcessed == 128);

  // ...and stops when none are
  rig.Voice(0).mBusy = false;
  rig.Render(64);
  rig.Render(64);
  REQUIRE(rig.mSilent);
}

TEST_CASE("MidiSynth: voices are processed in blocks of the synth's block size", "[unit][extras][synth][midisynth]")
{
  class BlockRecordingVoice : public GateVoice
  {
  public:
    void ProcessSamplesAccumulating(sample** inputs, sample** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
    {
      mBlocks.push_back({startIdx, nFrames});
      GateVoice::ProcessSamplesAccumulating(inputs, outputs, nInputs, nOutputs, startIdx, nFrames);
    }

    std::vector<std::pair<int, int>> mBlocks;
  };

  const int synthBlockSize = GENERATE(16, 32, 64);
  constexpr int nFrames = 100;

  BlockRecordingVoice voice;
  MidiSynth synth(VoiceAllocator::kPolyModePoly, synthBlockSize);
  synth.AddVoice(&voice, 0);
  synth.SetSampleRateAndBlockSize(kSampleRate, kHostBlockSize);

  IMidiMsg msg;
  msg.MakeNoteOnMsg(60, 127, 0);
  synth.AddMidiMsgToQueue(msg);

  std::vector<sample> out(nFrames, 0.);
  sample* outputs[1] = {out.data()};
  REQUIRE_FALSE(synth.ProcessBlock(nullptr, outputs, 0, 1, nFrames));

  // contiguous blocks covering the host block, all full size bar the last
  int expectedStart = 0;
  for (size_t i = 0; i < voice.mBlocks.size(); i++)
  {
    REQUIRE(voice.mBlocks[i].first == expectedStart);
    REQUIRE(voice.mBlocks[i].second == std::min(synthBlockSize, nFrames - expectedStart));
    expectedStart += voice.mBlocks[i].second;
  }

  CAPTURE(synthBlockSize);
  REQUIRE(expectedStart == nFrames);
  REQUIRE(AllEqual(out, 0, nFrames, 1.));
}

TEST_CASE("MidiSynth: MPE mode gives each member channel its own expression", "[unit][extras][synth][midisynth]")
{
  Rig rig(4);

  const bool viaRPN = GENERATE(false, true);
  if (viaRPN)
  {
    // MPE configuration message: RPN 6 on the lower zone master channel, with 15 member channels
    rig.RPN(6, 15, 0);
    rig.Render(64);
  }
  else
  {
    rig.mSynth.InitBasicMPE();
  }

  rig.NoteOn(60, 127, 0, 1);
  rig.NoteOn(64, 100, 0, 2);
  const auto out = rig.Render(64);
  REQUIRE(AllEqual(out, 0, 64, 1. + 100. / 127.));

  GateVoice* pChannel1 = rig.VoiceForKey(60);
  GateVoice* pChannel2 = rig.VoiceForKey(64);
  REQUIRE(pChannel1 != nullptr);
  REQUIRE(pChannel2 != nullptr);

  SECTION("member channel pitch bend has a range of 48 semitones")
  {
    rig.PitchWheel(-1., 2);
    rig.Render(64);
    REQUIRE(pChannel2->Bend() == Approx(-4.).margin(1e-6));
    REQUIRE(pChannel1->Bend() == 0.);

    rig.PitchWheel(0.25, 1);
    rig.Render(64);
    REQUIRE(pChannel1->Bend() == Approx(1.).margin(1e-3));
    REQUIRE(pChannel2->Bend() == Approx(-4.).margin(1e-6));
  }

  SECTION("master channel pitch bend has a range of 2 semitones and adds to the member channels")
  {
    rig.PitchWheel(-1., 0);
    rig.PitchWheel(0., 1);
    rig.PitchWheel(-0.5, 2);
    rig.Render(64);
    REQUIRE(pChannel1->Bend() == Approx(-2. / 12.).margin(1e-6));
    REQUIRE(pChannel2->Bend() == Approx(-2. / 12. - 2.).margin(1e-3));
  }

  SECTION("channel pressure")
  {
    rig.ChannelPressure(127, 1);
    rig.ChannelPressure(32, 2);
    rig.Render(64);
    REQUIRE(pChannel1->Pressure() == Approx(1.));
    REQUIRE(pChannel2->Pressure() == Approx(32. / 127.));

    // master channel pressure adds to the member channel's
    rig.ChannelPressure(32, 0);
    rig.ChannelPressure(64, 2);
    rig.Render(64);
    REQUIRE(pChannel2->Pressure() == Approx(96. / 127.));
    REQUIRE(pChannel1->Pressure() == Approx(1.));
  }

  SECTION("timbre (CC74)")
  {
    rig.CC(IMidiMsg::kCutoffFrequency, 127, 2);
    rig.Render(64);
    REQUIRE(pChannel2->Timbre() == Approx(1.));
    REQUIRE(pChannel1->Timbre() == 0.);

    rig.CC(IMidiMsg::kCutoffFrequency, 20, 0);
    rig.CC(IMidiMsg::kCutoffFrequency, 40, 1);
    rig.Render(64);
    REQUIRE(pChannel1->Timbre() == Approx(60. / 127.));
    REQUIRE(pChannel2->Timbre() == Approx(1.));
  }

  SECTION("polyphonic aftertouch is ignored")
  {
    rig.PolyPressure(60, 127, 1);
    rig.Render(64);
    REQUIRE(pChannel1->Pressure() == 0.);
    REQUIRE(pChannel2->Pressure() == 0.);
    REQUIRE(rig.NumBusy() == 2);
  }

  SECTION("other controllers go to the voices on their channel")
  {
    rig.CC(IMidiMsg::kModWheel, 127, 2);
    rig.Render(64);
    REQUIRE(pChannel2->mControls.size() == 1);
    REQUIRE(pChannel2->mControls[0].first == 1);
    REQUIRE(pChannel1->mControls.empty());
  }

  SECTION("notes are released per channel")
  {
    rig.NoteOff(64, 0, 2);
    const auto released = rig.Render(64);
    REQUIRE(AllEqual(released, 0, 64, 1.));
    REQUIRE(rig.NumBusy() == 1);
    REQUIRE(pChannel1->GetBusy());
  }

  SECTION("all notes off")
  {
    rig.CC(IMidiMsg::kAllNotesOff, 0, 0);
    rig.Render(64);
    REQUIRE(rig.NumBusy() == 0);
  }
}

TEST_CASE("MidiSynth: MPE mode is left by setting the zone to no channels", "[unit][extras][synth][midisynth]")
{
  Rig rig(2);
  rig.NoteOn(60, 127, 0, 1);
  rig.Render(64);
  GateVoice* pVoice = rig.VoiceForKey(60);

  // basic mode: an octave
  rig.PitchWheel(-1., 1);
  rig.Render(64);
  REQUIRE(pVoice->Bend() == Approx(-1.).margin(1e-6));

  // MPE on: four octaves for a member channel
  rig.RPN(6, 15, 0);
  rig.PitchWheel(-1., 1);
  rig.Render(64);
  REQUIRE(pVoice->Bend() == Approx(-4.).margin(1e-6));

  // MPE off again: back to the non-MPE range
  rig.RPN(6, 0, 0);
  rig.PitchWheel(-1., 1);
  rig.Render(64);
  REQUIRE(pVoice->Bend() == Approx(-1.).margin(1e-6));

  // zone configuration is only accepted on the master channels
  rig.RPN(6, 15, 5);
  rig.PitchWheel(-1., 1);
  rig.Render(64);
  REQUIRE(pVoice->Bend() == Approx(-1.).margin(1e-6));
}

TEST_CASE("MidiSynth: SetPitchBendRange is remembered across MPE mode", "[unit][extras][synth][midisynth]")
{
  Rig rig(2);
  rig.mSynth.SetPitchBendRange(7);
  rig.NoteOn(60, 127, 0, 1);
  rig.Render(64);
  GateVoice* pVoice = rig.VoiceForKey(60);

  rig.RPN(6, 15, 0);
  rig.RPN(6, 0, 0);
  rig.PitchWheel(-1., 1);
  rig.Render(64);
  REQUIRE(pVoice->Bend() == Approx(-7. / 12.).margin(1e-6));
}

// BUG: IPlug/Extras/Synth/MidiSynth.cpp:422 ProcessBlock() stops taking messages for an internal block with
// "if (msg.mOffset > startIndex + blockSize) break;", which should be ">=": a message timed exactly
// on the first sample of the NEXT internal block (offset 32, 64, ... with the default block size of
// 32, or offset == nFrames) is handled in the current one with a sample offset equal to the block
// size. The voice is triggered a block early and is handed a gate ramp whose transition lies outside
// the block (transitionStart == nFrames), contradicting ControlRamp's documented layout: a voice that
// reads the ramp's end value starts up to a block early, and one that uses ControlRamp::Write()
// writes one sample past the end of the block.
TEST_CASE("MidiSynth: a message on an internal block boundary belongs to the block it starts", "[.][bug]")
{
  const int offset = GENERATE(kSynthBlockSize, 2 * kSynthBlockSize, 3 * kSynthBlockSize);
  constexpr int nFrames = 128;

  Rig rig(1);
  rig.NoteOn(60, 127, offset);
  const auto out = rig.Render(nFrames);

  CAPTURE(offset);

  // the audio (from a voice that evaluates the ramp carefully) is right...
  REQUIRE(AllEqual(out, 0, offset, 0.));
  REQUIRE(AllEqual(out, offset, nFrames, 1.));

  // ...but the voice should first have been run for the block the note is in, with a ramp inside that block
  REQUIRE(rig.Voice(0).mStartIdxOfFirstProcessAfterTrigger == offset);
  REQUIRE_FALSE(rig.Voice(0).mSawTransitionOutsideBlock);
}

// BUG: IPlug/Extras/Synth/MidiSynth.cpp:200-212 in MidiMessageToEventMPE() the kProgramChange case has no
// break on the member channel path (the comment above it says such messages are ignored): it sets
// kNullAction and then falls through into the kNoteOn case. A program change has no velocity, so
// that turns it into a note off - with the key set to "all keys", since NoteNumber() is -1 - and
// every note sounding on that member channel is released.
TEST_CASE("MidiSynth: MPE program change on a member channel is ignored", "[.][bug]")
{
  Rig rig(2);
  rig.mSynth.InitBasicMPE();

  rig.NoteOn(60, 127, 0, 1);
  rig.Render(64);
  REQUIRE(rig.NumBusy() == 1);

  rig.ProgramChange(5, 1);
  const auto out = rig.Render(64);

  REQUIRE(rig.NumBusy() == 1);
  REQUIRE(rig.Voice(0).mReleases + rig.Voice(1).mReleases == 0);
  REQUIRE(AllEqual(out, 0, 64, 1.));
  REQUIRE(rig.Voice(0).mPrograms.empty());
}

// voices playing notes are on member channels, and get the master channel's program change too
TEST_CASE("MidiSynth: MPE program change on the master channel reaches the whole zone", "[unit][extras][synth][midisynth]")
{
  Rig rig(3);
  rig.mSynth.InitBasicMPE();

  rig.NoteOn(60, 127, 0, 1);
  rig.NoteOn(64, 127, 0, 2);
  rig.Render(64);

  rig.ProgramChange(5, 0);
  rig.Render(64);

  for (auto& voice : rig.mVoices)
    REQUIRE(voice->mPrograms == std::vector<int>{5});

  REQUIRE(rig.NumBusy() == 2);
}

TEST_CASE("MidiSynth: MPE program change on the upper zone's master channel only reaches the upper zone", "[unit][extras][synth][midisynth]")
{
  Rig rig(1);
  GateVoice upperVoice;
  rig.mSynth.AddVoice(&upperVoice, 1);

  // lower zone with 7 member channels, upper zone with 7 member channels
  rig.RPN(6, 7, 0);
  rig.RPN(6, 7, 15);
  rig.Render(64);

  rig.ProgramChange(5, 15);
  rig.Render(64);
  REQUIRE(rig.Voice(0).mPrograms.empty());
  REQUIRE(upperVoice.mPrograms == std::vector<int>{5});

  rig.ProgramChange(6, 0);
  rig.Render(64);
  REQUIRE(rig.Voice(0).mPrograms == std::vector<int>{6});
  REQUIRE(upperVoice.mPrograms == std::vector<int>{5});
}

// BUG: IPlug/Extras/Synth/MidiSynth.h:184-185 UpperZoneStart()/UpperZoneEnd() return 15 - mMPEUpperZoneChannels
// and 15, i.e. a range that includes the upper zone's master channel (15) and the channel below the
// zone, where the lower zone equivalents return just the member channels. SetMPEZones()
// (MidiSynth.cpp:303-304) sets the master channel's pitch bend range to 2 semitones and then the
// member channels' to 48 using that range, which overwrites the master channel's with 48 (and sets a
// channel outside the zone). Master channel pitch bend in the upper zone is therefore 24 times too
// large: +/-4 octaves rather than the +/-2 semitones of the MPE specification and of the lower zone.
TEST_CASE("MidiSynth: MPE upper zone master pitch bend has a range of 2 semitones", "[.][bug]")
{
  // the upper zone is zone 1 for the voice allocator
  Rig rig(2, VoiceAllocator::kPolyModePoly, 1);

  // upper zone with 3 member channels: 12, 13, 14, and master channel 15
  rig.RPN(6, 3, 15);
  rig.NoteOn(60, 127, 0, 14);
  rig.Render(64);
  REQUIRE(rig.NumBusy() == 1);
  GateVoice* pVoice = rig.VoiceForKey(60);

  // member channel bend is 48 semitones
  rig.PitchWheel(-1., 14);
  rig.Render(64);
  REQUIRE(pVoice->Bend() == Approx(-4.).margin(1e-6));

  // master channel bend is 2 semitones, added to the member channel's (here centred)
  rig.PitchWheel(-1., 15);
  rig.PitchWheel(0., 14);
  rig.Render(64);
  REQUIRE(pVoice->Bend() == Approx(-2. / 12.).margin(1e-6));
}
