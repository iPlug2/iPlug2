/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include <functional>
#include <cmath>

#include "IPlugPlatform.h"
#include "LanczosResampler.h"

#include "heapbuf.h"
#include "ptrlist.h"

BEGIN_IPLUG_NAMESPACE

/** A multi-channel real-time resampling container that can be used to resample
 * audio processing to a specified sample rate for the situation where you have
 * some arbitary DSP code that requires a specific rate

 * Two modes are supported:
 * - Linear interpolation: simple linear interpolation between samples
 * - Lanczos resampling uses an approximation of the sinc function to
 *   interpolate between samples. This is the highest quality resampling mode.
 * 
 * The Lanczos resampler has a configurable filter size (A) that affects the 
 * latency of the resampler. It can also optionally use SIMD instructions
 * when T==float.
 *
 * @tparam T the sampletype float or double
 * @tparam NCHANS the number of channels
 * @tparam A The Lanczos filter size for the LanczosResampler resampler mode
 * A higher value makes the filter closer to an ideal stop-band that rejects high-frequency
 * content (anti-aliasing), but at the expense of higher latency
 */
template<typename T = double, int NCHANS=2, size_t A=12>
class RealtimeResampler
{
  static_assert(std::is_same<T, float>::value || std::is_same<T, double>::value, "T must be float or double");

public:
  enum class ESRCMode
  {
    kLinearInterpolation = 0,
    kLancsoz,
    kNumResamplingModes
  };

  using BlockProcessFunc = std::function<void(T**, T**, int, int)>;
  using LanczosResampler = LanczosResampler<T, NCHANS, A>;

  /** Constructor
   * @param innerSampleRate The sample rate that the provided DSP block will process at
   * @param mode The sample rate conversion mode
   */
  RealtimeResampler(double innerSampleRate, ESRCMode mode = ESRCMode::kLancsoz)
  : mResamplingMode(mode)
  , mInnerSampleRate(innerSampleRate)
  {
  }
  
  RealtimeResampler(const RealtimeResampler&) = delete;
  RealtimeResampler& operator=(const RealtimeResampler&) = delete;

  /** Reset the underlying DSP (when the samplerate or max expected block size changes)
   *  @param inputSampleRate: The external sample rate interacting with this object.
   *  @param maxBlockSize: The largest block size expected to be passed via nFrames in processBlock
   */
  void Reset(double inputSampleRate, int maxBlockSize = DEFAULT_BLOCK_SIZE)
  {
    mOuterSampleRate = inputSampleRate;
    mInRatio = mOuterSampleRate / mInnerSampleRate;
    mOutRatio = mInnerSampleRate / mOuterSampleRate;
    mMaxOuterLength = maxBlockSize;
    mMaxInnerLength = CalculateMaxInnerLength(mMaxOuterLength);

    mInputData.Resize(mMaxInnerLength * NCHANS);
    mOutputData.Resize(mMaxInnerLength * NCHANS);
    mInputPtrs.Empty();
    mOutputPtrs.Empty();
    
    for (auto chan=0; chan<NCHANS; chan++)
    {
      mInputPtrs.Add(mInputData.Get() + (chan * mMaxInnerLength));
      mOutputPtrs.Add(mOutputData.Get() + (chan * mMaxInnerLength));
    }

    ClearBuffers();
  
    if (mResamplingMode == ESRCMode::kLancsoz)
    {
      mInResampler = std::make_unique<LanczosResampler>(mOuterSampleRate, mInnerSampleRate);
      mOutResampler = std::make_unique<LanczosResampler>(mInnerSampleRate, mOuterSampleRate);
      
      // Warm up the resamplers with enough silence that the first real buffer can yield the required number of output samples.
      const auto outSamplesRequired = mOutResampler->GetNumSamplesRequiredFor(1);
      const auto inSamplesRequired = mInResampler->GetNumSamplesRequiredFor(outSamplesRequired);
      mInResampler->PushBlock(mInputPtrs.GetList(), inSamplesRequired, NCHANS);
      const auto populated = mInResampler->PopBlock(mInputPtrs.GetList(), outSamplesRequired, NCHANS);
      assert(populated >= outSamplesRequired && "Didn't get enough samples required for warm up!");
      mOutResampler->PushBlock(mOutputPtrs.GetList(), populated, NCHANS);
      
      // magic number that we seem to need to align when compensating for latency
      constexpr auto addedLatency = 2;
      mLatency = static_cast<int>(inSamplesRequired + addedLatency);
    }
    else
    {
      mLatency = 0;
    }
  }

  /** Resample an input block with a per-block function (resample input -> process with function -> resample back to external sample rate)
   * @param inputs Two-dimensional array containing the non-interleaved input buffers of audio samples for all channels
   * @param outputs Two-dimensional array for audio output (non-interleaved).
   * @param nFrames The block size for this block: number of samples per channel.
   * @param func The function that processes the audio sample at the higher sampling rate. 
   * NOTE: std::function can call malloc if you pass in captures */
  void ProcessBlock(T** inputs, T** outputs, int nFrames, int nChans, BlockProcessFunc func)
  {
    if (mInnerSampleRate == mOuterSampleRate) // nothing to do!
    {
      func(inputs, outputs, nFrames, nChans);
      return;
    }

    switch (mResamplingMode)
    {
      case ESRCMode::kLinearInterpolation:
      {
        const auto nNewFrames = LinearInterpolate(inputs, mInputPtrs.GetList(), nFrames, nChans, mInRatio, mMaxInnerLength);
        func(mInputPtrs.GetList(), mOutputPtrs.GetList(), nNewFrames, nChans);
        LinearInterpolate(mOutputPtrs.GetList(), outputs, nNewFrames, nChans, mOutRatio, nFrames);
        break;
      }
      case ESRCMode::kLancsoz:
      {
        mInResampler->PushBlock(inputs, nFrames, nChans);
        const auto maxInnerLength = CalculateMaxInnerLength(nFrames);

        while (mInResampler->GetNumSamplesRequiredFor(1) == 0) // i.e. there's signal still available to pop
        {
          const auto populated = mInResampler->PopBlock(mInputPtrs.GetList(), maxInnerLength, nChans);
          assert(populated <= maxInnerLength && "Received more samples than the encapsulated DSP is able to handle!");
          func(mInputPtrs.GetList(), mOutputPtrs.GetList(), static_cast<int>(populated), nChans);
          mOutResampler->PushBlock(mOutputPtrs.GetList(), populated, nChans);
        }
        
#ifdef _DEBUG
        const auto populated =
#endif
        mOutResampler->PopBlock(outputs, nFrames, nChans);
        assert(populated >= nFrames && "Did not yield enough samples to provide the required output buffer!");
        
        mInResampler->RenormalizePhases();
        mOutResampler->RenormalizePhases();
        break;
      }
      default:
        break;
    }
  }
  
  /** Get the latency of the resampling, not including any latency of the encapsulated DSP */
  int GetLatency() const { return mLatency; }

private:
  /** Interpolate the signal across the block with a specific resampling ratio */
  static inline int LinearInterpolate(T** inputs, T** outputs, int inputLength, int nChans, double ratio, int maxOutputLength)
  {
    const auto outputLength =
      std::min(static_cast<int>(std::ceil(static_cast<double>(inputLength) / ratio)), maxOutputLength);

    for (auto writePos=0; writePos<outputLength; writePos++)
    {
      const auto readPos = ratio * static_cast<double>(writePos);
      const auto readPostionTrunc = std::floor(readPos);
      const auto readPosInt = static_cast<int>(readPostionTrunc);

      if (readPosInt < inputLength)
      {
        const auto y = readPos - readPostionTrunc;

        for (auto chan=0; chan<nChans; chan++)
        {
          const auto x0 = inputs[chan][readPosInt];
          const auto x1 = ((readPosInt + 1) < inputLength) ? inputs[chan][readPosInt + 1] 
                                                           : inputs[chan][readPosInt - 1];
          outputs[chan][writePos] = (1.0 - y) * x0 + y * x1;
        }
      }
    }

    return outputLength;
  }

  /** Zero the memory in the scratch buffers */
  void ClearBuffers()
  {
    const auto nBytes = mMaxInnerLength * NCHANS * sizeof(T);
    memset(mInputData.Get(), 0, nBytes);
    memset(mOutputData.Get(), 0, nBytes);
  }
  
  /** Calculate the signal length in samples required for the encapsulated processing */
  int CalculateMaxInnerLength(const int outerLength) const
  {
    return static_cast<int>(std::ceil(double(outerLength) / mInRatio));
  }
  
  WDL_TypedBuf<T> mInputData, mOutputData;
  WDL_PtrList<T> mInputPtrs, mOutputPtrs;
  double mInRatio = 0.0, mOutRatio = 0.0;
  double mOuterSampleRate = DEFAULT_SAMPLE_RATE;
  const double mInnerSampleRate;
  int mMaxOuterLength = 0; // The maximum outer block size expected
  int mMaxInnerLength = 0; // The computed maximum inner block size
  int mLatency = 0;
  const ESRCMode mResamplingMode;
  std::unique_ptr<LanczosResampler> mInResampler, mOutResampler;
} WDL_FIXALIGN;

END_IPLUG_NAMESPACE
