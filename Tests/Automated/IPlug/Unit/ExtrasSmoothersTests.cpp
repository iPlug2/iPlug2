/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for LogParamSmooth and SmoothedGain (IPlug/Extras/Smoothers.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

#include "Smoothers.h"
#include "ExtrasTestUtils.h"

using namespace iplug;
using Catch::Approx;

namespace
{
// The smoother is a one pole lowpass with a = exp(-2pi / (time * sr)), so after
// the smoothing time a step has covered 1 - exp(-2pi) of the distance
const double kSettledAfterSmoothTime = 1. - std::exp(-2. * PI);
} // namespace

TEST_CASE("LogParamSmooth: starts at the initial value", "[unit][extras][smoothers]")
{
  LogParamSmooth<double> zero;
  REQUIRE(zero.Process(0.) == 0.);

  LogParamSmooth<double> smoother(5., 0.75);
  REQUIRE(smoother.Process(0.75) == Approx(0.75).margin(1e-12));

  // one sample of smoothing towards a different target barely moves
  LogParamSmooth<double> slow(100., 0.75);
  REQUIRE(slow.Process(0.) == Approx(0.75).margin(0.01));
}

TEST_CASE("LogParamSmooth: step response converges monotonically to the target", "[unit][extras][smoothers]")
{
  const double target = GENERATE(1., -3., 0.001);

  LogParamSmooth<double> smoother(10., 0.);
  smoother.SetSmoothTime(10., 44100.);

  double prev = 0.;
  bool monotonic = true;
  bool overshoot = false;

  for (int s = 0; s < 44100; s++)
  {
    const double v = smoother.Process(target);

    if (std::fabs(v) < std::fabs(prev))
      monotonic = false;
    if (std::fabs(v) > std::fabs(target))
      overshoot = true;

    prev = v;
  }

  REQUIRE(monotonic);
  REQUIRE_FALSE(overshoot);
  REQUIRE(prev == Approx(target).epsilon(1e-9));
}

TEST_CASE("LogParamSmooth: smoothing time is independent of the sample rate", "[unit][extras][smoothers]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const double timeMs = GENERATE(1., 5., 50.);

  LogParamSmooth<double> smoother(timeMs, 0.);
  smoother.SetSmoothTime(timeMs, sr);

  const int nSmooth = static_cast<int>(std::lround(timeMs * 0.001 * sr));
  double v = 0.;

  // half the smoothing time
  for (int s = 0; s < nSmooth / 2; s++)
    v = smoother.Process(1.);

  CAPTURE(sr, timeMs);
  REQUIRE(v == Approx(1. - std::exp(-PI)).margin(0.02));

  for (int s = nSmooth / 2; s < nSmooth; s++)
    v = smoother.Process(1.);

  REQUIRE(v == Approx(kSettledAfterSmoothTime).margin(2e-3));

  // and after several smoothing times we are there
  for (int s = 0; s < nSmooth * 4; s++)
    v = smoother.Process(1.);

  REQUIRE(v == Approx(1.).margin(1e-9));
}

TEST_CASE("LogParamSmooth: default smoothing time uses the default sample rate", "[unit][extras][smoothers]")
{
  LogParamSmooth<double> smoother; // 5ms
  const int nSmooth = static_cast<int>(std::lround(5. * 0.001 * DEFAULT_SAMPLE_RATE));
  double v = 0.;

  for (int s = 0; s < nSmooth; s++)
    v = smoother.Process(1.);

  REQUIRE(v == Approx(kSettledAfterSmoothTime).margin(2e-3));
}

TEST_CASE("LogParamSmooth: a longer time smooths more slowly", "[unit][extras][smoothers]")
{
  LogParamSmooth<double> fast(1.), medium(10.), slow(100.);
  double f = 0., m = 0., sl = 0.;

  for (int s = 0; s < 200; s++)
  {
    f = fast.Process(1.);
    m = medium.Process(1.);
    sl = slow.Process(1.);
  }

  REQUIRE(f > m);
  REQUIRE(m > sl);
  REQUIRE(sl > 0.);
  REQUIRE(f <= 1.);
}

TEST_CASE("LogParamSmooth: SetValue jumps without smoothing", "[unit][extras][smoothers]")
{
  LogParamSmooth<double> smoother(20., 0.);
  smoother.SetSmoothTime(20., 48000.);

  for (int s = 0; s < 100; s++)
    smoother.Process(1.);

  smoother.SetValue(-0.5);
  REQUIRE(smoother.Process(-0.5) == Approx(-0.5).margin(1e-12));

  // and smoothing carries on from the new value
  const double next = smoother.Process(1.);
  REQUIRE(next > -0.5);
  REQUIRE(next < -0.45);
}

TEST_CASE("LogParamSmooth: a constant input equal to the state is a fixed point", "[unit][extras][smoothers]")
{
  LogParamSmooth<double> smoother(5., 0.3);

  for (int s = 0; s < 1000; s++)
    REQUIRE(smoother.Process(0.3) == Approx(0.3).margin(1e-12));
}

TEST_CASE("LogParamSmooth: multichannel block processing matches per-sample processing", "[unit][extras][smoothers]")
{
  const double sr = GENERATE(44100., 48000.);
  constexpr int nFrames = 512;

  LogParamSmooth<double, 2> block(10., 0.);
  block.SetSmoothTime(10., sr);

  LogParamSmooth<double> refL(10., 0.), refR(10., 0.);
  refL.SetSmoothTime(10., sr);
  refR.SetSmoothTime(10., sr);

  double targets[2] = {1., -2.};
  extrastest::Buffers<double> out(2, nFrames);

  block.ProcessBlock(targets, out.Get(), nFrames);

  double maxDiff = 0.;

  for (int s = 0; s < nFrames; s++)
  {
    maxDiff = std::max(maxDiff, std::fabs(out[0][static_cast<size_t>(s)] - refL.Process(targets[0])));
    maxDiff = std::max(maxDiff, std::fabs(out[1][static_cast<size_t>(s)] - refR.Process(targets[1])));
  }

  REQUIRE(maxDiff < 1e-12);

  // the two channels are smoothed independently, each towards its own target
  REQUIRE(out[0][nFrames - 1] > 0.);
  REQUIRE(out[1][nFrames - 1] < 0.);
  REQUIRE(out[1][nFrames - 1] == Approx(-2. * out[0][nFrames - 1]).margin(1e-9));
}

TEST_CASE("LogParamSmooth: state carries over between blocks", "[unit][extras][smoothers]")
{
  constexpr int nFrames = 400;

  LogParamSmooth<double, 2> oneBlock(10., 0.), manyBlocks(10., 0.);
  double targets[2] = {0.5, 4.};

  extrastest::Buffers<double> a(2, nFrames), b(2, nFrames);
  oneBlock.ProcessBlock(targets, a.Get(), nFrames);

  constexpr int nSub = 25;
  for (int start = 0; start < nFrames; start += nSub)
  {
    double* ptrs[2] = {b[0].data() + start, b[1].data() + start};
    manyBlocks.ProcessBlock(targets, ptrs, nSub);
  }

  REQUIRE(extrastest::MaxAbsDiff(a[0].data(), b[0].data(), nFrames) < 1e-12);
  REQUIRE(extrastest::MaxAbsDiff(a[1].data(), b[1].data(), nFrames) < 1e-12);
}

TEST_CASE("LogParamSmooth: SetValues and SetValue set every channel", "[unit][extras][smoothers]")
{
  LogParamSmooth<double, 3> smoother(10., 0.);
  double values[3] = {0.1, 0.2, 0.3};
  smoother.SetValues(values);

  extrastest::Buffers<double> out(3, 1);
  smoother.ProcessBlock(values, out.Get(), 1);

  REQUIRE(out[0][0] == Approx(0.1).margin(1e-12));
  REQUIRE(out[1][0] == Approx(0.2).margin(1e-12));
  REQUIRE(out[2][0] == Approx(0.3).margin(1e-12));

  smoother.SetValue(7.);
  double sevens[3] = {7., 7., 7.};
  smoother.ProcessBlock(sevens, out.Get(), 1);

  for (int c = 0; c < 3; c++)
    REQUIRE(out[c][0] == Approx(7.).margin(1e-12));
}

TEST_CASE("LogParamSmooth: the initial value applies to every channel", "[unit][extras][smoothers]")
{
  LogParamSmooth<double, 4> smoother(10., 0.25);
  double targets[4] = {0.25, 0.25, 0.25, 0.25};
  extrastest::Buffers<double> out(4, 8);
  smoother.ProcessBlock(targets, out.Get(), 8);

  for (int c = 0; c < 4; c++)
    for (size_t s = 0; s < 8; s++)
      REQUIRE(out[c][s] == Approx(0.25).margin(1e-12));
}

TEST_CASE("LogParamSmooth: channel offset addresses a subset of a larger channel array", "[unit][extras][smoothers]")
{
  constexpr int nFrames = 64;
  constexpr double kSentinel = 123.;

  // two smoothed channels living at channels 1 and 2 of a 4 channel layout
  LogParamSmooth<double, 2> smoother(1., 0.);
  double targets[4] = {9., 1., -1., 9.};
  extrastest::Buffers<double> out(4, nFrames, kSentinel);

  smoother.ProcessBlock(targets, out.Get(), nFrames, 1);

  for (size_t s = 0; s < nFrames; s++)
  {
    REQUIRE(out[0][s] == kSentinel);
    REQUIRE(out[3][s] == kSentinel);
  }

  REQUIRE(out[1][nFrames - 1] > 0.5);
  REQUIRE(out[1][nFrames - 1] <= 1.);
  REQUIRE(out[2][nFrames - 1] == Approx(-out[1][nFrames - 1]).margin(1e-12));
}

TEST_CASE("LogParamSmooth: single precision converges", "[unit][extras][smoothers]")
{
  LogParamSmooth<float, 2> block(5., 0.f);
  LogParamSmooth<float> single(5., 0.f);
  block.SetSmoothTime(5., 44100.);
  single.SetSmoothTime(5., 44100.);

  constexpr int nFrames = 4410;
  float targets[2] = {1.f, 0.5f};
  extrastest::Buffers<float> out(2, nFrames);
  block.ProcessBlock(targets, out.Get(), nFrames);

  double maxDiff = 0.;
  for (size_t s = 0; s < nFrames; s++)
    maxDiff = std::max(maxDiff, std::fabs(static_cast<double>(out[0][s]) - static_cast<double>(single.Process(1.f))));

  REQUIRE(maxDiff < 1e-4);
  REQUIRE(out[0][nFrames - 1] == Approx(1.f).margin(1e-4));
  REQUIRE(out[1][nFrames - 1] == Approx(0.5f).margin(1e-4));
  REQUIRE(extrastest::AllFinite(out[0].data(), nFrames));
}

TEST_CASE("SmoothedGain: ramps up from silence to the requested gain", "[unit][extras][smoothers]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  constexpr int nChans = 2;
  const int nFrames = static_cast<int>(sr / 10.);

  SmoothedGain<double> gain(5.);
  gain.SetSampleRate(sr);

  extrastest::Buffers<double> in(nChans, nFrames, 1.), out(nChans, nFrames);
  in[1].assign(static_cast<size_t>(nFrames), -0.5);

  gain.ProcessBlock(in.Get(), out.Get(), nChans, nFrames, 0.5);

  CAPTURE(sr);

  // starts (almost) silent, with no jump
  REQUIRE(std::fabs(out[0][0]) < 0.05);

  bool monotonic = true;
  for (size_t s = 1; s < static_cast<size_t>(nFrames); s++)
  {
    if (out[0][s] < out[0][s - 1])
      monotonic = false;
  }
  REQUIRE(monotonic);

  // after the smoothing time
  const size_t nSmooth = static_cast<size_t>(std::lround(0.005 * sr));
  REQUIRE(out[0][nSmooth - 1] == Approx(0.5 * kSettledAfterSmoothTime).margin(2e-3));

  // settled
  REQUIRE(out[0][static_cast<size_t>(nFrames) - 1] == Approx(0.5).margin(1e-9));
  REQUIRE(out[1][static_cast<size_t>(nFrames) - 1] == Approx(-0.25).margin(1e-9));

  // the same gain is applied to every channel on every sample
  double maxDiff = 0.;
  for (size_t s = 0; s < static_cast<size_t>(nFrames); s++)
    maxDiff = std::max(maxDiff, std::fabs(out[1][s] + 0.5 * out[0][s]));
  REQUIRE(maxDiff < 1e-12);
}

TEST_CASE("SmoothedGain: gain changes are smoothed, not stepped", "[unit][extras][smoothers]")
{
  constexpr int nFrames = 4800;

  SmoothedGain<double> gain(10.);
  gain.SetSampleRate(48000.);

  extrastest::Buffers<double> in(1, nFrames, 1.), out(1, nFrames);

  gain.ProcessBlock(in.Get(), out.Get(), 1, nFrames, 1.);
  REQUIRE(out[0][nFrames - 1] == Approx(1.).margin(1e-6));

  gain.ProcessBlock(in.Get(), out.Get(), 1, nFrames, 0.);

  // 10ms smoothing at 48kHz: the largest step is a small fraction of the change
  double maxStep = std::fabs(1. - out[0][0]);
  for (size_t s = 1; s < nFrames; s++)
    maxStep = std::max(maxStep, std::fabs(out[0][s] - out[0][s - 1]));

  REQUIRE(maxStep < 0.02);
  REQUIRE(out[0][0] > 0.98);
  REQUIRE(out[0][nFrames - 1] == Approx(0.).margin(1e-6));
}

TEST_CASE("SmoothedGain: works in place and across block boundaries", "[unit][extras][smoothers]")
{
  constexpr int nFrames = 1024;

  SmoothedGain<double> a(5.), b(5.);
  a.SetSampleRate(44100.);
  b.SetSampleRate(44100.);

  const auto noise = extrastest::MakeNoise<double>(nFrames, 99u);

  extrastest::Buffers<double> in(1, nFrames), ref(1, nFrames), inPlace(1, nFrames);
  in[0] = noise;
  inPlace[0] = noise;

  a.ProcessBlock(in.Get(), ref.Get(), 1, nFrames, 0.8);

  for (int start = 0; start < nFrames; start += 64)
  {
    double* ptrs[1] = {inPlace[0].data() + start};
    b.ProcessBlock(ptrs, ptrs, 1, 64, 0.8);
  }

  REQUIRE(extrastest::MaxAbsDiff(ref[0].data(), inPlace[0].data(), nFrames) < 1e-12);
}

TEST_CASE("SmoothedGain: single precision", "[unit][extras][smoothers]")
{
  constexpr int nFrames = 4800;

  SmoothedGain<float> gain;
  gain.SetSampleRate(48000.);

  extrastest::Buffers<float> in(2, nFrames, 1.f), out(2, nFrames);
  gain.ProcessBlock(in.Get(), out.Get(), 2, nFrames, 2.);

  REQUIRE(out[0][nFrames - 1] == Approx(2.f).margin(1e-5));
  REQUIRE(out[1][nFrames - 1] == Approx(2.f).margin(1e-5));
  REQUIRE(extrastest::PeakAbs(out[0].data(), nFrames) <= 2.0 + 1e-6);
}
