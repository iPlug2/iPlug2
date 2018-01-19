#pragma once

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cassert>

#include "wdlstring.h"
#include "ptrlist.h"

/**
 * @file
 * @copydoc IPopupMenu
 * @ingroup IGraphicsStructs
 */

// this (and the platform implementation in IGraphics*) is largely based on the VSTGUI COptionMenu

class IPopupMenu;

/** A class to specify an item of a pop up menu */
class IPopupMenuItem
{
public:
  enum Flags
  {
    kNoFlags  = 0,
    kDisabled = 1 << 0,     // item is gray and not selectable
    kTitle    = 1 << 1,     // item indicates a title and is not selectable
    kChecked  = 1 << 2,     // item has a checkmark
    kSeparator  = 1 << 3    // item is a separator
  };

  IPopupMenuItem(const char* text, int flags = kNoFlags)
    : mFlags(flags)
  {
    SetText(text);
  }

  IPopupMenuItem (const char* text, IPopupMenu *pSubMenu)
    : mFlags(kNoFlags)
    , mSubmenu(pSubMenu)
  {
    SetText(text);
  }

  ~IPopupMenuItem()
  {
    //don't need to delete submenu
  }

  void SetText(const char* text) { mText.Set(text); }
  const char* GetText() { return mText.Get(); };

  bool GetEnabled() const { return !(mFlags & kDisabled); }
  bool GetChecked() const { return (mFlags & kChecked) != 0; }
  bool GetIsTitle() const { return (mFlags & kTitle) != 0; }
  bool GetIsSeparator() const { return (mFlags & kSeparator) != 0; }

  IPopupMenu* GetSubmenu() const { return mSubmenu; }

  void SetChecked(bool state)
  {
    if (state)
    {
      mFlags |= kChecked;
    }
    else
    {
      mFlags &= ~kChecked;
    }
  }

protected:
  WDL_String mText;
  IPopupMenu* mSubmenu = nullptr;
  int mFlags;
};

/** A class for setting the contents of a pop up menu */
class IPopupMenu
{
public:

  IPopupMenu(int prefix = 0, bool multicheck = false)
  : mPrefix(prefix)
  , mCanMultiCheck(multicheck)
  {}

  ~IPopupMenu()
  {
    mMenuItems.Empty(true);
  }

  IPopupMenuItem* AddItem(IPopupMenuItem* pItem, int index = -1)
  {
    if (index == -1)
    {
      mMenuItems.Add(pItem); // add it to the end
    }
    else
    {
      mMenuItems.Insert(index, pItem);
    }
    
    return pItem;
  }
  
  IPopupMenuItem* AddItem(const char* text, int index = -1, int itemFlags = IPopupMenuItem::kNoFlags)
  {
    return AddItem(new IPopupMenuItem(text, itemFlags), index);
  }
  
  IPopupMenuItem* AddItem(const char* text, int index, IPopupMenu* pSubmenu)
  {
    return AddItem(new IPopupMenuItem(text, pSubmenu), index);
  }
  
  IPopupMenuItem* AddItem(const char* text, IPopupMenu* pSubmenu)
  {
    return AddItem(new IPopupMenuItem(text, pSubmenu), -1);
  }
  
  IPopupMenuItem* AddSeparator(int index = -1)
  {
    IPopupMenuItem* pItem = new IPopupMenuItem ("", IPopupMenuItem::kSeparator);
    return AddItem(pItem, index);
  }

  void SetChosenItemIdx(int index) { mChosenItemIdx = index; };
  int GetChosenItemIdx() { return mChosenItemIdx; }
  int GetNItems() { return mMenuItems.GetSize(); }
  int GetPrefix() { return mPrefix; }
  bool GetCanMultiCheck() { return mCanMultiCheck; }

  IPopupMenuItem* GetItem(int index)
  {
    int nItems = GetNItems();
    
    if (index >= 0 && index < nItems)
    {
      return mMenuItems.Get(index);
    }
    else
    {
      return nullptr;
    }
  }
  
  const char* GetItemText(int index)
  {
    return GetItem(index)->GetText();
  }
  
  void SetPrefix(int count)
  {
    if (count >= 0 && count < 4)
    {
      mPrefix = count;
    }
  }
  
  void SetMultiCheck(bool multicheck) { mCanMultiCheck = multicheck; }
  
  bool CheckItem(int index, bool state)
  {
    IPopupMenuItem* pItem = mMenuItems.Get(index);
    
    if (pItem)
    {
      pItem->SetChecked(state);
      return true;
    }
    return false;
  }
  
  void CheckItemAlone(int index)
  {
    for (int i = 0; i < mMenuItems.GetSize(); i++)
    {
      mMenuItems.Get(i)->SetChecked(i == index);
    }
  }
  
  bool IsItemChecked(int index)
  {
    IPopupMenuItem* item = mMenuItems.Get(index);
    
    if (item)
    {
      return item->GetChecked();
    }
    
    return false;
  }

private:
  int mPrefix; // 0 = no prefix, 1 = numbers no leading zeros, 2 = 1 lz, 3 = 2lz
  int mChosenItemIdx = -1;
  bool mCanMultiCheck; // multicheck = 0 doesn't actually prohibit multichecking, you should do that in your code, by calling CheckItemAlone instead of CheckItem
  WDL_PtrList<IPopupMenuItem> mMenuItems;
};
