#if WINDOWS_VERSION
#include <windows.h>
#include "Mac2Win.H"
#endif

#include "IPlugGroup.h"
#include "IPlugDigiView.h"

#include "CEffectTypeAS.h"
#include "CEffectTypeRTAS.h"
#include "IPlugProcessRTAS.h"
//#include "IPlugProcessAS.h"
#include "Resource.h"

#ifndef PLUG_SC_CHANS
  #define PLUG_SC_CHANS 0
#endif

static OSType plugid = PLUG_UNIQUE_ID;

// TODO: does plugid have to be different for different channel configs?
static CEffectProcess* NewProcessRTAS() 
{
  return new IPlugProcessRTAS(plugid);
}

//static CEffectProcess* NewProcessAS()
//{
//  return new IPlugProcessAS(plugid + 100);
//}

//static CEffectProcess* NewProcessRTASMono() 
//{
//  return new IPlugProcessRTAS(plugid);
//}
//
//static CEffectProcess* NewProcessRTASStereo() 
//{
//  return new IPlugProcessRTAS(plugid+1);
//}
//
//static CEffectProcess* NewProcessRTASMono2Stereo()  
//{
//  return new IPlugProcessRTAS(plugid+2);
//}

#if WINDOWS_VERSION
extern void *hInstance;
extern HINSTANCE gHInstance;
#endif

//TODO: what about ePlugIn_StemFormat_LCRS, ePlugIn_StemFormat_6dot0, ePlugIn_StemFormat_7dot0SDDS, ePlugIn_StemFormat_7dot1SDDS, ePlugIn_StemFormat_7dot0DTS
static EPlugIn_StemFormat getStemFormatForChans(const int numChans)
{
  switch (numChans)
  {
    case 0:
      return ePlugIn_StemFormat_Generic;
    case 1:
      return ePlugIn_StemFormat_Mono;
    case 2:
      return ePlugIn_StemFormat_Stereo;
    case 3:
      return ePlugIn_StemFormat_LCR;
    case 4:
      return ePlugIn_StemFormat_Quad;
    case 5:
      return ePlugIn_StemFormat_5dot0;
    case 6:
      return ePlugIn_StemFormat_5dot1;
    case 7:
      return ePlugIn_StemFormat_6dot1;
    case 8:
      return ePlugIn_StemFormat_7dot1DTS;
    default:
      return ePlugIn_StemFormat_Generic;
  }
}

IPlugGroup::IPlugGroup(void)
{ 
#if WINDOWS_VERSION
  hInstance=gThisModule;
  gHInstance = (HINSTANCE)gThisModule;
#endif
  
  DefineManufacturerNamesAndID (PLUG_MFR_DIGI, PLUG_MFR_ID);
  DefinePlugInNamesAndVersion(PLUG_NAME_DIGI, (PLUG_VER & 0xFFFF0000) >> 16);
  AddGestalt(pluginGestalt_IsCacheable);
}

IPlugGroup::~IPlugGroup(void) {}

void IPlugGroup::CreateEffectTypes(void)
{  
  OSType productID = PLUG_UNIQUE_ID;
  EPlugInCategory category = ePlugInCategory_Effect;
  
  if (PLUG_IS_INST) category = ePlugInCategory_SWGenerators;
  else if(strcmp(EFFECT_TYPE_DIGI, "None") == 0) category = ePlugInCategory_None;
  else if(strcmp(EFFECT_TYPE_DIGI, "EQ") == 0) category = ePlugInCategory_EQ;
  else if(strcmp(EFFECT_TYPE_DIGI, "Dynamics") == 0) category = ePlugInCategory_Dynamics;
  else if(strcmp(EFFECT_TYPE_DIGI, "PitchShift") == 0) category = ePlugInCategory_PitchShift;
  else if(strcmp(EFFECT_TYPE_DIGI, "Reverb") == 0) category = ePlugInCategory_Reverb;
  else if(strcmp(EFFECT_TYPE_DIGI, "Delay") == 0) category = ePlugInCategory_Delay;
  else if(strcmp(EFFECT_TYPE_DIGI, "Modulation") == 0) category = ePlugInCategory_Modulation;
  else if(strcmp(EFFECT_TYPE_DIGI, "Harmonic") == 0) category = ePlugInCategory_Harmonic;
  else if(strcmp(EFFECT_TYPE_DIGI, "NoiseReduction") == 0) category = ePlugInCategory_NoiseReduction;
  else if(strcmp(EFFECT_TYPE_DIGI, "Dither") == 0) category = ePlugInCategory_Dither;
  else if(strcmp(EFFECT_TYPE_DIGI, "SoundField") == 0) category = ePlugInCategory_SoundField;
  else if(strcmp(EFFECT_TYPE_DIGI, "Effect") == 0) category = ePlugInCategory_Effect;

  char *channelIOStr = PLUG_CHANNEL_IO;
  
  int ioConfigIdx = 0;
  int nSIn = (PLUG_SC_CHANS > 0); // force it to 1
  
  while (channelIOStr) 
  {
    int nIn = 0, nOut = 0;

    if (sscanf(channelIOStr, "%d-%d", &nIn, &nOut) == 2)    
    {
      // if we have a 1-N config + sidechain we don't want to include the sidechain
      if (nIn > 1)
        nIn -= nSIn;
      
      OSType typeId = plugid + ioConfigIdx;
            
      CEffectType* RTAS = new CEffectTypeRTAS(typeId, productID, category);
      RTAS->DefineTypeNames(PLUG_NAME_DIGI);
      RTAS->DefineSampleRateSupport(eSupports48kAnd96kAnd192k);
      RTAS->DefineStemFormats(getStemFormatForChans(nIn), getStemFormatForChans(nOut));
      RTAS->AddGestalt(pluginGestalt_CanBypass);
      RTAS->AddGestalt(pluginGestalt_SupportsVariableQuanta);
      RTAS->AddGestalt(pluginGestalt_DoesNotUseDigiUI);
      //RTAS->AddGestalt(pluginGestalt_UsesCustomPlugInSettingsFile); // TODO
      RTAS->AttachEffectProcessCreator(NewProcessRTAS);
      
      if (nSIn)
        RTAS->AddGestalt(pluginGestalt_SideChainInput);
      
      AddEffectType (RTAS);
      
#if PLUG_DOES_OFFLINE
      typeId = plugid + 100 + ioConfigIdx;
      
      CEffectType* AS = new CEffectTypeAS(typeId, productID, category);
      AS->DefineTypeNames(PLUG_NAME_DIGI);
      AS->DefineSampleRateSupport(eSupports48kAnd96kAnd192k);
      AS->AddGestalt(pluginGestalt_UseSmallPreviewBuffer);
      AS->DefineStemFormats(getStemFormatForChans(nIn), getStemFormatForChans(nOut));
      AS->AddGestalt(pluginGestalt_CanBypass);
      AS->AddGestalt(pluginGestalt_DoesNotUseDigiUI);
      //RTAS->AddGestalt(pluginGestalt_UsesCustomPlugInSettingsFile); // TODO
      AS->AttachEffectProcessCreator(NewProcessAS);
      
      AddEffectType (AS);
#endif      
      ioConfigIdx++;
    }
      
    channelIOStr = strstr(channelIOStr, " ");
      
    if (channelIOStr)
      ++channelIOStr;
  }
}

// This is here just to save an extra .cpp
CPlugInView* CreateIPlugDigiView()
{
  return new IPlugDigiView;
}

void IPlugGroup::Initialize(void)     
{
  CEffectGroup::Initialize ();  // Always call inherited first
  
  CCustomView::AddNewViewProc("IPlugDigiView", CreateIPlugDigiView);
}

CProcessGroupInterface* CProcessGroup::CreateProcessGroup(void)
{
  return new IPlugGroup;
}
