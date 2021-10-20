/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include <cstdio>
#include "IPlugCLAP.h"
#include "IPlugPluginBase.h"

using namespace iplug;

IPlugCLAP::IPlugCLAP(const InstanceInfo& info, const Config& config)
  : IPlugAPIBase(config, kAPICLAP)
  , IPlugProcessor(config, kAPICLAP)
  , clap::Plugin(info.mDesc, info.mHost)
{
  Trace(TRACELOC, "%s", config.pluginName);
  
  CreateTimer();
}

//void IPlugCLAP::BeginInformHostOfParamChange(int idx)
//{
//}
//
//void IPlugCLAP::InformHostOfParamChange(int idx, double normalizedValue)
//{
//}
//
//void IPlugCLAP::EndInformHostOfParamChange(int idx)
//{
//}
//
//void IPlugCLAP::InformHostOfPresetChange()
//{
//}
//
//bool IPlugCLAP::EditorResize(int viewWidth, int viewHeight)
//{
//  bool resized = false;
//
//  if (HasUI())
//  {
//    if (viewWidth != GetEditorWidth() || viewHeight != GetEditorHeight())
//    {
//      SetEditorSize(viewWidth, viewHeight);
//
//    }
//  }
//
//  return resized;
//}
//
//void IPlugCLAP::SetLatency(int samples)
//{
//  IPlugProcessor::SetLatency(samples);
//}
//
bool IPlugCLAP::SendMidiMsg(const IMidiMsg& msg)
{
  return false;
}
//
//bool IPlugCLAP::SendSysEx(const ISysEx& msg)
//{
//  return false;
//}

bool IPlugCLAP::init() noexcept
{
  return true;
}
