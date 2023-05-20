#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IVTabbedPagesControl.h"

IPlugEffect::IPlugEffect(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->AttachBubbleControl();
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    
    const IRECT b = pGraphics->GetBounds();
    
    auto resizeFunc = [](IContainerBase* pCaller, const IRECT& r) {
      auto innerBounds = r.GetPadded(-10);
      pCaller->GetChild(0)->SetTargetAndDrawRECTs(innerBounds.SubRectHorizontal(3 , 0));
    };
    
    auto subControls =  std::map<const char*, IVTabbedPageBase*>
    {
      {"1", new IVTabbedPageBase([](IContainerBase* pParent, const IRECT& r) {
        pParent->AddChildControl(new IVKnobControlWithMarks(IRECT(), kGain, "Knob"))
        ->SetActionFunction(ShowBubbleHorizontalActionFunc);
      }, resizeFunc)},
      {"2", new IVTabbedPageBase([](IContainerBase* pParent, const IRECT& r) {
        pParent->AddChildControl(new IVSliderControlWithMarks(IRECT(), kGain, "Slider"))
        ->SetActionFunction(ShowBubbleHorizontalActionFunc);
      }, resizeFunc)},
      {"3", new IVTabbedPageBase([resizeFunc](IContainerBase* pParent, const IRECT& r) {
      }, resizeFunc)}
    };
    
    pGraphics->AttachControl(new IVTabbedPagesControl(b.GetPadded(-10).FracRectVertical(0.5, true),
    {
      {"1", new IVTabbedPageBase([](IContainerBase* pParent, const IRECT& r) {
        pParent->AddChildControl(new IVKnobControlWithMarks(IRECT(), kGain, "Knob"))
                                  ->SetActionFunction(ShowBubbleHorizontalActionFunc);
                                }, resizeFunc)},
      {"2", new IVTabbedPageBase([](IContainerBase* pParent, const IRECT& r) {
        pParent->AddChildControl(new IVSliderControlWithMarks(IRECT(), kGain, "Slider"))
                                  ->SetActionFunction(ShowBubbleHorizontalActionFunc);
                                }, resizeFunc)},
      {"3", new IVTabbedPageBase([subControls](IContainerBase* pParent, const IRECT& r) {
        pParent->AddChildControl(new IVTabbedPagesControl(r.GetPadded(-10).FracRectVertical(0.5, true), subControls));
      }, resizeFunc)}
    }));
    
    pGraphics->AttachControl(new IVSliderControlWithMarks(b.GetFromBottom(100), kGain, "Horizontal", DEFAULT_STYLE, true, EDirection::Horizontal));
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
