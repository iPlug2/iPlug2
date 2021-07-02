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

#include "IPlugPlatform.h"
#include "IPlugQueue.h"
#include <array>

BEGIN_IPLUG_NAMESPACE

static const float SENDER_THRESHOLD = (float) DBToAmp(-90.);

/** ISenderData is used to represent a typed data packet, that may contain values for multiple channels */
template <int MAXNC = 1, typename T = float>
struct ISenderData
{
  int ctrlTag = kNoTag;
  int nChans = MAXNC;
  int chanOffset = 0;
  std::array<T, MAXNC> vals {0};
  
  ISenderData() {}
  
  ISenderData(int ctrlTag, int nChans, int chanOffset)
  : ctrlTag(ctrlTag)
  , nChans(nChans)
  , chanOffset(chanOffset)
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

  /** Pops elements off the queue and sends messages to controls.
   *  This must be called on the main thread - typically in MyPlugin::OnIdle() */
  void TransmitData(IEditorDelegate& dlg)
  {
    while(mQueue.ElementsAvailable())
    {
      ISenderData<MAXNC, T> d;
      mQueue.Pop(d);
      dlg.SendControlMsgFromDelegate(d.ctrlTag, kUpdateMessage, sizeof(ISenderData<MAXNC, T>), (void*) &d);
    }
  }

private:
  IPlugQueue<ISenderData<MAXNC, T>> mQueue {QUEUE_SIZE};
};

/** IPeakSender is a utility class which can be used to defer peak data from sample buffers for sending to the GUI */
template <int MAXNC = 1, int QUEUE_SIZE = 64>
class IPeakSender : public ISender<MAXNC, QUEUE_SIZE, float>
{
public:
  /** Queue peaks from sample buffers into the sender, checking the data is over the required threshold. This can be called on the realtime audio thread. */
  void ProcessBlock(sample** inputs, int nFrames, int ctrlTag, int nChans = MAXNC, int chanOffset = 0)
  {
    ISenderData<MAXNC, float> d {ctrlTag, nChans, chanOffset};

    for (auto s = 0; s < nFrames; s++)
    {
      for (auto c = chanOffset; c < (chanOffset + nChans); c++)
      {
        d.vals[c] += std::fabs((float) inputs[c][s]);
      }
    }
    
    float sum = 0.;

    for (auto c = chanOffset; c < (chanOffset + nChans); c++)
    {
      d.vals[c] /= (float) nFrames;
      sum += d.vals[c];
    }

    if(sum > SENDER_THRESHOLD || mPreviousSum > SENDER_THRESHOLD)
      ISender<MAXNC, QUEUE_SIZE, float>::PushData(d);

    mPreviousSum = sum;
  }
private:
  float mPreviousSum = 1.f;
};

/** IBufferSender is a utility class which can be used to defer buffer data for sending to the GUI */
template <int MAXNC = 1, int QUEUE_SIZE = 64, int MAXBUF = 128>
class IBufferSender : public ISender<MAXNC, QUEUE_SIZE, std::array<float, MAXBUF>>
{
public:

  /** Queue sample buffers into the sender, checking the data is over the required threshold. This can be called on the realtime audio thread. */
  void ProcessBlock(sample** inputs, int nFrames, int ctrlTag, int nChans = MAXNC, int chanOffset = 0)
  {
    for (auto s = 0; s < nFrames; s++)
    {
      if(mBufCount == MAXBUF)
      {
        float sum = 0.f;
        for (auto c = chanOffset; c < (chanOffset + nChans); c++)
        {
          sum += mRunningSum[c];
          mRunningSum[c] = 0.f;
        }

        if (sum > SENDER_THRESHOLD || mPreviousSum > SENDER_THRESHOLD)
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
        mBuffer.vals[c][mBufCount] = (float) inputs[c][s];
        mRunningSum[c] += std::fabs( (float) inputs[c][s]);
      }

      mBufCount++;
    }
  }
protected:
  ISenderData<MAXNC, std::array<float, MAXBUF>> mBuffer;
  int mBufCount = 0;
  std::array<float, MAXNC> mRunningSum {0.};
  float mPreviousSum = 1.f;
};

END_IPLUG_NAMESPACE
