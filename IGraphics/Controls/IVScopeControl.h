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
#include "ISender.h"
#include "IPlugStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Vectorial multi-channel capable oscilloscope control
 * @ingroup IControls */
template <int MAXNC = 1, int MAXBUF = 128>
class IVScopeControl : public IControl
                     , public IVectorBase
{
public:
  /** Constructs an IVScopeControl 
   * @param bounds The rectangular area that the control occupies
   * @param label A CString to label the control
   * @param style, /see IVStyle */
  IVScopeControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE)
  : IControl(bounds)
  , IVectorBase(style)
  {
    AttachIControl(this, label);
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    
    if (mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  void DrawWidget(IGraphics& g) override
  {
    g.DrawHorizontalLine(GetColor(kSH), mWidgetBounds, 0.5, &mBlend, mStyle.frameThickness);
    
    IRECT r = mWidgetBounds.GetPadded(-mPadding);

    for (int c=0; c<mBuf.nChans; c++)
    {
      // drawdata expects normalized values and buffer contains unnormalized, so draw in the top half
      g.DrawData(GetColor(kFG), r.FracRectVertical(0.5, true), mBuf.vals[c].data(), mBufferSize, nullptr, &mBlend, mTrackSize);
    }
  }
  
  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    SetDirty(false);
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      IByteStream stream(pData, dataSize);

      int pos = 0;
      pos = stream.Get(&mBuf, pos);

      SetDirty(false);
    }
  }
  
  void SetBufferSize(int bufferSize)
  {
    assert(bufferSize > 0);
    assert(bufferSize <= MAXBUF);
    mBufferSize = bufferSize;
  }

private:
  ISenderData<MAXNC, std::array<float, MAXBUF>> mBuf;
  float mPadding = 2.f;
  int mBufferSize = MAXBUF;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

