#include "IPlugSurroundEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

uint64_t GetAPIBusTypeForChannelIOConfig(int configIdx, ERoute dir, int busIdx, const IOConfig* pConfig, WDL_TypedBuf<uint64_t>* APIBusTypes)
{
  assert(pConfig != nullptr);
  assert(busIdx >= 0 && busIdx < pConfig->NBuses(dir));
  
  int numChans = pConfig->GetBusInfo(dir, busIdx)->NChans();

#if defined AU_API || defined AUv3_API
  switch (numChans)
  {
    case 0: APIBusTypes->Add(kAudioChannelLayoutTag_UseChannelDescriptions | 0); break;
    case 1: APIBusTypes->Add(kAudioChannelLayoutTag_Mono); break;
    case 2: APIBusTypes->Add(kAudioChannelLayoutTag_Stereo); break;
    case 6: APIBusTypes->Add(kAudioChannelLayoutTag_AudioUnit_5_1); break;
    case 8: APIBusTypes->Add(kAudioChannelLayoutTag_AudioUnit_7_1); break;
    default: APIBusTypes->Add(kAudioChannelLayoutTag_DiscreteInOrder | numChans); break;
  }
  return 0;
#elif defined VST3_API
  switch (numChans)
  {
    case 0: return Steinberg::Vst::SpeakerArr::kEmpty;
    case 1: return Steinberg::Vst::SpeakerArr::kMono;
    case 2: return Steinberg::Vst::SpeakerArr::kStereo;
    case 6: return Steinberg::Vst::SpeakerArr::k51;
    case 8: return Steinberg::Vst::SpeakerArr::k71CineSideFill;
    default: return Steinberg::Vst::SpeakerArr::kEmpty;
  }
#elif defined AAX_API
  switch (numChans)
  {
    case 0: return AAX_eStemFormat_None;
    case 1: return AAX_eStemFormat_Mono;
    case 2: return AAX_eStemFormat_Stereo;
    case 6: return AAX_eStemFormat_5_1;
    case 8: return AAX_eStemFormat_7_1_DTS;
    default: return AAX_eStemFormat_None;
  }
#endif
}

IPlugSurroundEffect::IPlugSurroundEffect(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    IRECT b = pGraphics->GetBounds().GetPadded(-10);
    IRECT s = b.ReduceFromRight(50.f);

    const IVStyle meterStyle = DEFAULT_STYLE.WithColor(kFG, COLOR_WHITE.WithOpacity(0.3f));
    pGraphics->AttachControl(new IVMeterControl<8>(b.FracRectVertical(0.5, true), "Inputs", meterStyle, EDirection::Vertical, {"1", "2", "3", "4", "5", "6", "7", "8"}), kCtrlTagInputMeter);
    pGraphics->AttachControl(new IVMeterControl<8>(b.FracRectVertical(0.5, false), "Outputs", meterStyle, EDirection::Vertical, {"1", "2", "3", "4", "5", "6", "7", "8"}), kCtrlTagOutputMeter);
    pGraphics->AttachControl(new IVSliderControl(s, kGain));
  };
#endif
}

#if IPLUG_DSP
void IPlugSurroundEffect::OnIdle()
{
  mInputPeakSender.TransmitData(*this);
  mOutputPeakSender.TransmitData(*this);
}

void IPlugSurroundEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }

  mInputPeakSender.ProcessBlock(inputs, nFrames, kCtrlTagInputMeter);
  mOutputPeakSender.ProcessBlock(outputs, nFrames, kCtrlTagOutputMeter);
}
#endif
