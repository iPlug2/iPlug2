/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup Controls
 * @copydoc IVScopeControl
 */

#include "IControl.h"
#include "IPlugStructs.h"
#include "IPlugQueue.h"

/** Vectorial multichannel capable oscilloscope control
 * @ingroup IControls */
template <int MAXNC = 1, int MAXBUF = 128, int QUEUE_SIZE = 1024>
class IVScopeControl : public IControl
                     , public IVectorBase
{
public:
  static constexpr int kUpdateMessage = 0;

  /* Data packet */
  struct Data
  {
    int nchans = MAXNC;
    float vals[MAXNC][MAXBUF] = {};

    bool AboveThreshold()
    {
      static const float threshold = (float) DBToAmp(-90.);

      float sum = 0.f;

      for(auto c = 0; c < MAXNC; c++)
      {
        for(auto s = 0; s < MAXBUF; s++)
        {
          sum += vals[c][s];
        }
      }

      return std::abs(sum) > threshold;
    }
  };

  /** Used on the DSP side in order to queue sample values and transfer data to low priority thread. */
  class IVScopeBallistics
  {
  public:
    IVScopeBallistics(int controlTag)
    : mControlTag(controlTag)
    {
    }

    void ProcessBlock(sample** inputs, int nFrames)
    {
      for (auto s = 0; s < nFrames; s++)
      {
        if(mBufCount == MAXBUF)
        {
          if(mPrevAboveThreshold)
            mQueue.Push(mBuf); // TODO: expensive?

          mPrevAboveThreshold = mBuf.AboveThreshold();

          mBufCount = 0;
        }

        for (auto c = 0; c < MAXNC; c++)
        {
          mBuf.vals[c][mBufCount] = (float) inputs[c][s];
        }

        mBufCount++;
      }
    }

    // this must be called on the main thread - typically in MyPlugin::OnIdle()
    void TransmitData(IEditorDelegate& dlg)
    {
      Data d;

      while(mQueue.ElementsAvailable())
      {
        mQueue.Pop(d);
        dlg.SendControlMsgFromDelegate(mControlTag, kUpdateMessage, sizeof(Data), (void*) &d);
      }
    }

  private:
    Data mBuf;
    int mControlTag;
    int mBufCount = 0;
    IPlugQueue<Data> mQueue {QUEUE_SIZE};
    bool mPrevAboveThreshold = true;
  };

  IVScopeControl(IRECT bounds, const char* trackNames = 0, ...)
  : IControl(bounds)
  {
    AttachIControl(this);
  }

  virtual void Draw(IGraphics& g) override
  {
    g.FillRect(GetColor(kBG), mRECT);

    IRECT r = mRECT.GetPadded(-mPadding);

    const float maxY = (r.H() / 2.f); // y +/- centre

    float xPerData = r.W() / (float) MAXBUF;

    for (int c = 0; c < mBuf.nchans; c++)
    {
      float xHi = 0.f;
      float yHi = mBuf.vals[c][0] * maxY;
      yHi = Clip(yHi, -maxY, maxY);

      for (int s = 1; s < MAXBUF; s++)
      {
        float xLo = xHi, yLo = yHi;
        xHi = ((float) s * xPerData);
        yHi = mBuf.vals[c][s] * maxY;
        yHi = Clip(yHi, -maxY, maxY);
        g.DrawLine(GetColor(kFG), r.L + xLo, r.MH() - yLo, r.L + xHi, r.MH() - yHi);
      }
    }
  }

  void OnMsgFromDelegate(int messageTag, int dataSize, const void* pData) override
  {
    IByteStream stream(pData, dataSize);

    int pos = stream.Get(&mBuf.nchans, 0);

    while(pos < stream.Size())
    {
      for (auto ch = 0; ch < mBuf.nchans; ch++) {
        for (auto s = 0; s < MAXBUF; s++) {
          pos = stream.Get(&mBuf.vals[ch][s], pos);
        }
      }
    }

    SetDirty(false);
  }

private:
  Data mBuf;
  float mPadding = 2.f;
};

