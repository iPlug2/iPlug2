/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include <cmath>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <memory>

#include "wdlstring.h"
#include "ptrlist.h"

/**
 * @file
 * @copydoc IPopupMenu
 * @addtogroup IGraphicsStructs
 * @{
 */

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** @brief A class for setting the contents of a pop up menu.
 *
 * An IPopupMenu must not be declared as a temporary. In order for a receiving IControl or lambda function
 * to be triggered when something is selected, the menu should persist across function calls, therefore
 * it should almost always be a member variable.
 * An IPopupMenu owns its sub items, including submenus
 * This (and the platform implementations) are largely based on the VSTGUI COptionMenu */
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
    : mSubmenu(pSubMenu)
    , mFlags(kNoFlags)
    {
      SetText(str);
    }
    
    Item(const Item&) = delete;
    void operator=(const Item&) = delete;

    ~Item()
    {
    }
    
    void SetText(const char* str) { mText.Set(str); }
    const char* GetText() const { return mText.Get(); }; // TODO: Text -> Str!
    
    bool GetEnabled() const { return !(mFlags & kDisabled); }
    bool GetChecked() const { return (mFlags & kChecked) != 0; }
    bool GetIsTitle() const { return (mFlags & kTitle) != 0; }
    bool GetIsSeparator() const { return (mFlags & kSeparator) != 0; }
    int GetTag() const { return mTag; }
    IPopupMenu* GetSubmenu() const { return mSubmenu.get(); }
    bool GetIsChoosable() const
    {
      if(GetIsTitle()) return false;
      if(GetIsSeparator()) return false;
      if(GetSubmenu() != nullptr) return false;
      if(!GetEnabled()) return false;
      
      return true;
    }
    
    void SetEnabled(bool state) { SetFlag(kDisabled, !state); }
    void SetChecked(bool state) { SetFlag(kChecked, state); }
    void SetTitle(bool state) {SetFlag(kTitle, state); }
    void SetSubmenu(IPopupMenu* pSubmenu) { mSubmenu.reset(pSubmenu); }

  protected:
    void SetFlag(Flags flag, bool state)
    {
      if (state)
        mFlags |= flag;
      else
        mFlags &= ~flag;
    }

    WDL_String mText;
    std::unique_ptr<IPopupMenu> mSubmenu;
    int mFlags;
    int mTag = -1;
  };
  
  #pragma mark -
  
  IPopupMenu(const char* rootTitle = "", int prefix = 0, bool multicheck = false, const std::initializer_list<const char*>& items = {})
  : mPrefix(prefix)
  , mCanMultiCheck(multicheck)
  , mRootTitle(rootTitle)
  {
    for (auto& item : items)
      AddItem(item);
  }
  
  IPopupMenu(const char* rootTitle, const std::initializer_list<const char*>& items, IPopupFunction func = nullptr)
  : mPrefix(0)
  , mCanMultiCheck(false)
  , mRootTitle(rootTitle)
  {
    for (auto& item : items)
      AddItem(item);
    
    SetFunction(func);
  }
  
  IPopupMenu(const IPopupMenu&) = delete;
  void operator=(const IPopupMenu&) = delete;
  
  ~IPopupMenu()
  {
    mMenuItems.Empty(true);
  }

  static int Sortfunc(const Item **a, const Item **b)
  {
    return stricmp((*a)->GetText(),(*b)->GetText());
  }
  
  Item* AddItem(Item* pItem, int index = -1)
  {
    if (index == -1)
      mMenuItems.Add(pItem); // add it to the end
    else if (index == -2)
      mMenuItems.InsertSorted(pItem, Sortfunc);
    else
      mMenuItems.Insert(index, pItem);
    
    return pItem;
  }
  
  Item* AddItem(const char* str, int index = -1, int itemFlags = Item::kNoFlags) { return AddItem(new Item(str, itemFlags), index); }
  
  Item* AddItem(const char* str, int index, IPopupMenu* pSubmenu)
  {
    assert(pSubmenu->GetFunction() == nullptr); // submenus should not have existing functions
    
    if(GetFunction())
      pSubmenu->SetFunction(GetFunction());
    
    return AddItem(new Item(str, pSubmenu), index);
  }
  
  Item* AddItem(const char* str, IPopupMenu* pSubmenu, int index = -1)
  {
    assert(pSubmenu->GetFunction() == nullptr); // submenus should not have existing functions
    
    if(GetFunction())
      pSubmenu->SetFunction(GetFunction());
    
    return AddItem(new Item(str, pSubmenu), index);
  }
  
  Item* AddSeparator(int index = -1)
  {
    Item* pItem = new Item ("", Item::kSeparator);
    return AddItem(pItem, index);
  }
  
  void RemoveEmptySubmenus()
  {
    int n = mMenuItems.GetSize();
    
    WDL_PtrList<IPopupMenu::Item> toDelete;
    
    for (int i = 0; i < n; i++)
    {
      IPopupMenu::Item* pItem = GetItem(i);
      
      IPopupMenu* pSubmenu = pItem->GetSubmenu();
      
      if(pSubmenu && pSubmenu->NItems() == 0)
      {
        toDelete.Add(pItem);
      }
    }
    
    for (int i = 0; i < toDelete.GetSize(); i++)
    {
      mMenuItems.DeletePtr(toDelete.Get(i));
    }
  }

  void SetChosenItemIdx(int index) { mChosenItemIdx = index; };
  int GetChosenItemIdx() const { return mChosenItemIdx; }
  int NItems() const { return mMenuItems.GetSize(); }
  int NItemsPerColumn() const { return mNItemsPerColumn; }
  void SetNItemsPerColumn(int nItemsPerColumn) { mNItemsPerColumn = nItemsPerColumn; }
  int GetPrefix() const { return mPrefix; }
  bool GetCanMultiCheck() const { return mCanMultiCheck; }

  Item* GetItem(int index)
  {
    int nItems = NItems();
    
    if (index >= 0 && index < nItems)
    {
      return mMenuItems.Get(index);
    }
    else
    {
      return nullptr;
    }
  }
  
  Item* GetChosenItem()
  {
    return GetItem(mChosenItemIdx);
  }
  
  const char* GetItemText(int index)
  {
    Item* pItem = GetItem(index);
    if(pItem)
      return pItem->GetText();
    else
      return "";
  }
  
  void SetPrefix(int count)
  {
    if (count >= 0 && count < 4)
    {
      mPrefix = count;
    }
  }
  
  void SetMultiCheck(bool multicheck) { mCanMultiCheck = multicheck; }

  void Clear(bool resetEverything = true)
  {
    if (resetEverything)
    {
      SetChosenItemIdx(-1);
      SetPrefix(0);
      mCanMultiCheck = false;
    }
    
    mMenuItems.Empty(true);
  }

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

  void CheckItemWithText(const char* str, bool state = true)
  {
    for (int i = 0; i < mMenuItems.GetSize(); i++)
    {
      if (strcmp(mMenuItems.Get(i)->GetText(), str) == 0)
      {
        mMenuItems.Get(i)->SetChecked(state);
        break;
      }
    }
  }
  
  void CheckItemAlone(Item* pItemToCheck)
  {
    int n = mMenuItems.GetSize();
    
    for (int i = 0; i < n; i++)
    {
      IPopupMenu::Item* pItem = GetItem(i);
      pItem->SetChecked(false);
      IPopupMenu* pSubmenu = pItem->GetSubmenu();
      
      if (pSubmenu)
      {
        pSubmenu->CheckItemAlone(pItemToCheck);
      }
      else if (pItem == pItemToCheck)
      {
        pItem->SetChecked(true);
      }
    }
  }
  
  bool IsItemChecked(int index)
  {
    Item* pItem = mMenuItems.Get(index);
    
    if (pItem)
      return pItem->GetChecked();
    
    return false;
  }

  void SetFunction(IPopupFunction func)
  {
    mPopupFunc = func;
  }
  
  IPopupFunction GetFunction()
  {
    return mPopupFunc;
  }
  
  void ExecFunction()
  {
    mPopupFunc(this);
  }
  
  const char* GetRootTitle() const
  {
    return mRootTitle.Get();
  }
  
  void SetRootTitle(const char* rootTitle)
  {
    return mRootTitle.Set(rootTitle);
  }
  
private:
  int mNItemsPerColumn = 0; // Windows can divide popup menu into columns
  int mPrefix; // 0 = no prefix, 1 = numbers no leading zeros, 2 = 1 lz, 3 = 2lz
  int mChosenItemIdx = -1;
  bool mCanMultiCheck; // multicheck = 0 doesn't actually prohibit multichecking, you should do that in your code, by calling CheckItemAlone instead of CheckItem
  WDL_PtrList<Item> mMenuItems;
  IPopupFunction mPopupFunc = nullptr;
  WDL_String mRootTitle;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

/**@}*/
