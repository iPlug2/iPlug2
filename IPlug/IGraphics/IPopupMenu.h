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

/** A class for setting the contents of a pop up menu */
class IPopupMenu
{
public:
  /** A class to specify an item of a pop up menu */
#pragma mark - IPopupMenu::Item
  class Item
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
    
    Item(const char* str, int flags = kNoFlags, int tag = -1)
    : mFlags(flags)
    , mTag(tag)
    {
      SetText(str);
    }
    
    Item (const char* str, IPopupMenu* pSubMenu)
    : mFlags(kNoFlags)
    , mSubmenu(pSubMenu)
    {
      SetText(str);
    }
    
    ~Item()
    {
      if (mSubmenu)
        DELETE_NULL(mSubmenu);
    }
    
    void SetText(const char* str) { mText.Set(str); }
    const char* GetText() { return mText.Get(); };
    
    bool GetEnabled() const { return !(mFlags & kDisabled); }
    bool GetChecked() const { return (mFlags & kChecked) != 0; }
    bool GetIsTitle() const { return (mFlags & kTitle) != 0; }
    bool GetIsSeparator() const { return (mFlags & kSeparator) != 0; }
    int GetTag() const { return mTag; }
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
    int mTag = -1;
  };
  
  #pragma mark -
  
  IPopupMenu(int prefix = 0, bool multicheck = false)
  : mPrefix(prefix)
  , mCanMultiCheck(multicheck)
  {}

  ~IPopupMenu()
  {
    mMenuItems.Empty(true);
  }

  Item* AddItem(Item* pItem, int index = -1)
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
  
  Item* AddItem(const char* str, int index = -1, int itemFlags = Item::kNoFlags) { return AddItem(new Item(str, itemFlags), index); }
  Item* AddItem(const char* str, int index, IPopupMenu* pSubmenu) { return AddItem(new Item(str, pSubmenu), index); }
  Item* AddItem(const char* str, IPopupMenu* pSubmenu) { return AddItem(new Item(str, pSubmenu), -1); }
  
  Item* AddSeparator(int index = -1)
  {
    Item* pItem = new Item ("", Item::kSeparator);
    return AddItem(pItem, index);
  }

  void SetChosenItemIdx(int index) { mChosenItemIdx = index; };
  int GetChosenItemIdx() { return mChosenItemIdx; }
  int GetNItems() { return mMenuItems.GetSize(); }
  int GetPrefix() { return mPrefix; }
  bool GetCanMultiCheck() { return mCanMultiCheck; }

  Item* GetItem(int index)
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
  
  const char* GetItemText(int index) return GetItem(index)->GetText(); }
  
  void SetPrefix(int count)
  {
    if (count >= 0 && count < 4)
    {
      mPrefix = count;
    }
  }
  
  void SetMultiCheck(bool multicheck) { mCanMultiCheck = multicheck; }
  
  void Clear() { mMenuItems.Empty(true); }

  bool CheckItem(int index, bool state)
  {
    Item* pItem = mMenuItems.Get(index);
    
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
    Item* item = mMenuItems.Get(index);
    
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
  WDL_PtrList<Item> mMenuItems;
};
