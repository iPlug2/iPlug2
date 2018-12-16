/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"

class TestDirBrowseControl : public IDirBrowseControlBase
{
public:
  TestDirBrowseControl(IGEditorDelegate& dlg, IRECT rect, IActionFunction actionFunc, const IText& text,
           const char* extension)
  : IDirBrowseControlBase(dlg, rect, extension)
  {
    mText = text;
    mLabel.SetFormatted(32, "%s \n%s File", "Select a", extension);
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_BLUE, mRECT);
    g.DrawText(mText, mLabel.Get(), mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    IPopupMenu* menu = GetUI()->CreatePopupMenu(mMainMenu, mRECT);

    if(menu)
    {
      IPopupMenu::Item* item = menu->GetItem(menu->GetChosenItemIdx());
      mSelectedIndex = item->GetTag();
      mSelectedMenu = menu; // TODO: what if this is a submenu do we end up with pointer to an invalid object?
      mLabel.Set(item->GetText());
    }

    SetDirty();
  }
  
  void SetPath(const char* path)
  {
    AddPath(path, "");
    SetUpMenu();
  }

private:
  WDL_String mLabel;
};
