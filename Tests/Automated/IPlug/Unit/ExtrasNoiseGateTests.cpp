/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for NoiseGate (IPlug/Extras/NoiseGate.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "NoiseGate.h"
#include "ExtrasTestUtils.h"

using namespace iplug;
using Catch::Approx;

namespace
{
struct GateSettings
{
  double sampleRate = 48000.;
  double thresholdDB = -20.;
  double attack = 0.001; // seconds
  double hold = 0.001;
  double release = 0.01;
};

template <typename T, int NC>
void Configure(NoiseGate<T, NC>& gate, const GateSettings& settings)
{
  // the sample rate has to come first, as the times are clamped to one sample
  gate.SetSampleRate(settings.sampleRate);
  gate.SetThreshold(settings.thresholdDB);
  gate.SetAttackTime(settings.attack);
  gate.SetHoldTime(settings.hold);
  gate.SetReleaseTime(settings.release);
}

/** @return The gain the gate applies on each sample for the given sidechain signal */
std::vector<double> GainFor(const GateSettings& settings, const std::vector<double>& sidechain, int blockSize = 64)
{
  NoiseGate<double, 1> gate;
  Configure(gate, settings);

  const int nFrames = static_cast<int>(sidechain.size());
  std::vector<double> ones(sidechain.size(), 1.);
  std::vector<double> gain(sidechain.size(), 0.);
  std::vector<double> sc = sidechain;

  for (int start = 0; start < nFrames; start += blockSize)
  {
    double* ins[1] = {ones.data() + start};
    double* outs[1] = {gain.data() + start};
    gate.ProcessBlock(ins, outs, sc.data() + start, 1, std::min(blockSize, nFrames - start));
  }

  return gain;
}

/** A sidechain signal that is at "level" for nOn samples and then silent for nOff samples */
std::vector<double> Burst(double level, int nOn, int nOff)
{
  std::vector<double> v(static_cast<size_t>(nOn + nOff), 0.);
  std::fill(v.begin(), v.begin() + nOn, level);
  return v;
}

int FirstIndexWhere(const std::vector<double>& v, int start, bool (*pred)(double))
{
  for (int s = start; s < static_cast<int>(v.size()); s++)
  {
    if (pred(v[static_cast<size_t>(s)]))
      return s;
  }

  return -1;
}
} // namespace

TEST_CASE("NoiseGate: opens fully for a sidechain above the threshold", "[unit][extras][noisegate]")
{
  GateSettings settings;
  settings.sampleRate = GENERATE(44100., 48000., 96000.);
  const int nFrames = static_cast<int>(settings.sampleRate / 10.);

  const auto gain = GainFor(settings, std::vector<double>(static_cast<size_t>(nFrames), 1.));

  REQUIRE(gain[0] < 0.1); // closed to start with
  REQUIRE(gain.back() == Approx(1.).margin(1e-6));

  bool monotonic = true;
  for (size_t s = 1; s < gain.size(); s++)
  {
    if (gain[s] < gain[s - 1])
      monotonic = false;
  }
  REQUIRE(monotonic);
}

TEST_CASE("NoiseGate: stays shut for a sidechain below the threshold", "[unit][extras][noisegate]")
{
  GateSettings settings;
  settings.sampleRate = GENERATE(44100., 48000., 96000.);
  const int nFrames = static_cast<int>(settings.sampleRate / 10.);

  // threshold is -20dB = 0.1
  const auto gain = GainFor(settings, std::vector<double>(static_cast<size_t>(nFrames), 0.05));
  REQUIRE(extrastest::PeakAbs(gain.data(), nFrames) == 0.);

  const auto silent = GainFor(settings, std::vector<double>(static_cast<size_t>(nFrames), 0.));
  REQUIRE(extrastest::PeakAbs(silent.data(), nFrames) == 0.);
}

TEST_CASE("NoiseGate: threshold is set in decibels", "[unit][extras][noisegate]")
{
  GateSettings settings;
  settings.thresholdDB = GENERATE(-60., -40., -20., -6., 0.);
  const double thresholdAmp = std::pow(10., settings.thresholdDB / 20.);
  constexpr int nFrames = 9600;

  CAPTURE(settings.thresholdDB);

  const auto above = GainFor(settings, std::vector<double>(nFrames, thresholdAmp * 1.2));
  REQUIRE(above.back() == Approx(1.).margin(1e-6));

  const auto below = GainFor(settings, std::vector<double>(nFrames, thresholdAmp * 0.8));
  REQUIRE(extrastest::PeakAbs(below.data(), nFrames) == 0.);
}

TEST_CASE("NoiseGate: default threshold is half scale", "[unit][extras][noisegate]")
{
  constexpr int nFrames = 9600;

  NoiseGate<double, 1> gate;
  gate.SetSampleRate(48000.);

  std::vector<double> ones(nFrames, 1.), out(nFrames, 0.);
  std::vector<double> quiet(nFrames, 0.4), loud(nFrames, 0.6);
  double* ins[1] = {ones.data()};
  double* outs[1] = {out.data()};

  gate.ProcessBlock(ins, outs, quiet.data(), 1, nFrames);
  REQUIRE(extrastest::PeakAbs(out.data(), nFrames) == 0.);

  gate.ProcessBlock(ins, outs, loud.data(), 1, nFrames);
  REQUIRE(out.back() == Approx(1.).margin(1e-6));
}

TEST_CASE("NoiseGate: the sidechain is rectified", "[unit][extras][noisegate]")
{
  GateSettings settings;
  constexpr int nFrames = 4800;

  const auto gain = GainFor(settings, std::vector<double>(nFrames, -1.));
  REQUIRE(gain.back() == Approx(1.).margin(1e-6));

  // an audio rate sidechain well above the threshold holds the gate open
  const auto sine = extrastest::MakeSine<double>(1000., settings.sampleRate, nFrames, 0.8);
  const auto sineGain = GainFor(settings, sine);
  REQUIRE(sineGain.back() == Approx(1.).margin(1e-3));
}

TEST_CASE("NoiseGate: gain rises with the attack time constant", "[unit][extras][noisegate]")
{
  GateSettings settings;
  settings.sampleRate = GENERATE(44100., 48000., 96000.);
  settings.attack = GENERATE(0.002, 0.01, 0.03);
  settings.release = 0.05;

  const int nAttack = static_cast<int>(std::lround(settings.attack * settings.sampleRate));
  const int nFrames = nAttack * 8;

  const auto gain = GainFor(settings, std::vector<double>(static_cast<size_t>(nFrames), 1.));

  // the gate opens once the level detector has crossed the threshold...
  const int opened = FirstIndexWhere(gain, 0, [](double g) { return g > 0.; });
  REQUIRE(opened >= 0);
  REQUIRE(opened < nAttack);

  // ...after which the gain is a one pole rise: 1 - 1/e after one time constant, and so on
  CAPTURE(settings.sampleRate, settings.attack, opened);
  REQUIRE(gain[static_cast<size_t>(opened + nAttack - 1)] == Approx(1. - std::exp(-1.)).margin(0.01));
  REQUIRE(gain[static_cast<size_t>(opened + 3 * nAttack - 1)] == Approx(1. - std::exp(-3.)).margin(0.01));
}

TEST_CASE("NoiseGate: gain falls with the release time constant", "[unit][extras][noisegate]")
{
  GateSettings settings;
  settings.sampleRate = GENERATE(44100., 48000., 96000.);
  settings.release = GENERATE(0.005, 0.02);

  const int nRelease = static_cast<int>(std::lround(settings.release * settings.sampleRate));
  const int nOn = static_cast<int>(settings.sampleRate / 20.);
  const int nOff = nRelease * 12;

  const auto gain = GainFor(settings, Burst(1., nOn, nOff));

  REQUIRE(gain[static_cast<size_t>(nOn - 1)] == Approx(1.).margin(1e-6));

  // find the release, then check it decays by 1/e per time constant
  const int falling = FirstIndexWhere(gain, nOn, [](double g) { return g < 0.9; });
  REQUIRE(falling > nOn);

  CAPTURE(settings.sampleRate, settings.release);
  const double g0 = gain[static_cast<size_t>(falling)];
  REQUIRE(gain[static_cast<size_t>(falling + nRelease)] / g0 == Approx(std::exp(-1.)).margin(0.01));
  REQUIRE(gain[static_cast<size_t>(falling + 2 * nRelease)] / g0 == Approx(std::exp(-2.)).margin(0.01));

  // and closes
  REQUIRE(gain.back() < 1e-3);

  bool monotonic = true;
  for (size_t s = static_cast<size_t>(falling) + 1; s < gain.size(); s++)
  {
    if (gain[s] > gain[s - 1])
      monotonic = false;
  }
  REQUIRE(monotonic);
}

TEST_CASE("NoiseGate: hold time keeps the gate open after the signal has gone", "[unit][extras][noisegate]")
{
  const double sr = GENERATE(44100., 48000.);

  GateSettings shortHold, longHold;
  shortHold.sampleRate = longHold.sampleRate = sr;
  shortHold.hold = 0.001;
  longHold.hold = 0.05;

  const int nOn = static_cast<int>(sr / 20.);
  const int nOff = static_cast<int>(sr / 5.);
  const auto burst = Burst(1., nOn, nOff);

  const auto gainShort = GainFor(shortHold, burst);
  const auto gainLong = GainFor(longHold, burst);

  const auto isClosing = [](double g) { return g < 0.5; };
  const int closedShort = FirstIndexWhere(gainShort, nOn, isClosing);
  const int closedLong = FirstIndexWhere(gainLong, nOn, isClosing);

  REQUIRE(closedShort > nOn);
  REQUIRE(closedLong > closedShort);

  const int expectedDifference = static_cast<int>(std::lround((longHold.hold - shortHold.hold) * sr));
  CAPTURE(sr, closedShort, closedLong);
  REQUIRE(std::abs((closedLong - closedShort) - expectedDifference) <= 2);

  // still fully open most of the way through the long hold
  REQUIRE(gainLong[static_cast<size_t>(nOn + static_cast<int>(0.04 * sr))] == Approx(1.).margin(1e-6));
}

TEST_CASE("NoiseGate: re-opens when the signal returns", "[unit][extras][noisegate]")
{
  GateSettings settings;
  const int n = 4800;

  std::vector<double> sidechain = Burst(1., n, n);
  const auto second = Burst(1., n, n);
  sidechain.insert(sidechain.end(), second.begin(), second.end());

  const auto gain = GainFor(settings, sidechain);

  REQUIRE(gain[static_cast<size_t>(n - 1)] == Approx(1.).margin(1e-6));
  REQUIRE(gain[static_cast<size_t>(2 * n - 1)] < 1e-3);
  REQUIRE(gain[static_cast<size_t>(3 * n - 1)] == Approx(1.).margin(1e-6));
  REQUIRE(gain[static_cast<size_t>(4 * n - 1)] < 1e-3);
}

TEST_CASE("NoiseGate: gain is always within [0, 1]", "[unit][extras][noisegate]")
{
  const double sr = GENERATE(44100., 96000.);
  constexpr int nFrames = 30000;

  extrastest::LCG lcg(777u);
  NoiseGate<double, 1> gate;
  gate.SetSampleRate(sr);

  std::vector<double> ones(nFrames, 1.), gain(nFrames, 0.), sidechain(nFrames, 0.);

  // noise bursts of random level, with the settings changing as we go, including degenerate times
  double level = 1.;
  for (int s = 0; s < nFrames; s++)
  {
    if ((s % 500) == 0)
      level = (lcg.NextInt() % 3u) == 0u ? 0. : lcg.NextUnipolar() * 4.;

    sidechain[static_cast<size_t>(s)] = lcg.NextBipolar() * level;
  }

  for (int start = 0; start < nFrames; start += 1000)
  {
    gate.SetThreshold(-60. + lcg.NextUnipolar() * 60.);
    gate.SetAttackTime(lcg.NextUnipolar() * 0.02);
    gate.SetHoldTime(lcg.NextUnipolar() * 0.02);
    gate.SetReleaseTime((lcg.NextInt() % 4u) == 0u ? 0. : lcg.NextUnipolar() * 0.02);

    double* ins[1] = {ones.data() + start};
    double* outs[1] = {gain.data() + start};
    gate.ProcessBlock(ins, outs, sidechain.data() + start, 1, 1000);
  }

  bool bounded = true;
  for (double g : gain)
  {
    if (!(g >= 0. && g <= 1.))
      bounded = false;
  }

  REQUIRE(bounded);
  REQUIRE(extrastest::PeakAbs(gain.data(), nFrames) > 0.5); // it did open at some point
}

TEST_CASE("NoiseGate: applies the same gain to every channel", "[unit][extras][noisegate]")
{
  constexpr int nFrames = 9600;
  GateSettings settings;

  NoiseGate<double, 3> gate;
  Configure(gate, settings);

  extrastest::Buffers<double> in(3, nFrames, 1.), out(3, nFrames);
  in[1] = extrastest::MakeNoise<double>(nFrames, 31u);
  in[2] = extrastest::MakeSine<double>(440., settings.sampleRate, nFrames, 0.5);

  auto sidechain = Burst(1., nFrames / 2, nFrames / 2);
  gate.ProcessBlock(in.Get(), out.Get(), sidechain.data(), 3, nFrames);

  // channel 0 is all ones, so its output is the gain
  const auto expected = GainFor(settings, sidechain);
  REQUIRE(extrastest::MaxAbsDiff(out[0].data(), expected.data(), nFrames) < 1e-12);

  double maxDiff = 0.;
  for (size_t s = 0; s < nFrames; s++)
  {
    maxDiff = std::max(maxDiff, std::fabs(out[1][s] - in[1][s] * out[0][s]));
    maxDiff = std::max(maxDiff, std::fabs(out[2][s] - in[2][s] * out[0][s]));
  }
  REQUIRE(maxDiff < 1e-12);
}

TEST_CASE("NoiseGate: processing fewer channels than the maximum leaves the rest untouched", "[unit][extras][noisegate]")
{
  constexpr int nFrames = 512;
  constexpr double kSentinel = 42.;

  NoiseGate<double, 2> gate;
  Configure(gate, GateSettings());

  extrastest::Buffers<double> in(2, nFrames, 1.), out(2, nFrames, kSentinel);
  std::vector<double> sidechain(nFrames, 1.);
  gate.ProcessBlock(in.Get(), out.Get(), sidechain.data(), 1, nFrames);

  REQUIRE(out[0][nFrames - 1] > 0.9);
  for (size_t s = 0; s < nFrames; s++)
    REQUIRE(out[1][s] == kSentinel);
}

TEST_CASE("NoiseGate: result does not depend on the block size", "[unit][extras][noisegate]")
{
  GateSettings settings;
  settings.sampleRate = 44100.;
  constexpr int nFrames = 8000;

  // noise that comes and goes
  auto sidechain = extrastest::MakeNoise<double>(nFrames, 5150u);
  for (int s = 0; s < nFrames; s++)
  {
    if ((s / 1000) % 2 == 1)
      sidechain[static_cast<size_t>(s)] *= 0.001;
  }

  const auto reference = GainFor(settings, sidechain, nFrames);
  const int blockSize = GENERATE(1, 17, 64, 512);
  const auto gain = GainFor(settings, sidechain, blockSize);

  REQUIRE(extrastest::MaxAbsDiff(reference.data(), gain.data(), nFrames) < 1e-12);
  REQUIRE(extrastest::PeakAbs(reference.data(), nFrames) > 0.9);
}

TEST_CASE("NoiseGate: single precision", "[unit][extras][noisegate]")
{
  constexpr int nFrames = 9600;
  GateSettings settings;

  NoiseGate<float, 2> gate;
  Configure(gate, settings);

  extrastest::Buffers<float> in(2, nFrames, 1.f), out(2, nFrames);
  std::vector<float> sidechain(nFrames, 0.f);
  std::fill(sidechain.begin(), sidechain.begin() + nFrames / 2, 1.f);

  gate.ProcessBlock(in.Get(), out.Get(), sidechain.data(), 2, nFrames);

  REQUIRE(extrastest::AllFinite(out[0].data(), nFrames));
  REQUIRE(out[0][nFrames / 2 - 1] == Approx(1.f).margin(1e-4));
  REQUIRE(out[1][nFrames / 2 - 1] == Approx(1.f).margin(1e-4));
  REQUIRE(out[0][nFrames - 1] < 1e-3f);

  const auto expected = GainFor(settings, std::vector<double>(sidechain.begin(), sidechain.end()));
  double maxDiff = 0.;
  for (size_t s = 0; s < nFrames; s++)
    maxDiff = std::max(maxDiff, std::fabs(static_cast<double>(out[0][s]) - expected[s]));
  REQUIRE(maxDiff < 0.05); // a threshold crossing may land on a neighbouring sample
}
