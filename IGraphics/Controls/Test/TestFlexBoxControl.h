/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestFlexBoxControl
 */

#include "IControl.h"
#include "IGraphicsFlexBox.h"

/** Control to test IGraphicsFlexBox
 *   @ingroup TestControls */
class TestFlexBoxControl : public IControl
{
public:
  TestFlexBoxControl(const IRECT& rect)
  : IControl(rect)
  {
    mText = IText(70.f, EVAlign::Middle);
    SetTooltip("TestFlexboxControl");
  }
  
  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_WHITE, mRootRect);
    
    WDL_String str;
    for (int i=0; i<7; i++)
    {
      g.FillRect(GetRainbow(i), mItemRects[i]);
      g.DrawRect(COLOR_BLACK, mItemRects[i]);
      str.SetFormatted(2, "%i", i);
      g.DrawText(mText, str.Get(), mItemRects[i]);
    }
  }
  
  void OnResize() override
  {
    DoLayout();
  }
  
  void DoLayout()
  {
    mItemRects.clear();
    
    for (int i=0; i<7; i++)
    {
      mItemRects.push_back(IRECT());
    }
    
    IFlexBox f;
    f.Init(mRECT, YGFlexDirectionColumn);

    YGNodeRef r;
    
    for (int i=0; i<7; i++)
    {
      r = f.AddItem(100.f, 100.f);
    }

    f.CalcLayout();

    for (int i=0; i<7; i++)
    {
      mItemRects[i] = mRECT.Inset(f.GetItemBounds(i));
    }

    mRootRect = mRECT.Inset(f.GetRootBounds());
  }

private:
  IRECT mRootRect;
  std::vector<IRECT> mItemRects;
};
