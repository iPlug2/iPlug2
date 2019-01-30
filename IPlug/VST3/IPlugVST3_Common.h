
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
