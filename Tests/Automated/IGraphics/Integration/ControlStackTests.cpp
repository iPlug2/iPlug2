/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the IGraphics control stack: attaching, finding, iterating and removing controls, z-order and containers

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestEditorDelegate.h"
#include "TestControls.h"
#include "IControls.h"
#include "IPopupMenuControl.h"
#include "ITextEntryControl.h"
#include "IBubbleControl.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;

namespace
{
enum EParams { kParamA = 0, kParamB, kParamC, kNumParams };
enum ECtrlTags { kTagA = 0, kTagB, kTagC, kTagD, kTagContainer, kTagChild1, kTagChild2 };

struct Fixture
{
  Fixture(TestEditorDelegate::LayoutFunc layoutFunc = nullptr)
  : mDlg(kNumParams, 400, 300, layoutFunc)
  {
    mDlg.GetParam(kParamA)->InitDouble("A", 0., 0., 1., 0.01);
    mDlg.GetParam(kParamB)->InitDouble("B", 0., 0., 1., 0.01);
    mDlg.GetParam(kParamC)->InitDouble("C", 0., 0., 1., 0.01);
    mUI = mDlg.OpenUI();
  }

  TestEditorDelegate mDlg;
  IGraphicsHeadless* mUI = nullptr;
};

std::vector<IControl*> Collect(IGraphics* pGraphics, const char* group)
{
  std::vector<IControl*> controls;
  pGraphics->ForControlInGroup(group, [&controls](IControl* pControl) { controls.push_back(pControl); });
  return controls;
}
} // namespace

TEST_CASE("AttachControl: returns the control, initialises it and adds it to the top of the stack", "[integration][controlstack]")
{
  Fixture f;
  auto* pProbe = new ProbeControl(IRECT(10, 10, 50, 50));

  REQUIRE(f.mUI->NControls() == 0);
  REQUIRE(f.mUI->AttachControl(pProbe) == pProbe);
  REQUIRE(f.mUI->NControls() == 1);
  REQUIRE(f.mUI->GetControl(0) == pProbe);
  REQUIRE(f.mUI->GetControlIdx(pProbe) == 0);
  REQUIRE(pProbe->GetUI() == f.mUI);
  REQUIRE(pProbe->GetDelegate() == &f.mDlg);
  REQUIRE(pProbe->mNumInits == 1);
  REQUIRE(pProbe->mNumAttaches == 1);
  REQUIRE(pProbe->mNumResizes == 1);
  REQUIRE(pProbe->mNumRescales == 1);
  REQUIRE(pProbe->GetParent() == nullptr);
  REQUIRE(pProbe->IsDirty());

  auto* pSecond = f.mUI->AttachControl(new ProbeControl(IRECT(60, 10, 100, 50)));
  REQUIRE(f.mUI->NControls() == 2);
  REQUIRE(f.mUI->GetControl(1) == pSecond);
  REQUIRE(f.mUI->GetControlIdx(pSecond) == 1);
}

TEST_CASE("GetControl: an index that is out of range gives nullptr", "[integration][controlstack]")
{
  Fixture f;
  f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)));

  REQUIRE(f.mUI->GetControl(1) == nullptr);
  REQUIRE(f.mUI->GetControl(-1) == nullptr);

  ProbeControl unattached {IRECT()};
  REQUIRE(f.mUI->GetControlIdx(&unattached) == -1);
}

TEST_CASE("Control tags: controls can be found by tag and report their tag", "[integration][controlstack][tags]")
{
  Fixture f;
  auto* pA = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)), kTagA);
  auto* pUntagged = f.mUI->AttachControl(new ProbeControl(IRECT(60, 10, 100, 50)));
  auto* pC = f.mUI->AttachControl(new ProbeControl(IRECT(110, 10, 150, 50)), kTagC);

  REQUIRE(f.mUI->GetControlWithTag(kTagA) == pA);
  REQUIRE(f.mUI->GetControlWithTag(kTagC) == pC);
  REQUIRE(f.mUI->GetControlWithTag(kTagB) == nullptr);
  REQUIRE(f.mUI->GetControlTag(pA) == kTagA);
  REQUIRE(f.mUI->GetControlTag(pC) == kTagC);
  REQUIRE(f.mUI->GetControlTag(pUntagged) == kNoTag);
  REQUIRE(pA->GetTag() == kTagA);
  REQUIRE(pUntagged->GetTag() == kNoTag);
  REQUIRE(f.mUI->GetIdxOfTaggedControl(kTagA) == 0);
  REQUIRE(f.mUI->GetIdxOfTaggedControl(kTagC) == 2);
  REQUIRE(f.mUI->GetIdxOfTaggedControl(kTagB) == -1);
}

TEST_CASE("Control groups: ForControlInGroup visits exactly the controls in the group, in stack order", "[integration][controlstack][groups]")
{
  Fixture f;
  auto* pA = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)), kNoTag, "left");
  auto* pB = f.mUI->AttachControl(new ProbeControl(IRECT(60, 10, 100, 50)), kNoTag, "right");
  auto* pC = f.mUI->AttachControl(new ProbeControl(IRECT(110, 10, 150, 50)), kNoTag, "left");
  auto* pD = f.mUI->AttachControl(new ProbeControl(IRECT(160, 10, 200, 50)));

  REQUIRE(std::string(pA->GetGroup()) == "left");
  REQUIRE(std::string(pB->GetGroup()) == "right");
  REQUIRE(std::string(pD->GetGroup()).empty());

  REQUIRE(Collect(f.mUI, "left") == std::vector<IControl*>{pA, pC});
  REQUIRE(Collect(f.mUI, "right") == std::vector<IControl*>{pB});
  REQUIRE(Collect(f.mUI, "middle").empty());
  // Controls with no group are not in the "" group
  REQUIRE(Collect(f.mUI, "").empty());

  pD->SetGroup("right");
  REQUIRE(Collect(f.mUI, "right") == std::vector<IControl*>{pB, pD});
}

TEST_CASE("AttachPanelBackground: the background goes to the bottom of the stack and fills the UI", "[integration][controlstack]")
{
  Fixture f;
  auto* pProbe = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)));
  f.mUI->AttachPanelBackground(COLOR_BLUE);

  REQUIRE(f.mUI->NControls() == 2);
  REQUIRE(f.mUI->GetControl(1) == pProbe);
  REQUIRE(f.mUI->GetBackgroundControl() == f.mUI->GetControl(0));
  REQUIRE(f.mUI->GetBackgroundControl()->GetRECT() == f.mUI->GetBounds());
  REQUIRE(f.mUI->GetBackgroundControl()->As<IPanelControl>() != nullptr);

  f.mUI->RenderFrame();
  bool drewBackground = false;

  for (auto& op : f.mUI->mDrawOps)
  {
    if (op.mType == IGraphicsHeadless::DrawOp::EType::Fill && op.mBounds == f.mUI->GetBounds())
    {
      drewBackground = (op.mColor.R == COLOR_BLUE.R && op.mColor.G == COLOR_BLUE.G && op.mColor.B == COLOR_BLUE.B);
      break;
    }
  }

  REQUIRE(drewBackground);
}

TEST_CASE("Iteration: ForStandardControlsFunc visits the attached controls in order", "[integration][controlstack]")
{
  Fixture f;
  f.mUI->AttachPanelBackground(COLOR_GRAY);
  auto* pA = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)));
  auto* pB = f.mUI->AttachControl(new ProbeControl(IRECT(60, 10, 100, 50)));

  std::vector<IControl*> visited;
  f.mUI->ForStandardControlsFunc([&visited](IControl* pControl) { visited.push_back(pControl); });

  REQUIRE(visited == std::vector<IControl*>{f.mUI->GetControl(0), pA, pB});
}

TEST_CASE("Iteration: ForAllControlsFunc also visits the special controls", "[integration][controlstack]")
{
  Fixture f;
  auto* pA = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)));

  f.mUI->AttachPopupMenuControl();
  f.mUI->AttachTextEntryControl();
  f.mUI->AttachBubbleControl();
  f.mUI->ShowFPSDisplay(true);

  std::vector<IControl*> standard, all;
  f.mUI->ForStandardControlsFunc([&standard](IControl* pControl) { standard.push_back(pControl); });
  f.mUI->ForAllControlsFunc([&all](IControl* pControl) { all.push_back(pControl); });

  REQUIRE(standard == std::vector<IControl*>{pA});
  REQUIRE(all.size() == 5);
  REQUIRE(all.front() == pA);

  auto contains = [&all](IControl* pControl) { return std::find(all.begin(), all.end(), pControl) != all.end(); };
  REQUIRE(contains(static_cast<IControl*>(f.mUI->GetPopupMenuControl())));
  REQUIRE(contains(static_cast<IControl*>(f.mUI->GetTextEntryControl())));
  REQUIRE(contains(static_cast<IControl*>(f.mUI->GetBubbleControl())));

  // The special controls are not part of the control stack
  REQUIRE(f.mUI->NControls() == 1);
  REQUIRE(f.mUI->NBubbleControls() == 1);
  REQUIRE(f.mUI->ShowingFPSDisplay());
}

TEST_CASE("Parameter queries: GetControlWithParamIdx and ForControlWithParam", "[integration][controlstack][params]")
{
  Fixture f;
  auto* pA1 = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50), kParamA));
  auto* pB = f.mUI->AttachControl(new ProbeControl(IRECT(60, 10, 100, 50), kParamB));
  auto* pA2 = f.mUI->AttachControl(new ProbeControl(IRECT(110, 10, 150, 50), kParamA));
  auto* pBC = f.mUI->AttachControl(new ProbeControl(IRECT(160, 10, 200, 50), {kParamB, kParamC}));
  f.mUI->AttachControl(new ProbeControl(IRECT(210, 10, 250, 50)));

  REQUIRE(f.mUI->GetControlWithParamIdx(kParamA) == pA1);
  REQUIRE(f.mUI->GetControlWithParamIdx(kParamB) == pB);
  REQUIRE(f.mUI->GetControlWithParamIdx(kParamC) == pBC);

  REQUIRE(pA1->LinkedToParam(kParamA) == 0);
  REQUIRE(pA1->LinkedToParam(kParamB) == kNoValIdx);
  REQUIRE(pBC->LinkedToParam(kParamB) == 0);
  REQUIRE(pBC->LinkedToParam(kParamC) == 1);
  REQUIRE(pBC->NVals() == 2);

  std::vector<IControl*> visited;
  auto collect = [&visited](IControl* pControl) { visited.push_back(pControl); };

  f.mUI->ForControlWithParam(kParamA, collect);
  REQUIRE(visited == std::vector<IControl*>{pA1, pA2});

  visited.clear();
  f.mUI->ForControlWithParam(kParamC, collect);
  REQUIRE(visited == std::vector<IControl*>{pBC});

  visited.clear();
  f.mUI->ForControlWithParam({kParamA, kParamC}, collect);
  REQUIRE(visited == std::vector<IControl*>{pA1, pA2, pBC});
}

TEST_CASE("HideControl/DisableControl: apply to every control linked to the parameter", "[integration][controlstack][params]")
{
  Fixture f;
  auto* pA1 = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50), kParamA));
  auto* pB = f.mUI->AttachControl(new ProbeControl(IRECT(60, 10, 100, 50), kParamB));
  auto* pA2 = f.mUI->AttachControl(new ProbeControl(IRECT(110, 10, 150, 50), kParamA));
  f.mUI->RenderUntilClean();

  f.mUI->HideControl(kParamA, true);
  REQUIRE(pA1->IsHidden());
  REQUIRE(pA2->IsHidden());
  REQUIRE_FALSE(pB->IsHidden());
  REQUIRE(pA1->IsDirty());
  REQUIRE_FALSE(pB->IsDirty());

  f.mUI->HideControl(kParamA, false);
  REQUIRE_FALSE(pA1->IsHidden());
  REQUIRE_FALSE(pA2->IsHidden());

  f.mUI->DisableControl(kParamB, true);
  REQUIRE(pB->IsDisabled());
  REQUIRE_FALSE(pA1->IsDisabled());
  REQUIRE(pB->GetBlend().mWeight == Approx(GRAYED_ALPHA));

  f.mUI->DisableControl(kParamB, false);
  REQUIRE_FALSE(pB->IsDisabled());
  REQUIRE(pB->GetBlend().mWeight == Approx(1.f));
}

TEST_CASE("RemoveControl: by index and by pointer", "[integration][controlstack][remove]")
{
  Fixture f;
  auto* pA = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)), kTagA);
  auto* pB = f.mUI->AttachControl(new ProbeControl(IRECT(60, 10, 100, 50)), kTagB);
  auto* pC = f.mUI->AttachControl(new ProbeControl(IRECT(110, 10, 150, 50)), kTagC);
  f.mUI->RenderUntilClean();

  SECTION("by index")
  {
    f.mUI->RemoveControl(1);
    REQUIRE(f.mUI->NControls() == 2);
    REQUIRE(f.mUI->GetControl(0) == pA);
    REQUIRE(f.mUI->GetControl(1) == pC);
    REQUIRE(f.mUI->GetControlWithTag(kTagB) == nullptr);
    REQUIRE(f.mUI->GetControlWithTag(kTagC) == pC);
  }

  SECTION("by pointer")
  {
    f.mUI->RemoveControl(pA);
    REQUIRE(f.mUI->NControls() == 2);
    REQUIRE(f.mUI->GetControl(0) == pB);
    REQUIRE(f.mUI->GetControlIdx(pC) == 1);
    REQUIRE(f.mUI->GetControlWithTag(kTagA) == nullptr);
  }

  // What is left is redrawn, since the removed control may have been on top of it
  REQUIRE(f.mUI->GetControl(0)->IsDirty());
  REQUIRE(f.mUI->RenderFrame());
}

TEST_CASE("RemoveControlWithTag: removes the control and frees the tag", "[integration][controlstack][remove][tags]")
{
  Fixture f;
  auto* pA = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)), kTagA);
  f.mUI->AttachControl(new ProbeControl(IRECT(60, 10, 100, 50)), kTagB);

  f.mUI->RemoveControlWithTag(kTagB);
  REQUIRE(f.mUI->NControls() == 1);
  REQUIRE(f.mUI->GetControl(0) == pA);
  REQUIRE(f.mUI->GetControlWithTag(kTagB) == nullptr);

  // The tag can be used again
  auto* pNew = f.mUI->AttachControl(new ProbeControl(IRECT(60, 10, 100, 50)), kTagB);
  REQUIRE(pNew != nullptr);
  REQUIRE(f.mUI->GetControlWithTag(kTagB) == pNew);
}

TEST_CASE("RemoveControls: removes everything from an index upwards", "[integration][controlstack][remove]")
{
  Fixture f;
  f.mUI->AttachPanelBackground(COLOR_GRAY);
  auto* pA = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)), kTagA);
  f.mUI->AttachControl(new ProbeControl(IRECT(60, 10, 100, 50)), kTagB);
  f.mUI->AttachControl(new ProbeControl(IRECT(110, 10, 150, 50)), kTagC);

  f.mUI->RemoveControls(2);
  REQUIRE(f.mUI->NControls() == 2);
  REQUIRE(f.mUI->GetControl(1) == pA);
  REQUIRE(f.mUI->GetControlWithTag(kTagA) == pA);
  REQUIRE(f.mUI->GetControlWithTag(kTagB) == nullptr);
  REQUIRE(f.mUI->GetControlWithTag(kTagC) == nullptr);

  // An index past the end removes nothing
  f.mUI->RemoveControls(5);
  REQUIRE(f.mUI->NControls() == 2);

  f.mUI->RemoveControls(0);
  REQUIRE(f.mUI->NControls() == 0);
  REQUIRE_FALSE(f.mUI->RenderFrame());
}

TEST_CASE("RemoveAllControls: removes the standard and the special controls", "[integration][controlstack][remove]")
{
  Fixture f;
  f.mUI->AttachPanelBackground(COLOR_GRAY);
  f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)), kTagA);
  f.mUI->AttachPopupMenuControl();
  f.mUI->AttachTextEntryControl();
  f.mUI->AttachBubbleControl();
  f.mUI->ShowFPSDisplay(true);

  f.mUI->RemoveAllControls();

  REQUIRE(f.mUI->NControls() == 0);
  REQUIRE(f.mUI->GetControlWithTag(kTagA) == nullptr);
  REQUIRE(f.mUI->GetPopupMenuControl() == nullptr);
  REQUIRE(f.mUI->GetTextEntryControl() == nullptr);
  REQUIRE(f.mUI->NBubbleControls() == 0);
  REQUIRE_FALSE(f.mUI->ShowingFPSDisplay());

  int nVisited = 0;
  f.mUI->ForAllControlsFunc([&nVisited](IControl*) { nVisited++; });
  REQUIRE(nVisited == 0);
  REQUIRE_FALSE(f.mUI->RenderFrame());

  // The UI can be populated again
  f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)), kTagA);
  REQUIRE(f.mUI->NControls() == 1);
  REQUIRE(f.mUI->RenderFrame());
}

TEST_CASE("Removing a control that is captured or under the mouse leaves no dangling state", "[integration][controlstack][remove][mouse]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);
  f.mUI->AttachPanelBackground(COLOR_GRAY);
  auto* pBelow = f.mUI->AttachControl(new ProbeControl(IRECT(0, 0, 200, 200)))->As<ProbeControl>();
  auto* pA = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)), kTagA);

  SECTION("RemoveControl(pointer) while captured")
  {
    f.mUI->MouseDown(20, 20);
    REQUIRE(f.mUI->ControlIsCaptured(pA));
    f.mUI->RemoveControl(pA);
    REQUIRE_FALSE(f.mUI->ControlIsCaptured());
    f.mUI->MouseDragTo(30, 30);
    f.mUI->MouseUp(30, 30);
    REQUIRE(pBelow->Count("drag") == 0);
    REQUIRE(pBelow->Count("up") == 0);
  }

  SECTION("RemoveControl(pointer) while under the mouse")
  {
    REQUIRE(f.mUI->MouseMove(20, 20));
    f.mUI->RemoveControl(pA);
    REQUIRE(f.mUI->MouseMove(21, 21));
    REQUIRE(pBelow->Count("over") == 1);
    f.mUI->OnMouseOut();
  }

  SECTION("RemoveControls(idx) while captured and under the mouse")
  {
    f.mUI->MouseMove(20, 20);
    f.mUI->MouseDown(20, 20);
    f.mUI->RemoveControls(2);
    REQUIRE_FALSE(f.mUI->ControlIsCaptured());
    f.mUI->MouseUp(20, 20);
    f.mUI->OnMouseOut();
    REQUIRE(f.mUI->NControls() == 2);
  }

  SECTION("RemoveAllControls while captured and under the mouse")
  {
    f.mUI->MouseMove(20, 20);
    f.mUI->MouseDown(20, 20);
    f.mUI->RemoveAllControls();
    REQUIRE_FALSE(f.mUI->ControlIsCaptured());
    f.mUI->MouseDragTo(25, 25);
    f.mUI->MouseUp(25, 25);
    f.mUI->OnMouseOut();
    REQUIRE(f.mUI->NControls() == 0);
  }
}

// BUG: IGraphics::RemoveControlWithTag() (IGraphics/IGraphics.cpp:149) deletes the control without releasing the mouse
// capture or clearing mMouseOver/mInTextEntry/mInPopupMenu, as RemoveControl() and RemoveControls() do. The next mouse
// event is then delivered to the deleted control (heap use-after-free, reported by ASan).
TEST_CASE("RemoveControlWithTag: a captured or moused-over control is released before it is deleted", "[.][bug]")
{
  Fixture f;
  f.mUI->EnableMouseOver(true);
  f.mUI->AttachPanelBackground(COLOR_GRAY);
  f.mUI->AttachControl(new ProbeControl(IRECT(0, 0, 200, 200)));
  auto* pA = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50)), kTagA);

  SECTION("captured")
  {
    f.mUI->MouseDown(20, 20);
    REQUIRE(f.mUI->ControlIsCaptured(pA));
    f.mUI->RemoveControlWithTag(kTagA);
    REQUIRE_FALSE(f.mUI->ControlIsCaptured());
    f.mUI->MouseUp(20, 20);
  }

  SECTION("under the mouse")
  {
    f.mUI->MouseMove(20, 20);
    f.mUI->RemoveControlWithTag(kTagA);
    f.mUI->MouseMove(300, 250);
  }

  SECTION("in a text entry")
  {
    f.mUI->CreateTextEntry(*pA, IText(), pA->GetRECT(), "abc");
    REQUIRE(f.mUI->GetControlInTextEntry() == pA);
    f.mUI->RemoveControlWithTag(kTagA);
    REQUIRE(f.mUI->GetControlInTextEntry() == nullptr);
    f.mUI->SetControlValueAfterTextEdit("def");
  }
}

TEST_CASE("Z-order: later controls are on top for hit testing", "[integration][controlstack][mouse]")
{
  Fixture f;
  auto* pBottom = f.mUI->AttachControl(new ProbeControl(IRECT(0, 0, 100, 100)))->As<ProbeControl>();
  auto* pTop = f.mUI->AttachControl(new ProbeControl(IRECT(50, 50, 150, 150)))->As<ProbeControl>();

  SECTION("the overlap goes to the top control")
  {
    f.mUI->Click(75, 75);
    REQUIRE(pTop->Count("down") == 1);
    REQUIRE(pBottom->Count("down") == 0);
  }

  SECTION("outside the overlap each control gets its own clicks")
  {
    f.mUI->Click(25, 25);
    f.mUI->Click(125, 125);
    REQUIRE(pBottom->Count("down") == 1);
    REQUIRE(pTop->Count("down") == 1);
    REQUIRE(pBottom->Last("down")->mX == Approx(25.f));
    REQUIRE(pTop->Last("down")->mX == Approx(125.f));
  }

  SECTION("a hidden control lets the click through")
  {
    pTop->Hide(true);
    f.mUI->Click(75, 75);
    REQUIRE(pTop->Count("down") == 0);
    REQUIRE(pBottom->Count("down") == 1);
  }

  SECTION("a control that ignores the mouse lets the click through")
  {
    pTop->SetIgnoreMouse(true);
    REQUIRE(pTop->GetIgnoreMouse());
    f.mUI->Click(75, 75);
    REQUIRE(pTop->Count("down") == 0);
    REQUIRE(pBottom->Count("down") == 1);
  }

  SECTION("a disabled control gets no mouse events")
  {
    pTop->SetDisabled(true);
    f.mUI->Click(125, 125);
    REQUIRE(pTop->Count("down") == 0);
    REQUIRE(pTop->Count("up") == 0);
  }

  SECTION("a disabled control can opt in to mouse events")
  {
    pTop->SetDisabled(true);
    pTop->SetMouseEventsWhenDisabled(true);
    REQUIRE(pTop->GetMouseEventsWhenDisabled());
    f.mUI->Click(125, 125);
    REQUIRE(pTop->Count("down") == 1);
  }

  SECTION("drawing is back to front")
  {
    pBottom->mColor = COLOR_GREEN;
    pTop->mColor = COLOR_BLUE;
    f.mUI->SetStrictDrawing(true); // one pass over the union of the dirty areas
    f.mUI->RenderFrame();
    REQUIRE(f.mUI->mDrawOps.size() == 2);
    REQUIRE(f.mUI->mDrawOps[0].mBounds == pBottom->GetRECT());
    REQUIRE(f.mUI->mDrawOps[1].mBounds == pTop->GetRECT());
  }
}

TEST_CASE("Control bounds: the target rectangle is used for hit testing, the draw rectangle for drawing", "[integration][controlstack][mouse]")
{
  Fixture f;
  auto* pProbe = f.mUI->AttachControl(new ProbeControl(IRECT(100, 100, 200, 200)))->As<ProbeControl>();

  REQUIRE(pProbe->GetRECT() == IRECT(100, 100, 200, 200));
  REQUIRE(pProbe->GetTargetRECT() == IRECT(100, 100, 200, 200));
  REQUIRE(pProbe->IsHit(150, 150));
  REQUIRE_FALSE(pProbe->IsHit(99, 150));
  REQUIRE_FALSE(pProbe->IsHit(150, 201));

  SECTION("SetTargetRECT changes only the mouse target")
  {
    const int nResizes = pProbe->mNumResizes;
    pProbe->SetTargetRECT(IRECT(100, 100, 120, 120));
    REQUIRE(pProbe->GetRECT() == IRECT(100, 100, 200, 200));
    REQUIRE(pProbe->GetTargetRECT() == IRECT(100, 100, 120, 120));
    REQUIRE(pProbe->mNumResizes == nResizes);

    f.mUI->Click(150, 150);
    REQUIRE(pProbe->Count("down") == 0);
    f.mUI->Click(110, 110);
    REQUIRE(pProbe->Count("down") == 1);
  }

  SECTION("SetRECT changes only the draw area and calls OnResize")
  {
    const int nResizes = pProbe->mNumResizes;
    pProbe->SetRECT(IRECT(0, 0, 50, 50));
    REQUIRE(pProbe->GetRECT() == IRECT(0, 0, 50, 50));
    REQUIRE(pProbe->GetTargetRECT() == IRECT(100, 100, 200, 200));
    REQUIRE(pProbe->mNumResizes == nResizes + 1);

    f.mUI->Click(25, 25);
    REQUIRE(pProbe->Count("down") == 0);
    f.mUI->Click(150, 150);
    REQUIRE(pProbe->Count("down") == 1);
  }

  SECTION("SetTargetAndDrawRECTs changes both and calls OnResize")
  {
    const int nResizes = pProbe->mNumResizes;
    pProbe->SetTargetAndDrawRECTs(IRECT(0, 0, 50, 50));
    REQUIRE(pProbe->GetRECT() == IRECT(0, 0, 50, 50));
    REQUIRE(pProbe->GetTargetRECT() == IRECT(0, 0, 50, 50));
    REQUIRE(pProbe->mNumResizes == nResizes + 1);

    f.mUI->Click(150, 150);
    REQUIRE(pProbe->Count("down") == 0);
    f.mUI->Click(25, 25);
    REQUIRE(pProbe->Count("down") == 1);
  }

  SECTION("SetPosition moves both rectangles, keeping their sizes and relative offset")
  {
    pProbe->SetTargetRECT(IRECT(110, 120, 150, 160));
    pProbe->SetPosition(10, 20);
    REQUIRE(pProbe->GetRECT() == IRECT(10, 20, 110, 120));
    REQUIRE(pProbe->GetTargetRECT() == IRECT(20, 40, 60, 80));

    // Negative positions are clamped to the origin
    pProbe->SetPosition(-50, -50);
    REQUIRE(pProbe->GetRECT() == IRECT(0, 0, 100, 100));
  }

  SECTION("SetSize resizes from the top left corner")
  {
    pProbe->SetSize(30, 40);
    REQUIRE(pProbe->GetRECT() == IRECT(100, 100, 130, 140));
    REQUIRE(pProbe->GetTargetRECT() == IRECT(100, 100, 130, 140));

    pProbe->SetSize(-1, -1);
    REQUIRE(pProbe->GetRECT().W() == Approx(0.f));
    REQUIRE(pProbe->GetRECT().H() == Approx(0.f));
  }
}

TEST_CASE("IGraphics::SetControlPosition/Size/Bounds move a control and redraw the UI", "[integration][controlstack][drawing]")
{
  Fixture f;
  f.mUI->AttachPanelBackground(COLOR_GRAY);
  auto* pProbe = f.mUI->AttachControl(new ProbeControl(IRECT(100, 100, 200, 200)))->As<ProbeControl>();
  f.mUI->RenderUntilClean();

  f.mUI->SetControlPosition(pProbe, 10, 20);
  REQUIRE(pProbe->GetRECT() == IRECT(10, 20, 110, 120));
  // Everything is redrawn: the area the control left has to be repainted too
  REQUIRE(f.mUI->GetControl(0)->IsDirty());
  REQUIRE(f.mUI->RenderFrame());

  f.mUI->SetControlSize(pProbe, 20, 30);
  REQUIRE(pProbe->GetRECT() == IRECT(10, 20, 30, 50));
  REQUIRE(f.mUI->RenderFrame());

  f.mUI->SetControlBounds(pProbe, IRECT(200, 200, 250, 250));
  REQUIRE(pProbe->GetRECT() == IRECT(200, 200, 250, 250));
  REQUIRE(pProbe->GetTargetRECT() == IRECT(200, 200, 250, 250));
  REQUIRE(f.mUI->RenderFrame());

  // Moving a hidden control needs no redraw
  pProbe->Hide(true);
  f.mUI->RenderUntilClean();
  f.mUI->SetControlBounds(pProbe, IRECT(0, 0, 50, 50));
  REQUIRE(pProbe->GetRECT() == IRECT(0, 0, 50, 50));
  REQUIRE_FALSE(f.mUI->GetControl(0)->IsDirty());
}

TEST_CASE("Hide/SetDisabled: change the state and mark the control dirty", "[integration][controlstack]")
{
  Fixture f;
  auto* pProbe = f.mUI->AttachControl(new ProbeControl(IRECT(100, 100, 200, 200)))->As<ProbeControl>();
  f.mUI->RenderUntilClean();
  REQUIRE_FALSE(pProbe->IsDirty());

  pProbe->Hide(true);
  REQUIRE(pProbe->IsHidden());
  REQUIRE(pProbe->IsDirty());
  f.mUI->RenderUntilClean();

  pProbe->Hide(false);
  REQUIRE_FALSE(pProbe->IsHidden());
  REQUIRE(pProbe->IsDirty());
  f.mUI->RenderUntilClean();

  pProbe->SetDisabled(true);
  REQUIRE(pProbe->IsDisabled());
  REQUIRE(pProbe->IsDirty());
}

TEST_CASE("AssignParamNameToolTips: gives each control linked to a parameter the parameter's name", "[integration][controlstack][params]")
{
  Fixture f;
  auto* pA = f.mUI->AttachControl(new ProbeControl(IRECT(10, 10, 50, 50), kParamA));
  auto* pB = f.mUI->AttachControl(new ProbeControl(IRECT(60, 10, 100, 50), kParamB));
  auto* pNone = f.mUI->AttachControl(new ProbeControl(IRECT(110, 10, 150, 50)));

  f.mUI->AssignParamNameToolTips();
  REQUIRE(std::string(pA->GetTooltip()) == "A");
  REQUIRE(std::string(pB->GetTooltip()) == "B");
  REQUIRE(std::string(pNone->GetTooltip()).empty());
}

TEST_CASE("StyleAllVectorControls: applies a style to every vector control", "[integration][controlstack][style]")
{
  Fixture f;
  auto* pKnob = f.mUI->AttachControl(new IVKnobControl(IRECT(10, 10, 110, 110), kParamA))->As<IVKnobControl>();
  auto* pSlider = f.mUI->AttachControl(new IVSliderControl(IRECT(120, 10, 160, 110), kParamB))->As<IVSliderControl>();
  f.mUI->AttachControl(new ProbeControl(IRECT(170, 10, 200, 50)));

  f.mUI->StyleAllVectorControls(DEFAULT_STYLE.WithColor(kFG, COLOR_ORANGE).WithRoundness(0.75f));

  REQUIRE(pKnob->GetColor(kFG).R == COLOR_ORANGE.R);
  REQUIRE(pKnob->GetColor(kFG).G == COLOR_ORANGE.G);
  REQUIRE(pSlider->GetColor(kFG).G == COLOR_ORANGE.G);
  REQUIRE(pSlider->GetStyle().roundness == Approx(0.75f));
}

#pragma mark - Containers

namespace
{
struct ContainerFixture : Fixture
{
  ContainerFixture()
  {
    mUI->AttachPanelBackground(COLOR_GRAY);
    mContainer = new IContainerBase(IRECT(100, 100, 300, 200),
      [this](IContainerBase* pContainer, const IRECT& bounds) {
        mAttachBounds = bounds;
        mChild1 = pContainer->AddChildControl(new ProbeControl(bounds.FracRectHorizontal(0.5f, false)), kTagChild1, "children")->As<ProbeControl>();
        mChild2 = pContainer->AddChildControl(new ProbeControl(bounds.FracRectHorizontal(0.5f, true)), kTagChild2, "children")->As<ProbeControl>();
      },
      [this](IContainerBase* pContainer, const IRECT& bounds) {
        mNumResizeCalls++;
        mResizeBounds = bounds;
        pContainer->GetChild(0)->SetTargetAndDrawRECTs(bounds.FracRectHorizontal(0.5f, false));
        pContainer->GetChild(1)->SetTargetAndDrawRECTs(bounds.FracRectHorizontal(0.5f, true));
      });
    mUI->AttachControl(mContainer, kTagContainer);
  }

  IContainerBase* mContainer = nullptr;
  ProbeControl* mChild1 = nullptr;
  ProbeControl* mChild2 = nullptr;
  IRECT mAttachBounds;
  IRECT mResizeBounds;
  int mNumResizeCalls = 0;
};
} // namespace

TEST_CASE("IContainerBase: the attach function adds children to the control stack above the container", "[integration][controlstack][container]")
{
  ContainerFixture f;

  REQUIRE(f.mAttachBounds == IRECT(100, 100, 300, 200));
  REQUIRE(f.mContainer->NChildren() == 2);
  REQUIRE(f.mContainer->GetChild(0) == f.mChild1);
  REQUIRE(f.mContainer->GetChild(1) == f.mChild2);
  REQUIRE(f.mContainer->GetChild(2) == nullptr);
  REQUIRE(f.mChild1->GetParent() == f.mContainer);
  REQUIRE(f.mChild2->GetParent() == f.mContainer);
  REQUIRE(f.mContainer->GetParent() == nullptr);

  // background, container, child, child
  REQUIRE(f.mUI->NControls() == 4);
  REQUIRE(f.mUI->GetControlIdx(f.mContainer) == 1);
  REQUIRE(f.mUI->GetControlIdx(f.mChild1) == 2);
  REQUIRE(f.mUI->GetControlIdx(f.mChild2) == 3);
  REQUIRE(f.mUI->GetControlWithTag(kTagChild1) == f.mChild1);
  REQUIRE(std::string(f.mChild2->GetGroup()) == "children");
  REQUIRE(f.mChild1->GetRECT() == IRECT(100, 100, 200, 200));
  REQUIRE(f.mChild2->GetRECT() == IRECT(200, 100, 300, 200));

  int nVisited = 0;
  f.mContainer->ForAllChildrenFunc([&](int childIdx, IControl* pChild) {
    REQUIRE(pChild == f.mContainer->GetChild(childIdx));
    nVisited++;
  });
  REQUIRE(nVisited == 2);
}

TEST_CASE("IContainerBase: children take the mouse, a container made with attach/resize functions ignores it", "[integration][controlstack][container][mouse]")
{
  ContainerFixture f;

  REQUIRE(f.mContainer->GetIgnoreMouse());
  f.mUI->Click(150, 150);
  f.mUI->Click(250, 150);
  REQUIRE(f.mChild1->Count("down") == 1);
  REQUIRE(f.mChild2->Count("down") == 1);
}

TEST_CASE("IContainerBase: Hide and SetDisabled propagate to the children", "[integration][controlstack][container]")
{
  ContainerFixture f;

  f.mContainer->Hide(true);
  REQUIRE(f.mContainer->IsHidden());
  REQUIRE(f.mChild1->IsHidden());
  REQUIRE(f.mChild2->IsHidden());
  f.mUI->Click(150, 150);
  REQUIRE(f.mChild1->Count("down") == 0);

  f.mContainer->Hide(false);
  REQUIRE_FALSE(f.mChild1->IsHidden());
  REQUIRE_FALSE(f.mChild2->IsHidden());

  f.mContainer->SetDisabled(true);
  REQUIRE(f.mContainer->IsDisabled());
  REQUIRE(f.mChild1->IsDisabled());
  REQUIRE(f.mChild2->IsDisabled());

  f.mContainer->SetDisabled(false);
  REQUIRE_FALSE(f.mChild1->IsDisabled());
  REQUIRE_FALSE(f.mChild2->IsDisabled());
}

TEST_CASE("IContainerBase: the resize function lays the children out again when the bounds change", "[integration][controlstack][container][resize]")
{
  ContainerFixture f;
  const int nCalls = f.mNumResizeCalls;

  f.mContainer->SetTargetAndDrawRECTs(IRECT(0, 0, 100, 50));

  REQUIRE(f.mNumResizeCalls == nCalls + 1);
  REQUIRE(f.mResizeBounds == IRECT(0, 0, 100, 50));
  REQUIRE(f.mChild1->GetRECT() == IRECT(0, 0, 50, 50));
  REQUIRE(f.mChild2->GetRECT() == IRECT(50, 0, 100, 50));
}

TEST_CASE("IContainerBase: RemoveChildControl removes the child from the container and from the UI", "[integration][controlstack][container][remove]")
{
  ContainerFixture f;

  f.mContainer->RemoveChildControl(f.mChild1);

  REQUIRE(f.mContainer->NChildren() == 1);
  REQUIRE(f.mContainer->GetChild(0) == f.mChild2);
  REQUIRE(f.mUI->NControls() == 3);
  REQUIRE(f.mUI->GetControlWithTag(kTagChild1) == nullptr);
  REQUIRE(f.mUI->GetControlWithTag(kTagChild2) == f.mChild2);
  REQUIRE(f.mUI->RenderFrame());
}

TEST_CASE("IContainerBase: a child's dirty area is clipped to its parent", "[integration][controlstack][container][drawing]")
{
  ContainerFixture f;
  f.mUI->RenderUntilClean();
  f.mUI->ClearRecorded();

  // Move the child so that it sticks out of the container on the right
  f.mChild2->SetTargetAndDrawRECTs(IRECT(250, 100, 350, 200));
  f.mChild2->SetDirty(false);

  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->mLastDirtyBounds.R <= f.mContainer->GetRECT().R + 1.f);
  REQUIRE(f.mChild2->mNumDraws == 2);
  REQUIRE(f.mChild2->mClipDuringDraw.R <= f.mContainer->GetRECT().R + 1.f);
}

TEST_CASE("IContainerBase: nested containers", "[integration][controlstack][container]")
{
  Fixture f;
  ProbeControl* pLeaf = nullptr;
  IContainerBase* pInner = nullptr;

  auto* pOuter = new IContainerBase(IRECT(0, 0, 200, 200),
    [&](IContainerBase* pContainer, const IRECT& bounds) {
      pInner = new IContainerBase(bounds.GetPadded(-50.f),
        [&](IContainerBase* pInnerContainer, const IRECT& innerBounds) {
          pLeaf = pInnerContainer->AddChildControl(new ProbeControl(innerBounds))->As<ProbeControl>();
        }, nullptr);
      pContainer->AddChildControl(pInner);
    }, nullptr);

  f.mUI->AttachControl(pOuter);

  REQUIRE(f.mUI->NControls() == 3);
  REQUIRE(pInner->GetParent() == pOuter);
  REQUIRE(pLeaf->GetParent() == pInner);
  REQUIRE(pLeaf->GetRECT() == IRECT(50, 50, 150, 150));

  pOuter->Hide(true);
  REQUIRE(pInner->IsHidden());
  REQUIRE(pLeaf->IsHidden());

  pOuter->Hide(false);
  f.mUI->RenderUntilClean();
  REQUIRE(pLeaf->mNumDraws == 1);
  f.mUI->Click(100, 100);
  REQUIRE(pLeaf->Count("down") == 1);
}

// The children are removed with their container, so nothing is left with a parent pointer to a deleted control
TEST_CASE("IContainerBase: removing a container does not leave children pointing at it", "[integration][controlstack][container][remove]")
{
  ContainerFixture f;
  ProbeControl* pChild = f.mChild1;

  f.mUI->RemoveControl(f.mContainer);

  const bool childRemoved = f.mUI->GetControlIdx(pChild) < 0;
  REQUIRE((childRemoved || pChild->GetParent() == nullptr));

  if (!childRemoved)
    pChild->SetDirty(false);

  f.mUI->RenderFrame();
}

TEST_CASE("IContainerBase: removing a container removes nested containers and their children", "[integration][controlstack][container][remove]")
{
  Fixture f;
  ProbeControl* pLeaf = nullptr;
  IContainerBase* pInner = nullptr;

  f.mUI->AttachPanelBackground(COLOR_GRAY);
  auto* pOuter = new IContainerBase(IRECT(0, 0, 200, 200),
    [&](IContainerBase* pContainer, const IRECT& bounds) {
      pInner = new IContainerBase(bounds.GetPadded(-50.f),
        [&](IContainerBase* pInnerContainer, const IRECT& innerBounds) {
          pLeaf = pInnerContainer->AddChildControl(new ProbeControl(innerBounds), kTagChild1)->As<ProbeControl>();
        }, nullptr);
      pContainer->AddChildControl(pInner);
      pContainer->AddChildControl(new ProbeControl(bounds.GetFromTop(20.f)), kTagChild2);
    }, nullptr);
  f.mUI->AttachControl(pOuter, kTagContainer);
  auto* pOther = f.mUI->AttachControl(new ProbeControl(IRECT(300, 0, 400, 100)), kTagA);

  // background, outer, inner, leaf, second child of outer, other
  REQUIRE(f.mUI->NControls() == 6);

  f.mUI->RemoveControl(pOuter);

  REQUIRE(f.mUI->NControls() == 2);
  REQUIRE(f.mUI->GetControl(1) == pOther);
  REQUIRE(f.mUI->GetControlIdx(pOuter) == -1);
  REQUIRE(f.mUI->GetControlIdx(pInner) == -1);
  REQUIRE(f.mUI->GetControlIdx(pLeaf) == -1);
  REQUIRE(f.mUI->GetControlWithTag(kTagContainer) == nullptr);
  REQUIRE(f.mUI->GetControlWithTag(kTagChild1) == nullptr);
  REQUIRE(f.mUI->GetControlWithTag(kTagChild2) == nullptr);
  REQUIRE(f.mUI->GetControlWithTag(kTagA) == pOther);

  f.mUI->RenderUntilClean();
  f.mUI->Click(100, 100);
}

TEST_CASE("IContainerBase: removing a container whose child has the mouse", "[integration][controlstack][container][remove][mouse]")
{
  ContainerFixture f;

  SECTION("captured")
  {
    f.mUI->MouseDown(150, 150);
    REQUIRE(f.mUI->ControlIsCaptured(f.mChild1));

    f.mUI->RemoveControl(f.mContainer);

    REQUIRE_FALSE(f.mUI->ControlIsCaptured());
    REQUIRE(f.mUI->NControls() == 1);
    f.mUI->MouseDragTo(160, 160);
    f.mUI->MouseUp(160, 160);
  }

  SECTION("under the mouse")
  {
    f.mUI->EnableMouseOver(true);
    REQUIRE(f.mUI->MouseMove(150, 150));
    REQUIRE(f.mChild1->Count("over") == 1);

    f.mUI->RemoveControl(f.mContainer);

    REQUIRE(f.mUI->NControls() == 1);
    f.mUI->MouseMove(160, 160);
    f.mUI->OnMouseOut();
  }

  f.mUI->RenderUntilClean();
}

// IGraphics::RemoveControl() finds the children by their parent pointer, not through the container's own list, which
// still holds a child that was removed directly
TEST_CASE("IContainerBase: a container can be removed after one of its children was removed directly", "[integration][controlstack][container][remove]")
{
  ContainerFixture f;

  f.mUI->RemoveControl(f.mChild1);
  REQUIRE(f.mUI->NControls() == 3);

  f.mUI->RemoveControl(f.mContainer);
  REQUIRE(f.mUI->NControls() == 1);
  f.mUI->RenderUntilClean();
}

#if !defined(NDEBUG) || defined(IPLUG_LIVE_EDIT)
// The selection holds the container and its children, which are removed with it (#1549)
TEST_CASE("Live edit: deleting a selection of a container and its children", "[integration][controlstack][container][remove][liveedit]")
{
  ContainerFixture f;
  auto* pOther = f.mUI->AttachControl(new ProbeControl(IRECT(310, 100, 340, 200)));

  f.mUI->EnableLiveEdit(true);
  REQUIRE(f.mUI->LiveEditEnabled());

  // A marquee around the container, both children and the other control, starting on the background
  f.mUI->Drag(50, 50, 350, 250);
  REQUIRE(f.mUI->KeyDown(kVK_DELETE));

  REQUIRE(f.mUI->NControls() == 1);
  REQUIRE(f.mUI->GetControlIdx(f.mContainer) == -1);
  REQUIRE(f.mUI->GetControlIdx(pOther) == -1);
  REQUIRE(f.mUI->GetControlWithTag(kTagChild1) == nullptr);

  // Nothing is selected any more
  REQUIRE_FALSE(f.mUI->KeyDown(kVK_DELETE));
  f.mUI->RenderFrame();
  f.mUI->EnableLiveEdit(false);
}
#endif
