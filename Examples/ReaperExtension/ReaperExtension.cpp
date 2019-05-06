#include "ReaperExtension.h"
#include "ReaperExt_include_in_plug_src.h"

#include "IControls.h"

ReaperExtension::ReaperExtension(reaper_plugin_info_t* pRec)
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
  RegisterAction("Reaper extension: Action 1 - MsgBox", action1, true);
  RegisterAction("Reaper extension: Action 2 - AddTrack", action2);
  RegisterAction("Reaper extension: Action 3 - Show/Hide UI", [&]() { ShowHideMainWindow(); mGUIToggle = !mGUIToggle; }, true, &mGUIToggle);
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT bounds = pGraphics->GetBounds();
    
    if(pGraphics->NControls()) {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(bounds);
      return;
    }
    
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPanelBackground(COLOR_RED);
    pGraphics->AttachCornerResizer();
    pGraphics->SetSizeConstraints(100, 4096, 100, 4096);
    pGraphics->AttachControl(new IVButtonControl(bounds.GetGridCell(0, 2, 2).GetPadded(-20.).SubRectVertical(2, 0).GetMidVPadded(20),
                                                 [&](IControl* pCaller) {
                                                   SplashClickActionFunc(pCaller);
                                                   action2();
                                                 }, "Action 2 - Add Track"));

//    pGraphics->AttachControl(new IVButtonControl(bounds.GetGridCell(0, 2, 2).GetPadded(-20.).SubRectVertical(2, 1).GetMidVPadded(20),
//                                                 [&](IControl* pCaller) {
//                                                   SplashClickActionFunc(pCaller);
//                                                   ToggleDocking();
//                                                 }, "Dock"));
    
    WDL_String str;
    mPrevTrackCount = CountTracks(0);
    str.SetFormatted(64, "NumTracks: %i", mPrevTrackCount);
    
    pGraphics->AttachControl(new ITextControl(bounds.GetGridCell(1, 2, 2), str.Get(), IText(24, IText::kAlignNear)), kCtrlTagText);
    
    pGraphics->AttachControl(new IVSliderControl(bounds.GetGridCell(3, 2, 2).GetPadded(-20), [](IControl* pCaller) {
                                                   WDL_String valStr;
                                                   valStr.SetFormatted(32, "slider %f\n", pCaller->GetValue());
                                                   ShowConsoleMsg(valStr.Get());
                                                 }));
    
//    pGraphics->AttachImGui([](IGraphics* pGraphics) {
//      ImGui::ShowDemoWindow();
//    });
  };
}

void ReaperExtension::OnIdle()
{
  int tracks = CountTracks(0);
  
  if(tracks != mPrevTrackCount) {
    mPrevTrackCount = tracks;
    
    if(GetUI()) {
      dynamic_cast<ITextControl*>(GetUI()->GetControlWithTag(kCtrlTagText))->SetStrFmt(64, "NumTracks: %i", tracks);
    }
  }
}
