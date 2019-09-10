/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestSVGControl
 */

#include "IControl.h"
#include "nanosvg.h"

/** Control to test drawing SVGs
 *   @ingroup TestControls */
class TestSVGControl : public IControl
{
public:
  TestSVGControl(const IRECT& bounds, const ISVG& svg, bool hq = false)
  : IControl(bounds)
  , mSVG(svg)
  , mHQ(hq)
  {
    SetTooltip("TestSVGControl - Click or Drag 'n drop here to load a new SVG.");
  }

  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);

    if(mSVG.IsValid())
    {
#if 1
      if (!g.CheckLayer(mLayer))
      {
        auto layerBitmap = g.StartLayer(this, mRECT);
        
        if(mHQ)
          g.RasterizeSVGToLayer(mSVG, layerBitmap);
        else
          g.DrawSVG(mSVG, mRECT);
        
        mLayer = g.EndLayer();
      }
      g.DrawLayer(mLayer);
#else
      g.DrawSVG(mSVG, mRECT);
#endif
    }
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    WDL_String file;
    WDL_String path;

    GetUI()->PromptForFile(file, path, EFileAction::Open, "svg");

    if(file.GetLength())
      LoadSVG(file.Get());
  }

  void OnDrop(const char* str) override
  {
    LoadSVG(str);
  }

  void LoadSVG(const char* str)
  {
    ISVG svg = GetUI()->LoadSVG(str);

    if(svg.IsValid())
    {
      mSVG = svg;
      if(mLayer)
        mLayer->Invalidate();
      SetDirty(false);
    }
  }

private:
  ILayerPtr mLayer;
  ISVG mSVG;
  bool mHQ = false;
};
