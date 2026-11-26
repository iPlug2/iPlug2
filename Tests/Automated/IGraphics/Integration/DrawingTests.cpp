/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the IGraphics draw loop (dirty tracking, animation, layers, clipping) and the drawing helper functions

#include <cmath>
#include <cstring>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestEditorDelegate.h"
#include "TestControls.h"
#include "IControls.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;
using DrawOp = IGraphicsHeadless::DrawOp;

namespace
{
const IRECT kBoundsA = IRECT(20, 20, 80, 80);
const IRECT kBoundsB = IRECT(300, 200, 380, 280);
const IRECT kBoundsOverA = IRECT(60, 60, 120, 120);

/** A background and two probes far apart, already drawn once */
struct Fixture
{
  Fixture()
  : mDlg(0, 400, 300)
  {
    mUI = mDlg.OpenUI();
    mBackground = mUI->AttachControl(new ProbeControl(IRECT(0, 0, 400, 300)))->As<ProbeControl>();
    mBackground->SetIgnoreMouse(true);
    mA = mUI->AttachControl(new ProbeControl(kBoundsA))->As<ProbeControl>();
    mB = mUI->AttachControl(new ProbeControl(kBoundsB))->As<ProbeControl>();
    mUI->RenderUntilClean();
    mUI->ClearRecorded();
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
  ProbeControl* mBackground = nullptr;
  ProbeControl* mA = nullptr;
  ProbeControl* mB = nullptr;
};

bool SameColor(const IColor& a, const IColor& b)
{
  return a.A == b.A && a.R == b.R && a.G == b.G && a.B == b.B;
}

void RequireRect(const IRECT& actual, const IRECT& expected, float margin = 0.01f)
{
  REQUIRE(actual.L == Approx(expected.L).margin(margin));
  REQUIRE(actual.T == Approx(expected.T).margin(margin));
  REQUIRE(actual.R == Approx(expected.R).margin(margin));
  REQUIRE(actual.B == Approx(expected.B).margin(margin));
}

/** Draws its content into a layer, and counts how often the content had to be rendered */
class LayerControl : public IControl
{
public:
  LayerControl(const IRECT& bounds) : IControl(bounds) {}

  void Draw(IGraphics& g) override
  {
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(this, mRECT);
      mClipInLayer = static_cast<IGraphicsHeadless&>(g).mClipRect;
      g.FillRect(COLOR_GREEN, mRECT.GetPadded(-5.f));
      mLayer = g.EndLayer();
      mNumContentDraws++;
    }

    g.DrawLayer(mLayer);
    mNumDraws++;
  }

  ILayerPtr mLayer;
  IRECT mClipInLayer;
  int mNumContentDraws = 0;
  int mNumDraws = 0;
};
} // namespace

#pragma mark - Dirty tracking

TEST_CASE("Draw loop: a clean UI draws nothing", "[integration][drawing][dirty]")
{
  Fixture f;

  for (int i = 0; i < 3; i++)
    REQUIRE_FALSE(f.mUI->RenderFrame());

  REQUIRE(f.mUI->mDrawOps.empty());
  REQUIRE(f.mUI->mNumFrames == 0);
  REQUIRE(f.mA->mNumDraws == 1);
}

TEST_CASE("Draw loop: only the area of a dirty control is redrawn", "[integration][drawing][dirty]")
{
  Fixture f;

  f.mA->SetDirty(false);
  REQUIRE(f.mA->IsDirty());
  REQUIRE_FALSE(f.mB->IsDirty());

  REQUIRE(f.mUI->RenderFrame());

  REQUIRE(f.mUI->mLastDirtyBounds.Contains(kBoundsA));
  REQUIRE_FALSE(f.mUI->mLastDirtyBounds.Intersects(kBoundsB));
  // The dirty area is the control, plus a little for outlines
  REQUIRE(kBoundsA.GetPadded(2.f).Contains(f.mUI->mLastDirtyBounds));

  REQUIRE(f.mA->mNumDraws == 2);
  REQUIRE(f.mB->mNumDraws == 1);
  // What is underneath the dirty control is redrawn too, clipped to the dirty area
  REQUIRE(f.mBackground->mNumDraws == 2);
  REQUIRE(kBoundsA.GetPadded(2.f).Contains(f.mBackground->mClipDuringDraw));

  REQUIRE_FALSE(f.mA->IsDirty());
  REQUIRE_FALSE(f.mUI->RenderFrame());
}

TEST_CASE("Draw loop: a control overlapping a dirty one is redrawn with it", "[integration][drawing][dirty]")
{
  Fixture f;
  auto* pOver = f.mUI->AttachControl(new ProbeControl(kBoundsOverA))->As<ProbeControl>();
  f.mUI->RenderUntilClean();
  const int nDrawsA = f.mA->mNumDraws;
  const int nDrawsOver = pOver->mNumDraws;
  const int nDrawsB = f.mB->mNumDraws;

  f.mA->SetDirty(false);
  f.mUI->RenderFrame();

  REQUIRE(f.mA->mNumDraws == nDrawsA + 1);
  REQUIRE(pOver->mNumDraws == nDrawsOver + 1);
  REQUIRE(f.mB->mNumDraws == nDrawsB);
  // ... but only the part of it that is inside the dirty area
  REQUIRE(pOver->mClipDuringDraw.R <= kBoundsA.R + 1.f);
  REQUIRE(pOver->mClipDuringDraw.B <= kBoundsA.B + 1.f);
}

TEST_CASE("Draw loop: two dirty controls far apart are drawn as separate areas", "[integration][drawing][dirty]")
{
  Fixture f;

  f.mA->SetDirty(false);
  f.mB->SetDirty(false);
  f.mUI->RenderFrame();

  REQUIRE(f.mA->mNumDraws == 2);
  REQUIRE(f.mB->mNumDraws == 2);
  // The background is drawn once for each area rather than once for everything in between
  REQUIRE(f.mBackground->mNumDraws == 3);
  REQUIRE(f.mUI->mLastDirtyBounds.Contains(kBoundsA));
  REQUIRE(f.mUI->mLastDirtyBounds.Contains(kBoundsB));

  for (auto& clip : f.mUI->mClipRects)
    REQUIRE((kBoundsA.GetPadded(2.f).Contains(clip) || kBoundsB.GetPadded(2.f).Contains(clip)));
}

TEST_CASE("Draw loop: strict drawing redraws the union of the dirty areas in one pass", "[integration][drawing][dirty]")
{
  Fixture f;
  f.mUI->SetStrictDrawing(true);
  // Changing the mode redraws everything
  REQUIRE(f.mUI->RenderFrame());
  const int nBackgroundDraws = f.mBackground->mNumDraws;

  f.mA->SetDirty(false);
  f.mB->SetDirty(false);
  f.mUI->RenderFrame();

  REQUIRE(f.mBackground->mNumDraws == nBackgroundDraws + 1);
  REQUIRE(f.mBackground->mClipDuringDraw.Contains(kBoundsA));
  REQUIRE(f.mBackground->mClipDuringDraw.Contains(kBoundsB));
}

TEST_CASE("Draw loop: SetAllControlsDirty redraws everything, SetAllControlsClean cancels pending redraws", "[integration][drawing][dirty]")
{
  Fixture f;

  f.mUI->SetAllControlsDirty();
  REQUIRE(f.mA->IsDirty());
  REQUIRE(f.mB->IsDirty());
  REQUIRE(f.mBackground->IsDirty());
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->mLastDirtyBounds.Contains(f.mUI->GetBounds()));
  REQUIRE(f.mA->mNumDraws == 2);
  REQUIRE(f.mB->mNumDraws == 2);

  f.mUI->SetAllControlsDirty();
  f.mUI->SetAllControlsClean();
  REQUIRE_FALSE(f.mA->IsDirty());
  REQUIRE_FALSE(f.mUI->RenderFrame());
}

TEST_CASE("Draw loop: hidden controls are not drawn, and hiding one repaints what was underneath", "[integration][drawing][dirty]")
{
  Fixture f;

  f.mA->Hide(true);
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mA->mNumDraws == 1);
  REQUIRE(f.mBackground->mNumDraws == 2);
  REQUIRE(f.mUI->mLastDirtyBounds.Contains(kBoundsA));

  f.mUI->SetAllControlsDirty();
  f.mUI->RenderFrame();
  REQUIRE(f.mA->mNumDraws == 1);
  REQUIRE(f.mB->mNumDraws == 2);

  f.mA->Hide(false);
  f.mUI->RenderFrame();
  REQUIRE(f.mA->mNumDraws == 2);
}

TEST_CASE("Draw loop: a control is clipped to its own bounds while it draws", "[integration][drawing][clip]")
{
  Fixture f;

  f.mUI->SetAllControlsDirty();
  f.mUI->RenderFrame();

  // The clip region allows for a one pixel outline around the control, and no more
  REQUIRE(f.mA->mClipDuringDraw.Contains(kBoundsA));
  REQUIRE(kBoundsA.GetPadded(1.f).Contains(f.mA->mClipDuringDraw));
  REQUIRE(f.mB->mClipDuringDraw.Contains(kBoundsB));
  REQUIRE(kBoundsB.GetPadded(1.f).Contains(f.mB->mClipDuringDraw));

  // Each operation recorded the clip that applied to it
  for (auto& op : f.mUI->mDrawOps)
  {
    if (op.mBounds == kBoundsA)
      REQUIRE(kBoundsA.GetPadded(1.f).Contains(op.mClip));
  }
}

TEST_CASE("Draw loop: the display tick function is called every frame, dirty or not", "[integration][drawing]")
{
  Fixture f;
  int nTicks = 0;
  f.mUI->SetDisplayTickFunc([&]() { nTicks++; });

  f.mUI->RenderFrame();
  f.mUI->RenderFrame();
  f.mA->SetDirty(false);
  f.mUI->RenderFrame();

  REQUIRE(nTicks == 3);
}

#pragma mark - Animation

TEST_CASE("Animation: an animating control is redrawn every frame until the animation ends", "[integration][drawing][animation]")
{
  Fixture f;
  int nAnimationCalls = 0;
  int nEndActions = 0;
  f.mA->SetAnimationEndActionFunction([&](IControl*) { nEndActions++; });

  // Long enough that it cannot end by itself during the test
  f.mA->SetAnimation([&](IControl* pCaller) { nAnimationCalls++; }, 100000000);
  REQUIRE(f.mA->GetAnimationDuration().count() == Approx(100000000.));

  for (int i = 1; i <= 5; i++)
  {
    REQUIRE(f.mUI->RenderFrame());
    REQUIRE(nAnimationCalls == i);
    REQUIRE(f.mA->mNumDraws == 1 + i);
    REQUIRE(f.mA->GetAnimationProgress() >= 0.);
    REQUIRE(f.mA->GetAnimationProgress() < 0.5);
  }

  // Only the animating control's area is redrawn
  REQUIRE(f.mB->mNumDraws == 1);
  REQUIRE(nEndActions == 0);

  f.mA->OnEndAnimation();
  REQUIRE(nEndActions == 1);
  REQUIRE(f.mA->GetAnimationFunction() == nullptr);

  // One last frame to draw the final state, then the UI is idle again
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE_FALSE(f.mUI->RenderFrame());
  REQUIRE(nAnimationCalls == 5);
}

TEST_CASE("Animation: the default animation function ends the animation when its time is up", "[integration][drawing][animation]")
{
  Fixture f;
  int nEndActions = 0;
  f.mA->SetAnimationEndActionFunction([&](IControl*) { nEndActions++; });

  // A zero length animation is over as soon as the clock has moved on at all. Frames are rendered until then; the
  // bound is only there to turn a hang into a failure.
  f.mA->SetAnimation(DefaultAnimationFunc, 0);

  int nFrames = 0;
  while (f.mA->GetAnimationFunction() && nFrames < 50000000)
  {
    f.mUI->RenderFrame();
    nFrames++;
  }

  REQUIRE(f.mA->GetAnimationFunction() == nullptr);
  REQUIRE(nEndActions == 1);
  f.mUI->RenderUntilClean();
  REQUIRE_FALSE(f.mUI->RenderFrame());
}

TEST_CASE("Animation: a new animation replaces the one in progress", "[integration][drawing][animation]")
{
  Fixture f;
  int nFirst = 0, nSecond = 0;

  f.mA->SetAnimation([&](IControl*) { nFirst++; }, 100000000);
  f.mUI->RenderFrame();
  f.mA->SetAnimation([&](IControl*) { nSecond++; }, 100000000);
  f.mUI->RenderFrame();
  f.mUI->RenderFrame();

  REQUIRE(nFirst == 1);
  REQUIRE(nSecond == 2);
  f.mA->OnEndAnimation();
}

TEST_CASE("Animation: ILambdaControl animates on a click and passes the mouse state to its draw function", "[integration][drawing][animation][controls]")
{
  Fixture f;
  int nDraws = 0;
  IMouseInfo lastMouse;
  auto* pLambda = new ILambdaControl(IRECT(150, 20, 250, 120), [&](ILambdaControl* pCaller, IGraphics& g, IRECT& r) {
    nDraws++;
    lastMouse = pCaller->mMouseInfo;
    g.FillRect(COLOR_BLUE, r);
  }, 100000000);
  f.mUI->AttachControl(pLambda);
  f.mUI->RenderUntilClean();
  nDraws = 0;

  REQUIRE(pLambda->GetAnimationFunction() == nullptr);
  f.mUI->MouseDown(200, 70);
  REQUIRE(pLambda->GetAnimationFunction() != nullptr);
  f.mUI->RenderFrame();
  f.mUI->RenderFrame();
  REQUIRE(nDraws == 2);
  REQUIRE(lastMouse.x == Approx(200.f));
  REQUIRE(lastMouse.ms.L);

  f.mUI->MouseDragTo(210, 75);
  f.mUI->RenderFrame();
  REQUIRE(lastMouse.x == Approx(210.f));
  REQUIRE(lastMouse.dX == Approx(10.f));

  f.mUI->MouseUp(210, 75);
  FinishAnimation(pLambda);
  f.mUI->RenderUntilClean();
  REQUIRE_FALSE(lastMouse.ms.L);
  REQUIRE_FALSE(f.mUI->RenderFrame());
}

#pragma mark - Layers

TEST_CASE("Layers: content drawn into a layer is cached and reused on later frames", "[integration][drawing][layers]")
{
  Fixture f;
  const IRECT bounds = IRECT(150, 20, 250, 120);
  auto* pControl = new LayerControl(bounds);
  f.mUI->AttachControl(pControl);
  f.mUI->RenderUntilClean();

  REQUIRE(pControl->mNumContentDraws == 1);
  REQUIRE(pControl->mNumDraws >= 1);
  REQUIRE(f.mUI->CheckLayer(pControl->mLayer));
  RequireRect(pControl->mLayer->Bounds(), bounds);
  // While the layer was being drawn into, drawing was clipped to it
  RequireRect(pControl->mClipInLayer, bounds);

  // The layer's bitmap has the size of the control in pixels
  const APIBitmap* pBitmap = pControl->mLayer->GetAPIBitmap();
  REQUIRE(pBitmap->GetWidth() == 100);
  REQUIRE(pBitmap->GetHeight() == 100);

  f.mUI->ClearRecorded();
  const int nDraws = pControl->mNumDraws;

  for (int i = 0; i < 3; i++)
  {
    pControl->SetDirty(false);
    f.mUI->RenderFrame();
  }

  REQUIRE(pControl->mNumDraws == nDraws + 3);
  REQUIRE(pControl->mNumContentDraws == 1);
  // Each frame the layer was blitted to its place, and the content was not drawn
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Bitmap) == 3);
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Fill) == 3); // the background probe only
  RequireRect(f.mUI->LastOp(DrawOp::EType::Bitmap)->mBounds, bounds);
}

TEST_CASE("Layers: a layer is rendered again when it is invalidated or its control changes size", "[integration][drawing][layers]")
{
  Fixture f;
  auto* pControl = new LayerControl(IRECT(150, 20, 250, 120));
  f.mUI->AttachControl(pControl);
  f.mUI->RenderUntilClean();
  REQUIRE(pControl->mNumContentDraws == 1);

  SECTION("invalidated explicitly")
  {
    pControl->mLayer->Invalidate();
    REQUIRE_FALSE(f.mUI->CheckLayer(pControl->mLayer));
    pControl->SetDirty(false);
    f.mUI->RenderFrame();
    REQUIRE(pControl->mNumContentDraws == 2);
    REQUIRE(f.mUI->CheckLayer(pControl->mLayer));
  }

  SECTION("the control is resized")
  {
    pControl->SetTargetAndDrawRECTs(IRECT(150, 20, 300, 100));
    pControl->SetDirty(false);
    f.mUI->RenderFrame();
    REQUIRE(pControl->mNumContentDraws == 2);
    RequireRect(pControl->mLayer->Bounds(), IRECT(150, 20, 300, 100));
    REQUIRE(pControl->mLayer->GetAPIBitmap()->GetWidth() == 150);
    REQUIRE(pControl->mLayer->GetAPIBitmap()->GetHeight() == 80);
  }

  SECTION("the UI is scaled")
  {
    f.mUI->Resize(400, 300, 2.f);
    REQUIRE_FALSE(f.mUI->CheckLayer(pControl->mLayer));
    f.mUI->RenderUntilClean();
    REQUIRE(pControl->mNumContentDraws == 2);
    // Twice the pixels for the same control size
    REQUIRE(pControl->mLayer->GetAPIBitmap()->GetWidth() == 200);
    REQUIRE(pControl->mLayer->GetAPIBitmap()->GetHeight() == 200);
    REQUIRE(pControl->mLayer->GetAPIBitmap()->GetDrawScale() == Approx(2.f));
  }

  SECTION("the screen scale changes")
  {
    f.mUI->SetScreenScale(2.f);
    REQUIRE_FALSE(f.mUI->CheckLayer(pControl->mLayer));
    f.mUI->RenderUntilClean();
    REQUIRE(pControl->mNumContentDraws == 2);
    REQUIRE(pControl->mLayer->GetAPIBitmap()->GetWidth() == 200);
    REQUIRE(pControl->mLayer->GetAPIBitmap()->GetScale() == Approx(2.f));
  }
}

TEST_CASE("Layers: an empty layer pointer is never valid", "[integration][drawing][layers]")
{
  Fixture f;
  ILayerPtr layer;
  REQUIRE_FALSE(f.mUI->CheckLayer(layer));
}

TEST_CASE("Layers: a layer can be resumed to add to its content", "[integration][drawing][layers]")
{
  Fixture f;
  const IRECT bounds = IRECT(10, 10, 60, 60);

  f.mUI->StartLayer(nullptr, bounds);
  f.mUI->FillRect(COLOR_RED, bounds);
  ILayerPtr layer = f.mUI->EndLayer();
  REQUIRE(layer != nullptr);
  REQUIRE(f.mUI->CheckLayer(layer));

  const APIBitmap* pBitmap = layer->GetAPIBitmap();
  f.mUI->ResumeLayer(layer);
  REQUIRE(layer == nullptr);
  f.mUI->FillRect(COLOR_BLUE, bounds.GetPadded(-10.f));
  layer = f.mUI->EndLayer();

  REQUIRE(layer != nullptr);
  REQUIRE(layer->GetAPIBitmap() == pBitmap);

  f.mUI->ClearRecorded();
  f.mUI->DrawFittedLayer(layer, IRECT(100, 100, 200, 200), nullptr);
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Bitmap) == 1);
  RequireRect(f.mUI->mDrawOps[0].mDeviceBounds, IRECT(100, 100, 200, 200));
}

#pragma mark - Text

TEST_CASE("MeasureText: positions the text rectangle in the bounds according to the alignment", "[integration][drawing][text][measure]")
{
  Fixture f;
  const float size = 20.f;
  const float w = 6.f * size * IGraphicsHeadless::kCharWidth; // "abcdef"
  const IRECT bounds = IRECT(100, 100, 300, 200);

  auto measure = [&](EAlign align, EVAlign valign) {
    IRECT r = bounds;
    const float width = f.mUI->MeasureText(IText(size, COLOR_BLACK, nullptr, align, valign), "abcdef", r);
    REQUIRE(width == Approx(w));
    REQUIRE(r.W() == Approx(w));
    REQUIRE(r.H() == Approx(size));
    return r;
  };

  REQUIRE(measure(EAlign::Near, EVAlign::Top).L == Approx(bounds.L));
  REQUIRE(measure(EAlign::Near, EVAlign::Top).T == Approx(bounds.T));
  REQUIRE(measure(EAlign::Center, EVAlign::Middle).MW() == Approx(bounds.MW()));
  REQUIRE(measure(EAlign::Center, EVAlign::Middle).MH() == Approx(bounds.MH()));
  REQUIRE(measure(EAlign::Far, EVAlign::Bottom).R == Approx(bounds.R));
  REQUIRE(measure(EAlign::Far, EVAlign::Bottom).B == Approx(bounds.B));
}

TEST_CASE("MeasureText: an empty string measures as nothing and leaves the bounds alone", "[integration][drawing][text][measure]")
{
  Fixture f;
  IRECT r = IRECT(1, 2, 3, 4);

  REQUIRE(f.mUI->MeasureText(DEFAULT_TEXT, "", r) == Approx(0.f));
  REQUIRE(r == IRECT(1, 2, 3, 4));
  REQUIRE(f.mUI->MeasureText(DEFAULT_TEXT, nullptr, r) == Approx(0.f));
  REQUIRE(r == IRECT(1, 2, 3, 4));
}

TEST_CASE("MeasureText: the width grows with the string and the text size", "[integration][drawing][text][measure]")
{
  Fixture f;
  IRECT r;

  const float w4 = f.mUI->MeasureText(IText(10.f), "abcd", r);
  const float w8 = f.mUI->MeasureText(IText(10.f), "abcdefgh", r);
  const float w4Big = f.mUI->MeasureText(IText(20.f), "abcd", r);

  REQUIRE(w8 == Approx(2.f * w4));
  REQUIRE(w4Big == Approx(2.f * w4));
}

TEST_CASE("DrawText: records the string, bounds and colour; empty strings are not drawn", "[integration][drawing][text]")
{
  Fixture f;
  const IRECT bounds = IRECT(10, 20, 110, 40);

  f.mUI->DrawText(IText(16.f, COLOR_ORANGE), "Some text", bounds);
  f.mUI->DrawText(DEFAULT_TEXT, "", bounds);
  f.mUI->DrawText(DEFAULT_TEXT, nullptr, bounds);
  f.mUI->DrawText(DEFAULT_TEXT, "At a point", 50.f, 60.f);

  REQUIRE(f.mUI->CountOps(DrawOp::EType::Text) == 2);
  REQUIRE(f.mUI->mDrawOps[0].mText == "Some text");
  REQUIRE(f.mUI->mDrawOps[0].mBounds == bounds);
  REQUIRE(SameColor(f.mUI->mDrawOps[0].mColor, COLOR_ORANGE));
  REQUIRE(f.mUI->mDrawOps[0].mTextSize == Approx(16.f));
  REQUIRE(f.mUI->mDrawOps[1].mBounds == IRECT(50, 60, 50, 60));
  REQUIRE(f.mUI->DidDrawText("At a point"));
  REQUIRE(f.mUI->DidDrawTextContaining("text"));
  REQUIRE_FALSE(f.mUI->DidDrawText("Some"));
}

TEST_CASE("LoadFont: fonts can be registered, and text using them is drawn", "[integration][drawing][text][fonts]")
{
  Fixture f;
  uint8_t fontData[16] = {};

  REQUIRE(f.mUI->LoadFont("System", "Arial", ETextStyle::Normal));
  REQUIRE(f.mUI->LoadFont("Memory", fontData, sizeof(fontData)));
  REQUIRE(f.mUI->LoadFont("File", "font.ttf"));

  f.mUI->DrawText(IText(14.f, COLOR_BLACK, "Memory"), "With a font", IRECT(0, 0, 100, 20));
  REQUIRE(f.mUI->DidDrawText("With a font"));
}

#pragma mark - Shapes

TEST_CASE("Drawing helpers: filled shapes become fill operations with the shape's bounds and colour", "[integration][drawing][shapes]")
{
  Fixture f;
  const IRECT r = IRECT(10, 20, 110, 80);

  SECTION("FillRect")
  {
    f.mUI->FillRect(COLOR_RED, r);
    REQUIRE(f.mUI->mDrawOps.size() == 1);
    REQUIRE(f.mUI->mDrawOps[0].mType == DrawOp::EType::Fill);
    REQUIRE(f.mUI->mDrawOps[0].mBounds == r);
    REQUIRE(SameColor(f.mUI->mDrawOps[0].mColor, COLOR_RED));
  }

  SECTION("FillRoundRect")
  {
    f.mUI->FillRoundRect(COLOR_GREEN, r, 8.f);
    f.mUI->FillRoundRect(COLOR_GREEN, r, 2.f, 4.f, 6.f, 8.f);
    REQUIRE(f.mUI->mDrawOps.size() == 2);
    RequireRect(f.mUI->mDrawOps[0].mDeviceBounds, r);
    RequireRect(f.mUI->mDrawOps[1].mDeviceBounds, r);
    REQUIRE(SameColor(f.mUI->mDrawOps[0].mColor, COLOR_GREEN));
  }

  SECTION("FillCircle and FillEllipse")
  {
    f.mUI->FillCircle(COLOR_BLUE, 100.f, 100.f, 25.f);
    f.mUI->FillEllipse(COLOR_BLUE, r);
    REQUIRE(f.mUI->mDrawOps.size() == 2);
    RequireRect(f.mUI->mDrawOps[0].mDeviceBounds, IRECT(75, 75, 125, 125));
    RequireRect(f.mUI->mDrawOps[1].mDeviceBounds, r);
  }

  SECTION("FillTriangle and FillConvexPolygon")
  {
    f.mUI->FillTriangle(COLOR_BLACK, 10.f, 80.f, 60.f, 20.f, 110.f, 80.f);
    float x[5] = {10.f, 60.f, 110.f, 90.f, 30.f};
    float y[5] = {50.f, 20.f, 50.f, 80.f, 80.f};
    f.mUI->FillConvexPolygon(COLOR_BLACK, x, y, 5);
    REQUIRE(f.mUI->mDrawOps.size() == 2);
    REQUIRE(f.mUI->mDrawOps[0].mBounds == r);
    REQUIRE(f.mUI->mDrawOps[1].mBounds == r);
  }

  SECTION("a gradient fill reports its first colour")
  {
    f.mUI->PathRect(r);
    f.mUI->PathFill(IPattern::CreateLinearGradient(r, EDirection::Vertical, {{COLOR_ORANGE, 0.f}, {COLOR_BLUE, 1.f}}), IFillOptions(), nullptr);
    REQUIRE(f.mUI->mDrawOps.size() == 1);
    REQUIRE(SameColor(f.mUI->mDrawOps[0].mColor, COLOR_ORANGE));
    REQUIRE(f.mUI->mDrawOps[0].mBounds == r);
  }
}

TEST_CASE("Drawing helpers: outlined shapes become stroke operations with the given thickness", "[integration][drawing][shapes]")
{
  Fixture f;
  const IRECT r = IRECT(10, 20, 110, 80);

  SECTION("DrawRect")
  {
    f.mUI->DrawRect(COLOR_RED, r, nullptr, 3.f);
    REQUIRE(f.mUI->mDrawOps.size() == 1);
    REQUIRE(f.mUI->mDrawOps[0].mType == DrawOp::EType::Stroke);
    REQUIRE(f.mUI->mDrawOps[0].mBounds == r);
    REQUIRE(f.mUI->mDrawOps[0].mThickness == Approx(3.f));
    REQUIRE(SameColor(f.mUI->mDrawOps[0].mColor, COLOR_RED));
  }

  SECTION("DrawRoundRect")
  {
    f.mUI->DrawRoundRect(COLOR_RED, r, 5.f, nullptr, 2.f);
    REQUIRE(f.mUI->mDrawOps.size() == 1);
    REQUIRE(f.mUI->mDrawOps[0].mType == DrawOp::EType::Stroke);
    RequireRect(f.mUI->mDrawOps[0].mDeviceBounds, r);
    REQUIRE(f.mUI->mDrawOps[0].mThickness == Approx(2.f));
  }

  SECTION("DrawLine, and the default thickness")
  {
    f.mUI->DrawLine(COLOR_BLUE, 10.f, 80.f, 110.f, 20.f);
    REQUIRE(f.mUI->mDrawOps.size() == 1);
    REQUIRE(f.mUI->mDrawOps[0].mBounds == r);
    REQUIRE(f.mUI->mDrawOps[0].mThickness == Approx(1.f));
  }

  SECTION("DrawCircle, DrawEllipse and DrawArc")
  {
    f.mUI->DrawCircle(COLOR_BLUE, 100.f, 100.f, 25.f);
    f.mUI->DrawEllipse(COLOR_BLUE, r);
    f.mUI->DrawArc(COLOR_BLUE, 100.f, 100.f, 30.f, 0.f, 90.f);
    REQUIRE(f.mUI->mDrawOps.size() == 3);
    RequireRect(f.mUI->mDrawOps[0].mDeviceBounds, IRECT(75, 75, 125, 125));
    RequireRect(f.mUI->mDrawOps[1].mDeviceBounds, r);
    REQUIRE(IRECT(70, 70, 130, 130).Contains(f.mUI->mDrawOps[2].mDeviceBounds));
  }

  SECTION("DrawTriangle and DrawConvexPolygon")
  {
    f.mUI->DrawTriangle(COLOR_BLACK, 10.f, 80.f, 60.f, 20.f, 110.f, 80.f);
    float x[4] = {10.f, 110.f, 110.f, 10.f};
    float y[4] = {20.f, 20.f, 80.f, 80.f};
    f.mUI->DrawConvexPolygon(COLOR_BLACK, x, y, 4, nullptr, 4.f);
    REQUIRE(f.mUI->mDrawOps.size() == 2);
    REQUIRE(f.mUI->mDrawOps[0].mBounds == r);
    REQUIRE(f.mUI->mDrawOps[1].mBounds == r);
    REQUIRE(f.mUI->mDrawOps[1].mThickness == Approx(4.f));
  }

  SECTION("DrawPoint fills a single pixel")
  {
    f.mUI->DrawPoint(COLOR_WHITE, 30.f, 40.f);
    REQUIRE(f.mUI->mDrawOps.size() == 1);
    REQUIRE(f.mUI->mDrawOps[0].mType == DrawOp::EType::Fill);
    REQUIRE(f.mUI->mDrawOps[0].mBounds == IRECT(30, 40, 31, 41));
  }
}

TEST_CASE("Drawing helpers: lines positioned by a normalized value within a rectangle", "[integration][drawing][shapes]")
{
  Fixture f;
  const IRECT r = IRECT(100, 100, 300, 200);

  f.mUI->DrawVerticalLine(COLOR_BLACK, r, 0.25f);
  f.mUI->DrawHorizontalLine(COLOR_BLACK, r, 0.25f);
  f.mUI->DrawLineAcross(COLOR_BLACK, r, EDirection::Vertical, 1.f);
  f.mUI->DrawLineAcross(COLOR_BLACK, r, EDirection::Horizontal, 0.f);
  // Out of range positions are clamped to the rectangle
  f.mUI->DrawVerticalLine(COLOR_BLACK, r, 7.f);
  f.mUI->DrawHorizontalLine(COLOR_BLACK, r, -3.f);

  REQUIRE(f.mUI->mDrawOps.size() == 6);
  // A vertical line a quarter of the way across
  REQUIRE(f.mUI->mDrawOps[0].mBounds == IRECT(150, 100, 150, 200));
  // A horizontal line a quarter of the way up
  REQUIRE(f.mUI->mDrawOps[1].mBounds == IRECT(100, 175, 300, 175));
  REQUIRE(f.mUI->mDrawOps[2].mBounds == IRECT(300, 100, 300, 200));
  REQUIRE(f.mUI->mDrawOps[3].mBounds == IRECT(100, 200, 300, 200));
  REQUIRE(f.mUI->mDrawOps[4].mBounds == IRECT(300, 100, 300, 200));
  REQUIRE(f.mUI->mDrawOps[5].mBounds == IRECT(100, 200, 300, 200));
}

TEST_CASE("Drawing helpers: DrawRadialLine draws along a radius from the centre", "[integration][drawing][shapes]")
{
  Fixture f;

  // Zero degrees is straight up
  f.mUI->DrawRadialLine(COLOR_BLACK, 100.f, 100.f, 0.f, 10.f, 50.f);
  // 90 degrees is to the right
  f.mUI->DrawRadialLine(COLOR_BLACK, 100.f, 100.f, 90.f, 10.f, 50.f, nullptr, 2.f);

  REQUIRE(f.mUI->mDrawOps.size() == 2);
  RequireRect(f.mUI->mDrawOps[0].mBounds, IRECT(100, 50, 100, 90), 0.01f);
  RequireRect(f.mUI->mDrawOps[1].mBounds, IRECT(110, 100, 150, 100), 0.01f);
  REQUIRE(f.mUI->mDrawOps[1].mThickness == Approx(2.f));
}

TEST_CASE("Drawing helpers: DrawData plots normalized values across a rectangle", "[integration][drawing][shapes]")
{
  Fixture f;
  const IRECT r = IRECT(100, 100, 300, 200);
  float ramp[5] = {0.f, 0.25f, 0.5f, 0.75f, 1.f};
  float mid[3] = {0.25f, 0.5f, 0.75f};

  f.mUI->DrawData(COLOR_RED, r, ramp, 5, nullptr, nullptr, 2.f);
  REQUIRE(f.mUI->mDrawOps.size() == 1);
  REQUIRE(f.mUI->mDrawOps[0].mType == DrawOp::EType::Stroke);
  REQUIRE(f.mUI->mDrawOps[0].mBounds == r);
  REQUIRE(f.mUI->mDrawOps[0].mThickness == Approx(2.f));

  // Values that do not span the whole range: 0 is the bottom of the rectangle and 1 the top
  f.mUI->DrawData(COLOR_RED, r, mid, 3);
  REQUIRE(f.mUI->mDrawOps[1].mBounds == IRECT(100, 125, 300, 175));

  // Explicit x positions
  float xs[3] = {0.5f, 0.75f, 1.f};
  f.mUI->DrawData(COLOR_RED, r, mid, 3, xs);
  REQUIRE(f.mUI->mDrawOps[2].mBounds.R == Approx(300.f));
  REQUIRE(f.mUI->mDrawOps[2].mBounds.W() <= 200.f);

  // With a fill colour the area is filled as well as stroked
  f.mUI->ClearRecorded();
  const IColor fill = COLOR_BLUE;
  f.mUI->DrawData(COLOR_RED, r, ramp, 5, nullptr, nullptr, 1.f, &fill);
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Fill) == 1);
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Stroke) == 1);

  // No points, nothing drawn
  f.mUI->ClearRecorded();
  f.mUI->DrawData(COLOR_RED, r, ramp, 0);
  REQUIRE(f.mUI->mDrawOps.empty());
}

TEST_CASE("Drawing helpers: DrawGrid draws grid lines inside a rectangle", "[integration][drawing][shapes]")
{
  Fixture f;
  const IRECT r = IRECT(100, 100, 300, 200);

  f.mUI->DrawGrid(COLOR_GRAY, r, 50.f, 25.f, nullptr, 1.5f);

  REQUIRE(f.mUI->mDrawOps.size() == 1);
  REQUIRE(f.mUI->mDrawOps[0].mType == DrawOp::EType::Stroke);
  REQUIRE(f.mUI->mDrawOps[0].mBounds == r);
  REQUIRE(f.mUI->mDrawOps[0].mThickness == Approx(1.5f));
  REQUIRE(SameColor(f.mUI->mDrawOps[0].mColor, COLOR_GRAY));
}

#pragma mark - Transforms

TEST_CASE("Path transforms: translate and scale apply to what is drawn afterwards", "[integration][drawing][transform]")
{
  Fixture f;
  const IRECT r = IRECT(0, 0, 10, 20);

  f.mUI->PathTransformReset(true);
  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, r);

  f.mUI->PathTransformTranslate(100.f, 50.f);
  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, IRECT(100, 50, 110, 70));
  // The operation itself is still in the control's own coordinates
  REQUIRE(f.mUI->mDrawOps.back().mBounds == r);

  f.mUI->PathTransformScale(2.f);
  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, IRECT(100, 50, 120, 90));

  f.mUI->PathTransformScale(1.f, 0.5f);
  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, IRECT(100, 50, 120, 70));

  f.mUI->PathTransformReset();
  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, r);
}

TEST_CASE("Path transforms: save and restore nest", "[integration][drawing][transform]")
{
  Fixture f;
  const IRECT r = IRECT(0, 0, 10, 10);
  f.mUI->PathTransformReset(true);

  f.mUI->PathTransformTranslate(10.f, 0.f);
  f.mUI->PathTransformSave();
  f.mUI->PathTransformTranslate(0.f, 20.f);
  f.mUI->PathTransformSave();
  f.mUI->PathTransformScale(3.f);

  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, IRECT(10, 20, 40, 50));

  f.mUI->PathTransformRestore();
  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, IRECT(10, 20, 20, 30));

  f.mUI->PathTransformRestore();
  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, IRECT(10, 0, 20, 10));

  // A restore with nothing saved changes nothing
  f.mUI->PathTransformRestore();
  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, IRECT(10, 0, 20, 10));

  f.mUI->PathTransformReset(true);
}

TEST_CASE("Path transforms: rotation and arbitrary matrices", "[integration][drawing][transform]")
{
  Fixture f;
  const IRECT r = IRECT(0, 0, 10, 20);
  f.mUI->PathTransformReset(true);

  // A quarter turn swaps the width and the height
  f.mUI->PathTransformSave();
  f.mUI->PathTransformTranslate(100.f, 100.f);
  f.mUI->PathTransformRotate(90.f);
  f.mUI->FillRect(COLOR_RED, r);
  const IRECT rotated = f.mUI->mDrawOps.back().mDeviceBounds;
  REQUIRE(rotated.W() == Approx(20.f).margin(0.001));
  REQUIRE(rotated.H() == Approx(10.f).margin(0.001));
  f.mUI->PathTransformRestore();

  f.mUI->PathTransformSave();
  f.mUI->PathTransformMatrix(IMatrix().Translate(5.f, 6.f).Scale(2.f, 2.f));
  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, IRECT(5, 6, 25, 46));
  f.mUI->PathTransformRestore();

  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, r);
}

TEST_CASE("Path transforms: each control starts drawing with a clean transform", "[integration][drawing][transform]")
{
  Fixture f;

  // Leave a transform behind, as a careless control might
  f.mUI->PathTransformTranslate(500.f, 500.f);
  f.mUI->PathTransformSave();

  f.mUI->SetAllControlsDirty();
  f.mUI->RenderFrame();

  for (auto& op : f.mUI->mDrawOps)
    RequireRect(op.mDeviceBounds, op.mBounds);
}

TEST_CASE("PathClipRegion: clips to the given rectangle within the current draw area", "[integration][drawing][clip]")
{
  Fixture f;
  f.mA->SetDirty(false);
  f.mUI->RenderFrame(); // leaves the draw area set to control A's region
  const IRECT drawArea = f.mUI->mClipRect;

  f.mUI->PathClipRegion(IRECT(30, 30, 50, 50));
  REQUIRE(f.mUI->mClipRect == IRECT(30, 30, 50, 50));

  // The clip cannot be widened beyond the draw area
  f.mUI->PathClipRegion(IRECT(0, 0, 400, 300));
  REQUIRE(f.mUI->mClipRect == drawArea);

  // An empty rectangle resets it to the draw area
  f.mUI->PathClipRegion(IRECT(30, 30, 50, 50));
  f.mUI->PathClipRegion();
  REQUIRE(f.mUI->mClipRect == drawArea);
}

// BUG: IGraphics::PathEllipse(float x, float y, float r1, float r2, float angle) (IGraphics/IGraphics.cpp:3010) calls
// PathTransformSave() and then returns early for a zero radius without the matching PathTransformRestore(). The
// transform stack is left one deeper, so the caller's next PathTransformRestore() pops that stray state and the
// caller's own transform stays in force for whatever is drawn afterwards.
TEST_CASE("Path transforms: drawing an ellipse with a zero radius leaves the transform stack balanced", "[.][bug]")
{
  Fixture f;
  const IRECT r = IRECT(0, 0, 10, 10);
  f.mUI->PathTransformReset(true);

  f.mUI->PathTransformSave();
  f.mUI->PathTransformTranslate(100.f, 100.f);
  f.mUI->FillEllipse(COLOR_RED, 5.f, 5.f, 0.f, 0.f);
  f.mUI->PathTransformRestore();

  f.mUI->ClearRecorded();
  f.mUI->FillRect(COLOR_RED, r);
  RequireRect(f.mUI->mDrawOps.back().mDeviceBounds, r);
  f.mUI->PathTransformReset(true);
}

#pragma mark - SVG

namespace
{
const char* kTestSVG =
  "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" height=\"50\" viewBox=\"0 0 100 50\">"
  "<rect x=\"0\" y=\"0\" width=\"100\" height=\"50\" fill=\"#ff0000\"/>"
  "<path d=\"M 10 10 L 90 10 L 90 40 Z\" fill=\"none\" stroke=\"#0000ff\" stroke-width=\"2\"/>"
  "</svg>";
}

TEST_CASE("SVG: an SVG loaded from memory has the size of the document and draws as paths", "[integration][drawing][svg]")
{
  Fixture f;
  ISVG svg = f.mUI->LoadSVG("test_image.svg", static_cast<const void*>(kTestSVG), static_cast<int>(strlen(kTestSVG)));

  REQUIRE(svg.IsValid());
  REQUIRE(svg.W() == Approx(100.f));
  REQUIRE(svg.H() == Approx(50.f));

  // Drawn at twice the size
  const IRECT dest = IRECT(100, 100, 300, 200);
  f.mUI->PathTransformReset(true);
  f.mUI->DrawSVG(svg, dest);

  REQUIRE(f.mUI->CountOps(DrawOp::EType::Fill) == 1);
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Stroke) == 1);

  const DrawOp* pFill = f.mUI->LastOp(DrawOp::EType::Fill);
  RequireRect(pFill->mDeviceBounds, dest, 0.5f);
  REQUIRE(pFill->mColor.R == 255);
  REQUIRE(pFill->mColor.B == 0);

  const DrawOp* pStroke = f.mUI->LastOp(DrawOp::EType::Stroke);
  RequireRect(pStroke->mDeviceBounds, IRECT(120, 120, 280, 180), 0.5f);
  REQUIRE(pStroke->mColor.B == 255);
  REQUIRE(pStroke->mColor.R == 0);

  // The transform that DrawSVG used is not left behind
  f.mUI->ClearRecorded();
  f.mUI->FillRect(COLOR_BLACK, IRECT(0, 0, 10, 10));
  RequireRect(f.mUI->mDrawOps[0].mDeviceBounds, IRECT(0, 0, 10, 10));
}

TEST_CASE("SVG: the aspect ratio is kept when the destination has a different shape", "[integration][drawing][svg]")
{
  Fixture f;
  ISVG svg = f.mUI->LoadSVG("test_image.svg", static_cast<const void*>(kTestSVG), static_cast<int>(strlen(kTestSVG)));

  f.mUI->PathTransformReset(true);
  f.mUI->DrawSVG(svg, IRECT(0, 0, 100, 100));

  const DrawOp* pFill = f.mUI->LastOp(DrawOp::EType::Fill);
  REQUIRE(pFill->mDeviceBounds.W() == Approx(100.f).margin(0.5));
  REQUIRE(pFill->mDeviceBounds.H() == Approx(50.f).margin(0.5));
}

TEST_CASE("SVG: colours can be replaced when drawing", "[integration][drawing][svg]")
{
  Fixture f;
  ISVG svg = f.mUI->LoadSVG("test_image.svg", static_cast<const void*>(kTestSVG), static_cast<int>(strlen(kTestSVG)));
  const IColor stroke = COLOR_GREEN;
  const IColor fill = COLOR_ORANGE;

  f.mUI->PathTransformReset(true);
  f.mUI->DrawSVG(svg, IRECT(0, 0, 100, 50), nullptr, &stroke, &fill);

  REQUIRE(SameColor(f.mUI->LastOp(DrawOp::EType::Fill)->mColor, COLOR_ORANGE));
  REQUIRE(SameColor(f.mUI->LastOp(DrawOp::EType::Stroke)->mColor, COLOR_GREEN));
}

TEST_CASE("SVG: ISVGControl and ISVGKnobControl draw their image", "[integration][drawing][svg][controls]")
{
  TestEditorDelegate dlg {1, 400, 300};
  dlg.GetParam(0)->InitDouble("Gain", 50., 0., 100., 0.01);
  auto* pUI = dlg.OpenUI();
  ISVG svg = pUI->LoadSVG("test_image.svg", static_cast<const void*>(kTestSVG), static_cast<int>(strlen(kTestSVG)));
  const IRECT imageBounds = IRECT(20, 20, 220, 120);
  const IRECT knobBounds = IRECT(250, 20, 350, 120);

  pUI->AttachControl(new ISVGControl(imageBounds, svg));
  auto* pKnob = pUI->AttachControl(new ISVGKnobControl(knobBounds, svg, 0));
  dlg.SendCurrentParamValuesFromDelegate();
  pUI->SetStrictDrawing(true);
  pUI->RenderFrame();

  REQUIRE(pUI->CountOps(DrawOp::EType::Fill) == 2);
  RequireRect(pUI->mDrawOps[0].mDeviceBounds, imageBounds, 0.5f);

  // The knob is half way round, so its image is not rotated, and it is centred on the control
  const DrawOp* pKnobFill = pUI->LastOp(DrawOp::EType::Fill);
  REQUIRE(pKnobFill->mDeviceBounds.MW() == Approx(knobBounds.MW()).margin(0.5));
  REQUIRE(pKnobFill->mDeviceBounds.W() == Approx(100.f).margin(0.5));
  REQUIRE(pKnobFill->mDeviceBounds.H() == Approx(50.f).margin(0.5));

  // Dragging the knob rotates the image
  pUI->Drag(knobBounds.MW(), knobBounds.MH(), knobBounds.MW(), knobBounds.MH() - 5000.f);
  REQUIRE(pKnob->GetValue() == Approx(1.));
  pUI->ClearRecorded();
  pUI->RenderUntilClean();
  REQUIRE(pUI->LastOp(DrawOp::EType::Fill)->mDeviceBounds.H() > 60.f);
}

#pragma mark - Debug aids

TEST_CASE("Debug aids: showing control bounds and the area drawn can be switched on and off", "[integration][drawing][debug]")
{
  Fixture f;

  REQUIRE_FALSE(f.mUI->ShowControlBoundsEnabled());
  f.mUI->ShowControlBounds(true);
  REQUIRE(f.mUI->ShowControlBoundsEnabled());
  // Switching it on redraws everything
  REQUIRE(f.mUI->RenderFrame());

#ifndef NDEBUG
  // In debug builds each control gets an outline
  int nOutlines = 0;
  for (auto& op : f.mUI->mDrawOps)
    nOutlines += (op.mType == DrawOp::EType::Stroke && (op.mBounds == kBoundsA || op.mBounds == kBoundsB));
  REQUIRE(nOutlines >= 2);
#endif

  f.mUI->ShowControlBounds(false);
  REQUIRE_FALSE(f.mUI->ShowControlBoundsEnabled());
  f.mUI->ClearRecorded();
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Stroke) == 0);

  REQUIRE_FALSE(f.mUI->ShowAreaDrawnEnabled());
  f.mUI->ShowAreaDrawn(true);
  REQUIRE(f.mUI->ShowAreaDrawnEnabled());
  f.mA->SetDirty(false);
  REQUIRE(f.mUI->RenderFrame());
  f.mUI->ShowAreaDrawn(false);
  REQUIRE_FALSE(f.mUI->ShowAreaDrawnEnabled());
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE_FALSE(f.mUI->RenderFrame());
}

TEST_CASE("Debug aids: the FPS display can be shown and hidden", "[integration][drawing][debug]")
{
  Fixture f;
  const IRECT displayBounds = IRECT(200, 10, 390, 60);
  f.mUI->SetFPSDisplayBounds(displayBounds);

  f.mUI->ShowFPSDisplay(true);
  REQUIRE(f.mUI->ShowingFPSDisplay());
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->mLastDirtyBounds.Contains(displayBounds));

  // It draws something of its own on top of the UI
  f.mUI->ClearRecorded();
  f.mUI->SetAllControlsDirty();
  f.mUI->SetStrictDrawing(true);
  f.mUI->RenderFrame();
  REQUIRE(f.mUI->mDrawOps.size() > 3);

  // Showing it twice changes nothing
  f.mUI->ShowFPSDisplay(true);
  REQUIRE(f.mUI->ShowingFPSDisplay());

  f.mUI->ShowFPSDisplay(false);
  REQUIRE_FALSE(f.mUI->ShowingFPSDisplay());
  f.mUI->ClearRecorded();
  f.mUI->RenderFrame();
  REQUIRE(f.mUI->mDrawOps.size() == 3);
}

TEST_CASE("Debug aids: live edit mode can be switched on and off", "[integration][drawing][debug]")
{
  Fixture f;

  REQUIRE_FALSE(f.mUI->LiveEditEnabled());
  f.mUI->MouseDown(50, 50);
  f.mUI->EnableLiveEdit(true);

#if !defined(NDEBUG) || defined(IPLUG_LIVE_EDIT)
  REQUIRE(f.mUI->LiveEditEnabled());
  // Entering live edit ends any gesture that was in progress
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
#else
  REQUIRE_FALSE(f.mUI->LiveEditEnabled());
#endif

  f.mUI->RenderUntilClean();
  f.mUI->EnableLiveEdit(false);
  REQUIRE_FALSE(f.mUI->LiveEditEnabled());
  f.mUI->RenderUntilClean();
  f.mUI->MouseUp(50, 50);

  // Back to normal: controls get their events again
  f.mA->ClearEvents();
  f.mUI->Click(50, 50);
  REQUIRE(f.mA->Count("down") == 1);
}
