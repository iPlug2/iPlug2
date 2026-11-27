/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for DCBlocker (IPlug/Extras/DCBlocker.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

#include "DCBlocker.h"
#include "ExtrasTestUtils.h"

using namespace iplug;
using Catch::Approx;

namespace
{
/** Run a mono signal through a fresh DC blocker in blocks of 64 */
std::vector<double> Process(const std::vector<double>& input)
{
  DCBlocker<double, 1> blocker;
  std::vector<double> in = input;
  std::vector<double> out(in.size(), 0.);
  const int nFrames = static_cast<int>(in.size());

  for (int start = 0; start < nFrames; start += 64)
  {
    double* ins[1] = {in.data() + start};
    double* outs[1] = {out.data() + start};
    blocker.ProcessBlock(ins, outs, 1, std::min(64, nFrames - start));
  }

  return out;
}
} // namespace

TEST_CASE("DCBlocker: removes a constant offset", "[unit][extras][dcblocker]")
{
  const double dc = GENERATE(1., -0.25, 100.);
  constexpr int nFrames = 8192;

  const std::vector<double> in(nFrames, dc);
  const auto out = Process(in);

  // the step itself gets through...
  REQUIRE(out[0] == Approx(dc));

  // ...and then decays away monotonically
  bool monotonic = true;
  for (size_t s = 1; s < nFrames; s++)
  {
    if (std::fabs(out[s]) > std::fabs(out[s - 1]))
      monotonic = false;
  }

  REQUIRE(monotonic);
  REQUIRE(std::fabs(out[2000]) < std::fabs(dc) * 1e-3);
  REQUIRE(std::fabs(out[nFrames - 1]) < std::fabs(dc) * 1e-9);
}

TEST_CASE("DCBlocker: removes the offset from an offset sine and keeps the sine", "[unit][extras][dcblocker]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const int nFrames = static_cast<int>(sr);
  const int nSettle = nFrames / 2;
  const int nMeasure = nFrames - nSettle;

  const auto in = extrastest::MakeSine<double>(1000., sr, nFrames, 0.5, 0.3);
  const auto out = Process(in);

  CAPTURE(sr);
  REQUIRE(extrastest::Mean(in.data() + nSettle, nMeasure) == Approx(0.3).margin(1e-3));
  REQUIRE(extrastest::Mean(out.data() + nSettle, nMeasure) == Approx(0.).margin(1e-3));
  REQUIRE(extrastest::RMS(out.data() + nSettle, nMeasure) == Approx(0.5 / std::sqrt(2.)).epsilon(0.01));
  REQUIRE(extrastest::CountRisingZeroCrossings(out.data() + nSettle, nMeasure) == Approx(500).margin(1));
}

TEST_CASE("DCBlocker: passes audio band signals at unity gain", "[unit][extras][dcblocker]")
{
  const double sr = GENERATE(44100., 48000.);
  const double freq = GENERATE(200., 1000., 5000., 15000.);
  constexpr int nFrames = 16384;
  constexpr int nSettle = 8192;

  const auto in = extrastest::MakeSine<double>(freq, sr, nFrames);
  const auto out = Process(in);

  const double gainDB = extrastest::AmpToDB(extrastest::RMS(out.data() + nSettle, nFrames - nSettle) / extrastest::RMS(in.data() + nSettle, nFrames - nSettle));

  CAPTURE(sr, freq);
  REQUIRE(gainDB == Approx(0.).margin(0.25));
}

TEST_CASE("DCBlocker: attenuates subsonic content", "[unit][extras][dcblocker]")
{
  const double sr = 48000.;
  const int nFrames = 96000;
  const int nSettle = 48000;

  const auto in = extrastest::MakeSine<double>(2., sr, nFrames);
  const auto out = Process(in);

  REQUIRE(extrastest::RMS(out.data() + nSettle, nFrames - nSettle) < 0.1 * extrastest::RMS(in.data() + nSettle, nFrames - nSettle));
}

TEST_CASE("DCBlocker: silence in, silence out", "[unit][extras][dcblocker]")
{
  const std::vector<double> in(1024, 0.);
  const auto out = Process(in);
  REQUIRE(extrastest::PeakAbs(out.data(), 1024) == 0.);
}

TEST_CASE("DCBlocker: channels are filtered independently", "[unit][extras][dcblocker]")
{
  constexpr int nFrames = 4096;

  DCBlocker<double, 2> stereo;
  extrastest::Buffers<double> in(2, nFrames), out(2, nFrames, 55.);
  in[0].assign(nFrames, 1.);
  in[1] = extrastest::MakeSine<double>(1000., 48000., nFrames, 1., -0.5);

  stereo.ProcessBlock(in.Get(), out.Get(), 2, nFrames);

  const auto refL = Process(in[0]);
  const auto refR = Process(in[1]);

  REQUIRE(extrastest::MaxAbsDiff(out[0].data(), refL.data(), nFrames) < 1e-12);
  REQUIRE(extrastest::MaxAbsDiff(out[1].data(), refR.data(), nFrames) < 1e-12);
}

TEST_CASE("DCBlocker: processing fewer channels leaves the others alone", "[unit][extras][dcblocker]")
{
  constexpr int nFrames = 256;
  constexpr double kSentinel = 55.;

  DCBlocker<double, 2> stereo;
  extrastest::Buffers<double> in(2, nFrames, 1.), out(2, nFrames, kSentinel);

  stereo.ProcessBlock(in.Get(), out.Get(), 1, nFrames);

  REQUIRE(out[0][0] == Approx(1.));
  for (size_t s = 0; s < nFrames; s++)
    REQUIRE(out[1][s] == kSentinel);

  // the second channel's state was untouched, so it now behaves like a fresh filter
  stereo.ProcessBlock(in.Get(), out.Get(), 2, nFrames);
  const auto ref = Process(in[1]);
  REQUIRE(extrastest::MaxAbsDiff(out[1].data(), ref.data(), nFrames) < 1e-12);
}

TEST_CASE("DCBlocker: works in place", "[unit][extras][dcblocker]")
{
  constexpr int nFrames = 2048;
  const auto in = extrastest::MakeSine<double>(440., 44100., nFrames, 0.5, 0.5);
  const auto ref = Process(in);

  DCBlocker<double, 1> blocker;
  std::vector<double> buf = in;
  double* ptrs[1] = {buf.data()};
  blocker.ProcessBlock(ptrs, ptrs, 1, nFrames);

  REQUIRE(extrastest::MaxAbsDiff(buf.data(), ref.data(), nFrames) < 1e-12);
}

TEST_CASE("DCBlocker: single precision removes DC and stays finite", "[unit][extras][dcblocker]")
{
  constexpr int nFrames = 48000;
  constexpr int nSettle = 24000;

  DCBlocker<float, 1> blocker;
  auto in = extrastest::MakeSine<float>(1000., 48000., nFrames, 0.5, 0.25);
  std::vector<float> out(nFrames, 0.f);
  float* ins[1] = {in.data()};
  float* outs[1] = {out.data()};

  blocker.ProcessBlock(ins, outs, 1, nFrames);

  REQUIRE(extrastest::AllFinite(out.data(), nFrames));
  REQUIRE(extrastest::Mean(out.data() + nSettle, nFrames - nSettle) == Approx(0.).margin(1e-3));
  REQUIRE(extrastest::RMS(out.data() + nSettle, nFrames - nSettle) == Approx(0.5 / std::sqrt(2.)).epsilon(0.01));
}
