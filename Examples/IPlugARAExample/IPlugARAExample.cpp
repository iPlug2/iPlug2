#include "IPlugARAExample.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#include "IPlugARAExample_Controls.h"
#endif

IPlugARAExample::IPlugARAExample(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(IColor(255, 24, 26, 30));
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    const IRECT bounds = pGraphics->GetBounds().GetPadded(-8.f);
    const IRECT title = bounds.GetFromTop(22.f);
    const IRECT footer = bounds.GetFromBottom(80.f);
    const IRECT timeline = IRECT(bounds.L, title.B + 4.f, bounds.R, footer.T - 4.f);

    pGraphics->AttachControl(new ITextControl(title, "IPlugARAExample - ARA 2 demo", IText(16.f, COLOR_WHITE, "Roboto-Regular", EAlign::Near, EVAlign::Middle)));
    pGraphics->AttachControl(new ARATimelineControl(timeline, *this));
    const IVStyle sliderStyle = DEFAULT_STYLE
      .WithLabelText(IText(14.f, COLOR_WHITE, "Roboto-Regular", EAlign::Center, EVAlign::Middle))
      .WithValueText(IText(12.f, COLOR_WHITE, "Roboto-Regular", EAlign::Center, EVAlign::Middle));
    pGraphics->AttachControl(new IVSliderControl(footer.GetFromRight(220.f).GetMidVPadded(30.f), kGain, "Gain", sliderStyle, false, EDirection::Horizontal));
    pGraphics->AttachControl(new IMultiLineTextControl(footer.GetReducedFromRight(230.f),
      "As an ARA playback renderer this plug-in replaces its input with the audio of the playback regions assigned by the host, "
      "stretched to the region borders and with the audio modification's edits applied. It analyses each audio source on a background "
      "thread (reporting progress to the host) and exports the detected notes back to the host as ARA content.\n"
      "Click a region to move the host transport there, alt-click to audition it via the ARA editor renderer, right-click to reverse it.",
      IText(12.f, IColor(255, 150, 150, 150), "Roboto-Regular", EAlign::Near, EVAlign::Top)));
  };
#endif
}

#if IPLUG_DSP
void IPlugARAExample::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nChans = NOutChansConnected();
  const double gain = GetParam(kGain)->Value() / 100.;

  mPlayheadPos.store(GetSamplePos() / GetSampleRate(), std::memory_order_relaxed);
  mTransportRunning.store(GetTransportIsRunning(), std::memory_order_relaxed);

  if (auto* pRenderer = GetARAPlaybackRenderer<IPlugARAExamplePlaybackRenderer>())
  {
    // when fulfilling the ARA playback renderer role, the output is the audio of the
    // playback regions that the host has assigned to this instance
    pRenderer->RenderPlaybackRegions(outputs, nChans, GetSampleRate(), static_cast<ARA::ARASamplePosition>(GetSamplePos()), nFrames, GetTransportIsRunning());
  }
  else
  {
    // in any other role (e.g. ARA editor renderer only, or non-ARA operation) pass the input
    // through - per the ARA spec an editor renderer that is not also a playback renderer forwards
    // its input and adds its preview signal on top
    for (int c = 0; c < nChans; c++)
      std::copy_n(inputs[c], nFrames, outputs[c]);
  }

  // the editor renderer role adds auxiliary editing output (here: auditioning a region that was
  // alt-clicked in the plug-in UI) on top of whatever was rendered above
  if (auto* pEditorRenderer = GetARAEditorRenderer<IPlugARAExampleEditorRenderer>())
    pEditorRenderer->RenderPreview(outputs, nChans, GetSampleRate(), nFrames);

  for (int c = 0; c < nChans; c++)
  {
    for (int s = 0; s < nFrames; s++)
      outputs[c][s] *= gain;
  }
}
#endif
