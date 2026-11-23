#include "IPlugSpectrogramDisplay.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "ISpectrogramControl.h"
#endif

constexpr int kCtrlTagSpectrogram = 0;

IPlugSpectrogramDisplay::IPlugSpectrogramDisplay(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kColorMapRange)->InitDouble("ColorMapRange", 50.0, 0., 100.0, 0.01, "%");
  GetParam(kColorMapContrast)->InitDouble("ColorMapContrast", 50.0, 0., 100.0, 0.01, "%");
 
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
      auto r = b.GetFromTop(100).GetCentredInside(100, 30);
      pGraphics->GetControl(2)->SetTargetAndDrawRECTs(r.GetGridCell(0, 1, 3));
      pGraphics->GetControl(3)->SetTargetAndDrawRECTs(r.GetGridCell(2, 1, 3));
      return;
    }
    
    pGraphics->EnableMouseOver(true);
    pGraphics->SetLayoutOnResize(true);
//    pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachControl(new ISpectrogramControl(b), kCtrlTagSpectrogram);
    auto r = b.GetFromTop(100).GetCentredInside(100, 30);
    auto s = DEFAULT_STYLE
      .WithDrawShadows(false)
      .WithColor(kFG, COLOR_GRAY.WithOpacity(0.2))
      .WithColor(kPR, COLOR_GRAY.WithOpacity(0.5))
      .WithColor(kX1, COLOR_GRAY.WithOpacity(0.5))
      .WithLabelText(IText(10, COLOR_WHITE))
      .WithValueText(DEFAULT_VALUE_TEXT.WithFGColor(COLOR_WHITE))
      .WithShowValue(false)
      .WithShowLabel(false);

    pGraphics->AttachControl(new IVKnobControl(r.GetGridCell(0, 1, 3), kColorMapRange, "Range", s));
    pGraphics->AttachControl(new IVKnobControl(r.GetGridCell(2, 1, 3), kColorMapContrast, "Contrast", s));
#ifdef OS_LINUX
    pGraphics->AttachPopupMenuControl();
#endif
    // For mouse over spectrogram getting freq and time values
    pGraphics->EnableMouseOver(true);
  };
#endif
}

#if IPLUG_DSP
void IPlugSpectrogramDisplay::OnReset()
{
}

bool IPlugSpectrogramDisplay::OnMessage(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  if (msgTag == ISpectrogramControl::kMsgTagFFTSize)
  {
    mFFTSize = *reinterpret_cast<const int*>(pData);
    mSender.SetFFTSizeAndOverlap(mFFTSize, mOverlap);
    return true;
  }
  else if (msgTag == ISpectrogramControl::kMsgTagOverlap)
  {
    mOverlap = *reinterpret_cast<const int*>(pData);
    mSender.SetFFTSizeAndOverlap(mFFTSize, mOverlap);
    return true;
  }

  return false;
}
#endif

void IPlugSpectrogramDisplay::OnIdle()
{
//#if IPLUG_EDITOR
//#if !defined APP_API && !defined WEB_API
//  bool scrollEnabled = (IsTransportPlaying() && !GetBypassed());
//  SendControlMsgFromDelegate(kCtrlTagSpectrogram, ISpectrogramControl::kMsgTagScrollEnabled, sizeof(bool), &scrollEnabled);
//#endif
//#endif

#if IPLUG_DSP
//  auto sr = GetSampleRate();
//  SendControlMsgFromDelegate(kCtrlTagSpectrogram, ISpectrogramControl::kMsgTagSampleRate, sizeof(double), &sr);
  mSender.TransmitData(*this);
#endif
}

#if IPLUG_DSP
void IPlugSpectrogramDisplay::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mSender.ProcessBlock(inputs, nFrames, kCtrlTagSpectrogram);
}
#endif

#if IPLUG_EDITOR
void IPlugSpectrogramDisplay::OnParentWindowResize(int width, int height)
{
  if (GetUI())
  {
    GetUI()->Resize(width, height, 1.f, false);
  }
}

void IPlugSpectrogramDisplay::OnParamChangeUI(int paramIdx, EParamSource source)
{
  if (paramIdx == kColorMapRange)
  {
    double colorMapRange = GetParam(kColorMapRange)->GetNormalized();
    SendControlMsgFromDelegate(kCtrlTagSpectrogram, ISpectrogramControl::kMsgTagColorMapRange, sizeof(double), &colorMapRange);
  }
  else if (paramIdx == kColorMapContrast)
  {
    double colorMapContrast = GetParam(kColorMapContrast)->GetNormalized();
    SendControlMsgFromDelegate(kCtrlTagSpectrogram, ISpectrogramControl::kMsgTagColorMapContrast, sizeof(double), &colorMapContrast);
  }
  
  //  auto overlap = mSender.GetOverlap();
  //  auto fftSize = mSender.GetFFTSize();
  //  SendControlMsgFromDelegate(kCtrlTagSpectrogram, ISpectrogramControl::kMsgTagFFTSize, sizeof(int), &fftSize);
  //  SendControlMsgFromDelegate(kCtrlTagSpectrogram, ISpectrogramControl::kMsgTagOverlap, sizeof(int), &overlap);
}
#endif
