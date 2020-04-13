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
#include "IPlugVST3_Defs.h"

#include "IPlugMidi.h"

BEGIN_IPLUG_NAMESPACE

/** Shared VST3 controller code */
class IPlugVST3ControllerBase
{
public:
    
  IPlugVST3ControllerBase() = default;
  IPlugVST3ControllerBase(const IPlugVST3ControllerBase&) = delete;
  IPlugVST3ControllerBase& operator=(const IPlugVST3ControllerBase&) = delete;
    
  void Initialize(IPlugAPIBase* pPlug, Steinberg::Vst::ParameterContainer& parameters, bool plugIsInstrument, bool midiIn)
  {
    Steinberg::Vst::EditControllerEx1* pEditController = dynamic_cast<Steinberg::Vst::EditControllerEx1*>(pPlug);

    Steinberg::Vst::UnitInfo unitInfo;
    unitInfo.id = Steinberg::Vst::kRootUnitId;
    unitInfo.parentUnitId = Steinberg::Vst::kNoParentUnitId;
    Steinberg::UString unitNameSetter(unitInfo.name, 128);
    unitNameSetter.fromAscii("Root");
    
    Steinberg::Vst::UnitID unitID = Steinberg::Vst::kRootUnitId;
    
    #ifdef VST3_PRESET_LIST
    if (pPlug->NPresets())
    {
      unitInfo.programListId = kPresetParam;
      parameters.addParameter(new IPlugVST3PresetParameter(pPlug->NPresets()));
      
      Steinberg::Vst::ProgramListWithPitchNames* pList = new Steinberg::Vst::ProgramListWithPitchNames(STR16("Factory Presets"), 0 /* list id */, Steinberg::Vst::kRootUnitId);
      
      Steinberg::Vst::String128 programName;
      Steinberg::Vst::String128 pitchName;

      for (int programIdx=0; programIdx<pPlug->NPresets(); programIdx++)
      {
        Steinberg::UString(programName, str16BufferSize(Steinberg::Vst::String128)).assign(pPlug->GetPresetName(programIdx));
        pList->addProgram (programName);
        
        //Set named notes. This could be different per-preset in VST3
        for (int pitch = 0; pitch < 128; pitch++)
        {
          char pNoteText[32] = "";
          if(pPlug->GetMidiNoteText(pitch, pNoteText))
          {
            Steinberg::UString(pitchName, str16BufferSize(Steinberg::Vst::String128)).assign(pNoteText);
            pList->setPitchName(programIdx, pitch, pitchName);
          }
        }
      }

      pEditController->addProgramList(pList);
    }
    else
    #endif
      unitInfo.programListId = Steinberg::Vst::kNoProgramListId;
    
    if (!plugIsInstrument)
      parameters.addParameter(mBypassParameter = new IPlugVST3BypassParameter());
    
    pEditController->addUnit(new Steinberg::Vst::Unit(unitInfo));

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
          unitInfo.id = unitID;
          unitInfo.parentUnitId = Steinberg::Vst::kRootUnitId;
          unitInfo.programListId = Steinberg::Vst::kNoProgramListId;
          unitNameSetter.fromAscii(paramGroupName);
          pEditController->addUnit (new Steinberg::Vst::Unit (unitInfo));
        }
      }
      
      Steinberg::Vst::Parameter* pVST3Parameter = new IPlugVST3Parameter(pParam, i, unitID);
      parameters.addParameter(pVST3Parameter);
    }

    assert(VST3_NUM_CC_CHANS <= VST3_NUM_MIDI_IN_CHANS && "VST3_NUM_CC_CHANS must be less than or equal to VST3_NUM_MIDI_IN_CHANS");
    
#if VST3_NUM_CC_CHANS > 0
    if (midiIn)
    {
      unitInfo.id = unitID = pEditController->getUnitCount() + 1;
      unitInfo.parentUnitId = Steinberg::Vst::kRootUnitId;
      unitInfo.programListId = Steinberg::Vst::kNoProgramListId;
      unitNameSetter.fromAscii(VST3_CC_UNITNAME);
      pEditController->addUnit(new Steinberg::Vst::Unit(unitInfo));

      Steinberg::Vst::ParamID paramIdx = kMIDICCParamStartIdx;

      WDL_String chanGroupStr;
      Steinberg::Vst::UnitID midiCCsUnitID = unitID;

      for (int chan = 0; chan < VST3_NUM_CC_CHANS; chan++)
      {
        chanGroupStr.SetFormatted(32, "CH%i", chan + 1);

        unitInfo.id = unitID = pEditController->getUnitCount() + 1;
        unitInfo.parentUnitId = midiCCsUnitID;
        unitInfo.programListId = Steinberg::Vst::kNoProgramListId;
        unitNameSetter.fromAscii(chanGroupStr.Get());
        pEditController->addUnit(new Steinberg::Vst::Unit(unitInfo));
        // add 128 MIDI CCs
        Steinberg::Vst::String128 paramName;
        for (int i = 0; i < 128; i++)
        {
          Steinberg::UString(paramName, str16BufferSize(Steinberg::Vst::String128)).assign(IMidiMsg::CCNameStr(i));
          parameters.addParameter(paramName, STR16(""), 0, 0, 0, paramIdx++, unitID);
        }

        parameters.addParameter(STR16("Channel Aftertouch"), STR16(""), 0, 0, 0, paramIdx++, unitID);
        parameters.addParameter(STR16("Pitch Bend"), STR16(""), 0, 0.5, 0, paramIdx++, unitID);
      }
    }
#endif
  }
  
//  https://github.com/iPlug2/iPlug2/issues/488
//  Steinberg::Vst::ParamValue PLUGIN_API GetParamNormalized(IPlugAPIBase* pPlug, Steinberg::Vst::ParamID tag)
//  {
//    IParam* pParam = pPlug->GetParam(tag);
//
//    if (pParam)
//    {
//      return pParam->GetNormalized();
//    }
//
//    return 0.0;
//  }
    
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

// in VST3, parameter changes are managed by the host
#if !defined VST3C_API // && !defined VST3_API //TODO
        pPlug->SendParameterValueFromAPI(tag, value, true);
#else
        pPlug->SendParameterValueFromDelegate(tag, value, true);
#endif
      }
    }
  }
  
public:
  IPlugVST3BypassParameter* mBypassParameter = nullptr;
};

END_IPLUG_NAMESPACE
