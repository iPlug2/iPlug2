/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the processor -> editor messaging methods of IEditorDelegate, the OnIdle() hook and ISender.

#include <array>
#include <cstring>
#include <string>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "AudioBuffers.h"
#include "TestPlugins.h"
#include "ISender.h" // not self-contained: needs the IPlug headers which TestPlugins.h brings in

using namespace iplug;
using Catch::Approx;

namespace
{

constexpr int kCtrlTagMeter = 7;

/** A stereo effect with a peak meter: the audio thread feeds an IPeakSender, which OnIdle() empties towards the UI */
class MeterPlug : public RecordingPlug
{
public:
  using MeterData = ISenderData<2, float>;

  MeterPlug()
  : RecordingPlug(InstanceInfo{}, TestConfig{})
  {
  }

  void OnReset() override
  {
    RecordingPlug::OnReset();
    mSender.Reset(GetSampleRate());
  }

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override
  {
    for (int c = 0; c < 2; c++)
      std::copy(inputs[c], inputs[c] + nFrames, outputs[c]);

    mSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
  }

  void OnIdle() override
  {
    RecordingPlug::OnIdle();
    mSender.TransmitData(*this);
  }

  /** @return The meter packets the UI has received so far */
  std::vector<MeterData> MeterPackets() const
  {
    std::vector<MeterData> packets;

    for (const Message& msg : mMessages)
    {
      if (msg.mCtrlTag == kCtrlTagMeter && msg.mData.size() == sizeof(MeterData))
      {
        MeterData data;
        std::memcpy(&data, msg.mData.data(), sizeof(MeterData));
        packets.push_back(data);
      }
    }

    return packets;
  }

  IPeakSender<2> mSender;
};

/** Processes nFrames of DC, with a different level on each channel */
void ProcessDC(MeterPlug& plug, int nFrames, float left, float right)
{
  AudioBuffers<float> in {2, nFrames}, out {2, nFrames};
  std::fill(in[0].begin(), in[0].end(), left);
  std::fill(in[1].begin(), in[1].end(), right);
  plug.HostProcess(in.Get(), out.Get(), nFrames);
}

} // namespace

TEST_CASE("Messaging: OnIdle() is called on every idle tick", "[integration][messaging]")
{
  GainPlug plug;

  for (int i = 1; i <= 5; i++)
  {
    plug.HostIdle();
    CHECK(plug.mNumIdleCalls == i);
  }
}

TEST_CASE("Messaging: SendControlValueFromDelegate() reaches the editor", "[integration][messaging]")
{
  GainPlug plug;

  // Called through the delegate interface, as an ISender or a DSP class holding an IEditorDelegate& would
  IEditorDelegate& delegate = plug;
  delegate.SendControlValueFromDelegate(3, 0.25);
  delegate.SendControlValueFromDelegate(4, 1.);

  REQUIRE(plug.mControlValuesToUI.size() == 2);
  CHECK(plug.mControlValuesToUI[0].mCtrlTag == 3);
  CHECK(plug.mControlValuesToUI[0].mNormalizedValue == 0.25);
  CHECK(plug.mControlValuesToUI[1].mCtrlTag == 4);
  CHECK(plug.mControlValuesToUI[1].mNormalizedValue == 1.);

  // A control value is not a parameter: nothing else is notified
  CHECK(plug.mParamChanges.empty());
  CHECK(plug.mParamChangesUI.empty());
  CHECK(plug.mParamEvents.empty());
  CHECK(plug.mMessages.empty());
}

TEST_CASE("Messaging: SendControlMsgFromDelegate() arrives in OnMessage()", "[integration][messaging]")
{
  GainPlug plug;
  IEditorDelegate& delegate = plug;

  SECTION("with a data payload")
  {
    const std::vector<uint8_t> payload {1, 2, 3, 4, 5};
    delegate.SendControlMsgFromDelegate(11, 22, static_cast<int>(payload.size()), payload.data());

    REQUIRE(plug.mMessages.size() == 1);
    CHECK(plug.mMessages[0].mCtrlTag == 11);
    CHECK(plug.mMessages[0].mMsgTag == 22);
    CHECK(plug.mMessages[0].mData == payload);
  }

  SECTION("without data")
  {
    delegate.SendControlMsgFromDelegate(11, 22);

    REQUIRE(plug.mMessages.size() == 1);
    CHECK(plug.mMessages[0].mCtrlTag == 11);
    CHECK(plug.mMessages[0].mMsgTag == 22);
    CHECK(plug.mMessages[0].mData.empty());
  }

  SECTION("a struct survives the trip")
  {
    struct Payload
    {
      int mA;
      double mB;
      char mC[8];
    };

    Payload sent;
    std::memset(&sent, 0, sizeof(Payload));
    sent.mA = 42;
    sent.mB = 2.5;
    std::memcpy(sent.mC, "abc", 4);
    delegate.SendControlMsgFromDelegate(1, 2, static_cast<int>(sizeof(Payload)), &sent);

    REQUIRE(plug.mMessages.size() == 1);
    REQUIRE(plug.mMessages[0].mData.size() == sizeof(Payload));

    Payload received;
    std::memcpy(&received, plug.mMessages[0].mData.data(), sizeof(Payload));
    CHECK(received.mA == 42);
    CHECK(received.mB == 2.5);
    CHECK(std::string(received.mC) == "abc");
  }
}

TEST_CASE("Messaging: SendArbitraryMsgFromDelegate() arrives in OnMessage() without a control tag", "[integration][messaging]")
{
  GainPlug plug;
  IEditorDelegate& delegate = plug;

  const char text[] = "hello editor";
  delegate.SendArbitraryMsgFromDelegate(99, static_cast<int>(sizeof(text)), text);
  delegate.SendArbitraryMsgFromDelegate(100);

  REQUIRE(plug.mMessages.size() == 2);
  CHECK(plug.mMessages[0].mMsgTag == 99);
  CHECK(plug.mMessages[0].mCtrlTag == kNoTag);
  REQUIRE(plug.mMessages[0].mData.size() == sizeof(text));
  CHECK(std::string(reinterpret_cast<const char*>(plug.mMessages[0].mData.data())) == "hello editor");

  CHECK(plug.mMessages[1].mMsgTag == 100);
  CHECK(plug.mMessages[1].mCtrlTag == kNoTag);
  CHECK(plug.mMessages[1].mData.empty());
}

TEST_CASE("Messaging: SendArbitraryMsgFromUI() arrives in the plug-in's OnMessage()", "[integration][messaging]")
{
  GainPlug plug;

  const int value = 1234;
  plug.SendArbitraryMsgFromUI(5, 6, static_cast<int>(sizeof(value)), &value);
  plug.SendArbitraryMsgFromUI(7);

  REQUIRE(plug.mMessages.size() == 2);
  CHECK(plug.mMessages[0].mMsgTag == 5);
  CHECK(plug.mMessages[0].mCtrlTag == 6);
  REQUIRE(plug.mMessages[0].mData.size() == sizeof(value));

  int received = 0;
  std::memcpy(&received, plug.mMessages[0].mData.data(), sizeof(received));
  CHECK(received == 1234);

  CHECK(plug.mMessages[1].mMsgTag == 7);
  CHECK(plug.mMessages[1].mCtrlTag == kNoTag);
  CHECK(plug.mMessages[1].mData.empty());
}

TEST_CASE("Messaging: OnMessage() is not handled by default", "[integration][messaging]")
{
  class PlainPlug : public Plugin
  {
  public:
    PlainPlug() : Plugin(InstanceInfo{}, TestConfig{}.Make()) {}
  };

  PlainPlug plug;
  CHECK_FALSE(plug.OnMessage(1, 2, 0, nullptr));

  // ...and sending to a plug-in which doesn't handle messages is harmless
  plug.SendArbitraryMsgFromDelegate(1);
  plug.SendControlMsgFromDelegate(1, 2);
  plug.SendArbitraryMsgFromUI(1);
  plug.SendControlValueFromDelegate(1, 0.5);
  SUCCEED();
}

TEST_CASE("Messaging: SendParameterValueFromDelegate() notifies the UI without changing the parameter", "[integration][messaging]")
{
  GainPlug plug;

  plug.SendParameterValueFromDelegate(GainPlug::kGain, 0.75, true);

  REQUIRE(plug.mParamChangesUI.size() == 1);
  CHECK(plug.mParamChangesUI[0].mIdx == GainPlug::kGain);
  CHECK(plug.mParamChangesUI[0].mSource == kDelegate);

  CHECK(plug.GetParam(GainPlug::kGain)->Value() == 100.);
  CHECK(plug.mParamChanges.empty());
  CHECK(plug.mParamEvents.empty());
}

TEST_CASE("Messaging: SendCurrentParamValuesFromDelegate() sends every normalized value to the UI", "[integration][messaging]")
{
  GainPlug plug;
  plug.GetParam(GainPlug::kGain)->Set(150.);
  plug.GetParam(GainPlug::kMute)->Set(1.);
  plug.GetParam(GainPlug::kMode)->Set(1.);

  plug.SendCurrentParamValuesFromDelegate();

  REQUIRE(plug.mParamValuesToUI.size() == 3);

  const double expected[] = {0.75, 1., 0.5};

  for (int i = 0; i < 3; i++)
  {
    const auto& toUI = plug.mParamValuesToUI[static_cast<size_t>(i)];
    CHECK(toUI.mIdx == i);
    CHECK(toUI.mNormalized);
    CHECK(toUI.mValue == Approx(expected[i]));
  }

  REQUIRE(plug.mParamChangesUI.size() == 3);

  for (const auto& change : plug.mParamChangesUI)
    CHECK(change.mSource == kDelegate);

  CHECK(plug.mParamChanges.empty());
}

TEST_CASE("Messaging: ISender queues data until it is transmitted to the delegate", "[integration][messaging][sender]")
{
  GainPlug plug;
  ISender<2, 8, float> sender;

  using Data = ISenderData<2, float>;

  const std::array<float, 2> firstVals {{0.25f, 0.5f}};
  const std::array<float, 2> secondVals {{0.75f, 1.f}};

  sender.PushData(Data {kCtrlTagMeter, firstVals});
  sender.PushData(Data {kCtrlTagMeter + 1, secondVals});

  CHECK(plug.mMessages.empty());

  SECTION("TransmitData() sends each packet to the control it was tagged with")
  {
    sender.TransmitData(plug);

    REQUIRE(plug.mMessages.size() == 2);

    for (size_t i = 0; i < 2; i++)
    {
      CHECK(plug.mMessages[i].mMsgTag == ISender<2, 8, float>::kUpdateMessage);
      REQUIRE(plug.mMessages[i].mData.size() == sizeof(Data));
    }

    Data first, second;
    std::memcpy(&first, plug.mMessages[0].mData.data(), sizeof(Data));
    std::memcpy(&second, plug.mMessages[1].mData.data(), sizeof(Data));

    CHECK(plug.mMessages[0].mCtrlTag == kCtrlTagMeter);
    CHECK(first.ctrlTag == kCtrlTagMeter);
    CHECK(first.nChans == 2);
    CHECK(first.chanOffset == 0);
    CHECK(first.vals[0] == 0.25f);
    CHECK(first.vals[1] == 0.5f);

    CHECK(plug.mMessages[1].mCtrlTag == kCtrlTagMeter + 1);
    CHECK(second.vals[0] == 0.75f);
    CHECK(second.vals[1] == 1.f);

    // The queue is now empty
    sender.TransmitData(plug);
    CHECK(plug.mMessages.size() == 2);
  }

  SECTION("TransmitDataToControlsWithTags() sends each packet to every listed control")
  {
    sender.TransmitDataToControlsWithTags(plug, {100, 200, 300});

    REQUIRE(plug.mMessages.size() == 6);

    const int expectedTags[] = {100, 200, 300, 100, 200, 300};

    for (size_t i = 0; i < 6; i++)
    {
      CHECK(plug.mMessages[i].mCtrlTag == expectedTags[i]);

      Data data;
      REQUIRE(plug.mMessages[i].mData.size() == sizeof(Data));
      std::memcpy(&data, plug.mMessages[i].mData.data(), sizeof(Data));
      CHECK(data.ctrlTag == expectedTags[i]);
      CHECK(data.vals[0] == (i < 3 ? 0.25f : 0.75f));
    }
  }
}

TEST_CASE("Messaging: a peak meter fed by ProcessBlock() and transmitted from OnIdle()", "[integration][messaging][sender]")
{
  // The sender's default window is 5 ms, which is 240 samples at 48 kHz
  const int window = 240;

  MeterPlug plug;
  plug.HostActivate(48000., window);

  SECTION("nothing reaches the UI from the audio thread")
  {
    ProcessDC(plug, window, 0.5f, 0.25f);
    ProcessDC(plug, window, 0.5f, 0.25f);
    CHECK(plug.mMessages.empty());
  }

  SECTION("each window's average level is transmitted on the next idle call")
  {
    ProcessDC(plug, window, 0.5f, 0.25f);
    ProcessDC(plug, window, 0.5f, 0.25f);
    ProcessDC(plug, window, 0.5f, 0.25f);
    plug.HostIdle();

    // One (empty) packet from the very first sample, and one at the start of each of the following windows
    const std::vector<MeterPlug::MeterData> packets = plug.MeterPackets();
    REQUIRE(packets.size() == 3);
    REQUIRE(plug.mMessages.size() == 3);

    for (const auto& msg : plug.mMessages)
      CHECK(msg.mMsgTag == IPeakSender<2>::kUpdateMessage);

    for (const auto& packet : packets)
    {
      CHECK(packet.ctrlTag == kCtrlTagMeter);
      CHECK(packet.nChans == 2);
      CHECK(packet.chanOffset == 0);
    }

    CHECK(packets[0].vals[0] == 0.f);
    CHECK(packets[0].vals[1] == 0.f);
    CHECK(packets[1].vals[0] == Approx(0.5f));
    CHECK(packets[1].vals[1] == Approx(0.25f));
    CHECK(packets[2].vals[0] == Approx(0.5f));
    CHECK(packets[2].vals[1] == Approx(0.25f));

    SECTION("packets are transmitted once")
    {
      plug.HostIdle();
      CHECK(plug.mMessages.size() == 3);
    }

    SECTION("the meter falls to zero and then stops sending when the input goes silent")
    {
      plug.ClearAll();

      for (int block = 0; block < 6; block++)
        ProcessDC(plug, window, 0.f, 0.f);

      plug.HostIdle();

      // The last window of signal, then a single window of silence
      const std::vector<MeterPlug::MeterData> silentPackets = plug.MeterPackets();
      REQUIRE(silentPackets.size() == 2);
      CHECK(silentPackets[0].vals[0] == Approx(0.5f));
      CHECK(silentPackets[1].vals[0] == 0.f);
      CHECK(silentPackets[1].vals[1] == 0.f);
    }
  }

  SECTION("the window does not have to line up with the host's blocks")
  {
    // 10 windows worth of signal in blocks of 100
    for (int block = 0; block < 24; block++)
      ProcessDC(plug, 100, 1.f, 0.5f);

    plug.HostIdle();

    const std::vector<MeterPlug::MeterData> packets = plug.MeterPackets();
    REQUIRE(packets.size() == 10);

    for (size_t i = 1; i < packets.size(); i++)
    {
      CHECK(packets[i].vals[0] == Approx(1.f));
      CHECK(packets[i].vals[1] == Approx(0.5f));
    }
  }

  SECTION("the audio passes through unaffected")
  {
    AudioBuffers<float> in {2, window, 0.5f}, out {2, window};
    plug.HostProcess(in.Get(), out.Get(), window);
    CHECK(out[0][0] == 0.5f);
    CHECK(out[1][static_cast<size_t>(window - 1)] == 0.5f);
  }
}
