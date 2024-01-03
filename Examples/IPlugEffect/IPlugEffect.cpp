#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

class ScrollerControl : public IContainerBase
{
public:
  ScrollerControl(const IRECT& bounds)
  : IContainerBase(bounds)
  {}
  
  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_LIGHT_GRAY, mRECT);
  }
  
  void OnAttached() override
  {
    auto sliderStyle = DEFAULT_STYLE.WithColor(kX1, COLOR_TRANSPARENT).WithDrawShadows(false).WithRoundness(1);

    AddChildControl(new IVSliderControl(IRECT(), [this](IControl* pCaller) {
      mYOffset = pCaller->GetValue();
      this->GetChild(1)->SetTargetAndDrawRECTs(mRECT.GetReducedFromRight(20));
      SetDirty(false);
    }, "", sliderStyle, false, EDirection::Vertical, DEFAULT_GEARING, 8.f, 8.f, true));
    
    auto buttonStyle = DEFAULT_STYLE.WithDrawFrame(false).WithColor(kSH, COLOR_BLACK).WithShadowOffset(1).WithLabelText({25.f, EVAlign::Middle});

    AddChildControl(new IContainerBase(IRECT(), [&](IContainerBase* pCaller, const IRECT& r){
      for (auto i = 0; i < (this->mNumRows*2); i++)
      {
        pCaller->AddChildControl(new IVButtonControl(IRECT(), SplashClickActionFunc, "Hello World", buttonStyle, true))->SetIgnoreMouse(true);
      }
    }, [&](IContainerBase* pCaller, const IRECT& r){
      for (auto i = 0; i < (this->mNumRows*2); i++)
      {
        pCaller->GetChild(i)->SetTargetAndDrawRECTs(r.SubRectVertical(mNumRows, i).GetVShifted(-r.H() + (mYOffset * r.H())));
      }
    }));
    
    OnResize();
  }

  void OnResize() override
  {
    if (NChildren())
    {
      GetChild(0)->SetTargetAndDrawRECTs(mRECT);
      GetChild(1)->SetTargetAndDrawRECTs(mRECT.GetReducedFromRight(20));
    }
  }
  
private:
  float mYOffset = 0.0;
  const int mNumRows = 10;
  //void OnMouseDown(float x, float y, const IMouseMod& mod) override {}
  //void OnMouseUp(float x, float y, const IMouseMod& mod) override {}
  //void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override {}
};

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
    pGraphics->AttachControl(new ScrollerControl(b.GetCentredInside(300)));
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
