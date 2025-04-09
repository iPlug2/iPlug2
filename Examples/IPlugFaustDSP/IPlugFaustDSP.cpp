#include "IPlugFaustDSP.h"
#include "IPlug_include_in_plug_src.h"

IPlugFaustDSP::IPlugFaustDSP(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, 1))
, mUIBuilder(mFaustProcessor, DEFAULT_STYLE
                    .WithDrawShadows(false)
                    .WithLabelText({15})
                    .WithShowValue(false))
{
  InitParamRange(0, kNumParams-1, 1, "Param %i", 0., 0., 1., 0.01, "", IParam::kFlagsNone); // initialize kNumParams generic iplug params
  
#if IPLUG_DSP
  mFaustProcessor.SetMaxChannelCount(MaxNChannels(ERoute::kInput), MaxNChannels(ERoute::kOutput));
  mFaustProcessor.Init();
  mFaustProcessor.CompileCPP();
  mFaustProcessor.SetAutoRecompile(true);
  mFaustProcessor.CreateIPlugParameters(this);
#ifndef FAUST_COMPILED
  mFaustProcessor.SetCompileFunc([&](){
    GetUI()->RemoveAllControls();
    LayoutUI(GetUI());
    OnParamReset(EParamSource::kRecompile);
    mFaustProcessor.SyncFaustParams();  });
#endif
#endif
  
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    IRECT b = pGraphics->GetBounds().GetPadded(-20);

    IRECT ui = b.GetPadded(-10);
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPanelBackground(COLOR_GRAY);

    pGraphics->AttachControl(mUIBuilder.CreateFaustUIContainer(ui));

//    for (int i = 0; i < kNumParams; i++) {
//      pGraphics->AttachControl(new IVKnobControl(knobs.GetGridCell(i, 1, kNumParams).GetPadded(-5.f), i));
//    }
//    
//    pGraphics->AttachControl(new IVScopeControl<2>(scope, "", DEFAULT_STYLE.WithColor(kBG, COLOR_BLACK).WithColor(kFG, COLOR_GREEN)), kCtrlTagScope);
//    pGraphics->AttachControl(new IVKeyboardControl(keyb));
  };
#endif
}

#if IPLUG_DSP

void IPlugFaustDSP::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mFaustProcessor.ProcessBlock(inputs, outputs, nFrames);
  mScopeSender.ProcessBlock(outputs, nFrames, kCtrlTagScope);
}

void IPlugFaustDSP::OnReset()
{
  mFaustProcessor.SetSampleRate(GetSampleRate());
}

void IPlugFaustDSP::ProcessMidiMsg(const IMidiMsg& msg)
{
  mFaustProcessor.ProcessMidiMsg(msg);
}

void IPlugFaustDSP::OnParamChange(int paramIdx)
{
//  mFaustProcessor.SetParameterValueNormalised(paramIdx, GetParam(paramIdx)->GetNormalized());
}

void IPlugFaustDSP::OnIdle()
{
//  mScopeSender.TransmitData(*this);
}
#endif
