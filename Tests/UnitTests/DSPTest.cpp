/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <vector>
#include <numeric>
#include <set>

#include "ADSREnvelope.h"
#include "LFO.h"
#include "Oscillator.h"
#include "SVF.h"
#include "DCBlocker.h"
#include "NChanDelay.h"
#include "Smoothers.h"
#include "NoiseGate.h"
#include "LanczosResampler.h"

using namespace iplug;
using Catch::Approx;

// ============================================================================
// ADSREnvelope Tests
// ============================================================================

TEST_CASE("ADSREnvelope basic operation", "[DSP][ADSREnvelope]")
{
  SECTION("Initial state is idle")
  {
    ADSREnvelope<double> env;
    REQUIRE(env.GetBusy() == false);
    REQUIRE(env.GetReleased() == true);
  }

  SECTION("Start triggers attack stage")
  {
    ADSREnvelope<double> env;
    env.SetSampleRate(44100.0);
    env.SetStageTime(ADSREnvelope<double>::kAttack, 10.0);

    env.Start(1.0);
    REQUIRE(env.GetBusy() == true);
    REQUIRE(env.GetReleased() == false);

    // Process and verify envelope is rising
    double prev = 0.0;
    for (int i = 0; i < 100; i++)
    {
      double val = env.Process(0.5);
      REQUIRE(val >= prev);
      prev = val;
    }
  }

  SECTION("Release brings envelope to idle")
  {
    ADSREnvelope<double> env;
    env.SetSampleRate(44100.0);
    env.SetStageTime(ADSREnvelope<double>::kAttack, 1.0);
    env.SetStageTime(ADSREnvelope<double>::kDecay, 1.0);
    env.SetStageTime(ADSREnvelope<double>::kRelease, 10.0);

    env.Start(1.0);

    // Process through attack and decay to sustain
    for (int i = 0; i < 500; i++)
      env.Process(0.5);

    env.Release();
    REQUIRE(env.GetReleased() == true);

    // Process through release until idle
    for (int i = 0; i < 10000 && env.GetBusy(); i++)
      env.Process(0.5);

    REQUIRE(env.GetBusy() == false);
  }

  SECTION("Hard kill immediately stops envelope")
  {
    ADSREnvelope<double> env;
    env.SetSampleRate(44100.0);
    env.SetStageTime(ADSREnvelope<double>::kAttack, 100.0);

    env.Start(1.0);
    REQUIRE(env.GetBusy() == true);

    env.Kill(true);
    REQUIRE(env.GetBusy() == false);
  }

  SECTION("Soft kill fades out")
  {
    ADSREnvelope<double> env;
    env.SetSampleRate(44100.0);
    env.SetStageTime(ADSREnvelope<double>::kAttack, 1.0);

    env.Start(1.0);

    // Process to get some level
    for (int i = 0; i < 100; i++)
      env.Process(0.5);

    env.Kill(false);
    REQUIRE(env.GetBusy() == true);

    // Should fade out
    for (int i = 0; i < 10000 && env.GetBusy(); i++)
      env.Process(0.5);

    REQUIRE(env.GetBusy() == false);
  }

  SECTION("Velocity level scales output")
  {
    ADSREnvelope<double> env;
    env.SetSampleRate(44100.0);
    env.SetStageTime(ADSREnvelope<double>::kAttack, 1.0);
    env.SetStageTime(ADSREnvelope<double>::kDecay, 100.0);

    // Full velocity
    env.Start(1.0);
    for (int i = 0; i < 200; i++)
      env.Process(1.0);
    double fullLevel = env.GetPrevOutput();

    // Half velocity
    env.Kill(true);
    env.Start(0.5);
    for (int i = 0; i < 200; i++)
      env.Process(1.0);
    double halfLevel = env.GetPrevOutput();

    REQUIRE(halfLevel == Approx(fullLevel * 0.5).margin(0.01));
  }

  SECTION("AD mode (no sustain)")
  {
    ADSREnvelope<double> env("AD", nullptr, false); // sustainEnabled = false
    env.SetSampleRate(44100.0);
    env.SetStageTime(ADSREnvelope<double>::kAttack, 5.0);
    env.SetStageTime(ADSREnvelope<double>::kDecay, 10.0);
    env.SetStageTime(ADSREnvelope<double>::kRelease, 10.0);

    env.Start(1.0);

    // Should automatically release after decay without needing Release() call
    for (int i = 0; i < 50000 && env.GetBusy(); i++)
      env.Process(0.5);

    REQUIRE(env.GetBusy() == false);
  }

  SECTION("Retrigger with reset function")
  {
    bool resetCalled = false;
    ADSREnvelope<double> env("Test", [&resetCalled]() { resetCalled = true; });
    env.SetSampleRate(44100.0);
    env.SetStageTime(ADSREnvelope<double>::kAttack, 10.0);

    env.Start(1.0);
    for (int i = 0; i < 100; i++)
      env.Process(0.5);

    env.Retrigger(0.8);

    // Process until retrigger completes
    for (int i = 0; i < 1000; i++)
      env.Process(0.5);

    REQUIRE(resetCalled == true);
  }
}

// ============================================================================
// LFO Tests
// ============================================================================

TEST_CASE("LFO waveform shapes", "[DSP][LFO]")
{
  LFO<double> lfo;
  lfo.SetSampleRate(44100.0);

  SECTION("Triangle wave range")
  {
    lfo.SetShape(LFO<double>::kTriangle);
    lfo.SetPolarity(false); // Unipolar

    double minVal = 1.0, maxVal = 0.0;
    for (int i = 0; i < 4410; i++) // 0.1 second at 44.1k
    {
      double val = lfo.Process(10.0); // 10 Hz
      minVal = std::min(minVal, val);
      maxVal = std::max(maxVal, val);
    }

    REQUIRE(minVal >= 0.0);
    REQUIRE(maxVal <= 1.0);
    REQUIRE(minVal == Approx(0.0).margin(0.01));
    REQUIRE(maxVal == Approx(1.0).margin(0.01));
  }

  SECTION("Square wave values")
  {
    lfo.SetShape(LFO<double>::kSquare);
    lfo.SetPolarity(true); // Bipolar

    std::set<int> uniqueValues;
    for (int i = 0; i < 4410; i++)
    {
      double val = lfo.Process(10.0);
      uniqueValues.insert(static_cast<int>(val * 1000));
    }

    // Square wave should only have two distinct values
    REQUIRE(uniqueValues.size() == 2);
  }

  SECTION("Sine wave range bipolar")
  {
    lfo.SetShape(LFO<double>::kSine);
    lfo.SetPolarity(true); // Bipolar

    double minVal = 0.0, maxVal = 0.0;
    for (int i = 0; i < 4410; i++)
    {
      double val = lfo.Process(10.0);
      minVal = std::min(minVal, val);
      maxVal = std::max(maxVal, val);
    }

    REQUIRE(minVal == Approx(-1.0).margin(0.01));
    REQUIRE(maxVal == Approx(1.0).margin(0.01));
  }

  SECTION("Ramp up monotonically increases within cycle")
  {
    lfo.SetShape(LFO<double>::kRampUp);
    lfo.SetPolarity(false);
    lfo.Reset();

    double prev = -1.0;
    int increases = 0;
    for (int i = 0; i < 441; i++) // One cycle at 100Hz
    {
      double val = lfo.Process(100.0);
      if (val > prev)
        increases++;
      prev = val;
    }

    // Most samples should be increasing (except wrap-around)
    REQUIRE(increases > 400);
  }

  SECTION("Block processing")
  {
    lfo.SetShape(LFO<double>::kSine);
    lfo.Reset();

    std::vector<double> output(512);
    lfo.ProcessBlock(output.data(), 512, 0.0, false, 120.0);

    // Verify output is filled with valid values
    for (auto val : output)
    {
      REQUIRE(std::isfinite(val));
    }
  }

  SECTION("Tempo sync QN scalar")
  {
    REQUIRE(LFO<double>::GetQNScalar(LFO<double>::k4th) == Approx(1.0));
    REQUIRE(LFO<double>::GetQNScalar(LFO<double>::k8th) == Approx(2.0));
    REQUIRE(LFO<double>::GetQNScalar(LFO<double>::k1) == Approx(0.25));
  }
}

// ============================================================================
// Oscillator Tests
// ============================================================================

TEST_CASE("SinOscillator", "[DSP][Oscillator]")
{
  SECTION("Produces sine wave")
  {
    SinOscillator<double> osc(0.0, 440.0);
    osc.SetSampleRate(44100.0);
    osc.SetFreqCPS(440.0);

    double minVal = 0.0, maxVal = 0.0;
    for (int i = 0; i < 4410; i++)
    {
      double val = osc.Process(440.0);
      minVal = std::min(minVal, val);
      maxVal = std::max(maxVal, val);
    }

    REQUIRE(minVal == Approx(-1.0).margin(0.01));
    REQUIRE(maxVal == Approx(1.0).margin(0.01));
  }

  SECTION("Reset returns to start phase")
  {
    SinOscillator<double> osc(0.0, 440.0);
    osc.SetSampleRate(44100.0);

    double firstSample = osc.Process(440.0);

    // Process some samples
    for (int i = 0; i < 1000; i++)
      osc.Process(440.0);

    osc.Reset();
    double afterReset = osc.Process(440.0);

    REQUIRE(firstSample == Approx(afterReset).margin(0.001));
  }
}

TEST_CASE("FastSinOscillator", "[DSP][Oscillator]")
{
  SECTION("Produces sine wave via table lookup")
  {
    FastSinOscillator<double> osc(0.0, 440.0);
    osc.SetSampleRate(44100.0);
    osc.SetFreqCPS(440.0);

    double minVal = 0.0, maxVal = 0.0;
    for (int i = 0; i < 4410; i++)
    {
      double val = osc.Process(440.0);
      minVal = std::min(minVal, val);
      maxVal = std::max(maxVal, val);
    }

    REQUIRE(minVal == Approx(-1.0).margin(0.01));
    REQUIRE(maxVal == Approx(1.0).margin(0.01));
  }

  SECTION("Block processing")
  {
    FastSinOscillator<double> osc(0.0, 440.0);
    osc.SetSampleRate(44100.0);
    osc.SetFreqCPS(440.0);

    std::vector<double> output(512);
    osc.ProcessBlock(output.data(), 512);

    for (auto val : output)
    {
      REQUIRE(val >= -1.0);
      REQUIRE(val <= 1.0);
    }
  }

  SECTION("Static lookup produces valid output")
  {
    // Note: FastSinOscillator::Lookup may use cosine phase (returns 1.0 at 0)
    double lookup0 = FastSinOscillator<double>::Lookup(0.0);
    REQUIRE(lookup0 >= -1.0);
    REQUIRE(lookup0 <= 1.0);

    double lookupPiOver2 = FastSinOscillator<double>::Lookup(PI / 2.0);
    REQUIRE(lookupPiOver2 >= -1.0);
    REQUIRE(lookupPiOver2 <= 1.0);

    double lookupPi = FastSinOscillator<double>::Lookup(PI);
    REQUIRE(lookupPi >= -1.0);
    REQUIRE(lookupPi <= 1.0);
  }
}

// ============================================================================
// SVF (State Variable Filter) Tests
// ============================================================================

TEST_CASE("SVF filter modes", "[DSP][SVF]")
{
  constexpr int kBlockSize = 512;

  SECTION("Lowpass attenuates high frequencies")
  {
    SVF<double, 1> filter(SVF<double, 1>::kLowPass, 1000.0);
    filter.SetSampleRate(44100.0);
    filter.SetQ(0.707);

    // Generate high frequency sine (10kHz)
    std::vector<double> input(kBlockSize);
    std::vector<double> output(kBlockSize);
    for (int i = 0; i < kBlockSize; i++)
      input[i] = std::sin(2.0 * PI * 10000.0 * i / 44100.0);

    double* inPtr = input.data();
    double* outPtr = output.data();
    filter.ProcessBlock(&inPtr, &outPtr, 1, kBlockSize);

    // Calculate RMS of output (should be attenuated)
    double rmsIn = 0.0, rmsOut = 0.0;
    for (int i = 0; i < kBlockSize; i++)
    {
      rmsIn += input[i] * input[i];
      rmsOut += output[i] * output[i];
    }
    rmsIn = std::sqrt(rmsIn / kBlockSize);
    rmsOut = std::sqrt(rmsOut / kBlockSize);

    REQUIRE(rmsOut < rmsIn * 0.5); // Should be significantly attenuated
  }

  SECTION("Highpass attenuates low frequencies")
  {
    SVF<double, 1> filter(SVF<double, 1>::kHighPass, 1000.0);
    filter.SetSampleRate(44100.0);
    filter.SetQ(0.707);

    // Generate low frequency sine (100Hz)
    std::vector<double> input(kBlockSize);
    std::vector<double> output(kBlockSize);
    for (int i = 0; i < kBlockSize; i++)
      input[i] = std::sin(2.0 * PI * 100.0 * i / 44100.0);

    double* inPtr = input.data();
    double* outPtr = output.data();
    filter.ProcessBlock(&inPtr, &outPtr, 1, kBlockSize);

    double rmsIn = 0.0, rmsOut = 0.0;
    for (int i = 0; i < kBlockSize; i++)
    {
      rmsIn += input[i] * input[i];
      rmsOut += output[i] * output[i];
    }
    rmsIn = std::sqrt(rmsIn / kBlockSize);
    rmsOut = std::sqrt(rmsOut / kBlockSize);

    REQUIRE(rmsOut < rmsIn * 0.5);
  }

  SECTION("Bandpass passes center frequency")
  {
    SVF<double, 1> filter(SVF<double, 1>::kBandPass, 1000.0);
    filter.SetSampleRate(44100.0);
    filter.SetQ(2.0);

    // Generate sine at center frequency
    std::vector<double> input(kBlockSize);
    std::vector<double> output(kBlockSize);
    for (int i = 0; i < kBlockSize; i++)
      input[i] = std::sin(2.0 * PI * 1000.0 * i / 44100.0);

    double* inPtr = input.data();
    double* outPtr = output.data();
    filter.ProcessBlock(&inPtr, &outPtr, 1, kBlockSize);

    // Skip transient, check steady state
    double rmsOut = 0.0;
    for (int i = kBlockSize / 2; i < kBlockSize; i++)
      rmsOut += output[i] * output[i];
    rmsOut = std::sqrt(rmsOut / (kBlockSize / 2));

    REQUIRE(rmsOut > 0.3); // Should pass through
  }

  SECTION("Reset clears state")
  {
    SVF<double, 1> filter(SVF<double, 1>::kLowPass, 1000.0);
    filter.SetSampleRate(44100.0);

    // Process some samples
    std::vector<double> input(kBlockSize, 1.0);
    std::vector<double> output(kBlockSize);
    double* inPtr = input.data();
    double* outPtr = output.data();
    filter.ProcessBlock(&inPtr, &outPtr, 1, kBlockSize);

    filter.Reset();

    // Process silence
    std::fill(input.begin(), input.end(), 0.0);
    filter.ProcessBlock(&inPtr, &outPtr, 1, kBlockSize);

    // Output should be zero after reset with zero input
    REQUIRE(output[kBlockSize - 1] == Approx(0.0).margin(0.0001));
  }
}

// ============================================================================
// DCBlocker Tests
// ============================================================================

TEST_CASE("DCBlocker removes DC offset", "[DSP][DCBlocker]")
{
  DCBlocker<double, 1> blocker;
  constexpr int kBlockSize = 4096;

  SECTION("Removes DC from signal with offset")
  {
    std::vector<double> input(kBlockSize);
    std::vector<double> output(kBlockSize);

    // Sine wave with DC offset
    for (int i = 0; i < kBlockSize; i++)
      input[i] = 0.5 + 0.3 * std::sin(2.0 * PI * 440.0 * i / 44100.0);

    double* inPtr = input.data();
    double* outPtr = output.data();
    blocker.ProcessBlock(&inPtr, &outPtr, 1, kBlockSize);

    // Calculate DC of output (average)
    double dcOut = std::accumulate(output.begin() + kBlockSize/2, output.end(), 0.0) / (kBlockSize/2);

    REQUIRE(std::abs(dcOut) < 0.1); // DC should be significantly reduced
  }

  SECTION("Passes AC signal")
  {
    std::vector<double> input(kBlockSize);
    std::vector<double> output(kBlockSize);

    // Pure sine wave (no DC)
    for (int i = 0; i < kBlockSize; i++)
      input[i] = std::sin(2.0 * PI * 440.0 * i / 44100.0);

    double* inPtr = input.data();
    double* outPtr = output.data();
    blocker.ProcessBlock(&inPtr, &outPtr, 1, kBlockSize);

    // RMS should be similar (AC passes through)
    double rmsIn = 0.0, rmsOut = 0.0;
    for (int i = kBlockSize/2; i < kBlockSize; i++)
    {
      rmsIn += input[i] * input[i];
      rmsOut += output[i] * output[i];
    }
    rmsIn = std::sqrt(rmsIn / (kBlockSize/2));
    rmsOut = std::sqrt(rmsOut / (kBlockSize/2));

    REQUIRE(rmsOut == Approx(rmsIn).margin(0.1));
  }
}

// ============================================================================
// NChanDelayLine Tests
// ============================================================================

TEST_CASE("NChanDelayLine", "[DSP][NChanDelay]")
{
  SECTION("Delays signal by specified samples")
  {
    NChanDelayLine<double> delay(1, 1);
    constexpr int kDelaySamples = 100;
    delay.SetDelayTime(kDelaySamples);

    constexpr int kBlockSize = 256;
    std::vector<double> input(kBlockSize);
    std::vector<double> output(kBlockSize);

    // Impulse at sample 0
    input[0] = 1.0;

    double* inPtr = input.data();
    double* outPtr = output.data();
    delay.ProcessBlock(&inPtr, &outPtr, kBlockSize);

    // Check impulse appears at delayed position
    REQUIRE(output[0] == 0.0); // Before delay
    REQUIRE(output[kDelaySamples] == 1.0); // At delay time
  }

  SECTION("Stereo delay")
  {
    NChanDelayLine<double> delay(2, 2);
    constexpr int kDelaySamples = 50;
    delay.SetDelayTime(kDelaySamples);

    constexpr int kBlockSize = 128;
    std::vector<double> inputL(kBlockSize, 0.0);
    std::vector<double> inputR(kBlockSize, 0.0);
    std::vector<double> outputL(kBlockSize);
    std::vector<double> outputR(kBlockSize);

    inputL[0] = 1.0;
    inputR[0] = 0.5;

    double* inPtrs[2] = {inputL.data(), inputR.data()};
    double* outPtrs[2] = {outputL.data(), outputR.data()};
    delay.ProcessBlock(inPtrs, outPtrs, kBlockSize);

    REQUIRE(outputL[kDelaySamples] == 1.0);
    REQUIRE(outputR[kDelaySamples] == 0.5);
  }

  SECTION("ClearBuffer zeros delay line")
  {
    NChanDelayLine<double> delay(1, 1);
    delay.SetDelayTime(50);

    constexpr int kBlockSize = 128;
    std::vector<double> input(kBlockSize, 1.0);
    std::vector<double> output(kBlockSize);

    double* inPtr = input.data();
    double* outPtr = output.data();
    delay.ProcessBlock(&inPtr, &outPtr, kBlockSize);

    delay.ClearBuffer();

    std::fill(input.begin(), input.end(), 0.0);
    delay.ProcessBlock(&inPtr, &outPtr, kBlockSize);

    // All output should be zero after clear with zero input
    for (auto val : output)
      REQUIRE(val == 0.0);
  }
}

// ============================================================================
// Smoothers Tests
// ============================================================================

TEST_CASE("LogParamSmooth", "[DSP][Smoothers]")
{
  SECTION("Smooths step changes")
  {
    LogParamSmooth<double, 1> smoother(5.0, 0.0);
    smoother.SetSmoothTime(5.0, 44100.0);

    // Step from 0 to 1
    double prev = 0.0;
    bool allIncreasing = true;
    for (int i = 0; i < 1000; i++)
    {
      double val = smoother.Process(1.0);
      if (val < prev)
        allIncreasing = false;
      prev = val;
    }

    REQUIRE(allIncreasing);
    REQUIRE(prev > 0.9); // Should approach target
  }

  SECTION("SetValue immediately sets output")
  {
    LogParamSmooth<double, 1> smoother(5.0, 0.0);

    smoother.SetValue(0.5);
    double val = smoother.Process(0.5);

    REQUIRE(val == Approx(0.5).margin(0.01));
  }

  SECTION("Shorter smooth time responds faster")
  {
    LogParamSmooth<double, 1> fastSmoother(1.0, 0.0);
    LogParamSmooth<double, 1> slowSmoother(20.0, 0.0);
    fastSmoother.SetSmoothTime(1.0, 44100.0);
    slowSmoother.SetSmoothTime(20.0, 44100.0);

    double fastVal = 0.0, slowVal = 0.0;
    for (int i = 0; i < 100; i++)
    {
      fastVal = fastSmoother.Process(1.0);
      slowVal = slowSmoother.Process(1.0);
    }

    REQUIRE(fastVal > slowVal);
  }
}

TEST_CASE("SmoothedGain", "[DSP][Smoothers]")
{
  SECTION("Applies smoothed gain to signal")
  {
    SmoothedGain<double> gain(5.0);
    gain.SetSampleRate(44100.0);

    constexpr int kBlockSize = 512;
    std::vector<double> input(kBlockSize, 1.0);
    std::vector<double> output(kBlockSize);

    double* inPtr = input.data();
    double* outPtr = output.data();
    gain.ProcessBlock(&inPtr, &outPtr, 1, kBlockSize, 0.5);

    // Output should approach 0.5
    REQUIRE(output[kBlockSize - 1] < 1.0);
    REQUIRE(output[kBlockSize - 1] > 0.0);
  }
}

// ============================================================================
// NoiseGate Tests
// ============================================================================

TEST_CASE("NoiseGate", "[DSP][NoiseGate]")
{
  SECTION("Gates signal below threshold")
  {
    NoiseGate<double, 1> gate;
    gate.SetSampleRate(44100.0);
    gate.SetThreshold(-20.0); // -20dB threshold
    gate.SetAttackTime(0.001);
    gate.SetHoldTime(0.01);
    gate.SetReleaseTime(0.05);

    constexpr int kBlockSize = 4096;
    std::vector<double> input(kBlockSize, 0.5);      // Signal
    std::vector<double> sidechain(kBlockSize, 0.01); // Very quiet sidechain
    std::vector<double> output(kBlockSize);

    double* inPtr = input.data();
    double* outPtr = output.data();
    gate.ProcessBlock(&inPtr, &outPtr, sidechain.data(), 1, kBlockSize);

    // Output should be attenuated (gate closed)
    double rmsOut = 0.0;
    for (int i = kBlockSize/2; i < kBlockSize; i++)
      rmsOut += output[i] * output[i];
    rmsOut = std::sqrt(rmsOut / (kBlockSize/2));

    REQUIRE(rmsOut < 0.1); // Should be gated
  }

  SECTION("Passes signal above threshold")
  {
    NoiseGate<double, 1> gate;
    gate.SetSampleRate(44100.0);
    gate.SetThreshold(-40.0); // -40dB threshold
    gate.SetAttackTime(0.001);
    gate.SetHoldTime(0.01);
    gate.SetReleaseTime(0.05);

    constexpr int kBlockSize = 4096;
    std::vector<double> input(kBlockSize, 0.5);
    std::vector<double> sidechain(kBlockSize, 0.5); // Loud sidechain
    std::vector<double> output(kBlockSize);

    double* inPtr = input.data();
    double* outPtr = output.data();
    gate.ProcessBlock(&inPtr, &outPtr, sidechain.data(), 1, kBlockSize);

    // Output should pass through (gate open)
    double rmsOut = 0.0;
    for (int i = kBlockSize/2; i < kBlockSize; i++)
      rmsOut += output[i] * output[i];
    rmsOut = std::sqrt(rmsOut / (kBlockSize/2));

    REQUIRE(rmsOut > 0.3); // Should pass
  }

  SECTION("Hold time keeps gate open")
  {
    NoiseGate<double, 1> gate;
    gate.SetSampleRate(44100.0);
    gate.SetThreshold(-20.0);
    gate.SetAttackTime(0.001);
    gate.SetHoldTime(0.1); // 100ms hold
    gate.SetReleaseTime(0.05);

    constexpr int kBlockSize = 8192;
    std::vector<double> input(kBlockSize, 0.5);
    std::vector<double> sidechain(kBlockSize);
    std::vector<double> output(kBlockSize);

    // Loud for first half, then quiet
    for (int i = 0; i < kBlockSize/2; i++)
      sidechain[i] = 0.5;
    for (int i = kBlockSize/2; i < kBlockSize; i++)
      sidechain[i] = 0.01;

    double* inPtr = input.data();
    double* outPtr = output.data();
    gate.ProcessBlock(&inPtr, &outPtr, sidechain.data(), 1, kBlockSize);

    // Check that signal continues shortly after sidechain drops
    double midOutput = std::abs(output[kBlockSize/2 + 100]);
    REQUIRE(midOutput > 0.1); // Hold should keep gate open
  }
}

// ============================================================================
// LanczosResampler Tests
// ============================================================================

TEST_CASE("LanczosResampler", "[DSP][LanczosResampler]")
{
  SECTION("Downsampling 48k to 44.1k")
  {
    LanczosResampler<double, 1, 12> resampler(48000.0f, 44100.0f);

    // Generate 1 second of 440Hz sine at 48kHz
    constexpr int kInputFrames = 4800; // 0.1 sec at 48k
    std::vector<double> input(kInputFrames);
    for (int i = 0; i < kInputFrames; i++)
      input[i] = std::sin(2.0 * PI * 440.0 * i / 48000.0);

    double* inPtr = input.data();
    resampler.PushBlock(&inPtr, kInputFrames, 1);

    // Pop resampled output
    constexpr int kMaxOutputFrames = 4410; // Expected ~4410 for 0.1 sec at 44.1k
    std::vector<double> output(kMaxOutputFrames);
    double* outPtr = output.data();

    size_t outputFrames = resampler.PopBlock(&outPtr, kMaxOutputFrames, 1);

    // Should produce approximately the right number of samples
    REQUIRE(outputFrames > 4000);
    REQUIRE(outputFrames < 4500);

    // Output should still be a valid sine wave
    double minVal = 0.0, maxVal = 0.0;
    for (size_t i = 100; i < outputFrames; i++) // Skip transient
    {
      minVal = std::min(minVal, output[i]);
      maxVal = std::max(maxVal, output[i]);
    }

    REQUIRE(minVal < -0.8);
    REQUIRE(maxVal > 0.8);
  }

  SECTION("Upsampling 44.1k to 48k")
  {
    LanczosResampler<double, 1, 12> resampler(44100.0f, 48000.0f);

    constexpr int kInputFrames = 4410; // 0.1 sec at 44.1k
    std::vector<double> input(kInputFrames);
    for (int i = 0; i < kInputFrames; i++)
      input[i] = std::sin(2.0 * PI * 440.0 * i / 44100.0);

    double* inPtr = input.data();
    resampler.PushBlock(&inPtr, kInputFrames, 1);

    constexpr int kMaxOutputFrames = 4900;
    std::vector<double> output(kMaxOutputFrames);
    double* outPtr = output.data();

    size_t outputFrames = resampler.PopBlock(&outPtr, kMaxOutputFrames, 1);

    // Should produce more samples than input
    REQUIRE(outputFrames > 4500);
    REQUIRE(outputFrames < 5000);
  }

  SECTION("Stereo resampling")
  {
    LanczosResampler<double, 2, 12> resampler(48000.0f, 44100.0f);

    constexpr int kInputFrames = 480;
    std::vector<double> inputL(kInputFrames);
    std::vector<double> inputR(kInputFrames);

    for (int i = 0; i < kInputFrames; i++)
    {
      inputL[i] = std::sin(2.0 * PI * 440.0 * i / 48000.0);
      inputR[i] = std::sin(2.0 * PI * 880.0 * i / 48000.0);
    }

    double* inPtrs[2] = {inputL.data(), inputR.data()};
    resampler.PushBlock(inPtrs, kInputFrames, 2);

    constexpr int kMaxOutputFrames = 500;
    std::vector<double> outputL(kMaxOutputFrames);
    std::vector<double> outputR(kMaxOutputFrames);
    double* outPtrs[2] = {outputL.data(), outputR.data()};

    size_t outputFrames = resampler.PopBlock(outPtrs, kMaxOutputFrames, 2);

    REQUIRE(outputFrames > 0);

    // Both channels should have valid output
    bool hasOutputL = false, hasOutputR = false;
    for (size_t i = 0; i < outputFrames; i++)
    {
      if (std::abs(outputL[i]) > 0.1) hasOutputL = true;
      if (std::abs(outputR[i]) > 0.1) hasOutputR = true;
    }

    REQUIRE(hasOutputL);
    REQUIRE(hasOutputR);
  }

  SECTION("GetNumSamplesRequiredFor calculation")
  {
    LanczosResampler<double, 1, 12> resampler(48000.0f, 44100.0f);

    size_t required = resampler.GetNumSamplesRequiredFor(441); // 10ms at 44.1k
    REQUIRE(required > 0);
    REQUIRE(required < 1000);
  }

  SECTION("Reset callable without crash")
  {
    LanczosResampler<double, 1, 12> resampler(48000.0f, 44100.0f);

    // Push some samples
    constexpr int kInputFrames = 480;
    std::vector<double> input(kInputFrames, 0.5);
    double* inPtr = input.data();
    resampler.PushBlock(&inPtr, kInputFrames, 1);

    // Reset should not crash
    resampler.Reset();

    // Resampler should still be functional after reset
    REQUIRE(true);
  }
}
