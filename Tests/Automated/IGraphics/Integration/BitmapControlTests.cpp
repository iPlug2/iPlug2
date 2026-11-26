/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests loading bitmaps from memory and the bitmap (IB) control library: frame selection, layout and gestures

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestEditorDelegate.h"
#include "TestControls.h"
#include "IControls.h"
#include "ISender.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;
using ParamEvent = TestEditorDelegate::ParamEvent;
using DrawOp = IGraphicsHeadless::DrawOp;

namespace
{
enum EParams { kGain = 0, kOnOff, kMode, kNumParams };

const int kFrameSize = 40;

struct Fixture
{
  Fixture()
  : mDlg(kNumParams, 400, 300)
  {
    mDlg.GetParam(kGain)->InitDouble("Gain", 0., 0., 100., 0.01, "%");
    mDlg.GetParam(kOnOff)->InitBool("OnOff", false);
    mDlg.GetParam(kMode)->InitEnum("Mode", 0, {"Sine", "Saw", "Square"});
    mUI = mDlg.OpenUI();
    mUI->AttachPanelBackground(COLOR_GRAY);
  }

  /** Loads a film strip of nFrames square frames from an in-memory PNG header */
  IBitmap LoadStrip(const char* name, int nFrames, bool horizontal = false)
  {
    const std::vector<uint8_t> png = horizontal ? MakePNGHeader(kFrameSize * nFrames, kFrameSize) : MakePNGHeader(kFrameSize, kFrameSize * nFrames);
    return mUI->LoadBitmap(name, png.data(), static_cast<int>(png.size()), nFrames, horizontal);
  }

  template <class T>
  T* Attach(T* pControl)
  {
    mUI->AttachControl(pControl);
    mDlg.SendCurrentParamValuesFromDelegate();
    mUI->RenderUntilClean(4);
    mUI->ClearRecorded();
    mDlg.ClearRecorded();
    return pControl;
  }

  /** Redraw everything and return the bitmap operations */
  std::vector<DrawOp> RedrawBitmaps()
  {
    mUI->ClearRecorded();
    mUI->SetStrictDrawing(true);
    mUI->RenderFrame();

    std::vector<DrawOp> ops;
    for (auto& op : mUI->mDrawOps)
    {
      if (op.mType == DrawOp::EType::Bitmap)
        ops.push_back(op);
    }
    return ops;
  }

  /** The (zero based) frame of a vertical film strip that the control drew */
  int DrawnFrame()
  {
    const std::vector<DrawOp> ops = RedrawBitmaps();
    REQUIRE(ops.size() == 1);
    REQUIRE(ops[0].mSrcY % kFrameSize == 0);
    return ops[0].mSrcY / kFrameSize;
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
};
} // namespace

#pragma mark - Loading

TEST_CASE("LoadBitmap: a bitmap loaded from memory reports its size and frames", "[integration][bitmap]")
{
  Fixture f;

  SECTION("a single image")
  {
    const std::vector<uint8_t> png = MakePNGHeader(120, 80);
    IBitmap bitmap = f.mUI->LoadBitmap("single.png", png.data(), static_cast<int>(png.size()));

    REQUIRE(bitmap.IsValid());
    REQUIRE(bitmap.W() == 120);
    REQUIRE(bitmap.H() == 80);
    REQUIRE(bitmap.N() == 1);
    REQUIRE(bitmap.FW() == 120);
    REQUIRE(bitmap.FH() == 80);
    REQUIRE(bitmap.GetScale() == Approx(1.f));
    REQUIRE(std::string(bitmap.GetResourceName().Get()) == "single.png");
  }

  SECTION("a vertical film strip")
  {
    IBitmap bitmap = f.LoadStrip("vertical.png", 10);

    REQUIRE(bitmap.W() == kFrameSize);
    REQUIRE(bitmap.H() == kFrameSize * 10);
    REQUIRE(bitmap.N() == 10);
    REQUIRE(bitmap.FW() == kFrameSize);
    REQUIRE(bitmap.FH() == kFrameSize);
    REQUIRE_FALSE(bitmap.GetFramesAreHorizontal());
  }

  SECTION("a horizontal film strip")
  {
    IBitmap bitmap = f.LoadStrip("horizontal.png", 5, true);

    REQUIRE(bitmap.W() == kFrameSize * 5);
    REQUIRE(bitmap.H() == kFrameSize);
    REQUIRE(bitmap.FW() == kFrameSize);
    REQUIRE(bitmap.FH() == kFrameSize);
    REQUIRE(bitmap.GetFramesAreHorizontal());
  }
}

TEST_CASE("LoadBitmap: a file type the drawing backend cannot decode gives an invalid bitmap", "[integration][bitmap]")
{
  Fixture f;
  const std::vector<uint8_t> png = MakePNGHeader(10, 10);

  IBitmap bitmap = f.mUI->LoadBitmap("image.tiff", png.data(), static_cast<int>(png.size()));
  REQUIRE_FALSE(bitmap.IsValid());
}

TEST_CASE("LoadBitmap: bitmaps are cached by name", "[integration][bitmap][cache]")
{
  Fixture f;
  const std::vector<uint8_t> png = MakePNGHeader(64, 32);
  const std::vector<uint8_t> other = MakePNGHeader(20, 20);

  IBitmap first = f.mUI->LoadBitmap("cached.png", png.data(), static_cast<int>(png.size()));
  // The second load finds the first in the cache, so the data is not even looked at
  IBitmap second = f.mUI->LoadBitmap("cached.png", other.data(), static_cast<int>(other.size()));

  REQUIRE(second.GetAPIBitmap() == first.GetAPIBitmap());
  REQUIRE(second.W() == 64);

  // The cache is shared between IGraphics instances while any of them is open
  TestEditorDelegate dlg2 {0, 100, 100};
  auto* pUI2 = dlg2.OpenUI();
  IBitmap third = pUI2->LoadBitmap("cached.png", other.data(), static_cast<int>(other.size()));
  REQUIRE(third.GetAPIBitmap() == first.GetAPIBitmap());
  dlg2.CloseUI();

  f.mUI->ReleaseBitmap(first);
  IBitmap fourth = f.mUI->LoadBitmap("cached.png", other.data(), static_cast<int>(other.size()));
  REQUIRE(fourth.W() == 20);
}

TEST_CASE("DrawBitmap: selects the frame of a film strip by its source offset", "[integration][bitmap][drawing]")
{
  Fixture f;
  IBitmap vertical = f.LoadStrip("draw_vertical.png", 4);
  IBitmap horizontal = f.LoadStrip("draw_horizontal.png", 4, true);
  const IRECT bounds = IRECT(10, 10, 50, 50);

  f.mUI->DrawBitmap(vertical, bounds, 1);
  f.mUI->DrawBitmap(vertical, bounds, 3);
  f.mUI->DrawBitmap(horizontal, bounds, 4);
  // Out of range states are clamped
  f.mUI->DrawBitmap(vertical, bounds, 9);
  f.mUI->DrawBitmap(vertical, bounds, -2);

  REQUIRE(f.mUI->mDrawOps.size() == 5);
  REQUIRE(f.mUI->mDrawOps[0].mSrcY == 0);
  REQUIRE(f.mUI->mDrawOps[1].mSrcY == 2 * kFrameSize);
  REQUIRE(f.mUI->mDrawOps[1].mSrcX == 0);
  REQUIRE(f.mUI->mDrawOps[2].mSrcX == 3 * kFrameSize);
  REQUIRE(f.mUI->mDrawOps[2].mSrcY == 0);
  REQUIRE(f.mUI->mDrawOps[3].mSrcY == 3 * kFrameSize);
  REQUIRE(f.mUI->mDrawOps[4].mSrcY == 0);
  REQUIRE(f.mUI->mDrawOps[0].mBounds == bounds);
}

#pragma mark - IBitmapControl

TEST_CASE("IBitmapControl: takes its size from the bitmap and draws it", "[integration][controls][bitmap][drawing]")
{
  Fixture f;
  const std::vector<uint8_t> png = MakePNGHeader(120, 80);
  IBitmap bitmap = f.mUI->LoadBitmap("logo.png", png.data(), static_cast<int>(png.size()));
  auto* pControl = f.Attach(new IBitmapControl(30, 40, bitmap));

  REQUIRE(pControl->GetRECT() == IRECT(30, 40, 150, 120));

  const std::vector<DrawOp> ops = f.RedrawBitmaps();
  REQUIRE(ops.size() == 1);
  REQUIRE(ops[0].mBounds == IRECT(30, 40, 150, 120));
  REQUIRE(ops[0].mSrcX == 0);
  REQUIRE(ops[0].mSrcY == 0);
}

TEST_CASE("IBitmapControl: a film strip linked to a parameter shows the frame for the value", "[integration][controls][bitmap][drawing][params]")
{
  Fixture f;
  auto* pControl = f.Attach(new IBitmapControl(30, 40, f.LoadStrip("indicator.png", 5), kGain));

  REQUIRE(pControl->GetRECT() == IRECT(30, 40, 30 + kFrameSize, 40 + kFrameSize));
  REQUIRE(f.DrawnFrame() == 0);

  f.mDlg.SendParameterValueFromDelegate(kGain, 0.5, true);
  REQUIRE(f.DrawnFrame() == 2);
  f.mDlg.SendParameterValueFromDelegate(kGain, 1., true);
  REQUIRE(f.DrawnFrame() == 4);
}

#pragma mark - IBKnobControl

TEST_CASE("IBKnobControl: the frame that is drawn follows the value", "[integration][controls][bitmap][knob][drawing]")
{
  Fixture f;
  const int nFrames = 11;
  auto* pKnob = f.Attach(new IBKnobControl(100, 100, f.LoadStrip("knob.png", nFrames), kGain));
  const IRECT bounds = IRECT(100, 100, 100 + kFrameSize, 100 + kFrameSize);

  REQUIRE(pKnob->GetRECT() == bounds);
  REQUIRE(f.DrawnFrame() == 0);

  for (int frame = 0; frame < nFrames; frame++)
  {
    f.mDlg.SendParameterValueFromDelegate(kGain, frame / static_cast<double>(nFrames - 1), true);
    REQUIRE(f.DrawnFrame() == frame);
  }

  // Values in between round to the nearest frame
  f.mDlg.SendParameterValueFromDelegate(kGain, 0.26, true);
  REQUIRE(f.DrawnFrame() == 3);
  f.mDlg.SendParameterValueFromDelegate(kGain, 0.24, true);
  REQUIRE(f.DrawnFrame() == 2);

  const std::vector<DrawOp> ops = f.RedrawBitmaps();
  REQUIRE(ops[0].mBounds == bounds);
}

TEST_CASE("IBKnobControl: dragging changes the parameter and the frame", "[integration][controls][bitmap][knob][mouse][params]")
{
  Fixture f;
  auto* pKnob = f.Attach(new IBKnobControl(100, 100, f.LoadStrip("knob_drag.png", 11), kGain));
  const float x = 100.f + kFrameSize / 2.f, y = 100.f + kFrameSize / 2.f;

  f.mUI->MouseDown(x, y);
  REQUIRE(f.mUI->mCursorHidden);
  f.mUI->MouseDragTo(x, y - 20.f);
  const double v = pKnob->GetValue();
  REQUIRE(v > 0.);
  REQUIRE(v < 1.);
  f.mUI->MouseDragTo(x, y - 5000.f);
  f.mUI->MouseUp(x, y - 5000.f);

  REQUIRE(pKnob->GetValue() == Approx(1.));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(100.));
  REQUIRE(f.mDlg.mParamEvents.front().mType == ParamEvent::EType::Begin);
  REQUIRE(f.mDlg.mParamEvents.back().mType == ParamEvent::EType::End);
  REQUIRE(f.DrawnFrame() == 10);
  REQUIRE_FALSE(f.mUI->mCursorHidden);
}

TEST_CASE("IBKnobControl: given bounds bigger than a frame, the knob is centred in them", "[integration][controls][bitmap][knob]")
{
  Fixture f;
  auto* pKnob = f.Attach(new IBKnobControl(IRECT(100, 100, 200, 200), f.LoadStrip("knob_centred.png", 4), kGain));

  REQUIRE(pKnob->GetRECT() == IRECT(130, 130, 170, 170));
  REQUIRE(f.RedrawBitmaps()[0].mBounds == IRECT(130, 130, 170, 170));

  // Outside the knob is not part of the control
  f.mUI->Drag(105, 105, 105, 50);
  REQUIRE(pKnob->GetValue() == Approx(0.));
}

TEST_CASE("IBKnobRotaterControl: rotates a single image about the centre of the control", "[integration][controls][bitmap][knob][drawing]")
{
  Fixture f;
  const std::vector<uint8_t> png = MakePNGHeader(kFrameSize, kFrameSize);
  IBitmap bitmap = f.mUI->LoadBitmap("rotater.png", png.data(), static_cast<int>(png.size()));
  auto* pKnob = f.Attach(new IBKnobRotaterControl(100, 100, bitmap, kGain));
  const IRECT bounds = pKnob->GetRECT();

  // Half way round the knob is pointing straight up: no rotation
  f.mDlg.SendParameterValueFromDelegate(kGain, 0.5, true);
  std::vector<DrawOp> ops = f.RedrawBitmaps();
  REQUIRE(ops.size() == 1);
  REQUIRE(ops[0].mDeviceBounds.L == Approx(bounds.L).margin(0.01));
  REQUIRE(ops[0].mDeviceBounds.T == Approx(bounds.T).margin(0.01));
  REQUIRE(ops[0].mDeviceBounds.R == Approx(bounds.R).margin(0.01));
  REQUIRE(ops[0].mDeviceBounds.B == Approx(bounds.B).margin(0.01));

  // At other values the image is rotated, so its corners stick out, but the centre stays put
  f.mDlg.SendParameterValueFromDelegate(kGain, 0.25, true);
  ops = f.RedrawBitmaps();
  REQUIRE(ops[0].mDeviceBounds.W() > bounds.W() + 1.f);
  REQUIRE(ops[0].mDeviceBounds.MW() == Approx(bounds.MW()).margin(0.01));
  REQUIRE(ops[0].mDeviceBounds.MH() == Approx(bounds.MH()).margin(0.01));
}

#pragma mark - IBSwitchControl

TEST_CASE("IBSwitchControl: clicks step through the frames and the parameter values", "[integration][controls][bitmap][switch][params]")
{
  Fixture f;
  auto* pSwitch = f.Attach(new IBSwitchControl(100, 100, f.LoadStrip("switch.png", 3), kMode));
  const float x = 120.f, y = 120.f;

  REQUIRE(f.DrawnFrame() == 0);

  f.mUI->Click(x, y);
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 1);
  REQUIRE(pSwitch->GetValue() == Approx(0.5));
  REQUIRE(f.DrawnFrame() == 1);

  f.mUI->Click(x, y);
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 2);
  REQUIRE(f.DrawnFrame() == 2);

  f.mUI->Click(x, y);
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 0);
  REQUIRE(f.DrawnFrame() == 0);

  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kMode) == 3);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kMode) == 3);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End, kMode) == 3);
}

TEST_CASE("IBSwitchControl: a two-frame switch toggles a bool parameter", "[integration][controls][bitmap][switch][params]")
{
  Fixture f;
  f.Attach(new IBSwitchControl(100, 100, f.LoadStrip("toggle.png", 2), kOnOff));

  f.mUI->Click(120, 120);
  REQUIRE(f.mDlg.GetParam(kOnOff)->Bool());
  REQUIRE(f.DrawnFrame() == 1);

  f.mUI->Click(120, 120);
  REQUIRE_FALSE(f.mDlg.GetParam(kOnOff)->Bool());
  REQUIRE(f.DrawnFrame() == 0);

  f.mDlg.SendParameterValueFromDelegate(kOnOff, 1., true);
  REQUIRE(f.DrawnFrame() == 1);
}

#pragma mark - IBButtonControl

TEST_CASE("IBButtonControl: shows the pressed frame while its click animation runs", "[integration][controls][bitmap][button][animation]")
{
  Fixture f;
  int nActions = 0;
  auto* pButton = f.Attach(new IBButtonControl(100, 100, f.LoadStrip("button.png", 2), [&](IControl* pCaller) { nActions++; DefaultClickActionFunc(pCaller); }));

  REQUIRE(pButton->GetRECT() == IRECT(100, 100, 140, 140));
  REQUIRE(f.DrawnFrame() == 0);

  f.mUI->Click(120, 120);
  REQUIRE(nActions == 1);
  REQUIRE(pButton->GetValue() == Approx(1.));
  REQUIRE(f.DrawnFrame() == 1);

  FinishAnimation(pButton);
  REQUIRE(pButton->GetValue() == Approx(0.));
  REQUIRE(f.DrawnFrame() == 0);
}

#pragma mark - IBSliderControl

TEST_CASE("IBSliderControl: the handle bitmap is drawn at the position for the value", "[integration][controls][bitmap][slider][drawing]")
{
  Fixture f;
  const std::vector<uint8_t> handlePNG = MakePNGHeader(20, 10);
  IBitmap handle = f.mUI->LoadBitmap("handle.png", handlePNG.data(), static_cast<int>(handlePNG.size()));

  SECTION("vertical")
  {
    auto* pSlider = f.Attach(new IBSliderControl(100.f, 50.f, 210.f, handle, IBitmap(), kGain));
    REQUIRE(pSlider->GetRECT() == IRECT(100, 50, 120, 260));

    // The minimum is at the bottom
    REQUIRE(pSlider->GetHandleBounds() == IRECT(100, 250, 120, 260));
    REQUIRE(pSlider->GetHandleBounds(1.) == IRECT(100, 50, 120, 60));
    REQUIRE(pSlider->GetHandleBounds(0.5).MH() == Approx(155.f));

    std::vector<DrawOp> ops = f.RedrawBitmaps();
    REQUIRE(ops.size() == 1);
    REQUIRE(ops[0].mBounds == IRECT(100, 250, 120, 260));

    f.mDlg.SendParameterValueFromDelegate(kGain, 1., true);
    ops = f.RedrawBitmaps();
    REQUIRE(ops[0].mBounds == IRECT(100, 50, 120, 60));
  }

  SECTION("horizontal")
  {
    auto* pSlider = f.Attach(new IBSliderControl(100.f, 50.f, 220.f, handle, IBitmap(), kGain, EDirection::Horizontal));
    REQUIRE(pSlider->GetRECT() == IRECT(100, 50, 320, 60));

    // The minimum is on the left
    REQUIRE(pSlider->GetHandleBounds() == IRECT(100, 50, 120, 60));
    REQUIRE(pSlider->GetHandleBounds(1.) == IRECT(300, 50, 320, 60));

    f.mDlg.SendParameterValueFromDelegate(kGain, 0.5, true);
    const std::vector<DrawOp> ops = f.RedrawBitmaps();
    REQUIRE(ops[0].mBounds.MW() == Approx(210.f));
  }
}

TEST_CASE("IBSliderControl: a track bitmap is drawn beneath the handle", "[integration][controls][bitmap][slider][drawing]")
{
  Fixture f;
  const std::vector<uint8_t> handlePNG = MakePNGHeader(20, 10);
  const std::vector<uint8_t> trackPNG = MakePNGHeader(6, 200);
  IBitmap handle = f.mUI->LoadBitmap("handle2.png", handlePNG.data(), static_cast<int>(handlePNG.size()));
  IBitmap track = f.mUI->LoadBitmap("track.png", trackPNG.data(), static_cast<int>(trackPNG.size()));
  const IRECT bounds = IRECT(100, 50, 140, 250);
  f.Attach(new IBSliderControl(bounds, handle, track, kGain));

  const std::vector<DrawOp> ops = f.RedrawBitmaps();
  REQUIRE(ops.size() == 2);
  // The track is centred in the control
  REQUIRE(ops[0].mBounds == IRECT(117, 50, 123, 250));
  REQUIRE(bounds.Contains(ops[1].mBounds));
  REQUIRE(ops[1].mBounds.W() == Approx(20.f));
  REQUIRE(ops[1].mBounds.H() == Approx(10.f));
}

TEST_CASE("IBSliderControl: clicking and dragging sets the parameter", "[integration][controls][bitmap][slider][mouse][params]")
{
  Fixture f;
  const std::vector<uint8_t> handlePNG = MakePNGHeader(20, 10);
  IBitmap handle = f.mUI->LoadBitmap("handle3.png", handlePNG.data(), static_cast<int>(handlePNG.size()));
  auto* pSlider = f.Attach(new IBSliderControl(100.f, 50.f, 200.f, handle, IBitmap(), kGain));

  f.mUI->Click(110, 100);
  REQUIRE(pSlider->GetValue() == Approx(0.75));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(75.));

  f.mUI->Click(110, 240);
  REQUIRE(pSlider->GetValue() == Approx(0.05));
  f.mUI->Click(110, 60);
  REQUIRE(pSlider->GetValue() == Approx(0.95));

  f.mUI->MouseDown(110, 150);
  REQUIRE(pSlider->GetValue() == Approx(0.5));
  f.mUI->MouseDragTo(110, 130);
  REQUIRE(pSlider->GetValue() > 0.5);
  f.mUI->MouseDragTo(110, 190);
  REQUIRE(pSlider->GetValue() < 0.5);
  f.mUI->MouseUp(110, 190);
  REQUIRE(f.mDlg.GetParam(kGain)->GetNormalized() == Approx(pSlider->GetValue()));
}

#pragma mark - IBTextControl

TEST_CASE("IBTextControl: draws one character cell of the font bitmap per character", "[integration][controls][bitmap][text][drawing]")
{
  Fixture f;
  // A font strip with 96 character frames (ASCII 32-127), 6 x 12 each
  const std::vector<uint8_t> png = MakePNGHeader(6, 12 * 96);
  IBitmap font = f.mUI->LoadBitmap("font.png", png.data(), static_cast<int>(png.size()), 96);
  const IRECT bounds = IRECT(100, 50, 200, 70);
  auto* pText = f.Attach(new IBTextControl(bounds, font, IText(12.f, EAlign::Near), "AB C", 6, 12));

  std::vector<DrawOp> ops = f.RedrawBitmaps();
  REQUIRE(ops.size() == 4);

  for (size_t i = 0; i < ops.size(); i++)
  {
    REQUIRE(ops[i].mBounds.W() == Approx(6.f));
    REQUIRE(ops[i].mBounds.H() == Approx(12.f));
    REQUIRE(ops[i].mBounds.L == Approx(bounds.L + 6.f * static_cast<float>(i)));
    REQUIRE(ops[i].mBounds.MH() == Approx(bounds.MH()));
  }

  // 'A' and 'B' are neighbouring frames; the two spaces are the same frame
  REQUIRE(ops[1].mSrcY == ops[0].mSrcY + 12);
  REQUIRE(ops[3].mSrcY - ops[2].mSrcY == ('C' - ' ') * 12);

  pText->SetStr("Hello, world");
  ops = f.RedrawBitmaps();
  REQUIRE(ops.size() == 12);
}

#pragma mark - IBMeterControl

TEST_CASE("IBMeterControl: shows the frame for the level sent from the processor", "[integration][controls][bitmap][meter][sender]")
{
  Fixture f;
  auto* pMeter = f.Attach(new IBMeterControl(100, 100, f.LoadStrip("meter.png", 11), IBMeterControl::EResponse::Linear));

  auto send = [&](float average) {
    ISenderData<1, std::pair<float, float>> data {kNoTag, 1, 0};
    data.vals[0] = std::make_pair(average, average);
    pMeter->OnMsgFromDelegate(ISender<>::kUpdateMessage, sizeof(data), &data);
  };

  REQUIRE(f.DrawnFrame() == 0);

  send(0.5f);
  REQUIRE(pMeter->GetValue() == Approx(0.5));
  REQUIRE(pMeter->IsDirty());
  REQUIRE(f.DrawnFrame() == 5);

  send(4.f);
  REQUIRE(pMeter->GetValue() == Approx(1.));
  REQUIRE(f.DrawnFrame() == 10);

  // Other messages are ignored
  ISenderData<1, std::pair<float, float>> data {kNoTag, 1, 0};
  pMeter->OnMsgFromDelegate(ISender<>::kUpdateMessage + 1, sizeof(data), &data);
  REQUIRE(pMeter->GetValue() == Approx(1.));

  // ... as are levels sent while the meter is disabled
  pMeter->SetDisabled(true);
  send(0.f);
  REQUIRE(pMeter->GetValue() == Approx(1.));
}

#pragma mark - Screen scale

// Rescaling looks for a resource at the new scale, which on the Web means the page's preloaded images
#ifndef OS_WEB
TEST_CASE("Bitmap controls: keep their size and frame when the screen scale changes", "[integration][controls][bitmap][resize]")
{
  Fixture f;
  auto* pKnob = f.Attach(new IBKnobControl(100, 100, f.LoadStrip("knob_scaled.png", 11), kGain));
  f.mDlg.SendParameterValueFromDelegate(kGain, 0.3, true);
  REQUIRE(f.DrawnFrame() == 3);

  f.mUI->SetScreenScale(2.f);

  REQUIRE(pKnob->GetRECT() == IRECT(100, 100, 140, 140));
  const std::vector<DrawOp> ops = f.RedrawBitmaps();
  REQUIRE(ops.size() == 1);
  REQUIRE(ops[0].mBounds == IRECT(100, 100, 140, 140));
  REQUIRE(ops[0].mSrcY == 3 * kFrameSize);
}
#endif
