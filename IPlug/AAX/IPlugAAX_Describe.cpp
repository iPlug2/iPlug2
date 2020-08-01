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
static uint64_t GetAPIBusTypeForChannelIOConfig(int configIdx, ERoute dir, int busIdx, const IOConfig* pConfig, WDL_TypedBuf<uint64_t>* APIBusTypes = nullptr)
{
  assert(pConfig != nullptr);
  assert(busIdx >= 0 && busIdx < pConfig->NBuses(dir));
  
  int numChans = pConfig->GetBusInfo(dir, busIdx)->NChans();

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
extern uint64_t GetAPIBusTypeForChannelIOConfig(int configIdx, iplug::ERoute dir, int busIdx, const iplug::IOConfig* pConfig, WDL_TypedBuf<uint64_t>* APIBusTypes = nullptr);
#endif //CUSTOM_BUSTYPE_FUNC

AAX_Result GetEffectDescriptions(AAX_ICollection* pC)
{
  AAX_Result err = AAX_SUCCESS;
  AAX_IEffectDescriptor* pDesc = pC->NewDescriptor();
 
  if (pDesc == NULL)
    return AAX_ERROR_NULL_OBJECT;

  WDL_String subStr;

  WDL_String fullPlugNameStr {AAX_PLUG_NAME_STR};
  char *plugNameStr = fullPlugNameStr.Get();
    
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
  
  AAX_CTypeID aaxTypeIDs[] = {AAX_TYPE_IDS};
#ifdef AAX_TYPE_IDS_AUDIOSUITE
  AAX_CTypeID aaxTypeIDsAudioSuite[] = {AAX_TYPE_IDS_AUDIOSUITE};
#endif
  
  WDL_PtrList<IOConfig> channelIO;
  int totalNInChans = 0, totalNOutChans = 0;
  int totalNInBuses = 0, totalNOutBuses = 0;
  
  const int NIOConfigs = IPlugProcessor::ParseChannelIOStr(PLUG_CHANNEL_IO, channelIO, totalNInChans, totalNOutChans, totalNInBuses, totalNOutBuses);
  
  auto PopulateSetupInfo = [](int configIdx, const IOConfig* pConfig, int typeId, int audioSuiteId, AAX_SIPlugSetupInfo& setupInfo){
    if(PLUG_TYPE == 1 && pConfig->GetTotalNChannels(kInput) == 0) {
      // For some reason in protools instruments need to have input buses if not defined set input chan count the same as output
      setupInfo.mInputStemFormat = (AAX_EStemFormat) GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kOutput, 0 /* first bus */, pConfig);
    }
    else
      setupInfo.mInputStemFormat = (AAX_EStemFormat) GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kInput, 0 /* first bus */, pConfig);

    setupInfo.mOutputStemFormat = (AAX_EStemFormat) GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kOutput, 0 /* first bus */, pConfig);
    
    setupInfo.mNumAuxOutputStems = pConfig->NBuses(ERoute::kOutput) - 1;
        
    for (int i = 0; i < setupInfo.mNumAuxOutputStems; ++i)
    {
      setupInfo.mAuxOutputStemFormats[i] = (AAX_EStemFormat) GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kOutput, i, pConfig);
      
#ifdef AAX_AOS_STRS
      static const char* aaxAOSStrs[] = {AAX_AOS_STRS};
      setupInfo.mAuxOutputStemNames[i] = aaxAOSStrs[i];
#else
      WDL_String str;
      str.SetFormatted(MAX_BUS_NAME_LEN, "Output %i", i+2);
      setupInfo.mAuxOutputStemNames[i] = str.Get();
#endif
    }
    
    setupInfo.mManufacturerID = PLUG_MFR_ID;
    setupInfo.mProductID = PLUG_UNIQUE_ID;
    setupInfo.mPluginID = typeId;
    #if AAX_DOES_AUDIOSUITE
    setupInfo.mAudiosuiteID = audioSuiteId;
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
  };
  
  if((PLUG_TYPE != 1) && (totalNInBuses > 1)) // Effect with sidechain input
  {
    int aaxTypeIdIdx = 0;
    for (int configIdx = 0; configIdx < NIOConfigs; configIdx++) // loop through all configs
    {
      const IOConfig* pConfig = channelIO.Get(configIdx);
      
      if(pConfig->NBuses(ERoute::kInput) == 1) // and only add the configs with no explicit sidechain
      {
        AAX_CTypeID typeId = aaxTypeIDs[aaxTypeIdIdx];
        #if AAX_DOES_AUDIOSUITE
        AAX_CTypeID audioSuiteId = aaxTypeIDsAudioSuite[aaxTypeIdIdx];
        #else
        AAX_CTypeID audioSuiteId = 0;
        #endif
        AAX_SIPlugSetupInfo setupInfo;
        PopulateSetupInfo(configIdx, pConfig, typeId, audioSuiteId, setupInfo);
        setupInfo.mWantsSideChain = true;
        
        err |= AAX_CIPlugParameters::StaticDescribe(pDesc, setupInfo);

        AAX_ASSERT (err == AAX_SUCCESS);
        
        aaxTypeIdIdx++;
      }
    }
  }
  else if((PLUG_TYPE == 1) && (totalNOutBuses > 1)) // Instrument with multi-bus outputs
  {
    int configIdx = NIOConfigs-1; // Take the last IOConfig, assuming it contains all the busses
    
    const IOConfig* pConfig = channelIO.Get(configIdx);
    
    AAX_CTypeID typeId = aaxTypeIDs[0]; // There should only be one ID
    
    AAX_SIPlugSetupInfo setupInfo;
    PopulateSetupInfo(configIdx, pConfig, typeId, 0, setupInfo);
    
    err |= AAX_CIPlugParameters::StaticDescribe(pDesc, setupInfo);

    AAX_ASSERT (err == AAX_SUCCESS);
  }
  else
  {
    for (int configIdx = 0; configIdx < NIOConfigs; configIdx++)
    {
      const IOConfig* pConfig = channelIO.Get(configIdx);
      
      AAX_CTypeID typeId = aaxTypeIDs[configIdx];
#if AAX_DOES_AUDIOSUITE
      AAX_CTypeID audioSuiteId = aaxTypeIDsAudioSuite[configIdx];
#else
      AAX_CTypeID audioSuiteId = 0;
#endif
      AAX_SIPlugSetupInfo setupInfo;
      PopulateSetupInfo(configIdx, pConfig, typeId, audioSuiteId, setupInfo);
      
      err |= AAX_CIPlugParameters::StaticDescribe(pDesc, setupInfo);

      AAX_ASSERT (err == AAX_SUCCESS);
    }
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
