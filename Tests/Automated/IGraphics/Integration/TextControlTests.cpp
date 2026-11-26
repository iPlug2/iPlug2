/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the text controls: ITextControl, IEditableTextControl, IURLControl, ITextToggleControl and ICaptionControl

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestEditorDelegate.h"
#include "TestControls.h"
#include "IControls.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;
using ParamEvent = TestEditorDelegate::ParamEvent;
using DrawOp = IGraphicsHeadless::DrawOp;

namespace
{
enum EParams { kGain = 0, kOnOff, kMode, kNumParams };

const IRECT kBounds = IRECT(100, 50, 300, 80);

struct Fixture
{
  Fixture()
  : mDlg(kNumParams, 400, 300)
  {
    mDlg.GetParam(kGain)->InitDouble("Gain", 50., 0., 100., 0.01, "%");
    mDlg.GetParam(kOnOff)->InitBool("OnOff", false);
    mDlg.GetParam(kMode)->InitEnum("Mode", 1, {"Sine", "Saw", "Square"});
    mUI = mDlg.OpenUI();
    mUI->AttachPanelBackground(COLOR_GRAY);
  }

  template <class T>
  T* Attach(T* pControl)
  {
    mUI->AttachControl(pControl);
    mDlg.SendCurrentParamValuesFromDelegate();
    mUI->RenderUntilClean();
    mUI->ClearRecorded();
    mDlg.ClearRecorded();
    return pControl;
  }

  void Redraw()
  {
    mUI->ClearRecorded();
    mUI->SetAllControlsDirty();
    mUI->RenderFrame();
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
};

bool SameColor(const IColor& a, const IColor& b)
{
  return a.A == b.A && a.R == b.R && a.G == b.G && a.B == b.B;
}
} // namespace

#pragma mark - ITextControl

TEST_CASE("ITextControl: draws its background and its string with its text style", "[integration][controls][text][drawing]")
{
  Fixture f;
  const IText text(18.f, COLOR_BLUE);
  f.Attach(new ITextControl(kBounds, "Hello", text, COLOR_YELLOW));

  f.Redraw();

  const DrawOp* pText = f.mUI->FindText("Hello");
  REQUIRE(pText != nullptr);
  REQUIRE(pText->mBounds == kBounds);
  REQUIRE(SameColor(pText->mColor, COLOR_BLUE));
  REQUIRE(pText->mTextSize == Approx(18.f));

  bool drewBackground = false;
  for (auto& op : f.mUI->mDrawOps)
    drewBackground |= (op.mType == DrawOp::EType::Fill && op.mBounds == kBounds && SameColor(op.mColor, COLOR_YELLOW));
  REQUIRE(drewBackground);
}

TEST_CASE("ITextControl: SetStr changes the text and redraws only when it is different", "[integration][controls][text][drawing]")
{
  Fixture f;
  auto* pText = f.Attach(new ITextControl(kBounds, "One"));

  REQUIRE(std::string(pText->GetStr()) == "One");

  pText->SetStr("One");
  REQUIRE_FALSE(pText->IsDirty());
  REQUIRE_FALSE(f.mUI->RenderFrame());

  pText->SetStr("Two");
  REQUIRE(std::string(pText->GetStr()) == "Two");
  REQUIRE(pText->IsDirty());
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->DidDrawText("Two"));
  REQUIRE_FALSE(f.mUI->DidDrawText("One"));
  REQUIRE(f.mUI->mLastDirtyBounds.Contains(kBounds));
}

TEST_CASE("ITextControl: SetStrFmt formats the text", "[integration][controls][text]")
{
  Fixture f;
  auto* pText = f.Attach(new ITextControl(kBounds, ""));

  pText->SetStrFmt(64, "%s = %d (%.1f)", "x", 42, 1.5);
  REQUIRE(std::string(pText->GetStr()) == "x = 42 (1.5)");
  REQUIRE(pText->IsDirty());

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("x = 42 (1.5)"));
}

TEST_CASE("ITextControl: an empty string draws no text", "[integration][controls][text][drawing]")
{
  Fixture f;
  auto* pText = f.Attach(new ITextControl(kBounds, "Something"));

  pText->ClearStr();
  REQUIRE(std::string(pText->GetStr()).empty());
  f.Redraw();
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Text) == 0);
}

TEST_CASE("ITextControl: can size itself to fit its string", "[integration][controls][text][measure]")
{
  Fixture f;
  const float size = 20.f;
  const float charWidth = size * IGraphicsHeadless::kCharWidth;
  auto* pText = f.Attach(new ITextControl(kBounds, "12345678", IText(size), DEFAULT_BGCOLOR, true));

  // Centred on the original bounds, as wide as eight characters and one line high
  REQUIRE(pText->GetRECT().W() == Approx(8.f * charWidth));
  REQUIRE(pText->GetRECT().H() == Approx(size));
  REQUIRE(pText->GetRECT().MW() == Approx(kBounds.MW()));
  REQUIRE(pText->GetRECT().MH() == Approx(kBounds.MH()));
  REQUIRE(pText->GetTargetRECT() == pText->GetRECT());

  pText->SetStr("1234");
  REQUIRE(pText->GetRECT().W() == Approx(4.f * charWidth));
  REQUIRE(pText->GetRECT().MW() == Approx(kBounds.MW()));

  pText->SetStr("1234567890123456");
  REQUIRE(pText->GetRECT().W() == Approx(16.f * charWidth));
  REQUIRE(pText->GetRECT().MW() == Approx(kBounds.MW()));
}

TEST_CASE("ITextControl: SetBoundsBasedOnStr fits the bounds on request", "[integration][controls][text][measure]")
{
  Fixture f;
  auto* pText = f.Attach(new ITextControl(kBounds, "abcd", IText(10.f)));

  REQUIRE(pText->GetRECT() == kBounds);
  pText->SetBoundsBasedOnStr();
  REQUIRE(pText->GetRECT().W() == Approx(4.f * 10.f * IGraphicsHeadless::kCharWidth));
  REQUIRE(pText->GetRECT().H() == Approx(10.f));

  // It does not follow later changes unless asked again
  pText->SetStr("abcdefgh");
  REQUIRE(pText->GetRECT().W() == Approx(4.f * 10.f * IGraphicsHeadless::kCharWidth));
}

TEST_CASE("ITextControl: is transparent to the mouse", "[integration][controls][text][mouse]")
{
  Fixture f;
  auto* pBelow = f.Attach(new ProbeControl(kBounds));
  auto* pText = f.Attach(new ITextControl(kBounds, "On top"));

  REQUIRE(pText->GetIgnoreMouse());
  f.mUI->Click(kBounds.MW(), kBounds.MH());
  REQUIRE(pBelow->Count("down") == 1);
}

#pragma mark - IEditableTextControl

TEST_CASE("IEditableTextControl: a click opens a text entry and the result becomes the text", "[integration][controls][text][textentry]")
{
  Fixture f;
  auto* pText = f.Attach(new IEditableTextControl(kBounds, "Edit me"));

  f.mUI->Click(kBounds.MW(), kBounds.MH());

  REQUIRE(f.mUI->mNumTextEntries == 1);
  REQUIRE(f.mUI->mTextEntryStr == "Edit me");
  REQUIRE(f.mUI->mTextEntryBounds == kBounds);
  REQUIRE(f.mUI->GetControlInTextEntry() == pText);
  REQUIRE(f.mUI->IsInPlatformTextEntry());

  // While it is being edited the control does not draw its own text underneath the entry box
  f.Redraw();
  REQUIRE_FALSE(f.mUI->DidDrawText("Edit me"));

  f.mUI->SetControlValueAfterTextEdit("Edited");

  REQUIRE(std::string(pText->GetStr()) == "Edited");
  REQUIRE(f.mUI->GetControlInTextEntry() == nullptr);
  REQUIRE(pText->IsDirty());
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Edited"));
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("IEditableTextControl: the action function runs when the text has been edited", "[integration][controls][text][textentry][action]")
{
  Fixture f;
  std::string received;
  auto* pText = f.Attach(new IEditableTextControl(kBounds, "Name"));
  pText->SetActionFunction([&](IControl* pCaller) { received = pCaller->As<IEditableTextControl>()->GetStr(); });

  f.mUI->Click(kBounds.MW(), kBounds.MH());
  f.mUI->SetControlValueAfterTextEdit("New name");

  REQUIRE(received == "New name");
}

#pragma mark - IURLControl

TEST_CASE("IURLControl: a click opens the URL", "[integration][controls][text][url]")
{
  Fixture f;
  f.Attach(new IURLControl(kBounds, "iPlug2", "https://iplug2.github.io"));

  REQUIRE(f.mUI->mLastOpenedURL.empty());
  f.mUI->MouseDown(kBounds.MW(), kBounds.MH());

  REQUIRE(f.mUI->mLastOpenedURL == "https://iplug2.github.io");
  // The browser takes over, so the control does not hold on to the mouse
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
  f.mUI->MouseUp(kBounds.MW(), kBounds.MH());
}

TEST_CASE("IURLControl: draws underlined text, coloured by its state", "[integration][controls][text][url][drawing][mouseover]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);
  const IColor normalColor = COLOR_BLUE;
  const IColor mouseOverColor = COLOR_GREEN;
  const IColor clickedColor = COLOR_RED;
  auto* pURL = f.Attach(new IURLControl(kBounds, "link", "https://example.com", IText(14.f, normalColor), DEFAULT_BGCOLOR, mouseOverColor, clickedColor));

  auto textColor = [&]() {
    f.Redraw();
    const DrawOp* pText = f.mUI->FindText("link");
    REQUIRE(pText != nullptr);
    // The underline is drawn in the same colour as the text
    const DrawOp* pLine = f.mUI->LastOp(DrawOp::EType::Stroke);
    REQUIRE(pLine != nullptr);
    REQUIRE(SameColor(pLine->mColor, pText->mColor));
    REQUIRE(pLine->mBounds.H() == Approx(0.f));
    REQUIRE(pLine->mBounds.W() == Approx(4.f * 14.f * IGraphicsHeadless::kCharWidth));
    return pText->mColor;
  };

  REQUIRE(SameColor(textColor(), normalColor));

  f.mUI->MouseMove(kBounds.MW(), kBounds.MH());
  REQUIRE(SameColor(textColor(), mouseOverColor));

  f.mUI->MouseMove(390, 290);
  REQUIRE(SameColor(textColor(), normalColor));

  f.mUI->Click(kBounds.MW(), kBounds.MH());
  f.mUI->MouseMove(390, 290);
  REQUIRE(SameColor(textColor(), clickedColor));

  pURL->SetCLColor(COLOR_ORANGE);
  REQUIRE(SameColor(textColor(), COLOR_ORANGE));
}

#pragma mark - ITextToggleControl

TEST_CASE("ITextToggleControl: a click toggles the parameter and the text", "[integration][controls][text][toggle][params]")
{
  Fixture f;
  auto* pToggle = f.Attach(new ITextToggleControl(kBounds, kOnOff, "Muted", "Playing"));

  REQUIRE(std::string(pToggle->GetStr()) == "Muted");

  f.mUI->Click(kBounds.MW(), kBounds.MH());
  REQUIRE(pToggle->GetValue() == Approx(1.));
  REQUIRE(f.mDlg.GetParam(kOnOff)->Bool());
  REQUIRE(std::string(pToggle->GetStr()) == "Playing");
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Playing"));

  f.mUI->Click(kBounds.MW(), kBounds.MH());
  REQUIRE_FALSE(f.mDlg.GetParam(kOnOff)->Bool());
  REQUIRE(std::string(pToggle->GetStr()) == "Muted");

  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kOnOff) == 2);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kOnOff) == 2);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End, kOnOff) == 2);
}

TEST_CASE("ITextToggleControl: a value from the host changes the text", "[integration][controls][text][toggle][delegate]")
{
  Fixture f;
  auto* pToggle = f.Attach(new ITextToggleControl(kBounds, kOnOff, "Muted", "Playing"));

  f.mDlg.SendParameterValueFromDelegate(kOnOff, 1., true);
  REQUIRE(std::string(pToggle->GetStr()) == "Playing");
  REQUIRE(pToggle->IsDirty());
  REQUIRE(f.mDlg.mParamEvents.empty());

  f.mDlg.SendParameterValueFromDelegate(kOnOff, 0., true);
  REQUIRE(std::string(pToggle->GetStr()) == "Muted");
}

TEST_CASE("ITextToggleControl: without a parameter it calls its action function", "[integration][controls][text][toggle][action]")
{
  Fixture f;
  std::vector<double> values;
  auto* pToggle = f.Attach(new ITextToggleControl(kBounds, [&](IControl* pCaller) { values.push_back(pCaller->GetValue()); }, "A", "B"));

  f.mUI->Click(kBounds.MW(), kBounds.MH());
  f.mUI->DblClick(kBounds.MW(), kBounds.MH());
  f.mUI->MouseUp(kBounds.MW(), kBounds.MH());

  REQUIRE(values == std::vector<double>{1., 0.});
  REQUIRE(std::string(pToggle->GetStr()) == "A");
}

#pragma mark - ICaptionControl

TEST_CASE("ICaptionControl: shows the parameter's display text, with or without its label", "[integration][controls][text][caption][drawing]")
{
  Fixture f;

  SECTION("with the label")
  {
    f.Attach(new ICaptionControl(kBounds, kGain));
    f.Redraw();
    REQUIRE(f.mUI->DidDrawText("50.00 %"));
  }

  SECTION("without the label")
  {
    f.Attach(new ICaptionControl(kBounds, kGain, DEFAULT_TEXT, DEFAULT_BGCOLOR, false));
    f.Redraw();
    REQUIRE(f.mUI->DidDrawText("50.00"));
  }

  SECTION("an enum parameter shows the name of the option, and a drop-down arrow")
  {
    f.Attach(new ICaptionControl(kBounds, kMode, DEFAULT_TEXT, DEFAULT_BGCOLOR, false));
    f.Redraw();
    REQUIRE(f.mUI->DidDrawText("Saw"));

    // background + arrow
    REQUIRE(f.mUI->CountOps(DrawOp::EType::Fill) >= 2);
    const DrawOp* pArrow = f.mUI->LastOp(DrawOp::EType::Fill);
    REQUIRE(kBounds.Contains(pArrow->mBounds));
    REQUIRE(pArrow->mBounds.MW() > kBounds.MW());
  }
}

TEST_CASE("ICaptionControl: follows the parameter", "[integration][controls][text][caption][delegate]")
{
  Fixture f;
  auto* pCaption = f.Attach(new ICaptionControl(kBounds, kGain));

  f.mDlg.GetParam(kGain)->Set(12.34);
  f.mDlg.SendCurrentParamValuesFromDelegate();

  REQUIRE(pCaption->IsDirty());
  f.mUI->ClearRecorded();
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->DidDrawText("12.34 %"));
}

TEST_CASE("ICaptionControl: clicking a numeric caption opens a text entry over the control", "[integration][controls][text][caption][textentry][params]")
{
  Fixture f;
  auto* pCaption = f.Attach(new ICaptionControl(kBounds, kGain));

  f.mUI->Click(kBounds.MW(), kBounds.MH());

  REQUIRE(f.mUI->mNumTextEntries == 1);
  REQUIRE(f.mUI->mNumPopupMenus == 0);
  REQUIRE(f.mUI->mTextEntryStr == "50.00");
  REQUIRE(f.mUI->mTextEntryBounds == kBounds);
  REQUIRE(f.mUI->GetControlInTextEntry() == pCaption);

  f.mUI->SetControlValueAfterTextEdit("80");
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(80.));
  REQUIRE(pCaption->GetValue() == Approx(0.8));
  REQUIRE(f.mDlg.LastParamValue(kGain) == Approx(0.8));

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("80.00 %"));
}

TEST_CASE("ICaptionControl: clicking an enum caption opens a menu, and the choice is one complete gesture", "[integration][controls][text][caption][popupmenu][params]")
{
  Fixture f;
  auto* pCaption = f.Attach(new ICaptionControl(kBounds, kMode, DEFAULT_TEXT, DEFAULT_BGCOLOR, false));

  SECTION("an option is chosen")
  {
    f.mUI->mPopupMenuChoice = 2;
    f.mUI->Click(kBounds.MW(), kBounds.MH());

    REQUIRE(f.mUI->mNumPopupMenus == 1);
    REQUIRE(f.mUI->mNumTextEntries == 0);
    REQUIRE(f.mUI->mPopupMenuItems == std::vector<std::string>{"Sine", "Saw", "Square"});
    REQUIRE(f.mUI->mPopupMenuBounds == kBounds);
    REQUIRE(f.mDlg.GetParam(kMode)->Int() == 2);
    REQUIRE(pCaption->GetValue() == Approx(1.));

    REQUIRE(f.mDlg.mParamEvents.size() == 3);
    REQUIRE(f.mDlg.mParamEvents[0].mType == ParamEvent::EType::Begin);
    REQUIRE(f.mDlg.mParamEvents[1].mType == ParamEvent::EType::Value);
    REQUIRE(f.mDlg.mParamEvents[1].mNormalizedValue == Approx(1.));
    REQUIRE(f.mDlg.mParamEvents[2].mType == ParamEvent::EType::End);

    f.Redraw();
    REQUIRE(f.mUI->DidDrawText("Square"));
  }

  SECTION("the menu is dismissed")
  {
    f.mUI->mPopupMenuChoice = -1;
    f.mUI->Click(kBounds.MW(), kBounds.MH());

    REQUIRE(f.mUI->mNumPopupMenus == 1);
    REQUIRE(f.mDlg.GetParam(kMode)->Int() == 1);
    REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value) == 0);
    REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin) == f.mDlg.CountParamEvents(ParamEvent::EType::End));
  }

  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
}

#pragma mark - PlaceHolder

TEST_CASE("PlaceHolder: draws its position and size", "[integration][controls][text][placeholder][drawing]")
{
  Fixture f;
  auto* pPlaceHolder = f.Attach(new PlaceHolder(IRECT(100, 50, 300, 150), "TODO"));

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("TODO"));
  REQUIRE(f.mUI->DidDrawText("100.0, 50.0"));
  REQUIRE(f.mUI->DidDrawText("200.0"));
  REQUIRE(f.mUI->DidDrawText("100.0"));

  pPlaceHolder->SetTargetAndDrawRECTs(IRECT(10, 20, 60, 50));
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("10.0, 20.0"));
  REQUIRE(f.mUI->DidDrawText("50.0"));
  REQUIRE(f.mUI->DidDrawText("30.0"));
}
