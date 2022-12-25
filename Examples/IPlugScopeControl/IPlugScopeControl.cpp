#include "IPlugScopeControl.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IVScopeControl.h"
#endif

constexpr int kCtrlTagScope = 0;

IPlugScopeControl::IPlugScopeControl(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT b = pGraphics->GetBounds();

    if (pGraphics->NControls())
    {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(b);
      pGraphics->GetControl(1)->SetTargetAndDrawRECTs(b);
      return;
    }
    
    pGraphics->EnableMouseOver(true);
    pGraphics->SetLayoutOnResize(true);
    pGraphics->AttachCornerResizer(EUIResizerMode::Size, true); //
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
//    auto r = b.GetFromTop(100).GetCentredInside(100, 30);
    auto s = DEFAULT_STYLE
      .WithDrawShadows(false)
      .WithColor(kFG, COLOR_BLACK)
      .WithLabelText(IText(10, COLOR_WHITE))
      .WithValueText(DEFAULT_VALUE_TEXT.WithFGColor(COLOR_WHITE))
      .WithShowValue(false)
      .WithShowLabel(false);
    pGraphics->AttachControl(new IVScopeControl<2, BUFFER_SIZE * 2>(b, "Scope", s), kCtrlTagScope);
//
  };
#endif
}

void IPlugScopeControl::OnIdle()
{
#if IPLUG_DSP
  mSender.TransmitData(*this);
#endif
}

#if IPLUG_DSP
void IPlugScopeControl::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mSender.ProcessBlock(inputs, nFrames, kCtrlTagScope);
}
#endif

#if IPLUG_EDITOR
void IPlugScopeControl::OnParentWindowResize(int width, int height)
{
  if (GetUI())
  {
    GetUI()->Resize(width, height, 1.f, false);
  }
}
#endif
