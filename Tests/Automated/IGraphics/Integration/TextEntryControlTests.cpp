/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests ITextEntryControl, the text entry box that is drawn by IGraphics instead of by the platform

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestEditorDelegate.h"
#include "TestControls.h"
#include "IControls.h"
#include "ITextEntryControl.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;
using DrawOp = IGraphicsHeadless::DrawOp;

namespace
{
enum EParams { kGain = 0, kSteps, kNumParams };

const IRECT kTextBounds = IRECT(50, 50, 250, 80);
const IRECT kCaptionBounds = IRECT(50, 100, 250, 130);
const IRECT kStepCaptionBounds = IRECT(50, 150, 250, 180);
const IRECT kProbeBounds = IRECT(300, 200, 380, 280);

struct Fixture
{
  Fixture()
  : mDlg(kNumParams, 400, 300)
  {
    mDlg.GetParam(kGain)->InitDouble("Gain", 50., 0., 100., 0.01, "%");
    mDlg.GetParam(kSteps)->InitInt("Steps", 2, 0, 10);
    mUI = mDlg.OpenUI();
    mUI->AttachPanelBackground(COLOR_GRAY);
    mText = mUI->AttachControl(new IEditableTextControl(kTextBounds, "Name"))->As<IEditableTextControl>();
    mCaption = mUI->AttachControl(new ICaptionControl(kCaptionBounds, kGain, DEFAULT_TEXT, DEFAULT_BGCOLOR, false));
    mStepCaption = mUI->AttachControl(new ICaptionControl(kStepCaptionBounds, kSteps, DEFAULT_TEXT, DEFAULT_BGCOLOR, false));
    mProbe = mUI->AttachControl(new ProbeControl(kProbeBounds))->As<ProbeControl>();
    mUI->AttachTextEntryControl();
    mEntry = mUI->GetTextEntryControl();
    mDlg.SendCurrentParamValuesFromDelegate();
    mUI->RenderUntilClean();
    mUI->ClearRecorded();
    mDlg.ClearRecorded();
  }

  /** Start editing the free text control */
  void EditText()
  {
    mUI->Click(kTextBounds.MW(), kTextBounds.MH());
    REQUIRE(mEntry->EditInProgress());
  }

  /** The string that the text entry is showing ("" if it draws none) */
  std::string EntryString()
  {
    mUI->ClearRecorded();
    mUI->SetAllControlsDirty();
    mUI->RenderFrame();

    // The text entry is drawn on top of everything else
    const DrawOp* pText = mUI->LastOp(DrawOp::EType::Text);

    if (pText && pText->mBounds == mEntry->GetRECT())
      return pText->mText;

    return "";
  }

  void Ctrl(int vk) { REQUIRE(mUI->KeyDown(vk, "", false, true)); }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
  IEditableTextControl* mText = nullptr;
  IControl* mCaption = nullptr;
  IControl* mStepCaption = nullptr;
  ProbeControl* mProbe = nullptr;
  ITextEntryControl* mEntry = nullptr;
};
} // namespace

TEST_CASE("ITextEntryControl: replaces the platform text entry, and opens over the control with its text", "[integration][controls][textentry]")
{
  Fixture f;

  REQUIRE(f.mEntry != nullptr);
  REQUIRE_FALSE(f.mEntry->EditInProgress());

  f.mUI->Click(kTextBounds.MW(), kTextBounds.MH());

  REQUIRE(f.mUI->mNumTextEntries == 0);
  REQUIRE(f.mEntry->EditInProgress());
  REQUIRE(f.mEntry->GetRECT() == kTextBounds);
  REQUIRE(f.mUI->GetControlInTextEntry() == f.mText);
  REQUIRE_FALSE(f.mUI->IsInPlatformTextEntry());
  REQUIRE(f.EntryString() == "Name");

  // Attaching a second time keeps the first
  f.mUI->AttachTextEntryControl();
  REQUIRE(f.mUI->GetTextEntryControl() == f.mEntry);
}

TEST_CASE("ITextEntryControl: typing replaces the selected text and return commits it", "[integration][controls][textentry][keyboard]")
{
  Fixture f;
  f.EditText();

  f.mUI->TypeText("Hello World 2");
  REQUIRE(f.EntryString() == "Hello World 2");
  // Nothing is committed until return is pressed
  REQUIRE(std::string(f.mText->GetStr()) == "Name");

  f.mUI->KeyDown(kVK_RETURN);

  REQUIRE(std::string(f.mText->GetStr()) == "Hello World 2");
  REQUIRE_FALSE(f.mEntry->EditInProgress());
  REQUIRE(f.mUI->GetControlInTextEntry() == nullptr);
  REQUIRE(f.mEntry->GetRECT().Empty());

  f.mUI->ClearRecorded();
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->DidDrawText("Hello World 2"));
}

TEST_CASE("ITextEntryControl: escape cancels the edit", "[integration][controls][textentry][keyboard]")
{
  Fixture f;
  f.EditText();

  f.mUI->TypeText("Changed");
  f.mUI->KeyDown(kVK_ESCAPE);

  REQUIRE(std::string(f.mText->GetStr()) == "Name");
  REQUIRE_FALSE(f.mEntry->EditInProgress());
  REQUIRE(f.mUI->GetControlInTextEntry() == nullptr);

  f.mUI->ClearRecorded();
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->DidDrawText("Name"));
  REQUIRE(f.mUI->CountOpsInside(IGraphicsHeadless::DrawOp::EType::Text, kTextBounds) == 1);
}

// The control stays in the control stack with an empty rectangle, which is still drawn when the origin is redrawn
TEST_CASE("ITextEntryControl: draws nothing once the edit has ended", "[integration][controls][textentry][drawing]")
{
  Fixture f;
  f.EditText();
  f.mUI->TypeText("Changed");
  f.mUI->KeyDown(kVK_ESCAPE);

  f.mUI->ClearRecorded();
  f.mUI->SetAllControlsDirty();
  f.mUI->RenderFrame();
  REQUIRE_FALSE(f.mUI->DidDrawText("Changed"));
}

TEST_CASE("ITextEntryControl: clicking outside commits the edit, and the click goes no further", "[integration][controls][textentry][mouse]")
{
  Fixture f;
  f.EditText();
  f.mUI->TypeText("Outside");

  f.mUI->Click(kProbeBounds.MW(), kProbeBounds.MH());

  REQUIRE(std::string(f.mText->GetStr()) == "Outside");
  REQUIRE_FALSE(f.mEntry->EditInProgress());
  REQUIRE(f.mProbe->Count("down") == 0);

  // The next click is an ordinary one
  f.mUI->Click(kProbeBounds.MW(), kProbeBounds.MH());
  REQUIRE(f.mProbe->Count("down") == 1);
}

TEST_CASE("ITextEntryControl: keys go to the text entry wherever the pointer is", "[integration][controls][textentry][keyboard]")
{
  Fixture f;
  f.mProbe->mHandlesKeys = true;
  f.EditText();

  f.mUI->MouseMove(kProbeBounds.MW(), kProbeBounds.MH());
  f.mUI->TypeText("abc");

  REQUIRE(f.EntryString() == "abc");
  REQUIRE(f.mProbe->Count("keydown") == 0);
}

TEST_CASE("ITextEntryControl: cursor and delete keys edit the text", "[integration][controls][textentry][keyboard]")
{
  Fixture f;
  f.EditText();
  f.mUI->TypeText("abcd");

  f.mUI->KeyDown(kVK_LEFT);
  f.mUI->KeyDown(kVK_BACK);
  REQUIRE(f.EntryString() == "abd");

  f.mUI->KeyDown(kVK_HOME);
  f.mUI->KeyDown(kVK_DELETE);
  REQUIRE(f.EntryString() == "bd");

  f.mUI->KeyDown(kVK_RIGHT);
  f.mUI->TypeText("X");
  REQUIRE(f.EntryString() == "bXd");

  f.mUI->KeyDown(kVK_END);
  f.mUI->TypeText("!");
  REQUIRE(f.EntryString() == "bXd!");

  f.mUI->KeyDown(kVK_RETURN);
  REQUIRE(std::string(f.mText->GetStr()) == "bXd!");
}

TEST_CASE("ITextEntryControl: copy, cut and paste use the clipboard", "[integration][controls][textentry][keyboard][clipboard]")
{
  Fixture f;
  f.EditText();
  f.mUI->TypeText("hello");

  // Nothing selected: nothing copied
  f.Ctrl('C');
  REQUIRE(f.mUI->mClipboard.empty());

  f.Ctrl('A');
  f.Ctrl('C');
  REQUIRE(f.mUI->mClipboard == "hello");
  REQUIRE(f.EntryString() == "hello");

  f.mUI->KeyDown(kVK_END);
  f.Ctrl('V');
  REQUIRE(f.EntryString() == "hellohello");

  // Select the last two characters with shift + left
  f.mUI->KeyDown(kVK_LEFT, "", true);
  f.mUI->KeyDown(kVK_LEFT, "", true);
  f.Ctrl('X');
  REQUIRE(f.mUI->mClipboard == "lo");
  REQUIRE(f.EntryString() == "hellohel");

  // Paste replaces the selection
  f.mUI->mClipboard = "pasted";
  f.Ctrl('A');
  f.Ctrl('V');
  REQUIRE(f.EntryString() == "pasted");

  f.mUI->KeyDown(kVK_RETURN);
  REQUIRE(std::string(f.mText->GetStr()) == "pasted");
}

TEST_CASE("ITextEntryControl: undo and redo", "[integration][controls][textentry][keyboard]")
{
  Fixture f;
  f.EditText();
  f.mUI->KeyDown(kVK_END);
  f.mUI->TypeText("!");
  REQUIRE(f.EntryString() == "Name!");

  f.Ctrl('Z');
  REQUIRE(f.EntryString() == "Name");

  REQUIRE(f.mUI->KeyDown('Z', "", true, true));
  REQUIRE(f.EntryString() == "Name!");
}

TEST_CASE("ITextEntryControl: a double click selects everything", "[integration][controls][textentry][mouse]")
{
  Fixture f;
  f.EditText();
  f.mUI->KeyDown(kVK_END);
  f.mUI->TypeText("s");
  REQUIRE(f.EntryString() == "Names");

  f.mUI->DblClick(kTextBounds.MW(), kTextBounds.MH());
  REQUIRE(f.mEntry->EditInProgress());
  f.mUI->TypeText("x");
  REQUIRE(f.EntryString() == "x");
}

TEST_CASE("ITextEntryControl: clicking in the text places the cursor", "[integration][controls][textentry][mouse]")
{
  Fixture f;
  f.EditText();
  f.mUI->TypeText("abcdef");

  // A click in the entry box moves the cursor instead of ending the edit; where exactly depends on the text layout
  f.mUI->Click(kTextBounds.MW(), kTextBounds.MH());
  REQUIRE(f.mEntry->EditInProgress());
  f.mUI->TypeText("X");

  const std::string str = f.EntryString();
  REQUIRE(str.size() == 7);
  REQUIRE(str.find('X') != std::string::npos);
  std::string without = str;
  without.erase(without.find('X'), 1);
  REQUIRE(without == "abcdef");
}

TEST_CASE("ITextEntryControl: return commits a typed value to the parameter", "[integration][controls][textentry][params]")
{
  Fixture f;

  f.mUI->Click(kCaptionBounds.MW(), kCaptionBounds.MH());
  REQUIRE(f.mEntry->EditInProgress());
  REQUIRE(f.mEntry->GetRECT() == kCaptionBounds);
  REQUIRE(f.EntryString() == "50.00");

  f.mUI->TypeText("12.5");
  REQUIRE(f.EntryString() == "12.5");
  f.mUI->KeyDown(kVK_RETURN);

  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(12.5));
  REQUIRE(f.mCaption->GetValue() == Approx(0.125));
  REQUIRE(f.mDlg.LastParamValue(kGain) == Approx(0.125));
  REQUIRE_FALSE(f.mEntry->EditInProgress());

  f.mUI->ClearRecorded();
  f.mUI->RenderFrame();
  REQUIRE(f.mUI->DidDrawText("12.50"));
}

TEST_CASE("ITextEntryControl: escape leaves the parameter alone", "[integration][controls][textentry][params]")
{
  Fixture f;

  f.mUI->Click(kCaptionBounds.MW(), kCaptionBounds.MH());
  f.mUI->TypeText("99");
  f.mUI->KeyDown(kVK_ESCAPE);

  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(50.));
  REQUIRE(f.mDlg.CountParamEvents(TestEditorDelegate::ParamEvent::EType::Value) == 0);
  REQUIRE_FALSE(f.mEntry->EditInProgress());
}

TEST_CASE("ITextEntryControl: only characters that make sense for the parameter type are accepted", "[integration][controls][textentry][params][keyboard]")
{
  Fixture f;

  SECTION("a double parameter takes digits, signs and a decimal point")
  {
    f.mUI->Click(kCaptionBounds.MW(), kCaptionBounds.MH());
    f.mUI->TypeText("7a5.x2%");
    REQUIRE(f.EntryString() == "75.2");
    f.mUI->KeyDown(kVK_RETURN);
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(75.2));
  }

  SECTION("an integer parameter takes digits and signs")
  {
    f.mUI->Click(kStepCaptionBounds.MW(), kStepCaptionBounds.MH());
    REQUIRE(f.EntryString() == "2");
    f.mUI->TypeText("x9.");
    REQUIRE(f.EntryString() == "9");
    f.mUI->KeyDown(kVK_RETURN);
    REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 9);
  }

  SECTION("a value outside the range is clamped by the parameter")
  {
    f.mUI->Click(kStepCaptionBounds.MW(), kStepCaptionBounds.MH());
    f.mUI->TypeText("77");
    f.mUI->KeyDown(kVK_RETURN);
    REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 10);
  }
}

TEST_CASE("ITextEntryControl: draws a highlight behind selected text", "[integration][controls][textentry][drawing]")
{
  Fixture f;
  f.EditText();

  auto countFillsInEntry = [&]() {
    f.EntryString();
    return f.mUI->CountOpsInside(DrawOp::EType::Fill, kTextBounds);
  };

  // Everything is selected when the entry opens
  const int withSelection = countFillsInEntry();
  REQUIRE(withSelection >= 1);

  bool drewSelection = false;
  for (auto& op : f.mUI->mDrawOps)
    drewSelection |= (op.mType == DrawOp::EType::Fill && op.mBounds.W() == Approx(4.f * DEFAULT_TEXT_SIZE * IGraphicsHeadless::kCharWidth) && kTextBounds.Contains(op.mBounds));
  REQUIRE(drewSelection);
}

TEST_CASE("ITextEntryControl: removing it goes back to the platform text entry", "[integration][controls][textentry]")
{
  Fixture f;

  f.mUI->RemoveTextEntryControl();
  REQUIRE(f.mUI->GetTextEntryControl() == nullptr);

  f.mUI->Click(kTextBounds.MW(), kTextBounds.MH());
  REQUIRE(f.mUI->mNumTextEntries == 1);
  REQUIRE(f.mUI->IsInPlatformTextEntry());
}

TEST_CASE("ITextEntryControl: closing the UI while editing is safe", "[integration][controls][textentry][lifecycle]")
{
  Fixture f;
  f.EditText();
  f.mUI->TypeText("abc");

  f.mDlg.CloseUI();
  REQUIRE(f.mDlg.GetUI() == nullptr);
}
