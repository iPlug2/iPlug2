/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc ISender
 * @brief 
 */

#include "denormal.h"
#include "fft.h"

#include "IPlugPlatform.h"
#include "IPlugQueue.h"
#include <array>

#if defined OS_IOS || defined OS_MAC
#include <accelerate/accelerate.h>
#endif


BEGIN_IPLUG_NAMESPACE

/** ISenderData is used to represent a typed data packet, that may contain values for multiple channels */
template <int MAXNC = 1, typename T = float>
struct ISenderData
{
  int ctrlTag = kNoTag;
  int nChans = MAXNC;
  int chanOffset = 0;
  std::array<T, MAXNC> vals;
  
  ISenderData() {}
  
  ISenderData(int ctrlTag, int nChans, int chanOffset)
  : ctrlTag(ctrlTag)
  , nChans(nChans)
  , chanOffset(chanOffset)
  {
    memset(vals.data(), 0, sizeof(vals));
  }
  
  ISenderData(int ctrlTag, const std::array<T, MAXNC>& vals, int nChans = MAXNC, int chanOffset = 0)
  : ctrlTag(ctrlTag)
  , nChans(nChans)
  , chanOffset(chanOffset)
  , vals(vals)
  {
  }
};

/** ISender is a utility class which can be used to defer data from the realtime audio processing and send it to the GUI for visualization */
template <int MAXNC = 1, int QUEUE_SIZE = 64, typename T = float>
class ISender
{
public:
  static constexpr int kUpdateMessage = 0;

  /** Pushes a data element onto the queue. This can be called on the realtime audio thread. */
  void PushData(const ISenderData<MAXNC, T>& d)
  {
    mQueue.Push(d);
  }

  /** This is called on the main thread and can be used to transform the data, e.g. take an FFT. */
  virtual void PrepareDataForUI(ISenderData<MAXNC, T>& d) { /* NO-OP*/ }
  
  /** Pops elements off the queue and sends messages to controls.
   *  This must be called on the main thread - typically in MyPlugin::OnIdle() */
  void TransmitData(IEditorDelegate& dlg)
  {
    while (mQueue.ElementsAvailable())
    {
      ISenderData<MAXNC, T> d;
      mQueue.Pop(d);
      PrepareDataForUI(d);
      dlg.SendControlMsgFromDelegate(d.ctrlTag, kUpdateMessage, sizeof(ISenderData<MAXNC, T>), (void*) &d);
    }
  }

private:
  IPlugQueue<ISenderData<MAXNC, T>> mQueue {QUEUE_SIZE};
};

/** IPeakSender is a utility class which can be used to defer peak data from sample buffers for sending to the GUI
 * It sends the average peak value over a certain time window.
 */
template <int MAXNC = 1, int QUEUE_SIZE = 64>
class IPeakSender : public ISender<MAXNC, QUEUE_SIZE, float>
{
public:
  IPeakSender(double minThresholdDb = -90., float windowSizeMs = 5.0f)
  : ISender<MAXNC, QUEUE_SIZE, float>()
  , mThreshold(static_cast<float>(DBToAmp(minThresholdDb)))
  {
    Reset(DEFAULT_SAMPLE_RATE);
  }
  
  void Reset(double sampleRate)
  {
    SetWindowSizeMs(mWindowSizeMs, sampleRate);
    std::fill(mPeaks.begin(), mPeaks.end(), 0.0f);
  }
  
  void SetWindowSizeMs(double timeMs, double sampleRate)
  {
    mWindowSizeMs = timeMs;
    mWindowSize = static_cast<int>(timeMs * 0.001 * sampleRate);
  }
  
  /** Queue peaks from sample buffers into the sender. This can be called on the realtime audio thread. */
  void ProcessBlock(sample** inputs, int nFrames, int ctrlTag, int nChans = MAXNC, int chanOffset = 0)
  {
    for (auto s = 0; s < nFrames; s++)
    {
      if (mCount == 0)
      {
        ISenderData<MAXNC, float> d {ctrlTag, nChans, chanOffset};
        
        float sum = 0.0f;
        
        for (auto c = chanOffset; c < (chanOffset + nChans); c++)
        {
          d.vals[c] = mPeaks[c] / mWindowSize;
          mPeaks[c] = 0.0f;
          sum += d.vals[c];
        }
        
        if (sum > mThreshold || mPreviousSum > mThreshold)
          ISender<MAXNC, QUEUE_SIZE, float>::PushData(d);

        mPreviousSum = sum;
      }
      
      for (auto c = chanOffset; c < (chanOffset + nChans); c++)
      {
        mPeaks[c] += std::fabs(inputs[c][s]);
      }
      
      mCount++;
      mCount %= mWindowSize;
    }
  }
private:
  float mPreviousSum = 1.f;
  float mThreshold = 0.01f;
  float mWindowSizeMs = 5.0f;
  int mWindowSize = 32;
  int mCount = 0;
  std::array<float, MAXNC> mPeaks = {0.0};
};

/** IPeakAvgSender is a utility class which can be used to defer peak & avg/RMS data from sample buffers for sending to the GUI
 * It also features an envelope follower to control meter ballistics
 */
template <int MAXNC = 1, int QUEUE_SIZE = 64>
class IPeakAvgSender : public ISender<MAXNC, QUEUE_SIZE, std::pair<float, float>>
{
public:
  class EnvelopeFollower
  {
  public:
    inline float Process(float& input, float& attackTimeSamples, float& decayTimeSamples)
    {
      if (input > mPreviousOutput)
      {
        if (attackTimeSamples > 1.0f)
        {
          mPreviousOutput += (input - mPreviousOutput) / attackTimeSamples;
        }
        else
        {
          mPreviousOutput = input;
        }
      }
      else if (input < mPreviousOutput)
      {
        if (decayTimeSamples > 1.0f)
        {
          mPreviousOutput += (input - mPreviousOutput) / decayTimeSamples;
        }
        else
        {
          mPreviousOutput = input;
        }
      }
      
      denormal_fix(&mPreviousOutput);
      return mPreviousOutput;
    }

  private:
    float mPreviousOutput = 0.0f;
  };
  
  IPeakAvgSender(double minThresholdDb = -90.0, bool rmsMode = true, float windowSizeMs = 5.0f, float attackTimeMs = 1.0f, float decayTimeMs = 100.0f, float peakHoldTimeMs = 500.0f)
  : ISender<MAXNC, QUEUE_SIZE, std::pair<float, float>>()
  , mThreshold(static_cast<float>(DBToAmp(minThresholdDb)))
  , mRMSMode(rmsMode)
  , mWindowSizeMs(windowSizeMs)
  , mAttackTimeMs(attackTimeMs)
  , mDecayTimeMs(decayTimeMs)
  , mPeakHoldTimeMs(peakHoldTimeMs)
  {
    Reset(DEFAULT_SAMPLE_RATE);
  }
  
  void Reset(double sampleRate)
  {
    SetWindowSizeMs(mWindowSizeMs, sampleRate);
    SetAttackTimeMs(mAttackTimeMs, sampleRate);
    SetDecayTimeMs(mDecayTimeMs, sampleRate);
    SetPeakHoldTimeMs(mPeakHoldTimeMs, sampleRate);
    std::fill(mHeldPeaks.begin(), mHeldPeaks.end(), 0.0f);
  }
  
  void SetAttackTimeMs(double timeMs, double sampleRate)
  {
    mAttackTimeMs = timeMs;
    mAttackTimeSamples = static_cast<int>(timeMs * 0.001 * (sampleRate / mWindowSize));
  }
  
  void SetDecayTimeMs(double timeMs, double sampleRate)
  {
    mDecayTimeMs = timeMs;
    mDecayTimeSamples = static_cast<int>(timeMs * 0.001 * (sampleRate / mWindowSize));
  }
  
  void SetWindowSizeMs(double timeMs, double sampleRate)
  {
    mWindowSizeMs = timeMs;
    mWindowSize = static_cast<int>(timeMs * 0.001 * sampleRate);

    for (auto i=0; i<MAXNC; i++)
    {
      mBuffers[i].resize(mWindowSize);
      std::fill(mBuffers[i].begin(), mBuffers[i].end(), 0.0f);
    }
  }
  
  void SetPeakHoldTimeMs(double timeMs, double sampleRate)
  {
    mPeakHoldTimeMs = timeMs;
    mPeakHoldTime = static_cast<int>(timeMs * 0.001 * sampleRate);
    std::fill(mPeakHoldCounters.begin(), mPeakHoldCounters.end(), mPeakHoldTime);
  }
  
  /** Queue peaks from sample buffers into the sender. This can be called on the realtime audio thread. */
  void ProcessBlock(sample** inputs, int nFrames, int ctrlTag, int nChans = MAXNC, int chanOffset = 0)
  {
    for (auto s = 0; s < nFrames; s++)
    {
      int windowPos = s % mWindowSize;
      
      for (auto c = chanOffset; c < (chanOffset + nChans); c++)
      {
        mBuffers[c][windowPos] = inputs[c][s];
      }
      
      if (mCount == 0)
      {
        ISenderData<MAXNC, std::pair<float, float>> d {ctrlTag, nChans, chanOffset};
        
        auto avgSum = 0.0f;
        
        for (auto c = chanOffset; c < (chanOffset + nChans); c++)
        {
          auto peakVal = 0.0f;
          auto avgVal = 0.0f;
#if defined OS_IOS || defined OS_MAC
          vDSP_vabs(mBuffers[c].data(), 1, mBuffers[c].data(), 1, mWindowSize);
          vDSP_maxv(mBuffers[c].data(), 1, &peakVal, mWindowSize);
          
          if (mRMSMode)
          {
            vDSP_rmsqv(mBuffers[c].data(), 1, &avgVal, mWindowSize);
          }
          else
          {
            vDSP_meanv(mBuffers[c].data(), 1, &avgVal, mWindowSize);
          }
#else
          for (auto i=0; i<mWindowSize; i++)
          {
            auto absVal = std::fabs(mBuffers[c][i]);
            
            if (absVal > peakVal)
            {
              peakVal = absVal;
            }
            
            if (mRMSMode)
            {
              absVal = absVal * absVal;
            }
            
            avgVal += absVal;
          }
          
          avgVal /= static_cast<float>(mWindowSize);
          
          if (mRMSMode)
          {
            avgVal = std::sqrt(avgVal);
          }
#endif
      
          // set peak-hold value
          if (mPeakHoldCounters[c] <= 0)
          {
            mHeldPeaks[c] = 0.0f;
          }
          
          if (mHeldPeaks[c] < peakVal)
          {
            mHeldPeaks[c] = peakVal;
            mPeakHoldCounters[c] = mPeakHoldTime;
          }
          else
          {
            if (mPeakHoldCounters[c] > 0)
            {
              mPeakHoldCounters[c] -= mWindowSize;
            }
          }
          
          std::get<0>(d.vals[c]) = mHeldPeaks[c];
          
          // set avg value
          auto smoothedAvg = mEnvFollowers[c].Process(avgVal, mAttackTimeSamples, mDecayTimeSamples);
          std::get<1>(d.vals[c]) = smoothedAvg;
          
          avgSum += smoothedAvg;
        }
        
        if (mPreviousSum > mThreshold)
        {
          ISender<MAXNC, QUEUE_SIZE, std::pair<float, float>>::PushData(d);
        }
        else
        {
          // This makes sure that the data is still pushed if
          // peakholds are still active
          bool counterActive = false;
          
          for (auto c = chanOffset; c < (chanOffset + nChans); c++)
          {
            counterActive &= mPeakHoldCounters[c] > 0;
            std::get<0>(d.vals[c]) = 0.0f;
            std::get<1>(d.vals[c]) = 0.0f;
          }
          
          if (counterActive)
          {
            ISender<MAXNC, QUEUE_SIZE, std::pair<float, float>>::PushData(d);
          }
        }
        
        mPreviousSum = avgSum;
      }
      
      mCount++;
      mCount %= mWindowSize;
    }
  }
private:
  float mThreshold = 0.01f;
  bool mRMSMode = false;
  float mPreviousSum = 1.0f;
  int mWindowSize = 32;
  int mPeakHoldTime = 1 << 16;
  int mCount = 0;
  float mWindowSizeMs = 5.f;
  float mAttackTimeMs = 1.f;
  float mDecayTimeMs = 100.f;
  float mPeakHoldTimeMs = 100.f;
  float mAttackTimeSamples = 1.0f;
  float mDecayTimeSamples = DEFAULT_SAMPLE_RATE/10.0f;
  std::array<float, MAXNC> mHeldPeaks = {0};
  std::array<std::vector<float>, MAXNC> mBuffers;
  std::array<int, MAXNC> mPeakHoldCounters;
  std::array<EnvelopeFollower, MAXNC> mEnvFollowers;
};

/** IBufferSender is a utility class which can be used to defer buffer data for sending to the GUI */
template <int MAXNC = 1, int QUEUE_SIZE = 64, int MAXBUF = 128>
class IBufferSender : public ISender<MAXNC, QUEUE_SIZE, std::array<float, MAXBUF>>
{
public:
  using TDataPacket = std::array<float, MAXBUF>;
  using TSender = ISender<MAXNC, QUEUE_SIZE, TDataPacket>;
  
  IBufferSender(double minThresholdDb = -90., int bufferSize = MAXBUF)
  : TSender()
  {
    if (minThresholdDb == -std::numeric_limits<double>::infinity())
    {
      mThreshold = -1.0f;
    }
    else
    {
      mThreshold = static_cast<float>(DBToAmp(minThresholdDb));
    }
    
    SetBufferSize(bufferSize);
  }

  /** Queue sample buffers into the sender, checking the data is over the required threshold. This can be called on the realtime audio thread. */
  void ProcessBlock(sample** inputs, int nFrames, int ctrlTag, int nChans = MAXNC, int chanOffset = 0)
  {
    for (auto s = 0; s < nFrames; s++)
    {
      if (mBufCount == mBufferSize)
      {
        float sum = 0.0f;
        for (auto c = chanOffset; c < (chanOffset + nChans); c++)
        {
          sum += mRunningSum[c];
          mRunningSum[c] = 0.0f;
        }

        if (sum > mThreshold || mPreviousSum > mThreshold)
        {
          mBuffer.ctrlTag = ctrlTag;
          mBuffer.nChans = nChans;
          mBuffer.chanOffset = chanOffset;
          TSender::PushData(mBuffer);
        }

        mPreviousSum = sum;
        mBufCount = 0;
      }
      
      for (auto c = chanOffset; c < (chanOffset + nChans); c++)
      {
        const float inputSample = static_cast<float>(inputs[c][s]);
        mBuffer.vals[c][mBufCount] = inputSample;
        mRunningSum[c] += std::fabs(inputSample);
      }

      mBufCount++;
    }
  }
  
  void SetBufferSize(int bufferSize)
  {
    assert(bufferSize > 0);
    assert(bufferSize <= MAXBUF);

    mBufferSize = bufferSize;
    mBufCount = 0;
  }

  int GetBufferSize() const { return mBufferSize; }
  
private:
  ISenderData<MAXNC, TDataPacket> mBuffer;
  int mBufCount = 0;
  int mBufferSize = MAXBUF;
  std::array<float, MAXNC> mRunningSum {0.};
  float mPreviousSum = 1.f;
  float mThreshold = 0.01f;
};

template <int MAXNC = 1, int QUEUE_SIZE = 64, int MAX_FFT_SIZE = 4096>
class ISpectrumSender : public IBufferSender<MAXNC, QUEUE_SIZE, MAX_FFT_SIZE>
{
public:
  using TDataPacket = std::array<float, MAX_FFT_SIZE>;
  using TBufferSender = IBufferSender<MAXNC, QUEUE_SIZE, MAX_FFT_SIZE>;
  
  enum class EWindowType {
    Hann = 0,
    BlackmanHarris,
    Hamming,
    Flattop,
    Rectangular
  };
  
  enum class EOutputType {
    Complex = 0,
    MagPhase,
  };
  
  ISpectrumSender(int fftSize = 1024, int overlap = 2, EWindowType window = EWindowType::Hann, EOutputType outputType = EOutputType::MagPhase)
  : TBufferSender(-std::numeric_limits<double>::infinity(), fftSize)
  , mWindowType(window)
  , mOutputType(outputType)
  {
    WDL_fft_init();
    SetFFTSizeAndOverlap(fftSize, overlap);
  }

  void SetFFTSizeAndOverlap(int fftSize, int overlap)
  {
    mOverlap = overlap;
    TBufferSender::SetBufferSize(fftSize);
    SetFFTSize();
    CalculateWindow();
    CalculateScalingFactors();
  }
  
  void SetWindowType(EWindowType windowType)
  {
    mWindowType = windowType;
    CalculateWindow();
  }
  
  void PrepareDataForUI(ISenderData<MAXNC, TDataPacket>& d) override
  {
    auto fftSize = TBufferSender::GetBufferSize();

    for (auto s = 0; s < fftSize; s++)
    {
      for (int stftFrameIdx = 0; stftFrameIdx < mOverlap; stftFrameIdx++)
      {
        auto& stftFrame = mSTFTFrames[stftFrameIdx];
        
        for (auto ch = 0; ch < MAXNC; ch++)
        {
          auto windowedValue = (float) d.vals[ch][s] * mWindow[stftFrame.pos];
          stftFrame.bins[ch][stftFrame.pos].re = windowedValue;
          stftFrame.bins[ch][stftFrame.pos].im = 0.0f;
        }
        
        stftFrame.pos++;

        if (stftFrame.pos >= fftSize)
        {
          stftFrame.pos = 0;
          
          for (auto ch = 0; ch < MAXNC; ch++)
          {
            Permute(ch, stftFrameIdx);
            memcpy(d.vals[ch].data(), mSTFTOutput[ch].data(), fftSize * sizeof(float));
          }
        }
      }
    }
  }
  
private:
  void SetFFTSize()
  {
    if (mSTFTFrames.size() != mOverlap)
    {
      mSTFTFrames.resize(mOverlap);
    }
    
    for (auto&& frame : mSTFTFrames)
    {
      for (auto ch = 0; ch < MAXNC; ch++)
      {
        std::fill(frame.bins[ch].begin(), frame.bins[ch].end(), WDL_FFT_COMPLEX{0.0f, 0.0f});
      }
      
      frame.pos = 0;
    }
    
    for (auto ch = 0; ch < MAXNC; ch++)
    {
      std::fill(mSTFTOutput[ch].begin(), mSTFTOutput[ch].end(), 0.0f);
    }
  }
  
  void CalculateWindow()
  {
    const auto fftSize = TBufferSender::GetBufferSize();

    const float M = static_cast<float>(fftSize - 1);
    
    switch (mWindowType)
    {
      case EWindowType::Hann:
        for (auto i = 0; i < fftSize; i++) { mWindow[i] = 0.5f * (1.0f - std::cos(PI * 2.0f * i / M)); }
        break;
      case EWindowType::BlackmanHarris:
        for (auto i = 0; i < fftSize; i++) {
          mWindow[i] = 0.35875 - (0.48829f * std::cos(2.0f * PI * i / M)) +
                                 (0.14128f * std::cos(4.0f * PI * i / M)) -
                                 (0.01168f * std::cos(6.0f * PI * i / M));
        }
        break;
      case EWindowType::Hamming:
        for (auto i = 0; i < fftSize; i++) { mWindow[i] = 0.54f - 0.46f * std::cos(2.0f * PI * i / M); }
        break;
      case EWindowType::Flattop:
        for (auto i = 0; i < fftSize; i++) {
          mWindow[i] = 0.21557895f - 0.41663158f * std::cos(2.0f * PI * i / M) +
                                    0.277263158f * std::cos(4.0f * PI * i / M) -
                                    0.083578947f * std::cos(6.0f * PI * i / M) +
                                    0.006947368f * std::cos(8.0f * PI * i / M);
        }
        break;
      case EWindowType::Rectangular:
        std::fill(mWindow.begin(), mWindow.end(), 1.0f);
        break;
      default:
        break;
    }
  }
  
  void CalculateScalingFactors()
  {
    const auto fftSize = TBufferSender::GetBufferSize();
    const float M = static_cast<float>(fftSize - 1);

    auto scaling = 0.0f;
    
    for (auto i = 0; i < fftSize; i++)
    {
      auto v = 0.5f * (1.0f - std::cos(2.0f * PI * i / M));
      scaling += v;
    }
    
    mScalingFactor = scaling * scaling;
  }
  
  void Permute(int ch, int frameIdx)
  {
    const auto fftSize = TBufferSender::GetBufferSize();
    WDL_fft(mSTFTFrames[frameIdx].bins[ch].data(), fftSize, false);

    if (mOutputType == EOutputType::Complex)
    {
      auto nBins = fftSize/2;
      for (auto i = 0; i < nBins; ++i)
      {
        int sortIdx = WDL_fft_permute(fftSize, i);
        mSTFTOutput[ch][i] = mSTFTFrames[frameIdx].bins[ch][sortIdx].re;
        mSTFTOutput[ch][i + nBins] = mSTFTFrames[frameIdx].bins[ch][sortIdx].im;
      }
    }
    else // magPhase
    {
      for (auto i = 0; i < fftSize; ++i)
      {
        int sortIdx = WDL_fft_permute(fftSize, i);
        auto re = mSTFTFrames[frameIdx].bins[ch][sortIdx].re;
        auto im = mSTFTFrames[frameIdx].bins[ch][sortIdx].im;
        mSTFTOutput[ch][i] = std::sqrt(2.0f * (re * re + im * im) / mScalingFactor);
      }
    }
  }
  
  struct STFTFrame
  {
    int pos;
    std::array<std::array<WDL_FFT_COMPLEX, MAX_FFT_SIZE>, MAXNC> bins;
  };
  
  int mOverlap = 2;
  EWindowType mWindowType;
  EOutputType mOutputType;
  std::array<float, MAX_FFT_SIZE> mWindow;
  std::vector<STFTFrame> mSTFTFrames;
  std::array<std::array<float, MAX_FFT_SIZE>, MAXNC> mSTFTOutput;
  float mScalingFactor = 0.0f;
};

END_IPLUG_NAMESPACE
