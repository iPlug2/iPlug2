/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"
#include "nanosvg.h"

class TestSVGControl : public IControl
{
public:
  TestSVGControl(IGEditorDelegate& dlg, IRECT bounds, const ISVG& svg)
  : IControl(dlg, bounds)
  , mSVG(svg)
  {
    SetTooltip("Click or Drag 'n drop here to load a new SVG.");
  }
  
  void Draw(IGraphics& g) override
  {
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);
    
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(mRECT);
      g.DrawSVG(mSVG, mRECT);
      mLayer = g.EndLayer();
    }

    g.DrawLayer(mLayer);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    WDL_String file;
    WDL_String path;

    GetUI()->PromptForFile(file, path, kFileOpen, "svg");
    
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
