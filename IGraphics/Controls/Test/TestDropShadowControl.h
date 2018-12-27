/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"
#include "nanosvg.h"

class TestDropShadowControl : public IControl
{
public:
  TestDropShadowControl(IGEditorDelegate& dlg, IRECT bounds, const ISVG& svg)
  : IControl(dlg, bounds)
  , mSVG(svg)
  {
    SetTooltip("TestDropShadowControl - Drag 'n drop here to load a new SVG.");
  }

  void Draw(IGraphics& g) override
  {
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(mRECT);
      g.DrawSVG(mSVG, mRECT);
      mLayer = g.EndLayer();
      IPattern pattern = IPattern::CreateRadialGradient(mRECT.MW(), mRECT.MH(), mRECT.W() / 2.0);
      pattern.AddStop(COLOR_BLACK, 0.8);
      pattern.AddStop(COLOR_TRANSPARENT, 1.0);
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
