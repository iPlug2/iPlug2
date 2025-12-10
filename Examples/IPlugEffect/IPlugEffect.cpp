#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

#if IPLUG_EDITOR
  #include "IShaderControl.h"
  #include "IPlugEffect_shaders.h"

  #if defined(IGRAPHICS_SKIA)
    #define IPLUGEFFECT_SHADER_CONTROL(bounds, animate) \
      new IShaderControl(bounds, kIPlugEffectShader, animate)
  #elif defined(IGRAPHICS_NANOVG) && defined(IGRAPHICS_GL)
    #define IPLUGEFFECT_SHADER_CONTROL(bounds, animate) \
      new IShaderControl(bounds, kIPlugEffectShader, nullptr, animate)
  #elif defined(IGRAPHICS_NANOVG) && defined(IGRAPHICS_METAL)
    #define IPLUGEFFECT_SHADER_CONTROL(bounds, animate) \
      new IShaderControl(bounds, kIPlugEffectMetalShader, kIPlugEffectVertexFunc, kIPlugEffectFragmentFunc, animate)
  #else
    #define IPLUGEFFECT_SHADER_CONTROL(bounds, animate) nullptr
  #endif
#endif


IPlugEffect::IPlugEffect(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), "Hello iPlug 2!", IText(50)));
    pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain));

    // Create multiple shader controls to demonstrate shader caching
    // All instances share the same compiled shader program
    const int nShaders = 4;
    IRECT shaderArea = b.GetFromBottom(100);
    for (int i = 0; i < nShaders; i++)
    {
      IRECT shaderBounds = shaderArea.GetGridCell(i, 1, nShaders);
      pGraphics->AttachControl(IPLUGEFFECT_SHADER_CONTROL(shaderBounds, true));
    }
  };
#endif
}

#if IPLUG_DSP
void IPlugEffect::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
#endif
