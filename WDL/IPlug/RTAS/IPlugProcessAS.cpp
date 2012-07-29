#if WINDOWS_VERSION
  #include <windows.h>
  #include "Mac2Win.H"
#endif

#include "IPlugProcessAS.h"

IPlugProcessAS::IPlugProcessAS(OSType type)
  : IPlugProcess(type)
{
  mPlug->Created(this);
  
  inputAudioStreams = (float**) malloc(GetNumOutputs() * sizeof(float*)); // getNumOutputs correct
  outputAudioStreams = (float**) malloc(GetNumOutputs() * sizeof(float*));
  
  for (int ch=0; ch < GetNumOutputs(); ch++)
  {
    if (inputAudioStreams != NULL)
      inputAudioStreams[ch] = NULL;
    if (outputAudioStreams != NULL)
      outputAudioStreams[ch] = NULL;
  }
}

IPlugProcessAS::~IPlugProcessAS(void)
{
	if (inputAudioStreams != NULL)
    free(inputAudioStreams);

	inputAudioStreams = NULL;
	
  if (outputAudioStreams != NULL)
    free(outputAudioStreams);
	
  outputAudioStreams = NULL;
}

UInt32 IPlugProcessAS::ProcessAudio(bool isMasterBypassed)
{
  long  totalInputSamples = 0;    // total number of input samples in one input buffer.

  if (GetInputConnection(0) != NULL)
    totalInputSamples = GetInputConnection(0)->mNumSamplesInBuf;
  
  for(int ch = 0; ch < GetNumOutputs(); ch++) // for each output channel.
  {
    inputAudioStreams[ch] = NULL;
		outputAudioStreams[ch] = NULL;
    
    DAEConnectionPtr outputConnection = GetOutputConnection(ch);
    
    if (outputConnection != NULL)	// if no valid connection, don't do anything
		{
      outputAudioStreams[ch] = (float*)(outputConnection->mBuffer);

      DAEConnectionPtr inputConnection = GetInputConnection(ch);
      
			if (inputConnection != NULL)	// have a valid input connection
        inputAudioStreams[ch] = (float*)(inputConnection->mBuffer);
     
      // do the sample number adjustment
      outputConnection->mNumSamplesInBuf = totalInputSamples;
      if (inputConnection != NULL)
        inputConnection->mNumSamplesInBuf = 0;
    }
  }
  
  mPlug->SetIO(GetNumInputs(), GetNumOutputs());
    
  if (isMasterBypassed)
  {
    mPlug->ProcessAudioBypassed(inputAudioStreams, outputAudioStreams, totalInputSamples);
  }
  else
  {
    mPlug->ProcessAudio(inputAudioStreams, outputAudioStreams, totalInputSamples);
  }
  
  return totalInputSamples;
}

//void IPlugProcessAS::GetMetersFromDSPorRTAS(long *allMeters, bool *clipIndicators)
//{
//  for (int i = 0; i < GetNumOutputs(); i++)
//  {
//    clipIndicators[i] = false;
//    allMeters[i] = 0;
//  }
//}

//void IPlugProcessAS::SetViewOrigin (Point anOrigin)
//{
//  // First, call inherited.  This will offset the rect stored in
//  // the CProcess member fOurPlugInView by the requested amount.
//  IPlugProcess::SetViewOrigin(anOrigin); // Call inherited
//
//  // Make sure that our rect matches up with the new plug-in rect.
//  Rect ourPlugInViewRect;
//  fOurPlugInView->GetRect(& ourPlugInViewRect);
//
//  mLeftOffset = ourPlugInViewRect.left;
//  mTopOffset = ourPlugInViewRect.top;
//
//}
