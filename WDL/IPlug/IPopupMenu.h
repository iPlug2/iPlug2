#ifndef _IPOPUPMENU_
#define _IPOPUPMENU_

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "../mutex.h"
#include "../wdlstring.h"
#include "../ptrlist.h"

// this (and the platform implementation in IGraphics*) is largely based on the VSTGUI COptionMenu

class IPopupMenu;

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
    , mSubmenu(0)
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

  void SetChecked(bool state);

protected:
  WDL_String mText;
  IPopupMenu* mSubmenu;
  int mFlags;
};

class IPopupMenu
{
public:

  IPopupMenu(int prefix = 0, bool multicheck = false)
    : mChosenItemIdx(-1)
    , mPrefix(prefix)
    , mCanMultiCheck(multicheck)
  {}

  ~IPopupMenu()
  {
    mMenuItems.Empty(true);
  }

  IPopupMenuItem* AddItem(IPopupMenuItem* item, int index = -1);
  IPopupMenuItem* AddItem(const char* text, int index, IPopupMenu* submenu);
  IPopupMenuItem* AddItem(const char* text, IPopupMenu* submenu);
  IPopupMenuItem* AddItem(const char* text, int index = -1, int itemFlags = IPopupMenuItem::kNoFlags);
  IPopupMenuItem* AddSeparator(int index = -1);

  void SetChosenItemIdx(int index) { mChosenItemIdx = index; };
  int GetChosenItemIdx() { return mChosenItemIdx; }
  int GetNItems() { return mMenuItems.GetSize(); }
  int GetPrefix() { return mPrefix; }
  bool GetCanMultiCheck() { return mCanMultiCheck; }

  IPopupMenuItem* GetItem(int index);
  const char* GetItemText(int index);
  void SetPrefix(int count);
  void SetMultiCheck(bool multicheck) { mCanMultiCheck = multicheck; }

  bool CheckItem(int index, bool state);
  void CheckItemAlone(int index);

  bool IsItemChecked(int index);

private:
  int mPrefix; // 0 = no prefix, 1 = numbers no leading zeros, 2 = 1 lz, 3 = 2lz
  int mChosenItemIdx;
  bool mCanMultiCheck; // multicheck = 0 doesn't actually prohibit multichecking, you should do that in your code, by calling CheckItemAlone instead of CheckItem
  WDL_PtrList<IPopupMenuItem> mMenuItems;
};

#endif