#include "IPopupMenu.h"

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