#include "IPlugWeb.h"

#ifndef NO_IGRAPHICS
#include "IGraphicsWeb.h"
extern IGraphics* gGraphics;

void IPlugWeb::AttachGraphics(IGraphics* pGraphics)
{
  gGraphics = pGraphics;
  IGraphicsDelegate::AttachGraphics(pGraphics);
  gGraphics->Draw(gGraphics->GetBounds());
}
#endif

void IPlugWeb::SetParameterValueFromUI(int paramIdx, double value)
{
  WDL_String jsname;
  jsname.SetFormatted(64, "%s_WAM", GetPluginName());
  emscripten::val::global(jsname.Get()).call<void>("setParam", paramIdx, value);

  IPlugBase::SetParameterValueFromUI(paramIdx, value);
};

void IPlugWeb::BeginInformHostOfParamChangeFromUI(int paramIdx)
{
  //     emscripten::val::global(GetPluginName()).call<emscripten::val>("setParam", paramIdx, value);
};

void IPlugWeb::EndInformHostOfParamChangeFromUI(int paramIdx)
{
  //     emscripten::val::global(GetPluginName()).call<emscripten::val>("setParam", paramIdx, value);
}
