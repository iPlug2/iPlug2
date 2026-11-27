/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for the tempo-syncable LFO (IPlug/Extras/LFO.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "IPlugConstants.h"
#include "IPlugUtilities.h"
#include "LFO.h"
#include "ExtrasTestUtils.h"

using namespace iplug;
using Catch::Approx;

namespace
{
using Lfo = LFO<double>;

Lfo MakeLFO(double sr, int shape, bool bipolar)
{
  Lfo lfo;
  lfo.SetSampleRate(sr);
  lfo.SetShape(shape);
  lfo.SetPolarity(bipolar);
  return lfo;
}

std::vector<double> RenderPerSample(Lfo& lfo, double freqHz, int nFrames)
{
  std::vector<double> out(static_cast<size_t>(nFrames));

  for (auto& v : out)
    v = lfo.Process(freqHz);

  return out;
}

/** Free running (Hz) block render */
std::vector<double> RenderBlocks(Lfo& lfo, double freqHz, int nFrames, int blockSize)
{
  std::vector<double> out(static_cast<size_t>(nFrames));
  lfo.SetFreqCPS(freqHz);

  for (int start = 0; start < nFrames; start += blockSize)
    lfo.ProcessBlock(out.data() + start, std::min(blockSize, nFrames - start));

  return out;
}

/** The number of times the signal goes from below the midpoint to above it */
int CountCycles(const std::vector<double>& v, double midpoint)
{
  int n = 0;

  for (size_t s = 1; s < v.size(); s++)
  {
    if (v[s - 1] < midpoint && v[s] >= midpoint)
      n++;
  }

  return n;
}

double Frac(double x)
{
  return x - std::floor(x);
}

/** @return true if a phase is within a few samples of where a ramp wraps */
bool NearWrap(double phase, double margin = 0.001)
{
  return phase < margin || phase > 1. - margin;
}
} // namespace

TEST_CASE("LFO: defaults to a unipolar triangle", "[unit][extras][lfo]")
{
  Lfo lfo;
  Lfo reference = MakeLFO(44100., Lfo::kTriangle, false);

  const auto out = RenderPerSample(lfo, 10., 44100);
  const auto refOut = RenderPerSample(reference, 10., 44100);

  REQUIRE(extrastest::MaxAbsDiff(out.data(), refOut.data(), 44100) == 0.);
  REQUIRE(*std::min_element(out.begin(), out.end()) >= 0.);
}

TEST_CASE("LFO: output range follows the polarity for every shape", "[unit][extras][lfo]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const int shape = GENERATE(range(0, static_cast<int>(Lfo::kNumShapes)));
  const bool bipolar = GENERATE(false, true);

  auto lfo = MakeLFO(sr, shape, bipolar);
  const auto out = RenderPerSample(lfo, 7., static_cast<int>(sr));

  const double lo = bipolar ? -1. : 0.;
  const double hi = 1.;
  const double minValue = *std::min_element(out.begin(), out.end());
  const double maxValue = *std::max_element(out.begin(), out.end());

  CAPTURE(sr, shape, bipolar);
  REQUIRE(extrastest::AllFinite(out.data(), static_cast<int>(out.size())));
  REQUIRE(minValue >= lo - 1e-12);
  REQUIRE(maxValue <= hi + 1e-12);
  REQUIRE(minValue == Approx(lo).margin(0.01));
  REQUIRE(maxValue == Approx(hi).margin(0.01));
  REQUIRE(extrastest::Mean(out.data(), static_cast<int>(out.size())) == Approx((lo + hi) / 2.).margin(0.01));
}

TEST_CASE("LFO: cycles at the requested rate", "[unit][extras][lfo]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const double freq = GENERATE(0.5, 1., 5., 20., 100.);
  const int shape = GENERATE(range(0, static_cast<int>(Lfo::kNumShapes)));
  const int nFrames = static_cast<int>(sr * 4.); // four seconds

  auto lfo = MakeLFO(sr, shape, true);

  // decimate: we only need to see the midpoint crossings
  std::vector<double> out;
  out.reserve(static_cast<size_t>(nFrames));
  for (int s = 0; s < nFrames; s++)
    out.push_back(lfo.Process(freq));

  CAPTURE(sr, freq, shape);
  REQUIRE(std::abs(CountCycles(out, 0.) - static_cast<int>(freq * 4.)) <= 1);
}

TEST_CASE("LFO: ramps are linear in phase", "[unit][extras][lfo]")
{
  const double sr = GENERATE(44100., 48000.);
  const double freq = 3.;
  const int nFrames = static_cast<int>(sr);

  auto up = MakeLFO(sr, Lfo::kRampUp, false);
  auto down = MakeLFO(sr, Lfo::kRampDown, false);
  auto upBipolar = MakeLFO(sr, Lfo::kRampUp, true);
  auto downBipolar = MakeLFO(sr, Lfo::kRampDown, true);

  const auto upOut = RenderPerSample(up, freq, nFrames);
  const auto downOut = RenderPerSample(down, freq, nFrames);
  const auto upBipolarOut = RenderPerSample(upBipolar, freq, nFrames);
  const auto downBipolarOut = RenderPerSample(downBipolar, freq, nFrames);

  double maxError = 0.;

  for (int s = 0; s < nFrames; s++)
  {
    // the phase is advanced before the output is calculated
    const double phase = Frac(static_cast<double>(s + 1) * freq / sr);

    if (NearWrap(phase))
      continue;

    const size_t i = static_cast<size_t>(s);
    maxError = std::max(maxError, std::fabs(upOut[i] - phase));
    maxError = std::max(maxError, std::fabs(downOut[i] - (1. - phase)));
    maxError = std::max(maxError, std::fabs(upBipolarOut[i] - (2. * phase - 1.)));
    maxError = std::max(maxError, std::fabs(downBipolarOut[i] - (1. - 2. * phase)));
  }

  CAPTURE(sr);
  REQUIRE(maxError < 1e-9);
}

TEST_CASE("LFO: square is two-valued with a 50% duty cycle", "[unit][extras][lfo]")
{
  const double sr = GENERATE(44100., 48000.);
  const bool bipolar = GENERATE(false, true);
  const double freq = 5.;
  const int nFrames = static_cast<int>(sr);

  auto lfo = MakeLFO(sr, Lfo::kSquare, bipolar);
  const auto out = RenderPerSample(lfo, freq, nFrames);

  const double lo = bipolar ? -1. : 0.;
  int nHigh = 0, nLow = 0, nOther = 0;

  for (double v : out)
  {
    if (v == 1.)
      nHigh++;
    else if (v == lo)
      nLow++;
    else
      nOther++;
  }

  CAPTURE(sr, bipolar);
  REQUIRE(nOther == 0);
  REQUIRE(std::abs(nHigh - nLow) <= 10); // 5 cycles, a sample or two of slack on each

  // one rising and one falling edge per cycle
  int nEdges = 0;
  for (size_t s = 1; s < out.size(); s++)
  {
    if (out[s] != out[s - 1])
      nEdges++;
  }
  REQUIRE(std::abs(nEdges - 10) <= 1);
}

TEST_CASE("LFO: triangle has a constant slope magnitude", "[unit][extras][lfo]")
{
  const double sr = GENERATE(44100., 48000.);
  const bool bipolar = GENERATE(false, true);
  const double freq = 4.;
  const int nFrames = static_cast<int>(sr);

  auto lfo = MakeLFO(sr, Lfo::kTriangle, bipolar);
  const auto out = RenderPerSample(lfo, freq, nFrames);

  // up and down the full range once per cycle
  const double expectedSlope = (bipolar ? 4. : 2.) * freq / sr;
  int nOffSlope = 0;
  double maxSlope = 0.;

  for (size_t s = 1; s < out.size(); s++)
  {
    const double slope = std::fabs(out[s] - out[s - 1]);
    maxSlope = std::max(maxSlope, slope);

    if (std::fabs(slope - expectedSlope) > 1e-9)
      nOffSlope++;
  }

  CAPTURE(sr, bipolar);
  REQUIRE(maxSlope <= expectedSlope + 1e-9);
  REQUIRE(nOffSlope <= 2 * 4 + 1); // only at the two corners of each cycle
}

TEST_CASE("LFO: sine shape is a sinusoid", "[unit][extras][lfo]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const double freq = 6.;
  const int nFrames = static_cast<int>(sr);

  auto bipolar = MakeLFO(sr, Lfo::kSine, true);
  auto unipolar = MakeLFO(sr, Lfo::kSine, false);

  const auto bipolarOut = RenderPerSample(bipolar, freq, nFrames);
  const auto unipolarOut = RenderPerSample(unipolar, freq, nFrames);

  const double w = 2. * PI * freq / sr;
  const auto bipolarFit = extrastest::FitSinusoid(bipolarOut.data(), nFrames, w);
  const auto unipolarFit = extrastest::FitSinusoid(unipolarOut.data(), nFrames, w, 0.5);

  CAPTURE(sr);
  REQUIRE(bipolarFit.amplitude == Approx(1.).margin(1e-9));
  REQUIRE(bipolarFit.maxResidual < 1e-9);
  REQUIRE(unipolarFit.amplitude == Approx(0.5).margin(1e-9));
  REQUIRE(unipolarFit.maxResidual < 1e-9);
}

TEST_CASE("LFO: unipolar output is the bipolar output rescaled to [0, 1]", "[unit][extras][lfo]")
{
  const int shape = GENERATE(static_cast<int>(Lfo::kSquare), static_cast<int>(Lfo::kRampUp), static_cast<int>(Lfo::kRampDown), static_cast<int>(Lfo::kSine));

  auto unipolar = MakeLFO(48000., shape, false);
  auto bipolar = MakeLFO(48000., shape, true);

  const auto unipolarOut = RenderPerSample(unipolar, 3., 48000);
  const auto bipolarOut = RenderPerSample(bipolar, 3., 48000);

  double maxError = 0.;
  for (size_t s = 0; s < unipolarOut.size(); s++)
    maxError = std::max(maxError, std::fabs(unipolarOut[s] - (0.5 + 0.5 * bipolarOut[s])));

  CAPTURE(shape);
  REQUIRE(maxError < 1e-12);
}

// SUSPECTED BUG (inconsistency): IPlug/Extras/LFO.h:192-193. For every other shape the unipolar
// output is the bipolar output rescaled to [0, 1], but the two triangles are a quarter of a cycle
// apart: the bipolar triangle starts at its midpoint rising (like the sine), while the unipolar one
// starts at its minimum. Switching polarity therefore shifts the phase of a triangle LFO.
TEST_CASE("LFO: unipolar triangle is the bipolar triangle rescaled to [0, 1]", "[.][bug]")
{
  auto unipolar = MakeLFO(48000., Lfo::kTriangle, false);
  auto bipolar = MakeLFO(48000., Lfo::kTriangle, true);

  const auto unipolarOut = RenderPerSample(unipolar, 3., 48000);
  const auto bipolarOut = RenderPerSample(bipolar, 3., 48000);

  double maxError = 0.;
  for (size_t s = 0; s < unipolarOut.size(); s++)
    maxError = std::max(maxError, std::fabs(unipolarOut[s] - (0.5 + 0.5 * bipolarOut[s])));

  REQUIRE(maxError < 1e-9);
}

TEST_CASE("LFO: block processing matches per-sample processing", "[unit][extras][lfo]")
{
  const double sr = GENERATE(44100., 48000.);
  const int shape = GENERATE(range(0, static_cast<int>(Lfo::kNumShapes)));
  const bool bipolar = GENERATE(false, true);
  const int blockSize = GENERATE(1, 64, 1000);
  constexpr int nFrames = 20000;
  const double freq = 13.7;

  auto perSample = MakeLFO(sr, shape, bipolar);
  auto block = MakeLFO(sr, shape, bipolar);

  const auto perSampleOut = RenderPerSample(perSample, freq, nFrames);
  const auto blockOut = RenderBlocks(block, freq, nFrames, blockSize);

  CAPTURE(sr, shape, bipolar, blockSize);
  REQUIRE(extrastest::MaxAbsDiff(perSampleOut.data(), blockOut.data(), nFrames) < 1e-12);
}

TEST_CASE("LFO: scalar scales the output and GetLastOutput returns it", "[unit][extras][lfo]")
{
  const double scalar = GENERATE(0., 0.25, 2., -1.);

  auto reference = MakeLFO(48000., Lfo::kSine, true);
  auto scaled = MakeLFO(48000., Lfo::kSine, true);
  scaled.SetScalar(scalar);

  double maxError = 0.;
  bool lastOutputMatches = true;

  for (int s = 0; s < 10000; s++)
  {
    const double a = reference.Process(11.);
    const double b = scaled.Process(11.);
    maxError = std::max(maxError, std::fabs(b - a * scalar));

    if (scaled.GetLastOutput() != b)
      lastOutputMatches = false;
  }

  REQUIRE(maxError < 1e-12);
  REQUIRE(lastOutputMatches);

  std::vector<double> block(100);
  scaled.ProcessBlock(block.data(), 100);
  REQUIRE(scaled.GetLastOutput() == block.back());
}

TEST_CASE("LFO: SetShape clamps out of range values", "[unit][extras][lfo]")
{
  struct ShapePair
  {
    int requested, expected;
  };

  const auto shapes = GENERATE(ShapePair{-1, Lfo::kTriangle}, ShapePair{-100, Lfo::kTriangle}, ShapePair{Lfo::kNumShapes, Lfo::kSine}, ShapePair{1000, Lfo::kSine});

  auto lfo = MakeLFO(48000., shapes.requested, true);
  auto reference = MakeLFO(48000., shapes.expected, true);

  const auto out = RenderPerSample(lfo, 9., 12000);
  const auto refOut = RenderPerSample(reference, 9., 12000);

  CAPTURE(shapes.requested);
  REQUIRE(extrastest::MaxAbsDiff(out.data(), refOut.data(), 12000) == 0.);
}

TEST_CASE("LFO: Reset and SetPhase", "[unit][extras][lfo]")
{
  auto lfo = MakeLFO(48000., Lfo::kRampUp, false);

  const auto first = RenderPerSample(lfo, 7., 1000);
  lfo.Reset();
  const auto second = RenderPerSample(lfo, 7., 1000);
  REQUIRE(extrastest::MaxAbsDiff(first.data(), second.data(), 1000) == 0.);

  // phase is in cycles, and a unipolar ramp reads it back
  lfo.SetPhase(0.5);
  REQUIRE(lfo.Process(1.) == Approx(0.5).margin(1e-4));
  lfo.SetPhase(0.9);
  REQUIRE(lfo.Process(1.) == Approx(0.9).margin(1e-4));
}

TEST_CASE("LFO: tempo division scalars for the straight note values", "[unit][extras][lfo]")
{
  // the scalar is the number of LFO cycles per quarter note
  REQUIRE(Lfo::GetQNScalar(Lfo::k64th) == Approx(16.));
  REQUIRE(Lfo::GetQNScalar(Lfo::k32nd) == Approx(8.));
  REQUIRE(Lfo::GetQNScalar(Lfo::k16th) == Approx(4.));
  REQUIRE(Lfo::GetQNScalar(Lfo::k8th) == Approx(2.));
  REQUIRE(Lfo::GetQNScalar(Lfo::k4th) == Approx(1.));
  REQUIRE(Lfo::GetQNScalar(Lfo::k2th) == Approx(0.5));
  REQUIRE(Lfo::GetQNScalar(Lfo::k1) == Approx(0.25));
  REQUIRE(Lfo::GetQNScalar(Lfo::k2) == Approx(0.125));
  REQUIRE(Lfo::GetQNScalar(Lfo::k4) == Approx(0.0625));
  REQUIRE(Lfo::GetQNScalar(Lfo::k8) == Approx(0.03125));

  // three sixteenth note triplets to an eighth note
  REQUIRE(Lfo::GetQNScalar(Lfo::k16thT) == Approx(6.));

  REQUIRE(LFO<float>::GetQNScalar(LFO<float>::k16th) == Approx(4.f));
}

TEST_CASE("LFO: tempo division scalars get slower as the note values get longer", "[unit][extras][lfo]")
{
  // straight, dotted and triplet values interleave, so compare within each family
  const Lfo::ETempoDivison straight[] = {Lfo::k64th, Lfo::k32nd, Lfo::k16th, Lfo::k8th, Lfo::k4th, Lfo::k2th, Lfo::k1, Lfo::k2, Lfo::k4, Lfo::k8};
  const Lfo::ETempoDivison dotted[] = {Lfo::k16thD, Lfo::k8thD, Lfo::k4thD};
  const Lfo::ETempoDivison triplet[] = {Lfo::k16thT, Lfo::k8thT};

  for (size_t i = 1; i < sizeof(straight) / sizeof(straight[0]); i++)
    REQUIRE(Lfo::GetQNScalar(straight[i]) < Lfo::GetQNScalar(straight[i - 1]));

  for (size_t i = 1; i < sizeof(dotted) / sizeof(dotted[0]); i++)
    REQUIRE(Lfo::GetQNScalar(dotted[i]) < Lfo::GetQNScalar(dotted[i - 1]));

  REQUIRE(Lfo::GetQNScalar(triplet[1]) < Lfo::GetQNScalar(triplet[0]));

  for (int i = 0; i < Lfo::kNumDivisions; i++)
    REQUIRE(Lfo::GetQNScalar(static_cast<Lfo::ETempoDivison>(i)) > 0.);
}

// BUG: IPlug/Extras/LFO.h:80-85 the dotted and eighth-triplet entries of the GetQNScalar() table do
// not correspond to the note lengths they are named after. A dotted note is 1.5x as long as the
// straight note, so its rate is 2/3 of it (the table has 3/4), and an eighth note triplet is 2/3 as
// long as an eighth note, so its rate is 3 per quarter note (the table has 9/4, although the
// sixteenth note triplet is right at 6):
//   k16thD: 3 (should be 8/3)   k8thT: 2.25 (should be 3)
//   k8thD: 1.5 (should be 4/3)  k4thD: 0.75 (should be 2/3)
// So e.g. a "1/8T" LFO takes 4/9 of a beat rather than 1/3 and drifts against the host tempo.
TEST_CASE("LFO: tempo division scalars for dotted and triplet note values", "[.][bug]")
{
  REQUIRE(Lfo::GetQNScalar(Lfo::k16thD) == Approx(4. / 1.5));
  REQUIRE(Lfo::GetQNScalar(Lfo::k8thT) == Approx(2. * 1.5));
  REQUIRE(Lfo::GetQNScalar(Lfo::k8thD) == Approx(2. / 1.5));
  REQUIRE(Lfo::GetQNScalar(Lfo::k4thD) == Approx(1. / 1.5));
}

TEST_CASE("LFO: tempo division display strings", "[unit][extras][lfo]")
{
  REQUIRE(std::strcmp(Lfo::GetQNDisplay(Lfo::k64th), "1/64") == 0);
  REQUIRE(std::strcmp(Lfo::GetQNDisplay(Lfo::k16thT), "1/16T") == 0);
  REQUIRE(std::strcmp(Lfo::GetQNDisplay(Lfo::k8thD), "1/8D") == 0);
  REQUIRE(std::strcmp(Lfo::GetQNDisplay(Lfo::k4th), "1/4") == 0);
  REQUIRE(std::strcmp(Lfo::GetQNDisplay(Lfo::k1), "1/1") == 0);
  REQUIRE(std::strcmp(Lfo::GetQNDisplay(Lfo::k8), "8/1") == 0);

  for (int i = 0; i < Lfo::kNumDivisions; i++)
  {
    const char* str = Lfo::GetQNDisplay(static_cast<Lfo::ETempoDivison>(i));
    REQUIRE(str != nullptr);
    REQUIRE(std::strlen(str) > 0);
  }
}

TEST_CASE("LFO: tempo sync locks the phase to the transport position", "[unit][extras][lfo]")
{
  const double sr = GENERATE(44100., 48000.);
  const double tempo = GENERATE(60., 120., 140.);
  const int division = GENERATE(static_cast<int>(Lfo::k16th), static_cast<int>(Lfo::k8th), static_cast<int>(Lfo::k4th), static_cast<int>(Lfo::k2th), static_cast<int>(Lfo::k1));
  const double qnPos = GENERATE(0., 2.25, 1000.6);
  constexpr int nFrames = 4096;

  auto lfo = MakeLFO(sr, Lfo::kRampUp, false);
  lfo.SetRateMode(true);
  lfo.SetQNScalarFromDivision(division);

  std::vector<double> out(nFrames);
  lfo.ProcessBlock(out.data(), nFrames, qnPos, true, tempo);

  const double cyclesPerBeat = Lfo::GetQNScalar(static_cast<Lfo::ETempoDivison>(division));
  const double samplesPerBeat = sr * 60. / tempo;
  double maxError = 0.;

  for (int s = 0; s < nFrames; s++)
  {
    const double phase = Frac((qnPos + static_cast<double>(s) / samplesPerBeat) * cyclesPerBeat);

    if (NearWrap(phase))
      continue;

    maxError = std::max(maxError, std::fabs(out[static_cast<size_t>(s)] - phase));
  }

  CAPTURE(sr, tempo, division, qnPos);
  REQUIRE(maxError < 1e-6);

  // it is the position that matters, not what the LFO was doing before
  std::vector<double> again(nFrames);
  lfo.ProcessBlock(again.data(), nFrames, qnPos, true, tempo);
  REQUIRE(extrastest::MaxAbsDiff(out.data(), again.data(), nFrames) == 0.);
}

TEST_CASE("LFO: tempo sync with an explicit scalar", "[unit][extras][lfo]")
{
  const double sr = 48000.;
  constexpr int nFrames = 48000;

  // 3 cycles per beat at 60bpm is 3Hz
  auto lfo = MakeLFO(sr, Lfo::kSine, true);
  lfo.SetRateMode(true);
  lfo.SetQNScalar(3.);

  std::vector<double> out(nFrames);
  lfo.ProcessBlock(out.data(), nFrames, 0., true, 60.);

  const auto fit = extrastest::FitSinusoid(out.data(), nFrames, 2. * PI * 3. / sr);
  REQUIRE(fit.amplitude == Approx(1.).margin(1e-6));
  REQUIRE(fit.maxResidual < 1e-6);
}

TEST_CASE("LFO: tempo sync free-runs at the tempo when the transport is stopped", "[unit][extras][lfo]")
{
  const double sr = GENERATE(44100., 48000.);
  const double tempo = GENERATE(90., 120.);
  const int division = GENERATE(static_cast<int>(Lfo::k8th), static_cast<int>(Lfo::k4th), static_cast<int>(Lfo::k1));

  auto lfo = MakeLFO(sr, Lfo::kRampUp, false);
  lfo.SetRateMode(true);
  lfo.SetQNScalarFromDivision(division);

  const double cyclesPerSecond = (tempo / 60.) * Lfo::GetQNScalar(static_cast<Lfo::ETempoDivison>(division));
  const int nFrames = static_cast<int>(sr * 4.);
  constexpr int blockSize = 512;

  std::vector<double> out(static_cast<size_t>(nFrames));
  for (int start = 0; start < nFrames; start += blockSize)
    lfo.ProcessBlock(out.data() + start, std::min(blockSize, nFrames - start), 123.456 /* ignored */, false, tempo);

  double maxError = 0.;
  for (int s = 0; s < nFrames; s++)
  {
    const double phase = Frac(static_cast<double>(s + 1) * cyclesPerSecond / sr);

    if (NearWrap(phase))
      continue;

    maxError = std::max(maxError, std::fabs(out[static_cast<size_t>(s)] - phase));
  }

  CAPTURE(sr, tempo, division);
  REQUIRE(maxError < 1e-6);
}

TEST_CASE("LFO: Hz mode ignores the transport", "[unit][extras][lfo]")
{
  constexpr int nFrames = 10000;

  auto a = MakeLFO(48000., Lfo::kSine, true);
  auto b = MakeLFO(48000., Lfo::kSine, true);
  b.SetQNScalarFromDivision(Lfo::k16th);
  b.SetRateMode(false);
  a.SetFreqCPS(5.);
  b.SetFreqCPS(5.);

  std::vector<double> outA(nFrames), outB(nFrames);
  a.ProcessBlock(outA.data(), nFrames);
  b.ProcessBlock(outB.data(), nFrames, 17.5, true, 174.);

  REQUIRE(extrastest::MaxAbsDiff(outA.data(), outB.data(), nFrames) == 0.);

  const auto fit = extrastest::FitSinusoid(outA.data(), nFrames, 2. * PI * 5. / 48000.);
  REQUIRE(fit.amplitude == Approx(1.).margin(1e-9));
  REQUIRE(fit.maxResidual < 1e-9);
}

TEST_CASE("LFO: a tempo of zero does not produce invalid output", "[unit][extras][lfo]")
{
  const bool transportIsRunning = GENERATE(false, true);

  auto lfo = MakeLFO(48000., Lfo::kSine, true);
  lfo.SetRateMode(true);

  std::vector<double> out(1024, 99.);
  lfo.ProcessBlock(out.data(), 1024, 0., transportIsRunning, 0.);

  REQUIRE(extrastest::AllFinite(out.data(), 1024));
  REQUIRE(extrastest::PeakAbs(out.data(), 1024) <= 1.);
}

TEST_CASE("LFO: single precision", "[unit][extras][lfo]")
{
  const int shape = GENERATE(range(0, static_cast<int>(LFO<float>::kNumShapes)));

  LFO<float> lfo;
  lfo.SetSampleRate(48000.);
  lfo.SetShape(shape);
  lfo.SetPolarity(true);
  lfo.SetFreqCPS(10.);

  std::vector<float> out(48000);
  lfo.ProcessBlock(out.data(), 48000);

  REQUIRE(extrastest::AllFinite(out.data(), 48000));
  REQUIRE(extrastest::PeakAbs(out.data(), 48000) <= 1.0 + 1e-6);
  REQUIRE(extrastest::PeakAbs(out.data(), 48000) > 0.99);
  REQUIRE(std::abs(extrastest::CountRisingZeroCrossings(out.data(), 48000) - 10) <= 1);
}

TEST_CASE("LFO: SetQNScalarFromDivision clamps out of range divisions", "[unit][extras][lfo]")
{
  constexpr int nFrames = 4096;
  const int outOfRange = GENERATE(static_cast<int>(Lfo::kNumDivisions), 1000);

  auto lfo = MakeLFO(48000., Lfo::kRampUp, false);
  auto reference = MakeLFO(48000., Lfo::kRampUp, false);
  lfo.SetRateMode(true);
  reference.SetRateMode(true);

  lfo.SetQNScalarFromDivision(outOfRange);
  reference.SetQNScalarFromDivision(Lfo::k8);

  std::vector<double> out(nFrames), refOut(nFrames);
  lfo.ProcessBlock(out.data(), nFrames, 3.3, true, 120.);
  reference.ProcessBlock(refOut.data(), nFrames, 3.3, true, 120.);

  REQUIRE(extrastest::MaxAbsDiff(out.data(), refOut.data(), nFrames) == 0.);
}
