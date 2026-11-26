/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the remaining IGraphics services: dialogs, clipboard, cursor, bitmap cache, messages from controls

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestEditorDelegate.h"
#include "TestControls.h"
#include "IControls.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;

namespace
{
struct Fixture
{
  Fixture()
  : mDlg(1, 400, 300)
  {
    mDlg.GetParam(0)->InitDouble("Gain", 50., 0., 100., 0.01, "%");
    mUI = mDlg.OpenUI();
    mUI->AttachPanelBackground(COLOR_GRAY);
    mProbe = mUI->AttachControl(new ProbeControl(IRECT(20, 20, 120, 120)))->As<ProbeControl>();
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
  ProbeControl* mProbe = nullptr;
};
} // namespace

TEST_CASE("IGraphics: reports the configuration it was created with", "[integration][misc]")
{
  TestEditorDelegate dlg {0, 640, 480};
  dlg.SetMakeGraphicsFunc([&dlg]() { return new IGraphicsHeadless(dlg, 640, 480, 30, 1.5f); });
  auto* pUI = dlg.OpenUI();

  REQUIRE(pUI->Width() == 640);
  REQUIRE(pUI->Height() == 480);
  REQUIRE(pUI->FPS() == 30);
  REQUIRE(pUI->GetDrawScale() == Approx(1.5f));
  REQUIRE(pUI->WindowWidth() == 960);
  REQUIRE(pUI->WindowHeight() == 720);
  REQUIRE(pUI->GetDelegate() == &dlg);
  REQUIRE(std::string(pUI->GetDrawingAPIStr()) == "Headless");
}

TEST_CASE("ShowMessageBox: the completion handler receives the button that was pressed", "[integration][misc][dialogs]")
{
  Fixture f;
  int nCalls = 0;
  EMsgBoxResult received = kNoResult;

  f.mUI->mMessageBoxResult = kYES;
  const EMsgBoxResult returned = f.mUI->ShowMessageBox("Really?", "Confirm", kMB_YESNO, [&](EMsgBoxResult result) { nCalls++; received = result; });

  REQUIRE(nCalls == 1);
  REQUIRE(received == kYES);
  REQUIRE(returned == kYES);
  REQUIRE(f.mUI->mLastMessageBoxText == "Really?");

  // The handler is optional
  f.mUI->mMessageBoxResult = kCANCEL;
  REQUIRE(f.mUI->ShowMessageBox("Again", "Confirm", kMB_OKCANCEL) == kCANCEL);
}

TEST_CASE("PromptForFile: the completion handler receives the chosen file", "[integration][misc][dialogs]")
{
  Fixture f;
  WDL_String fileName, path;
  std::string receivedFile, receivedPath;
  int nCalls = 0;
  auto handler = [&](const WDL_String& file, const WDL_String& dir) { nCalls++; receivedFile = file.Get(); receivedPath = dir.Get(); };

  f.mUI->mFileDialogFileName = "preset.fxp";
  f.mUI->mFileDialogPath = "/presets";
  f.mUI->PromptForFile(fileName, path, EFileAction::Open, "fxp", handler);

  REQUIRE(nCalls == 1);
  REQUIRE(receivedFile == "preset.fxp");
  REQUIRE(receivedPath == "/presets");
  REQUIRE(std::string(fileName.Get()) == "preset.fxp");
  REQUIRE(f.mUI->mLastFileDialogExt == "fxp");

  // A cancelled dialog reports an empty file name
  f.mUI->mFileDialogFileName.clear();
  f.mUI->PromptForFile(fileName, path, EFileAction::Save, "fxp", handler);
  REQUIRE(nCalls == 2);
  REQUIRE(receivedFile.empty());
  REQUIRE(f.mUI->mNumFileDialogs == 2);
}

TEST_CASE("Clipboard: text can be put on and read back from the clipboard", "[integration][misc][clipboard]")
{
  Fixture f;
  WDL_String str;

  REQUIRE_FALSE(f.mUI->GetTextFromClipboard(str));
  REQUIRE(f.mUI->SetTextInClipboard("copied text"));
  REQUIRE(f.mUI->GetTextFromClipboard(str));
  REQUIRE(std::string(str.Get()) == "copied text");
}

TEST_CASE("OpenURL: is passed to the platform", "[integration][misc]")
{
  Fixture f;

  REQUIRE(f.mUI->OpenURL("https://github.com/iPlug2/iPlug2"));
  REQUIRE(f.mUI->mLastOpenedURL == "https://github.com/iPlug2/iPlug2");
}

TEST_CASE("SetMouseCursor: returns the previous cursor", "[integration][misc][cursor]")
{
  Fixture f;

  REQUIRE(f.mUI->SetMouseCursor(ECursor::HAND) == ECursor::ARROW);
  REQUIRE(f.mUI->SetMouseCursor(ECursor::IBEAM) == ECursor::HAND);
  REQUIRE(f.mUI->SetMouseCursor() == ECursor::IBEAM);

  // The pointer leaving the window resets the cursor, and remembers it for when it comes back
  f.mUI->SetMouseCursor(ECursor::HAND);
  f.mUI->OnMouseOut();
  f.mUI->OnSetCursor();
  REQUIRE(f.mUI->SetMouseCursor() == ECursor::HAND);
}

TEST_CASE("HideMouseCursor: hides and shows the cursor, and a locked cursor is unlocked when shown", "[integration][misc][cursor]")
{
  Fixture f;

  f.mUI->HideMouseCursor(true, true);
  REQUIRE(f.mUI->mCursorHidden);
  REQUIRE(f.mUI->mCursorLocked);

  f.mUI->HideMouseCursor(false);
  REQUIRE_FALSE(f.mUI->mCursorHidden);
  REQUIRE_FALSE(f.mUI->mCursorLocked);

  f.mUI->HideMouseCursor(true, false);
  REQUIRE(f.mUI->mCursorHidden);
  REQUIRE_FALSE(f.mUI->mCursorLocked);
  f.mUI->HideMouseCursor(false);
}

TEST_CASE("Messages from controls: SendArbitraryMsgFromUI reaches the delegate with its tags and data", "[integration][misc][messages]")
{
  Fixture f;
  const int payload[2] = {7, 9};

  f.mProbe->GetDelegate()->SendArbitraryMsgFromUI(3, 12, sizeof(payload), payload);

  REQUIRE(f.mDlg.mMessages.size() == 1);
  REQUIRE(f.mDlg.mMessages[0].mMsgTag == 3);
  REQUIRE(f.mDlg.mMessages[0].mCtrlTag == 12);
  REQUIRE(f.mDlg.mMessages[0].mData.size() == sizeof(payload));
  REQUIRE(memcmp(f.mDlg.mMessages[0].mData.data(), payload, sizeof(payload)) == 0);
}

TEST_CASE("SetQwertyMidiKeyHandlerFunc: computer keys play notes through the delegate", "[integration][misc][keyboard][midi]")
{
  Fixture f;
  std::vector<IMidiMsg> received;
  f.mUI->SetQwertyMidiKeyHandlerFunc([&](const IMidiMsg& msg) { received.push_back(msg); });
  f.mUI->MouseMove(390, 290);

  REQUIRE(f.mUI->KeyDown(kVK_A, "a"));
  REQUIRE(f.mDlg.mMidiFromUI.size() == 1);
  REQUIRE(f.mDlg.mMidiFromUI[0].StatusMsg() == IMidiMsg::kNoteOn);
  const int root = f.mDlg.mMidiFromUI[0].NoteNumber();

  // Key repeat does not retrigger the note
  REQUIRE(f.mUI->KeyDown(kVK_A, "a"));
  REQUIRE(f.mDlg.mMidiFromUI.size() == 1);

  // The row of keys is laid out like a piano: S is a tone above A, W is the black key between them
  f.mUI->KeyDown(kVK_S, "s");
  f.mUI->KeyDown(kVK_W, "w");
  REQUIRE(f.mDlg.mMidiFromUI[1].NoteNumber() == root + 2);
  REQUIRE(f.mDlg.mMidiFromUI[2].NoteNumber() == root + 1);

  f.mUI->KeyUp(kVK_A, "a");
  REQUIRE(f.mDlg.mMidiFromUI.back().StatusMsg() == IMidiMsg::kNoteOff);
  REQUIRE(f.mDlg.mMidiFromUI.back().NoteNumber() == root);
  f.mUI->KeyUp(kVK_S, "s");
  f.mUI->KeyUp(kVK_W, "w");

  // X and Z shift the octave up and down
  f.mDlg.mMidiFromUI.clear();
  f.mUI->KeyDown(kVK_X, "x");
  f.mUI->KeyDown(kVK_A, "a");
  REQUIRE(f.mDlg.mMidiFromUI.back().NoteNumber() == root + 12);
  f.mUI->KeyUp(kVK_A, "a");
  f.mUI->KeyDown(kVK_Z, "z");
  f.mUI->KeyDown(kVK_A, "a");
  REQUIRE(f.mDlg.mMidiFromUI.back().NoteNumber() == root);
  f.mUI->KeyUp(kVK_A, "a");

  // The function given gets the same messages as the delegate
  REQUIRE(received.size() == 6 + f.mDlg.mMidiFromUI.size());

  // Other keys are swallowed without making a sound
  const size_t n = f.mDlg.mMidiFromUI.size();
  REQUIRE(f.mUI->KeyDown(kVK_F5, ""));
  REQUIRE(f.mDlg.mMidiFromUI.size() == n);
}

TEST_CASE("OnAppearanceChanged: calls the function registered for appearance changes", "[integration][misc]")
{
  Fixture f;
  int nCalls = 0;
  EUIAppearance received = EUIAppearance::Light;

  f.mUI->OnAppearanceChanged(EUIAppearance::Dark); // nothing registered: ignored
  f.mUI->SetUIAppearanceChangedFunc([&](EUIAppearance appearance) { nCalls++; received = appearance; });
  f.mUI->OnAppearanceChanged(EUIAppearance::Dark);

  REQUIRE(nCalls == 1);
  REQUIRE(received == EUIAppearance::Dark);
  REQUIRE(f.mUI->GetUIAppearance() == EUIAppearance::Light);
}

TEST_CASE("Platform context and shared resources path are stored", "[integration][misc]")
{
  Fixture f;
  int context = 0;

  REQUIRE(f.mUI->GetPlatformContext() == nullptr);
  f.mUI->SetPlatformContext(&context);
  REQUIRE(f.mUI->GetPlatformContext() == &context);

  f.mUI->SetSharedResourcesSubPath("MyPlugin/Resources");
  REQUIRE(std::string(f.mUI->GetSharedResourcesSubPath()) == "MyPlugin/Resources");
}

TEST_CASE("Bitmaps: RetainBitmap keeps a bitmap in the cache under a name", "[integration][misc][bitmap][cache]")
{
  Fixture f;
  const std::vector<uint8_t> png = MakePNGHeader(32, 16);
  const std::vector<uint8_t> other = MakePNGHeader(8, 8);

  // A layer's bitmap can be kept as a named bitmap
  f.mUI->StartLayer(nullptr, IRECT(0, 0, 50, 30));
  ILayerPtr layer = f.mUI->EndLayer();
  IBitmap layerBitmap = layer->GetBitmap();
  REQUIRE(layerBitmap.W() == 50);
  REQUIRE(layerBitmap.H() == 30);

  IBitmap loaded = f.mUI->LoadBitmap("retained.png", png.data(), static_cast<int>(png.size()));
  REQUIRE(loaded.W() == 32);

  // Loading the same name again gives the cached bitmap, whatever data is passed
  IBitmap again = f.mUI->LoadBitmap("retained.png", other.data(), static_cast<int>(other.size()));
  REQUIRE(again.GetAPIBitmap() == loaded.GetAPIBitmap());

  f.mUI->ReleaseBitmap(loaded);
  IBitmap reloaded = f.mUI->LoadBitmap("retained.png", other.data(), static_cast<int>(other.size()));
  REQUIRE(reloaded.W() == 8);
}

TEST_CASE("Bitmaps: a bitmap is scaled for the screen when no resource exists at that scale", "[integration][misc][bitmap][scale]")
{
  Fixture f;
  const std::vector<uint8_t> png = MakePNGHeader(40, 20);
  IBitmap bitmap = f.mUI->LoadBitmap("to_scale.png", png.data(), static_cast<int>(png.size()), 2, true);
  REQUIRE(bitmap.GetScale() == Approx(1.f));

  f.mUI->SetScreenScale(2.f);
  IBitmap scaled = f.mUI->GetScaledBitmap(bitmap);

  REQUIRE(scaled.IsValid());
  REQUIRE(scaled.GetScale() == Approx(2.f));
  // The same size in points, with twice the pixels
  REQUIRE(scaled.W() == 40);
  REQUIRE(scaled.H() == 20);
  REQUIRE(scaled.GetAPIBitmap()->GetWidth() == 80);
  REQUIRE(scaled.GetAPIBitmap()->GetHeight() == 40);
  REQUIRE(scaled.N() == 2);
  REQUIRE(scaled.GetFramesAreHorizontal());

  // Asking again gives the cached scaled bitmap
  IBitmap scaledAgain = f.mUI->GetScaledBitmap(bitmap);
  REQUIRE(scaledAgain.GetAPIBitmap() == scaled.GetAPIBitmap());

  // The UI's own scale is not disturbed by the scaling
  REQUIRE(f.mUI->GetScreenScale() == Approx(2.f));
  REQUIRE(f.mUI->GetDrawScale() == Approx(1.f));
}

TEST_CASE("Tooltips: a control keeps the tooltip it is given", "[integration][misc][tooltips]")
{
  Fixture f;

  REQUIRE(std::string(f.mProbe->GetTooltip()).empty());
  REQUIRE(f.mProbe->SetTooltip("Does a thing") == f.mProbe);
  REQUIRE(std::string(f.mProbe->GetTooltip()) == "Does a thing");
}

TEST_CASE("Text entry state: IsInPlatformTextEntry is only true for an entry made by the platform", "[integration][misc][textentry]")
{
  Fixture f;

  REQUIRE_FALSE(f.mUI->IsInPlatformTextEntry());
  f.mUI->CreateTextEntry(*f.mProbe, DEFAULT_TEXT, f.mProbe->GetRECT(), "abc");
  REQUIRE(f.mUI->IsInPlatformTextEntry());
  REQUIRE(f.mUI->GetControlInTextEntry() == f.mProbe);
  REQUIRE(f.mUI->mTextEntryStr == "abc");

  // Drags are not delivered to controls while the platform text entry has the focus
  f.mUI->MouseDown(50, 50);
  f.mUI->MouseDragTo(60, 60);
  REQUIRE(f.mProbe->Count("drag") == 0);
  f.mUI->MouseUp(60, 60);

  f.mUI->SetControlValueAfterTextEdit("xyz");
  REQUIRE(f.mProbe->Last("textentry")->mStr == "xyz");
  REQUIRE_FALSE(f.mUI->IsInPlatformTextEntry());

  f.mUI->CreateTextEntry(*f.mProbe, DEFAULT_TEXT, f.mProbe->GetRECT(), "abc");
  f.mUI->ClearInTextEntryControl();
  REQUIRE(f.mUI->GetControlInTextEntry() == nullptr);
}

TEST_CASE("Idle: OnGUIIdle is passed to every control", "[integration][misc]")
{
  Fixture f;

  f.mUI->OnGUIIdle();
  f.mUI->OnGUIIdle();
  REQUIRE(f.mProbe->mNumIdles == 2);
}
