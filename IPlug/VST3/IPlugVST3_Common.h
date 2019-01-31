
/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

#include "pluginterfaces/base/ibstream.h"

#include "IPlugAPIBase.h"

#include "IPlugVST3_Parameter.h"

using namespace Steinberg;
using namespace Vst;

class IPlugVST3ControllerBase
{
public:
  
  void Initialize(IPlugAPIBase* plug, ParameterContainer& parameters, bool plugIsInstrument)
  {
    if (plug->NPresets())
      parameters.addParameter(new IPlugVST3PresetParameter(plug->NPresets()));
    
    if (plugIsInstrument)
      parameters.addParameter(new IPlugVST3BypassParameter());
    
    for (int i = 0; i < plug->NParams(); i++)
    {
      IParam *p = plug->GetParam(i);
      
      UnitID unitID = kRootUnitId;
      
      const char* paramGroupName = p->GetGroupForHost();
      
      if (CStringHasContents(paramGroupName))
      {
        for (int j = 0; j < plug->NParamGroups(); j++)
        {
          if (strcmp(paramGroupName, plug->GetParamGroupName(j)) == 0)
          {
            unitID = j + 1;
          }
        }
        
        if (unitID == kRootUnitId) // new unit, nothing found, so add it
        {
          unitID = plug->AddParamGroup(paramGroupName);
        }
      }
      
      Parameter* pVST3Parameter = new IPlugVST3Parameter(p, i, unitID);
      parameters.addParameter(pVST3Parameter);
    }
    
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
  }
};

// State

struct IPlugVST3State
{
  template <class T>
  static bool GetState(T* plug, IBStream* state)
  {
    IByteChunk chunk;
    
    // TODO: IPlugVer should be in chunk!
    //  IByteChunk::GetIPlugVerFromChunk(chunk)
    
    if (plug->SerializeState(chunk))
    {
      /*
       int chunkSize = chunk.Size();
       void* data = (void*) &chunkSize;
       state->write(data, (int32) sizeof(int));*/
      state->write(chunk.GetData(), chunk.Size());
    }
    else
    {
      return false;
    }
    
    int32 toSaveBypass = plug->GetBypassed() ? 1 : 0;
    state->write(&toSaveBypass, sizeof (int32));
    
    return true;
  };
  
  template <class T>
  static bool SetState(T* plug, IBStream* state)
  {
    TRACE;
    
    IByteChunk chunk;
    
    const int bytesPerBlock = 128;
    char buffer[bytesPerBlock];
    
    while(true)
    {
      Steinberg::int32 bytesRead = 0;
      auto status = state->read(buffer, (Steinberg::int32) bytesPerBlock, &bytesRead);
      
      if (bytesRead <= 0 || (status != kResultTrue && plug->GetHost() != kHostWaveLab))
        break;
      
      chunk.PutBytes(buffer, bytesRead);
    }
    int pos = plug->UnserializeState(chunk,0);
    
    int32 savedBypass = 0;
    
    state->seek(pos,IBStream::IStreamSeekMode::kIBSeekSet);
    if (state->read (&savedBypass, sizeof (Steinberg::int32)) != kResultOk) {
      return kResultFalse;
    }
    
    Parameter* bypassParameter = plug->getParameterObject(kBypassParam);
    if (bypassParameter)
      bypassParameter->setNormalized(savedBypass);
    
    plug->OnRestoreState();
    return kResultOk;
  }
};

// Host

static void IPlugVST3GetHost(IPlugAPIBase* plug, FUnknown* context)
{
  String128 tmpStringBuf;
  char hostNameCString[128];
  FUnknownPtr<IHostApplication>app(context);
  
  if ((plug->GetHost() == kHostUninit) && app)
  {
    app->getName(tmpStringBuf);
    Steinberg::UString(tmpStringBuf, 128).toAscii(hostNameCString, 128);
    plug->SetHost(hostNameCString, 0); // Can't get version in VST3
  }
}
