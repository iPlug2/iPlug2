#include <cmath>
#include <cstdio>
#include <ctime>
#include <cassert>


#include "wdlendian.h"

#include "IPlugBase.h"

IPlugBase::IPlugBase(IPlugConfig c, EAPI plugAPI)
  : IPLUG_DELEGATE(c.nParams, c.nPresets)
  , mUniqueID(c.uniqueID)
  , mMfrID(c.mfrID)
  , mVersion(c.vendorVersion)
  , mPluginName(c.pluginName, MAX_PLUGIN_NAME_LEN)
  , mProductName(c.productName, MAX_PLUGIN_NAME_LEN)
  , mMfrName(c.mfrName, MAX_PLUGIN_NAME_LEN)
  , mHasUI(c.plugHasUI)
  , mWidth(c.plugWidth)
  , mHeight(c.plugHeight)
  , mAPI(plugAPI)
  , mHighPriorityToUIQueue(512) // TODO: CONSTANT
{
  mStateChunks = c.plugDoesChunks;
  
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

int IPlugBase::GetPluginVersion(bool decimal) const
{
  if (decimal)
    return GetDecimalVersion(mVersion);
  else
    return mVersion;
}

void IPlugBase::GetPluginVersionStr(WDL_String& str) const
{
  GetVersionStr(mVersion, str);
#if defined TRACER_BUILD
  str.Append("T");
#endif
#if defined _DEBUG
  str.Append("D");
#endif
}

int IPlugBase::GetHostVersion(bool decimal)
{
  GetHost();
  if (decimal)
  {
    return GetDecimalVersion(mHostVersion);
  }
  return mHostVersion;
}

void IPlugBase::GetHostVersionStr(WDL_String& str)
{
  GetHost();
  GetVersionStr(mHostVersion, str);
}

const char* IPlugBase::GetAPIStr() const
{
  switch (GetAPI()) 
  {
    case kAPIVST2: return "VST2";
    case kAPIVST3: return "VST3";
    case kAPIAU: return "AU";
    case kAPIAAX: return "AAX";
    case kAPIAPP: return "Standalone";
    default: return "";
  }
}

const char* IPlugBase::GetArchStr() const
{
#ifdef ARCH_64BIT
  return "x64";
#else
  return "x86";
#endif
}

void IPlugBase::GetBuildInfoStr(WDL_String& str) const
{
  WDL_String version;
  GetPluginVersionStr(version);
  str.SetFormatted(MAX_BUILD_INFO_STR_LEN, "%s version %s %s %s, built on %s at %.5s ", GetPluginName(), version.Get(), GetArchStr(), GetAPIStr(), __DATE__, __TIME__);
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
    SendParameterValueToUIFromDelegate(p.paramIdx, p.value, p.normalized);
  }
  
  OnIdle();
}
