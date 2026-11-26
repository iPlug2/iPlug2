/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the IGEditorDelegate lifecycle: opening/closing the UI, restoring its size, and delegate calls with no UI

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
enum EParams { kGain = 0, kNumParams };
enum ECtrlTags { kCtrlTagKnob = 0, kCtrlTagProbe };

void Layout(IGraphics* pGraphics)
{
  pGraphics->AttachPanelBackground(COLOR_GRAY);
  pGraphics->AttachControl(new IVKnobControl(IRECT(100, 100, 200, 200), kGain), kCtrlTagKnob);
  pGraphics->AttachControl(new ProbeControl(IRECT(250, 100, 300, 150)), kCtrlTagProbe);
}

struct Fixture
{
  Fixture()
  : mDlg(kNumParams, 400, 300, Layout)
  {
    mDlg.GetParam(kGain)->InitDouble("Gain", 0., 0., 100., 0.01, "%");
  }

  TestEditorDelegate mDlg;
};
} // namespace

TEST_CASE("Editor: opening creates the graphics, runs the layout once and calls OnUIOpen", "[integration][lifecycle]")
{
  Fixture f;

  REQUIRE(f.mDlg.GetUI() == nullptr);
  REQUIRE(f.mDlg.mNumLayouts == 0);

  auto* pUI = f.mDlg.OpenUI();

  REQUIRE(pUI != nullptr);
  REQUIRE(f.mDlg.GetUI() == pUI);
  REQUIRE(pUI->GetDelegate() == &f.mDlg);
  REQUIRE(pUI->WindowIsOpen());
  REQUIRE(f.mDlg.mNumLayouts == 1);
  REQUIRE(f.mDlg.mNumUIOpens == 1);
  REQUIRE(f.mDlg.mNumUICloses == 0);
  REQUIRE(pUI->NControls() == 3);
  REQUIRE(pUI->Width() == 400);
  REQUIRE(pUI->Height() == 300);
  REQUIRE(pUI->GetDrawScale() == Approx(1.f));
  REQUIRE(f.mDlg.GetEditorWidth() == 400);
  REQUIRE(f.mDlg.GetEditorHeight() == 300);
}

TEST_CASE("Editor: the first frame after opening draws everything, the next draws nothing", "[integration][lifecycle]")
{
  Fixture f;
  auto* pUI = f.mDlg.OpenUI();
  auto* pProbe = pUI->GetControlWithTag(kCtrlTagProbe)->As<ProbeControl>();

  REQUIRE(pUI->RenderFrame());
  REQUIRE(pProbe->mNumDraws == 1);
  REQUIRE(pUI->mLastDirtyBounds.Contains(pUI->GetBounds()));
  REQUIRE_FALSE(pUI->RenderFrame());
  REQUIRE(pProbe->mNumDraws == 1);
}

TEST_CASE("Editor: controls show the current parameter values when the UI opens", "[integration][lifecycle][params]")
{
  Fixture f;
  f.mDlg.GetParam(kGain)->Set(75.);

  auto* pUI = f.mDlg.OpenUI();

  REQUIRE(pUI->GetControlWithTag(kCtrlTagKnob)->GetValue() == Approx(0.75));
  // Pushing the values to the UI is not a change made by the user
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("Editor: closing calls OnUIClose and destroys the graphics", "[integration][lifecycle]")
{
  Fixture f;
  f.mDlg.OpenUI();
  f.mDlg.CloseUI();

  REQUIRE(f.mDlg.GetUI() == nullptr);
  REQUIRE(f.mDlg.UI() == nullptr);
  REQUIRE(f.mDlg.mNumUICloses == 1);
}

TEST_CASE("Editor: closing a closed editor does nothing", "[integration][lifecycle]")
{
  Fixture f;
  f.mDlg.OpenUI();
  f.mDlg.CloseUI();
  REQUIRE(f.mDlg.mNumUICloses == 1);

  f.mDlg.CloseWindow();
  REQUIRE(f.mDlg.mNumUICloses == 1);
  REQUIRE(f.mDlg.GetUI() == nullptr);
}

TEST_CASE("Editor: const GetUI() matches the non-const one", "[integration][lifecycle]")
{
  Fixture f;
  const TestEditorDelegate& constDlg = f.mDlg;

  REQUIRE(constDlg.GetUI() == nullptr);
  auto* pUI = f.mDlg.OpenUI();
  REQUIRE(constDlg.GetUI() == pUI);
}

TEST_CASE("Editor: the UI can be opened and closed repeatedly", "[integration][lifecycle]")
{
  Fixture f;

  for (int i = 1; i <= 3; i++)
  {
    auto* pUI = f.mDlg.OpenUI();
    REQUIRE(pUI != nullptr);
    REQUIRE(pUI->NControls() == 3);
    REQUIRE(f.mDlg.mNumLayouts == i);
    REQUIRE(f.mDlg.mNumUIOpens == i);
    REQUIRE(pUI->RenderFrame());
    f.mDlg.CloseUI();
    REQUIRE(f.mDlg.mNumUICloses == i);
    REQUIRE(f.mDlg.GetUI() == nullptr);
  }
}

TEST_CASE("Editor: reopening restores the size and scale the UI had when it was closed", "[integration][lifecycle][resize]")
{
  Fixture f;
  auto* pUI = f.mDlg.OpenUI();

  pUI->Resize(500, 350, 1.5f);
  REQUIRE(pUI->Width() == 500);
  f.mDlg.CloseUI();

  pUI = f.mDlg.OpenUI();
  REQUIRE(pUI != nullptr);
  REQUIRE(pUI->Width() == 500);
  REQUIRE(pUI->Height() == 350);
  REQUIRE(pUI->GetDrawScale() == Approx(1.5f));
  REQUIRE(pUI->WindowWidth() == 750);
  REQUIRE(pUI->WindowHeight() == 525);
  REQUIRE(f.mDlg.GetEditorWidth() == 750);
  REQUIRE(f.mDlg.GetEditorHeight() == 525);
  // The layout runs against the restored size
  REQUIRE(pUI->GetControl(0)->GetRECT() == IRECT(0, 0, 500, 350));
}

TEST_CASE("Editor: SerializeEditorSize/UnserializeEditorSize round trip", "[integration][lifecycle][state]")
{
  IByteChunk chunk;

  {
    Fixture f;
    auto* pUI = f.mDlg.OpenUI();
    pUI->Resize(640, 480, 2.f);
    REQUIRE(f.mDlg.SerializeEditorSize(chunk));
    REQUIRE(chunk.Size() == static_cast<int>(2 * sizeof(int) + sizeof(float)));
  }

  SECTION("into a closed editor: applied when the UI is next opened")
  {
    Fixture f;
    const int endPos = f.mDlg.UnserializeEditorSize(chunk, 0);
    REQUIRE(endPos == chunk.Size());
    REQUIRE(f.mDlg.GetUI() == nullptr);

    auto* pUI = f.mDlg.OpenUI();
    REQUIRE(pUI->Width() == 640);
    REQUIRE(pUI->Height() == 480);
    REQUIRE(pUI->GetDrawScale() == Approx(2.f));
  }

  SECTION("into an open editor: applied immediately")
  {
    Fixture f;
    auto* pUI = f.mDlg.OpenUI();
    const int endPos = f.mDlg.UnserializeEditorSize(chunk, 0);
    REQUIRE(endPos == chunk.Size());
    REQUIRE(pUI->Width() == 640);
    REQUIRE(pUI->Height() == 480);
    REQUIRE(pUI->GetDrawScale() == Approx(2.f));
    REQUIRE(f.mDlg.GetEditorWidth() == 1280);
    REQUIRE(f.mDlg.GetEditorHeight() == 960);
  }

  SECTION("a closed editor serializes the size it had when it was closed")
  {
    Fixture f;
    f.mDlg.UnserializeEditorSize(chunk, 0);
    f.mDlg.OpenUI();
    f.mDlg.CloseUI();

    IByteChunk chunk2;
    REQUIRE(f.mDlg.SerializeEditorSize(chunk2));
    REQUIRE(chunk2.IsEqual(chunk));
  }

  SECTION("the default editor state is the editor size")
  {
    Fixture f;
    REQUIRE(f.mDlg.UnserializeEditorState(chunk, 0) == chunk.Size());
    f.mDlg.OpenUI();

    IByteChunk chunk2;
    REQUIRE(f.mDlg.SerializeEditorState(chunk2));
    REQUIRE(chunk2.IsEqual(chunk));
  }
}

TEST_CASE("Editor: UnserializeEditorSize reads from the given start position", "[integration][lifecycle][state]")
{
  Fixture f;
  IByteChunk chunk;
  const int padding = 1234;
  chunk.Put(&padding);
  const int startPos = chunk.Size();
  const int w = 444, h = 333;
  const float scale = 1.f;
  chunk.Put(&w);
  chunk.Put(&h);
  chunk.Put(&scale);

  auto* pUI = f.mDlg.OpenUI();
  REQUIRE(f.mDlg.UnserializeEditorSize(chunk, startPos) == chunk.Size());
  REQUIRE(pUI->Width() == 444);
  REQUIRE(pUI->Height() == 333);
}

TEST_CASE("Editor: an empty saved size does not resize the UI", "[integration][lifecycle][state]")
{
  Fixture f;
  IByteChunk chunk;
  const int zero = 0;
  const float zeroScale = 0.f;
  chunk.Put(&zero);
  chunk.Put(&zero);
  chunk.Put(&zeroScale);

  SECTION("when open")
  {
    auto* pUI = f.mDlg.OpenUI();
    f.mDlg.UnserializeEditorSize(chunk, 0);
    REQUIRE(pUI->Width() == 400);
    REQUIRE(pUI->Height() == 300);
    REQUIRE(pUI->GetDrawScale() == Approx(1.f));
  }

  SECTION("when closed")
  {
    f.mDlg.UnserializeEditorSize(chunk, 0);
    auto* pUI = f.mDlg.OpenUI();
    REQUIRE(pUI->Width() == 400);
    REQUIRE(pUI->Height() == 300);
    REQUIRE(pUI->GetDrawScale() == Approx(1.f));
  }
}

TEST_CASE("Editor: delegate to UI calls are safe no-ops when the UI is closed", "[integration][lifecycle]")
{
  Fixture f;

  auto exercise = [&]() {
    const float data[2] = {1.f, 2.f};
    IMidiMsg msg;
    msg.MakeNoteOnMsg(60, 100, 0);

    f.mDlg.SendControlValueFromDelegate(kCtrlTagKnob, 0.5);
    f.mDlg.SendControlMsgFromDelegate(kCtrlTagProbe, 1, sizeof(data), data);
    f.mDlg.SendParameterValueFromDelegate(kGain, 0.5, true);
    f.mDlg.SendParameterValueFromDelegate(kGain, 50., false);
    f.mDlg.SendMidiMsgFromDelegate(msg);
    f.mDlg.OnParentWindowResize(800, 600);
    REQUIRE_FALSE(f.mDlg.OnKeyDown(IKeyPress("a", 'A')));
    REQUIRE_FALSE(f.mDlg.OnKeyUp(IKeyPress("a", 'A')));
    REQUIRE(f.mDlg.GetUI() == nullptr);
  };

  SECTION("before the UI was ever opened")
  {
    exercise();
  }

  SECTION("after the UI was closed")
  {
    f.mDlg.OpenUI();
    f.mDlg.CloseUI();
    f.mDlg.ClearRecorded();
    exercise();
  }

  // None of that told the host that the user changed something
  REQUIRE(f.mDlg.mParamEvents.empty());
  // ... but the delegate itself still hears about the parameter
  REQUIRE(f.mDlg.mParamChangesUI.size() == 2);
}

TEST_CASE("Editor: a delegate that creates no graphics does not open", "[integration][lifecycle]")
{
  Fixture f;
  f.mDlg.SetMakeGraphicsFunc(nullptr);

  REQUIRE(f.mDlg.OpenWindow(nullptr) == nullptr);
  REQUIRE(f.mDlg.GetUI() == nullptr);
  REQUIRE(f.mDlg.mNumUIOpens == 0);
  f.mDlg.CloseWindow();
}

TEST_CASE("Editor: a delegate with no layout function opens an empty UI", "[integration][lifecycle]")
{
  TestEditorDelegate dlg {0, 200, 100};
  auto* pUI = dlg.OpenUI();

  REQUIRE(pUI != nullptr);
  REQUIRE(pUI->NControls() == 0);
  REQUIRE_FALSE(pUI->RenderFrame());
  // Events on an empty UI go nowhere
  pUI->Click(10, 10);
  REQUIRE_FALSE(pUI->DblClick(10, 10));
  REQUIRE_FALSE(pUI->Wheel(10, 10, 1.f));
  REQUIRE_FALSE(pUI->KeyDown('A', "a"));
}

TEST_CASE("Editor: the screen scale given when the window opens is applied", "[integration][lifecycle][resize]")
{
  Fixture f;
  f.mDlg.SetMakeGraphicsFunc([&f]() {
    auto* pGraphics = new IGraphicsHeadless(f.mDlg, 400, 300);
    pGraphics->mInitialScreenScale = 2.f;
    return pGraphics;
  });

  auto* pUI = f.mDlg.OpenUI();
  REQUIRE(pUI->GetScreenScale() == Approx(2.f));
  REQUIRE(pUI->GetRoundedScreenScale() == 2);
  REQUIRE(pUI->GetTotalScale() == Approx(2.f));
  // The screen scale does not change the size of the UI in points
  REQUIRE(pUI->Width() == 400);
  REQUIRE(pUI->WindowWidth() == 400);
}

TEST_CASE("Editor: closing the UI while a control is captured or being edited is safe", "[integration][lifecycle][mouse]")
{
  Fixture f;
  auto* pUI = f.mDlg.OpenUI();

  pUI->MouseDown(150, 150);
  pUI->MouseDragTo(150, 120);
  REQUIRE(pUI->ControlIsCaptured());
  f.mDlg.CloseUI();
  REQUIRE(f.mDlg.GetUI() == nullptr);

  // and it comes back in a sane state
  pUI = f.mDlg.OpenUI();
  REQUIRE_FALSE(pUI->ControlIsCaptured());
  REQUIRE_FALSE(pUI->mCursorHidden);
}

#ifdef OS_LINUX
TEST_CASE("Editor (Linux): PrepareGraphics creates the UI model without opening a window", "[integration][lifecycle][linux]")
{
  Fixture f;

  REQUIRE(f.mDlg.PrepareGraphics());
  REQUIRE(f.mDlg.GetUI() != nullptr);
  REQUIRE_FALSE(f.mDlg.GetUI()->WindowIsOpen());
  REQUIRE(f.mDlg.mNumUIOpens == 0);
  REQUIRE(f.mDlg.mNumLayouts == 0);
  REQUIRE(f.mDlg.GetEditorWidth() == 400);
  REQUIRE(f.mDlg.GetEditorHeight() == 300);

  // Opening afterwards uses the same graphics
  IGraphics* pPrepared = f.mDlg.GetUI();
  REQUIRE(f.mDlg.OpenUI() == pPrepared);
  REQUIRE(f.mDlg.mNumUIOpens == 1);
  REQUIRE(f.mDlg.mNumLayouts == 1);
}

TEST_CASE("Editor (Linux): a host screen scale can be set before the window opens", "[integration][lifecycle][linux]")
{
  Fixture f;

  REQUIRE_FALSE(f.mDlg.HasHostScreenScale());
  REQUIRE_FALSE(f.mDlg.SetHostScreenScale(0.f));
  REQUIRE_FALSE(f.mDlg.SetHostScreenScale(-1.f));
  REQUIRE_FALSE(f.mDlg.HasHostScreenScale());

  REQUIRE(f.mDlg.SetHostScreenScale(2.f));
  REQUIRE(f.mDlg.HasHostScreenScale());
  REQUIRE(f.mDlg.GetUI() != nullptr);
  REQUIRE(f.mDlg.GetUI()->GetScreenScale() == Approx(2.f));
  // No window was open, so the host was not asked to resize anything
  REQUIRE(f.mDlg.mNumResizeRequests == 0);
}

TEST_CASE("Editor (Linux): a close requested during graphics dispatch is completed later", "[integration][lifecycle][linux]")
{
  Fixture f;
  auto* pUI = f.mDlg.OpenUI();

  REQUIRE_FALSE(f.mDlg.CloseWindowIfRequested());
  REQUIRE(f.mDlg.GetUI() == pUI);

  pUI->RequestWindowClose();
  REQUIRE(f.mDlg.CloseWindowIfRequested());
  REQUIRE(f.mDlg.GetUI() == nullptr);
  REQUIRE(f.mDlg.mNumUICloses == 1);
}
#endif
