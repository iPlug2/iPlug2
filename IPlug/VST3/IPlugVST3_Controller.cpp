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

using namespace Steinberg;
using namespace Steinberg::Vst;

IPlugVST3Controller::IPlugVST3Controller(IPlugInstanceInfo instanceInfo, IPlugConfig c)
: IPlugAPIBase(c, kAPIVST3)
, mPlugIsInstrument(c.plugType == kInstrument)
, mProcessorGUID(instanceInfo.mOtherGUID)
{
}

IPlugVST3Controller::~IPlugVST3Controller()
{
}

#pragma mark -
#pragma mark IEditController overrides

tresult PLUGIN_API IPlugVST3Controller::initialize(FUnknown* context)
{
  if (EditControllerEx1::initialize(context) == kResultTrue)
  {
    Initialize(this, parameters, mPlugIsInstrument);
    /*
    UnitInfo uinfo;
    uinfo.id = kRootUnitId;
    uinfo.parentUnitId = kNoParentUnitId;

    if (NPresets() > 1)
      uinfo.programListId = kPresetParam;
    else
      uinfo.programListId = kNoProgramListId;

    UString name(uinfo.name, 128);
    name.fromAscii("Root");
    addUnit(new Unit(uinfo));

    int32 flags = 0;
    UnitID unitID = kRootUnitId;

    for (int i = 0; i < NParams(); i++)
    {
      IParam* pParam = GetParam(i);

      pParam->SetToDefault();

      flags = 0;
      unitID = kRootUnitId;

      const char* paramGroupName = pParam->GetGroupForHost();

      if (CStringHasContents(paramGroupName))
      {
        for(int j = 0; j < mParamGroups.GetSize(); j++)
        {
          if(strcmp(paramGroupName, mParamGroups.Get(j)) == 0)
          {
            unitID = j+1;
          }
        }

        if (unitID == kRootUnitId) // new unit, nothing found, so add it
        {
          mParamGroups.Add(paramGroupName);
          unitID = mParamGroups.GetSize();

          // Add the unit
          uinfo.id = unitID;
          uinfo.parentUnitId = kRootUnitId;
          uinfo.programListId = kNoProgramListId;
          name.fromAscii(paramGroupName);
          addUnit(new Unit(uinfo));
        }
      }

      if (pParam->GetCanAutomate())
        flags |= ParameterInfo::kCanAutomate;

//      if (pParam->IsReadOnly())
//        flags |= ParameterInfo::kIsReadOnly;

      Parameter* pVSTParam = new IPlugVST3Parameter(pParam, flags, unitID);
      pVSTParam->setNormalized(pParam->GetDefault(true));
      parameters.addParameter(pVSTParam);
    }

    if (!IsInstrument())
      parameters.addParameter(new IPlugVST3BypassParameter());
      
//
//    if (NPresets() > 1)
//     parameters.addParameter(new IPlugVST3PresetParameter(NPresets()));

//      parameters.addParameter(STR16("Preset"), STR16(""), NPresets(), 0, ParameterInfo::kIsProgramChange|ParameterInfo::kIsList, kPresetParam, kRootUnitId);
//
//    if (DoesMIDIIn())
//    {
//      mParamGroups.Add("MIDI Controllers");
//      uinfo.id = unitID = mParamGroups.GetSize();
//      uinfo.parentUnitId = kRootUnitId;
//      uinfo.programListId = kNoProgramListId;
//      name.fromAscii("MIDI Controllers");
//      addUnit(new Unit(uinfo));
//
//      ParamID midiParamIdx = kMIDICCParamStartIdx;
//      UnitID midiControllersID = unitID;
//
//      char buf[32];

//      for (int chan = 0; chan < NUM_CC_CHANS_TO_ADD; chan++)
//      {
//        sprintf(buf, "Ch %i", chan+1);
//
//        mParamGroups.Add(buf);
//        uinfo.id = unitID = mParamGroups.GetSize();
//        uinfo.parentUnitId = midiControllersID;
//        uinfo.programListId = kNoProgramListId;
//        name.fromAscii(buf);
//        addUnit(new Unit(uinfo));
//
//        for (int i = 0; i < 128; i++)
//        {
//          name.fromAscii(ControlStr(i));
//          parameters.addParameter(name, STR16(""), 0, 0, 0, midiParamIdx++, unitID);
//        }
//
//        parameters.addParameter(STR16("Channel Aftertouch"), STR16(""), 0, 0, 0, midiParamIdx++, unitID);
//        parameters.addParameter(STR16("Pitch Bend"), STR16(""), 0, 0.5, 0, midiParamIdx++, unitID);
//      }
//    }

    if (NPresets())
    {
      ProgramListWithPitchNames* list = new ProgramListWithPitchNames(String("Factory Presets"), kPresetParam, kRootUnitId);

      for (int i = 0; i< NPresets(); i++)
      {
        list->addProgram(String(GetPresetName(i)));
      }

//      char noteName[128];

      // TODO: GetMidiNote ? !

//      for (int i = 0; i< 128; i++)
//      {
//        if (MidiNoteName(i, noteName))
//        {
//          name.fromAscii(noteName);
//          list->setPitchName(0, i, name); // TODO: this will only set it for the first preset!
//        }
//      }

      addProgramList(list);
    }
     */

    IPlugVST3GetHost(this, context);
    OnHostIdentified();

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

tresult PLUGIN_API IPlugVST3Controller::setComponentState(IBStream* state)
{
  return IPlugVST3State::SetState(this, state) ? kResultOk :kResultFalse;
}

tresult PLUGIN_API IPlugVST3Controller::setState(IBStream* state)
{
  // Currently nothing to do here
  return kResultOk;
}

tresult PLUGIN_API IPlugVST3Controller::getState(IBStream* state)
{
// Currently nothing to do here
  return kResultOk;
}

tresult PLUGIN_API IPlugVST3Controller::setParamNormalized(ParamID tag, ParamValue value)
{
  if (tag >= kBypassParam)
  {
    switch (tag)
    {
      case kBypassParam:
      {
//        bool bypassed = (value > 0.5);
//
//        if (bypassed != IsBypassed())
//          mIsBypassed = bypassed;
//
        break;
      }
      case kPresetParam:
      {
        RestorePreset(NPresets() * value);

        break;
      }
      default:
        break;
    }
  }
  else
  {
    IParam* pParam = GetParam(tag);

    if (pParam)
    {
      pParam->SetNormalized(value);
      OnParamChangeUI(tag, kHost);
    }
  }

  return EditControllerEx1::setParamNormalized(tag, value);
}

tresult PLUGIN_API IPlugVST3Controller::getMidiControllerAssignment(int32 busIndex, int16 midiChannel, CtrlNumber midiControllerNumber, ParamID& tag)
{
//  if (busIndex == 0)
//  {
//    tag = kMIDICCParamStartIdx + (midiChannel * kCountCtrlNumber) + midiControllerNumber;
//    return kResultTrue;
//  }

  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3Controller::queryInterface(const char* iid, void** obj)
{
  QUERY_INTERFACE(iid, obj, IMidiMapping::iid, IMidiMapping)
  return EditControllerEx1::queryInterface(iid, obj);
}

tresult PLUGIN_API IPlugVST3Controller::getProgramName(ProgramListID listId, int32 programIndex, String128 name /*out*/)
{
  if (NPresets() && listId == kPresetParam)
  {
    Steinberg::UString(name, 128).fromAscii(GetPresetName(programIndex));
    return kResultTrue;
  }

  return kResultFalse;
}

//void IPlugVST3Controller::InformHostOfProgramChange()
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

void IPlugVST3Controller::EditorPropertiesChangedFromDelegate(int viewWidth, int viewHeight, const IByteChunk& data)
{
  if (HasUI())
  {
    if (viewWidth != GetEditorWidth() || viewHeight != GetEditorHeight())
      mView->resize(viewWidth, viewHeight);
 
    IPlugAPIBase::EditorPropertiesChangedFromDelegate(viewWidth, viewHeight, data);
  }
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
    Steinberg::int64 controlTag = kNoTag;
    double normalizedValue = 0.;
    
    if(message->getAttributes()->getInt("CT", controlTag) == kResultFalse)
      return kResultFalse;
    
    if(message->getAttributes()->getFloat("NV", normalizedValue) == kResultFalse)
      return kResultFalse;
    
    SendControlValueFromDelegate((int) controlTag, normalizedValue);

  }
  else if (!strcmp(message->getMessageID(), "SCMFD"))
  {
    const void* data;
    Steinberg::int64 controlTag = kNoTag;
    Steinberg::int64 messageTag = kNoTag;

    if(message->getAttributes()->getInt("CT", controlTag) == kResultFalse)
      return kResultFalse;
    
    if(message->getAttributes()->getInt("MT", messageTag) == kResultFalse)
      return kResultFalse;

    Steinberg::uint32 size;
    
    if (message->getAttributes()->getBinary("D", data, size) == kResultOk)
    {
      SendControlMsgFromDelegate((int) controlTag, (int) messageTag, size, data);
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

void IPlugVST3Controller::SendArbitraryMsgFromUI(int messageTag, int controlTag, int dataSize, const void* pData)
{
  OPtr<IMessage> message = allocateMessage();
  
  if (!message)
    return;
  
  if(dataSize == 0) // allow sending messages with no data
  {
    dataSize = 1;
    uint8_t dummy = 0;
    pData = &dummy;
  }
  
  message->setMessageID("SAMFUI");
  message->getAttributes()->setInt("MT", messageTag);
  message->getAttributes()->setInt("CT", controlTag);
  message->getAttributes()->setBinary("D", pData, dataSize);
  sendMessage(message);
}

