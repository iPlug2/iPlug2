/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestCursorControl
 */

#include "IControl.h"

/** Control to test changing the platform cursor
 *   @ingroup TestControls */
class TestCursorControl : public IControl
{
public:
  TestCursorControl(IRECT rect, int paramIdx = kNoParameter)
  : IControl(rect, paramIdx)
  {
    SetTooltip("TestCursorControl");
  }

  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);
    g.DrawText(mText, GetCursorStr(mCursor), mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mCursor++;
    
    if (mCursor > (int) ECursor::HELP)
      mCursor = -1;
    
    GetUI()->SetMouseCursor((ECursor) mCursor);
    
    SetDirty(false);
  }

  void OnMouseOut() override
  {
    mCursor = -1;
    GetUI()->SetMouseCursor(ARROW);
    
    IControl::OnMouseOut();
  }
  
private:
  const char* GetCursorStr(int cursor)
  {
    switch (cursor)
    {
      case -1:           return "Click to set cursor";
      case ARROW:        return "arrow";
      case IBEAM:        return "ibeam";
      case WAIT:         return "wait";
      case CROSS:        return "cross";
      case UPARROW:      return "up arrow";
      case SIZENWSE:     return "size NW-SE";
      case SIZENESW:     return "size NE-SW";
      case SIZEWE:       return "size WE";
      case SIZENS:       return "size NS";
      case SIZEALL:      return "size all";
      case INO:          return "no";
      case HAND:         return "hand";
      case APPSTARTING:  return "app starting";
      case HELP:         return "help";
    }
    
    return "";
  }
  
  int mCursor = -1;
};
