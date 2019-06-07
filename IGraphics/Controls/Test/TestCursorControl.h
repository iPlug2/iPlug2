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
    
    if (mCursor > static_cast<int>(ECursor::HELP))
      mCursor = -1;
    
    GetUI()->SetMouseCursor((ECursor) std::max(0, mCursor));
    
    SetDirty(false);
  }

  void OnMouseOut() override
  {
    mCursor = -1;
    GetUI()->SetMouseCursor(ECursor::ARROW);
    
    IControl::OnMouseOut();
  }
  
private:
  const char* GetCursorStr(int cursor)
  {
    if(cursor == -1)
      return "Click to set cursor";
    
    switch (static_cast<ECursor>(cursor))
    {
      case ECursor::ARROW:        return "arrow";
      case ECursor::IBEAM:        return "ibeam";
      case ECursor::WAIT:         return "wait";
      case ECursor::CROSS:        return "cross";
      case ECursor::UPARROW:      return "up arrow";
      case ECursor::SIZENWSE:     return "size NW-SE";
      case ECursor::SIZENESW:     return "size NE-SW";
      case ECursor::SIZEWE:       return "size WE";
      case ECursor::SIZENS:       return "size NS";
      case ECursor::SIZEALL:      return "size all";
      case ECursor::INO:          return "no";
      case ECursor::HAND:         return "hand";
      case ECursor::APPSTARTING:  return "app starting";
      case ECursor::HELP:         return "help";
    }
    
    return "";
  }
  
  int mCursor = -1;
};
