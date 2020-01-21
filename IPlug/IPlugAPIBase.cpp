/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

/**
 * @file
 * @brief IPlugAPIBase implementation
 */

#include <cmath>
#include <cstdio>
#include <ctime>
#include <cassert>

#include "IPlugAPIBase.h"

using namespace iplug;

IPlugAPIBase::IPlugAPIBase(Config c, EAPI plugAPI)
  : IPluginBase(c.nParams, c.nPresets)
{
  mUniqueID = c.uniqueID;
  mMfrID = c.mfrID;
  mVersion = c.vendorVersion;
  mPluginName.Set(c.pluginName, MAX_PLUGIN_NAME_LEN);
  mProductName.Set(c.productName, MAX_PLUGIN_NAME_LEN);
  mMfrName.Set(c.mfrName, MAX_PLUGIN_NAME_LEN);
  mHasUI = c.plugHasUI;
  mEditorWidth = c.plugWidth;
  mEditorHeight = c.plugHeight;
  mStateChunks = c.plugDoesChunks;
  mAPI = plugAPI;
  mBundleID.Set(c.bundleID);

  Trace(TRACELOC, "%s:%s", c.pluginName, CurrentTime());
  
  mParamDisplayStr.Set("", MAX_PARAM_DISPLAY_LEN);
}

IPlugAPIBase::~IPlugAPIBase()
{
  if(mTimer)
  {
    mTimer->Stop();
  }

  TRACE
}

void IPlugAPIBase::OnHostRequestingImportantParameters(int count, WDL_TypedBuf<int>& results)
{
  if(NParams() > count)
  {
    for (int i = 0; i < count; i++)
      results.Add(i);
  }
}

void IPlugAPIBase::CreateTimer()
{
  mTimer = std::unique_ptr<Timer>(Timer::Create(std::bind(&IPlugAPIBase::OnTimer, this, std::placeholders::_1), IDLE_TIMER_RATE));
}

bool IPlugAPIBase::CompareState(const uint8_t* pIncomingState, int startPos) const
{
  bool isEqual = true;
  
  const double* data = (const double*) pIncomingState + startPos;
  
  // dirty hack here because protools treats param values as 32 bit int and in IPlug they are 64bit float
  // if we memcmp() the incoming state with the current they may have tiny differences due to the quantization
  for (int i = 0; i < NParams(); i++)
  {
    float v = (float) GetParam(i)->Value();
    float vi = (float) *(data++);
    
    isEqual &= (std::fabs(v - vi) < 0.00001);
  }
  
  return isEqual;
}

bool IPlugAPIBase::EditorResizeFromDelegate(int width, int height)
{
  mEditorWidth = width;
  mEditorHeight = height;

  return false;
}

#pragma mark -

void IPlugAPIBase::PrintDebugInfo() const
{
  WDL_String buildInfo;
  GetBuildInfoStr(buildInfo);
  DBGMSG("\n--------------------------------------------------\n%s\n", buildInfo.Get());
}

#pragma mark -

void IPlugAPIBase::SetHost(const char* host, int version)
{
  assert(mHost == kHostUninit);
    
  mHost = LookUpHost(host);
  mHostVersion = version;
  
  WDL_String vStr;
  GetVersionStr(version, vStr);
  Trace(TRACELOC, "host_%sknown:%s:%s", (mHost == kHostUnknown ? "un" : ""), host, vStr.Get());
    
  HostSpecificInit();
  OnHostIdentified();
}

void IPlugAPIBase::SetParameterValue(int idx, double normalizedValue)
{
  Trace(TRACELOC, "%d:%f", idx, normalizedValue);
  GetParam(idx)->SetNormalized(normalizedValue);
  InformHostOfParamChange(idx, normalizedValue);
  OnParamChange(idx, kUI);
}

void IPlugAPIBase::DirtyParametersFromUI()
{
  for (int p = 0; p < NParams(); p++)
  {
    double normalizedValue = GetParam(p)->GetNormalized();
    InformHostOfParamChange(p, normalizedValue);
  }
}

void IPlugAPIBase::SendParameterValueFromAPI(int paramIdx, double value, bool normalized)
{
  //TODO: Can we assume that no host is stupid enough to try and set parameters on multiple threads at the same time?
  // If that is the case then we need a MPSPC queue not SPSC
  if (normalized)
    value = GetParam(paramIdx)->FromNormalized(value);
  
  mParamChangeFromProcessor.Push(ParamTuple { paramIdx, value } );
}

void IPlugAPIBase::OnTimer(Timer& t)
{
  if(HasUI())
  {
    // in distributed VST 3, parameter changes are managed by the host
  #if !defined VST3C_API && !defined VST3P_API
    while(mParamChangeFromProcessor.ElementsAvailable())
    {
      ParamTuple p;
      mParamChangeFromProcessor.Pop(p);
      SendParameterValueFromDelegate(p.idx, p.value, false); // TODO:  if the parameter hasn't changed maybe we shouldn't do anything?
    }
    
    while (mMidiMsgsFromProcessor.ElementsAvailable())
    {
      IMidiMsg msg;
      mMidiMsgsFromProcessor.Pop(msg);
      SendMidiMsgFromDelegate(msg);
    }
    
    while (mSysExDataFromProcessor.ElementsAvailable())
    {
      SysExData msg;
      mSysExDataFromProcessor.Pop(msg);
      SendSysexMsgFromDelegate({msg.mOffset, msg.mData, msg.mSize});
    }
  #endif
    
    // Midi messages from the processor to the controller, are sent as IMessages and SendMidiMsgFromDelegate gets triggered on the other side's notify
  #if defined VST3P_API
    while (mMidiMsgsFromProcessor.ElementsAvailable())
    {
      IMidiMsg msg;
      mMidiMsgsFromProcessor.Pop(msg);
      TransmitMidiMsgFromProcessor(msg);
    }
    
    while (mSysExDataFromProcessor.ElementsAvailable())
    {
      SysExData data;
      mSysExDataFromProcessor.Pop(data);
      TransmitSysExDataFromProcessor(data);
    }
  #endif
  }
  
  OnIdle();
}

void IPlugAPIBase::SendMidiMsgFromUI(const IMidiMsg& msg)
{
  DeferMidiMsg(msg); // queue the message so that it will be handled by the processor
  EDITOR_DELEGATE_CLASS::SendMidiMsgFromUI(msg); // for remote editors
}

void IPlugAPIBase::SendSysexMsgFromUI(const ISysEx& msg)
{
  DeferSysexMsg(msg); // queue the message so that it will be handled by the processor
  EDITOR_DELEGATE_CLASS::SendSysexMsgFromUI(msg); // for remote editors
}

void IPlugAPIBase::SendArbitraryMsgFromUI(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  OnMessage(msgTag, ctrlTag, dataSize, pData); // IPlugAPIBase implementation handles non distributed plug-ins - just call OnMessage() directly
  
  EDITOR_DELEGATE_CLASS::SendArbitraryMsgFromUI(msgTag, ctrlTag, dataSize, pData);
}
