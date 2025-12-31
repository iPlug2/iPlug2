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
#include <string>

#include "IPlugMidi.h"

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
