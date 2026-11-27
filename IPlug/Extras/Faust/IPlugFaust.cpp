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

#include <cstring>
#include <map>

#include "faust/gui/GUI.h"
#include "faust/gui/JSONUI.h"

// Static members of Faust's GUI base class (used by MidiUI). If you combine IPlugFaust with other Faust architecture code that defines these, define FAUST_GUI_STATICS_DEFINED
#ifndef FAUST_GUI_STATICS_DEFINED
std::list<GUI*> GUI::fGuiList;
ztimedmap GUI::gTimedZoneMap;
#endif

using namespace iplug;

namespace
{
  // Faust names anonymous groups "0x00"
  std::string CleanLabel(const char* label)
  {
    if (!label || strcmp(label, "0x00") == 0)
      return "";

    return label;
  }
}

IPlugFaust::IPlugFaust(const char* name, int nVoices, int ctrlTagStart)
: mName(name)
, mNVoices(nVoices)
, mCtrlTagStart(ctrlTagStart)
{
}

IPlugFaust::~IPlugFaust()
{
  ReleaseDSP();
}

void IPlugFaust::ReleaseDSP()
{
  if (mUITimer)
  {
    mUITimer->Stop();
    mUITimer = nullptr;
  }

  std::unique_ptr<MidiUI> pMidiUI;
  std::unique_ptr<::dsp> pDSP;
  std::unique_ptr<iplug2_midi_handler> pMidiHandler;

  {
    std::lock_guard<std::mutex> lock(mDSPMutex);
    std::swap(mMidiUI, pMidiUI);
    std::swap(mDSP, pDSP);
    std::swap(mMidiHandler, pMidiHandler);
    mParams.clear();
    mBargraphs.clear();
  }

  // A polyphonic DSP unregisters itself from the MidiUI, which unregisters itself from the MIDI handler
  pDSP = nullptr;
  pMidiUI = nullptr;
  pMidiHandler = nullptr;
}

void IPlugFaust::SetMaxChannelCount(int maxNInputs, int maxNOutputs)
{
  mMaxNInputs = maxNInputs;
  mMaxNOutputs = maxNOutputs;
}

void IPlugFaust::SetSampleRate(double sampleRate)
{
  mSampleRate = sampleRate;

  {
    std::lock_guard<std::mutex> lock(mDSPMutex);

    if (mDSP)
      mDSP->init(static_cast<int>(sampleRate)); // NOTE: this resets the zones to their default values
  }

  SyncFaustParams();
}

void IPlugFaust::ProcessMidiMsg(const IMidiMsg& msg)
{
  std::unique_lock<std::mutex> lock(mDSPMutex, std::try_to_lock);

  if (lock.owns_lock() && mMidiHandler)
    mMidiHandler->decodeMessage(msg);
}

void IPlugFaust::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  // The DSP may be swapped on the main thread (recompilation). In that case we output silence for this block
  std::unique_lock<std::mutex> lock(mDSPMutex, std::try_to_lock);

  int nOutputsProcessed = 0;

  if (lock.owns_lock() && mDSP && !mChannelCountError && !mHasSoundfile)
  {
    mDSP->compute(nFrames, inputs, outputs);
    nOutputsProcessed = mDSP->getNumOutputs();
  }

  if (mMaxNOutputs > -1)
  {
    for (int c = nOutputsProcessed; c < mMaxNOutputs; c++)
    {
      memset(outputs[c], 0, nFrames * sizeof(sample));
    }
  }
}

#pragma mark - Parameters

int IPlugFaust::CreateIPlugParameters(IPlugAPIBase* pPlug, int startIdx, int endIdx, bool setToDefault)
{
  assert(pPlug != nullptr);

  mPlug = pPlug;

  if (NParams() == 0)
    return -1;

  mPlugParamStartIdx = startIdx;
  mPlugParamEndIdx = (endIdx < 0 || endIdx > NParams()) ? NParams() : endIdx;

  for (int p = 0; p < mPlugParamEndIdx; p++)
  {
    const int plugParamIdx = mPlugParamStartIdx + p;

    if (plugParamIdx >= pPlug->NParams())
    {
      DBGMSG("IPlugFaust-%s: the plug-in doesn't have enough parameters for the Faust DSP (needs %i)\n", mName.Get(), mPlugParamStartIdx + mPlugParamEndIdx);
      mPlugParamEndIdx = p;
      break;
    }

    IParam* pPlugParam = pPlug->GetParam(plugParamIdx);
    const double currentValueNormalized = pPlugParam->GetNormalized();
    pPlugParam->Init(*mParams[p].param);

    if (setToDefault)
      pPlugParam->SetToDefault();
    else
      pPlugParam->SetNormalized(currentValueNormalized);
  }

  SyncFaustParams();

  return mPlugParamStartIdx;
}

bool IPlugFaust::OnParamChange(int plugParamIdx)
{
  if (!mPlug || mPlugParamStartIdx < 0)
    return false;

  const int faustParamIdx = plugParamIdx - mPlugParamStartIdx;

  if (faustParamIdx < 0 || faustParamIdx >= mPlugParamEndIdx)
    return false;

  WriteZone(faustParamIdx, mPlug->GetParam(plugParamIdx)->Value());
  return true;
}

void IPlugFaust::SetParameterValueNormalised(int faustParamIdx, double normalizedValue)
{
  if (faustParamIdx < 0 || faustParamIdx >= NParams())
  {
    DBGMSG("IPlugFaust-%s: no parameter %i\n", mName.Get(), faustParamIdx);
    return;
  }

  WriteZone(faustParamIdx, mParams[faustParamIdx].param->FromNormalized(normalizedValue));
}

void IPlugFaust::SetParameterValue(int faustParamIdx, double value)
{
  if (faustParamIdx < 0 || faustParamIdx >= NParams())
  {
    DBGMSG("IPlugFaust-%s: no parameter %i\n", mName.Get(), faustParamIdx);
    return;
  }

  WriteZone(faustParamIdx, value);
}

void IPlugFaust::SetParameterValue(const char* labelOrPath, double value)
{
  std::unique_lock<std::mutex> lock(mDSPMutex, std::try_to_lock);

  if (!lock.owns_lock())
    return;

  for (auto& p : mParams)
  {
    if (p.path == labelOrPath || strcmp(p.param->GetName(), labelOrPath) == 0)
    {
      *p.zone = static_cast<FAUSTFLOAT>(value);
      return;
    }
  }

  DBGMSG("IPlugFaust-%s: no parameter named %s\n", mName.Get(), labelOrPath);
}

void IPlugFaust::SyncFaustParams()
{
  for (int p = 0; p < NParams(); p++)
  {
    const int plugParamIdx = GetPlugParamIdx(p);

    if (plugParamIdx > kNoParameter)
      WriteZone(p, mPlug->GetParam(plugParamIdx)->Value());
    else
      WriteZone(p, mParams[p].param->Value());
  }
}

int IPlugFaust::GetFaustParamIdxForZone(FAUSTFLOAT* zone) const
{
  for (int p = 0; p < NParams(); p++)
  {
    if (mParams[p].zone == zone)
      return p;
  }

  return kNoParameter;
}

int IPlugFaust::GetPlugParamIdx(int faustParamIdx) const
{
  if (!mPlug || mPlugParamStartIdx < 0 || faustParamIdx < 0 || faustParamIdx >= mPlugParamEndIdx)
    return kNoParameter;

  return mPlugParamStartIdx + faustParamIdx;
}

void IPlugFaust::WriteZone(int faustParamIdx, double value)
{
  std::unique_lock<std::mutex> lock(mDSPMutex, std::try_to_lock);

  // If the lock is held, the DSP is being swapped and SyncFaustParams() will be called afterwards
  if (!lock.owns_lock() || faustParamIdx >= NParams())
    return;

  Param& p = mParams[faustParamIdx];

  if (!p.enumValues.empty())
  {
    const int idx = Clip(static_cast<int>(value + 0.5), 0, static_cast<int>(p.enumValues.size()) - 1);
    value = p.enumValues[idx];
  }

  *p.zone = static_cast<FAUSTFLOAT>(value);
}

void IPlugFaust::RelinkParams()
{
  if (!mPlug || mPlugParamStartIdx < 0)
    return;

  // NOTE: IParam::Init() keeps the type of a parameter that was already initialized, so a parameter that changes from e.g. a slider to a checkbox stays a double
  mPlugParamEndIdx = std::min(NParams(), mPlug->NParams() - mPlugParamStartIdx);

  if (mPlugParamEndIdx < NParams())
    DBGMSG("IPlugFaust-%s: the plug-in doesn't have enough parameters for the Faust DSP (needs %i)\n", mName.Get(), mPlugParamStartIdx + NParams());

  for (int p = 0; p < mPlugParamEndIdx; p++)
  {
    IParam* pPlugParam = mPlug->GetParam(mPlugParamStartIdx + p);
    pPlugParam->Init(*mParams[p].param);
    pPlugParam->Set(mParams[p].param->Value());
  }

  mParamsChanged = true;
}

void IPlugFaust::InformHostIfParamsChanged()
{
  if (mParamsChanged && mPlug)
  {
    mParamsChanged = false;
    mPlug->InformHostOfParameterDetailsChange();
  }
}

#pragma mark - DSP

void IPlugFaust::SetDSP(::dsp* pMonoDSP)
{
  assert(pMonoDSP != nullptr);

  // Polyphony is enabled with declare options "[midi:on][nvoices:N]" or the nVoices constructor argument
  bool midi = false, midiSync = false;
  int nVoices = 0;
  MidiMeta::analyse(pMonoDSP, midi, midiSync, nVoices);

  if (nVoices == 0 && mNVoices > 1)
    nVoices = mNVoices;

  auto pMidiHandler = std::make_unique<iplug2_midi_handler>();
  std::unique_ptr<::dsp> pDSP;

  if (nVoices > 0)
    pDSP.reset(new mydsp_poly(pMonoDSP, nVoices, true /* control */, true /* group */));
  else
    pDSP.reset(pMonoDSP);

  // MidiUI maps MIDI to the zones with [midi:...] metadata. A polyphonic DSP also registers itself with it, to receive notes
  auto pMidiUI = std::make_unique<MidiUI>(pMidiHandler.get());
  pDSP->buildUserInterface(pMidiUI.get());
  pMidiUI->run();

  pDSP->init(mSampleRate > 0. ? static_cast<int>(mSampleRate) : static_cast<int>(DEFAULT_SAMPLE_RATE));

  JSONUI jsonUI(pDSP->getNumInputs(), pDSP->getNumOutputs());
  pDSP->buildUserInterface(&jsonUI);
  pDSP->metadata(&jsonUI);

  // Keep values of parameters with the same path, when the DSP is recompiled
  std::map<std::string, double> prevValues;

  for (int p = 0; p < NParams(); p++)
  {
    const int plugParamIdx = GetPlugParamIdx(p);
    prevValues[mParams[p].path] = plugParamIdx > kNoParameter ? mPlug->GetParam(plugParamIdx)->Value() : mParams[p].param->Value();
  }

  const bool hadDSP = mDSP != nullptr;

  {
    std::lock_guard<std::mutex> lock(mDSPMutex);

    // The previous objects are destroyed when they go out of scope, after the lock has been released
    std::swap(mMidiUI, pMidiUI);
    std::swap(mDSP, pDSP);
    std::swap(mMidiHandler, pMidiHandler);

    BuildDescription(mDSP.get());
    mJSON = jsonUI.JSON();

    mChannelCountError = (mMaxNInputs > -1 && mDSP->getNumInputs() > mMaxNInputs) ||
                         (mMaxNOutputs > -1 && mDSP->getNumOutputs() > mMaxNOutputs);

    for (auto& p : mParams)
    {
      auto it = prevValues.find(p.path);

      if (it != prevValues.end())
        p.param->Set(it->second);
    }
  }

  if (mChannelCountError)
  {
    DBGMSG("IPlugFaust-%s: the DSP has %i inputs and %i outputs, but the plug-in only provides %i inputs and %i outputs. Outputting silence.\n",
           mName.Get(), mDSP->getNumInputs(), mDSP->getNumOutputs(), mMaxNInputs, mMaxNOutputs);
  }

  // A polyphonic DSP unregisters itself from the MidiUI, which unregisters itself from the MIDI handler
  pDSP = nullptr;
  pMidiUI = nullptr;
  pMidiHandler = nullptr;

  if (hadDSP)
    RelinkParams();

  SyncFaustParams();

  if (!mBargraphs.empty() && !mUITimer)
    mUITimer = std::unique_ptr<Timer>(Timer::Create(std::bind(&IPlugFaust::OnUITimer, this, std::placeholders::_1), FAUST_UI_INTERVAL));
}

void IPlugFaust::BuildDescription(::dsp* pDSP)
{
  mParams.clear();
  mBargraphs.clear();
  mHasSoundfile = false;
  clearMetadata();

  mUIRoot = FaustUIItem();
  mUIRoot.type = FaustUIItem::EType::VBox;
  mGroupStack.clear();
  mGroupStack.push_back(&mUIRoot);

  pDSP->buildUserInterface(this);

  mGroupStack.clear();

  // Faust DSPs normally have a single top-level group. If so, make that the root
  if (mUIRoot.children.size() == 1 && mUIRoot.children[0].IsGroup())
  {
    FaustUIItem root = std::move(mUIRoot.children[0]);
    mUIRoot = std::move(root);
  }
}

void IPlugFaust::BuildUI(UI* pUI)
{
  if (mDSP)
  {
    mDSP->buildUserInterface(pUI);

    if (auto* pMeta = dynamic_cast<Meta*>(pUI))
      mDSP->metadata(pMeta);
  }
}

void IPlugFaust::OnUITimer(Timer& timer)
{
  if (!mPlug)
    return;

  // Zones are written on the audio thread, here we just read the latest value, like Faust's own GUIs
  for (auto& b : mBargraphs)
  {
    const double range = b.max - b.min;
    const double normalized = range != 0. ? Clip((static_cast<double>(*b.zone) - b.min) / range, 0., 1.) : 0.;

    if (normalized != b.lastSentValue)
    {
      b.lastSentValue = normalized;
      mPlug->SendControlValueFromDelegate(b.ctrlTag, normalized);
    }
  }
}

#pragma mark - Faust UI interface

void IPlugFaust::OpenGroup(FaustUIItem::EType type, const char* label)
{
  std::string cleanLabel;
  std::map<std::string, std::string> metadata;
  extractMetadata(CleanLabel(label), cleanLabel, metadata);

  FaustUIItem& parent = *mGroupStack.back();
  parent.children.emplace_back();
  FaustUIItem& group = parent.children.back();
  group.type = type;
  group.label = cleanLabel;
  group.path = parent.path + "/" + cleanLabel;
  group.hidden = metadata.count("hidden") && metadata["hidden"] == "1";
  group.tooltip = metadata.count("tooltip") ? metadata["tooltip"] : "";

  mGroupStack.push_back(&group);
}

void IPlugFaust::openTabBox(const char* label)
{
  OpenGroup(FaustUIItem::EType::TabBox, label);
}

void IPlugFaust::openHorizontalBox(const char* label)
{
  OpenGroup(FaustUIItem::EType::HBox, label);
}

void IPlugFaust::openVerticalBox(const char* label)
{
  OpenGroup(FaustUIItem::EType::VBox, label);
}

void IPlugFaust::closeBox()
{
  if (mGroupStack.size() > 1)
    mGroupStack.pop_back();
}

FaustUIItem& IPlugFaust::AddItem(FaustUIItem::EType type, const char* label, FAUSTFLOAT* zone)
{
  FaustUIItem& parent = *mGroupStack.back();
  parent.children.emplace_back();
  FaustUIItem& item = parent.children.back();
  item.type = type;
  item.label = CleanLabel(label);
  item.path = parent.path + "/" + item.label;
  item.zone = zone;
  item.hidden = isHidden(zone);
  item.knob = isKnob(zone);
  item.led = isLed(zone);
  item.numerical = isNumerical(zone);
  item.menu = isMenu(zone);
  item.radio = isRadio(zone);
  item.unit = getUnit(zone);
  item.tooltip = getTooltip(zone);

  if (item.menu)
    item.options = getMenuDescription(zone);
  else if (item.radio)
    item.options = getRadioDescription(zone);

  return item;
}

void IPlugFaust::AddParam(FaustUIItem& item, IParam::EParamType type)
{
  // The parameter group is the path of the containing boxes
  std::string group;

  for (size_t i = 1; i < mGroupStack.size(); i++)
  {
    const std::string& label = mGroupStack[i]->label;

    if (label.empty())
      continue;

    if (!group.empty())
      group += "/";

    group += label;
  }

  if (group.length() > MAX_PARAM_GROUP_LEN - 1)
    group = group.substr(0, MAX_PARAM_GROUP_LEN - 4) + "...";

  Param p;
  p.zone = item.zone;
  p.path = item.path;

  const char* name = item.label.c_str();
  const char* unit = item.unit.c_str();
  const auto flags = IParam::kFlagsNone;

  switch (type)
  {
    case IParam::kTypeBool:
      p.param->InitBool(name, item.init > 0.5, unit, flags, group.c_str());
      break;
    case IParam::kTypeEnum:
    {
      const int nOptions = static_cast<int>(item.options.size());
      int defaultIdx = 0;

      for (int i = 0; i < nOptions; i++)
      {
        p.enumValues.push_back(item.options[i].second);

        if (item.options[i].second == item.init)
          defaultIdx = i;
      }

      p.param->InitEnum(name, defaultIdx, nOptions, unit, flags, group.c_str());

      for (int i = 0; i < nOptions; i++)
        p.param->SetDisplayText(i, item.options[i].first.c_str());

      break;
    }
    case IParam::kTypeDouble:
    default:
    {
      const double step = item.step > 0. ? item.step : 0.001;

      switch (getScale(item.zone))
      {
        case MetaDataUI::kLog:
          if (item.min > 0.)
            p.param->InitDouble(name, item.init, item.min, item.max, step, unit, flags, group.c_str(), IParam::ShapeExp());
          else
            p.param->InitDouble(name, item.init, item.min, item.max, step, unit, flags, group.c_str(), IParam::ShapePowCurve(3.));
          break;
        case MetaDataUI::kExp:
          p.param->InitDouble(name, item.init, item.min, item.max, step, unit, flags, group.c_str(), IParam::ShapePowCurve(1./3.));
          break;
        case MetaDataUI::kLin:
        default:
          p.param->InitDouble(name, item.init, item.min, item.max, step, unit, flags, group.c_str());
          break;
      }
      break;
    }
  }

  item.paramIdx = static_cast<int>(mParams.size());
  mParams.push_back(std::move(p));
}

void IPlugFaust::addButton(const char* label, FAUSTFLOAT* zone)
{
  FaustUIItem& item = AddItem(FaustUIItem::EType::Button, label, zone);
  AddParam(item, IParam::kTypeBool);
}

void IPlugFaust::addCheckButton(const char* label, FAUSTFLOAT* zone)
{
  FaustUIItem& item = AddItem(FaustUIItem::EType::CheckButton, label, zone);
  AddParam(item, IParam::kTypeBool);
}

void IPlugFaust::addVerticalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
  FaustUIItem& item = AddItem(FaustUIItem::EType::VSlider, label, zone);
  item.init = init; item.min = min; item.max = max; item.step = step;
  AddParam(item, (item.menu || item.radio) ? IParam::kTypeEnum : IParam::kTypeDouble);
}

void IPlugFaust::addHorizontalSlider(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
  FaustUIItem& item = AddItem(FaustUIItem::EType::HSlider, label, zone);
  item.init = init; item.min = min; item.max = max; item.step = step;
  AddParam(item, (item.menu || item.radio) ? IParam::kTypeEnum : IParam::kTypeDouble);
}

void IPlugFaust::addNumEntry(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
  FaustUIItem& item = AddItem(FaustUIItem::EType::NumEntry, label, zone);
  item.init = init; item.min = min; item.max = max; item.step = step;
  AddParam(item, (item.menu || item.radio) ? IParam::kTypeEnum : IParam::kTypeDouble);
}

void IPlugFaust::addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
{
  FaustUIItem& item = AddItem(FaustUIItem::EType::HBargraph, label, zone);
  item.min = min; item.max = max;
  item.ctrlTag = mCtrlTagStart + static_cast<int>(mBargraphs.size());
  mBargraphs.push_back({zone, item.min, item.max, item.ctrlTag});
}

void IPlugFaust::addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
{
  FaustUIItem& item = AddItem(FaustUIItem::EType::VBargraph, label, zone);
  item.min = min; item.max = max;
  item.ctrlTag = mCtrlTagStart + static_cast<int>(mBargraphs.size());
  mBargraphs.push_back({zone, item.min, item.max, item.ctrlTag});
}

void IPlugFaust::addSoundfile(const char* label, const char* filename, Soundfile** sfZone)
{
  // The soundfile zone would need to be filled with a Soundfile, otherwise compute() crashes
  DBGMSG("IPlugFaust-%s: soundfiles are not supported (%s), the DSP will output silence\n", mName.Get(), label);
  mHasSoundfile = true;
}

void IPlugFaust::declare(FAUSTFLOAT* zone, const char* key, const char* value)
{
  MetaDataUI::declare(zone, key, value);
}
