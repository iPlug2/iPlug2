#include "IPlugChromogram.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IVChromogramControl.h"

constexpr int kCtrlTagSpectralDisplay = 0;

IPlugChromogram::IPlugChromogram(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kOctaveGain)->InitDouble("OctaveGain", 0.0, 0., 12.0, 0.1, "dB");
  
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
    
    pGraphics->SetLayoutOnResize(true);
//    pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    pGraphics->AttachControl(new IVChromogramControl<>(b), kCtrlTagSpectralDisplay);
    
//    pGraphics->AttachPopupMenuControl();
    // For mouse over spectrum getting freq and db values
    pGraphics->EnableMouseOver(true);
  };
#endif
}

bool IPlugChromogram::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if (msgTag == IVChromogramControl<>::kMsgTagFFTSize)
  {
    int fftSize = *reinterpret_cast<const int*>(pData);
    mSender.SetFFTSize(fftSize);
    return true;
  }

  return false;
}

void IPlugChromogram::OnIdle()
{
  // Octave gain
  double octaveGain = GetParam(kOctaveGain)->Value();
  SendControlMsgFromDelegate(kCtrlTagSpectralDisplay, IVChromogramControl<>::kMsgTagOctaveGain, sizeof(double), &octaveGain);
  
  mSender.TransmitData(*this);
}

#if IPLUG_DSP
void IPlugChromogram::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mSender.ProcessBlock(inputs, nFrames, kCtrlTagSpectralDisplay, NInChansConnected());
}
#endif

void IPlugChromogram::OnReset()
{
  auto sr = GetSampleRate();
  auto fftSize = mSender.GetFFTSize();
  SendControlMsgFromDelegate(kCtrlTagSpectralDisplay, IVChromogramControl<>::kMsgTagSampleRate, sizeof(double), &sr);
  SendControlMsgFromDelegate(kCtrlTagSpectralDisplay, IVChromogramControl<>::kMsgTagFFTSize, sizeof(int), &fftSize);
}

#if IPLUG_EDITOR
void IPlugChromogram::OnParentWindowResize(int width, int height)
{
  if (GetUI())
  {
    GetUI()->Resize(width, height, 1.f, false);
  }
}
#endif
