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
 * @copydoc ILEDControl
 */

#include "IControl.h"
#include "IPlugQueue.h"
#include "IPlugStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Glowing LED control
 * @ingroup IControls */
class ILEDControl : public IControl
{
public:
  ILEDControl(const IRECT& bounds, float hue = 0.f)
  : IControl(bounds)
  , mHue(hue)
  {
  }

  void Draw(IGraphics& g) override
  {
    g.FillEllipse(IColor::FromHSLA(mHue, 1.f, static_cast<float>(GetValue()) + 0.01f), mRECT, nullptr);
    g.DrawEllipse(COLOR_BLACK, mRECT, nullptr, 1.f);
  }

private:
  float mHue = 0.f;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
