/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for LanczosResampler and RealtimeResampler (IPlug/Extras/LanczosResampler.h, RealtimeResampler.h)

// RealtimeResampler.h does not compile with GCC before 13 ("declaration of 'using LanczosResampler = ...' changes
// meaning of 'LanczosResampler'"), so these tests are left out there
#if !defined(__GNUC__) || defined(__clang__) || __GNUC__ >= 13

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>

#include "IPlugConstants.h"
#include "wdltypes.h" // LanczosResampler.h uses WDL_FIXALIGN without including anything that defines it
#include "LanczosResampler.h"
#include "RealtimeResampler.h"
#include "ExtrasTestUtils.h"

using namespace iplug;
using Catch::Approx;

namespace
{
constexpr int kLanczosA = 12;

// NOTE: these objects are large (the Lanczos resampler holds its 2 x 8192 sample history per
// channel by value), so they are always created on the heap rather than on the test's stack
using Lanczos = LanczosResampler<double, 2, kLanczosA>;
using Realtime = RealtimeResampler<double, 2, kLanczosA>;

struct Rates
{
  double in, out;
};

/** Stream a stereo signal through a LanczosResampler, pushing blockSize frames at a time and popping everything available */
std::vector<std::vector<double>> Resample(Lanczos& resampler, const std::vector<double>& left, const std::vector<double>& right, int blockSize, bool renormalize = true)
{
  std::vector<double> inL = left, inR = right;
  std::vector<std::vector<double>> out(2);
  const int nFrames = static_cast<int>(inL.size());

  constexpr int kMaxPop = 8192;
  std::vector<double> popL(kMaxPop), popR(kMaxPop);

  for (int start = 0; start < nFrames; start += blockSize)
  {
    double* ins[2] = {inL.data() + start, inR.data() + start};
    resampler.PushBlock(ins, static_cast<size_t>(std::min(blockSize, nFrames - start)), 2);

    double* outs[2] = {popL.data(), popR.data()};
    const size_t populated = resampler.PopBlock(outs, kMaxPop, 2);

    out[0].insert(out[0].end(), popL.begin(), popL.begin() + static_cast<std::ptrdiff_t>(populated));
    out[1].insert(out[1].end(), popR.begin(), popR.begin() + static_cast<std::ptrdiff_t>(populated));

    if (renormalize)
      resampler.RenormalizePhases();
  }

  return out;
}

using BlockFunc = Realtime::BlockProcessFunc;

void Passthrough(double** inputs, double** outputs, int nFrames, int nChans)
{
  for (int c = 0; c < nChans; c++)
    std::memcpy(outputs[c], inputs[c], static_cast<size_t>(nFrames) * sizeof(double));
}

/** Run a stereo signal through a RealtimeResampler in blocks of blockSize */
std::vector<std::vector<double>> Process(Realtime& resampler, const std::vector<double>& left, const std::vector<double>& right, int blockSize, BlockFunc func)
{
  std::vector<double> inL = left, inR = right;
  const int nFrames = static_cast<int>(inL.size());
  std::vector<std::vector<double>> out(2, std::vector<double>(inL.size(), 0.));

  for (int start = 0; start < nFrames; start += blockSize)
  {
    double* ins[2] = {inL.data() + start, inR.data() + start};
    double* outs[2] = {out[0].data() + start, out[1].data() + start};
    resampler.ProcessBlock(ins, outs, std::min(blockSize, nFrames - start), 2, func);
  }

  return out;
}

double RadiansPerSample(double freqHz, double sr)
{
  return 2. * PI * freqHz / sr;
}

struct RoundTripDelay
{
  int reported;            // GetLatency()
  double measured;         // the delay (in samples at the outer rate) that best lines the output up with the input
  double fitError;         // the largest difference between the output and the input delayed by that
  double peakBeforeSignal; // the output level well before the reported latency
};

/** Pass a sine through a Lanczos mode RealtimeResampler with a pass through process function and measure the delay */
RoundTripDelay MeasureRoundTripDelay(double outerRate, double innerRate, int blockSize)
{
  const double freq = 1000.;
  constexpr int nFrames = 8192;

  Realtime resampler(innerRate, Realtime::ESRCMode::kLancsoz);
  resampler.Reset(outerRate, blockSize);

  RoundTripDelay result {resampler.GetLatency(), 0., 1e9, 0.};

  const auto in = extrastest::MakeSine<double>(freq, outerRate, nFrames);
  const auto out = Process(resampler, in, in, blockSize, Passthrough);

  // search within +/-3 samples of the reported latency, in steps of 1/100 sample
  for (int i = -300; i <= 300; i++)
  {
    const double delay = result.reported + static_cast<double>(i) * 0.01;
    double error = 0.;

    for (int s = nFrames / 2; s < nFrames; s++)
      error = std::max(error, std::fabs(out[0][static_cast<size_t>(s)] - std::sin(RadiansPerSample(freq, outerRate) * (static_cast<double>(s) - delay))));

    if (error < result.fitError)
    {
      result.fitError = error;
      result.measured = delay;
    }
  }

  result.peakBeforeSignal = extrastest::PeakAbs(out[0].data(), std::max(result.reported - 8, 0));

  return result;
}
} // namespace

// ----------------------------------------------------------------------------
// LanczosResampler
// ----------------------------------------------------------------------------

TEST_CASE("LanczosResampler: a 1:1 ratio reproduces the input after a short delay", "[unit][extras][resampler]")
{
  const double sr = GENERATE(44100., 48000.);
  constexpr int nFrames = 2048;

  auto pResampler = std::make_unique<Lanczos>(static_cast<float>(sr), static_cast<float>(sr));

  const auto left = extrastest::MakeNoise<double>(nFrames, 1u);
  const auto right = extrastest::MakeNoise<double>(nFrames, 2u);
  const auto out = Resample(*pResampler, left, right, 128);

  // everything bar the filter's look ahead comes out
  const int nOut = static_cast<int>(out[0].size());
  REQUIRE(nOut <= nFrames);
  REQUIRE(nOut >= nFrames - (kLanczosA + 2));
  REQUIRE(out[1].size() == out[0].size());

  // find the (whole sample) delay
  int bestDelay = -1;
  double bestError = 1e9;

  for (int delay = 0; delay <= 2; delay++)
  {
    const double error = extrastest::MaxAbsDiff(out[0].data() + delay, left.data(), nOut - delay);
    if (error < bestError)
    {
      bestError = error;
      bestDelay = delay;
    }
  }

  CAPTURE(sr, bestDelay);
  REQUIRE(bestError < 1e-9);
  REQUIRE(extrastest::MaxAbsDiff(out[1].data() + bestDelay, right.data(), nOut - bestDelay) < 1e-9);
}

TEST_CASE("LanczosResampler: GetNumSamplesRequiredFor says how much input an output block needs", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.}, Rates{96000., 44100.}, Rates{44100., 96000.}, Rates{48000., 48000.});
  const int nOutput = GENERATE(1, 64, 512);

  auto pResampler = std::make_unique<Lanczos>(static_cast<float>(rates.in), static_cast<float>(rates.out));

  const size_t required = pResampler->GetNumSamplesRequiredFor(static_cast<size_t>(nOutput));

  CAPTURE(rates.in, rates.out, nOutput, required);

  // the output block's worth of input, plus the look ahead of the filter
  const double ideal = static_cast<double>(nOutput) * rates.in / rates.out;
  REQUIRE(static_cast<double>(required) >= ideal);
  REQUIRE(static_cast<double>(required) <= ideal + kLanczosA + 3);

  std::vector<double> in(required, 0.25), outL(static_cast<size_t>(nOutput) + 8), outR(static_cast<size_t>(nOutput) + 8);
  double* ins[2] = {in.data(), in.data()};
  double* outs[2] = {outL.data(), outR.data()};

  // one sample short is not enough...
  pResampler->PushBlock(ins, required - 1, 2);
  REQUIRE(pResampler->GetNumSamplesRequiredFor(static_cast<size_t>(nOutput)) >= 1);

  // ...with that one sample it is
  pResampler->PushBlock(ins, 1, 2);
  REQUIRE(pResampler->GetNumSamplesRequiredFor(static_cast<size_t>(nOutput)) == 0);
  REQUIRE(pResampler->PopBlock(outs, static_cast<size_t>(nOutput), 2) == static_cast<size_t>(nOutput));

  // once everything available has been popped, more input is needed again
  while (pResampler->PopBlock(outs, 1, 2) > 0) {}
  REQUIRE(pResampler->GetNumSamplesRequiredFor(1) >= 1);
}

TEST_CASE("LanczosResampler: PopBlock never returns more than asked for", "[unit][extras][resampler]")
{
  auto pResampler = std::make_unique<Lanczos>(44100.f, 48000.f);

  std::vector<double> in(1000, 0.5), outL(1000, 0.), outR(1000, 0.);
  double* ins[2] = {in.data(), in.data()};
  double* outs[2] = {outL.data(), outR.data()};

  pResampler->PushBlock(ins, 1000, 2);

  REQUIRE(pResampler->PopBlock(outs, 0, 2) == 0);
  REQUIRE(pResampler->PopBlock(outs, 10, 2) == 10);
  REQUIRE(pResampler->PopBlock(outs, 100, 2) == 100);

  // and stops when it runs out of input
  size_t total = 110;
  size_t populated = 0;
  do
  {
    populated = pResampler->PopBlock(outs, 1000, 2);
    total += populated;
  } while (populated > 0);

  REQUIRE(static_cast<double>(total) == Approx((1000. - (kLanczosA + 1)) * 48000. / 44100.).margin(2.));
  REQUIRE(pResampler->PopBlock(outs, 1000, 2) == 0);
}

TEST_CASE("LanczosResampler: output length follows the ratio of the sample rates", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.}, Rates{44100., 96000.}, Rates{96000., 44100.}, Rates{44100., 22050.}, Rates{48000., 96000.});
  const int blockSize = GENERATE(64, 441);
  constexpr int nFrames = 20000;

  auto pResampler = std::make_unique<Lanczos>(static_cast<float>(rates.in), static_cast<float>(rates.out));

  const std::vector<double> in(nFrames, 0.);
  const auto out = Resample(*pResampler, in, in, blockSize);

  CAPTURE(rates.in, rates.out, blockSize);
  const double expected = (nFrames - (kLanczosA + 1)) * rates.out / rates.in;
  REQUIRE(static_cast<double>(out[0].size()) == Approx(expected).margin(2.));
}

TEST_CASE("LanczosResampler: a sine keeps its frequency and amplitude", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.}, Rates{44100., 96000.}, Rates{96000., 44100.}, Rates{96000., 48000.});
  const double freq = GENERATE(100., 1000., 5000.);
  constexpr int nFrames = 16384;

  auto pResampler = std::make_unique<Lanczos>(static_cast<float>(rates.in), static_cast<float>(rates.out));

  const auto left = extrastest::MakeSine<double>(freq, rates.in, nFrames);
  const auto right = extrastest::MakeSine<double>(freq, rates.in, nFrames, 0.25);
  const auto out = Resample(*pResampler, left, right, 256);

  // skip the start (where the filter runs onto the signal) and fit the rest
  const int nSkip = 256;
  const int nFit = std::min(static_cast<int>(out[0].size()) - nSkip, 8192);
  REQUIRE(nFit > 4096);

  const auto fitL = extrastest::FitSinusoid(out[0].data() + nSkip, nFit, RadiansPerSample(freq, rates.out));
  const auto fitR = extrastest::FitSinusoid(out[1].data() + nSkip, nFit, RadiansPerSample(freq, rates.out));

  CAPTURE(rates.in, rates.out, freq);
  REQUIRE(fitL.amplitude == Approx(1.).margin(2e-3));
  REQUIRE(fitL.maxResidual < 2e-3);
  REQUIRE(fitR.amplitude == Approx(0.25).margin(5e-4));
  REQUIRE(fitR.maxResidual < 5e-4);
}

TEST_CASE("LanczosResampler: the output is the input signal sampled at the new rate with a small constant delay", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.}, Rates{48000., 96000.});
  const double freq = 1000.;
  constexpr int nFrames = 8192;

  auto pResampler = std::make_unique<Lanczos>(static_cast<float>(rates.in), static_cast<float>(rates.out));

  const auto in = extrastest::MakeSine<double>(freq, rates.in, nFrames);
  const auto out = Resample(*pResampler, in, in, 256);

  // output sample k is the input at time k / outRate, less a delay of a whole number of input samples
  int bestDelay = -1;
  double bestError = 1e9;

  for (int delay = 0; delay <= 2; delay++)
  {
    double error = 0.;

    for (size_t k = 256; k < out[0].size(); k++)
    {
      const double t = static_cast<double>(k) / rates.out - static_cast<double>(delay) / rates.in;
      error = std::max(error, std::fabs(out[0][k] - std::sin(2. * PI * freq * t)));
    }

    if (error < bestError)
    {
      bestError = error;
      bestDelay = delay;
    }
  }

  CAPTURE(rates.in, rates.out, bestDelay);
  REQUIRE(bestError < 2e-3);
}

TEST_CASE("LanczosResampler: phase renormalisation does not change the output", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.});
  constexpr int nFrames = 12000; // more than the 4096 sample history, so the buffer wraps

  auto pWith = std::make_unique<Lanczos>(static_cast<float>(rates.in), static_cast<float>(rates.out));
  auto pWithout = std::make_unique<Lanczos>(static_cast<float>(rates.in), static_cast<float>(rates.out));

  const auto in = extrastest::MakeNoise<double>(nFrames, 3u);
  const auto with = Resample(*pWith, in, in, 300, true);
  const auto without = Resample(*pWithout, in, in, 300, false);

  REQUIRE(with[0].size() == without[0].size());
  REQUIRE(extrastest::MaxAbsDiff(with[0].data(), without[0].data(), static_cast<int>(with[0].size())) < 1e-9);
}

TEST_CASE("LanczosResampler: stays clean over a long stream", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.});
  const double freq = 440.;
  constexpr int nFrames = 60000;

  auto pResampler = std::make_unique<Lanczos>(static_cast<float>(rates.in), static_cast<float>(rates.out));

  const auto in = extrastest::MakeSine<double>(freq, rates.in, nFrames);
  const auto out = Resample(*pResampler, in, in, 512);

  const int nOut = static_cast<int>(out[0].size());
  REQUIRE(nOut > 50000);

  // one fit across (most of) the stream: any glitch or drift would show up as a residual
  const auto fit = extrastest::FitSinusoid(out[0].data() + 256, nOut - 256, RadiansPerSample(freq, rates.out));
  REQUIRE(fit.amplitude == Approx(1.).margin(2e-3));
  REQUIRE(fit.maxResidual < 2e-3);
}

TEST_CASE("LanczosResampler: Reset discards the buffered signal", "[unit][extras][resampler]")
{
  auto pResampler = std::make_unique<Lanczos>(44100.f, 48000.f);

  std::vector<double> in(1024, 1.), outL(4096, 99.), outR(4096, 99.);
  double* ins[2] = {in.data(), in.data()};
  double* outs[2] = {outL.data(), outR.data()};

  pResampler->PushBlock(ins, 1024, 2);

  SECTION("without a reset the signal is there")
  {
    const size_t populated = pResampler->PopBlock(outs, 4096, 2);
    REQUIRE(populated > 1000);
    REQUIRE(extrastest::PeakAbs(outL.data(), static_cast<int>(populated)) > 0.9);
  }

  SECTION("Reset")
  {
    pResampler->Reset();
    const size_t populated = pResampler->PopBlock(outs, 4096, 2);
    REQUIRE(extrastest::PeakAbs(outL.data(), static_cast<int>(populated)) == 0.);
    REQUIRE(extrastest::PeakAbs(outR.data(), static_cast<int>(populated)) == 0.);
  }

  SECTION("ClearBuffer")
  {
    pResampler->ClearBuffer();
    const size_t populated = pResampler->PopBlock(outs, 4096, 2);
    REQUIRE(extrastest::PeakAbs(outL.data(), static_cast<int>(populated)) == 0.);
    REQUIRE(extrastest::PeakAbs(outR.data(), static_cast<int>(populated)) == 0.);
  }
}

TEST_CASE("LanczosResampler: channels are independent", "[unit][extras][resampler]")
{
  constexpr int nFrames = 4096;
  auto pStereo = std::make_unique<Lanczos>(48000.f, 44100.f);
  auto pReference = std::make_unique<Lanczos>(48000.f, 44100.f);

  const auto noise = extrastest::MakeNoise<double>(nFrames, 4u);
  const std::vector<double> silence(nFrames, 0.);

  const auto out = Resample(*pStereo, noise, silence, 256);
  const auto swapped = Resample(*pReference, silence, noise, 256);

  REQUIRE(extrastest::PeakAbs(out[0].data(), static_cast<int>(out[0].size())) > 0.5);
  REQUIRE(extrastest::PeakAbs(out[1].data(), static_cast<int>(out[1].size())) == 0.);
  REQUIRE(extrastest::PeakAbs(swapped[0].data(), static_cast<int>(swapped[0].size())) == 0.);
  REQUIRE(extrastest::MaxAbsDiff(out[0].data(), swapped[1].data(), static_cast<int>(out[0].size())) == 0.);
}

TEST_CASE("LanczosResampler: processing one channel of two leaves the other output alone", "[unit][extras][resampler]")
{
  auto pResampler = std::make_unique<Lanczos>(44100.f, 48000.f);

  std::vector<double> in(512, 1.), outL(1024, 0.), outR(1024, 99.);
  double* ins[2] = {in.data(), in.data()};
  double* outs[2] = {outL.data(), outR.data()};

  pResampler->PushBlock(ins, 512, 1);
  const size_t populated = pResampler->PopBlock(outs, 1024, 1);

  REQUIRE(populated > 500);
  REQUIRE(outL[populated - 1] == Approx(1.).margin(1e-3));
  for (double v : outR)
    REQUIRE(v == 99.);
}

TEST_CASE("LanczosResampler: single precision", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.});
  const double freq = 1000.;
  constexpr int nFrames = 8192;

  auto pResampler = std::make_unique<LanczosResampler<float, 2, kLanczosA>>(static_cast<float>(rates.in), static_cast<float>(rates.out));

  auto in = extrastest::MakeSine<float>(freq, rates.in, nFrames);
  std::vector<float> out, pop(8192);

  for (int start = 0; start < nFrames; start += 256)
  {
    float* ins[2] = {in.data() + start, in.data() + start};
    float* outs[2] = {pop.data(), pop.data()};
    pResampler->PushBlock(ins, 256, 1);
    const size_t populated = pResampler->PopBlock(outs, 8192, 1);
    out.insert(out.end(), pop.begin(), pop.begin() + static_cast<std::ptrdiff_t>(populated));
    pResampler->RenormalizePhases();
  }

  const int nOut = static_cast<int>(out.size());
  REQUIRE(static_cast<double>(nOut) == Approx((nFrames - (kLanczosA + 1)) * rates.out / rates.in).margin(2.));

  const auto fit = extrastest::FitSinusoid(out.data() + 256, nOut - 256, RadiansPerSample(freq, rates.out));
  REQUIRE(extrastest::AllFinite(out.data(), nOut));
  REQUIRE(fit.amplitude == Approx(1.).margin(2e-3));
  REQUIRE(fit.maxResidual < 2e-3);
}

// ----------------------------------------------------------------------------
// RealtimeResampler
// ----------------------------------------------------------------------------

TEST_CASE("RealtimeResampler: equal rates call the process function directly", "[unit][extras][resampler]")
{
  const double sr = GENERATE(44100., 48000.);
  const auto mode = GENERATE(Realtime::ESRCMode::kLancsoz, Realtime::ESRCMode::kLinearInterpolation);
  constexpr int nFrames = 1024;
  constexpr int blockSize = 128;

  Realtime resampler(sr, mode);
  resampler.Reset(sr, blockSize);

  int nCalls = 0;
  int nFramesProcessed = 0;
  int nChansSeen = 0;
  const auto func = [&](double** inputs, double** outputs, int n, int nChans) {
    nCalls++;
    nFramesProcessed += n;
    nChansSeen = nChans;
    for (int c = 0; c < nChans; c++)
      for (int s = 0; s < n; s++)
        outputs[c][s] = inputs[c][s] * 0.5;
  };

  const auto left = extrastest::MakeNoise<double>(nFrames, 5u);
  const auto right = extrastest::MakeNoise<double>(nFrames, 6u);
  const auto out = Process(resampler, left, right, blockSize, func);

  REQUIRE(nCalls == nFrames / blockSize);
  REQUIRE(nFramesProcessed == nFrames);
  REQUIRE(nChansSeen == 2);

  double maxError = 0.;
  for (size_t s = 0; s < nFrames; s++)
  {
    maxError = std::max(maxError, std::fabs(out[0][s] - 0.5 * left[s]));
    maxError = std::max(maxError, std::fabs(out[1][s] - 0.5 * right[s]));
  }
  REQUIRE(maxError == 0.);
}

TEST_CASE("RealtimeResampler: the process function runs at the inner sample rate", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.}, Rates{96000., 44100.}, Rates{44100., 96000.}, Rates{88200., 48000.});
  const auto mode = GENERATE(Realtime::ESRCMode::kLancsoz, Realtime::ESRCMode::kLinearInterpolation);
  const int blockSize = GENERATE(64, 512);
  constexpr int nBlocks = 40;

  const double outerRate = rates.in;
  const double innerRate = rates.out;

  Realtime resampler(innerRate, mode);
  resampler.Reset(outerRate, blockSize);

  const int maxInnerFrames = static_cast<int>(std::ceil(blockSize * innerRate / outerRate));
  int nInnerFrames = 0;
  int maxFramesPerCall = 0;
  bool channelCountOK = true;

  const auto func = [&](double** inputs, double** outputs, int n, int nChans) {
    nInnerFrames += n;
    maxFramesPerCall = std::max(maxFramesPerCall, n);
    if (nChans != 2)
      channelCountOK = false;
    Passthrough(inputs, outputs, n, nChans);
  };

  const std::vector<double> in(static_cast<size_t>(blockSize * nBlocks), 0.1);
  const auto out = Process(resampler, in, in, blockSize, func);

  CAPTURE(outerRate, innerRate, static_cast<int>(mode), blockSize);
  REQUIRE(channelCountOK);

  // never more than the inner equivalent of the maximum block size in one go
  REQUIRE(maxFramesPerCall <= maxInnerFrames);

  // and over time, the ratio of the rates
  const double expected = static_cast<double>(blockSize * nBlocks) * innerRate / outerRate;
  REQUIRE(static_cast<double>(nInnerFrames) == Approx(expected).margin(nBlocks + 32.));

  REQUIRE(extrastest::AllFinite(out[0].data(), blockSize * nBlocks));
}

TEST_CASE("RealtimeResampler: Lanczos round trip keeps a sine intact", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.}, Rates{96000., 44100.}, Rates{96000., 48000.}, Rates{44100., 96000.});
  const double freq = GENERATE(200., 1000., 4000.);
  const int blockSize = GENERATE(64, 512);
  constexpr int nFrames = 16384;

  const double outerRate = rates.in;
  const double innerRate = rates.out;

  Realtime resampler(innerRate, Realtime::ESRCMode::kLancsoz);
  resampler.Reset(outerRate, blockSize);

  const auto left = extrastest::MakeSine<double>(freq, outerRate, nFrames);
  const auto right = extrastest::MakeSine<double>(freq, outerRate, nFrames, 0.5);
  const auto out = Process(resampler, left, right, blockSize, Passthrough);

  const auto fitL = extrastest::FitSinusoid(out[0].data() + nFrames / 2, nFrames / 2, RadiansPerSample(freq, outerRate));
  const auto fitR = extrastest::FitSinusoid(out[1].data() + nFrames / 2, nFrames / 2, RadiansPerSample(freq, outerRate));

  CAPTURE(outerRate, innerRate, freq, blockSize);
  REQUIRE(extrastest::AllFinite(out[0].data(), nFrames));
  REQUIRE(fitL.amplitude == Approx(1.).margin(5e-3));
  REQUIRE(fitL.maxResidual < 5e-3);
  REQUIRE(fitR.amplitude == Approx(0.5).margin(2.5e-3));
  REQUIRE(fitR.maxResidual < 2.5e-3);
}

TEST_CASE("RealtimeResampler: GetLatency reports the delay of the Lanczos round trip", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.}, Rates{96000., 44100.}, Rates{96000., 48000.}, Rates{44100., 96000.}, Rates{44100., 88200.});
  const int blockSize = GENERATE(64, 512);

  const auto delay = MeasureRoundTripDelay(rates.in, rates.out, blockSize);

  CAPTURE(rates.in, rates.out, blockSize, delay.reported, delay.measured);
  REQUIRE(delay.reported > 0);
  REQUIRE(delay.reported < 4 * (kLanczosA + 2)); // of the order of the two filters' look ahead
  REQUIRE(delay.fitError < 0.01);

  // the true delay is not in general a whole number of samples at the outer rate, so the reported
  // latency cannot always be exact (but see the hidden test below)
  REQUIRE(std::fabs(delay.measured - delay.reported) <= 1.5);

  // and nothing much comes out before then (the filters do ring slightly ahead of the signal)
  REQUIRE(delay.peakBeforeSignal < 0.02);
}

// SUSPECTED BUG: IPlug/Extras/RealtimeResampler.h:112-114 the latency is the number of input samples
// used to warm up the resamplers plus a "magic number" of 2. When the inner rate is lower than the
// outer rate that is one sample short: e.g. at 96kHz outer / 48kHz inner the round trip delay is
// exactly 45 samples but 44 is reported (and 88.2kHz / 44.1kHz likewise), so a host compensating for
// the reported latency leaves the plug-in one sample late.
TEST_CASE("RealtimeResampler: GetLatency is exact when the round trip delay is a whole number of samples", "[.][bug]")
{
  const auto rates = GENERATE(Rates{96000., 48000.}, Rates{88200., 44100.});

  const auto delay = MeasureRoundTripDelay(rates.in, rates.out, 256);

  CAPTURE(rates.in, rates.out, delay.reported, delay.measured);
  REQUIRE(delay.fitError < 0.01);
  REQUIRE(std::fabs(delay.measured - delay.reported) < 0.1);
}

TEST_CASE("RealtimeResampler: DSP inside the process function is pitched for the inner rate", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.}, Rates{96000., 48000.});
  const auto mode = GENERATE(Realtime::ESRCMode::kLancsoz, Realtime::ESRCMode::kLinearInterpolation);
  const double freq = 500.;
  constexpr int blockSize = 256;
  constexpr int nFrames = 16384;

  const double outerRate = rates.in;
  const double innerRate = rates.out;

  Realtime resampler(innerRate, mode);
  resampler.Reset(outerRate, blockSize);

  // an oscillator that believes it is running at the inner rate
  int n = 0;
  const auto oscillator = [&](double**, double** outputs, int nInner, int nChans) {
    for (int s = 0; s < nInner; s++, n++)
    {
      for (int c = 0; c < nChans; c++)
        outputs[c][s] = std::sin(RadiansPerSample(freq, innerRate) * static_cast<double>(n));
    }
  };

  const std::vector<double> silence(nFrames, 0.);
  const auto out = Process(resampler, silence, silence, blockSize, oscillator);

  CAPTURE(outerRate, innerRate, static_cast<int>(mode));

  if (mode == Realtime::ESRCMode::kLancsoz)
  {
    const auto fit = extrastest::FitSinusoid(out[0].data() + nFrames / 2, nFrames / 2, RadiansPerSample(freq, outerRate));
    REQUIRE(fit.amplitude == Approx(1.).margin(5e-3));
    REQUIRE(fit.maxResidual < 5e-3);
  }
  else
  {
    // linear interpolation works a block at a time and is only approximate, so just check the pitch
    const int nCycles = extrastest::CountRisingZeroCrossings(out[0].data(), nFrames);
    REQUIRE(static_cast<double>(nCycles) == Approx(freq * nFrames / outerRate).margin(3.));
    REQUIRE(extrastest::RMS(out[0].data(), nFrames) == Approx(std::sqrt(0.5)).margin(0.03));
  }
}

TEST_CASE("RealtimeResampler: linear interpolation mode has no latency and roughly preserves a low frequency sine", "[unit][extras][resampler]")
{
  const auto rates = GENERATE(Rates{44100., 48000.}, Rates{48000., 44100.}, Rates{96000., 44100.}, Rates{44100., 96000.});
  const int blockSize = GENERATE(64, 512);
  const double freq = 200.;
  constexpr int nFrames = 8192;

  const double outerRate = rates.in;
  const double innerRate = rates.out;

  Realtime resampler(innerRate, Realtime::ESRCMode::kLinearInterpolation);
  resampler.Reset(outerRate, blockSize);

  REQUIRE(resampler.GetLatency() == 0);

  const auto in = extrastest::MakeSine<double>(freq, outerRate, nFrames);
  const auto out = Process(resampler, in, in, blockSize, Passthrough);

  CAPTURE(outerRate, innerRate, blockSize);
  REQUIRE(extrastest::AllFinite(out[0].data(), nFrames));
  REQUIRE(extrastest::RMS(out[0].data(), nFrames) == Approx(extrastest::RMS(in.data(), nFrames)).epsilon(0.02));
  REQUIRE(extrastest::CountRisingZeroCrossings(out[0].data(), nFrames) == extrastest::CountRisingZeroCrossings(in.data(), nFrames));

  // no latency: the output tracks the input (with some error at the block edges)
  REQUIRE(extrastest::MaxAbsDiff(out[0].data(), in.data(), nFrames) < 0.1);
  REQUIRE(extrastest::MaxAbsDiff(out[1].data(), in.data(), nFrames) < 0.1);
}

TEST_CASE("RealtimeResampler: Reset clears the resamplers", "[unit][extras][resampler]")
{
  constexpr int blockSize = 128;
  constexpr int nFrames = 2048;

  Realtime resampler(48000., Realtime::ESRCMode::kLancsoz);
  resampler.Reset(44100., blockSize);

  const auto noise = extrastest::MakeNoise<double>(nFrames, 7u);
  const std::vector<double> silence(nFrames, 0.);

  const auto first = Process(resampler, noise, noise, blockSize, Passthrough);
  REQUIRE(extrastest::PeakAbs(first[0].data(), nFrames) > 0.5);

  SECTION("without a reset the tail of the signal is still to come")
  {
    const auto tail = Process(resampler, silence, silence, blockSize, Passthrough);
    REQUIRE(extrastest::PeakAbs(tail[0].data(), nFrames) > 0.1);
  }

  SECTION("after a reset there is silence")
  {
    resampler.Reset(44100., blockSize);
    const auto afterReset = Process(resampler, silence, silence, blockSize, Passthrough);
    REQUIRE(extrastest::PeakAbs(afterReset[0].data(), nFrames) == 0.);
    REQUIRE(extrastest::PeakAbs(afterReset[1].data(), nFrames) == 0.);
  }

  SECTION("after a reset the output repeats")
  {
    resampler.Reset(44100., blockSize);
    const auto second = Process(resampler, noise, noise, blockSize, Passthrough);
    REQUIRE(extrastest::MaxAbsDiff(first[0].data(), second[0].data(), nFrames) < 1e-12);
  }
}

TEST_CASE("RealtimeResampler: Reset can change the outer sample rate", "[unit][extras][resampler]")
{
  const double freq = 1000.;
  constexpr int blockSize = 256;
  constexpr int nFrames = 8192;

  Realtime resampler(48000., Realtime::ESRCMode::kLancsoz);

  for (double outerRate : {44100., 96000., 48000., 88200.})
  {
    resampler.Reset(outerRate, blockSize);

    int nInnerFrames = 0;
    const auto in = extrastest::MakeSine<double>(freq, outerRate, nFrames);
    const auto out = Process(resampler, in, in, blockSize, [&nInnerFrames](double** inputs, double** outputs, int n, int nChans) {
      nInnerFrames += n;
      Passthrough(inputs, outputs, n, nChans);
    });

    const auto fit = extrastest::FitSinusoid(out[0].data() + nFrames / 2, nFrames / 2, RadiansPerSample(freq, outerRate));

    CAPTURE(outerRate);
    REQUIRE(static_cast<double>(nInnerFrames) == Approx(nFrames * 48000. / outerRate).margin(64.));
    REQUIRE(fit.amplitude == Approx(1.).margin(5e-3));
    REQUIRE(fit.maxResidual < 5e-3);
  }
}

TEST_CASE("RealtimeResampler: channels are independent", "[unit][extras][resampler]")
{
  const auto mode = GENERATE(Realtime::ESRCMode::kLancsoz, Realtime::ESRCMode::kLinearInterpolation);
  constexpr int blockSize = 128;
  constexpr int nFrames = 2048;

  Realtime resampler(48000., mode);
  Realtime swappedResampler(48000., mode);
  resampler.Reset(44100., blockSize);
  swappedResampler.Reset(44100., blockSize);

  const auto sine = extrastest::MakeSine<double>(500., 44100., nFrames);
  const std::vector<double> silence(nFrames, 0.);

  const auto out = Process(resampler, sine, silence, blockSize, Passthrough);
  const auto swapped = Process(swappedResampler, silence, sine, blockSize, Passthrough);

  REQUIRE(extrastest::PeakAbs(out[0].data(), nFrames) > 0.9);
  REQUIRE(extrastest::PeakAbs(out[1].data(), nFrames) == 0.);
  REQUIRE(extrastest::PeakAbs(swapped[0].data(), nFrames) == 0.);
  REQUIRE(extrastest::MaxAbsDiff(out[0].data(), swapped[1].data(), nFrames) == 0.);
}

TEST_CASE("RealtimeResampler: single precision", "[unit][extras][resampler]")
{
  const double freq = 1000.;
  constexpr int blockSize = 256;
  constexpr int nFrames = 8192;

  RealtimeResampler<float, 2, kLanczosA> resampler(48000.);
  resampler.Reset(44100., blockSize);

  auto in = extrastest::MakeSine<float>(freq, 44100., nFrames);
  std::vector<float> outL(nFrames, 0.f), outR(nFrames, 0.f);

  for (int start = 0; start < nFrames; start += blockSize)
  {
    float* ins[2] = {in.data() + start, in.data() + start};
    float* outs[2] = {outL.data() + start, outR.data() + start};
    resampler.ProcessBlock(ins, outs, blockSize, 2, [](float** inputs, float** outputs, int n, int nChans) {
      for (int c = 0; c < nChans; c++)
        std::memcpy(outputs[c], inputs[c], static_cast<size_t>(n) * sizeof(float));
    });
  }

  const auto fit = extrastest::FitSinusoid(outL.data() + nFrames / 2, nFrames / 2, RadiansPerSample(freq, 44100.));
  REQUIRE(extrastest::AllFinite(outL.data(), nFrames));
  REQUIRE(fit.amplitude == Approx(1.).margin(5e-3));
  REQUIRE(fit.maxResidual < 5e-3);
  REQUIRE(extrastest::MaxAbsDiff(outL.data(), outR.data(), nFrames) == 0.);
}

// ProcessBlock() bypasses the resamplers when the inner and outer sample rates are equal
TEST_CASE("RealtimeResampler: no latency is reported when the rates are equal", "[unit][extras][resampler]")
{
  const double sr = GENERATE(44100., 48000.);

  Realtime resampler(sr, Realtime::ESRCMode::kLancsoz);
  resampler.Reset(sr, 512);

  // the signal really does come straight through...
  const auto in = extrastest::MakeNoise<double>(512, 8u);
  const auto out = Process(resampler, in, in, 512, Passthrough);
  REQUIRE(extrastest::MaxAbsDiff(in.data(), out[0].data(), 512) == 0.);

  // ...so the latency is zero
  REQUIRE(resampler.GetLatency() == 0);
}

// BUG: IPlug/Extras/RealtimeResampler.h:196-197 LinearInterpolate() reads inputs[chan][readPosInt - 1]
// when there is no next sample to interpolate towards. For a one sample block (which hosts do send,
// e.g. when splitting blocks for sample accurate automation) readPosInt is 0, so this reads
// inputs[chan][-1]: one element before the start of the caller's buffer (reported by AddressSanitizer
// as a buffer underflow). Since the interpolation weight is zero in that case the value read is not
// used, so a debugger/sanitizer is needed to see it; the expectation here is simply that a one sample
// block passes through.
TEST_CASE("RealtimeResampler: linear interpolation of a one sample block", "[.][bug]")
{
  Realtime resampler(48000., Realtime::ESRCMode::kLinearInterpolation);
  resampler.Reset(44100., 64);

  // exactly sized heap buffers, so that a sanitizer sees any out of bounds access
  std::vector<double> inL(1, 0.5), inR(1, -0.5), outL(1, 0.), outR(1, 0.);
  double* ins[2] = {inL.data(), inR.data()};
  double* outs[2] = {outL.data(), outR.data()};

  resampler.ProcessBlock(ins, outs, 1, 2, Passthrough);

  REQUIRE(outL[0] == Approx(0.5));
  REQUIRE(outR[0] == Approx(-0.5));
}

#endif // GCC >= 13
