#include "IPlugReaperExtension.h"
#include "ReaperExt_include_in_plug_src.h"

#include "IControls.h"
#include "roboto.hpp"

IPlugReaperExtension::IPlugReaperExtension(reaper_plugin_info_t* pRec)
: ReaperExtBase(pRec)
{
  //Use IMPAPI to register any Reaper APIs that you need to use
  IMPAPI(GetNumTracks);
  IMPAPI(CountTracks);
  IMPAPI(InsertTrackAtIndex);
  
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
  };
  
  //Define some lambdas that can be called from either GUI widgets or in response to commands
  auto action1 = [](){
    MessageBox(gParent, "Action 1!", "Reaper extension test", MB_OK); //gParent
  };
  
  auto action2 = [](){
    InsertTrackAtIndex(GetNumTracks(), false);
  };
  
  //Register an action. args: name: lambda, add menu item,
  RegisterAction("IPlugReaperExtension: Action 1 - MsgBox", action1, true);
  RegisterAction("IPlugReaperExtension: Action 2 - AddTrack", action2);
  RegisterAction("IPlugReaperExtension: Action 3 - Show/Hide UI", [&]() { ShowHideMainWindow(); mGUIToggle = !mGUIToggle; }, true, &mGUIToggle);
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT bounds = pGraphics->GetBounds();

    auto GetBounds = [](int ctrlIdx, const IRECT& b) {
      // Controls use a fixed-width column; background fills everything
      IRECT col = b.GetFromTLHC(PLUG_WIDTH, PLUG_WIDTH);
      switch (ctrlIdx) {
        case 0: return b; // background
        case 1: return col.GetGridCell(0, 3, 1).GetPadded(-20.).SubRectVertical(2, 0).GetMidVPadded(20.f);
        case 2: return col.GetGridCell(0, 2, 2).GetPadded(-20.).SubRectVertical(2, 1).GetMidVPadded(20.f);
        case 3: return col.GetGridCell(1, 3, 1);
        case 4: return col.GetGridCell(2, 3, 1).GetPadded(-20.f);
        default: return b;
      }
    };

    // Reposition controls on resize
    if (pGraphics->NControls()) {
      for (int i = 0; i < pGraphics->NControls(); i++)
        pGraphics->GetControl(i)->SetTargetAndDrawRECTs(GetBounds(i, bounds));
      return;
    }

    pGraphics->SetLayoutOnResize(true);
    pGraphics->LoadFont("Roboto-Regular", (void*) ROBOTO_REGULAR, ROBOTO_REGULAR_length);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->AttachControl(new IVButtonControl(GetBounds(1, bounds),
                                                 [&](IControl* pCaller) {
                                                   SplashClickActionFunc(pCaller);
                                                   action2();
                                                 }, "Action 2 - Add Track"));

    pGraphics->AttachControl(new IVButtonControl(GetBounds(2, bounds),
                                                 [&](IControl* pCaller) {
                                                   SplashClickActionFunc(pCaller);
                                                   ToggleDocking();
                                                 }, "Dock"));

    WDL_String str;
    mPrevTrackCount = CountTracks(0);
    str.SetFormatted(64, "NumTracks: %i", mPrevTrackCount);

    pGraphics->AttachControl(new ITextControl(GetBounds(3, bounds), str.Get(), IText(24, EAlign::Center)), kCtrlTagText);

    pGraphics->AttachControl(new IVSliderControl(GetBounds(4, bounds), [](IControl* pCaller) {
                                                   WDL_String valStr;
                                                   valStr.SetFormatted(32, "slider %f\n", pCaller->GetValue());
                                                   ShowConsoleMsg(valStr.Get());
                                                 }, "Value:", DEFAULT_STYLE, true, EDirection::Horizontal));
  };
}

void IPlugReaperExtension::OnIdle()
{
  int tracks = CountTracks(0);
  
  if(tracks != mPrevTrackCount) {
    mPrevTrackCount = tracks;
    
    if(GetUI()) {
      GetUI()->GetControlWithTag(kCtrlTagText)->As<ITextControl>()->SetStrFmt(64, "NumTracks: %i", tracks);
    }
  }
}
