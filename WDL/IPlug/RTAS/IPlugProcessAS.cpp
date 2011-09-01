#if WINDOWS_VERSION
	#include <windows.h>
	#include "Mac2Win.H"
#endif

#include "IPlugProcessAS.h"
//#include "FicPluginEnums.h"
//#include "CEffectType.h"
//#include "PlugInUtils.h"
//#include "CPluginControl_Continuous.h"

IPlugProcessAS::IPlugProcessAS(OSType type)
: IPlugProcess(type)
{//  for(int i = 0; i< EffectLayerDef::MAX_NUM_CONNECTIONS; i++)
//  {
//    mMeterVal[i] = 0.0;
//    mMeterMin[i] = 0.0;
//  }
  mPlug->Created(this);
}

IPlugProcessAS::~IPlugProcessAS(void)
{
}

UInt32 IPlugProcessAS::ProcessAudio(bool isMasterBypassed)
{
//  using std::fabs;
//
//  long  totalInputSamples = 0;    // total number of input samples in one input buffer.
//  long  CurrentInputSamples = 0;  // current input sample being processed.
//
//  long  bufferIncrement = 1;    // amount to increment input buffer (will = 0 if no valid input buffer exists)
//  long  updateMeterInterval = 2048; // for AS update meters every 2K samples.
//  long  updateMeterCounter = 0;   // counter for when to update meter.
//
//  SFloat32 defaultInputVal = 0.0;
//  double  a,b;
//  
//  for(int i = 0;i< GetNumOutputs(); i++) // One pass through loop for each output channel.
//  {
//    DAEConnectionPtr outputConnection = GetOutputConnection(i);
//    if(outputConnection) // If no valid connection don't do anything
//    {
//      SFloat32 *inputBuf;
//      SFloat32 gain = SFloat32(mGain[i]);
//      SFloat32 *outputBuf = (SFloat32 *)outputConnection->mBuffer;
//      
//      DAEConnectionPtr inputConnection = GetInputConnection(i);
//      
//      if (!inputConnection) // no input connection use default value of zero.
//      {
//        bufferIncrement = 0;      // don't increment!
//        inputBuf = &defaultInputVal;
//        // use the mono channel input sample number. (Guarenteed to be connected)
//        if(GetInputConnection(0))
//          totalInputSamples = GetInputConnection(0)->mNumSamplesInBuf;
//      }
//      else // have a valid input connection
//      {
//        inputBuf = (SFloat32 *)inputConnection->mBuffer;
//        totalInputSamples = inputConnection->mNumSamplesInBuf;
//      }
//      
//      CurrentInputSamples = totalInputSamples;
//      
//      // Just in case the audio buffer is smaller than the count to update the meter for AS 
//      if (updateMeterInterval > totalInputSamples)
//        updateMeterInterval = totalInputSamples/2;
//      
//      if (isMasterBypassed) {
//        while (CurrentInputSamples--) {
//          *outputBuf = *inputBuf;
//          outputBuf++;
//          inputBuf += bufferIncrement;
//        }
//      } else {      
//        // Go through the input buffer and perform our gain.
//        while (CurrentInputSamples--)
//        {
//        //  *outputBuf = multipleAmount * (*inputBuf);
//          a = *inputBuf;
//          b = gain * a;
//          *outputBuf = float(b);// mGain * (*inputBuf);
//        //  *outputBuf = SFloat32(inSample);
//          
//          if ( fabs(*outputBuf) > mMeterVal[i] ) mMeterVal[i] = fabs(*outputBuf);
//
//          // For RTAS (& TDM), meter updating is done in DoTokenIdle.
//          // For AS, meter updating is done here:
//          if (IsAS()) {
//            if(++updateMeterCounter >= updateMeterInterval) {
//              // update the meterview every updateMeterAt samples
//              this->UpdateMeters();
//              updateMeterCounter = 0;
//            }
//          }
//          
//          outputBuf++;
//          inputBuf += bufferIncrement;
//        }
//      }
//      // Do the sample number adjustment.
//      outputConnection->mNumSamplesInBuf = totalInputSamples;
//      if (inputConnection) inputConnection->mNumSamplesInBuf = 0;
//    } // end if(outputConnection)
//
//  } // end for loop
//  
//  // Get the current number of samples analyzed and pass this info
//  // back to the DAE application so it knows how much we've processed.
//  // This is a global position keeper, it should be incremented by the 
//  // number of samples processed on a single track.  Not the total processed 
//  // by all tracks.
//  return totalInputSamples;
  
  return 0;
}

//void IPlugProcessAS::UpdateControlValueInAlgorithm (long controlIndex)
//{
//  double gain;
//
//  if (controlIndex != kControl_MasterBypass) {
//      gain = dynamic_cast<CPluginControl_Continuous*>(GetControl(controlIndex))->GetContinuous();
//      mGain[controlIndex - kControl_GainMono] = gain;
//  }
//  
//}

void IPlugProcessAS::GetMetersFromDSPorRTAS(long *allMeters, bool *clipIndicators)
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
