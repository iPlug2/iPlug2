#include "IPlugWeb.h"
#include <emscripten.h>

#ifndef NO_IGRAPHICS
#include "IGraphicsWeb.h"
extern IGraphics* gGraphics;

void IPlugWEB::AttachGraphics(IGraphics* pGraphics)
{
  IGraphicsDelegate::AttachGraphics(pGraphics);
  gGraphics = pGraphics;
  emscripten_set_main_loop(dynamic_cast<IGraphicsWeb*>(pGraphics)->OnMainLoopTimer, pGraphics->FPS(), 1);
}
#endif

void IPlugWEB::SetParameterValueFromUI(int paramIdx, double value)
{
  emscripten::val::global(GetPluginName()).call<emscripten::val>("setParam", paramIdx, value);
};

void IPlugWEB::BeginInformHostOfParamChangeFromUI(int paramIdx)
{
  //     emscripten::val::global(GetPluginName()).call<emscripten::val>("setParam", paramIdx, value);
};

void IPlugWEB::EndInformHostOfParamChangeFromUI(int paramIdx)
{
  //     emscripten::val::global(GetPluginName()).call<emscripten::val>("setParam", paramIdx, value);
}
