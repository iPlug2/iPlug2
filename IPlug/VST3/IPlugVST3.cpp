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
#include "pluginterfaces/vst/ivstmidicontrollers.h"

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
, mChannelIndex(0)
, mChannelColor(0)
, mChannelNamespaceIndex(0)
{
  CreateTimer();
}

IPlugVST3::~IPlugVST3() {}

#pragma mark AudioEffect overrides

tresult PLUGIN_API IPlugVST3::initialize(FUnknown* context)
{
  TRACE

  if (SingleComponentEffect::initialize(context) == kResultOk)
  {
    IPlugVST3ProcessorBase::Initialize(this);
    IPlugVST3ControllerBase::Initialize(this, parameters, IsInstrument(), DoesMIDIIn());

    IPlugVST3GetHost(this, context);
    OnHostIdentified();
    OnParamReset(kReset);
    
    return kResultOk;
  }

  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::terminate()
{
  TRACE

  return SingleComponentEffect::terminate();
}

tresult PLUGIN_API IPlugVST3::setBusArrangements(SpeakerArrangement* pInputBusArrangements, int32 numInBuses, SpeakerArrangement* pOutputBusArrangements, int32 numOutBuses)
{
  TRACE

  SetBusArrangements(pInputBusArrangements, numInBuses, pOutputBusArrangements, numOutBuses);
  return kResultTrue;
}

tresult PLUGIN_API IPlugVST3::setActive(TBool state)
{
  TRACE

  OnActivate((bool) state);
  return SingleComponentEffect::setActive(state);
}

tresult PLUGIN_API IPlugVST3::setupProcessing(ProcessSetup& newSetup)
{
  TRACE

  return SetupProcessing(newSetup, processSetup) ? kResultOk : kResultFalse;
}

tresult PLUGIN_API IPlugVST3::setProcessing(TBool state)
{
  Trace(TRACELOC, " state: %i", state);

  return SetProcessing((bool) state) ? kResultOk : kResultFalse;
}

tresult PLUGIN_API IPlugVST3::process(ProcessData& data)
{
  TRACE

  Process(data, processSetup, audioInputs, audioOutputs, mMidiMsgsFromEditor, mMidiMsgsFromProcessor, mSysExDataFromEditor, mSysexBuf);
  return kResultOk;
}

tresult PLUGIN_API IPlugVST3::canProcessSampleSize(int32 symbolicSampleSize)
{
  return CanProcessSampleSize(symbolicSampleSize) ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API IPlugVST3::setState(IBStream* pState)
{
  TRACE
  
  return IPlugVST3State::SetState(this, pState) ? kResultOk :kResultFalse;
}

tresult PLUGIN_API IPlugVST3::getState(IBStream* pState)
{
  TRACE
  
  return IPlugVST3State::GetState(this, pState) ? kResultOk :kResultFalse;
}

#pragma mark IEditController overrides
ParamValue PLUGIN_API IPlugVST3::getParamNormalized(ParamID tag)
{
  if (tag >= kBypassParam)
    return EditControllerEx1::getParamNormalized(tag);
  
  return IPlugVST3ControllerBase::GetParamNormalized(this, tag);
}

tresult PLUGIN_API IPlugVST3::setParamNormalized(ParamID tag, ParamValue value)
{
  IPlugVST3ControllerBase::SetParamNormalized(this, tag, value);
  
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

#pragma mark IMidiMapping overrides

tresult PLUGIN_API IPlugVST3::getMidiControllerAssignment(int32 busIndex, int16 midiChannel, CtrlNumber midiCCNumber, ParamID& tag)
{
  if (busIndex == 0)
  {
    tag = kMIDICCParamStartIdx + (midiChannel * kCountCtrlNumber) + midiCCNumber;
    return kResultTrue;
  }

  return kResultFalse;
}

#pragma mark IInfoListener overrides

Steinberg::tresult PLUGIN_API IPlugVST3::setChannelContextInfos(Steinberg::Vst::IAttributeList* pList)
{
  if (pList)
  {
    // optional we can ask for the Channel Name Length
    int64 length;
    if (pList->getInt(ChannelContext::kChannelNameLengthKey, length) == kResultTrue)
    {
      // get the Channel Name where we, as Plug-in, are instantiated
      std::vector<TChar> name(length+1);
      if (pList->getString(ChannelContext::kChannelNameKey, name.data(), length+1) == kResultTrue)
      {
        Steinberg::String str(name.data());
        str.toMultiByte(kCP_Utf8);
        mChannelName.Set(str);
      }
    }
    
    // get the channel uid Namespace Length
    if (pList->getInt(ChannelContext::kChannelUIDLengthKey, length) == kResultTrue)
    {
      // get the Channel UID
      std::vector<TChar> name(length+1);
      if (pList->getString(ChannelContext::kChannelUIDKey, name.data(), length+1) == kResultTrue)
      {
        Steinberg::String str(name.data());
        str.toMultiByte(kCP_Utf8);
        mChannelUID.Set(str);
      }
    }

    
    // get Channel Index
    int64 index;
    if (pList->getInt(ChannelContext::kChannelIndexKey, index) == kResultTrue)
    {
      mChannelIndex = index;
    }
    
    // get the Channel Color
    int64 color;
    if (pList->getInt(ChannelContext::kChannelColorKey, color) == kResultTrue)
    {
      mChannelColor = (uint32)color;
    }

    // get Channel Index Namespace Order of the current used index namespace
    if (pList->getInt(ChannelContext::kChannelIndexNamespaceOrderKey, index) == kResultTrue)
    {
      mChannelNamespaceIndex = index;
    }
  
    // get the channel Index Namespace Length
    if (pList->getInt(ChannelContext::kChannelIndexNamespaceLengthKey, length) == kResultTrue)
    {
      // get the channel Index Namespace
      std::vector<TChar> name(length+1);
      if (pList->getString(ChannelContext::kChannelIndexNamespaceKey, name.data(), length+1) == kResultTrue)
      {
        Steinberg::String str(name.data());
        str.toMultiByte(kCP_Utf8);
        mChannelNamespace.Set(str);
      }
    }
    

    // get Plug-in Channel Location
    int64 location;
    if (pList->getInt(ChannelContext::kChannelPluginLocationKey, location) == kResultTrue)
    {
      String128 string128;
      switch (location)
      {
        case ChannelContext::kPreVolumeFader:
          Steinberg::UString(string128, 128).fromAscii ("PreVolFader");
        break;
        case ChannelContext::kPostVolumeFader:
          Steinberg::UString(string128, 128).fromAscii ("PostVolFader");
        break;
        case ChannelContext::kUsedAsPanner:
          Steinberg::UString(string128, 128).fromAscii ("UsedAsPanner");
        break;
        default: Steinberg::UString(string128, 128).fromAscii ("unknown!");
        break;
      }
    }
    
    // do not forget to call addRef () if you want to keep this list
  }
  
  return kResultTrue;
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
