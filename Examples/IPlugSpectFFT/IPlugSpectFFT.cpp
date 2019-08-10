#include "IPlugSpectFFT.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

IPlugSpectFFT::IPlugSpectFFT(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  //adding new FFT class with size and overlap, and setting the window function
 // const int fftSize = 4096;
  GetParam(kGain)->InitDouble("Gain", 0., -24., 24., 0.01, "dB", IParam::kFlagsNone, "", IParam::ShapePowCurve(3.));
  mGain = 1.;

#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {

    pGraphics->HandleMouseOver(true);
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    const IVStyle style{
    true, // Show label
    true, // Show value
    {
      DEFAULT_BGCOLOR, // Background
      DEFAULT_FGCOLOR, // Foreground
      DEFAULT_PRCOLOR, // Pressed
      COLOR_BLACK, // Frame
      DEFAULT_HLCOLOR, // Highlight
      DEFAULT_SHCOLOR, // Shadow
      COLOR_BLACK, // Extra 1
      DEFAULT_X2COLOR, // Extra 2
      DEFAULT_X3COLOR  // Extra 3
    }, // Colors
    IText(12.f, EAlign::Center) // Label text
    };

    const IBitmap knob = pGraphics->LoadBitmap(KNOB_FN, 60);
    pGraphics->AttachControl(new IBKnobControl(20., 20., knob,  kGain));

    const IRECT iView(80, 20, pGraphics->GetBounds().R - 10, pGraphics->GetBounds().B - 10);
    pFFTAnalyzer = pGraphics->AttachControl(new gFFTAnalyzer<>(iView), kCtrlTagFFT, "FFT");

    const IText textLabel{ 14, COLOR_BLACK, "Roboto-Regular", EAlign::Center, EVAlign::Middle, 0 };
    pFFTFreqDraw = pGraphics->AttachControl(new gFFTFreqDraw(iView, textLabel), -1, "FFT");


    dynamic_cast<gFFTAnalyzer<>*>(pFFTAnalyzer)->getFFT()->SetWindowType(Spect_FFT::win_BlackmanHarris);

    //setting the min/max freq for fft display and freq lines
   constexpr double maxF = 21000.;
   constexpr double minF = 15.;
   dynamic_cast<gFFTAnalyzer<>*>(pFFTAnalyzer)->SetMaxFreq(maxF);
     dynamic_cast<gFFTAnalyzer<>*>(pFFTAnalyzer)->SetMinFreq(minF);
     dynamic_cast<gFFTFreqDraw*>(pFFTFreqDraw)->SetMaxFreq(maxF);
     dynamic_cast<gFFTFreqDraw*>(pFFTFreqDraw)->SetMinFreq(minF);

    //setting +3dB/octave compensation to the fft display.  Most use +3.  Voxengo Span uses +4.5.
     dynamic_cast<gFFTAnalyzer<>*>(pFFTAnalyzer)->SetOctaveGain(3., true);

  };
#endif
}

#if IPLUG_DSP
void IPlugSpectFFT::OnIdle()
{
  mSender.TransmitData(*this);
}

void IPlugSpectFFT::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nChans = NOutChansConnected();
  double mGain = GetParam(kGain)->DBToAmp();
  double output = 0.;

  for (int s = 0; s < nFrames; s++) {
    output = 0.;
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * mGain ;
    }
  }
  mSender.ProcessBlock(outputs, nFrames, nChans);
}

void IPlugSpectFFT::OnReset()
{
//  dynamic_cast<gFFTAnalyzer*>(pFFTAnalyzer)->SetSampleRate(this->GetSampleRate());
}

void IPlugSpectFFT::OnParamChange(int paramIdx)
{
  switch (paramIdx)
  {
  case kGain:
    mGain = GetParam(kGain)->DBToAmp();
    break;
  default:
    break;
  }
}

#endif
