/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IPopupMenu and IPopupMenu::Item.

#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "IGraphicsStructs.h"
#include "IGraphicsPopupMenu.h"

using namespace iplug;
using namespace igraphics;

namespace
{
using Item = IPopupMenu::Item;

/** @return The text of every item in the menu, in order */
std::vector<std::string> ItemTexts(IPopupMenu& menu)
{
  std::vector<std::string> texts;

  for (int i = 0; i < menu.NItems(); i++)
    texts.push_back(menu.GetItemText(i));

  return texts;
}

/** @return The indices of the checked items in the menu */
std::vector<int> CheckedItems(IPopupMenu& menu)
{
  std::vector<int> checked;

  for (int i = 0; i < menu.NItems(); i++)
  {
    if (menu.IsItemChecked(i))
      checked.push_back(i);
  }

  return checked;
}

using Strings = std::vector<std::string>;
using Ints = std::vector<int>;
} // namespace

// MARK: - IPopupMenu::Item

TEST_CASE("IPopupMenu::Item: construction from text", "[unit][ipopupmenu]")
{
  Item item {"Open"};
  REQUIRE(std::string(item.GetText()) == "Open");
  REQUIRE(item.GetEnabled());
  REQUIRE_FALSE(item.GetChecked());
  REQUIRE_FALSE(item.GetIsTitle());
  REQUIRE_FALSE(item.GetIsSeparator());
  REQUIRE_FALSE(item.GetIsDeletable());
  REQUIRE(item.GetTag() == -1);
  REQUIRE(item.GetSubmenu() == nullptr);
  REQUIRE(item.GetIsChoosable());
}

TEST_CASE("IPopupMenu::Item: construction with flags", "[unit][ipopupmenu]")
{
  SECTION("disabled")
  {
    Item item {"Item", Item::kDisabled};
    REQUIRE_FALSE(item.GetEnabled());
    REQUIRE_FALSE(item.GetChecked());
    REQUIRE_FALSE(item.GetIsTitle());
    REQUIRE_FALSE(item.GetIsSeparator());
    REQUIRE_FALSE(item.GetIsDeletable());
    REQUIRE_FALSE(item.GetIsChoosable());
  }

  SECTION("title")
  {
    Item item {"Item", Item::kTitle};
    REQUIRE(item.GetEnabled());
    REQUIRE_FALSE(item.GetChecked());
    REQUIRE(item.GetIsTitle());
    REQUIRE_FALSE(item.GetIsSeparator());
    REQUIRE_FALSE(item.GetIsDeletable());
    REQUIRE_FALSE(item.GetIsChoosable());
  }

  SECTION("checked")
  {
    Item item {"Item", Item::kChecked};
    REQUIRE(item.GetEnabled());
    REQUIRE(item.GetChecked());
    REQUIRE_FALSE(item.GetIsTitle());
    REQUIRE_FALSE(item.GetIsSeparator());
    REQUIRE_FALSE(item.GetIsDeletable());
    REQUIRE(item.GetIsChoosable());
  }

  SECTION("separator")
  {
    Item item {"", Item::kSeparator};
    REQUIRE(item.GetEnabled());
    REQUIRE_FALSE(item.GetChecked());
    REQUIRE_FALSE(item.GetIsTitle());
    REQUIRE(item.GetIsSeparator());
    REQUIRE_FALSE(item.GetIsDeletable());
    REQUIRE_FALSE(item.GetIsChoosable());
  }

  SECTION("deletable")
  {
    Item item {"Item", Item::kDeletable};
    REQUIRE(item.GetEnabled());
    REQUIRE_FALSE(item.GetChecked());
    REQUIRE_FALSE(item.GetIsTitle());
    REQUIRE_FALSE(item.GetIsSeparator());
    REQUIRE(item.GetIsDeletable());
    REQUIRE(item.GetIsChoosable());
  }

  SECTION("combined flags")
  {
    Item item {"Item", Item::kChecked | Item::kDisabled | Item::kDeletable};
    REQUIRE_FALSE(item.GetEnabled());
    REQUIRE(item.GetChecked());
    REQUIRE_FALSE(item.GetIsTitle());
    REQUIRE_FALSE(item.GetIsSeparator());
    REQUIRE(item.GetIsDeletable());
    REQUIRE_FALSE(item.GetIsChoosable());
  }

  SECTION("no flags")
  {
    Item item {"Item", Item::kNoFlags};
    REQUIRE(item.GetEnabled());
    REQUIRE_FALSE(item.GetChecked());
    REQUIRE(item.GetIsChoosable());
  }
}

TEST_CASE("IPopupMenu::Item: tags", "[unit][ipopupmenu]")
{
  REQUIRE(Item("Item").GetTag() == -1);
  REQUIRE(Item("Item", Item::kNoFlags).GetTag() == -1);
  REQUIRE(Item("Item", Item::kNoFlags, 42).GetTag() == 42);
  REQUIRE(Item("Item", Item::kChecked, 0).GetTag() == 0);
  REQUIRE(Item("Item", Item::kChecked, 0).GetChecked());
}

TEST_CASE("IPopupMenu::Item: SetText", "[unit][ipopupmenu]")
{
  Item item {"Before"};
  item.SetText("After");
  REQUIRE(std::string(item.GetText()) == "After");
  item.SetText("");
  REQUIRE(std::string(item.GetText()).empty());
  item.SetText("A much longer piece of text than was there before, with UTF-8: \xC3\xA9");
  REQUIRE(std::string(item.GetText()) == "A much longer piece of text than was there before, with UTF-8: \xC3\xA9");

  // the text is copied
  std::string source = "Copied";
  item.SetText(source.c_str());
  source = "Changed";
  REQUIRE(std::string(item.GetText()) == "Copied");
}

TEST_CASE("IPopupMenu::Item: setters change one flag at a time", "[unit][ipopupmenu]")
{
  Item item {"Item"};

  item.SetEnabled(false);
  REQUIRE_FALSE(item.GetEnabled());
  REQUIRE_FALSE(item.GetChecked());
  REQUIRE_FALSE(item.GetIsTitle());
  REQUIRE_FALSE(item.GetIsDeletable());

  item.SetChecked(true);
  REQUIRE_FALSE(item.GetEnabled());
  REQUIRE(item.GetChecked());
  REQUIRE_FALSE(item.GetIsTitle());
  REQUIRE_FALSE(item.GetIsDeletable());

  item.SetTitle(true);
  REQUIRE_FALSE(item.GetEnabled());
  REQUIRE(item.GetChecked());
  REQUIRE(item.GetIsTitle());
  REQUIRE_FALSE(item.GetIsDeletable());

  item.SetDeletable(true);
  REQUIRE_FALSE(item.GetEnabled());
  REQUIRE(item.GetChecked());
  REQUIRE(item.GetIsTitle());
  REQUIRE(item.GetIsDeletable());
  REQUIRE_FALSE(item.GetIsSeparator());

  item.SetEnabled(true);
  REQUIRE(item.GetEnabled());
  REQUIRE(item.GetChecked());
  REQUIRE(item.GetIsTitle());
  REQUIRE(item.GetIsDeletable());

  item.SetChecked(false);
  REQUIRE(item.GetEnabled());
  REQUIRE_FALSE(item.GetChecked());
  REQUIRE(item.GetIsTitle());
  REQUIRE(item.GetIsDeletable());

  item.SetTitle(false);
  REQUIRE(item.GetEnabled());
  REQUIRE_FALSE(item.GetChecked());
  REQUIRE_FALSE(item.GetIsTitle());
  REQUIRE(item.GetIsDeletable());

  item.SetDeletable(false);
  REQUIRE(item.GetEnabled());
  REQUIRE_FALSE(item.GetChecked());
  REQUIRE_FALSE(item.GetIsTitle());
  REQUIRE_FALSE(item.GetIsDeletable());
}

TEST_CASE("IPopupMenu::Item: setting a flag to its current state does nothing", "[unit][ipopupmenu]")
{
  Item item {"Item", Item::kChecked};
  item.SetChecked(true);
  REQUIRE(item.GetChecked());
  item.SetEnabled(true);
  REQUIRE(item.GetEnabled());
  item.SetTitle(false);
  REQUIRE_FALSE(item.GetIsTitle());
  item.SetChecked(false);
  item.SetChecked(false);
  REQUIRE_FALSE(item.GetChecked());
}

TEST_CASE("IPopupMenu::Item: GetIsChoosable", "[unit][ipopupmenu]")
{
  Item item {"Item"};
  REQUIRE(item.GetIsChoosable());

  SECTION("checked items can be chosen")
  {
    item.SetChecked(true);
    REQUIRE(item.GetIsChoosable());
  }

  SECTION("disabled items can't")
  {
    item.SetEnabled(false);
    REQUIRE_FALSE(item.GetIsChoosable());
    item.SetEnabled(true);
    REQUIRE(item.GetIsChoosable());
  }

  SECTION("titles can't")
  {
    item.SetTitle(true);
    REQUIRE_FALSE(item.GetIsChoosable());
    item.SetTitle(false);
    REQUIRE(item.GetIsChoosable());
  }

  SECTION("items that open a submenu can't")
  {
    item.SetSubmenu(new IPopupMenu());
    REQUIRE_FALSE(item.GetIsChoosable());
    item.SetSubmenu(nullptr);
    REQUIRE(item.GetIsChoosable());
  }
}

TEST_CASE("IPopupMenu::Item: owns its submenu", "[unit][ipopupmenu]")
{
  SECTION("when constructed with one")
  {
    IPopupMenu* pSubmenu = new IPopupMenu("Sub", {"One", "Two"});
    Item item {"Parent", pSubmenu};
    REQUIRE(std::string(item.GetText()) == "Parent");
    REQUIRE(item.GetSubmenu() == pSubmenu);
    REQUIRE(item.GetSubmenu()->NItems() == 2);
    REQUIRE(item.GetEnabled());
    REQUIRE_FALSE(item.GetChecked());
    REQUIRE_FALSE(item.GetIsTitle());
    REQUIRE_FALSE(item.GetIsSeparator());
    REQUIRE(item.GetTag() == -1);
  }

  SECTION("when one is set")
  {
    Item item {"Parent"};
    IPopupMenu* pSubmenu = new IPopupMenu();
    item.SetSubmenu(pSubmenu);
    REQUIRE(item.GetSubmenu() == pSubmenu);
  }

  SECTION("replacing a submenu deletes the previous one")
  {
    // the function keeps the tracker alive for as long as the first submenu exists
    auto tracker = std::make_shared<int>(0);
    IPopupMenu* pFirst = new IPopupMenu();
    pFirst->SetFunction([tracker](IPopupMenu*) {});
    REQUIRE(tracker.use_count() == 2);

    Item item {"Parent", pFirst};
    IPopupMenu* pSecond = new IPopupMenu();
    item.SetSubmenu(pSecond);
    REQUIRE(tracker.use_count() == 1);
    REQUIRE(item.GetSubmenu() == pSecond);

    item.SetSubmenu(nullptr);
    REQUIRE(item.GetSubmenu() == nullptr);
  }

  SECTION("the submenu is deleted with the item")
  {
    auto tracker = std::make_shared<int>(0);

    {
      IPopupMenu* pSubmenu = new IPopupMenu();
      pSubmenu->SetFunction([tracker](IPopupMenu*) {});
      Item item {"Parent", pSubmenu};
      REQUIRE(tracker.use_count() == 2);
    }

    REQUIRE(tracker.use_count() == 1);
  }
}

// MARK: - IPopupMenu construction

TEST_CASE("IPopupMenu: default construction", "[unit][ipopupmenu]")
{
  IPopupMenu menu;
  REQUIRE(menu.NItems() == 0);
  REQUIRE(std::string(menu.GetRootTitle()).empty());
  REQUIRE(menu.GetPrefix() == 0);
  REQUIRE_FALSE(menu.GetCanMultiCheck());
  REQUIRE(menu.GetChosenItemIdx() == -1);
  REQUIRE(menu.GetChosenItem() == nullptr);
  REQUIRE(menu.NItemsPerColumn() == 0);
  REQUIRE_FALSE(menu.HasSubMenus());
  REQUIRE(menu.GetFunction() == nullptr);
  REQUIRE(menu.GetItem(0) == nullptr);
}

TEST_CASE("IPopupMenu: construction with a title, prefix, multicheck and items", "[unit][ipopupmenu]")
{
  SECTION("title only")
  {
    IPopupMenu menu {"Root"};
    REQUIRE(std::string(menu.GetRootTitle()) == "Root");
    REQUIRE(menu.GetPrefix() == 0);
    REQUIRE_FALSE(menu.GetCanMultiCheck());
    REQUIRE(menu.NItems() == 0);
  }

  SECTION("title and prefix")
  {
    IPopupMenu menu {"Root", 2};
    REQUIRE(std::string(menu.GetRootTitle()) == "Root");
    REQUIRE(menu.GetPrefix() == 2);
    REQUIRE_FALSE(menu.GetCanMultiCheck());
  }

  SECTION("title, prefix and multicheck")
  {
    IPopupMenu menu {"Root", 1, true};
    REQUIRE(menu.GetPrefix() == 1);
    REQUIRE(menu.GetCanMultiCheck());
    REQUIRE(menu.NItems() == 0);
  }

  SECTION("title, prefix, multicheck and items")
  {
    IPopupMenu menu {"Root", 3, true, {"One", "Two", "Three"}};
    REQUIRE(std::string(menu.GetRootTitle()) == "Root");
    REQUIRE(menu.GetPrefix() == 3);
    REQUIRE(menu.GetCanMultiCheck());
    REQUIRE(ItemTexts(menu) == Strings {"One", "Two", "Three"});
  }
}

TEST_CASE("IPopupMenu: construction from a title and a list of items", "[unit][ipopupmenu]")
{
  SECTION("without a function")
  {
    IPopupMenu menu {"Root", {"One", "Two", "Three"}};
    REQUIRE(std::string(menu.GetRootTitle()) == "Root");
    REQUIRE(menu.GetPrefix() == 0);
    REQUIRE_FALSE(menu.GetCanMultiCheck());
    REQUIRE(menu.GetFunction() == nullptr);
    REQUIRE(menu.GetChosenItemIdx() == -1);
    REQUIRE(ItemTexts(menu) == Strings {"One", "Two", "Three"});

    // the items are ordinary, choosable items
    for (int i = 0; i < menu.NItems(); i++)
    {
      REQUIRE(menu.GetItem(i)->GetIsChoosable());
      REQUIRE_FALSE(menu.GetItem(i)->GetChecked());
      REQUIRE(menu.GetItem(i)->GetTag() == -1);
      REQUIRE(menu.GetItem(i)->GetSubmenu() == nullptr);
    }
  }

  SECTION("with a function")
  {
    int nCalls = 0;
    IPopupMenu menu {"Root", {"One", "Two"}, [&nCalls](IPopupMenu*) { nCalls++; }};
    REQUIRE(menu.NItems() == 2);
    REQUIRE(menu.GetFunction() != nullptr);
    menu.ExecFunction();
    REQUIRE(nCalls == 1);
  }

  SECTION("a single item")
  {
    IPopupMenu menu {"", {"Only"}};
    REQUIRE(std::string(menu.GetRootTitle()).empty());
    REQUIRE(ItemTexts(menu) == Strings {"Only"});
  }
}

TEST_CASE("IPopupMenu: root title", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"Before"};
  menu.SetRootTitle("After");
  REQUIRE(std::string(menu.GetRootTitle()) == "After");
  menu.SetRootTitle("");
  REQUIRE(std::string(menu.GetRootTitle()).empty());
}

// MARK: - IPopupMenu adding items

TEST_CASE("IPopupMenu: AddItem by text appends by default", "[unit][ipopupmenu]")
{
  IPopupMenu menu;

  Item* pFirst = menu.AddItem("First");
  REQUIRE(pFirst != nullptr);
  REQUIRE(menu.NItems() == 1);
  REQUIRE(menu.GetItem(0) == pFirst);
  REQUIRE(std::string(pFirst->GetText()) == "First");

  Item* pSecond = menu.AddItem("Second");
  Item* pThird = menu.AddItem("Third", -1);
  REQUIRE(menu.NItems() == 3);
  REQUIRE(menu.GetItem(0) == pFirst);
  REQUIRE(menu.GetItem(1) == pSecond);
  REQUIRE(menu.GetItem(2) == pThird);
  REQUIRE(ItemTexts(menu) == Strings {"First", "Second", "Third"});

  // items added by text are plain items
  REQUIRE(pFirst->GetEnabled());
  REQUIRE_FALSE(pFirst->GetChecked());
  REQUIRE_FALSE(pFirst->GetIsTitle());
  REQUIRE_FALSE(pFirst->GetIsSeparator());
  REQUIRE(pFirst->GetSubmenu() == nullptr);
  REQUIRE(pFirst->GetTag() == -1);
}

TEST_CASE("IPopupMenu: AddItem by text with flags", "[unit][ipopupmenu]")
{
  IPopupMenu menu;
  Item* pTitle = menu.AddItem("Title", -1, Item::kTitle);
  Item* pChecked = menu.AddItem("Checked", -1, Item::kChecked);
  Item* pDisabled = menu.AddItem("Disabled", -1, Item::kDisabled);
  Item* pBoth = menu.AddItem("Both", -1, Item::kChecked | Item::kDisabled);

  REQUIRE(menu.NItems() == 4);
  REQUIRE(pTitle->GetIsTitle());
  REQUIRE_FALSE(pTitle->GetIsChoosable());
  REQUIRE(pChecked->GetChecked());
  REQUIRE(pChecked->GetIsChoosable());
  REQUIRE_FALSE(pDisabled->GetEnabled());
  REQUIRE_FALSE(pDisabled->GetChecked());
  REQUIRE(pBoth->GetChecked());
  REQUIRE_FALSE(pBoth->GetEnabled());

  REQUIRE(CheckedItems(menu) == Ints {1, 3});
}

TEST_CASE("IPopupMenu: AddItem at an index inserts", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"", {"A", "B", "C"}};

  SECTION("at the start")
  {
    Item* pItem = menu.AddItem("X", 0);
    REQUIRE(menu.GetItem(0) == pItem);
    REQUIRE(ItemTexts(menu) == Strings {"X", "A", "B", "C"});
  }

  SECTION("in the middle")
  {
    Item* pItem = menu.AddItem("X", 1);
    REQUIRE(menu.GetItem(1) == pItem);
    REQUIRE(ItemTexts(menu) == Strings {"A", "X", "B", "C"});

    menu.AddItem("Y", 3);
    REQUIRE(ItemTexts(menu) == Strings {"A", "X", "B", "Y", "C"});
  }

  SECTION("at the end")
  {
    Item* pItem = menu.AddItem("X", 3);
    REQUIRE(menu.GetItem(3) == pItem);
    REQUIRE(ItemTexts(menu) == Strings {"A", "B", "C", "X"});
  }

  SECTION("past the end appends")
  {
    Item* pItem = menu.AddItem("X", 100);
    REQUIRE(menu.GetItem(3) == pItem);
    REQUIRE(ItemTexts(menu) == Strings {"A", "B", "C", "X"});
  }

  SECTION("with flags")
  {
    Item* pItem = menu.AddItem("X", 1, Item::kChecked);
    REQUIRE(pItem->GetChecked());
    REQUIRE(CheckedItems(menu) == Ints {1});
  }

  SECTION("in to an empty menu")
  {
    IPopupMenu emptyMenu;
    emptyMenu.AddItem("X", 0);
    REQUIRE(ItemTexts(emptyMenu) == Strings {"X"});
  }
}

TEST_CASE("IPopupMenu: AddItem with an index of -2 keeps the items sorted", "[unit][ipopupmenu]")
{
  IPopupMenu menu;

  for (const char* text : {"delta", "alpha", "echo", "charlie", "bravo"})
    menu.AddItem(text, -2);

  REQUIRE(ItemTexts(menu) == Strings {"alpha", "bravo", "charlie", "delta", "echo"});

  SECTION("sorting ignores case")
  {
    IPopupMenu mixedCase;

    for (const char* text : {"banana", "Cherry", "apple", "Date"})
      mixedCase.AddItem(text, -2);

    REQUIRE(ItemTexts(mixedCase) == Strings {"apple", "banana", "Cherry", "Date"});
  }

  SECTION("by Item")
  {
    Item* pItem = menu.AddItem(new Item("cat"), -2);
    REQUIRE(menu.GetIndexOfItem(pItem) == 2);
    REQUIRE(ItemTexts(menu) == Strings {"alpha", "bravo", "cat", "charlie", "delta", "echo"});
  }

  SECTION("Sortfunc compares the text of two items without regard to case")
  {
    Item apple {"apple"};
    Item upperApple {"APPLE"};
    Item banana {"Banana"};
    REQUIRE(IPopupMenu::Sortfunc(&apple, &banana) < 0);
    REQUIRE(IPopupMenu::Sortfunc(&banana, &apple) > 0);
    REQUIRE(IPopupMenu::Sortfunc(&apple, &upperApple) == 0);
    REQUIRE(IPopupMenu::Sortfunc(&apple, &apple) == 0);
  }
}

TEST_CASE("IPopupMenu: AddItem by Item takes ownership", "[unit][ipopupmenu]")
{
  IPopupMenu menu;

  Item* pTagged = new Item("Tagged", Item::kChecked, 99);
  REQUIRE(menu.AddItem(pTagged) == pTagged);
  REQUIRE(menu.NItems() == 1);
  REQUIRE(menu.GetItem(0) == pTagged);
  REQUIRE(menu.GetItem(0)->GetTag() == 99);
  REQUIRE(menu.IsItemChecked(0));

  Item* pFirst = new Item("First");
  REQUIRE(menu.AddItem(pFirst, 0) == pFirst);
  Item* pLast = new Item("Last");
  REQUIRE(menu.AddItem(pLast, -1) == pLast);

  REQUIRE(ItemTexts(menu) == Strings {"First", "Tagged", "Last"});
  REQUIRE(menu.GetItem(0) == pFirst);
  REQUIRE(menu.GetItem(1) == pTagged);
  REQUIRE(menu.GetItem(2) == pLast);

  // no deletes here: the menu frees the items (checked by the sanitizer builds)
}

TEST_CASE("IPopupMenu: AddSeparator", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"", {"A", "B"}};

  SECTION("appends by default")
  {
    Item* pSeparator = menu.AddSeparator();
    REQUIRE(pSeparator != nullptr);
    REQUIRE(menu.NItems() == 3);
    REQUIRE(menu.GetItem(2) == pSeparator);
    REQUIRE(pSeparator->GetIsSeparator());
    REQUIRE_FALSE(pSeparator->GetIsChoosable());
    REQUIRE_FALSE(pSeparator->GetChecked());
    REQUIRE_FALSE(pSeparator->GetIsTitle());
    REQUIRE(pSeparator->GetSubmenu() == nullptr);
    REQUIRE(std::string(pSeparator->GetText()).empty());
    REQUIRE(std::string(menu.GetItemText(2)).empty());
  }

  SECTION("at an index")
  {
    Item* pSeparator = menu.AddSeparator(1);
    REQUIRE(menu.NItems() == 3);
    REQUIRE(menu.GetItem(1) == pSeparator);
    REQUIRE(ItemTexts(menu) == Strings {"A", "", "B"});
    REQUIRE_FALSE(menu.GetItem(0)->GetIsSeparator());
    REQUIRE(menu.GetItem(1)->GetIsSeparator());
    REQUIRE_FALSE(menu.GetItem(2)->GetIsSeparator());
  }

  SECTION("several")
  {
    menu.AddSeparator();
    menu.AddItem("C");
    menu.AddSeparator();
    REQUIRE(menu.NItems() == 5);

    int nSeparators = 0;
    for (int i = 0; i < menu.NItems(); i++)
      nSeparators += menu.GetItem(i)->GetIsSeparator() ? 1 : 0;

    REQUIRE(nSeparators == 2);
  }
}

// MARK: - IPopupMenu accessing items

TEST_CASE("IPopupMenu: GetItem", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"", {"A", "B", "C"}};

  REQUIRE(menu.GetItem(0) != nullptr);
  REQUIRE(menu.GetItem(2) != nullptr);
  REQUIRE(std::string(menu.GetItem(0)->GetText()) == "A");
  REQUIRE(std::string(menu.GetItem(1)->GetText()) == "B");
  REQUIRE(std::string(menu.GetItem(2)->GetText()) == "C");

  // out of range indices give null
  REQUIRE(menu.GetItem(3) == nullptr);
  REQUIRE(menu.GetItem(100) == nullptr);
  REQUIRE(menu.GetItem(-1) == nullptr);
  REQUIRE(menu.GetItem(-100) == nullptr);
}

TEST_CASE("IPopupMenu: GetItemText", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"", {"A", "B", "C"}};

  REQUIRE(std::string(menu.GetItemText(0)) == "A");
  REQUIRE(std::string(menu.GetItemText(2)) == "C");

  // out of range indices give an empty string
  REQUIRE(std::string(menu.GetItemText(3)).empty());
  REQUIRE(std::string(menu.GetItemText(-1)).empty());

  // changes to the item are reflected
  menu.GetItem(1)->SetText("Changed");
  REQUIRE(std::string(menu.GetItemText(1)) == "Changed");
}

TEST_CASE("IPopupMenu: GetIndexOfItem", "[unit][ipopupmenu]")
{
  IPopupMenu menu;
  Item* pA = menu.AddItem("A");
  Item* pB = menu.AddItem("B");
  Item* pC = menu.AddItem("C");

  REQUIRE(menu.GetIndexOfItem(pA) == 0);
  REQUIRE(menu.GetIndexOfItem(pB) == 1);
  REQUIRE(menu.GetIndexOfItem(pC) == 2);

  // indices follow insertions
  menu.AddItem("X", 0);
  REQUIRE(menu.GetIndexOfItem(pA) == 1);
  REQUIRE(menu.GetIndexOfItem(pC) == 3);

  // an item that isn't in the menu
  Item other {"Other"};
  REQUIRE(menu.GetIndexOfItem(&other) == -1);
  REQUIRE(menu.GetIndexOfItem(nullptr) == -1);

  // an item in a different menu
  IPopupMenu otherMenu;
  Item* pElsewhere = otherMenu.AddItem("A");
  REQUIRE(menu.GetIndexOfItem(pElsewhere) == -1);
}

TEST_CASE("IPopupMenu: holds many items", "[unit][ipopupmenu]")
{
  IPopupMenu menu;

  for (int i = 0; i < 500; i++)
    menu.AddItem(std::to_string(i).c_str());

  REQUIRE(menu.NItems() == 500);

  bool allMatch = true;
  for (int i = 0; i < 500; i++)
    allMatch = allMatch && std::to_string(i) == menu.GetItemText(i);

  REQUIRE(allMatch);
}

// MARK: - IPopupMenu chosen item

TEST_CASE("IPopupMenu: chosen item", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"", {"A", "B", "C"}};

  REQUIRE(menu.GetChosenItemIdx() == -1);
  REQUIRE(menu.GetChosenItem() == nullptr);

  menu.SetChosenItemIdx(1);
  REQUIRE(menu.GetChosenItemIdx() == 1);
  REQUIRE(menu.GetChosenItem() == menu.GetItem(1));
  REQUIRE(std::string(menu.GetChosenItem()->GetText()) == "B");

  menu.SetChosenItemIdx(0);
  REQUIRE(menu.GetChosenItem() == menu.GetItem(0));
  menu.SetChosenItemIdx(2);
  REQUIRE(menu.GetChosenItem() == menu.GetItem(2));

  // choosing doesn't check
  REQUIRE(CheckedItems(menu).empty());

  SECTION("an out of range index gives no chosen item")
  {
    menu.SetChosenItemIdx(3);
    REQUIRE(menu.GetChosenItemIdx() == 3);
    REQUIRE(menu.GetChosenItem() == nullptr);
  }

  SECTION("can be reset")
  {
    menu.SetChosenItemIdx(-1);
    REQUIRE(menu.GetChosenItemIdx() == -1);
    REQUIRE(menu.GetChosenItem() == nullptr);
  }
}

// MARK: - IPopupMenu checking items

TEST_CASE("IPopupMenu: CheckItem and IsItemChecked", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"", {"A", "B", "C"}};
  REQUIRE(CheckedItems(menu).empty());

  REQUIRE(menu.CheckItem(1, true));
  REQUIRE(menu.IsItemChecked(1));
  REQUIRE(menu.GetItem(1)->GetChecked());
  REQUIRE(CheckedItems(menu) == Ints {1});

  // CheckItem doesn't uncheck other items
  REQUIRE(menu.CheckItem(2, true));
  REQUIRE(CheckedItems(menu) == Ints {1, 2});

  REQUIRE(menu.CheckItem(1, false));
  REQUIRE_FALSE(menu.IsItemChecked(1));
  REQUIRE(CheckedItems(menu) == Ints {2});

  // checking twice is the same as once
  REQUIRE(menu.CheckItem(2, true));
  REQUIRE(CheckedItems(menu) == Ints {2});

  SECTION("out of range indices are reported and ignored")
  {
    REQUIRE_FALSE(menu.CheckItem(3, true));
    REQUIRE_FALSE(menu.CheckItem(-1, true));
    REQUIRE_FALSE(menu.CheckItem(100, false));
    REQUIRE_FALSE(menu.IsItemChecked(3));
    REQUIRE_FALSE(menu.IsItemChecked(-1));
    REQUIRE(CheckedItems(menu) == Ints {2});
  }

  SECTION("follows changes made directly to the item")
  {
    menu.GetItem(0)->SetChecked(true);
    REQUIRE(menu.IsItemChecked(0));
  }
}

TEST_CASE("IPopupMenu: CheckItemAlone by index", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"", {"A", "B", "C", "D"}};

  menu.CheckItemAlone(2);
  REQUIRE(CheckedItems(menu) == Ints {2});

  menu.CheckItemAlone(0);
  REQUIRE(CheckedItems(menu) == Ints {0});

  SECTION("unchecks everything that was checked before")
  {
    menu.CheckItem(1, true);
    menu.CheckItem(2, true);
    menu.CheckItem(3, true);
    REQUIRE(CheckedItems(menu) == Ints {0, 1, 2, 3});

    menu.CheckItemAlone(3);
    REQUIRE(CheckedItems(menu) == Ints {3});
  }

  SECTION("an index that isn't in the menu unchecks everything")
  {
    menu.CheckItemAlone(-1);
    REQUIRE(CheckedItems(menu).empty());

    menu.CheckItem(1, true);
    menu.CheckItemAlone(100);
    REQUIRE(CheckedItems(menu).empty());
  }

  SECTION("on an empty menu")
  {
    IPopupMenu emptyMenu;
    emptyMenu.CheckItemAlone(0);
    REQUIRE(emptyMenu.NItems() == 0);
  }
}

TEST_CASE("IPopupMenu: CheckItemAlone by Item", "[unit][ipopupmenu]")
{
  IPopupMenu menu;
  Item* pA = menu.AddItem("A", -1, Item::kChecked);
  Item* pB = menu.AddItem("B");
  Item* pC = menu.AddItem("C", -1, Item::kChecked);

  menu.CheckItemAlone(pB);
  REQUIRE_FALSE(pA->GetChecked());
  REQUIRE(pB->GetChecked());
  REQUIRE_FALSE(pC->GetChecked());

  menu.CheckItemAlone(pC);
  REQUIRE(CheckedItems(menu) == Ints {2});

  SECTION("an item that isn't in the menu unchecks everything")
  {
    Item other {"Other"};
    menu.CheckItemAlone(&other);
    REQUIRE(CheckedItems(menu).empty());
    REQUIRE_FALSE(other.GetChecked());
  }
}

TEST_CASE("IPopupMenu: CheckItemAlone by Item searches submenus", "[unit][ipopupmenu]")
{
  IPopupMenu menu;
  Item* pTop = menu.AddItem("Top", -1, Item::kChecked);

  IPopupMenu* pSubmenu = new IPopupMenu();
  Item* pSubA = pSubmenu->AddItem("Sub A", -1, Item::kChecked);
  Item* pSubB = pSubmenu->AddItem("Sub B");

  IPopupMenu* pNested = new IPopupMenu();
  Item* pNestedA = pNested->AddItem("Nested A", -1, Item::kChecked);
  Item* pNestedB = pNested->AddItem("Nested B");
  pSubmenu->AddItem("Nested", pNested);

  Item* pSubmenuItem = menu.AddItem("Submenu", pSubmenu);

  SECTION("checking an item two levels down")
  {
    menu.CheckItemAlone(pNestedB);
    REQUIRE(pNestedB->GetChecked());
    REQUIRE_FALSE(pNestedA->GetChecked());
    REQUIRE_FALSE(pSubA->GetChecked());
    REQUIRE_FALSE(pSubB->GetChecked());
    REQUIRE_FALSE(pTop->GetChecked());
    REQUIRE_FALSE(pSubmenuItem->GetChecked());
  }

  SECTION("checking an item one level down")
  {
    menu.CheckItemAlone(pSubB);
    REQUIRE(pSubB->GetChecked());
    REQUIRE_FALSE(pSubA->GetChecked());
    REQUIRE_FALSE(pNestedA->GetChecked());
    REQUIRE_FALSE(pNestedB->GetChecked());
    REQUIRE_FALSE(pTop->GetChecked());
  }

  SECTION("checking a top level item unchecks the items in submenus")
  {
    menu.CheckItemAlone(pTop);
    REQUIRE(pTop->GetChecked());
    REQUIRE_FALSE(pSubA->GetChecked());
    REQUIRE_FALSE(pSubB->GetChecked());
    REQUIRE_FALSE(pNestedA->GetChecked());
    REQUIRE_FALSE(pNestedB->GetChecked());
  }
}

TEST_CASE("IPopupMenu: CheckItemWithText", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"", {"A", "B", "C", "B"}};

  SECTION("checks by default")
  {
    menu.CheckItemWithText("C");
    REQUIRE(CheckedItems(menu) == Ints {2});
  }

  SECTION("doesn't uncheck other items")
  {
    menu.CheckItem(0, true);
    menu.CheckItemWithText("C", true);
    REQUIRE(CheckedItems(menu) == Ints {0, 2});
  }

  SECTION("can uncheck")
  {
    menu.CheckItem(0, true);
    menu.CheckItem(2, true);
    menu.CheckItemWithText("C", false);
    REQUIRE(CheckedItems(menu) == Ints {0});
  }

  SECTION("only the first match is changed")
  {
    menu.CheckItemWithText("B");
    REQUIRE(CheckedItems(menu) == Ints {1});
  }

  SECTION("text that doesn't match anything is ignored")
  {
    menu.CheckItemWithText("Z");
    menu.CheckItemWithText("");
    REQUIRE(CheckedItems(menu).empty());
  }

  SECTION("matching is exact and case sensitive")
  {
    menu.CheckItemWithText("a");
    menu.CheckItemWithText("A ");
    menu.CheckItemWithText("AB");
    REQUIRE(CheckedItems(menu).empty());
  }
}

// MARK: - IPopupMenu settings

TEST_CASE("IPopupMenu: SetPrefix accepts 0 to 3", "[unit][ipopupmenu]")
{
  IPopupMenu menu;

  for (int prefix : {1, 2, 3, 0})
  {
    menu.SetPrefix(prefix);
    REQUIRE(menu.GetPrefix() == prefix);
  }

  // other values are ignored
  menu.SetPrefix(2);
  menu.SetPrefix(4);
  REQUIRE(menu.GetPrefix() == 2);
  menu.SetPrefix(-1);
  REQUIRE(menu.GetPrefix() == 2);
  menu.SetPrefix(100);
  REQUIRE(menu.GetPrefix() == 2);
}

TEST_CASE("IPopupMenu: SetMultiCheck", "[unit][ipopupmenu]")
{
  IPopupMenu menu;
  REQUIRE_FALSE(menu.GetCanMultiCheck());
  menu.SetMultiCheck(true);
  REQUIRE(menu.GetCanMultiCheck());
  menu.SetMultiCheck(false);
  REQUIRE_FALSE(menu.GetCanMultiCheck());
}

TEST_CASE("IPopupMenu: SetNItemsPerColumn", "[unit][ipopupmenu]")
{
  IPopupMenu menu;
  REQUIRE(menu.NItemsPerColumn() == 0);
  menu.SetNItemsPerColumn(16);
  REQUIRE(menu.NItemsPerColumn() == 16);
  menu.SetNItemsPerColumn(0);
  REQUIRE(menu.NItemsPerColumn() == 0);
}

// MARK: - IPopupMenu function

TEST_CASE("IPopupMenu: SetFunction, GetFunction and ExecFunction", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"", {"A", "B", "C"}};
  REQUIRE(menu.GetFunction() == nullptr);

  int nCalls = 0;
  IPopupMenu* pReceived = nullptr;
  std::string chosenText;

  menu.SetFunction([&](IPopupMenu* pMenu) {
    nCalls++;
    pReceived = pMenu;

    if (Item* pItem = pMenu->GetChosenItem())
      chosenText = pItem->GetText();
  });

  REQUIRE(menu.GetFunction() != nullptr);
  REQUIRE(nCalls == 0);

  SECTION("the function is called with the menu")
  {
    menu.SetChosenItemIdx(1);
    menu.ExecFunction();
    REQUIRE(nCalls == 1);
    REQUIRE(pReceived == &menu);
    REQUIRE(chosenText == "B");

    menu.SetChosenItemIdx(2);
    menu.ExecFunction();
    REQUIRE(nCalls == 2);
    REQUIRE(chosenText == "C");
  }

  SECTION("the function returned by GetFunction can be called")
  {
    IPopupFunction func = menu.GetFunction();
    func(&menu);
    REQUIRE(nCalls == 1);
    REQUIRE(pReceived == &menu);
  }

  SECTION("the function can be replaced")
  {
    int nOtherCalls = 0;
    menu.SetFunction([&nOtherCalls](IPopupMenu*) { nOtherCalls++; });
    menu.ExecFunction();
    REQUIRE(nCalls == 0);
    REQUIRE(nOtherCalls == 1);
  }

  SECTION("the function can be removed")
  {
    menu.SetFunction(nullptr);
    REQUIRE(menu.GetFunction() == nullptr);
  }
}

// MARK: - IPopupMenu submenus

TEST_CASE("IPopupMenu: adding a submenu", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"", {"A", "B"}};
  REQUIRE_FALSE(menu.HasSubMenus());

  SECTION("appended by default")
  {
    IPopupMenu* pSubmenu = new IPopupMenu("Sub", {"One", "Two"});
    Item* pItem = menu.AddItem("More", pSubmenu);

    REQUIRE(menu.NItems() == 3);
    REQUIRE(menu.GetItem(2) == pItem);
    REQUIRE(std::string(pItem->GetText()) == "More");
    REQUIRE(pItem->GetSubmenu() == pSubmenu);
    REQUIRE_FALSE(pItem->GetIsChoosable());
    REQUIRE(pItem->GetEnabled());
    REQUIRE(menu.HasSubMenus());

    // the submenu's own items are untouched, and are not counted by the parent
    REQUIRE(ItemTexts(*pSubmenu) == Strings {"One", "Two"});
    REQUIRE(ItemTexts(menu) == Strings {"A", "B", "More"});
  }

  SECTION("at an index, with the submenu as the last argument")
  {
    IPopupMenu* pSubmenu = new IPopupMenu();
    Item* pItem = menu.AddItem("More", 0, pSubmenu);
    REQUIRE(menu.GetItem(0) == pItem);
    REQUIRE(pItem->GetSubmenu() == pSubmenu);
    REQUIRE(ItemTexts(menu) == Strings {"More", "A", "B"});
  }

  SECTION("at an index, with the submenu as the second argument")
  {
    IPopupMenu* pSubmenu = new IPopupMenu();
    Item* pItem = menu.AddItem("More", pSubmenu, 1);
    REQUIRE(menu.GetItem(1) == pItem);
    REQUIRE(pItem->GetSubmenu() == pSubmenu);
    REQUIRE(ItemTexts(menu) == Strings {"A", "More", "B"});
  }

  SECTION("appended with an explicit index of -1")
  {
    menu.AddItem("First", -1, new IPopupMenu());
    menu.AddItem("Second", new IPopupMenu(), -1);
    REQUIRE(ItemTexts(menu) == Strings {"A", "B", "First", "Second"});
    REQUIRE(menu.GetItem(2)->GetSubmenu() != nullptr);
    REQUIRE(menu.GetItem(3)->GetSubmenu() != nullptr);
  }

  SECTION("by Item")
  {
    IPopupMenu* pSubmenu = new IPopupMenu();
    menu.AddItem(new Item("More", pSubmenu));
    REQUIRE(menu.HasSubMenus());
    REQUIRE(menu.GetItem(2)->GetSubmenu() == pSubmenu);
  }
}

TEST_CASE("IPopupMenu: HasSubMenus only looks at this menu's items", "[unit][ipopupmenu]")
{
  IPopupMenu menu;
  REQUIRE_FALSE(menu.HasSubMenus());

  menu.AddItem("A");
  menu.AddSeparator();
  REQUIRE_FALSE(menu.HasSubMenus());

  IPopupMenu* pSubmenu = new IPopupMenu("", {"One"});
  menu.AddItem("More", pSubmenu);
  REQUIRE(menu.HasSubMenus());
  REQUIRE_FALSE(pSubmenu->HasSubMenus());
}

TEST_CASE("IPopupMenu: a submenu inherits the parent's function when it is added", "[unit][ipopupmenu]")
{
  int nCalls = 0;
  IPopupMenu* pReceived = nullptr;

  auto func = [&](IPopupMenu* pMenu) {
    nCalls++;
    pReceived = pMenu;
  };

  SECTION("when the parent has a function")
  {
    IPopupMenu menu;
    menu.SetFunction(func);

    IPopupMenu* pSubmenu = new IPopupMenu("", {"One", "Two"});
    menu.AddItem("More", pSubmenu);
    REQUIRE(pSubmenu->GetFunction() != nullptr);

    // the function is called with the submenu that the item was chosen from
    pSubmenu->SetChosenItemIdx(1);
    pSubmenu->ExecFunction();
    REQUIRE(nCalls == 1);
    REQUIRE(pReceived == pSubmenu);
    REQUIRE(std::string(pReceived->GetChosenItem()->GetText()) == "Two");
  }

  SECTION("with either argument order")
  {
    IPopupMenu menu;
    menu.SetFunction(func);

    IPopupMenu* pFirst = new IPopupMenu();
    IPopupMenu* pSecond = new IPopupMenu();
    menu.AddItem("First", 0, pFirst);
    menu.AddItem("Second", pSecond, 1);
    REQUIRE(pFirst->GetFunction() != nullptr);
    REQUIRE(pSecond->GetFunction() != nullptr);
  }

  SECTION("when the parent has no function")
  {
    IPopupMenu menu;
    IPopupMenu* pSubmenu = new IPopupMenu();
    menu.AddItem("More", pSubmenu);
    REQUIRE(pSubmenu->GetFunction() == nullptr);
  }

  SECTION("a menu constructed with a function passes it on")
  {
    IPopupMenu menu {"Root", {"A"}, func};
    IPopupMenu* pSubmenu = new IPopupMenu();
    menu.AddItem("More", pSubmenu);
    pSubmenu->ExecFunction();
    REQUIRE(nCalls == 1);
    REQUIRE(pReceived == pSubmenu);
  }
}

TEST_CASE("IPopupMenu: nested submenus", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"Root"};

  // built from the top down, as a plug-in would
  IPopupMenu* pLevel1 = new IPopupMenu("Level 1");
  menu.AddItem("To level 1", pLevel1);
  IPopupMenu* pLevel2 = new IPopupMenu("Level 2");
  pLevel1->AddItem("To level 2", pLevel2);
  IPopupMenu* pLevel3 = new IPopupMenu("Level 3", {"Leaf A", "Leaf B"});
  pLevel2->AddItem("To level 3", pLevel3);

  REQUIRE(menu.NItems() == 1);
  REQUIRE(menu.HasSubMenus());
  REQUIRE(pLevel1->HasSubMenus());
  REQUIRE(pLevel2->HasSubMenus());
  REQUIRE_FALSE(pLevel3->HasSubMenus());

  // walking down from the root
  IPopupMenu* pWalk = menu.GetItem(0)->GetSubmenu();
  REQUIRE(pWalk == pLevel1);
  REQUIRE(std::string(pWalk->GetRootTitle()) == "Level 1");
  pWalk = pWalk->GetItem(0)->GetSubmenu();
  REQUIRE(pWalk == pLevel2);
  pWalk = pWalk->GetItem(0)->GetSubmenu();
  REQUIRE(pWalk == pLevel3);
  REQUIRE(ItemTexts(*pWalk) == Strings {"Leaf A", "Leaf B"});
  REQUIRE(pWalk->GetItem(0)->GetSubmenu() == nullptr);

  // each level has its own chosen item
  pLevel3->SetChosenItemIdx(1);
  REQUIRE(menu.GetChosenItemIdx() == -1);
  REQUIRE(pLevel1->GetChosenItemIdx() == -1);
  REQUIRE(std::string(pLevel3->GetChosenItem()->GetText()) == "Leaf B");
}

TEST_CASE("IPopupMenu: owns its items and submenus", "[unit][ipopupmenu]")
{
  // Each submenu's function holds a reference to the tracker, so its use count shows how many are still alive
  auto tracker = std::make_shared<int>(0);
  auto makeTrackedMenu = []() {
    IPopupMenu* pMenu = new IPopupMenu();
    pMenu->AddItem("Leaf");
    return pMenu;
  };
  auto track = [&tracker](IPopupMenu* pMenu) { pMenu->SetFunction([tracker](IPopupMenu*) {}); };

  SECTION("destroying a menu destroys its submenus, at every level")
  {
    {
      IPopupMenu menu;
      IPopupMenu* pLevel1 = makeTrackedMenu();
      IPopupMenu* pLevel2 = makeTrackedMenu();
      IPopupMenu* pSibling = makeTrackedMenu();
      menu.AddItem("Level 1", pLevel1);
      pLevel1->AddItem("Level 2", pLevel2);
      menu.AddItem("Sibling", pSibling);
      track(pLevel1);
      track(pLevel2);
      track(pSibling);
      REQUIRE(tracker.use_count() == 4);
    }

    REQUIRE(tracker.use_count() == 1);
  }

  SECTION("Clear destroys the submenus")
  {
    IPopupMenu menu;
    IPopupMenu* pLevel1 = makeTrackedMenu();
    IPopupMenu* pLevel2 = makeTrackedMenu();
    menu.AddItem("A");
    menu.AddItem("Level 1", pLevel1);
    pLevel1->AddItem("Level 2", pLevel2);
    track(pLevel1);
    track(pLevel2);
    REQUIRE(tracker.use_count() == 3);

    menu.Clear();
    REQUIRE(tracker.use_count() == 1);
    REQUIRE(menu.NItems() == 0);
    REQUIRE_FALSE(menu.HasSubMenus());
  }

  SECTION("a heap allocated menu")
  {
    auto pMenu = std::make_unique<IPopupMenu>("Root", std::initializer_list<const char*> {"A", "B"});
    IPopupMenu* pSubmenu = makeTrackedMenu();
    pMenu->AddItem("More", pSubmenu);
    track(pSubmenu);
    REQUIRE(tracker.use_count() == 2);
    pMenu.reset();
    REQUIRE(tracker.use_count() == 1);
  }
}

// MARK: - IPopupMenu removing items

TEST_CASE("IPopupMenu: Clear removes all the items and resets the menu", "[unit][ipopupmenu]")
{
  int nCalls = 0;
  IPopupMenu menu {"Root", 2, true, {"A", "B", "C"}};
  menu.SetChosenItemIdx(1);
  menu.SetNItemsPerColumn(8);
  menu.SetFunction([&nCalls](IPopupMenu*) { nCalls++; });

  SECTION("by default the chosen item, prefix and multicheck are reset")
  {
    menu.Clear();
    REQUIRE(menu.NItems() == 0);
    REQUIRE(menu.GetItem(0) == nullptr);
    REQUIRE(menu.GetChosenItemIdx() == -1);
    REQUIRE(menu.GetChosenItem() == nullptr);
    REQUIRE(menu.GetPrefix() == 0);
    REQUIRE_FALSE(menu.GetCanMultiCheck());
  }

  SECTION("resetting everything explicitly")
  {
    menu.Clear(true);
    REQUIRE(menu.NItems() == 0);
    REQUIRE(menu.GetChosenItemIdx() == -1);
    REQUIRE(menu.GetPrefix() == 0);
    REQUIRE_FALSE(menu.GetCanMultiCheck());
  }

  SECTION("the items alone can be cleared")
  {
    menu.Clear(false);
    REQUIRE(menu.NItems() == 0);
    REQUIRE(menu.GetChosenItemIdx() == 1);
    REQUIRE(menu.GetChosenItem() == nullptr);
    REQUIRE(menu.GetPrefix() == 2);
    REQUIRE(menu.GetCanMultiCheck());
  }

  SECTION("the title and function are kept")
  {
    menu.Clear();
    REQUIRE(std::string(menu.GetRootTitle()) == "Root");
    REQUIRE(menu.GetFunction() != nullptr);
    menu.ExecFunction();
    REQUIRE(nCalls == 1);
  }

  SECTION("the menu can be refilled")
  {
    menu.Clear();
    menu.AddItem("X");
    menu.AddItem("Y");
    REQUIRE(ItemTexts(menu) == Strings {"X", "Y"});
    REQUIRE(CheckedItems(menu).empty());
  }

  SECTION("clearing an empty menu")
  {
    IPopupMenu emptyMenu;
    emptyMenu.Clear();
    emptyMenu.Clear(false);
    REQUIRE(emptyMenu.NItems() == 0);
  }
}

TEST_CASE("IPopupMenu: RemoveEmptySubmenus leaves a menu without empty submenus alone", "[unit][ipopupmenu]")
{
  IPopupMenu menu {"", {"A", "B"}};
  menu.AddSeparator();
  IPopupMenu* pSubmenu = new IPopupMenu("", {"One"});
  Item* pSubmenuItem = menu.AddItem("More", pSubmenu);

  menu.RemoveEmptySubmenus();

  REQUIRE(ItemTexts(menu) == Strings {"A", "B", "", "More"});
  REQUIRE(menu.GetItem(3) == pSubmenuItem);
  REQUIRE(pSubmenuItem->GetSubmenu() == pSubmenu);
  REQUIRE(pSubmenu->NItems() == 1);

  // and on a menu with no items at all
  IPopupMenu emptyMenu;
  emptyMenu.RemoveEmptySubmenus();
  REQUIRE(emptyMenu.NItems() == 0);
}

// BUG: IGraphicsPopupMenu.h:214 IPopupMenu::RemoveEmptySubmenus() removes the items from the list with
// WDL_PtrList::DeletePtr(p), which only removes the pointer: the Item (and the empty IPopupMenu it owns) is
// never deleted, so every removed submenu is leaked (as LeakSanitizer also reports when this test is run).
TEST_CASE("IPopupMenu: RemoveEmptySubmenus removes and frees the items with empty submenus", "[.][bug]")
{
  auto tracker = std::make_shared<int>(0);

  IPopupMenu menu;
  menu.AddItem("A");
  IPopupMenu* pEmpty1 = new IPopupMenu();
  menu.AddItem("Empty 1", pEmpty1);
  IPopupMenu* pFull = new IPopupMenu("", {"One"});
  menu.AddItem("Full", pFull);
  menu.AddItem("B");
  IPopupMenu* pEmpty2 = new IPopupMenu();
  menu.AddItem("Empty 2", pEmpty2);

  pEmpty1->SetFunction([tracker](IPopupMenu*) {});
  pEmpty2->SetFunction([tracker](IPopupMenu*) {});
  CHECK(tracker.use_count() == 3);

  menu.RemoveEmptySubmenus();

  CHECK(ItemTexts(menu) == Strings {"A", "Full", "B"});
  CHECK(menu.GetItem(1)->GetSubmenu() == pFull);
  CHECK(menu.HasSubMenus());

  // the removed submenus should have been destroyed
  CHECK(tracker.use_count() == 1);
}
