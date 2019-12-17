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
  template <class T>
  static bool GetState(T* pPlug, Steinberg::IBStream* pState)
  {
    IByteChunk chunk;
    
    // TODO: IPlugVer should be in chunk!
    //  IByteChunk::GetIPlugVerFromChunk(chunk)
    
    if (pPlug->SerializeState(chunk))
    {
      /*
       int chunkSize = chunk.Size();
       void* data = (void*) &chunkSize;
       state->write(data, (Steinberg::int32) sizeof(int));*/
      pState->write(chunk.GetData(), chunk.Size());
    }
    else
      return false;
    
    Steinberg::int32 toSaveBypass = pPlug->GetBypassed() ? 1 : 0;
    pState->write(&toSaveBypass, sizeof (Steinberg::int32));
    
    return true;
  };
  
  template <class T>
  static bool SetState(T* pPlug, Steinberg::IBStream* pState)
  {
    TRACE
    
    IByteChunk chunk;
    
    const int bytesPerBlock = 128;
    char buffer[bytesPerBlock];
    
    while(true)
    {
      Steinberg::int32 bytesRead = 0;
      auto status = pState->read(buffer, (Steinberg::int32) bytesPerBlock, &bytesRead);
      
      if (bytesRead <= 0 || (status != Steinberg::kResultTrue && pPlug->GetHost() != kHostWaveLab))
        break;
      
      chunk.PutBytes(buffer, bytesRead);
    }
    int pos = pPlug->UnserializeState(chunk,0);
    
    Steinberg::int32 savedBypass = 0;
    
    pState->seek(pos,Steinberg::IBStream::IStreamSeekMode::kIBSeekSet);
    if (pState->read (&savedBypass, sizeof (Steinberg::int32)) != Steinberg::kResultOk) {
      return false;
    }
    
    IPlugVST3ControllerBase* pController = dynamic_cast<IPlugVST3ControllerBase*>(pPlug);
    
    if(pController)
    {
      if (pController->mBypassParameter)
        pController->mBypassParameter->setNormalized(savedBypass);
    }
    
    pPlug->OnRestoreState();
    
    return true;
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
