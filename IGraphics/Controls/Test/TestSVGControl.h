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
  TestSVGControl(const IRECT& bounds, const ISVG& svg)
  : IControl(bounds)
  , mSVG(svg)
  {
    SetTooltip("TestSVGControl - Click or Drag 'n drop here to load a new SVG.");
  }

  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);

#if 1
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(this, mRECT);
      g.DrawSVG(mSVG, mRECT);
      mLayer = g.EndLayer();
    }

    g.DrawLayer(mLayer);
#else
    g.DrawSVG(mSVG, mRECT);
#endif
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    WDL_String file;
    WDL_String path;

    GetUI()->PromptForFile(file, path, EFileAction::Open, "svg");

    if(file.GetLength())
      SetSVG(GetUI()->LoadSVG(file.Get()));

    SetDirty(false);
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
