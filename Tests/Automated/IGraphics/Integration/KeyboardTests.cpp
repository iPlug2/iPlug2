/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests key event delivery, the global key handler, keyboard navigation/focus and modal controls in IGraphics

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestEditorDelegate.h"
#include "TestControls.h"
#include "IControls.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;
using ParamEvent = TestEditorDelegate::ParamEvent;

namespace
{
enum EParams { kGain = 0, kSteps, kOnOff, kMode, kNumParams };

const IVStyle kPlainStyle = DEFAULT_STYLE.WithShowLabel(false).WithShowValue(false);
const IColor kRingColor = IColor(255, 10, 200, 30);

struct KeyRecord
{
  int mVK;
  std::string mUTF8;
  bool mIsUp;
  bool mS, mC, mA;
};

/** A background, two probes, and a row of controls that can take the keyboard focus */
struct Fixture
{
  Fixture()
  : mDlg(kNumParams, 500, 300)
  {
    mDlg.GetParam(kGain)->InitDouble("Gain", 50., 0., 100., 0.01, "%");
    mDlg.GetParam(kSteps)->InitInt("Steps", 2, 0, 10);
    mDlg.GetParam(kOnOff)->InitBool("OnOff", false);
    mDlg.GetParam(kMode)->InitEnum("Mode", 0, {"One", "Two", "Three"});
    mUI = mDlg.OpenUI();
    mUI->AttachPanelBackground(COLOR_GRAY);
    mA = mUI->AttachControl(new ProbeControl(IRECT(20, 20, 120, 120)))->As<ProbeControl>();
    mB = mUI->AttachControl(new ProbeControl(IRECT(200, 20, 300, 120)))->As<ProbeControl>();
    mKnob = mUI->AttachControl(new IVKnobControl(IRECT(20, 150, 100, 230), kGain, "", kPlainStyle));
    mLabel = mUI->AttachControl(new ITextControl(IRECT(20, 240, 100, 260), "A label"));
    mStepSlider = mUI->AttachControl(new IVSliderControl(IRECT(110, 150, 150, 290), kSteps, "", kPlainStyle));
    mToggle = mUI->AttachControl(new IVToggleControl(IRECT(160, 150, 240, 190), kOnOff, "", kPlainStyle));
    mTabs = mUI->AttachControl(new IVTabSwitchControl(IRECT(250, 150, 490, 190), kMode, {}, "", kPlainStyle));
    mDlg.SendCurrentParamValuesFromDelegate();
    mDlg.ClearRecorded();
    mUI->SetFocusRingColor(kRingColor);
    mUI->RenderUntilClean();
    mUI->ClearRecorded();
  }

  void InstallHandler(bool returnValue)
  {
    mUI->SetKeyHandlerFunc([this, returnValue](const IKeyPress& key, bool isUp) {
      mHandlerKeys.push_back({key.VK, key.utf8, isUp, key.S, key.C, key.A});
      return returnValue;
    });
  }

  /** @return true if the last frame drew the focus ring just inside the bounds of the control */
  bool DrewFocusRing(IControl* pControl)
  {
    for (auto& op : mUI->mDrawOps)
    {
      if (op.mType == IGraphicsHeadless::DrawOp::EType::Stroke && op.mColor.R == kRingColor.R && op.mColor.G == kRingColor.G && op.mColor.B == kRingColor.B
          && pControl->GetRECT().Contains(op.mBounds) && pControl->GetRECT().GetPadded(-3.f).W() <= op.mBounds.W())
        return true;
    }
    return false;
  }

  bool RenderAndCheckRing(IControl* pControl)
  {
    mUI->ClearRecorded();
    mUI->RenderFrame();
    return DrewFocusRing(pControl);
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
  ProbeControl* mA = nullptr;
  ProbeControl* mB = nullptr;
  IControl* mKnob = nullptr;
  IControl* mLabel = nullptr;
  IControl* mStepSlider = nullptr;
  IControl* mToggle = nullptr;
  IControl* mTabs = nullptr;
  std::vector<KeyRecord> mHandlerKeys;
};
} // namespace

#pragma mark - Delivery

TEST_CASE("Keys: a key press goes to the control under the pointer", "[integration][keyboard]")
{
  Fixture f;
  f.mA->mHandlesKeys = true;
  f.mUI->MouseMove(50, 60);

  REQUIRE(f.mUI->KeyDown('A', "a"));
  REQUIRE(f.mA->Names() == "keydown");
  REQUIRE(f.mA->Last("keydown")->mVK == 'A');
  REQUIRE(f.mA->Last("keydown")->mStr == "a");
  REQUIRE(f.mA->Last("keydown")->mX == Approx(50.f));
  REQUIRE(f.mA->Last("keydown")->mY == Approx(60.f));

  REQUIRE(f.mUI->KeyUp('A', "a"));
  REQUIRE(f.mA->Names() == "keydown keyup");
  REQUIRE(f.mB->mEvents.empty());
}

TEST_CASE("Keys: the return value says whether the key was handled", "[integration][keyboard]")
{
  Fixture f;
  f.mUI->MouseMove(50, 60);

  f.mA->mHandlesKeys = false;
  REQUIRE_FALSE(f.mUI->KeyDown('A', "a"));
  REQUIRE_FALSE(f.mUI->KeyUp('A', "a"));
  REQUIRE(f.mA->Names() == "keydown keyup");

  // Nothing under the pointer
  f.mUI->MouseMove(450, 250);
  REQUIRE_FALSE(f.mUI->KeyDown('A', "a"));
  REQUIRE_FALSE(f.mUI->KeyUp('A', "a"));
  REQUIRE(f.mA->Count("keydown") == 1);
}

TEST_CASE("Keys: the global key handler gets keys that no control handled", "[integration][keyboard][keyhandler]")
{
  Fixture f;
  f.InstallHandler(true);
  REQUIRE(f.mUI->GetKeyHandlerFunc() != nullptr);

  SECTION("no control under the pointer")
  {
    f.mUI->MouseMove(450, 250);
    REQUIRE(f.mUI->KeyDown(kVK_F1, "", true, false, true));
    REQUIRE(f.mUI->KeyUp(kVK_F1, ""));

    REQUIRE(f.mHandlerKeys.size() == 2);
    REQUIRE(f.mHandlerKeys[0].mVK == kVK_F1);
    REQUIRE_FALSE(f.mHandlerKeys[0].mIsUp);
    REQUIRE(f.mHandlerKeys[0].mS);
    REQUIRE_FALSE(f.mHandlerKeys[0].mC);
    REQUIRE(f.mHandlerKeys[0].mA);
    REQUIRE(f.mHandlerKeys[1].mIsUp);
  }

  SECTION("the control under the pointer does not handle the key")
  {
    f.mUI->MouseMove(50, 60);
    REQUIRE(f.mUI->KeyDown('Z', "z"));
    REQUIRE(f.mA->Count("keydown") == 1);
    REQUIRE(f.mHandlerKeys.size() == 1);
    REQUIRE(f.mHandlerKeys[0].mUTF8 == "z");
  }

  SECTION("the control under the pointer handles the key: the handler is not called")
  {
    f.mA->mHandlesKeys = true;
    f.mUI->MouseMove(50, 60);
    REQUIRE(f.mUI->KeyDown('Z', "z"));
    REQUIRE(f.mUI->KeyUp('Z', "z"));
    REQUIRE(f.mA->Count("keydown") == 1);
    REQUIRE(f.mA->Count("keyup") == 1);
    REQUIRE(f.mHandlerKeys.empty());
  }
}

TEST_CASE("Keys: the global key handler's return value is passed on", "[integration][keyboard][keyhandler]")
{
  Fixture f;
  f.InstallHandler(false);
  f.mUI->MouseMove(450, 250);

  REQUIRE_FALSE(f.mUI->KeyDown('Q', "q"));
  REQUIRE_FALSE(f.mUI->KeyUp('Q', "q"));
  REQUIRE(f.mHandlerKeys.size() == 2);

  f.mUI->SetKeyHandlerFunc(nullptr);
  REQUIRE_FALSE(f.mUI->KeyDown('Q', "q"));
  REQUIRE(f.mHandlerKeys.size() == 2);
}

TEST_CASE("Keys: the background control does not receive keys", "[integration][keyboard]")
{
  TestEditorDelegate dlg {0, 200, 200};
  auto* pUI = dlg.OpenUI();
  auto* pBackground = pUI->AttachControl(new ProbeControl(IRECT(0, 0, 200, 200)))->As<ProbeControl>();
  pBackground->mHandlesKeys = true;
  int nHandlerCalls = 0;
  pUI->SetKeyHandlerFunc([&](const IKeyPress&, bool) { nHandlerCalls++; return true; });

  pUI->MouseMove(100, 100);
  REQUIRE(pUI->KeyDown('A', "a"));
  REQUIRE(pUI->KeyUp('A', "a"));
  REQUIRE(pBackground->mEvents.empty());
  REQUIRE(nHandlerCalls == 2);
}

TEST_CASE("Keys: a control that has captured the mouse gets the keys wherever the pointer is", "[integration][keyboard][mouse]")
{
  Fixture f;
  f.mA->mHandlesKeys = true;

  f.mUI->MouseDown(50, 50);
  f.mUI->MouseDragTo(250, 50); // now over B
  REQUIRE(f.mUI->KeyDown(kVK_SHIFT, ""));
  REQUIRE(f.mA->Count("keydown") == 1);
  REQUIRE(f.mB->Count("keydown") == 0);
  f.mUI->MouseUp(250, 50);
}

TEST_CASE("Keys: key events from the delegate are delivered at the pointer position", "[integration][keyboard]")
{
  Fixture f;
  f.mB->mHandlesKeys = true;
  f.mUI->MouseMove(250, 50);

  REQUIRE(f.mDlg.OnKeyDown(IKeyPress("x", 'X')));
  REQUIRE(f.mDlg.OnKeyUp(IKeyPress("x", 'X')));
  REQUIRE(f.mB->Names() == "keydown keyup");
  REQUIRE(f.mB->Last("keydown")->mX == Approx(250.f));
}

#pragma mark - Keyboard navigation

TEST_CASE("Keyboard navigation: is opt-in, and tab does nothing while it is off", "[integration][keyboard][focus]")
{
  Fixture f;

#ifdef IGRAPHICS_ACCESSKIT
  REQUIRE(f.mUI->KeyboardNavigationEnabled());
  f.mUI->EnableKeyboardNavigation(false);
#endif

  REQUIRE_FALSE(f.mUI->KeyboardNavigationEnabled());
  f.mUI->MouseMove(450, 250);
  REQUIRE_FALSE(f.mUI->KeyDown(kVK_TAB));
  REQUIRE(f.mUI->GetKeyboardFocus() == nullptr);

  f.mUI->EnableKeyboardNavigation(true);
  REQUIRE(f.mUI->KeyboardNavigationEnabled());
  REQUIRE(f.mUI->KeyDown(kVK_TAB));
  REQUIRE(f.mUI->GetKeyboardFocus() != nullptr);
}

TEST_CASE("Keyboard navigation: tab and shift-tab cycle through the interactive controls in stack order", "[integration][keyboard][focus]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);
  f.mUI->MouseMove(450, 250);

  // The probes and the label are not interactive, so they are skipped
  REQUIRE(f.mUI->KeyDown(kVK_TAB));
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mKnob);
  REQUIRE(f.mUI->KeyDown(kVK_TAB));
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mStepSlider);
  REQUIRE(f.mUI->KeyDown(kVK_TAB));
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mToggle);
  REQUIRE(f.mUI->KeyDown(kVK_TAB));
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mTabs);
  // ... and round to the start
  REQUIRE(f.mUI->KeyDown(kVK_TAB));
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mKnob);

  REQUIRE(f.mUI->KeyDown(kVK_TAB, "", true));
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mTabs);
  REQUIRE(f.mUI->KeyDown(kVK_TAB, "", true));
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mToggle);
}

TEST_CASE("Keyboard navigation: shift-tab with nothing focused goes to the last control", "[integration][keyboard][focus]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);

  REQUIRE(f.mUI->KeyDown(kVK_TAB, "", true));
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mTabs);
}

TEST_CASE("Keyboard navigation: hidden and disabled controls are skipped", "[integration][keyboard][focus]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);
  f.mStepSlider->Hide(true);
  f.mToggle->SetDisabled(true);

  f.mUI->KeyDown(kVK_TAB);
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mKnob);
  f.mUI->KeyDown(kVK_TAB);
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mTabs);
  f.mUI->KeyDown(kVK_TAB);
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mKnob);
}

TEST_CASE("Keyboard navigation: tab is not handled when nothing can take the focus", "[integration][keyboard][focus]")
{
  TestEditorDelegate dlg {0, 200, 200};
  auto* pUI = dlg.OpenUI();
  pUI->AttachPanelBackground(COLOR_GRAY);
  pUI->AttachControl(new ITextControl(IRECT(10, 10, 100, 30), "Only a label"));
  pUI->EnableKeyboardNavigation(true);

  REQUIRE_FALSE(pUI->KeyDown(kVK_TAB));
  REQUIRE(pUI->GetKeyboardFocus() == nullptr);
}

TEST_CASE("Keyboard navigation: the focus ring is drawn around the focused control", "[integration][keyboard][focus][drawing]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);

  REQUIRE_FALSE(f.RenderAndCheckRing(f.mKnob));

  f.mUI->KeyDown(kVK_TAB);
  REQUIRE(f.mKnob->IsDirty());
  REQUIRE(f.RenderAndCheckRing(f.mKnob));

  // Moving on redraws both controls, and the ring moves
  f.mUI->KeyDown(kVK_TAB);
  REQUIRE(f.mKnob->IsDirty());
  REQUIRE(f.mStepSlider->IsDirty());
  f.mUI->ClearRecorded();
  f.mUI->RenderFrame();
  REQUIRE(f.DrewFocusRing(f.mStepSlider));
  REQUIRE_FALSE(f.DrewFocusRing(f.mKnob));
}

TEST_CASE("Keyboard navigation: escape gives up the focus", "[integration][keyboard][focus]")
{
  Fixture f;
  f.InstallHandler(false);
  f.mUI->EnableKeyboardNavigation(true);
  f.mUI->MouseMove(450, 250);
  f.mUI->KeyDown(kVK_TAB);
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mKnob);

  REQUIRE(f.mUI->KeyDown(kVK_ESCAPE));
  REQUIRE(f.mUI->GetKeyboardFocus() == nullptr);
  REQUIRE(f.mHandlerKeys.empty());
  REQUIRE_FALSE(f.RenderAndCheckRing(f.mKnob));

  // With no focus, escape is an ordinary key again
  REQUIRE_FALSE(f.mUI->KeyDown(kVK_ESCAPE));
  REQUIRE(f.mHandlerKeys.size() == 1);
}

TEST_CASE("Keyboard navigation: arrow keys adjust a focused knob and tell the host", "[integration][keyboard][focus][params]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);
  f.mUI->MouseMove(450, 250);
  f.mUI->SetKeyboardFocus(f.mKnob);
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mKnob);

  REQUIRE(f.mUI->KeyDown(kVK_UP));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(51.));
  REQUIRE(f.mKnob->GetValue() == Approx(0.51));
  REQUIRE(f.mDlg.mParamEvents.size() == 3);
  REQUIRE(f.mDlg.mParamEvents[0].mType == ParamEvent::EType::Begin);
  REQUIRE(f.mDlg.mParamEvents[1].mType == ParamEvent::EType::Value);
  REQUIRE(f.mDlg.mParamEvents[1].mNormalizedValue == Approx(0.51));
  REQUIRE(f.mDlg.mParamEvents[2].mType == ParamEvent::EType::End);

  REQUIRE(f.mUI->KeyDown(kVK_RIGHT));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(52.));
  REQUIRE(f.mUI->KeyDown(kVK_DOWN));
  REQUIRE(f.mUI->KeyDown(kVK_LEFT));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(50.));

  REQUIRE(f.mUI->KeyDown(kVK_PRIOR));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(60.));
  REQUIRE(f.mUI->KeyDown(kVK_NEXT));
  REQUIRE(f.mUI->KeyDown(kVK_NEXT));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(40.));

  REQUIRE(f.mUI->KeyDown(kVK_END));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(100.));
  REQUIRE(f.mUI->KeyDown(kVK_UP));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(100.));
  REQUIRE(f.mUI->KeyDown(kVK_HOME));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(0.));
  REQUIRE(f.mKnob->GetValue() == Approx(0.));

  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin) == f.mDlg.CountParamEvents(ParamEvent::EType::End));
}

TEST_CASE("Keyboard navigation: arrow keys move a control linked to a stepped parameter one step at a time", "[integration][keyboard][focus][params]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);
  f.mUI->SetKeyboardFocus(f.mStepSlider);

  f.mUI->KeyDown(kVK_UP);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 3);
  f.mUI->KeyDown(kVK_DOWN);
  f.mUI->KeyDown(kVK_DOWN);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 1);
  f.mUI->KeyDown(kVK_PRIOR);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 10);
  f.mUI->KeyDown(kVK_HOME);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 0);
  f.mUI->KeyDown(kVK_DOWN);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 0);
}

TEST_CASE("Keyboard navigation: space and return click a focused toggle", "[integration][keyboard][focus][params]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);
  f.mUI->MouseMove(450, 250);
  f.mUI->SetKeyboardFocus(f.mToggle);

  REQUIRE(f.mUI->KeyDown(kVK_SPACE, " "));
  REQUIRE(f.mDlg.GetParam(kOnOff)->Bool());
  REQUIRE(f.mToggle->GetValue() == Approx(1.));
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kOnOff) == 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kOnOff) == 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End, kOnOff) == 1);

  REQUIRE(f.mUI->KeyDown(kVK_RETURN));
  REQUIRE_FALSE(f.mDlg.GetParam(kOnOff)->Bool());

  // Arrow keys mean nothing to a toggle
  REQUIRE_FALSE(f.mUI->KeyDown(kVK_UP));
  REQUIRE_FALSE(f.mDlg.GetParam(kOnOff)->Bool());
}

TEST_CASE("Keyboard navigation: arrow keys change the selection of a focused tab switch", "[integration][keyboard][focus][params]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);
  f.mUI->MouseMove(450, 250);
  f.mUI->SetKeyboardFocus(f.mTabs);

  // In a list of options down/right go to the next option
  REQUIRE(f.mUI->KeyDown(kVK_DOWN));
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 1);
  REQUIRE(f.mUI->KeyDown(kVK_RIGHT));
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 2);
  // ... and stop at the end
  REQUIRE(f.mUI->KeyDown(kVK_RIGHT));
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 2);
  REQUIRE(f.mUI->KeyDown(kVK_UP));
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 1);
  REQUIRE(f.mUI->KeyDown(kVK_LEFT));
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 0);
  REQUIRE(f.mTabs->GetValue() == Approx(0.));

  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kMode) == f.mDlg.CountParamEvents(ParamEvent::EType::End, kMode));
}

TEST_CASE("Keyboard navigation: ctrl and alt key combinations are left alone", "[integration][keyboard][focus]")
{
  Fixture f;
  f.InstallHandler(true);
  f.mUI->EnableKeyboardNavigation(true);
  f.mUI->MouseMove(450, 250);
  f.mUI->SetKeyboardFocus(f.mKnob);

  REQUIRE(f.mUI->KeyDown(kVK_UP, "", false, true));
  REQUIRE(f.mUI->KeyDown(kVK_TAB, "", false, false, true));

  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(50.));
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mKnob);
  REQUIRE(f.mHandlerKeys.size() == 2);
}

TEST_CASE("Keyboard navigation: the focused control gets the first chance to handle a key itself", "[integration][keyboard][focus]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);
  f.mA->SetAccessibleRole(EAccessibleRole::Slider);
  f.mA->mHandlesKeys = true;
  f.mUI->MouseMove(450, 250);
  f.mUI->SetKeyboardFocus(f.mA);

  REQUIRE(f.mUI->KeyDown(kVK_UP));
  REQUIRE(f.mA->Count("keydown") == 1);
  REQUIRE(f.mA->Last("keydown")->mVK == kVK_UP);
  // The control consumed the key, so the default adjustment was not applied
  REQUIRE(f.mA->GetValue() == Approx(0.));

  f.mA->mHandlesKeys = false;
  REQUIRE(f.mUI->KeyDown(kVK_UP));
  REQUIRE(f.mA->GetValue() == Approx(0.01));
}

TEST_CASE("Keyboard navigation: clicking a control focuses it without showing the ring until a key is used", "[integration][keyboard][focus][mouse][drawing]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);

  f.mUI->Click(f.mToggle->GetRECT().MW(), f.mToggle->GetRECT().MH());
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mToggle);
  REQUIRE(f.mDlg.GetParam(kOnOff)->Bool());
  REQUIRE_FALSE(f.RenderAndCheckRing(f.mToggle));

  REQUIRE(f.mUI->KeyDown(kVK_SPACE, " "));
  REQUIRE_FALSE(f.mDlg.GetParam(kOnOff)->Bool());
  REQUIRE(f.RenderAndCheckRing(f.mToggle));

  // Clicking a control that cannot take the focus leaves it where it was
  f.mUI->Click(50, 50);
  REQUIRE(f.mUI->GetKeyboardFocus() == f.mToggle);
}

TEST_CASE("Keyboard navigation: removing or hiding the focused control", "[integration][keyboard][focus][remove]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);
  f.mUI->MouseMove(450, 250);
  f.mUI->SetKeyboardFocus(f.mStepSlider);

  SECTION("removed: the focus is dropped")
  {
    f.mUI->RemoveControl(f.mStepSlider);
    REQUIRE(f.mUI->GetKeyboardFocus() == nullptr);
    REQUIRE_FALSE(f.mUI->KeyDown(kVK_UP));
    f.mUI->RenderFrame();
  }

  SECTION("hidden: keys are not applied to it")
  {
    f.mStepSlider->Hide(true);
    REQUIRE_FALSE(f.mUI->KeyDown(kVK_UP));
    REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 2);
    REQUIRE_FALSE(f.RenderAndCheckRing(f.mStepSlider));
  }

  SECTION("disabled: keys are not applied to it")
  {
    f.mStepSlider->SetDisabled(true);
    REQUIRE_FALSE(f.mUI->KeyDown(kVK_UP));
    REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 2);
  }
}

TEST_CASE("Keyboard navigation: turning it off drops the focus", "[integration][keyboard][focus]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);
  f.mUI->SetKeyboardFocus(f.mKnob);

  f.mUI->EnableKeyboardNavigation(false);
  REQUIRE(f.mUI->GetKeyboardFocus() == nullptr);
  REQUIRE_FALSE(f.RenderAndCheckRing(f.mKnob));
}

TEST_CASE("Keyboard navigation: GetKeyboardFocus reports the focused node", "[integration][keyboard][focus]")
{
  Fixture f;
  int nodeIdx = -1;

  REQUIRE(f.mUI->GetKeyboardFocus(&nodeIdx) == nullptr);
  REQUIRE(nodeIdx == 0);

  f.mUI->SetKeyboardFocus(f.mTabs, 2);
  REQUIRE(f.mUI->GetKeyboardFocus(&nodeIdx) == f.mTabs);
  REQUIRE(nodeIdx == 2);

  f.mUI->SetKeyboardFocus(nullptr, 5);
  REQUIRE(f.mUI->GetKeyboardFocus(&nodeIdx) == nullptr);
  REQUIRE(nodeIdx == 0);
}

#pragma mark - Modal controls

TEST_CASE("Modal control: restricts the keyboard to the modal control and its children", "[integration][keyboard][focus][modal]")
{
  Fixture f;
  f.mUI->EnableKeyboardNavigation(true);
  f.mA->mHandlesKeys = f.mB->mHandlesKeys = true;

  IControl* pInnerKnob = nullptr;
  IControl* pInnerToggle = nullptr;
  auto* pDialog = new IContainerBase(IRECT(300, 200, 500, 300),
    [&](IContainerBase* pContainer, const IRECT& bounds) {
      pInnerKnob = pContainer->AddChildControl(new IVKnobControl(bounds.FracRectHorizontal(0.5f, false), kGain, "", kPlainStyle));
      pInnerToggle = pContainer->AddChildControl(new IVToggleControl(bounds.FracRectHorizontal(0.5f, true), kOnOff, "", kPlainStyle));
    }, nullptr);
  f.mUI->AttachControl(pDialog);

  REQUIRE(f.mUI->GetModalControl() == nullptr);
  REQUIRE(f.mUI->ControlIsInModalScope(f.mKnob));

  f.mUI->SetKeyboardFocus(f.mKnob);
  f.mUI->SetModalControl(pDialog);

  REQUIRE(f.mUI->GetModalControl() == pDialog);
  REQUIRE(f.mUI->ControlIsInModalScope(pDialog));
  REQUIRE(f.mUI->ControlIsInModalScope(pInnerKnob));
  REQUIRE_FALSE(f.mUI->ControlIsInModalScope(f.mKnob));
  // The focus cannot stay outside the modal control
  REQUIRE(f.mUI->GetKeyboardFocus() == nullptr);

  // Tab only visits the controls inside
  f.mUI->KeyDown(kVK_TAB);
  REQUIRE(f.mUI->GetKeyboardFocus() == pInnerKnob);
  f.mUI->KeyDown(kVK_TAB);
  REQUIRE(f.mUI->GetKeyboardFocus() == pInnerToggle);
  f.mUI->KeyDown(kVK_TAB);
  REQUIRE(f.mUI->GetKeyboardFocus() == pInnerKnob);
  f.mUI->KeyDown(kVK_ESCAPE);

  // Keys are not delivered to a control outside the modal control, even with the pointer over it
  f.mUI->MouseMove(50, 50);
  REQUIRE_FALSE(f.mUI->KeyDown('A', "a"));
  REQUIRE(f.mA->Count("keydown") == 0);

  // Clicking outside does not move the focus out
  f.mUI->Click(f.mToggle->GetRECT().MW(), f.mToggle->GetRECT().MH());
  REQUIRE(f.mUI->GetKeyboardFocus() != f.mToggle);

  SECTION("ending the modal state gives the keyboard back to the rest of the UI")
  {
    f.mUI->SetModalControl(nullptr);
    REQUIRE(f.mUI->GetModalControl() == nullptr);
    f.mUI->MouseMove(50, 50);
    REQUIRE(f.mUI->KeyDown('A', "a"));
    REQUIRE(f.mA->Count("keydown") == 1);
    f.mUI->MouseMove(450, 100);
    f.mUI->KeyDown(kVK_TAB);
    REQUIRE(f.mUI->GetKeyboardFocus() == f.mKnob);
  }

  SECTION("removing the modal control ends the modal state")
  {
    pDialog->RemoveChildControl(pInnerKnob);
    pDialog->RemoveChildControl(pInnerToggle);
    f.mUI->RemoveControl(pDialog);
    REQUIRE(f.mUI->GetModalControl() == nullptr);
    REQUIRE(f.mUI->ControlIsInModalScope(f.mKnob));
    f.mUI->MouseMove(50, 50);
    REQUIRE(f.mUI->KeyDown('A', "a"));
  }
}
