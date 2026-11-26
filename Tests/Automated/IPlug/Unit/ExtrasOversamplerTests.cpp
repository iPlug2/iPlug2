/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for OverSampler and the HIIR half band filters it uses (IPlug/Extras/Oversampler.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <vector>

#include "IPlugConstants.h"
#include "Oversampler.h"
#include "ExtrasTestUtils.h"

using namespace iplug;
using Catch::Approx;

namespace
{
using OS = OverSampler<double>;

int RateFor(EFactor factor)
{
  return 1 << static_cast<int>(factor);
}

/** Copies the first channel across unchanged */
void Identity(double** inputs, double** outputs, int nFrames)
{
  std::memcpy(outputs[0], inputs[0], static_cast<size_t>(nFrames) * sizeof(double));
}

double SoftClip(double x)
{
  return std::tanh(4. * x);
}

/** Run a mono signal through a block based oversampler in blocks of blockSize */
std::vector<double> ProcessBlocks(OS& os, const std::vector<double>& input, int blockSize, OS::BlockProcessFunc func)
{
  std::vector<double> in = input;
  std::vector<double> out(in.size(), 0.);
  const int nFrames = static_cast<int>(in.size());

  for (int start = 0; start < nFrames; start += blockSize)
  {
    double* ins[1] = {in.data() + start};
    double* outs[1] = {out.data() + start};
    os.ProcessBlock(ins, outs, std::min(blockSize, nFrames - start), 1, 1, func);
  }

  return out;
}
} // namespace

TEST_CASE("OverSampler: factor and rate", "[unit][extras][oversampler]")
{
  REQUIRE(OS(kNone).GetRate() == 1);
  REQUIRE(OS(k2x).GetRate() == 2);
  REQUIRE(OS(k4x).GetRate() == 4);
  REQUIRE(OS(k8x).GetRate() == 8);
  REQUIRE(OS(k16x).GetRate() == 16);
  REQUIRE(OS().GetRate() == 1);

  REQUIRE(OS::RateToFactor(1) == kNone);
  REQUIRE(OS::RateToFactor(2) == k2x);
  REQUIRE(OS::RateToFactor(4) == k4x);
  REQUIRE(OS::RateToFactor(8) == k8x);
  REQUIRE(OS::RateToFactor(16) == k16x);

  OS os;
  for (int f = 0; f < kNumFactors; f++)
  {
    os.SetOverSampling(static_cast<EFactor>(f));
    REQUIRE(os.GetRate() == RateFor(static_cast<EFactor>(f)));
    REQUIRE(OS::RateToFactor(os.GetRate()) == static_cast<EFactor>(f));
  }
}

TEST_CASE("OverSampler: the process function runs on rate times as many samples", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(kNone, k2x, k4x, k8x, k16x);
  const int nFrames = GENERATE(1, 64, 100);
  constexpr int blockSize = 128;

  OS os(factor);
  os.Reset(blockSize);

  int nFramesProcessed = 0;
  int maxFramesPerCall = 0;
  const auto countingFunc = [&](double** inputs, double** outputs, int n) {
    nFramesProcessed += n;
    maxFramesPerCall = std::max(maxFramesPerCall, n);
    Identity(inputs, outputs, n);
  };

  std::vector<double> in(static_cast<size_t>(nFrames), 0.5), out(static_cast<size_t>(nFrames), 0.);
  double* ins[1] = {in.data()};
  double* outs[1] = {out.data()};
  os.ProcessBlock(ins, outs, nFrames, 1, 1, countingFunc);

  CAPTURE(static_cast<int>(factor), nFrames);
  REQUIRE(nFramesProcessed == nFrames * RateFor(factor));
  REQUIRE(maxFramesPerCall <= nFrames * RateFor(factor));
}

TEST_CASE("OverSampler: no oversampling is a straight pass through", "[unit][extras][oversampler]")
{
  constexpr int nFrames = 1000;
  const auto in = extrastest::MakeNoise<double>(nFrames, 11u);

  SECTION("block")
  {
    OS os(kNone);
    const auto out = ProcessBlocks(os, in, 64, Identity);
    REQUIRE(extrastest::MaxAbsDiff(in.data(), out.data(), nFrames) == 0.);
  }

  SECTION("per sample")
  {
    OS os(kNone, false);
    for (double x : in)
      REQUIRE(os.Process(x, [](double v) { return v * 2.; }) == x * 2.);
  }

  SECTION("generator")
  {
    OS os(kNone, false);
    int n = 0;
    for (int s = 0; s < 100; s++)
      REQUIRE(os.ProcessGen([&n]() { return static_cast<double>(++n); }) == static_cast<double>(s + 1));
  }
}

TEST_CASE("OverSampler: an identity process leaves a sine at unity gain", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(k2x, k4x, k8x, k16x);
  const double sr = GENERATE(44100., 48000.);
  const double freq = GENERATE(100., 1000., 5000.);
  constexpr int nFrames = 8192;
  constexpr int blockSize = 64;

  OS os(factor);
  os.Reset(blockSize);

  const auto in = extrastest::MakeSine<double>(freq, sr, nFrames);
  const auto out = ProcessBlocks(os, in, blockSize, Identity);

  // after the filters have settled the output is still a clean sinusoid of the same frequency and level
  const auto fit = extrastest::FitSinusoid(out.data() + nFrames / 2, nFrames / 2, 2. * PI * freq / sr);

  CAPTURE(static_cast<int>(factor), sr, freq);
  REQUIRE(extrastest::AllFinite(out.data(), nFrames));
  REQUIRE(fit.amplitude == Approx(1.).margin(2e-3));
  REQUIRE(fit.maxResidual < 2e-3);
}

TEST_CASE("OverSampler: DC passes at unity gain", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(k2x, k4x, k8x, k16x);
  constexpr int nFrames = 4096;

  OS os(factor);
  const std::vector<double> in(nFrames, 0.75);
  const auto out = ProcessBlocks(os, in, 256, Identity);

  CAPTURE(static_cast<int>(factor));
  REQUIRE(out[nFrames - 1] == Approx(0.75).margin(1e-6));
  REQUIRE(extrastest::Mean(out.data() + nFrames / 2, nFrames / 2) == Approx(0.75).margin(1e-6));

  // the step response settles quickly and without wild overshoot
  REQUIRE(extrastest::PeakAbs(out.data(), nFrames) < 0.75 * 1.5);
  REQUIRE(out[200] == Approx(0.75).margin(0.01));
}

TEST_CASE("OverSampler: the process function sees the signal at the higher rate", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(k2x, k4x, k8x, k16x);
  const double sr = 48000.;
  const double freq = 1000.;
  constexpr int nFrames = 4096;
  const int rate = RateFor(factor);

  OS os(factor);
  os.Reset(nFrames);

  // the function is called once per chunk of the upsampled block, in order
  std::vector<double> upsampled;
  const auto capture = [&upsampled](double** inputs, double** outputs, int n) {
    upsampled.insert(upsampled.end(), inputs[0], inputs[0] + n);
    Identity(inputs, outputs, n);
  };

  const auto in = extrastest::MakeSine<double>(freq, sr, nFrames);
  ProcessBlocks(os, in, nFrames, capture);

  REQUIRE(static_cast<int>(upsampled.size()) == nFrames * rate);

  // the same sine, at rate times the sample rate, with nothing added
  const int nUp = nFrames * rate;
  const auto fit = extrastest::FitSinusoid(upsampled.data() + nUp / 2, nUp / 2, 2. * PI * freq / (sr * rate));

  CAPTURE(rate);
  REQUIRE(fit.amplitude == Approx(1.).margin(2e-3));
  REQUIRE(fit.maxResidual < 2e-3);
}

TEST_CASE("OverSampler: content above the original Nyquist is removed on the way back down", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(k2x, k4x, k8x, k16x);
  const double sr = 44100.;
  constexpr int nFrames = 4096;
  const int rate = RateFor(factor);

  // ignore the input and generate a tone in the oversampled domain
  const auto renderTone = [&](double freq) {
    OS os(factor);
    os.Reset(nFrames);

    int n = 0;
    const double w = 2. * PI * freq / (sr * rate);
    const auto tone = [&n, w](double**, double** outputs, int nChunk) {
      for (int s = 0; s < nChunk; s++)
        outputs[0][s] = std::sin(w * static_cast<double>(n++));
    };

    const std::vector<double> silence(nFrames, 0.);
    return ProcessBlocks(os, silence, nFrames, tone);
  };

  CAPTURE(rate);

  // in band: comes through as it is
  const auto inBand = renderTone(5000.);
  const auto fit = extrastest::FitSinusoid(inBand.data() + nFrames / 2, nFrames / 2, 2. * PI * 5000. / sr);
  REQUIRE(fit.amplitude == Approx(1.).margin(2e-3));
  REQUIRE(fit.maxResidual < 2e-3);

  // out of band (would alias to 14.1kHz): filtered out
  const auto outOfBand = renderTone(30000.);
  REQUIRE(extrastest::RMS(outOfBand.data() + nFrames / 2, nFrames / 2) < 0.01);
}

TEST_CASE("OverSampler: per-sample and block processing agree", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(k2x, k4x, k8x, k16x);
  constexpr int nFrames = 2048;

  const auto in = extrastest::MakeSine<double>(997., 44100., nFrames, 0.9);

  OS blockOS(factor, true);
  blockOS.Reset(256);
  const auto blockOut = ProcessBlocks(blockOS, in, 256, [](double** inputs, double** outputs, int n) {
    for (int s = 0; s < n; s++)
      outputs[0][s] = SoftClip(inputs[0][s]);
  });

  OS sampleOS(factor, false);
  std::vector<double> sampleOut(nFrames);
  for (size_t s = 0; s < nFrames; s++)
    sampleOut[s] = sampleOS.Process(in[s], SoftClip);

  CAPTURE(static_cast<int>(factor));
  REQUIRE(extrastest::MaxAbsDiff(blockOut.data(), sampleOut.data(), nFrames) < 1e-9);
  REQUIRE(extrastest::RMS(blockOut.data(), nFrames) > 0.5);
}

TEST_CASE("OverSampler: result does not depend on how the signal is split into blocks", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(k2x, k4x, k8x, k16x);
  const int blockSize = GENERATE(1, 17, 64);
  constexpr int nFrames = 1024;
  constexpr int maxBlockSize = 1024;

  const auto in = extrastest::MakeNoise<double>(nFrames, 2u, 0.5);

  OS reference(factor), os(factor);
  reference.Reset(maxBlockSize);
  os.Reset(maxBlockSize);

  const auto refOut = ProcessBlocks(reference, in, maxBlockSize, Identity);
  const auto out = ProcessBlocks(os, in, blockSize, Identity);

  CAPTURE(static_cast<int>(factor), blockSize);
  REQUIRE(extrastest::MaxAbsDiff(refOut.data(), out.data(), nFrames) < 1e-12);
}

TEST_CASE("OverSampler: ProcessGen decimates a generator running at the higher rate", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(kNone, k2x, k4x, k8x, k16x);
  const double sr = GENERATE(44100., 48000.);
  const double freq = 1000.;
  constexpr int nFrames = 4096;
  const int rate = RateFor(factor);

  OS os(factor, false);

  int n = 0;
  int nCalls = 0;
  const double w = 2. * PI * freq / (sr * rate);
  const auto generator = [&]() {
    nCalls++;
    return std::sin(w * static_cast<double>(n++));
  };

  std::vector<double> out(nFrames);
  for (auto& v : out)
    v = os.ProcessGen(generator);

  CAPTURE(rate, sr);
  REQUIRE(nCalls == nFrames * rate);

  const auto fit = extrastest::FitSinusoid(out.data() + nFrames / 2, nFrames / 2, 2. * PI * freq / sr);
  REQUIRE(fit.amplitude == Approx(1.).margin(2e-3));
  REQUIRE(fit.maxResidual < 2e-3);
}

TEST_CASE("OverSampler: Reset clears the filter state", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(k2x, k4x, k8x, k16x);
  constexpr int nFrames = 512;

  const auto noise = extrastest::MakeNoise<double>(nFrames, 3u);
  const std::vector<double> silence(nFrames, 0.);

  CAPTURE(static_cast<int>(factor));

  SECTION("block")
  {
    OS os(factor);
    os.Reset(nFrames);
    const auto first = ProcessBlocks(os, noise, nFrames, Identity);

    // without a reset the filters ring on into the silence
    const auto tail = ProcessBlocks(os, silence, nFrames, Identity);
    REQUIRE(extrastest::PeakAbs(tail.data(), nFrames) > 1e-6);

    ProcessBlocks(os, noise, nFrames, Identity);
    os.Reset(nFrames);
    const auto afterReset = ProcessBlocks(os, silence, nFrames, Identity);
    REQUIRE(extrastest::PeakAbs(afterReset.data(), nFrames) == 0.);

    os.Reset(nFrames);
    const auto second = ProcessBlocks(os, noise, nFrames, Identity);
    REQUIRE(extrastest::MaxAbsDiff(first.data(), second.data(), nFrames) == 0.);
  }

  SECTION("per sample")
  {
    OS os(factor, false);
    const auto identity = [](double x) { return x; };

    for (double x : noise)
      os.Process(x, identity);

    os.Reset();

    double peak = 0.;
    for (int s = 0; s < nFrames; s++)
      peak = std::max(peak, std::fabs(os.Process(0., identity)));

    REQUIRE(peak == 0.);
  }
}

TEST_CASE("OverSampler: changing the factor while running", "[unit][extras][oversampler]")
{
  const double sr = 48000.;
  const double freq = 2000.;
  constexpr int nFrames = 4096;

  OS os(k2x);
  const auto in = extrastest::MakeSine<double>(freq, sr, nFrames);

  for (EFactor factor : {k16x, kNone, k4x, k2x, k8x})
  {
    os.SetOverSampling(factor);
    REQUIRE(os.GetRate() == RateFor(factor));

    int nFramesProcessed = 0;
    const auto out = ProcessBlocks(os, in, 512, [&nFramesProcessed](double** inputs, double** outputs, int n) {
      nFramesProcessed += n;
      Identity(inputs, outputs, n);
    });

    const auto fit = extrastest::FitSinusoid(out.data() + nFrames / 2, nFrames / 2, 2. * PI * freq / sr);

    CAPTURE(static_cast<int>(factor));
    REQUIRE(nFramesProcessed == nFrames * RateFor(factor));
    REQUIRE(fit.amplitude == Approx(1.).margin(2e-3));
    REQUIRE(fit.maxResidual < 2e-3);
  }
}

TEST_CASE("OverSampler: a hard nonlinearity stays bounded", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(kNone, k2x, k4x, k8x, k16x);
  constexpr int nFrames = 8192;

  const auto hardClip = [](double** inputs, double** outputs, int n) {
    for (int s = 0; s < n; s++)
      outputs[0][s] = std::max(-1., std::min(1., inputs[0][s] * 100.));
  };

  // a loud mix of a sine and noise
  auto in = extrastest::MakeSine<double>(3000., 44100., nFrames, 2.);
  const auto noise = extrastest::MakeNoise<double>(nFrames, 4u, 2.);
  for (size_t s = 0; s < nFrames; s++)
    in[s] += noise[s];

  OS os(factor);
  const auto out = ProcessBlocks(os, in, 512, hardClip);

  CAPTURE(static_cast<int>(factor));
  REQUIRE(extrastest::AllFinite(out.data(), nFrames));

  // the function's output is a full scale square-ish wave within +/-1: the steep decimation filters
  // ring beyond that (by a factor of about 2.5 for this signal), but not without limit
  REQUIRE(extrastest::PeakAbs(out.data(), nFrames) < 5.);
  REQUIRE(extrastest::RMS(out.data(), nFrames) > 0.5);

  // odd symmetry is preserved
  for (auto& v : in)
    v = -v;

  OS negOS(factor);
  const auto negOut = ProcessBlocks(negOS, in, 512, hardClip);

  double maxError = 0.;
  for (size_t s = 0; s < nFrames; s++)
    maxError = std::max(maxError, std::fabs(out[s] + negOut[s]));
  REQUIRE(maxError < 1e-12);
}

TEST_CASE("OverSampler: oversampling reduces the aliasing of a nonlinearity", "[unit][extras][oversampler]")
{
  const double sr = 44100.;
  constexpr int nFrames = 16384;
  constexpr int nMeasure = 8192;

  // a 5kHz sine (snapped to a whole number of cycles in the measurement window) through a clipper:
  // its harmonics at 15k, 25k, 35k... fold back to inharmonic frequencies unless oversampled
  const double freq = std::round(5000. * nMeasure / sr) * sr / nMeasure;
  const auto in = extrastest::MakeSine<double>(freq, sr, nFrames);

  const auto clip = [](double** inputs, double** outputs, int n) {
    for (int s = 0; s < n; s++)
      outputs[0][s] = std::max(-0.5, std::min(0.5, inputs[0][s]));
  };

  // the level of everything that is not the fundamental or its in-band 3rd harmonic
  const auto aliasLevel = [&](EFactor factor) {
    OS os(factor);
    const auto out = ProcessBlocks(os, in, 512, clip);
    const double* pMeasure = out.data() + nFrames - nMeasure;

    double residualPower = extrastest::RMS(pMeasure, nMeasure);
    residualPower *= residualPower;

    for (double harmonic : {1., 3.})
    {
      const auto fit = extrastest::FitSinusoid(pMeasure, nMeasure, 2. * PI * freq * harmonic / sr);
      residualPower -= 0.5 * fit.amplitude * fit.amplitude;
    }

    return std::sqrt(std::max(residualPower, 0.));
  };

  const double none = aliasLevel(kNone);
  const double x2 = aliasLevel(k2x);
  const double x8 = aliasLevel(k8x);

  CAPTURE(none, x2, x8);
  REQUIRE(none > 0.01);
  REQUIRE(x2 < none * 0.7);
  REQUIRE(x8 < none * 0.25);
}

TEST_CASE("OverSampler: channels are processed independently", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(k2x, k4x, k8x, k16x);
  constexpr int nFrames = 1024;
  constexpr int blockSize = 256;

  OS stereo(factor, true, 2, 2);
  stereo.Reset(blockSize);

  extrastest::Buffers<double> in(2, nFrames), out(2, nFrames, 9.);
  in[0] = extrastest::MakeNoise<double>(nFrames, 5u);
  // channel 1 is silent

  const auto gains = [](double** inputs, double** outputs, int n) {
    for (int s = 0; s < n; s++)
    {
      outputs[0][s] = inputs[0][s];
      outputs[1][s] = inputs[1][s] * 2.;
    }
  };

  for (int start = 0; start < nFrames; start += blockSize)
  {
    double* ins[2] = {in[0].data() + start, in[1].data() + start};
    double* outs[2] = {out[0].data() + start, out[1].data() + start};
    stereo.ProcessBlock(ins, outs, blockSize, 2, 2, gains);
  }

  OS mono(factor);
  mono.Reset(blockSize);
  const auto ref = ProcessBlocks(mono, in[0], blockSize, Identity);

  CAPTURE(static_cast<int>(factor));
  REQUIRE(extrastest::MaxAbsDiff(out[0].data(), ref.data(), nFrames) < 1e-12);
  REQUIRE(extrastest::PeakAbs(out[1].data(), nFrames) == 0.);

  // and the other way round, with the per-channel gain applied
  extrastest::Buffers<double> swappedOut(2, nFrames, 9.);
  stereo.Reset(blockSize);

  for (int start = 0; start < nFrames; start += blockSize)
  {
    double* ins[2] = {in[1].data() + start, in[0].data() + start};
    double* outs[2] = {swappedOut[0].data() + start, swappedOut[1].data() + start};
    stereo.ProcessBlock(ins, outs, blockSize, 2, 2, gains);
  }

  REQUIRE(extrastest::PeakAbs(swappedOut[0].data(), nFrames) == 0.);

  double maxError = 0.;
  for (size_t s = 0; s < nFrames; s++)
    maxError = std::max(maxError, std::fabs(swappedOut[1][s] - 2. * ref[s]));
  REQUIRE(maxError < 1e-12);
}

TEST_CASE("OverSampler: single precision", "[unit][extras][oversampler]")
{
  const EFactor factor = GENERATE(k2x, k4x, k8x, k16x);
  const double sr = 48000.;
  const double freq = 1000.;
  constexpr int nFrames = 4096;

  OverSampler<float> os(factor);
  auto in = extrastest::MakeSine<float>(freq, sr, nFrames);
  std::vector<float> out(nFrames, 0.f);

  for (int start = 0; start < nFrames; start += 512)
  {
    float* ins[1] = {in.data() + start};
    float* outs[1] = {out.data() + start};
    os.ProcessBlock(ins, outs, 512, 1, 1, [](float** inputs, float** outputs, int n) {
      std::memcpy(outputs[0], inputs[0], static_cast<size_t>(n) * sizeof(float));
    });
  }

  const auto fit = extrastest::FitSinusoid(out.data() + nFrames / 2, nFrames / 2, 2. * PI * freq / sr);

  CAPTURE(static_cast<int>(factor));
  REQUIRE(extrastest::AllFinite(out.data(), nFrames));
  REQUIRE(fit.amplitude == Approx(1.).margin(2e-3));
  REQUIRE(fit.maxResidual < 2e-3);
}

// BUG: IPlug/Extras/Oversampler.h does not work when there are more output channels than input
// channels, e.g. a mono-in/stereo-out effect or an instrument (constructor arguments nInChannels = 1,
// nOutChannels = 2), although it takes the two counts separately:
//  - Reset() (lines 128-131) sizes the mDown* buffers with mNInChannels rather than mNOutChannels,
//    so the per-channel pointers for the extra output channels point past the end of the buffer
//    (heap buffer overflow when the process function writes to them and the downsamplers read them)
//  - ProcessBlock() (lines 226-229) only sets mNextOutputPtrs for c < nInChans, so the process
//    function is handed stale (initially null) pointers for the remaining output channels
// NOTE: this test crashes rather than fails with the current implementation.
TEST_CASE("OverSampler: more output channels than input channels", "[.][bug]")
{
  const EFactor factor = GENERATE(k2x, k4x);
  constexpr int nFrames = 2048;
  constexpr int blockSize = 256;

  OS os(factor, true, 1, 2);
  os.Reset(blockSize);

  auto in = extrastest::MakeSine<double>(1000., 48000., nFrames);
  extrastest::Buffers<double> out(2, nFrames);

  // mono to stereo: the right channel is the left channel inverted
  const auto split = [](double** inputs, double** outputs, int n) {
    for (int s = 0; s < n; s++)
    {
      outputs[0][s] = inputs[0][s];
      outputs[1][s] = -inputs[0][s];
    }
  };

  for (int start = 0; start < nFrames; start += blockSize)
  {
    double* ins[1] = {in.data() + start};
    double* outs[2] = {out[0].data() + start, out[1].data() + start};
    os.ProcessBlock(ins, outs, blockSize, 1, 2, split);
  }

  const double w = 2. * PI * 1000. / 48000.;
  const auto fitL = extrastest::FitSinusoid(out[0].data() + nFrames / 2, nFrames / 2, w);
  const auto fitR = extrastest::FitSinusoid(out[1].data() + nFrames / 2, nFrames / 2, w);

  REQUIRE(fitL.amplitude == Approx(1.).margin(2e-3));
  REQUIRE(fitL.maxResidual < 2e-3);
  REQUIRE(fitR.amplitude == Approx(1.).margin(2e-3));
  REQUIRE(fitR.maxResidual < 2e-3);

  double maxError = 0.;
  for (size_t s = 0; s < nFrames; s++)
    maxError = std::max(maxError, std::fabs(out[0][s] + out[1][s]));
  REQUIRE(maxError < 1e-12);
}
