#include "IPlugVisualizer.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

constexpr int kCtrlTagSpectrumAnalyzer = 0;

IPlugVisualizer::IPlugVisualizer(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
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
    
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->ShowFPSDisplay(true);
    pGraphics->EnableMouseOver(true);
    pGraphics->SetLayoutOnResize(true);

    IVStyle style = DEFAULT_STYLE
      .WithColor(kBG, COLOR_BLACK)
      .WithColor(kFG, {255, 128, 128, 128})
      .WithLabelText(DEFAULT_LABEL_TEXT.WithFGColor(COLOR_WHITE))
      .WithValueText(DEFAULT_VALUE_TEXT.WithFGColor(COLOR_WHITE));
    pGraphics->AttachControl(new IVSpectrumAnalyzerControl<2>(b, "Spectrum", style), kCtrlTagSpectrumAnalyzer);
  };
#endif
}

#if IPLUG_DSP
bool IPlugVisualizer::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if (msgTag == IVSpectrumAnalyzerControl<>::kMsgTagFFTSize)
  {
    int fftSize = *reinterpret_cast<const int*>(pData);
    mSender.SetFFTSize(fftSize);
    return true;
  }

  return false;
}

void IPlugVisualizer::OnIdle()
{
  mSender.TransmitData(*this);
}

void IPlugVisualizer::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mSender.ProcessBlock(inputs, nFrames, kCtrlTagSpectrumAnalyzer, NInChansConnected());
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[0][s];
    }
  }
}

void IPlugVisualizer::OnReset()
{
  auto sr = GetSampleRate();
  auto fftSize = mSender.GetFFTSize();
  SendControlMsgFromDelegate(kCtrlTagSpectrumAnalyzer, IVSpectrumAnalyzerControl<>::kMsgTagSampleRate, sizeof(double), &sr);
  SendControlMsgFromDelegate(kCtrlTagSpectrumAnalyzer, IVSpectrumAnalyzerControl<>::kMsgTagFFTSize, sizeof(int), &fftSize);
}

#endif

#if IPLUG_EDITOR
void IPlugVisualizer::OnParentWindowResize(int width, int height)
{
  if (GetUI())
  {
    GetUI()->Resize(width, height, 1.f, false);
  }
}

void IPlugVisualizer::OnParamChangeUI(int paramIdx, EParamSource source)
{
  if (paramIdx == kOctaveGain)
  {
    double octaveGain = GetParam(kOctaveGain)->Value();
    SendControlMsgFromDelegate(kCtrlTagSpectrumAnalyzer, IVSpectrumAnalyzerControl<>::kMsgTagOctaveGain, sizeof(double), &octaveGain);
  }
}
#endif
