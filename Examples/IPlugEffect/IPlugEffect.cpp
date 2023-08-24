#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugEffect::IPlugEffect(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();

    static IPopupMenu menu {"mymenu", {}, [](IPopupMenu* pMenu){
      
      if (pMenu->GetChosenItem())
        pMenu->CheckItemAlone(pMenu->GetChosenItem());
    }};
        
    for (auto i=0; i<100; i++)
    {
      if (i==10)
      {
        menu.AddItem("Item_10", new IPopupMenu("submenu", {"one", "two", "three"}));
      }
      else
      {
        WDL_String itemStr;
        itemStr.SetFormatted(32, "Item_%i", i);
        menu.AddItem(itemStr.Get());
      }
    }
    
    menu.SetChosenItemIdx(0);
    
    pGraphics->AttachControl(new IVButtonControl(b.GetCentredInside(100, 30)))->SetAnimationEndActionFunction([&](IControl* pControl){
      pControl->GetUI()->CreatePopupMenu(*pControl, menu, pControl->GetRECT());
    });
    
    pGraphics->AttachPopupMenuControl({20});
  };
#endif
}

#if IPLUG_DSP
void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
#endif
