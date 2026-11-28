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

  std::vector<Param> params;

  {
    ScopedDSPReplace replace(mDSPGuard);
    std::swap(mMidiUI, pMidiUI);
    std::swap(mDSP, pDSP);
    std::swap(mMidiHandler, pMidiHandler);
    std::swap(mParams, params);
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
    ScopedDSPReplace replace(mDSPGuard);

    if (mDSP)
      mDSP->init(static_cast<int>(sampleRate)); // NOTE: this resets the zones to their default values
  }

  SyncFaustParams();
}

void IPlugFaust::ProcessMidiMsg(const IMidiMsg& msg)
{
  // Messages are only dropped while the DSP is being replaced. The new DSP has no sounding voices, so a dropped note-off can't leave a stuck note
  ScopedDSPUser user(mDSPGuard);

  if (user.IsEntered() && mMidiHandler)
    mMidiHandler->decodeMessage(msg);
}

void IPlugFaust::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  // If the DSP is being replaced on the main thread (recompilation), output silence for this block. This never blocks
  ScopedDSPUser user(mDSPGuard);

  int nOutputsProcessed = 0;

  if (user.IsEntered() && mDSP && !mChannelCountError && !mHasSoundfile)
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

  // The mapping is recorded even if there are no parameters yet, so that a recompiled DSP that has some can be linked
  mPlugParamStartIdx = startIdx;
  mPlugParamEndIdx = (endIdx < 0 || endIdx > NParams()) ? NParams() : endIdx;

  if (NParams() == 0)
    return -1;

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
  // mParams is replaced with the DSP, so it is only used while the DSP can't be replaced
  ScopedDSPUser user(mDSPGuard);

  if (!user.IsEntered())
    return;

  if (faustParamIdx < 0 || faustParamIdx >= NParams())
  {
    DBGMSG("IPlugFaust-%s: no parameter %i\n", mName.Get(), faustParamIdx);
    return;
  }

  WriteZoneUnguarded(faustParamIdx, mParams[faustParamIdx].param->FromNormalized(normalizedValue));
}

void IPlugFaust::SetParameterValue(int faustParamIdx, double value)
{
  WriteZone(faustParamIdx, value);
}

void IPlugFaust::SetParameterValue(const char* labelOrPath, double value)
{
  ScopedDSPUser user(mDSPGuard);

  if (!user.IsEntered())
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
  ScopedDSPUser user(mDSPGuard);

  // If the DSP is being replaced, the thread replacing it syncs the parameters afterwards
  if (!user.IsEntered())
    return;

  for (int p = 0; p < NParams(); p++)
  {
    const int plugParamIdx = GetPlugParamIdx(p);

    if (plugParamIdx > kNoParameter)
      WriteZoneUnguarded(p, mPlug->GetParam(plugParamIdx)->Value());
    else
      WriteZoneUnguarded(p, mParams[p].param->Value());
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
  ScopedDSPUser user(mDSPGuard);

  // If the DSP is being replaced, SyncFaustParams() applies the parameter values to the new DSP afterwards
  if (user.IsEntered())
    WriteZoneUnguarded(faustParamIdx, value);
}

void IPlugFaust::WriteZoneUnguarded(int faustParamIdx, double value)
{
  if (faustParamIdx < 0 || faustParamIdx >= NParams())
    return;

  Param& p = mParams[faustParamIdx];

  if (!p.enumValues.empty())
  {
    const int idx = Clip(static_cast<int>(value + 0.5), 0, static_cast<int>(p.enumValues.size()) - 1);
    value = p.enumValues[idx];
  }

  *p.zone = static_cast<FAUSTFLOAT>(value);
}

namespace
{
  // Whether two parameters have the same definition (ignoring their values), so that re-initializing one from the other would change nothing
  bool SameDefinition(const IParam& a, const IParam& b)
  {
    if (a.Type() != b.Type() || strcmp(a.GetName(), b.GetName()) != 0 || strcmp(a.GetLabel(), b.GetLabel()) != 0 ||
        strcmp(a.GetGroup(), b.GetGroup()) != 0 || a.GetMin() != b.GetMin() || a.GetMax() != b.GetMax() ||
        a.GetStep() != b.GetStep() || a.GetDefault() != b.GetDefault() || a.GetFlags() != b.GetFlags() ||
        a.GetShapeID() != b.GetShapeID() || a.GetShapeValue() != b.GetShapeValue() || a.NDisplayTexts() != b.NDisplayTexts())
      return false;

    for (int i = 0; i < a.NDisplayTexts(); i++)
    {
      double valueA, valueB;
      const char* textA = a.GetDisplayTextAtIdx(i, &valueA);
      const char* textB = b.GetDisplayTextAtIdx(i, &valueB);

      if (valueA != valueB || strcmp(textA, textB) != 0)
        return false;
    }

    return true;
  }
}

void IPlugFaust::RelinkParams()
{
  if (!mPlug || mPlugParamStartIdx < 0)
    return;

  const int prevEndIdx = mPlugParamEndIdx;
  mPlugParamEndIdx = std::min(NParams(), mPlug->NParams() - mPlugParamStartIdx);

  if (mPlugParamEndIdx < NParams())
    DBGMSG("IPlugFaust-%s: the plug-in doesn't have enough parameters for the Faust DSP (needs %i)\n", mName.Get(), mPlugParamStartIdx + NParams());

  bool changed = mPlugParamEndIdx != prevEndIdx;

  // Only parameters whose definition changed are re-initialized. Re-initializing a parameter isn't thread safe (the host may be using it
  // on the audio thread), so this keeps the risk to recompilations that actually change the parameters
  for (int p = 0; p < mPlugParamEndIdx; p++)
  {
    IParam* pPlugParam = mPlug->GetParam(mPlugParamStartIdx + p);
    const IParam& faustParam = *mParams[p].param;

    if (!SameDefinition(*pPlugParam, faustParam))
    {
      pPlugParam->Init(faustParam);
      pPlugParam->Set(faustParam.Value());
      changed = true;
    }
  }

  if (changed)
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

  // Build the new parameters and UI description (into mNew...), before taking the audio thread out
  BuildDescription(pDSP.get());

  // Keep the values of parameters with the same path, when the DSP is recompiled
  for (auto& newParam : mNewParams)
  {
    for (int p = 0; p < NParams(); p++)
    {
      if (mParams[p].path == newParam.path)
      {
        const int plugParamIdx = GetPlugParamIdx(p);
        newParam.param->Set(plugParamIdx > kNoParameter ? mPlug->GetParam(plugParamIdx)->Value() : mParams[p].param->Value());
        break;
      }
    }
  }

  const bool channelCountError = (mMaxNInputs > -1 && pDSP->getNumInputs() > mMaxNInputs) ||
                                 (mMaxNOutputs > -1 && pDSP->getNumOutputs() > mMaxNOutputs);

  const bool hadDSP = mDSP != nullptr;
  std::string json = jsonUI.JSON();

  {
    // Only pointers and containers are swapped while the audio thread is kept out.
    // The previous objects are destroyed afterwards, when they go out of scope
    ScopedDSPReplace replace(mDSPGuard);

    std::swap(mMidiUI, pMidiUI);
    std::swap(mDSP, pDSP);
    std::swap(mMidiHandler, pMidiHandler);
    std::swap(mParams, mNewParams);
    std::swap(mBargraphs, mNewBargraphs);
    std::swap(mUIRoot, mNewUIRoot);
    std::swap(mJSON, json);
    mChannelCountError = channelCountError;
    mHasSoundfile = mNewHasSoundfile;
  }

  mNewParams.clear();
  mNewBargraphs.clear();
  mNewUIRoot = FaustUIItem();

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
  // Only uses mNew... members, so the audio thread can keep using the current ones
  mNewParams.clear();
  mNewBargraphs.clear();
  mNewHasSoundfile = false;
  clearMetadata();

  mNewUIRoot = FaustUIItem();
  mNewUIRoot.type = FaustUIItem::EType::VBox;
  mGroupStack.clear();
  mGroupStack.push_back(&mNewUIRoot);

  pDSP->buildUserInterface(this);

  mGroupStack.clear();

  // Faust DSPs normally have a single top-level group. If so, make that the root
  if (mNewUIRoot.children.size() == 1 && mNewUIRoot.children[0].IsGroup())
  {
    FaustUIItem root = std::move(mNewUIRoot.children[0]);
    mNewUIRoot = std::move(root);
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
    if (b.hidden)
      continue;

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

  item.paramIdx = static_cast<int>(mNewParams.size());
  mNewParams.push_back(std::move(p));
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

void IPlugFaust::AddBargraph(FaustUIItem::EType type, const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
{
  FaustUIItem& item = AddItem(type, label, zone);
  item.min = min; item.max = max;
  item.ctrlTag = mCtrlTagStart + static_cast<int>(mNewBargraphs.size());

  // A hidden bargraph, or one in a hidden group, gets no control in IGraphicsFaustUI, so no values are sent to its tag
  bool hidden = item.hidden;

  for (const FaustUIItem* pGroup : mGroupStack)
    hidden = hidden || pGroup->hidden;

  Bargraph b;
  b.zone = zone;
  b.min = item.min;
  b.max = item.max;
  b.ctrlTag = item.ctrlTag;
  b.hidden = hidden;
  mNewBargraphs.push_back(b);
}

void IPlugFaust::addHorizontalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
{
  AddBargraph(FaustUIItem::EType::HBargraph, label, zone, min, max);
}

void IPlugFaust::addVerticalBargraph(const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
{
  AddBargraph(FaustUIItem::EType::VBargraph, label, zone, min, max);
}

void IPlugFaust::addSoundfile(const char* label, const char* filename, Soundfile** sfZone)
{
  // The soundfile zone would need to be filled with a Soundfile, otherwise compute() crashes
  DBGMSG("IPlugFaust-%s: soundfiles are not supported (%s), the DSP will output silence\n", mName.Get(), label);
  mNewHasSoundfile = true;
}

void IPlugFaust::declare(FAUSTFLOAT* zone, const char* key, const char* value)
{
  MetaDataUI::declare(zone, key, value);
}
