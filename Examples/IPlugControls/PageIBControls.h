#pragma once

#include "IVTabbedPagesControl.h"

using namespace iplug;
using namespace igraphics;

class PageIBControls : public IVTabbedPageBase
{
public:
  PageIBControls()
  : IVTabbedPageBase()
  {
  }
  
  void OnAttached() override
  {
   const IBitmap knobBitmap = GetUI()->LoadBitmap(PNGKNOB_FN, 60);
   const IBitmap knobRotateBitmap = GetUI()->LoadBitmap(PNGKNOBROTATE_FN);
   const IBitmap switchBitmap = GetUI()->LoadBitmap((PNGSWITCH_FN), 2, true);
   const IBitmap buttonBitmap = GetUI()->LoadBitmap(PNGBUTTON_FN, 10);
   const IBitmap sliderHandleBitmap = GetUI()->LoadBitmap(PNGSLIDERHANDLE_FN);
   const IBitmap sliderTrackBitmap = GetUI()->LoadBitmap(PNGSLIDERTRACK_FN);
   const IBitmap bitmapText = GetUI()->LoadBitmap(PNGTEXT_FN, 95, true);

   //AddLabel("IBKnobControl");
   AddChildControl(new IBKnobControl(IRECT(), knobBitmap, kParamGain), kNoTag, "bcontrols");
   //AddLabel("IBKnobRotaterControl");
   AddChildControl(new IBKnobRotaterControl(IRECT(), knobRotateBitmap, kParamGain), kNoTag, "bcontrols");
   //AddLabel("IBButtonControl");
   AddChildControl(new IBButtonControl(IRECT(), buttonBitmap, [](IControl* pCaller) {
     pCaller->SetAnimation([](IControl* pCaller){
       auto progress = pCaller->GetAnimationProgress();
       if (progress > 1.) {
         pCaller->OnEndAnimation();
         return;
       }
       pCaller->SetValue(Clip(progress + 0.5, 0.0, 1.0));
     }, 100);
   }), kNoTag, "bcontrols");
//    //AddLabel("IBSwitchControl");
   AddChildControl(new IBSwitchControl(IRECT(), switchBitmap), kNoTag, "bcontrols");
   //AddLabel("IBSliderControl");
   AddChildControl(new IBSliderControl(IRECT(), sliderHandleBitmap, sliderTrackBitmap, kParamGain, EDirection::Vertical), kNoTag, "bcontrols");
   //AddChildControl(new IVGroupControl("Bitmap Controls", "bcontrols", 10.f, 30.f, 30.f, 10.f));
   //AddLabel("IBTextControl");
   AddChildControl(new IBTextControl(IRECT(), bitmapText, DEFAULT_LABEL_TEXT, "HELLO", 10, 16, 0, false));
  }
  
  void OnResize() override
  {
    ForAllChildrenFunc([this](int childIdx, IControl* pControl) {
      pControl->SetTargetAndDrawRECTs(this->mRECT.GetPadded(-10.0f).GetGridCell(childIdx, 4, 5).GetPadded(-10.0f));
    });
  }
};
