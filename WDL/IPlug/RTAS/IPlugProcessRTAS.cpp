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

  long nInputs = GetNumInputs();
  long nOutputs = GetNumOutputs();
  //DBGMSG("Number of inputs: %d outputs: %d\n", nInputs, nOutputs);

  mPlug->SetIO(nInputs, nOutputs);

  if (mBypassed)
  {
    mPlug->ProcessAudioBypassed(inputs, outputs, frames);
  }
  else if (mPlug)
  {
    #if PLUG_DOES_MIDI
    HandleMIDI();
    #endif

    mPlug->ProcessAudio(inputs, outputs, frames);
  }
}

//void IPlugProcessRTAS::GetMetersFromDSPorRTAS(long *allMeters, bool *clipIndicators)
//{
//  for (int i = 0; i < GetNumOutputs(); i++)
//  {
//    clipIndicators[i] = false;
//    allMeters[i] = 0;
//  }
//}

int IPlugProcessRTAS::GetSamplePos()
{
  if (mDirectMidiInterface)
  {
    Cmn_Int64 samplePos;
    mDirectMidiInterface->GetCurrentRTASSampleLocation(&samplePos);
    return (int) samplePos;
  }
  else
    return 0;
}

double IPlugProcessRTAS::GetTempo()
{
  if (mDirectMidiInterface)
  {
    Cmn_Float64 t;
    mDirectMidiInterface->GetCurrentTempo(&t);
    return (double) t;
  }
  else
    return 0.;
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
  if (mDirectMidiInterface)
  {
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
      mDirectMidiInterface->GetCurrentRTASSampleLocation(&samplePos);
    else
      mDirectMidiInterface->GetCurrentTDMSampleLocation(&samplePos);

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


