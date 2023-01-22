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
  {
    memset(vals.data(), 0, sizeof(vals));
  }
  
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

  /** Pops elements off the queue and sends messages to controls.
   *  This must be called on the main thread - typically in MyPlugin::OnIdle() */
  void TransmitData(IEditorDelegate& dlg)
  {
    while(mQueue.ElementsAvailable())
    {
      ISenderData<MAXNC, T> d;
      mQueue.Pop(d);
      assert(d.ctrlTag != kNoTag && "You must supply a control tag");
      dlg.SendControlMsgFromDelegate(d.ctrlTag, kUpdateMessage, sizeof(ISenderData<MAXNC, T>), (void*) &d);
    }
  }
  
  /** This variation can be used if you need to supply multiple controls with the same ISenderData, overrideing the tags in the data packet
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
  IBufferSender(double minThresholdDb = -90., int bufferSize = MAXBUF)
  : ISender<MAXNC, QUEUE_SIZE, std::array<float, MAXBUF>>()
  , mThreshold(static_cast<float>(DBToAmp(minThresholdDb)))
  {
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
          ISender<MAXNC, QUEUE_SIZE, std::array<float, MAXBUF>>::PushData(mBuffer);
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
  ISenderData<MAXNC, std::array<float, MAXBUF>> mBuffer;
  int mBufCount = 0;
  int mBufferSize = MAXBUF;
  std::array<float, MAXNC> mRunningSum {0.};
  float mPreviousSum = 1.f;
  float mThreshold = 0.01f;
};

END_IPLUG_NAMESPACE
