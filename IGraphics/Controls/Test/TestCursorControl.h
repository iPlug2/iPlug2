/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"

class TestCursorControl : public IControl
{
public:
  TestCursorControl(IGEditorDelegate& dlg, IRECT rect, int paramIdx = kNoParameter)
  : IControl(dlg, rect, paramIdx)
  {
    SetTooltip("TestCursorControl");
  }

  void Draw(IGraphics& g) override
  {
    const char* str = "Cursor";
      
    switch ((mCursor - 1) % (HELP + 1))
    {
      case ARROW:         str = "arrow";            break;
      case IBEAM:         str = "ibeam";            break;
      case WAIT:          str = "wait";             break;
      case CROSS:         str = "cross";            break;
      case UPARROW:       str = "up arrow";         break;
      case SIZENWSE:      str = "size NW-SE";       break;
      case SIZENESW:      str = "size NE-SW";       break;
      case SIZEWE:        str = "size WE";          break;
      case SIZENS:        str = "size NS";          break;
      case SIZEALL:       str = "size all";         break;
      case INO:           str = "no";               break;
      case HAND:          str = "hand";             break;
      case APPSTARTING:   str = "app starting";     break;
      case HELP:          str = "help";             break;
    }
      
    g.DrawText(mText, str, mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    GetUI()->SetMouseCursor((ECursor) mCursor++);
    if (mCursor > (int) ECursor::HELP)
      mCursor = 0;
    SetDirty(false);
  }

private:

int mCursor = 1;
};
