/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

/*
 This code is derived from
 https://github.com/surge-synthesizer/sst-basic-blocks/blob/main/include/sst/basic-blocks/dsp/LanczosResampler.h
 
 The following license info is copied from the above file:
 
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

#include "IPlugConstants.h"

namespace iplug
{
/* LanczosResampler
 *
 * A class that implements Lanczos resampling, optionally using SIMD instructions.
 * Define IPLUG_SIMDE at project level in order to use SIMD and if on non-x86_64
 * include the SIMDE library in your search paths in order to translate intel
 * intrinsics to e.g. arm64
 *
 * See https://en.wikipedia.org/wiki/Lanczos_resampling
 *
 * @tparam T the sampletype
 * @tparam NCHANS the number of channels
 * @tparam A The Lanczos filter size. A higher value makes the filter closer to an 
   ideal stop-band that rejects high-frequency content (anti-aliasing), 
   but at the expense of higher latency
 */
template<typename T = double, int NCHANS=2, size_t A=12>
class LanczosResampler
{
private:
#if IPLUG_SIMDE
  static_assert(std::is_same<T, float>::value, "LanczosResampler requires T to be float when using SIMD instructions");
#endif

  // The buffer size. This needs to be at least as large as the largest block of samples
  // that the input side will see.
  static constexpr size_t kBufferSize = 4096;
  // The filter width. 2x because the filter goes from -A to A
  static constexpr size_t kFilterWidth = A * 2;
  // The discretization resolution for the filter table.
  static constexpr size_t kTablePoints = 8192;
  static constexpr double kDeltaX = 1.0 / (kTablePoints);

public:
  /** Constructor
    * @param inputRate The input sample rate
    * @param outputRate The output sample rate
    */
  LanczosResampler(float inputRate, float outputRate)
  : mInputSampleRate(inputRate)
  , mOutputSamplerate(outputRate)
  , mPhaseOutIncr(mInputSampleRate / mOutputSamplerate)
  {
    ClearBuffer();
    
    auto kernel = [](double x) {
      if (std::fabs(x) < 1e-7)
        return T(1.0);
      
      const auto pi = iplug::PI;
      return T(A * std::sin(pi * x) * std::sin(pi * x / A) / (pi * pi * x * x));
    };
    
    if (!sTablesInitialized)
    {
      for (auto t=0; t<kTablePoints+1; ++t)
      {
        const double x0 = kDeltaX * t;
        
        for (auto i=0; i<kFilterWidth; ++i)
        {
          const double x = x0 + i - A;
          sTable[t][i] = kernel(x);
        }
      }
      
      for (auto t=0; t<kTablePoints; ++t)
      {
        for (auto i=0; i<kFilterWidth; ++i)
        {
          sDeltaTable[t][i] = sTable[t + 1][i] - sTable[t][i];
        }
      }
      
      for (auto i=0; i<kFilterWidth; ++i)
      {
        // Wrap at the end - delta is the same
        sDeltaTable[kTablePoints][i] = sDeltaTable[0][i];
      }
      sTablesInitialized = true;
    }
  }
  
  inline size_t GetNumSamplesRequiredFor(size_t nOutputSamples) const
  {
    /*
     * So (mPhaseIn + mPhaseInIncr * res - mPhaseOut - mPhaseOutIncr * nOutputSamples) * sri > A + 1
     *
     * Use the fact that mPhaseInIncr = mInputSampleRate and find
     * res > (A+1) - (mPhaseIn - mPhaseOut + mPhaseOutIncr * desiredOutputs) * sri
     */
    auto res = A + 1.0 - (mPhaseIn - mPhaseOut - mPhaseOutIncr * nOutputSamples);
    
    return static_cast<size_t>(std::max(res + 1.0, 0.0));
  }
  
  inline void PushBlock(T** inputs, size_t nFrames, int nChans)
  {
    for (auto s=0; s<nFrames; s++)
    {
      for (auto c=0; c<nChans; c++)
      {
        mInputBuffer[c][mWritePos] = inputs[c][s];
        mInputBuffer[c][mWritePos + kBufferSize] = inputs[c][s]; // this way we can always wrap
      }
      
      mWritePos = (mWritePos + 1) & (kBufferSize - 1);
      mPhaseIn += mPhaseInIncr;
    }
  }
  
  size_t PopBlock(T** outputs, size_t max, int nChans)
  {
    int populated = 0;
    while (populated < max && (mPhaseIn - mPhaseOut) > A + 1)
    {
      ReadSamples((mPhaseIn - mPhaseOut), outputs, populated, nChans);
      mPhaseOut += mPhaseOutIncr;
      populated++;
    }
    return populated;
  }
  
  inline void RenormalizePhases()
  {
    mPhaseIn -= mPhaseOut;
    mPhaseOut = 0;
  }

  void Reset()
  {
    ClearBuffer();
  }
  
  void ClearBuffer()
  {
    memset(mInputBuffer, 0, NCHANS * kBufferSize * 2 * sizeof(T));
  }
  
private:
#ifdef IPLUG_SIMDE
  inline void ReadSamples(double xBack, T** outputs, int s, int nChans) const
  {
    float bufferReadPosition = static_cast<float>(mWritePos - xBack);
    int bufferReadIndex = static_cast<int>(std::floor(bufferReadPosition));
    float bufferFracPosition = 1.0f - (bufferReadPosition - static_cast<float>(bufferReadIndex));
    
    bufferReadIndex = (bufferReadIndex + kBufferSize) & (kBufferSize - 1);
    bufferReadIndex += (bufferReadIndex <= static_cast<int>(A)) * kBufferSize;
    
    float tablePosition = bufferFracPosition * kTablePoints;
    int tableIndex = static_cast<int>(tablePosition);
    float tableFracPosition = (tablePosition - tableIndex);
    
    __m128 sum[NCHANS];
    for (auto & v : sum) {
      v = _mm_setzero_ps(); // Initialize sum vectors to zero
    }
    
    for (int i=0; i<A; i+=4) // Process four samples at a time
    {
      // Load filter coefficients and input samples into SSE registers
      __m128 f0 = _mm_load_ps(&sTable[tableIndex][i]);
      __m128 df0 = _mm_load_ps(&sDeltaTable[tableIndex][i]);
      __m128 f1 = _mm_load_ps(&sTable[tableIndex][A + i]);
      __m128 df1 = _mm_load_ps(&sDeltaTable[tableIndex][A + i]);
      
      // Interpolate filter coefficients
      __m128 tfp = _mm_set1_ps(tableFracPosition);
      f0 = _mm_add_ps(f0, _mm_mul_ps(df0, tfp));
      f1 = _mm_add_ps(f1, _mm_mul_ps(df1, tfp));
      
      for (int c=0; c<nChans; c++)
      {
        // Load input data
        __m128 d0 = _mm_set_ps(mInputBuffer[c][bufferReadIndex - A + i + 3],
                               mInputBuffer[c][bufferReadIndex - A + i + 2],
                               mInputBuffer[c][bufferReadIndex - A + i + 1],
                               mInputBuffer[c][bufferReadIndex - A + i]);
        __m128 d1 = _mm_set_ps(mInputBuffer[c][bufferReadIndex + i + 3],
                               mInputBuffer[c][bufferReadIndex + i + 2],
                               mInputBuffer[c][bufferReadIndex + i + 1],
                               mInputBuffer[c][bufferReadIndex + i]);
        
        // Perform multiplication and accumulate
        __m128 result0 = _mm_mul_ps(f0, d0);
        __m128 result1 = _mm_mul_ps(f1, d1);
        sum[c] = _mm_add_ps(sum[c], _mm_add_ps(result0, result1));
      }
    }
    
    // Extract the final sums and store them in the output
    for (int c=0; c<nChans; c++)
    {
      float sumArray[4];
      _mm_store_ps(sumArray, sum[c]);
      outputs[c][s] = sumArray[0] + sumArray[1] + sumArray[2] + sumArray[3];
    }
  }
#else // scalar
  inline void ReadSamples(double xBack, T** outputs, int s, int nChans) const
  {
    double bufferReadPosition = mWritePos - xBack;
    int bufferReadIndex = std::floor(bufferReadPosition);
    double bufferFracPosition = 1.0 - (bufferReadPosition - bufferReadIndex);

    bufferReadIndex = (bufferReadIndex + kBufferSize) & (kBufferSize - 1);
    bufferReadIndex += (bufferReadIndex <= static_cast<int>(A)) * kBufferSize;

    double tablePosition = bufferFracPosition * kTablePoints;
    int tableIndex = static_cast<int>(tablePosition);
    double tableFracPosition = (tablePosition - tableIndex);

    T sum[NCHANS] = {0.0};

    for (auto i=0; i<A; i++)
    {
      auto f0 = sTable[tableIndex][i];
      const auto df0 = sDeltaTable[tableIndex][i];
      f0 += df0 * tableFracPosition;

      auto f1 = sTable[tableIndex][A+i];
      const auto df1 = sDeltaTable[tableIndex][A+i];
      f1 += df1 * tableFracPosition;

      for (auto c=0; c<nChans;c++)
      {
        const auto d0 = mInputBuffer[c][bufferReadIndex - A + i];
        const auto d1 = mInputBuffer[c][bufferReadIndex + i];
        const auto rv = (f0 * d0) + (f1 * d1);
        sum[c] += rv;
      }
    }

    for (auto c=0; c<nChans;c++)
    {
      outputs[c][s] = sum[c];
    }
  }
#endif
  
  static T sTable alignas(16)[kTablePoints + 1][kFilterWidth];
  static T sDeltaTable alignas(16)[kTablePoints + 1][kFilterWidth];
  static bool sTablesInitialized;
  
  T mInputBuffer[NCHANS][kBufferSize * 2];
  int mWritePos = 0;
  const float mInputSampleRate;
  const float mOutputSamplerate;
  double mPhaseIn = 0.0;
  double mPhaseOut = 0.0;
  double mPhaseInIncr = 1.0;
  double mPhaseOutIncr = 0.0;
} WDL_FIXALIGN;

template<typename T, int NCHANS, size_t A>
T LanczosResampler<T, NCHANS, A>::sTable alignas(16) [LanczosResampler<T, NCHANS, A>::kTablePoints + 1][LanczosResampler::kFilterWidth];

template<typename T, int NCHANS, size_t A>
T LanczosResampler<T, NCHANS, A>::sDeltaTable alignas(16) [LanczosResampler<T, NCHANS, A>::kTablePoints + 1][LanczosResampler::kFilterWidth];

template<typename T, int NCHANS, size_t A>
bool LanczosResampler<T, NCHANS, A>::sTablesInitialized{false};

} // namespace iplug

