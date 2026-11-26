/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for SinOscillator and FastSinOscillator (IPlug/Extras/Oscillator.h, Oscillator_table.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "IPlugConstants.h"
#include "Oscillator.h"
#include "ExtrasTestUtils.h"

using namespace iplug;
using Catch::Approx;

namespace
{
double RadiansPerSample(double freqHz, double sr)
{
  return 2. * PI * freqHz / sr;
}

template <typename OSC>
std::vector<double> RenderPerSample(OSC& osc, double freqHz, int nFrames)
{
  std::vector<double> out(static_cast<size_t>(nFrames));

  for (auto& v : out)
    v = static_cast<double>(osc.Process(freqHz));

  return out;
}

template <typename OSC>
std::vector<double> RenderPerSample(OSC& osc, int nFrames)
{
  std::vector<double> out(static_cast<size_t>(nFrames));

  for (auto& v : out)
    v = static_cast<double>(osc.Process());

  return out;
}

// The table oscillator interpolates a 512 point table of 14 bit values
constexpr double kFastSinTolerance = 5e-4;
} // namespace

// ----------------------------------------------------------------------------
// SinOscillator
// ----------------------------------------------------------------------------

TEST_CASE("SinOscillator: generates a unit sinusoid at the requested frequency", "[unit][extras][oscillator]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const double freq = GENERATE(20., 440., 1000., 12345.6);
  const int nFrames = static_cast<int>(sr); // one second

  SinOscillator<double> osc;
  osc.SetSampleRate(sr);
  const auto out = RenderPerSample(osc, freq, nFrames);

  CAPTURE(sr, freq);

  const auto fit = extrastest::FitSinusoid(out.data(), 8192, RadiansPerSample(freq, sr));
  REQUIRE(fit.amplitude == Approx(1.).margin(1e-9));
  REQUIRE(fit.maxResidual < 1e-9);

  // the same again a long way in, i.e. the frequency does not drift
  const auto lateFit = extrastest::FitSinusoid(out.data() + nFrames - 8192, 8192, RadiansPerSample(freq, sr));
  REQUIRE(lateFit.amplitude == Approx(1.).margin(1e-7));
  REQUIRE(lateFit.maxResidual < 1e-7);

  REQUIRE(extrastest::CountRisingZeroCrossings(out.data(), nFrames) == Approx(freq).margin(1.));
  REQUIRE(extrastest::PeakAbs(out.data(), nFrames) <= 1.);
  REQUIRE(extrastest::PeakAbs(out.data(), nFrames) > 0.99);
}

TEST_CASE("SinOscillator: Process() without a frequency uses SetFreqCPS", "[unit][extras][oscillator]")
{
  const double sr = GENERATE(44100., 96000.);
  const double freq = 1000.;

  SinOscillator<double> osc;
  osc.SetSampleRate(sr); // must come before SetFreqCPS, which converts to a phase increment
  osc.SetFreqCPS(freq);
  const auto out = RenderPerSample(osc, 4096);

  const auto fit = extrastest::FitSinusoid(out.data(), 4096, RadiansPerSample(freq, sr));
  CAPTURE(sr);
  REQUIRE(fit.amplitude == Approx(1.).margin(1e-9));
  REQUIRE(fit.maxResidual < 1e-9);

  // and gives the same thing as passing the frequency each sample
  SinOscillator<double> other;
  other.SetSampleRate(sr);
  const auto otherOut = RenderPerSample(other, freq, 4096);
  REQUIRE(extrastest::MaxAbsDiff(out.data(), otherOut.data(), 4096) < 1e-12);
}

TEST_CASE("SinOscillator: constructor start frequency", "[unit][extras][oscillator]")
{
  // 441Hz at the default sample rate of 44.1kHz is a period of exactly 100 samples
  SinOscillator<double> osc(0., 441.);
  const auto out = RenderPerSample(osc, 1000);

  const auto fit = extrastest::FitSinusoid(out.data(), 1000, 2. * PI / 100.);
  REQUIRE(fit.amplitude == Approx(1.).margin(1e-9));
  REQUIRE(fit.maxResidual < 1e-9);
}

TEST_CASE("SinOscillator: frequency changes are phase continuous", "[unit][extras][oscillator]")
{
  const double sr = 48000.;
  SinOscillator<double> osc;
  osc.SetSampleRate(sr);

  // sweep 100Hz to 2kHz over a second
  double prev = 0.;
  double maxStep = 0.;

  for (int s = 0; s < 48000; s++)
  {
    const double freq = 100. + 1900. * static_cast<double>(s) / 48000.;
    const double v = osc.Process(freq);
    maxStep = std::max(maxStep, std::fabs(v - prev));
    prev = v;
  }

  // a sinusoid can change by at most its angular frequency per sample
  REQUIRE(maxStep <= RadiansPerSample(2000., sr) + 1e-9);
  REQUIRE(maxStep > RadiansPerSample(1000., sr));
}

TEST_CASE("SinOscillator: Reset restarts the waveform", "[unit][extras][oscillator]")
{
  SinOscillator<double> osc;
  osc.SetSampleRate(48000.);

  const auto first = RenderPerSample(osc, 997., 500);
  const auto carriedOn = RenderPerSample(osc, 997., 500);
  REQUIRE(extrastest::MaxAbsDiff(first.data(), carriedOn.data(), 500) > 0.1);

  osc.Reset();
  const auto second = RenderPerSample(osc, 997., 500);
  REQUIRE(extrastest::MaxAbsDiff(first.data(), second.data(), 500) < 1e-12);
}

TEST_CASE("SinOscillator: phase is in cycles", "[unit][extras][oscillator]")
{
  // at 1Hz the phase barely moves in a sample, so the first output shows the phase we set
  struct PhaseAndValue
  {
    double phase, value;
  };

  const auto expected = GENERATE(PhaseAndValue{0., 0.}, PhaseAndValue{0.25, 1.}, PhaseAndValue{0.5, 0.}, PhaseAndValue{0.75, -1.}, PhaseAndValue{1., 0.});

  CAPTURE(expected.phase);

  SECTION("SetPhase")
  {
    SinOscillator<double> osc;
    osc.SetPhase(expected.phase);
    REQUIRE(osc.Process(1.) == Approx(expected.value).margin(1e-3));
  }

  SECTION("start phase, applied by Reset")
  {
    SinOscillator<double> osc(expected.phase);
    osc.Process(1.);
    osc.SetPhase(0.123);
    osc.Reset();
    REQUIRE(osc.Process(1.) == Approx(expected.value).margin(1e-3));
  }
}

// BUG (minor): IPlug/Extras/Oscillator.h:22 IOscillator's constructor stores startPhase in mStartPhase
// but leaves mPhase at 0, so the start phase passed to the constructor has no effect until Reset()
// has been called. A newly constructed oscillator should start at its start phase.
TEST_CASE("SinOscillator: constructor start phase applies from the first sample", "[.][bug]")
{
  SinOscillator<double> osc(0.25);
  REQUIRE(osc.Process(1.) == Approx(1.).margin(1e-3));
}

TEST_CASE("SinOscillator: single precision", "[unit][extras][oscillator]")
{
  const double sr = 44100.;
  SinOscillator<float> osc;
  osc.SetSampleRate(sr);

  const auto out = RenderPerSample(osc, 440., 4096);
  const auto fit = extrastest::FitSinusoid(out.data(), 4096, RadiansPerSample(440., sr));

  REQUIRE(fit.amplitude == Approx(1.).margin(1e-5));
  REQUIRE(fit.maxResidual < 1e-5);
}

// ----------------------------------------------------------------------------
// FastSinOscillator
// ----------------------------------------------------------------------------

TEST_CASE("FastSinOscillator: generates a unit sinusoid at the requested frequency", "[unit][extras][oscillator]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const double freq = GENERATE(20., 440., 1000., 12345.6);
  const int nFrames = static_cast<int>(sr); // one second

  FastSinOscillator<double> osc;
  osc.SetSampleRate(sr);
  osc.SetFreqCPS(freq);

  // in blocks of a realistic size (see the hidden test below about very large blocks)
  constexpr int blockSize = 256;
  std::vector<double> out(static_cast<size_t>(nFrames));
  for (int start = 0; start < nFrames; start += blockSize)
    osc.ProcessBlock(out.data() + start, std::min(blockSize, nFrames - start));

  CAPTURE(sr, freq);

  const auto fit = extrastest::FitSinusoid(out.data(), 8192, RadiansPerSample(freq, sr));
  REQUIRE(fit.amplitude == Approx(1.).margin(kFastSinTolerance));
  REQUIRE(fit.maxResidual < kFastSinTolerance);

  // the same again a long way in, i.e. the frequency does not drift
  const auto lateFit = extrastest::FitSinusoid(out.data() + nFrames - 8192, 8192, RadiansPerSample(freq, sr));
  REQUIRE(lateFit.amplitude == Approx(1.).margin(kFastSinTolerance));
  REQUIRE(lateFit.maxResidual < kFastSinTolerance);

  REQUIRE(extrastest::CountRisingZeroCrossings(out.data(), nFrames) == Approx(freq).margin(1.));
  REQUIRE(extrastest::PeakAbs(out.data(), nFrames) <= 1.);
  REQUIRE(extrastest::PeakAbs(out.data(), nFrames) > 0.99);
  REQUIRE(extrastest::RMS(out.data(), nFrames) == Approx(std::sqrt(0.5)).margin(1e-3));
}

TEST_CASE("FastSinOscillator: stays in tune over a long run of small blocks", "[unit][extras][oscillator]")
{
  const double sr = 48000.;
  const double freq = 1000.;
  constexpr int blockSize = 64;
  constexpr int nBlocks = 7500; // 10 seconds

  FastSinOscillator<double> osc;
  osc.SetSampleRate(sr);
  osc.SetFreqCPS(freq);

  std::vector<double> block(blockSize);
  int nCycles = 0;
  double prev = 1.;
  double peak = 0.;

  for (int b = 0; b < nBlocks; b++)
  {
    osc.ProcessBlock(block.data(), blockSize);

    for (double v : block)
    {
      if (prev < 0. && v >= 0.)
        nCycles++;
      prev = v;
      peak = std::max(peak, std::fabs(v));
    }
  }

  REQUIRE(std::abs(nCycles - 10000) <= 1);
  REQUIRE(peak <= 1.);

  // the last block is still a clean sinusoid of the right frequency
  const auto fit = extrastest::FitSinusoid(block.data(), blockSize, RadiansPerSample(freq, sr));
  REQUIRE(fit.amplitude == Approx(1.).margin(kFastSinTolerance));
  REQUIRE(fit.maxResidual < kFastSinTolerance);
}

TEST_CASE("FastSinOscillator: block and per-sample processing agree", "[unit][extras][oscillator]")
{
  const double sr = GENERATE(44100., 48000.);
  const double freq = 440.;
  constexpr int nFrames = 4096;

  FastSinOscillator<double> blockOsc, sampleOsc, sampleOscWithFreq, splitOsc;

  for (auto* pOsc : {&blockOsc, &sampleOsc, &sampleOscWithFreq, &splitOsc})
  {
    pOsc->SetSampleRate(sr);
    pOsc->SetFreqCPS(freq);
  }

  std::vector<double> blockOut(nFrames), splitOut(nFrames);
  blockOsc.ProcessBlock(blockOut.data(), nFrames);

  for (int start = 0; start < nFrames; start += 128)
    splitOsc.ProcessBlock(splitOut.data() + start, 128);

  const auto sampleOut = RenderPerSample(sampleOsc, nFrames);
  const auto sampleOutWithFreq = RenderPerSample(sampleOscWithFreq, freq, nFrames);

  // the phase is stored between calls with less precision than within a block
  REQUIRE(extrastest::MaxAbsDiff(blockOut.data(), splitOut.data(), nFrames) < 1e-5);
  REQUIRE(extrastest::MaxAbsDiff(blockOut.data(), sampleOut.data(), nFrames) < 1e-5);
  REQUIRE(extrastest::MaxAbsDiff(blockOut.data(), sampleOutWithFreq.data(), nFrames) < 1e-5);
}

TEST_CASE("FastSinOscillator: Process(freq) changes the frequency", "[unit][extras][oscillator]")
{
  const double sr = 44100.;
  FastSinOscillator<double> osc;
  osc.SetSampleRate(sr);

  for (double freq : {250., 2500., 100.})
  {
    const auto out = RenderPerSample(osc, freq, 4096);
    const auto fit = extrastest::FitSinusoid(out.data(), 4096, RadiansPerSample(freq, sr));

    CAPTURE(freq);
    REQUIRE(fit.amplitude == Approx(1.).margin(kFastSinTolerance));
    REQUIRE(fit.maxResidual < kFastSinTolerance);
  }
}

TEST_CASE("FastSinOscillator: mLastOutput holds the most recent sample", "[unit][extras][oscillator]")
{
  FastSinOscillator<double> osc;
  osc.SetFreqCPS(1234.);

  for (int s = 0; s < 100; s++)
  {
    const double v = osc.Process();
    REQUIRE(osc.mLastOutput == v);
  }

  std::vector<double> block(256);
  osc.ProcessBlock(block.data(), 256);
  REQUIRE(osc.mLastOutput == block.back());
}

TEST_CASE("FastSinOscillator: Reset restarts the waveform", "[unit][extras][oscillator]")
{
  FastSinOscillator<double> osc;
  osc.SetSampleRate(48000.);
  osc.SetFreqCPS(997.);

  std::vector<double> first(500), carriedOn(500), second(500);
  osc.ProcessBlock(first.data(), 500);
  osc.ProcessBlock(carriedOn.data(), 500);
  REQUIRE(extrastest::MaxAbsDiff(first.data(), carriedOn.data(), 500) > 0.1);

  osc.Reset();
  osc.ProcessBlock(second.data(), 500);
  REQUIRE(extrastest::MaxAbsDiff(first.data(), second.data(), 500) < 1e-9);
}

TEST_CASE("FastSinOscillator: start phase shifts the waveform by that fraction of a cycle", "[unit][extras][oscillator]")
{
  // a period of exactly 128 samples
  const double sr = 44100.;
  const double freq = sr / 128.;
  const int quarterCycles = GENERATE(1, 2, 3);

  FastSinOscillator<double> reference(0., freq);
  FastSinOscillator<double> shifted(0.25 * quarterCycles, freq);
  shifted.Reset(); // applies the start phase

  std::vector<double> refOut(512), shiftedOut(256);
  reference.ProcessBlock(refOut.data(), 512);
  shifted.ProcessBlock(shiftedOut.data(), 256);

  CAPTURE(quarterCycles);
  REQUIRE(extrastest::MaxAbsDiff(shiftedOut.data(), refOut.data() + 32 * quarterCycles, 256) < 0.02);
}

TEST_CASE("FastSinOscillator: single precision", "[unit][extras][oscillator]")
{
  const double sr = 48000.;
  FastSinOscillator<float> osc;
  osc.SetSampleRate(sr);
  osc.SetFreqCPS(440.);

  std::vector<float> out(4096);
  osc.ProcessBlock(out.data(), 4096);

  const auto fit = extrastest::FitSinusoid(out.data(), 4096, RadiansPerSample(440., sr));
  REQUIRE(fit.amplitude == Approx(1.).margin(kFastSinTolerance));
  REQUIRE(fit.maxResidual < kFastSinTolerance);
  REQUIRE(extrastest::PeakAbs(out.data(), 4096) <= 1.);
}

TEST_CASE("FastSinOscillator: Lookup is bounded for any phase", "[unit][extras][oscillator]")
{
  bool bounded = true;

  for (int i = -2000; i <= 2000; i++)
  {
    const double v = FastSinOscillator<double>::Lookup(static_cast<double>(i) * 0.01);
    if (!(v >= -1. && v <= 1.))
      bounded = false;
  }

  REQUIRE(bounded);
}

// BUG: IPlug/Extras/Oscillator.h:179-189 FastSinOscillator::ProcessBlock() never wraps its local
// "phase" inside the loop (line 189 just keeps adding the increment), but the bit trick used to
// split it into a table index and a fraction only works while UNITBIT32 + phase stays below 2^21,
// i.e. for the first 1024 cycles. A single call that spans more cycles than that produces garbage
// from that point in the block onwards, e.g. 12kHz at 44.1kHz with a block size of 4096 or more.
// (Pure Data's phasor~, which this is based on, wraps the phase on every sample.)
TEST_CASE("FastSinOscillator: large blocks at high frequencies", "[.][bug]")
{
  const double sr = 44100.;
  const double freq = 12000.;
  constexpr int nFrames = 8192;

  FastSinOscillator<double> osc;
  osc.SetSampleRate(sr);
  osc.SetFreqCPS(freq);

  std::vector<double> out(nFrames);
  osc.ProcessBlock(out.data(), nFrames);

  // the first 1024 cycles (about 3763 samples) are fine...
  const auto earlyFit = extrastest::FitSinusoid(out.data(), 2048, RadiansPerSample(freq, sr));
  REQUIRE(earlyFit.amplitude == Approx(1.).margin(kFastSinTolerance));
  REQUIRE(earlyFit.maxResidual < kFastSinTolerance);

  // ...and the rest of the block should be too
  const auto lateFit = extrastest::FitSinusoid(out.data() + nFrames - 2048, 2048, RadiansPerSample(freq, sr));
  REQUIRE(lateFit.amplitude == Approx(1.).margin(kFastSinTolerance));
  REQUIRE(lateFit.maxResidual < kFastSinTolerance);
}

TEST_CASE("FastSinOscillator: Lookup has a period of 2pi", "[unit][extras][oscillator]")
{
  for (int i = 0; i < 64; i++)
  {
    const double phase = 2. * PI * static_cast<double>(i) / 64.;
    CAPTURE(i);
    REQUIRE(FastSinOscillator<double>::Lookup(phase + 2. * PI) == Approx(FastSinOscillator<double>::Lookup(phase)).margin(1e-3));
    REQUIRE(FastSinOscillator<double>::Lookup(phase + PI) == Approx(-FastSinOscillator<double>::Lookup(phase)).margin(1e-3));
  }
}

// BUG: IPlug/Extras/Oscillator.h FastSinOscillator keeps IOscillator::mPhase in table index units
// (0 to 512) rather than the documented "float phase (goes between 0. and 1.)" (line 50), so the
// inherited SetPhase() means something different to SinOscillator's: SetPhase(0.5) moves it half a
// table entry rather than half a cycle. The constructor works around this by scaling the start phase
// (by tableSizeM1, line 136 - which should be tableSize) but SetPhase() does not.
TEST_CASE("FastSinOscillator: SetPhase is in cycles", "[.][bug]")
{
  FastSinOscillator<double> reference, halfCycle;
  reference.SetFreqCPS(441.);
  halfCycle.SetFreqCPS(441.);

  reference.SetPhase(0.);
  halfCycle.SetPhase(0.5);

  for (int s = 0; s < 100; s++)
  {
    CAPTURE(s);
    REQUIRE(halfCycle.Process() == Approx(-reference.Process()).margin(1e-3));
  }
}
