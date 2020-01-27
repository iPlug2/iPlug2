/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "pluginterfaces/base/ibstream.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

#include "IPlugAPIBase.h"
#include "IPlugVST3_Parameter.h"

BEGIN_IPLUG_NAMESPACE

/** Shared VST3 controller code */
class IPlugVST3ControllerBase
{
public:
    
  IPlugVST3ControllerBase() = default;
  IPlugVST3ControllerBase(const IPlugVST3ControllerBase&) = delete;
  IPlugVST3ControllerBase& operator=(const IPlugVST3ControllerBase&) = delete;
    
  void Initialize(IPlugAPIBase* pPlug, Steinberg::Vst::ParameterContainer& parameters, bool plugIsInstrument/*, bool midiIn*/)
  {
    Steinberg::Vst::UnitInfo uinfo;
    uinfo.id = Steinberg::Vst::kRootUnitId;
    uinfo.parentUnitId = Steinberg::Vst::kNoParentUnitId;
    Steinberg::UString unitNameSetter(uinfo.name, 128);
    unitNameSetter.fromAscii("Root");
    
    Steinberg::Vst::UnitID unitID = Steinberg::Vst::kRootUnitId;
    
    if (pPlug->NPresets())
    {
      uinfo.programListId = kPresetParam;
      parameters.addParameter(new IPlugVST3PresetParameter(pPlug->NPresets()));
    }
    else
      uinfo.programListId = Steinberg::Vst::kNoProgramListId;
    
    if (!plugIsInstrument)
      parameters.addParameter(mBypassParameter = new IPlugVST3BypassParameter());

    Steinberg::Vst::EditControllerEx1* pEditController = dynamic_cast<Steinberg::Vst::EditControllerEx1*>(pPlug);
    
    pEditController->addUnit(new Steinberg::Vst::Unit(uinfo));

    for (int i = 0; i < pPlug->NParams(); i++)
    {
      IParam* pParam = pPlug->GetParam(i);
      unitID = Steinberg::Vst::kRootUnitId; // reset unitID
    
      const char* paramGroupName = pParam->GetGroupForHost();
      
      if (CStringHasContents(paramGroupName)) // if the parameter has a group
      {
        for (int j = 0; j < pPlug->NParamGroups(); j++) // loop through previously added groups
        {
          if (strcmp(paramGroupName, pPlug->GetParamGroupName(j)) == 0) // if group name found in existing groups
          {
            unitID = j + 1; // increment unitID
          }
        }
        
        if (unitID == Steinberg::Vst::kRootUnitId) // if unitID was still kRootUnitId, we found a new group, so add it and add the unit
        {
          unitID = pPlug->AddParamGroup(paramGroupName); // updates unitID
          uinfo.id = unitID;
          uinfo.parentUnitId = Steinberg::Vst::kRootUnitId;
          uinfo.programListId = Steinberg::Vst::kNoProgramListId;
          unitNameSetter.fromAscii(paramGroupName);
          pEditController->addUnit (new Steinberg::Vst::Unit (uinfo));
        }
      }
      
      Steinberg::Vst::Parameter* pVST3Parameter = new IPlugVST3Parameter(pParam, i, unitID);
      parameters.addParameter(pVST3Parameter);
    }

//    if (midiIn)
//    {
//      mParamGroups.Add("MIDI Controllers");
//      uinfo.id = unitID = mParamGroups.GetSize();
//      uinfo.parentUnitId = kRootUnitId;
//      uinfo.programListId = kNoProgramListId;
//      name.fromAscii("MIDI Controllers");
//      addUnit(new Unit(uinfo));
//
//      Steinberg::Vst::ParamID midiSteinberg::Vst::ParamIDx = kMIDICCParamStartIdx;
//      UnitID midiControllersID = unitID;
//
//      char buf[32];
//
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
//          parameters.addParameter(name, STR16(""), 0, 0, 0, midiSteinberg::Vst::ParamIDx++, unitID);
//        }
//
//        parameters.addParameter(STR16("Channel Aftertouch"), STR16(""), 0, 0, 0, midiSteinberg::Vst::ParamIDx++, unitID);
//        parameters.addParameter(STR16("Pitch Bend"), STR16(""), 0, 0.5, 0, midiSteinberg::Vst::ParamIDx++, unitID);
//      }
//    }
     /*
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
  }
  
  Steinberg::Vst::ParamValue PLUGIN_API GetParamNormalized(IPlugAPIBase* pPlug, Steinberg::Vst::ParamID tag)
  {
    IParam* param = pPlug->GetParam(tag);
        
    if (param)
    {
      return param->GetNormalized();
    }
        
    return 0.0;
  }
    
  void PLUGIN_API SetParamNormalized(IPlugAPIBase* pPlug, Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value)
  {
    if (tag >= kBypassParam)
    {
      switch (tag)
      {
        case kPresetParam:
        {
          pPlug->RestorePreset(std::round((pPlug->NPresets() - 1) * value));
          break;
        }
        default:
          break;
      }
    }
    else
    {
      IParam* pParam = pPlug->GetParam(tag);
      
      if (pParam)
      {
        pParam->SetNormalized(value);
        pPlug->OnParamChangeUI(tag, kHost);
        pPlug->SendParameterValueFromAPI(tag, value, true);
      }
    }
  }
  
public:
  IPlugVST3BypassParameter* mBypassParameter = nullptr;
};

END_IPLUG_NAMESPACE
