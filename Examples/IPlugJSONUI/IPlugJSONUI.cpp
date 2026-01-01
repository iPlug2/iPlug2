#include "IPlugJSONUI.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IPlugPaths.h"
#include <string>

IPlugJSONUI::IPlugJSONUI(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, 1))
{
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100., 0.1, "%");
  GetParam(kPan)->InitDouble("Pan", 0., -100., 100., 0.1, "%");
  GetParam(kMix)->InitDouble("Mix", 100., 0., 100., 0.1, "%");

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT b = pGraphics->GetBounds();

    // First time: load JSON and create controls
    if (pGraphics->NControls() == 0)
    {
      pGraphics->SetLayoutOnResize(true);
      pGraphics->AttachCornerResizer(EUIResizerMode::Size, true);
      pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

      mJSONUI = std::make_unique<IGraphicsJSON>(pGraphics, this);

      // Map parameter names used in JSON to enum values
      mJSONUI->SetParamMapping({
        {"kGain", kGain},
        {"kPan", kPan},
        {"kMix", kMix}
      });

      // Map control IDs used in JSON to tag values
      mJSONUI->SetTagMapping({
        {"gainKnob", kGainKnob},
        {"panKnob", kPanKnob},
        {"mixSlider", kMixSlider}
      });

#if defined(IPLUG_SRCDIR)
      // In debug with source dir: load from source for hot-reload
      mJSONUI->Load(IPLUG_SRCDIR "/resources/ui.json");
      mJSONUI->EnableHotReload(true);
#elif defined(_DEBUG)
      // In debug: load from bundle for hot-reload
      WDL_String path;
      EResourceLocation loc = LocateResource("ui.json", "json", path,
        pGraphics->GetBundleID(), pGraphics->GetWinModuleHandle(),
        pGraphics->GetSharedResourcesSubPath());
      if (loc != EResourceLocation::kNotFound)
      {
        mJSONUI->Load(path.Get());
        mJSONUI->EnableHotReload(true);
      }
#else
      // In release: load JSON content via resource system
      WDL_TypedBuf<uint8_t> data = pGraphics->LoadResource("ui.json", "json");
      if (data.GetSize() > 0)
      {
        std::string jsonStr(reinterpret_cast<const char*>(data.Get()), data.GetSize());
        mJSONUI->LoadFromString(jsonStr.c_str());
      }
#endif
    }

    // On every resize: recompute all bounds
    mJSONUI->OnResize(b);
  };
#endif
}

#if IPLUG_EDITOR
void IPlugJSONUI::OnIdle()
{
  if (mJSONUI)
    mJSONUI->CheckForChanges();
}
#endif

#if IPLUG_DSP
void IPlugJSONUI::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const double pan = GetParam(kPan)->Value() / 100.; // -1 to 1
  const double mix = GetParam(kMix)->Value() / 100.;
  const int nChans = NOutChansConnected();

  // Simple stereo pan law
  const double panL = std::min(1.0, 1.0 - pan);
  const double panR = std::min(1.0, 1.0 + pan);

  for (int s = 0; s < nFrames; s++)
  {
    for (int c = 0; c < nChans; c++)
    {
      double wet = inputs[c][s] * gain;
      if (c == 0) wet *= panL;
      else if (c == 1) wet *= panR;

      outputs[c][s] = mix * wet + (1.0 - mix) * inputs[c][s];
    }
  }
}
#endif
