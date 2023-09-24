
/*
 LanczosResampler derived from
 https://github.com/surge-synthesizer/sst-basic-blocks/blob/main/include/sst/basic-blocks/dsp/LanczosResampler.h
 
 * sst-basic-blocks - an open source library of core audio utilities
 * built by Surge Synth Team.
 *
 * Provides a collection of tools useful on the audio thread for blocks,
 * modulation, etc... or useful for adapting code to multiple environments.
 *
 * Copyright 2023, various authors, as described in the GitHub
 * transaction log. Parts of this code are derived from similar
 * functions original in Surge or ShortCircuit.
 *
 * sst-basic-blocks is released under the GNU General Public Licence v3
 * or later (GPL-3.0-or-later). The license is found in the "LICENSE"
 * file in the root of this repository, or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * All source in sst-basic-blocks available at
 * https://github.com/surge-synthesizer/sst-basic-blocks
 */

/*
 * A special note on licensing: This file (and only this file)
 * has Paul Walker (baconpaul) as the sole author to date.
 *
 * In order to make this handy small function based on public
 * information available to a set of open source projects
 * adapting hardware to software, but which are licensed under
 * MIT or BSD or similar licenses, this file and only this file
 * can be used in an MIT/BSD context as well as a GPL3 context, by
 * copying it and modifying it as you see fit.
 *
 * If you do that, you will need to replace the `sum_ps_to_float`
 * call below with either an hadd if you are SSE3 or higher or
 * an appropriate reduction operator from your toolkit.
 *
 * But basically: Need to resample 48k to variable rate with
 * a small window and want to use this? Go for it!
 *
 * For avoidance of doubt, this license exception only
 * applies to this file.
 */

#pragma once

#include <algorithm>
#include <utility>
#include <cmath>
#include <cstring>

#if defined IPLUG_SIMDE
  #if defined(__arm64__)
    #define SIMDE_ENABLE_NATIVE_ALIASES
    #include "simde/x86/sse2.h"
  #else
    #include <emmintrin.h>
  #endif
#endif

namespace iplug
{
/*
 * See https://en.wikipedia.org/wiki/Lanczos_resampling
 */

template<typename T = double, int NCHANS=2>
class LanczosResampler
{
private:
  static constexpr size_t A = 4;
  static constexpr size_t kBufferSize = 4096;
  static constexpr size_t kFilterWidth = A * 2;
  static constexpr size_t kTableObs = 8192;
  static constexpr double kDeltaX = 1.0 / (kTableObs);

public:
  LanczosResampler(float inputRate, float outputRate)
  : mInputSampleRate(inputRate)
  , mOutputSamplerate(outputRate)
  , mPhaseOutIncr(mInputSampleRate / mOutputSamplerate)
  {
    memset(mInputBuffer, 0, NCHANS * kBufferSize * sizeof(T));

    auto kernel = [](double x) {
      if (std::fabs(x) < 1e-7)
        return T(1.0);
      return T(A * std::sin(M_PI * x) * std::sin(M_PI * x / A) / (M_PI * M_PI * x * x));
    };
  
    if (!sTablesInitialized)
    {
      for (auto t = 0; t < kTableObs + 1; ++t)
      {
        const double x0 = kDeltaX * t;
        
        for (auto i=0; i<kFilterWidth; ++i)
        {
          const double x = x0 + i - A;
          sTable[t][i] = kernel(x);
        }
      }
      
      for (auto t=0; t<kTableObs; ++t)
      {
        for (auto i=0; i<kFilterWidth; ++i)
        {
          sDeltaTable[t][i] = sTable[t + 1][i] - sTable[t][i];
        }
      }
      
      for (auto i=0; i<kFilterWidth; ++i)
      {
        // Wrap at the end - deriv is the same
        sDeltaTable[kTableObs][i] = sDeltaTable[0][i];
      }
      sTablesInitialized = true;
    }
  }

  inline size_t inputsRequiredToGenerateOutputs(size_t desiredOutputs) const
  {
    /*
     * So (mPhaseIn + mPhaseInIncr * res - mPhaseOut - mPhaseOutIncr * desiredOutputs) * sri > A + 1
     *
     * Use the fact that mPhaseInIncr = mInputSampleRate and find
     * res > (A+1) - (mPhaseIn - mPhaseOut + mPhaseOutIncr * desiredOutputs) * sri
     */
    auto res = A + 1.0 - (mPhaseIn - mPhaseOut - mPhaseOutIncr * desiredOutputs);

    return static_cast<size_t>(std::max(res + 1.0, 0.0)); // Check this calculation
  }

  inline void pushBlock(T** inputs, size_t nFrames)
  {
    for (auto s=0; s<nFrames; s++)
    {
      for (auto c=0; c<NCHANS; c++)
      {
        mInputBuffer[c][mWritePos] = inputs[c][s];
        mInputBuffer[c][mWritePos + kBufferSize] = inputs[c][s]; // this way we can always wrap
      }
      
      mWritePos = (mWritePos + 1) & (kBufferSize - 1);
      mPhaseIn += mPhaseInIncr;
    }
  }

  size_t popBlock(T** outputs, size_t max)
  {
    int populated = 0;
    while (populated < max && (mPhaseIn - mPhaseOut) > A + 1)
    {
      read((mPhaseIn - mPhaseOut), outputs, populated);
      mPhaseOut += mPhaseOutIncr;
      populated++;
    }
    return populated;
  }

  inline void renormalizePhases()
  {
    mPhaseIn -= mPhaseOut;
    mPhaseOut = 0;
  }

private:
  inline void read(double xBack, T** outputs, int s) const
  {
    double p0 = mWritePos - xBack;
    int idx0 = std::floor(p0);
    double off0 = 1.0 - (p0 - idx0);

    idx0 = (idx0 + kBufferSize) & (kBufferSize - 1);
    idx0 += (idx0 <= (int)A) * kBufferSize;

    double off0byto = off0 * kTableObs;
    int tidx = (int)(off0byto);
    double fidx = (off0byto - tidx);
    
    T temp[NCHANS] = {0.0};

#if defined IPLUG_SIMDE && SAMPLE_TYPE_FLOAT
    auto sum_ps_to_float = [](__m128 x) {

      auto sum_ps_to_ss = [](__m128 x) {
        // FIXME: With SSE 3 this can be a dual hadd
        __m128 a = _mm_add_ps(x, _mm_movehl_ps(x, x));
        return _mm_add_ss(a, _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 0, 0, 1)));
      };
      
      __m128 r = sum_ps_to_ss(x);
      float f;
      _mm_store_ss(&f, r);
      return f;
    };
    
    auto fl = _mm_set1_ps((float)fidx);
    auto f0 = _mm_load_ps(&sTable[tidx][0]);
    auto df0 = _mm_load_ps(&sDeltaTable[tidx][0]);

    f0 = _mm_add_ps(f0, _mm_mul_ps(df0, fl));

    auto f1 = _mm_load_ps(&sTable[tidx][4]);
    auto df1 = _mm_load_ps(&sDeltaTable[tidx][4]);
    f1 = _mm_add_ps(f1, _mm_mul_ps(df1, fl));

    for (auto c=0; c<NCHANS;c++)
    {
      auto d0 = _mm_loadu_ps(&mInputBuffer[c][idx0 - A]);
      auto d1 = _mm_loadu_ps(&mInputBuffer[c][idx0]);
      auto rv = _mm_add_ps(_mm_mul_ps(f0, d0), _mm_mul_ps(f1, d1));
      temp[c] = sum_ps_to_float(rv);
    }
#else
    
    for (auto i=0; i<4; i++)
    {
      const auto fl = fidx;
      auto f0 = sTable[tidx][i];
      const auto df0 = sDeltaTable[tidx][i];
      f0 += df0 * fl;
      
      auto f1 = sTable[tidx][4+i];
      const auto df1 = sDeltaTable[tidx][4+i];
      f1 += df1 * fl;
      
      for (auto c=0; c<NCHANS;c++)
      {
        const auto d0 = mInputBuffer[c][idx0 - A + i];
        const auto d1 = mInputBuffer[c][idx0 + i];
        const auto rv = (f0 * d0) + (f1 * d1);
        temp[c] += rv;
      }
    }
#endif
    
    for (auto c=0; c<NCHANS;c++)
    {
      outputs[c][s] = temp[c];
    }
  }

  
  static T sTable alignas(16)[kTableObs + 1][kFilterWidth];
  static T sDeltaTable alignas(16)[kTableObs + 1][kFilterWidth];
  static bool sTablesInitialized;

  T mInputBuffer[NCHANS][kBufferSize * 2];
  int mWritePos = 0;
  const float mInputSampleRate;
  const float mOutputSamplerate;
  double mPhaseIn = 0.0;
  double mPhaseOut = 0.0;
  double mPhaseInIncr = 1.0;
  double mPhaseOutIncr = 0.0;
};

template<typename T, int NCHANS>
T LanczosResampler<T, NCHANS>::sTable alignas(16) [LanczosResampler<T, NCHANS>::kTableObs + 1][LanczosResampler::kFilterWidth];

template<typename T, int NCHANS>
T LanczosResampler<T, NCHANS>::sDeltaTable alignas(16) [LanczosResampler<T, NCHANS>::kTableObs + 1][LanczosResampler::kFilterWidth];

template<typename T, int NCHANS>
bool LanczosResampler<T, NCHANS>::sTablesInitialized{false};

} // namespace iplug

