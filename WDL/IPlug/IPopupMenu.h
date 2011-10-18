#ifndef _IPOPUPMENU_
#define _IPOPUPMENU_

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "../mutex.h"
#include "../wdlstring.h"
#include "../ptrlist.h"

class IPopupMenu;

class IPopupMenuItem 
{
public:
	enum Flags {
		kNoFlags	= 0,
		kDisabled	= 1 << 0,     // item is gray and not selectable
		kTitle		= 1 << 1,     // item indicates a title and is not selectable
		kChecked	= 1 << 2,     // item has a checkmark
		kSeparator	= 1 << 3    // item is a separator
	};
	
	IPopupMenuItem(const char* text, const int flags = kNoFlags)
  : mFlags (flags)
  , mSubmenu (0)
  {
    SetText(text);
  }
	
//  IPopupMenuItem (const char* text, IPopupMenu* pSubMenu)
//  : mFlags (kNoFlags)
//  , mSubmenu (pSubMenu)
//  {
//    SetText(text);
//  }
	//IPopupMenuItem(const char* text, int tag);
	//IPopupMenuItem (const IPopupMenuItem& item);
	
	~IPopupMenuItem()
  {
//    if (mSubmenu) {
//      delete mSubmenu;
//    }
  }
	
	void SetText(const char* text) { mText.Set(text); }
	const char* GetText() { return mText.Get(); };
	
	bool GetEnabled () const { return !(mFlags & kDisabled); }
	bool GetChecked () const { return (mFlags & kChecked) != 0; }
	bool GetIsTitle () const { return (mFlags & kTitle) != 0; }
	bool GetIsSeparator () const { return (mFlags & kSeparator) != 0; }

	IPopupMenu* GetSubmenu () const { return mSubmenu; }
  
protected:
	WDL_String mText;
	IPopupMenu* mSubmenu;
	int mFlags;
};

class IPopupMenu
{
public:
	
  IPopupMenu() : mChosenItemIdx(-1), mPrefix(0) {}
  ~IPopupMenu() 
  {
    mMenuItems.Empty(true);
  }
	
	virtual IPopupMenuItem* AddItem(IPopupMenuItem* item, const int index = -1);
	//virtual IPopupMenuItem* AddItem(const char* text, int index, IPopupMenu* submenu);
	virtual IPopupMenuItem* AddItem(const char* text, int index = -1, int itemFlags = IPopupMenuItem::kNoFlags);
	virtual IPopupMenuItem* AddSeparator(int index = -1);
	
	void SetChosenItemIdx(const int itemIdx) { mChosenItemIdx = itemIdx; };
	int GetChosenItemIdx() { return mChosenItemIdx; }
	int GetNItems(){ return mMenuItems.GetSize(); }
	int GetPrefix() { return mPrefix; }
	
	IPopupMenuItem* GetItem(const int itemIdx);
	//const char* GetItemText(const int value);
	void SetPrefix (int count);

private:
	int mPrefix; // 0 = no prefix, 1 = numbers no leading zeros, 2 = 1 lz, 3 = 2lz
	int mChosenItemIdx;
	WDL_PtrList<IPopupMenuItem> mMenuItems;
};

#endif