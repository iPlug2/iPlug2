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
    return 0;
  }
}

const char* IPopupMenu::GetItemText(int index)
{
  return GetItem(index)->GetText();
}

IPopupMenuItem* IPopupMenu::AddItem(IPopupMenuItem* item, int index)
{
  if (index == -1)
  {
    mMenuItems.Add(item); // add it to the end
  }
  else
  {
    mMenuItems.Insert(index, item);
  }

  return item;
}

IPopupMenuItem* IPopupMenu::AddItem(const char* text, int index, int itemFlags)
{
  IPopupMenuItem* item = new IPopupMenuItem(text, itemFlags);
  return AddItem(item, index);
}

IPopupMenuItem* IPopupMenu::AddItem(const char* text, int index, IPopupMenu* submenu)
{
  IPopupMenuItem* item = new IPopupMenuItem(text, submenu);
  return AddItem(item, index);
}

IPopupMenuItem* IPopupMenu::AddItem(const char* text, IPopupMenu* submenu)
{
  IPopupMenuItem* item = new IPopupMenuItem(text, submenu);
  return AddItem(item, -1);
}

IPopupMenuItem* IPopupMenu::AddSeparator(int index)
{
  IPopupMenuItem* item = new IPopupMenuItem ("", IPopupMenuItem::kSeparator);
  return AddItem(item, index);
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
  IPopupMenuItem* item = mMenuItems.Get(index);

  if (item)
  {
    item->SetChecked(state);
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