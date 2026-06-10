#include "IPlugVisualizer.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IVBarGraphSpectrumAnalyzerControl.h"

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
//    pGraphics->ShowFPSDisplay(true);
    pGraphics->EnableMouseOver(true);
    pGraphics->SetLayoutOnResize(true);

    IVStyle style = DEFAULT_STYLE
      .WithColor(kBG, COLOR_BLACK)
      .WithColor(kFG, {255, 128, 128, 128})
      .WithLabelText(DEFAULT_LABEL_TEXT.WithFGColor(COLOR_WHITE))
      .WithValueText(DEFAULT_VALUE_TEXT.WithFGColor(COLOR_WHITE));
    pGraphics->AttachControl(new IVSpectrumAnalyzerControl<2>(b, "Spectrum", style), kCtrlTagSpectrumAnalyzer);
    
//    auto* pControl = new IVBarGraphSpectrumAnalyzerControl<2>(
//       b, "Spectrum", DEFAULT_STYLE, 32, 16,
//       IVBarGraphSpectrumAnalyzerControl<2>::EFrequencyScale::Log,
//       IVBarGraphSpectrumAnalyzerControl<2>::EColorMode::Smooth,
//       IVBarGraphSpectrumAnalyzerControl<2>::EChannelMode::Sum,
//       IPattern::CreateLinearGradient(b, EDirection::Vertical,
//                               {{IColor(255, 0, 200, 0), 0.0f},      // Green at bottom
//                                {IColor(255, 0, 200, 0), 0.85f},     // Green until near 0dB
//                                {IColor(255, 200, 200, 0), 0.90f}}), // Yellow just before clip
//       {},        // ledRanges
//       0.2f,      // gapRatio
//       0.1f,      // segGapRatio
//       5.0f,      // attackTimeMs
//       50.0f,     // decayTimeMs
//       -60.f,     // lowRangeDB
//       6.f        // highRangeDB
//    );
//
//    // Enable clip indicator at 0dB with red color
//    pControl->SetClipIndicator(0.f, COLOR_RED);
//
//    pGraphics->AttachControl(pControl, kCtrlTagSpectrumAnalyzer);
    
//    pGraphics->AttachControl(new IVBarGraphSpectrumAnalyzerControl<2>(
//       b, "Spectrum", DEFAULT_STYLE, 32, 16,
//       IVBarGraphSpectrumAnalyzerControl<2>::EFrequencyScale::Log,
//       IVBarGraphSpectrumAnalyzerControl<2>::EColorMode::Segments,
//       IVBarGraphSpectrumAnalyzerControl<2>::EChannelMode::Split,
//       IPattern::CreateLinearGradient(b, EDirection::Vertical,
//                               {{SPEC_LED1, 0.0f},
//                                {SPEC_LED2, 0.3f},
//                                {SPEC_LED3, 0.5f},
//                                {SPEC_LED4, 0.7f},
//                                {SPEC_LED5, 1.0f}})
//     ), kCtrlTagSpectrumAnalyzer);
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
  else if (msgTag == IVSpectrumAnalyzerControl<>::kMsgTagOverlap)
  {
    int overlap = *reinterpret_cast<const int*>(pData);
    mSender.SetFFTSizeAndOverlap(mSender.GetFFTSize(), overlap);
    return true;
  }
  else if (msgTag == IVSpectrumAnalyzerControl<>::kMsgTagWindowType)
  {
    int idx = *reinterpret_cast<const int*>(pData);
    mSender.SetWindowType(static_cast<ISpectrumSender<2>::EWindowType>(idx));
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
  const int nInChans = NInChansConnected();
  const int nOutChans = NOutChansConnected();

  mSender.ProcessBlock(inputs, nFrames, kCtrlTagSpectrumAnalyzer, nInChans);

  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nOutChans; c++) {
      outputs[c][s] = inputs[c % nInChans][s];
    }
  }
}

void IPlugVisualizer::OnReset()
{
  SyncUIControl();
}

bool IPlugVisualizer::SerializeState(IByteChunk &chunk) const
{
  int fftSize = mSender.GetFFTSize();
  int overlap = mSender.GetOverlap();
  int windowType = static_cast<int>(mSender.GetWindowType());
  chunk.Put(&fftSize);
  chunk.Put(&overlap);
  chunk.Put(&windowType);

  return SerializeParams(chunk); // must remember to call SerializeParams at the end
}

int IPlugVisualizer::UnserializeState(const IByteChunk &chunk, int startPos)
{
  int fftSize, overlap, windowType;

  startPos = chunk.Get(&fftSize, startPos);
  startPos = chunk.Get(&overlap, startPos);
  startPos = chunk.Get(&windowType, startPos);

  return UnserializeParams(chunk, startPos);
}

void IPlugVisualizer::SyncUIControl()
{
  auto sr = GetSampleRate();
  auto fftSize = mSender.GetFFTSize();
  auto overlap = mSender.GetOverlap();
  auto windowType = static_cast<int>(mSender.GetWindowType());
  SendControlMsgFromDelegate(kCtrlTagSpectrumAnalyzer, IVSpectrumAnalyzerControl<>::kMsgTagSampleRate, sizeof(double), &sr);
  SendControlMsgFromDelegate(kCtrlTagSpectrumAnalyzer, IVSpectrumAnalyzerControl<>::kMsgTagFFTSize, sizeof(int), &fftSize);
  SendControlMsgFromDelegate(kCtrlTagSpectrumAnalyzer, IVSpectrumAnalyzerControl<>::kMsgTagOverlap, sizeof(int), &overlap);
  SendControlMsgFromDelegate(kCtrlTagSpectrumAnalyzer, IVSpectrumAnalyzerControl<>::kMsgTagWindowType, sizeof(int), &windowType);
}

#endif

#if IPLUG_EDITOR
void IPlugVisualizer::OnParamChangeUI(int paramIdx, EParamSource source)
{
  if (paramIdx == kOctaveGain)
  {
    double octaveGain = GetParam(kOctaveGain)->Value();
    SendControlMsgFromDelegate(kCtrlTagSpectrumAnalyzer, IVSpectrumAnalyzerControl<>::kMsgTagOctaveGain, sizeof(double), &octaveGain);
  }
}

void IPlugVisualizer::OnUIOpen()
{
#if IPLUG_DSP
  SyncUIControl();
#endif
}

#endif
