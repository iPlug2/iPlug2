#ifndef _IPLUG_INCLUDE_SRC_
#define _IPLUG_INCLUDE_SRC_

// Include this file in the main source for your plugin,
// after #including the main header for your plugin.

#if defined OS_WIN
  HINSTANCE gHInstance = 0;
  #if defined(VST_API) || defined(AAX_API) //TODO check
  #ifdef __MINGW32__
  extern "C"
  #endif
  BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID res)
  {
    gHInstance = hDllInst;
    return TRUE;
  }
  #endif

  IGraphics* MakeGraphics(IPlug* pPlug, int w, int h, int FPS = 0)
  {
    IGraphicsWin* pGraphics = new IGraphicsWin(pPlug, w, h, FPS);

    pGraphics->SetHInstance(gHInstance);
    return pGraphics;
  }
#elif defined OS_OSX
  IGraphics* MakeGraphics(IPlug* pPlug, int w, int h, int FPS = 0)
  {
    IGraphicsMac* pGraphics = new IGraphicsMac(pPlug, w, h, FPS);
    pGraphics->SetBundleID(BUNDLE_ID);
    return pGraphics;
  }
#elif defined OS_IOS
  IGraphics* MakeGraphics(IPlug* pPlug, int w, int h, int FPS = 0)
  {
    return 0;
  }
#else
  #error "No OS defined!"
#endif

#if defined VST_API
  extern "C"
  {
    EXPORT void* VSTPluginMain(audioMasterCallback hostCallback)
    {
      static WDL_Mutex sMutex;
      WDL_MutexLock lock(&sMutex);
      IPlugInstanceInfo instanceInfo;
      instanceInfo.mVSTHostCallback = hostCallback;
      IPlugVST* pPlug = new PLUG_CLASS_NAME(instanceInfo);
      if (pPlug) {
        pPlug->EnsureDefaultPreset();
        pPlug->mAEffect.numPrograms = IPMAX(pPlug->mAEffect.numPrograms, 1);
        return &(pPlug->mAEffect);
      }
      return 0;
    }
    EXPORT int main(int hostCallback)
    {
    #if defined OS_OSX
      return (VstIntPtr) VSTPluginMain((audioMasterCallback)hostCallback);
    #else
      return (int) VSTPluginMain((audioMasterCallback)hostCallback);
    #endif
    }
  };
#elif defined VST3_API
#include "public.sdk/source/main/pluginfactoryvst3.h"

unsigned int GUID_DATA1 = 0xF2AEE70D;
unsigned int GUID_DATA2 = 0x00DE4F4E;
unsigned int GUID_DATA3 = PLUG_MFR_ID;
unsigned int GUID_DATA4 = PLUG_UNIQUE_ID;

#ifndef EFFECT_TYPE_VST3
  #if PLUG_IS_INST
    #define EFFECT_TYPE_VST3 kInstrumentSynth
  #else
    #define EFFECT_TYPE_VST3 kFx
  #endif
#endif

using namespace Steinberg::Vst;

// called after library was loaded
bool InitModule ()
{
  #ifdef OS_WIN
  extern void* moduleHandle;
  gHInstance = (HINSTANCE) moduleHandle;
  #endif

  return true;
}

// called after library is unloaded
bool DeinitModule ()
{
  return true;
}

IPlug* MakePlug()
{
  static WDL_Mutex sMutex;
  WDL_MutexLock lock(&sMutex);
  IPlugInstanceInfo instanceInfo;

  return new PLUG_CLASS_NAME(instanceInfo);
}

static FUnknown* createInstance (void*) {
  return (IAudioProcessor*) MakePlug();
}

// Company Information
BEGIN_FACTORY_DEF (PLUG_MFR, MFR_URL, MFR_EMAIL)

DEF_CLASS2 (INLINE_UID(GUID_DATA1, GUID_DATA2, GUID_DATA3, GUID_DATA4),
            PClassInfo::kManyInstances,                         // cardinality
            kVstAudioEffectClass,                               // the component category (don't change this)
            PLUG_NAME,                                          // plug-in name
            Vst::kSimpleModeSupported,                          // kSimpleModeSupported because we can't split the gui and plugin
            EFFECT_TYPE_VST3,                                   // Subcategory for this plug-in
            VST3_VER_STR,                                       // plug-in version
            kVstVersionString,                                  // the VST 3 SDK version (dont changed this, use always this define)
            createInstance)                                     // function pointer called when this component should be instantiated

END_FACTORY

#elif defined AU_API
  IPlug* MakePlug()
  {
    static WDL_Mutex sMutex;
    WDL_MutexLock lock(&sMutex);
    IPlugInstanceInfo instanceInfo;
    instanceInfo.mOSXBundleID.Set(BUNDLE_ID);
    instanceInfo.mCocoaViewFactoryClassName.Set(VIEW_CLASS_STR);
    return new PLUG_CLASS_NAME(instanceInfo);
  }
  extern "C"
  {
    EXPORT ComponentResult PLUG_ENTRY(ComponentParameters* params, void* pPlug)
    {
      return IPlugAU::IPlugAUEntry(params, pPlug);
    }
    EXPORT ComponentResult PLUG_VIEW_ENTRY(ComponentParameters* params, void* pView)
    {
      return IPlugAU::IPlugAUCarbonViewEntry(params, pView);
    }
  };
#elif defined RTAS_API
  IPlug* MakePlug()
  {
    static WDL_Mutex sMutex;
    WDL_MutexLock lock(&sMutex);
    IPlugInstanceInfo instanceInfo;

    return new PLUG_CLASS_NAME(instanceInfo);
  }
#elif defined AAX_API
  IPlug* MakePlug()
  {
    static WDL_Mutex sMutex;
    WDL_MutexLock lock(&sMutex);
    IPlugInstanceInfo instanceInfo;
    
    return new PLUG_CLASS_NAME(instanceInfo);
  }
#elif defined SA_API
  //IPlug* MakePlug(void* pMidiOutput, unsigned short* pMidiOutChan)
  IPlug* MakePlug(void* pMidiOutput, unsigned short* pMidiOutChan, void* ioslink)
  {
    static WDL_Mutex sMutex;
    WDL_MutexLock lock(&sMutex);
    IPlugInstanceInfo instanceInfo;

    #if defined OS_WIN
      instanceInfo.mRTMidiOut = (RtMidiOut*) pMidiOutput;
      instanceInfo.mMidiOutChan = pMidiOutChan;
    #elif defined OS_OSX
      instanceInfo.mRTMidiOut = (RtMidiOut*) pMidiOutput;
      instanceInfo.mMidiOutChan = pMidiOutChan;
      instanceInfo.mOSXBundleID.Set(BUNDLE_ID);
    #elif defined OS_IOS
      instanceInfo.mIOSBundleID.Set(BUNDLE_ID);
      instanceInfo.mIOSLink = (IOSLink*) ioslink;
    #endif

    return new PLUG_CLASS_NAME(instanceInfo);
  }

#else
  #error "No API defined!"
#endif
/*
#if defined _DEBUG
  #define PUBLIC_NAME APPEND_TIMESTAMP(PLUG_NAME " DEBUG")
#elif defined TRACER_BUILD
  #define PUBLIC_NAME APPEND_TIMESTAMP(PLUG_NAME " TRACER")
#elif defined TIMESTAMP_PLUG_NAME
  #pragma REMINDER("plug name is timestamped")
  #define PUBLIC_NAME APPEND_TIMESTAMP(PLUG_NAME)
#else
  #define PUBLIC_NAME PLUG_NAME
#endif
  */

#ifndef PLUG_SC_CHANS
  #define PLUG_SC_CHANS 0
#endif

#define PUBLIC_NAME PLUG_NAME

#define IPLUG_CTOR(nParams, nPresets, instanceInfo) \
  IPlug(instanceInfo, nParams, PLUG_CHANNEL_IO, nPresets, \
    PUBLIC_NAME, "", PLUG_MFR, PLUG_VER, PLUG_UNIQUE_ID, PLUG_MFR_ID, \
    PLUG_LATENCY, PLUG_DOES_MIDI, PLUG_DOES_STATE_CHUNKS, PLUG_IS_INST, PLUG_SC_CHANS)

#endif
