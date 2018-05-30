/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

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
