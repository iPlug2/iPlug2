/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "pluginterfaces/base/ibstream.h"
#
#include "IPlugAPIBase.h"
#include "IPlugVST3_Parameter.h"
#include "IPlugVST3_ControllerBase.h"

BEGIN_IPLUG_NAMESPACE

/** Shared VST3 State management code */
struct IPlugVST3State
{
  /** Called when saving the plugin state to a host project or .vstpreset file.
   * Writes chunk, bypass state, and preset name of currently selected preset to IBStream *state. */
  template <class T>
  static bool GetState(T* pPlug, Steinberg::IBStream* pState)
  {
    IByteChunk chunk;
    
    // TODO: IPlugVer should be in chunk!
    //  IByteChunk::GetIPlugVerFromChunk(chunk)
    Steinberg::int32 chunksize;
    
    if (pPlug->SerializeState(chunk))
    {
      chunksize = chunk.Size();
      
      pState->write(chunk.GetData(), chunksize);
    }
    else
      return false;
    
    Steinberg::int32 toSaveBypass = pPlug->GetBypassed() ? 1 : 0;
    pState->write(&toSaveBypass, sizeof (Steinberg::int32));
    
    IPreset* pPreset = pPlug->GetPreset(pPlug->GetCurrentPresetIdx());
    
    Steinberg::Vst::String128 toSavePresetName;
    WDL_String mPresetName;
    mPresetName.Set(pPreset->mName);
    Steinberg::UString(toSavePresetName, sizeof(Steinberg::Vst::String128)).fromAscii(mPresetName.Get());
    pState->write(&toSavePresetName, sizeof(Steinberg::Vst::String128));
    
    pState->write(pPreset->mChunk.GetData(), chunksize);
    
    return true;
  }
  
  /** Called when restoring the plugin state from a saved host project or .vstpreset file.
   * Loads chunk, bypass state, and preset name of saved preset from IBStream *state and restores at index 0. */
  template <class T>
  static bool SetState(T* pPlug, Steinberg::IBStream* pState)
  {
    TRACE
    
    IByteChunk chunk;
    
    pPlug->SerializeState(chunk); // to get the size
    Steinberg::int32 chunksize = chunk.Size();
    
    if (chunksize > 0)
    {
      pState->read(chunk.GetData(), chunksize);
      pPlug->UnserializeState(chunk, 0);
      
      Steinberg::int32 savedBypass = 0;
      
      if (pState->read(&savedBypass, sizeof(Steinberg::int32)) != Steinberg::kResultOk)
      {
        return false;
      }
      
      IPlugVST3ControllerBase* pController = dynamic_cast<IPlugVST3ControllerBase*>(pPlug);
      
      if(pController)
      {
        if (pController->mBypassParameter)
          pController->mBypassParameter->setNormalized(savedBypass);
      }
      
      IPreset* pPreset = pPlug->GetPreset(0);
      
      Steinberg::Vst::String128 savedPresetName;
      if (pState->read(&savedPresetName, sizeof(Steinberg::Vst::String128)) != Steinberg::kResultOk)
      {
        return false;
      }
      
      WDL_String mPresetName;
      char PresetName[128];
      Steinberg::UString(savedPresetName, sizeof(Steinberg::Vst::String128)).toAscii(PresetName, sizeof(Steinberg::Vst::String128));
      mPresetName.Set(PresetName);
      strcpy(pPreset->mName, mPresetName.Get());
      
      pState->read(pPreset->mChunk.GetData(), chunksize);
      pPlug->ModifyPreset(0, pPreset->mName);
      pPlug->RestorePreset(0);
      
      pPlug->OnRestoreState();
      return true;
    }
    
    return false;
  }
};

// Host
static void IPlugVST3GetHost(IPlugAPIBase* pPlug, Steinberg::FUnknown* context)
{
  Steinberg::Vst::String128 tmpStringBuf;
  char hostNameCString[128];
  Steinberg::FUnknownPtr<Steinberg::Vst::IHostApplication>pApp(context);
  
  if ((pPlug->GetHost() == kHostUninit) && pApp)
  {
    pApp->getName(tmpStringBuf);
    Steinberg::UString(tmpStringBuf, 128).toAscii(hostNameCString, 128);
    pPlug->SetHost(hostNameCString, 0); // Can't get version in VST3
  }
}

END_IPLUG_NAMESPACE
