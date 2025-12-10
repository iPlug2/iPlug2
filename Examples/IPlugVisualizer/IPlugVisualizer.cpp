#include "IPlugVisualizer.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IVGoniometerControl.h"

constexpr int kCtrlTagSpectrumAnalyzer = 0;
constexpr int kCtrlTagGoniometer = 1;

IPlugVisualizer::IPlugVisualizer(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kOctaveGain)->InitDouble("OctaveGain", 0.0, 0., 12.0, 0.1, "dB");
  GetParam(kOscFreq)->InitDouble("Osc Freq", 440.0, 20.0, 2000.0, 0.1, "Hz");
  GetParam(kPhaseOffset)->InitDouble("Phase Offset", 0.0, -180.0, 180.0, 0.1, "deg");
  GetParam(kNoiseLevel)->InitDouble("Noise Level", -60.0, -60.0, 0.0, 0.1, "dB");
  GetParam(kOscLevel)->InitDouble("Osc Level", -6.0, -60.0, 0.0, 0.1, "dB");
  
#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT b = pGraphics->GetBounds();

    if (pGraphics->NControls())
    {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(b);

      const auto mainArea = b.GetPadded(-10);
      const auto controlsArea = mainArea.GetFromRight(180);
      const auto visualArea = mainArea.GetReducedFromRight(190);

      pGraphics->GetControl(1)->SetTargetAndDrawRECTs(visualArea.GetFromTop(visualArea.H() * 0.6f));
      pGraphics->GetControl(2)->SetTargetAndDrawRECTs(visualArea.GetFromBottom(visualArea.H() * 0.4f));

      // Reposition controls
      for (int i = 3; i < pGraphics->NControls(); i++) {
        auto* ctrl = pGraphics->GetControl(i);
        if (ctrl) {
          int paramIdx = i - 3;
          if (paramIdx < kNumParams) {
            ctrl->SetTargetAndDrawRECTs(controlsArea.GetGridCell(paramIdx, kNumParams, 1).GetPadded(-2));
          }
        }
      }
      return;
    }

    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->ShowFPSDisplay(true);
    pGraphics->EnableMouseOver(true);
    pGraphics->SetLayoutOnResize(true);

    IVStyle style = DEFAULT_STYLE
      .WithColor(kBG, COLOR_TRANSPARENT)
      .WithColor(kFG, {255, 128, 128, 128})
      .WithLabelText(DEFAULT_LABEL_TEXT.WithFGColor(COLOR_WHITE))
      .WithValueText(DEFAULT_VALUE_TEXT.WithFGColor(COLOR_WHITE));

    const auto mainArea = b.GetPadded(-10);
    const auto controlsArea = mainArea.GetFromRight(180);
    const auto visualArea = mainArea.GetReducedFromRight(190);

    pGraphics->AttachControl(new IVSpectrumAnalyzerControl<2>(visualArea.GetFromTop(visualArea.H() * 0.6f), "Spectrum", style.WithColor(kBG, COLOR_BLACK)), kCtrlTagSpectrumAnalyzer);
    pGraphics->AttachControl(new IVGoniometerControl<512>(visualArea.GetFromBottom(visualArea.H() * 0.4f), "Stereo", style.WithColor(kBG, COLOR_BLACK), true, IVGoniometerControl<>::EMode::Dots), kCtrlTagGoniometer);

    // Add parameter controls
    for (int i = 0; i < kNumParams; i++) {
      pGraphics->AttachControl(new IVSliderControl(controlsArea.GetGridCell(i, kNumParams, 1).GetPadded(-2), i, "", style, false, EDirection::Horizontal));
    }
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
  mGoniometerSender.TransmitData(*this);
}

void IPlugVisualizer::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nChans = NOutChansConnected();
  const double freq = GetParam(kOscFreq)->Value();
  const double phaseOffsetDeg = GetParam(kPhaseOffset)->Value();
  const double noiseLevel = GetParam(kNoiseLevel)->DBToAmp();
  const double oscLevel = GetParam(kOscLevel)->DBToAmp();

  const double phaseIncrement = 2.0 * PI * freq / mSampleRate;
  const double phaseOffsetRad = phaseOffsetDeg * PI / 180.0;

  for (int s = 0; s < nFrames; s++) {
    // Generate oscillator
    const double oscL = std::sin(mPhase) * oscLevel;
    const double oscR = std::sin(mPhase + phaseOffsetRad) * oscLevel;

    // Generate noise
    const double noiseL = ((std::rand() / (double)RAND_MAX) * 2.0 - 1.0) * noiseLevel;
    const double noiseR = ((std::rand() / (double)RAND_MAX) * 2.0 - 1.0) * noiseLevel;

    // Mix and output
    if (nChans >= 2) {
      outputs[0][s] = oscL + noiseL;
      outputs[1][s] = oscR + noiseR;
    }
    else if (nChans == 1) {
      outputs[0][s] = (oscL + oscR) * 0.5 + (noiseL + noiseR) * 0.5;
    }

    mPhase += phaseIncrement;
    if (mPhase >= 2.0 * PI)
      mPhase -= 2.0 * PI;
  }

  mSender.ProcessBlock(outputs, nFrames, kCtrlTagSpectrumAnalyzer, nChans);
  mGoniometerSender.ProcessBlock(outputs, nFrames, kCtrlTagGoniometer);
}

void IPlugVisualizer::OnReset()
{
  mSampleRate = GetSampleRate();
  mPhase = 0.0;

  auto sr = GetSampleRate();
  auto fftSize = mSender.GetFFTSize();
  SendControlMsgFromDelegate(kCtrlTagSpectrumAnalyzer, IVSpectrumAnalyzerControl<>::kMsgTagSampleRate, sizeof(double), &sr);
  SendControlMsgFromDelegate(kCtrlTagSpectrumAnalyzer, IVSpectrumAnalyzerControl<>::kMsgTagFFTSize, sizeof(int), &fftSize);

  mGoniometerSender.Reset(GetSampleRate());
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
#endif
