/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for VoiceAllocator and SynthVoice (IPlug/Extras/Synth/VoiceAllocator.h, SynthVoice.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "VoiceAllocator.h"

using namespace iplug;
using Catch::Approx;

namespace
{
constexpr double kSampleRate = 44100.;
constexpr int kBlockSize = 32;
constexpr uint8_t kNoKey = 255; // what the allocator sets a voice's key to when it is not playing

/** A voice that records everything the allocator does to it. It is busy from Trigger() to Release(),
 *  unless mBusyAfterRelease is set (to stand in for a voice with a long release tail) */
class RecordingVoice : public SynthVoice
{
public:
  struct ProcessCall
  {
    int nInputs, nOutputs, startIdx, nFrames;
  };

  bool GetBusy() const override { return mBusy; }

  void Trigger(double level, bool isRetrigger) override
  {
    mBusy = true;
    mTriggers++;
    mLastLevel = level;
    mLastTriggerWasRetrigger = isRetrigger;
  }

  void Release() override
  {
    mBusy = mBusyAfterRelease;
    mReleases++;
  }

  void ProcessSamplesAccumulating(sample** inputs, sample** outputs, int nInputs, int nOutputs, int startIdx, int nFrames) override
  {
    mProcessCalls.push_back({nInputs, nOutputs, startIdx, nFrames});

    for (int c = 0; c < nOutputs; c++)
      for (int s = startIdx; s < startIdx + nFrames; s++)
        outputs[c][s] += 1.;
  }

  void SetProgramNumber(int pgm) override { mPrograms.push_back(pgm); }
  void SetControl(int controlNumber, float value) override { mControls.push_back({controlNumber, value}); }

  // access to what the allocator sets
  const ControlRamp& Input(int idx) const { return mInputs[static_cast<size_t>(idx)]; }
  double Gate() const { return Input(kVoiceControlGate).endValue; }
  double Pitch() const { return Input(kVoiceControlPitch).endValue; }
  uint8_t Key() const { return mKey; }
  uint8_t Channel() const { return mChannel; }
  uint8_t Zone() const { return mZone; }
  double Gain() const { return mGain; }
  int64_t LastTriggeredTime() const { return mLastTriggeredTime; }

  bool mBusy = false;
  bool mBusyAfterRelease = false;
  int mTriggers = 0;
  int mReleases = 0;
  double mLastLevel = 0.;
  bool mLastTriggerWasRetrigger = false;
  std::vector<ProcessCall> mProcessCalls;
  std::vector<int> mPrograms;
  std::vector<std::pair<int, float>> mControls;
};

VoiceInputEvent MakeEvent(EVoiceAction action, uint8_t zone, uint8_t channel, uint8_t key, float value, int offset = 0, uint8_t flags = 0, int controllerNumber = 0)
{
  VoiceInputEvent event {};
  event.mAddress.mZone = zone;
  event.mAddress.mChannel = channel;
  event.mAddress.mKey = key;
  event.mAddress.mFlags = flags;
  event.mAction = action;
  event.mControllerNumber = controllerNumber;
  event.mValue = value;
  event.mSampleOffset = offset;
  return event;
}

/** An allocator with a set of recording voices, and a clock */
struct Rig
{
  explicit Rig(int nVoices, uint8_t zone = 0)
  {
    mAllocator.SetSampleRateAndBlockSize(kSampleRate, kBlockSize);

    for (int i = 0; i < nVoices; i++)
      AddVoice(zone);
  }

  RecordingVoice& AddVoice(uint8_t zone)
  {
    mVoices.push_back(std::make_unique<RecordingVoice>());
    mAllocator.AddVoice(mVoices.back().get(), zone);
    return *mVoices.back();
  }

  RecordingVoice& Voice(int idx) { return *mVoices[static_cast<size_t>(idx)]; }

  /** Process the queued events, and move on one block */
  void Process()
  {
    mAllocator.ProcessEvents(kBlockSize, mSampleTime);
    mSampleTime += kBlockSize;
  }

  void NoteOn(int key, float velocity = 1.f, int channel = 0, int offset = 0, uint8_t zone = 0)
  {
    mAllocator.AddEvent(MakeEvent(kNoteOnAction, zone, static_cast<uint8_t>(channel), static_cast<uint8_t>(key), velocity, offset));
    Process();
  }

  void NoteOff(int key, int channel = 0, int offset = 0, uint8_t zone = 0)
  {
    mAllocator.AddEvent(MakeEvent(kNoteOffAction, zone, static_cast<uint8_t>(channel), static_cast<uint8_t>(key), 0.f, offset));
    Process();
  }

  void Send(const VoiceInputEvent& event)
  {
    mAllocator.AddEvent(event);
    Process();
  }

  void Sustain(bool down, uint8_t zone = 0) { Send(MakeEvent(kSustainAction, zone, 0, kAllKeys, down ? 1.f : 0.f)); }

  int NumBusy() const
  {
    int n = 0;
    for (const auto& voice : mVoices)
      n += voice->GetBusy() ? 1 : 0;
    return n;
  }

  /** @return The busy voice playing the key, or nullptr */
  RecordingVoice* VoiceForKey(int key)
  {
    for (auto& voice : mVoices)
    {
      if (voice->GetBusy() && voice->Key() == key)
        return voice.get();
    }

    return nullptr;
  }

  std::vector<std::unique_ptr<RecordingVoice>> mVoices; // declared first: must outlive the allocator
  VoiceAllocator mAllocator;
  int64_t mSampleTime = 0;
};

double PitchForKey(int key)
{
  return (key - 69) / 12.; // "1v/octave", with A440 at zero
}
} // namespace

TEST_CASE("VoiceAllocator: adding voices", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(0);
  REQUIRE(rig.mAllocator.GetNVoices() == 0);

  auto& a = rig.AddVoice(0);
  auto& b = rig.AddVoice(3);

  REQUIRE(rig.mAllocator.GetNVoices() == 2);
  REQUIRE(rig.mAllocator.GetVoice(0) == &a);
  REQUIRE(rig.mAllocator.GetVoice(1) == &b);
  REQUIRE(a.Zone() == 0);
  REQUIRE(b.Zone() == 3);

  for (int i = 0; i < kNumVoiceControlRamps; i++)
  {
    REQUIRE_FALSE(a.Input(i).IsNonzero());
    REQUIRE_FALSE(b.Input(i).IsNonzero());
  }

  // processing with nothing going on does nothing
  rig.Process();
  REQUIRE(a.mTriggers == 0);
  REQUIRE(a.Gate() == 0.);
}

TEST_CASE("VoiceAllocator: the number of voices is limited", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(0);

  // voice indices have to fit a 255 bit mask with one value to spare
  for (int i = 0; i < 254; i++)
    REQUIRE_NOTHROW(rig.AddVoice(0));

  REQUIRE(rig.mAllocator.GetNVoices() == 254);

  RecordingVoice oneTooMany;
  REQUIRE_THROWS_AS(rig.mAllocator.AddVoice(&oneTooMany, 0), std::runtime_error);
  REQUIRE(rig.mAllocator.GetNVoices() == 254);
}

TEST_CASE("VoiceAllocator: note on starts a voice", "[unit][extras][synth][voiceallocator]")
{
  const int key = GENERATE(0, 57, 69, 81, 127);
  const int channel = GENERATE(0, 5, 15);
  const int offset = GENERATE(0, 17, 31);
  const float velocity = 0.6f;

  Rig rig(4);
  rig.NoteOn(key, velocity, channel, offset);

  REQUIRE(rig.NumBusy() == 1);
  RecordingVoice* pVoice = rig.VoiceForKey(key);
  REQUIRE(pVoice != nullptr);

  CAPTURE(key, channel, offset);
  REQUIRE(pVoice->mTriggers == 1);
  REQUIRE(pVoice->mLastLevel == Approx(velocity));
  REQUIRE_FALSE(pVoice->mLastTriggerWasRetrigger);
  REQUIRE(pVoice->Channel() == channel);
  REQUIRE(pVoice->Gain() == 1.);
  REQUIRE(pVoice->LastTriggeredTime() == 0);

  // the gate steps up to the velocity on the sample of the note on
  const ControlRamp& gate = pVoice->Input(kVoiceControlGate);
  REQUIRE(gate.startValue == 0.);
  REQUIRE(gate.endValue == Approx(velocity));
  REQUIRE(gate.transitionStart == offset);
  REQUIRE(gate.transitionEnd == offset + 1);

  // with no glide time the pitch gets there at the same moment
  const ControlRamp& pitch = pVoice->Input(kVoiceControlPitch);
  REQUIRE(pitch.endValue == Approx(PitchForKey(key)));
  REQUIRE(pitch.transitionStart == offset);
  REQUIRE(pitch.transitionEnd == offset + 1);

  // and they stay there
  rig.Process();
  REQUIRE(gate.startValue == Approx(velocity));
  REQUIRE(gate.endValue == Approx(velocity));
  REQUIRE(pitch.startValue == Approx(PitchForKey(key)));
  REQUIRE(pVoice->mTriggers == 1);
}

TEST_CASE("VoiceAllocator: each note gets its own voice", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(4);
  const int keys[4] = {60, 64, 67, 72};

  for (int i = 0; i < 4; i++)
  {
    rig.NoteOn(keys[i], 0.25f * static_cast<float>(i + 1));
    REQUIRE(rig.NumBusy() == i + 1);
  }

  for (int i = 0; i < 4; i++)
  {
    RecordingVoice* pVoice = rig.VoiceForKey(keys[i]);
    REQUIRE(pVoice != nullptr);
    REQUIRE(pVoice->mTriggers == 1);
    REQUIRE(pVoice->Gate() == Approx(0.25 * (i + 1)));
    REQUIRE(pVoice->Pitch() == Approx(PitchForKey(keys[i])));
    REQUIRE(pVoice->LastTriggeredTime() == i * kBlockSize);
  }
}

TEST_CASE("VoiceAllocator: several notes in one block", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(4);
  rig.mAllocator.AddEvent(MakeEvent(kNoteOnAction, 0, 0, 60, 1.f, 3));
  rig.mAllocator.AddEvent(MakeEvent(kNoteOnAction, 0, 0, 64, 0.5f, 10));
  rig.mAllocator.AddEvent(MakeEvent(kNoteOnAction, 0, 0, 67, 0.25f, 20));
  rig.Process();

  REQUIRE(rig.NumBusy() == 3);
  REQUIRE(rig.VoiceForKey(60)->Input(kVoiceControlGate).transitionStart == 3);
  REQUIRE(rig.VoiceForKey(64)->Input(kVoiceControlGate).transitionStart == 10);
  REQUIRE(rig.VoiceForKey(67)->Input(kVoiceControlGate).transitionStart == 20);
  REQUIRE(rig.VoiceForKey(64)->Gate() == Approx(0.5));
}

TEST_CASE("VoiceAllocator: note off releases only the matching voice", "[unit][extras][synth][voiceallocator]")
{
  const int offset = GENERATE(0, 9, 31);

  Rig rig(4);
  rig.NoteOn(60);
  rig.NoteOn(64);
  rig.NoteOn(67);

  RecordingVoice* pReleased = rig.VoiceForKey(64);
  RecordingVoice* pLow = rig.VoiceForKey(60);
  RecordingVoice* pHigh = rig.VoiceForKey(67);

  rig.NoteOff(64, 0, offset);

  REQUIRE(rig.NumBusy() == 2);
  REQUIRE(pReleased->mReleases == 1);
  REQUIRE_FALSE(pReleased->GetBusy());

  // the gate steps down on the sample of the note off
  const ControlRamp& gate = pReleased->Input(kVoiceControlGate);
  REQUIRE(gate.startValue == Approx(1.));
  REQUIRE(gate.endValue == 0.);
  REQUIRE(gate.transitionStart == offset);
  REQUIRE(gate.transitionEnd == offset + 1);

  // the others carry on
  REQUIRE(pLow->mReleases == 0);
  REQUIRE(pHigh->mReleases == 0);
  REQUIRE(pLow->Gate() == Approx(1.));
  REQUIRE(pHigh->Gate() == Approx(1.));
  REQUIRE(pLow->GetBusy());
  REQUIRE(pHigh->GetBusy());

  // a second note off for the same key does nothing more
  rig.NoteOff(64);
  REQUIRE(pReleased->mReleases == 1);
  REQUIRE(rig.NumBusy() == 2);
}

TEST_CASE("VoiceAllocator: note off for a key that is not playing is ignored", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(2);
  rig.NoteOn(60);
  rig.NoteOff(61);

  REQUIRE(rig.NumBusy() == 1);
  REQUIRE(rig.VoiceForKey(60)->mReleases == 0);
  REQUIRE(rig.VoiceForKey(60)->Gate() == Approx(1.));
}

TEST_CASE("VoiceAllocator: note off matches the channel as well as the key", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(4);
  rig.NoteOn(60, 1.f, 1);
  rig.NoteOn(60, 1.f, 2);
  REQUIRE(rig.NumBusy() == 2);

  rig.NoteOff(60, 2);
  REQUIRE(rig.NumBusy() == 1);
  REQUIRE(rig.VoiceForKey(60)->Channel() == 1);

  // or all channels at once
  rig.NoteOn(60, 1.f, 2);
  rig.NoteOff(60, kAllChannels);
  REQUIRE(rig.NumBusy() == 0);
}

TEST_CASE("VoiceAllocator: released voices are reused", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(2);

  // far more notes than voices, one at a time
  for (int i = 0; i < 20; i++)
  {
    rig.NoteOn(40 + i);
    REQUIRE(rig.NumBusy() == 1);
    REQUIRE(rig.VoiceForKey(40 + i) != nullptr);
    rig.NoteOff(40 + i);
    REQUIRE(rig.NumBusy() == 0);
  }

  REQUIRE(rig.Voice(0).mTriggers + rig.Voice(1).mTriggers == 20);
  REQUIRE(rig.Voice(0).mReleases + rig.Voice(1).mReleases == 20);

  // a held note is left alone while the other voice comes and goes
  rig.NoteOn(100);
  RecordingVoice* pHeld = rig.VoiceForKey(100);
  const int heldTriggers = pHeld->mTriggers;

  for (int i = 0; i < 5; i++)
  {
    rig.NoteOn(40 + i);
    rig.NoteOff(40 + i);
  }

  REQUIRE(pHeld->mTriggers == heldTriggers);
  REQUIRE(pHeld->Key() == 100);
  REQUIRE(pHeld->GetBusy());
}

TEST_CASE("VoiceAllocator: a free voice is preferred to one still sounding its release", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(2);
  rig.Voice(0).mBusyAfterRelease = true;
  rig.Voice(1).mBusyAfterRelease = true;

  rig.NoteOn(60);
  RecordingVoice* pFirst = rig.VoiceForKey(60);
  rig.NoteOff(60);
  REQUIRE(pFirst->GetBusy()); // released, but still sounding

  rig.NoteOn(62);
  REQUIRE(pFirst->mTriggers == 1); // not cut off
  REQUIRE(rig.NumBusy() == 2);
}

TEST_CASE("VoiceAllocator: when all voices are busy the oldest is stolen", "[unit][extras][synth][voiceallocator]")
{
  const int nVoices = GENERATE(1, 2, 4);

  Rig rig(nVoices);

  for (int i = 0; i < nVoices; i++)
    rig.NoteOn(60 + i);

  REQUIRE(rig.NumBusy() == nVoices);
  RecordingVoice* pOldest = rig.VoiceForKey(60);
  REQUIRE(pOldest != nullptr);

  // one note too many: polyphony stays at the limit, and it is the oldest note that goes
  rig.NoteOn(100, 0.5f);

  CAPTURE(nVoices);
  REQUIRE(rig.NumBusy() == nVoices);
  REQUIRE(rig.VoiceForKey(60) == nullptr);
  REQUIRE(rig.VoiceForKey(100) == pOldest);
  REQUIRE(pOldest->mTriggers == 2);
  REQUIRE(pOldest->Gate() == Approx(0.5));
  REQUIRE(pOldest->Pitch() == Approx(PitchForKey(100)));

  for (int i = 1; i < nVoices; i++)
  {
    REQUIRE(rig.VoiceForKey(60 + i) != nullptr);
    REQUIRE(rig.VoiceForKey(60 + i)->mTriggers == 1);
  }

  // and again: now the second oldest
  if (nVoices > 1)
  {
    rig.NoteOn(101);
    REQUIRE(rig.NumBusy() == nVoices);
    REQUIRE(rig.VoiceForKey(61) == nullptr);
    REQUIRE(rig.VoiceForKey(100) != nullptr);
    REQUIRE(rig.VoiceForKey(101) != nullptr);
  }

  // the note off for the stolen note no longer does anything
  rig.NoteOff(60);
  REQUIRE(rig.NumBusy() == nVoices);
}

TEST_CASE("VoiceAllocator: key to pitch function and pitch offset", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(4);

  SECTION("default: one unit per octave, zero at A440")
  {
    rig.NoteOn(69);
    rig.NoteOn(81);
    rig.NoteOn(45);
    REQUIRE(rig.VoiceForKey(69)->Pitch() == Approx(0.).margin(1e-9));
    REQUIRE(rig.VoiceForKey(81)->Pitch() == Approx(1.));
    REQUIRE(rig.VoiceForKey(45)->Pitch() == Approx(-2.));
  }

  SECTION("custom function")
  {
    rig.mAllocator.SetKeyToPitchFunction([](int key) { return static_cast<float>(key) * 0.5f; });
    rig.NoteOn(10);
    rig.NoteOn(21);
    REQUIRE(rig.VoiceForKey(10)->Pitch() == Approx(5.));
    REQUIRE(rig.VoiceForKey(21)->Pitch() == Approx(10.5));
  }

  SECTION("pitch offset is in keys")
  {
    rig.mAllocator.SetPitchOffset(12.f);
    rig.NoteOn(69);
    REQUIRE(rig.VoiceForKey(69)->Pitch() == Approx(1.));

    rig.mAllocator.SetPitchOffset(-24.f);
    rig.NoteOn(72);
    REQUIRE(rig.VoiceForKey(72)->Pitch() == Approx(PitchForKey(72) - 2.));
  }
}

TEST_CASE("VoiceAllocator: note glide time", "[unit][extras][synth][voiceallocator]")
{
  const double sr = GENERATE(44100., 48000.);
  const double glideTime = 0.01;

  Rig rig(1);
  rig.mAllocator.mPolyMode = VoiceAllocator::kPolyModeMono;
  rig.mAllocator.SetSampleRateAndBlockSize(sr, kBlockSize);
  rig.mAllocator.SetNoteGlideTime(glideTime);

  auto& voice = rig.Voice(0);
  const int glideSamples = static_cast<int>(glideTime * sr);

  // first note: A440, which is where the pitch already is
  rig.NoteOn(69);
  REQUIRE(voice.Pitch() == Approx(0.).margin(1e-9));

  // up an octave: the pitch ramps there over the glide time...
  rig.NoteOn(81);
  REQUIRE(voice.Pitch() == Approx(static_cast<double>(kBlockSize) / glideSamples).margin(1e-6));
  REQUIRE(voice.Input(kVoiceControlPitch).transitionStart == 0);
  REQUIRE(voice.Input(kVoiceControlPitch).transitionEnd == kBlockSize);

  // ...while the gate changes immediately
  REQUIRE(voice.Input(kVoiceControlGate).endValue == Approx(1.));

  double prev = voice.Pitch();
  int nBlocks = 1;
  bool monotonic = true;
  bool continuous = true;

  while (voice.Pitch() < 1. && nBlocks < 100)
  {
    rig.Process();
    if (voice.Pitch() < prev)
      monotonic = false;
    if (voice.Input(kVoiceControlPitch).startValue != prev)
      continuous = false;
    prev = voice.Pitch();
    nBlocks++;
  }

  CAPTURE(sr);
  REQUIRE(monotonic);
  REQUIRE(continuous);
  REQUIRE(voice.Pitch() == 1.);
  REQUIRE(nBlocks == (glideSamples + kBlockSize - 1) / kBlockSize);
  REQUIRE(voice.Input(kVoiceControlPitch).transitionEnd == glideSamples - (nBlocks - 1) * kBlockSize);

  // without a glide time it is immediate again
  rig.mAllocator.SetNoteGlideTime(0.);
  rig.NoteOn(57);
  REQUIRE(voice.Pitch() == Approx(-1.));
}

TEST_CASE("VoiceAllocator: sustain pedal holds notes after their note off", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(4);

  SECTION("pedal down before the notes")
  {
    rig.Sustain(true);
    rig.NoteOn(60);
    rig.NoteOn(64);
    rig.NoteOff(60);
    rig.NoteOff(64);

    REQUIRE(rig.NumBusy() == 2);
    REQUIRE(rig.VoiceForKey(60)->Gate() == Approx(1.));
    REQUIRE(rig.VoiceForKey(60)->mReleases == 0);

    RecordingVoice* pFirst = rig.VoiceForKey(60);
    RecordingVoice* pSecond = rig.VoiceForKey(64);
    rig.Sustain(false);

    REQUIRE(rig.NumBusy() == 0);
    REQUIRE(pFirst->mReleases == 1);
    REQUIRE(pSecond->mReleases == 1);
    REQUIRE(pFirst->Gate() == 0.);
    REQUIRE(pSecond->Gate() == 0.);
  }

  SECTION("pedal down while the notes are held")
  {
    rig.NoteOn(60);
    rig.Sustain(true);
    rig.NoteOff(60);
    REQUIRE(rig.NumBusy() == 1);

    rig.Sustain(false);
    REQUIRE(rig.NumBusy() == 0);
  }

  SECTION("keys still held when the pedal comes up keep sounding")
  {
    rig.Sustain(true);
    rig.NoteOn(60);
    rig.NoteOn(64);
    rig.NoteOff(64);

    RecordingVoice* pHeld = rig.VoiceForKey(60);
    RecordingVoice* pSustained = rig.VoiceForKey(64);
    rig.Sustain(false);

    REQUIRE(pHeld->GetBusy());
    REQUIRE(pHeld->mReleases == 0);
    REQUIRE(pHeld->Gate() == Approx(1.));
    REQUIRE_FALSE(pSustained->GetBusy());

    // and release normally afterwards
    rig.NoteOff(60);
    REQUIRE(rig.NumBusy() == 0);
  }

  SECTION("the pedal releases at its sample offset")
  {
    rig.Sustain(true);
    rig.NoteOn(60);
    rig.NoteOff(60);
    RecordingVoice* pVoice = rig.VoiceForKey(60);

    rig.Send(MakeEvent(kSustainAction, 0, 0, kAllKeys, 0.f, 21));
    REQUIRE(pVoice->Input(kVoiceControlGate).transitionStart == 21);
    REQUIRE(pVoice->Gate() == 0.);
  }

  SECTION("pedal values below half are up")
  {
    rig.Send(MakeEvent(kSustainAction, 0, 0, kAllKeys, 0.4f));
    rig.NoteOn(60);
    rig.NoteOff(60);
    REQUIRE(rig.NumBusy() == 0);

    rig.Send(MakeEvent(kSustainAction, 0, 0, kAllKeys, 0.6f));
    rig.NoteOn(60);
    rig.NoteOff(60);
    REQUIRE(rig.NumBusy() == 1);
  }

  SECTION("without the pedal nothing is held")
  {
    rig.Sustain(false);
    rig.NoteOn(60);
    rig.NoteOff(60);
    REQUIRE(rig.NumBusy() == 0);
  }
}

TEST_CASE("VoiceAllocator: pitch bend, pressure and timbre are routed to the voice inputs", "[unit][extras][synth][voiceallocator]")
{
  struct Routing
  {
    EVoiceAction action;
    int input;
  };

  const auto routing = GENERATE(Routing{kPitchBendAction, kVoiceControlPitchBend}, Routing{kPressureAction, kVoiceControlPressure}, Routing{kTimbreAction, kVoiceControlTimbre});

  Rig rig(3);
  rig.mAllocator.SetControlGlideTime(0.); // immediate
  rig.NoteOn(60, 1.f, 1);
  rig.NoteOn(64, 1.f, 2);

  RecordingVoice* pChannel1 = rig.VoiceForKey(60);
  RecordingVoice* pChannel2 = rig.VoiceForKey(64);

  CAPTURE(static_cast<int>(routing.action));

  // to one channel
  rig.Send(MakeEvent(routing.action, 0, 2, kAllKeys, 0.75f));
  REQUIRE(pChannel2->Input(routing.input).endValue == Approx(0.75));
  REQUIRE(pChannel1->Input(routing.input).endValue == 0.);

  // to one key
  rig.Send(MakeEvent(routing.action, 0, kAllChannels, 60, -0.5f));
  REQUIRE(pChannel1->Input(routing.input).endValue == Approx(-0.5));
  REQUIRE(pChannel2->Input(routing.input).endValue == Approx(0.75));

  // to everything
  rig.Send(MakeEvent(routing.action, 0, kAllChannels, kAllKeys, 0.1f));
  REQUIRE(pChannel1->Input(routing.input).endValue == Approx(0.1));
  REQUIRE(pChannel2->Input(routing.input).endValue == Approx(0.1));

  // none of which touches the other inputs, or retriggers anything
  for (int i = kVoiceControlPitchBend; i < kNumVoiceControlRamps; i++)
  {
    if (i != routing.input)
      REQUIRE(pChannel1->Input(i).endValue == 0.);
  }

  REQUIRE(pChannel1->Gate() == Approx(1.));
  REQUIRE(pChannel1->Pitch() == Approx(PitchForKey(60)));
  REQUIRE(pChannel1->mTriggers == 1);
  REQUIRE(pChannel1->mControls.empty());
}

TEST_CASE("VoiceAllocator: control changes glide over the control glide time", "[unit][extras][synth][voiceallocator]")
{
  const double sr = GENERATE(44100., 48000.);
  const double glideTime = GENERATE(0.01, 0.05); // 0.01 is the default

  Rig rig(1);
  rig.mAllocator.SetSampleRateAndBlockSize(sr, kBlockSize);
  if (glideTime != 0.01)
    rig.mAllocator.SetControlGlideTime(glideTime);

  rig.NoteOn(60);
  auto& voice = rig.Voice(0);
  const int glideSamples = static_cast<int>(glideTime * sr);

  rig.Send(MakeEvent(kPitchBendAction, 0, kAllChannels, kAllKeys, 1.f));

  CAPTURE(sr, glideTime);
  const ControlRamp& bend = voice.Input(kVoiceControlPitchBend);
  REQUIRE(bend.startValue == 0.);
  REQUIRE(bend.endValue == Approx(static_cast<double>(kBlockSize) / glideSamples).margin(1e-6));

  int nBlocks = 1;
  while (bend.endValue < 1. && nBlocks < 1000)
  {
    rig.Process();
    nBlocks++;
  }

  REQUIRE(bend.endValue == 1.);
  REQUIRE(nBlocks == (glideSamples + kBlockSize - 1) / kBlockSize);
}

TEST_CASE("VoiceAllocator: other controllers and program changes go straight to the voices", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(3);
  rig.NoteOn(60, 1.f, 1);
  rig.NoteOn(64, 1.f, 2);

  RecordingVoice* pChannel1 = rig.VoiceForKey(60);
  RecordingVoice* pChannel2 = rig.VoiceForKey(64);

  rig.Send(MakeEvent(kControllerAction, 0, 1, kAllKeys, 0.25f, 0, 0, 7));
  REQUIRE(pChannel1->mControls.size() == 1);
  REQUIRE(pChannel1->mControls[0].first == 7);
  REQUIRE(pChannel1->mControls[0].second == Approx(0.25f));
  REQUIRE(pChannel2->mControls.empty());

  rig.Send(MakeEvent(kControllerAction, 0, kAllChannels, kAllKeys, 1.f, 0, 0, 1));
  REQUIRE(pChannel1->mControls.size() == 2);
  REQUIRE(pChannel1->mControls[1].first == 1);
  REQUIRE(pChannel2->mControls.size() == 1);
  REQUIRE(pChannel2->mControls[0].first == 1);
  REQUIRE(pChannel2->mControls[0].second == Approx(1.f));

  // the program number travels in the controller number field
  rig.Send(MakeEvent(kProgramChangeAction, 0, 2, kAllKeys, 0.f, 0, 0, 42));
  REQUIRE(pChannel1->mPrograms.empty());
  REQUIRE(pChannel2->mPrograms == std::vector<int>{42});

  // nothing here affects the ramps
  for (int i = kVoiceControlPitchBend; i < kNumVoiceControlRamps; i++)
    REQUIRE(pChannel1->Input(i).endValue == 0.);
}

TEST_CASE("VoiceAllocator: events are addressed by zone", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(0);
  auto& zone0a = rig.AddVoice(0);
  auto& zone0b = rig.AddVoice(0);
  auto& zone1 = rig.AddVoice(1);

  rig.Send(MakeEvent(kControllerAction, 1, kAllChannels, kAllKeys, 0.5f, 0, 0, 20));
  REQUIRE(zone0a.mControls.empty());
  REQUIRE(zone0b.mControls.empty());
  REQUIRE(zone1.mControls.size() == 1);

  rig.Send(MakeEvent(kControllerAction, 0, kAllChannels, kAllKeys, 0.5f, 0, 0, 21));
  REQUIRE(zone0a.mControls.size() == 1);
  REQUIRE(zone0b.mControls.size() == 1);
  REQUIRE(zone1.mControls.size() == 1);

  rig.Send(MakeEvent(kControllerAction, kAllZones, kAllChannels, kAllKeys, 0.5f, 0, 0, 22));
  REQUIRE(zone0a.mControls.size() == 2);
  REQUIRE(zone0b.mControls.size() == 2);
  REQUIRE(zone1.mControls.size() == 2);

  // a zone with no voices
  rig.Send(MakeEvent(kControllerAction, 7, kAllChannels, kAllKeys, 0.5f, 0, 0, 23));
  REQUIRE(zone0a.mControls.size() == 2);
  REQUIRE(zone1.mControls.size() == 2);

  rig.Send(MakeEvent(kProgramChangeAction, 1, kAllChannels, kAllKeys, 0.f, 0, 0, 5));
  REQUIRE(zone0a.mPrograms.empty());
  REQUIRE(zone1.mPrograms == std::vector<int>{5});
}

TEST_CASE("VoiceAllocator: address flags", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(4);
  rig.NoteOn(60, 1.f, 1);
  rig.NoteOn(64, 1.f, 2);
  rig.NoteOn(67, 1.f, 3);
  // the fourth voice is idle

  int nIdle = 0;
  RecordingVoice* pIdle = nullptr;
  for (auto& voice : rig.mVoices)
  {
    if (!voice->GetBusy())
    {
      pIdle = voice.get();
      nIdle++;
    }
  }
  REQUIRE(nIdle == 1);

  SECTION("kVoicesAll ignores the channel and key")
  {
    rig.Send(MakeEvent(kControllerAction, 0, 9, 99, 0.5f, 0, kVoicesAll, 30));
    for (auto& voice : rig.mVoices)
      REQUIRE(voice->mControls.size() == 1);
  }

  SECTION("kVoicesBusy skips idle voices")
  {
    rig.Send(MakeEvent(kControllerAction, 0, kAllChannels, kAllKeys, 0.5f, 0, kVoicesBusy, 30));
    REQUIRE(pIdle->mControls.empty());
    REQUIRE(rig.VoiceForKey(60)->mControls.size() == 1);
    REQUIRE(rig.VoiceForKey(64)->mControls.size() == 1);
    REQUIRE(rig.VoiceForKey(67)->mControls.size() == 1);
  }

  SECTION("kVoicesMostRecent picks the last voice triggered")
  {
    rig.Send(MakeEvent(kControllerAction, 0, kAllChannels, kAllKeys, 0.5f, 0, kVoicesMostRecent, 30));
    REQUIRE(rig.VoiceForKey(60)->mControls.empty());
    REQUIRE(rig.VoiceForKey(64)->mControls.empty());
    REQUIRE(rig.VoiceForKey(67)->mControls.size() == 1);

    // ...of those that match the rest of the address
    rig.Send(MakeEvent(kControllerAction, 0, 1, kAllKeys, 0.5f, 0, kVoicesMostRecent, 31));
    REQUIRE(rig.VoiceForKey(60)->mControls.size() == 1);
    REQUIRE(rig.VoiceForKey(67)->mControls.size() == 1);
  }

  SECTION("no flags: channel and key both have to match")
  {
    rig.Send(MakeEvent(kControllerAction, 0, 2, 60, 0.5f, 0, 0, 30));
    for (auto& voice : rig.mVoices)
      REQUIRE(voice->mControls.empty());

    rig.Send(MakeEvent(kControllerAction, 0, 2, 64, 0.5f, 0, 0, 30));
    REQUIRE(rig.VoiceForKey(64)->mControls.size() == 1);
    REQUIRE(rig.VoiceForKey(60)->mControls.empty());
  }
}

TEST_CASE("VoiceAllocator: mono mode plays one note on every voice", "[unit][extras][synth][voiceallocator]")
{
  constexpr int nVoices = 3;
  Rig rig(nVoices);
  rig.mAllocator.mPolyMode = VoiceAllocator::kPolyModeMono;

  // all voices play the note (for unison)
  rig.NoteOn(60, 0.5f, 3, 7);

  for (auto& voice : rig.mVoices)
  {
    REQUIRE(voice->GetBusy());
    REQUIRE(voice->mTriggers == 1);
    REQUIRE(voice->Key() == 60);
    REQUIRE(voice->Channel() == 3);
    REQUIRE(voice->Gate() == Approx(0.5));
    REQUIRE(voice->Pitch() == Approx(PitchForKey(60)));
    REQUIRE(voice->Input(kVoiceControlGate).transitionStart == 7);
  }

  // a second note takes over all of them
  rig.NoteOn(64, 0.8f, 3);

  for (auto& voice : rig.mVoices)
  {
    REQUIRE(voice->mTriggers == 2);
    REQUIRE(voice->Key() == 64);
    REQUIRE(voice->Gate() == Approx(0.8));
    REQUIRE(voice->Pitch() == Approx(PitchForKey(64)));
    REQUIRE(voice->mReleases == 0);
  }

  SECTION("releasing the newest note returns to the one still held")
  {
    rig.NoteOff(64, 3);

    for (auto& voice : rig.mVoices)
    {
      REQUIRE(voice->GetBusy());
      REQUIRE(voice->Key() == 60);
      REQUIRE(voice->Pitch() == Approx(PitchForKey(60)));
      REQUIRE(voice->mReleases == 0);
      REQUIRE(voice->Gate() > 0.);
    }

    rig.NoteOff(60, 3, 11);

    for (auto& voice : rig.mVoices)
    {
      REQUIRE_FALSE(voice->GetBusy());
      REQUIRE(voice->mReleases == 1);
      REQUIRE(voice->Gate() == 0.);
      REQUIRE(voice->Input(kVoiceControlGate).transitionStart == 11);
    }
  }

  SECTION("releasing the older note changes nothing")
  {
    rig.NoteOff(60, 3);

    for (auto& voice : rig.mVoices)
    {
      REQUIRE(voice->GetBusy());
      REQUIRE(voice->mTriggers == 2);
      REQUIRE(voice->Key() == 64);
      REQUIRE(voice->Gate() == Approx(0.8));
      REQUIRE(voice->mReleases == 0);
    }

    rig.NoteOff(64, 3);
    REQUIRE(rig.NumBusy() == 0);
  }

  SECTION("a stack of held notes unwinds in order")
  {
    rig.NoteOn(67, 1.f, 3);
    REQUIRE(rig.Voice(0).Key() == 67);
    rig.NoteOff(67, 3);
    REQUIRE(rig.Voice(0).Key() == 64);
    rig.NoteOff(64, 3);
    REQUIRE(rig.Voice(0).Key() == 60);
    REQUIRE(rig.NumBusy() == nVoices);
    rig.NoteOff(60, 3);
    REQUIRE(rig.NumBusy() == 0);
  }
}

TEST_CASE("VoiceAllocator: mono mode only plays the voices of the note's zone", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(0);
  auto& zone0 = rig.AddVoice(0);
  auto& zone1a = rig.AddVoice(1);
  auto& zone1b = rig.AddVoice(1);
  rig.mAllocator.mPolyMode = VoiceAllocator::kPolyModeMono;

  rig.NoteOn(60, 1.f, 0, 0, 1);
  REQUIRE_FALSE(zone0.GetBusy());
  REQUIRE(zone1a.GetBusy());
  REQUIRE(zone1b.GetBusy());

  rig.NoteOff(60, 0, 0, 1);
  REQUIRE(rig.NumBusy() == 0);
  REQUIRE(zone0.mTriggers == 0);
}

TEST_CASE("VoiceAllocator: mono mode with the sustain pedal", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(2);
  rig.mAllocator.mPolyMode = VoiceAllocator::kPolyModeMono;

  rig.Sustain(true);
  rig.NoteOn(60);
  rig.NoteOff(60);

  // held by the pedal
  REQUIRE(rig.NumBusy() == 2);
  REQUIRE(rig.Voice(0).Gate() == Approx(1.));
  REQUIRE(rig.Voice(0).mReleases == 0);

  rig.Sustain(false);
  REQUIRE(rig.NumBusy() == 0);
  REQUIRE(rig.Voice(0).Gate() == 0.);
  REQUIRE(rig.Voice(1).Gate() == 0.);
}

TEST_CASE("VoiceAllocator: soft kill releases every voice", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(3);
  rig.Sustain(true);
  rig.NoteOn(60);
  rig.NoteOn(64);
  rig.NoteOff(64); // sustained

  SECTION("SoftKillAllVoices")
  {
    rig.mAllocator.SoftKillAllVoices();
  }

  SECTION("a note off addressed to all voices (as for MIDI all notes off)")
  {
    rig.mAllocator.AddEvent(MakeEvent(kNoteOffAction, 0, 0, 0, 0.f, 0, kVoicesAll));
  }

  rig.Process();

  REQUIRE(rig.NumBusy() == 0);
  for (auto& voice : rig.mVoices)
  {
    REQUIRE(voice->Gate() == 0.);
    REQUIRE(voice->mReleases >= 1);
  }

  // voices are left to finish their release: the gain is not cut
  REQUIRE(rig.Voice(0).Gain() == 1.);

  // the pedal and held keys have been forgotten
  rig.NoteOn(72);
  rig.NoteOff(72);
  REQUIRE(rig.NumBusy() == 0);
}

TEST_CASE("VoiceAllocator: hard kill also zeroes the voice gain", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(3);
  rig.NoteOn(60);
  rig.NoteOn(64);
  REQUIRE(rig.VoiceForKey(60)->Gain() == 1.);

  SECTION("HardKillAllVoices")
  {
    rig.mAllocator.HardKillAllVoices();
  }

  SECTION("Clear")
  {
    rig.mAllocator.Clear();
  }

  rig.Process();

  REQUIRE(rig.NumBusy() == 0);
  for (auto& voice : rig.mVoices)
  {
    REQUIRE(voice->Gain() == 0.);
    REQUIRE(voice->Gate() == 0.);
  }

  // a new note brings its voice back
  rig.NoteOn(67);
  REQUIRE(rig.NumBusy() == 1);
  REQUIRE(rig.VoiceForKey(67)->Gain() == 1.);
  REQUIRE(rig.VoiceForKey(67)->Gate() == Approx(1.));
}

TEST_CASE("VoiceAllocator: ProcessVoices renders the busy voices into the outputs", "[unit][extras][synth][voiceallocator]")
{
  constexpr int nFrames = 64;
  Rig rig(4);
  rig.NoteOn(60);
  rig.NoteOn(64);

  std::vector<sample> left(nFrames, 0.5), right(nFrames, 0.5);
  sample* outputs[2] = {left.data(), right.data()};

  // a sub-block in the middle of the buffer
  rig.mAllocator.ProcessVoices(nullptr, outputs, 0, 2, 16, 32);

  int nProcessed = 0;
  for (auto& voice : rig.mVoices)
  {
    if (voice->GetBusy())
    {
      REQUIRE(voice->mProcessCalls.size() == 1);
      REQUIRE(voice->mProcessCalls[0].nInputs == 0);
      REQUIRE(voice->mProcessCalls[0].nOutputs == 2);
      REQUIRE(voice->mProcessCalls[0].startIdx == 16);
      REQUIRE(voice->mProcessCalls[0].nFrames == 32);
      nProcessed++;
    }
    else
    {
      REQUIRE(voice->mProcessCalls.empty());
    }
  }
  REQUIRE(nProcessed == 2);

  // each recording voice adds 1 to what was there
  for (size_t s = 0; s < nFrames; s++)
  {
    const sample expected = (s >= 16 && s < 48) ? 2.5 : 0.5;
    REQUIRE(left[s] == expected);
    REQUIRE(right[s] == expected);
  }
}

TEST_CASE("SynthVoice: the default implementations do nothing", "[unit][extras][synth][voiceallocator]")
{
  class MinimalVoice : public SynthVoice
  {
  public:
    bool GetBusy() const override { return true; }
  };

  MinimalVoice voice;
  VoiceAllocator allocator;
  allocator.SetSampleRateAndBlockSize(kSampleRate, kBlockSize);
  allocator.AddVoice(&voice, 0);

  allocator.AddEvent(MakeEvent(kNoteOnAction, 0, 0, 60, 1.f));
  allocator.AddEvent(MakeEvent(kControllerAction, 0, kAllChannels, kAllKeys, 1.f, 0, 0, 1));
  allocator.AddEvent(MakeEvent(kProgramChangeAction, 0, kAllChannels, kAllKeys, 1.f, 0, 0, 1));
  allocator.AddEvent(MakeEvent(kNoteOffAction, 0, 0, 60, 0.f));
  allocator.ProcessEvents(kBlockSize, 0);
  voice.SetSampleRateAndBlockSize(48000., 64);

  // the base class render accumulates silence
  std::vector<sample> buffer(64, 0.25);
  sample* outputs[1] = {buffer.data()};
  allocator.ProcessVoices(nullptr, outputs, 0, 1, 0, 64);

  for (sample v : buffer)
    REQUIRE(v == 0.25);
}

// a note on must not start or steal a voice of another zone, which its note off would never reach
TEST_CASE("VoiceAllocator: poly mode note on only uses voices in the note's zone", "[unit][extras][synth][voiceallocator]")
{
  Rig rig(0);
  auto& zone0 = rig.AddVoice(0);
  auto& zone1 = rig.AddVoice(1);

  rig.NoteOn(60, 1.f, 0, 0, 1);
  REQUIRE_FALSE(zone0.GetBusy());
  REQUIRE(zone1.GetBusy());

  rig.NoteOff(60, 0, 0, 1);
  REQUIRE(rig.NumBusy() == 0);

  // and it steals within the zone rather than taking the other zone's voice
  rig.NoteOn(60, 1.f, 0, 0, 1);
  rig.NoteOn(62, 1.f, 0, 0, 1);
  REQUIRE(zone0.mTriggers == 0);
  REQUIRE(zone1.Key() == 62);
}
