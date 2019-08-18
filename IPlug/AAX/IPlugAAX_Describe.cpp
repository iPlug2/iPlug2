/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugAAX.h"
#include "config.h"

//AAX Components
#include "AAX_ICollection.h"
#include "AAX_IComponentDescriptor.h"
#include "AAX_IEffectDescriptor.h"
#include "AAX_IPropertyMap.h"
#include "AAX_Errors.h"
#include "AAX_Assert.h"

#ifndef BUNDLE_ID
#define BUNDLE_ID BUNDLE_DOMAIN "." BUNDLE_MFR ".aax." BUNDLE_NAME
#endif

#define args(...) __VA_ARGS__
#define AAX_TYPE_ID_ARRAY(VARNAME, ARR_DATA) AAX_CTypeID VARNAME[] = {args ARR_DATA}

using namespace iplug;

#ifndef CUSTOM_BUSTYPE_FUNC

/**
 A method to get a sensible API tag for a particular number of channels allocated to a bus in the channel i/o string

 @param configIdx The index of the configuration in the channel i/o string (not used here)
 @param dir Whether this is being called for an input or output bus
 @param element The index of that bus in the list of dir buses
 @param pConfig The config struct derived from the channel i/o string token, this already contains data but \todo
 @return an integer corresponding to one of the AAX_eStemFormat
 */
static uint64_t GetAPIBusTypeForChannelIOConfig(int configIdx, ERoute dir, int busIdx, IOConfig* pConfig)
{
  assert(pConfig != nullptr);
  assert(busIdx >= 0 && busIdx < pConfig->NBuses(dir));
  
  int numChans = pConfig->GetBusInfo(dir, busIdx)->mNChans;

  switch (numChans)
  {
    case 0: return AAX_eStemFormat_None;
    case 1: return AAX_eStemFormat_Mono;
    case 2: return AAX_eStemFormat_Stereo;
    case 3: return AAX_eStemFormat_LCR;
    case 4: return AAX_eStemFormat_Ambi_1_ACN;
    case 5: return AAX_eStemFormat_5_0;
    case 6: return AAX_eStemFormat_5_1;
    case 7: return AAX_eStemFormat_7_0_DTS;
    case 8: return AAX_eStemFormat_7_1_DTS;
    case 9: return AAX_eStemFormat_Ambi_2_ACN;
    case 10:return AAX_eStemFormat_7_1_2;
    case 16:return AAX_eStemFormat_Ambi_3_ACN;
    default:
      DBGMSG("No stem format found for this channel count, you need to implement GetAPIBusTypeForChannelIOConfig() and #define CUSTOM_BUSTYPE_FUNC\n");
      assert(0);
      return AAX_eStemFormat_None;
  }
}
#else
extern uint64_t GetAPIBusTypeForChannelIOConfig(int configIdx, iplug::ERoute dir, int busIdx, iplug::IOConfig* pConfig);
#endif //CUSTOM_BUSTYPE_FUNC

AAX_Result GetEffectDescriptions(AAX_ICollection* pC)
{
  AAX_Result err = AAX_SUCCESS;
  AAX_IEffectDescriptor* pDesc = pC->NewDescriptor();
 
  if (pDesc == NULL)
    return AAX_ERROR_NULL_OBJECT;

  WDL_String subStr;

  char *plugNameStr = AAX_PLUG_NAME_STR;
    
  while (plugNameStr)
  {
    auto span = strcspn(plugNameStr, "\n");
    
    if (span)
    {
      subStr.Set(plugNameStr, (int) span);
      err |= pDesc->AddName(subStr.Get());
      pC->AddPackageName(subStr.Get());
      plugNameStr = strstr(plugNameStr, "\n");
      
      if (plugNameStr)
        ++plugNameStr;
    }
    else
    {
      break;
    }
  }
  
  AAX_EPlugInCategory category = AAX_ePlugInCategory_None;
  if (PLUG_TYPE == 1) category = AAX_ePlugInCategory_SWGenerators;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "None") == (0)) category = AAX_ePlugInCategory_None;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "EQ") == (0)) category = AAX_ePlugInCategory_EQ;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "Dynamics") == (0)) category = AAX_ePlugInCategory_Dynamics;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "PitchShift") == (0)) category = AAX_ePlugInCategory_PitchShift;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "Reverb") == (0)) category = AAX_ePlugInCategory_Reverb;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "Delay") == (0)) category = AAX_ePlugInCategory_Delay;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "Modulation") == (0)) category = AAX_ePlugInCategory_Modulation;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "Harmonic") == (0)) category = AAX_ePlugInCategory_Harmonic;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "NoiseReduction") == (0)) category = AAX_ePlugInCategory_NoiseReduction;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "Dither") == (0)) category = AAX_ePlugInCategory_Dither;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "SoundField") == (0)) category = AAX_ePlugInCategory_SoundField;
  else if(strcmp(AAX_PLUG_CATEGORY_STR, "Effect") == (0)) category = AAX_ePlugInCategory_None;
  err |= pDesc->AddCategory(category);
  
  //err |= effectDescriptor->AddResourceInfo(AAX_eResourceType_PageTable, PLUG_NAME ".xml");
  
  AAX_TYPE_ID_ARRAY(aaxTypeIDs,(AAX_TYPE_IDS));
#ifdef AAX_TYPE_IDS_AUDIOSUITE
  AAX_TYPE_ID_ARRAY(aaxTypeIDsAudioSuite,(AAX_TYPE_IDS_AUDIOSUITE));
#endif
  
  WDL_PtrList<IOConfig> channelIO;
  int totalNInChans = 0, totalNOutChans = 0;
  int totalNInBuses = 0, totalNOutBuses = 0;
  
  const int NIOConfigs = IPlugProcessor::ParseChannelIOStr(PLUG_CHANNEL_IO, channelIO, totalNInChans, totalNOutChans, totalNInBuses, totalNOutBuses);
  
  for (int configIdx = 0; configIdx < NIOConfigs; configIdx++)
  {
    IOConfig* pConfig = channelIO.Get(configIdx);
    
    AAX_CTypeID typeId = aaxTypeIDs[configIdx]; // TODO: aaxTypeIDs must be the same size as NIOConfigs, can we assert somehow if not?
    
    // Describe the algorithm and effect specifics using the CInstrumentParameters convenience layer.  (Native Only)
    AAX_SIPlugSetupInfo setupInfo;
    if(PLUG_TYPE == 1 && pConfig->GetTotalNChannels(kInput) == 0) {
      // For some reason in protools instruments need to have input buses if not defined set input chan count the same as output
      setupInfo.mInputStemFormat = (AAX_EStemFormat) GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kOutput, 0 /* first bus */, pConfig);
    }
    else
      setupInfo.mInputStemFormat = (AAX_EStemFormat) GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kInput, 0 /* first bus */, pConfig);

    setupInfo.mOutputStemFormat = (AAX_EStemFormat) GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kOutput, 0 /* first bus */, pConfig);
    setupInfo.mManufacturerID = PLUG_MFR_ID;
    setupInfo.mProductID = PLUG_UNIQUE_ID;
    setupInfo.mPluginID = typeId;
    #if AAX_DOES_AUDIOSUITE
    setupInfo.mAudioSuiteID = aaxTypeIDsAudioSuite[configIdx];
    #endif
    setupInfo.mCanBypass = true;
    setupInfo.mNeedsInputMIDI = PLUG_DOES_MIDI_IN;
    setupInfo.mInputMIDINodeName = PLUG_NAME" Midi";
    setupInfo.mInputMIDIChannelMask = 0x0001;
    
    setupInfo.mNeedsOutputMIDI = PLUG_DOES_MIDI_OUT;
    setupInfo.mOutputMIDINodeName = PLUG_NAME" Midi";
    setupInfo.mOutputMIDIChannelMask = 0x0001;
    
    setupInfo.mNeedsTransport = true;
    setupInfo.mLatency = PLUG_LATENCY;

    err |= AAX_CIPlugParameters::StaticDescribe(pDesc, setupInfo);

    AAX_ASSERT (err == AAX_SUCCESS);
  }

  // Data model
  err |= pDesc->AddProcPtr((void*) IPlugAAX::Create, kAAX_ProcPtrID_Create_EffectParameters);
  
  // GUI
#if PLUG_HAS_UI
  err |= pDesc->AddProcPtr((void*) AAX_CEffectGUI_IPLUG::Create, kAAX_ProcPtrID_Create_EffectGUI);
#endif
  
  if (err == AAX_SUCCESS)
    err = pC->AddEffect(BUNDLE_ID, pDesc);
  
  AAX_ASSERT (err == AAX_SUCCESS);
  
  pC->SetManufacturerName(AAX_PLUG_MFR_STR);
  
  pC->SetPackageVersion(PLUG_VERSION_HEX);
  
  return err;
}
