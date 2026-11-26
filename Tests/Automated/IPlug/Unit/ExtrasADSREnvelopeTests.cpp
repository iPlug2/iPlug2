/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for ADSREnvelope (IPlug/Extras/ADSREnvelope.h)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "ADSREnvelope.h"
#include "ExtrasTestUtils.h"

using namespace iplug;
using Catch::Approx;

namespace
{
using Env = ADSREnvelope<double>;

// The attack stage ends by jumping to 1.0 when the ramp passes ENV_VALUE_HIGH (0.999)
const double kAttackDone = 0.9995;

/** Sample rate has to be set before the stage times, as the increments are derived from it */
void Setup(Env& env, double sr, double attackMs, double decayMs, double releaseMs)
{
  env.SetSampleRate(sr);
  env.SetStageTime(Env::kAttack, attackMs);
  env.SetStageTime(Env::kDecay, decayMs);
  env.SetStageTime(Env::kRelease, releaseMs);
}

void Run(Env& env, int nSamples, double sustain)
{
  for (int s = 0; s < nSamples; s++)
    env.Process(sustain);
}

int MsToSamples(double ms, double sr)
{
  return static_cast<int>(std::lround(ms * 0.001 * sr));
}
} // namespace

TEST_CASE("ADSREnvelope: idle until started", "[unit][extras][adsr]")
{
  Env env;
  Setup(env, 44100., 10., 10., 10.);

  REQUIRE_FALSE(env.GetBusy());
  REQUIRE(env.GetReleased());

  for (int s = 0; s < 100; s++)
    REQUIRE(env.Process(0.5) == 0.);

  REQUIRE(env.GetPrevOutput() == 0.);
  REQUIRE_FALSE(env.GetBusy());
}

TEST_CASE("ADSREnvelope: attack is a linear ramp lasting the attack time", "[unit][extras][adsr]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const double attackMs = GENERATE(1., 10., 50.);

  Env env;
  Setup(env, sr, attackMs, 100., 100.);
  env.Start(1.);

  REQUIRE(env.GetBusy());
  REQUIRE_FALSE(env.GetReleased());

  const int expected = MsToSamples(attackMs, sr);
  int nAttack = 0;
  double prev = 0.;
  bool monotonic = true;
  bool linear = true;

  // a sustain level of 1 means the output stays at 1 once the attack has finished
  for (int s = 0; s < expected * 2; s++)
  {
    const double v = env.Process(1.);

    if (v < prev)
      monotonic = false;

    if (v < kAttackDone)
    {
      nAttack++;
      const double ideal = static_cast<double>(s + 1) / static_cast<double>(expected);
      if (std::fabs(v - ideal) > 0.01)
        linear = false;
    }

    prev = v;
  }

  CAPTURE(sr, attackMs, nAttack, expected);
  REQUIRE(monotonic);
  REQUIRE(linear);
  // the ramp is cut short by 0.1%, when it passes ENV_VALUE_HIGH
  REQUIRE(std::abs(nAttack - expected) <= 2 + expected / 500);
  REQUIRE(prev == Approx(1.).margin(1e-9));
}

TEST_CASE("ADSREnvelope: decay falls 60dB towards the sustain level in the decay time", "[unit][extras][adsr]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const double sustain = GENERATE(0., 0.25, 0.8);
  const double attackMs = 2.;
  const double decayMs = 40.;

  Env env;
  Setup(env, sr, attackMs, decayMs, 100.);
  env.Start(1.);

  const int nAttack = MsToSamples(attackMs, sr);
  const int nDecay = MsToSamples(decayMs, sr);

  Run(env, nAttack + 2, sustain);

  double prev = env.GetPrevOutput();
  REQUIRE(prev > 0.98);

  // half way through the decay time we should be 30dB down on the way to the sustain level
  bool monotonic = true;
  for (int s = 0; s < nDecay / 2; s++)
  {
    const double v = env.Process(sustain);
    if (v > prev)
      monotonic = false;
    prev = v;
  }

  CAPTURE(sr, sustain);
  REQUIRE(monotonic);
  const double halfwayDB = extrastest::AmpToDB((prev - sustain) / (1. - sustain));
  REQUIRE(halfwayDB == Approx(-30.).margin(1.5));

  Run(env, nDecay - (nDecay / 2), sustain);
  const double endDB = extrastest::AmpToDB((env.GetPrevOutput() - sustain) / (1. - sustain));
  REQUIRE(endDB == Approx(-60.).margin(2.));

  // ...and well after the decay time the envelope holds the sustain level exactly
  Run(env, nDecay * 2, sustain);
  for (int s = 0; s < 100; s++)
    REQUIRE(env.Process(sustain) == Approx(sustain).margin(1e-12));

  REQUIRE(env.GetBusy());
  REQUIRE_FALSE(env.GetReleased());
}

TEST_CASE("ADSREnvelope: sustain stage follows the sustain level argument", "[unit][extras][adsr]")
{
  Env env;
  Setup(env, 48000., 1., 1., 10.);
  env.Start(1.);
  Run(env, 48000 / 10, 0.5);

  REQUIRE(env.Process(0.5) == Approx(0.5));
  REQUIRE(env.Process(0.3) == Approx(0.3));
  REQUIRE(env.Process(1.0) == Approx(1.0));
  REQUIRE(env.Process(0.0) == Approx(0.0).margin(1e-12));
  REQUIRE(env.GetBusy()); // a sustain level of zero does not end the envelope
}

TEST_CASE("ADSREnvelope: start level scales the whole envelope", "[unit][extras][adsr]")
{
  const double level = GENERATE(0.1, 0.5, 1.0);

  Env ref, scaled;
  Setup(ref, 44100., 5., 20., 30.);
  Setup(scaled, 44100., 5., 20., 30.);
  ref.Start(1.);
  scaled.Start(level);

  double maxDiff = 0.;

  for (int s = 0; s < 5000; s++)
  {
    if (s == 3000)
    {
      ref.Release();
      scaled.Release();
    }

    const double a = ref.Process(0.6);
    const double b = scaled.Process(0.6);
    maxDiff = std::max(maxDiff, std::fabs(b - a * level));
  }

  REQUIRE(maxDiff < 1e-12);
  REQUIRE(scaled.GetPrevOutput() == Approx(ref.GetPrevOutput() * level).margin(1e-12));
}

TEST_CASE("ADSREnvelope: GetPrevOutput returns the last processed value", "[unit][extras][adsr]")
{
  Env env;
  Setup(env, 44100., 5., 20., 30.);
  env.Start(0.75);

  for (int s = 0; s < 2000; s++)
  {
    const double v = env.Process(0.4);
    REQUIRE(env.GetPrevOutput() == v);
  }
}

TEST_CASE("ADSREnvelope: release decays 60dB in the release time then goes idle", "[unit][extras][adsr]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const double sustain = 0.7;
  const double releaseMs = 30.;

  Env env;
  Setup(env, sr, 1., 1., releaseMs);

  int nEndCalls = 0;
  env.SetEndReleaseFunc([&nEndCalls]() { nEndCalls++; });

  env.Start(1.);
  Run(env, MsToSamples(20., sr), sustain);
  REQUIRE(env.GetPrevOutput() == Approx(sustain));

  env.Release();
  REQUIRE(env.GetReleased());
  REQUIRE(env.GetBusy());

  const int nRelease = MsToSamples(releaseMs, sr);
  double prev = sustain;
  bool monotonic = true;

  for (int s = 0; s < nRelease; s++)
  {
    const double v = env.Process(sustain);
    if (v > prev)
      monotonic = false;
    prev = v;
  }

  CAPTURE(sr);
  REQUIRE(monotonic);
  REQUIRE(extrastest::AmpToDB(prev / sustain) == Approx(-60.).margin(2.));
  REQUIRE(nEndCalls == 0);

  // the envelope carries on to -120dB before it goes idle, i.e. about twice the release time
  int nUntilIdle = 0;
  while (env.GetBusy() && nUntilIdle < nRelease * 4)
  {
    env.Process(sustain);
    nUntilIdle++;
  }

  REQUIRE_FALSE(env.GetBusy());
  REQUIRE(nUntilIdle <= nRelease + nRelease / 10 + 2);
  REQUIRE(nEndCalls == 1);

  for (int s = 0; s < 100; s++)
    REQUIRE(env.Process(sustain) == 0.);

  REQUIRE(nEndCalls == 1);
  REQUIRE(env.GetReleased());
}

TEST_CASE("ADSREnvelope: release during the attack starts from the current level", "[unit][extras][adsr]")
{
  const double sr = 48000.;
  Env env;
  Setup(env, sr, 100., 100., 50.);
  env.Start(1.);

  Run(env, MsToSamples(50., sr), 0.5);
  const double levelAtRelease = env.GetPrevOutput();
  REQUIRE(levelAtRelease == Approx(0.5).margin(0.01));

  env.Release();
  REQUIRE(env.GetReleased());

  const double first = env.Process(0.5);
  REQUIRE(first <= levelAtRelease);
  REQUIRE(first > levelAtRelease * 0.99); // no jump

  double prev = first;
  bool monotonic = true;

  for (int s = 0; s < MsToSamples(120., sr); s++)
  {
    const double v = env.Process(0.5);
    if (v > prev)
      monotonic = false;
    prev = v;
  }

  REQUIRE(monotonic);
  REQUIRE(prev == 0.);
  REQUIRE_FALSE(env.GetBusy());
}

TEST_CASE("ADSREnvelope: release during the decay starts from the current level", "[unit][extras][adsr]")
{
  const double sr = 44100.;
  Env env;
  Setup(env, sr, 1., 200., 50.);
  env.Start(1.);

  Run(env, MsToSamples(30., sr), 0.2);
  const double levelAtRelease = env.GetPrevOutput();
  REQUIRE(levelAtRelease > 0.3);
  REQUIRE(levelAtRelease < 1.0);

  env.Release();
  const double first = env.Process(0.2);
  REQUIRE(first <= levelAtRelease);
  REQUIRE(first > levelAtRelease * 0.99);
}

TEST_CASE("ADSREnvelope: retrigger fades out quickly, calls the reset func and restarts", "[unit][extras][adsr]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const double sustain = 0.8;

  int nResetCalls = 0;
  Env env("test", [&nResetCalls]() { nResetCalls++; });
  Setup(env, sr, 10., 5., 100.);
  env.Start(1.);
  Run(env, MsToSamples(50., sr), sustain);
  REQUIRE(env.GetPrevOutput() == Approx(sustain));

  const double newLevel = 0.5;
  env.Retrigger(newLevel);
  REQUIRE(env.GetBusy());
  REQUIRE_FALSE(env.GetReleased());

  // linear fade from the current level to zero in RETRIGGER_RELEASE_TIME
  const int nFade = MsToSamples(Env::RETRIGGER_RELEASE_TIME, sr);
  double prev = sustain;
  int nFadeMeasured = 0;
  bool monotonic = true;
  bool linear = true;

  while (nResetCalls == 0 && nFadeMeasured < nFade * 2)
  {
    const double v = env.Process(sustain);
    const double ideal = sustain * (1. - static_cast<double>(nFadeMeasured + 1) / static_cast<double>(nFade));

    if (v > prev || v < 0.)
      monotonic = false;
    if (std::fabs(v - ideal) > 0.02)
      linear = false;

    prev = v;
    nFadeMeasured++;
  }

  CAPTURE(sr, nFadeMeasured, nFade);
  REQUIRE(monotonic);
  REQUIRE(linear);
  REQUIRE(nResetCalls == 1);
  REQUIRE(std::abs(nFadeMeasured - nFade) <= 2);
  REQUIRE(prev == Approx(0.).margin(0.01));

  // then a fresh attack, scaled by the new level
  const int nAttack = MsToSamples(10., sr);
  prev = 0.;
  monotonic = true;

  for (int s = 0; s < nAttack - 3; s++)
  {
    const double v = env.Process(sustain);
    if (v < prev)
      monotonic = false;
    prev = v;
  }

  REQUIRE(monotonic);
  REQUIRE(prev == Approx(newLevel).margin(0.02));
  REQUIRE(prev < newLevel);
  REQUIRE(nResetCalls == 1);

  Run(env, MsToSamples(50., sr), sustain);
  REQUIRE(env.GetPrevOutput() == Approx(sustain * newLevel));
}

TEST_CASE("ADSREnvelope: soft kill fades out in EARLY_RELEASE_TIME", "[unit][extras][adsr]")
{
  const double sr = GENERATE(44100., 48000., 96000.);
  const double sustain = 0.9;

  Env env;
  Setup(env, sr, 1., 1., 5000.);

  int nEndCalls = 0;
  env.SetEndReleaseFunc([&nEndCalls]() { nEndCalls++; });

  env.Start(1.);
  Run(env, MsToSamples(20., sr), sustain);

  env.Kill(false);
  REQUIRE(env.GetBusy());

  const int nFade = MsToSamples(Env::EARLY_RELEASE_TIME, sr);
  double prev = sustain;
  int n = 0;
  bool monotonic = true;

  while (env.GetBusy() && n < nFade * 2)
  {
    const double v = env.Process(sustain);
    if (v > prev || v < 0.)
      monotonic = false;
    prev = v;
    n++;
  }

  CAPTURE(sr, n, nFade);
  REQUIRE(monotonic);
  REQUIRE_FALSE(env.GetBusy());
  REQUIRE(std::abs(n - nFade) <= 2);
  REQUIRE(prev == Approx(0.).margin(1e-3));
  REQUIRE(nEndCalls == 1);
  REQUIRE(env.Process(sustain) == 0.);
}

TEST_CASE("ADSREnvelope: hard kill silences the envelope immediately", "[unit][extras][adsr]")
{
  Env env;
  Setup(env, 44100., 1., 1., 5000.);
  env.Start(1.);
  Run(env, 2000, 0.9);
  REQUIRE(env.GetPrevOutput() == Approx(0.9));

  env.Kill(true);
  REQUIRE_FALSE(env.GetBusy());

  for (int s = 0; s < 10; s++)
    REQUIRE(env.Process(0.9) == 0.);
}

TEST_CASE("ADSREnvelope: killing an idle envelope is a no-op", "[unit][extras][adsr]")
{
  const bool hard = GENERATE(true, false);

  Env env;
  Setup(env, 44100., 1., 1., 10.);

  int nEndCalls = 0;
  env.SetEndReleaseFunc([&nEndCalls]() { nEndCalls++; });

  env.Kill(hard);
  REQUIRE_FALSE(env.GetBusy());
  Run(env, 2000, 0.5);
  REQUIRE(env.GetPrevOutput() == 0.);
  REQUIRE(nEndCalls == 0);
}

TEST_CASE("ADSREnvelope: time scalar stretches the stage times", "[unit][extras][adsr]")
{
  const double sr = 48000.;
  const double timeScalar = GENERATE(0.5, 2., 4.);
  const double attackMs = 10.;

  Env env;
  Setup(env, sr, attackMs, 10., 10.);
  env.Start(1., timeScalar);

  int nAttack = 0;
  while (env.Process(1.) < kAttackDone && nAttack < 100000)
    nAttack++;

  const int expected = MsToSamples(attackMs * timeScalar, sr);
  CAPTURE(timeScalar, nAttack, expected);
  REQUIRE(std::abs(nAttack - expected) <= 3 + expected / 500);
}

TEST_CASE("ADSREnvelope: stage times are clamped to a minimum rather than getting stuck", "[unit][extras][adsr]")
{
  const double sr = GENERATE(44100., 48000., 96000.);

  Env env;
  Setup(env, sr, 0., 0., 0.);
  env.Start(1.);

  // a zero attack is clamped to (about) one sample at 44.1kHz
  const int maxAttack = static_cast<int>(std::ceil(sr / 44100.)) + 1;
  int n = 0;
  while (env.Process(1.) < kAttackDone && n < 100)
    n++;
  REQUIRE(n <= maxAttack);

  // minimum decay and release still terminate, and quickly
  Run(env, 64, 0.5);
  REQUIRE(env.GetPrevOutput() == Approx(0.5));

  env.Release();
  Run(env, 64, 0.5);
  REQUIRE_FALSE(env.GetBusy());
  REQUIRE(env.GetPrevOutput() == 0.);
}

TEST_CASE("ADSREnvelope: AD mode releases by itself after the decay", "[unit][extras][adsr]")
{
  const double sr = 44100.;
  int nEndCalls = 0;

  Env env("ad", nullptr, false);
  Setup(env, sr, 5., 20., 20.);
  env.SetEndReleaseFunc([&nEndCalls]() { nEndCalls++; });
  env.Start(1.);

  REQUIRE(env.GetBusy());
  REQUIRE_FALSE(env.GetReleased());

  double peak = 0.;
  bool bounded = true;
  int n = 0;
  const int limit = MsToSamples(200., sr);

  while (env.GetBusy() && n < limit)
  {
    const double v = env.Process(0.);
    if (v < 0. || v > 1.)
      bounded = false;
    peak = std::max(peak, v);
    n++;
  }

  REQUIRE(bounded);
  REQUIRE(peak == Approx(1.).margin(1e-9));
  REQUIRE_FALSE(env.GetBusy());
  REQUIRE(env.GetReleased());
  REQUIRE(nEndCalls == 1);
  REQUIRE(n > MsToSamples(20., sr));
  REQUIRE(env.Process(0.) == 0.);
}

TEST_CASE("ADSREnvelope: can be restarted after finishing", "[unit][extras][adsr]")
{
  Env env;
  Setup(env, 44100., 2., 2., 2.);

  for (int i = 0; i < 3; i++)
  {
    env.Start(1.);
    REQUIRE(env.GetBusy());
    REQUIRE_FALSE(env.GetReleased());
    Run(env, 1000, 0.5);
    REQUIRE(env.GetPrevOutput() == Approx(0.5));
    env.Release();
    Run(env, 1000, 0.5);
    REQUIRE_FALSE(env.GetBusy());
    REQUIRE(env.GetPrevOutput() == 0.);
  }
}

TEST_CASE("ADSREnvelope: output stays within [0, 1] for arbitrary event sequences", "[unit][extras][adsr]")
{
  const double sr = GENERATE(44100., 96000.);
  extrastest::LCG lcg(2024u);

  Env env;
  Setup(env, sr, 3., 7., 11.);

  bool bounded = true;
  bool finite = true;

  for (int s = 0; s < 60000; s++)
  {
    // every so often do something to the envelope
    if ((s % 173) == 0)
    {
      switch (lcg.NextInt() % 7u)
      {
        case 0: env.Start(lcg.NextUnipolar()); break;
        case 1: env.Release(); break;
        case 2: env.Retrigger(lcg.NextUnipolar()); break;
        case 3: env.Kill(false); break;
        case 4: env.Kill(true); break;
        case 5: env.SetStageTime(static_cast<int>(lcg.NextInt() % 4u), lcg.NextUnipolar() * 50.); break;
        default: break;
      }
    }

    const double v = env.Process(lcg.NextUnipolar());

    if (!std::isfinite(v))
      finite = false;
    if (v < 0. || v > 1.)
      bounded = false;
  }

  REQUIRE(finite);
  REQUIRE(bounded);
}

TEST_CASE("ADSREnvelope: single precision tracks double precision", "[unit][extras][adsr]")
{
  ADSREnvelope<float> envF;
  Env envD;

  envF.SetSampleRate(48000.f);
  envF.SetStageTime(ADSREnvelope<float>::kAttack, 20.f);
  envF.SetStageTime(ADSREnvelope<float>::kDecay, 20.f);
  envF.SetStageTime(ADSREnvelope<float>::kRelease, 20.f);
  Setup(envD, 48000., 20., 20., 20.);

  envF.Start(0.8f);
  envD.Start(0.8);

  double maxDiff = 0.;

  for (int s = 0; s < 10000; s++)
  {
    if (s == 6000)
    {
      envF.Release();
      envD.Release();
    }

    const double f = static_cast<double>(envF.Process(0.5f));
    const double d = envD.Process(0.5);
    maxDiff = std::max(maxDiff, std::fabs(f - d));
  }

  REQUIRE(maxDiff < 5e-3);
  REQUIRE_FALSE(envF.GetBusy());
}
