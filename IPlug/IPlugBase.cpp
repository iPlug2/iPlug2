#include <cmath>
#include <cstdio>
#include <ctime>
#include <cassert>


#include "wdlendian.h"

#include "IPlugBase.h"

IPlugBase::IPlugBase(IPlugConfig c, EAPI plugAPI)
  : PLUG_DELEGATE(c.nParams, c.nPresets)
  , mHighPriorityToUIQueue(512) // TODO: CONSTANT
{
  mUniqueID = c.uniqueID;
  mMfrID = c.mfrID;
  mVersion = c.vendorVersion;
  mPluginName.Set(c.pluginName, MAX_PLUGIN_NAME_LEN);
  mProductName.Set(c.productName, MAX_PLUGIN_NAME_LEN);
  mMfrName.Set(c.mfrName, MAX_PLUGIN_NAME_LEN);
  mHasUI = c.plugHasUI;
  mWidth = c.plugWidth;
  mHeight = c.plugHeight;
  mStateChunks = c.plugDoesChunks;
  mAPI = plugAPI;

  Trace(TRACELOC, "%s:%s", c.pluginName, CurrentTime());
  
  mParamDisplayStr.Set("", MAX_PARAM_DISPLAY_LEN);
  mTimer = Timer::Create(*this, 20); // TODO: CONSTANT
}

IPlugBase::~IPlugBase()
{
  if(mTimer)
  {
    mTimer->Stop();
    delete mTimer;
  }

  TRACE;
}

bool IPlugBase::CompareState(const uint8_t* pIncomingState, int startPos)
{
  bool isEqual = true;
  
  const double* data = (const double*) pIncomingState + startPos;
  
  // dirty hack here because protools treats param values as 32 bit int and in IPlug they are 64bit float
  // if we memcmp() the incoming state with the current they may have tiny differences due to the quantization
  for (int i = 0; i < NParams(); i++)
  {
    float v = (float) GetParam(i)->Value();
    float vi = (float) *(data++);
    
    isEqual &= (fabsf(v - vi) < 0.00001);
  }
  
  return isEqual;
}

#pragma mark -

void IPlugBase::PrintDebugInfo() const
{
  WDL_String buildInfo;
  GetBuildInfoStr(buildInfo);
  DBGMSG("\n--------------------------------------------------\n%s\nNO_IGRAPHICS\n", buildInfo.Get());
}

#pragma mark -

void IPlugBase::SetHost(const char* host, int version)
{
  mHost = LookUpHost(host);
  mHostVersion = version;
  
  WDL_String vStr;
  GetVersionStr(version, vStr);
  Trace(TRACELOC, "host_%sknown:%s:%s", (mHost == kHostUnknown ? "un" : ""), host, vStr.Get());
}

void IPlugBase::SetParameterValue(int idx, double normalizedValue)
{
  Trace(TRACELOC, "%d:%f", idx, normalizedValue);
  GetParam(idx)->SetNormalized(normalizedValue);
  InformHostOfParamChange(idx, normalizedValue);
  OnParamChange(idx, kGUI);
}

void IPlugBase::OnParamReset(EParamSource source)
{
  for (int i = 0; i < mParams.GetSize(); ++i)
  {
    OnParamChange(i, source);
  }
}

void IPlugBase::DirtyParameters()
{
  for (int p = 0; p < NParams(); p++)
  {
    double normalizedValue = GetParam(p)->GetNormalized();
    InformHostOfParamChange(p, normalizedValue);
  }
}

void IPlugBase::_SendParameterValueToUIFromAPI(int paramIdx, double value, bool normalized)
{
  mHighPriorityToUIQueue.Push(ParamChange { paramIdx, value, normalized } );
}

void IPlugBase::OnTimer(Timer& t)
{
  //TODO: if transport not running why do this?
  while(mHighPriorityToUIQueue.ElementsAvailable())
  {
    ParamChange p;
    mHighPriorityToUIQueue.Pop(p);
    SendParameterValueToUIFromDelegate(p.paramIdx, p.value, p.normalized); // TODO:  if the parameter hasn't changed maybe we shouldn't do anything?
  }
  
  OnIdle();
}
