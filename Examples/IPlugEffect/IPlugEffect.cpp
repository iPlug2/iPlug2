#include "IPlugEffect.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

#if defined(IGRAPHICS_SKIA) || defined(IGRAPHICS_NANOVG)
#include "IUnifiedShaderControl.h"
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

#if defined(IGRAPHICS_SKIA)
    const char* shaderCode = R"(
      uniform float uTime;
      uniform float2 uResolution;
      half4 main(float2 fragCoord) {
        float2 uv = fragCoord / uResolution;
        return half4(uv.x, uv.y, sin(uTime) * 0.5 + 0.5, 1.0);
      }
    )";
    pGraphics->AttachControl(new IUnifiedShaderControl(b.GetFromBottom(100), shaderCode, true));
#elif defined(IGRAPHICS_NANOVG) && defined(IGRAPHICS_GL)
    const char* shaderCode = R"(#version 150
      uniform float uTime;
      uniform vec2 uResolution;
      out vec4 FragColor;
      void main() {
        vec2 uv = gl_FragCoord.xy / uResolution;
        FragColor = vec4(uv.x, uv.y, sin(uTime) * 0.5 + 0.5, 1.0);
      }
    )";
    pGraphics->AttachControl(new IUnifiedShaderControl(b.GetFromBottom(100), shaderCode, nullptr, true));
#elif defined(IGRAPHICS_NANOVG) && defined(IGRAPHICS_METAL)
    // Metal shaders are pre-compiled in IPlugEffect.metal
    pGraphics->AttachControl(new IUnifiedShaderControl(b.GetFromBottom(100), nullptr, nullptr, true));
#endif
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
