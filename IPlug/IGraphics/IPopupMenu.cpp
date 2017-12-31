#include "IPopupMenu.h"

void IPopupMenuItem::SetChecked(bool state)
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

IPopupMenuItem* IPopupMenu::GetItem(int index)
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

const char* IPopupMenu::GetItemText(int index)
{
  return GetItem(index)->GetText();
}

IPopupMenuItem* IPopupMenu::AddItem(IPopupMenuItem* pItem, int index)
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

IPopupMenuItem* IPopupMenu::AddItem(const char* text, int index, int itemFlags)
{
  return AddItem(new IPopupMenuItem(text, itemFlags), index);
}

IPopupMenuItem* IPopupMenu::AddItem(const char* text, int index, IPopupMenu* pSubmenu)
{
  return AddItem(new IPopupMenuItem(text, pSubmenu), index);
}

IPopupMenuItem* IPopupMenu::AddItem(const char* text, IPopupMenu* pSubmenu)
{
  return AddItem(new IPopupMenuItem(text, pSubmenu), -1);
}

IPopupMenuItem* IPopupMenu::AddSeparator(int index)
{
  IPopupMenuItem* pItem = new IPopupMenuItem ("", IPopupMenuItem::kSeparator);
  return AddItem(pItem, index);
}

void IPopupMenu::SetPrefix(int count)
{
  if (count >= 0 && count < 4)
  {
    mPrefix = count;
  }
}

bool IPopupMenu::CheckItem(int index, bool state)
{
  IPopupMenuItem* pItem = mMenuItems.Get(index);

  if (pItem)
  {
    pItem->SetChecked(state);
    return true;
  }
  return false;
}

void IPopupMenu::CheckItemAlone(int index)
{
  for (int i = 0; i < mMenuItems.GetSize(); i++)
  {
    mMenuItems.Get(i)->SetChecked(i == index);
  }
}

bool IPopupMenu::IsItemChecked(int index)
{
  IPopupMenuItem* item = mMenuItems.Get(index);

  if (item)
  {
    return item->GetChecked();
  }

  return false;
}
