#include "IPlugFaustDSP.h"
#include "IPlug_include_in_plug_src.h"

#if IPLUG_EDITOR
#include "IControls.h"
#include "IGraphicsFaustUI.h"
#endif

IPlugFaustDSP::IPlugFaustDSP(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, 0))
{
  mFaust.SetMaxChannelCount(MaxNChannels(ERoute::kInput), MaxNChannels(ERoute::kOutput));

#if defined FAUST_BUNDLED && !defined FAUST_COMPILED
  // Use the .dsp file and the Faust libraries copied into the bundle, when the original .dsp file doesn't exist
  WDL_String resourcePath;
  BundleResourcePath(resourcePath, GetBundleID());
  mFaust.SetBundleResourcePath(resourcePath.Get());
#endif

  mFaust.Init();
  mFaust.CreateIPlugParameters(this);
  InitUnusedParams();

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->EnableTooltips(true);

    const IRECT bounds = pGraphics->GetBounds().GetPadded(-10.f);

    // Build controls for the Faust DSP's widgets, linked to the parameters created by CreateIPlugParameters()
    IGraphicsFaustUI faustUI(mFaust, DEFAULT_STYLE.WithDrawShadows(false).WithLabelText(IText(14.f)));

    if (IControl* pFaustControl = faustUI.CreateControl(bounds.GetReducedFromBottom(20.f)))
      pGraphics->AttachControl(pFaustControl);

    WDL_String status;
#ifdef FAUST_COMPILED
    status.Set("Faust DSP: compiled");
#else
    if (CStringHasContents(mFaust.GetErrorMessage()))
    {
      status.SetFormatted(1024, "Faust error: %s", mFaust.GetErrorMessage());
      // only show the first line
      if (char* pNewLine = strchr(status.Get(), '\n'))
        *pNewLine = '\0';
    }
    else
    {
      WDL_String dspFile(mFaust.GetDSPFile());
      status.SetFormatted(1024, "Faust DSP: interpreter - edit %s to recompile", dspFile.get_filepart());
    }
#endif
    pGraphics->AttachControl(new ITextControl(bounds.GetFromBottom(20.f), status.Get(), DEFAULT_TEXT.WithAlign(EAlign::Near)), kCtrlTagStatus);
  };
#endif

#ifndef FAUST_COMPILED
  // When the .dsp file is edited and saved, it is recompiled. The parameters are updated, the UI needs to be rebuilt
  mFaust.SetOnCompileFunc([&](bool success) {
    InitUnusedParams();

#if IPLUG_EDITOR
    if (IGraphics* pGraphics = GetUI())
    {
      pGraphics->RemoveAllControls();
      mLayoutFunc(pGraphics);
    }
#endif
  });

  mFaust.SetAutoRecompile(true);
#endif
}

void IPlugFaustDSP::InitUnusedParams()
{
  for (int i = mFaust.NParams(); i < kNumParams; i++)
  {
    WDL_String name;
    name.SetFormatted(32, "Unused %i", i - mFaust.NParams() + 1);
    GetParam(i)->InitDouble(name.Get(), 0., 0., 1., 0.01);
  }
}

void IPlugFaustDSP::OnParamChange(int paramIdx)
{
  mFaust.OnParamChange(paramIdx);
}

#if IPLUG_DSP
void IPlugFaustDSP::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mFaust.ProcessBlock(inputs, outputs, nFrames);
}

void IPlugFaustDSP::OnReset()
{
  mFaust.SetSampleRate(GetSampleRate());
}
#endif
