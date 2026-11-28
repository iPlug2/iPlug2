/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests IPopupMenuControl, the pop-up menu that is drawn by IGraphics instead of by the platform

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestEditorDelegate.h"
#include "TestControls.h"
#include "IControls.h"
#include "IPopupMenuControl.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;
using ParamEvent = TestEditorDelegate::ParamEvent;
using DrawOp = IGraphicsHeadless::DrawOp;

namespace
{
enum EParams { kMode = 0, kNumParams };

const IRECT kCaptionBounds = IRECT(50, 50, 150, 80);
const IRECT kProbeBounds = IRECT(400, 300, 480, 380);

struct Fixture
{
  Fixture()
  : mDlg(kNumParams, 600, 400)
  {
    mDlg.GetParam(kMode)->InitEnum("Mode", 0, {"Sine", "Saw", "Square", "Noise"});
    mUI = mDlg.OpenUI();
    mUI->AttachPanelBackground(COLOR_GRAY);
    mCaption = mUI->AttachControl(new ICaptionControl(kCaptionBounds, kMode, DEFAULT_TEXT, DEFAULT_BGCOLOR, false));
    mProbe = mUI->AttachControl(new ProbeControl(kProbeBounds))->As<ProbeControl>();
    mUI->AttachPopupMenuControl();
    mPopup = mUI->GetPopupMenuControl();
    mDlg.SendCurrentParamValuesFromDelegate();
    mUI->RenderUntilClean();
    mUI->ClearRecorded();
    mDlg.ClearRecorded();
  }

  /** Let the menu's current animation (appearing, sub-menu appearing, ...) finish */
  void FinishMenuAnimation()
  {
    FinishAnimation(mPopup);
  }

  /** Run the collapse animation sequence to its end */
  void FinishCollapse()
  {
    for (int i = 0; i < 8 && mPopup->GetState() != IPopupMenuControl::kCollapsed; i++)
      FinishAnimation(mPopup);
  }

  /** Draw the UI and return the centre of the menu item with the given text
   * @return false if no such item was drawn */
  bool FindItem(const char* text, float& x, float& y)
  {
    mUI->ClearRecorded();
    mUI->SetAllControlsDirty();
    mUI->RenderFrame();

    // The menu is drawn last, so take the last piece of text that matches
    for (auto itr = mUI->mDrawOps.rbegin(); itr != mUI->mDrawOps.rend(); ++itr)
    {
      if (itr->mType == DrawOp::EType::Text && itr->mText == text)
      {
        x = itr->mBounds.MW();
        y = itr->mBounds.MH();
        return true;
      }
    }

    return false;
  }

  void ClickItem(const char* text)
  {
    float x = 0.f, y = 0.f;
    REQUIRE(FindItem(text, x, y));
    mUI->MouseMove(x, y);
    mUI->Click(x, y);
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
  IControl* mCaption = nullptr;
  ProbeControl* mProbe = nullptr;
  IPopupMenuControl* mPopup = nullptr;
};
} // namespace

TEST_CASE("IPopupMenuControl: is hidden and collapsed until a menu is requested", "[integration][controls][popupmenu]")
{
  Fixture f;

  REQUIRE(f.mPopup != nullptr);
  REQUIRE(f.mPopup->IsHidden());
  REQUIRE_FALSE(f.mPopup->GetExpanded());
  REQUIRE(f.mPopup->GetState() == IPopupMenuControl::kCollapsed);

  // Attaching a second time keeps the first
  f.mUI->AttachPopupMenuControl();
  REQUIRE(f.mUI->GetPopupMenuControl() == f.mPopup);
}

TEST_CASE("IPopupMenuControl: replaces the platform menu for parameter prompts", "[integration][controls][popupmenu][params]")
{
  Fixture f;

  f.mUI->Click(kCaptionBounds.MW(), kCaptionBounds.MH());

  REQUIRE(f.mUI->mNumPopupMenus == 0);
  REQUIRE_FALSE(f.mPopup->IsHidden());
  REQUIRE(f.mPopup->GetState() == IPopupMenuControl::kExpanding);
  REQUIRE(f.mUI->RenderFrame());

  f.FinishMenuAnimation();
  REQUIRE(f.mPopup->GetExpanded());

  // Every option is drawn, inside the menu, in order from top to bottom
  float x = 0.f, y = 0.f, previousY = 0.f;

  for (const char* item : {"Sine", "Saw", "Square", "Noise"})
  {
    REQUIRE(f.FindItem(item, x, y));
    REQUIRE(f.mPopup->GetRECT().Contains(x, y));
    REQUIRE(y > previousY);
    previousY = y;
  }

  // The menu is within the UI, near the control that opened it
  REQUIRE(f.mUI->GetBounds().Contains(f.mPopup->GetRECT()));
  REQUIRE(f.mPopup->GetRECT().MH() > kCaptionBounds.B);
  REQUIRE(f.mPopup->GetRECT().L < kCaptionBounds.R);
}

TEST_CASE("IPopupMenuControl: clicking an item delivers the choice to the control that opened the menu", "[integration][controls][popupmenu][params]")
{
  Fixture f;

  f.mUI->Click(kCaptionBounds.MW(), kCaptionBounds.MH());
  f.FinishMenuAnimation();
  f.ClickItem("Square");

  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 2);
  REQUIRE(f.mCaption->GetValue() == Approx(2. / 3.));
  REQUIRE(f.mDlg.LastParamValue(kMode) == Approx(2. / 3.));
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kMode) == 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kMode) == 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End, kMode) == 1);
  REQUIRE_FALSE(f.mPopup->GetExpanded());

  f.FinishCollapse();
  REQUIRE(f.mPopup->GetState() == IPopupMenuControl::kCollapsed);
  REQUIRE(f.mPopup->IsHidden());

  // Once collapsed the menu is no longer drawn, and the caption shows the choice
  float x = 0.f, y = 0.f;
  REQUIRE_FALSE(f.FindItem("Noise", x, y));
  REQUIRE(f.mUI->DidDrawText("Square"));

  // ... and the mouse goes to the controls again
  f.mUI->Click(kProbeBounds.MW(), kProbeBounds.MH());
  REQUIRE(f.mProbe->Count("down") == 1);
}

TEST_CASE("IPopupMenuControl: clicking outside the menu dismisses it without a choice", "[integration][controls][popupmenu][params]")
{
  Fixture f;

  f.mUI->Click(kCaptionBounds.MW(), kCaptionBounds.MH());
  f.FinishMenuAnimation();
  REQUIRE(f.mPopup->GetExpanded());

  // The click lands on the probe, but while the menu is open it belongs to the menu
  f.mUI->Click(kProbeBounds.MW(), kProbeBounds.MH());

  REQUIRE(f.mProbe->Count("down") == 0);
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 0);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value) == 0);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin) == f.mDlg.CountParamEvents(ParamEvent::EType::End));
  REQUIRE_FALSE(f.mPopup->GetExpanded());

  f.FinishCollapse();
  REQUIRE(f.mPopup->IsHidden());
}

TEST_CASE("IPopupMenuControl: the menu can be opened again after it has collapsed", "[integration][controls][popupmenu]")
{
  Fixture f;

  for (const char* item : {"Noise", "Saw", "Sine"})
  {
    f.mUI->Click(kCaptionBounds.MW(), kCaptionBounds.MH());
    f.FinishMenuAnimation();
    REQUIRE(f.mPopup->GetExpanded());
    f.ClickItem(item);
    f.FinishCollapse();
    REQUIRE(std::string(f.mDlg.GetParam(kMode)->GetDisplayText(f.mDlg.GetParam(kMode)->Int())) == item);
  }
}

TEST_CASE("IPopupMenuControl: a menu created by a control reports the chosen item and runs the menu's function", "[integration][controls][popupmenu]")
{
  Fixture f;
  int functionChoice = -2;
  IPopupMenu menu {"Actions", {"Copy", "Paste", "Delete"}, [&](IPopupMenu* pMenu) { functionChoice = pMenu->GetChosenItemIdx(); }};
  menu.AddSeparator();
  menu.AddItem("Disabled", -1, IPopupMenu::Item::kDisabled);

  f.mUI->CreatePopupMenu(*f.mProbe, menu, kProbeBounds);
  f.FinishMenuAnimation();
  REQUIRE(f.mPopup->GetExpanded());

  SECTION("an ordinary item")
  {
    f.ClickItem("Paste");
    REQUIRE(f.mProbe->mNumPopupSelections == 1);
    REQUIRE(f.mProbe->mPopupSelectionMenu == &menu);
    REQUIRE(f.mProbe->mPopupSelection == 1);
    REQUIRE(f.mProbe->mPopupSelectionText == "Paste");
    REQUIRE(functionChoice == 1);
    REQUIRE(menu.GetChosenItemIdx() == 1);
  }

  SECTION("a disabled item cannot be chosen")
  {
    f.ClickItem("Disabled");
    REQUIRE(f.mProbe->mNumPopupSelections == 1);
    REQUIRE(f.mProbe->mPopupSelectionMenu == nullptr);
    REQUIRE(f.mProbe->mPopupSelection == -1);
    REQUIRE(menu.GetChosenItemIdx() == -1);
  }

  f.FinishCollapse();
  REQUIRE(f.mPopup->GetState() == IPopupMenuControl::kCollapsed);
}

TEST_CASE("IPopupMenuControl: hovering an item with a sub-menu opens it, and its items can be chosen", "[integration][controls][popupmenu][submenu]")
{
  Fixture f;
  IPopupMenu menu;
  IPopupMenu* pSubMenu = new IPopupMenu(); // owned by the item it is added to
  pSubMenu->AddItem("Small");
  pSubMenu->AddItem("Medium");
  pSubMenu->AddItem("Large");
  menu.AddItem("Plain");
  menu.AddItem("Size", pSubMenu);
  menu.AddItem("Other");

  f.mUI->CreatePopupMenu(*f.mProbe, menu, IRECT(100, 100, 200, 120));
  f.FinishMenuAnimation();

  float x = 0.f, y = 0.f;
  REQUIRE_FALSE(f.FindItem("Medium", x, y));
  REQUIRE(f.FindItem("Size", x, y));

  f.mUI->MouseMove(x, y);
  f.FinishMenuAnimation();
  REQUIRE(f.mPopup->GetExpanded());

  REQUIRE(f.FindItem("Medium", x, y));
  f.mUI->MouseMove(x, y);
  f.mUI->Click(x, y);

  REQUIRE(f.mProbe->mNumPopupSelections == 1);
  REQUIRE(f.mProbe->mPopupSelectionMenu == pSubMenu);
  REQUIRE(f.mProbe->mPopupSelection == 1);
  REQUIRE(f.mProbe->mPopupSelectionText == "Medium");

  f.FinishCollapse();
  REQUIRE(f.mPopup->IsHidden());
}

TEST_CASE("IPopupMenuControl: checked items are drawn with a tick", "[integration][controls][popupmenu][drawing]")
{
  Fixture f;
  IPopupMenu plain {"", {"One", "Two"}};
  IPopupMenu checked {"", {"One", "Two"}};
  checked.CheckItem(1, true);

  auto countOps = [&](IPopupMenu& menu) {
    f.mUI->CreatePopupMenu(*f.mProbe, menu, IRECT(100, 100, 200, 120));
    f.FinishMenuAnimation();
    float x = 0.f, y = 0.f;
    REQUIRE(f.FindItem("Two", x, y));
    const size_t n = f.mUI->mDrawOps.size();
    f.mUI->Click(590, 390);
    f.FinishCollapse();
    return n;
  };

  const size_t nPlain = countOps(plain);
  const size_t nChecked = countOps(checked);
  REQUIRE(nChecked > nPlain);
}

TEST_CASE("IPopupMenuControl: removing it goes back to platform menus", "[integration][controls][popupmenu]")
{
  Fixture f;

  f.mUI->RemovePopupMenuControl();
  REQUIRE(f.mUI->GetPopupMenuControl() == nullptr);

  f.mUI->mPopupMenuChoice = 3;
  f.mUI->Click(kCaptionBounds.MW(), kCaptionBounds.MH());
  REQUIRE(f.mUI->mNumPopupMenus == 1);
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 3);
}

TEST_CASE("IPopupMenuControl: closing the UI while the menu is open is safe", "[integration][controls][popupmenu][lifecycle]")
{
  Fixture f;

  f.mUI->Click(kCaptionBounds.MW(), kCaptionBounds.MH());
  f.FinishMenuAnimation();
  REQUIRE(f.mPopup->GetExpanded());

  f.mDlg.CloseUI();
  REQUIRE(f.mDlg.GetUI() == nullptr);
}
