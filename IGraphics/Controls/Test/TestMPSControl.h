/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

#include "IControl.h"
#if defined IGRAPHICS_NANOVG && defined IGRAPHICS_METAL

/**
 * @file
 * @copydoc TestMPSControl
 */

#include "IGraphicsNanoVG.h"

using namespace iplug;
using namespace igraphics;

/** Control to test IGraphicsNanoVG with Metal Performance Shaders
 *   @ingroup TestControls */
class TestMPSControl : public IKnobControlBase
                     , public IBitmapBase
{
public:
  TestMPSControl(const IRECT& bounds, const IBitmap& bitmap)
  : IKnobControlBase(bounds)
  , IBitmapBase(bitmap)
  {
    SetTooltip("TestMPSControl");
  }
  
  ~TestMPSControl()
  {
    if (mFBO)
      nvgDeleteFramebuffer(mFBO);
  }
  
  void Draw(IGraphics& g) override;

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if(mod.R)
      GetUI()->CreatePopupMenu(*this, mMenu, x, y);
    
    SetDirty(false);
  }
  
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if(pSelectedMenu)
      mKernelType = pSelectedMenu->GetChosenItemIdx();
  }
  
private:
  int mKernelType = 0;
  NVGframebuffer* mFBO = nullptr;
  IPopupMenu mMenu {"MPS Type", 0, false, {"MPSImageGaussianBlur", "MPSImageSobel", "MPSImageThresholdToZero"}};
};

#else
class TestMPSControl : public IControl
{
public:
  TestMPSControl(IRECT rect, const IBitmap& bmp)
  : IControl(rect)
  {
    SetTooltip("TestMPSControl");
  }
  
  void Draw(IGraphics& g) override
  {
    g.DrawText(mText, "UNSUPPORTED", mRECT);
  }
};
#endif
