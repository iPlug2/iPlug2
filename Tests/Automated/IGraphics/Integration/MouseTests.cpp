/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests how IGraphics dispatches mouse, touch, wheel, gesture and drag-and-drop events to controls

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
enum EParams { kGain = 0, kSteps, kNumParams };

const IRECT kBoundsA = IRECT(20, 20, 120, 120);
const IRECT kBoundsB = IRECT(200, 20, 300, 120);
const IRECT kKnobBounds = IRECT(20, 150, 120, 250);
const IRECT kStepKnobBounds = IRECT(150, 150, 250, 250);
const IVStyle kPlainStyle = DEFAULT_STYLE.WithShowLabel(false).WithShowValue(false);

/** A panel background, two probes side by side, and two knobs beneath them */
struct Fixture
{
  Fixture()
  : mDlg(kNumParams, 400, 300)
  {
    mDlg.GetParam(kGain)->InitDouble("Gain", 50., 0., 100., 0.01, "%");
    mDlg.GetParam(kSteps)->InitInt("Steps", 2, 0, 10);
    mUI = mDlg.OpenUI();
    mUI->AttachPanelBackground(COLOR_GRAY);
    mA = mUI->AttachControl(new ProbeControl(kBoundsA))->As<ProbeControl>();
    mB = mUI->AttachControl(new ProbeControl(kBoundsB))->As<ProbeControl>();
    mKnob = mUI->AttachControl(new IVKnobControl(kKnobBounds, kGain, "", kPlainStyle))->As<IVKnobControl>();
    mStepKnob = mUI->AttachControl(new IVKnobControl(kStepKnobBounds, kSteps, "", kPlainStyle))->As<IVKnobControl>();
    mDlg.SendCurrentParamValuesFromDelegate();
    mDlg.ClearRecorded();
    mUI->RenderUntilClean();
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
  ProbeControl* mA = nullptr;
  ProbeControl* mB = nullptr;
  IVKnobControl* mKnob = nullptr;
  IVKnobControl* mStepKnob = nullptr;
};

/** Drag the gain knob vertically from its centre and return the change in its value */
double DragKnob(Fixture& f, IControl* pKnob, float dX, float dY, const IMouseMod& mod = IMouseMod(true))
{
  const double before = pKnob->GetValue();
  const float x = pKnob->GetRECT().MW();
  const float y = pKnob->GetRECT().MH();
  f.mUI->Drag(x, y, x + dX, y + dY, 4, mod);
  return pKnob->GetValue() - before;
}

/** Only handles single drops, so that the default OnDropMultiple() is used */
class SingleDropControl : public IControl
{
public:
  SingleDropControl(const IRECT& bounds) : IControl(bounds) {}
  void Draw(IGraphics& g) override {}
  void OnDrop(const char* str) override { mDropped.push_back(str); }
  std::vector<std::string> mDropped;
};
} // namespace

#pragma mark - Capture

TEST_CASE("Mouse: down, drag and up are delivered to the control under the mouse down", "[integration][mouse]")
{
  Fixture f;

  f.mUI->MouseDown(50, 60);
  REQUIRE(f.mA->Names() == "down");
  REQUIRE(f.mA->Last("down")->mX == Approx(50.f));
  REQUIRE(f.mA->Last("down")->mY == Approx(60.f));
  REQUIRE(f.mA->Last("down")->mMod.L);
  REQUIRE(f.mUI->ControlIsCaptured());
  REQUIRE(f.mUI->ControlIsCaptured(f.mA));
  REQUIRE_FALSE(f.mUI->ControlIsCaptured(f.mB));

  float downX = 0.f, downY = 0.f;
  f.mUI->GetMouseDownPoint(downX, downY);
  REQUIRE(downX == Approx(50.f));
  REQUIRE(downY == Approx(60.f));

  f.mUI->MouseDragTo(70, 65);
  REQUIRE(f.mA->Names() == "down drag");
  REQUIRE(f.mA->Last("drag")->mX == Approx(70.f));
  REQUIRE(f.mA->Last("drag")->mDX == Approx(20.f));
  REQUIRE(f.mA->Last("drag")->mDY == Approx(5.f));

  f.mUI->MouseUp(70, 65);
  REQUIRE(f.mA->Names() == "down drag up");
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
  REQUIRE(f.mB->mEvents.empty());
}

TEST_CASE("Mouse: a captured control keeps receiving the drag outside its bounds, until the button is released", "[integration][mouse]")
{
  Fixture f;

  f.mUI->MouseDown(50, 50);
  f.mUI->MouseDragTo(250, 50);  // over control B
  f.mUI->MouseDragTo(390, 290); // over nothing
  f.mUI->MouseDragTo(-20, -20); // outside the window

  REQUIRE(f.mA->Count("drag") == 3);
  REQUIRE(f.mA->Last("drag")->mX == Approx(-20.f));
  REQUIRE(f.mB->mEvents.empty());

  f.mUI->MouseUp(250, 50);
  REQUIRE(f.mA->Count("up") == 1);
  REQUIRE(f.mA->Last("up")->mX == Approx(250.f));
  REQUIRE(f.mB->Count("up") == 0);
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());

  // With the capture gone, a drag goes nowhere
  f.mUI->MouseDragTo(260, 60);
  REQUIRE(f.mA->Count("drag") == 3);
  REQUIRE(f.mB->Count("drag") == 0);
}

TEST_CASE("Mouse: a drag event with no movement is not delivered", "[integration][mouse]")
{
  Fixture f;

  f.mUI->MouseDown(50, 50);
  f.mUI->MouseDragTo(50, 50);
  REQUIRE(f.mA->Count("drag") == 0);
  f.mUI->MouseDragTo(50, 51);
  REQUIRE(f.mA->Count("drag") == 1);
  f.mUI->MouseUp(50, 51);
}

TEST_CASE("Mouse: events with no control under them are ignored", "[integration][mouse]")
{
  Fixture f;

  // The panel background ignores the mouse
  f.mUI->Click(350, 280);
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
  f.mUI->MouseDown(350, 280);
  f.mUI->MouseDragTo(50, 50);
  f.mUI->MouseUp(50, 50);

  REQUIRE(f.mA->Count("down") == 0);
  REQUIRE(f.mA->Count("drag") == 0);
  REQUIRE(f.mA->Count("up") == 0);
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("Mouse: an up with no preceding down is ignored", "[integration][mouse]")
{
  Fixture f;

  f.mUI->MouseUp(50, 50);
  REQUIRE(f.mA->Count("up") == 0);
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("Mouse: ReleaseMouseCapture ends the gesture for the captured control", "[integration][mouse]")
{
  Fixture f;

  f.mUI->MouseDown(50, 50);
  f.mUI->MouseDragTo(60, 60);
  f.mUI->ReleaseMouseCapture();
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());

  f.mUI->MouseDragTo(70, 70);
  REQUIRE(f.mA->Count("drag") == 1);
  f.mUI->MouseUp(70, 70);
  REQUIRE(f.mA->Count("up") == 0);
}

TEST_CASE("Mouse: hidden, disabled and mouse-ignoring controls receive no events", "[integration][mouse]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);

  SECTION("hidden")
  {
    f.mA->Hide(true);
  }

  SECTION("disabled")
  {
    f.mA->SetDisabled(true);
  }

  SECTION("ignoring the mouse")
  {
    f.mA->SetIgnoreMouse(true);
  }

  f.mUI->MouseMove(50, 50);
  f.mUI->Click(50, 50);
  f.mUI->Drag(50, 50, 60, 60);
  f.mUI->DblClick(50, 50);
  f.mUI->Wheel(50, 50, 1.f);
  f.mUI->KeyDown('A', "a");
  f.mUI->OnDrop("file.txt", 50, 50);

  REQUIRE(f.mA->mEvents.empty());
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
}

#pragma mark - Mouse over

TEST_CASE("Mouse over: nothing is delivered unless mouse-overs are enabled", "[integration][mouse][mouseover]")
{
  Fixture f;

  REQUIRE_FALSE(f.mUI->MouseOverEnabled());
  REQUIRE_FALSE(f.mUI->MouseMove(50, 50));
  REQUIRE_FALSE(f.mUI->MouseMove(250, 50));
  f.mUI->OnMouseOut();

  REQUIRE(f.mA->Count("over") == 0);
  REQUIRE(f.mB->Count("over") == 0);
  REQUIRE_FALSE(f.mA->GetMouseIsOver());
  REQUIRE(f.mUI->GetMouseOver() == -1);
}

TEST_CASE("Mouse over: OnMouseOver/OnMouseOut follow the pointer from control to control", "[integration][mouse][mouseover]")
{
  Fixture f;
  f.mA->mLogMouseOut = f.mB->mLogMouseOut = true;
  f.mUI->EnableMouseOver(true);
  REQUIRE(f.mUI->MouseOverEnabled());

  REQUIRE(f.mUI->MouseMove(50, 50));
  REQUIRE(f.mA->Names() == "over");
  REQUIRE(f.mA->GetMouseIsOver());
  REQUIRE(f.mUI->GetMouseOver() == f.mUI->GetControlIdx(f.mA));
  REQUIRE(f.mA->IsDirty());

  // Moving within the control keeps reporting the position, with no out in between
  REQUIRE(f.mUI->MouseMove(60, 55));
  REQUIRE(f.mA->Names() == "over over");
  REQUIRE(f.mA->Last("over")->mX == Approx(60.f));
  REQUIRE(f.mA->Last("over")->mY == Approx(55.f));

  // Onto the other control
  REQUIRE(f.mUI->MouseMove(250, 50));
  REQUIRE(f.mA->Names() == "over over out");
  REQUIRE_FALSE(f.mA->GetMouseIsOver());
  REQUIRE(f.mB->Names() == "over");
  REQUIRE(f.mB->GetMouseIsOver());
  REQUIRE(f.mUI->GetMouseOver() == f.mUI->GetControlIdx(f.mB));

  // Onto empty space
  REQUIRE_FALSE(f.mUI->MouseMove(390, 290));
  REQUIRE(f.mB->Names() == "over out");
  REQUIRE_FALSE(f.mB->GetMouseIsOver());
  REQUIRE(f.mUI->GetMouseOver() == -1);
}

TEST_CASE("Mouse over: the pointer leaving the window sends OnMouseOut", "[integration][mouse][mouseover]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);

  f.mUI->MouseMove(50, 50);
  REQUIRE(f.mA->GetMouseIsOver());

  f.mUI->OnMouseOut();
  REQUIRE_FALSE(f.mA->GetMouseIsOver());
  REQUIRE(f.mA->Count("out") >= 1);
  REQUIRE(f.mUI->GetMouseOver() == -1);

  // Coming back in is a new mouse over
  f.mA->ClearEvents();
  f.mUI->MouseMove(50, 50);
  REQUIRE(f.mA->Names() == "over");
  REQUIRE(f.mA->GetMouseIsOver());
}

TEST_CASE("Mouse over: a mouse-over state change redraws the control", "[integration][mouse][mouseover][drawing]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);
  const int nDraws = f.mA->mNumDraws;

  f.mUI->MouseMove(50, 50);
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mA->mNumDraws == nDraws + 1);

  // Still over: nothing changed, nothing to redraw
  f.mUI->MouseMove(55, 55);
  REQUIRE_FALSE(f.mUI->RenderFrame());

  f.mUI->MouseMove(390, 290);
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mA->mNumDraws == nDraws + 2);
}

TEST_CASE("Mouse over: disabled controls only get mouse-overs if they ask for them", "[integration][mouse][mouseover]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);
  f.mA->SetDisabled(true);

  REQUIRE_FALSE(f.mUI->MouseMove(50, 50));
  REQUIRE(f.mA->Count("over") == 0);

  f.mA->SetMouseOverWhenDisabled(true);
  REQUIRE(f.mA->GetMouseOverWhenDisabled());
  REQUIRE(f.mUI->MouseMove(51, 51));
  REQUIRE(f.mA->Count("over") == 1);
}

TEST_CASE("Mouse over: releasing the button reports the control that is now under the pointer", "[integration][mouse][mouseover]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);

  f.mUI->MouseDown(50, 50);
  f.mUI->MouseDragTo(250, 50);
  REQUIRE(f.mB->Count("over") == 0);
  f.mUI->MouseUp(250, 50);

  REQUIRE(f.mB->Count("over") == 1);
  REQUIRE(f.mB->GetMouseIsOver());
}

TEST_CASE("Mouse over: EnableTooltips turns mouse-overs on", "[integration][mouse][mouseover][tooltips]")
{
  Fixture f;

  REQUIRE_FALSE(f.mUI->TooltipsEnabled());
  f.mUI->EnableTooltips(true);
  REQUIRE(f.mUI->TooltipsEnabled());
  REQUIRE(f.mUI->MouseOverEnabled());

  f.mA->SetTooltip("Probe A");
  REQUIRE(std::string(f.mA->GetTooltip()) == "Probe A");
  REQUIRE(f.mUI->MouseMove(50, 50));
  REQUIRE(std::string(f.mUI->GetControl(f.mUI->GetMouseOver())->GetTooltip()) == "Probe A");

  f.mUI->EnableTooltips(false);
  REQUIRE_FALSE(f.mUI->TooltipsEnabled());
}

TEST_CASE("Mouse over: IURLControl shows the hand cursor while the pointer is over it", "[integration][mouse][mouseover][cursor]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);
  f.mUI->AttachControl(new IURLControl(IRECT(300, 200, 390, 220), "link", "https://example.com"));

  REQUIRE(f.mUI->SetMouseCursor(ECursor::ARROW) == ECursor::ARROW);
  f.mUI->MouseMove(340, 210);
  REQUIRE(f.mUI->SetMouseCursor(ECursor::HAND) == ECursor::HAND);
  f.mUI->MouseMove(390, 290);
  REQUIRE(f.mUI->SetMouseCursor(ECursor::ARROW) == ECursor::ARROW);
}

#pragma mark - Double click

TEST_CASE("Double click: is delivered as OnMouseDblClick and does not leave the control captured", "[integration][mouse][dblclick]")
{
  Fixture f;

  REQUIRE(f.mUI->DblClick(50, 50));
  REQUIRE(f.mA->Names() == "dblclick");
  REQUIRE(f.mA->Last("dblclick")->mX == Approx(50.f));
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());

  // Nothing there
  REQUIRE_FALSE(f.mUI->DblClick(390, 290));
}

TEST_CASE("Double click: a control can ask for double clicks to be delivered as single clicks", "[integration][mouse][dblclick]")
{
  Fixture f;
  f.mA->SetDblAsSingleClick(true);
  REQUIRE(f.mA->GetMouseDblAsSingleClick());

  REQUIRE(f.mUI->DblClick(50, 50));
  REQUIRE(f.mA->Names() == "down");
  REQUIRE(f.mUI->ControlIsCaptured(f.mA));

  f.mUI->MouseUp(50, 50);
  REQUIRE(f.mA->Names() == "down up");
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
}

TEST_CASE("Double click: resets a knob to the parameter's default value and tells the host", "[integration][mouse][dblclick][params]")
{
  Fixture f;
  f.mDlg.SendParameterValueFromDelegate(kGain, 0.9, true);
  REQUIRE(f.mKnob->GetValue() == Approx(0.9));

  REQUIRE(f.mUI->DblClick(kKnobBounds.MW(), kKnobBounds.MH()));

  REQUIRE(f.mKnob->GetValue() == Approx(0.5));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(50.));
  REQUIRE(f.mDlg.LastParamValue(kGain) == Approx(0.5));
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
  REQUIRE_FALSE(f.mUI->mCursorHidden);
}

TEST_CASE("Double click: resets a slider to the parameter's default value", "[integration][mouse][dblclick][params]")
{
  Fixture f;
  auto* pSlider = f.mUI->AttachControl(new IVSliderControl(IRECT(300, 150, 340, 290), kGain, "", kPlainStyle));
  f.mDlg.SendParameterValueFromDelegate(kGain, 0.1, true);
  REQUIRE(pSlider->GetValue() == Approx(0.1));

  f.mUI->DblClick(320, 220);
  REQUIRE(pSlider->GetValue() == Approx(0.5));
  // The knob that shows the same parameter follows
  REQUIRE(f.mKnob->GetValue() == Approx(0.5));
}

#pragma mark - Wheel

TEST_CASE("Mouse wheel: is delivered to the control under the pointer, without capturing it", "[integration][mouse][wheel]")
{
  Fixture f;

  REQUIRE(f.mUI->Wheel(50, 50, 1.f));
  REQUIRE(f.mUI->Wheel(50, 50, -2.f, IMouseMod(false, false, true)));

  REQUIRE(f.mA->Names() == "wheel wheel");
  REQUIRE(f.mA->mEvents[0].mDX == Approx(1.f));
  REQUIRE(f.mA->mEvents[1].mDX == Approx(-2.f));
  REQUIRE(f.mA->mEvents[1].mMod.S);
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());

  REQUIRE_FALSE(f.mUI->Wheel(390, 290, 1.f));
}

TEST_CASE("Mouse wheel: goes to the captured control while a drag is in progress", "[integration][mouse][wheel]")
{
  Fixture f;

  f.mUI->MouseDown(50, 50);
  REQUIRE(f.mUI->Wheel(250, 50, 1.f));
  REQUIRE(f.mA->Count("wheel") == 1);
  REQUIRE(f.mB->Count("wheel") == 0);
  f.mUI->MouseUp(50, 50);
}

TEST_CASE("Mouse wheel: moves a knob in the direction of the wheel, with finer steps when a modifier is held", "[integration][mouse][wheel][knob]")
{
  Fixture f;
  const float x = kKnobBounds.MW(), y = kKnobBounds.MH();

  f.mUI->Wheel(x, y, 1.f);
  const double coarseStep = f.mKnob->GetValue() - 0.5;
  REQUIRE(coarseStep > 0.);
  REQUIRE(f.mDlg.GetParam(kGain)->GetNormalized() == Approx(0.5 + coarseStep));
  REQUIRE(f.mDlg.LastParamValue(kGain) == Approx(0.5 + coarseStep));

  f.mUI->Wheel(x, y, -1.f);
  REQUIRE(f.mKnob->GetValue() == Approx(0.5));

  // A bigger wheel movement moves the knob further
  f.mUI->Wheel(x, y, 3.f);
  REQUIRE(f.mKnob->GetValue() == Approx(0.5 + 3. * coarseStep));
  f.mUI->Wheel(x, y, -3.f);

  SECTION("shift")
  {
    f.mUI->Wheel(x, y, 1.f, IMouseMod(false, false, true));
  }

  SECTION("ctrl/cmd")
  {
    f.mUI->Wheel(x, y, 1.f, IMouseMod(false, false, false, true));
  }

  const double fineStep = f.mKnob->GetValue() - 0.5;
  REQUIRE(fineStep > 0.);
  REQUIRE(fineStep == Approx(coarseStep / 10.));
}

TEST_CASE("Mouse wheel: a knob stops at the ends of its range", "[integration][mouse][wheel][knob]")
{
  Fixture f;
  const float x = kKnobBounds.MW(), y = kKnobBounds.MH();

  f.mUI->Wheel(x, y, 1000.f);
  REQUIRE(f.mKnob->GetValue() == Approx(1.));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(100.));

  f.mUI->Wheel(x, y, -5000.f);
  REQUIRE(f.mKnob->GetValue() == Approx(0.));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(0.));
}

TEST_CASE("Mouse wheel: a knob linked to a stepped parameter moves one step per wheel event", "[integration][mouse][wheel][knob][params]")
{
  Fixture f;
  const float x = kStepKnobBounds.MW(), y = kStepKnobBounds.MH();
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 2);

  f.mUI->Wheel(x, y, 1.f);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 3);
  f.mUI->Wheel(x, y, 0.25f);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 4);
  f.mUI->Wheel(x, y, -1.f, IMouseMod(false, false, true));
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 3);
  REQUIRE(f.mStepKnob->GetValue() == Approx(0.3));

  for (int i = 0; i < 20; i++)
    f.mUI->Wheel(x, y, 1.f);

  REQUIRE(f.mDlg.GetParam(kSteps)->Int() == 10);
  REQUIRE(f.mStepKnob->GetValue() == Approx(1.));
}

#pragma mark - Modifiers

TEST_CASE("Knob drag: up increases the value, down decreases it, sideways does nothing", "[integration][mouse][knob]")
{
  Fixture f;

  const double up = DragKnob(f, f.mKnob, 0.f, -20.f);
  REQUIRE(up > 0.);
  const double down = DragKnob(f, f.mKnob, 0.f, 20.f);
  REQUIRE(down == Approx(-up));
  REQUIRE(DragKnob(f, f.mKnob, 30.f, 0.f) == Approx(0.).margin(1e-9));
  REQUIRE(f.mDlg.GetParam(kGain)->GetNormalized() == Approx(f.mKnob->GetValue()));
}

TEST_CASE("Knob drag: the distance dragged is proportional to the change, whatever the number of mouse events", "[integration][mouse][knob]")
{
  Fixture f;
  const float x = kKnobBounds.MW(), y = kKnobBounds.MH();

  f.mUI->Drag(x, y, x, y - 10.f, 1);
  const double oneStep = f.mKnob->GetValue() - 0.5;
  f.mUI->Drag(x, y, x, y + 10.f, 1);
  REQUIRE(f.mKnob->GetValue() == Approx(0.5));

  f.mUI->Drag(x, y, x, y - 10.f, 10);
  REQUIRE(f.mKnob->GetValue() - 0.5 == Approx(oneStep));

  f.mUI->Drag(x, y, x, y - 10.f, 5);
  REQUIRE(f.mKnob->GetValue() - 0.5 == Approx(2. * oneStep));
}

TEST_CASE("Knob drag: holding shift or ctrl/cmd gives fine control", "[integration][mouse][knob][modifiers]")
{
  Fixture f;
  const double coarse = DragKnob(f, f.mKnob, 0.f, -20.f);
  DragKnob(f, f.mKnob, 0.f, 20.f);

  double fine = 0.;

  SECTION("shift")
  {
    fine = DragKnob(f, f.mKnob, 0.f, -20.f, IMouseMod(true, false, true));
  }

  SECTION("ctrl/cmd")
  {
    fine = DragKnob(f, f.mKnob, 0.f, -20.f, IMouseMod(true, false, false, true));
  }

  REQUIRE(fine > 0.);
  REQUIRE(fine == Approx(coarse / 10.));
}

TEST_CASE("Knob drag: a higher gearing makes the knob less sensitive", "[integration][mouse][knob]")
{
  Fixture f;
  const double normal = DragKnob(f, f.mKnob, 0.f, -20.f);
  DragKnob(f, f.mKnob, 0.f, 20.f);

  f.mKnob->SetGearing(DEFAULT_GEARING * 2.);
  REQUIRE(DragKnob(f, f.mKnob, 0.f, -20.f) == Approx(normal / 2.));
}

TEST_CASE("Knob drag: a horizontal knob responds to horizontal movement", "[integration][mouse][knob]")
{
  Fixture f;
  auto* pKnob = f.mUI->AttachControl(new IVKnobControl(IRECT(280, 150, 380, 250), kGain, "", kPlainStyle, false, false, -135.f, 135.f, -135.f, EDirection::Horizontal));
  f.mDlg.SendCurrentParamValuesFromDelegate();

  REQUIRE(DragKnob(f, pKnob, 20.f, 0.f) > 0.);
  REQUIRE(DragKnob(f, pKnob, -40.f, 0.f) < 0.);
  REQUIRE(DragKnob(f, pKnob, 0.f, -30.f) == Approx(0.).margin(1e-9));
}

TEST_CASE("Knob drag: the value is clamped while the drag continues past the end of the range", "[integration][mouse][knob]")
{
  Fixture f;
  const float x = kKnobBounds.MW(), y = kKnobBounds.MH();

  f.mUI->MouseDown(x, y);
  f.mUI->MouseDragTo(x, y - 5000.f);
  REQUIRE(f.mKnob->GetValue() == Approx(1.));
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(100.));

  // Coming back takes effect immediately: the overshoot is not remembered
  f.mUI->MouseDragTo(x, y - 4980.f);
  REQUIRE(f.mKnob->GetValue() < 1.);
  f.mUI->MouseDragTo(x, y + 10000.f);
  REQUIRE(f.mKnob->GetValue() == Approx(0.));
  f.mUI->MouseUp(x, y);
}

TEST_CASE("Knob drag: a knob linked to a stepped parameter only takes values on the steps", "[integration][mouse][knob][params]")
{
  Fixture f;
  const float x = kStepKnobBounds.MW(), y = kStepKnobBounds.MH();

  f.mUI->MouseDown(x, y);

  for (int i = 1; i <= 60; i++)
  {
    f.mUI->MouseDragTo(x, y - static_cast<float>(i));
    const double stepped = f.mStepKnob->GetValue() * 10.;
    REQUIRE(stepped == Approx(std::round(stepped)).margin(1e-9));
  }

  f.mUI->MouseUp(x, y - 60.f);
  REQUIRE(f.mDlg.GetParam(kSteps)->Int() > 2);
  REQUIRE(f.mDlg.GetParam(kSteps)->Value() == Approx(f.mStepKnob->GetValue() * 10.));
}

#pragma mark - Cursor

TEST_CASE("Cursor: hidden and locked while a knob is dragged, restored on release", "[integration][mouse][cursor][knob]")
{
  Fixture f;
  const float x = kKnobBounds.MW(), y = kKnobBounds.MH();

  REQUIRE_FALSE(f.mUI->mCursorHidden);
  f.mUI->MouseDown(x, y);
  REQUIRE(f.mUI->mCursorHidden);
  REQUIRE(f.mUI->mCursorLocked);
  f.mUI->MouseDragTo(x, y - 30.f);
  REQUIRE(f.mUI->mCursorHidden);
  f.mUI->MouseUp(x, y - 30.f);
  REQUIRE_FALSE(f.mUI->mCursorHidden);
  REQUIRE_FALSE(f.mUI->mCursorLocked);
  REQUIRE(f.mUI->mNumCursorHides == 1);
}

TEST_CASE("Cursor: stays visible if the control's style says not to hide it", "[integration][mouse][cursor][knob]")
{
  Fixture f;
  f.mUI->AttachControl(new IVKnobControl(IRECT(280, 150, 380, 250), kGain, "", kPlainStyle.WithHideCursor(false)));

  f.mUI->MouseDown(330, 200);
  REQUIRE_FALSE(f.mUI->mCursorHidden);
  f.mUI->MouseDragTo(330, 180);
  REQUIRE_FALSE(f.mUI->mCursorHidden);
  f.mUI->MouseUp(330, 180);
  REQUIRE(f.mUI->mNumCursorHides == 0);
}

TEST_CASE("Cursor: hidden while a slider is dragged, restored on release", "[integration][mouse][cursor][slider]")
{
  Fixture f;
  f.mUI->AttachControl(new IVSliderControl(IRECT(300, 150, 340, 290), kGain, "", kPlainStyle));

  f.mUI->MouseDown(320, 220);
  REQUIRE(f.mUI->mCursorHidden);
  f.mUI->MouseUp(320, 220);
  REQUIRE_FALSE(f.mUI->mCursorHidden);
}

TEST_CASE("Cursor: ReleaseMouseCapture shows a cursor that a control had hidden", "[integration][mouse][cursor]")
{
  Fixture f;

  f.mUI->MouseDown(kKnobBounds.MW(), kKnobBounds.MH());
  REQUIRE(f.mUI->mCursorHidden);
  f.mUI->ReleaseMouseCapture();
  REQUIRE_FALSE(f.mUI->mCursorHidden);
}

TEST_CASE("Cursor: removing a control mid-drag shows the cursor again", "[integration][mouse][cursor][remove]")
{
  Fixture f;

  f.mUI->MouseDown(kKnobBounds.MW(), kKnobBounds.MH());
  REQUIRE(f.mUI->mCursorHidden);
  f.mUI->RemoveControl(f.mKnob);
  REQUIRE_FALSE(f.mUI->mCursorHidden);
}

#pragma mark - Context menu

TEST_CASE("Right click: on a control with a parameter opens the control's context menu instead of a mouse down", "[integration][mouse][contextmenu]")
{
  Fixture f;
  auto* pProbe = f.mUI->AttachControl(new ProbeControl(IRECT(300, 200, 380, 280), kGain))->As<ProbeControl>();
  pProbe->mContextMenuItems = {"Learn", "Clear", "Reset"};

  SECTION("an item is chosen")
  {
    f.mUI->mPopupMenuChoice = 1;
    f.mUI->MouseDown(340, 240, IMouseMod(false, true));

    REQUIRE(f.mUI->mNumPopupMenus == 1);
    REQUIRE(f.mUI->mPopupMenuItems == std::vector<std::string>{"Learn", "Clear", "Reset"});
    REQUIRE(f.mUI->mPopupMenuBounds.L == Approx(340.f));
    REQUIRE(f.mUI->mPopupMenuBounds.T == Approx(240.f));
    REQUIRE(pProbe->mNumContextSelections == 1);
    REQUIRE(pProbe->mContextSelection == 1);
  }

  SECTION("the menu is dismissed")
  {
    f.mUI->mPopupMenuChoice = -1;
    f.mUI->MouseDown(340, 240, IMouseMod(false, true));

    REQUIRE(f.mUI->mNumPopupMenus == 1);
    REQUIRE(pProbe->mNumContextSelections == 1);
    REQUIRE(pProbe->mContextSelection == -1);
  }

  REQUIRE(pProbe->Count("down") == 0);
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
  REQUIRE(f.mDlg.CountParamEvents(TestEditorDelegate::ParamEvent::EType::Begin) == 0);
  REQUIRE(f.mDlg.CountParamEvents(TestEditorDelegate::ParamEvent::EType::Value) == 0);
  // It is not a parameter pop-up, so the parameter is untouched
  REQUIRE(f.mDlg.GetParam(kGain)->Value() == Approx(50.));

  f.mUI->MouseUp(340, 240, IMouseMod(false, true));
  REQUIRE(pProbe->Count("up") == 0);
}

TEST_CASE("Right click: a control with a parameter but no context menu items shows no menu", "[integration][mouse][contextmenu]")
{
  Fixture f;
  auto* pProbe = f.mUI->AttachControl(new ProbeControl(IRECT(300, 200, 380, 280), kGain))->As<ProbeControl>();

  f.mUI->Click(340, 240, IMouseMod(false, true));

  REQUIRE(f.mUI->mNumPopupMenus == 0);
  REQUIRE(pProbe->mNumContextSelections == 0);
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
  REQUIRE(f.mDlg.mParamEvents.empty());
}

TEST_CASE("Right click: a control without a parameter gets the mouse down with the right button flag", "[integration][mouse][contextmenu]")
{
  Fixture f;

  f.mUI->Click(50, 50, IMouseMod(false, true));

  REQUIRE(f.mA->Names() == "down up");
  REQUIRE(f.mA->Last("down")->mMod.R);
  REQUIRE_FALSE(f.mA->Last("down")->mMod.L);
  REQUIRE(f.mUI->mNumPopupMenus == 0);
}

// BUG: for a right-click context menu IGraphics::OnMouseDown() (IGraphics/IGraphics.cpp:1129) returns before calling
// BeginInformHostOfParamChangeFromUI(), but IGraphics::SetControlValueAfterPopupMenu() (IGraphics/IGraphics.cpp:292)
// then calls EndInformHostOfParamChangeFromUI() for each of the control's parameters, so the host gets an "end
// gesture" that was never begun.
TEST_CASE("Right click: the context menu does not send the host an unbalanced end-of-gesture", "[.][bug]")
{
  Fixture f;
  auto* pProbe = f.mUI->AttachControl(new ProbeControl(IRECT(300, 200, 380, 280), kGain))->As<ProbeControl>();
  pProbe->mContextMenuItems = {"Learn"};
  f.mUI->mPopupMenuChoice = 0;

  f.mUI->Click(340, 240, IMouseMod(false, true));

  REQUIRE(pProbe->mContextSelection == 0);
  REQUIRE(f.mDlg.CountParamEvents(TestEditorDelegate::ParamEvent::EType::End) == f.mDlg.CountParamEvents(TestEditorDelegate::ParamEvent::EType::Begin));
}

#pragma mark - Multi-touch

TEST_CASE("Multi-touch: can only be enabled where the platform supports it", "[integration][mouse][multitouch]")
{
  Fixture f;

  REQUIRE_FALSE(f.mUI->EnableMultiTouch(true));
  REQUIRE_FALSE(f.mUI->MultiTouchEnabled());

  f.mUI->mSupportsMultiTouch = true;
  REQUIRE(f.mUI->EnableMultiTouch(true));
  REQUIRE(f.mUI->MultiTouchEnabled());
  REQUIRE(f.mUI->EnableMultiTouch(false));
  REQUIRE_FALSE(f.mUI->MultiTouchEnabled());
}

TEST_CASE("Multi-touch: simultaneous touches are routed to their own controls by touch ID", "[integration][mouse][multitouch]")
{
  Fixture f;
  f.mUI->mSupportsMultiTouch = true;
  f.mUI->EnableMultiTouch(true);

  const IMouseMod touch1(true, false, false, false, false, 1);
  const IMouseMod touch2(true, false, false, false, false, 2);
  using G = IGraphicsHeadless;

  f.mUI->OnMouseDown({G::MakeMouseInfo(50, 50, 0, 0, touch1), G::MakeMouseInfo(250, 50, 0, 0, touch2)});
  REQUIRE(f.mA->Names() == "down");
  REQUIRE(f.mB->Names() == "down");
  REQUIRE(f.mA->Last("down")->mMod.touchID == 1);
  REQUIRE(f.mB->Last("down")->mMod.touchID == 2);
  REQUIRE(f.mUI->ControlIsCaptured(f.mA));
  REQUIRE(f.mUI->ControlIsCaptured(f.mB));

  std::vector<ITouchID> touches;
  f.mUI->GetTouches(f.mB, touches);
  REQUIRE(touches == std::vector<ITouchID>{2});

  // The touches cross over, but stay with the controls they started on
  f.mUI->OnMouseDrag({G::MakeMouseInfo(250, 60, 200, 10, touch1), G::MakeMouseInfo(50, 60, -200, 10, touch2)});
  REQUIRE(f.mA->Count("drag") == 1);
  REQUIRE(f.mB->Count("drag") == 1);
  REQUIRE(f.mA->Last("drag")->mX == Approx(250.f));
  REQUIRE(f.mB->Last("drag")->mX == Approx(50.f));

  f.mUI->OnMouseUp({G::MakeMouseInfo(250, 60, 0, 0, touch1)});
  REQUIRE(f.mA->Count("up") == 1);
  REQUIRE(f.mB->Count("up") == 0);
  REQUIRE_FALSE(f.mUI->ControlIsCaptured(f.mA));
  REQUIRE(f.mUI->ControlIsCaptured(f.mB));

  f.mUI->OnMouseDrag({G::MakeMouseInfo(60, 70, 10, 10, touch2)});
  REQUIRE(f.mB->Count("drag") == 2);

  f.mUI->OnMouseUp({G::MakeMouseInfo(60, 70, 0, 0, touch2)});
  REQUIRE(f.mB->Count("up") == 1);
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
}

TEST_CASE("Multi-touch: a second touch on a control needs the control to want multi-touch", "[integration][mouse][multitouch]")
{
  Fixture f;
  f.mUI->mSupportsMultiTouch = true;
  f.mUI->EnableMultiTouch(true);

  const IMouseMod touch1(true, false, false, false, false, 1);
  const IMouseMod touch2(true, false, false, false, false, 2);
  using G = IGraphicsHeadless;

  SECTION("by default the second touch is ignored")
  {
    REQUIRE_FALSE(f.mA->GetWantsMultiTouch());
    f.mUI->OnMouseDown({G::MakeMouseInfo(50, 50, 0, 0, touch1)});
    f.mUI->OnMouseDown({G::MakeMouseInfo(70, 70, 0, 0, touch2)});
    REQUIRE(f.mA->Count("down") == 1);

    std::vector<ITouchID> touches;
    f.mUI->GetTouches(f.mA, touches);
    REQUIRE(touches.size() == 1);
  }

  SECTION("a multi-touch control gets both")
  {
    f.mA->SetWantsMultiTouch(true);
    f.mUI->OnMouseDown({G::MakeMouseInfo(50, 50, 0, 0, touch1)});
    f.mUI->OnMouseDown({G::MakeMouseInfo(70, 70, 0, 0, touch2)});
    REQUIRE(f.mA->Count("down") == 2);

    std::vector<ITouchID> touches;
    f.mUI->GetTouches(f.mA, touches);
    REQUIRE(touches.size() == 2);

    f.mUI->OnMouseUp({G::MakeMouseInfo(50, 50, 0, 0, touch1)});
    REQUIRE(f.mUI->ControlIsCaptured(f.mA));
    f.mUI->OnMouseUp({G::MakeMouseInfo(70, 70, 0, 0, touch2)});
    REQUIRE_FALSE(f.mUI->ControlIsCaptured(f.mA));
    REQUIRE(f.mA->Count("up") == 2);
  }
}

TEST_CASE("Multi-touch: a cancelled touch is reported to the control and releases it", "[integration][mouse][multitouch]")
{
  Fixture f;
  const IMouseMod touch1(true, false, false, false, false, 1);
  using G = IGraphicsHeadless;

  f.mUI->OnMouseDown({G::MakeMouseInfo(50, 50, 0, 0, touch1)});
  REQUIRE(f.mUI->ControlIsCaptured(f.mA));

  // A cancel for a touch that is not in progress is ignored
  f.mUI->OnTouchCancelled({G::MakeMouseInfo(50, 50, 0, 0, IMouseMod(true, false, false, false, false, 7))});
  REQUIRE(f.mA->Count("cancel") == 0);

  f.mUI->OnTouchCancelled({G::MakeMouseInfo(55, 55, 0, 0, touch1)});
  REQUIRE(f.mA->Names() == "down cancel");
  REQUIRE_FALSE(f.mUI->ControlIsCaptured());
}

TEST_CASE("Multi-touch: lifting a touch does not generate a mouse over", "[integration][mouse][multitouch][mouseover]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);
  const IMouseMod touch1(true, false, false, false, false, 1);
  using G = IGraphicsHeadless;

  f.mUI->OnMouseDown({G::MakeMouseInfo(50, 50, 0, 0, touch1)});
  f.mUI->OnMouseUp({G::MakeMouseInfo(50, 50, 0, 0, touch1)});

  REQUIRE(f.mA->Names() == "down up");
  REQUIRE_FALSE(f.mA->GetMouseIsOver());
}

#pragma mark - Drag and drop

TEST_CASE("Drag and drop: a dropped file is delivered to the control under the drop point", "[integration][mouse][drop]")
{
  Fixture f;

  f.mUI->OnDrop("/path/to/file.wav", 250, 50);
  REQUIRE(f.mB->Names() == "drop");
  REQUIRE(f.mB->Last("drop")->mStr == "/path/to/file.wav");
  REQUIRE(f.mA->mEvents.empty());

  // Dropping on nothing is ignored
  f.mUI->OnDrop("/path/to/file.wav", 390, 290);
  REQUIRE(f.mB->Count("drop") == 1);
}

TEST_CASE("Drag and drop: several files dropped at once are delivered together", "[integration][mouse][drop]")
{
  Fixture f;
  const std::vector<const char*> paths = {"a.wav", "b.wav", "c.wav"};

  f.mUI->OnDropMultiple(paths, 50, 50);
  REQUIRE(f.mA->Names() == "dropmultiple");
  REQUIRE(f.mA->Last("dropmultiple")->mStr == "a.wav;b.wav;c.wav;");
  REQUIRE(f.mB->mEvents.empty());

  f.mUI->OnDropMultiple(paths, 390, 290);
  REQUIRE(f.mA->Count("dropmultiple") == 1);
}

TEST_CASE("Drag and drop: a control that only handles single drops gets the first of several files", "[integration][mouse][drop]")
{
  Fixture f;
  auto* pControl = new SingleDropControl(IRECT(300, 200, 380, 280));
  f.mUI->AttachControl(pControl);

  f.mUI->OnDropMultiple({"first.wav", "second.wav"}, 340, 240);
  REQUIRE(pControl->mDropped == std::vector<std::string>{"first.wav"});
}

#pragma mark - Gestures

TEST_CASE("Gestures: a recognised gesture is delivered to the control that registered for it", "[integration][mouse][gestures]")
{
  Fixture f;
  int nCalls = 0;
  IGestureInfo received;

  REQUIRE_FALSE(f.mA->GetWantsGestures());
  REQUIRE_FALSE(f.mUI->RespondsToGesture(50, 50));

  f.mA->AttachGestureRecognizer(EGestureType::DoubleTap, [&](IControl* pCaller, const IGestureInfo& info) {
    nCalls++;
    received = info;
    REQUIRE(pCaller == f.mA);
  });

  REQUIRE(f.mA->GetWantsGestures());
  REQUIRE(f.mUI->RespondsToGesture(50, 50));
  REQUIRE_FALSE(f.mUI->RespondsToGesture(250, 50));

  IGestureInfo info;
  info.x = 60.f;
  info.y = 70.f;
  info.type = EGestureType::DoubleTap;
  f.mUI->OnGestureRecognized(info);

  REQUIRE(nCalls == 1);
  REQUIRE(received.x == Approx(60.f));
  REQUIRE(f.mA->GetLastGesture() == EGestureType::DoubleTap);

  // A gesture type the control did not register for is not handled
  info.type = EGestureType::Pinch;
  f.mUI->OnGestureRecognized(info);
  REQUIRE(nCalls == 1);
  REQUIRE(f.mA->GetLastGesture() == EGestureType::DoubleTap);

  // A gesture over a different control
  info.type = EGestureType::DoubleTap;
  info.x = 250.f;
  f.mUI->OnGestureRecognized(info);
  REQUIRE(nCalls == 1);
}

TEST_CASE("Gestures: regions of the UI can handle gestures without a control", "[integration][mouse][gestures]")
{
  Fixture f;
  int nCalls = 0;

  f.mUI->AttachGestureRecognizerToRegion(IRECT(300, 200, 400, 300), EGestureType::LongPress1, [&](IControl* pCaller, const IGestureInfo& info) {
    nCalls++;
    REQUIRE(pCaller == nullptr);
  });

  REQUIRE(f.mUI->RespondsToGesture(350, 250));
  REQUIRE_FALSE(f.mUI->RespondsToGesture(350, 150));

  IGestureInfo info;
  info.x = 350.f;
  info.y = 250.f;
  info.type = EGestureType::LongPress1;
  f.mUI->OnGestureRecognized(info);
  REQUIRE(nCalls == 1);

  info.y = 150.f;
  f.mUI->OnGestureRecognized(info);
  REQUIRE(nCalls == 1);

  f.mUI->ClearGestureRegions();
  REQUIRE_FALSE(f.mUI->RespondsToGesture(350, 250));
  info.y = 250.f;
  f.mUI->OnGestureRecognized(info);
  REQUIRE(nCalls == 1);
}

#pragma mark - Pro Tools automation queries

TEST_CASE("GetParamIdxForPTAutomation: reports the parameter of the control at a point", "[integration][mouse][params]")
{
  Fixture f;

  REQUIRE(f.mUI->GetParamIdxForPTAutomation(kKnobBounds.MW(), kKnobBounds.MH()) == kGain);
  REQUIRE(f.mUI->GetLastClickedParamForPTAutomation() == kGain);
  // Reading the last clicked parameter resets it
  REQUIRE(f.mUI->GetLastClickedParamForPTAutomation() == kNoParameter);

  REQUIRE(f.mUI->GetParamIdxForPTAutomation(kStepKnobBounds.MW(), kStepKnobBounds.MH()) == kSteps);
  REQUIRE(f.mUI->GetParamIdxForPTAutomation(50, 50) == kNoParameter);
  REQUIRE(f.mUI->GetParamIdxForPTAutomation(390, 290) == kNoParameter);
}
