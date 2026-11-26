/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for NChanDelayLine (IPlug/Extras/NChanDelay.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include "heapbuf.h"
#include "NChanDelay.h"
#include "ExtrasTestUtils.h"

using namespace iplug;

namespace
{
/** Push nFrames through the delay line in blocks of blockSize, starting at frame "start" */
template <typename T>
void ProcessInBlocks(NChanDelayLine<T>& delay, extrastest::Buffers<T>& in, extrastest::Buffers<T>& out, int nChans, int start, int nFrames, int blockSize)
{
  std::vector<T*> ins(static_cast<size_t>(nChans)), outs(static_cast<size_t>(nChans));

  for (int pos = start; pos < start + nFrames; pos += blockSize)
  {
    for (int c = 0; c < nChans; c++)
    {
      ins[static_cast<size_t>(c)] = in[c].data() + pos;
      outs[static_cast<size_t>(c)] = out[c].data() + pos;
    }

    delay.ProcessBlock(ins.data(), outs.data(), std::min(blockSize, start + nFrames - pos));
  }
}

/** @return true if out is exactly in delayed by delaySamples, with silence before that */
template <typename T>
bool IsDelayedCopy(const std::vector<T>& in, const std::vector<T>& out, int delaySamples, int start = 0)
{
  for (int s = start; s < static_cast<int>(out.size()); s++)
  {
    const T expected = (s - start) < delaySamples ? T(0) : in[static_cast<size_t>(s - delaySamples)];

    if (out[static_cast<size_t>(s)] != expected)
      return false;
  }

  return true;
}
} // namespace

TEST_CASE("NChanDelayLine: delays every channel by exactly the delay time", "[unit][extras][nchandelay]")
{
  const int delaySamples = GENERATE(1, 2, 7, 64, 1000);
  const int blockSize = GENERATE(1, 64, 333);
  constexpr int nFrames = 4000;

  NChanDelayLine<double> delay(2, 2);
  delay.SetDelayTime(delaySamples);

  extrastest::Buffers<double> in(2, nFrames), out(2, nFrames, 99.);
  in[0] = extrastest::MakeNoise<double>(nFrames, 1u);
  in[1] = extrastest::MakeNoise<double>(nFrames, 2u);

  ProcessInBlocks(delay, in, out, 2, 0, nFrames, blockSize);

  CAPTURE(delaySamples, blockSize);
  REQUIRE(IsDelayedCopy(in[0], out[0], delaySamples));
  REQUIRE(IsDelayedCopy(in[1], out[1], delaySamples));
}

TEST_CASE("NChanDelayLine: an impulse comes out once, at the delay time", "[unit][extras][nchandelay]")
{
  const int delaySamples = GENERATE(1, 16, 511);
  constexpr int nFrames = 2048;

  NChanDelayLine<double> delay(1, 1);
  delay.SetDelayTime(delaySamples);

  extrastest::Buffers<double> in(1, nFrames), out(1, nFrames);
  in[0][0] = 1.;

  ProcessInBlocks(delay, in, out, 1, 0, nFrames, 128);

  for (int s = 0; s < nFrames; s++)
    REQUIRE(out[0][static_cast<size_t>(s)] == (s == delaySamples ? 1. : 0.));
}

TEST_CASE("NChanDelayLine: supports more than two channels", "[unit][extras][nchandelay]")
{
  constexpr int nChans = 6;
  constexpr int nFrames = 1024;
  constexpr int delaySamples = 100;

  NChanDelayLine<double> delay(nChans, nChans);
  delay.SetDelayTime(delaySamples);

  extrastest::Buffers<double> in(nChans, nFrames), out(nChans, nFrames);
  for (int c = 0; c < nChans; c++)
    in[c] = extrastest::MakeNoise<double>(nFrames, static_cast<uint32_t>(100 + c));

  ProcessInBlocks(delay, in, out, nChans, 0, nFrames, 64);

  for (int c = 0; c < nChans; c++)
  {
    CAPTURE(c);
    REQUIRE(IsDelayedCopy(in[c], out[c], delaySamples));
  }
}

TEST_CASE("NChanDelayLine: SetDelayTime changes the delay and clears the line", "[unit][extras][nchandelay]")
{
  constexpr int nFrames = 2000;

  NChanDelayLine<double> delay(2, 2);
  delay.SetDelayTime(50);

  extrastest::Buffers<double> in(2, nFrames), out(2, nFrames);
  in[0] = extrastest::MakeNoise<double>(nFrames, 3u);
  in[1] = extrastest::MakeNoise<double>(nFrames, 4u);

  ProcessInBlocks(delay, in, out, 2, 0, 1000, 64);

  const int newDelay = GENERATE(10, 50, 300);
  delay.SetDelayTime(newDelay);
  ProcessInBlocks(delay, in, out, 2, 1000, 1000, 64);

  CAPTURE(newDelay);

  // nothing from before the change leaks out; the new delay applies from the change onwards
  for (int c = 0; c < 2; c++)
  {
    for (int s = 1000; s < nFrames; s++)
    {
      const double expected = (s - 1000) < newDelay ? 0. : in[c][static_cast<size_t>(s - newDelay)];
      REQUIRE(out[c][static_cast<size_t>(s)] == expected);
    }
  }
}

TEST_CASE("NChanDelayLine: ClearBuffer discards the delayed signal", "[unit][extras][nchandelay]")
{
  constexpr int nFrames = 1024;
  constexpr int delaySamples = 200;

  NChanDelayLine<double> delay(2, 2);
  delay.SetDelayTime(delaySamples);

  extrastest::Buffers<double> in(2, nFrames, 1.), out(2, nFrames);
  ProcessInBlocks(delay, in, out, 2, 0, 512, 64);
  REQUIRE(out[0][511] == 1.);

  delay.ClearBuffer();
  ProcessInBlocks(delay, in, out, 2, 512, 512, 64);

  for (int c = 0; c < 2; c++)
  {
    for (int s = 512; s < nFrames; s++)
      REQUIRE(out[c][static_cast<size_t>(s)] == ((s - 512) < delaySamples ? 0. : 1.));
  }
}

TEST_CASE("NChanDelayLine: ClearBuffer before a delay time has been set is harmless", "[unit][extras][nchandelay]")
{
  NChanDelayLine<double> delay(2, 2);
  delay.ClearBuffer();

  delay.SetDelayTime(4);
  extrastest::Buffers<double> in(2, 16, 1.), out(2, 16);
  ProcessInBlocks(delay, in, out, 2, 0, 16, 16);
  REQUIRE(out[0][3] == 0.);
  REQUIRE(out[0][4] == 1.);
}

TEST_CASE("NChanDelayLine: works in place", "[unit][extras][nchandelay]")
{
  constexpr int nFrames = 1024;
  constexpr int delaySamples = 37;

  NChanDelayLine<double> delay(2, 2);
  delay.SetDelayTime(delaySamples);

  extrastest::Buffers<double> orig(2, nFrames), buf(2, nFrames);
  orig[0] = extrastest::MakeNoise<double>(nFrames, 5u);
  orig[1] = extrastest::MakeNoise<double>(nFrames, 6u);
  buf[0] = orig[0];
  buf[1] = orig[1];

  ProcessInBlocks(delay, buf, buf, 2, 0, nFrames, 100);

  REQUIRE(IsDelayedCopy(orig[0], buf[0], delaySamples));
  REQUIRE(IsDelayedCopy(orig[1], buf[1], delaySamples));
}

TEST_CASE("NChanDelayLine: only channels present on both sides are processed", "[unit][extras][nchandelay]")
{
  constexpr int nFrames = 256;
  constexpr int delaySamples = 10;
  constexpr double kSentinel = 99.;

  SECTION("more inputs than outputs")
  {
    NChanDelayLine<double> delay(2, 1);
    delay.SetDelayTime(delaySamples);

    extrastest::Buffers<double> in(2, nFrames), out(2, nFrames, kSentinel);
    in[0] = extrastest::MakeNoise<double>(nFrames, 7u);
    in[1] = extrastest::MakeNoise<double>(nFrames, 8u);

    ProcessInBlocks(delay, in, out, 2, 0, nFrames, 64);

    REQUIRE(IsDelayedCopy(in[0], out[0], delaySamples));
    for (size_t s = 0; s < nFrames; s++)
      REQUIRE(out[1][s] == kSentinel);
  }

  SECTION("more outputs than inputs")
  {
    NChanDelayLine<double> delay(1, 2);
    delay.SetDelayTime(delaySamples);

    extrastest::Buffers<double> in(2, nFrames), out(2, nFrames, kSentinel);
    in[0] = extrastest::MakeNoise<double>(nFrames, 9u);

    ProcessInBlocks(delay, in, out, 2, 0, nFrames, 64);

    REQUIRE(IsDelayedCopy(in[0], out[0], delaySamples));
    for (size_t s = 0; s < nFrames; s++)
      REQUIRE(out[1][s] == kSentinel);
  }
}

TEST_CASE("NChanDelayLine: single precision", "[unit][extras][nchandelay]")
{
  constexpr int nFrames = 1024;
  constexpr int delaySamples = 128;

  NChanDelayLine<float> delay; // stereo by default
  delay.SetDelayTime(delaySamples);

  extrastest::Buffers<float> in(2, nFrames), out(2, nFrames);
  in[0] = extrastest::MakeNoise<float>(nFrames, 10u);
  in[1] = extrastest::MakeNoise<float>(nFrames, 11u);

  ProcessInBlocks(delay, in, out, 2, 0, nFrames, 64);

  REQUIRE(IsDelayedCopy(in[0], out[0], delaySamples));
  REQUIRE(IsDelayedCopy(in[1], out[1], delaySamples));
}

// BUG: IPlug/Extras/NChanDelay.h:58 does "mWriteAddress %= mDTSamples", which is an integer
// division by zero (undefined behaviour, SIGFPE on x86) when the delay time is zero. That is the
// case after SetDelayTime(0) - which a plug-in reporting zero latency would naturally call - and
// also when ProcessBlock() is called before SetDelayTime(). The buffer is also zero sized then, so
// lines 52-53 read and write out of bounds. A zero delay should pass the signal straight through.
// NOTE: this test crashes rather than fails with the current implementation.
TEST_CASE("NChanDelayLine: zero delay passes the signal through", "[.][bug]")
{
  constexpr int nFrames = 256;

  NChanDelayLine<double> delay(2, 2);
  delay.SetDelayTime(0);

  extrastest::Buffers<double> in(2, nFrames), out(2, nFrames);
  in[0] = extrastest::MakeNoise<double>(nFrames, 12u);
  in[1] = extrastest::MakeNoise<double>(nFrames, 13u);

  ProcessInBlocks(delay, in, out, 2, 0, nFrames, 64);

  REQUIRE(IsDelayedCopy(in[0], out[0], 0));
  REQUIRE(IsDelayedCopy(in[1], out[1], 0));
}
