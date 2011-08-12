#if WINDOWS_VERSION
	#include <windows.h>
	#include "Mac2Win.H"
#endif

#include "IPlugProcessRTAS.h"
#include "Resource.h"

#include "FicPluginEnums.h"
#include "CEffectType.h"
#include "PlugInUtils.h"

IPlugProcessRTAS::IPlugProcessRTAS(OSType type)
: mPluginID(type)
, mBlockSize(GetMaximumRTASQuantum()) 
{
  mPlug = MakePlug();
  
  if (mPlug != NULL)
  {
    if (mPlug->PluginDoesStateChunks())
    {
      AddChunk(mPluginID, PLUG_MFR);
    }
    
    mPlug->SetNumInputs(GetNumInputs());
    mPlug->SetNumOutputs(GetNumOutputs());  
    
    mPlug->Created(this);
  }
}

void IPlugProcessRTAS::HandleMIDI()
{
  TRACE;
  
  if (mDirectMidiInterface)
  {
    for (int nodeIdx=0; nodeIdx < mDirectMidiInterface->directMidiParamBlkPtr->mNumMidiNodes; ++nodeIdx)
    {
      DirectMidiNode * node = mDirectMidiInterface->directMidiParamBlkPtr->mNodeArray[nodeIdx];
      
      for (int i=0; i < node->mBufferSize; ++i)
      {
        DirectMidiPacket* iMessage = node->mBuffer+i;
        IMidiMsg msg = IMidiMsg(iMessage->mTimestamp, iMessage->mData[0], iMessage->mData[1], iMessage->mData[2]);
        mPlug->ProcessMidiMsg(&msg);
      }
    }
  }
}

void IPlugProcessRTAS::RenderAudio(float** inputs, float** outputs, long frames)
{
  HandleMIDI();
  
  // Two possible cases for RTAS since ip==op for PT8 and below.
  long ips=GetNumInputs();
  long ops=GetNumOutputs();
  long sip=GetSideChainConnectionNum();
  
  if (mBypassed)
  {
    if (ips>=1 && ops>=1) 
      memcpy(outputs[0],inputs[0],frames*sizeof(float));
    if (ips>=2 && ops>=2) 
      memcpy(outputs[1],inputs[1],frames*sizeof(float));
  }
  else if (mPlug)
  {
    int nFrames = frames;
    
    //we are delivered the sidechain buffer (if connected) as inputs[sip][0] where sip is always the last channel in the array
    //we need to extract it as we want to process it seperately through iPlug
    
    int numSideChainChannels = 1; // pro tools only supports mono side-chain ips
    
    float** side = new float*[numSideChainChannels];
    side[0] = new float[numSideChainChannels*nFrames];
    
    //memset(&side[0][0], 0, nFrames*sizeof(float));
    //we could memset here, but iPlug should use its scratchbuf (silence) when its not connected
    
    if (GetSideChainConnectionNum() != EffectLayerDef::NO_SIDECHAIN_CONNECTED)
    {
      memcpy(side[0],inputs[sip], nFrames*sizeof(float));
    }
    
    Trace(TRACELOC,"Number of inputs: %d Sidechain Connection Number: %d", ips, sip);
    if (GetSideChainConnectionNum() != EffectLayerDef::NO_SIDECHAIN_CONNECTED) {
      mPlug->SetNumInputs(ips+1);
      mPlug->SetNumSideChainInputs(1);
    } else {
      mPlug->SetNumInputs(ips);
      mPlug->SetNumSideChainInputs(0);
    }
    mPlug->SetNumOutputs(ops);
    
    mPlug->ProcessAudio(inputs, outputs, side, nFrames);
  }
}

void IPlugProcessRTAS::GetMetersFromDSPorRTAS(long *allMeters, bool *clipIndicators)
{
  for (int i = 0; i < GetNumOutputs(); i++) 
  {
    clipIndicators[i] = false;
    allMeters[i] = 0;
  }
}

double IPlugProcessRTAS::GetTempo()
{
  if (mDirectMidiInterface)
  {
    Cmn_Float64 t;
    mDirectMidiInterface->GetCurrentTempo(&t);
    return (double) t;
  }
  return 0.0;
}

ComponentResult IPlugProcessRTAS::IsControlAutomatable(long aControlIndex, short *aItIsP)
{
  TRACE;
  if (aControlIndex == 1) //master bypass
  {
    *aItIsP = 1; 
    return noErr;
  }
  *aItIsP = mPlug->GetParam(aControlIndex-2)->GetCanAutomate() ? 1 : 0;
  return noErr;
}

ComponentResult IPlugProcessRTAS::GetChunkSize(OSType chunkID, long *size)
{
  TRACE;
//  if (chunkID == EffectLayerDef::CONTROLS_CHUNK_ID) return noErr;

  if (chunkID == mPluginID) 
  {
    *size = sizeof(SFicPlugInChunkHeader);
    ByteChunk IPlugChunk;
    if (mPlug->SerializeState(&IPlugChunk))
    {
      *size = IPlugChunk.Size() + sizeof(SFicPlugInChunkHeader);
    }
    return noErr;
  }
  else 
    return CEffectProcess::GetChunkSize(chunkID, size); // Not our chunk
}

ComponentResult IPlugProcessRTAS::SetChunk(OSType chunkID, SFicPlugInChunk *chunk)
{
    TRACE;
//  if (chunkID == EffectLayerDef::CONTROLS_CHUNK_ID) return noErr;
  
  //called when project is loaded from save
  if (chunkID == mPluginID) 
  {
    const int dataSize = chunk->fSize - sizeof(SFicPlugInChunkHeader);
    
    ByteChunk IPlugChunk;
    IPlugChunk.PutBytes(chunk->fData, dataSize);  
    mPlug->UnserializeState(&IPlugChunk, 0);
    return noErr;
  }
  else 
    return CEffectProcess::SetChunk(chunkID, chunk); // Not our chunk
}

ComponentResult IPlugProcessRTAS::GetChunk(OSType chunkID, SFicPlugInChunk *chunk)
{
    TRACE;
//  if (chunkID == EffectLayerDef::CONTROLS_CHUNK_ID) return noErr;
  
  //called when project is saved
  if (chunkID == mPluginID)
  {
    ByteChunk IPlugChunk;
    if (mPlug->SerializeState(&IPlugChunk))
    {
      chunk->fSize = IPlugChunk.Size() + sizeof(SFicPlugInChunkHeader);
      memcpy(chunk->fData, IPlugChunk.GetBytes(), IPlugChunk.Size());
      return noErr;
    }
    return 1;
  }
  else 
    return CEffectProcess::GetChunk(chunkID, chunk); // Not our chunk
}






