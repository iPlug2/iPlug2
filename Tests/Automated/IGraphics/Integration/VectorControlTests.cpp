/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the IV (vector) control library: buttons, switches, knobs, sliders, pads, number boxes and containers

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestEditorDelegate.h"
#include "TestControls.h"
#include "IControls.h"
#include "IBubbleControl.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;
using ParamEvent = TestEditorDelegate::ParamEvent;
using DrawOp = IGraphicsHeadless::DrawOp;

namespace
{
enum EParams { kGain = 0, kSteps, kOnOff, kMode, kLow, kHigh, kNumParams };

const IVStyle kPlainStyle = DEFAULT_STYLE.WithShowLabel(false).WithShowValue(false);
const IRECT kBounds = IRECT(100, 50, 200, 150);

struct Fixture
{
  Fixture()
  : mDlg(kNumParams, 600, 400)
  {
    mDlg.GetParam(kGain)->InitDouble("Gain", 50., 0., 100., 0.01, "%");
    mDlg.GetParam(kSteps)->InitInt("Steps", 2, 0, 10, "st");
    mDlg.GetParam(kOnOff)->InitBool("OnOff", false);
    mDlg.GetParam(kMode)->InitEnum("Mode", 0, {"Sine", "Saw", "Square"});
    mDlg.GetParam(kLow)->InitDouble("Low", 25., 0., 100., 0.1, "%");
    mDlg.GetParam(kHigh)->InitDouble("High", 75., 0., 100., 0.1, "%");
    mUI = mDlg.OpenUI();
    mUI->AttachPanelBackground(COLOR_GRAY);
  }

  /** Attach a control, give it the current parameter values, draw it and forget what was recorded */
  template <class T>
  T* Attach(T* pControl, int ctrlTag = kNoTag, const char* group = "")
  {
    mUI->AttachControl(pControl, ctrlTag, group);
    Settle();
    return pControl;
  }

  void Settle()
  {
    mDlg.SendCurrentParamValuesFromDelegate();
    mUI->RenderUntilClean(4);
    mUI->ClearRecorded();
    mDlg.ClearRecorded();
  }

  /** Redraw everything and record it */
  void Redraw()
  {
    mUI->ClearRecorded();
    mUI->SetAllControlsDirty();
    mUI->RenderFrame();
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
};

void ClickCentre(Fixture& f, const IRECT& r)
{
  f.mUI->Click(r.MW(), r.MH());
}
} // namespace

#pragma mark - IVButtonControl

TEST_CASE("IVButtonControl: a click fires the action, and the value returns to zero when the click animation ends", "[integration][controls][button]")
{
  Fixture f;
  int nActions = 0;
  int nEndActions = 0;
  auto* pButton = f.Attach(new IVButtonControl(kBounds, [&](IControl* pCaller) { nActions++; SplashClickActionFunc(pCaller); }, "Go"));
  pButton->SetAnimationEndActionFunction([&](IControl*) { nEndActions++; });

  REQUIRE(pButton->GetValue() == Approx(0.));

  f.mUI->MouseDown(kBounds.MW(), kBounds.MH());
  REQUIRE(nActions == 1);
  REQUIRE(pButton->GetValue() == Approx(1.));
  REQUIRE(pButton->GetAnimationFunction() != nullptr);

  f.mUI->MouseUp(kBounds.MW(), kBounds.MH());
  REQUIRE(nActions == 1);
  REQUIRE(nEndActions == 0);

  // The control is redrawn every frame while it animates
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(pButton->IsDirty());

  FinishAnimation(pButton);
  REQUIRE(pButton->GetValue() == Approx(0.));
  REQUIRE(nEndActions == 1);
  REQUIRE(pButton->GetAnimationFunction() == nullptr);
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE_FALSE(f.mUI->RenderFrame());
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("IVButtonControl: draws its label inside the button", "[integration][controls][button][drawing]")
{
  Fixture f;
  auto* pButton = f.Attach(new IVButtonControl(kBounds, EmptyClickActionFunc, "Press"));

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Press"));
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Fill) > 1);
  REQUIRE(kBounds.Contains(f.mUI->FindText("Press")->mBounds));

  pButton->SetLabelStr("Again");
  REQUIRE(pButton->IsDirty());
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Again"));
  REQUIRE_FALSE(f.mUI->DidDrawText("Press"));
}

TEST_CASE("IVButtonControl: only the button itself is clickable, not the label area beside it", "[integration][controls][button][mouse]")
{
  Fixture f;
  int nActions = 0;
  auto* pButton = f.Attach(new IVButtonControl(kBounds, [&](IControl*) { nActions++; }, "Label", DEFAULT_STYLE, false));
  const IRECT widgetBounds = pButton->GetWidgetBounds();
  const IRECT labelBounds = pButton->GetLabelBounds();

  REQUIRE(labelBounds.H() > 0.f);
  REQUIRE(widgetBounds.H() < kBounds.H());

  f.mUI->Click(kBounds.MW(), labelBounds.T + 1.f);
  REQUIRE(nActions == 0);
  ClickCentre(f, widgetBounds);
  REQUIRE(nActions == 1);
}

TEST_CASE("IVButtonControl: a double click counts as another click", "[integration][controls][button][mouse]")
{
  Fixture f;
  int nActions = 0;
  f.Attach(new IVButtonControl(kBounds, [&](IControl*) { nActions++; }));

  ClickCentre(f, kBounds);
  f.mUI->DblClick(kBounds.MW(), kBounds.MH());
  f.mUI->MouseUp(kBounds.MW(), kBounds.MH());
  REQUIRE(nActions == 2);
}

#pragma mark - IVSwitchControl

TEST_CASE("IVSwitchControl: clicks cycle through the states of an enum parameter", "[integration][controls][switch][params]")
{
  Fixture f;
  auto* pSwitch = f.Attach(new IVSwitchControl(kBounds, kMode));
  const IRECT widget = pSwitch->GetWidgetBounds();

  REQUIRE(pSwitch->GetSelectedIdx() == 0);
  REQUIRE(std::string(pSwitch->GetLabelStr()) == "Mode");

  ClickCentre(f, widget);
  REQUIRE(pSwitch->GetSelectedIdx() == 1);
  REQUIRE(pSwitch->GetValue() == Approx(0.5));
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 1);
  REQUIRE(std::string(pSwitch->GetValueStr()) == "Saw");

  ClickCentre(f, widget);
  REQUIRE(pSwitch->GetSelectedIdx() == 2);
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 2);

  ClickCentre(f, widget);
  REQUIRE(pSwitch->GetSelectedIdx() == 0);
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 0);

  // Three gestures, each with one value
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kMode) == 3);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kMode) == 3);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End, kMode) == 3);
}

TEST_CASE("IVSwitchControl: draws the parameter name and the display text of the current state", "[integration][controls][switch][drawing]")
{
  Fixture f;
  auto* pSwitch = f.Attach(new IVSwitchControl(kBounds, kMode));

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Mode"));
  REQUIRE(f.mUI->DidDrawText("Sine"));

  ClickCentre(f, pSwitch->GetWidgetBounds());
  FinishAnimation(pSwitch);
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Saw"));
  REQUIRE_FALSE(f.mUI->DidDrawText("Sine"));

  f.mDlg.GetParam(kMode)->Set(2.);
  f.mDlg.SendCurrentParamValuesFromDelegate();
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Square"));
}

TEST_CASE("IVSwitchControl: without a parameter it cycles through the given number of states and fires its action", "[integration][controls][switch]")
{
  Fixture f;
  std::vector<int> states;
  auto* pSwitch = f.Attach(new IVSwitchControl(kBounds, [&](IControl* pCaller) { states.push_back(pCaller->As<IVSwitchControl>()->GetSelectedIdx()); }, "", DEFAULT_STYLE, 4));

  for (int i = 0; i < 5; i++)
    ClickCentre(f, pSwitch->GetWidgetBounds());

  REQUIRE(states == std::vector<int>{1, 2, 3, 0, 1});
  REQUIRE(pSwitch->GetValue() == Approx(1. / 3.));
  REQUIRE(f.mDlg.mParamEvents.empty());
}

#pragma mark - IVToggleControl

TEST_CASE("IVToggleControl: a click toggles a bool parameter", "[integration][controls][toggle][params]")
{
  Fixture f;
  auto* pToggle = f.Attach(new IVToggleControl(kBounds, kOnOff));
  const IRECT widget = pToggle->GetWidgetBounds();

  ClickCentre(f, widget);
  REQUIRE(pToggle->GetValue() == Approx(1.));
  REQUIRE(f.mDlg.GetParam(kOnOff)->Bool());
  REQUIRE(f.mDlg.LastParamValue(kOnOff) == Approx(1.));

  ClickCentre(f, widget);
  REQUIRE(pToggle->GetValue() == Approx(0.));
  REQUIRE_FALSE(f.mDlg.GetParam(kOnOff)->Bool());

  // A drag that starts on the toggle is still just one toggle
  f.mUI->Drag(widget.MW(), widget.MH(), widget.MW() + 300.f, widget.MH());
  REQUIRE(f.mDlg.GetParam(kOnOff)->Bool());
}

TEST_CASE("IVToggleControl: draws the on or off text for its state", "[integration][controls][toggle][drawing]")
{
  Fixture f;
  auto* pToggle = f.Attach(new IVToggleControl(kBounds, kOnOff, "", DEFAULT_STYLE, "Bypassed", "Active"));

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Bypassed"));
  REQUIRE_FALSE(f.mUI->DidDrawText("Active"));

  f.mDlg.SendParameterValueFromDelegate(kOnOff, 1., true);
  REQUIRE(pToggle->IsDirty());
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Active"));
  REQUIRE_FALSE(f.mUI->DidDrawText("Bypassed"));
}

TEST_CASE("IVToggleControl: without a parameter it starts in the given state and fires its action", "[integration][controls][toggle]")
{
  Fixture f;
  int nActions = 0;
  auto* pToggle = f.Attach(new IVToggleControl(kBounds, [&](IControl*) { nActions++; }, "", DEFAULT_STYLE, "OFF", "ON", true));

  REQUIRE(pToggle->GetValue() == Approx(1.));
  ClickCentre(f, pToggle->GetWidgetBounds());
  REQUIRE(pToggle->GetValue() == Approx(0.));
  REQUIRE(nActions == 1);
}

#pragma mark - IVSlideSwitchControl

TEST_CASE("IVSlideSwitchControl: clicks step through the states of the parameter", "[integration][controls][slideswitch][params]")
{
  Fixture f;
  auto* pSwitch = f.Attach(new IVSlideSwitchControl(kBounds, kMode));
  const IRECT widget = pSwitch->GetWidgetBounds();

  ClickCentre(f, widget);
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 1);
  REQUIRE(pSwitch->GetSelectedIdx() == 1);
  // The handle slides to its new position
  REQUIRE(pSwitch->GetAnimationFunction() != nullptr);
  REQUIRE(f.mUI->RenderFrame());
  FinishAnimation(pSwitch);
  REQUIRE(pSwitch->GetAnimationFunction() == nullptr);

  ClickCentre(f, widget);
  FinishAnimation(pSwitch);
  ClickCentre(f, widget);
  FinishAnimation(pSwitch);
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 0);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value, kMode) == 3);

  f.Redraw();
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Fill) > 2);
}

TEST_CASE("IVSlideSwitchControl: without a parameter the action fires once the handle has finished sliding", "[integration][controls][slideswitch][animation]")
{
  Fixture f;
  int nActions = 0;
  auto* pSwitch = f.Attach(new IVSlideSwitchControl(kBounds, [&](IControl*) { nActions++; }, "", DEFAULT_STYLE, false, EDirection::Horizontal, 3, 0));

  ClickCentre(f, pSwitch->GetWidgetBounds());
  REQUIRE(pSwitch->GetSelectedIdx() == 1);
  REQUIRE(nActions == 0);
  FinishAnimation(pSwitch);
  REQUIRE(nActions == 1);
}

#pragma mark - IVTabSwitchControl / IVRadioButtonControl

TEST_CASE("IVTabSwitchControl: clicking a tab selects the corresponding parameter value", "[integration][controls][tabswitch][params]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 400, 90);
  auto* pTabs = f.Attach(new IVTabSwitchControl(bounds, kMode, {}, "", kPlainStyle));
  const IRECT widget = pTabs->GetWidgetBounds();

  for (int target : {2, 0, 1, 1})
  {
    const IRECT tab = widget.SubRect(EDirection::Horizontal, 3, target);
    ClickCentre(f, tab);
    REQUIRE(pTabs->GetSelectedIdx() == target);
    REQUIRE(f.mDlg.GetParam(kMode)->Int() == target);
    REQUIRE(pTabs->GetValue() == Approx(target / 2.));
  }

  REQUIRE(std::string(pTabs->GetSelectedLabelStr()) == "Saw");
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kMode) == 4);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End, kMode) == 4);
}

TEST_CASE("IVTabSwitchControl: draws a label on each tab, from the parameter or from the given options", "[integration][controls][tabswitch][drawing]")
{
  Fixture f;

  SECTION("from the parameter")
  {
    f.Attach(new IVTabSwitchControl(IRECT(100, 50, 400, 90), kMode, {}, "", kPlainStyle));
    f.Redraw();
    REQUIRE(f.mUI->DidDrawText("Sine"));
    REQUIRE(f.mUI->DidDrawText("Saw"));
    REQUIRE(f.mUI->DidDrawText("Square"));
    // The labels are laid out left to right
    REQUIRE(f.mUI->FindText("Sine")->mBounds.L < f.mUI->FindText("Saw")->mBounds.L);
    REQUIRE(f.mUI->FindText("Saw")->mBounds.L < f.mUI->FindText("Square")->mBounds.L);
  }

  SECTION("from the options")
  {
    f.Attach(new IVTabSwitchControl(IRECT(100, 50, 400, 90), kMode, {"A", "B", "C"}, "", kPlainStyle));
    f.Redraw();
    REQUIRE(f.mUI->DidDrawText("A"));
    REQUIRE(f.mUI->DidDrawText("C"));
    REQUIRE_FALSE(f.mUI->DidDrawText("Sine"));
  }

  SECTION("vertical tabs are laid out top to bottom")
  {
    f.Attach(new IVTabSwitchControl(IRECT(100, 50, 200, 250), kMode, {}, "", kPlainStyle, EVShape::Rectangle, EDirection::Vertical));
    f.Redraw();
    REQUIRE(f.mUI->FindText("Sine")->mBounds.T < f.mUI->FindText("Saw")->mBounds.T);
    REQUIRE(f.mUI->FindText("Saw")->mBounds.T < f.mUI->FindText("Square")->mBounds.T);
  }
}

TEST_CASE("IVTabSwitchControl: without a parameter the action function gets the selection", "[integration][controls][tabswitch]")
{
  Fixture f;
  std::string selected;
  auto* pTabs = new IVTabSwitchControl(IRECT(100, 50, 400, 90), [&](IControl* pCaller) { selected = pCaller->As<IVTabSwitchControl>()->GetSelectedLabelStr(); },
                                       {"One", "Two", "Three", "Four"}, "", kPlainStyle);
  f.Attach(pTabs);

  ClickCentre(f, pTabs->GetWidgetBounds().SubRect(EDirection::Horizontal, 4, 3));
  REQUIRE(selected == "Four");
  REQUIRE(pTabs->GetValue() == Approx(1.));
  ClickCentre(f, pTabs->GetWidgetBounds().SubRect(EDirection::Horizontal, 4, 1));
  REQUIRE(selected == "Two");
  REQUIRE(pTabs->GetValue() == Approx(1. / 3.));
}

TEST_CASE("IVTabSwitchControl: a value from the host changes the selected tab", "[integration][controls][tabswitch][delegate]")
{
  Fixture f;
  auto* pTabs = f.Attach(new IVTabSwitchControl(IRECT(100, 50, 400, 90), kMode, {}, "", kPlainStyle));

  f.mDlg.SendParameterValueFromDelegate(kMode, 2., false);
  REQUIRE(pTabs->GetSelectedIdx() == 2);
  REQUIRE(std::string(pTabs->GetSelectedLabelStr()) == "Square");
  REQUIRE(pTabs->IsDirty());
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("IVRadioButtonControl: clicking an option selects it", "[integration][controls][radiobutton][params]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 250, 200);
  auto* pRadio = f.Attach(new IVRadioButtonControl(bounds, kMode, {}, "", kPlainStyle));
  const IRECT widget = pRadio->GetWidgetBounds();

  ClickCentre(f, widget.SubRect(EDirection::Vertical, 3, 2));
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 2);
  ClickCentre(f, widget.SubRect(EDirection::Vertical, 3, 1));
  REQUIRE(f.mDlg.GetParam(kMode)->Int() == 1);
  REQUIRE(pRadio->GetSelectedIdx() == 1);

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Sine"));
  REQUIRE(f.mUI->DidDrawText("Saw"));
  REQUIRE(f.mUI->DidDrawText("Square"));
  REQUIRE(f.mUI->FindText("Sine")->mBounds.T < f.mUI->FindText("Square")->mBounds.T);
}

#pragma mark - IVKnobControl

TEST_CASE("IVKnobControl: draws its label, its value and a knob", "[integration][controls][knob][drawing]")
{
  Fixture f;
  auto* pKnob = f.Attach(new IVKnobControl(IRECT(100, 50, 200, 200), kGain));

  REQUIRE(std::string(pKnob->GetLabelStr()) == "Gain");
  REQUIRE(std::string(pKnob->GetValueStr()) == "50.00 %");

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Gain"));
  REQUIRE(f.mUI->DidDrawText("50.00 %"));
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Fill) >= 2);
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Stroke) >= 1);
  // label above the knob, value beneath
  REQUIRE(f.mUI->FindText("Gain")->mBounds.T < pKnob->GetWidgetBounds().T + 1.f);
  REQUIRE(f.mUI->FindText("50.00 %")->mBounds.T >= pKnob->GetWidgetBounds().B - 1.f);

  f.mDlg.GetParam(kGain)->Set(12.5);
  f.mDlg.SendCurrentParamValuesFromDelegate();
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("12.50 %"));
}

TEST_CASE("IVKnobControl: the value text follows a drag", "[integration][controls][knob][drawing][mouse]")
{
  Fixture f;
  auto* pKnob = f.Attach(new IVKnobControl(IRECT(100, 50, 200, 200), kGain));
  const IRECT widget = pKnob->GetWidgetBounds();

  f.mUI->Drag(widget.MW(), widget.MH(), widget.MW(), widget.MH() - 5000.f);
  REQUIRE(std::string(pKnob->GetValueStr()) == "100.00 %");
  f.mUI->RenderUntilClean();
  REQUIRE(f.mUI->DidDrawText("100.00 %"));
}

TEST_CASE("IVKnobControl: an explicit label replaces the parameter name", "[integration][controls][knob][drawing]")
{
  Fixture f;
  f.Attach(new IVKnobControl(IRECT(100, 50, 200, 200), kGain, "Volume"));

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Volume"));
  REQUIRE_FALSE(f.mUI->DidDrawText("Gain"));
}

TEST_CASE("IVKnobControl: the style decides whether the label and value are shown", "[integration][controls][knob][drawing][style]")
{
  Fixture f;
  auto* pKnob = f.Attach(new IVKnobControl(IRECT(100, 50, 200, 200), kGain, "", kPlainStyle));

  f.Redraw();
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Text) == 0);
  const float plainWidgetHeight = pKnob->GetWidgetBounds().H();

  pKnob->SetShowLabel(true);
  pKnob->SetShowValue(true);
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Gain"));
  REQUIRE(f.mUI->DidDrawText("50.00 %"));
  // The text takes room from the knob
  REQUIRE(pKnob->GetWidgetBounds().H() < plainWidgetHeight);
}

TEST_CASE("IVKnobControl: only the knob itself responds to the mouse", "[integration][controls][knob][mouse]")
{
  Fixture f;
  auto* pKnob = f.Attach(new IVKnobControl(IRECT(100, 50, 200, 200), kGain));
  const IRECT label = pKnob->GetLabelBounds();

  f.mUI->Drag(label.MW(), label.MH(), label.MW(), label.MH() - 50.f);
  REQUIRE(f.mDlg.mParamEvents.empty());
  REQUIRE(pKnob->GetValue() == Approx(0.5));
}

TEST_CASE("IVKnobControl: without a parameter the action function reports the value", "[integration][controls][knob]")
{
  Fixture f;
  double lastValue = -1.;
  auto* pKnob = f.Attach(new IVKnobControl(kBounds, [&](IControl* pCaller) { lastValue = pCaller->GetValue(); }, "", kPlainStyle));

  f.mUI->Drag(kBounds.MW(), kBounds.MH(), kBounds.MW(), kBounds.MH() - 10.f);
  REQUIRE(pKnob->GetValue() > 0.);
  REQUIRE(lastValue == Approx(pKnob->GetValue()));
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("IVKnobControl: a bubble control shows the value while the knob is dragged", "[integration][controls][knob][bubble]")
{
  Fixture f;
  f.mUI->AttachBubbleControl();
  auto* pBubble = f.mUI->GetBubbleControl();
  auto* pKnob = f.Attach(new IVKnobControl(kBounds, kGain, "", kPlainStyle));
  pKnob->SetActionFunction(ShowBubbleHorizontalActionFunc);

  REQUIRE(pBubble->IsHidden());

  f.mUI->MouseDown(kBounds.MW(), kBounds.MH());
  f.mUI->MouseDragTo(kBounds.MW(), kBounds.MH() - 5000.f);

  REQUIRE_FALSE(pBubble->IsHidden());
  REQUIRE(pBubble->GetRECT().W() > 0.f);

  f.mUI->ClearRecorded();
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->DidDrawText("100.00 %"));
  f.mUI->MouseUp(kBounds.MW(), kBounds.MH() - 5000.f);
}

#pragma mark - IVSliderControl

TEST_CASE("IVSliderControl: a click on the track of a vertical slider sets the value from the position", "[integration][controls][slider][mouse]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 140, 350);
  auto* pSlider = f.Attach(new IVSliderControl(bounds, kGain, "", kPlainStyle));
  const IRECT track = pSlider->GetTrackBounds();

  REQUIRE(bounds.Contains(track));
  REQUIRE(track.H() > 0.8f * bounds.H());

  SECTION("bottom of the track is the minimum")
  {
    f.mUI->Click(bounds.MW(), bounds.B - 1.f);
    REQUIRE(pSlider->GetValue() == Approx(0.));
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(0.));
  }

  SECTION("top of the track is the maximum")
  {
    f.mUI->Click(bounds.MW(), bounds.T + 1.f);
    REQUIRE(pSlider->GetValue() == Approx(1.));
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(100.));
  }

  SECTION("the middle of the track is the middle of the range")
  {
    f.mDlg.GetParam(kGain)->Set(0.);
    f.Settle();
    f.mUI->Click(bounds.MW(), track.MH());
    REQUIRE(pSlider->GetValue() == Approx(0.5).margin(0.002));
    REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(50.).margin(0.2));
  }

  SECTION("positions in between map linearly")
  {
    f.mUI->Click(bounds.MW(), track.B - 0.25f * track.H());
    REQUIRE(pSlider->GetValue() == Approx(0.25).margin(0.01));
    f.mUI->Click(bounds.MW(), track.B - 0.9f * track.H());
    REQUIRE(pSlider->GetValue() == Approx(0.9).margin(0.01));
  }
}

TEST_CASE("IVSliderControl: a horizontal slider maps left to the minimum and right to the maximum", "[integration][controls][slider][mouse]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 400, 90);
  auto* pSlider = f.Attach(new IVSliderControl(bounds, kGain, "", kPlainStyle, false, EDirection::Horizontal));
  const IRECT track = pSlider->GetTrackBounds();

  f.mUI->Click(bounds.L + 1.f, bounds.MH());
  REQUIRE(pSlider->GetValue() == Approx(0.));
  f.mUI->Click(bounds.R - 1.f, bounds.MH());
  REQUIRE(pSlider->GetValue() == Approx(1.));
  f.mUI->Click(track.MW(), bounds.MH());
  REQUIRE(pSlider->GetValue() == Approx(0.5).margin(0.002));
  f.mUI->Click(track.L + 0.75f * track.W(), bounds.MH());
  REQUIRE(pSlider->GetValue() == Approx(0.75).margin(0.01));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(75.).margin(1.));
}

TEST_CASE("IVSliderControl: dragging moves the value relative to where the drag started", "[integration][controls][slider][mouse]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 140, 350);
  auto* pSlider = f.Attach(new IVSliderControl(bounds, kGain, "", kPlainStyle));
  const float x = bounds.MW(), y = pSlider->GetTrackBounds().MH();

  f.mUI->MouseDown(x, y);
  const double start = pSlider->GetValue();
  f.mUI->MouseDragTo(x, y - 40.f);
  const double up = pSlider->GetValue();
  REQUIRE(up > start);
  f.mUI->MouseDragTo(x, y);
  REQUIRE(pSlider->GetValue() == Approx(start));
  f.mUI->MouseDragTo(x, y + 40.f);
  REQUIRE(pSlider->GetValue() == Approx(start - (up - start)));

  // Fine control with shift: the same distance moves the value a tenth as far
  const double before = pSlider->GetValue();
  f.mUI->MouseDragTo(x, y, IMouseMod(true, false, true));
  REQUIRE(pSlider->GetValue() - before == Approx((up - start) / 10.));
  f.mUI->MouseUp(x, y);

  REQUIRE(f.mDlg.mParamEvents.front().mType == ParamEvent::EType::Begin);
  REQUIRE(f.mDlg.mParamEvents.back().mType == ParamEvent::EType::End);
  REQUIRE(f.mDlg.GetParam(kGain)->GetNormalized() == Approx(pSlider->GetValue()));
}

TEST_CASE("IVSliderControl: with the cursor left visible the handle follows the pointer exactly", "[integration][controls][slider][mouse]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 140, 350);
  auto* pSlider = f.Attach(new IVSliderControl(bounds, kGain, "", kPlainStyle.WithHideCursor(false)));
  const IRECT track = pSlider->GetTrackBounds();

  f.mUI->MouseDown(bounds.MW(), track.MH());
  REQUIRE_FALSE(f.mUI->mCursorHidden);

  f.mUI->MouseDragTo(bounds.MW(), track.B - 0.2f * track.H());
  REQUIRE(pSlider->GetValue() == Approx(0.2).margin(0.01));
  f.mUI->MouseDragTo(bounds.MW(), track.B - 0.8f * track.H());
  REQUIRE(pSlider->GetValue() == Approx(0.8).margin(0.01));
  f.mUI->MouseDragTo(bounds.MW(), bounds.T - 100.f);
  REQUIRE(pSlider->GetValue() == Approx(1.));
  f.mUI->MouseDragTo(bounds.MW(), bounds.B + 100.f);
  REQUIRE(pSlider->GetValue() == Approx(0.));
  f.mUI->MouseUp(bounds.MW(), bounds.B + 100.f);
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(0.));
}

TEST_CASE("IVSliderControl: a slider linked to a stepped parameter stays on the steps", "[integration][controls][slider][params]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 140, 350);
  auto* pSlider = f.Attach(new IVSliderControl(bounds, kSteps, "", kPlainStyle));
  const float x = bounds.MW(), y = bounds.MH();

  // The click itself does not jump the value
  f.mUI->MouseDown(x, y);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 2);

  for (int i = 1; i <= 200; i++)
  {
    f.mUI->MouseDragTo(x, y - static_cast<float>(i));
    const double stepped = pSlider->GetValue() * 10.;
    REQUIRE(stepped == Approx(std::round(stepped)).margin(1e-9));
  }

  f.mUI->MouseUp(x, y - 200.f);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() > 2);
}

TEST_CASE("IVSliderControl: draws label, value, track and handle; the filled part of the track grows with the value", "[integration][controls][slider][drawing]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 160, 350);
  auto* pSlider = f.Attach(new IVSliderControl(bounds, kGain));

  auto filledTrackHeight = [&]() {
    // The value part of the track is drawn in the kX1 colour
    const IColor c = pSlider->GetColor(kX1);
    for (auto& op : f.mUI->mDrawOps)
    {
      if (op.mType == DrawOp::EType::Fill && op.mColor.R == c.R && op.mColor.G == c.G && op.mColor.B == c.B && op.mColor.A == c.A)
        return op.mBounds.H();
    }
    return -1.f;
  };

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Gain"));
  REQUIRE(f.mUI->DidDrawText("50.00 %"));
  const float halfHeight = filledTrackHeight();
  REQUIRE(halfHeight > 0.f);

  f.mDlg.GetParam(kGain)->Set(100.);
  f.mDlg.SendCurrentParamValuesFromDelegate();
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("100.00 %"));
  REQUIRE(filledTrackHeight() == Approx(2.f * halfHeight).margin(1.f));
}

TEST_CASE("IVSliderControl: the wheel moves the slider", "[integration][controls][slider][wheel]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 140, 350);
  auto* pSlider = f.Attach(new IVSliderControl(bounds, kGain, "", kPlainStyle));

  f.mUI->Wheel(bounds.MW(), bounds.MH(), 1.f);
  const double step = pSlider->GetValue() - 0.5;
  REQUIRE(step > 0.);
  f.mUI->Wheel(bounds.MW(), bounds.MH(), -2.f);
  REQUIRE(pSlider->GetValue() == Approx(0.5 - step));
  f.mUI->Wheel(bounds.MW(), bounds.MH(), 1.f, IMouseMod(false, false, true));
  REQUIRE(pSlider->GetValue() == Approx(0.5 - step + step / 10.));
  REQUIRE(f.mDlg.GetParam(kGain)->GetNormalized() == Approx(pSlider->GetValue()));
}

#pragma mark - IVRangeSliderControl

TEST_CASE("IVRangeSliderControl: each handle is linked to its own parameter", "[integration][controls][rangeslider][params]")
{
  Fixture f;
  auto* pRange = f.Attach(new IVRangeSliderControl(IRECT(100, 50, 160, 350), {kLow, kHigh}, "", kPlainStyle));

  REQUIRE(pRange->NVals() == 2);
  REQUIRE(pRange->GetParamIdx(0) == kLow);
  REQUIRE(pRange->GetParamIdx(1) == kHigh);
  REQUIRE(pRange->GetValue(0) == Approx(0.25));
  REQUIRE(pRange->GetValue(1) == Approx(0.75));

  f.mDlg.SendParameterValueFromDelegate(kHigh, 0.9, true);
  REQUIRE(pRange->GetValue(0) == Approx(0.25));
  REQUIRE(pRange->GetValue(1) == Approx(0.9));

  f.Redraw();
  REQUIRE(f.mUI->CountOps(DrawOp::EType::Fill) > 2);
}

TEST_CASE("IVRangeSliderControl: dragging a handle changes its parameter, and the handles cannot cross", "[integration][controls][rangeslider][mouse]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);
  const IRECT bounds = IRECT(100, 50, 160, 350);
  auto* pRange = f.Attach(new IVRangeSliderControl(bounds, {kLow, kHigh}, "", kPlainStyle));
  const IRECT widget = pRange->GetWidgetBounds();
  const float handleSize = 8.f;
  auto yForValue = [&](double v) { return (widget.B - handleSize) - static_cast<float>(v) * (widget.H() - 2.f * handleSize); };

  // The handle for the low value is to the left of the track, the one for the high value to the right
  const float lowX = widget.MW() - handleSize;
  const float highX = widget.MW() + handleSize + 2.f;

  SECTION("the low handle")
  {
    f.mUI->MouseMove(lowX, yForValue(0.25));
    f.mUI->MouseDown(lowX, yForValue(0.25));
    f.mUI->MouseDragTo(lowX, widget.MH());
    REQUIRE(pRange->GetValue(0) == Approx(0.5).margin(0.01));
    REQUIRE(pRange->GetValue(1) == Approx(0.75));
    REQUIRE(f.mDlg.GetParam(kLow)->Value() == Approx(50.).margin(1.));
    REQUIRE(f.mDlg.GetParam(kHigh)->Value() == Approx(75.));

    // It stops at the high handle
    f.mUI->MouseDragTo(lowX, widget.T);
    REQUIRE(pRange->GetValue(0) == Approx(0.75));
    REQUIRE(f.mDlg.GetParam(kLow)->Value() == Approx(75.));
    f.mUI->MouseUp(lowX, widget.T);
  }

  SECTION("the high handle")
  {
    f.mUI->MouseMove(highX, yForValue(0.75));
    f.mUI->MouseDown(highX, yForValue(0.75));
    f.mUI->MouseDragTo(highX, widget.T);
    REQUIRE(pRange->GetValue(1) == Approx(1.));
    REQUIRE(f.mDlg.GetParam(kHigh)->Value() == Approx(100.));

    // It stops at the low handle
    f.mUI->MouseDragTo(highX, widget.B);
    REQUIRE(pRange->GetValue(1) == Approx(0.25));
    REQUIRE(pRange->GetValue(0) == Approx(0.25));
    f.mUI->MouseUp(highX, widget.B);
  }

  REQUIRE(f.mDlg.mParamEvents.front().mType == ParamEvent::EType::Begin);
  REQUIRE(f.mDlg.mParamEvents.back().mType == ParamEvent::EType::End);
}

TEST_CASE("IVRangeSliderControl: a click away from the handles changes nothing", "[integration][controls][rangeslider][mouse]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);
  const IRECT bounds = IRECT(100, 50, 160, 350);
  auto* pRange = f.Attach(new IVRangeSliderControl(bounds, {kLow, kHigh}, "", kPlainStyle));

  f.mUI->MouseMove(bounds.MW(), bounds.MH());
  f.mUI->Drag(bounds.MW(), bounds.MH(), bounds.MW(), bounds.T);

  REQUIRE(pRange->GetValue(0) == Approx(0.25));
  REQUIRE(pRange->GetValue(1) == Approx(0.75));
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value) == 0);
}

#pragma mark - IVXYPadControl

TEST_CASE("IVXYPadControl: the pointer position sets both values, clamped to the pad", "[integration][controls][xypad][mouse]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 300, 250);
  auto* pPad = f.Attach(new IVXYPadControl(bounds, {kLow, kHigh}, "", kPlainStyle));

  f.mUI->MouseDown(bounds.L + 50.f, bounds.T + 150.f);
  REQUIRE(pPad->GetValue(0) == Approx(0.25));
  REQUIRE(pPad->GetValue(1) == Approx(0.25));
  REQUIRE(f.mDlg.GetParam(kLow)->Value() == Approx(25.));
  REQUIRE(f.mDlg.GetParam(kHigh)->Value() == Approx(25.));

  f.mUI->MouseDragTo(bounds.R, bounds.T);
  REQUIRE(pPad->GetValue(0) == Approx(1.));
  REQUIRE(pPad->GetValue(1) == Approx(1.));

  // Outside the pad
  f.mUI->MouseDragTo(bounds.L - 500.f, bounds.B + 500.f);
  REQUIRE(pPad->GetValue(0) == Approx(0.));
  REQUIRE(pPad->GetValue(1) == Approx(0.));
  REQUIRE(f.mDlg.GetParam(kLow)->Value() == Approx(0.));
  REQUIRE(f.mDlg.GetParam(kHigh)->Value() == Approx(0.));

  f.mUI->MouseUp(bounds.L - 500.f, bounds.B + 500.f);
  REQUIRE_FALSE(f.mUI->mCursorHidden);
}

TEST_CASE("IVXYPadControl: draws a handle at the position given by the values", "[integration][controls][xypad][drawing]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 300, 250);
  auto* pPad = f.Attach(new IVXYPadControl(bounds, {kLow, kHigh}, "", kPlainStyle, 10.f));

  auto handleCentre = [&](float& cx, float& cy) {
    // The handle is the last thing filled; its shape is 20 points across
    for (auto itr = f.mUI->mDrawOps.rbegin(); itr != f.mUI->mDrawOps.rend(); ++itr)
    {
      if (itr->mType == DrawOp::EType::Fill && itr->mDeviceBounds.W() <= 21.f && itr->mDeviceBounds.W() >= 13.f)
      {
        cx = itr->mDeviceBounds.MW();
        cy = itr->mDeviceBounds.MH();
        return true;
      }
    }
    return false;
  };

  float cx = 0.f, cy = 0.f;
  f.Redraw();
  REQUIRE(handleCentre(cx, cy));
  const IRECT widget = pPad->GetWidgetBounds();
  REQUIRE(cx == Approx(widget.L + 0.25f * widget.W()).margin(3.f));
  REQUIRE(cy == Approx(widget.B - 0.75f * widget.H()).margin(3.f));

  f.mDlg.SendParameterValueFromDelegate(kLow, 0.9, true);
  f.mDlg.SendParameterValueFromDelegate(kHigh, 0.1, true);
  f.Redraw();
  REQUIRE(handleCentre(cx, cy));
  REQUIRE(cx == Approx(widget.L + 0.9f * widget.W()).margin(3.f));
  REQUIRE(cy == Approx(widget.B - 0.1f * widget.H()).margin(3.f));
}

#pragma mark - IVNumberBoxControl

TEST_CASE("IVNumberBoxControl: shows the parameter value, and the wheel changes it a step at a time", "[integration][controls][numberbox][wheel][params]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 50, 200, 80);
  auto* pBox = f.Attach(new IVNumberBoxControl(bounds, kSteps, nullptr, "", kPlainStyle));
  const float x = pBox->GetTargetRECT().MW(), y = pBox->GetTargetRECT().MH();

  REQUIRE(pBox->GetRealValue() == Approx(2.));
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("2"));

  f.mUI->Wheel(x, y, 1.f);
  REQUIRE(pBox->GetRealValue() == Approx(3.));
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 3);
  REQUIRE(f.mDlg.LastParamValue(kSteps) == Approx(0.3));

  f.mUI->Wheel(x, y, -1.f);
  f.mUI->Wheel(x, y, -1.f);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 1);

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("1"));
}

TEST_CASE("IVNumberBoxControl: the value is clamped to the range of the parameter", "[integration][controls][numberbox][wheel][params]")
{
  Fixture f;
  auto* pBox = f.Attach(new IVNumberBoxControl(IRECT(100, 50, 200, 80), kSteps, nullptr, "", kPlainStyle));
  const float x = pBox->GetTargetRECT().MW(), y = pBox->GetTargetRECT().MH();

  for (int i = 0; i < 15; i++)
    f.mUI->Wheel(x, y, 1.f);

  REQUIRE(pBox->GetRealValue() == Approx(10.));
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 10);

  for (int i = 0; i < 25; i++)
    f.mUI->Wheel(x, y, -1.f);

  REQUIRE(pBox->GetRealValue() == Approx(0.));
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 0);
}

TEST_CASE("IVNumberBoxControl: dragging up increases the value and dragging down decreases it", "[integration][controls][numberbox][mouse][params]")
{
  Fixture f;
  auto* pBox = f.Attach(new IVNumberBoxControl(IRECT(100, 50, 200, 80), kSteps, nullptr, "", kPlainStyle));
  const float x = pBox->GetTargetRECT().MW(), y = pBox->GetTargetRECT().MH();

  f.mUI->MouseDown(x, y);
  REQUIRE(f.mUI->mCursorHidden);
  f.mUI->MouseDragTo(x, y - 3.f);
  REQUIRE(pBox->GetRealValue() == Approx(5.));
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 5);
  f.mUI->MouseDragTo(x, y + 1.f);
  REQUIRE(pBox->GetRealValue() == Approx(1.));
  f.mUI->MouseDragTo(x, y + 100.f);
  REQUIRE(pBox->GetRealValue() == Approx(0.));
  f.mUI->MouseUp(x, y + 100.f);

  REQUIRE_FALSE(f.mUI->mCursorHidden);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 0);
  REQUIRE(f.mDlg.mParamEvents.front().mType == ParamEvent::EType::Begin);
  REQUIRE(f.mDlg.mParamEvents.back().mType == ParamEvent::EType::End);
}

// BUG: IVNumberBoxControl::OnMouseDown()/OnMouseUp() (IGraphics/Controls/IVNumberBoxControl.h:161 and :172) call
// BeginInformHostOfParamChangeFromUI()/EndInformHostOfParamChangeFromUI() themselves, but IGraphics::OnMouseDown() and
// IGraphics::OnMouseUp() already do that for every control linked to a parameter, so each drag on a number box sends
// the host two begin-gesture and two end-gesture notifications.
TEST_CASE("IVNumberBoxControl: a drag is a single gesture for the host", "[.][bug]")
{
  Fixture f;
  auto* pBox = f.Attach(new IVNumberBoxControl(IRECT(100, 50, 200, 80), kSteps, nullptr, "", kPlainStyle));
  const float x = pBox->GetTargetRECT().MW(), y = pBox->GetTargetRECT().MH();

  f.mUI->Drag(x, y, x, y - 3.f);

  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 5);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Begin, kSteps) == 1);
  REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::End, kSteps) == 1);
}

TEST_CASE("IVNumberBoxControl: the increment and decrement buttons step the value", "[integration][controls][numberbox][mouse]")
{
  Fixture f;
  auto* pBox = f.Attach(new IVNumberBoxControl(IRECT(100, 50, 220, 90), kSteps, nullptr, "", DEFAULT_STYLE, true));

  REQUIRE(pBox->NChildren() == 3);
  IControl* pInc = pBox->GetChild(1);
  IControl* pDec = pBox->GetChild(2);
  REQUIRE(pInc->GetRECT().MH() < pDec->GetRECT().MH());
  REQUIRE(pInc->GetRECT().L >= pBox->GetChild(0)->GetRECT().R - 1.f);

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("+"));
  REQUIRE(f.mUI->DidDrawText("-"));

  auto press = [&](IControl* pButton) {
    ClickCentre(f, pButton->GetRECT());
    FinishAnimation(pButton);
  };

  press(pInc);
  press(pInc);
  REQUIRE(pBox->GetRealValue() == Approx(4.));
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 4);

  press(pDec);
  REQUIRE(pBox->GetRealValue() == Approx(3.));
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 3);
  REQUIRE(f.mDlg.LastParamValue(kSteps) == Approx(0.3));

  for (int i = 0; i < 12; i++)
    press(pInc);

  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 10);
}

TEST_CASE("IVNumberBoxControl: a value from the host updates the readout without an echo", "[integration][controls][numberbox][delegate]")
{
  Fixture f;
  auto* pBox = f.Attach(new IVNumberBoxControl(IRECT(100, 50, 200, 80), kSteps, nullptr, "", kPlainStyle));

  f.mDlg.SendParameterValueFromDelegate(kSteps, 8., false);
  REQUIRE(pBox->GetRealValue() == Approx(8.));
  REQUIRE(pBox->GetValue() == Approx(0.8));
  REQUIRE(f.mDlg.mParamEvents.empty());

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("8"));
}

TEST_CASE("IVNumberBoxControl: without a parameter it works in its own range and reports changes to the action function", "[integration][controls][numberbox]")
{
  Fixture f;
  int nActions = 0;
  auto* pBox = f.Attach(new IVNumberBoxControl(IRECT(100, 50, 200, 80), kNoParameter, [&](IControl*) { nActions++; }, "", kPlainStyle, false, 20., 10., 22., "%0.1f"));
  const float x = pBox->GetTargetRECT().MW(), y = pBox->GetTargetRECT().MH();

  REQUIRE(pBox->GetRealValue() == Approx(20.));
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("20.0"));

  f.mUI->Wheel(x, y, 1.f);
  REQUIRE(pBox->GetRealValue() == Approx(21.));
  REQUIRE(nActions == 1);

  // The fine increment
  f.mUI->Wheel(x, y, 1.f, IMouseMod(false, false, true));
  REQUIRE(pBox->GetRealValue() == Approx(21.1));

  f.mUI->Wheel(x, y, 1.f);
  f.mUI->Wheel(x, y, 1.f);
  REQUIRE(pBox->GetRealValue() == Approx(22.));

  for (int i = 0; i < 20; i++)
    f.mUI->Wheel(x, y, -1.f);

  REQUIRE(pBox->GetRealValue() == Approx(10.));
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("10.0"));
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("IVNumberBoxControl: a double click opens a text entry for typing a value", "[integration][controls][numberbox][textentry]")
{
  Fixture f;
  auto* pBox = f.Attach(new IVNumberBoxControl(IRECT(100, 50, 200, 80), kSteps, nullptr, "", kPlainStyle));
  const float x = pBox->GetTargetRECT().MW(), y = pBox->GetTargetRECT().MH();

  f.mUI->DblClick(x, y);
  REQUIRE(f.mUI->mNumTextEntries == 1);
  REQUIRE(f.mUI->mTextEntryStr == "2");
  REQUIRE(f.mUI->mTextEntryBounds == pBox->GetChild(0)->GetRECT());

  f.mUI->SetControlValueAfterTextEdit("7");
  REQUIRE(pBox->GetRealValue() == Approx(7.));
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 7);

  f.mUI->DblClick(x, y);
  f.mUI->SetControlValueAfterTextEdit("99");
  REQUIRE(pBox->GetRealValue() == Approx(10.));
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 10);
}

#pragma mark - IVMenuButtonControl

TEST_CASE("IVMenuButtonControl: shows the current option, and clicking it offers the options in a menu", "[integration][controls][menubutton][popupmenu][params]")
{
  Fixture f;
  auto* pMenuButton = f.Attach(new IVMenuButtonControl(IRECT(100, 50, 220, 90), kMode, "", kPlainStyle.WithShowValue(true)));

  REQUIRE(pMenuButton->NChildren() == 1);
  auto* pButton = pMenuButton->GetChild(0)->As<IVButtonControl>();
  REQUIRE(pButton != nullptr);
  REQUIRE(std::string(pButton->GetValueStr()) == "Sine");

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Sine"));

  SECTION("an option is chosen")
  {
    f.mUI->mPopupMenuChoice = 2;
    ClickCentre(f, pButton->GetRECT());
    REQUIRE(f.mUI->mNumPopupMenus == 0);
    // The menu appears once the button's click animation has finished
    FinishAnimation(pButton);

    REQUIRE(f.mUI->mNumPopupMenus == 1);
    REQUIRE(f.mUI->mPopupMenuItems == std::vector<std::string>{"Sine", "Saw", "Square"});
    REQUIRE(f.mDlg.GetParam(kMode)->Int() == 2);
    REQUIRE(pMenuButton->GetValue() == Approx(1.));
    REQUIRE(std::string(pButton->GetValueStr()) == "Square");
    REQUIRE(f.mDlg.LastParamValue(kMode) == Approx(1.));

    f.Redraw();
    REQUIRE(f.mUI->DidDrawText("Square"));
  }

  SECTION("the menu is dismissed")
  {
    f.mUI->mPopupMenuChoice = -1;
    ClickCentre(f, pButton->GetRECT());
    FinishAnimation(pButton);

    REQUIRE(f.mUI->mNumPopupMenus == 1);
    REQUIRE(f.mDlg.GetParam(kMode)->Int() == 0);
    REQUIRE(std::string(pButton->GetValueStr()) == "Sine");
    REQUIRE(f.mDlg.CountParamEvents(ParamEvent::EType::Value) == 0);
  }
}

TEST_CASE("IVMenuButtonControl: a value from the host updates the text on the button", "[integration][controls][menubutton][delegate]")
{
  Fixture f;
  auto* pMenuButton = f.Attach(new IVMenuButtonControl(IRECT(100, 50, 220, 90), kMode, "", kPlainStyle.WithShowValue(true)));
  auto* pButton = pMenuButton->GetChild(0)->As<IVButtonControl>();

  f.mDlg.GetParam(kMode)->Set(1.);
  f.mDlg.SendCurrentParamValuesFromDelegate();

  REQUIRE(pMenuButton->GetValue() == Approx(0.5));
  REQUIRE(std::string(pButton->GetValueStr()) == "Saw");
  REQUIRE(f.mDlg.mParamEvents.empty());
}

#pragma mark - IVGroupControl / IVPanelControl / IVLabelControl

TEST_CASE("IVGroupControl: its bounds wrap the controls in its group", "[integration][controls][group]")
{
  Fixture f;
  f.mUI->AttachControl(new IVKnobControl(IRECT(100, 100, 160, 180), kGain), kNoTag, "osc");
  f.mUI->AttachControl(new IVKnobControl(IRECT(180, 120, 240, 220), kLow), kNoTag, "osc");
  f.mUI->AttachControl(new IVKnobControl(IRECT(400, 300, 460, 380), kHigh), kNoTag, "filter");
  const IRECT unionBounds = IRECT(100, 100, 240, 220);

  SECTION("with no padding")
  {
    auto* pGroup = f.Attach(new IVGroupControl("Oscillator", "osc"));
    REQUIRE(pGroup->GetRECT() == unionBounds);
  }

  SECTION("with padding")
  {
    auto* pGroup = f.Attach(new IVGroupControl("Oscillator", "osc", 10.f, 20.f, 30.f, 40.f));
    REQUIRE(pGroup->GetRECT() == IRECT(90, 80, 270, 260));
    REQUIRE(pGroup->GetRECT().Contains(unionBounds));
  }

  SECTION("it can be pointed at a different group")
  {
    auto* pGroup = f.Attach(new IVGroupControl("Oscillator", "osc"));
    pGroup->SetBoundsBasedOnGroup("filter", 5.f, 5.f, 5.f, 5.f);
    REQUIRE(pGroup->GetRECT() == IRECT(395, 295, 465, 385));
  }
}

TEST_CASE("IVGroupControl: draws its label and a frame, and lets the mouse through to the controls inside", "[integration][controls][group][drawing][mouse]")
{
  Fixture f;
  auto* pKnob = f.mUI->AttachControl(new IVKnobControl(IRECT(100, 100, 300, 200), kGain, "", kPlainStyle), kNoTag, "osc");
  auto* pGroup = f.Attach(new IVGroupControl("Oscillator", "osc", 10.f, 30.f, 10.f, 10.f));

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Oscillator"));
  REQUIRE(pGroup->GetRECT().Contains(f.mUI->FindText("Oscillator")->mBounds));

  bool drewFrame = false;
  for (auto& op : f.mUI->mDrawOps)
    drewFrame |= (op.mType == DrawOp::EType::Stroke && op.mBounds.W() > 100.f && pGroup->GetRECT().GetPadded(1.f).Contains(op.mBounds));
  REQUIRE(drewFrame);

  // The group is on top of the knob in the stack, but it ignores the mouse
  REQUIRE(f.mUI->GetControlIdx(pGroup) > f.mUI->GetControlIdx(pKnob));
  f.mUI->Drag(200, 150, 200, 100);
  REQUIRE(pKnob->GetValue() > 0.5);
}

TEST_CASE("IVGroupControl: can be a container that attaches its own children", "[integration][controls][group][container]")
{
  Fixture f;
  IControl* pChild = nullptr;
  IRECT attachBounds;
  auto* pGroup = f.Attach(new IVGroupControl(IRECT(100, 100, 300, 250), "Section", 10.f, DEFAULT_STYLE,
    [&](IContainerBase* pContainer, const IRECT& bounds) {
      attachBounds = bounds;
      pChild = pContainer->AddChildControl(new IVKnobControl(bounds.GetCentredInside(60.f), kGain, "", kPlainStyle));
    }));

  REQUIRE(pGroup->NChildren() == 1);
  REQUIRE(pChild->GetParent() == pGroup);
  REQUIRE(attachBounds == IRECT(100, 100, 300, 250));

  pGroup->Hide(true);
  REQUIRE(pChild->IsHidden());
}

TEST_CASE("IVPanelControl: draws a panel and hosts child controls", "[integration][controls][panel][container][drawing]")
{
  Fixture f;
  auto* pPanel = new IVPanelControl(IRECT(100, 100, 300, 250), "Panel");
  IControl* pChild = nullptr;
  pPanel->SetAttachFunc([&](IContainerBase* pContainer, const IRECT& bounds) {
    pChild = pContainer->AddChildControl(new IVKnobControl(bounds.GetCentredInside(60.f), kGain, "", kPlainStyle));
  });
  f.Attach(pPanel);

  REQUIRE(pPanel->NChildren() == 1);
  REQUIRE(pPanel->GetRECT().Contains(pChild->GetRECT()));
  REQUIRE(pPanel->GetIgnoreMouse());

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Panel"));
  REQUIRE(f.mUI->CountOpsInside(DrawOp::EType::Fill, pPanel->GetRECT().GetPadded(4.f)) >= 2);

  const IRECT childBounds = pChild->GetRECT();
  f.mUI->Drag(childBounds.MW(), childBounds.MH(), childBounds.MW(), childBounds.MH() - 20.f);
  REQUIRE(pChild->GetValue() > 0.5);
}

TEST_CASE("IVLabelControl: draws its text and can be changed", "[integration][controls][label][drawing]")
{
  Fixture f;
  auto* pLabel = f.Attach(new IVLabelControl(IRECT(100, 100, 300, 130), "Hello"));

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Hello"));

  pLabel->SetStr("World");
  REQUIRE(pLabel->IsDirty());
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->DidDrawText("World"));

  pLabel->SetStrFmt(32, "%s %d", "Take", 5);
  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("Take 5"));

  // A label does not take mouse events
  REQUIRE(pLabel->GetIgnoreMouse());
}

#pragma mark - IVColorSwatchControl

TEST_CASE("IVColorSwatchControl: clicking a cell picks a colour for it", "[integration][controls][colorswatch]")
{
  Fixture f;
  int chosenCell = -1;
  IColor chosenColor;
  auto* pSwatch = f.Attach(new IVColorSwatchControl(IRECT(100, 100, 400, 130), "", [&](int cell, IColor color) { chosenCell = cell; chosenColor = color; },
                                                   kPlainStyle, IVColorSwatchControl::ECellLayout::kHorizontal, {kBG, kFG, kPR}, {"", "", ""}));
  const IRECT widget = pSwatch->GetWidgetBounds();
  const IRECT cell1 = widget.GetGridCell(1, 1, 3);

  SECTION("the platform has no colour picker: nothing changes")
  {
    ClickCentre(f, cell1);
    REQUIRE(chosenCell == -1);
  }

  SECTION("a colour is picked")
  {
    f.mUI->mColorPickerAvailable = true;
    f.mUI->mColorPickerResult = IColor(255, 1, 2, 3);
    ClickCentre(f, cell1);

    REQUIRE(chosenCell == 1);
    REQUIRE(chosenColor.R == 1);
    REQUIRE(chosenColor.G == 2);
    REQUIRE(chosenColor.B == 3);
    REQUIRE(pSwatch->GetColor(kFG).B == 3);
    REQUIRE(pSwatch->IsDirty());

    // The cell is drawn in the new colour
    f.Redraw();
    bool drewCell = false;
    for (auto& op : f.mUI->mDrawOps)
      drewCell |= (op.mType == DrawOp::EType::Fill && op.mColor.R == 1 && op.mColor.G == 2 && op.mColor.B == 3 && cell1.Contains(op.mBounds));
    REQUIRE(drewCell);
  }
}

#pragma mark - IVTabbedPagesControl

TEST_CASE("IVTabbedPagesControl: shows one page at a time, and the tabs switch between them", "[integration][controls][tabbedpages][container]")
{
  Fixture f;
  IControl* pKnobA = nullptr;
  IControl* pKnobB = nullptr;
  auto* pPageA = new IVTabPage([&](IVTabPage* pPage, const IRECT& bounds) { pKnobA = pPage->AddChildControl(new IVKnobControl(bounds, kGain, "", kPlainStyle)); });
  auto* pPageB = new IVTabPage([&](IVTabPage* pPage, const IRECT& bounds) { pKnobB = pPage->AddChildControl(new IVKnobControl(bounds, kLow, "", kPlainStyle)); });
  const IRECT bounds = IRECT(50, 50, 450, 350);
  auto* pTabs = f.Attach(new IVTabbedPagesControl(bounds, {{"First", pPageA}, {"Second", pPageB}}, "", kPlainStyle));

  REQUIRE(pTabs->NPages() == 2);
  REQUIRE(pKnobA != nullptr);
  REQUIRE(pKnobB != nullptr);
  auto* pTabSwitch = pTabs->GetChild(0)->As<IVTabSwitchControl>();
  REQUIRE(pTabSwitch != nullptr);

  // The first page is showing
  REQUIRE_FALSE(pPageA->IsHidden());
  REQUIRE_FALSE(pKnobA->IsHidden());
  REQUIRE(pPageB->IsHidden());
  REQUIRE(pKnobB->IsHidden());
  REQUIRE(pPageA->GetRECT() == pTabs->GetPageArea());
  REQUIRE(pTabs->GetPageArea().Contains(pKnobA->GetRECT()));
  REQUIRE(pKnobA->GetRECT().W() > 0.f);

  f.Redraw();
  REQUIRE(f.mUI->DidDrawText("First"));
  REQUIRE(f.mUI->DidDrawText("Second"));

  // Only the visible page's control can be used
  const IRECT knobBounds = pKnobA->GetRECT();
  f.mUI->Drag(knobBounds.MW(), knobBounds.MH(), knobBounds.MW(), knobBounds.MH() - 20.f);
  REQUIRE(f.mDlg.GetParam(kGain)->Value() > 50.);
  REQUIRE(f.mDlg.GetParam(kLow)->Value() == Approx(25.));

  // Switch to the second page
  ClickCentre(f, pTabs->GetTabBarArea().SubRect(EDirection::Horizontal, 2, 1));
  REQUIRE(pTabSwitch->GetSelectedIdx() == 1);
  REQUIRE(pPageA->IsHidden());
  REQUIRE(pKnobA->IsHidden());
  REQUIRE_FALSE(pPageB->IsHidden());
  REQUIRE_FALSE(pKnobB->IsHidden());

  const double gainBefore = f.mDlg.GetParam(kGain)->Value();
  f.mUI->Drag(knobBounds.MW(), knobBounds.MH(), knobBounds.MW(), knobBounds.MH() - 20.f);
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(gainBefore));
  REQUIRE(f.mDlg.GetParam(kLow)->Value() > 25.);

  // ... and back
  ClickCentre(f, pTabs->GetTabBarArea().SubRect(EDirection::Horizontal, 2, 0));
  REQUIRE_FALSE(pKnobA->IsHidden());
  REQUIRE(pKnobB->IsHidden());
}

TEST_CASE("IVTabbedPagesControl: hiding and showing the whole control keeps only the selected page visible", "[integration][controls][tabbedpages][container]")
{
  Fixture f;
  IControl* pKnobA = nullptr;
  IControl* pKnobB = nullptr;
  auto* pPageA = new IVTabPage([&](IVTabPage* pPage, const IRECT& bounds) { pKnobA = pPage->AddChildControl(new IVKnobControl(bounds, kGain, "", kPlainStyle)); });
  auto* pPageB = new IVTabPage([&](IVTabPage* pPage, const IRECT& bounds) { pKnobB = pPage->AddChildControl(new IVKnobControl(bounds, kLow, "", kPlainStyle)); });
  auto* pTabs = f.Attach(new IVTabbedPagesControl(IRECT(50, 50, 450, 350), {{"First", pPageA}, {"Second", pPageB}}, "", kPlainStyle));

  ClickCentre(f, pTabs->GetTabBarArea().SubRect(EDirection::Horizontal, 2, 1));

  pTabs->Hide(true);
  REQUIRE(pTabs->IsHidden());
  REQUIRE(pTabs->GetChild(0)->IsHidden());
  REQUIRE(pKnobA->IsHidden());
  REQUIRE(pKnobB->IsHidden());

  pTabs->Hide(false);
  REQUIRE_FALSE(pTabs->IsHidden());
  REQUIRE_FALSE(pTabs->GetChild(0)->IsHidden());
  REQUIRE(pKnobA->IsHidden());
  REQUIRE_FALSE(pKnobB->IsHidden());
}

TEST_CASE("IVTabbedPagesControl: resizing lays out the tab bar and the pages again", "[integration][controls][tabbedpages][resize]")
{
  Fixture f;
  IControl* pKnobA = nullptr;
  auto* pPageA = new IVTabPage([&](IVTabPage* pPage, const IRECT& bounds) { pKnobA = pPage->AddChildControl(new IVKnobControl(bounds, kGain, "", kPlainStyle)); });
  auto* pPageB = new IVTabPage([&](IVTabPage* pPage, const IRECT& bounds) { pPage->AddChildControl(new IVKnobControl(bounds, kLow, "", kPlainStyle)); });
  auto* pTabs = f.Attach(new IVTabbedPagesControl(IRECT(50, 50, 450, 350), {{"First", pPageA}, {"Second", pPageB}}, "", kPlainStyle));

  pTabs->SetTargetAndDrawRECTs(IRECT(0, 0, 200, 200));

  REQUIRE(pPageA->GetRECT() == pTabs->GetPageArea());
  REQUIRE(IRECT(0, 0, 200, 200).Contains(pPageA->GetRECT()));
  REQUIRE(pTabs->GetChild(0)->GetRECT() == pTabs->GetTabBarArea());
  // The default page layout pads its single child
  REQUIRE(pKnobA->GetRECT() == pPageA->GetRECT().GetPadded(-static_cast<float>(IVTabPage::kDefaultPadding)));
}

#pragma mark - IVPlotControl

TEST_CASE("IVPlotControl: plots its functions as lines inside the control", "[integration][controls][plot][drawing]")
{
  Fixture f;
  const IRECT bounds = IRECT(100, 100, 400, 300);
  int nEvaluations = 0;

  SECTION("drawn directly")
  {
    f.Attach(new IVPlotControl(bounds, {{COLOR_RED, [&](double x) { nEvaluations++; return 2. * x - 1.; }}}, 32, "", kPlainStyle, -1.f, 1.f, false));
    nEvaluations = 0;
    f.Redraw();

    REQUIRE(nEvaluations == 32);
    bool drewPlot = false;
    for (auto& op : f.mUI->mDrawOps)
      drewPlot |= (op.mType == DrawOp::EType::Stroke && op.mColor.R == 255 && op.mColor.G == 0 && op.mBounds.W() > 250.f && bounds.Contains(op.mBounds));
    REQUIRE(drewPlot);
  }

  SECTION("cached in a layer: the functions are only evaluated when something changes")
  {
    auto* pPlot = f.Attach(new IVPlotControl(bounds, {{COLOR_RED, [&](double x) { nEvaluations++; return x; }}}, 32, "", kPlainStyle, 0.f, 1.f, true));
    nEvaluations = 0;

    f.Redraw();
    REQUIRE(nEvaluations == 0);
    REQUIRE(f.mUI->CountOps(DrawOp::EType::Bitmap) == 1);

    pPlot->AddPlotFunc(COLOR_BLUE, [](double x) { return 1. - x; });
    REQUIRE(pPlot->IsDirty());
    f.mUI->ClearRecorded();
    f.mUI->RenderFrame();
    REQUIRE(nEvaluations == 32);
    REQUIRE(f.mUI->CountOps(DrawOp::EType::Bitmap) == 1);
  }
}
