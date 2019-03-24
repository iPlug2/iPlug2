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
    SetTooltip("TestColorControl");
    
    SetActionFunction([&](IControl* pCaller) {
                        mLayer->Invalidate();
    });
  }
  
  void Draw(IGraphics& g) override;

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    mKernelType++;
    mKernelType %= 3;
    mLayer->Invalidate();
    SetDirty(false);
  }
  
private:
  int mKernelType = 0;
  ILayerPtr mLayer;
};
