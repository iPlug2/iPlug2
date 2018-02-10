#include <cmath>
#include <cstdio>
#include <ctime>
#include <cassert>

#include "wdlendian.h"
#include "wdl_base64.h"

#include "IPlugBase.h"


IPlugBase::IPlugBase(IPlugConfig c, EAPI plugAPI)
  : mUniqueID(c.uniqueID)
  , mMfrID(c.mfrID)
  , mVersion(c.vendorVersion)
  , mStateChunks(c.plugDoesChunks)
  , mEffectName(c.effectName, MAX_EFFECT_NAME_LEN)
  , mProductName(c.productName, MAX_EFFECT_NAME_LEN)
  , mMfrName(c.mfrName, MAX_EFFECT_NAME_LEN)
  , mAPI(plugAPI)
{
  Trace(TRACELOC, "%s:%s", c.effectName, CurrentTime());

  for (int i = 0; i < c.nParams; ++i)
    mParams.Add(new IParam());
}

IPlugBase::~IPlugBase()
{
  TRACE;

  mParams.Empty(true);
}

void IPlugBase::OnParamChange(int paramIdx, EParamSource source)
{
  Trace(TRACELOC, "idx:%i src:%s\n", paramIdx, ParamSourceStrs[source]);
  OnParamChange(paramIdx);
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

void IPlugBase::SetHost(const char* host, int version)
{
  mHost = LookUpHost(host);
  mHostVersion = version;

  WDL_String vStr;
  GetVersionStr(version, vStr);
  Trace(TRACELOC, "host_%sknown:%s:%s", (mHost == kHostUnknown ? "un" : ""), host, vStr.Get());
}

// Decimal = VVVVRRMM, otherwise 0xVVVVRRMM.
int IPlugBase::GetEffectVersion(bool decimal) const
{
  if (decimal)
    return GetDecimalVersion(mVersion);
  else
    return mVersion;
}

void IPlugBase::GetEffectVersionStr(WDL_String& str) const
{
  GetVersionStr(mVersion, str);
#if defined TRACER_BUILD
  str.Append("T");
#endif
#if defined _DEBUG
  str.Append("D");
#endif
}

const char* IPlugBase::GetAPIStr()
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

const char* IPlugBase::GetArchStr()
{
#ifdef ARCH_64BIT
  return "x64";
#else
  return "x86";
#endif
}

void IPlugBase::GetBuildInfoStr(WDL_String& str)
{
  WDL_String version;
  GetEffectVersionStr(version);
  str.SetFormatted(MAX_BUILD_INFO_STR_LEN, "%s version %s %s %s, built on %s at %.5s ", GetEffectName(), version.Get(), GetArchStr(), GetAPIStr(), __DATE__, __TIME__);
}

// this is over-ridden for AAX
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

bool IPlugBase::SerializeParams(IByteChunk& chunk)
{
  TRACE;
  bool savedOK = true;
  int i, n = mParams.GetSize();
  for (i = 0; i < n && savedOK; ++i)
  {
    IParam* pParam = mParams.Get(i);
    Trace(TRACELOC, "%d %s %f", i, pParam->GetNameForHost(), pParam->Value());
    double v = pParam->Value();
    savedOK &= (chunk.Put(&v) > 0);
  }
  return savedOK;
}

int IPlugBase::UnserializeParams(IByteChunk& chunk, int startPos)
{
  TRACE;
  LOCK_PARAMS_MUTEX;
  int i, n = mParams.GetSize(), pos = startPos;
  for (i = 0; i < n && pos >= 0; ++i)
  {
    IParam* pParam = mParams.Get(i);
    double v = 0.0;
    pos = chunk.Get(&v, pos);
    pParam->Set(v);
    Trace(TRACELOC, "%d %s %f", i, pParam->GetNameForHost(), pParam->Value());
  }
  OnParamReset(kPresetRecall);
  return pos;
}

bool IPlugBase::CompareState(const unsigned char* incomingState, int startPos)
{
  bool isEqual = true;
  
  const double* data = (const double*) incomingState + startPos;
  
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

void IPlugBase::DirtyParameters()
{
  for (int p = 0; p < NParams(); p++)
  {
    double normalizedValue = GetParam(p)->GetNormalized();
    InformHostOfParamChange(p, normalizedValue);
  }
}

void IPlugBase::InitChunkWithIPlugVer(IByteChunk& chunk)
{
  chunk.Clear();
  int magic = IPLUG_VERSION_MAGIC;
  chunk.Put(&magic);
  int ver = IPLUG_VERSION;
  chunk.Put(&ver);
}

int IPlugBase::GetIPlugVerFromChunk(IByteChunk& chunk, int& position)
{
  int magic = 0, ver = 0;
  int magicpos = chunk.Get(&magic, position);
  
  if (magicpos > position && magic == IPLUG_VERSION_MAGIC)
    position = chunk.Get(&ver, magicpos);
  
  return ver;
}

void IPlugBase::PrintDebugInfo()
{
  WDL_String buildInfo;
  GetBuildInfoStr(buildInfo);
  DBGMSG("\n--------------------------------------------------\n%s\nNO_IGRAPHICS\n", buildInfo.Get());
}
