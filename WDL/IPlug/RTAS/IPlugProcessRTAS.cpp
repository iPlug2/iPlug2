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
:IPlugProcess(type)
, mBlockSize(GetMaximumRTASQuantum()) 
{
  mPlug->Created(this);
}

void IPlugProcessRTAS::HandleMIDI()
{
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
  TRACE_PROCESS;
  
#if PLUG_DOES_MIDI
  HandleMIDI();
#endif

  // Two possible cases for RTAS since ip==op for PT8 and below.
  // TODO: do we need to call these here?
  long ips=GetNumInputs();
  long ops=GetNumOutputs();
  
  if (mBypassed)
  {
    // TODO: implement delay of bypassed signal equal to mPlug->GetLatency();
    if (ips>=1 && ops>=1) 
      memcpy(outputs[0],inputs[0],frames*sizeof(float));
    if (ips>=2 && ops>=2) 
      memcpy(outputs[1],inputs[1],frames*sizeof(float));
  }
  else if (mPlug)
  {    
    //DBGMSG("Number of inputs: %d outputs: %d, sip: %d\n", ips, ops, sip);
    
    mPlug->SetNumInputs(ips);
    mPlug->SetNumOutputs(ops);
    
    mPlug->ProcessAudio(inputs, outputs, frames);
  }
  
// TODO: Meters?  
  
//  for(int i = 0; i < GetNumOutputs(); i++)
//	{
//		float* outSamples = outputs[i];
//		
//		for (int j = 0; j<frames; j++)
//		{      
//			if ( fabsf(outSamples[j]) > mMeterVal[i] )
//				mMeterVal[i] = fabsf(outSamples[j]);
//		}
//	}
}

void IPlugProcessRTAS::GetMetersFromDSPorRTAS(long *allMeters, bool *clipIndicators)
{
// TODO: Meters?
  
//	SFloat32 currentVal = 0.0;
//  
//	for (int i = 0; i < GetNumOutputs(); i++) 
//	{
//		currentVal = mMeterVal[i];
//		
//		if(currentVal < mMeterMin[i])
//			currentVal = mMeterMin[i];
//		mMeterMin[i] = currentVal * 0.7;
//		
//		if (fabsf(currentVal) > 1.0)
//		{ 
//			currentVal = -1.0;
//			clipIndicators[i] = true;
//			fClipped = true;	
//		} 
//		else {
//			currentVal *= k32BitPosMax;
//			clipIndicators[i] = false;
//		}
//		
//		allMeters[i] = currentVal;
//		mMeterVal[i] = 0;
//  }
  
  for (int i = 0; i < GetNumOutputs(); i++) 
	{
		clipIndicators[i] = false;
		allMeters[i] = 0;
	}
}

int IPlugProcessRTAS::GetSamplePos()
{
  if (mDirectMidiInterface) {
    Cmn_Int64 samplePos;
    mDirectMidiInterface->GetCurrentRTASSampleLocation(&samplePos);
    return (int) samplePos;
  }
  else
    return 0;
}

double IPlugProcessRTAS::GetTempo()
{
  if (mDirectMidiInterface) {
    Cmn_Float64 t;
    mDirectMidiInterface->GetCurrentTempo(&t);
    return (double) t;
  }
  else
    return 0.0;
}

void IPlugProcessRTAS::GetTimeSig(int* pNum, int* pDenom)
{
  if (mDirectMidiInterface)
    mDirectMidiInterface->GetCurrentMeter((Cmn_Int32*) pNum,(Cmn_Int32*) pDenom);
}

void IPlugProcessRTAS::GetTime( double *pSamplePos, 
                                double *pTempo, 
                                double *pMusicalPos, 
                                double *pLastBar,
                                int* pNum, 
                                int* pDenom,
                                double *pCycleStart,
                                double *pCycleEnd,
                                bool *pTransportRunning,
                                bool *pTransportCycle)
{
  if (mDirectMidiInterface) {
    Cmn_Int64 samplePos;
    Cmn_Bool transportRunning;
    Cmn_Int64 ticks = 0;

    mDirectMidiInterface->GetCurrentRTASSampleLocation(&samplePos);
    *pSamplePos = (double) samplePos;
    
    mDirectMidiInterface->GetCurrentTempo((Cmn_Float64*) pTempo);
    mDirectMidiInterface->GetCurrentMeter((Cmn_Int32*) pNum,(Cmn_Int32*) pDenom);
    mDirectMidiInterface->IsTransportPlaying(&transportRunning);
    *pTransportRunning = (bool) transportRunning;
    
    if (transportRunning)
      mDirectMidiInterface->GetCurrentRTASSampleLocation (&samplePos);
    else
      mDirectMidiInterface->GetCurrentTDMSampleLocation (&samplePos);
    
    mDirectMidiInterface->GetCustomTickPosition (&ticks, samplePos);
    *pMusicalPos = ticks / 960000.0;
    
    // TODO: how to get these?
    *pLastBar = 0.;
    *pTransportCycle = false;
    *pCycleStart = 0.;
    *pCycleEnd = 0.;
  }
}

ComponentResult IPlugProcessRTAS::IsControlAutomatable(long aControlIndex, short *aItIsP)
{
  TRACE;
  
  if (aControlIndex == 1) // master bypass
    *aItIsP = 1; 
  else
    *aItIsP = mPlug->GetParam(aControlIndex-kPTParamIdxOffset)->GetCanAutomate() ? 1 : 0;
  
  return noErr;
}

// this is dynamic in PT9 > 
ComponentResult IPlugProcessRTAS::GetDelaySamplesLong(long* aNumSamples)
{
  *aNumSamples = (long) mPlug->GetLatency();
  return noErr;
}


