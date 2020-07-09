/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

/**
 * @file
 * @copydoc IPlugFaust
 */

#include "IPlugFaust.h"

std::list<GUI*> GUI::fGuiList;
ztimedmap GUI::gTimedZoneMap;
iplug::Timer* iplug::IPlugFaust::sUITimer = nullptr;

using namespace iplug;

IPlugFaust::IPlugFaust(const char* name, int nVoices, int rate)
: mNVoices(nVoices)
{
  if (rate > 1)
  {
    mOverSampler = std::make_unique<OverSampler<sample>>(OverSampler<sample>::RateToFactor(rate), true, 2 /* TODO: flexible channel count */);
  }

  mName.Set(name);

  if (sUITimer == nullptr)
  {
    sUITimer = Timer::Create(std::bind(&IPlugFaust::OnUITimer, this, std::placeholders::_1), FAUST_UI_INTERVAL);
  }
}

void IPlugFaust::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  if (mDSP)
  {
    assert(mDSP->getSampleRate() != 0); // did you forget to call SetSampleRate?

    if (mOverSampler)
      mOverSampler->ProcessBlock(inputs, outputs, nFrames, 2 /* TODO: flexible channel count */,
        [&](sample** inputs, sample** outputs, int nFrames) //TODO:: badness capture = allocated
        {
          mDSP->compute(nFrames, inputs, outputs);
        });
    else
      mDSP->compute(nFrames, inputs, outputs);
  }
  //    else silence?
}

void IPlugFaust::SetParameterValueNormalised(int paramIdx, double normalizedValue)
{
  if (paramIdx > kNoParameter && paramIdx >= NParams())
  {
    DBGMSG("IPlugFaust-%s:: No parameter %i\n", mName.Get(), paramIdx);
  }
  else
  {
    mParams.Get(paramIdx)->SetNormalized(normalizedValue);

    if (mZones.GetSize() == NParams())
      *(mZones.Get(paramIdx)) = mParams.Get(paramIdx)->Value();
    else
      DBGMSG("IPlugFaust-%s:: Missing zone for parameter %s\n", mName.Get(), mParams.Get(paramIdx)->GetName());
  }
}

void IPlugFaust::SetParameterValue(int paramIdx, double nonNormalizedValue)
{
  if (NParams()) {

    assert(paramIdx < NParams()); // Seems like we don't have enough parameters!

    mParams.Get(paramIdx)->Set(nonNormalizedValue);

    if (mZones.GetSize() == NParams())
      *(mZones.Get(paramIdx)) = nonNormalizedValue;
    else
      DBGMSG("IPlugFaust-%s:: Missing zone for parameter %s\n", mName.Get(), mParams.Get(paramIdx)->GetName());
  }
  else
    DBGMSG("SetParameterValue called with no FAUST params\n");
}

void IPlugFaust::SetParameterValue(const char* labelToLookup, double nonNormalizedValue)
{
  FAUSTFLOAT* dest = nullptr;
  dest = mMap.Get(labelToLookup, nullptr);
  //    mParams.Get(paramIdx)->Set(nonNormalizedValue); // TODO: we are not updating the IPlug parameter

  if (dest)
    *dest = nonNormalizedValue;
  else
    DBGMSG("IPlugFaust-%s:: No parameter named %s\n", mName.Get(), labelToLookup);
}

int IPlugFaust::CreateIPlugParameters(IPlugAPIBase* pPlug, int startIdx, int endIdx, bool setToDefault)
{
  assert(pPlug != nullptr);

  if (NParams() == 0)
    return -1;

  mPlug = pPlug;

  int plugParamIdx = mIPlugParamStartIdx = startIdx;

  if (endIdx == -1)
    endIdx = pPlug->NParams();

  for (auto p = 0; p < endIdx; p++)
  {
    assert(plugParamIdx + p < pPlug->NParams()); // plugin needs to have enough params!

    IParam* pPlugParam = pPlug->GetParam(plugParamIdx + p);
    const double currentValueNormalised = pPlugParam->GetNormalized();
    pPlugParam->Init(*mParams.Get(p));
    if (setToDefault)
      pPlugParam->SetToDefault();
    else
      pPlugParam->SetNormalized(currentValueNormalised);
  }

  return plugParamIdx;
}

void IPlugFaust::AddOrUpdateParam(IParam::EParamType type, const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
  IParam* pParam = nullptr;

  const int idx = FindExistingParameterWithName(label);

  if (idx > -1)
    pParam = mParams.Get(idx);
  else
    pParam = new IParam();

  switch (type)
  {
  case IParam::EParamType::kTypeBool:
    pParam->InitBool(label, 0);
    break;
  case IParam::EParamType::kTypeInt:
    pParam->InitInt(label, static_cast<int>(init), static_cast<int>(min), static_cast<int>(max));
    break;
  case IParam::EParamType::kTypeEnum:
    pParam->InitEnum(label, static_cast<int>(init), static_cast<int>(max - min));
    //TODO: metadata
    break;
  case IParam::EParamType::kTypeDouble:
    pParam->InitDouble(label, init, min, max, step);
    break;
  default:
    break;
  }

  if (idx == -1)
    mParams.Add(pParam);

  mZones.Add(zone);
}

void IPlugFaust::BuildParameterMap()
{
  for (auto p = 0; p < NParams(); p++)
  {
    mMap.Insert(mParams.Get(p)->GetName(), mZones.Get(p)); // insert will overwrite keys with the same name
  }

  if (mIPlugParamStartIdx > -1 && mPlug != nullptr) // if we've already linked parameters
  {
    CreateIPlugParameters(mPlug, mIPlugParamStartIdx);
  }

  for (auto p = 0; p < NParams(); p++)
  {
    DBGMSG("%i %s\n", p, mParams.Get(p)->GetName());
  }
}

int IPlugFaust::FindExistingParameterWithName(const char* name) // TODO: this needs to check meta data too - incase of grouping
{
  for (auto p = 0; p < NParams(); p++)
  {
    if (strcmp(name, mParams.Get(p)->GetName()) == 0)
    {
      return p;
    }
  }

  return -1;
}
