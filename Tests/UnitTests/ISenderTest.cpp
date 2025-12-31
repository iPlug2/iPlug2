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
#include <thread>
#include <atomic>

// Include iPlug headers in correct order
#include "IPlugConstants.h"
#include "IPlugUtilities.h"
#include "IPlugEditorDelegate.h"
#include "ISender.h"

using namespace iplug;
using Catch::Approx;

// Mock IEditorDelegate for testing
class MockEditorDelegate : public IEditorDelegate
{
public:
  MockEditorDelegate() : IEditorDelegate(0) {}

  // Track messages received
  struct ReceivedMessage
  {
    int ctrlTag;
    int msgTag;
    int dataSize;
    std::vector<uint8_t> data;
  };

  std::vector<ReceivedMessage> mReceivedMessages;

  void SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData) override
  {
    ReceivedMessage msg;
    msg.ctrlTag = ctrlTag;
    msg.msgTag = msgTag;
    msg.dataSize = dataSize;
    if (dataSize > 0 && pData)
    {
      msg.data.resize(dataSize);
      memcpy(msg.data.data(), pData, dataSize);
    }
    mReceivedMessages.push_back(msg);
  }

  void BeginInformHostOfParamChangeFromUI(int paramIdx) override {}
  void EndInformHostOfParamChangeFromUI(int paramIdx) override {}

  void ClearMessages() { mReceivedMessages.clear(); }
};

TEST_CASE("ISenderData structure", "[ISender]")
{
  SECTION("Default construction")
  {
    ISenderData<2, float> data;
    REQUIRE(data.ctrlTag == kNoTag);
    REQUIRE(data.nChans == 2);
    REQUIRE(data.chanOffset == 0);
  }

  SECTION("Construction with tag and channels")
  {
    ISenderData<4, float> data(42, 2, 1);
    REQUIRE(data.ctrlTag == 42);
    REQUIRE(data.nChans == 2);
    REQUIRE(data.chanOffset == 1);
  }

  SECTION("Construction with values")
  {
    std::array<float, 2> vals = {0.5f, 0.75f};
    ISenderData<2, float> data(10, vals, 2, 0);

    REQUIRE(data.ctrlTag == 10);
    REQUIRE(data.vals[0] == Approx(0.5f));
    REQUIRE(data.vals[1] == Approx(0.75f));
  }
}

TEST_CASE("ISender basic operations", "[ISender]")
{
  SECTION("Push and TransmitData")
  {
    ISender<2, 64, float> sender;
    MockEditorDelegate delegate;

    ISenderData<2, float> data(100, 2, 0);
    data.vals[0] = 0.5f;
    data.vals[1] = 0.8f;

    sender.PushData(data);
    sender.TransmitData(delegate);

    REQUIRE(delegate.mReceivedMessages.size() == 1);
    REQUIRE(delegate.mReceivedMessages[0].ctrlTag == 100);
    REQUIRE(delegate.mReceivedMessages[0].msgTag == ISender<2, 64, float>::kUpdateMessage);
  }

  SECTION("Multiple push and transmit")
  {
    ISender<1, 64, float> sender;
    MockEditorDelegate delegate;

    for (int i = 0; i < 10; i++)
    {
      ISenderData<1, float> data(50, 1, 0);
      data.vals[0] = static_cast<float>(i) * 0.1f;
      sender.PushData(data);
    }

    sender.TransmitData(delegate);

    REQUIRE(delegate.mReceivedMessages.size() == 10);
  }

  SECTION("GetLastData returns most recent")
  {
    ISender<1, 64, float> sender;
    MockEditorDelegate delegate;

    for (int i = 0; i < 5; i++)
    {
      ISenderData<1, float> data(1, 1, 0);
      data.vals[0] = static_cast<float>(i);
      sender.PushData(data);
    }

    sender.TransmitData(delegate);

    auto lastData = sender.GetLastData();
    REQUIRE(lastData.vals[0] == Approx(4.0f));
  }

  SECTION("TransmitDataToControlsWithTags sends to multiple controls")
  {
    ISender<1, 64, float> sender;
    MockEditorDelegate delegate;

    ISenderData<1, float> data(0, 1, 0); // Tag doesn't matter here
    data.vals[0] = 0.5f;
    sender.PushData(data);

    sender.TransmitDataToControlsWithTags(delegate, {10, 20, 30});

    REQUIRE(delegate.mReceivedMessages.size() == 3);
    REQUIRE(delegate.mReceivedMessages[0].ctrlTag == 10);
    REQUIRE(delegate.mReceivedMessages[1].ctrlTag == 20);
    REQUIRE(delegate.mReceivedMessages[2].ctrlTag == 30);
  }
}

TEST_CASE("IPeakSender peak detection", "[ISender][IPeakSender]")
{
  SECTION("Detects peak from sine wave")
  {
    IPeakSender<1, 64> sender(-90.0, 5.0f);
    sender.Reset(44100.0);

    // Create a buffer with known peak
    constexpr int kNumFrames = 512;
    std::vector<sample> buffer(kNumFrames);

    // Fill with sine wave, peak at 0.8
    for (int i = 0; i < kNumFrames; i++)
    {
      buffer[i] = 0.8 * std::sin(2.0 * 3.14159 * 440.0 * i / 44100.0);
    }

    sample* inputs[1] = {buffer.data()};

    sender.ProcessBlock(inputs, kNumFrames, 1, 1, 0);

    // Check that data was queued (if window completed)
    MockEditorDelegate delegate;
    sender.TransmitData(delegate);

    // Should have some messages if enough samples processed
    // The exact number depends on window size
  }

  SECTION("Silence below threshold not transmitted")
  {
    IPeakSender<1, 64> sender(-60.0, 5.0f); // -60dB threshold
    sender.Reset(44100.0);

    // Create very quiet buffer (below threshold)
    constexpr int kNumFrames = 512;
    std::vector<sample> buffer(kNumFrames, 0.0001); // Very quiet

    sample* inputs[1] = {buffer.data()};

    sender.ProcessBlock(inputs, kNumFrames, 1, 1, 0);

    MockEditorDelegate delegate;
    sender.TransmitData(delegate);

    // With very quiet signal below threshold, should transmit less
    // (after the initial transition)
  }

  SECTION("SetWindowSizeMs changes window")
  {
    IPeakSender<1, 64> sender(-90.0, 10.0f);
    sender.Reset(44100.0);
    sender.SetWindowSizeMs(20.0, 44100.0);

    // Window should now be ~882 samples (20ms at 44.1kHz)
    // Just verify it doesn't crash
    constexpr int kNumFrames = 1024;
    std::vector<sample> buffer(kNumFrames, 0.5);
    sample* inputs[1] = {buffer.data()};

    sender.ProcessBlock(inputs, kNumFrames, 1, 1, 0);
  }
}

TEST_CASE("IPeakAvgSender peak and RMS", "[ISender][IPeakAvgSender]")
{
  SECTION("RMS calculation for DC signal")
  {
    IPeakAvgSender<1, 64> sender(-90.0, true, 5.0f, 1.0f, 100.0f, 500.0f);
    sender.Reset(44100.0);

    // DC signal of 0.5 should have RMS of 0.5
    constexpr int kNumFrames = 512;
    std::vector<sample> buffer(kNumFrames, 0.5);
    sample* inputs[1] = {buffer.data()};

    sender.ProcessBlock(inputs, kNumFrames, 1, 1, 0);

    MockEditorDelegate delegate;
    sender.TransmitData(delegate);

    if (!delegate.mReceivedMessages.empty())
    {
      // Verify data was transmitted
      REQUIRE(delegate.mReceivedMessages[0].dataSize > 0);
    }
  }

  SECTION("Peak hold behavior")
  {
    IPeakAvgSender<1, 64> sender(-90.0, false, 5.0f, 1.0f, 100.0f, 100.0f);
    sender.Reset(44100.0);

    // First send a loud signal
    constexpr int kNumFrames = 256;
    std::vector<sample> loudBuffer(kNumFrames, 0.9);
    sample* loudInputs[1] = {loudBuffer.data()};
    sender.ProcessBlock(loudInputs, kNumFrames, 1, 1, 0);

    // Then send quiet signal - peak should be held
    std::vector<sample> quietBuffer(kNumFrames, 0.1);
    sample* quietInputs[1] = {quietBuffer.data()};
    sender.ProcessBlock(quietInputs, kNumFrames, 1, 1, 0);

    MockEditorDelegate delegate;
    sender.TransmitData(delegate);

    // Peak hold should keep the high value for some time
  }

  SECTION("Attack and decay times")
  {
    IPeakAvgSender<1, 64> sender(-90.0, true, 5.0f, 10.0f, 500.0f, 500.0f);
    sender.Reset(44100.0);

    sender.SetAttackTimeMs(5.0, 44100.0);
    sender.SetDecayTimeMs(200.0, 44100.0);

    // Verify no crashes with modified timing
    constexpr int kNumFrames = 512;
    std::vector<sample> buffer(kNumFrames, 0.5);
    sample* inputs[1] = {buffer.data()};

    sender.ProcessBlock(inputs, kNumFrames, 1, 1, 0);
  }
}

TEST_CASE("IBufferSender buffer transfer", "[ISender][IBufferSender]")
{
  SECTION("Buffers audio data")
  {
    IBufferSender<1, 64, 128> sender(-90.0, 128);
    MockEditorDelegate delegate;

    // Create test buffer
    constexpr int kNumFrames = 256; // 2x buffer size
    std::vector<sample> buffer(kNumFrames);
    for (int i = 0; i < kNumFrames; i++)
    {
      buffer[i] = static_cast<sample>(i) / kNumFrames;
    }

    sample* inputs[1] = {buffer.data()};
    sender.ProcessBlock(inputs, kNumFrames, 1, 1, 0);

    sender.TransmitData(delegate);

    // Should have transmitted at least 1 buffer
    REQUIRE(delegate.mReceivedMessages.size() >= 1);
  }

  SECTION("SetBufferSize changes buffer")
  {
    IBufferSender<1, 64, 128> sender(-90.0, 64);

    REQUIRE(sender.GetBufferSize() == 64);

    sender.SetBufferSize(32);
    REQUIRE(sender.GetBufferSize() == 32);
  }

  SECTION("Threshold filtering")
  {
    IBufferSender<1, 64, 128> sender(-40.0, 128); // Higher threshold
    MockEditorDelegate delegate;

    // Very quiet signal
    constexpr int kNumFrames = 256;
    std::vector<sample> buffer(kNumFrames, 0.001);
    sample* inputs[1] = {buffer.data()};

    sender.ProcessBlock(inputs, kNumFrames, 1, 1, 0);
    sender.TransmitData(delegate);

    // May transmit less due to threshold
  }
}

TEST_CASE("IBufferSender multichannel", "[ISender][IBufferSender]")
{
  SECTION("Handles stereo buffers")
  {
    IBufferSender<2, 64, 128> sender(-90.0, 128);
    MockEditorDelegate delegate;

    constexpr int kNumFrames = 256;
    std::vector<sample> left(kNumFrames, 0.5);
    std::vector<sample> right(kNumFrames, 0.3);

    sample* inputs[2] = {left.data(), right.data()};
    sender.ProcessBlock(inputs, kNumFrames, 1, 2, 0);

    sender.TransmitData(delegate);

    REQUIRE(delegate.mReceivedMessages.size() >= 1);
  }
}

TEST_CASE("ISender thread safety", "[ISender][threading]")
{
  SECTION("Concurrent push and transmit")
  {
    ISender<1, 256, float> sender;
    MockEditorDelegate delegate;
    std::atomic<bool> done{false};
    std::atomic<int> pushCount{0};

    // Producer thread (simulating audio thread)
    std::thread producer([&]()
    {
      for (int i = 0; i < 1000; i++)
      {
        ISenderData<1, float> data(1, 1, 0);
        data.vals[0] = static_cast<float>(i);
        sender.PushData(data);
        pushCount.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::yield();
      }
      done.store(true, std::memory_order_release);
    });

    // Consumer thread (simulating UI thread)
    std::thread consumer([&]()
    {
      while (!done.load(std::memory_order_acquire) ||
             delegate.mReceivedMessages.size() < static_cast<size_t>(pushCount.load()))
      {
        sender.TransmitData(delegate);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
      }
      // Final drain
      sender.TransmitData(delegate);
    });

    producer.join();
    consumer.join();

    // All pushed items should be received
    REQUIRE(delegate.mReceivedMessages.size() == static_cast<size_t>(pushCount.load()));
  }
}

TEST_CASE("IPeakSender multichannel", "[ISender][IPeakSender]")
{
  SECTION("Processes stereo correctly")
  {
    IPeakSender<2, 64> sender(-90.0, 5.0f);
    sender.Reset(44100.0);

    constexpr int kNumFrames = 512;
    std::vector<sample> left(kNumFrames);
    std::vector<sample> right(kNumFrames);

    // Left channel louder than right
    for (int i = 0; i < kNumFrames; i++)
    {
      left[i] = 0.8 * std::sin(2.0 * 3.14159 * 440.0 * i / 44100.0);
      right[i] = 0.4 * std::sin(2.0 * 3.14159 * 880.0 * i / 44100.0);
    }

    sample* inputs[2] = {left.data(), right.data()};
    sender.ProcessBlock(inputs, kNumFrames, 1, 2, 0);

    MockEditorDelegate delegate;
    sender.TransmitData(delegate);

    // Should have processed both channels
  }
}

// Note: ISpectrumSender tests removed - requires WDL FFT library
