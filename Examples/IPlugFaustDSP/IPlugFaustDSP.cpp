#include "IPlugFaustDSP.h"
#include "IPlug_include_in_plug_src.h"
#include "IPlugFaust_edit.h"

IPlugFaustDSP::IPlugFaustDSP(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, 1, instanceInfo)
{
  InitParamRange(0, kNumParams-1, 0, "Param %i", 0., 0., 1., 0.01, "", IParam::kFlagsNone);
  
  mFaustProcessor.SetMaxChannelCount(MaxNChannels(kInput), MaxNChannels(kOutput));
  mFaustProcessor.Init();
  mFaustProcessor.CompileCPP();
  mFaustProcessor.SetAutoRecompile(true);
  
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, 60, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    IRECT b = pGraphics->GetBounds().GetPadded(-20);
    
    pGraphics->AttachCornerResizer(kUIResizerScale);
    pGraphics->LoadFont(ROBOTTO_FN);
    
    pGraphics->AttachPanelBackground(COLOR_BLACK);
    pGraphics->AttachControl(new IVScopeControl<>(*this, b.GetReducedFromTop(50)), kControlTagScope);
    
#ifndef FAUST_COMPILED
    pGraphics->AttachControl(new IVButtonControl(*this, b.GetFromTRHC(150, 30), [](IControl* pCaller)
                                                 {
                                                   FlashCircleClickActionFunc(pCaller);

                                                   OpenFaustEditorWindow(DSP_FILE);
                                                 }, "Edit FAUST File"));
#endif
  };
}

void IPlugFaustDSP::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mFaustProcessor.ProcessBlock(inputs, outputs, nFrames);
  mScopeBallistics.ProcessBlock(outputs, nFrames);
}

void IPlugFaustDSP::OnReset()
{
  mFaustProcessor.SetSampleRate(GetSampleRate());
}

void IPlugFaustDSP::OnParamChange(int paramIdx)
{
  mFaustProcessor.SetParameterValueNormalised(paramIdx, GetParam(paramIdx)->Value());
}

void IPlugFaustDSP::OnIdle()
{
  mScopeBallistics.TransmitData(*this);
}
