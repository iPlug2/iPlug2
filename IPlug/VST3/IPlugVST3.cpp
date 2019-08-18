/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include <cstdio>

#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"

#include "IPlugVST3.h"

using namespace iplug;
using namespace Steinberg;
using namespace Vst;

#include "IPlugVST3_Parameter.h"

#pragma mark - IPlugVST3 Constructor/Destructor

IPlugVST3::IPlugVST3(const InstanceInfo& info, const Config& config)
: IPlugAPIBase(config, kAPIVST3)
, IPlugVST3ProcessorBase(config, *this)
, mView(nullptr)
{
  CreateTimer();
}

IPlugVST3::~IPlugVST3() {}

#pragma mark AudioEffect overrides

tresult PLUGIN_API IPlugVST3::initialize(FUnknown* context)
{
  TRACE;

  if (SingleComponentEffect::initialize(context) == kResultOk)
  {
    IPlugVST3ProcessorBase::Initialize(this);
    IPlugVST3ControllerBase::Initialize(this, parameters, IsInstrument());

    IPlugVST3GetHost(this, context);
    OnHostIdentified();
    OnParamReset(kReset);
    
    return kResultOk;
  }

  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::terminate()
{
  TRACE;

  return SingleComponentEffect::terminate();
}

tresult PLUGIN_API IPlugVST3::setBusArrangements(SpeakerArrangement* pInputBusArrangements, int32 numInBuses, SpeakerArrangement* pOutputBusArrangements, int32 numOutBuses)
{
  TRACE;

  SetBusArrangments(pInputBusArrangements, numInBuses, pOutputBusArrangements, numOutBuses);
  return kResultTrue;
}

tresult PLUGIN_API IPlugVST3::setActive(TBool state)
{
  TRACE;

  OnActivate((bool) state);
  return SingleComponentEffect::setActive(state);
}

tresult PLUGIN_API IPlugVST3::setupProcessing(ProcessSetup& newSetup)
{
  TRACE;

  return SetupProcessing(newSetup, processSetup) ? kResultOk : kResultFalse;
}

tresult PLUGIN_API IPlugVST3::process(ProcessData& data)
{
  TRACE;

  Process(data, processSetup, audioInputs, audioOutputs, mMidiMsgsFromEditor, mMidiMsgsFromProcessor, mSysExDataFromEditor, mSysexBuf);
  return kResultOk;
}

tresult PLUGIN_API IPlugVST3::canProcessSampleSize(int32 symbolicSampleSize)
{
  return CanProcessSampleSize(symbolicSampleSize) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API IPlugVST3::setState(IBStream* pState)
{
  TRACE;
  
  return IPlugVST3State::SetState(this, pState) ? kResultOk :kResultFalse;
}

tresult PLUGIN_API IPlugVST3::getState(IBStream* pState)
{
  TRACE;
  
  return IPlugVST3State::GetState(this, pState) ? kResultOk :kResultFalse;
}

#pragma mark IEditController overrides
ParamValue PLUGIN_API IPlugVST3::getParamNormalized(ParamID tag)
{
  if (tag >= kBypassParam)
    return EditControllerEx1::getParamNormalized(tag);
  
  return IPlugVST3ControllerBase::getParamNormalized(this, tag);
}

tresult PLUGIN_API IPlugVST3::setParamNormalized(ParamID tag, ParamValue value)
{
  IPlugVST3ControllerBase::setParamNormalized(this, tag, value);
  
  return EditControllerEx1::setParamNormalized(tag, value);
}

IPlugView* PLUGIN_API IPlugVST3::createView(const char* name)
{
  if (name && strcmp(name, "editor") == 0)
  {
    mView = new ViewType(*this);
    return mView;
  }
  
  return 0;
}

tresult PLUGIN_API IPlugVST3::setEditorState(IBStream* pState)
{
  // Currently nothing to do here
  return kResultOk;
}

tresult PLUGIN_API IPlugVST3::getEditorState(IBStream* pState)
{
  // Currently nothing to do here
  return kResultOk;
}

tresult PLUGIN_API IPlugVST3::setComponentState(IBStream* pState)
{
  // We get the state through setState so do nothing here
  return kResultOk;
}

#pragma mark IUnitInfo overrides

int32 PLUGIN_API IPlugVST3::getUnitCount()
{
  TRACE;

  return NParamGroups() + 1;
}

tresult PLUGIN_API IPlugVST3::getUnitInfo(int32 unitIndex, UnitInfo& info)
{
  TRACE;

  if (unitIndex == 0)
  {
    info.id = kRootUnitId;
    info.parentUnitId = kNoParentUnitId;
    UString name(info.name, 128);
    name.fromAscii("Root Unit");
#ifdef VST3_PRESET_LIST
    info.programListId = kPresetParam;
#else
    info.programListId = kNoProgramListId;
#endif
    return kResultTrue;
  }
  else if (unitIndex > 0 && NParamGroups())
  {
    info.id = unitIndex;
    info.parentUnitId = kRootUnitId;
    info.programListId = kNoProgramListId;

    UString name(info.name, 128);
    name.fromAscii(GetParamGroupName(unitIndex-1));

    return kResultTrue;
  }

  return kResultFalse;
}

int32 PLUGIN_API IPlugVST3::getProgramListCount()
{
#ifdef VST3_PRESET_LIST
  return (NPresets() > 0);
#else
  return 0;
#endif
}

tresult PLUGIN_API IPlugVST3::getProgramListInfo(int32 listIndex, ProgramListInfo& info /*out*/)
{
  if (listIndex == 0)
  {
    info.id = kPresetParam;
    info.programCount = (int32) NPresets();
    UString name(info.name, 128);
    name.fromAscii("Factory Presets");
    return kResultTrue;
  }
  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::getProgramName(ProgramListID listId, int32 programIndex, String128 name /*out*/)
{
  if (listId == kPresetParam)
  {
    Steinberg::UString(name, 128).fromAscii(GetPresetName(programIndex));
    return kResultTrue;
  }
  return kResultFalse;
}

#pragma mark IPlugAPIBase overrides

void IPlugVST3::BeginInformHostOfParamChange(int idx)
{
  Trace(TRACELOC, "%d", idx);
  beginEdit(idx);
}

void IPlugVST3::InformHostOfParamChange(int idx, double normalizedValue)
{
  Trace(TRACELOC, "%d:%f", idx, normalizedValue);
  performEdit(idx, normalizedValue);
}

void IPlugVST3::EndInformHostOfParamChange(int idx)
{
  Trace(TRACELOC, "%d", idx);
  endEdit(idx);
}

void IPlugVST3::InformHostOfParameterDetailsChange()
{
  FUnknownPtr<IComponentHandler>handler(componentHandler);
  handler->restartComponent(kParamTitlesChanged);
}

bool IPlugVST3::EditorResizeFromDelegate(int viewWidth, int viewHeight)
{
  if (HasUI())
  {
    if (viewWidth != GetEditorWidth() || viewHeight != GetEditorHeight())
      mView->resize(viewWidth, viewHeight);

    IPlugAPIBase::EditorResizeFromDelegate(viewWidth, viewHeight);
  }
  
  return true;
}

void IPlugVST3::DirtyParametersFromUI()
{
  startGroupEdit();
  IPlugAPIBase::DirtyParametersFromUI();
  finishGroupEdit();
}

void IPlugVST3::SetLatency(int latency)
{
  IPlugProcessor::SetLatency(latency);

  FUnknownPtr<IComponentHandler>handler(componentHandler);
  handler->restartComponent(kLatencyChanged);
}
