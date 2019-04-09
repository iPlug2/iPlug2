/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file IPlug_include_in_plug_src.h
 * @brief IPlug source include
 * Include this file in the main source for your plugin, after #including the main header for your plugin.
 * A preprocessor macro for a particular API such as VST2_API should be defined at project level
 * Depending on the API macro defined, a different entry point and helper methods are activated
*/

#pragma mark - VST2
#if defined OS_WIN
  HINSTANCE gHINSTANCE = 0;
  #if defined(VST2_API) || defined(AAX_API)
  #ifdef __MINGW32__
  extern "C"
  #endif
  BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID res)
  {
    gHINSTANCE = hDllInst;
    return TRUE;
  }
  #endif
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
        AEffect& aEffect = pPlug->GetAEffect();
        pPlug->EnsureDefaultPreset();
        aEffect.numPrograms = std::max(aEffect.numPrograms, 1); // some hosts don't like 0 presets
        return &(aEffect);
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

#pragma mark - VST3
#elif defined VST3_API || VST3C_API || defined VST3P_API
#include "public.sdk/source/main/pluginfactory.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"

static unsigned int PROC_GUID_DATA1 = 0xF2AEE70D;
static unsigned int PROC_GUID_DATA2 = 0x00DE4F4E;
static unsigned int CTRL_GUID_DATA1 = 0xF2AEE70E;
static unsigned int CTRL_GUID_DATA2 = 0x00DE4F4F;
static unsigned int GUID_DATA3 = PLUG_MFR_ID;
static unsigned int GUID_DATA4 = PLUG_UNIQUE_ID;

#ifndef EFFECT_TYPE_VST3
  #if PLUG_TYPE == 1
    #define EFFECT_TYPE_VST3 kInstrumentSynth
  #else
    #define EFFECT_TYPE_VST3 kFx
  #endif
#endif

#if defined VST3P_API || defined VST3_API
bool InitModule()
{
#ifdef OS_WIN
  extern void* moduleHandle;
  gHINSTANCE = (HINSTANCE) moduleHandle;
#endif
  return true;
}

// called after library is unloaded
bool DeinitModule()
{
  return true;
}
#endif

#if defined VST3P_API
IPlug* MakeProcessor()
{
  static WDL_Mutex sMutex;
  WDL_MutexLock lock(&sMutex);
  IPlugVST3Processor::IPlugInstanceInfo instanceInfo;
  instanceInfo.mOtherGUID = FUID(CTRL_GUID_DATA1, CTRL_GUID_DATA2, GUID_DATA3, GUID_DATA4);
  return new PLUG_CLASS_NAME(instanceInfo);
}

static Steinberg::FUnknown* createProcessorInstance (void*) {
  return (Steinberg::Vst::IAudioProcessor*) MakeProcessor();
}

extern IPlug* MakeController();

static Steinberg::FUnknown* createControllerInstance (void*) {
  return (Steinberg::Vst::IEditController*) MakeController();
}

#elif defined VST3C_API
IPlug* MakeController()
{
  static WDL_Mutex sMutex;
  WDL_MutexLock lock(&sMutex);
  IPlugVST3Controller::IPlugInstanceInfo instanceInfo;
  instanceInfo.mOtherGUID = FUID(PROC_GUID_DATA1, PROC_GUID_DATA2, GUID_DATA3, GUID_DATA4);
  
  //If you are trying to build a distributed VST3 plug-in and you hit an error here "no matching constructor...",
  //you need to replace all instances of PLUG_CLASS_NAME in your plug-in class, with the macro PLUG_CLASS_NAME
  return new PLUG_CLASS_NAME(instanceInfo);
}
#elif defined VST3_API
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

BEGIN_FACTORY_DEF (PLUG_MFR, PLUG_URL_STR, PLUG_EMAIL_STR)

DEF_CLASS2 (INLINE_UID(PROC_GUID_DATA1, PROC_GUID_DATA2, GUID_DATA3, GUID_DATA4),
            Steinberg::PClassInfo::kManyInstances,              // cardinality
            kVstAudioEffectClass,                               // the component category (don't change this)
            PLUG_NAME,                                          // plug-in name
            Steinberg::Vst::kSimpleModeSupported,               // kSimpleModeSupported because we can't split the gui and plugin
            VST3_SUBCATEGORY,                                   // Subcategory for this plug-in
            PLUG_VERSION_STR,                                       // plug-in version
            kVstVersionString,                                  // the VST 3 SDK version (dont changed this, use always this define)
            createInstance)                                     // function pointer called when this component should be instantiated

END_FACTORY
#endif

#if defined VST3P_API
BEGIN_FACTORY_DEF (PLUG_MFR, PLUG_URL_STR, PLUG_EMAIL_STR)

DEF_CLASS2 (INLINE_UID(PROC_GUID_DATA1, PROC_GUID_DATA2, GUID_DATA3, GUID_DATA4),
            PClassInfo::kManyInstances,  // cardinality
            kVstAudioEffectClass,  // the component category (do not changed this)
            PLUG_NAME,    // here the Plug-in name (to be changed)
            Vst::kDistributable,  // means that component and controller could be distributed on different computers
            VST3_SUBCATEGORY,    // Subcategory for this Plug-in (to be changed)
            PLUG_VERSION_STR,    // Plug-in version (to be changed)
            kVstVersionString,    // the VST 3 SDK version (do not changed this, use always this define)
            createProcessorInstance)  // function pointer called when this component should be instantiated
DEF_CLASS2 (INLINE_UID(CTRL_GUID_DATA1, CTRL_GUID_DATA2, GUID_DATA3, GUID_DATA4),
            PClassInfo::kManyInstances,  // cardinality
            kVstComponentControllerClass,// the Controller category (do not changed this)
            PLUG_NAME " Controller",  // controller name (could be the same than component name)
            0,            // not used here
            "",            // not used here
            PLUG_VERSION_STR,    // Plug-in version (to be changed)
            kVstVersionString,    // the VST 3 SDK version (do not changed this, use always this define)
            createControllerInstance)// function pointer called when this component should be instantiated

END_FACTORY
#endif
#pragma mark - AUv2
#elif defined AU_API
//#include <AvailabilityMacros.h>
  IPlug* MakePlug(void* pMemory)
  {
    IPlugInstanceInfo instanceInfo;
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

    static AudioComponentMethod Lookup(SInt16 selector)
    {
      switch (selector) {
        case kAudioUnitInitializeSelect: return (AudioComponentMethod)IPlugAU::AUMethodInitialize;
        case kAudioUnitUninitializeSelect: return (AudioComponentMethod)IPlugAU::AUMethodUninitialize;
        case kAudioUnitGetPropertyInfoSelect: return (AudioComponentMethod)IPlugAU::AUMethodGetPropertyInfo;
        case kAudioUnitGetPropertySelect: return (AudioComponentMethod)IPlugAU::AUMethodGetProperty;
        case kAudioUnitSetPropertySelect: return (AudioComponentMethod)IPlugAU::AUMethodSetProperty;
        case kAudioUnitAddPropertyListenerSelect:return (AudioComponentMethod)IPlugAU::AUMethodAddPropertyListener;
        case kAudioUnitRemovePropertyListenerSelect:  return (AudioComponentMethod)IPlugAU::AUMethodRemovePropertyListener;
        case kAudioUnitRemovePropertyListenerWithUserDataSelect:  return (AudioComponentMethod)IPlugAU::AUMethodRemovePropertyListenerWithUserData;
        case kAudioUnitAddRenderNotifySelect: return (AudioComponentMethod)IPlugAU::AUMethodAddRenderNotify;
        case kAudioUnitRemoveRenderNotifySelect:return (AudioComponentMethod)IPlugAU::AUMethodRemoveRenderNotify;
        case kAudioUnitGetParameterSelect: return (AudioComponentMethod)IPlugAU::AUMethodGetParameter;
        case kAudioUnitSetParameterSelect: return (AudioComponentMethod)IPlugAU::AUMethodSetParameter;
        case kAudioUnitScheduleParametersSelect:return (AudioComponentMethod)IPlugAU::AUMethodScheduleParameters;
        case kAudioUnitRenderSelect: return (AudioComponentMethod)IPlugAU::AUMethodRender;
        case kAudioUnitResetSelect: return (AudioComponentMethod)IPlugAU::AUMethodReset;
#if PLUG_DOES_MIDI_IN
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
#pragma mark - AUv3
#elif defined AUv3_API
  IPlug* MakePlug()
  {
    IPlugInstanceInfo instanceInfo;
    return new PLUG_CLASS_NAME(instanceInfo);
  }
#pragma mark - AAX
#elif defined AAX_API
  IPlug* MakePlug()
  {
    IPlugInstanceInfo instanceInfo;

    return new PLUG_CLASS_NAME(instanceInfo);
  }
#pragma mark - APP
#elif defined APP_API
  IPlug* MakePlug(void* pAppHost)
  {
    IPlugInstanceInfo instanceInfo;
    instanceInfo.pAppHost = pAppHost;
    
    return new PLUG_CLASS_NAME(instanceInfo);
  }
#pragma mark - WAM
#elif defined WAM_API
  IPlug* MakePlug()
  {
    IPlugInstanceInfo instanceInfo;
    return new PLUG_CLASS_NAME(instanceInfo);
  }

  extern "C"
  {
    EMSCRIPTEN_KEEPALIVE void* createModule()
    {
      Processor* pWAM = dynamic_cast<Processor*>(MakePlug());
      return (void*) pWAM;
    }
  }
#pragma mark - WEB
#elif defined WEB_API
  #include "config.h"

  IPlug* MakePlug()
  {
    IPlugInstanceInfo instanceInfo;
    return new PLUG_CLASS_NAME(instanceInfo);
  }

  IPlugWeb* gPlug = nullptr;
  extern void StartMainLoopTimer();

  extern "C"
  {
    EMSCRIPTEN_KEEPALIVE void iplug_syncfs()
    {
      EM_ASM({
        if(Module.syncdone == 1) {
          Module.syncdone = 0;
          FS.syncfs(false, function (err) {
            assert(!err);
            console.log("Synced to IDBFS...");
            Module.syncdone = 1;
          });
        }
      });
    }
    
    EMSCRIPTEN_KEEPALIVE void iplug_fsready()
    {
      gPlug = MakePlug();
      gPlug->SetHost("www", 0);
      gPlug->OpenWindow(nullptr);
      gPlug->OnUIOpen();
      iplug_syncfs(); // plug in may initialise settings in constructor, write to persistent data after init
    }
  }

  int main()
  {
    //create persistent data file system and synchronise
    EM_ASM(
           var name = '/' + UTF8ToString($0) + '_data';
           FS.mkdir(name);
           FS.mount(IDBFS, {}, name);

           Module.syncdone = 0;
           FS.syncfs(true, function (err) {
            assert(!err);
            console.log("Synced from IDBFS...");
            Module.syncdone = 1;
            ccall('iplug_fsready', 'v');
          });
        , PLUG_NAME);
    
    StartMainLoopTimer();

    // TODO: when do we delete!
    // delete gPlug;
    
    return 0;
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

#define PUBLIC_NAME PLUG_NAME

#define IPLUG_CTOR(nParams, nPresets, instanceInfo) \
  IPlug(instanceInfo, IPlugConfig(nParams, nPresets, PLUG_CHANNEL_IO,\
    PUBLIC_NAME, "", PLUG_MFR, PLUG_VERSION_HEX, PLUG_UNIQUE_ID, PLUG_MFR_ID, \
    PLUG_LATENCY, PLUG_DOES_MIDI_IN, PLUG_DOES_MIDI_OUT, PLUG_DOES_MPE, PLUG_DOES_STATE_CHUNKS, PLUG_TYPE, \
    PLUG_HAS_UI, PLUG_WIDTH, PLUG_HEIGHT, BUNDLE_ID))

#if !defined NO_IGRAPHICS && !defined VST3P_API
#include "IGraphics_include_in_plug_src.h"
#endif
