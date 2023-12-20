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
    mButtonRect = mRECT.GetCentredInside(mRECT.W()-10.f, 20.f);
    mArrowRect = mButtonRect.GetFromRight(20.).GetPadded(-5.);
    mUsePlatformMenuTextRect = mRECT.GetFromBottom(30).GetPadded(-5);
    mUsePlatformMenuButtonRect = mUsePlatformMenuTextRect.GetFromRight(20.).GetPadded(-5.);
  }
  
  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);
    g.FillRect(COLOR_WHITE, mButtonRect);
    g.DrawText(mText, mLabel.Get(), mButtonRect);
    g.FillTriangle(COLOR_GRAY, mArrowRect.L, mArrowRect.T, mArrowRect.R, mArrowRect.T, mArrowRect.MW(), mArrowRect.B);
    g.DrawText(IText(DEFAULT_TEXT_SIZE, EAlign::Near), "Use platform menu", mUsePlatformMenuTextRect);
    g.DrawRect(COLOR_BLACK, mUsePlatformMenuButtonRect);

    if (mUsePlatformMenu)
      g.FillRect(COLOR_BLACK, mUsePlatformMenuButtonRect.GetPadded(-2));
  }
  
  void OnPopupMenuSelection(IPopupMenu* pMenu, int valIdx) override
  {
    if (pMenu)
    {
      IPopupMenu::Item* pItem = pMenu->GetChosenItem();
      
      if (pItem)
      {
        mSelectedItemIndex = mItems.Find(pItem);
        mLabel.Set(pItem->GetText());
        CheckSelectedItem();
      }
    }
    
    SetDirty(false);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mButtonRect.Contains(x, y))
    {
      GetUI()->CreatePopupMenu(*this, mMainMenu, x, y);
    }
    else if (mUsePlatformMenuButtonRect.Contains(x, y))
    {
      mUsePlatformMenu = !mUsePlatformMenu;
      
      if (!mUsePlatformMenu)
        GetUI()->AttachPopupMenuControl();
      else
        GetUI()->RemovePopupMenuControl();
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
  bool mUsePlatformMenu = true;
  IRECT mButtonRect;
  IRECT mArrowRect;
  IRECT mUsePlatformMenuTextRect;
  IRECT mUsePlatformMenuButtonRect;
};
