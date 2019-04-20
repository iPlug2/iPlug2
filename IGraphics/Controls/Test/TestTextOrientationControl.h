/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestTextOrientationControl
 */

#include "IControl.h"

/** Control to test drawing text with orientation
 *   @ingroup TestControls */
class TestTextOrientationControl : public IKnobControlBase
{
  static const int size = 14;
    
public:
    TestTextOrientationControl(IRECT bounds)
  : IKnobControlBase(bounds), mCount(-1)
  {
    SetTooltip("TestTextOrientationControl");
    mDblAsSingleClick = true;
    Next();
    mValue = 0.5;
  }

  void TextRotation(IGraphics& g, const IRECT& textRECT, IText::EAlign hAlign, IText::EVAlign vAlign, double angle)
  {
    IMatrix m = IMatrix().Rotate(angle);
    
    double x0 = textRECT.L;
    double y0 = textRECT.T;
    double x1 = textRECT.R;
    double y1 = textRECT.T;
    double x2 = textRECT.R;
    double y2 = textRECT.B;
    double x3 = textRECT.L;
    double y3 = textRECT.B;
      
    m.TransformPoint(x0, y0);
    m.TransformPoint(x1, y1);
    m.TransformPoint(x2, y2);
    m.TransformPoint(x3, y3);
      
    IRECT r1(std::min(x0, x3), std::min(y0, y3), std::max(x0, x3), std::max(y0, y3));
    IRECT r2(std::min(x1, x2), std::min(y1, y2), std::max(x1, x2), std::max(y1, y2));
    IRECT rotatedBounds = r1.Union(r2);
    
    double tx, ty;
      
    switch (hAlign)
    {
      case IText::kAlignNear:     tx = mRECT.L - rotatedBounds.L;           break;
      case IText::kAlignCenter:   tx = mRECT.MW() - rotatedBounds.MW();     break;
      case IText::kAlignFar:      tx = mRECT.R - rotatedBounds.R;           break;
    }
      
    switch (vAlign)
    {
      case IText::kVAlignTop:      ty = mRECT.T - rotatedBounds.T;          break;
      case IText::kVAlignMiddle:   ty = mRECT.MH() - rotatedBounds.MH();    break;
      case IText::kVAlignBottom:   ty = mRECT.B - rotatedBounds.B;          break;
    }
      
    g.PathTransformTranslate(tx, ty);
    g.PathTransformRotate(angle);
  }
    
  void Draw(IGraphics& g) override
  {
    IRECT rect = mRECT;

    const char* str = "Some Text To Rotate";

    g.MeasureText(mText, str, rect);
    
    double angle = mValue * 360.0 - 180.0;
    
    g.FillRect(COLOR_WHITE, mRECT);
    g.PathTransformSave();
    TextRotation(g, rect, mText.mAlign, mText.mVAlign, angle);
    g.FillRect(COLOR_MID_GRAY, rect);
    g.DrawText(mText, str, mRECT);
    g.PathTransformRestore();
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mDrag = false;
  }
    
  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if (!mDrag)
    {
      Next();
      SetDirty(false);
    }
  }
    
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    mDrag = true;
    IKnobControlBase::OnMouseDrag(x, y, dX, dY, mod);
  }

  void Next()
  {
    if (++mCount > 8)
      mCount = 0;
      
    IColor c = DEFAULT_TEXT_FGCOLOR;
    const char* font = "Roboto-Regular";
    if (mCount == 0)
      mText = IText(size, c, font, IText::kAlignNear, IText::kVAlignTop);
    else if (mCount == 1)
      mText = IText(size, c, font, IText::kAlignCenter, IText::kVAlignTop);
    else if (mCount == 2)
      mText = IText(size, c, font, IText::kAlignFar, IText::kVAlignTop);
    else if (mCount == 3)
      mText = IText(size, c, font, IText::kAlignNear, IText::kVAlignMiddle);
    else if (mCount == 4)
      mText = IText(size, c, font, IText::kAlignCenter, IText::kVAlignMiddle);
    else if (mCount == 5)
      mText = IText(size, c, font, IText::kAlignFar, IText::kVAlignMiddle);
    else if (mCount == 6)
      mText = IText(size, c, font, IText::kAlignNear, IText::kVAlignBottom);
    else if (mCount == 7)
      mText = IText(size, c, font, IText::kAlignCenter, IText::kVAlignBottom);
    else
      mText = IText(size, c, font, IText::kAlignFar, IText::kVAlignBottom);
  }

private:

  bool mDrag = false;
  int mCount;
};
