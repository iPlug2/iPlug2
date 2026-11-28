/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the keyboard, meter, scope, display, multi-slider, LED and spectrum analyzer controls

#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestEditorDelegate.h"
#include "TestControls.h"
#include "IControls.h"
#include "ISender.h"
#include "IVKeyboardControl.h"
#include "IVMeterControl.h"
#include "IVScopeControl.h"
#include "IVDisplayControl.h"
#include "IVMultiSliderControl.h"
#include "IVSpectrumAnalyzerControl.h"
#include "ILEDControl.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;
using ParamEvent = TestEditorDelegate::ParamEvent;
using DrawOp = IGraphicsHeadless::DrawOp;

namespace
{
enum EParams { kStep0 = 0, kStep1, kStep2, kStep3, kNumParams };
enum ECtrlTags { kTagControl = 0 };

const IVStyle kPlainStyle = DEFAULT_STYLE.WithShowLabel(false).WithShowValue(false);

struct Fixture
{
  Fixture()
  : mDlg(kNumParams, 600, 400)
  {
    for (int i = 0; i < kNumParams; i++)
      mDlg.GetParam(i)->InitDouble("Step", 0., 0., 100., 0.1, "%");

    mUI = mDlg.OpenUI();
    mUI->AttachPanelBackground(COLOR_GRAY);
  }

  template <class T>
  T* Attach(T* pControl)
  {
    mUI->AttachControl(pControl, kTagControl);
    mDlg.SendCurrentParamValuesFromDelegate();
    mUI->RenderUntilClean(4);
    mUI->ClearRecorded();
    mDlg.ClearRecorded();
    return pControl;
  }

  void Redraw()
  {
    mUI->ClearRecorded();
    mUI->SetAllControlsDirty();
    mUI->SetStrictDrawing(true);
    mUI->RenderFrame();
  }

  /** @return true if the last frame filled a rectangle of about the given height inside bounds */
  bool DrewFillOfHeight(const IRECT& bounds, float height, float margin = 4.f)
  {
    for (auto& op : mUI->mDrawOps)
    {
      if (op.mType == DrawOp::EType::Fill && bounds.GetPadded(1.f).Contains(op.mBounds) && std::fabs(op.mBounds.H() - height) <= margin && op.mBounds.W() > 2.f)
        return true;
    }
    return false;
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
};

bool SameColor(const IColor& a, const IColor& b)
{
  return a.A == b.A && a.R == b.R && a.G == b.G && a.B == b.B;
}
} // namespace

#pragma mark - IVKeyboardControl

namespace
{
// Two octaves from C3: 15 white keys, each 20 points wide
const IRECT kKeyboardBounds = IRECT(100, 100, 400, 200);
const float kWhiteKeyWidth = 20.f;

/** A point near the bottom of the n-th white key, below the black keys */
float WhiteKeyX(int whiteKeyIdx) { return kKeyboardBounds.L + (static_cast<float>(whiteKeyIdx) + 0.5f) * kWhiteKeyWidth; }
const float kWhiteKeyY = kKeyboardBounds.B - 5.f;
} // namespace

TEST_CASE("IVKeyboardControl: clicking a key sends a note on, releasing it sends the note off", "[integration][controls][keyboard-control][midi]")
{
  Fixture f;
  f.Attach(new IVKeyboardControl(kKeyboardBounds, 48, 72));

  // The third white key is E
  f.mUI->MouseDown(WhiteKeyX(2), kWhiteKeyY);

  REQUIRE(f.mDlg.mMidiFromUI.size() == 1);
  REQUIRE(f.mDlg.mMidiFromUI[0].StatusMsg() == IMidiMsg::kNoteOn);
  REQUIRE(f.mDlg.mMidiFromUI[0].NoteNumber() == 52);
  REQUIRE(f.mDlg.mMidiFromUI[0].Velocity() > 0);

  f.mUI->MouseUp(WhiteKeyX(2), kWhiteKeyY);

  REQUIRE(f.mDlg.mMidiFromUI.size() == 2);
  REQUIRE(f.mDlg.mMidiFromUI[1].StatusMsg() == IMidiMsg::kNoteOff);
  REQUIRE(f.mDlg.mMidiFromUI[1].NoteNumber() == 52);
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("IVKeyboardControl: every white key plays the right note", "[integration][controls][keyboard-control][midi]")
{
  Fixture f;
  f.Attach(new IVKeyboardControl(kKeyboardBounds, 48, 72));
  const int whiteNotes[15] = {48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65, 67, 69, 71, 72};

  for (int i = 0; i < 15; i++)
  {
    f.mDlg.mMidiFromUI.clear();
    f.mUI->Click(WhiteKeyX(i), kWhiteKeyY);
    REQUIRE(f.mDlg.mMidiFromUI.size() == 2);
    REQUIRE(f.mDlg.mMidiFromUI[0].StatusMsg() == IMidiMsg::kNoteOn);
    REQUIRE(f.mDlg.mMidiFromUI[0].NoteNumber() == whiteNotes[i]);
    REQUIRE(f.mDlg.mMidiFromUI[1].StatusMsg() == IMidiMsg::kNoteOff);
    REQUIRE(f.mDlg.mMidiFromUI[1].NoteNumber() == whiteNotes[i]);
  }
}

TEST_CASE("IVKeyboardControl: black keys sit on top of the boundaries between white keys", "[integration][controls][keyboard-control][midi]")
{
  Fixture f;
  f.Attach(new IVKeyboardControl(kKeyboardBounds, 48, 72));
  const float blackKeyY = kKeyboardBounds.T + 20.f;

  // Between C and D is C#, between D and E is D#; there is no black key between E and F
  f.mUI->Click(kKeyboardBounds.L + kWhiteKeyWidth, blackKeyY);
  REQUIRE(f.mDlg.mMidiFromUI[0].NoteNumber() == 49);

  f.mDlg.mMidiFromUI.clear();
  f.mUI->Click(kKeyboardBounds.L + 2.f * kWhiteKeyWidth, blackKeyY);
  REQUIRE(f.mDlg.mMidiFromUI[0].NoteNumber() == 51);

  f.mDlg.mMidiFromUI.clear();
  f.mUI->Click(kKeyboardBounds.L + 3.f * kWhiteKeyWidth + 1.f, blackKeyY);
  REQUIRE(f.mDlg.mMidiFromUI[0].NoteNumber() == 53);

  // The same x further down is a white key
  f.mDlg.mMidiFromUI.clear();
  f.mUI->Click(kKeyboardBounds.L + kWhiteKeyWidth + 1.f, kWhiteKeyY);
  REQUIRE(f.mDlg.mMidiFromUI[0].NoteNumber() == 50);
}

TEST_CASE("IVKeyboardControl: the velocity depends on how far down the key is hit", "[integration][controls][keyboard-control][midi]")
{
  Fixture f;
  f.Attach(new IVKeyboardControl(kKeyboardBounds, 48, 72));

  f.mUI->Click(WhiteKeyX(0), kKeyboardBounds.B - 2.f);
  const int bottomVelocity = f.mDlg.mMidiFromUI[0].Velocity();
  f.mDlg.mMidiFromUI.clear();

  f.mUI->Click(WhiteKeyX(0) - 5.f, kKeyboardBounds.T + 10.f);
  const int topVelocity = f.mDlg.mMidiFromUI[0].Velocity();

  REQUIRE(f.mDlg.mMidiFromUI[0].NoteNumber() == 48);
  REQUIRE(bottomVelocity == 127);
  REQUIRE(topVelocity > 0);
  REQUIRE(topVelocity < 40);
}

TEST_CASE("IVKeyboardControl: dragging across keys plays each one and releases the previous", "[integration][controls][keyboard-control][midi][mouse]")
{
  Fixture f;
  f.Attach(new IVKeyboardControl(kKeyboardBounds, 48, 72));

  f.mUI->MouseDown(WhiteKeyX(0), kWhiteKeyY);
  f.mUI->MouseDragTo(WhiteKeyX(0) + 3.f, kWhiteKeyY); // still on C
  REQUIRE(f.mDlg.mMidiFromUI.size() == 1);

  f.mUI->MouseDragTo(WhiteKeyX(1), kWhiteKeyY);
  REQUIRE(f.mDlg.mMidiFromUI.size() == 3);
  REQUIRE(f.mDlg.mMidiFromUI[1].StatusMsg() == IMidiMsg::kNoteOn);
  REQUIRE(f.mDlg.mMidiFromUI[1].NoteNumber() == 50);
  REQUIRE(f.mDlg.mMidiFromUI[2].StatusMsg() == IMidiMsg::kNoteOff);
  REQUIRE(f.mDlg.mMidiFromUI[2].NoteNumber() == 48);

  // Dragging off the end of the keyboard holds the last key
  f.mUI->MouseDragTo(kKeyboardBounds.R + 200.f, kKeyboardBounds.B + 100.f);
  REQUIRE(f.mDlg.mMidiFromUI.back().StatusMsg() == IMidiMsg::kNoteOff);
  const size_t nBeforeUp = f.mDlg.mMidiFromUI.size();

  f.mUI->MouseUp(kKeyboardBounds.R + 200.f, kKeyboardBounds.B + 100.f);
  REQUIRE(f.mDlg.mMidiFromUI.size() == nBeforeUp + 1);
  REQUIRE(f.mDlg.mMidiFromUI.back().StatusMsg() == IMidiMsg::kNoteOff);
  REQUIRE(f.mDlg.mMidiFromUI.back().NoteNumber() == 72);

  // Every note that was started has been stopped
  int nOn = 0, nOff = 0;
  for (auto& msg : f.mDlg.mMidiFromUI)
  {
    nOn += msg.StatusMsg() == IMidiMsg::kNoteOn;
    nOff += msg.StatusMsg() == IMidiMsg::kNoteOff;
  }
  REQUIRE(nOn == nOff);
}

TEST_CASE("IVKeyboardControl: the note range can be changed", "[integration][controls][keyboard-control][midi]")
{
  Fixture f;
  auto* pKeyboard = f.Attach(new IVKeyboardControl(kKeyboardBounds, 48, 72));

  pKeyboard->SetNoteRange(36, 47); // one octave: 7 white keys across the same width
  REQUIRE(pKeyboard->GetRECT().W() == Approx(kKeyboardBounds.W()));

  f.mUI->Click(kKeyboardBounds.L + 5.f, kWhiteKeyY);
  REQUIRE(f.mDlg.mMidiFromUI[0].NoteNumber() == 36);

  f.mDlg.mMidiFromUI.clear();
  f.mUI->Click(kKeyboardBounds.R - 5.f, kWhiteKeyY);
  REQUIRE(f.mDlg.mMidiFromUI[0].NoteNumber() == 47);
}

TEST_CASE("IVKeyboardControl: draws the keys, and notes from MIDI are shown as pressed", "[integration][controls][keyboard-control][midi][drawing]")
{
  Fixture f;
  auto* pKeyboard = f.Attach(new IVKeyboardControl(kKeyboardBounds, 48, 72));

  auto countFills = [&](const IColor& color) {
    f.Redraw();
    int n = 0;
    for (auto& op : f.mUI->mDrawOps)
      n += (op.mType == DrawOp::EType::Fill && SameColor(op.mColor, color) && kKeyboardBounds.Contains(op.mBounds));
    return n;
  };

  REQUIRE(countFills(IVKeyboardControl::DEFAULT_WK_COLOR) == 15);
  REQUIRE(countFills(IVKeyboardControl::DEFAULT_BK_COLOR) == 10);
  REQUIRE(countFills(IVKeyboardControl::DEFAULT_PK_COLOR) == 0);

  pKeyboard->SetNoteFromMidi(60, true);
  REQUIRE(pKeyboard->IsDirty());
  REQUIRE(countFills(IVKeyboardControl::DEFAULT_PK_COLOR) == 1);

  pKeyboard->SetNoteFromMidi(64, true);
  REQUIRE(countFills(IVKeyboardControl::DEFAULT_PK_COLOR) == 2);

  pKeyboard->SetNoteFromMidi(60, false);
  REQUIRE(countFills(IVKeyboardControl::DEFAULT_PK_COLOR) == 1);

  // Notes outside the range of the keyboard are ignored
  pKeyboard->SetNoteFromMidi(20, true);
  pKeyboard->SetNoteFromMidi(100, true);
  REQUIRE(countFills(IVKeyboardControl::DEFAULT_PK_COLOR) == 1);

  pKeyboard->ClearNotesFromMidi();
  REQUIRE(countFills(IVKeyboardControl::DEFAULT_PK_COLOR) == 0);

  // Showing a note does not send it back out
  REQUIRE(f.mDlg.mMidiFromUI.empty());
}

TEST_CASE("IVKeyboardControl: follows MIDI sent from the delegate", "[integration][controls][keyboard-control][midi][delegate]")
{
  Fixture f;
  auto* pKeyboard = f.Attach(new IVKeyboardControl(kKeyboardBounds, 48, 72));
  REQUIRE(pKeyboard->GetWantsMidi());

  auto countPressed = [&]() {
    f.Redraw();
    int n = 0;
    for (auto& op : f.mUI->mDrawOps)
      n += (op.mType == DrawOp::EType::Fill && SameColor(op.mColor, IVKeyboardControl::DEFAULT_PK_COLOR) && kKeyboardBounds.Contains(op.mBounds));
    return n;
  };

  IMidiMsg msg;
  msg.MakeNoteOnMsg(55, 100, 0);
  f.mDlg.SendMidiMsgFromDelegate(msg);
  REQUIRE(pKeyboard->IsDirty());
  REQUIRE(countPressed() == 1);

  msg.MakeNoteOnMsg(57, 100, 0);
  f.mDlg.SendMidiMsgFromDelegate(msg);
  REQUIRE(countPressed() == 2);

  msg.MakeNoteOffMsg(55, 0);
  f.mDlg.SendMidiMsgFromDelegate(msg);
  REQUIRE(countPressed() == 1);

  // A note on with zero velocity is a note off
  msg.MakeNoteOnMsg(57, 0, 0);
  f.mDlg.SendMidiMsgFromDelegate(msg);
  REQUIRE(countPressed() == 0);

  msg.MakeNoteOnMsg(60, 100, 0);
  f.mDlg.SendMidiMsgFromDelegate(msg);
  msg.MakeControlChangeMsg(IMidiMsg::kAllNotesOff, 0., 0);
  f.mDlg.SendMidiMsgFromDelegate(msg);
  REQUIRE(countPressed() == 0);
}

#pragma mark - Meters

TEST_CASE("IVMeterControl: shows the levels sent from the processor, one track per channel", "[integration][controls][meter][sender]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 160, 350);
  auto* pMeter = f.Attach(new IVMeterControl<2>(bounds, "", kPlainStyle));
  REQUIRE(pMeter->NVals() == 2);

  ISenderData<2> data {kTagControl, 2, 0};
  data.vals[0] = 0.25f;
  data.vals[1] = 0.8f;
  f.mDlg.SendControlMsgFromDelegate(kTagControl, ISender<>::kUpdateMessage, sizeof(data), &data);

  REQUIRE(pMeter->GetValue(0) == Approx(0.25));
  REQUIRE(pMeter->GetValue(1) == Approx(0.8));
  REQUIRE(pMeter->IsDirty());
  REQUIRE(f.mDlg.mParamEvents.empty());

  REQUIRE(f.mUI->RenderFrame());
  const float trackHeight = pMeter->GetWidgetBounds().H();
  f.Redraw();
  REQUIRE(f.DrewFillOfHeight(bounds, 0.25f * trackHeight));
  REQUIRE(f.DrewFillOfHeight(bounds, 0.8f * trackHeight));

  // Levels above full scale are clipped
  data.vals[0] = 3.f;
  data.vals[1] = -1.f;
  pMeter->OnMsgFromDelegate(ISender<>::kUpdateMessage, sizeof(data), &data);
  REQUIRE(pMeter->GetValue(0) == Approx(1.));
  REQUIRE(pMeter->GetValue(1) == Approx(0.));
}

TEST_CASE("IVMeterControl: data for a subset of the channels only updates those tracks", "[integration][controls][meter][sender]")
{
  Fixture f;
  auto* pMeter = f.Attach(new IVMeterControl<4>(IRECT(100, 50, 200, 350), "", kPlainStyle));

  ISenderData<4> data {kTagControl, 2, 1};
  data.vals = {0.1f, 0.2f, 0.3f, 0.4f};
  pMeter->OnMsgFromDelegate(ISender<>::kUpdateMessage, sizeof(data), &data);

  REQUIRE(pMeter->GetValue(0) == Approx(0.));
  REQUIRE(pMeter->GetValue(1) == Approx(0.2));
  REQUIRE(pMeter->GetValue(2) == Approx(0.3));
  REQUIRE(pMeter->GetValue(3) == Approx(0.));
}

TEST_CASE("IVMeterControl: ignores other messages, and data sent while it is disabled", "[integration][controls][meter][sender]")
{
  Fixture f;
  auto* pMeter = f.Attach(new IVMeterControl<1>(IRECT(100, 50, 140, 350), "", kPlainStyle));

  ISenderData<1> data {kTagControl, 1, 0};
  data.vals[0] = 0.5f;

  pMeter->OnMsgFromDelegate(ISender<>::kUpdateMessage + 5, sizeof(data), &data);
  REQUIRE(pMeter->GetValue() == Approx(0.));
  REQUIRE_FALSE(pMeter->IsDirty());

  pMeter->SetDisabled(true);
  f.mUI->RenderUntilClean();
  pMeter->OnMsgFromDelegate(ISender<>::kUpdateMessage, sizeof(data), &data);
  REQUIRE(pMeter->GetValue() == Approx(0.));

  pMeter->SetDisabled(false);
  pMeter->OnMsgFromDelegate(ISender<>::kUpdateMessage, sizeof(data), &data);
  REQUIRE(pMeter->GetValue() == Approx(0.5));
}

TEST_CASE("IVMeterControl: with a log response the level is shown in decibels", "[integration][controls][meter][sender]")
{
  Fixture f;
  auto* pMeter = f.Attach(new IVMeterControl<1>(IRECT(100, 50, 140, 350), "", kPlainStyle, EDirection::Vertical, {}, 0, IVMeterControl<1>::EResponse::Log, -60.f, 0.f));

  auto send = [&](float amp) {
    ISenderData<1> data {kTagControl, 1, 0};
    data.vals[0] = amp;
    pMeter->OnMsgFromDelegate(ISender<>::kUpdateMessage, sizeof(data), &data);
  };

  send(1.f); // 0 dB
  REQUIRE(pMeter->GetValue() == Approx(1.));
  send(0.001f); // -60 dB
  REQUIRE(pMeter->GetValue() == Approx(0.).margin(1e-6));
  send(static_cast<float>(DBToAmp(-30.)));
  REQUIRE(pMeter->GetValue() == Approx(0.5).margin(1e-4));
  send(0.f); // silence
  REQUIRE(pMeter->GetValue() == Approx(0.));
}

TEST_CASE("IVMeterControl: draws its label and track names", "[integration][controls][meter][drawing]")
{
  Fixture f;
  f.Attach(new IVMeterControl<2>(IRECT(100, 50, 220, 350), "Output", DEFAULT_STYLE, EDirection::Vertical, {"L", "R"}));

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Output"));
  REQUIRE(f.mUI->DidDrawText("L"));
  REQUIRE(f.mUI->DidDrawText("R"));
  REQUIRE(f.mUI->FindText("L")->mBounds.MW() < f.mUI->FindText("R")->mBounds.MW());
}

TEST_CASE("IVPeakAvgMeterControl: the bar shows the average level and a marker shows the peak", "[integration][controls][meter][sender]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 160, 350);
  auto* pMeter = f.Attach(new IVPeakAvgMeterControl<2>(bounds, "", kPlainStyle, EDirection::Vertical, {}, 0, -60.f, 0.f, {}));

  ISenderData<2, std::pair<float, float>> data {kTagControl, 2, 0};
  data.vals[0] = std::make_pair(1.f, static_cast<float>(DBToAmp(-30.)));  // peak at 0 dB, average at -30 dB
  data.vals[1] = std::make_pair(static_cast<float>(DBToAmp(-15.)), static_cast<float>(DBToAmp(-45.)));
  f.mDlg.SendControlMsgFromDelegate(kTagControl, ISender<>::kUpdateMessage, sizeof(data), &data);

  REQUIRE(pMeter->GetValue(0) == Approx(0.5).margin(1e-4));
  REQUIRE(pMeter->GetValue(1) == Approx(0.25).margin(1e-4));
  REQUIRE(pMeter->IsDirty());

  const float trackHeight = pMeter->GetWidgetBounds().H();
  f.Redraw();
  REQUIRE(f.DrewFillOfHeight(bounds, 0.5f * trackHeight));
  REQUIRE(f.DrewFillOfHeight(bounds, 0.25f * trackHeight));

  // The peak marker of the second channel is a thin bar three quarters of the way up
  const float peakY = pMeter->GetWidgetBounds().B - 0.75f * trackHeight;
  bool drewPeak = false;
  for (auto& op : f.mUI->mDrawOps)
    drewPeak |= (op.mType == DrawOp::EType::Fill && op.mBounds.H() < 6.f && op.mBounds.H() > 0.f && std::fabs(op.mBounds.T - peakY) < 4.f && bounds.Contains(op.mBounds));
  REQUIRE(drewPeak);
}

TEST_CASE("IVLEDMeterControl: lights more segments as the level rises", "[integration][controls][meter][sender][drawing]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 140, 350);
  auto* pMeter = f.Attach(new IVLEDMeterControl<1>(bounds, "", kPlainStyle));

  auto litSegments = [&](double levelDB) {
    ISenderData<1, std::pair<float, float>> data {kTagControl, 1, 0};
    const float amp = static_cast<float>(DBToAmp(levelDB));
    data.vals[0] = std::make_pair(amp, amp);
    pMeter->OnMsgFromDelegate(ISender<>::kUpdateMessage, sizeof(data), &data);
    f.Redraw();

    int n = 0;
    for (auto& op : f.mUI->mDrawOps)
    {
      if (op.mType == DrawOp::EType::Fill && bounds.Contains(op.mBounds))
        n += (SameColor(op.mColor, LED1) || SameColor(op.mColor, LED2) || SameColor(op.mColor, LED3) || SameColor(op.mColor, LED4) || SameColor(op.mColor, LED5));
    }
    return n;
  };

  const int quiet = litSegments(-70.);
  const int medium = litSegments(-30.);
  const int loud = litSegments(-3.);
  const int clipping = litSegments(6.);

  REQUIRE(quiet < medium);
  REQUIRE(medium < loud);
  REQUIRE(loud < clipping);
  REQUIRE(clipping == 13);
}

#pragma mark - IVScopeControl / IVDisplayControl

TEST_CASE("IVScopeControl: plots the buffer sent from the processor", "[integration][controls][scope][sender][drawing]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 400, 250);
  auto* pScope = f.Attach(new IVScopeControl<1, 64>(bounds, "Scope", DEFAULT_STYLE.WithShowValue(false)));
  const IColor traceColor = pScope->GetColor(kFG);

  auto traceBounds = [&]() {
    f.Redraw();
    IRECT r;
    for (auto& op : f.mUI->mDrawOps)
    {
      if (op.mType == DrawOp::EType::Stroke && SameColor(op.mColor, traceColor) && op.mBounds.W() > 200.f)
        r = op.mBounds;
    }
    return r;
  };

  // Silence is a flat line across the middle of the scope
  const IRECT flat = traceBounds();
  REQUIRE(f.mUI->DidDrawText("Scope"));
  REQUIRE(flat.W() > 200.f);
  REQUIRE(flat.H() == Approx(0.f));
  REQUIRE(flat.MH() == Approx(pScope->GetWidgetBounds().MH()).margin(2.f));

  ISenderData<1, std::array<float, 64>> data {kTagControl, 1, 0};
  for (int i = 0; i < 64; i++)
    data.vals[0][static_cast<size_t>(i)] = (i % 2) ? 1.f : -1.f;

  f.mDlg.SendControlMsgFromDelegate(kTagControl, ISender<>::kUpdateMessage, sizeof(data), &data);
  REQUIRE(pScope->IsDirty());

  // A full scale signal fills the height of the scope
  const IRECT full = traceBounds();
  REQUIRE(full.H() > 0.9f * pScope->GetWidgetBounds().H() - 8.f);
  REQUIRE(bounds.Contains(full));
  REQUIRE(full.MH() == Approx(pScope->GetWidgetBounds().MH()).margin(2.f));
}

TEST_CASE("IVDisplayControl: scrolls the values sent from the processor through its buffer", "[integration][controls][display][sender][drawing]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 400, 250);
  auto* pDisplay = f.Attach(new IVDisplayControl(bounds, "", kPlainStyle, EDirection::Horizontal, 0.f, 1.f, 0.f, 16));

  auto traceBounds = [&]() {
    f.Redraw();
    IRECT r;
    for (auto& op : f.mUI->mDrawOps)
    {
      // The trace is the first thing stroked; the frame comes later
      if (op.mType == DrawOp::EType::Stroke && op.mBounds.W() > 200.f && bounds.Contains(op.mBounds))
        return op.mBounds;
    }
    return r;
  };

  auto send = [&](float value) {
    ISenderData<1> data {kTagControl, 1, 0};
    data.vals[0] = value;
    f.mDlg.SendControlMsgFromDelegate(kTagControl, ISender<>::kUpdateMessage, sizeof(data), &data);
  };

  // All at the default value: a flat line along the bottom
  const IRECT flat = traceBounds();
  REQUIRE(flat.H() == Approx(0.f));
  REQUIRE(flat.B > pDisplay->GetWidgetBounds().B - 6.f);

  send(1.f);
  REQUIRE(pDisplay->IsDirty());
  const IRECT spike = traceBounds();
  REQUIRE(spike.H() > 0.9f * pDisplay->GetWidgetBounds().H() - 8.f);

  // After a buffer's worth of zeros the spike has scrolled out
  for (int i = 0; i < 16; i++)
    send(0.f);
  REQUIRE(traceBounds().H() == Approx(0.f));

  // Half scale values
  for (int i = 0; i < 16; i++)
    send(0.5f);
  const IRECT half = traceBounds();
  REQUIRE(half.H() == Approx(0.f));
  REQUIRE(half.MH() == Approx(pDisplay->GetWidgetBounds().MH()).margin(2.f));
}

#pragma mark - IVMultiSliderControl

namespace
{
const IRECT kMultiSliderBounds = IRECT(100, 50, 300, 250);

float TrackX(const IRECT& widget, int track) { return widget.L + (static_cast<float>(track) + 0.5f) * widget.W() / 4.f; }
float YForValue(const IRECT& widget, double value) { return widget.B - static_cast<float>(value) * widget.H(); }
} // namespace

TEST_CASE("IVMultiSliderControl: each track is linked to its own parameter", "[integration][controls][multislider][params]")
{
  Fixture f;
  auto* pSliders = f.Attach(new IVMultiSliderControl<4>(kMultiSliderBounds, "", kPlainStyle, kStep0, 0, EDirection::Vertical));

  REQUIRE(pSliders->NVals() == 4);
  for (int i = 0; i < 4; i++)
    REQUIRE(pSliders->GetParamIdx(i) == kStep0 + i);

  f.mDlg.SendParameterValueFromDelegate(kStep2, 0.6, true);
  REQUIRE(pSliders->GetValue(2) == Approx(0.6));
  REQUIRE(pSliders->GetValue(1) == Approx(0.));
  REQUIRE(pSliders->IsDirty());

  const float trackHeight = pSliders->GetWidgetBounds().H();
  f.Redraw();
  REQUIRE(f.DrewFillOfHeight(kMultiSliderBounds, 0.6f * trackHeight));
}

TEST_CASE("IVMultiSliderControl: a click sets the track under the pointer", "[integration][controls][multislider][params][mouse]")
{
  Fixture f;
  auto* pSliders = f.Attach(new IVMultiSliderControl<4>(kMultiSliderBounds, "", kPlainStyle, kStep0, 0, EDirection::Vertical));
  const IRECT widget = pSliders->GetWidgetBounds();
  std::vector<std::pair<int, double>> newValues;
  pSliders->SetOnNewValueFunc([&](int trackIdx, double value) { newValues.push_back({trackIdx, value}); });

  f.mUI->Click(TrackX(widget, 2), YForValue(widget, 0.7));

  REQUIRE(pSliders->GetValue(2) == Approx(0.7).margin(0.002));
  REQUIRE(f.mDlg.GetParam(kStep2)->Value() == Approx(70.).margin(0.2));
  REQUIRE(f.mDlg.GetParam(kStep0)->Value() == Approx(0.));
  REQUIRE(f.mDlg.GetParam(kStep1)->Value() == Approx(0.));
  REQUIRE(f.mDlg.GetParam(kStep3)->Value() == Approx(0.));
  REQUIRE(pSliders->GetLastSliderHit() == 2);
  REQUIRE(newValues.size() == 1);
  REQUIRE(newValues[0].first == 2);
  REQUIRE(newValues[0].second == Approx(0.7).margin(0.002));

  // The gesture covers all four parameters
  for (int i = 0; i < 4; i++)
  {
    REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kStep0 + i) == 1);
    REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End, kStep0 + i) == 1);
  }
}

TEST_CASE("IVMultiSliderControl: dragging across the tracks draws a curve through them", "[integration][controls][multislider][params][mouse]")
{
  Fixture f;
  auto* pSliders = f.Attach(new IVMultiSliderControl<4>(kMultiSliderBounds, "", kPlainStyle, kStep0, 0, EDirection::Vertical));
  const IRECT widget = pSliders->GetWidgetBounds();

  f.mUI->MouseDown(TrackX(widget, 0), YForValue(widget, 0.2));
  f.mUI->MouseDragTo(TrackX(widget, 1), YForValue(widget, 0.4));
  f.mUI->MouseDragTo(TrackX(widget, 2), YForValue(widget, 0.6));
  f.mUI->MouseDragTo(TrackX(widget, 3), YForValue(widget, 0.8));
  f.mUI->MouseUp(TrackX(widget, 3), YForValue(widget, 0.8));

  for (int i = 0; i < 4; i++)
  {
    const double expected = 0.2 * (i + 1);
    REQUIRE(pSliders->GetValue(i) == Approx(expected).margin(0.002));
    REQUIRE(f.mDlg.GetParam(kStep0 + i)->Value() == Approx(expected * 100.).margin(0.2));
  }
}

TEST_CASE("IVMultiSliderControl: a fast drag that skips tracks fills in the ones in between", "[integration][controls][multislider][params][mouse]")
{
  Fixture f;
  auto* pSliders = f.Attach(new IVMultiSliderControl<4>(kMultiSliderBounds, "", kPlainStyle, kStep0, 0, EDirection::Vertical));
  const IRECT widget = pSliders->GetWidgetBounds();

  f.mUI->MouseDown(TrackX(widget, 0), YForValue(widget, 0.9));
  f.mUI->MouseDragTo(TrackX(widget, 3), YForValue(widget, 0.3));
  f.mUI->MouseUp(TrackX(widget, 3), YForValue(widget, 0.3));

  REQUIRE(pSliders->GetValue(0) == Approx(0.9).margin(0.002));
  REQUIRE(pSliders->GetValue(1) == Approx(0.7).margin(0.002));
  REQUIRE(pSliders->GetValue(2) == Approx(0.5).margin(0.002));
  REQUIRE(pSliders->GetValue(3) == Approx(0.3).margin(0.002));

  for (int i = 0; i < 4; i++)
    REQUIRE(f.mDlg.GetParam(kStep0 + i)->GetNormalized() == Approx(pSliders->GetValue(i)).margin(0.001));
}

TEST_CASE("IVMultiSliderControl: values are clamped when the pointer leaves the control", "[integration][controls][multislider][mouse]")
{
  Fixture f;
  auto* pSliders = f.Attach(new IVMultiSliderControl<4>(kMultiSliderBounds, "", kPlainStyle, kStep0, 0, EDirection::Vertical));
  const IRECT widget = pSliders->GetWidgetBounds();

  f.mUI->MouseDown(TrackX(widget, 1), widget.MH());
  f.mUI->MouseDragTo(TrackX(widget, 1), widget.T - 500.f);
  REQUIRE(pSliders->GetValue(1) == Approx(1.));
  f.mUI->MouseDragTo(TrackX(widget, 1), widget.B + 500.f);
  REQUIRE(pSliders->GetValue(1) == Approx(0.));
  f.mUI->MouseUp(TrackX(widget, 1), widget.B + 500.f);
}

TEST_CASE("IVMultiSliderControl: a message from the delegate highlights a track", "[integration][controls][multislider][delegate]")
{
  Fixture f;
  auto* pSliders = f.Attach(new IVMultiSliderControl<4>(kMultiSliderBounds, "", kPlainStyle, kStep0, 0, EDirection::Vertical));
  const int track = 2;

  REQUIRE_FALSE(pSliders->IsDirty());
  f.mDlg.SendControlMsgFromDelegate(kTagControl, IVMultiSliderControl<4>::kMsgTagSetHighlight, sizeof(int), &track);
  REQUIRE(pSliders->IsDirty());
  REQUIRE(f.mUI->RenderFrame());
}

TEST_CASE("IVMultiSliderControl: a stepped multi-slider snaps to its steps", "[integration][controls][multislider][mouse]")
{
  Fixture f;
  auto* pSliders = f.Attach(new IVMultiSliderControl<4>(kMultiSliderBounds, "", kPlainStyle, 4, EDirection::Vertical));
  const IRECT widget = pSliders->GetWidgetBounds();

  for (double target : {0.1, 0.3, 0.45, 0.62, 0.9})
  {
    f.mUI->Click(TrackX(widget, 1), YForValue(widget, target));
    const double steps = pSliders->GetValue(1) * 4.;
    REQUIRE(steps == Approx(std::round(steps)).margin(1e-6));
  }
}

#pragma mark - ILEDControl

TEST_CASE("ILEDControl: draws brighter for higher values", "[integration][controls][led][drawing]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 100, 140, 140);
  auto* pLED = f.Attach(new ILEDControl(bounds, COLOR_RED));

  auto ledColor = [&]() {
    f.Redraw();
    for (auto& op : f.mUI->mDrawOps)
    {
      if (op.mType == DrawOp::EType::Fill && bounds.Contains(op.mDeviceBounds) && op.mDeviceBounds.W() == Approx(20.f).margin(0.1))
        return op.mColor;
    }
    return IColor(0, 0, 0, 0);
  };

  const IColor off = ledColor();
  REQUIRE(off.A == 255);
  REQUIRE(off.R == 0);

  f.mDlg.SendControlValueFromDelegate(kTagControl, 1.);
  REQUIRE(pLED->IsDirty());
  const IColor on = ledColor();
  REQUIRE(on.R > 200);
  REQUIRE(on.R > off.R);

  f.mDlg.SendControlValueFromDelegate(kTagControl, 0.5);
  const IColor half = ledColor();
  REQUIRE(half.R > off.R);
  REQUIRE(half.R < on.R);
}

TEST_CASE("ILEDControl: a trigger lights it and it decays to off when the animation ends", "[integration][controls][led][animation]")
{
  Fixture f;
  auto* pLED = f.Attach(new ILEDControl(IRECT(100, 100, 140, 140), 0.3f));

  pLED->TriggerWithDecay(1000000);
  REQUIRE(pLED->GetAnimationFunction() != nullptr);
  REQUIRE(f.mUI->RenderFrame());
  // Barely any of the decay time has passed
  REQUIRE(pLED->GetValue() > 0.99);
  REQUIRE(f.mUI->RenderFrame());

  FinishAnimation(pLED);
  REQUIRE(pLED->GetAnimationFunction() == nullptr);
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE_FALSE(f.mUI->RenderFrame());
}

#pragma mark - IVSpectrumAnalyzerControl

TEST_CASE("IVSpectrumAnalyzerControl: accepts spectrum data and settings from the delegate and draws", "[integration][controls][spectrum][sender][drawing]")
{
  Fixture f;
  using TAnalyzer = IVSpectrumAnalyzerControl<2, 1024>;
  const IRECT bounds = IRECT(50, 50, 550, 350);
  auto* pAnalyzer = f.Attach(new TAnalyzer(bounds, "Spectrum"));

  f.Redraw();
  REQUIRE(f.mUI->mDrawOps.size() > 1);

  const double sampleRate = 48000.;
  const int fftSize = 512;
  f.mDlg.SendControlMsgFromDelegate(kTagControl, TAnalyzer::kMsgTagSampleRate, sizeof(double), &sampleRate);
  f.mDlg.SendControlMsgFromDelegate(kTagControl, TAnalyzer::kMsgTagFFTSize, sizeof(int), &fftSize);

  auto pData = std::make_unique<ISenderData<2, TAnalyzer::TDataPacket>>(kTagControl, 2, 0);
  for (auto& channel : pData->vals)
  {
    for (size_t i = 0; i < channel.size(); i++)
      channel[i] = (i == 32) ? 1.f : 0.001f;
  }

  f.mDlg.SendControlMsgFromDelegate(kTagControl, ISender<>::kUpdateMessage, static_cast<int>(sizeof(*pData)), pData.get());

  f.Redraw();
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Stroke) + f.mUI->CountOps(DrawOp::EType::Fill) > 1);
  REQUIRE_FALSE(pAnalyzer->IsHidden());
}
