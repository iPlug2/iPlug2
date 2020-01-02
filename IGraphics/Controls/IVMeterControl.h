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
 * @copydoc IVMeterControl
 */

#include "IControl.h"
#include "IPlugQueue.h"
#include "IPlugStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Vectorial multichannel capable meter control
 * @ingroup IControls */
template <int MAXNC = 1, int QUEUE_SIZE = 1024>
class IVMeterControl : public IVTrackControlBase
{
public:
  static constexpr int kUpdateMessage = 0;

  /** Data packet */
  struct Data
  {
    int nchans = MAXNC;
    float vals[MAXNC] = {};

    bool AboveThreshold()
    {
      static const float threshold = (float) DBToAmp(-90.);

      float sum = 0.f;

      for(int i = 0; i < MAXNC; i++)
      {
        sum += vals[i];
      }

      return std::abs(sum) > threshold;
    }
  };

  /** Used on the DSP side in order to queue sample values and transfer data to low priority thread. */
  class Sender
  {
  public:
    Sender(int ctrlTag)
    : mCtrlTag(ctrlTag)
    {
    }

    void ProcessBlock(sample** inputs, int nFrames)
    {
      Data d;

      for (auto s = 0; s < nFrames; s++)
      {
        for (auto c = 0; c < MAXNC; c++)
        {
          d.vals[c] += std::fabs((float) inputs[c][s]);
        }
      }

      for (auto c = 0; c < MAXNC; c++)
      {
        d.vals[c] /= (float) nFrames;
      }

      if(mPrevAboveThreshold)
        mQueue.Push(d); // TODO: expensive?

      mPrevAboveThreshold = d.AboveThreshold();
    }

    void ProcessData(Data d)
    {
      mQueue.Push(d);
    }

    // this must be called on the main thread - typically in MyPlugin::OnIdle()
    void TransmitData(IEditorDelegate& dlg)
    {
      while(mQueue.ElementsAvailable())
      {
        Data d;
        mQueue.Pop(d);
        dlg.SendControlMsgFromDelegate(mCtrlTag, kUpdateMessage, sizeof(Data), (void*) &d);
      }
    }

  private:
    int mCtrlTag;
    bool mPrevAboveThreshold = true;
    IPlugQueue<Data> mQueue {QUEUE_SIZE};
  };

  IVMeterControl(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Vertical, const char* trackNames = 0, ...)
  : IVTrackControlBase(bounds, label, style, MAXNC, dir, 0, 1., trackNames)
  {
  }

  void Draw(IGraphics& g) override
  {
    DrawBackGround(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);

    if(mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, nullptr, mStyle.frameThickness);
  }

  //  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;
  //  void OnMouseDown(float x, float y, const IMouseMod& mod) override;

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    IByteStream stream(pData, dataSize);

    int pos = 0;
    Data data;
    pos = stream.Get(&data.nchans, pos);

    while(pos < stream.Size())
    {
      for (auto i = 0; i < data.nchans; i++) {
        pos = stream.Get(&data.vals[i], pos);
        SetValue(Clip(data.vals[i], 0.f, 1.f), i);
      }
    }

    SetDirty(false);
  }
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
