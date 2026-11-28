/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests resizing and scaling the UI: Resize, screen scale, size constraints, layout on resize, the corner resizer

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestEditorDelegate.h"
#include "TestControls.h"
#include "IControls.h"
#include "ICornerResizerControl.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;

namespace
{
enum ECtrlTags { kTagProbe = 0, kTagFill };

/** Attaches the controls the first time, and positions them relative to the UI bounds every time */
void Layout(IGraphics* pGraphics)
{
  const IRECT bounds = pGraphics->GetBounds();

  if (pGraphics->NControls() == 0)
  {
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->AttachControl(new ProbeControl(IRECT()), kTagProbe);
    pGraphics->AttachControl(new ProbeControl(IRECT()), kTagFill);
  }

  pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(bounds);
  pGraphics->GetControlWithTag(kTagProbe)->SetTargetAndDrawRECTs(bounds.GetFromTLHC(50.f, 50.f));
  pGraphics->GetControlWithTag(kTagFill)->SetTargetAndDrawRECTs(bounds.GetPadded(-100.f));
}

struct Fixture
{
  Fixture()
  : mDlg(0, 400, 300, Layout)
  {
    mUI = mDlg.OpenUI();
    mProbe = mUI->GetControlWithTag(kTagProbe)->As<ProbeControl>();
    mFill = mUI->GetControlWithTag(kTagFill)->As<ProbeControl>();
    mUI->RenderUntilClean();
    mUI->ClearRecorded();
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
  ProbeControl* mProbe = nullptr;
  ProbeControl* mFill = nullptr;
};
} // namespace

TEST_CASE("Resize: changes the size of the UI and tells the delegate, the platform and the controls", "[integration][resize]")
{
  Fixture f;
  const int nResizeRequests = f.mDlg.mNumResizeRequests;
  const int nPlatformResizes = f.mUI->mNumPlatformResizes;
  const int nDrawResizes = f.mUI->mNumDrawResizes;
  const int nControlResizes = f.mProbe->mNumResizes;

  f.mUI->Resize(500, 350, 1.f);

  REQUIRE(f.mUI->Width() == 500);
  REQUIRE(f.mUI->Height() == 350);
  REQUIRE(f.mUI->GetDrawScale() == Approx(1.f));
  REQUIRE(f.mUI->WindowWidth() == 500);
  REQUIRE(f.mUI->WindowHeight() == 350);
  REQUIRE(f.mUI->GetBounds() == IRECT(0, 0, 500, 350));

  REQUIRE(f.mDlg.mNumResizeRequests == nResizeRequests + 1);
  REQUIRE(f.mDlg.mLastResizeNeededPlatformResize);
  REQUIRE(f.mDlg.GetEditorWidth() == 500);
  REQUIRE(f.mDlg.GetEditorHeight() == 350);
  REQUIRE(f.mUI->mNumPlatformResizes == nPlatformResizes + 1);
  REQUIRE(f.mUI->mNumDrawResizes == nDrawResizes + 1);
  REQUIRE(f.mProbe->mNumResizes == nControlResizes + 1);

  // Everything is redrawn at the new size
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->mLastDirtyBounds.Contains(f.mProbe->GetRECT()));
}

TEST_CASE("Resize: a draw scale stretches the window without changing the UI's own coordinates", "[integration][resize][scale]")
{
  Fixture f;

  f.mUI->Resize(400, 300, 1.5f);

  REQUIRE(f.mUI->Width() == 400);
  REQUIRE(f.mUI->Height() == 300);
  REQUIRE(f.mUI->GetDrawScale() == Approx(1.5f));
  REQUIRE(f.mUI->WindowWidth() == 600);
  REQUIRE(f.mUI->WindowHeight() == 450);
  REQUIRE(f.mUI->GetTotalScale() == Approx(1.5f));
  REQUIRE(f.mDlg.GetEditorWidth() == 600);
  REQUIRE(f.mDlg.GetEditorHeight() == 450);

  // Controls keep their bounds, and mouse events are still in UI coordinates
  REQUIRE(f.mProbe->GetRECT() == IRECT(0, 0, 50, 50));
  f.mUI->Click(25, 25);
  REQUIRE(f.mProbe->Count("down") == 1);
}

TEST_CASE("Resize: resizing to the current size and scale does nothing", "[integration][resize]")
{
  Fixture f;
  const int nResizeRequests = f.mDlg.mNumResizeRequests;
  const int nControlResizes = f.mProbe->mNumResizes;

  f.mUI->Resize(400, 300, 1.f);

  REQUIRE(f.mDlg.mNumResizeRequests == nResizeRequests);
  REQUIRE(f.mProbe->mNumResizes == nControlResizes);
  REQUIRE_FALSE(f.mUI->RenderFrame());
}

TEST_CASE("Resize: the draw scale is limited by the scale constraints", "[integration][resize][scale][constraints]")
{
  Fixture f;

  SECTION("the defaults")
  {
    f.mUI->Resize(400, 300, 100.f);
    REQUIRE(f.mUI->GetDrawScale() == Approx(DEFAULT_MAX_DRAW_SCALE));
    f.mUI->Resize(400, 300, 0.01f);
    REQUIRE(f.mUI->GetDrawScale() == Approx(DEFAULT_MIN_DRAW_SCALE));
  }

  SECTION("custom constraints, given in either order")
  {
    f.mUI->SetScaleConstraints(2.f, 0.75f);
    f.mUI->Resize(400, 300, 3.f);
    REQUIRE(f.mUI->GetDrawScale() == Approx(2.f));
    REQUIRE(f.mUI->WindowWidth() == 800);
    f.mUI->Resize(400, 300, 0.5f);
    REQUIRE(f.mUI->GetDrawScale() == Approx(0.75f));
    f.mUI->Resize(400, 300, 1.25f);
    REQUIRE(f.mUI->GetDrawScale() == Approx(1.25f));
  }
}

TEST_CASE("Resize: the size is limited by the delegate's size constraints", "[integration][resize][constraints]")
{
  Fixture f;
  f.mDlg.SetSizeConstraints(300, 600, 200, 500);

  REQUIRE(f.mDlg.GetMinWidth() == 300);
  REQUIRE(f.mDlg.GetMaxWidth() == 600);
  REQUIRE(f.mDlg.GetMinHeight() == 200);
  REQUIRE(f.mDlg.GetMaxHeight() == 500);

  f.mUI->Resize(1000, 1000, 1.f);
  REQUIRE(f.mUI->Width() == 600);
  REQUIRE(f.mUI->Height() == 500);
  REQUIRE(f.mDlg.GetEditorWidth() == 600);

  f.mUI->Resize(10, 10, 1.f);
  REQUIRE(f.mUI->Width() == 300);
  REQUIRE(f.mUI->Height() == 200);

  f.mUI->Resize(450, 1000, 1.f);
  REQUIRE(f.mUI->Width() == 450);
  REQUIRE(f.mUI->Height() == 500);

  int w = 700, h = 100;
  REQUIRE_FALSE(f.mDlg.ConstrainEditorResize(w, h));
  REQUIRE(w == 600);
  REQUIRE(h == 200);
  w = 400;
  h = 300;
  REQUIRE(f.mDlg.ConstrainEditorResize(w, h));
  REQUIRE(w == 400);
  REQUIRE(h == 300);
}

TEST_CASE("Resize: the layout function only runs again if layout on resize is enabled", "[integration][resize][layout]")
{
  Fixture f;
  REQUIRE(f.mDlg.mNumLayouts == 1);

  SECTION("disabled (the default): controls stay where they are")
  {
    f.mUI->Resize(600, 500, 1.f);
    REQUIRE(f.mDlg.mNumLayouts == 1);
    REQUIRE(f.mFill->GetRECT() == IRECT(100, 100, 300, 200));
    REQUIRE(f.mUI->GetBackgroundControl()->GetRECT() == IRECT(0, 0, 400, 300));
  }

  SECTION("enabled: the layout function repositions the existing controls")
  {
    f.mUI->SetLayoutOnResize(true);
    f.mUI->Resize(600, 500, 1.f);

    REQUIRE(f.mDlg.mNumLayouts == 2);
    REQUIRE(f.mUI->NControls() == 3);
    REQUIRE(f.mUI->GetControlWithTag(kTagFill) == f.mFill);
    REQUIRE(f.mFill->GetRECT() == IRECT(100, 100, 500, 400));
    REQUIRE(f.mUI->GetBackgroundControl()->GetRECT() == IRECT(0, 0, 600, 500));
    REQUIRE(f.mProbe->GetRECT() == IRECT(0, 0, 50, 50));

    // Hit testing follows the new layout
    f.mUI->Click(450, 350);
    REQUIRE(f.mFill->Count("down") == 1);

    // A change of scale alone is also a resize
    f.mUI->Resize(600, 500, 2.f);
    REQUIRE(f.mDlg.mNumLayouts == 3);
    REQUIRE(f.mFill->GetRECT() == IRECT(100, 100, 500, 400));
  }
}

TEST_CASE("Resize: ends a mouse gesture that is in progress", "[integration][resize][mouse]")
{
  Fixture f;

  f.mUI->MouseDown(25, 25);
  REQUIRE(f.mUI->ControlIsCaptured(f.mProbe));

  f.mUI->Resize(500, 400, 1.f);
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());

  f.mUI->MouseDragTo(30, 30);
  REQUIRE(f.mProbe->Count("drag") == 0);
  f.mUI->MouseUp(30, 30);
}

TEST_CASE("SetScreenScale: changes the pixel scale but not the size of the UI", "[integration][resize][scale][screenscale]")
{
  Fixture f;
  const int nRescales = f.mProbe->mNumRescales;
  const int nResizes = f.mProbe->mNumResizes;
  const int nDrawResizes = f.mUI->mNumDrawResizes;
  const int nPlatformResizes = f.mUI->mNumPlatformResizes;

  f.mUI->SetScreenScale(2.f);

  REQUIRE(f.mUI->GetScreenScale() == Approx(2.f));
  REQUIRE(f.mUI->GetRoundedScreenScale() == 2);
  REQUIRE(f.mUI->GetDrawScale() == Approx(1.f));
  REQUIRE(f.mUI->GetTotalScale() == Approx(2.f));
  REQUIRE(f.mUI->Width() == 400);
  REQUIRE(f.mUI->Height() == 300);
  REQUIRE(f.mUI->WindowWidth() == 400);

  REQUIRE(f.mProbe->mNumRescales == nRescales + 1);
  REQUIRE(f.mProbe->mNumResizes == nResizes);
  REQUIRE(f.mUI->mNumDrawResizes == nDrawResizes + 1);
  REQUIRE(f.mUI->mNumPlatformResizes == nPlatformResizes + 1);
  REQUIRE(f.mUI->RenderFrame());

  // Fractional screen scales round up for choosing bitmap resources
  f.mUI->SetScreenScale(1.5f);
  REQUIRE(f.mUI->GetRoundedScreenScale() == 2);
  f.mUI->Resize(400, 300, 2.f);
  REQUIRE(f.mUI->GetTotalScale() == Approx(3.f));
}

TEST_CASE("SetScreenScale: rectangles snap to the device pixel grid", "[integration][resize][scale][screenscale]")
{
  Fixture f;
  IRECT r = IRECT(10.3f, 10.3f, 20.3f, 20.3f);

  REQUIRE(f.mUI->GetPixelSnapped(r) == IRECT(10, 10, 20, 20));

  f.mUI->SetScreenScale(2.f);
  REQUIRE(f.mUI->GetPixelSnapped(r) == IRECT(10.5f, 10.5f, 20.5f, 20.5f));
}

TEST_CASE("IGEditorDelegate::SetScreenScale: is passed on to the open UI", "[integration][resize][scale][screenscale]")
{
  Fixture f;

  f.mDlg.SetScreenScale(2.f);
  REQUIRE(f.mUI->GetScreenScale() == Approx(2.f));
}

TEST_CASE("OnParentWindowResize: resizes the UI to fit the window the host gave it", "[integration][resize][host]")
{
  Fixture f;

  f.mDlg.OnParentWindowResize(640, 480);

  REQUIRE(f.mUI->Width() == 640);
  REQUIRE(f.mUI->Height() == 480);
  REQUIRE(f.mUI->GetDrawScale() == Approx(1.f));
  REQUIRE(f.mDlg.GetEditorWidth() == 640);
  REQUIRE(f.mDlg.GetEditorHeight() == 480);
  // The host has already resized the window, so the platform does not need to
  REQUIRE_FALSE(f.mDlg.mLastResizeNeededPlatformResize);
}

TEST_CASE("Resizer mode: the default depends on the platform", "[integration][resize][cornerresizer]")
{
  Fixture f;

#ifdef OS_LINUX
  REQUIRE(f.mUI->GetResizerMode() == EUIResizerMode::Size);
#else
  REQUIRE(f.mUI->GetResizerMode() == EUIResizerMode::Scale);
#endif
  REQUIRE_FALSE(f.mUI->GetResizingInProcess());
}

#ifdef OS_LINUX
TEST_CASE("Corner resizer (Linux): no control is attached, but the resize mode is still set", "[integration][resize][cornerresizer][linux]")
{
  Fixture f;

  f.mUI->AttachCornerResizer(EUIResizerMode::Scale, true);
  REQUIRE(f.mUI->GetResizerMode() == EUIResizerMode::Scale);

  // The host or window manager provides the resize handle, so the corner of the UI behaves like the rest of it
  int nSpecialControls = -f.mUI->NControls();
  f.mUI->ForAllControlsFunc([&](IControl*) { nSpecialControls++; });
  REQUIRE(nSpecialControls == 0);

  f.mUI->MouseDown(397, 297);
  REQUIRE_FALSE(f.mUI->GetResizingInProcess());
  f.mUI->MouseDragTo(600, 500);
  f.mUI->MouseUp(600, 500);
  REQUIRE(f.mUI->Width() == 400);
  REQUIRE(f.mUI->Height() == 300);

  // layoutOnResize was taken from the call
  f.mUI->Resize(500, 400, 1.f);
  REQUIRE(f.mDlg.mNumLayouts == 2);

  f.mUI->AttachCornerResizer(EUIResizerMode::Size);
  REQUIRE(f.mUI->GetResizerMode() == EUIResizerMode::Size);
}
#else
TEST_CASE("Corner resizer: sits in the bottom right corner and is drawn on top", "[integration][resize][cornerresizer]")
{
  Fixture f;
  f.mUI->AttachCornerResizer(EUIResizerMode::Scale, false, COLOR_RED);

  IControl* pResizer = nullptr;
  f.mUI->ForAllControlsFunc([&](IControl* pControl) {
    if (pControl->As<ICornerResizerControl>())
      pResizer = pControl;
  });

  REQUIRE(pResizer != nullptr);
  REQUIRE(f.mUI->NControls() == 3);
  REQUIRE(pResizer->GetRECT() == IRECT(380, 280, 400, 300));
  REQUIRE(f.mUI->GetResizerMode() == EUIResizerMode::Scale);

  f.mUI->SetAllControlsDirty();
  f.mUI->SetStrictDrawing(true);
  f.mUI->ClearRecorded();
  f.mUI->RenderFrame();
  REQUIRE(f.mUI->mDrawOps.back().mBounds == IRECT(380, 280, 400, 300));
  REQUIRE(f.mUI->mDrawOps.back().mColor.R == 255);
}

TEST_CASE("Corner resizer: dragging it in scale mode scales the UI", "[integration][resize][cornerresizer][scale]")
{
  Fixture f;
  f.mUI->AttachCornerResizer(EUIResizerMode::Scale);

  f.mUI->MouseDown(390, 290);
  REQUIRE(f.mUI->GetResizingInProcess());
  // The controls underneath the corner do not see the gesture
  REQUIRE(f.mFill->Count("down") == 0);

  f.mUI->MouseDragTo(585, 435);
  REQUIRE(f.mUI->GetDrawScale() == Approx(1.5f));
  REQUIRE(f.mUI->Width() == 400);
  REQUIRE(f.mUI->Height() == 300);
  REQUIRE(f.mUI->WindowWidth() == 600);
  REQUIRE(f.mUI->WindowHeight() == 450);
  REQUIRE(f.mDlg.GetEditorWidth() == 600);

  f.mUI->MouseUp(585, 435);
  REQUIRE_FALSE(f.mUI->GetResizingInProcess());
  REQUIRE(f.mUI->GetDrawScale() == Approx(1.5f));
  REQUIRE(f.mUI->RenderFrame());
}

TEST_CASE("Corner resizer: in scale mode the smaller of the two directions wins, within the scale limits", "[integration][resize][cornerresizer][scale][constraints]")
{
  Fixture f;
  f.mUI->AttachCornerResizer(EUIResizerMode::Scale);

  SECTION("an uneven drag")
  {
    f.mUI->MouseDown(390, 290);
    f.mUI->MouseDragTo(780, 348);
    f.mUI->MouseUp(780, 348);
    REQUIRE(f.mUI->GetDrawScale() == Approx(1.2f));
  }

  SECTION("a drag far beyond the maximum scale")
  {
    f.mUI->MouseDown(390, 290);
    f.mUI->MouseDragTo(39000, 29000);
    f.mUI->MouseUp(39000, 29000);
    REQUIRE(f.mUI->GetDrawScale() == Approx(DEFAULT_MAX_DRAW_SCALE));
  }

  SECTION("a drag towards the origin stops at the minimum scale")
  {
    f.mUI->MouseDown(390, 290);
    f.mUI->MouseDragTo(1, 1);
    f.mUI->MouseUp(1, 1);
    REQUIRE(f.mUI->GetDrawScale() == Approx(DEFAULT_MIN_DRAW_SCALE));
  }
}

TEST_CASE("Corner resizer: dragging it in size mode changes the size of the UI and lays it out again", "[integration][resize][cornerresizer][layout]")
{
  Fixture f;
  f.mUI->AttachCornerResizer(EUIResizerMode::Size, true);
  REQUIRE(f.mUI->GetResizerMode() == EUIResizerMode::Size);

  f.mUI->MouseDown(390, 290);
  REQUIRE(f.mUI->GetResizingInProcess());
  f.mUI->MouseDragTo(600, 500);

  REQUIRE(f.mUI->Width() == 600);
  REQUIRE(f.mUI->Height() == 500);
  REQUIRE(f.mUI->GetDrawScale() == Approx(1.f));
  REQUIRE(f.mDlg.GetEditorWidth() == 600);
  REQUIRE(f.mDlg.GetEditorHeight() == 500);
  REQUIRE(f.mDlg.mNumLayouts == 2);
  REQUIRE(f.mFill->GetRECT() == IRECT(100, 100, 500, 400));

  f.mUI->MouseDragTo(450, 350);
  REQUIRE(f.mUI->Width() == 450);
  REQUIRE(f.mUI->Height() == 350);

  f.mUI->MouseUp(450, 350);
  REQUIRE_FALSE(f.mUI->GetResizingInProcess());

  // The resizer has moved to the new corner
  f.mUI->MouseDown(445, 345);
  REQUIRE(f.mUI->GetResizingInProcess());
  f.mUI->MouseUp(445, 345);
}

TEST_CASE("Corner resizer: size mode respects the delegate's size constraints", "[integration][resize][cornerresizer][constraints]")
{
  Fixture f;
  f.mDlg.SetSizeConstraints(300, 500, 250, 450);
  f.mUI->AttachCornerResizer(EUIResizerMode::Size);

  f.mUI->MouseDown(390, 290);
  f.mUI->MouseDragTo(2000, 2000);
  REQUIRE(f.mUI->Width() == 500);
  REQUIRE(f.mUI->Height() == 450);
  f.mUI->MouseDragTo(10, 10);
  REQUIRE(f.mUI->Width() == 300);
  REQUIRE(f.mUI->Height() == 250);
  f.mUI->MouseUp(10, 10);
}

TEST_CASE("Corner resizer: a double click restores the original size", "[integration][resize][cornerresizer]")
{
  Fixture f;
  f.mUI->AttachCornerResizer(EUIResizerMode::Size);

  f.mUI->MouseDown(390, 290);
  f.mUI->MouseDragTo(600, 500);
  f.mUI->MouseUp(600, 500);
  REQUIRE(f.mUI->Width() == 600);

  f.mUI->DblClick(595, 495);
  REQUIRE(f.mUI->Width() == 400);
  REQUIRE(f.mUI->Height() == 300);
  REQUIRE(f.mUI->GetDrawScale() == Approx(1.f));
  REQUIRE_FALSE(f.mUI->GetResizingInProcess());
}

TEST_CASE("Corner resizer: changes the cursor while the pointer is over it", "[integration][resize][cornerresizer][cursor][mouseover]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);
  f.mUI->AttachCornerResizer(EUIResizerMode::Scale);

  f.mUI->MouseMove(390, 290);
  REQUIRE(f.mUI->SetMouseCursor(ECursor::SIZENWSE) == ECursor::SIZENWSE);
  f.mUI->MouseMove(200, 150);
  REQUIRE(f.mUI->SetMouseCursor(ECursor::ARROW) == ECursor::ARROW);
}
#endif
