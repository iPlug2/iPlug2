#include "IPopupMenu.h"

IPopupMenuItem::IPopupMenuItem(const char* text, const int flags)
: mFlags (flags)
, mSubmenu (0)
{
	SetText (text);
}

void IPopupMenuItem::SetText(const char* text)
{
	//TODO: is this a bit brash?
	strcpy (mText, text);
}

IPopupMenu::IPopupMenu() : mChosenItemIdx(-1), mPrefix(0) {}
IPopupMenu::~IPopupMenu() {}

IPopupMenuItem* IPopupMenu::GetItem(const int itemIdx)
{
	int nItems = GetNItems();
	
	if (itemIdx >= 0 && itemIdx < nItems) 
	{
		return mMenuItems.Get(itemIdx);
	}
	else {
		return 0;
	}

}

IPopupMenuItem* IPopupMenu::AddItem(IPopupMenuItem* item, const int index)
{
	if (index == -1) mMenuItems.Add(item); // add it to the end
	else
	{
//		mMenuItems.Add.Insert(index, item);
	}

	return item;
}

IPopupMenuItem* IPopupMenu::AddItem(const char* text, int index, int itemFlags)
{
	IPopupMenuItem* item = new IPopupMenuItem (text, itemFlags);
	return AddItem(item, index);
}

IPopupMenuItem* IPopupMenu::AddSeparator (int index)
{
	IPopupMenuItem* item = new IPopupMenuItem ("", IPopupMenuItem::kSeparator);
	return AddItem(item, index);
}

void IPopupMenu::SetPrefix(int count)
{
	if (count >= 0 && count < 4)
		mPrefix = count;
}