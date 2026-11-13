#include "IPlugPlatform.h"
#include "IPlugVST3.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

using namespace iplug;
using namespace Steinberg;
using namespace Steinberg::Vst;

namespace {
void Require(bool condition, const char* message)
{
  if (!condition)
    throw std::runtime_error(message);
}

Config TestConfig()
{
  return Config(1, 1, "0-2", "MIDI test", "MIDI test", "iPlug2", 0x10000,
                0, 0, 0, true, false, false, false, kInstrument, false,
                0, 0, false, 0, 0, 0, 0, "org.iplug2.miditest", "");
}

class TestPlug final : public IPlugVST3
{
public:
  TestPlug() : IPlugVST3(InstanceInfo{}, TestConfig())
  {
    GetParam(0)->InitDouble("Ordinary parameter", 0., 0., 1., 0.001);
  }

  void ProcessMidiMsg(const IMidiMsg& msg) override { mMessages.push_back(msg); }
  void OnParamChange(int idx, EParamSource source, int offset) override
  {
    Require(idx == 0 && source == kHost, "Unexpected parameter callback");
    mParameterOffsets.push_back(offset);
  }

  std::vector<IMidiMsg> mMessages;
  std::vector<int> mParameterOffsets;
};

void CheckMessage(const IMidiMsg& actual, const IMidiMsg& expected)
{
  Require(actual.mStatus == expected.mStatus && actual.mData1 == expected.mData1 &&
          actual.mData2 == expected.mData2 && actual.mOffset == expected.mOffset,
          "MIDI value, channel, offset or ordering changed");
}

ParamID ControllerID(int controller, int channel)
{
  return kMIDICCParamStartIdx + channel * kCountCtrlNumber + controller;
}

void AddPoint(ParameterChanges& changes, ParamID id, int offset, double value)
{
  int32 queueIndex = 0, pointIndex = 0;
  auto* pQueue = changes.addParameterData(id, queueIndex);
  Require(pQueue && pQueue->addPoint(offset, value, pointIndex) == kResultOk, "Cannot add parameter point");
}

void CheckNotes(TestPlug& plug)
{
  IPlugQueue<IMidiMsg> fromEditor(4), fromProcessor(4);
  for (int velocity = 0; velocity <= 127; ++velocity)
  {
    // Check ordinary MIDI values and floats just below integer boundaries,
    // including the round-trip failures reported in issue #1088.
    for (float value : {static_cast<float>(velocity / 127.),
                        std::nextafter(static_cast<float>(velocity / 127.), 0.f)})
    {
      EventList events;
      Event event{};
      event.type = Event::kNoteOnEvent;
      event.sampleOffset = 17;
      event.noteOn.channel = 15;
      event.noteOn.pitch = 60;
      event.noteOn.velocity = value;
      events.addEvent(event);
      plug.mMessages.clear();
      plug.ProcessMidiIn(&events, fromEditor, fromProcessor);
      Require(plug.mMessages.size() == 1, "Note was lost");
      CheckMessage(plug.mMessages[0], IMidiMsg(17, 0x9f, 60, velocity));
      IMidiMsg forwarded;
      Require(fromProcessor.Pop(forwarded), "Note was not forwarded to the editor");
      CheckMessage(forwarded, plug.mMessages[0]);
    }
  }
  EventList events;
  Event event{};
  event.type = Event::kNoteOnEvent;
  event.noteOn.pitch = 60;
  event.noteOn.velocity = 0.5f;
  events.addEvent(event);
  event.type = Event::kPolyPressureEvent;
  event.sampleOffset = 23;
  event.polyPressure.channel = 7;
  event.polyPressure.pitch = 60;
  event.polyPressure.pressure = 0.5f;
  events.addEvent(event);
  event.type = Event::kNoteOffEvent;
  event.sampleOffset = 31;
  event.noteOff.channel = 0;
  event.noteOff.pitch = 60;
  events.addEvent(event);
  plug.mMessages.clear();
  plug.ProcessMidiIn(&events, fromEditor, fromProcessor);
  Require(plug.mMessages.size() == 3, "Note/pressure sequence was lost");
  CheckMessage(plug.mMessages[0], IMidiMsg(0, 0x90, 60, 64));
  CheckMessage(plug.mMessages[1], IMidiMsg(23, 0xa7, 60, 64));
  CheckMessage(plug.mMessages[2], IMidiMsg(31, 0x80, 60, 0));
  IMidiMsg message;
  while (fromProcessor.Pop(message)) {}
  fromEditor.Push(IMidiMsg(13, 0x90, 72, 100));
  plug.mMessages.clear();
  plug.ProcessMidiIn(nullptr, fromEditor, fromProcessor);
  Require(plug.mMessages.size() == 1, "Editor MIDI was lost");
  CheckMessage(plug.mMessages[0], IMidiMsg(13, 0x90, 72, 100));
}

void CheckControllers(TestPlug& plug)
{
  IPlugQueue<IMidiMsg> fromProcessor(8);
  for (int controller : {1, 64, static_cast<int>(kAfterTouch), static_cast<int>(kPitchBend)})
  {
    const int maximum = controller == kPitchBend ? 16383 : 127;
    for (int value = 0; value <= maximum; ++value)
    {
      ParameterChanges changes;
      AddPoint(changes, ControllerID(controller, 15), 19, double(value) / maximum);
      ProcessData data{};
      data.inputParameterChanges = &changes;
      plug.mMessages.clear();
      plug.ProcessParameterChanges(data, fromProcessor);
      Require(plug.mMessages.size() == 1, "Controller was lost");
      const IMidiMsg expected = controller == kPitchBend ? IMidiMsg(19, 0xef, value & 127, value >> 7)
                             : controller == kAfterTouch ? IMidiMsg(19, 0xdf, value, 0)
                             : IMidiMsg(19, 0xbf, controller, value);
      CheckMessage(plug.mMessages[0], expected);
      IMidiMsg forwarded;
      Require(fromProcessor.Pop(forwarded), "Controller was not forwarded to the editor");
      CheckMessage(forwarded, expected);
    }
  }
  for (int value = 0; value <= 8192; ++value)
  {
    ParameterChanges changes;
    AddPoint(changes, ControllerID(kPitchBend, 0), 0, double(value) / 16384.);
    ProcessData data{};
    data.inputParameterChanges = &changes;
    plug.mMessages.clear();
    plug.ProcessParameterChanges(data, fromProcessor);
    Require(plug.mMessages.size() == 1, "Centered pitch bend was lost");
    CheckMessage(plug.mMessages[0], IMidiMsg(0, 0xe0, value & 127, value >> 7));
    IMidiMsg forwarded;
    Require(fromProcessor.Pop(forwarded), "Centered pitch bend was not forwarded");
    CheckMessage(forwarded, plug.mMessages[0]);
  }
  ParameterChanges changes;
  AddPoint(changes, ControllerID(kPitchBend, 0), 0, 0.5);
  ProcessData data{};
  data.inputParameterChanges = &changes;
  plug.mMessages.clear();
  plug.ProcessParameterChanges(data, fromProcessor);
  Require(plug.mMessages.size() == 1, "Pitch center was lost");
  CheckMessage(plug.mMessages[0], IMidiMsg(0, 0xe0, 0, 64));
}

void CheckPoints(TestPlug& plug)
{
  ParameterChanges changes;
  // An empty queue must not be queried at point -1. Ordinary automation
  // retains last-point behavior; MIDI retains every original sample offset.
  int32 index = 0;
  changes.addParameterData(ControllerID(1, 0), index);
  AddPoint(changes, 0, 5, 0.25);
  AddPoint(changes, 0, 63, 0.75);
  AddPoint(changes, ControllerID(64, 2), 7, 1.);
  AddPoint(changes, ControllerID(64, 2), 8, 0.);
  AddPoint(changes, ControllerID(64, 2), 61, 1.);
  AddPoint(changes, ControllerID(kPitchBend, 9), 11, 0.);
  AddPoint(changes, ControllerID(kPitchBend, 9), 47, 0.5);
  AddPoint(changes, ControllerID(kPitchBend, 9), 63, 1.);
  ProcessData data{};
  data.inputParameterChanges = &changes;
  IPlugQueue<IMidiMsg> fromProcessor(8);
  plug.mMessages.clear();
  plug.mParameterOffsets.clear();
  plug.ProcessParameterChanges(data, fromProcessor);
  const IMidiMsg expected[] {IMidiMsg(7, 0xb2, 64, 127), IMidiMsg(8, 0xb2, 64, 0), IMidiMsg(61, 0xb2, 64, 127),
                            IMidiMsg(11, 0xe9, 0, 0), IMidiMsg(47, 0xe9, 0, 64), IMidiMsg(63, 0xe9, 127, 127)};
  Require(plug.mMessages.size() == 6, "Intermediate controller points were lost");
  for (int i = 0; i < 6; ++i)
  {
    CheckMessage(plug.mMessages[i], expected[i]);
    IMidiMsg forwarded;
    Require(fromProcessor.Pop(forwarded), "Controller point was not forwarded to the editor");
    CheckMessage(forwarded, expected[i]);
  }
  Require(plug.mParameterOffsets == std::vector<int>{63} && plug.GetParam(0)->Value() == 0.75,
          "Ordinary parameter automation changed");
  ProcessData empty{};
  plug.ProcessParameterChanges(empty, fromProcessor);
  Require(plug.mMessages.size() == 6, "Null parameter changes delivered MIDI");
}
}

int main(int argc, char** argv)
{
  try
  {
    TestPlug plug;
    if (argc < 2 || std::strcmp(argv[1], "notes") == 0) CheckNotes(plug);
    if (argc < 2 || std::strcmp(argv[1], "controllers") == 0) CheckControllers(plug);
    if (argc < 2 || std::strcmp(argv[1], "points") == 0) CheckPoints(plug);
    std::puts("VST3 MIDI values, channels, queue points and sample offsets passed");
    return 0;
  }
  catch (const std::exception& error)
  {
    std::fprintf(stderr, "%s\n", error.what());
    return 1;
  }
}
