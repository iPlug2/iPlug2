#include "IPlugSpectralDisplay.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

constexpr int kCtrlTagSpectralDisplay = 0;

IPlugSpectralDisplay::IPlugSpectralDisplay(const InstanceInfo& info)
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
    
    pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    IVStyle style = DEFAULT_STYLE.WithColor(kBG, COLOR_BLACK).WithColor(kFG, {255, 128, 128, 128}).WithLabelText(DEFAULT_LABEL_TEXT.WithFGColor(COLOR_WHITE)).WithValueText(DEFAULT_VALUE_TEXT.WithFGColor(COLOR_WHITE));
    pGraphics->AttachControl(new IVSpectrumAnalyzerControl<2>(b, "Spectrum", style), kCtrlTagSpectralDisplay);
    
//    pGraphics->AttachPopupMenuControl();
    // For mouse over spectrum getting freq and db values
    pGraphics->EnableMouseOver(true);
  };
#endif
}

bool IPlugSpectralDisplay::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if (msgTag == IVSpectrumAnalyzerControl<>::kMsgTagFFTSize)
  {
    int fftSize = *reinterpret_cast<const int*>(pData);
    mSender.SetFFTSize(fftSize);
    return true;
  }

  return false;
}

void IPlugSpectralDisplay::OnIdle()
{
  mSender.TransmitData(*this);
}

#if IPLUG_DSP
void IPlugSpectralDisplay::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mSender.ProcessBlock(inputs, nFrames, kCtrlTagSpectralDisplay, NInChansConnected());
}
#endif

void IPlugSpectralDisplay::OnReset()
{
  auto sr = GetSampleRate();
  auto fftSize = mSender.GetFFTSize();
  SendControlMsgFromDelegate(kCtrlTagSpectralDisplay, IVSpectrumAnalyzerControl<>::kMsgTagSampleRate, sizeof(double), &sr);
  SendControlMsgFromDelegate(kCtrlTagSpectralDisplay, IVSpectrumAnalyzerControl<>::kMsgTagFFTSize, sizeof(int), &fftSize);
}

#if IPLUG_EDITOR
void IPlugSpectralDisplay::OnParentWindowResize(int width, int height)
{
  if (GetUI())
  {
    GetUI()->Resize(width, height, 1.f, false);
  }
}

void IPlugSpectralDisplay::OnParamChangeUI(int paramIdx, EParamSource source)
{
  if (paramIdx == kOctaveGain)
  {
    double octaveGain = GetParam(kOctaveGain)->Value();
    SendControlMsgFromDelegate(kCtrlTagSpectralDisplay, IVSpectrumAnalyzerControl<>::kMsgTagOctaveGain, sizeof(double), &octaveGain);
  }
}
#endif
