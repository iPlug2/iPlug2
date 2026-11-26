/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

// Unit tests for ISender.h: ISenderData, ISender, IPeakSender, IPeakAvgSender, IBufferSender and ISpectrumSender.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "IPlugEditorDelegate.h"
#include "ISender.h"

using namespace iplug;
using Catch::Approx;

namespace
{
const int kTag = 7;
const int kPeakWindow = 240; // 5 ms at the default sample rate of 48 kHz

/** A minimal editor delegate that records the control messages that senders transmit.
 * TData is the ISenderData specialisation that is expected to arrive. */
template <typename TData>
class RecordingDelegate : public IEditorDelegate
{
public:
  struct Msg
  {
    int ctrlTag;
    int msgTag;
    int dataSize;
    TData data;
  };

  RecordingDelegate() : IEditorDelegate(0) {}

  void BeginInformHostOfParamChangeFromUI(int) override {}
  void EndInformHostOfParamChangeFromUI(int) override {}

  void SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize = 0, const void* pData = nullptr) override
  {
    Msg msg {ctrlTag, msgTag, dataSize, TData()};

    if (pData && dataSize == static_cast<int>(sizeof(TData)))
      msg.data = *static_cast<const TData*>(pData);
    else
      mNumMalformed++;

    mMsgs.push_back(msg);
  }

  int NMsgs() const { return static_cast<int>(mMsgs.size()); }
  const Msg& operator[](int idx) const { return mMsgs[static_cast<size_t>(idx)]; }
  const Msg& Last() const { return mMsgs.back(); }
  void Clear() { mMsgs.clear(); }

  std::vector<Msg> mMsgs;
  int mNumMalformed = 0;
};

/** Multichannel sample buffers in the form that ProcessBlock() takes */
class Signal
{
public:
  Signal(int nChans, int nFrames, sample value = 0.)
  : mData(static_cast<size_t>(nChans), std::vector<sample>(static_cast<size_t>(nFrames), value))
  , mNFrames(nFrames)
  {
    for (auto& chan : mData)
      mPtrs.push_back(chan.data());
  }

  Signal(const Signal&) = delete;
  Signal& operator=(const Signal&) = delete;

  sample** Get() { return mPtrs.data(); }
  int NFrames() const { return mNFrames; }
  std::vector<sample>& operator[](int chan) { return mData[static_cast<size_t>(chan)]; }

  void Fill(int chan, sample value)
  {
    for (auto& s : mData[static_cast<size_t>(chan)])
      s = value;
  }

private:
  std::vector<std::vector<sample>> mData;
  std::vector<sample*> mPtrs;
  int mNFrames;
};

/** Feeds a signal to a sender in blocks of a given size, as a host with that buffer size would */
template <typename TSender>
void ProcessInBlocks(TSender& sender, Signal& signal, int blockSize, int ctrlTag, int nChans, int chanOffset = 0)
{
  std::vector<sample*> ptrs(static_cast<size_t>(chanOffset + nChans), nullptr);

  for (int start = 0; start < signal.NFrames(); start += blockSize)
  {
    for (int c = chanOffset; c < chanOffset + nChans; c++)
      ptrs[static_cast<size_t>(c)] = signal[c].data() + start;

    sender.ProcessBlock(ptrs.data(), std::min(blockSize, signal.NFrames() - start), ctrlTag, nChans, chanOffset);
  }
}

using TPeakData = ISenderData<2, float>;
using TPeakAvgData = ISenderData<2, std::pair<float, float>>;

const int kBufSize = 16;
using TBufferData = ISenderData<2, std::array<float, kBufSize>>;

const int kFFTSize = 64;
using TSpectrumData = ISenderData<1, std::array<float, kFFTSize>>;
using TSpectrumSender = ISpectrumSender<1, 4, kFFTSize>;

/** Runs enough of a sinusoid through a spectrum sender to produce nHops packets, and returns the last one */
template <typename TSenderType>
TSpectrumData LastSpectrumOfSinusoid(TSenderType& sender, int bin, double amplitude, bool cosine = false, int nHops = 1)
{
  const int nFrames = sender.GetHopSize() * nHops + 1;
  Signal signal {1, nFrames};

  for (int s = 0; s < nFrames; s++)
  {
    const double phase = 2. * PI * bin * s / sender.GetFFTSize();
    signal[0][s] = amplitude * (cosine ? std::cos(phase) : std::sin(phase));
  }

  sender.ProcessBlock(signal.Get(), nFrames, kTag);

  RecordingDelegate<TSpectrumData> dlg;
  sender.TransmitData(dlg);
  REQUIRE(dlg.NMsgs() == nHops);
  REQUIRE(dlg.mNumMalformed == 0);
  return dlg.Last().data;
}

int LoudestBin(const TSpectrumData& d, int nBins)
{
  int loudest = 0;
  for (int i = 1; i < nBins; i++)
  {
    if (d.vals[0][static_cast<size_t>(i)] > d.vals[0][static_cast<size_t>(loudest)])
      loudest = i;
  }
  return loudest;
}
} // namespace

// ISenderData

TEST_CASE("ISenderData: construction", "[unit][sender]")
{
  SECTION("default")
  {
    ISenderData<3, float> d;
    REQUIRE(d.ctrlTag == kNoTag);
    REQUIRE(d.nChans == 3);
    REQUIRE(d.chanOffset == 0);
    REQUIRE(d.vals.size() == 3);
    for (float v : d.vals)
      REQUIRE(v == 0.f);

    ISenderData<> single;
    REQUIRE(single.nChans == 1);
    REQUIRE(single.vals.size() == 1);
    REQUIRE(single.vals[0] == 0.f);
  }

  SECTION("tag, channel count and offset")
  {
    ISenderData<4, float> d {kTag, 2, 1};
    REQUIRE(d.ctrlTag == kTag);
    REQUIRE(d.nChans == 2);
    REQUIRE(d.chanOffset == 1);
    for (float v : d.vals)
      REQUIRE(v == 0.f);
  }

  SECTION("values")
  {
    ISenderData<2, float> d {kTag, std::array<float, 2> {0.25f, 0.75f}};
    REQUIRE(d.ctrlTag == kTag);
    REQUIRE(d.nChans == 2);
    REQUIRE(d.chanOffset == 0);
    REQUIRE(d.vals[0] == 0.25f);
    REQUIRE(d.vals[1] == 0.75f);

    ISenderData<2, float> partial {kTag, std::array<float, 2> {1.f, 2.f}, 1, 1};
    REQUIRE(partial.nChans == 1);
    REQUIRE(partial.chanOffset == 1);
    REQUIRE(partial.vals[1] == 2.f);
  }

  SECTION("other value types are value initialised")
  {
    ISenderData<2, std::pair<float, float>> pairs;
    REQUIRE(pairs.vals[1].first == 0.f);
    REQUIRE(pairs.vals[1].second == 0.f);

    ISenderData<1, std::array<float, 8>> buffers;
    for (float v : buffers.vals[0])
      REQUIRE(v == 0.f);

    ISenderData<2, int> ints {kTag, 2, 0};
    REQUIRE(ints.vals[0] == 0);
    REQUIRE(ints.vals[1] == 0);
  }
}

// ISender

TEST_CASE("ISender: queued data is transmitted to the delegate in order", "[unit][sender]")
{
  ISender<2, 8, float> sender;
  RecordingDelegate<TPeakData> dlg;

  SECTION("nothing queued, nothing sent")
  {
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 0);
  }

  SECTION("each packet becomes one control message")
  {
    for (int i = 0; i < 3; i++)
      sender.PushData(TPeakData {kTag + i, std::array<float, 2> {i * 1.f, i * 10.f}});

    // nothing happens until the main thread transmits
    REQUIRE(dlg.NMsgs() == 0);

    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 3);
    REQUIRE(dlg.mNumMalformed == 0);

    for (int i = 0; i < 3; i++)
    {
      CAPTURE(i);
      REQUIRE(dlg[i].ctrlTag == kTag + i);
      REQUIRE(dlg[i].msgTag == (ISender<2, 8, float>::kUpdateMessage));
      REQUIRE(dlg[i].dataSize == static_cast<int>(sizeof(TPeakData)));
      REQUIRE(dlg[i].data.ctrlTag == kTag + i);
      REQUIRE(dlg[i].data.nChans == 2);
      REQUIRE(dlg[i].data.chanOffset == 0);
      REQUIRE(dlg[i].data.vals[0] == i * 1.f);
      REQUIRE(dlg[i].data.vals[1] == i * 10.f);
    }

    // the queue has been drained
    dlg.Clear();
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 0);
  }

  SECTION("GetLastData returns the most recently transmitted packet")
  {
    REQUIRE(sender.GetLastData().ctrlTag == kNoTag);

    sender.PushData(TPeakData {kTag, std::array<float, 2> {0.1f, 0.2f}});
    sender.PushData(TPeakData {kTag, std::array<float, 2> {0.3f, 0.4f}});

    // not until it has been transmitted
    REQUIRE(sender.GetLastData().ctrlTag == kNoTag);

    sender.TransmitData(dlg);
    REQUIRE(sender.GetLastData().ctrlTag == kTag);
    REQUIRE(sender.GetLastData().vals[0] == 0.3f);
    REQUIRE(sender.GetLastData().vals[1] == 0.4f);
  }

  SECTION("packets pushed when the queue is full are dropped")
  {
    for (int i = 0; i < 20; i++)
      sender.PushData(TPeakData {kTag, std::array<float, 2> {i * 1.f, 0.f}});

    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 8);
    for (int i = 0; i < 8; i++)
      REQUIRE(dlg[i].data.vals[0] == i * 1.f);

    // and the sender keeps working afterwards
    dlg.Clear();
    sender.PushData(TPeakData {kTag, std::array<float, 2> {99.f, 0.f}});
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 1);
    REQUIRE(dlg[0].data.vals[0] == 99.f);
  }
}

TEST_CASE("ISender: TransmitDataToControlsWithTags sends each packet to every tag", "[unit][sender]")
{
  ISender<2, 8, float> sender;
  RecordingDelegate<TPeakData> dlg;

  // no tag is needed in the packets themselves
  sender.PushData(TPeakData {kNoTag, std::array<float, 2> {1.f, 2.f}});
  sender.PushData(TPeakData {kNoTag, std::array<float, 2> {3.f, 4.f}});

  sender.TransmitDataToControlsWithTags(dlg, {10, 11, 12});

  REQUIRE(dlg.NMsgs() == 6);
  REQUIRE(dlg.mNumMalformed == 0);

  for (int packet = 0; packet < 2; packet++)
  {
    for (int tagIdx = 0; tagIdx < 3; tagIdx++)
    {
      const auto& msg = dlg[packet * 3 + tagIdx];
      CAPTURE(packet, tagIdx);
      REQUIRE(msg.ctrlTag == 10 + tagIdx);
      REQUIRE(msg.msgTag == (ISender<2, 8, float>::kUpdateMessage));
      REQUIRE(msg.data.ctrlTag == 10 + tagIdx);
      REQUIRE(msg.data.vals[0] == (packet == 0 ? 1.f : 3.f));
      REQUIRE(msg.data.vals[1] == (packet == 0 ? 2.f : 4.f));
    }
  }

  dlg.Clear();
  sender.TransmitDataToControlsWithTags(dlg, {10, 11, 12});
  REQUIRE(dlg.NMsgs() == 0);

  SECTION("an empty tag list still drains the queue")
  {
    sender.PushData(TPeakData {kNoTag, std::array<float, 2> {1.f, 2.f}});
    sender.TransmitDataToControlsWithTags(dlg, {});
    REQUIRE(dlg.NMsgs() == 0);
    sender.TransmitDataToControlsWithTags(dlg, {10});
    REQUIRE(dlg.NMsgs() == 0);
  }
}

namespace
{
struct DoublingSender : public ISender<2, 8, float>
{
  void PrepareDataForUI(ISenderData<2, float>& d) override
  {
    mNumPrepared++;
    for (auto& v : d.vals)
      v *= 2.f;
  }

  int mNumPrepared = 0;
};
} // namespace

TEST_CASE("ISender: PrepareDataForUI can transform data before it is sent", "[unit][sender]")
{
  DoublingSender sender;
  RecordingDelegate<TPeakData> dlg;

  sender.PushData(TPeakData {kTag, std::array<float, 2> {1.f, 2.f}});
  sender.PushData(TPeakData {kTag, std::array<float, 2> {3.f, 4.f}});
  REQUIRE(sender.mNumPrepared == 0);

  sender.TransmitData(dlg);
  REQUIRE(sender.mNumPrepared == 2);
  REQUIRE(dlg.NMsgs() == 2);
  REQUIRE(dlg[0].data.vals[0] == 2.f);
  REQUIRE(dlg[0].data.vals[1] == 4.f);
  REQUIRE(dlg[1].data.vals[0] == 6.f);
  REQUIRE(dlg[1].data.vals[1] == 8.f);

  // the last data is what the UI received
  REQUIRE(sender.GetLastData().vals[0] == 6.f);
}

// BUG (suspected): ISender.h:97-110. TransmitDataToControlsWithTags() is documented as a variation of
// TransmitData() for sending the same data to several controls, but unlike TransmitData() it never calls
// PrepareDataForUI() and does not update the data returned by GetLastData(). An ISpectrumSender used this
// way sends raw sample buffers instead of spectra.
TEST_CASE("ISender: TransmitDataToControlsWithTags prepares data like TransmitData", "[.][bug]")
{
  DoublingSender sender;
  RecordingDelegate<TPeakData> dlg;

  sender.PushData(TPeakData {kNoTag, std::array<float, 2> {1.f, 2.f}});
  sender.TransmitDataToControlsWithTags(dlg, {10, 11});

  REQUIRE(sender.mNumPrepared == 1);
  REQUIRE(dlg.NMsgs() == 2);
  REQUIRE(dlg[0].data.vals[0] == 2.f);
  REQUIRE(dlg[1].data.vals[0] == 2.f);
  REQUIRE(sender.GetLastData().vals[0] == 2.f);
}

// IPeakSender

TEST_CASE("IPeakSender: sends the mean absolute level of each window", "[unit][sender][peak]")
{
  IPeakSender<2> sender;
  RecordingDelegate<TPeakData> dlg;

  Signal signal {2, kPeakWindow * 4};
  signal.Fill(0, 1.);
  signal.Fill(1, 0.5);

  SECTION("constant signal")
  {
    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    REQUIRE(dlg.NMsgs() == 0);

    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() >= 3);
    REQUIRE(dlg.mNumMalformed == 0);

    const auto& last = dlg.Last();
    REQUIRE(last.ctrlTag == kTag);
    REQUIRE(last.msgTag == 0);
    REQUIRE(last.dataSize == static_cast<int>(sizeof(TPeakData)));
    REQUIRE(last.data.ctrlTag == kTag);
    REQUIRE(last.data.nChans == 2);
    REQUIRE(last.data.chanOffset == 0);
    REQUIRE(last.data.vals[0] == Approx(1.f));
    REQUIRE(last.data.vals[1] == Approx(0.5f));

    for (int i = 0; i < dlg.NMsgs(); i++)
    {
      REQUIRE(dlg[i].data.vals[0] >= 0.f);
      REQUIRE(dlg[i].data.vals[0] <= Approx(1.f));
    }
  }

  SECTION("the signal is rectified")
  {
    for (int s = 0; s < signal.NFrames(); s++)
    {
      signal[0][s] = (s % 2) ? 0.5 : -0.5;
      signal[1][s] = -0.25;
    }

    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() >= 3);
    REQUIRE(dlg.Last().data.vals[0] == Approx(0.5f));
    REQUIRE(dlg.Last().data.vals[1] == Approx(0.25f));
  }

  SECTION("a ramp averages to its mean")
  {
    // every window sees the same ramp from 0 towards 1
    for (int s = 0; s < signal.NFrames(); s++)
      signal[0][s] = static_cast<sample>(s % kPeakWindow) / kPeakWindow;

    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.Last().data.vals[0] == Approx(0.5 * (kPeakWindow - 1) / kPeakWindow));
  }

  SECTION("the host's block size does not change the result")
  {
    const int blockSize = GENERATE(1, 7, 64, 240, 333);
    CAPTURE(blockSize);

    IPeakSender<2> reference;
    RecordingDelegate<TPeakData> referenceDlg;
    reference.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    reference.TransmitData(referenceDlg);

    ProcessInBlocks(sender, signal, blockSize, kTag, 2);
    sender.TransmitData(dlg);

    REQUIRE(dlg.NMsgs() == referenceDlg.NMsgs());
    for (int i = 0; i < dlg.NMsgs(); i++)
    {
      REQUIRE(dlg[i].data.vals[0] == Approx(referenceDlg[i].data.vals[0]));
      REQUIRE(dlg[i].data.vals[1] == Approx(referenceDlg[i].data.vals[1]));
    }
  }
}

TEST_CASE("IPeakSender: sends one packet per window while there is signal", "[unit][sender][peak]")
{
  IPeakSender<2> sender;
  RecordingDelegate<TPeakData> dlg;
  Signal signal {2, kPeakWindow * 8, 0.5};

  SECTION("default 5 ms window at 48 kHz")
  {
    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 8);
  }

  SECTION("SetWindowSizeMs")
  {
    sender.SetWindowSizeMs(10., 48000.);
    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 4);
    REQUIRE(dlg.Last().data.vals[0] == Approx(0.5f));
  }

  SECTION("Reset applies the window duration to a new sample rate")
  {
    sender.Reset(96000.);
    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 4);
    REQUIRE(dlg.Last().data.vals[0] == Approx(0.5f));

    dlg.Clear();
    sender.Reset(24000.);
    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 16);
    REQUIRE(dlg.Last().data.vals[0] == Approx(0.5f));
  }
}

// BUG: ISender.h:127-132. The windowSizeMs constructor argument of IPeakSender is never used: mWindowSizeMs
// keeps its default of 5 ms, so the window can only be changed afterwards with SetWindowSizeMs().
TEST_CASE("IPeakSender: the constructor's window size is used", "[.][bug]")
{
  IPeakSender<2> sender {-90., 10.f};
  RecordingDelegate<TPeakData> dlg;
  Signal signal {2, kPeakWindow * 8, 0.5};

  sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
  sender.TransmitData(dlg);
  REQUIRE(dlg.NMsgs() == 4);
}

TEST_CASE("IPeakSender: stops sending when the signal falls below the threshold", "[unit][sender][peak]")
{
  RecordingDelegate<TPeakData> dlg;
  Signal silence {2, kPeakWindow * 10};
  Signal loud {2, kPeakWindow * 4, 0.5};

  SECTION("silence produces no packets once the meter has been zeroed")
  {
    IPeakSender<2> sender;
    sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
    sender.TransmitData(dlg);
    dlg.Clear();

    sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 0);
  }

  SECTION("after a signal ends the meter is returned to zero, then the sender goes quiet")
  {
    IPeakSender<2> sender;
    sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
    sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
    sender.TransmitData(dlg);

    REQUIRE(dlg.NMsgs() >= 2);
    REQUIRE(dlg.Last().data.vals[0] == 0.f);
    REQUIRE(dlg.Last().data.vals[1] == 0.f);

    bool sawSignal = false;
    for (int i = 0; i < dlg.NMsgs(); i++)
      sawSignal = sawSignal || dlg[i].data.vals[0] > 0.4f;
    REQUIRE(sawSignal);

    dlg.Clear();
    sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 0);

    // and it wakes up again
    sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() >= 3);
    REQUIRE(dlg.Last().data.vals[0] == Approx(0.5f));
  }

  SECTION("the threshold is configurable")
  {
    Signal quiet {2, kPeakWindow * 10, 0.001}; // -60 dB

    IPeakSender<2> highThreshold {-20.};
    highThreshold.ProcessBlock(quiet.Get(), quiet.NFrames(), kTag);
    highThreshold.TransmitData(dlg);
    dlg.Clear();
    highThreshold.ProcessBlock(quiet.Get(), quiet.NFrames(), kTag);
    highThreshold.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 0);

    IPeakSender<2> lowThreshold {-90.};
    lowThreshold.ProcessBlock(quiet.Get(), quiet.NFrames(), kTag);
    lowThreshold.TransmitData(dlg);
    dlg.Clear();
    lowThreshold.ProcessBlock(quiet.Get(), quiet.NFrames(), kTag);
    lowThreshold.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 10);
    REQUIRE(dlg.Last().data.vals[0] == Approx(0.001f));
  }
}

TEST_CASE("IPeakSender: channel count and offset", "[unit][sender][peak]")
{
  using TData = ISenderData<4, float>;
  IPeakSender<4> sender;
  RecordingDelegate<TData> dlg;

  Signal signal {4, kPeakWindow * 4};
  signal.Fill(0, 0.1);
  signal.Fill(1, 0.2);
  signal.Fill(2, 0.3);
  signal.Fill(3, 0.4);

  SECTION("a subset of channels starting at an offset")
  {
    ProcessInBlocks(sender, signal, 64, kTag, 2, 2);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() >= 3);
    REQUIRE(dlg.mNumMalformed == 0);

    const auto& d = dlg.Last().data;
    REQUIRE(d.nChans == 2);
    REQUIRE(d.chanOffset == 2);
    REQUIRE(d.vals[0] == 0.f);
    REQUIRE(d.vals[1] == 0.f);
    REQUIRE(d.vals[2] == Approx(0.3f));
    REQUIRE(d.vals[3] == Approx(0.4f));
  }

  SECTION("fewer channels than the maximum")
  {
    ProcessInBlocks(sender, signal, 64, kTag, 1);
    sender.TransmitData(dlg);
    const auto& d = dlg.Last().data;
    REQUIRE(d.nChans == 1);
    REQUIRE(d.chanOffset == 0);
    REQUIRE(d.vals[0] == Approx(0.1f));
    REQUIRE(d.vals[1] == 0.f);
    REQUIRE(d.vals[2] == 0.f);
    REQUIRE(d.vals[3] == 0.f);
  }

  SECTION("all channels by default")
  {
    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    sender.TransmitData(dlg);
    const auto& d = dlg.Last().data;
    REQUIRE(d.nChans == 4);
    for (int c = 0; c < 4; c++)
      REQUIRE(d.vals[static_cast<size_t>(c)] == Approx(0.1f * (c + 1)));
  }
}

TEST_CASE("IPeakSender: packets without a tag can be routed with TransmitDataToControlsWithTags", "[unit][sender][peak]")
{
  IPeakSender<2> sender;
  RecordingDelegate<TPeakData> dlg;
  Signal signal {2, kPeakWindow * 2, 0.5};

  sender.ProcessBlock(signal.Get(), signal.NFrames());
  sender.TransmitDataToControlsWithTags(dlg, {20, 21});

  REQUIRE(dlg.NMsgs() == 4);
  REQUIRE(dlg[0].ctrlTag == 20);
  REQUIRE(dlg[1].ctrlTag == 21);
  REQUIRE(dlg[2].ctrlTag == 20);
  REQUIRE(dlg[3].ctrlTag == 21);
  REQUIRE(dlg[3].data.ctrlTag == 21);
  REQUIRE(dlg[3].data.vals[0] == Approx(0.5f));
  REQUIRE(dlg[2].data.vals[0] == dlg[3].data.vals[0]);
}

// IPeakAvgSender

TEST_CASE("IPeakAvgSender: EnvelopeFollower", "[unit][sender][peakavg]")
{
  IPeakAvgSender<1>::EnvelopeFollower env;
  float attack = 1.f;
  float decay = 1.f;
  float in = 0.f;

  SECTION("times of one window or less follow the input immediately")
  {
    in = 0.8f;
    REQUIRE(env.Process(in, attack, decay) == 0.8f);
    in = 0.2f;
    REQUIRE(env.Process(in, attack, decay) == 0.2f);
    in = 0.f;
    REQUIRE(env.Process(in, attack, decay) == 0.f);

    attack = 0.f;
    decay = 0.5f;
    in = 1.f;
    REQUIRE(env.Process(in, attack, decay) == 1.f);
    in = 0.5f;
    REQUIRE(env.Process(in, attack, decay) == 0.5f);
  }

  SECTION("attack moves a fraction of the way to the input per step")
  {
    attack = 4.f;
    in = 1.f;
    REQUIRE(env.Process(in, attack, decay) == Approx(0.25f));
    REQUIRE(env.Process(in, attack, decay) == Approx(0.4375f));
    REQUIRE(env.Process(in, attack, decay) == Approx(0.578125f));

    float prev = 0.578125f;
    for (int i = 0; i < 100; i++)
    {
      const float out = env.Process(in, attack, decay);
      REQUIRE(out >= prev);
      REQUIRE(out <= 1.f);
      prev = out;
    }
    REQUIRE(prev == Approx(1.f));
  }

  SECTION("decay moves a fraction of the way to the input per step")
  {
    in = 1.f;
    env.Process(in, attack, decay);

    decay = 2.f;
    in = 0.f;
    REQUIRE(env.Process(in, attack, decay) == Approx(0.5f));
    REQUIRE(env.Process(in, attack, decay) == Approx(0.25f));
    REQUIRE(env.Process(in, attack, decay) == Approx(0.125f));

    // decays towards the input, not towards zero
    in = 0.1f;
    float prev = 0.125f;
    for (int i = 0; i < 100; i++)
    {
      const float out = env.Process(in, attack, decay);
      REQUIRE(out <= prev);
      REQUIRE(out >= 0.1f);
      prev = out;
    }
    REQUIRE(prev == Approx(0.1f));
  }

  SECTION("attack and decay are independent")
  {
    attack = 1.f;
    decay = 10.f;
    in = 1.f;
    REQUIRE(env.Process(in, attack, decay) == 1.f);
    in = 0.f;
    REQUIRE(env.Process(in, attack, decay) == Approx(0.9f));
    in = 1.f;
    REQUIRE(env.Process(in, attack, decay) == 1.f);
  }

  SECTION("an unchanged input holds the output")
  {
    attack = 8.f;
    decay = 8.f;
    in = 0.f;
    REQUIRE(env.Process(in, attack, decay) == 0.f);
    REQUIRE(env.Process(in, attack, decay) == 0.f);
  }
}

TEST_CASE("IPeakAvgSender: sends peak and average levels", "[unit][sender][peakavg]")
{
  RecordingDelegate<TPeakAvgData> dlg;
  Signal signal {2, kPeakWindow * 6};

  SECTION("constant signal: peak and RMS are both the level")
  {
    IPeakAvgSender<2> sender;
    signal.Fill(0, 0.5);
    signal.Fill(1, -0.25);

    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    REQUIRE(dlg.NMsgs() == 0);
    sender.TransmitData(dlg);

    REQUIRE(dlg.NMsgs() == 6);
    REQUIRE(dlg.mNumMalformed == 0);

    const auto& last = dlg.Last();
    REQUIRE(last.ctrlTag == kTag);
    REQUIRE(last.msgTag == 0);
    REQUIRE(last.dataSize == static_cast<int>(sizeof(TPeakAvgData)));
    REQUIRE(last.data.ctrlTag == kTag);
    REQUIRE(last.data.nChans == 2);
    REQUIRE(last.data.chanOffset == 0);
    REQUIRE(last.data.vals[0].first == Approx(0.5f));
    REQUIRE(last.data.vals[0].second == Approx(0.5f));
    REQUIRE(last.data.vals[1].first == Approx(0.25f));
    REQUIRE(last.data.vals[1].second == Approx(0.25f));
  }

  SECTION("RMS mode versus mean absolute mode")
  {
    // half the samples at +1, half at -0.5
    for (int s = 0; s < signal.NFrames(); s++)
    {
      signal[0][s] = (s % 2) ? -0.5 : 1.;
      signal[1][s] = signal[0][s];
    }

    IPeakAvgSender<2> rms {-90., true};
    rms.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    rms.TransmitData(dlg);
    REQUIRE(dlg.Last().data.vals[0].first == Approx(1.f));
    REQUIRE(dlg.Last().data.vals[0].second == Approx(std::sqrt(0.625f)).epsilon(1e-3));
    REQUIRE(dlg.Last().data.vals[1].second == Approx(std::sqrt(0.625f)).epsilon(1e-3));

    dlg.Clear();
    IPeakAvgSender<2> mean {-90., false};
    mean.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    mean.TransmitData(dlg);
    REQUIRE(dlg.Last().data.vals[0].first == Approx(1.f));
    REQUIRE(dlg.Last().data.vals[0].second == Approx(0.75f).epsilon(1e-3));
  }

  SECTION("a sine wave's RMS is 3 dB below its peak")
  {
    // 6 cycles per window, so every window is identical
    for (int s = 0; s < signal.NFrames(); s++)
      signal[0][s] = signal[1][s] = std::sin(2. * PI * 6. * s / kPeakWindow);

    IPeakAvgSender<2> sender;
    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.Last().data.vals[0].first == Approx(1.f).epsilon(1e-3));
    REQUIRE(dlg.Last().data.vals[0].second == Approx(std::sqrt(0.5f)).epsilon(1e-3));
  }

  SECTION("host blocks that are a whole number of windows")
  {
    const int blockSize = GENERATE(240, 480, 720);
    CAPTURE(blockSize);
    signal.Fill(0, 0.5);
    signal.Fill(1, 0.125);

    IPeakAvgSender<2> sender;
    ProcessInBlocks(sender, signal, blockSize, kTag, 2);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 6);
    REQUIRE(dlg.Last().data.vals[0].first == Approx(0.5f));
    REQUIRE(dlg.Last().data.vals[0].second == Approx(0.5f));
    REQUIRE(dlg.Last().data.vals[1].first == Approx(0.125f));
    REQUIRE(dlg.Last().data.vals[1].second == Approx(0.125f));
  }
}

// BUG: ISender.h:298. IPeakAvgSender::ProcessBlock() indexes its analysis window with the sample index
// within the current host block ("s % mWindowSize") instead of a position that carries on across calls.
// Whenever the host's block size is not a whole number of windows the window is filled wrongly: with blocks
// shorter than the window (e.g. 64 samples against the default 240 sample window) only the first blockSize
// entries are ever written and the rest stay at zero, so the average reads several dB low.
TEST_CASE("IPeakAvgSender: the host's block size does not change the result", "[.][bug]")
{
  const int blockSize = GENERATE(1, 16, 64, 100, 333);
  CAPTURE(blockSize);

  Signal signal {2, kPeakWindow * 6};
  signal.Fill(0, 0.5);
  signal.Fill(1, 0.125);

  IPeakAvgSender<2> sender;
  RecordingDelegate<TPeakAvgData> dlg;
  ProcessInBlocks(sender, signal, blockSize, kTag, 2);
  sender.TransmitData(dlg);

  REQUIRE(dlg.NMsgs() == 6);
  REQUIRE(dlg.Last().data.vals[0].first == Approx(0.5f));
  REQUIRE(dlg.Last().data.vals[0].second == Approx(0.5f));
  REQUIRE(dlg.Last().data.vals[1].first == Approx(0.125f));
  REQUIRE(dlg.Last().data.vals[1].second == Approx(0.125f));
}

TEST_CASE("IPeakAvgSender: window size", "[unit][sender][peakavg]")
{
  RecordingDelegate<TPeakAvgData> dlg;
  Signal signal {2, kPeakWindow * 8, 0.5};

  SECTION("constructor argument")
  {
    IPeakAvgSender<2> sender {-90., true, 10.f};
    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 4);
    REQUIRE(dlg.Last().data.vals[0].second == Approx(0.5f));
  }

  SECTION("SetWindowSizeMs")
  {
    IPeakAvgSender<2> sender;
    sender.SetWindowSizeMs(20., 48000.);
    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 2);
  }

  SECTION("Reset applies the window duration to a new sample rate")
  {
    IPeakAvgSender<2> sender;
    sender.Reset(96000.);
    sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 4);
    REQUIRE(dlg.Last().data.vals[0].first == Approx(0.5f));
  }
}

TEST_CASE("IPeakAvgSender: peak hold", "[unit][sender][peakavg]")
{
  // 20 ms hold = 4 windows
  IPeakAvgSender<2> sender {-90., true, 5.f, 1.f, 100.f, 20.f};
  RecordingDelegate<TPeakAvgData> dlg;

  Signal loud {2, kPeakWindow * 2, 1.};
  Signal quiet {2, kPeakWindow * 2, 0.1};
  Signal longQuiet {2, kPeakWindow * 12, 0.1};

  sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
  sender.ProcessBlock(quiet.Get(), quiet.NFrames(), kTag);
  sender.TransmitData(dlg);

  SECTION("the peak is held after the signal drops")
  {
    REQUIRE(dlg.Last().data.vals[0].first == Approx(1.f));
    REQUIRE(dlg.Last().data.vals[1].first == Approx(1.f));
  }

  SECTION("after the hold time the peak falls to the current level")
  {
    sender.ProcessBlock(longQuiet.Get(), longQuiet.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.Last().data.vals[0].first == Approx(0.1f));
    REQUIRE(dlg.Last().data.vals[1].first == Approx(0.1f));

    // peaks only ever step down once the loud signal has gone
    float prev = 1.f;
    bool neverRises = true;
    for (int i = 0; i < dlg.NMsgs(); i++)
    {
      neverRises = neverRises && (dlg[i].data.vals[0].first <= prev || i < 2);
      prev = dlg[i].data.vals[0].first;
    }
    REQUIRE(neverRises);
  }

  SECTION("a louder peak replaces the held value straight away")
  {
    Signal louder {2, kPeakWindow * 2, 1.5};
    sender.ProcessBlock(louder.Get(), louder.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.Last().data.vals[0].first == Approx(1.5f));
  }

  SECTION("SetPeakHoldTimeMs")
  {
    // 200 ms = 40 windows: still held after 12 more quiet windows
    sender.SetPeakHoldTimeMs(200., 48000.);
    sender.ProcessBlock(longQuiet.Get(), longQuiet.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.Last().data.vals[0].first == Approx(1.f));
  }
}

TEST_CASE("IPeakAvgSender: attack and decay ballistics of the average", "[unit][sender][peakavg]")
{
  RecordingDelegate<TPeakAvgData> dlg;
  Signal loud {2, kPeakWindow * 4, 0.5};
  Signal silence {2, kPeakWindow * 8};

  SECTION("a slow attack rises gradually towards the level")
  {
    IPeakAvgSender<2> sender {-90., true, 5.f, 50.f, 100.f};
    sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 4);

    float prev = 0.f;
    for (int i = 0; i < dlg.NMsgs(); i++)
    {
      const float avg = dlg[i].data.vals[0].second;
      REQUIRE(avg > prev);
      REQUIRE(avg < 0.5f);
      prev = avg;
    }

    // the peak is not smoothed
    REQUIRE(dlg.Last().data.vals[0].first == Approx(0.5f));
  }

  SECTION("a fast attack reaches the level within a window")
  {
    IPeakAvgSender<2> sender {-90., true, 5.f, 1.f, 100.f};
    sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg[1].data.vals[0].second == Approx(0.5f));
  }

  SECTION("the average decays gradually after the signal stops")
  {
    IPeakAvgSender<2> sender {-90., true, 5.f, 1.f, 100.f};
    sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
    sender.TransmitData(dlg);
    dlg.Clear();

    sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 8);

    float prev = 0.5f;
    for (int i = 0; i < dlg.NMsgs(); i++)
    {
      const float avg = dlg[i].data.vals[0].second;
      REQUIRE(avg < prev);
      REQUIRE(avg > 0.f);
      prev = avg;
    }

    // 100 ms decay: far from finished after 40 ms
    REQUIRE(prev > 0.25f);
  }

  SECTION("SetAttackTimeMs and SetDecayTimeMs")
  {
    IPeakAvgSender<2> sender {-90., true, 5.f, 50.f, 100.f};
    sender.SetAttackTimeMs(1., 48000.);
    sender.SetDecayTimeMs(1., 48000.);

    sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.Last().data.vals[0].second == Approx(0.5f));

    dlg.Clear();
    sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() >= 2);
    REQUIRE(dlg.Last().data.vals[0].second == 0.f);
  }
}

TEST_CASE("IPeakAvgSender: stops sending once everything has died away", "[unit][sender][peakavg]")
{
  // immediate decay and a short (20 ms) peak hold
  IPeakAvgSender<2> sender {-90., true, 5.f, 1.f, 1.f, 20.f};
  RecordingDelegate<TPeakAvgData> dlg;
  Signal loud {2, kPeakWindow * 2, 0.5};
  Signal silence {2, kPeakWindow * 20};

  sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
  sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
  sender.TransmitData(dlg);
  REQUIRE(dlg.NMsgs() >= 2);
  dlg.Clear();

  sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
  sender.TransmitData(dlg);
  REQUIRE(dlg.NMsgs() == 0);

  // and it wakes up again
  sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
  sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
  sender.TransmitData(dlg);
  REQUIRE(dlg.NMsgs() >= 2);
  REQUIRE(dlg.Last().data.vals[0].first == Approx(0.5f));
  REQUIRE(dlg.Last().data.vals[0].second == Approx(0.5f));
}

// BUG: ISender.h:387-401. When the smoothed average has fallen below the threshold, the code intends to keep
// pushing data while a peak hold is still active ("This makes sure that the data is still pushed if peakholds
// are still active"), but counterActive is initialised to false and then combined with "&=", so it can never
// become true and that branch is dead: updates stop as soon as the average decays, leaving the held peak
// stuck on screen without the sender ever reporting that it has been released.
TEST_CASE("IPeakAvgSender: keeps sending while a peak hold is active", "[.][bug]")
{
  // immediate decay, 2 second peak hold (400 windows)
  IPeakAvgSender<2> sender {-90., true, 5.f, 1.f, 1.f, 2000.f};
  RecordingDelegate<TPeakAvgData> dlg;
  Signal loud {2, kPeakWindow * 2, 0.5};
  Signal silence {2, kPeakWindow * 10};

  sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
  sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
  sender.TransmitData(dlg);
  dlg.Clear();

  // 50 ms into a 2 s hold
  sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
  sender.TransmitData(dlg);
  REQUIRE(dlg.NMsgs() > 0);
}

TEST_CASE("IPeakAvgSender: channel count and offset", "[unit][sender][peakavg]")
{
  using TData = ISenderData<4, std::pair<float, float>>;
  IPeakAvgSender<4> sender;
  RecordingDelegate<TData> dlg;

  Signal signal {4, kPeakWindow * 4};
  signal.Fill(0, 0.1);
  signal.Fill(1, 0.2);
  signal.Fill(2, 0.3);
  signal.Fill(3, 0.4);

  ProcessInBlocks(sender, signal, 240, kTag, 2, 2);
  sender.TransmitData(dlg);
  REQUIRE(dlg.NMsgs() == 4);
  REQUIRE(dlg.mNumMalformed == 0);

  const auto& d = dlg.Last().data;
  REQUIRE(d.nChans == 2);
  REQUIRE(d.chanOffset == 2);
  REQUIRE(d.vals[0].first == 0.f);
  REQUIRE(d.vals[0].second == 0.f);
  REQUIRE(d.vals[1].first == 0.f);
  REQUIRE(d.vals[1].second == 0.f);
  REQUIRE(d.vals[2].first == Approx(0.3f));
  REQUIRE(d.vals[2].second == Approx(0.3f));
  REQUIRE(d.vals[3].first == Approx(0.4f));
  REQUIRE(d.vals[3].second == Approx(0.4f));
}

// IBufferSender

TEST_CASE("IBufferSender: buffer size", "[unit][sender][buffer]")
{
  IBufferSender<2, 8, kBufSize> def;
  REQUIRE(def.GetBufferSize() == kBufSize);

  IBufferSender<2, 8, kBufSize> shortBuf {-90., 4};
  REQUIRE(shortBuf.GetBufferSize() == 4);

  shortBuf.SetBufferSize(8);
  REQUIRE(shortBuf.GetBufferSize() == 8);
  shortBuf.SetBufferSize(1);
  REQUIRE(shortBuf.GetBufferSize() == 1);
  shortBuf.SetBufferSize(kBufSize);
  REQUIRE(shortBuf.GetBufferSize() == kBufSize);
}

TEST_CASE("IBufferSender: sends consecutive buffers of the input", "[unit][sender][buffer]")
{
  IBufferSender<2, 8, kBufSize> sender;
  RecordingDelegate<TBufferData> dlg;

  // a ramp, so that every sample can be identified. One extra frame marks the end of the last full buffer
  const int nBuffers = 3;
  Signal signal {2, kBufSize * nBuffers + 1};
  for (int s = 0; s < signal.NFrames(); s++)
  {
    signal[0][s] = s;
    signal[1][s] = -s;
  }

  const int blockSize = GENERATE(1, 5, 16, 17, 1000);
  CAPTURE(blockSize);

  ProcessInBlocks(sender, signal, blockSize, kTag, 2);
  REQUIRE(dlg.NMsgs() == 0);
  sender.TransmitData(dlg);

  REQUIRE(dlg.NMsgs() == nBuffers);
  REQUIRE(dlg.mNumMalformed == 0);

  for (int b = 0; b < nBuffers; b++)
  {
    const auto& msg = dlg[b];
    CAPTURE(b);
    REQUIRE(msg.ctrlTag == kTag);
    REQUIRE(msg.msgTag == 0);
    REQUIRE(msg.dataSize == static_cast<int>(sizeof(TBufferData)));
    REQUIRE(msg.data.ctrlTag == kTag);
    REQUIRE(msg.data.nChans == 2);
    REQUIRE(msg.data.chanOffset == 0);

    bool contentsOK = true;
    for (int i = 0; i < kBufSize; i++)
    {
      const float expected = static_cast<float>(b * kBufSize + i);
      contentsOK = contentsOK && (msg.data.vals[0][static_cast<size_t>(i)] == expected);
      contentsOK = contentsOK && (msg.data.vals[1][static_cast<size_t>(i)] == -expected);
    }
    REQUIRE(contentsOK);
  }
}

TEST_CASE("IBufferSender: a smaller buffer size sends shorter buffers more often", "[unit][sender][buffer]")
{
  IBufferSender<2, 8, kBufSize> sender {-90., 4};
  RecordingDelegate<TBufferData> dlg;

  Signal signal {2, 4 * 5 + 1};
  for (int s = 0; s < signal.NFrames(); s++)
    signal[0][s] = signal[1][s] = s + 1;

  sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
  sender.TransmitData(dlg);
  REQUIRE(dlg.NMsgs() == 5);

  for (int b = 0; b < 5; b++)
  {
    for (int i = 0; i < 4; i++)
      REQUIRE(dlg[b].data.vals[0][static_cast<size_t>(i)] == static_cast<float>(b * 4 + i + 1));
  }
}

TEST_CASE("IBufferSender: threshold", "[unit][sender][buffer]")
{
  RecordingDelegate<TBufferData> dlg;
  Signal silence {2, kBufSize * 4 + 1};
  Signal loud {2, kBufSize * 4, 0.5};

  SECTION("silence is not sent once a zeroed buffer has gone out")
  {
    IBufferSender<2, 8, kBufSize> sender;
    sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
    sender.TransmitData(dlg);
    dlg.Clear();

    sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 0);
  }

  SECTION("a threshold of -100 dB or below disables the check, so silence is sent too")
  {
    const double thresholdDb = GENERATE(-100., -120., -1000.);
    IBufferSender<2, 8, kBufSize> sender {thresholdDb};

    sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 4);

    dlg.Clear();
    sender.ProcessBlock(silence.Get(), silence.NFrames() - 1, kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 4);

    for (int b = 0; b < 4; b++)
    {
      for (float v : dlg[b].data.vals[0])
        REQUIRE(v == 0.f);
    }
  }

  SECTION("after a signal ends one silent buffer is sent, then nothing")
  {
    IBufferSender<2, 8, kBufSize> sender;
    sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
    sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
    sender.TransmitData(dlg);

    REQUIRE(dlg.NMsgs() >= 2);
    for (float v : dlg.Last().data.vals[0])
      REQUIRE(v == 0.f);

    bool sawSignal = false;
    for (int i = 0; i < dlg.NMsgs(); i++)
      sawSignal = sawSignal || dlg[i].data.vals[0][0] == 0.5f;
    REQUIRE(sawSignal);

    dlg.Clear();
    sender.ProcessBlock(silence.Get(), silence.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 0);

    // and it wakes up again
    sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
    sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() >= 4);
    REQUIRE(dlg.Last().data.vals[0][kBufSize - 1] == 0.5f);
    REQUIRE(dlg.Last().data.vals[1][0] == 0.5f);
  }

  SECTION("signals far below the threshold are not sent")
  {
    Signal quiet {2, kBufSize * 4 + 1, 1e-6};
    IBufferSender<2, 8, kBufSize> sender {-20.};
    sender.ProcessBlock(quiet.Get(), quiet.NFrames(), kTag);
    sender.TransmitData(dlg);
    dlg.Clear();

    sender.ProcessBlock(quiet.Get(), quiet.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() == 0);

    sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
    sender.ProcessBlock(loud.Get(), loud.NFrames(), kTag);
    sender.TransmitData(dlg);
    REQUIRE(dlg.NMsgs() >= 4);
  }
}

TEST_CASE("IBufferSender: channel count and offset", "[unit][sender][buffer]")
{
  using TData = ISenderData<4, std::array<float, kBufSize>>;
  IBufferSender<4, 8, kBufSize> sender;
  RecordingDelegate<TData> dlg;

  Signal signal {4, kBufSize * 2 + 1};
  for (int c = 0; c < 4; c++)
    signal.Fill(c, 0.1 * (c + 1));

  ProcessInBlocks(sender, signal, 8, kTag, 1, 3);
  sender.TransmitData(dlg);
  REQUIRE(dlg.NMsgs() == 2);
  REQUIRE(dlg.mNumMalformed == 0);

  const auto& d = dlg.Last().data;
  REQUIRE(d.nChans == 1);
  REQUIRE(d.chanOffset == 3);

  for (int i = 0; i < kBufSize; i++)
  {
    REQUIRE(d.vals[3][static_cast<size_t>(i)] == Approx(0.4f));
    REQUIRE(d.vals[0][static_cast<size_t>(i)] == 0.f);
    REQUIRE(d.vals[1][static_cast<size_t>(i)] == 0.f);
    REQUIRE(d.vals[2][static_cast<size_t>(i)] == 0.f);
  }
}

TEST_CASE("IBufferSender: buffers pushed when the queue is full are dropped", "[unit][sender][buffer]")
{
  IBufferSender<1, 4, kBufSize> sender;
  RecordingDelegate<ISenderData<1, std::array<float, kBufSize>>> dlg;

  Signal signal {1, kBufSize * 10 + 1};
  for (int s = 0; s < signal.NFrames(); s++)
    signal[0][s] = s + 1;

  sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
  sender.TransmitData(dlg);

  // the oldest four are kept
  REQUIRE(dlg.NMsgs() == 4);
  for (int b = 0; b < 4; b++)
    REQUIRE(dlg[b].data.vals[0][0] == static_cast<float>(b * kBufSize + 1));
}

// ISpectrumSender

TEST_CASE("ISpectrumSender: configuration", "[unit][sender][spectrum]")
{
  TSpectrumSender sender {kFFTSize};

  REQUIRE(sender.GetFFTSize() == kFFTSize);
  REQUIRE(sender.GetOverlap() == 1);
  REQUIRE(sender.GetHopSize() == kFFTSize);
  REQUIRE(sender.GetBufferSize() == kFFTSize);
  REQUIRE(sender.GetWindowType() == TSpectrumSender::EWindowType::Hann);

  SECTION("constructor arguments")
  {
    TSpectrumSender custom {32, 2, TSpectrumSender::EWindowType::Hamming, TSpectrumSender::EOutputType::Complex};
    REQUIRE(custom.GetFFTSize() == 32);
    REQUIRE(custom.GetOverlap() == 2);
    REQUIRE(custom.GetHopSize() == 16);
    REQUIRE(custom.GetWindowType() == TSpectrumSender::EWindowType::Hamming);
  }

  SECTION("SetFFTSize keeps the overlap")
  {
    sender.SetFFTSizeAndOverlap(kFFTSize, 4);
    REQUIRE(sender.GetOverlap() == 4);
    REQUIRE(sender.GetHopSize() == 16);

    sender.SetFFTSize(32);
    REQUIRE(sender.GetFFTSize() == 32);
    REQUIRE(sender.GetOverlap() == 4);
    REQUIRE(sender.GetHopSize() == 8);
  }

  SECTION("SetFFTSizeAndOverlap")
  {
    struct Case { int fftSize; int overlap; int hop; };
    auto c = GENERATE(Case{64, 1, 64}, Case{64, 2, 32}, Case{64, 4, 16}, Case{32, 1, 32}, Case{16, 2, 8}, Case{8, 8, 1});
    sender.SetFFTSizeAndOverlap(c.fftSize, c.overlap);
    REQUIRE(sender.GetFFTSize() == c.fftSize);
    REQUIRE(sender.GetOverlap() == c.overlap);
    REQUIRE(sender.GetHopSize() == c.hop);
  }

  SECTION("SetWindowType")
  {
    using EWindowType = TSpectrumSender::EWindowType;
    auto type = GENERATE(EWindowType::Hann, EWindowType::BlackmanHarris, EWindowType::Hamming, EWindowType::Flattop, EWindowType::Rectangular);
    sender.SetWindowType(type);
    REQUIRE(sender.GetWindowType() == type);
    // the transform size is unaffected
    REQUIRE(sender.GetFFTSize() == kFFTSize);
  }
}

TEST_CASE("ISpectrumSender: magnitude spectrum of a sinusoid", "[unit][sender][spectrum]")
{
  const int nBins = kFFTSize / 2;

  SECTION("the energy is at the sinusoid's bin, at its RMS level")
  {
    const int bin = GENERATE(4, 8, 13, 24);
    const double amplitude = GENERATE(1., 0.25);
    CAPTURE(bin, amplitude);

    TSpectrumSender sender {kFFTSize};
    const TSpectrumData d = LastSpectrumOfSinusoid(sender, bin, amplitude);

    REQUIRE(d.ctrlTag == kTag);
    REQUIRE(d.nChans == 1);
    REQUIRE(LoudestBin(d, nBins) == bin);
    REQUIRE(d.vals[0][static_cast<size_t>(bin)] == Approx(amplitude / std::sqrt(2.)).epsilon(0.05));

    for (int i = 0; i < nBins; i++)
    {
      CAPTURE(i);
      const float mag = d.vals[0][static_cast<size_t>(i)];
      REQUIRE(mag >= 0.f);

      // the Hann window spreads the peak into the adjacent bins, but no further
      if (std::abs(i - bin) > 2)
      {
        REQUIRE(mag < 0.02 * amplitude);
      }
    }
  }

  SECTION("phases are in the range of atan2")
  {
    TSpectrumSender sender {kFFTSize};
    const TSpectrumData d = LastSpectrumOfSinusoid(sender, 8, 1.);

    for (int i = nBins; i < kFFTSize; i++)
    {
      REQUIRE(d.vals[0][static_cast<size_t>(i)] >= -3.1416f);
      REQUIRE(d.vals[0][static_cast<size_t>(i)] <= 3.1416f);
    }
  }

  SECTION("silence gives an empty spectrum")
  {
    TSpectrumSender sender {kFFTSize};
    const TSpectrumData d = LastSpectrumOfSinusoid(sender, 8, 0.);

    for (int i = 0; i < nBins; i++)
      REQUIRE(d.vals[0][static_cast<size_t>(i)] == 0.f);
  }

  SECTION("smaller transforms")
  {
    TSpectrumSender sender {32};
    const TSpectrumData d = LastSpectrumOfSinusoid(sender, 5, 1.);
    REQUIRE(LoudestBin(d, 16) == 5);
    REQUIRE(d.vals[0][5] == Approx(1. / std::sqrt(2.)).epsilon(0.08));
  }

  SECTION("with overlapping frames the steady state is the same")
  {
    const int overlap = GENERATE(2, 4);
    CAPTURE(overlap);
    TSpectrumSender sender {kFFTSize, overlap};
    // the sender's queue holds 4 packets: enough hops for every frame to have been filled once
    const TSpectrumData d = LastSpectrumOfSinusoid(sender, 8, 1., false, 4);
    REQUIRE(LoudestBin(d, nBins) == 8);
    REQUIRE(d.vals[0][8] == Approx(1. / std::sqrt(2.)).epsilon(0.05));
  }

  SECTION("every window type finds the peak")
  {
    using EWindowType = TSpectrumSender::EWindowType;
    auto type = GENERATE(EWindowType::Hann, EWindowType::BlackmanHarris, EWindowType::Hamming, EWindowType::Flattop, EWindowType::Rectangular);
    TSpectrumSender sender {kFFTSize, 1, type};
    const TSpectrumData d = LastSpectrumOfSinusoid(sender, 10, 1.);
    // flat-top windows trade frequency resolution for amplitude accuracy, so allow the adjacent bins
    REQUIRE(std::abs(LoudestBin(d, nBins) - 10) <= 1);
    REQUIRE(d.vals[0][10] > 0.2f);
    REQUIRE(d.vals[0][20] < 0.05f);
  }
}

// A full scale sinusoid at a bin centre reads its RMS level whichever window is used
TEST_CASE("ISpectrumSender: magnitudes are calibrated for every window type", "[unit][sender][spectrum]")
{
  using EWindowType = TSpectrumSender::EWindowType;
  auto type = GENERATE(EWindowType::Hann, EWindowType::BlackmanHarris, EWindowType::Hamming, EWindowType::Flattop, EWindowType::Rectangular);

  TSpectrumSender sender {kFFTSize, 1, type};
  const TSpectrumData d = LastSpectrumOfSinusoid(sender, 10, 1.);
  REQUIRE(d.vals[0][10] == Approx(1. / std::sqrt(2.)).epsilon(0.05));
}

TEST_CASE("ISpectrumSender: complex output", "[unit][sender][spectrum]")
{
  using EWindowType = TSpectrumSender::EWindowType;
  using EOutputType = TSpectrumSender::EOutputType;

  const int nBins = kFFTSize / 2;
  const int bin = 4;

  // a rectangular window and a whole number of cycles: the transform is exact
  TSpectrumSender sender {kFFTSize, 1, EWindowType::Rectangular, EOutputType::Complex};

  SECTION("a cosine is purely real")
  {
    const TSpectrumData d = LastSpectrumOfSinusoid(sender, bin, 1., true);

    for (int i = 0; i < nBins; i++)
    {
      CAPTURE(i);
      const float re = d.vals[0][static_cast<size_t>(i)];
      const float im = d.vals[0][static_cast<size_t>(i + nBins)];
      REQUIRE(re == Approx(i == bin ? kFFTSize / 2. : 0.).margin(1e-3));
      REQUIRE(im == Approx(0.).margin(1e-3));
    }
  }

  SECTION("a sine is purely imaginary")
  {
    const TSpectrumData d = LastSpectrumOfSinusoid(sender, bin, 0.5);

    for (int i = 0; i < nBins; i++)
    {
      CAPTURE(i);
      const float re = d.vals[0][static_cast<size_t>(i)];
      const float im = d.vals[0][static_cast<size_t>(i + nBins)];
      REQUIRE(re == Approx(0.).margin(1e-3));
      REQUIRE(std::fabs(im) == Approx(i == bin ? 0.5 * kFFTSize / 2. : 0.).margin(1e-3));
    }
  }

  SECTION("SetOutputType switches to magnitude and phase")
  {
    sender.SetOutputType(EOutputType::MagPhase);
    sender.SetWindowType(EWindowType::Hann);
    const TSpectrumData d = LastSpectrumOfSinusoid(sender, bin, 1.);
    REQUIRE(LoudestBin(d, nBins) == bin);
    REQUIRE(d.vals[0][static_cast<size_t>(bin)] == Approx(1. / std::sqrt(2.)).epsilon(0.05));
  }
}

TEST_CASE("ISpectrumSender: channels are transformed independently", "[unit][sender][spectrum]")
{
  using TData = ISenderData<2, std::array<float, kFFTSize>>;
  ISpectrumSender<2, 4, kFFTSize> sender {kFFTSize};
  RecordingDelegate<TData> dlg;

  Signal signal {2, kFFTSize + 1};
  for (int s = 0; s < signal.NFrames(); s++)
  {
    signal[0][s] = std::sin(2. * PI * 6. * s / kFFTSize);
    signal[1][s] = 0.5 * std::sin(2. * PI * 20. * s / kFFTSize);
  }

  sender.ProcessBlock(signal.Get(), signal.NFrames(), kTag);
  sender.TransmitData(dlg);
  REQUIRE(dlg.NMsgs() == 1);
  REQUIRE(dlg.mNumMalformed == 0);

  const auto& d = dlg.Last().data;
  REQUIRE(d.nChans == 2);
  REQUIRE(d.vals[0][6] == Approx(1. / std::sqrt(2.)).epsilon(0.05));
  REQUIRE(d.vals[0][20] < 0.02f);
  REQUIRE(d.vals[1][20] == Approx(0.5 / std::sqrt(2.)).epsilon(0.05));
  REQUIRE(d.vals[1][6] < 0.02f);
}
