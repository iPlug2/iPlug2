/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Shared helpers for the IPlug Extras unit tests (signal generation and measurement)

#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "IPlugConstants.h"

namespace extrastest
{
/** Tiny deterministic linear congruential generator, so that tests never depend on rand() */
class LCG
{
public:
  explicit LCG(uint32_t seed = 12345u) : mState(seed) {}

  uint32_t NextInt()
  {
    mState = mState * 1664525u + 1013904223u;
    return mState >> 8; // the low bits of an LCG are not very random
  }

  /** @return A value in [0, 1) */
  double NextUnipolar() { return static_cast<double>(NextInt()) / 16777216.0; }

  /** @return A value in [-1, 1) */
  double NextBipolar() { return NextUnipolar() * 2.0 - 1.0; }

private:
  uint32_t mState;
};

/** Owns nChans x nFrames samples and exposes them as the T** that the DSP classes expect */
template <typename T>
class Buffers
{
public:
  Buffers(int nChans, int nFrames, T fill = T(0))
  : mData(static_cast<size_t>(nChans), std::vector<T>(static_cast<size_t>(nFrames), fill))
  , mPtrs(static_cast<size_t>(nChans), nullptr)
  {
  }

  Buffers(const Buffers&) = delete;
  Buffers& operator=(const Buffers&) = delete;

  /** The channel pointers are refreshed on every call, since assigning to a channel vector may reallocate it */
  T** Get()
  {
    for (size_t c = 0; c < mData.size(); c++)
      mPtrs[c] = mData[c].data();

    return mPtrs.data();
  }

  std::vector<T>& operator[](int chan) { return mData[static_cast<size_t>(chan)]; }

private:
  std::vector<std::vector<T>> mData;
  std::vector<T*> mPtrs;
};

template <typename T>
std::vector<T> MakeSine(double freqHz, double sampleRate, int nFrames, double amp = 1.0, double offset = 0.0)
{
  std::vector<T> v(static_cast<size_t>(nFrames));

  for (int s = 0; s < nFrames; s++)
    v[static_cast<size_t>(s)] = static_cast<T>(offset + amp * std::sin(2.0 * iplug::PI * freqHz * static_cast<double>(s) / sampleRate));

  return v;
}

template <typename T>
std::vector<T> MakeNoise(int nFrames, uint32_t seed, double amp = 1.0)
{
  LCG lcg(seed);
  std::vector<T> v(static_cast<size_t>(nFrames));

  for (auto& x : v)
    x = static_cast<T>(amp * lcg.NextBipolar());

  return v;
}

template <typename T>
double RMS(const T* pData, int nFrames)
{
  double sum = 0.0;

  for (int s = 0; s < nFrames; s++)
    sum += static_cast<double>(pData[s]) * static_cast<double>(pData[s]);

  return nFrames > 0 ? std::sqrt(sum / static_cast<double>(nFrames)) : 0.0;
}

template <typename T>
double Mean(const T* pData, int nFrames)
{
  double sum = 0.0;

  for (int s = 0; s < nFrames; s++)
    sum += static_cast<double>(pData[s]);

  return nFrames > 0 ? sum / static_cast<double>(nFrames) : 0.0;
}

template <typename T>
double PeakAbs(const T* pData, int nFrames)
{
  double peak = 0.0;

  for (int s = 0; s < nFrames; s++)
  {
    const double a = std::fabs(static_cast<double>(pData[s]));
    if (a > peak)
      peak = a;
  }

  return peak;
}

template <typename T>
bool AllFinite(const T* pData, int nFrames)
{
  for (int s = 0; s < nFrames; s++)
  {
    if (!std::isfinite(static_cast<double>(pData[s])))
      return false;
  }

  return true;
}

/** @return The largest absolute difference between two buffers (infinity if either holds a NaN) */
template <typename T>
double MaxAbsDiff(const T* pA, const T* pB, int nFrames)
{
  double maxDiff = 0.0;

  for (int s = 0; s < nFrames; s++)
  {
    const double d = std::fabs(static_cast<double>(pA[s]) - static_cast<double>(pB[s]));

    if (std::isnan(d))
      return HUGE_VAL;

    if (d > maxDiff)
      maxDiff = d;
  }

  return maxDiff;
}

/** @return The number of negative-to-positive transitions, i.e. the number of whole cycles of a periodic signal */
template <typename T>
int CountRisingZeroCrossings(const T* pData, int nFrames)
{
  int count = 0;

  for (int s = 1; s < nFrames; s++)
  {
    if (pData[s - 1] < T(0) && pData[s] >= T(0))
      count++;
  }

  return count;
}

struct SineFit
{
  double amplitude;   // amplitude of the best fitting sinusoid
  double maxResidual; // largest deviation of any sample from that sinusoid
};

/** Least squares fit of a sinusoid of known frequency (but unknown amplitude and phase) to a signal.
 *  A small maxResidual means that the signal is a clean sinusoid of exactly that frequency.
 * @param radiansPerSample The angular frequency, 2 * pi * freqHz / sampleRate
 * @param offset A DC offset to remove from the signal before fitting */
template <typename T>
SineFit FitSinusoid(const T* pData, int nFrames, double radiansPerSample, double offset = 0.0)
{
  double scc = 0.0, sss = 0.0, scs = 0.0, syc = 0.0, sys = 0.0;

  for (int s = 0; s < nFrames; s++)
  {
    const double c = std::cos(radiansPerSample * static_cast<double>(s));
    const double sn = std::sin(radiansPerSample * static_cast<double>(s));
    const double y = static_cast<double>(pData[s]) - offset;
    scc += c * c;
    sss += sn * sn;
    scs += c * sn;
    syc += y * c;
    sys += y * sn;
  }

  const double det = scc * sss - scs * scs;
  const double a = (syc * sss - sys * scs) / det;
  const double b = (sys * scc - syc * scs) / det;

  SineFit fit {std::sqrt(a * a + b * b), 0.0};

  for (int s = 0; s < nFrames; s++)
  {
    const double model = a * std::cos(radiansPerSample * static_cast<double>(s)) + b * std::sin(radiansPerSample * static_cast<double>(s));
    const double residual = std::fabs(static_cast<double>(pData[s]) - offset - model);

    if (std::isnan(residual))
    {
      fit.maxResidual = HUGE_VAL;
      break;
    }

    if (residual > fit.maxResidual)
      fit.maxResidual = residual;
  }

  return fit;
}

inline double AmpToDB(double amp)
{
  return 20.0 * std::log10(amp > 1e-12 ? amp : 1e-12);
}
} // namespace extrastest
