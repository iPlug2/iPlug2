/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestDirBrowseControl
 */

#include "IControl.h"
#include "IPlugPaths.h"

/** Control to test IDirBrowseControlBase
 *   @ingroup TestControls */
class TestDirBrowseControl : public IDirBrowseControlBase
{
public:
  TestDirBrowseControl(IRECT rect, const char* extension, const char* path)
  : IDirBrowseControlBase(rect, extension)
  {
    mLabel.SetFormatted(32, "Select a %s file", extension);
    SetPath(path);
    SetTooltip("TestDirBrowseControl");
  }
  
  void OnResize() override
  {
    but = mRECT.GetCentredInside(mRECT.W()-10.f, 20.f);
    arrow = but.GetFromRight(20.).GetPadded(-5.);
    useplat = mRECT.GetFromBottom(30).GetPadded(-5);
    useplatbut = useplat.GetFromRight(20.).GetPadded(-5.);
  }
  
  void Draw(IGraphics& g) override
  {
    #if defined OS_MAC || defined OS_IOS
    if(AppIsSandboxed())
      g.DrawText(IText(14, EVAlign::Middle), "App is sandboxed... filesystem restricted", mRECT);
    else
    #endif
    {
      g.DrawDottedRect(COLOR_BLACK, mRECT);
      g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);
      g.FillRect(COLOR_WHITE, but);
      g.DrawText(mText, mLabel.Get(), but);
      g.FillTriangle(COLOR_GRAY, arrow.L, arrow.T, arrow.R, arrow.T, arrow.MW(), arrow.B);
      
      
      g.DrawText(IText(DEFAULT_TEXT_SIZE, EAlign::Near), "Use platform menu", useplat);
      
      g.DrawRect(COLOR_BLACK, useplatbut);

      if(mUsePlatform)
        g.FillRect(COLOR_BLACK, useplatbut.GetPadded(-2));
    }
  }
  
  void OnPopupMenuSelection(IPopupMenu* pMenu, int valIdx) override
  {
    if(pMenu)
    {
      IPopupMenu::Item* pItem = pMenu->GetChosenItem();
      
      if(pItem)
      {
        mSelectedIndex = pItem->GetTag();
        mSelectedMenu = pMenu; // TODO: what if this is a submenu do we end up with pointer to an invalid object?
        mLabel.Set(pItem->GetText());
      }
    }
    
    SetDirty(false);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if(but.Contains(x, y))
    {
      GetUI()->CreatePopupMenu(*this, mMainMenu, x, y);
    }
    else if(useplatbut.Contains(x, y))
    {
      mUsePlatform = !mUsePlatform;
      
      if(!mUsePlatform)
        GetUI()->AttachPopupMenuControl();
    }
    
    SetDirty(false);
  }

  void SetPath(const char* path)
  {
    AddPath(path, "");
    SetupMenu();
  }

private:
  WDL_String mLabel;
  bool mUsePlatform = true;
  IRECT but;
  IRECT arrow;
  IRECT useplat;
  IRECT useplatbut;
};
