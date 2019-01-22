/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
 */

#pragma once

/**
 * @file
 * @copydoc ICornerResizerControl
 */

#include "IControl.h"

/** A control for resizing the plug-in window by clicking and dragging in the bottom right-hand corner
 * This can be added with IGraphics::AttachCornerResizer().
 * @ingroup SpecialControls */
class ICornerResizerControl : public IControl
{
public:
  ICornerResizerControl(IRECT graphicsBounds, float size)
  : IControl(graphicsBounds.GetFromBRHC(size, size).GetPadded(-1))
  , mInitialGraphicsBounds(graphicsBounds)
  , mSize(size)
  {
  }

  void Draw(IGraphics& g) override
  {
    if(GetMouseIsOver() || GetUI()->mResizingInProcess)
      g.FillTriangle(COLOR_BLACK, mRECT.L, mRECT.B, mRECT.R, mRECT.T, mRECT.R, mRECT.B);
    else
      g.FillTriangle(COLOR_TRANSLUCENT, mRECT.L, mRECT.B, mRECT.R, mRECT.T, mRECT.R, mRECT.B);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if(mod.S || mod.R)
      GetUI()->Resize((int) mInitialGraphicsBounds.W(), (int) mInitialGraphicsBounds.H(), 1.f);
    else
      GetUI()->StartResizeGesture();
  }

  void OnRescale() override
  {
    float size = mSize * (1.f/GetUI()->GetDrawScale());
    IRECT r = GetUI()->GetBounds().GetFromBRHC(size, size);
    SetTargetAndDrawRECTs(r);
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    GetUI()->SetMouseCursor(ECursor::SIZENWSE);
    IControl::OnMouseOver(x, y, mod);
  }

  void OnMouseOut() override
  {
    GetUI()->SetMouseCursor(ECursor::ARROW);
    IControl::OnMouseOut();
  }

private:
  float mSize;
  IRECT mInitialGraphicsBounds;
};
