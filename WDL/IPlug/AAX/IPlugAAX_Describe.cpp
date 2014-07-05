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

  WDL_String subStr;

  char *plugNameStr = PLUG_NAME_PT;
    
  while (plugNameStr)
  {
    int span = strcspn(plugNameStr, "\n");
    
    if (span)
    {
      subStr.Set(plugNameStr, span);
      err |= effectDescriptor->AddName(subStr.Get());
      outCollection->AddPackageName(subStr.Get());
      plugNameStr = strstr(plugNameStr, "\n");
      
      if (plugNameStr)
        ++plugNameStr;
    }
    else
    {
      break;
    }
  }
  
  AAX_EPlugInCategory category;
  if (PLUG_IS_INST) category = AAX_ePlugInCategory_SWGenerators;
  else if(strcmp(PLUG_TYPE_PT, "None") == 0) category = AAX_ePlugInCategory_None;
  else if(strcmp(PLUG_TYPE_PT, "EQ") == 0) category = AAX_ePlugInCategory_EQ;
  else if(strcmp(PLUG_TYPE_PT, "Dynamics") == 0) category = AAX_ePlugInCategory_Dynamics;
  else if(strcmp(PLUG_TYPE_PT, "PitchShift") == 0) category = AAX_ePlugInCategory_PitchShift;
  else if(strcmp(PLUG_TYPE_PT, "Reverb") == 0) category = AAX_ePlugInCategory_Reverb;
  else if(strcmp(PLUG_TYPE_PT, "Delay") == 0) category = AAX_ePlugInCategory_Delay;
  else if(strcmp(PLUG_TYPE_PT, "Modulation") == 0) category = AAX_ePlugInCategory_Modulation;
  else if(strcmp(PLUG_TYPE_PT, "Harmonic") == 0) category = AAX_ePlugInCategory_Harmonic;
  else if(strcmp(PLUG_TYPE_PT, "NoiseReduction") == 0) category = AAX_ePlugInCategory_NoiseReduction;
  else if(strcmp(PLUG_TYPE_PT, "Dither") == 0) category = AAX_ePlugInCategory_Dither;
  else if(strcmp(PLUG_TYPE_PT, "SoundField") == 0) category = AAX_ePlugInCategory_SoundField;
  else if(strcmp(PLUG_TYPE_PT, "Effect") == 0) category = AAX_ePlugInCategory_None;
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
      
      AAX_CTypeID typeId = PLUG_TYPE_IDS[ioConfigIdx];

      // Describe the algorithm and effect specifics using the CInstrumentParameters convenience layer.  (Native Only)
      AAX_SIPlugSetupInfo setupInfo;
      setupInfo.mInputStemFormat = getStemFormatForChans(nIn);
      setupInfo.mOutputStemFormat = getStemFormatForChans(nOut);
      setupInfo.mManufacturerID = PLUG_MFR_ID;
      setupInfo.mProductID = PLUG_UNIQUE_ID;
      setupInfo.mPluginID = typeId;
      #if PLUG_DOES_AUDIOSUITE
      setupInfo.mAudioSuiteID = PLUG_TYPE_IDS_AS[ioConfigIdx];
      #endif
      setupInfo.mCanBypass = true;
      setupInfo.mNeedsInputMIDI = PLUG_DOES_MIDI;
      setupInfo.mInputMIDINodeName = PLUG_NAME" Midi";
      setupInfo.mInputMIDIChannelMask = 0x0001;
//      setupInfo.mNeedsGlobalMIDI = PLUG_DOES_MIDI;
//      setupInfo.mGlobalMIDIEventMask = 0x3;
      setupInfo.mNeedsTransport = true;
      setupInfo.mLatency = PLUG_LATENCY;
            
      err |= AAX_CIPlugParameters::StaticDescribe(effectDescriptor, setupInfo);
      
      AAX_ASSERT (err == AAX_SUCCESS);
          
      ioConfigIdx++;
    }
    
    channelIOStr = strstr(channelIOStr, " ");
    
    if (channelIOStr)
      ++channelIOStr;
  }
  
  // Data model
  err |= effectDescriptor->AddProcPtr( (void *) IPlugAAX::Create, kAAX_ProcPtrID_Create_EffectParameters );
  
  // GUI
  err |= effectDescriptor->AddProcPtr( (void *) AAX_CEffectGUI_IPLUG::Create, kAAX_ProcPtrID_Create_EffectGUI );
  
  if ( err == AAX_SUCCESS )
    err = outCollection->AddEffect(BUNDLE_ID, effectDescriptor );
  
  AAX_ASSERT (err == AAX_SUCCESS);
  
  char *mfrNameStr = PLUG_MFR_PT;
  
  while (mfrNameStr)
  {
    int span = strcspn(mfrNameStr, "\n");
    
    if (span)
    {
      subStr.Set(mfrNameStr, span);
      outCollection->SetManufacturerName(subStr.Get());
      mfrNameStr = strstr(mfrNameStr, "\n");
      
      if (mfrNameStr)
        ++mfrNameStr;
    }
    else
    {
      break;
    }
  }
  
  outCollection->SetPackageVersion(PLUG_VER);   
  
  return err;
}
