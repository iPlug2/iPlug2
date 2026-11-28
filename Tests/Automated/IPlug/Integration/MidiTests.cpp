/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests MIDI and SysEx travelling between host, processor and UI, and an instrument built on MidiSynth.

#include <cstdint>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "AudioBuffers.h"
#include "TestPlugins.h"

using namespace iplug;
using Catch::Approx;

namespace
{

IMidiMsg NoteOn(int noteNumber, int velocity, int offset = 0, int channel = 0)
{
  IMidiMsg msg;
  msg.MakeNoteOnMsg(noteNumber, velocity, offset, channel);
  return msg;
}

IMidiMsg NoteOff(int noteNumber, int offset = 0, int channel = 0)
{
  IMidiMsg msg;
  msg.MakeNoteOffMsg(noteNumber, offset, channel);
  return msg;
}

bool SameMsg(const IMidiMsg& a, const IMidiMsg& b)
{
  return a.mOffset == b.mOffset && a.mStatus == b.mStatus && a.mData1 == b.mData1 && a.mData2 == b.mData2;
}

/** Processes one block of a plug-in with no audio I/O */
void ProcessNoAudio(IPlugTestAPI& plug, int nFrames = 64)
{
  AudioBuffers<float> none {0, nFrames};
  plug.HostProcess(none.Get(), none.Get(), nFrames);
}

/** Runs one block of the synth and returns its first output channel, having checked that the second is identical */
template <typename T>
std::vector<T> ProcessSynth(SynthPlug& synth, int nFrames)
{
  AudioBuffers<T> in {0, nFrames}, out {2, nFrames, T(99)};
  synth.HostProcess(in.Get(), out.Get(), nFrames);
  CHECK(out[0] == out[1]);
  return out[0];
}

template <typename T>
bool IsSilent(const std::vector<T>& signal, int start = 0, int end = -1)
{
  const int last = end < 0 ? static_cast<int>(signal.size()) : end;

  for (int s = start; s < last; s++)
  {
    if (signal[static_cast<size_t>(s)] != T(0))
      return false;
  }

  return true;
}

/** An effect which leaves ProcessMidiMsg() alone */
class DefaultMidiPlug : public Plugin
{
public:
  DefaultMidiPlug()
  : Plugin(InstanceInfo{}, []() {
    TestConfig config;
    config.midiIn = true;
    config.midiOut = true;
    return config.Make();
  }())
  {
  }
};

} // namespace

TEST_CASE("MIDI: host messages reach ProcessMidiMsg() before the block is processed", "[integration][midi]")
{
  MidiFxPlug plug;
  plug.HostActivate(48000., 64);

  const IMidiMsg first = NoteOn(60, 100, 5, 2);
  const IMidiMsg second = NoteOff(60, 40, 2);

  plug.HostSendMidi(first);
  plug.HostSendMidi(second);

  // Nothing happens until the host processes
  CHECK(plug.mMidiIn.empty());

  ProcessNoAudio(plug);

  REQUIRE(plug.mMidiIn.size() == 2);
  CHECK(SameMsg(plug.mMidiIn[0], first));
  CHECK(SameMsg(plug.mMidiIn[1], second));

  // Both messages had arrived by the time ProcessBlock() ran
  REQUIRE(plug.mNumMidiMsgsSeenAtBlockStart.size() == 1);
  CHECK(plug.mNumMidiMsgsSeenAtBlockStart[0] == 2);

  SECTION("messages are delivered once")
  {
    ProcessNoAudio(plug);
    CHECK(plug.mMidiIn.size() == 2);
    REQUIRE(plug.mNumMidiMsgsSeenAtBlockStart.size() == 2);
    CHECK(plug.mNumMidiMsgsSeenAtBlockStart[1] == 2);
  }

  SECTION("status, channel, data and offset are intact")
  {
    CHECK(plug.mMidiIn[0].StatusMsg() == IMidiMsg::kNoteOn);
    CHECK(plug.mMidiIn[0].Channel() == 2);
    CHECK(plug.mMidiIn[0].NoteNumber() == 60);
    CHECK(plug.mMidiIn[0].Velocity() == 100);
    CHECK(plug.mMidiIn[0].mOffset == 5);
    CHECK(plug.mMidiIn[1].StatusMsg() == IMidiMsg::kNoteOff);
    CHECK(plug.mMidiIn[1].mOffset == 40);
  }
}

TEST_CASE("MIDI: a MIDI effect transforms messages and sends them to the host", "[integration][midi]")
{
  MidiFxPlug plug;
  plug.HostActivate(48000., 64);
  plug.GetParam(MidiFxPlug::kTranspose)->Set(12.);

  SECTION("notes are transposed, keeping velocity, channel and offset")
  {
    plug.HostSendMidi(NoteOn(60, 100, 7, 3));
    plug.HostSendMidi(NoteOff(60, 33, 3));
    ProcessNoAudio(plug);

    REQUIRE(plug.mMidiOut.size() == 2);
    CHECK(plug.mMidiOut[0].StatusMsg() == IMidiMsg::kNoteOn);
    CHECK(plug.mMidiOut[0].NoteNumber() == 72);
    CHECK(plug.mMidiOut[0].Velocity() == 100);
    CHECK(plug.mMidiOut[0].Channel() == 3);
    CHECK(plug.mMidiOut[0].mOffset == 7);
    CHECK(plug.mMidiOut[1].StatusMsg() == IMidiMsg::kNoteOff);
    CHECK(plug.mMidiOut[1].NoteNumber() == 72);
    CHECK(plug.mMidiOut[1].Channel() == 3);
    CHECK(plug.mMidiOut[1].mOffset == 33);
  }

  SECTION("other messages are forwarded unchanged")
  {
    IMidiMsg cc;
    cc.MakeControlChangeMsg(IMidiMsg::kModWheel, 0.5, 4, 9);

    IMidiMsg bend;
    bend.MakePitchWheelMsg(-0.25, 1, 11);

    plug.HostSendMidi(cc);
    plug.HostSendMidi(bend);
    ProcessNoAudio(plug);

    REQUIRE(plug.mMidiOut.size() == 2);
    CHECK(SameMsg(plug.mMidiOut[0], cc));
    CHECK(SameMsg(plug.mMidiOut[1], bend));
    CHECK(plug.mMidiOut[0].StatusMsg() == IMidiMsg::kControlChange);
    CHECK(plug.mMidiOut[0].ControlChangeIdx() == IMidiMsg::kModWheel);
    CHECK(plug.mMidiOut[0].Channel() == 4);
    CHECK(plug.mMidiOut[1].StatusMsg() == IMidiMsg::kPitchWheel);
  }

  SECTION("the transposition follows host automation of the parameter")
  {
    plug.HostSetParameterNormalized(MidiFxPlug::kTranspose, 0.); // -24
    plug.HostSendMidi(NoteOn(60, 100));
    ProcessNoAudio(plug);

    plug.HostSetParameterNormalized(MidiFxPlug::kTranspose, 0.5); // 0
    plug.HostSendMidi(NoteOn(60, 100));
    ProcessNoAudio(plug);

    REQUIRE(plug.mMidiOut.size() == 2);
    CHECK(plug.mMidiOut[0].NoteNumber() == 36);
    CHECK(plug.mMidiOut[1].NoteNumber() == 60);
  }

  SECTION("many messages in one block keep their order")
  {
    for (int i = 0; i < 20; i++)
      plug.HostSendMidi(NoteOn(40 + i, 1 + i, i * 3));

    ProcessNoAudio(plug);

    REQUIRE(plug.mMidiOut.size() == 20);

    for (int i = 0; i < 20; i++)
    {
      CHECK(plug.mMidiOut[static_cast<size_t>(i)].NoteNumber() == 52 + i);
      CHECK(plug.mMidiOut[static_cast<size_t>(i)].Velocity() == 1 + i);
      CHECK(plug.mMidiOut[static_cast<size_t>(i)].mOffset == i * 3);
    }
  }
}

TEST_CASE("MIDI: the default ProcessMidiMsg() passes messages through to the output", "[integration][midi]")
{
  DefaultMidiPlug plug;
  plug.HostActivate(48000., 32);

  const IMidiMsg msg = NoteOn(64, 90, 3, 1);
  plug.HostSendMidi(msg);

  AudioBuffers<float> in {2, 32}, out {2, 32};
  plug.HostProcess(in.Get(), out.Get(), 32);

  REQUIRE(plug.mMidiOut.size() == 1);
  CHECK(SameMsg(plug.mMidiOut[0], msg));
}

TEST_CASE("MIDI: SendMidiMsgs() sends a buffer of messages in order", "[integration][midi]")
{
  MidiFxPlug plug;

  WDL_TypedBuf<IMidiMsg> msgs;
  msgs.Add(NoteOn(60, 100, 0));
  msgs.Add(NoteOn(64, 101, 10));
  msgs.Add(NoteOff(60, 20));

  CHECK(plug.SendMidiMsgs(msgs));

  REQUIRE(plug.mMidiOut.size() == 3);

  for (int i = 0; i < 3; i++)
    CHECK(SameMsg(plug.mMidiOut[static_cast<size_t>(i)], msgs.Get()[i]));

  SECTION("an empty buffer sends nothing")
  {
    WDL_TypedBuf<IMidiMsg> empty;
    CHECK(plug.SendMidiMsgs(empty));
    CHECK(plug.mMidiOut.size() == 3);
  }
}

TEST_CASE("MIDI: messages from the UI are queued and reach the processor in the next block", "[integration][midi]")
{
  MidiFxPlug plug;
  plug.HostActivate(48000., 64);

  const IMidiMsg first = NoteOn(48, 80);
  const IMidiMsg second = NoteOff(48);

  plug.SendMidiMsgFromUI(first);
  plug.SendMidiMsgFromUI(second);

  // The UI thread must not call into the processor directly
  CHECK(plug.mMidiIn.empty());
  CHECK(plug.mMidiOut.empty());

  ProcessNoAudio(plug);

  REQUIRE(plug.mMidiIn.size() == 2);
  CHECK(SameMsg(plug.mMidiIn[0], first));
  CHECK(SameMsg(plug.mMidiIn[1], second));
  REQUIRE(plug.mNumMidiMsgsSeenAtBlockStart.size() == 1);
  CHECK(plug.mNumMidiMsgsSeenAtBlockStart[0] == 2);

  // ...from where the plug-in sent them on to the host
  CHECK(plug.mMidiOut.size() == 2);

  SECTION("they are delivered once")
  {
    ProcessNoAudio(plug);
    CHECK(plug.mMidiIn.size() == 2);
  }

  SECTION("messages which originated in the UI are not echoed back to it")
  {
    plug.HostIdle();
    CHECK(plug.mMidiToUI.empty());
  }

  SECTION("host and UI messages in the same block: the host's come first")
  {
    plug.ClearAll();
    plug.SendMidiMsgFromUI(NoteOn(50, 1));
    plug.HostSendMidi(NoteOn(51, 2));
    ProcessNoAudio(plug);

    REQUIRE(plug.mMidiIn.size() == 2);
    CHECK(plug.mMidiIn[0].NoteNumber() == 51);
    CHECK(plug.mMidiIn[1].NoteNumber() == 50);
  }
}

TEST_CASE("MIDI: messages from the host reach the UI on the idle call", "[integration][midi]")
{
  MidiFxPlug plug;
  plug.HostActivate(48000., 64);

  const IMidiMsg first = NoteOn(60, 100, 5);
  const IMidiMsg second = NoteOff(60, 50);

  plug.HostSendMidi(first);
  plug.HostSendMidi(second);
  ProcessNoAudio(plug);

  // The audio thread only queues them
  CHECK(plug.mMidiToUI.empty());

  plug.HostIdle();

  REQUIRE(plug.mMidiToUI.size() == 2);
  CHECK(SameMsg(plug.mMidiToUI[0], first));
  CHECK(SameMsg(plug.mMidiToUI[1], second));

  SECTION("they are delivered once")
  {
    plug.HostIdle();
    CHECK(plug.mMidiToUI.size() == 2);
  }

  SECTION("messages from several blocks accumulate until the next idle call")
  {
    plug.ClearAll();

    for (int block = 0; block < 5; block++)
    {
      plug.HostSendMidi(NoteOn(60 + block, 100));
      ProcessNoAudio(plug);
    }

    plug.HostIdle();

    REQUIRE(plug.mMidiToUI.size() == 5);

    for (int block = 0; block < 5; block++)
      CHECK(plug.mMidiToUI[static_cast<size_t>(block)].NoteNumber() == 60 + block);
  }
}

TEST_CASE("MIDI: SendMidiMsgFromDelegate() delivers straight to the UI", "[integration][midi]")
{
  MidiFxPlug plug;

  const IMidiMsg msg = NoteOn(72, 64, 0, 5);
  plug.SendMidiMsgFromDelegate(msg);

  REQUIRE(plug.mMidiToUI.size() == 1);
  CHECK(SameMsg(plug.mMidiToUI[0], msg));

  // It is a message for the UI only
  CHECK(plug.mMidiIn.empty());
  CHECK(plug.mMidiOut.empty());
}

TEST_CASE("MIDI: SysEx from the host reaches the processor and then the UI", "[integration][midi][sysex]")
{
  MidiFxPlug plug;
  plug.HostActivate(48000., 64);

  const std::vector<uint8_t> data {0xF0, 0x7D, 0x01, 0x02, 0x03, 0x7F, 0xF7};
  plug.HostSendSysEx(ISysEx(12, data.data(), static_cast<int>(data.size())));

  CHECK(plug.mSysExIn.empty());

  ProcessNoAudio(plug);

  REQUIRE(plug.mSysExIn.size() == 1);
  CHECK(plug.mSysExIn[0] == data);
  CHECK(plug.mSysExInOffsets[0] == 12);

  SECTION("the plug-in can send SysEx to the host")
  {
    // MidiFxPlug echoes what it receives
    REQUIRE(plug.mSysExOut.size() == 1);
    CHECK(plug.mSysExOut[0] == data);
  }

  SECTION("the UI gets it on the idle call")
  {
    CHECK(plug.mSysExToUI.empty());
    plug.HostIdle();
    REQUIRE(plug.mSysExToUI.size() == 1);
    CHECK(plug.mSysExToUI[0] == data);

    plug.HostIdle();
    CHECK(plug.mSysExToUI.size() == 1);
  }

  SECTION("several messages of different lengths")
  {
    plug.HostIdle();
    plug.ClearAll();

    const std::vector<uint8_t> shortMsg {0xF0, 0xF7};
    std::vector<uint8_t> longMsg(200, 0x55);
    longMsg.front() = 0xF0;
    longMsg.back() = 0xF7;

    plug.HostSendSysEx(ISysEx(0, shortMsg.data(), static_cast<int>(shortMsg.size())));
    plug.HostSendSysEx(ISysEx(1, longMsg.data(), static_cast<int>(longMsg.size())));
    ProcessNoAudio(plug);
    plug.HostIdle();

    REQUIRE(plug.mSysExIn.size() == 2);
    CHECK(plug.mSysExIn[0] == shortMsg);
    CHECK(plug.mSysExIn[1] == longMsg);

    REQUIRE(plug.mSysExToUI.size() == 2);
    CHECK(plug.mSysExToUI[0] == shortMsg);
    CHECK(plug.mSysExToUI[1] == longMsg);
  }
}

TEST_CASE("MIDI: SysEx from the UI is sent to the host in the next block", "[integration][midi][sysex]")
{
  MidiFxPlug plug;
  plug.HostActivate(48000., 64);

  std::vector<uint8_t> data {0xF0, 0x7D, 0x10, 0x20, 0xF7};
  const std::vector<uint8_t> expected = data;

  plug.SendSysexMsgFromUI(ISysEx(0, data.data(), static_cast<int>(data.size())));

  // The message is copied into the queue: the UI's buffer need not outlive the call
  std::fill(data.begin(), data.end(), uint8_t(0));

  CHECK(plug.mSysExOut.empty());

  ProcessNoAudio(plug);

  REQUIRE(plug.mSysExOut.size() == 1);
  CHECK(plug.mSysExOut[0] == expected);

  // SysEx from the UI goes to the host, not through ProcessSysEx()
  CHECK(plug.mSysExIn.empty());

  SECTION("it is sent once")
  {
    ProcessNoAudio(plug);
    CHECK(plug.mSysExOut.size() == 1);
  }
}

TEST_CASE("MIDI: SendSysexMsgFromDelegate() delivers straight to the UI", "[integration][midi][sysex]")
{
  MidiFxPlug plug;

  const std::vector<uint8_t> data {0xF0, 0x01, 0xF7};
  plug.SendSysexMsgFromDelegate(ISysEx(0, data.data(), static_cast<int>(data.size())));

  REQUIRE(plug.mSysExToUI.size() == 1);
  CHECK(plug.mSysExToUI[0] == data);
  CHECK(plug.mSysExIn.empty());
  CHECK(plug.mSysExOut.empty());
}

TEST_CASE("Instrument: silent until a note is played", "[integration][midi][instrument]")
{
  SynthPlug synth;
  synth.HostActivate(48000., 128);

  REQUIRE(synth.IsInstrument());
  REQUIRE(synth.MaxNChannels(ERoute::kInput) == 0);

  CHECK(IsSilent(ProcessSynth<float>(synth, 128)));
  CHECK(IsSilent(ProcessSynth<double>(synth, 128)));
}

TEST_CASE("Instrument: a note starts at its sample offset within the block", "[integration][midi][instrument]")
{
  const int blockSize = 128;

  // Offsets around the boundaries of MidiSynth's internal 32 sample blocks, and at both ends of the host block
  const int offsets[] = {0, 1, 2, 30, 31, 32, 33, 63, 64, 65, 100, 126, 127};

  for (int offset : offsets)
  {
    INFO("note-on offset " << offset);

    SynthPlug synth;
    synth.HostActivate(48000., blockSize);
    synth.HostSendMidi(NoteOn(60, 127, offset));

    const std::vector<double> out = ProcessSynth<double>(synth, blockSize);

    CHECK(IsSilent(out, 0, offset));

    bool allAtFullLevel = true;

    for (int s = offset; s < blockSize; s++)
      allAtFullLevel &= out[static_cast<size_t>(s)] == 1.;

    CHECK(allAtFullLevel);

    // The note is still sounding in the next block
    const std::vector<double> next = ProcessSynth<double>(synth, blockSize);
    CHECK(next.front() == 1.);
    CHECK(next.back() == 1.);
  }
}

TEST_CASE("Instrument: single and double precision hosts hear the same thing", "[integration][midi][instrument]")
{
  SynthPlug synthF, synthD;
  synthF.HostActivate(48000., 64);
  synthD.HostActivate(48000., 64);

  synthF.HostSendMidi(NoteOn(60, 64, 17));
  synthD.HostSendMidi(NoteOn(60, 64, 17));

  const std::vector<float> outF = ProcessSynth<float>(synthF, 64);
  const std::vector<double> outD = ProcessSynth<double>(synthD, 64);

  for (size_t s = 0; s < 64; s++)
    CHECK(outF[s] == static_cast<float>(outD[s]));

  // Level follows velocity
  CHECK(outD[16] == 0.);
  CHECK(outD[17] == Approx(64. / 127.));
  CHECK(outD[63] == Approx(64. / 127.));
}

TEST_CASE("Instrument: a note-off releases the note and the output returns to silence", "[integration][midi][instrument]")
{
  const int blockSize = 128;
  const int release = SynthPlug::kReleaseSamples;

  SynthPlug synth;
  synth.HostActivate(48000., blockSize);

  synth.HostSendMidi(NoteOn(60, 127, 0));
  ProcessSynth<double>(synth, blockSize);

  SECTION("note-off at a sample offset")
  {
    const int offsets[] = {0, 1, 31, 32, 33, 64, 100};

    for (int offset : offsets)
    {
      INFO("note-off offset " << offset);

      SynthPlug fresh;
      fresh.HostActivate(48000., blockSize);
      fresh.HostSendMidi(NoteOn(60, 127, 0));
      ProcessSynth<double>(fresh, blockSize);

      fresh.HostSendMidi(NoteOff(60, offset));
      const std::vector<double> out = ProcessSynth<double>(fresh, blockSize);

      // Full level up to the note-off...
      bool heldUntilOffset = true;

      for (int s = 0; s < offset; s++)
        heldUntilOffset &= out[static_cast<size_t>(s)] == 1.;

      CHECK(heldUntilOffset);

      // ...then the release ramp...
      for (int s = offset; s < offset + release && s < blockSize; s++)
        CHECK(out[static_cast<size_t>(s)] == Approx(1. - (s - offset + 1) / static_cast<double>(release)).margin(1e-9));

      // ...then silence
      CHECK(IsSilent(out, offset + release));
      CHECK(IsSilent(ProcessSynth<double>(fresh, blockSize)));
    }
  }

  SECTION("a release which crosses a block boundary")
  {
    synth.HostSendMidi(NoteOff(60, blockSize - 4));
    const std::vector<double> out = ProcessSynth<double>(synth, blockSize);
    CHECK(out[static_cast<size_t>(blockSize - 5)] == 1.);
    CHECK(out.back() == Approx(1. - 4. / release));

    const std::vector<double> next = ProcessSynth<double>(synth, blockSize);
    CHECK(next.front() == Approx(1. - 5. / release));
    CHECK(next[static_cast<size_t>(release - 5)] == Approx(0.).margin(1e-9));
    CHECK(IsSilent(next, release - 4));

    CHECK(IsSilent(ProcessSynth<double>(synth, blockSize)));
  }

  SECTION("a note-on with zero velocity is a note-off")
  {
    synth.HostSendMidi(NoteOn(60, 0, 10));
    const std::vector<double> out = ProcessSynth<double>(synth, blockSize);

    CHECK(out[9] == 1.);
    CHECK(out[10] < 1.);
    CHECK(IsSilent(out, 10 + release));
  }

  SECTION("a note-off for a different note is ignored")
  {
    synth.HostSendMidi(NoteOff(61, 10));
    const std::vector<double> out = ProcessSynth<double>(synth, blockSize);

    CHECK(out.front() == 1.);
    CHECK(out.back() == 1.);
  }

  SECTION("note-on and note-off in the same block")
  {
    SynthPlug fresh;
    fresh.HostActivate(48000., blockSize);
    fresh.HostSendMidi(NoteOn(72, 127, 40));
    fresh.HostSendMidi(NoteOff(72, 90));

    const std::vector<double> out = ProcessSynth<double>(fresh, blockSize);

    CHECK(IsSilent(out, 0, 40));
    CHECK(out[40] == 1.);
    CHECK(out[89] == 1.);
    CHECK(out[90] < 1.);
    CHECK(IsSilent(out, 90 + release));
  }
}

TEST_CASE("Instrument: notes are polyphonic", "[integration][midi][instrument]")
{
  const int blockSize = 128;

  SynthPlug synth;
  synth.HostActivate(48000., blockSize);

  synth.HostSendMidi(NoteOn(60, 127, 10));
  synth.HostSendMidi(NoteOn(64, 127, 50));
  synth.HostSendMidi(NoteOn(67, 127, 90));

  const std::vector<double> out = ProcessSynth<double>(synth, blockSize);

  CHECK(IsSilent(out, 0, 10));
  CHECK(out[10] == 1.);
  CHECK(out[49] == 1.);
  CHECK(out[50] == 2.);
  CHECK(out[89] == 2.);
  CHECK(out[90] == 3.);
  CHECK(out[127] == 3.);

  int nTriggers = 0;

  for (const auto& pVoice : synth.mVoices)
    nTriggers += pVoice->mNumTriggers;

  CHECK(nTriggers == 3);

  SECTION("releasing one note leaves the others sounding")
  {
    auto countReleases = [&]() {
      int nReleases = 0;

      for (const auto& pVoice : synth.mVoices)
        nReleases += pVoice->mNumReleases;

      return nReleases;
    };

    const int nReleasesBefore = countReleases(); // resetting the synth releases every voice

    synth.HostSendMidi(NoteOff(64, 0));
    const std::vector<double> next = ProcessSynth<double>(synth, blockSize);

    CHECK(next.front() < 3.);
    CHECK(next.back() == 2.);
    CHECK(countReleases() == nReleasesBefore + 1);
  }
}

TEST_CASE("Instrument: notes played on a UI keyboard sound in the next block", "[integration][midi][instrument]")
{
  SynthPlug synth;
  synth.HostActivate(48000., 64);

  synth.SendMidiMsgFromUI(NoteOn(60, 127));

  const std::vector<float> out = ProcessSynth<float>(synth, 64);
  CHECK(out.front() == 1.f);
  CHECK(out.back() == 1.f);

  synth.SendMidiMsgFromUI(NoteOff(60));
  ProcessSynth<float>(synth, 64);
  CHECK(IsSilent(ProcessSynth<float>(synth, 64)));
}

TEST_CASE("Instrument: block sizes which are not a multiple of the synth's internal block", "[integration][midi][instrument]")
{
  const int blockSizes[] = {1, 7, 31, 33, 50, 100, 257};

  for (int blockSize : blockSizes)
  {
    INFO("block size " << blockSize);

    SynthPlug synth;
    synth.HostActivate(48000., blockSize);

    const int offset = blockSize / 2;
    synth.HostSendMidi(NoteOn(60, 127, offset));

    const std::vector<double> out = ProcessSynth<double>(synth, blockSize);

    CHECK(IsSilent(out, 0, offset));
    CHECK(out[static_cast<size_t>(offset)] == 1.);
    CHECK(out.back() == 1.);

    // Hold for a few blocks, then release and wait for silence
    for (int block = 0; block < 3; block++)
      CHECK(ProcessSynth<double>(synth, blockSize).back() == 1.);

    synth.HostSendMidi(NoteOff(60, 0));

    for (int block = 0; block <= SynthPlug::kReleaseSamples / blockSize + 1; block++)
      ProcessSynth<double>(synth, blockSize);

    CHECK(IsSilent(ProcessSynth<double>(synth, blockSize)));
  }
}

// MidiSynth calls its voices with blocks shorter than the ramp's transition when the host block size is not a multiple of its own
TEST_CASE("Instrument: ControlRamp::Write() does not write more than the frames asked for", "[integration][midi][instrument]")
{
  ControlRamp ramp;
  ramp.startValue = 1.;
  ramp.endValue = 1.;
  ramp.transitionStart = 31;
  ramp.transitionEnd = 32;

  std::vector<float> buffer(64, -1.f);
  ramp.Write(buffer.data(), 0, 16);

  for (size_t i = 0; i < 16; i++)
    CHECK(buffer[i] == 1.f);

  for (size_t i = 16; i < 64; i++)
    CHECK(buffer[i] == -1.f);
}
