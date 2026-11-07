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

IPlugFaust::IPlugFaust(const char* name, int nVoices, int rate, int ctrlTagStart)
: mNVoices(nVoices)
, mCtrlTagStart(ctrlTagStart)
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

void IPlugFaust:: SetSampleRate(double sampleRate)
{
  int multiplier = 1;
  
  if (mOverSampler)
    multiplier = mOverSampler->GetRate();
  
  if (mDSP) {
    mDSP->init(((int) sampleRate) * multiplier);
    SyncFaustParams();
  }
  
  if (int nSenders = mSenders.GetSize())
  {
    IPeakAvgSender<>** ppSender = mSenders.GetList();

    for (int i = 0; i < nSenders; ++i, ++ppSender)
    {
      IPeakAvgSender<>* pSender = *ppSender;
      pSender->Reset(sampleRate);
    }
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
    
    if (int nSenders = mSenders.GetSize())
    {
      IPeakAvgSender<>** ppSender = mSenders.GetList();

      for (int i = 0; i < nSenders; ++i, ++ppSender)
      {
        IPeakAvgSender<>* pSender = *ppSender;
        auto* pZone = mSenderZones.Get(i);
        sample val = (sample) *pZone;
        sample* valPtr = &val;
        sample* tmp[1] = {valPtr};
        
        // TODO: this is silly, but currently IPeakAvgSender is fed with blocks.
        // This will result in nFrames of the same value in the block
        for (auto s=0;s<nFrames;s++) {
          pSender->ProcessBlock(tmp, 1, mCtrlTagStart + i);
        }
      }
    }
  }
  //    else silence?
}

void IPlugFaust::FreeDSP()
{
  mMidiHandler->stopMidi();
  mMidiUI = nullptr;
  mJSONUI = nullptr;
  mDSP = nullptr;
  mMidiHandler = nullptr;
  mSenders.Empty(true);
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

    if (mParamZones.GetSize() == NParams())
      *(mParamZones.Get(paramIdx)) = mParams.Get(paramIdx)->Value();
    else
      DBGMSG("IPlugFaust-%s:: Missing zone for parameter %s\n", mName.Get(), mParams.Get(paramIdx)->GetName());
  }
}

void IPlugFaust::SetParameterValue(int paramIdx, double nonNormalizedValue)
{
  if (paramIdx < NParams())
  {
    mParams.Get(paramIdx)->Set(nonNormalizedValue);

    if (mParamZones.GetSize() == NParams())
      *(mParamZones.Get(paramIdx)) = nonNormalizedValue;
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
  // mParams.Get(paramIdx)->Set(nonNormalizedValue); // TODO: we are not updating the IPlug parameter

  if (dest)
    *dest = nonNormalizedValue;
  else
    DBGMSG("IPlugFaust-%s:: No parameter named %s\n", mName.Get(), labelToLookup);
}

int IPlugFaust::CreateIPlugParameters(IPlugAPIBase* pPlug, int startIdx, int endIdx, bool setToDefault)
{
  assert(pPlug != nullptr);

  mPlug = pPlug;

  if (NParams() == 0)
    return -1;

  int plugParamIdx = mIPlugParamStartIdx = startIdx;

  if (endIdx == -1)
    endIdx = NParams();

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

void IPlugFaust::AddOrUpdateParam(IParam::EParamType type, const char* name, ffloat* zone, ffloat init, ffloat min, ffloat max, ffloat step)
{
  IParam* pParam = nullptr;

  const int paramIdx = FindExistingParameterWithName(name);

  if (paramIdx > -1)
    pParam = mParams.Get(paramIdx);
  else
    pParam = new IParam();

  auto flags = IParam::kFlagsNone;
  auto unit = fUnit[zone].c_str();
  
  // Create groupStr.c_str() string from mGroupStack
  std::string groupStr;
  if (!mGroupStack.empty()) {
    // Create a vector to store the elements in the correct order
    std::vector<std::string_view> elements;
    std::stack<const char*> tempStack = mGroupStack;
    
    // Extract elements from the stack
    while (!tempStack.empty()) {
      elements.push_back(tempStack.top());
      tempStack.pop();
    }
    
    // Join elements with "/" separator
    for (size_t i = 0; i < elements.size(); ++i) {
      if (i > 0) groupStr += "/";
      groupStr += elements[i];
    }
  }

  if (groupStr.length() > MAX_PARAM_GROUP_LEN-1) {
    groupStr = groupStr.substr(0, MAX_PARAM_GROUP_LEN - 4) + "...";
  }
  
  switch (type)
  {
  case IParam::EParamType::kTypeBool:
    pParam->InitBool(name, static_cast<bool>(init), unit, flags, groupStr.c_str());
    break;
  case IParam::EParamType::kTypeInt:
    pParam->InitInt(name, static_cast<int>(init), static_cast<int>(min), static_cast<int>(max), unit, flags, groupStr.c_str());
    break;
  case IParam::EParamType::kTypeEnum:
    {
      pParam->InitEnum(name, static_cast<int>(init), static_cast<int>(max - min), unit, flags, groupStr.c_str());

      std::vector<std::string> names;
      std::vector<double> values;
      const char* menuDesc = fMenuDescription[zone].c_str();
      if (parseMenuList(menuDesc, names, values)) {
        for (auto i=0; i<names.size(); i++)
        {
          pParam->SetDisplayText(values[i], names[i].c_str());
        }
      }
    }
    break;
  case IParam::EParamType::kTypeDouble:
    {
      auto scale = getScale(zone);
      switch (scale) {
        case MetaDataUI::kLin:
          pParam->InitDouble(name, init, min, max, step, unit, flags, groupStr.c_str(), IParam::ShapeLinear());
          break;
        case MetaDataUI::kLog:
          pParam->InitDouble(name, init, min, max, step, unit, flags, groupStr.c_str(), IParam::ShapePowCurve(2.0));
          break;
        case MetaDataUI::kExp:
          pParam->InitDouble(name, init, min, max, step, unit, flags, groupStr.c_str(), IParam::ShapeExp());
          break;
      }
      break;
    }
  default:
    break;
  }
  
  if (paramIdx == -1)
  {
    mParams.Add(pParam);
  }
  
  mParamZones.Add(zone);
}

void IPlugFaust::BuildParameterMap()
{
  for (auto p = 0; p < NParams(); p++)
  {
    mMap.Insert(mParams.Get(p)->GetName(), mParamZones.Get(p)); // insert will overwrite keys with the same name
  }

  if (mIPlugParamStartIdx > -1 && mPlug != nullptr) // if we've already linked parameters
  {
    CreateIPlugParameters(mPlug, mIPlugParamStartIdx);
  }
}

// TODO: this needs to check meta data too - incase of grouping
int IPlugFaust::FindExistingParameterWithName(const char* name)
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
    *mParamZones.Get(p) = mParams.Get(p)->Value();
  }
}

void IPlugFaust::OnUITimer(Timer& timer)
{
  GUI::updateAllGuis();
  
  if (int nSenders = mSenders.GetSize())
  {
    IPeakAvgSender<>** ppSender = mSenders.GetList();

    for (int i = 0; i < nSenders; ++i, ++ppSender)
    {
      IPeakAvgSender<>* pSender = *ppSender;
      pSender->TransmitData(*mPlug);
    }
  }
}

int IPlugFaust::NParams() const
{
  return mParams.GetSize();
}

void IPlugFaust::BuildUI(UI* pFaustUIToBuild)
{
  mDSP->buildUserInterface(pFaustUIToBuild);
  mDSP->metadata(dynamic_cast<Meta*>(pFaustUIToBuild));
}

int IPlugFaust::GetParamIdxForZone(ffloat* zone)
{
  return mParamZones.Find(zone);
}

#pragma mark -

void IPlugFaust::openTabBox(const char *label)
{
  mGroupStack.push(label);
}

void IPlugFaust::openHorizontalBox(const char *label)
{
  mGroupStack.push(label);
}

void IPlugFaust::openVerticalBox(const char *label)
{
  mGroupStack.push(label);
}

void IPlugFaust::closeBox()
{
  mGroupStack.pop();
}

void IPlugFaust::addButton(const char *label, ffloat *zone)
{
  AddOrUpdateParam(IParam::kTypeBool, label, zone, 0, 0, 1);
}

void IPlugFaust::addCheckButton(const char *label, ffloat *zone)
{
  AddOrUpdateParam(IParam::kTypeBool, label, zone, 0, 0, 1);
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
  if (isRadio(zone)) {
    AddOrUpdateParam(IParam::kTypeInt, label, zone, init, min, max, step);
  }
  else if (isMenu(zone)) {
    AddOrUpdateParam(IParam::kTypeEnum, label, zone, init, min, max, step);
  }
  else if (isNumerical(zone)) {
    AddOrUpdateParam(IParam::kTypeEnum, label, zone, init, min, max, step);
  }
}

void IPlugFaust::addHorizontalBargraph(const char *label, ffloat *zone, ffloat min, ffloat max)
{
  mSenders.Add(new IPeakAvgSender<>());
  mSenderZones.Add(zone);
}

void IPlugFaust::addVerticalBargraph(const char *label, ffloat *zone, ffloat min, ffloat max)
{
  mSenders.Add(new IPeakAvgSender<>());
  mSenderZones.Add(zone);
}

void IPlugFaust::declare(FAUSTFLOAT* zone, const char* key, const char* value)
{
  MetaDataUI::declare(zone, key, value);
}
