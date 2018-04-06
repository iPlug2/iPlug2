#pragma once

/** \file IPlug_include_in_plug_src.h
    \brief IPlug source include

    Include this file in the main source for your plugin, after #including the main header for your plugin.
    A preprocessor macro for a particular API such as VST2_API should be defined at project level
    Depending on the API macro defined, a different entry point and helper methods are activated
*/
#if defined OS_WIN
  HINSTANCE gHInstance = 0;
  #if defined(VST2_API) || defined(AAX_API)
  #ifdef __MINGW32__
  extern "C"
  #endif
  BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID res)
  {
    gHInstance = hDllInst;
    return TRUE;
  }
  #endif

  #ifndef NO_IGRAPHICS
  IGraphics* MakeGraphics(IDelegate& dlg, int w, int h, int fps = 0)
  {
    IGraphicsWin* pGraphics = new IGraphicsWin(dlg, w, h, fps);
    pGraphics->SetPlatformInstance(gHInstance);
    return pGraphics;
  }
  #endif
#elif defined OS_MAC
  #ifndef NO_IGRAPHICS
  IGraphics* MakeGraphics(IDelegate& dlg, int w, int h, int fps = 0)
  {
    IGraphicsMac* pGraphics = new IGraphicsMac(dlg, w, h, fps);
    pGraphics->SetBundleID(BUNDLE_ID);
    #ifdef IGRAPHICS_NANOVG
    pGraphics->CreateMetalLayer();
    #endif
    return pGraphics;
  }
  #endif
#elif defined OS_WEB
//   #ifndef NO_IGRAPHICS
//   IGraphics* MakeGraphics(IGraphicsDelegate& dlg, int w, int h, int fps = 0)
//   {
//     IGraphicsWeb* pGraphics = new IGraphicsWeb(dlg, w, h, fps);
//     return pGraphics;
//   }
//   #endif
#else
  #error "No OS defined!"
#endif

#if defined VST2_API
  extern "C"
  {
    EXPORT void* VSTPluginMain(audioMasterCallback hostCallback)
    {
      IPlugInstanceInfo instanceInfo;
      instanceInfo.mVSTHostCallback = hostCallback;
      IPlugVST2* pPlug = new PLUG_CLASS_NAME(instanceInfo);

      if (pPlug)
      {
        pPlug->EnsureDefaultPreset();
        pPlug->mAEffect.numPrograms = std::max(pPlug->mAEffect.numPrograms, 1); // some hosts don't like 0 presets
        return &(pPlug->mAEffect);
      }
      return 0;
    }
    EXPORT int main(int hostCallback)
    {
    #if defined OS_MAC
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

static Steinberg::FUnknown* createInstance (void*) {
  return (Steinberg::Vst::IAudioProcessor*) MakePlug();
}

// Company Information
BEGIN_FACTORY_DEF (PLUG_MFR, PLUG_URL_STR, PLUG_EMAIL_STR)

DEF_CLASS2 (INLINE_UID(GUID_DATA1, GUID_DATA2, GUID_DATA3, GUID_DATA4),
            Steinberg::PClassInfo::kManyInstances,              // cardinality
            kVstAudioEffectClass,                               // the component category (don't change this)
            PLUG_NAME,                                          // plug-in name
            Steinberg::Vst::kSimpleModeSupported,               // kSimpleModeSupported because we can't split the gui and plugin
            VST3_SUBCATEGORY,                                   // Subcategory for this plug-in
            PLUG_VERSION_STR,                                       // plug-in version
            kVstVersionString,                                  // the VST 3 SDK version (dont changed this, use always this define)
            createInstance)                                     // function pointer called when this component should be instantiated

END_FACTORY

#elif defined AU_API
//#include <AvailabilityMacros.h>
  IPlug* MakePlug(void* pMemory)
  {
    IPlugInstanceInfo instanceInfo;
    instanceInfo.mBundleID.Set(BUNDLE_ID);
    instanceInfo.mCocoaViewFactoryClassName.Set(AUV2_VIEW_CLASS_STR);

    if(pMemory)
      return new(pMemory) PLUG_CLASS_NAME(instanceInfo);
    else
      return new PLUG_CLASS_NAME(instanceInfo);
  }

class IPlugAUFactory
{
  public:
    static void* Construct(void* pMemory, AudioComponentInstance compInstance)
    {
      return MakePlug(pMemory);
    }

    static void Destruct(void* pMemory)
    {
      ((PLUG_CLASS_NAME*) pMemory)->~PLUG_CLASS_NAME();
    }

    static AudioComponentMethod Lookup (SInt16 selector)
    {
      switch (selector) {
        case kAudioUnitInitializeSelect:  return (AudioComponentMethod)IPlugAU::AUMethodInitialize;
        case kAudioUnitUninitializeSelect:  return (AudioComponentMethod)IPlugAU::AUMethodUninitialize;
        case kAudioUnitGetPropertyInfoSelect:	return (AudioComponentMethod)IPlugAU::AUMethodGetPropertyInfo;
        case kAudioUnitGetPropertySelect: return (AudioComponentMethod)IPlugAU::AUMethodGetProperty;
        case kAudioUnitSetPropertySelect: return (AudioComponentMethod)IPlugAU::AUMethodSetProperty;
        case kAudioUnitAddPropertyListenerSelect:return (AudioComponentMethod)IPlugAU::AUMethodAddPropertyListener;
        case kAudioUnitRemovePropertyListenerSelect:  return (AudioComponentMethod)IPlugAU::AUMethodRemovePropertyListener;
        case kAudioUnitRemovePropertyListenerWithUserDataSelect:  return (AudioComponentMethod)IPlugAU::AUMethodRemovePropertyListenerWithUserData;
        case kAudioUnitAddRenderNotifySelect:	return (AudioComponentMethod)IPlugAU::AUMethodAddRenderNotify;
        case kAudioUnitRemoveRenderNotifySelect:return (AudioComponentMethod)IPlugAU::AUMethodRemoveRenderNotify;
        case kAudioUnitGetParameterSelect:  return (AudioComponentMethod)IPlugAU::AUMethodGetParameter;
        case kAudioUnitSetParameterSelect:  return (AudioComponentMethod)IPlugAU::AUMethodSetParameter;
        case kAudioUnitScheduleParametersSelect:return (AudioComponentMethod)IPlugAU::AUMethodScheduleParameters;
        case kAudioUnitRenderSelect:  return (AudioComponentMethod)IPlugAU::AUMethodRender;
        case kAudioUnitResetSelect: return (AudioComponentMethod)IPlugAU::AUMethodReset;
#if PLUG_DOES_MIDI
        case kMusicDeviceMIDIEventSelect:  return (AudioComponentMethod)IPlugAU::AUMethodMIDIEvent;
        case kMusicDeviceSysExSelect:  return (AudioComponentMethod)IPlugAU::AUMethodSysEx;
#endif
        default:
          break;
      }
      return NULL;
    }

    static OSStatus Open(void* pSelf, AudioUnit compInstance)
    {
      AudioComponentPlugInInstance* acpi = (AudioComponentPlugInInstance *) pSelf;
      assert(acpi);

      (*acpi->mConstruct)(&acpi->mInstanceStorage, compInstance);
      IPlugAU* plug = (IPlugAU*) &acpi->mInstanceStorage;

      plug->mCI = compInstance;
      plug->HostSpecificInit();
      plug->PruneUninitializedPresets();

      return noErr;
    }

    static OSStatus Close(void* pSelf)
    {
      AudioComponentPlugInInstance* acpi = (AudioComponentPlugInInstance *) pSelf;
      assert(acpi);
      (*acpi->mDestruct)(&acpi->mInstanceStorage);
      free(pSelf);
      return noErr;
    }

    static AudioComponentPlugInInterface* Factory(const AudioComponentDescription* pInDesc)
    {
      AudioComponentPlugInInstance* acpi = (AudioComponentPlugInInstance*) malloc( offsetof(AudioComponentPlugInInstance, mInstanceStorage) + sizeof(PLUG_CLASS_NAME) );
      acpi->mPlugInInterface.Open = Open;
      acpi->mPlugInInterface.Close = Close;
      acpi->mPlugInInterface.Lookup = Lookup;
      acpi->mPlugInInterface.reserved = NULL;
      acpi->mConstruct = Construct;
      acpi->mDestruct = Destruct;
      acpi->mPad[0] = NULL;
      acpi->mPad[1] = NULL;
      return (AudioComponentPlugInInterface*)acpi;
    }

  };

extern "C"
  {
#ifndef AU_NO_COMPONENT_ENTRY
    //Component Manager
    EXPORT ComponentResult AUV2_ENTRY(ComponentParameters* pParams, void* pPlug)
    {
      return IPlugAU::IPlugAUEntry(pParams, pPlug);
    }
#endif

    //>10.7 SDK AUPlugin
    EXPORT void* AUV2_FACTORY(const AudioComponentDescription* pInDesc)
    {
      return IPlugAUFactory::Factory(pInDesc);
    }

  };
#elif defined AUv3_API
  IPlug* MakePlug()
  {
    IPlugInstanceInfo instanceInfo;
    instanceInfo.mBundleID.Set(BUNDLE_ID);
    return new PLUG_CLASS_NAME(instanceInfo);
  }
#elif defined AAX_API
  IPlug* MakePlug()
  {
    IPlugInstanceInfo instanceInfo;

    return new PLUG_CLASS_NAME(instanceInfo);
  }
#elif defined APP_API
  IPlug* MakePlug(void* pMidiOutput, uint16_t& midiOutChan)
  {
    IPlugInstanceInfo instanceInfo;

    instanceInfo.mRTMidiOut = (RtMidiOut*) pMidiOutput;
    instanceInfo.mMidiOutChan = midiOutChan;

    #if defined OS_MAC
      instanceInfo.mBundleID.Set(BUNDLE_ID);
    #endif

    return new PLUG_CLASS_NAME(instanceInfo);
  }
#elif defined WAM_API

#else
  #error "No API defined!"
#endif

#if defined OS_MAC && !defined APP_API
#include <sys/time.h>
#include <unistd.h>
void Sleep(int ms)
{
  usleep(ms?ms*1000:100);
}

DWORD GetTickCount()
{
  struct timeval tm={0,};
  gettimeofday(&tm,NULL);
  return (DWORD) (tm.tv_sec*1000 + tm.tv_usec/1000);
}
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

#define PUBLIC_NAME PLUG_NAME

#define IPLUG_CTOR(nParams, nPresets, instanceInfo) \
  IPlug(instanceInfo, IPlugConfig(nParams, nPresets, PLUG_CHANNEL_IO,\
    PUBLIC_NAME, "", PLUG_MFR, PLUG_VERSION_HEX, PLUG_UNIQUE_ID, PLUG_MFR_ID, \
    PLUG_LATENCY, PLUG_DOES_MIDI, PLUG_DOES_STATE_CHUNKS, PLUG_IS_INSTRUMENT, \
    PLUG_HAS_UI, PLUG_WIDTH, PLUG_HEIGHT))
