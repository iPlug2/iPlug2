/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for ControlRamp and ControlRampProcessor (IPlug/Extras/Synth/ControlRamp.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "IPlugPlatform.h"
#include "ControlRamp.h"

using namespace iplug;
using Catch::Approx;

namespace
{
constexpr float kSentinel = -99.f;

ControlRamp MakeRamp(double startValue, double endValue, int transitionStart, int transitionEnd)
{
  ControlRamp ramp;
  ramp.startValue = startValue;
  ramp.endValue = endValue;
  ramp.transitionStart = transitionStart;
  ramp.transitionEnd = transitionEnd;
  return ramp;
}

/** Run a processor for nBlocks blocks, writing its ramp out as a signal */
std::vector<float> Render(ControlRampProcessor& processor, ControlRamp& ramp, int blockSize, int nBlocks)
{
  std::vector<float> signal(static_cast<size_t>(blockSize * nBlocks), kSentinel);

  for (int b = 0; b < nBlocks; b++)
  {
    processor.Process(blockSize);
    ramp.Write(signal.data(), b * blockSize, blockSize);
  }

  return signal;
}
} // namespace

TEST_CASE("ControlRamp: Clear and IsNonzero", "[unit][extras][synth][controlramp]")
{
  ControlRamp ramp = MakeRamp(0.5, 1., 3, 7);
  REQUIRE(ramp.IsNonzero());

  ramp.Clear();
  REQUIRE_FALSE(ramp.IsNonzero());
  REQUIRE(ramp.startValue == 0.);
  REQUIRE(ramp.endValue == 0.);
  REQUIRE(ramp.transitionStart == 0);
  REQUIRE(ramp.transitionEnd == 0);

  REQUIRE(MakeRamp(0., 1., 0, 1).IsNonzero());
  REQUIRE(MakeRamp(1., 0., 0, 1).IsNonzero());
  REQUIRE(MakeRamp(-1., -1., 0, 1).IsNonzero());
  REQUIRE_FALSE(MakeRamp(0., 0., 0, 32).IsNonzero());
}

TEST_CASE("ControlRamp: Write produces the three pieces of the ramp", "[unit][extras][synth][controlramp]")
{
  constexpr int nFrames = 32;
  ControlRamp ramp = MakeRamp(0.25, 0.75, 10, 20);

  std::vector<float> buffer(nFrames, kSentinel);
  ramp.Write(buffer.data(), 0, nFrames);

  // start value up to the transition
  for (size_t i = 0; i < 10; i++)
    REQUIRE(buffer[i] == Approx(0.25f));

  // a linear transition, arriving at the end value on its last sample
  for (size_t i = 10; i < 20; i++)
    REQUIRE(buffer[i] == Approx(0.25f + 0.5f * static_cast<float>(i - 9) / 10.f).margin(1e-5));

  // end value for the rest of the block
  for (size_t i = 20; i < nFrames; i++)
    REQUIRE(buffer[i] == Approx(0.75f).margin(1e-5));
}

TEST_CASE("ControlRamp: Write handles ramps in either direction and across the whole block", "[unit][extras][synth][controlramp]")
{
  constexpr int nFrames = 64;

  SECTION("downwards")
  {
    ControlRamp ramp = MakeRamp(1., -1., 0, nFrames);
    std::vector<float> buffer(nFrames, kSentinel);
    ramp.Write(buffer.data(), 0, nFrames);

    for (size_t i = 1; i < nFrames; i++)
      REQUIRE(buffer[i] < buffer[i - 1]);

    REQUIRE(buffer[0] == Approx(1.f - 2.f / nFrames).margin(1e-5));
    REQUIRE(buffer[nFrames - 1] == Approx(-1.f).margin(1e-4));
  }

  SECTION("a one sample step")
  {
    ControlRamp ramp = MakeRamp(0., 1., 5, 6);
    std::vector<float> buffer(nFrames, kSentinel);
    ramp.Write(buffer.data(), 0, nFrames);

    for (size_t i = 0; i < nFrames; i++)
      REQUIRE(buffer[i] == (i < 5 ? 0.f : 1.f));
  }

  SECTION("a constant")
  {
    ControlRamp ramp = MakeRamp(0.3, 0.3, 0, nFrames);
    std::vector<float> buffer(nFrames, kSentinel);
    ramp.Write(buffer.data(), 0, nFrames);

    for (size_t i = 0; i < nFrames; i++)
      REQUIRE(buffer[i] == Approx(0.3f));
  }
}

TEST_CASE("ControlRamp: Write only touches the requested part of the buffer", "[unit][extras][synth][controlramp]")
{
  constexpr int startIdx = 16;
  constexpr int nFrames = 32;
  ControlRamp ramp = MakeRamp(0., 1., 4, 12);

  std::vector<float> buffer(64, kSentinel);
  ramp.Write(buffer.data(), startIdx, nFrames);

  for (size_t i = 0; i < startIdx; i++)
    REQUIRE(buffer[i] == kSentinel);

  for (size_t i = startIdx + nFrames; i < buffer.size(); i++)
    REQUIRE(buffer[i] == kSentinel);

  // transition positions are relative to the start index
  REQUIRE(buffer[startIdx] == 0.f);
  REQUIRE(buffer[startIdx + 3] == 0.f);
  REQUIRE(buffer[startIdx + 4] == Approx(0.125f));
  REQUIRE(buffer[startIdx + 11] == Approx(1.f).margin(1e-5));
  REQUIRE(buffer[startIdx + nFrames - 1] == Approx(1.f).margin(1e-5));
}

TEST_CASE("ControlRamp: Write with an empty transition steps to the end value", "[unit][extras][synth][controlramp]")
{
  ControlRamp ramp = MakeRamp(0., 1., 4, 4);

  std::vector<float> buffer(16, kSentinel);
  ramp.Write(buffer.data(), 0, 16);

  for (size_t i = 0; i < 16; i++)
    REQUIRE(buffer[i] == (i < 4 ? 0.f : 1.f));
}

TEST_CASE("ControlRampProcessor: a glide within one block", "[unit][extras][synth][controlramp]")
{
  constexpr int blockSize = 32;
  ControlRamp ramp;
  ramp.Clear();
  ControlRampProcessor processor(ramp);

  processor.SetTarget(1., 5, 10, blockSize);

  // nothing changes until the block is processed
  REQUIRE(ramp.endValue == 0.);

  processor.Process(blockSize);
  REQUIRE(ramp.startValue == 0.);
  REQUIRE(ramp.endValue == 1.);
  REQUIRE(ramp.transitionStart == 5);
  REQUIRE(ramp.transitionEnd == 15);

  // the next block holds the target
  processor.Process(blockSize);
  REQUIRE(ramp.startValue == 1.);
  REQUIRE(ramp.endValue == 1.);

  std::vector<float> buffer(blockSize, kSentinel);
  ramp.Write(buffer.data(), 0, blockSize);
  for (float v : buffer)
    REQUIRE(v == 1.f);
}

TEST_CASE("ControlRampProcessor: without a target the ramp holds its value", "[unit][extras][synth][controlramp]")
{
  ControlRamp ramp = MakeRamp(0.2, 0.7, 0, 16);
  ControlRampProcessor processor(ramp);

  // each block starts where the last one ended
  processor.Process(32);
  REQUIRE(ramp.startValue == 0.7);
  REQUIRE(ramp.endValue == 0.7);

  processor.Process(32);
  REQUIRE(ramp.startValue == 0.7);
  REQUIRE(ramp.endValue == 0.7);
}

TEST_CASE("ControlRampProcessor: an immediate change is a one sample step at the offset", "[unit][extras][synth][controlramp]")
{
  const int glideSamples = GENERATE(-5, 0, 1); // anything less than one sample is one sample
  const int offset = GENERATE(0, 13, 31);
  constexpr int blockSize = 32;

  ControlRamp ramp;
  ramp.Clear();
  ControlRampProcessor processor(ramp);

  processor.SetTarget(0.8, offset, glideSamples, blockSize);
  const auto signal = Render(processor, ramp, blockSize, 2);

  CAPTURE(glideSamples, offset);
  for (int i = 0; i < 2 * blockSize; i++)
    REQUIRE(signal[static_cast<size_t>(i)] == Approx(i < offset ? 0.f : 0.8f));
}

TEST_CASE("ControlRampProcessor: a glide spanning several blocks is one continuous line", "[unit][extras][synth][controlramp]")
{
  const int blockSize = GENERATE(16, 32, 64);
  const int offset = GENERATE(0, 8);
  const int glideSamples = GENERATE(100, 441, 1000);
  const double target = GENERATE(1., -0.5);

  ControlRamp ramp;
  ramp.Clear();
  ControlRampProcessor processor(ramp);

  processor.SetTarget(target, offset, glideSamples, blockSize);

  const int nBlocks = (offset + glideSamples) / blockSize + 3;
  std::vector<float> signal(static_cast<size_t>(blockSize * nBlocks), kSentinel);
  bool continuous = true;
  double prevEnd = 0.;

  for (int b = 0; b < nBlocks; b++)
  {
    processor.Process(blockSize);

    if (ramp.startValue != prevEnd)
      continuous = false;
    prevEnd = ramp.endValue;

    REQUIRE(ramp.transitionStart >= 0);
    REQUIRE(ramp.transitionEnd <= blockSize);
    ramp.Write(signal.data(), b * blockSize, blockSize);
  }

  CAPTURE(blockSize, offset, glideSamples, target);
  REQUIRE(continuous);
  REQUIRE(ramp.endValue == target);

  double maxError = 0.;
  for (int i = 0; i < blockSize * nBlocks; i++)
  {
    const double progress = std::max(0., std::min(1., static_cast<double>(i + 1 - offset) / static_cast<double>(glideSamples)));
    maxError = std::max(maxError, std::fabs(static_cast<double>(signal[static_cast<size_t>(i)]) - target * progress));
  }
  REQUIRE(maxError < 1e-4);
}

TEST_CASE("ControlRampProcessor: a new target starts from the current value", "[unit][extras][synth][controlramp]")
{
  constexpr int blockSize = 32;
  ControlRamp ramp;
  ramp.Clear();
  ControlRampProcessor processor(ramp);

  // half way through a 64 sample glide to 1...
  processor.SetTarget(1., 0, 64, blockSize);
  processor.Process(blockSize);
  REQUIRE(ramp.endValue == Approx(0.5));

  // ...turn round and go to -0.5 in 32 samples
  processor.SetTarget(-0.5, 0, 32, blockSize);
  processor.Process(blockSize);
  REQUIRE(ramp.startValue == Approx(0.5));
  REQUIRE(ramp.endValue == Approx(-0.5));
  REQUIRE(ramp.transitionStart == 0);
  REQUIRE(ramp.transitionEnd == 32);

  processor.Process(blockSize);
  REQUIRE(ramp.startValue == Approx(-0.5));
  REQUIRE(ramp.endValue == Approx(-0.5));
}

TEST_CASE("ControlRampProcessor: Create makes one processor per ramp", "[unit][extras][synth][controlramp]")
{
  ControlRamp::RampArray<4> ramps;
  for (auto& ramp : ramps)
    ramp.Clear();

  std::unique_ptr<ControlRampProcessor::ProcessorArray<4>> processors(ControlRampProcessor::Create(ramps));
  REQUIRE(processors != nullptr);
  REQUIRE(processors->size() == 4);

  // each processor drives its own ramp
  for (size_t i = 0; i < 4; i++)
    processors->at(i).SetTarget(static_cast<double>(i + 1), 0, 1, 32);

  for (auto& processor : *processors)
    processor.Process(32);

  for (size_t i = 0; i < 4; i++)
    REQUIRE(ramps[i].endValue == static_cast<double>(i + 1));

  // and only its own
  processors->at(2).SetTarget(-7., 0, 1, 32);
  for (auto& processor : *processors)
    processor.Process(32);

  REQUIRE(ramps[0].endValue == 1.);
  REQUIRE(ramps[1].endValue == 2.);
  REQUIRE(ramps[2].endValue == -7.);
  REQUIRE(ramps[3].endValue == 4.);
}

// A glide no longer than a block, but which starts too late in it to finish there
TEST_CASE("ControlRampProcessor: a glide starting late in a block carries over into the next", "[unit][extras][synth][controlramp]")
{
  constexpr int blockSize = 32;
  ControlRamp ramp;
  ramp.Clear();
  ControlRampProcessor processor(ramp);

  // 20 samples starting at sample 20: 12 in this block, 8 in the next
  processor.SetTarget(1., 20, 20, blockSize);

  processor.Process(blockSize);
  REQUIRE(ramp.transitionStart == 20);
  REQUIRE(ramp.transitionEnd <= blockSize);
  REQUIRE(ramp.endValue == Approx(12. / 20.));

  processor.Process(blockSize);
  REQUIRE(ramp.startValue == Approx(12. / 20.));
  REQUIRE(ramp.endValue == Approx(1.));
  REQUIRE(ramp.transitionEnd == 8);
}
