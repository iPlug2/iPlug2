/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestDropShadowControl
 */

#include "IControl.h"
#include "nanosvg.h"

/** Control to test layer drop shadows
 *   @ingroup TestControls */
class TestDropShadowControl : public IControl
{
public:
  TestDropShadowControl(const IRECT& bounds, const ISVG& svg)
  : IControl(bounds)
  , mSVG(svg)
  {
    SetTooltip("TestDropShadowControl");
  }

  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);

    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(this, mRECT);
      
      g.DrawSVG(mSVG, mRECT);

      mLayer = g.EndLayer();
      IShadow shadow(COLOR_BLACK, 10.0, 5.0, 10.0, 0.7f, true);
      g.ApplyLayerDropShadow(mLayer, shadow);
    }

    g.DrawLayer(mLayer);
  }

  void OnDrop(const char* str) override
  {
    SetSVG(GetUI()->LoadSVG(str));
    SetDirty(false);
  }

  void SetSVG(const ISVG& svg)
  {
    mSVG = svg;
    mLayer->Invalidate();
  }

private:
  ILayerPtr mLayer;
  ISVG mSVG;
};
