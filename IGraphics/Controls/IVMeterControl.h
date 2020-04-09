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
#include "ISender.h"
#include "IPlugStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Vectorial multi-channel capable meter control
 * @ingroup IControls */
template <int MAXNC = 1>
class IVMeterControl : public IVTrackControlBase
{
public:
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
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      IByteStream stream(pData, dataSize);

      int pos = 0;
      ISenderData<MAXNC> d;
      pos = stream.Get(&d, pos);

      for (auto c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
      {
        SetValue(Clip(d.vals[c], 0.f, 1.f), c);
      }

      SetDirty(false);
    }
  }
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
