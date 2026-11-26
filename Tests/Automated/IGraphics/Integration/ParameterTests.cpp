/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the link between controls and parameters: UI gestures to the host, delegate values to the UI, prompts

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
enum EParams { kGain = 0, kFreq, kSteps, kOnOff, kMode, kPan, kNumParams };
enum ECtrlTags { kTagKnob = 0, kTagSlider, kTagFreqKnob, kTagProbe, kTagPad, kTagUnlinked };

const IVStyle kPlainStyle = DEFAULT_STYLE.WithShowLabel(false).WithShowValue(false);
const IRECT kKnobBounds = IRECT(20, 20, 120, 120);
const IRECT kSliderBounds = IRECT(140, 20, 180, 220);
const IRECT kFreqKnobBounds = IRECT(200, 20, 300, 120);
const IRECT kProbeBounds = IRECT(320, 20, 380, 80);
const IRECT kPadBounds = IRECT(200, 140, 360, 300);

struct Fixture
{
  Fixture()
  : mDlg(kNumParams, 400, 320)
  {
    mDlg.GetParam(kGain)->InitDouble("Gain", 50., 0., 100., 0.01, "%");
    mDlg.GetParam(kFreq)->InitDouble("Freq", 1000., 20., 20000., 0.1, "Hz", 0, "", IParam::ShapePowCurve(3.));
    mDlg.GetParam(kSteps)->InitInt("Steps", 2, 0, 10, "st");
    mDlg.GetParam(kOnOff)->InitBool("OnOff", false);
    mDlg.GetParam(kMode)->InitEnum("Mode", 1, {"Sine", "Saw", "Square", "Noise"});
    mDlg.GetParam(kPan)->InitDouble("Pan", 0., -100., 100., 1., "%");

    mUI = mDlg.OpenUI();
    mUI->AttachPanelBackground(COLOR_GRAY);
    mKnob = mUI->AttachControl(new IVKnobControl(kKnobBounds, kGain, "", kPlainStyle), kTagKnob);
    mSlider = mUI->AttachControl(new IVSliderControl(kSliderBounds, kGain, "", kPlainStyle), kTagSlider);
    mFreqKnob = mUI->AttachControl(new IVKnobControl(kFreqKnobBounds, kFreq, "", kPlainStyle), kTagFreqKnob);
    mProbe = mUI->AttachControl(new ProbeControl(kProbeBounds, kSteps), kTagProbe)->As<ProbeControl>();
    mPad = mUI->AttachControl(new IVXYPadControl(kPadBounds, {kPan, kGain}, "", kPlainStyle), kTagPad);
    mUnlinked = mUI->AttachControl(new ProbeControl(IRECT(20, 240, 60, 280)), kTagUnlinked)->As<ProbeControl>();
    Settle();
  }

  /** Push the parameter values to the UI, draw, and forget everything recorded so far */
  void Settle()
  {
    mDlg.SendCurrentParamValuesFromDelegate();
    mUI->RenderUntilClean();
    mUI->ClearRecorded();
    mDlg.ClearRecorded();
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
  IControl* mKnob = nullptr;
  IControl* mSlider = nullptr;
  IControl* mFreqKnob = nullptr;
  ProbeControl* mProbe = nullptr;
  IControl* mPad = nullptr;
  ProbeControl* mUnlinked = nullptr;
};
} // namespace

#pragma mark - UI to host

TEST_CASE("Parameter gesture: a knob drag sends begin, values and end to the host, in that order", "[integration][params][gesture]")
{
  Fixture f;
  const float x = kKnobBounds.MW(), y = kKnobBounds.MH();

  f.mUI->MouseDown(x, y);
  REQUIRE(f.mDlg.mParamEvents.size() == 1);
  REQUIRE(f.mDlg.mParamEvents[0].mType == ParamEvent::EType::Begin);
  REQUIRE(f.mDlg.mParamEvents[0].mIdx == kGain);

  f.mUI->MouseDragTo(x, y - 10.f);
  f.mUI->MouseDragTo(x, y - 20.f);
  f.mUI->MouseDragTo(x, y - 30.f);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kGain) == 3);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End) == 0);

  f.mUI->MouseUp(x, y - 30.f);

  auto& events = f.mDlg.mParamEvents;
  REQUIRE(events.front().mType == ParamEvent::EType::Begin);
  REQUIRE(events.back().mType == ParamEvent::EType::End);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin) == 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End) == 1);

  double previous = 0.5;

  for (size_t i = 1; i + 1 < events.size(); i++)
  {
    REQUIRE(events[i].mType == ParamEvent::EType::Value);
    REQUIRE(events[i].mIdx == kGain);
    REQUIRE(events[i].mNormalizedValue >= previous);
    REQUIRE(events[i].mNormalizedValue <= 1.);
    previous = events[i].mNormalizedValue;
  }

  REQUIRE(previous > 0.5);
  REQUIRE(f.mKnob->GetValue() == Approx(previous));
  REQUIRE(f.mDlg.GetParam(kGain)->GetNormalized() == Approx(previous));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(previous * 100.));
  REQUIRE(f.mDlg.mParamChangesUI.size() >= 3);
}

TEST_CASE("Parameter gesture: a click with no movement still brackets the gesture with begin and end", "[integration][params][gesture]")
{
  Fixture f;

  f.mUI->Click(kKnobBounds.MW(), kKnobBounds.MH());

  REQUIRE(f.mDlg.mParamEvents.front().mType == ParamEvent::EType::Begin);
  REQUIRE(f.mDlg.mParamEvents.back().mType == ParamEvent::EType::End);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kGain) == 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End, kGain) == 1);
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(50.));
}

TEST_CASE("Parameter gesture: a control with no parameter sends nothing to the host", "[integration][params][gesture]")
{
  Fixture f;

  f.mUI->Drag(40, 260, 50, 270);

  REQUIRE(f.mUnlinked->Count("drag") == 4);
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("Parameter gesture: the normalized control value is mapped through the parameter's shape", "[integration][params][gesture]")
{
  Fixture f;
  const float x = kFreqKnobBounds.MW(), y = kFreqKnobBounds.MH();
  const IParam* pFreq = f.mDlg.GetParam(kFreq);

  REQUIRE(f.mFreqKnob->GetValue() == Approx(pFreq->ToNormalized(1000.)));

  f.mUI->Drag(x, y, x, y - 25.f);

  const double normalized = f.mFreqKnob->GetValue();
  REQUIRE(normalized > pFreq->ToNormalized(1000.));
  REQUIRE(f.mDlg.LastParamValue(kFreq) == Approx(normalized));
  REQUIRE(pFreq->GetNormalized() == Approx(normalized));
  REQUIRE(pFreq->Value() == Approx(pFreq->FromNormalized(normalized)));
  // A power curve: half way round the knob is well below the middle of the range
  REQUIRE(pFreq->FromNormalized(0.5) < 0.5 * (20. + 20000.));

  f.mUI->Drag(x, y, x, y - 5000.f);
  REQUIRE(pFreq->Value() == Approx(20000.));
  f.mUI->Drag(x, y, x, y + 5000.f);
  REQUIRE(pFreq->Value() == Approx(20.));
}

TEST_CASE("Parameter gesture: other controls linked to the same parameter follow, without telling the host again", "[integration][params][gesture]")
{
  Fixture f;
  const float x = kKnobBounds.MW(), y = kKnobBounds.MH();

  f.mUI->MouseDown(x, y);
  f.mUI->MouseDragTo(x, y - 20.f);

  const double v = f.mKnob->GetValue();
  REQUIRE(v > 0.5);
  REQUIRE(f.mSlider->GetValue() == Approx(v));
  REQUIRE(f.mPad->GetValue(1) == Approx(v));
  REQUIRE(f.mSlider->IsDirty());
  REQUIRE(f.mPad->IsDirty());
  // The pad's other value, and controls for other parameters, are unaffected
  REQUIRE(f.mPad->GetValue(0) == Approx(0.5));
  REQUIRE_FALSE(f.mFreqKnob->IsDirty());
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value) == 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin) == 1);

  f.mUI->MouseUp(x, y - 20.f);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End) == 1);
}

#pragma mark - Host to UI

TEST_CASE("SendParameterValueFromDelegate: updates every control linked to the parameter", "[integration][params][delegate]")
{
  Fixture f;

  SECTION("normalized")
  {
    f.mDlg.SendParameterValueFromDelegate(kGain, 0.25, true);
  }

  SECTION("not normalized")
  {
    f.mDlg.SendParameterValueFromDelegate(kGain, 25., false);
  }

  REQUIRE(f.mKnob->GetValue() == Approx(0.25));
  REQUIRE(f.mSlider->GetValue() == Approx(0.25));
  REQUIRE(f.mPad->GetValue(1) == Approx(0.25));
  REQUIRE(f.mPad->GetValue(0) == Approx(0.5));
  REQUIRE(f.mKnob->IsDirty());
  REQUIRE(f.mSlider->IsDirty());
  REQUIRE(f.mPad->IsDirty());
  REQUIRE_FALSE(f.mFreqKnob->IsDirty());
  REQUIRE_FALSE(f.mProbe->IsDirty());
  REQUIRE(f.mFreqKnob->GetValue() == Approx(f.mDlg.GetParam(kFreq)->ToNormalized(1000.)));

  // It is not echoed back to the host
  REQUIRE(f.mDlg.mParamEvents.empty());
  // ... but the delegate's own hook is called
  REQUIRE(f.mDlg.mParamChangesUI == std::vector<int>{kGain});

  // The change is drawn on the next frame, covering the controls that changed
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->mLastDirtyBounds.Contains(kKnobBounds));
  REQUIRE(f.mUI->mLastDirtyBounds.Contains(kSliderBounds));
  REQUIRE_FALSE(f.mUI->RenderFrame());
}

TEST_CASE("SendParameterValueFromDelegate: a non-normalized value is converted with the parameter's shape", "[integration][params][delegate]")
{
  Fixture f;
  const IParam* pFreq = f.mDlg.GetParam(kFreq);

  f.mDlg.SendParameterValueFromDelegate(kFreq, 5000., false);
  REQUIRE(f.mFreqKnob->GetValue() == Approx(pFreq->ToNormalized(5000.)));
  REQUIRE(f.mFreqKnob->GetValue() > 0.5);

  f.mDlg.SendParameterValueFromDelegate(kSteps, 7., false);
  REQUIRE(f.mProbe->GetValue() == Approx(0.7));

  f.mDlg.SendParameterValueFromDelegate(kPan, -50., false);
  REQUIRE(f.mPad->GetValue(0) == Approx(0.25));
}

TEST_CASE("SendParameterValueFromDelegate: sending the value a control already has does not redraw it", "[integration][params][delegate][drawing]")
{
  Fixture f;

  f.mDlg.SendParameterValueFromDelegate(kGain, 0.5, true);

  REQUIRE_FALSE(f.mKnob->IsDirty());
  REQUIRE_FALSE(f.mSlider->IsDirty());
  REQUIRE_FALSE(f.mUI->RenderFrame());
}

TEST_CASE("SendParameterValueFromDelegate: a control the user is dragging is not moved by the host", "[integration][params][delegate][mouse]")
{
  Fixture f;
  const float x = kKnobBounds.MW(), y = kKnobBounds.MH();

  f.mUI->MouseDown(x, y);
  f.mUI->MouseDragTo(x, y - 20.f);
  const double dragged = f.mKnob->GetValue();

  f.mDlg.SendParameterValueFromDelegate(kGain, 0.1, true);
  REQUIRE(f.mKnob->GetValue() == Approx(dragged));
  // Controls that are not being touched do follow the host
  REQUIRE(f.mSlider->GetValue() == Approx(0.1));

  f.mUI->MouseUp(x, y - 20.f);

  // Once released, the knob follows the host again
  f.mDlg.SendParameterValueFromDelegate(kGain, 0.2, true);
  REQUIRE(f.mKnob->GetValue() == Approx(0.2));
}

TEST_CASE("SendControlValueFromDelegate: sets the value of the control with the tag, without telling the host", "[integration][params][delegate][tags]")
{
  Fixture f;

  f.mDlg.SendControlValueFromDelegate(kTagUnlinked, 0.8);
  REQUIRE(f.mUnlinked->GetValue() == Approx(0.8));
  REQUIRE(f.mUnlinked->IsDirty());

  f.mDlg.SendControlValueFromDelegate(kTagKnob, 0.3);
  REQUIRE(f.mKnob->GetValue() == Approx(0.3));
  // Only that control: it is a control value, not a parameter value
  REQUIRE(f.mSlider->GetValue() == Approx(0.5));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(50.));
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("SendControlMsgFromDelegate: delivers the message and its data to the control with the tag", "[integration][params][delegate][tags]")
{
  Fixture f;
  const uint8_t data[4] = {1, 2, 3, 250};

  f.mDlg.SendControlMsgFromDelegate(kTagUnlinked, 42, sizeof(data), data);

  REQUIRE(f.mUnlinked->mNumMsgs == 1);
  REQUIRE(f.mUnlinked->mLastMsgTag == 42);
  REQUIRE(f.mUnlinked->mLastMsgData == std::vector<uint8_t>{1, 2, 3, 250});
  REQUIRE(f.mProbe->mNumMsgs == 0);

  // A message with no data
  f.mDlg.SendControlMsgFromDelegate(kTagProbe, 7);
  REQUIRE(f.mProbe->mNumMsgs == 1);
  REQUIRE(f.mProbe->mLastMsgTag == 7);
  REQUIRE(f.mProbe->mLastMsgData.empty());
}

TEST_CASE("SendMidiMsgFromDelegate: reaches the controls that want MIDI", "[integration][params][delegate][midi]")
{
  Fixture f;
  f.mUnlinked->SetWantsMidi(true);
  REQUIRE(f.mUnlinked->GetWantsMidi());
  REQUIRE_FALSE(f.mProbe->GetWantsMidi());

  IMidiMsg msg;
  msg.MakeNoteOnMsg(64, 99, 0);
  f.mDlg.SendMidiMsgFromDelegate(msg);

  REQUIRE(f.mUnlinked->mMidi.size() == 1);
  REQUIRE(f.mUnlinked->mMidi[0].NoteNumber() == 64);
  REQUIRE(f.mUnlinked->mMidi[0].Velocity() == 99);
  REQUIRE(f.mProbe->mMidi.empty());
}

#pragma mark - Control value API

TEST_CASE("SetDirty: with triggerAction the host is told the value and the action function runs, without it neither happens", "[integration][params][dirty]")
{
  Fixture f;
  int nActions = 0;
  IControl* pActionCaller = nullptr;
  f.mProbe->SetActionFunction([&](IControl* pCaller) { nActions++; pActionCaller = pCaller; });
  REQUIRE(f.mProbe->GetActionFunction() != nullptr);

  f.mProbe->SetValue(0.4);
  f.mProbe->SetDirty(false);
  REQUIRE(f.mProbe->IsDirty());
  REQUIRE(nActions == 0);
  REQUIRE(f.mDlg.mParamEvents.empty());
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 2);

  f.mUI->RenderUntilClean();
  REQUIRE_FALSE(f.mProbe->IsDirty());

  f.mProbe->SetDirty(true);
  REQUIRE(f.mProbe->IsDirty());
  REQUIRE(nActions == 1);
  REQUIRE(pActionCaller == f.mProbe);
  REQUIRE(f.mDlg.mParamEvents.size() == 1);
  REQUIRE(f.mDlg.mParamEvents[0].mType == ParamEvent::EType::Value);
  REQUIRE(f.mDlg.mParamEvents[0].mIdx == kSteps);
  REQUIRE(f.mDlg.mParamEvents[0].mNormalizedValue == Approx(0.4));
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 4);
}

TEST_CASE("SetDirty: clamps the control value to the normalized range", "[integration][params][dirty]")
{
  Fixture f;

  f.mUnlinked->SetValue(1.7);
  f.mUnlinked->SetDirty(false);
  REQUIRE(f.mUnlinked->GetValue() == Approx(1.));

  f.mUnlinked->SetValue(-0.3);
  f.mUnlinked->SetDirty(false);
  REQUIRE(f.mUnlinked->GetValue() == Approx(0.));
}

TEST_CASE("Action function: runs for a control without a parameter when the user changes it", "[integration][params][action]")
{
  Fixture f;
  int nActions = 0;
  auto* pButton = f.mUI->AttachControl(new IVButtonControl(IRECT(80, 240, 180, 280), [&](IControl* pCaller) { nActions++; }, "Go", kPlainStyle));

  f.mUI->Click(130, 260);
  REQUIRE(nActions == 1);
  REQUIRE(pButton->GetValue() == Approx(1.));
  REQUIRE(f.mDlg.mParamEvents.empty());

  f.mDlg.SendControlValueFromDelegate(kTagUnlinked, 0.5);
  REQUIRE(nActions == 1);
}

TEST_CASE("Animation end action function: runs when the control's animation ends", "[integration][params][action][animation]")
{
  Fixture f;
  int nEndActions = 0;
  f.mUnlinked->SetAnimationEndActionFunction([&](IControl* pCaller) { nEndActions++; });

  f.mUnlinked->SetAnimation([](IControl*) {}, 1000000);
  REQUIRE(f.mUnlinked->GetAnimationFunction() != nullptr);
  REQUIRE(nEndActions == 0);

  f.mUnlinked->OnEndAnimation();
  REQUIRE(nEndActions == 1);
  REQUIRE(f.mUnlinked->GetAnimationFunction() == nullptr);
  REQUIRE(f.mUnlinked->GetAnimationProgress() == Approx(0.));
}

TEST_CASE("SetValueFromUserInput: changes the value and tells the host, unless the value is unchanged", "[integration][params][userinput]")
{
  Fixture f;

  f.mKnob->SetValueFromUserInput(0.8);
  REQUIRE(f.mKnob->GetValue() == Approx(0.8));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(80.));
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kGain) == 1);
  REQUIRE(f.mDlg.LastParamValue(kGain) == Approx(0.8));
  REQUIRE(f.mSlider->GetValue() == Approx(0.8));
  REQUIRE(f.mKnob->IsDirty());

  f.mKnob->SetValueFromUserInput(0.8);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kGain) == 1);
}

TEST_CASE("SetValueFromDelegate: changes the value without telling the host", "[integration][params][delegate]")
{
  Fixture f;

  f.mKnob->SetValueFromDelegate(0.8);
  REQUIRE(f.mKnob->GetValue() == Approx(0.8));
  REQUIRE(f.mKnob->IsDirty());
  REQUIRE(f.mDlg.mParamEvents.empty());
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(50.));
}

TEST_CASE("SetValueToDefault: resets the control to the parameter's default and tells the host", "[integration][params][default]")
{
  Fixture f;
  f.mDlg.GetParam(kGain)->Set(10.);
  f.mDlg.GetParam(kFreq)->Set(200.);
  f.mDlg.GetParam(kPan)->Set(80.);
  f.Settle();

  SECTION("a single value control")
  {
    f.mFreqKnob->SetValueToDefault();
    REQUIRE(f.mDlg.GetParam(kFreq)->Value() == Approx(1000.));
    REQUIRE(f.mFreqKnob->GetValue() == Approx(f.mDlg.GetParam(kFreq)->ToNormalized(1000.)));
    REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kFreq) == 1);
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(10.));
  }

  SECTION("every value of a multi-value control")
  {
    f.mPad->SetValueToDefault();
    REQUIRE(f.mDlg.GetParam(kPan)->Value() == Approx(0.));
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(50.));
    REQUIRE(f.mPad->GetValue(0) == Approx(0.5));
    REQUIRE(f.mPad->GetValue(1) == Approx(0.5));
    REQUIRE(f.mKnob->GetValue() == Approx(0.5));
  }

  SECTION("one value of a multi-value control")
  {
    f.mPad->SetValueToDefault(0);
    REQUIRE(f.mDlg.GetParam(kPan)->Value() == Approx(0.));
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(10.));
    REQUIRE(f.mPad->GetValue(0) == Approx(0.5));
    REQUIRE(f.mPad->GetValue(1) == Approx(0.1));
  }
}

TEST_CASE("SetParamIdx: relinks a control to a different parameter", "[integration][params]")
{
  Fixture f;

  REQUIRE(f.mProbe->GetParamIdx() == kSteps);
  REQUIRE(f.mProbe->GetParam() == f.mDlg.GetParam(kSteps));
  REQUIRE(f.mUnlinked->GetParamIdx() == kNoParameter);
  REQUIRE(f.mUnlinked->GetParam() == nullptr);

  f.mUnlinked->SetParamIdx(kOnOff);
  REQUIRE(f.mUnlinked->GetParamIdx() == kOnOff);
  REQUIRE(f.mUnlinked->GetParam() == f.mDlg.GetParam(kOnOff));
  REQUIRE(f.mUnlinked->IsDirty());

  f.mDlg.SendParameterValueFromDelegate(kOnOff, 1., true);
  REQUIRE(f.mUnlinked->GetValue() == Approx(1.));
  REQUIRE(f.mUI->GetControlWithParamIdx(kOnOff) == f.mUnlinked);
}

#pragma mark - Multi-parameter controls

TEST_CASE("Multi-parameter control: each value index is linked to its own parameter", "[integration][params][multiparam][xypad]")
{
  Fixture f;

  REQUIRE(f.mPad->NVals() == 2);
  REQUIRE(f.mPad->GetParamIdx(0) == kPan);
  REQUIRE(f.mPad->GetParamIdx(1) == kGain);
  REQUIRE(f.mPad->GetParam(0) == f.mDlg.GetParam(kPan));
  REQUIRE(f.mPad->GetParam(1) == f.mDlg.GetParam(kGain));

  f.mPad->SetValue(0.2, 0);
  f.mPad->SetValue(0.9, 1);
  REQUIRE(f.mPad->GetValue(0) == Approx(0.2));
  REQUIRE(f.mPad->GetValue(1) == Approx(0.9));

  // Telling the host about one value only
  f.mPad->SetDirty(true, 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kGain) == 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kPan) == 0);
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(90.));
  REQUIRE(f.mDlg.GetParam(kPan)->Value() == Approx(0.));

  // ... and about all of them
  f.mPad->SetDirty(true);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kPan) == 1);
  REQUIRE(f.mDlg.GetParam(kPan)->Value() == Approx(-60.));
}

TEST_CASE("Multi-parameter control: a gesture on an XY pad begins, changes and ends both parameters", "[integration][params][multiparam][xypad][gesture]")
{
  Fixture f;

  // Three quarters across and a quarter of the way down
  const float x = kPadBounds.L + 0.75f * kPadBounds.W();
  const float y = kPadBounds.T + 0.25f * kPadBounds.H();

  f.mUI->MouseDown(kPadBounds.MW(), kPadBounds.MH());
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kPan) == 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kGain) == 1);

  f.mUI->MouseDragTo(x, y);
  REQUIRE(f.mPad->GetValue(0) == Approx(0.75));
  REQUIRE(f.mPad->GetValue(1) == Approx(0.75));
  REQUIRE(f.mDlg.GetParam(kPan)->Value() == Approx(50.));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(75.));
  REQUIRE(f.mDlg.LastParamValue(kPan) == Approx(0.75));
  REQUIRE(f.mDlg.LastParamValue(kGain) == Approx(0.75));
  // The knob and the slider for the gain follow the pad
  REQUIRE(f.mKnob->GetValue() == Approx(0.75));
  REQUIRE(f.mSlider->GetValue() == Approx(0.75));

  f.mUI->MouseUp(x, y);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End, kPan) == 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End, kGain) == 1);
  REQUIRE(f.mDlg.mParamEvents.front().mType == ParamEvent::EType::Begin);
  REQUIRE(f.mDlg.mParamEvents.back().mType == ParamEvent::EType::End);
}

TEST_CASE("Multi-parameter control: a parameter value from the host goes to the right value index", "[integration][params][multiparam][delegate]")
{
  Fixture f;
  auto* pTwo = f.mUI->AttachControl(new ProbeControl(IRECT(80, 240, 120, 280), {kFreq, kSteps}));
  f.Settle();

  f.mDlg.SendParameterValueFromDelegate(kSteps, 0.9, true);
  REQUIRE(pTwo->GetValue(1) == Approx(0.9));
  REQUIRE(pTwo->GetValue(0) == Approx(f.mDlg.GetParam(kFreq)->ToNormalized(1000.)));
  REQUIRE(f.mProbe->GetValue() == Approx(0.9));

  f.mDlg.SendParameterValueFromDelegate(kFreq, 0.1, true);
  REQUIRE(pTwo->GetValue(0) == Approx(0.1));
  REQUIRE(pTwo->GetValue(1) == Approx(0.9));
}

#pragma mark - Prompting for a value

TEST_CASE("PromptUserInput: prompting is off unless the control enables it", "[integration][params][prompt]")
{
  Fixture f;

  f.mProbe->PromptUserInput();
  REQUIRE(f.mUI->mNumTextEntries == 0);
  REQUIRE(f.mUI->GetControlInTextEntry() == nullptr);

  f.mProbe->DisablePrompt(false);
  f.mProbe->PromptUserInput();
  REQUIRE(f.mUI->mNumTextEntries == 1);
  REQUIRE(f.mUI->GetControlInTextEntry() == f.mProbe);

  // A control with no parameter has nothing to prompt for
  f.mUnlinked->DisablePrompt(false);
  f.mUnlinked->PromptUserInput();
  REQUIRE(f.mUI->mNumTextEntries == 1);
}

TEST_CASE("PromptUserInput: a numeric parameter opens a text entry showing the current value", "[integration][params][prompt][textentry]")
{
  Fixture f;
  f.mKnob->DisablePrompt(false);
  REQUIRE_FALSE(f.mUI->IsInPlatformTextEntry());

  SECTION("without the label")
  {
    f.mKnob->PromptUserInput();
    REQUIRE(f.mUI->mTextEntryStr == "50.00");
  }

  SECTION("with the label")
  {
    f.mKnob->SetPromptShowsParamLabel(true);
    REQUIRE(f.mKnob->GetPromptShowsParamLabel());
    f.mKnob->PromptUserInput();
    REQUIRE(f.mUI->mTextEntryStr == "50.00 %");
  }

  REQUIRE(f.mUI->mNumTextEntries == 1);
  REQUIRE(f.mUI->mNumPopupMenus == 0);
  REQUIRE(f.mUI->IsInPlatformTextEntry());
  REQUIRE(f.mUI->GetControlInTextEntry() == f.mKnob);
  // By default the entry box is centred on the control
  REQUIRE(f.mUI->mTextEntryBounds.MW() == Approx(kKnobBounds.MW()));
  REQUIRE(f.mUI->mTextEntryBounds.MH() == Approx(kKnobBounds.MH()));
  REQUIRE(kKnobBounds.Contains(f.mUI->mTextEntryBounds));
}

TEST_CASE("PromptUserInput: the text entry can be given explicit bounds", "[integration][params][prompt][textentry]")
{
  Fixture f;
  f.mKnob->DisablePrompt(false);

  f.mKnob->PromptUserInput(IRECT(1, 2, 31, 22));
  REQUIRE(f.mUI->mTextEntryBounds == IRECT(1, 2, 31, 22));
  REQUIRE(f.mUI->mTextEntryStr == "50.00");
}

TEST_CASE("SetControlValueAfterTextEdit: the text is parsed by the parameter and the host is told", "[integration][params][prompt][textentry]")
{
  Fixture f;
  f.mKnob->DisablePrompt(false);
  f.mKnob->PromptUserInput();

  SECTION("a plain number")
  {
    f.mUI->SetControlValueAfterTextEdit("75");
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(75.));
    REQUIRE(f.mKnob->GetValue() == Approx(0.75));
    REQUIRE(f.mSlider->GetValue() == Approx(0.75));
    REQUIRE(f.mDlg.LastParamValue(kGain) == Approx(0.75));
  }

  SECTION("a number with units")
  {
    f.mUI->SetControlValueAfterTextEdit("12.5 %");
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(12.5));
    REQUIRE(f.mKnob->GetValue() == Approx(0.125));
  }

  SECTION("a number above the range is clamped")
  {
    f.mUI->SetControlValueAfterTextEdit("500");
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(100.));
    REQUIRE(f.mKnob->GetValue() == Approx(1.));
  }

  SECTION("a number below the range is clamped")
  {
    f.mUI->SetControlValueAfterTextEdit("-3");
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(0.));
    REQUIRE(f.mKnob->GetValue() == Approx(0.));
  }

  SECTION("the same value as before changes nothing")
  {
    f.mUI->SetControlValueAfterTextEdit("50");
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(50.));
    REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value) == 0);
  }

  REQUIRE_FALSE(f.mUI->IsInPlatformTextEntry());
  REQUIRE(f.mUI->GetControlInTextEntry() == nullptr);

  // With no text entry in progress this is ignored
  const double before = f.mDlg.GetParam(kGain)->Value();
  f.mUI->SetControlValueAfterTextEdit("33");
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(before));
}

TEST_CASE("SetControlValueAfterTextEdit: values are converted with the parameter's shape and stepping", "[integration][params][prompt][textentry]")
{
  Fixture f;

  SECTION("a shaped parameter")
  {
    f.mFreqKnob->DisablePrompt(false);
    f.mFreqKnob->PromptUserInput();
    REQUIRE(f.mUI->mTextEntryStr == "1000.0");
    f.mUI->SetControlValueAfterTextEdit("4400 Hz");
    REQUIRE(f.mDlg.GetParam(kFreq)->Value() == Approx(4400.));
    REQUIRE(f.mFreqKnob->GetValue() == Approx(f.mDlg.GetParam(kFreq)->ToNormalized(4400.)));
  }

  SECTION("an integer parameter")
  {
    f.mProbe->DisablePrompt(false);
    f.mProbe->PromptUserInput();
    REQUIRE(f.mUI->mTextEntryStr == "2");
    f.mUI->SetControlValueAfterTextEdit("6.7");
    REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 7);
    REQUIRE(f.mProbe->GetValue() == Approx(0.7));
  }
}

TEST_CASE("Text entry: clicking the value of a knob with an editable value opens a text entry", "[integration][params][prompt][textentry][knob]")
{
  Fixture f;
  auto* pKnob = f.mUI->AttachControl(new IVKnobControl(IRECT(20, 140, 120, 300), kSteps, "Steps", DEFAULT_STYLE, true))->As<IVKnobControl>();
  f.Settle();
  const IRECT valueBounds = pKnob->GetValueBounds();
  REQUIRE(valueBounds.W() > 0.f);
  REQUIRE(valueBounds.H() > 0.f);

  f.mUI->Click(valueBounds.MW(), valueBounds.MH());

  REQUIRE(f.mUI->mNumTextEntries == 1);
  REQUIRE(f.mUI->mTextEntryStr == "2");
  REQUIRE(f.mUI->mTextEntryBounds == valueBounds);
  REQUIRE(f.mUI->GetControlInTextEntry() == pKnob);
  REQUIRE_FALSE(f.mUI->mCursorHidden);

  f.mUI->SetControlValueAfterTextEdit("9");
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 9);
  REQUIRE(pKnob->GetValue() == Approx(0.9));
  REQUIRE(std::string(pKnob->GetValueStr()) == "9 st");
}

TEST_CASE("Text entry: a knob whose value is not editable does not open one", "[integration][params][prompt][textentry][knob]")
{
  Fixture f;
  auto* pKnob = f.mUI->AttachControl(new IVKnobControl(IRECT(20, 140, 120, 300), kSteps, "Steps", DEFAULT_STYLE, false))->As<IVKnobControl>();
  f.Settle();
  const IRECT valueBounds = pKnob->GetValueBounds();

  f.mUI->Click(valueBounds.MW(), valueBounds.MH());
  REQUIRE(f.mUI->mNumTextEntries == 0);
}

TEST_CASE("PromptUserInput: an enum parameter opens a pop-up menu of its options", "[integration][params][prompt][popupmenu]")
{
  Fixture f;
  auto* pProbe = f.mUI->AttachControl(new ProbeControl(IRECT(80, 240, 180, 280), kMode))->As<ProbeControl>();
  pProbe->DisablePrompt(false);
  f.Settle();

  SECTION("an item is chosen")
  {
    f.mUI->mPopupMenuChoice = 3;
    pProbe->PromptUserInput();

    REQUIRE(f.mUI->mNumPopupMenus == 1);
    REQUIRE(f.mUI->mNumTextEntries == 0);
    REQUIRE(f.mUI->mPopupMenuItems == std::vector<std::string>{"Sine", "Saw", "Square", "Noise"});
    REQUIRE(f.mUI->mPopupMenuBounds == pProbe->GetRECT());
    REQUIRE(pProbe->mNumPopupSelections == 1);
    REQUIRE(pProbe->mPopupSelection == 3);
    REQUIRE(pProbe->mPopupSelectionValIdx == 0);
    REQUIRE(f.mDlg.GetParam(kMode)->Int() == 3);
    REQUIRE(pProbe->GetValue() == Approx(1.));
    REQUIRE(f.mDlg.LastParamValue(kMode) == Approx(1.));
  }

  SECTION("the menu is dismissed")
  {
    f.mUI->mPopupMenuChoice = -1;
    pProbe->PromptUserInput();

    REQUIRE(f.mUI->mNumPopupMenus == 1);
    REQUIRE(pProbe->mNumPopupSelections == 1);
    REQUIRE(pProbe->mPopupSelection == -1);
    REQUIRE(f.mDlg.GetParam(kMode)->Int() == 1);
    REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value) == 0);
  }

  // The menu that was shown has the current value checked
  IPopupMenu& menu = f.mUI->GetPromptMenu();
  REQUIRE(menu.NItems() == 4);
  REQUIRE(std::string(menu.GetRootTitle()) == "Mode");
  REQUIRE_FALSE(menu.GetItem(0)->GetChecked());
  REQUIRE(menu.GetItem(1)->GetChecked());
  REQUIRE_FALSE(menu.GetItem(2)->GetChecked());
}

TEST_CASE("PromptUserInput: a bool parameter opens a pop-up menu of its two states", "[integration][params][prompt][popupmenu]")
{
  Fixture f;
  auto* pProbe = f.mUI->AttachControl(new ProbeControl(IRECT(80, 240, 180, 280), kOnOff))->As<ProbeControl>();
  pProbe->DisablePrompt(false);
  f.Settle();

  f.mUI->mPopupMenuChoice = 1;
  pProbe->PromptUserInput();

  REQUIRE(f.mUI->mPopupMenuItems == std::vector<std::string>{"off", "on"});
  REQUIRE(f.mDlg.GetParam(kOnOff)->Bool());
  REQUIRE(pProbe->GetValue() == Approx(1.));
}

TEST_CASE("SetControlValueAfterPopupMenu: is ignored when no pop-up menu is in progress", "[integration][params][prompt][popupmenu]")
{
  Fixture f;
  IPopupMenu menu;
  menu.AddItem("One");
  menu.SetChosenItemIdx(0);

  f.mUI->SetControlValueAfterPopupMenu(&menu);
  f.mUI->SetControlValueAfterPopupMenu(nullptr);

  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("PromptUserInput: a pop-up menu for a value other than the first uses that value's parameter", "[integration][params][prompt][popupmenu]")
{
  Fixture f;
  auto* pProbe = f.mUI->AttachControl(new ProbeControl(IRECT(80, 240, 180, 280), {kGain, kMode}))->As<ProbeControl>();
  pProbe->DisablePrompt(false);
  f.Settle();

  f.mUI->mPopupMenuChoice = 3;
  pProbe->PromptUserInput(1);

  REQUIRE(f.mUI->mPopupMenuItems.size() == 4);
  REQUIRE(pProbe->mPopupSelectionValIdx == 1);
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 3);
  REQUIRE(pProbe->GetValue(1) == Approx(1.));
}
