#include "IPlugWeb.h"

#ifndef NO_IGRAPHICS
#include "IGraphicsWeb.h"
extern IGraphics* gGraphics;

void IPlugWeb::AttachGraphics(IGraphics* pGraphics)
{
  gGraphics = pGraphics;
  IGraphicsEditorDelegate::AttachGraphics(pGraphics);
  gGraphics->Draw(gGraphics->GetBounds()); //TODO: weird
}
#endif

//void IPlugWeb::BeginInformHostOfParamChangeFromUI(int paramIdx)
//{
//};

void IPlugWeb::SetParameterValueFromUI(int paramIdx, double value)
{
  WDL_String jsname;
  jsname.SetFormatted(64, "%s_WAM", GetPluginName()); // TODO: move me
  emscripten::val::global(jsname.Get()).call<void>("setParam", paramIdx, value);
  
  IPlugAPIBase::SetParameterValueFromUI(paramIdx, value);
};

//void IPlugWeb::EndInformHostOfParamChangeFromUI(int paramIdx)
//{
//}
