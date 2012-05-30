#include "IPlugAAX.h"
#include "resource.h"

//AAX Components
#include "AAX_ICollection.h"
#include "AAX_IComponentDescriptor.h"
#include "AAX_IEffectDescriptor.h"
#include "AAX_IPropertyMap.h"
#include "AAX_Errors.h"
#include "AAX_Assert.h"

#ifndef BUNDLE_ID
  #define BUNDLE_ID "com." BUNDLE_MFR ".aax." BUNDLE_NAME
#endif

static AAX_EStemFormat getStemFormatForChans(const int numChans)
{
  switch (numChans)
  {
    case 0:
      return AAX_eStemFormat_None;
    case 1:
      return AAX_eStemFormat_Mono;
    case 2:
      return AAX_eStemFormat_Stereo;
    case 3:
      return AAX_eStemFormat_LCR;
    case 4:
      return AAX_eStemFormat_Quad;
    case 5:
      return AAX_eStemFormat_5_0;
    case 6:
      return AAX_eStemFormat_5_1;
    case 7:
      return AAX_eStemFormat_6_1;
    case 8:
      return AAX_eStemFormat_7_1_DTS;
    default:
      return AAX_eStemFormat_None;
      break;
  }
}

AAX_Result GetEffectDescriptions( AAX_ICollection * outCollection )
{
  AAX_Result        err = AAX_SUCCESS;
  AAX_IEffectDescriptor * effectDescriptor = outCollection->NewDescriptor();
  if ( effectDescriptor == NULL )
    return AAX_ERROR_NULL_OBJECT;

  // Effect identifiers
  err |= effectDescriptor->AddName(PLUG_NAME);
  //TODO: call a couple of times with PLUG_NAME_DIGI
  
  AAX_EPlugInCategory category;
  if (PLUG_IS_INST) category = AAX_ePlugInCategory_SWGenerators;
  else if(strcmp(EFFECT_TYPE_DIGI, "None") == 0) category = AAX_ePlugInCategory_None;
  else if(strcmp(EFFECT_TYPE_DIGI, "EQ") == 0) category = AAX_ePlugInCategory_EQ;
  else if(strcmp(EFFECT_TYPE_DIGI, "Dynamics") == 0) category = AAX_ePlugInCategory_Dynamics;
  else if(strcmp(EFFECT_TYPE_DIGI, "PitchShift") == 0) category = AAX_ePlugInCategory_PitchShift;
  else if(strcmp(EFFECT_TYPE_DIGI, "Reverb") == 0) category = AAX_ePlugInCategory_Reverb;
  else if(strcmp(EFFECT_TYPE_DIGI, "Delay") == 0) category = AAX_ePlugInCategory_Delay;
  else if(strcmp(EFFECT_TYPE_DIGI, "Modulation") == 0) category = AAX_ePlugInCategory_Modulation;
  else if(strcmp(EFFECT_TYPE_DIGI, "Harmonic") == 0) category = AAX_ePlugInCategory_Harmonic;
  else if(strcmp(EFFECT_TYPE_DIGI, "NoiseReduction") == 0) category = AAX_ePlugInCategory_NoiseReduction;
  else if(strcmp(EFFECT_TYPE_DIGI, "Dither") == 0) category = AAX_ePlugInCategory_Dither;
  else if(strcmp(EFFECT_TYPE_DIGI, "SoundField") == 0) category = AAX_ePlugInCategory_SoundField;
  else if(strcmp(EFFECT_TYPE_DIGI, "Effect") == 0) category = PDA_ePlugInCategory_Effect;
  err |= effectDescriptor->AddCategory(category);
  
  //err |= effectDescriptor->AddResourceInfo ( AAX_eResourceType_PageTable, PLUG_NAME ".xml" );
  
  char *channelIOStr = PLUG_CHANNEL_IO;
  
  int ioConfigIdx = 0;
  int nSIn = 0;//(PLUG_SC_CHANS > 0); // force it to 1
  
  while (channelIOStr) 
  {
    int nIn = 0, nOut = 0;
    
    if (sscanf(channelIOStr, "%d-%d", &nIn, &nOut) == 2)    
    {
      // if we have a 1-N config + sidechain we don't want to include the sidechain
      if (nIn > 1)
        nIn -= nSIn;
      
      AAX_CTypeID typeId = PLUG_UNIQUE_ID + 1 + ioConfigIdx;

      // Describe the algorithm and effect specifics using the CInstrumentParameters convenience layer.  (Native Only)
      AAX_SIPlugSetupInfo setupInfo;
      setupInfo.mInputStemFormat = getStemFormatForChans(nIn);
      setupInfo.mOutputStemFormat = getStemFormatForChans(nOut);
      setupInfo.mManufacturerID = PLUG_MFR_ID;
      setupInfo.mProductID = PLUG_UNIQUE_ID;
      setupInfo.mPluginID = typeId;
      #if PLUG_DOES_OFFLINE
      setupInfo.mAudioSuiteID = typeId+1;
      #endif
      setupInfo.mCanBypass = true;
      setupInfo.mNeedsInputMIDI = PLUG_DOES_MIDI;
      setupInfo.mInputMIDINodeName = "Midi Input";
      setupInfo.mInputMIDIChannelMask = 0x0001;
      setupInfo.mNeedsGlobalMIDI = PLUG_DOES_MIDI;
      setupInfo.mGlobalMIDIEventMask = 0x3;
      setupInfo.mNeedsTransport = true;
      setupInfo.mLatency = PLUG_LATENCY;
            
      err |= AAX_CIPlugParameters::StaticDescribe(effectDescriptor, setupInfo);
      
      AAX_ASSERT (err == AAX_SUCCESS);
      
      // Data model
      err |= effectDescriptor->AddProcPtr( (void *) IPlugAAX::Create, kAAX_ProcPtrID_Create_EffectParameters );
      
      // GUI
      err |= effectDescriptor->AddProcPtr( (void *) AAX_CEffectGUI_IPLUG::Create, kAAX_ProcPtrID_Create_EffectGUI );
      
      if ( err == AAX_SUCCESS )
        err = outCollection->AddEffect(BUNDLE_ID, effectDescriptor );
      
      AAX_ASSERT (err == AAX_SUCCESS);
          
      ioConfigIdx++;
    }
    
    channelIOStr = strstr(channelIOStr, " ");
    
    if (channelIOStr)
      ++channelIOStr;
  }
  
  outCollection->SetManufacturerName(PLUG_MFR);
  //TODO: call a couple of times with PLUG_MFR_DIGI

  outCollection->AddPackageName(PLUG_NAME);
  //TODO: call a couple of times with PLUG_NAME_DIGI

  outCollection->SetPackageVersion(PLUG_VER);   
  
  return err;
}
