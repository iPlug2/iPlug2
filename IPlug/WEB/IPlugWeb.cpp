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

using namespace emscripten;

IPlugWeb::IPlugWeb(IPlugInstanceInfo instanceInfo, IPlugConfig config)
: IPlugAPIBase(config, kAPIWEB)
{
  mWAMCtrlrJSObjectName.SetFormatted(32, "%s_WAM", GetPluginName());
}

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

void IPlugWeb::SetParameterValueFromUI(int paramIdx, double value)
{
  val::global(mWAMCtrlrJSObjectName.Get()).call<void>("setParam", paramIdx, value);
  IPlugAPIBase::SetParameterValueFromUI(paramIdx, value); // call super class in order to make sure OnParamChangeUI() gets triggered
};

void IPlugWeb::SendMidiMsgFromUI(const IMidiMsg& msg)
{
//  val::global(mWAMCtrlrJSObjectName.Get()).call<void>("sendMessage", "midiMsg", "", );
}

void IPlugWeb::SendMsgFromUI(int messageTag, int dataSize, const void* pData)
{
//  val::global(mWAMCtrlrJSObjectName.Get()).call<void>("sendMessage", msgID, "", "");
}

