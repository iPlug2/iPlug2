#pragma once

// Include this file in the main source for your plugin,
// after #including the main header for your plugin.

#if defined OS_WIN
  HINSTANCE gHInstance = 0;
  #if defined(VST_API) || defined(AAX_API)
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
  IGraphics* MakeGraphics(IPlugBaseGraphics& plug, int w, int h, int fps = 0)
  {
    IGraphicsWin* pGraphics = new IGraphicsWin(plug, w, h, fps);

    pGraphics->SetHInstance(gHInstance);
    return pGraphics;
  }
  #endif
#elif defined OS_OSX
  #ifndef NO_IGRAPHICS
  IGraphics* MakeGraphics(IPlugBaseGraphics& plug, int w, int h, int fps = 0)
  {
    IGraphicsMac* pGraphics = new IGraphicsMac(plug, w, h, fps);
    pGraphics->SetBundleID(BUNDLE_ID);
    return pGraphics;
  }
  #endif
#else
  #error "No OS defined!"
#endif

#if defined VST_API
  extern "C"
  {
    EXPORT void* VSTPluginMain(audioMasterCallback hostCallback)
    {
      IPlugInstanceInfo instanceInfo;
      instanceInfo.mVSTHostCallback = hostCallback;
      IPlugVST* pPlug = new PLUG_CLASS_NAME(instanceInfo);
      if (pPlug) {
        pPlug->EnsureDefaultPreset();
        pPlug->mAEffect.numPrograms = std::max(pPlug->mAEffect.numPrograms, 1);
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
BEGIN_FACTORY_DEF (PLUG_MFR, MFR_URL, MFR_EMAIL)

DEF_CLASS2 (INLINE_UID(GUID_DATA1, GUID_DATA2, GUID_DATA3, GUID_DATA4),
            Steinberg::PClassInfo::kManyInstances,              // cardinality
            kVstAudioEffectClass,                               // the component category (don't change this)
            PLUG_NAME,                                          // plug-in name
            Steinberg::Vst::kSimpleModeSupported,               // kSimpleModeSupported because we can't split the gui and plugin
            EFFECT_TYPE_VST3,                                   // Subcategory for this plug-in
            VST3_VER_STR,                                       // plug-in version
            kVstVersionString,                                  // the VST 3 SDK version (dont changed this, use always this define)
            createInstance)                                     // function pointer called when this component should be instantiated

END_FACTORY

#elif defined AU_API
#include <AvailabilityMacros.h>

  IPlug* MakePlug(void *memory)
  {
    IPlugInstanceInfo instanceInfo;
    instanceInfo.mOSXBundleID.Set(BUNDLE_ID);
    instanceInfo.mCocoaViewFactoryClassName.Set(VIEW_CLASS_STR);
    
    if(memory)
      return new(memory) PLUG_CLASS_NAME(instanceInfo);
    else
      return new PLUG_CLASS_NAME(instanceInfo);
  }

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
class IPlugAUFactory
{
  public:
    static void* Construct(void *memory, AudioComponentInstance compInstance)
    {
      return MakePlug(memory);
    }
    
    static void Destruct(void *memory)
    {
      ((PLUG_CLASS_NAME*)memory)->~PLUG_CLASS_NAME();
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
     
    static OSStatus Open(void *self, AudioUnit compInstance)
    {
      AudioComponentPlugInInstance* acpi = (AudioComponentPlugInInstance *) self;
      assert(acpi);

      (*acpi->mConstruct)(&acpi->mInstanceStorage, compInstance);
      IPlugAU* plug = (IPlugAU*) &acpi->mInstanceStorage;

      plug->mCI = compInstance;
      plug->HostSpecificInit();
      plug->PruneUninitializedPresets();
      
      return noErr;
    }
    
    static OSStatus Close(void *self)
    {
      AudioComponentPlugInInstance* acpi = (AudioComponentPlugInInstance *) self;
      assert(acpi);
      (*acpi->mDestruct)(&acpi->mInstanceStorage);
      free(self);
      return noErr;
    }

    static AudioComponentPlugInInterface *Factory(const AudioComponentDescription* inDesc)
    {
      AudioComponentPlugInInstance *acpi = (AudioComponentPlugInInstance *) malloc( offsetof(AudioComponentPlugInInstance, mInstanceStorage) + sizeof(PLUG_CLASS_NAME) );
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
#endif
  extern "C"
  {
    //Component Manager
    EXPORT ComponentResult PLUG_ENTRY(ComponentParameters* params, void* pPlug)
    {
      return IPlugAU::IPlugAUEntry(params, pPlug);
    }
    //Carbon view
    EXPORT ComponentResult PLUG_VIEW_ENTRY(ComponentParameters* params, void* pView)
    {
      return IPlugAU::IPlugAUCarbonViewEntry(params, pView);
    }
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
    //>10.7 SDK AUPlugin
    EXPORT void* PLUG_FACTORY(const AudioComponentDescription* inDesc)
    {
      return IPlugAUFactory::Factory (inDesc);
    }
#endif
  };
#elif defined AAX_API
  IPlug* MakePlug()
  {
    IPlugInstanceInfo instanceInfo;
    
    return new PLUG_CLASS_NAME(instanceInfo);
  }
#elif defined SA_API
  IPlug* MakePlug(void* pMidiOutput, unsigned short* pMidiOutChan)
  {
    IPlugInstanceInfo instanceInfo;

    #if defined OS_WIN
      instanceInfo.mRTMidiOut = (RtMidiOut*) pMidiOutput;
      instanceInfo.mMidiOutChan = pMidiOutChan;
    #elif defined OS_OSX
      instanceInfo.mRTMidiOut = (RtMidiOut*) pMidiOutput;
      instanceInfo.mMidiOutChan = pMidiOutChan;
      instanceInfo.mOSXBundleID.Set(BUNDLE_ID);
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
