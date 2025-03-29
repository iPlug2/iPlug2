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

//struct Meta : std::map<const char*, const char*>
//{
//    void declare(const char* key, const char* value) { (*this)[key] = value; }
//    
//    const char* get(const char* key, const char* defaultString)
//    {
//        if (this->find(key) != this->end()) {
//            return (*this)[key];
//        } else {
//            return defaultString;
//        }
//    }
//};

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

IPlugFaust::~IPlugFaust()
{
  mParams.Empty(true);
}

void IPlugFaust::declare(const char* key, const char* value)
{
}

void IPlugFaust:: SetSampleRate(double sampleRate)
{
  int multiplier = 1;
  
  if (mOverSampler)
    multiplier = mOverSampler->GetRate();
  
  if (mDSP) {
    mDSP->init(((int) sampleRate) * multiplier);
    SyncFaustParams();
  }
}

void IPlugFaust::ProcessMidiMsg(const IMidiMsg& msg)
{
  mMidiHandler->decodeMessage(msg);
}

void IPlugFaust::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  if (mDSP)
  {
    assert(mDSP->getSampleRate() != 0); // did you forget to call SetSampleRate?

    if (mOverSampler)
      mOverSampler->ProcessBlock(inputs, outputs, nFrames, 2, 2 /* TODO: flexible channel count */,
        [&](sample** inputs, sample** outputs, int nFrames) //TODO:: badness capture = allocated
        {
          mDSP->compute(nFrames, inputs, outputs);
        });
    else
      mDSP->compute(nFrames, inputs, outputs);
  }
  //    else silence?
}

void IPlugFaust::FreeDSP()
{
  mMidiHandler->stopMidi();
  mMidiUI = nullptr;
  mDSP = nullptr;
  mMidiHandler = nullptr;
}

void IPlugFaust::SetOverSamplingRate(int rate)
{
  if (mOverSampler)
    mOverSampler->SetOverSampling(OverSampler<sample>::RateToFactor(rate));
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
  if (NParams())
  {
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
  ffloat* dest = nullptr;
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
    pPlugParam->SetDisplayPrecision(2);
    if (setToDefault)
      pPlugParam->SetToDefault();
    else
      pPlugParam->SetNormalized(currentValueNormalised);
  }

  return plugParamIdx;
}

void IPlugFaust::AddOrUpdateParam(IParam::EParamType type, const char* label, ffloat* zone, ffloat init, ffloat min, ffloat max, ffloat step)
{
  IParam* pParam = nullptr;

  const int paramIdx = FindExistingParameterWithName(label);

  if (paramIdx > -1)
    pParam = mParams.Get(paramIdx);
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

  if (paramIdx == -1)
  {
    mParams.Add(pParam);
  }
  
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

void IPlugFaust::SyncFaustParams()
{
  for (auto p = 0; p < NParams(); p++)
  {
    *mZones.Get(p) = mParams.Get(p)->Value();
  }
}

void IPlugFaust::addButton(const char *label, ffloat *zone)
{
  AddOrUpdateParam(IParam::kTypeBool, label, zone);
}

void IPlugFaust::addCheckButton(const char *label, ffloat *zone)
{
  AddOrUpdateParam(IParam::kTypeBool, label, zone);
}

void IPlugFaust::addVerticalSlider(const char *label, ffloat *zone, ffloat init, ffloat min, ffloat max, ffloat step)
{
  AddOrUpdateParam(IParam::kTypeDouble, label, zone, init, min, max, step);
}

void IPlugFaust::addHorizontalSlider(const char *label, ffloat *zone, ffloat init, ffloat min, ffloat max, ffloat step)
{
  AddOrUpdateParam(IParam::kTypeDouble, label, zone, init, min, max, step);
}

void IPlugFaust::addNumEntry(const char *label, ffloat *zone, ffloat init, ffloat min, ffloat max, ffloat step)
{
  AddOrUpdateParam(IParam::kTypeEnum, label, zone, init, min, max, step);
}

void IPlugFaust::OnUITimer(Timer& timer)
{
  GUI::updateAllGuis();
}

int IPlugFaust::NParams() const
{
  return mParams.GetSize();
}
