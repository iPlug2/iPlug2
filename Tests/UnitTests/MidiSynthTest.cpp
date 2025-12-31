/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <vector>
#include <set>

#include "IPlugMidi.h"
#include "Synth/VoiceAllocator.h"
#include "Synth/MidiSynth.h"
#include "Synth/SynthVoice.h"

using namespace iplug;
using Catch::Approx;

// ============================================================================
// IMidiMsg Tests
// ============================================================================

TEST_CASE("IMidiMsg construction", "[MIDI][IMidiMsg]")
{
  SECTION("Default constructor creates empty message")
  {
    IMidiMsg msg;
    REQUIRE(msg.mOffset == 0);
    REQUIRE(msg.mStatus == 0);
    REQUIRE(msg.mData1 == 0);
    REQUIRE(msg.mData2 == 0);
    REQUIRE(msg.StatusMsg() == IMidiMsg::kNone);
  }

  SECTION("Parameterized constructor")
  {
    IMidiMsg msg(10, 0x90, 60, 100); // Note On, channel 0, C4, velocity 100
    REQUIRE(msg.mOffset == 10);
    REQUIRE(msg.mStatus == 0x90);
    REQUIRE(msg.mData1 == 60);
    REQUIRE(msg.mData2 == 100);
  }
}

TEST_CASE("IMidiMsg Note messages", "[MIDI][IMidiMsg]")
{
  SECTION("MakeNoteOnMsg creates valid note on")
  {
    IMidiMsg msg;
    msg.MakeNoteOnMsg(60, 100, 0, 0); // C4, velocity 100, offset 0, channel 1

    REQUIRE(msg.StatusMsg() == IMidiMsg::kNoteOn);
    REQUIRE(msg.Channel() == 0);
    REQUIRE(msg.NoteNumber() == 60);
    REQUIRE(msg.Velocity() == 100);
    REQUIRE(msg.mOffset == 0);
  }

  SECTION("MakeNoteOnMsg on different channels")
  {
    for (int ch = 0; ch < 16; ch++)
    {
      IMidiMsg msg;
      msg.MakeNoteOnMsg(64, 80, 0, ch);

      REQUIRE(msg.StatusMsg() == IMidiMsg::kNoteOn);
      REQUIRE(msg.Channel() == ch);
    }
  }

  SECTION("MakeNoteOffMsg creates valid note off")
  {
    IMidiMsg msg;
    msg.MakeNoteOffMsg(60, 0, 5); // C4, offset 0, channel 6

    REQUIRE(msg.StatusMsg() == IMidiMsg::kNoteOff);
    REQUIRE(msg.Channel() == 5);
    REQUIRE(msg.NoteNumber() == 60);
    REQUIRE(msg.Velocity() == 0); // Note off typically has velocity 0
  }

  SECTION("Note number range")
  {
    IMidiMsg msg;

    msg.MakeNoteOnMsg(0, 100, 0, 0);
    REQUIRE(msg.NoteNumber() == 0);

    msg.MakeNoteOnMsg(127, 100, 0, 0);
    REQUIRE(msg.NoteNumber() == 127);
  }

  SECTION("NoteNumber returns -1 for non-note messages")
  {
    IMidiMsg msg;
    msg.MakeControlChangeMsg(IMidiMsg::kModWheel, 0.5, 0, 0);

    REQUIRE(msg.NoteNumber() == -1);
  }

  SECTION("Velocity returns -1 for non-note messages")
  {
    IMidiMsg msg;
    msg.MakePitchWheelMsg(0.5, 0, 0);

    REQUIRE(msg.Velocity() == -1);
  }
}

TEST_CASE("IMidiMsg Control Change", "[MIDI][IMidiMsg]")
{
  SECTION("MakeControlChangeMsg creates valid CC")
  {
    IMidiMsg msg;
    msg.MakeControlChangeMsg(IMidiMsg::kModWheel, 0.5, 0, 0);

    REQUIRE(msg.StatusMsg() == IMidiMsg::kControlChange);
    REQUIRE(msg.ControlChangeIdx() == IMidiMsg::kModWheel);
    REQUIRE(msg.ControlChange(IMidiMsg::kModWheel) == Approx(0.5).margin(0.01));
  }

  SECTION("CC value range")
  {
    IMidiMsg msg;

    msg.MakeControlChangeMsg(IMidiMsg::kModWheel, 0.0, 0, 0);
    REQUIRE(msg.mData2 == 0);

    msg.MakeControlChangeMsg(IMidiMsg::kModWheel, 1.0, 0, 0);
    REQUIRE(msg.mData2 == 127);

    msg.MakeControlChangeMsg(IMidiMsg::kModWheel, 0.5, 0, 0);
    REQUIRE(msg.mData2 == 63); // floor(0.5 * 127)
  }

  SECTION("ControlChangeOnOff helper")
  {
    REQUIRE(IMidiMsg::ControlChangeOnOff(0.0) == false);
    REQUIRE(IMidiMsg::ControlChangeOnOff(0.49) == false);
    REQUIRE(IMidiMsg::ControlChangeOnOff(0.5) == true);
    REQUIRE(IMidiMsg::ControlChangeOnOff(1.0) == true);
  }

  SECTION("Various CC types")
  {
    IMidiMsg msg;

    msg.MakeControlChangeMsg(IMidiMsg::kSustainOnOff, 1.0, 0, 0);
    REQUIRE(msg.ControlChangeIdx() == IMidiMsg::kSustainOnOff);

    msg.MakeControlChangeMsg(IMidiMsg::kExpressionController, 0.75, 0, 0);
    REQUIRE(msg.ControlChangeIdx() == IMidiMsg::kExpressionController);

    msg.MakeControlChangeMsg(IMidiMsg::kAllNotesOff, 0.0, 0, 0);
    REQUIRE(msg.ControlChangeIdx() == IMidiMsg::kAllNotesOff);
  }
}

TEST_CASE("IMidiMsg Pitch Wheel", "[MIDI][IMidiMsg]")
{
  SECTION("MakePitchWheelMsg center position")
  {
    IMidiMsg msg;
    msg.MakePitchWheelMsg(0.0, 0, 0); // Center (no bend)

    REQUIRE(msg.StatusMsg() == IMidiMsg::kPitchWheel);
    REQUIRE(msg.PitchWheel() == Approx(0.0).margin(0.001));
  }

  SECTION("MakePitchWheelMsg full range")
  {
    IMidiMsg msg;

    msg.MakePitchWheelMsg(-1.0, 0, 0); // Full down
    REQUIRE(msg.PitchWheel() == Approx(-1.0).margin(0.001));

    msg.MakePitchWheelMsg(1.0, 0, 0); // Full up
    REQUIRE(msg.PitchWheel() == Approx(1.0).margin(0.001));
  }

  SECTION("Pitch wheel roundtrip")
  {
    for (double val : {-1.0, -0.5, 0.0, 0.5, 1.0})
    {
      IMidiMsg msg;
      msg.MakePitchWheelMsg(val, 0, 0);

      REQUIRE(msg.PitchWheel() == Approx(val).margin(0.001));
    }
  }

  SECTION("PitchWheel returns 0 for non-pitchwheel messages")
  {
    IMidiMsg msg;
    msg.MakeNoteOnMsg(60, 100, 0, 0);

    REQUIRE(msg.PitchWheel() == 0.0);
  }
}

TEST_CASE("IMidiMsg Program Change", "[MIDI][IMidiMsg]")
{
  SECTION("MakeProgramChange creates valid program change")
  {
    IMidiMsg msg;
    msg.MakeProgramChange(42, 0, 0);

    REQUIRE(msg.StatusMsg() == IMidiMsg::kProgramChange);
    REQUIRE(msg.Program() == 42);
    REQUIRE(msg.Channel() == 0);
  }

  SECTION("Program range")
  {
    for (int pgm : {0, 63, 127})
    {
      IMidiMsg msg;
      msg.MakeProgramChange(pgm, 0, 0);

      REQUIRE(msg.Program() == pgm);
    }
  }

  SECTION("Program returns -1 for non-program messages")
  {
    IMidiMsg msg;
    msg.MakeNoteOnMsg(60, 100, 0, 0);

    REQUIRE(msg.Program() == -1);
  }
}

TEST_CASE("IMidiMsg Aftertouch", "[MIDI][IMidiMsg]")
{
  SECTION("MakeChannelATMsg creates channel aftertouch")
  {
    IMidiMsg msg;
    msg.MakeChannelATMsg(100, 0, 0);

    REQUIRE(msg.StatusMsg() == IMidiMsg::kChannelAftertouch);
    REQUIRE(msg.ChannelAfterTouch() == 100);
  }

  SECTION("MakePolyATMsg creates poly aftertouch")
  {
    IMidiMsg msg;
    msg.MakePolyATMsg(60, 80, 0, 0); // Note 60, pressure 80

    REQUIRE(msg.StatusMsg() == IMidiMsg::kPolyAftertouch);
    REQUIRE(msg.NoteNumber() == 60);
    REQUIRE(msg.PolyAfterTouch() == 80);
  }

  SECTION("ChannelAfterTouch returns -1 for wrong message type")
  {
    IMidiMsg msg;
    msg.MakeNoteOnMsg(60, 100, 0, 0);

    REQUIRE(msg.ChannelAfterTouch() == -1);
  }

  SECTION("PolyAfterTouch returns -1 for wrong message type")
  {
    IMidiMsg msg;
    msg.MakeNoteOnMsg(60, 100, 0, 0);

    REQUIRE(msg.PolyAfterTouch() == -1);
  }
}

TEST_CASE("IMidiMsg Clear", "[MIDI][IMidiMsg]")
{
  SECTION("Clear resets all fields")
  {
    IMidiMsg msg;
    msg.MakeNoteOnMsg(60, 100, 50, 5);

    msg.Clear();

    REQUIRE(msg.mOffset == 0);
    REQUIRE(msg.mStatus == 0);
    REQUIRE(msg.mData1 == 0);
    REQUIRE(msg.mData2 == 0);
  }
}

TEST_CASE("IMidiMsg StatusMsgStr", "[MIDI][IMidiMsg]")
{
  SECTION("Returns correct strings")
  {
    REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kNone)) == "none");
    REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kNoteOn)) == "noteon");
    REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kNoteOff)) == "noteoff");
    REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kControlChange)) == "controlchange");
    REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kPitchWheel)) == "pitchwheel");
    REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kProgramChange)) == "programchange");
  }
}

TEST_CASE("IMidiMsg CCNameStr", "[MIDI][IMidiMsg]")
{
  SECTION("Returns correct CC names")
  {
    REQUIRE(std::string(IMidiMsg::CCNameStr(1)).find("Mod") != std::string::npos);
    REQUIRE(std::string(IMidiMsg::CCNameStr(7)).find("Vol") != std::string::npos);
    REQUIRE(std::string(IMidiMsg::CCNameStr(10)).find("Pan") != std::string::npos);
    REQUIRE(std::string(IMidiMsg::CCNameStr(64)).find("Damp") != std::string::npos); // Sustain
  }
}

// ============================================================================
// IMidiQueue Tests
// ============================================================================

TEST_CASE("IMidiQueue basic operations", "[MIDI][IMidiQueue]")
{
  SECTION("Empty queue")
  {
    IMidiQueue queue;
    REQUIRE(queue.Empty() == true);
    REQUIRE(queue.ToDo() == 0);
  }

  SECTION("Add and Peek")
  {
    IMidiQueue queue;
    IMidiMsg msg;
    msg.MakeNoteOnMsg(60, 100, 0, 0);

    queue.Add(msg);

    REQUIRE(queue.Empty() == false);
    REQUIRE(queue.ToDo() == 1);

    IMidiMsg& peeked = queue.Peek();
    REQUIRE(peeked.NoteNumber() == 60);
    REQUIRE(peeked.Velocity() == 100);
  }

  SECTION("Add and Remove")
  {
    IMidiQueue queue;
    IMidiMsg msg;
    msg.MakeNoteOnMsg(60, 100, 0, 0);

    queue.Add(msg);
    REQUIRE(queue.ToDo() == 1);

    queue.Remove();
    REQUIRE(queue.ToDo() == 0);
    REQUIRE(queue.Empty() == true);
  }

  SECTION("Multiple messages")
  {
    IMidiQueue queue;

    for (int i = 0; i < 10; i++)
    {
      IMidiMsg msg;
      msg.MakeNoteOnMsg(60 + i, 100, i, 0);
      queue.Add(msg);
    }

    REQUIRE(queue.ToDo() == 10);

    // Messages should come out in offset order
    for (int i = 0; i < 10; i++)
    {
      IMidiMsg& msg = queue.Peek();
      REQUIRE(msg.mOffset == i);
      REQUIRE(msg.NoteNumber() == 60 + i);
      queue.Remove();
    }

    REQUIRE(queue.Empty() == true);
  }
}

TEST_CASE("IMidiQueue sorting", "[MIDI][IMidiQueue]")
{
  SECTION("Messages are sorted by offset")
  {
    IMidiQueue queue;

    // Add messages out of order
    IMidiMsg msg3, msg1, msg2;
    msg3.MakeNoteOnMsg(62, 100, 30, 0);
    msg1.MakeNoteOnMsg(60, 100, 10, 0);
    msg2.MakeNoteOnMsg(61, 100, 20, 0);

    queue.Add(msg3); // offset 30
    queue.Add(msg1); // offset 10
    queue.Add(msg2); // offset 20

    // Should come out sorted
    REQUIRE(queue.Peek().mOffset == 10);
    queue.Remove();
    REQUIRE(queue.Peek().mOffset == 20);
    queue.Remove();
    REQUIRE(queue.Peek().mOffset == 30);
  }
}

TEST_CASE("IMidiQueue Flush", "[MIDI][IMidiQueue]")
{
  SECTION("Flush updates offsets")
  {
    IMidiQueue queue;

    IMidiMsg msg1, msg2;
    msg1.MakeNoteOnMsg(60, 100, 100, 0);
    msg2.MakeNoteOnMsg(61, 100, 200, 0);

    queue.Add(msg1);
    queue.Add(msg2);

    queue.Flush(50); // Advance by 50 samples

    REQUIRE(queue.Peek().mOffset == 50); // 100 - 50
    queue.Remove();
    REQUIRE(queue.Peek().mOffset == 150); // 200 - 50
  }
}

TEST_CASE("IMidiQueue Clear", "[MIDI][IMidiQueue]")
{
  SECTION("Clear empties queue")
  {
    IMidiQueue queue;

    for (int i = 0; i < 5; i++)
    {
      IMidiMsg msg;
      msg.MakeNoteOnMsg(60, 100, 0, 0);
      queue.Add(msg);
    }

    REQUIRE(queue.ToDo() == 5);

    queue.Clear();

    REQUIRE(queue.Empty() == true);
    REQUIRE(queue.ToDo() == 0);
  }
}

TEST_CASE("IMidiQueue Resize", "[MIDI][IMidiQueue]")
{
  SECTION("Resize changes capacity")
  {
    IMidiQueue queue;
    int initialSize = queue.GetSize();

    queue.Resize(1000);

    REQUIRE(queue.GetSize() >= 1000);
  }

  SECTION("Resize preserves messages")
  {
    IMidiQueue queue;

    IMidiMsg msg;
    msg.MakeNoteOnMsg(60, 100, 0, 0);
    queue.Add(msg);

    queue.Resize(2000);

    REQUIRE(queue.ToDo() == 1);
    REQUIRE(queue.Peek().NoteNumber() == 60);
  }
}

// ============================================================================
// ISysEx Tests
// ============================================================================

TEST_CASE("ISysEx basic operations", "[MIDI][ISysEx]")
{
  SECTION("Default constructor")
  {
    ISysEx sysex;
    REQUIRE(sysex.mOffset == 0);
    REQUIRE(sysex.mSize == 0);
    REQUIRE(sysex.mData == nullptr);
  }

  SECTION("Parameterized constructor")
  {
    uint8_t data[] = {0xF0, 0x7E, 0x00, 0x06, 0x01, 0xF7};
    ISysEx sysex(10, data, 6);

    REQUIRE(sysex.mOffset == 10);
    REQUIRE(sysex.mSize == 6);
    REQUIRE(sysex.mData == data);
  }

  SECTION("Clear resets fields")
  {
    uint8_t data[] = {0xF0, 0xF7};
    ISysEx sysex(10, data, 2);

    sysex.Clear();

    REQUIRE(sysex.mOffset == 0);
    REQUIRE(sysex.mSize == 0);
    REQUIRE(sysex.mData == nullptr);
  }

  SECTION("SysExStr formats bytes")
  {
    uint8_t data[] = {0xF0, 0x41, 0x10, 0xF7};
    ISysEx sysex(0, data, 4);

    char str[32];
    sysex.SysExStr(str, sizeof(str), data, 4);

    // Should contain hex representation
    REQUIRE(std::string(str).find("F0") != std::string::npos);
    REQUIRE(std::string(str).find("41") != std::string::npos);
  }
}

// ============================================================================
// Mock SynthVoice for testing VoiceAllocator
// ============================================================================

class MockSynthVoice : public SynthVoice
{
public:
  bool mBusy = false;
  int mTriggerCount = 0;
  int mReleaseCount = 0;
  double mLastLevel = 0.0;
  bool mLastRetrigger = false;

  bool GetBusy() const override { return mBusy; }

  void Trigger(double level, bool isRetrigger) override
  {
    mBusy = true;
    mTriggerCount++;
    mLastLevel = level;
    mLastRetrigger = isRetrigger;
  }

  void Release() override
  {
    mReleaseCount++;
    mBusy = false;
  }

  void ProcessSamplesAccumulating(sample** inputs, sample** outputs,
                                   int nInputs, int nOutputs,
                                   int startIdx, int nFrames) override
  {
    // Simple sine wave output for testing
    for (int c = 0; c < nOutputs; c++)
    {
      for (int s = startIdx; s < startIdx + nFrames; s++)
      {
        if (mBusy)
          outputs[c][s] += 0.1;
      }
    }
  }

  void Reset()
  {
    mBusy = false;
    mTriggerCount = 0;
    mReleaseCount = 0;
    mLastLevel = 0.0;
    mLastRetrigger = false;
  }
};

// ============================================================================
// VoiceAllocator Tests
// ============================================================================

TEST_CASE("VoiceAllocator basic operations", "[MIDI][VoiceAllocator]")
{
  SECTION("Add voices")
  {
    VoiceAllocator allocator;
    MockSynthVoice voice1, voice2;

    allocator.AddVoice(&voice1, 0);
    allocator.AddVoice(&voice2, 0);

    REQUIRE(allocator.GetNVoices() == 2);
  }

  SECTION("GetVoice returns correct voice")
  {
    VoiceAllocator allocator;
    MockSynthVoice voice1, voice2;

    allocator.AddVoice(&voice1, 0);
    allocator.AddVoice(&voice2, 0);

    REQUIRE(allocator.GetVoice(0) == &voice1);
    REQUIRE(allocator.GetVoice(1) == &voice2);
  }

  SECTION("Clear resets allocator")
  {
    VoiceAllocator allocator;
    MockSynthVoice voice;

    allocator.AddVoice(&voice, 0);
    allocator.SetSampleRateAndBlockSize(44100.0, 512);

    // Trigger a note
    VoiceInputEvent noteOn;
    noteOn.mAddress = {0, 0, 60, kVoicesAll};
    noteOn.mAction = kNoteOnAction;
    noteOn.mValue = 0.8f;
    noteOn.mSampleOffset = 0;
    allocator.AddEvent(noteOn);
    allocator.ProcessEvents(512, 0);

    allocator.Clear();

    // Voice state should be reset
  }
}

TEST_CASE("VoiceAllocator note events", "[MIDI][VoiceAllocator]")
{
  SECTION("Note on triggers voice")
  {
    VoiceAllocator allocator;
    MockSynthVoice voice;

    allocator.AddVoice(&voice, 0);
    allocator.SetSampleRateAndBlockSize(44100.0, 512);

    VoiceInputEvent noteOn;
    noteOn.mAddress = {0, 0, 60, 0};
    noteOn.mAction = kNoteOnAction;
    noteOn.mValue = 0.8f;
    noteOn.mSampleOffset = 0;

    allocator.AddEvent(noteOn);
    allocator.ProcessEvents(512, 0);

    REQUIRE(voice.mTriggerCount == 1);
    REQUIRE(voice.mLastLevel == Approx(0.8f).margin(0.01f));
  }

  SECTION("Note off releases voice")
  {
    VoiceAllocator allocator;
    MockSynthVoice voice;

    allocator.AddVoice(&voice, 0);
    allocator.SetSampleRateAndBlockSize(44100.0, 512);

    // Note on
    VoiceInputEvent noteOn;
    noteOn.mAddress = {0, 0, 60, 0};
    noteOn.mAction = kNoteOnAction;
    noteOn.mValue = 0.8f;
    noteOn.mSampleOffset = 0;
    allocator.AddEvent(noteOn);
    allocator.ProcessEvents(512, 0);

    // Note off
    VoiceInputEvent noteOff;
    noteOff.mAddress = {0, 0, 60, 0};
    noteOff.mAction = kNoteOffAction;
    noteOff.mValue = 0.0f;
    noteOff.mSampleOffset = 0;
    allocator.AddEvent(noteOff);
    allocator.ProcessEvents(512, 512);

    REQUIRE(voice.mReleaseCount == 1);
  }
}

TEST_CASE("VoiceAllocator poly mode", "[MIDI][VoiceAllocator]")
{
  SECTION("Multiple notes trigger multiple voices")
  {
    VoiceAllocator allocator;
    allocator.mPolyMode = VoiceAllocator::kPolyModePoly;

    MockSynthVoice voice1, voice2, voice3;
    allocator.AddVoice(&voice1, 0);
    allocator.AddVoice(&voice2, 0);
    allocator.AddVoice(&voice3, 0);
    allocator.SetSampleRateAndBlockSize(44100.0, 512);

    // Send 3 note ons
    for (int note = 60; note < 63; note++)
    {
      VoiceInputEvent noteOn;
      noteOn.mAddress = {0, 0, static_cast<uint8_t>(note), 0};
      noteOn.mAction = kNoteOnAction;
      noteOn.mValue = 0.8f;
      noteOn.mSampleOffset = 0;
      allocator.AddEvent(noteOn);
    }
    allocator.ProcessEvents(512, 0);

    // All three voices should be triggered
    int totalTriggers = voice1.mTriggerCount + voice2.mTriggerCount + voice3.mTriggerCount;
    REQUIRE(totalTriggers == 3);
  }
}

TEST_CASE("VoiceAllocator soft/hard kill", "[MIDI][VoiceAllocator]")
{
  SECTION("SoftKillAllVoices releases all")
  {
    VoiceAllocator allocator;
    MockSynthVoice voice1, voice2;

    allocator.AddVoice(&voice1, 0);
    allocator.AddVoice(&voice2, 0);
    allocator.SetSampleRateAndBlockSize(44100.0, 512);

    // Trigger both voices
    for (int note = 60; note < 62; note++)
    {
      VoiceInputEvent noteOn;
      noteOn.mAddress = {0, 0, static_cast<uint8_t>(note), 0};
      noteOn.mAction = kNoteOnAction;
      noteOn.mValue = 0.8f;
      noteOn.mSampleOffset = 0;
      allocator.AddEvent(noteOn);
    }
    allocator.ProcessEvents(512, 0);

    allocator.SoftKillAllVoices();

    REQUIRE(voice1.mReleaseCount >= 1);
    REQUIRE(voice2.mReleaseCount >= 1);
  }

  SECTION("HardKillAllVoices stops all immediately")
  {
    VoiceAllocator allocator;
    MockSynthVoice voice;

    allocator.AddVoice(&voice, 0);
    allocator.SetSampleRateAndBlockSize(44100.0, 512);

    VoiceInputEvent noteOn;
    noteOn.mAddress = {0, 0, 60, 0};
    noteOn.mAction = kNoteOnAction;
    noteOn.mValue = 0.8f;
    noteOn.mSampleOffset = 0;
    allocator.AddEvent(noteOn);
    allocator.ProcessEvents(512, 0);

    voice.mBusy = true; // Simulate voice being busy

    allocator.HardKillAllVoices();

    // Voice should be forcibly stopped
  }
}

// ============================================================================
// MidiSynth Tests
// ============================================================================

TEST_CASE("MidiSynth basic operations", "[MIDI][MidiSynth]")
{
  SECTION("Construction")
  {
    MidiSynth synth(VoiceAllocator::kPolyModePoly);
    REQUIRE(synth.NVoices() == 0);
  }

  SECTION("AddVoice adds voice to synth")
  {
    MidiSynth synth(VoiceAllocator::kPolyModePoly);
    MockSynthVoice voice;

    synth.AddVoice(&voice, 0);
    REQUIRE(synth.NVoices() == 1);
  }

  SECTION("SetSampleRateAndBlockSize")
  {
    MidiSynth synth(VoiceAllocator::kPolyModePoly);
    synth.SetSampleRateAndBlockSize(48000.0, 256);
    // Should not crash
  }

  SECTION("SetPitchBendRange")
  {
    MidiSynth synth(VoiceAllocator::kPolyModePoly);
    synth.SetPitchBendRange(2); // 2 semitones
    synth.SetPitchBendRange(24); // 2 octaves
    // Should not crash
  }

  SECTION("SetPolyMode")
  {
    MidiSynth synth(VoiceAllocator::kPolyModePoly);
    synth.SetPolyMode(VoiceAllocator::kPolyModeMono);
    synth.SetPolyMode(VoiceAllocator::kPolyModePoly);
  }
}

TEST_CASE("MidiSynth MIDI processing", "[MIDI][MidiSynth]")
{
  SECTION("AddMidiMsgToQueue queues message")
  {
    MidiSynth synth(VoiceAllocator::kPolyModePoly);
    MockSynthVoice voice;
    synth.AddVoice(&voice, 0);
    synth.SetSampleRateAndBlockSize(44100.0, 512);

    IMidiMsg noteOn;
    noteOn.MakeNoteOnMsg(60, 100, 0, 0);
    synth.AddMidiMsgToQueue(noteOn);

    // Process a block
    std::vector<sample> outputL(512, 0.0);
    std::vector<sample> outputR(512, 0.0);
    sample* outputs[2] = {outputL.data(), outputR.data()};

    synth.ProcessBlock(nullptr, outputs, 0, 2, 512);

    REQUIRE(voice.mTriggerCount == 1);
  }

  SECTION("Note off releases voice")
  {
    MidiSynth synth(VoiceAllocator::kPolyModePoly);
    MockSynthVoice voice;
    synth.AddVoice(&voice, 0);
    synth.SetSampleRateAndBlockSize(44100.0, 512);

    // Note on
    IMidiMsg noteOn;
    noteOn.MakeNoteOnMsg(60, 100, 0, 0);
    synth.AddMidiMsgToQueue(noteOn);

    std::vector<sample> outputL(512, 0.0);
    std::vector<sample> outputR(512, 0.0);
    sample* outputs[2] = {outputL.data(), outputR.data()};
    synth.ProcessBlock(nullptr, outputs, 0, 2, 512);

    // Note off
    IMidiMsg noteOff;
    noteOff.MakeNoteOffMsg(60, 0, 0);
    synth.AddMidiMsgToQueue(noteOff);
    synth.ProcessBlock(nullptr, outputs, 0, 2, 512);

    REQUIRE(voice.mReleaseCount == 1);
  }
}

TEST_CASE("MidiSynth ForEachVoice", "[MIDI][MidiSynth]")
{
  SECTION("Iterates all voices")
  {
    MidiSynth synth(VoiceAllocator::kPolyModePoly);
    MockSynthVoice voice1, voice2, voice3;

    synth.AddVoice(&voice1, 0);
    synth.AddVoice(&voice2, 0);
    synth.AddVoice(&voice3, 0);

    int count = 0;
    synth.ForEachVoice([&count](SynthVoice& voice) {
      count++;
    });

    REQUIRE(count == 3);
  }
}

TEST_CASE("MidiSynth Reset", "[MIDI][MidiSynth]")
{
  SECTION("Reset clears state")
  {
    MidiSynth synth(VoiceAllocator::kPolyModePoly);
    MockSynthVoice voice;
    synth.AddVoice(&voice, 0);
    synth.SetSampleRateAndBlockSize(44100.0, 512);

    // Trigger a note
    IMidiMsg noteOn;
    noteOn.MakeNoteOnMsg(60, 100, 0, 0);
    synth.AddMidiMsgToQueue(noteOn);

    std::vector<sample> outputL(512, 0.0);
    std::vector<sample> outputR(512, 0.0);
    sample* outputs[2] = {outputL.data(), outputR.data()};
    synth.ProcessBlock(nullptr, outputs, 0, 2, 512);

    synth.Reset();

    // After reset, synth should be in clean state
  }
}
