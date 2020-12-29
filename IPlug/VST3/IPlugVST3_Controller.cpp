/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugVST3_Controller.h"

#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
//#include "public.sdk/source/vst/vstpresetfile.cpp"

#include "IPlugVST3_Parameter.h"

using namespace iplug;
using namespace Steinberg;
using namespace Vst;

IPlugVST3Controller::IPlugVST3Controller(const InstanceInfo& info, const Config& config)
: IPlugAPIBase(config, kAPIVST3)
, mPlugIsInstrument(config.plugType == kInstrument)
, mDoesMidiIn(config.plugDoesMidiIn)
, mProcessorGUID(info.mOtherGUID)
, IPlugVST3ControllerBase(parameters)
{
  CreateTimer();
}

IPlugVST3Controller::~IPlugVST3Controller()
{
}

#pragma mark IEditController overrides

tresult PLUGIN_API IPlugVST3Controller::initialize(FUnknown* context)
{
  if (EditControllerEx1::initialize(context) == kResultTrue)
  {
    Initialize(this, mPlugIsInstrument, mDoesMidiIn);
    IPlugVST3GetHost(this, context);
    OnHostIdentified();
    OnParamReset(kReset);
    
    // Load iplug parameters into the GUI thread visible values
    
    for (int i = 0; i < NParams(); ++i)
      parameters.getParameter(i)->setNormalized(GetParam(i)->GetNormalized());
    
    return kResultTrue;
  }

  return kResultFalse;
}

IPlugView* PLUGIN_API IPlugVST3Controller::createView(const char* name)
{
  if (name && strcmp(name, "editor") == 0)
  {
    mView = new ViewType(*this);
    return mView;
  }
  
  return nullptr;
}

tresult PLUGIN_API IPlugVST3Controller::setComponentState(IBStream* pState)
{
  return IPlugVST3State::SetState(this, pState) ? kResultOk : kResultFalse;
}

tresult PLUGIN_API IPlugVST3Controller::setState(IBStream* pState)
{
  // Currently nothing to do here
  return kResultOk;
}

tresult PLUGIN_API IPlugVST3Controller::getState(IBStream* pState)
{
// Currently nothing to do here
  return kResultOk;
}

ParamValue PLUGIN_API IPlugVST3Controller::getParamNormalized(ParamID tag)
{
  return IPlugVST3ControllerBase::GetParamNormalized(tag);
}

tresult PLUGIN_API IPlugVST3Controller::setParamNormalized(ParamID tag, ParamValue value)
{
  if (IPlugVST3ControllerBase::SetParamNormalized(this, tag, value))
    return kResultTrue;
  else
    return kResultFalse;
}

tresult PLUGIN_API IPlugVST3Controller::getMidiControllerAssignment(int32 busIndex, int16 midiChannel, CtrlNumber midiCCNumber, ParamID& tag)
{
  if (busIndex == 0 && midiChannel < VST3_NUM_CC_CHANS)
  {
    tag = kMIDICCParamStartIdx + (midiChannel * kCountCtrlNumber) + midiCCNumber;
    return kResultTrue;
  }

  return kResultFalse;
}

#pragma mark IUnitInfo overrides

tresult PLUGIN_API IPlugVST3Controller::getProgramName(ProgramListID listId, int32 programIndex, String128 name /*out*/)
{
  if (NPresets() && listId == kPresetParam)
  {
    Steinberg::UString(name, 128).fromAscii(GetPresetName(programIndex));
    return kResultTrue;
  }

  return kResultFalse;
}

//void IPlugVST3Controller::InformHostOfPresetChange()
//{
//  if (NPresets())
//  {
//    //    beginEdit(kPresetParam);
//    //    performEdit(kPresetParam, ToNormalizedParam(mCurrentPresetIdx, 0., NPresets(), 1.));
//    //    endEdit(kPresetParam);
//
//    notifyProgramListChange(kPresetParam, mCurrentPresetIdx);
//  }
//}

#pragma mark IInfoListener overrides

Steinberg::tresult PLUGIN_API IPlugVST3Controller::setChannelContextInfos(Steinberg::Vst::IAttributeList* pList)
{
  return IPlugVST3ControllerBase::SetChannelContextInfos(pList) ? kResultTrue : kResultFalse;
}

#pragma mark -

bool IPlugVST3Controller::EditorResize(int viewWidth, int viewHeight)
{
  if (HasUI())
  {
    if (viewWidth != GetEditorWidth() || viewHeight != GetEditorHeight())
      mView->Resize(viewWidth, viewHeight);
 
    SetEditorSize(viewWidth, viewHeight);
  }
  
  return true;
}

void IPlugVST3Controller::DirtyParametersFromUI()
{
  startGroupEdit();
  IPlugAPIBase::DirtyParametersFromUI();
  finishGroupEdit();
}

#pragma mark Message with Processor

tresult PLUGIN_API IPlugVST3Controller::notify(IMessage* message)
{
  if (!message)
    return kInvalidArgument;
  
  if (!strcmp(message->getMessageID(), "SCVFD"))
  {
    Steinberg::int64 ctrlTag = kNoTag;
    double normalizedValue = 0.;
    
    if(message->getAttributes()->getInt("CT", ctrlTag) == kResultFalse)
      return kResultFalse;
    
    if(message->getAttributes()->getFloat("NV", normalizedValue) == kResultFalse)
      return kResultFalse;
    
    SendControlValueFromDelegate((int) ctrlTag, normalizedValue);

  }
  else if (!strcmp(message->getMessageID(), "SCMFD"))
  {
    const void* data;
    Steinberg::int64 ctrlTag = kNoTag;
    Steinberg::int64 msgTag = kNoTag;

    if(message->getAttributes()->getInt("CT", ctrlTag) == kResultFalse)
      return kResultFalse;
    
    if(message->getAttributes()->getInt("MT", msgTag) == kResultFalse)
      return kResultFalse;

    Steinberg::uint32 size;
    
    if (message->getAttributes()->getBinary("D", data, size) == kResultOk)
    {
      SendControlMsgFromDelegate((int) ctrlTag, (int) msgTag, size, data);
      return kResultOk;
    }
  }
  else if (!strcmp(message->getMessageID(), "SMMFD"))
  {
    const void* data = nullptr;
    uint32 size;
    
    if (message->getAttributes()->getBinary("D", data, size) == kResultOk)
    {
      if (size == sizeof(IMidiMsg))
      {
        IMidiMsg msg;
        memcpy(&msg, data, size);
        SendMidiMsgFromDelegate(msg);
      }
    }
  }
  else if (!strcmp(message->getMessageID(), "SSMFD"))
  {
    const void* data = nullptr;
    uint32 size;
    int64 offset;
    
    if (message->getAttributes()->getBinary("D", data, size) == kResultOk)
    {
      if (message->getAttributes()->getInt("O", offset) == kResultOk)
      {
        ISysEx msg {static_cast<int>(offset), static_cast<const uint8_t*>(data), static_cast<int>(size)};
        SendSysexMsgFromDelegate(msg);
      }
    }
  }
  
  return ComponentBase::notify(message);
}

void IPlugVST3Controller::SendMidiMsgFromUI(const IMidiMsg& msg)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  message->setMessageID("SMMFUI");
  message->getAttributes()->setBinary("D", (void*) &msg, sizeof(IMidiMsg));
  sendMessage(message);
}

void IPlugVST3Controller::SendSysexMsgFromUI(const ISysEx& msg)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  message->setMessageID("SSMFUI");
  message->getAttributes()->setInt("O", (int64) msg.mOffset);
  message->getAttributes()->setBinary("D", msg.mData, msg.mSize);
  sendMessage(message);
}

void IPlugVST3Controller::SendArbitraryMsgFromUI(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  if (dataSize == 0) // allow sending messages with no data
  {
    dataSize = 1;
    uint8_t dummy = 0;
    pData = &dummy;
  }
  
  message->setMessageID("SAMFUI");
  message->getAttributes()->setInt("MT", msgTag);
  message->getAttributes()->setInt("CT", ctrlTag);
  message->getAttributes()->setBinary("D", pData, dataSize);
  sendMessage(message);
}

void IPlugVST3Controller::SendParameterValueFromUI(int paramIdx, double normalisedValue)
{
  IPlugVST3ControllerBase::SetVST3ParamNormalized(paramIdx, normalisedValue);
  IPlugAPIBase::SendParameterValueFromUI(paramIdx, normalisedValue);
}
