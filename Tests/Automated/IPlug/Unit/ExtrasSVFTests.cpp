/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for the multi-channel state variable filter SVF (IPlug/Extras/SVF.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <vector>

#include "IPlugConstants.h"
#include "IPlugUtilities.h"
#include "SVF.h"
#include "ExtrasTestUtils.h"

using namespace iplug;
using Catch::Approx;

namespace
{
using Filter = SVF<double, 1>;
using Mode = Filter::EMode;

constexpr int kSettle = 8192;
constexpr int kMeasure = 8192;
const double kButterworthQ = 0.70710678118654752440;

/** Move a frequency to the nearest one with a whole number of cycles in the measurement window, so that RMS is exact */
double SnapToWindow(double freqHz, double sr)
{
  const double bin = std::max(1., std::round(freqHz * kMeasure / sr));
  return bin * sr / kMeasure;
}

void ProcessMono(Filter& filter, std::vector<double>& in, std::vector<double>& out, int blockSize = 512)
{
  const int nFrames = static_cast<int>(in.size());

  for (int start = 0; start < nFrames; start += blockSize)
  {
    double* ins[1] = {in.data() + start};
    double* outs[1] = {out.data() + start};
    filter.ProcessBlock(ins, outs, 1, std::min(blockSize, nFrames - start));
  }
}

/** @return The steady state gain in dB of the filter for a sine at freqHz (which should come from SnapToWindow()) */
double MeasureGainDB(Filter& filter, double freqHz, double sr)
{
  filter.Reset();

  auto in = extrastest::MakeSine<double>(freqHz, sr, kSettle + kMeasure);
  std::vector<double> out(in.size(), 0.);
  ProcessMono(filter, in, out);

  return extrastest::AmpToDB(extrastest::RMS(out.data() + kSettle, kMeasure) / extrastest::RMS(in.data() + kSettle, kMeasure));
}

Filter MakeFilter(Mode mode, double sr, double freq, double Q, double gainDB = 0.)
{
  Filter filter(mode, freq);
  filter.SetSampleRate(sr);
  filter.SetFreqCPS(freq);
  filter.SetQ(Q);
  filter.SetGain(gainDB);
  return filter;
}

/** The textbook response of a 2nd order analogue state variable filter, at the frequency
 *  that the bilinear transform (with the cutoff pre-warped) maps freqHz to */
double ExpectedGainDB(Mode mode, double freqHz, double cutoffHz, double Q, double sr)
{
  const double x = std::tan(PI * freqHz / sr) / std::tan(PI * cutoffHz / sr);
  const std::complex<double> jx(0., x);
  const std::complex<double> denominator = 1. - (x * x) + jx / Q;
  std::complex<double> numerator;

  switch (mode)
  {
    case Filter::kLowPass: numerator = 1.; break;
    case Filter::kHighPass: numerator = -(x * x); break;
    case Filter::kBandPass: numerator = jx; break; // peak gain is Q
    case Filter::kNotch: numerator = 1. - (x * x); break;
    case Filter::kPeak: numerator = -(1. + (x * x)); break; // lowpass minus highpass
    default: return 0.;
  }

  return extrastest::AmpToDB(std::abs(numerator / denominator));
}
} // namespace

TEST_CASE("SVF: lowpass passes lows and attenuates highs", "[unit][extras][svf]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  auto filter = MakeFilter(Filter::kLowPass, sr, 1000., kButterworthQ);

  CAPTURE(sr);
  REQUIRE(MeasureGainDB(filter, SnapToWindow(100., sr), sr) == Approx(0.).margin(0.1));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(1000., sr), sr) == Approx(-3.01).margin(0.2));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(10000., sr), sr) < -38.);

  // 12dB per octave: an octave further up loses a further 12dB or more (more as we approach Nyquist)
  const double at4k = MeasureGainDB(filter, SnapToWindow(4000., sr), sr);
  const double at8k = MeasureGainDB(filter, SnapToWindow(8000., sr), sr);
  REQUIRE(at4k == Approx(-24.).margin(1.5));
  REQUIRE(at8k < at4k - 11.5);
}

TEST_CASE("SVF: highpass passes highs and attenuates lows", "[unit][extras][svf]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  auto filter = MakeFilter(Filter::kHighPass, sr, 1000., kButterworthQ);

  CAPTURE(sr);
  REQUIRE(MeasureGainDB(filter, SnapToWindow(10000., sr), sr) == Approx(0.).margin(0.1));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(1000., sr), sr) == Approx(-3.01).margin(0.2));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(250., sr), sr) == Approx(-24.).margin(1.));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(100., sr), sr) == Approx(-40.).margin(1.));
}

TEST_CASE("SVF: bandpass peaks at the centre frequency and rejects either side", "[unit][extras][svf]")
{
  const double sr = GENERATE(44100., 48000.);
  const double Q = GENERATE(1., 4.);
  auto filter = MakeFilter(Filter::kBandPass, sr, 1000., Q);

  const double centre = MeasureGainDB(filter, SnapToWindow(1000., sr), sr);
  const double below = MeasureGainDB(filter, SnapToWindow(100., sr), sr);
  const double above = MeasureGainDB(filter, SnapToWindow(10000., sr), sr);

  CAPTURE(sr, Q);
  REQUIRE(centre == Approx(extrastest::AmpToDB(Q)).margin(0.3)); // not normalised: the gain at the centre is Q
  REQUIRE(below < centre - 15.);
  REQUIRE(above < centre - 15.);

  // 6dB per octave skirts
  const double at50 = MeasureGainDB(filter, SnapToWindow(50., sr), sr);
  REQUIRE(below - at50 == Approx(6.).margin(0.75));
}

TEST_CASE("SVF: notch removes the centre frequency and passes the rest", "[unit][extras][svf]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  auto filter = MakeFilter(Filter::kNotch, sr, 1000., 2.);

  // exactly at the centre frequency for this one, the settle time takes care of the rest
  const double centre = SnapToWindow(1000., sr);
  filter.SetFreqCPS(centre);

  CAPTURE(sr);
  REQUIRE(MeasureGainDB(filter, centre, sr) < -60.);
  REQUIRE(MeasureGainDB(filter, SnapToWindow(100., sr), sr) == Approx(0.).margin(0.1));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(10000., sr), sr) == Approx(0.).margin(0.1));
}

TEST_CASE("SVF: peak mode is flat away from the centre and resonant at it", "[unit][extras][svf]")
{
  const double sr = GENERATE(44100., 48000.);
  auto filter = MakeFilter(Filter::kPeak, sr, 1000., 4.);

  CAPTURE(sr);
  REQUIRE(MeasureGainDB(filter, SnapToWindow(50., sr), sr) == Approx(0.).margin(0.1));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(15000., sr), sr) == Approx(0.).margin(0.1));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(1000., sr), sr) == Approx(extrastest::AmpToDB(2. * 4.)).margin(0.5));
}

TEST_CASE("SVF: magnitude response matches the analogue prototype", "[unit][extras][svf]")
{
  const double sr = GENERATE(44100., 48000.);
  const Mode mode = GENERATE(Filter::kLowPass, Filter::kHighPass, Filter::kBandPass, Filter::kNotch, Filter::kPeak);
  const double cutoff = GENERATE(200., 3000.);
  const double Q = GENERATE(0.5, 3.);

  auto filter = MakeFilter(mode, sr, cutoff, Q);

  for (double freq : {60., 150., 400., 1100., 2500., 6000., 12000.})
  {
    const double f = SnapToWindow(freq, sr);
    const double expected = ExpectedGainDB(mode, f, cutoff, Q, sr);

    // deep in a stop band or a notch small errors in the measurement dominate
    if (expected < -50.)
      continue;

    CAPTURE(sr, static_cast<int>(mode), cutoff, Q, f);
    REQUIRE(MeasureGainDB(filter, f, sr) == Approx(expected).margin(0.25));
  }
}

TEST_CASE("SVF: resonance at the cutoff follows Q", "[unit][extras][svf]")
{
  const double sr = 48000.;
  const double Q = GENERATE(0.5, 1., 2., 10.);
  const double cutoff = SnapToWindow(2000., sr);

  auto lowpass = MakeFilter(Filter::kLowPass, sr, cutoff, Q);
  auto highpass = MakeFilter(Filter::kHighPass, sr, cutoff, Q);

  CAPTURE(Q);
  REQUIRE(MeasureGainDB(lowpass, cutoff, sr) == Approx(extrastest::AmpToDB(Q)).margin(0.1));
  REQUIRE(MeasureGainDB(highpass, cutoff, sr) == Approx(extrastest::AmpToDB(Q)).margin(0.1));
}

TEST_CASE("SVF: bell boosts or cuts by the gain at the centre frequency only", "[unit][extras][svf]")
{
  const double sr = GENERATE(44100., 48000.);
  const double gainDB = GENERATE(-12., -3., 6., 18.);
  const double centre = SnapToWindow(1000., sr);

  auto filter = MakeFilter(Filter::kBell, sr, centre, 2., gainDB);

  CAPTURE(sr, gainDB);
  REQUIRE(MeasureGainDB(filter, centre, sr) == Approx(gainDB).margin(0.1));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(30., sr), sr) == Approx(0.).margin(0.5));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(18000., sr), sr) == Approx(0.).margin(0.5));

  // somewhere in between on the skirts
  const double skirt = MeasureGainDB(filter, SnapToWindow(700., sr), sr);
  if (gainDB > 0.)
  {
    REQUIRE(skirt > 0.);
    REQUIRE(skirt < gainDB);
  }
  else
  {
    REQUIRE(skirt < 0.);
    REQUIRE(skirt > gainDB);
  }
}

TEST_CASE("SVF: bell with zero gain is transparent", "[unit][extras][svf]")
{
  const double sr = 44100.;
  auto filter = MakeFilter(Filter::kBell, sr, 1000., 1., 0.);

  for (double freq : {100., 1000., 10000.})
    REQUIRE(MeasureGainDB(filter, SnapToWindow(freq, sr), sr) == Approx(0.).margin(0.01));
}

TEST_CASE("SVF: low shelf applies the gain below the cutoff", "[unit][extras][svf]")
{
  const double sr = GENERATE(44100., 48000.);
  const double gainDB = GENERATE(-12., 6., 12.);
  const double cutoff = SnapToWindow(1000., sr);

  auto filter = MakeFilter(Filter::kLowPassShelf, sr, cutoff, kButterworthQ, gainDB);

  CAPTURE(sr, gainDB);
  REQUIRE(MeasureGainDB(filter, SnapToWindow(30., sr), sr) == Approx(gainDB).margin(0.2));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(15000., sr), sr) == Approx(0.).margin(0.2));

  // half the gain (in dB) at the cutoff
  REQUIRE(MeasureGainDB(filter, cutoff, sr) == Approx(gainDB / 2.).margin(0.2));
}

TEST_CASE("SVF: high shelf applies the gain above the cutoff", "[unit][extras][svf]")
{
  const double sr = GENERATE(44100., 48000.);
  const double gainDB = GENERATE(-12., 6., 12.);

  auto filter = MakeFilter(Filter::kHighPassShelf, sr, 1000., kButterworthQ, gainDB);

  CAPTURE(sr, gainDB);
  REQUIRE(MeasureGainDB(filter, SnapToWindow(30., sr), sr) == Approx(0.).margin(0.2));
  REQUIRE(MeasureGainDB(filter, SnapToWindow(15000., sr), sr) == Approx(gainDB).margin(0.2));
}

// BUG: IPlug/Extras/SVF.h:234 (kHighPassShelf in UpdateCoefficients) computes "g = w / std::sqrt(A)",
// copied from the low shelf. In the Cytomic design this filter is based on, the high shelf uses
// g = w * sqrt(A). The shelf gains at DC and Nyquist are still correct, but the transition is
// centred a factor of A away from the cutoff (an octave too low for +12dB, an octave too high for
// -12dB) rather than at the cutoff, where the gain should be half the shelf gain in dB as it is for
// the low shelf.
TEST_CASE("SVF: high shelf transition is centred on the cutoff", "[.][bug]")
{
  const double sr = 48000.;
  const double gainDB = GENERATE(-12., 12.);
  const double cutoff = SnapToWindow(1000., sr);

  auto filter = MakeFilter(Filter::kHighPassShelf, sr, cutoff, kButterworthQ, gainDB);

  CAPTURE(gainDB);
  REQUIRE(MeasureGainDB(filter, cutoff, sr) == Approx(gainDB / 2.).margin(0.2));
}

TEST_CASE("SVF: cutoff frequency is in Hz at any sample rate", "[unit][extras][svf]")
{
  const double sr = GENERATE(44100., 48000., 88200., 96000.);
  const double cutoff = GENERATE(100., 1000., 5000.);

  auto filter = MakeFilter(Filter::kLowPass, sr, SnapToWindow(cutoff, sr), kButterworthQ);

  CAPTURE(sr, cutoff);
  REQUIRE(MeasureGainDB(filter, SnapToWindow(cutoff, sr), sr) == Approx(-3.01).margin(0.05));
}

TEST_CASE("SVF: parameter changes take effect on the next block", "[unit][extras][svf]")
{
  const double sr = 48000.;
  const double freq = SnapToWindow(2000., sr);
  auto filter = MakeFilter(Filter::kLowPass, sr, 200., kButterworthQ);

  REQUIRE(MeasureGainDB(filter, freq, sr) < -35.);

  filter.SetFreqCPS(10000.);
  REQUIRE(MeasureGainDB(filter, freq, sr) == Approx(0.).margin(0.1));

  filter.SetMode(Filter::kHighPass);
  REQUIRE(MeasureGainDB(filter, freq, sr) < -25.);

  filter.SetFreqCPS(freq);
  filter.SetQ(4.);
  REQUIRE(MeasureGainDB(filter, freq, sr) == Approx(extrastest::AmpToDB(4.)).margin(0.1));

  filter.SetMode(Filter::kBell);
  filter.SetGain(-6.);
  REQUIRE(MeasureGainDB(filter, freq, sr) == Approx(-6.).margin(0.1));
}

TEST_CASE("SVF: constructor sets the mode and frequency", "[unit][extras][svf]")
{
  const double sr = 44100.; // the default sample rate of the filter
  const double freq = SnapToWindow(500., sr);

  Filter lowpass(Filter::kLowPass, 5000.);
  lowpass.SetQ(kButterworthQ);
  Filter highpass(Filter::kHighPass, 5000.);
  highpass.SetQ(kButterworthQ);

  REQUIRE(MeasureGainDB(lowpass, freq, sr) == Approx(0.).margin(0.1));
  REQUIRE(MeasureGainDB(highpass, freq, sr) == Approx(-40.).margin(1.));

  // defaults: a 1kHz lowpass (heavily damped, the default Q being the minimum)
  Filter defaults;
  REQUIRE(MeasureGainDB(defaults, SnapToWindow(20., sr), sr) == Approx(0.).margin(0.5));
  REQUIRE(MeasureGainDB(defaults, SnapToWindow(15000., sr), sr) < -40.);
}

TEST_CASE("SVF: out of range parameters are clamped", "[unit][extras][svf]")
{
  const double sr = 48000.;
  constexpr int nFrames = 2048;

  struct Case
  {
    double freq, Q, gain;
    double clampedFreq, clampedQ, clampedGain;
  };

  const Case c = GENERATE(Case{1., 1., 0., 10., 1., 0.},
                          Case{-100., 1., 0., 10., 1., 0.},
                          Case{1e6, 1., 0., 20000., 1., 0.},
                          Case{1000., 0., 0., 1000., 0.1, 0.},
                          Case{1000., -5., 0., 1000., 0.1, 0.},
                          Case{1000., 1e6, 0., 1000., 100., 0.},
                          Case{1000., 1., 500., 1000., 1., 36.},
                          Case{1000., 1., -500., 1000., 1., -36.});

  const Mode mode = GENERATE(Filter::kLowPass, Filter::kBell, Filter::kHighPassShelf);

  auto unclamped = MakeFilter(mode, sr, c.freq, c.Q, c.gain);
  auto clamped = MakeFilter(mode, sr, c.clampedFreq, c.clampedQ, c.clampedGain);

  auto in = extrastest::MakeNoise<double>(nFrames, 4242u, 0.5);
  std::vector<double> outA(nFrames, 0.), outB(nFrames, 0.);
  ProcessMono(unclamped, in, outA);
  ProcessMono(clamped, in, outB);

  REQUIRE(extrastest::AllFinite(outA.data(), nFrames));
  REQUIRE(extrastest::MaxAbsDiff(outA.data(), outB.data(), nFrames) == 0.);
}

TEST_CASE("SVF: stable and bounded at the extremes of the parameter ranges", "[unit][extras][svf]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const double freq = GENERATE(10., 20000.);
  const double Q = GENERATE(0.1, 100.);
  const double gainDB = GENERATE(-36., 36.);
  constexpr int nFrames = 16384;

  auto in = extrastest::MakeNoise<double>(nFrames, 1234u);
  std::vector<double> out(nFrames, 0.);

  for (int m = 0; m < Filter::kNumModes; m++)
  {
    auto filter = MakeFilter(static_cast<Mode>(m), sr, freq, Q, gainDB);
    ProcessMono(filter, in, out);

    CAPTURE(sr, freq, Q, gainDB, m);
    REQUIRE(extrastest::AllFinite(out.data(), nFrames));

    // the largest gain available is the +36dB of the shelves (a factor of 63), or a resonance of Q = 100
    // on top of which a bell can add its gain
    REQUIRE(extrastest::PeakAbs(out.data(), nFrames) < 1e4);
  }
}

TEST_CASE("SVF: survives parameters being swept while processing", "[unit][extras][svf]")
{
  const double sr = 44100.;
  constexpr int nBlocks = 400;
  constexpr int blockSize = 32;

  extrastest::LCG lcg(99u);
  Filter filter;
  filter.SetSampleRate(sr);

  auto in = extrastest::MakeNoise<double>(nBlocks * blockSize, 5678u, 0.25);
  std::vector<double> out(in.size(), 0.);

  for (int b = 0; b < nBlocks; b++)
  {
    filter.SetMode(static_cast<Mode>(lcg.NextInt() % static_cast<uint32_t>(Filter::kNumModes)));
    filter.SetFreqCPS(20. * std::pow(1000., lcg.NextUnipolar()));
    filter.SetQ(0.1 + lcg.NextUnipolar() * 10.);
    filter.SetGain(lcg.NextBipolar() * 24.);

    double* ins[1] = {in.data() + b * blockSize};
    double* outs[1] = {out.data() + b * blockSize};
    filter.ProcessBlock(ins, outs, 1, blockSize);
  }

  REQUIRE(extrastest::AllFinite(out.data(), nBlocks * blockSize));
  REQUIRE(extrastest::PeakAbs(out.data(), nBlocks * blockSize) < 1e4);
}

TEST_CASE("SVF: silence in, silence out", "[unit][extras][svf]")
{
  for (int m = 0; m < Filter::kNumModes; m++)
  {
    auto filter = MakeFilter(static_cast<Mode>(m), 48000., 1000., 5., 12.);
    std::vector<double> in(1024, 0.), out(1024, 1.);
    ProcessMono(filter, in, out);
    REQUIRE(extrastest::PeakAbs(out.data(), 1024) == 0.);
  }
}

TEST_CASE("SVF: Reset clears the filter state", "[unit][extras][svf]")
{
  constexpr int nFrames = 1024;
  auto filter = MakeFilter(Filter::kBandPass, 48000., 500., 20.);

  auto noise = extrastest::MakeNoise<double>(nFrames, 8u);
  std::vector<double> silence(nFrames, 0.), first(nFrames, 0.), tail(nFrames, 0.), second(nFrames, 0.);

  ProcessMono(filter, noise, first);

  SECTION("without a reset the filter rings on")
  {
    ProcessMono(filter, silence, tail);
    REQUIRE(extrastest::PeakAbs(tail.data(), nFrames) > 1e-3);
  }

  SECTION("after a reset there is nothing left")
  {
    filter.Reset();
    ProcessMono(filter, silence, tail);
    REQUIRE(extrastest::PeakAbs(tail.data(), nFrames) == 0.);
  }

  SECTION("after a reset the filter repeats itself exactly")
  {
    filter.Reset();
    ProcessMono(filter, noise, second);
    REQUIRE(extrastest::MaxAbsDiff(first.data(), second.data(), nFrames) == 0.);
  }
}

TEST_CASE("SVF: result does not depend on the block size", "[unit][extras][svf]")
{
  constexpr int nFrames = 4096;
  const Mode mode = GENERATE(Filter::kLowPass, Filter::kPeak, Filter::kLowPassShelf);
  const int blockSize = GENERATE(1, 7, 64, 1000);

  auto reference = MakeFilter(mode, 44100., 800., 3., 9.);
  auto filter = MakeFilter(mode, 44100., 800., 3., 9.);

  auto in = extrastest::MakeNoise<double>(nFrames, 21u);
  std::vector<double> outRef(nFrames, 0.), out(nFrames, 0.);

  ProcessMono(reference, in, outRef, nFrames);
  ProcessMono(filter, in, out, blockSize);

  REQUIRE(extrastest::MaxAbsDiff(outRef.data(), out.data(), nFrames) < 1e-12);
}

TEST_CASE("SVF: works in place", "[unit][extras][svf]")
{
  constexpr int nFrames = 2048;
  auto reference = MakeFilter(Filter::kHighPass, 44100., 800., 2.);
  auto filter = MakeFilter(Filter::kHighPass, 44100., 800., 2.);

  auto in = extrastest::MakeNoise<double>(nFrames, 22u);
  std::vector<double> outRef(nFrames, 0.);
  ProcessMono(reference, in, outRef);

  std::vector<double> buf = in;
  ProcessMono(filter, buf, buf);

  REQUIRE(extrastest::MaxAbsDiff(outRef.data(), buf.data(), nFrames) < 1e-12);
}

TEST_CASE("SVF: channels are filtered independently with shared settings", "[unit][extras][svf]")
{
  constexpr int nFrames = 2048;
  constexpr double kSentinel = 77.;

  SVF<double, 3> multi(SVF<double, 3>::kLowPass, 2000.);
  multi.SetSampleRate(48000.);
  multi.SetQ(5.);

  auto mono = MakeFilter(Filter::kLowPass, 48000., 2000., 5.);

  extrastest::Buffers<double> in(3, nFrames), out(3, nFrames, kSentinel);
  in[0] = extrastest::MakeNoise<double>(nFrames, 23u);
  // channel 1 is silent
  in[2] = extrastest::MakeSine<double>(2000., 48000., nFrames);

  SECTION("all channels")
  {
    multi.ProcessBlock(in.Get(), out.Get(), 3, nFrames);

    std::vector<double> ref(nFrames, 0.);
    ProcessMono(mono, in[0], ref);
    REQUIRE(extrastest::MaxAbsDiff(out[0].data(), ref.data(), nFrames) < 1e-12);

    REQUIRE(extrastest::PeakAbs(out[1].data(), nFrames) == 0.);

    mono.Reset();
    ProcessMono(mono, in[2], ref);
    REQUIRE(extrastest::MaxAbsDiff(out[2].data(), ref.data(), nFrames) < 1e-12);
  }

  SECTION("a subset of the channels")
  {
    multi.ProcessBlock(in.Get(), out.Get(), 1, nFrames);

    for (size_t s = 0; s < nFrames; s++)
    {
      REQUIRE(out[1][s] == kSentinel);
      REQUIRE(out[2][s] == kSentinel);
    }

    // the state of the channels that were skipped is still clean
    multi.ProcessBlock(in.Get(), out.Get(), 3, nFrames);
    std::vector<double> ref(nFrames, 0.);
    ProcessMono(mono, in[2], ref);
    REQUIRE(extrastest::MaxAbsDiff(out[2].data(), ref.data(), nFrames) < 1e-12);
  }
}

TEST_CASE("SVF: single precision samples", "[unit][extras][svf]")
{
  const double sr = 48000.;
  constexpr int nFrames = kSettle + kMeasure;

  SVF<float, 2> filter(SVF<float, 2>::kLowPass, 1000.);
  filter.SetSampleRate(sr);
  filter.SetQ(kButterworthQ);

  extrastest::Buffers<float> in(2, nFrames), out(2, nFrames);
  in[0] = extrastest::MakeSine<float>(SnapToWindow(100., sr), sr, nFrames);
  in[1] = extrastest::MakeSine<float>(SnapToWindow(8000., sr), sr, nFrames);

  filter.ProcessBlock(in.Get(), out.Get(), 2, nFrames);

  REQUIRE(extrastest::AllFinite(out[0].data(), nFrames));
  REQUIRE(extrastest::AllFinite(out[1].data(), nFrames));

  const double lowGain = extrastest::AmpToDB(extrastest::RMS(out[0].data() + kSettle, kMeasure) * std::sqrt(2.));
  const double highGain = extrastest::AmpToDB(extrastest::RMS(out[1].data() + kSettle, kMeasure) * std::sqrt(2.));

  REQUIRE(lowGain == Approx(0.).margin(0.1));
  REQUIRE(highGain == Approx(ExpectedGainDB(Filter::kLowPass, SnapToWindow(8000., sr), 1000., kButterworthQ, sr)).margin(0.25));
}

// BUG: IPlug/Extras/SVF.h:52 SVF::PlotResponse() does not return the magnitude response in dB:
//  - line 64 computes the frequency as pow(10, x * (maxLogHz - minLogHz) + minHz), adding minHz
//    where it should add minLogHz, so the frequency axis is off by a factor of 10 with the defaults
//  - line 81 takes std::abs(std::real(result)) rather than std::abs(result), so e.g. a lowpass at its
//    cutoff, where the response is purely imaginary, comes out as -infinity rather than 20log10(Q)
//  - minLogHz/maxLogHz are function-local statics, so the minHz/maxHz of the first ever call are
//    used for every later call
TEST_CASE("SVF: PlotResponse returns the magnitude response in dB", "[.][bug]")
{
  const double cutoff = 1000.;
  const double minHz = 1.;
  const double maxHz = 20000.;

  // x is the normalised position on a log frequency axis from minHz to maxHz
  const auto xForFreq = [=](double freq) { return std::log10(freq / minHz) / std::log10(maxHz / minHz); };

  // lowpass: flat in the pass band, 20log10(Q) at the cutoff, -12dB per octave above it
  REQUIRE(Filter::PlotResponse(cutoff, kButterworthQ, Filter::kLowPass, xForFreq(10.), 0., minHz, maxHz) == Approx(0.).margin(0.1));
  REQUIRE(Filter::PlotResponse(cutoff, kButterworthQ, Filter::kLowPass, xForFreq(1000.), 0., minHz, maxHz) == Approx(-3.01).margin(0.1));
  REQUIRE(Filter::PlotResponse(cutoff, 2., Filter::kLowPass, xForFreq(1000.), 0., minHz, maxHz) == Approx(6.02).margin(0.1));
  REQUIRE(Filter::PlotResponse(cutoff, kButterworthQ, Filter::kLowPass, xForFreq(10000.), 0., minHz, maxHz) == Approx(-40.).margin(0.1));

  // highpass mirrors it
  REQUIRE(Filter::PlotResponse(cutoff, kButterworthQ, Filter::kHighPass, xForFreq(100.), 0., minHz, maxHz) == Approx(-40.).margin(0.1));
  REQUIRE(Filter::PlotResponse(cutoff, kButterworthQ, Filter::kHighPass, xForFreq(1000.), 0., minHz, maxHz) == Approx(-3.01).margin(0.1));
}
