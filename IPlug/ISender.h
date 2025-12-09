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
#include <Accelerate/Accelerate.h>
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
  
  ISenderData()
  : vals{}
  {
  }
  
  ISenderData(int ctrlTag, int nChans, int chanOffset)
  : ctrlTag(ctrlTag)
  , nChans(nChans)
  , chanOffset(chanOffset)
  , vals{}
  {
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
      assert(d.ctrlTag != kNoTag && "You must supply a control tag");
      PrepareDataForUI(d);
      dlg.SendControlMsgFromDelegate(d.ctrlTag, kUpdateMessage, sizeof(ISenderData<MAXNC, T>), (void*) &d);
    }
  }
  
  /** This variation can be used if you need to supply multiple controls with the same ISenderData, overriding the tags in the data packet
   @param dlg The editor delegate
   @param ctrlTags A list of control tags that should receive the updates from this sender */
  void TransmitDataToControlsWithTags(IEditorDelegate& dlg, const std::initializer_list<int>& ctrlTags)
  {
    while(mQueue.ElementsAvailable())
    {
      ISenderData<MAXNC, T> d;
      mQueue.Pop(d);
      
      for (auto tag : ctrlTags)
      {
        d.ctrlTag = tag;
        dlg.SendControlMsgFromDelegate(tag, kUpdateMessage, sizeof(ISenderData<MAXNC, T>), (void*) &d);
      }
    }
  }

protected:
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
    mWindowSizeMs = static_cast<float>(timeMs);
    mWindowSize = static_cast<int>(timeMs * 0.001 * sampleRate);
  }
  
  /** Queue peaks from sample buffers into the sender This can be called on the realtime audio thread.
   @param inputs the sample buffers to analyze
   @param nFrames the number of sample frames in the input buffers
   @param ctrlTag a control tag to indicate which control to send the buffers to. Note: if you don't supply the control tag here, you must use TransmitDataToControlsWithTags() and specify one or more tags there
   @param nChans the number of channels of data that should be sent
   @param chanOffset the starting channel */
  void ProcessBlock(sample** inputs, int nFrames, int ctrlTag = kNoTag, int nChans = MAXNC, int chanOffset = 0)
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
        mPeaks[c] += std::fabs(static_cast<float>(inputs[c][s]));
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
    mAttackTimeMs = static_cast<float>(timeMs);
    mAttackTimeSamples = static_cast<float>(timeMs * 0.001 * (sampleRate / double(mWindowSize)));
  }
  
  void SetDecayTimeMs(double timeMs, double sampleRate)
  {
    mDecayTimeMs = static_cast<float>(timeMs);
    mDecayTimeSamples = static_cast<float>(timeMs * 0.001 * (sampleRate / mWindowSize));
  }
  
  void SetWindowSizeMs(double timeMs, double sampleRate)
  {
    mWindowSizeMs = static_cast<float>(timeMs);
    mWindowSize = static_cast<int>(timeMs * 0.001 * sampleRate);

    for (auto i=0; i<MAXNC; i++)
    {
      mBuffers[i].resize(mWindowSize);
      std::fill(mBuffers[i].begin(), mBuffers[i].end(), 0.0f);
    }
  }
  
  void SetPeakHoldTimeMs(double timeMs, double sampleRate)
  {
    mPeakHoldTimeMs = static_cast<float>(timeMs);
    mPeakHoldTime = static_cast<int>(timeMs * 0.001 * sampleRate);
    std::fill(mPeakHoldCounters.begin(), mPeakHoldCounters.end(), mPeakHoldTime);
  }
  
  /** Queue peaks from sample buffers into the sender This can be called on the realtime audio thread.
   @param inputs the sample buffers to analyze
   @param nFrames the number of sample frames in the input buffers
   @param ctrlTag a control tag to indicate which control to send the buffers to. Note: if you don't supply the control tag here, you must use TransmitDataToControlsWithTags() and specify one or more tags there
   @param nChans the number of channels of data that should be sent
   @param chanOffset the starting channel */
  void ProcessBlock(sample** inputs, int nFrames, int ctrlTag = kNoTag, int nChans = MAXNC, int chanOffset = 0)
  {
    for (auto s = 0; s < nFrames; s++)
    {
      int windowPos = s % mWindowSize;
      
      for (auto c = chanOffset; c < (chanOffset + nChans); c++)
      {
        mBuffers[c][windowPos] = static_cast<float>(inputs[c][s]);
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
  const double kNoThresholdDb = -100;

  IBufferSender(double minThresholdDb = -90., int bufferSize = MAXBUF)
  : TSender()
  {
    if (minThresholdDb <= kNoThresholdDb)
      mThreshold = -1.0f;
    else
      mThreshold = DBToAmp(minThresholdDb);
    
    SetBufferSize(bufferSize);
  }

  /** Queue sample buffers into the sender, checking the data is over the required threshold. This can be called on the realtime audio thread.
   @param inputs the sample buffers
   @param nFrames the number of sample frames in the input buffers
   @param ctrlTag a control tag to indicate which control to send the buffers to. Note: if you don't supply the control tag here, you must use TransmitDataToControlsWithTags() and specify one or more tags there
   @param nChans the number of channels of data that should be sent
   @param chanOffset the starting channel */
  void ProcessBlock(sample** inputs, int nFrames, int ctrlTag = kNoTag, int nChans = MAXNC, int chanOffset = 0)
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

/** ISpectrumSender is designed for sending Spectral Data from the plug-in to the UI */
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
  
  ISpectrumSender(int fftSize = 1024, int overlap = 2, EWindowType window = EWindowType::Hann, EOutputType outputType = EOutputType::MagPhase, double minThresholdDb = -100.0)
  : TBufferSender(minThresholdDb, fftSize / overlap)
  , mWindowType(window)
  , mOutputType(outputType)
  {
    WDL_fft_init();
    SetFFTSizeAndOverlap(fftSize, overlap);
  }

  void SetFFTSize(int fftSize)
  {
    SetFFTSizeAndOverlap(fftSize, mOverlap);
  }

  void SetFFTSizeAndOverlap(int fftSize, int overlap)
  {
    mFFTSize = fftSize;
    mOverlap = overlap;
    int hopSize = fftSize / overlap;
    TBufferSender::SetBufferSize(hopSize);
    InitSTFTFrames();
    CalculateWindow();
    CalculateScalingFactors();
  }
  
  void SetWindowType(EWindowType windowType)
  {
    mWindowType = windowType;
    CalculateWindow();
  }
  
  void SetOutputType(EOutputType outputType)
  {
    mOutputType = outputType;
  }
  
  void PrepareDataForUI(ISenderData<MAXNC, TDataPacket>& d) override
  {
    int hopSize = TBufferSender::GetBufferSize();

    for (auto s = 0; s < hopSize; s++)
    {
      for (auto stftFrameIdx = 0; stftFrameIdx < mOverlap; stftFrameIdx++)
      {
        auto& stftFrame = mSTFTFrames[stftFrameIdx];

        for (auto ch = 0; ch < MAXNC; ch++)
        {
          auto windowedValue = (float) d.vals[ch][s] * mWindow[stftFrame.pos];
          stftFrame.bins[ch][stftFrame.pos].re = windowedValue;
          stftFrame.bins[ch][stftFrame.pos].im = 0.0f;
        }

        stftFrame.pos++;

        if (stftFrame.pos >= mFFTSize)
        {
          stftFrame.pos = 0;

          for (auto ch = 0; ch < MAXNC; ch++)
          {
            Permute(ch, stftFrameIdx);
            memcpy(d.vals[ch].data(), mSTFTOutput[ch].data(), mFFTSize * sizeof(float));
          }
        }
      }
    }
  }

  int GetFFTSize() const
  {
    return mFFTSize;
  }

  int GetOverlap() const
  {
    return mOverlap;
  }
    

private:
  void InitSTFTFrames()
  {
    if (mSTFTFrames.size() != mOverlap)
    {
      mSTFTFrames.resize(mOverlap);
    }

    int hopSize = mFFTSize / mOverlap;

    for (int i = 0; i < mOverlap; i++)
    {
      auto& frame = mSTFTFrames[i];
      for (auto ch = 0; ch < MAXNC; ch++)
      {
        std::fill(frame.bins[ch].begin(), frame.bins[ch].end(), WDL_FFT_COMPLEX{0.0f, 0.0f});
      }
      // Stagger frame positions so FFTs are computed at different times
      frame.pos = i * hopSize;
    }

    for (auto ch = 0; ch < MAXNC; ch++)
    {
      std::fill(mSTFTOutput[ch].begin(), mSTFTOutput[ch].end(), 0.0f);
    }
  }
  
  void CalculateWindow()
  {
    const float M = static_cast<float>(mFFTSize - 1);

    switch (mWindowType)
    {
      case EWindowType::Hann:
        for (auto i = 0; i < mFFTSize; i++) { mWindow[i] = 0.5f * (1.0f - std::cos(PI * 2.0f * i / M)); }
        break;
      case EWindowType::BlackmanHarris:
        for (auto i = 0; i < mFFTSize; i++) {
          mWindow[i] = 0.35875 - (0.48829f * std::cos(2.0f * PI * i / M)) +
                                 (0.14128f * std::cos(4.0f * PI * i / M)) -
                                 (0.01168f * std::cos(6.0f * PI * i / M));
        }
        break;
      case EWindowType::Hamming:
        for (auto i = 0; i < mFFTSize; i++) { mWindow[i] = 0.54f - 0.46f * std::cos(2.0f * PI * i / M); }
        break;
      case EWindowType::Flattop:
        for (auto i = 0; i < mFFTSize; i++) {
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
    const float M = static_cast<float>(mFFTSize - 1);

    auto scaling = 0.0f;

    for (auto i = 0; i < mFFTSize; i++)
    {
      auto v = 0.5f * (1.0f - std::cos(2.0f * PI * i / M));
      scaling += v;
    }

    mScalingFactor = scaling * scaling;
  }

  void Permute(int ch, int frameIdx)
  {
    WDL_fft(mSTFTFrames[frameIdx].bins[ch].data(), mFFTSize, false);

    if (mOutputType == EOutputType::Complex)
    {
      auto nBins = mFFTSize / 2;
      for (auto i = 0; i < nBins; ++i)
      {
        int sortIdx = WDL_fft_permute(mFFTSize, i);
        mSTFTOutput[ch][i] = mSTFTFrames[frameIdx].bins[ch][sortIdx].re;
        mSTFTOutput[ch][i + nBins] = mSTFTFrames[frameIdx].bins[ch][sortIdx].im;
      }
    }
    else // magPhase
    {
      auto nBins = mFFTSize / 2;
      for (auto i = 0; i < nBins; ++i)
      {
        int sortIdx = WDL_fft_permute(mFFTSize, i);
        auto re = mSTFTFrames[frameIdx].bins[ch][sortIdx].re;
        auto im = mSTFTFrames[frameIdx].bins[ch][sortIdx].im;
        mSTFTOutput[ch][i] = std::sqrt(2.0f * (re * re + im * im) / mScalingFactor);
        mSTFTOutput[ch][i + nBins] = std::atan2(im, re);
      }
    }
  }
  
  struct STFTFrame
  {
    int pos;
    std::array<std::array<WDL_FFT_COMPLEX, MAX_FFT_SIZE>, MAXNC> bins;
  };

  int mFFTSize = 1024;
  int mOverlap = 2;
  EWindowType mWindowType;
  EOutputType mOutputType;
  std::array<float, MAX_FFT_SIZE> mWindow;
  std::vector<STFTFrame> mSTFTFrames;
  std::array<std::array<float, MAX_FFT_SIZE>, MAXNC> mSTFTOutput;
  float mScalingFactor = 0.0f;
};

/** IGoniometerData represents a packet of stereo sample pairs for goniometer visualization,
 * along with computed stereo correlation coefficient */
template <int MAXPOINTS = 512>
struct IGoniometerData
{
  int ctrlTag = kNoTag;
  int nPoints = 0;
  float correlation = 0.0f; // -1 (out of phase) to 1 (mono/in phase)
  std::array<std::pair<float, float>, MAXPOINTS> points; // L/R sample pairs

  IGoniometerData() : points{} {}
};

/** IGoniometerSender is a utility class for sending stereo sample pairs and correlation data
 * to a goniometer/vectorscope display. It collects L/R sample pairs and computes the
 * stereo correlation coefficient over a configurable window.
 *
 * The correlation coefficient indicates phase coherence:
 * - +1.0 = mono (L and R identical)
 * - 0.0 = uncorrelated (independent L and R)
 * - -1.0 = out of phase (L and R are inverted)
 */
template <int MAXPOINTS = 512, int QUEUE_SIZE = 64>
class IGoniometerSender : public ISender<1, QUEUE_SIZE, IGoniometerData<MAXPOINTS>>
{
public:
  using TDataPacket = IGoniometerData<MAXPOINTS>;
  using TSender = ISender<1, QUEUE_SIZE, TDataPacket>;

  /** Construct an IGoniometerSender
   * @param windowSizeMs Time window for computing correlation coefficient
   * @param decimation Decimation factor - only every Nth sample pair is sent to reduce UI load */
  IGoniometerSender(float windowSizeMs = 50.0f, int decimation = 4)
  : TSender()
  , mWindowSizeMs(windowSizeMs)
  , mDecimation(decimation)
  {
    Reset(DEFAULT_SAMPLE_RATE);
  }

  void Reset(double sampleRate)
  {
    mSampleRate = sampleRate;
    mWindowSize = static_cast<int>(mWindowSizeMs * 0.001 * sampleRate);
    mSampleCount = 0;
    mPointCount = 0;
    mSumL = mSumR = mSumL2 = mSumR2 = mSumLR = 0.0;
  }

  /** Set the time window for correlation computation
   * @param windowSizeMs Window size in milliseconds */
  void SetWindowSizeMs(float windowSizeMs)
  {
    mWindowSizeMs = windowSizeMs;
    mWindowSize = static_cast<int>(windowSizeMs * 0.001 * mSampleRate);
  }

  /** Set decimation factor
   * @param decimation Only every Nth sample is stored for display */
  void SetDecimation(int decimation)
  {
    mDecimation = std::max(1, decimation);
  }

  /** Process a block of stereo audio and queue data for the goniometer
   * @param inputs Array of input buffers (must have at least 2 channels)
   * @param nFrames Number of sample frames
   * @param ctrlTag Control tag for the target goniometer control
   * @param leftChan Index of the left channel (default 0)
   * @param rightChan Index of the right channel (default 1) */
  void ProcessBlock(sample** inputs, int nFrames, int ctrlTag = kNoTag, int leftChan = 0, int rightChan = 1)
  {
    for (int s = 0; s < nFrames; s++)
    {
      const float L = static_cast<float>(inputs[leftChan][s]);
      const float R = static_cast<float>(inputs[rightChan][s]);

      // Accumulate for correlation calculation
      mSumL += L;
      mSumR += R;
      mSumL2 += L * L;
      mSumR2 += R * R;
      mSumLR += L * R;

      // Store decimated sample pairs
      if ((mSampleCount % mDecimation) == 0 && mPointCount < MAXPOINTS)
      {
        mBuffer.vals[0].points[mPointCount] = std::make_pair(L, R);
        mPointCount++;
      }

      mSampleCount++;

      // When window is complete, compute correlation and send data
      if (mSampleCount >= mWindowSize)
      {
        // Compute Pearson correlation coefficient
        const double n = static_cast<double>(mWindowSize);
        const double numerator = n * mSumLR - mSumL * mSumR;
        const double denomL = n * mSumL2 - mSumL * mSumL;
        const double denomR = n * mSumR2 - mSumR * mSumR;
        const double denom = std::sqrt(denomL * denomR);

        if (denom > 1e-10)
          mBuffer.vals[0].correlation = static_cast<float>(numerator / denom);
        else
          mBuffer.vals[0].correlation = 0.0f;

        mBuffer.ctrlTag = ctrlTag;
        mBuffer.vals[0].nPoints = mPointCount;

        TSender::PushData(mBuffer);

        // Reset accumulators
        mSampleCount = 0;
        mPointCount = 0;
        mSumL = mSumR = mSumL2 = mSumR2 = mSumLR = 0.0;
      }
    }
  }

private:
  ISenderData<1, TDataPacket> mBuffer;
  double mSampleRate = DEFAULT_SAMPLE_RATE;
  float mWindowSizeMs = 50.0f;
  int mWindowSize = 2205;
  int mDecimation = 4;
  int mSampleCount = 0;
  int mPointCount = 0;

  // Running sums for correlation calculation
  double mSumL = 0.0;
  double mSumR = 0.0;
  double mSumL2 = 0.0;
  double mSumR2 = 0.0;
  double mSumLR = 0.0;
};

END_IPLUG_NAMESPACE
