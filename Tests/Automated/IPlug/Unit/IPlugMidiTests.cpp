/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

// Unit tests for IPlugMidi.h: IMidiMsg, ISysEx and IMidiQueue.

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include "IPlugMidi.h"

using namespace iplug;
using Catch::Approx;

namespace
{
IMidiMsg NoteOnAt(int offset, int note = 60)
{
  IMidiMsg msg;
  msg.MakeNoteOnMsg(note, 100, offset);
  return msg;
}

/** Removes everything from a queue, returning the (offset, note number) pairs in the order they came out */
std::vector<std::pair<int, int>> Drain(IMidiQueue& queue)
{
  std::vector<std::pair<int, int>> result;
  while (!queue.Empty())
  {
    const IMidiMsg& msg = queue.Peek();
    result.emplace_back(msg.mOffset, msg.NoteNumber());
    queue.Remove();
  }
  return result;
}

std::string SysExToString(const std::vector<uint8_t>& data, int maxLen)
{
  std::vector<char> buf(static_cast<size_t>(maxLen) + 1, '#');
  ISysEx sysex;
  char* result = sysex.SysExStr(buf.data(), maxLen, data.empty() ? nullptr : data.data(), static_cast<int>(data.size()));
  // the result must be the caller's buffer, terminated inside it
  if (result != buf.data() || memchr(buf.data(), '\0', static_cast<size_t>(maxLen)) == nullptr || buf[maxLen] != '#')
    return "<invalid>";
  return result;
}
} // namespace

TEST_CASE("IMidiMsg: construction", "[unit][midi]")
{
  IMidiMsg def;
  REQUIRE(def.mOffset == 0);
  REQUIRE(def.mStatus == 0);
  REQUIRE(def.mData1 == 0);
  REQUIRE(def.mData2 == 0);
  REQUIRE(def.StatusMsg() == IMidiMsg::kNone);
  REQUIRE(def.Channel() == 0);

  IMidiMsg msg {12, 0x93, 64, 127};
  REQUIRE(msg.mOffset == 12);
  REQUIRE(msg.mStatus == 0x93);
  REQUIRE(msg.mData1 == 64);
  REQUIRE(msg.mData2 == 127);
  REQUIRE(msg.StatusMsg() == IMidiMsg::kNoteOn);
  REQUIRE(msg.Channel() == 3);
  REQUIRE(msg.NoteNumber() == 64);
  REQUIRE(msg.Velocity() == 127);

  IMidiMsg copy = msg;
  REQUIRE(copy.mOffset == 12);
  REQUIRE(copy.mStatus == 0x93);
  REQUIRE(copy.mData1 == 64);
  REQUIRE(copy.mData2 == 127);
}

TEST_CASE("IMidiMsg: StatusMsg and Channel decode the status byte", "[unit][midi]")
{
  const int nibble = GENERATE(range(0, 16));
  const int channel = GENERATE(0, 1, 9, 15);
  CAPTURE(nibble, channel);

  IMidiMsg msg {0, static_cast<uint8_t>((nibble << 4) | channel), 1, 2};

  REQUIRE(msg.Channel() == channel);

  if (nibble >= 8 && nibble <= 14)
  {
    REQUIRE(static_cast<int>(msg.StatusMsg()) == nibble);
  }
  else
  {
    // data bytes and system messages
    REQUIRE(msg.StatusMsg() == IMidiMsg::kNone);
  }
}

TEST_CASE("IMidiMsg: status constants match the MIDI specification", "[unit][midi]")
{
  REQUIRE(IMidiMsg::kNoteOff == 0x8);
  REQUIRE(IMidiMsg::kNoteOn == 0x9);
  REQUIRE(IMidiMsg::kPolyAftertouch == 0xA);
  REQUIRE(IMidiMsg::kControlChange == 0xB);
  REQUIRE(IMidiMsg::kProgramChange == 0xC);
  REQUIRE(IMidiMsg::kChannelAftertouch == 0xD);
  REQUIRE(IMidiMsg::kPitchWheel == 0xE);

  REQUIRE(IMidiMsg::kModWheel == 1);
  REQUIRE(IMidiMsg::kChannelVolume == 7);
  REQUIRE(IMidiMsg::kPan == 10);
  REQUIRE(IMidiMsg::kExpressionController == 11);
  REQUIRE(IMidiMsg::kSustainOnOff == 64);
  REQUIRE(IMidiMsg::kCutoffFrequency == 74);
  REQUIRE(IMidiMsg::kAllNotesOff == 123);
  REQUIRE(IMidiMsg::kNoCC == -1);
}

TEST_CASE("IMidiMsg: MakeNoteOnMsg", "[unit][midi]")
{
  IMidiMsg msg {99, 0xFF, 0xFF, 0xFF};

  SECTION("default channel")
  {
    msg.MakeNoteOnMsg(60, 100, 32);
    REQUIRE(msg.mStatus == 0x90);
    REQUIRE(msg.mData1 == 60);
    REQUIRE(msg.mData2 == 100);
    REQUIRE(msg.mOffset == 32);
    REQUIRE(msg.StatusMsg() == IMidiMsg::kNoteOn);
    REQUIRE(msg.Channel() == 0);
    REQUIRE(msg.NoteNumber() == 60);
    REQUIRE(msg.Velocity() == 100);
  }

  SECTION("all channels and the note/velocity extremes")
  {
    const int channel = GENERATE(range(0, 16));
    const int note = GENERATE(0, 1, 64, 127);
    const int velocity = GENERATE(0, 1, 127);
    msg.MakeNoteOnMsg(note, velocity, 7, channel);
    REQUIRE(msg.mStatus == (0x90 | channel));
    REQUIRE(msg.StatusMsg() == IMidiMsg::kNoteOn);
    REQUIRE(msg.Channel() == channel);
    REQUIRE(msg.NoteNumber() == note);
    REQUIRE(msg.Velocity() == velocity);
    REQUIRE(msg.mOffset == 7);
  }

  SECTION("the accessors of other message types report not-applicable")
  {
    msg.MakeNoteOnMsg(60, 100, 0, 2);
    REQUIRE(msg.PolyAfterTouch() == -1);
    REQUIRE(msg.ChannelAfterTouch() == -1);
    REQUIRE(msg.Program() == -1);
    REQUIRE(msg.PitchWheel() == 0.);
    REQUIRE(msg.ControlChange(static_cast<IMidiMsg::EControlChangeMsg>(60)) == -1.);
  }
}

TEST_CASE("IMidiMsg: MakeNoteOffMsg", "[unit][midi]")
{
  IMidiMsg msg {99, 0xFF, 0xFF, 0xFF};

  msg.MakeNoteOffMsg(72, 5);
  REQUIRE(msg.mStatus == 0x80);
  REQUIRE(msg.mData1 == 72);
  REQUIRE(msg.mData2 == 0);
  REQUIRE(msg.mOffset == 5);
  REQUIRE(msg.StatusMsg() == IMidiMsg::kNoteOff);
  REQUIRE(msg.NoteNumber() == 72);
  REQUIRE(msg.Velocity() == 0);
  REQUIRE(msg.Channel() == 0);

  const int channel = GENERATE(0, 5, 15);
  msg.MakeNoteOffMsg(127, 1000, channel);
  REQUIRE(msg.mStatus == (0x80 | channel));
  REQUIRE(msg.Channel() == channel);
  REQUIRE(msg.NoteNumber() == 127);
  REQUIRE(msg.mOffset == 1000);
  REQUIRE(msg.PolyAfterTouch() == -1);
  REQUIRE(msg.Program() == -1);

  SECTION("a note off with release velocity built by hand")
  {
    IMidiMsg raw {0, 0x81, 60, 45};
    REQUIRE(raw.StatusMsg() == IMidiMsg::kNoteOff);
    REQUIRE(raw.Velocity() == 45);
  }
}

TEST_CASE("IMidiMsg: MakePitchWheelMsg and PitchWheel", "[unit][midi]")
{
  IMidiMsg msg {99, 0xFF, 0xFF, 0xFF};

  SECTION("centre")
  {
    msg.MakePitchWheelMsg(0.);
    REQUIRE(msg.mStatus == 0xE0);
    REQUIRE(msg.mData1 == 0x00);
    REQUIRE(msg.mData2 == 0x40);
    REQUIRE(msg.mOffset == 0);
    REQUIRE(msg.StatusMsg() == IMidiMsg::kPitchWheel);
    REQUIRE(msg.PitchWheel() == 0.);
  }

  SECTION("minimum")
  {
    msg.MakePitchWheelMsg(-1.);
    REQUIRE(msg.mData1 == 0x00);
    REQUIRE(msg.mData2 == 0x00);
    REQUIRE(msg.PitchWheel() == -1.);
  }

  SECTION("maximum is the largest 14 bit value")
  {
    msg.MakePitchWheelMsg(1.);
    REQUIRE(msg.mData1 == 0x7F);
    REQUIRE(msg.mData2 == 0x7F);
    REQUIRE(msg.PitchWheel() == Approx(8191. / 8192.));
    REQUIRE(msg.PitchWheel() <= 1.);
  }

  SECTION("out of range values are clamped and data bytes stay 7 bit")
  {
    auto value = GENERATE(-100., -1.5, -1.0001, 1.0001, 1.5, 100.);
    CAPTURE(value);
    msg.MakePitchWheelMsg(value);
    REQUIRE(msg.mData1 <= 0x7F);
    REQUIRE(msg.mData2 <= 0x7F);
    REQUIRE(msg.PitchWheel() == Approx(value < 0. ? -1. : 8191. / 8192.));
  }

  SECTION("round trips within the 14 bit resolution")
  {
    auto value = GENERATE(-1., -0.75, -0.5, -0.123, -0.0001, 0., 0.0001, 0.25, 0.5, 0.987);
    CAPTURE(value);
    msg.MakePitchWheelMsg(value);
    REQUIRE(msg.mData1 <= 0x7F);
    REQUIRE(msg.mData2 <= 0x7F);
    REQUIRE(msg.PitchWheel() == Approx(value).margin(1. / 8192.));
  }

  SECTION("exactly representable values survive unchanged")
  {
    auto value = GENERATE(-0.5, -0.25, 0.25, 0.5, 0.75);
    msg.MakePitchWheelMsg(value);
    REQUIRE(msg.PitchWheel() == value);
  }

  SECTION("channel and offset")
  {
    msg.MakePitchWheelMsg(0.5, 11, 77);
    REQUIRE(msg.mStatus == 0xEB);
    REQUIRE(msg.Channel() == 11);
    REQUIRE(msg.mOffset == 77);
    REQUIRE(msg.mData1 == 0x00);
    REQUIRE(msg.mData2 == 0x60);
    REQUIRE(msg.NoteNumber() == -1);
    REQUIRE(msg.Velocity() == -1);
  }

  SECTION("hand built messages decode LSB then MSB")
  {
    IMidiMsg raw {0, 0xE0, 0x01, 0x40};
    REQUIRE(raw.PitchWheel() == Approx(1. / 8192.));
    IMidiMsg raw2 {0, 0xE0, 0x7F, 0x3F};
    REQUIRE(raw2.PitchWheel() == Approx(-1. / 8192.));
  }

  SECTION("other messages report zero")
  {
    IMidiMsg note {0, 0x90, 0x7F, 0x7F};
    REQUIRE(note.PitchWheel() == 0.);
  }
}

TEST_CASE("IMidiMsg: MakeControlChangeMsg and ControlChange", "[unit][midi]")
{
  IMidiMsg msg {99, 0xFF, 0xFF, 0xFF};

  SECTION("extremes")
  {
    msg.MakeControlChangeMsg(IMidiMsg::kModWheel, 0.);
    REQUIRE(msg.mStatus == 0xB0);
    REQUIRE(msg.mData1 == 1);
    REQUIRE(msg.mData2 == 0);
    REQUIRE(msg.mOffset == 0);
    REQUIRE(msg.StatusMsg() == IMidiMsg::kControlChange);
    REQUIRE(msg.ControlChangeIdx() == IMidiMsg::kModWheel);
    REQUIRE(msg.ControlChange(IMidiMsg::kModWheel) == 0.);

    msg.MakeControlChangeMsg(IMidiMsg::kModWheel, 1.);
    REQUIRE(msg.mData2 == 127);
    REQUIRE(msg.ControlChange(IMidiMsg::kModWheel) == 1.);
  }

  SECTION("round trips within the 7 bit resolution")
  {
    auto value = GENERATE(0., 0.01, 0.25, 0.5, 0.75, 0.99, 1.);
    CAPTURE(value);
    msg.MakeControlChangeMsg(IMidiMsg::kChannelVolume, value);
    REQUIRE(msg.mData2 <= 127);
    REQUIRE(msg.ControlChange(IMidiMsg::kChannelVolume) == Approx(value).margin(1. / 127.));
    REQUIRE(msg.ControlChange(IMidiMsg::kChannelVolume) <= value);
  }

  SECTION("every 7 bit value can be produced and read back")
  {
    bool allEqual = true;
    for (int i = 0; i < 127; i++)
    {
      // aim for the middle of each step so the result does not depend on rounding at the step edges
      msg.MakeControlChangeMsg(IMidiMsg::kPan, (i + 0.5) / 127.);
      allEqual = allEqual && (msg.mData2 == i) && (msg.ControlChange(IMidiMsg::kPan) == Approx(i / 127.));
    }
    REQUIRE(allEqual);
  }

  SECTION("channel, offset and controller index")
  {
    msg.MakeControlChangeMsg(IMidiMsg::kSustainOnOff, 1., 4, 21);
    REQUIRE(msg.mStatus == 0xB4);
    REQUIRE(msg.Channel() == 4);
    REQUIRE(msg.mOffset == 21);
    REQUIRE(msg.mData1 == 64);
    REQUIRE(msg.ControlChangeIdx() == IMidiMsg::kSustainOnOff);

    msg.MakeControlChangeMsg(IMidiMsg::kAllNotesOff, 0., 15);
    REQUIRE(msg.ControlChangeIdx() == IMidiMsg::kAllNotesOff);
    REQUIRE(msg.Channel() == 15);
  }

  SECTION("asking for a different controller reports not-applicable")
  {
    msg.MakeControlChangeMsg(IMidiMsg::kPan, 0.5);
    REQUIRE(msg.ControlChange(IMidiMsg::kPan) == Approx(63. / 127.));
    REQUIRE(msg.ControlChange(IMidiMsg::kModWheel) == -1.);
    REQUIRE(msg.ControlChange(IMidiMsg::kChannelVolume) == -1.);
    REQUIRE(msg.NoteNumber() == -1);
    REQUIRE(msg.Velocity() == -1);
    REQUIRE(msg.Program() == -1);
  }

  SECTION("messages that are not control changes report not-applicable")
  {
    IMidiMsg note;
    note.MakeNoteOnMsg(IMidiMsg::kPan, 100, 0);
    REQUIRE(note.ControlChange(IMidiMsg::kPan) == -1.);
  }

  SECTION("ControlChangeOnOff switches at the half way point")
  {
    REQUIRE_FALSE(IMidiMsg::ControlChangeOnOff(0.));
    REQUIRE_FALSE(IMidiMsg::ControlChangeOnOff(0.49));
    REQUIRE(IMidiMsg::ControlChangeOnOff(0.5));
    REQUIRE(IMidiMsg::ControlChangeOnOff(1.));
    REQUIRE_FALSE(IMidiMsg::ControlChangeOnOff(-1.)); // the not-applicable value is "off"

    // MIDI values 0-63 are off, 64-127 are on
    IMidiMsg pedal {0, 0xB0, 64, 63};
    REQUIRE_FALSE(IMidiMsg::ControlChangeOnOff(pedal.ControlChange(IMidiMsg::kSustainOnOff)));
    pedal.mData2 = 64;
    REQUIRE(IMidiMsg::ControlChangeOnOff(pedal.ControlChange(IMidiMsg::kSustainOnOff)));
  }
}

TEST_CASE("IMidiMsg: MakeProgramChange", "[unit][midi]")
{
  IMidiMsg msg {99, 0xFF, 0xFF, 0xFF};

  msg.MakeProgramChange(5);
  REQUIRE(msg.mStatus == 0xC0);
  REQUIRE(msg.mData1 == 5);
  REQUIRE(msg.mData2 == 0);
  REQUIRE(msg.mOffset == 0);
  REQUIRE(msg.StatusMsg() == IMidiMsg::kProgramChange);
  REQUIRE(msg.Program() == 5);
  REQUIRE(msg.NoteNumber() == -1);
  REQUIRE(msg.Velocity() == -1);
  REQUIRE(msg.ChannelAfterTouch() == -1);

  const int program = GENERATE(0, 1, 126, 127);
  msg.MakeProgramChange(program, 9, 300);
  REQUIRE(msg.mStatus == 0xC9);
  REQUIRE(msg.Program() == program);
  REQUIRE(msg.Channel() == 9);
  REQUIRE(msg.mOffset == 300);
}

TEST_CASE("IMidiMsg: MakeChannelATMsg", "[unit][midi]")
{
  IMidiMsg msg {99, 0xFF, 0xFF, 0xFF};

  const int pressure = GENERATE(0, 64, 127);
  msg.MakeChannelATMsg(pressure, 13, 6);
  REQUIRE(msg.mStatus == 0xD6);
  REQUIRE(msg.mData1 == pressure);
  REQUIRE(msg.mData2 == 0);
  REQUIRE(msg.mOffset == 13);
  REQUIRE(msg.StatusMsg() == IMidiMsg::kChannelAftertouch);
  REQUIRE(msg.Channel() == 6);
  REQUIRE(msg.ChannelAfterTouch() == pressure);
  REQUIRE(msg.PolyAfterTouch() == -1);
  REQUIRE(msg.NoteNumber() == -1);
  REQUIRE(msg.Velocity() == -1);
  REQUIRE(msg.Program() == -1);
}

TEST_CASE("IMidiMsg: MakePolyATMsg", "[unit][midi]")
{
  IMidiMsg msg {99, 0xFF, 0xFF, 0xFF};

  const int pressure = GENERATE(0, 64, 127);
  msg.MakePolyATMsg(61, pressure, 19, 3);
  REQUIRE(msg.mStatus == 0xA3);
  REQUIRE(msg.mData1 == 61);
  REQUIRE(msg.mData2 == pressure);
  REQUIRE(msg.mOffset == 19);
  REQUIRE(msg.StatusMsg() == IMidiMsg::kPolyAftertouch);
  REQUIRE(msg.Channel() == 3);
  REQUIRE(msg.NoteNumber() == 61);
  REQUIRE(msg.PolyAfterTouch() == pressure);
  REQUIRE(msg.ChannelAfterTouch() == -1);
  REQUIRE(msg.Velocity() == -1);
  REQUIRE(msg.Program() == -1);
}

TEST_CASE("IMidiMsg: Clear", "[unit][midi]")
{
  IMidiMsg msg;
  msg.MakeNoteOnMsg(60, 100, 50, 5);
  msg.Clear();
  REQUIRE(msg.mOffset == 0);
  REQUIRE(msg.mStatus == 0);
  REQUIRE(msg.mData1 == 0);
  REQUIRE(msg.mData2 == 0);
  REQUIRE(msg.StatusMsg() == IMidiMsg::kNone);
  REQUIRE(msg.NoteNumber() == -1);
  REQUIRE(msg.Velocity() == -1);
}

TEST_CASE("IMidiMsg: each Make function starts from a clean message", "[unit][midi]")
{
  IMidiMsg msg {55, 0x9F, 0x7F, 0x7F};

  SECTION("program change clears data byte 2")
  {
    msg.MakeProgramChange(1);
    REQUIRE(msg.mData2 == 0);
    REQUIRE(msg.mOffset == 0);
    REQUIRE(msg.Channel() == 0);
  }
  SECTION("pitch wheel resets channel and offset")
  {
    msg.MakePitchWheelMsg(0.);
    REQUIRE(msg.mOffset == 0);
    REQUIRE(msg.Channel() == 0);
  }
  SECTION("control change resets channel and offset")
  {
    msg.MakeControlChangeMsg(IMidiMsg::kPan, 0.);
    REQUIRE(msg.mData2 == 0);
    REQUIRE(msg.mOffset == 0);
    REQUIRE(msg.Channel() == 0);
  }
  SECTION("note off clears the velocity")
  {
    msg.MakeNoteOffMsg(1, 2);
    REQUIRE(msg.mData2 == 0);
    REQUIRE(msg.Channel() == 0);
  }
}

TEST_CASE("IMidiMsg: StatusMsgStr", "[unit][midi]")
{
  REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kNone)) == "none");
  REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kNoteOff)) == "noteoff");
  REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kNoteOn)) == "noteon");
  REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kPolyAftertouch)) == "aftertouch");
  REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kControlChange)) == "controlchange");
  REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kProgramChange)) == "programchange");
  REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kChannelAftertouch)) == "channelaftertouch");
  REQUIRE(std::string(IMidiMsg::StatusMsgStr(IMidiMsg::kPitchWheel)) == "pitchwheel");
  REQUIRE(std::string(IMidiMsg::StatusMsgStr(static_cast<IMidiMsg::EStatusMsg>(3))) == "unknown");

  // the string for a real message
  IMidiMsg msg;
  msg.MakePitchWheelMsg(0.25, 2);
  REQUIRE(std::string(IMidiMsg::StatusMsgStr(msg.StatusMsg())) == "pitchwheel");
}

TEST_CASE("IMidiMsg: CCNameStr", "[unit][midi]")
{
  REQUIRE(std::string(IMidiMsg::CCNameStr(0)) == "BankSel.MSB");
  REQUIRE(std::string(IMidiMsg::CCNameStr(IMidiMsg::kModWheel)) == "Modulation");
  REQUIRE(std::string(IMidiMsg::CCNameStr(IMidiMsg::kChannelVolume)) == "MainVolume");
  REQUIRE(std::string(IMidiMsg::CCNameStr(IMidiMsg::kPan)) == "Pan");
  REQUIRE(std::string(IMidiMsg::CCNameStr(IMidiMsg::kExpressionController)) == "Expression");
  REQUIRE(std::string(IMidiMsg::CCNameStr(32)) == "BankSel.LSB");
  REQUIRE(std::string(IMidiMsg::CCNameStr(IMidiMsg::kSustainOnOff)) == "Damper Ped");
  REQUIRE(std::string(IMidiMsg::CCNameStr(IMidiMsg::kUndefined102)) == "Contr. 102");
  REQUIRE(std::string(IMidiMsg::CCNameStr(IMidiMsg::kAllNotesOff)) == "AllNoteOff");
  REQUIRE(std::string(IMidiMsg::CCNameStr(127)) == "PolyModeOn");

  bool allNamed = true;
  for (int i = 0; i < 128; i++)
  {
    const char* name = IMidiMsg::CCNameStr(i);
    allNamed = allNamed && name != nullptr && name[0] != '\0';
  }
  REQUIRE(allNamed);

  // the generic names carry their own controller number
  for (int i : {3, 9, 14, 15, 20, 31, 35, 41, 63, 85, 90, 102, 119, 120})
  {
    CAPTURE(i);
    REQUIRE(std::string(IMidiMsg::CCNameStr(i)) == "Contr. " + std::to_string(i));
  }
}

TEST_CASE("ISysEx: construction and Clear", "[unit][midi][sysex]")
{
  ISysEx def;
  REQUIRE(def.mOffset == 0);
  REQUIRE(def.mSize == 0);
  REQUIRE(def.mData == nullptr);

  const uint8_t data[4] = {0xF0, 0x01, 0x02, 0xF7};
  ISysEx msg {9, data, 4};
  REQUIRE(msg.mOffset == 9);
  REQUIRE(msg.mSize == 4);
  REQUIRE(msg.mData == data); // the data is referenced, not copied

  msg.Clear();
  REQUIRE(msg.mOffset == 0);
  REQUIRE(msg.mSize == 0);
  REQUIRE(msg.mData == nullptr);
  REQUIRE(data[0] == 0xF0); // the external data is untouched
  REQUIRE(data[3] == 0xF7);
}

TEST_CASE("ISysEx: SysExStr formats bytes as hex", "[unit][midi][sysex]")
{
  SECTION("upper case, two digits, space separated")
  {
    REQUIRE(SysExToString({0xF0, 0x7E, 0x00, 0x0A, 0xF7}, 64) == "F0 7E 00 0A F7");
    REQUIRE(SysExToString({0x00}, 64) == "00");
    REQUIRE(SysExToString({0xFF}, 64) == "FF");
    REQUIRE(SysExToString({0x01, 0xAB}, 64) == "01 AB");
  }

  SECTION("no data gives an empty string")
  {
    REQUIRE(SysExToString({}, 64) == "");

    char buf[8] = "xxxxxxx";
    ISysEx sysex;
    const uint8_t byte = 0x12;
    REQUIRE(std::string(sysex.SysExStr(buf, 8, &byte, 0)) == "");
    REQUIRE(std::string(sysex.SysExStr(buf, 8, nullptr, 3)) == "");
  }

  SECTION("output is truncated to whole bytes that fit the buffer")
  {
    const std::vector<uint8_t> data = {0xF0, 0x11, 0x22, 0x33, 0xF7};
    REQUIRE(SysExToString(data, 3) == "F0");
    REQUIRE(SysExToString(data, 5) == "F0");
    REQUIRE(SysExToString(data, 6) == "F0 11");
    REQUIRE(SysExToString(data, 14) == "F0 11 22 33");
    REQUIRE(SysExToString(data, 15) == "F0 11 22 33 F7");
    REQUIRE(SysExToString(data, 100) == "F0 11 22 33 F7");
  }

  SECTION("formats the message's own data")
  {
    const uint8_t data[3] = {0xF0, 0x42, 0xF7};
    ISysEx msg {0, data, 3};
    char buf[32];
    REQUIRE(std::string(msg.SysExStr(buf, sizeof(buf), msg.mData, msg.mSize)) == "F0 42 F7");
  }
}

TEST_CASE("IMidiQueue: a new queue is empty and has storage", "[unit][midi][midiqueue]")
{
  IMidiQueue queue;
  REQUIRE(queue.Empty());
  REQUIRE(queue.ToDo() == 0);
  REQUIRE(queue.GetSize() > 0);

  SECTION("the requested size is a lower bound")
  {
    const int size = GENERATE(1, 16, 512, 513, 2000);
    IMidiQueue sized {size};
    REQUIRE(sized.GetSize() >= size);
    REQUIRE(sized.Empty());
  }
}

TEST_CASE("IMidiQueue: Add, Peek and Remove are first in first out", "[unit][midi][midiqueue]")
{
  IMidiQueue queue;

  queue.Add(NoteOnAt(0, 60));
  REQUIRE_FALSE(queue.Empty());
  REQUIRE(queue.ToDo() == 1);
  queue.Add(NoteOnAt(0, 61));
  queue.Add(NoteOnAt(10, 62));
  queue.Add(NoteOnAt(10, 63));
  REQUIRE(queue.ToDo() == 4);

  // peeking does not consume
  REQUIRE(queue.Peek().NoteNumber() == 60);
  REQUIRE(queue.Peek().NoteNumber() == 60);
  REQUIRE(queue.ToDo() == 4);

  queue.Remove();
  REQUIRE(queue.ToDo() == 3);
  REQUIRE(queue.Peek().NoteNumber() == 61);
  queue.Remove();
  REQUIRE(queue.Peek().NoteNumber() == 62);
  REQUIRE(queue.Peek().mOffset == 10);
  queue.Remove();
  REQUIRE(queue.Peek().NoteNumber() == 63);
  queue.Remove();
  REQUIRE(queue.Empty());
  REQUIRE(queue.ToDo() == 0);
}

TEST_CASE("IMidiQueue: the whole message is stored", "[unit][midi][midiqueue]")
{
  IMidiQueue queue;
  IMidiMsg in;
  in.MakeControlChangeMsg(IMidiMsg::kCutoffFrequency, 1., 7, 123);
  queue.Add(in);

  // the queue holds a copy
  in.Clear();

  const IMidiMsg& out = queue.Peek();
  REQUIRE(out.mOffset == 123);
  REQUIRE(out.mStatus == 0xB7);
  REQUIRE(out.mData1 == 74);
  REQUIRE(out.mData2 == 127);

  // Peek gives access to the stored message
  queue.Peek().mOffset = 5;
  REQUIRE(queue.Peek().mOffset == 5);
}

TEST_CASE("IMidiQueue: messages are kept sorted by offset", "[unit][midi][midiqueue]")
{
  IMidiQueue queue;

  SECTION("out of order additions are inserted at the right place")
  {
    for (int offset : {50, 10, 30, 0, 40, 20})
      queue.Add(NoteOnAt(offset, offset));

    const auto out = Drain(queue);
    REQUIRE(out.size() == 6);
    for (size_t i = 0; i < out.size(); i++)
    {
      REQUIRE(out[i].first == static_cast<int>(i) * 10);
      REQUIRE(out[i].second == out[i].first);
    }
  }

  SECTION("messages with the same offset keep the order they were added in")
  {
    queue.Add(NoteOnAt(5, 1));
    queue.Add(NoteOnAt(9, 2));
    queue.Add(NoteOnAt(5, 3));
    queue.Add(NoteOnAt(0, 4));
    queue.Add(NoteOnAt(9, 5));
    queue.Add(NoteOnAt(5, 6));
    queue.Add(NoteOnAt(0, 7));

    const auto out = Drain(queue);
    const std::vector<std::pair<int, int>> expected = {{0, 4}, {0, 7}, {5, 1}, {5, 3}, {5, 6}, {9, 2}, {9, 5}};
    REQUIRE(out == expected);
  }

  SECTION("reverse order")
  {
    for (int offset = 99; offset >= 0; offset--)
      queue.Add(NoteOnAt(offset, offset));

    REQUIRE(queue.ToDo() == 100);
    const auto out = Drain(queue);
    bool sorted = true;
    for (size_t i = 0; i < out.size(); i++)
      sorted = sorted && (out[i].first == static_cast<int>(i));
    REQUIRE(sorted);
  }

  SECTION("an early message added after some have been removed goes to the front")
  {
    queue.Add(NoteOnAt(5, 1));
    queue.Add(NoteOnAt(10, 2));
    queue.Add(NoteOnAt(20, 3));
    queue.Remove();
    queue.Add(NoteOnAt(1, 4));
    REQUIRE(queue.ToDo() == 3);

    const auto out = Drain(queue);
    const std::vector<std::pair<int, int>> expected = {{1, 4}, {10, 2}, {20, 3}};
    REQUIRE(out == expected);
  }

  SECTION("negative offsets sort before zero")
  {
    queue.Add(NoteOnAt(0, 1));
    queue.Add(NoteOnAt(-3, 2));
    REQUIRE(queue.Peek().NoteNumber() == 2);
  }
}

TEST_CASE("IMidiQueue: Flush rebases the offsets of the remaining messages", "[unit][midi][midiqueue]")
{
  IMidiQueue queue;
  queue.Add(NoteOnAt(10, 1));
  queue.Add(NoteOnAt(100, 2));
  queue.Add(NoteOnAt(600, 3));
  queue.Add(NoteOnAt(1100, 4));

  // process a 512 sample block: handle everything with an offset inside it
  int nHandled = 0;
  while (!queue.Empty() && queue.Peek().mOffset < 512)
  {
    queue.Remove();
    nHandled++;
  }
  REQUIRE(nHandled == 2);

  queue.Flush(512);
  REQUIRE(queue.ToDo() == 2);
  REQUIRE(queue.Peek().mOffset == 88);
  REQUIRE(queue.Peek().NoteNumber() == 3);

  // messages added now are sorted against the rebased offsets
  queue.Add(NoteOnAt(100, 5));
  queue.Add(NoteOnAt(0, 6));

  const auto out = Drain(queue);
  const std::vector<std::pair<int, int>> expected = {{0, 6}, {88, 3}, {100, 5}, {588, 4}};
  REQUIRE(out == expected);

  SECTION("flushing an empty queue is harmless")
  {
    queue.Flush(512);
    REQUIRE(queue.Empty());
    queue.Add(NoteOnAt(3, 9));
    REQUIRE(queue.Peek().mOffset == 3);
  }

  SECTION("flushing without removing anything")
  {
    queue.Add(NoteOnAt(700, 1));
    queue.Flush(512);
    REQUIRE(queue.ToDo() == 1);
    REQUIRE(queue.Peek().mOffset == 188);
    queue.Flush(0);
    REQUIRE(queue.Peek().mOffset == 188);
  }
}

TEST_CASE("IMidiQueue: Clear", "[unit][midi][midiqueue]")
{
  IMidiQueue queue;
  for (int i = 0; i < 10; i++)
    queue.Add(NoteOnAt(i, i));
  queue.Remove();

  const int size = queue.GetSize();
  queue.Clear();
  REQUIRE(queue.Empty());
  REQUIRE(queue.ToDo() == 0);
  REQUIRE(queue.GetSize() == size);

  queue.Add(NoteOnAt(4, 44));
  REQUIRE(queue.ToDo() == 1);
  REQUIRE(queue.Peek().NoteNumber() == 44);
}

TEST_CASE("IMidiQueue: grows automatically when full", "[unit][midi][midiqueue]")
{
  IMidiQueue queue {16};
  const int initialSize = queue.GetSize();
  const int nMsgs = initialSize * 3 + 5;

  for (int i = 0; i < nMsgs; i++)
    queue.Add(NoteOnAt(i, i % 128));

  REQUIRE(queue.ToDo() == nMsgs);
  REQUIRE(queue.GetSize() >= nMsgs);

  bool inOrder = true;
  for (int i = 0; i < nMsgs; i++)
  {
    inOrder = inOrder && (queue.Peek().mOffset == i) && (queue.Peek().NoteNumber() == i % 128);
    queue.Remove();
  }
  REQUIRE(inOrder);
  REQUIRE(queue.Empty());
}

TEST_CASE("IMidiQueue: reuses the space of removed messages before growing", "[unit][midi][midiqueue]")
{
  IMidiQueue queue {16};
  const int size = queue.GetSize();

  for (int i = 0; i < size; i++)
    queue.Add(NoteOnAt(i, i % 128));

  REQUIRE(queue.GetSize() == size);

  // steady state: one out, one in, many times over
  bool inOrder = true;
  for (int i = 0; i < size * 4; i++)
  {
    inOrder = inOrder && (queue.Peek().mOffset == i);
    queue.Remove();
    queue.Add(NoteOnAt(size + i, 0));
  }

  REQUIRE(inOrder);
  REQUIRE(queue.GetSize() == size);
  REQUIRE(queue.ToDo() == size);
  REQUIRE(queue.Peek().mOffset == size * 4);
}

TEST_CASE("IMidiQueue: Resize", "[unit][midi][midiqueue]")
{
  IMidiQueue queue {16};
  const int initialSize = queue.GetSize();

  SECTION("growing returns the new size, which is at least what was asked for")
  {
    const int newSize = queue.Resize(initialSize * 2 + 1);
    REQUIRE(newSize >= initialSize * 2 + 1);
    REQUIRE(queue.GetSize() == newSize);
    REQUIRE(queue.Empty());
  }

  SECTION("resizing to the current size changes nothing")
  {
    REQUIRE(queue.Resize(initialSize) == initialSize);
    REQUIRE(queue.GetSize() == initialSize);
  }

  SECTION("contents survive growing")
  {
    for (int i = 0; i < 10; i++)
      queue.Add(NoteOnAt(i, i));
    queue.Remove();
    queue.Remove();

    queue.Resize(initialSize * 4);
    REQUIRE(queue.ToDo() == 8);

    const auto out = Drain(queue);
    for (size_t i = 0; i < out.size(); i++)
      REQUIRE(out[i].first == static_cast<int>(i) + 2);
  }

  SECTION("a queue is never shrunk below the messages it holds")
  {
    queue.Resize(initialSize * 4);
    const int nMsgs = initialSize * 3;
    for (int i = 0; i < nMsgs; i++)
      queue.Add(NoteOnAt(i, 0));

    const int newSize = queue.Resize(1);
    REQUIRE(newSize >= nMsgs);
    REQUIRE(queue.GetSize() == newSize);
    REQUIRE(queue.ToDo() == nMsgs);

    bool inOrder = true;
    for (int i = 0; i < nMsgs; i++)
    {
      inOrder = inOrder && (queue.Peek().mOffset == i);
      queue.Remove();
    }
    REQUIRE(inOrder);
  }

  SECTION("shrinking an empty queue")
  {
    queue.Resize(initialSize * 4);
    REQUIRE(queue.GetSize() >= initialSize * 4);
    const int newSize = queue.Resize(initialSize);
    REQUIRE(newSize == initialSize);
    REQUIRE(queue.GetSize() == initialSize);

    // still usable, and still able to grow
    for (int i = 0; i < initialSize + 1; i++)
      queue.Add(NoteOnAt(i, 0));
    REQUIRE(queue.ToDo() == initialSize + 1);
  }

  SECTION("storage is allocated in whole 4 kB pages")
  {
    REQUIRE(sizeof(IMidiMsg) == 8);
    REQUIRE((static_cast<size_t>(queue.GetSize()) * sizeof(IMidiMsg)) % 4096 == 0);
    queue.Resize(1000);
    REQUIRE((static_cast<size_t>(queue.GetSize()) * sizeof(IMidiMsg)) % 4096 == 0);
  }
}

TEST_CASE("IMidiQueueBase: works for sysex messages too", "[unit][midi][midiqueue][sysex]")
{
  const uint8_t a[3] = {0xF0, 0x01, 0xF7};
  const uint8_t b[4] = {0xF0, 0x02, 0x03, 0xF7};

  IMidiQueueBase<ISysEx> queue;
  REQUIRE(queue.Empty());

  queue.Add(ISysEx {40, a, 3});
  queue.Add(ISysEx {8, b, 4});
  REQUIRE(queue.ToDo() == 2);

  REQUIRE(queue.Peek().mOffset == 8);
  REQUIRE(queue.Peek().mData == b);
  REQUIRE(queue.Peek().mSize == 4);
  queue.Remove();

  queue.Flush(32);
  REQUIRE(queue.Peek().mOffset == 8);
  REQUIRE(queue.Peek().mData == a);
  REQUIRE(queue.Peek().mSize == 3);
  queue.Remove();
  REQUIRE(queue.Empty());
}
