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
    
    IByteChunk::InitChunkWithIPlugVer(chunk);
    Steinberg::int32 toSaveBypass = pPlug->GetBypassed() ? 1 : 0;
    chunk.Put(&toSaveBypass);
    
    if (pPlug->SerializeState(chunk))
    {
      int chunkSize = chunk.Size();
      pState->write(static_cast<void*>(&chunkSize), static_cast<int>(sizeof(int)));
      pState->write(chunk.GetData(), chunkSize);
      return true;
    }
    else
      return false;
  };
  
  template <class T>
  static bool SetState(T* pPlug, Steinberg::IBStream* pState)
  {
    TRACE
    
    IByteChunk chunk;
    int chunkSize = 0;
    pState->read(&chunkSize, sizeof(int));
    chunk.Resize(chunkSize);
    pState->read(chunk.GetData(), chunk.Size());

    Steinberg::int32 savedBypass = 0;
    
    int pos = 0;
    IByteChunk::GetIPlugVerFromChunk(chunk, pos);
    
    pState->seek(pos, Steinberg::IBStream::IStreamSeekMode::kIBSeekSet);
    
    if (pState->read(&savedBypass, sizeof (Steinberg::int32)) != Steinberg::kResultOk)
    {
      return false;
    }
    else
      pos += sizeof (Steinberg::int32);

    pos = pPlug->UnserializeState(chunk, pos);
        
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
