/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests audio processing through IPlugProcessor: sample formats, channel connections, block sizes, bypass, latency,
// tail, offline rendering and transport information.

#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "AudioBuffers.h"
#include "TestPlugins.h"

using namespace iplug;
using Catch::Approx;

namespace
{

/** A deterministic, non-repeating test signal in (-1, 1) which is different on every channel and never zero */
template <typename T>
T TestSignal(int chan, int frame)
{
  const int v = ((frame * 37 + chan * 101 + 11) % 199) - 99; // -99 ... 99
  return static_cast<T>(v == 0 ? 100 : v) / static_cast<T>(128);
}

template <typename T>
void FillWithTestSignal(AudioBuffers<T>& buffers, int startFrame = 0)
{
  for (int c = 0; c < buffers.NChans(); c++)
    for (int s = 0; s < buffers.NFrames(); s++)
      buffers[c][static_cast<size_t>(s)] = TestSignal<T>(c, startFrame + s);
}

/** An effect which doesn't override ProcessBlock() */
class DefaultProcessPlug : public RecordingPlug
{
public:
  DefaultProcessPlug(const char* channelIO, int latency = 0)
  : RecordingPlug(InstanceInfo{}, GainPlug::MakeConfig(channelIO, latency))
  {
  }
};

/** Runs a signal through a bypassed plug-in in blocks and returns everything that came out of one channel */
template <typename T>
std::vector<T> ProcessBypassed(IPlugTestAPI& plug, int chan, int nChans, int blockSize, int nBlocks)
{
  std::vector<T> result;

  for (int block = 0; block < nBlocks; block++)
  {
    AudioBuffers<T> in {nChans, blockSize}, out {nChans, blockSize, T(99)};
    FillWithTestSignal(in, block * blockSize);
    plug.HostProcess(in.Get(), out.Get(), blockSize);
    result.insert(result.end(), out[chan].begin(), out[chan].end());
  }

  return result;
}

} // namespace

TEST_CASE("Processing: single and double precision host buffers give the same result", "[integration][processing]")
{
  const int nFrames = 128;

  GainPlug plugF, plugD;
  plugF.HostActivate(48000., nFrames);
  plugD.HostActivate(48000., nFrames);
  plugF.HostSetParameterNormalized(GainPlug::kGain, 0.25);
  plugD.HostSetParameterNormalized(GainPlug::kGain, 0.25);

  AudioBuffers<float> inF {2, nFrames}, outF {2, nFrames};
  AudioBuffers<double> inD {2, nFrames}, outD {2, nFrames};
  FillWithTestSignal(inF);

  // Feed the double precision plug-in exactly the values the single precision one gets
  for (int c = 0; c < 2; c++)
    for (int s = 0; s < nFrames; s++)
      inD[c][static_cast<size_t>(s)] = static_cast<double>(inF[c][static_cast<size_t>(s)]);

  plugF.HostProcess(inF.Get(), outF.Get(), nFrames);
  plugD.HostProcess(inD.Get(), outD.Get(), nFrames);

  for (int c = 0; c < 2; c++)
  {
    for (int s = 0; s < nFrames; s++)
    {
      const size_t i = static_cast<size_t>(s);
      CHECK(outD[c][i] == static_cast<double>(inF[c][i]) * 0.5);
      CHECK(outF[c][i] == static_cast<float>(outD[c][i]));
    }
  }
}

TEST_CASE("Processing: the host's input buffers are not modified", "[integration][processing]")
{
  GainPlug plug;
  plug.HostActivate(48000., 64);
  plug.HostSetParameterNormalized(GainPlug::kGain, 0.);

  AudioBuffers<float> inF {2, 64}, outF {2, 64};
  AudioBuffers<double> inD {2, 64}, outD {2, 64};
  FillWithTestSignal(inF);
  FillWithTestSignal(inD);

  plug.HostProcess(inF.Get(), outF.Get(), 64);
  plug.HostProcess(inD.Get(), outD.Get(), 64);

  for (int c = 0; c < 2; c++)
  {
    for (int s = 0; s < 64; s++)
    {
      CHECK(inF[c][static_cast<size_t>(s)] == TestSignal<float>(c, s));
      CHECK(inD[c][static_cast<size_t>(s)] == TestSignal<double>(c, s));
      CHECK(outF[c][static_cast<size_t>(s)] == 0.f);
      CHECK(outD[c][static_cast<size_t>(s)] == 0.);
    }
  }
}

TEST_CASE("Processing: double precision buffers can be processed in place", "[integration][processing]")
{
  GainPlug plug;
  plug.HostActivate(48000., 64);
  plug.HostSetParameterNormalized(GainPlug::kGain, 1.); // 200%

  AudioBuffers<double> buffers {2, 64};
  FillWithTestSignal(buffers);
  plug.HostProcess(buffers.Get(), buffers.Get(), 64);

  for (int c = 0; c < 2; c++)
    for (int s = 0; s < 64; s++)
      CHECK(buffers[c][static_cast<size_t>(s)] == TestSignal<double>(c, s) * 2.);
}

TEST_CASE("Processing: block sizes", "[integration][processing]")
{
  GainPlug plug;
  plug.HostSetParameterNormalized(GainPlug::kGain, 0.25);

  SECTION("a range of block sizes, including 1, odd sizes and sizes above the default")
  {
    const int blockSizes[] = {1, 2, 3, 7, 63, 64, 65, 441, 1000, DEFAULT_BLOCK_SIZE, DEFAULT_BLOCK_SIZE + 1, 4096, 8191};

    for (int blockSize : blockSizes)
    {
      INFO("block size " << blockSize);
      plug.HostActivate(48000., blockSize);

      AudioBuffers<float> inF {2, blockSize}, outF {2, blockSize, 99.f};
      AudioBuffers<double> inD {2, blockSize}, outD {2, blockSize, 99.};
      FillWithTestSignal(inF);
      FillWithTestSignal(inD);

      plug.HostProcess(inF.Get(), outF.Get(), blockSize);
      CHECK(plug.mLastNFrames == blockSize);
      plug.HostProcess(inD.Get(), outD.Get(), blockSize);
      CHECK(plug.mLastNFrames == blockSize);

      bool allCorrect = true;

      for (int c = 0; c < 2; c++)
      {
        for (int s = 0; s < blockSize; s++)
        {
          const size_t i = static_cast<size_t>(s);
          allCorrect &= outF[c][i] == inF[c][i] * 0.5f;
          allCorrect &= outD[c][i] == inD[c][i] * 0.5;
        }
      }

      CHECK(allCorrect);
    }
  }

  SECTION("the host may process fewer frames than the maximum block size")
  {
    plug.HostActivate(48000., 512);

    const int frameCounts[] = {512, 1, 100, 511, 37, 512};

    for (int nFrames : frameCounts)
    {
      INFO("frames " << nFrames);

      AudioBuffers<float> in {2, nFrames}, out {2, nFrames, 99.f};
      FillWithTestSignal(in);
      plug.HostProcess(in.Get(), out.Get(), nFrames);

      CHECK(plug.mLastNFrames == nFrames);
      CHECK(plug.GetBlockSize() == 512);
      CHECK(out[0][0] == in[0][0] * 0.5f);
      CHECK(out[1][static_cast<size_t>(nFrames - 1)] == in[1][static_cast<size_t>(nFrames - 1)] * 0.5f);
    }
  }

  SECTION("zero frames")
  {
    plug.HostActivate(48000., 64);
    AudioBuffers<float> in {2, 64, 1.f}, out {2, 64, 99.f};
    plug.HostProcess(in.Get(), out.Get(), 0);
    CHECK(plug.mLastNFrames == 0);
    CHECK(out[0][0] == 99.f);
  }
}

TEST_CASE("Processing: a mono host in a stereo plug-in", "[integration][processing]")
{
  const int nFrames = 64;
  GainPlug plug;
  plug.HostActivate(48000., nFrames);

  SECTION("double precision")
  {
    plug.HostSetChannels(1, 1);
    REQUIRE(plug.NInChansConnected() == 1);
    REQUIRE(plug.NOutChansConnected() == 1);

    AudioBuffers<double> in {1, nFrames, 0.5}, out {1, nFrames, 99.};
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    // The plug-in still gets two valid input buffers, the unconnected one being silent, and can write to both outputs
    CHECK(plug.mInputPeaks[0] == 0.5);
    CHECK(plug.mInputPeaks[1] == 0.);

    for (int s = 0; s < nFrames; s++)
      CHECK(out[0][static_cast<size_t>(s)] == 0.5);
  }

  SECTION("single precision")
  {
    plug.HostSetChannels(1, 1);

    AudioBuffers<float> in {1, nFrames, 0.5f}, out {1, nFrames, 99.f};
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    CHECK(plug.mInputPeaks[0] == 0.5);
    CHECK(plug.mInputPeaks[1] == 0.);

    for (int s = 0; s < nFrames; s++)
      CHECK(out[0][static_cast<size_t>(s)] == 0.5f);
  }

  SECTION("an input which is disconnected after having been used is silent")
  {
    AudioBuffers<float> stereoIn {2, nFrames, 0.75f}, stereoOut {2, nFrames};
    plug.HostProcess(stereoIn.Get(), stereoOut.Get(), nFrames);
    CHECK(plug.mInputPeaks[1] == 0.75);

    plug.HostSetChannels(1, 2);

    AudioBuffers<float> monoIn {1, nFrames, 0.25f};
    plug.HostProcess(monoIn.Get(), stereoOut.Get(), nFrames);

    CHECK(plug.mInputPeaks[0] == 0.25);
    CHECK(plug.mInputPeaks[1] == 0.);
    CHECK(stereoOut[0][0] == 0.25f);
    CHECK(stereoOut[1][0] == 0.f);
  }

  SECTION("mono in, stereo out")
  {
    plug.HostSetChannels(1, 2);

    AudioBuffers<double> in {1, nFrames, 0.5}, out {2, nFrames, 99.};
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    for (int s = 0; s < nFrames; s++)
    {
      CHECK(out[0][static_cast<size_t>(s)] == 0.5);
      CHECK(out[1][static_cast<size_t>(s)] == 0.);
    }
  }

  SECTION("going back to stereo")
  {
    plug.HostSetChannels(1, 1);
    AudioBuffers<float> monoIn {1, nFrames, 0.5f}, monoOut {1, nFrames};
    plug.HostProcess(monoIn.Get(), monoOut.Get(), nFrames);

    plug.HostSetChannels(2, 2);
    AudioBuffers<float> in {2, nFrames}, out {2, nFrames};
    FillWithTestSignal(in);
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    for (int c = 0; c < 2; c++)
      for (int s = 0; s < nFrames; s++)
        CHECK(out[c][static_cast<size_t>(s)] == in[c][static_cast<size_t>(s)]);
  }
}

TEST_CASE("Processing: nothing connected", "[integration][processing]")
{
  GainPlug plug;
  plug.HostActivate(48000., 64);
  plug.HostSetChannels(0, 0);

  AudioBuffers<float> none {0, 64};
  plug.HostProcess(none.Get(), none.Get(), 64);

  CHECK(plug.mNumProcessCalls == 1);
  CHECK(plug.mInputPeaks[0] == 0.);
  CHECK(plug.mInputPeaks[1] == 0.);
}

TEST_CASE("Processing: sidechain input", "[integration][processing]")
{
  const int nFrames = 32;
  SidechainPlug plug;
  plug.HostActivate(48000., nFrames);

  REQUIRE(plug.MaxNChannels(ERoute::kInput) == 4);

  SECTION("sidechain connected")
  {
    AudioBuffers<float> in {4, nFrames}, out {2, nFrames};
    FillWithTestSignal(in);
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    for (int c = 0; c < 2; c++)
      for (int s = 0; s < nFrames; s++)
        CHECK(out[c][static_cast<size_t>(s)] == Approx(TestSignal<float>(c, s) + TestSignal<float>(c + 2, s)));
  }

  SECTION("sidechain not connected")
  {
    plug.HostSetChannels(2, 2);

    AudioBuffers<float> in {2, nFrames}, out {2, nFrames};
    FillWithTestSignal(in);
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    for (int c = 0; c < 2; c++)
      for (int s = 0; s < nFrames; s++)
        CHECK(out[c][static_cast<size_t>(s)] == TestSignal<float>(c, s));
  }

  SECTION("sidechain disconnected while running")
  {
    AudioBuffers<double> in4 {4, nFrames, 0.25}, out {2, nFrames};
    plug.HostProcess(in4.Get(), out.Get(), nFrames);
    CHECK(out[0][0] == 0.5);

    plug.HostSetChannels(2, 2);
    AudioBuffers<double> in2 {2, nFrames, 0.25};
    plug.HostProcess(in2.Get(), out.Get(), nFrames);
    CHECK(out[0][0] == 0.25);
    CHECK(out[1][static_cast<size_t>(nFrames - 1)] == 0.25);
  }
}

TEST_CASE("Processing: the default ProcessBlock() passes audio through", "[integration][processing]")
{
  const int nFrames = 48;

  SECTION("stereo")
  {
    DefaultProcessPlug plug {"2-2"};
    plug.HostActivate(48000., nFrames);

    AudioBuffers<float> in {2, nFrames}, out {2, nFrames, 99.f};
    FillWithTestSignal(in);
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    for (int c = 0; c < 2; c++)
      for (int s = 0; s < nFrames; s++)
        CHECK(out[c][static_cast<size_t>(s)] == in[c][static_cast<size_t>(s)]);
  }

  SECTION("more outputs than inputs: the extra outputs are silent")
  {
    DefaultProcessPlug plug {"1-2"};
    plug.HostActivate(48000., nFrames);

    AudioBuffers<double> in {1, nFrames}, out {2, nFrames, 99.};
    FillWithTestSignal(in);
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    for (int s = 0; s < nFrames; s++)
    {
      CHECK(out[0][static_cast<size_t>(s)] == in[0][static_cast<size_t>(s)]);
      CHECK(out[1][static_cast<size_t>(s)] == 0.);
    }
  }

  SECTION("more inputs than outputs: the extra inputs are dropped")
  {
    DefaultProcessPlug plug {"2-1"};
    plug.HostActivate(48000., nFrames);

    AudioBuffers<double> in {2, nFrames}, out {1, nFrames, 99.};
    FillWithTestSignal(in);
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    for (int s = 0; s < nFrames; s++)
      CHECK(out[0][static_cast<size_t>(s)] == in[0][static_cast<size_t>(s)]);
  }

  SECTION("no inputs: silence")
  {
    DefaultProcessPlug plug {"0-2"};
    plug.HostActivate(48000., nFrames);

    AudioBuffers<float> in {0, nFrames}, out {2, nFrames, 99.f};
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    for (int c = 0; c < 2; c++)
      for (int s = 0; s < nFrames; s++)
        CHECK(out[c][static_cast<size_t>(s)] == 0.f);
  }
}

TEST_CASE("Processing: bypass passes audio through without calling ProcessBlock()", "[integration][processing][bypass]")
{
  const int nFrames = 64;
  GainPlug plug;
  plug.HostActivate(48000., nFrames);
  plug.HostSetParameterNormalized(GainPlug::kGain, 0.); // would silence the output if processed

  CHECK_FALSE(plug.GetBypassed());
  plug.HostSetBypassed(true);
  CHECK(plug.GetBypassed());

  SECTION("single precision")
  {
    AudioBuffers<float> in {2, nFrames}, out {2, nFrames, 99.f};
    FillWithTestSignal(in);
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    for (int c = 0; c < 2; c++)
      for (int s = 0; s < nFrames; s++)
        CHECK(out[c][static_cast<size_t>(s)] == in[c][static_cast<size_t>(s)]);
  }

  SECTION("double precision")
  {
    AudioBuffers<double> in {2, nFrames}, out {2, nFrames, 99.};
    FillWithTestSignal(in);
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    for (int c = 0; c < 2; c++)
      for (int s = 0; s < nFrames; s++)
        CHECK(out[c][static_cast<size_t>(s)] == in[c][static_cast<size_t>(s)]);
  }

  CHECK(plug.mNumProcessCalls == 0);

  SECTION("un-bypassing processes again")
  {
    plug.HostSetBypassed(false);
    CHECK_FALSE(plug.GetBypassed());

    AudioBuffers<float> in {2, nFrames, 1.f}, out {2, nFrames, 99.f};
    plug.HostProcess(in.Get(), out.Get(), nFrames);

    CHECK(plug.mNumProcessCalls == 1);
    CHECK(out[0][0] == 0.f);
  }
}

TEST_CASE("Processing: bypass with more outputs than inputs silences the extra outputs", "[integration][processing][bypass]")
{
  const int nFrames = 32;
  GainPlug plug {InstanceInfo{}, GainPlug::MakeConfig("1-2")};
  plug.HostActivate(48000., nFrames);
  plug.HostSetBypassed(true);

  AudioBuffers<double> in {1, nFrames}, out {2, nFrames, 99.};
  FillWithTestSignal(in);
  plug.HostProcess(in.Get(), out.Get(), nFrames);

  for (int s = 0; s < nFrames; s++)
  {
    CHECK(out[0][static_cast<size_t>(s)] == in[0][static_cast<size_t>(s)]);
    CHECK(out[1][static_cast<size_t>(s)] == 0.);
  }
}

TEST_CASE("Processing: bypass delays the dry signal by the reported latency", "[integration][processing][bypass][latency]")
{
  const int latency = 5;
  const int blockSize = 16;
  const int nBlocks = 4;

  GainPlug plug {InstanceInfo{}, GainPlug::MakeConfig("2-2", latency)};
  REQUIRE(plug.GetLatency() == latency);

  plug.HostActivate(48000., blockSize);
  plug.HostSetBypassed(true);

  SECTION("single precision")
  {
    for (int c = 0; c < 2; c++)
    {
      INFO("channel " << c);
      GainPlug fresh {InstanceInfo{}, GainPlug::MakeConfig("2-2", latency)};
      fresh.HostActivate(48000., blockSize);
      fresh.HostSetBypassed(true);

      const std::vector<float> out = ProcessBypassed<float>(fresh, c, 2, blockSize, nBlocks);
      REQUIRE(out.size() == static_cast<size_t>(blockSize * nBlocks));

      for (int s = 0; s < blockSize * nBlocks; s++)
      {
        INFO("frame " << s);
        CHECK(out[static_cast<size_t>(s)] == (s < latency ? 0.f : TestSignal<float>(c, s - latency)));
      }
    }
  }

  SECTION("double precision")
  {
    const std::vector<double> out = ProcessBypassed<double>(plug, 1, 2, blockSize, nBlocks);

    for (int s = 0; s < blockSize * nBlocks; s++)
    {
      INFO("frame " << s);
      CHECK(out[static_cast<size_t>(s)] == (s < latency ? 0. : TestSignal<double>(1, s - latency)));
    }
  }

  SECTION("latency longer than the block size")
  {
    const int longLatency = 40;
    GainPlug longPlug {InstanceInfo{}, GainPlug::MakeConfig("2-2", longLatency)};
    longPlug.HostActivate(48000., blockSize);
    longPlug.HostSetBypassed(true);

    const std::vector<double> out = ProcessBypassed<double>(longPlug, 0, 2, blockSize, 6);

    for (int s = 0; s < blockSize * 6; s++)
    {
      INFO("frame " << s);
      CHECK(out[static_cast<size_t>(s)] == (s < longLatency ? 0. : TestSignal<double>(0, s - longLatency)));
    }
  }

  CHECK(plug.mNumProcessCalls == 0);
}

TEST_CASE("Processing: latency can be changed at runtime", "[integration][processing][latency]")
{
  const int blockSize = 32;
  GainPlug plug;
  plug.HostActivate(48000., blockSize);

  CHECK(plug.GetLatency() == 0);
  CHECK(plug.mNumLatencyChanges == 0);

  plug.SetLatency(7);
  CHECK(plug.GetLatency() == 7);
  CHECK(plug.mNumLatencyChanges == 1); // the host was told

  SECTION("the bypass delay follows the new latency")
  {
    plug.HostSetBypassed(true);
    const std::vector<double> out = ProcessBypassed<double>(plug, 0, 2, blockSize, 2);

    for (int s = 0; s < blockSize * 2; s++)
    {
      INFO("frame " << s);
      CHECK(out[static_cast<size_t>(s)] == (s < 7 ? 0. : TestSignal<double>(0, s - 7)));
    }
  }

  SECTION("back to zero latency: bypass is a straight wire again")
  {
    plug.SetLatency(0);
    CHECK(plug.GetLatency() == 0);
    CHECK(plug.mNumLatencyChanges == 2);

    plug.HostSetBypassed(true);
    const std::vector<float> out = ProcessBypassed<float>(plug, 1, 2, blockSize, 2);

    for (int s = 0; s < blockSize * 2; s++)
      CHECK(out[static_cast<size_t>(s)] == TestSignal<float>(1, s));
  }

  SECTION("changing latency does not affect normal processing")
  {
    AudioBuffers<float> in {2, blockSize}, out {2, blockSize};
    FillWithTestSignal(in);
    plug.HostProcess(in.Get(), out.Get(), blockSize);

    for (int s = 0; s < blockSize; s++)
      CHECK(out[0][static_cast<size_t>(s)] == in[0][static_cast<size_t>(s)]);
  }
}

// BUG: With non-zero latency, IPlugProcessor::PassThroughBuffers() (IPlug/IPlugProcessor.cpp) hands the bypassed
// signal to NChanDelayLine::ProcessBlock() (IPlug/Extras/NChanDelay.h), which only writes the outputs that have a
// matching input. Outputs beyond the input count are left untouched, i.e. the host gets whatever was in its buffer,
// whereas the zero latency path (IPlugProcessor::ProcessBlock()) silences them.
TEST_CASE("Processing: bypass with latency silences outputs which have no matching input", "[.][bug]")
{
  const int nFrames = 32;
  GainPlug plug {InstanceInfo{}, GainPlug::MakeConfig("1-2", 4)};
  plug.HostActivate(48000., nFrames);
  plug.HostSetBypassed(true);

  AudioBuffers<double> in {1, nFrames, 0.5}, out {2, nFrames, 99.};
  plug.HostProcess(in.Get(), out.Get(), nFrames);

  CHECK(out[0][static_cast<size_t>(nFrames - 1)] == 0.5);

  for (int s = 0; s < nFrames; s++)
    CHECK(out[1][static_cast<size_t>(s)] == 0.);
}

TEST_CASE("Processing: tail size", "[integration][processing]")
{
  GainPlug plug;

  CHECK(plug.GetTailSize() == IPlugProcessor::kTailNone);
  CHECK_FALSE(plug.GetTailIsInfinite());

  plug.SetTailSize(4800);
  CHECK(plug.GetTailSize() == 4800);
  CHECK_FALSE(plug.GetTailIsInfinite());
  CHECK(plug.mNumTailSizeChanges == 1);

  plug.SetTailSize(IPlugProcessor::kTailInfinite);
  CHECK(plug.GetTailIsInfinite());

  plug.SetTailSize(IPlugProcessor::kTailNone);
  CHECK(plug.GetTailSize() == 0);
  CHECK_FALSE(plug.GetTailIsInfinite());
  CHECK(plug.mNumTailSizeChanges == 3);
}

TEST_CASE("Processing: offline rendering flag", "[integration][processing]")
{
  GainPlug plug;
  plug.HostActivate(48000., 64);

  CHECK_FALSE(plug.GetRenderingOffline());
  plug.HostSetRenderingOffline(true);
  CHECK(plug.GetRenderingOffline());

  // Processing is unaffected
  AudioBuffers<float> in {2, 64, 0.5f}, out {2, 64};
  plug.HostProcess(in.Get(), out.Get(), 64);
  CHECK(out[0][0] == 0.5f);
  CHECK(plug.GetRenderingOffline());

  plug.HostSetRenderingOffline(false);
  CHECK_FALSE(plug.GetRenderingOffline());
}

TEST_CASE("Processing: transport and time information", "[integration][processing][transport]")
{
  GainPlug plug;
  plug.HostActivate(48000., 64);

  SECTION("defaults before the host supplies anything")
  {
    CHECK(plug.GetTempo() == DEFAULT_TEMPO);
    CHECK(plug.GetSamplePos() == -1.);
    CHECK(plug.GetPPQPos() == -1.);
    CHECK_FALSE(plug.GetTransportIsRunning());

    int numerator = 0, denominator = 0;
    plug.GetTimeSig(numerator, denominator);
    CHECK(numerator == 4);
    CHECK(denominator == 4);

    CHECK(plug.GetSamplesPerBeat() == Approx(48000. * 60. / 120.));
  }

  SECTION("values supplied by the host")
  {
    ITimeInfo timeInfo;
    timeInfo.mTempo = 90.;
    timeInfo.mSamplePos = 123456.;
    timeInfo.mPPQPos = 17.25;
    timeInfo.mNumerator = 7;
    timeInfo.mDenominator = 8;
    timeInfo.mTransportIsRunning = true;
    plug.HostSetTimeInfo(timeInfo);

    CHECK(plug.GetTempo() == 90.);
    CHECK(plug.GetSamplePos() == 123456.);
    CHECK(plug.GetPPQPos() == 17.25);
    CHECK(plug.GetTransportIsRunning());

    int numerator = 0, denominator = 0;
    plug.GetTimeSig(numerator, denominator);
    CHECK(numerator == 7);
    CHECK(denominator == 8);

    CHECK(plug.GetSamplesPerBeat() == Approx(32000.));
  }

  SECTION("samples per beat follows the sample rate")
  {
    ITimeInfo timeInfo;
    timeInfo.mTempo = 60.;
    plug.HostSetTimeInfo(timeInfo);
    CHECK(plug.GetSamplesPerBeat() == Approx(48000.));

    plug.HostActivate(44100., 64);
    CHECK(plug.GetSamplesPerBeat() == Approx(44100.));

    timeInfo.mTempo = 120.;
    plug.HostSetTimeInfo(timeInfo);
    CHECK(plug.GetSamplesPerBeat() == Approx(22050.));
  }

  SECTION("no tempo: zero samples per beat rather than a division by zero")
  {
    ITimeInfo timeInfo;
    timeInfo.mTempo = 0.;
    plug.HostSetTimeInfo(timeInfo);
    CHECK(plug.GetSamplesPerBeat() == 0.);

    timeInfo.mTempo = -10.;
    plug.HostSetTimeInfo(timeInfo);
    CHECK(plug.GetSamplesPerBeat() == 0.);
  }

  SECTION("the transport can stop again")
  {
    ITimeInfo timeInfo;
    timeInfo.mTransportIsRunning = true;
    plug.HostSetTimeInfo(timeInfo);
    CHECK(plug.GetTransportIsRunning());

    timeInfo.mTransportIsRunning = false;
    plug.HostSetTimeInfo(timeInfo);
    CHECK_FALSE(plug.GetTransportIsRunning());
  }
}
