/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestImageControl
 */

#include "IControl.h"

/** Control to test drawing bitmaps
 *   @ingroup TestControls */
class TestImageControl : public IControl
{
public:
  TestImageControl(const IRECT& bounds, const IBitmap& bmp)
  : IControl(bounds)
  , mBitmap(bmp)
  {
    SetTooltip("TestImageControl - Click or Drag 'n drop here to load a new bitmap");
  }

  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);

    if(mBitmap.IsValid())
      g.DrawFittedBitmap(mBitmap, mRECT);
    else
      g.DrawText(DEFAULT_TEXT, "Invalid Bitmap", mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    WDL_String fileName, path;

    GetUI()->PromptForFile(fileName, path, EFileAction::Open, "bmp jpg png", 
    [this](const WDL_String& fileName, const WDL_String& path) {
      if (fileName.GetLength())
        SetBitmap(fileName.Get());
    });
  }

  void OnDrop(const char* str) override
  {
    SetBitmap(str);
  }
  
  void SetBitmap(const char* str)
  {
    mBitmap = GetUI()->LoadBitmap(str);
    SetDirty(false);
  }

private:
  IBitmap mBitmap;
};
