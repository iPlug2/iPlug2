/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

/**
 * @file
 * @copydoc TestMPSControl
 */

#include "IControl.h"
#include "IGraphicsNanoVG.h"

/** Control to test IGraphicsNanoVG with Metal Performance Shaders
 *   @ingroup TestControls */
class TestMPSControl : public IKnobControlBase
                     , public IBitmapBase
{
public:
  TestMPSControl(IRECT bounds, const IBitmap& bitmap)
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
      GetUI()->CreatePopupMenu(mMenu, x, y, this);
    
    SetDirty(false);
  }
  
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu) override
  {
    if(pSelectedMenu)
      mKernelType = pSelectedMenu->GetChosenItemIdx();
  }
  
private:
  int mKernelType = 0;
  NVGframebuffer* mFBO = nullptr;
  IPopupMenu mMenu {0, false, {"MPSImageGaussianBlur", "MPSImageSobel", "MPSImageThresholdToZero"}};
};
