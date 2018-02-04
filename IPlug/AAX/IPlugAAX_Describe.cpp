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
  #define BUNDLE_ID "com." BUNDLE_MFR ".aax." BUNDLE_NAME
#endif

#define args(...) __VA_ARGS__
#define AAX_TYPE_ID_ARRAY(VARNAME, ARR_DATA) AAX_CTypeID VARNAME[] = {args ARR_DATA}

#ifndef MULTICHANNEL_BUSTYPE_FUNC
uint64_t GetAPIBusTypeForChannelIOConfig(int channelIOConfigIdx, int element, int numChans)
{
  switch (numChans)
  {
    case 0:
      return (uint64_t) AAX_eStemFormat_None;
    case 1:
      return (uint64_t) AAX_eStemFormat_Mono;
    case 2:
      return (uint64_t) AAX_eStemFormat_Stereo;
    default:
      DBGMSG("for anything other than mono or stereo buses, you need to implement GetAPIBusTypeForChannelIOConfig() and #define MULTICHANNEL_BUSTYPE_FUNC\n");
      assert(numChans > 2);
      return AAX_eStemFormat_None;
  }
}
#else
extern uint64_t GetAPIBusTypeForChannelIOConfig(int channelIOConfigIdx, int element, int numChans);
#endif //MULTICHANNEL_BUSTYPE_FUNC

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
  if (PLUG_IS_INSTRUMENT) category = AAX_ePlugInCategory_SWGenerators;
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
  
  //err |= effectDescriptor->AddResourceInfo ( AAX_eResourceType_PageTable, PLUG_NAME ".xml" );
  
  AAX_TYPE_ID_ARRAY(aaxTypeIDs,(AAX_TYPE_IDS));
  AAX_TYPE_ID_ARRAY(aaxTypeIDsAudioSuite,(AAX_TYPE_IDS_AUDIOSUITE));

  WDL_PtrList<IOConfig> channelIO;
  int totalNInChans = 0, totalNOutChans = 0;
  int totalNInBuses = 0, totalNOutBuses = 0;
  
  const int NIOConfigs = IPlugBase::ParseChannelIOStr(PLUG_CHANNEL_IO, channelIO, totalNInChans, totalNOutChans, totalNInBuses, totalNOutBuses);
  
  for (int ioConfigIdx = 0; ioConfigIdx < NIOConfigs; ioConfigIdx++)
  {
    AAX_CTypeID typeId = aaxTypeIDs[ioConfigIdx]; // TODO: aaxTypeIDs must be the same size as NIOConfigs, can we assert somehow if not?
    
    // Describe the algorithm and effect specifics using the CInstrumentParameters convenience layer.  (Native Only)
    AAX_SIPlugSetupInfo setupInfo;
    if(PLUG_IS_INSTRUMENT) // For some reason in protools instruments need to have input buses. 
      setupInfo.mInputStemFormat = (AAX_EStemFormat) GetAPIBusTypeForChannelIOConfig(ioConfigIdx, -1, channelIO.Get(ioConfigIdx)->GetOutputBusInfo(0)->mNChans);
    else
      setupInfo.mInputStemFormat = (AAX_EStemFormat) GetAPIBusTypeForChannelIOConfig(ioConfigIdx, -1, channelIO.Get(ioConfigIdx)->GetInputBusInfo(0)->mNChans);

    setupInfo.mOutputStemFormat = (AAX_EStemFormat) GetAPIBusTypeForChannelIOConfig(ioConfigIdx, -1, channelIO.Get(ioConfigIdx)->GetOutputBusInfo(0)->mNChans);
    setupInfo.mManufacturerID = PLUG_MFR_ID;
    setupInfo.mProductID = PLUG_UNIQUE_ID;
    setupInfo.mPluginID = typeId;
    #if AAX_DOES_AUDIOSUITE
    setupInfo.mAudioSuiteID = aaxTypeIDsAudioSuite[ioConfigIdx];
    #endif
    setupInfo.mCanBypass = true;
    setupInfo.mNeedsInputMIDI = PLUG_DOES_MIDI;
    setupInfo.mInputMIDINodeName = PLUG_NAME" Midi";
    setupInfo.mInputMIDIChannelMask = 0x0001;
//    setupInfo.mNeedsGlobalMIDI = PLUG_DOES_MIDI;
//    setupInfo.mGlobalMIDIEventMask = 0x3;
    setupInfo.mNeedsTransport = true;
    setupInfo.mLatency = PLUG_LATENCY;

    err |= AAX_CIPlugParameters::StaticDescribe(pDesc, setupInfo);

    AAX_ASSERT (err == AAX_SUCCESS);
  }

  // Data model
  err |= pDesc->AddProcPtr( (void*) IPlugAAX::Create, kAAX_ProcPtrID_Create_EffectParameters );
  
  // GUI
#if PLUG_HAS_UI
  err |= pDesc->AddProcPtr( (void*) AAX_CEffectGUI_IPLUG::Create, kAAX_ProcPtrID_Create_EffectGUI );
#endif
  
  if ( err == AAX_SUCCESS )
    err = pC->AddEffect(BUNDLE_ID, pDesc );
  
  AAX_ASSERT (err == AAX_SUCCESS);
  
  char* mfrNameStr = AAX_PLUG_MFR_STR;
  
  while (mfrNameStr)
  {
    auto span = strcspn(mfrNameStr, "\n");
    
    if (span)
    {
      subStr.Set(mfrNameStr, (int) span);
      pC->SetManufacturerName(subStr.Get());
      mfrNameStr = strstr(mfrNameStr, "\n");
      
      if (mfrNameStr)
        ++mfrNameStr;
    }
    else
    {
      break;
    }
  }
  
  pC->SetPackageVersion(PLUG_VERSION_HEX);
  
  return err;
}
