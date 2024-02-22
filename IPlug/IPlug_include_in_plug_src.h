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

#pragma mark - OS_WIN

// clang-format off

#if defined OS_WIN && !defined VST3C_API
  HINSTANCE gHINSTANCE = 0;
  #if defined(VST2_API) || defined(AAX_API)
  #ifdef __MINGW32__
  extern "C"
  #endif
  BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID res)
  {
    gHINSTANCE = hDllInst;
    return true;
  }
  #endif

  UINT(WINAPI *__GetDpiForWindow)(HWND);

  float GetScaleForHWND(HWND hWnd)
  {
    if (!__GetDpiForWindow)
    {
      HINSTANCE h = LoadLibraryW(L"user32.dll");
      if (h) *(void **)&__GetDpiForWindow = GetProcAddress(h, "GetDpiForWindow");

      if (!__GetDpiForWindow)
        return 1;
    }

    int dpi = __GetDpiForWindow(hWnd);

    if (dpi != USER_DEFAULT_SCREEN_DPI)
    {
#if defined IGRAPHICS_QUANTISE_SCREENSCALE
      return std::round(static_cast<float>(dpi) / USER_DEFAULT_SCREEN_DPI);
#else
      return static_cast<float>(dpi) / USER_DEFAULT_SCREEN_DPI;
#endif
    }

    return 1;
  }

#endif

#pragma mark - ** Global Functions and Defines **

#pragma mark - VST2
#if defined VST2_API
  extern "C"
  {
    EXPORT void* VSTPluginMain(audioMasterCallback hostCallback)
    {
      using namespace iplug;

      IPlugVST2* pPlug = iplug::MakePlug(iplug::InstanceInfo{hostCallback});

      if (pPlug)
      {
        AEffect& aEffect = pPlug->GetAEffect();
        pPlug->EnsureDefaultPreset();
        aEffect.numPrograms = std::max(aEffect.numPrograms, 1); // some hosts don't like 0 presets
        return &aEffect;
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
#pragma mark - VST3 (All)
#elif defined VST3_API || VST3C_API || defined VST3P_API
  #include "public.sdk/source/main/pluginfactory.h"
  #include "pluginterfaces/vst/ivstcomponent.h"
  #include "pluginterfaces/vst/ivsteditcontroller.h"

#if !defined VST3_PROCESSOR_UID && !defined VST3_CONTROLLER_UID
#define VST3_PROCESSOR_UID 0xF2AEE70D, 0x00DE4F4E, PLUG_MFR_ID, PLUG_UNIQUE_ID
#define VST3_CONTROLLER_UID 0xF2AEE70E, 0x00DE4F4F, PLUG_MFR_ID, PLUG_UNIQUE_ID
#endif

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
  #pragma mark - VST3
  #if defined VST3_API
  static Steinberg::FUnknown* createInstance(void*)
  {
    return (Steinberg::Vst::IAudioProcessor*) iplug::MakePlug(iplug::InstanceInfo());
  }

  BEGIN_FACTORY_DEF(PLUG_MFR, PLUG_URL_STR, PLUG_EMAIL_STR)

  DEF_CLASS2(INLINE_UID_FROM_FUID(FUID(VST3_PROCESSOR_UID)),
              Steinberg::PClassInfo::kManyInstances,          // cardinality
              kVstAudioEffectClass,                           // the component category (don't change this)
              PLUG_NAME,                                      // plug-in name
              Steinberg::Vst::kSimpleModeSupported,           // means gui and plugin aren't split
              VST3_SUBCATEGORY,                               // Subcategory for this plug-in
              PLUG_VERSION_STR,                               // plug-in version
              kVstVersionString,                              // the VST 3 SDK version (don't change - use define)
              createInstance)                                 // function pointer called to be instantiate

  END_FACTORY
  #pragma mark - VST3 Processor
  #elif defined VST3P_API
  static Steinberg::FUnknown* createProcessorInstance(void*)
  {
      return MakeProcessor();
  }

  static Steinberg::FUnknown* createControllerInstance(void*)
  {
    return MakeController();
  }

  BEGIN_FACTORY_DEF(PLUG_MFR, PLUG_URL_STR, PLUG_EMAIL_STR)

  DEF_CLASS2 (INLINE_UID_FROM_FUID(FUID(VST3_PROCESSOR_UID)),
              PClassInfo::kManyInstances,                     // cardinality
              kVstAudioEffectClass,                           // the component category (do not changed this)
              PLUG_NAME,                                      // here the Plug-in name (to be changed)
              Vst::kDistributable,                            // means component/controller can on different computers
              VST3_SUBCATEGORY,                               // Subcategory for this Plug-in (to be changed)
              PLUG_VERSION_STR,                               // Plug-in version (to be changed)
              kVstVersionString,                              // the VST 3 SDK version (don't change - use define)
              createProcessorInstance)                        // function pointer called to be instantiate

  DEF_CLASS2(INLINE_UID_FROM_FUID(FUID(VST3_CONTROLLER_UID)),
              PClassInfo::kManyInstances,                     // cardinality
              kVstComponentControllerClass,                   // the Controller category (do not changed this)
              PLUG_NAME " Controller",                        // controller name (could be the same than component name)
              0,                                              // not used here
              "",                                             // not used here
              PLUG_VERSION_STR,                               // Plug-in version (to be changed)
              kVstVersionString,                              // the VST 3 SDK version (don't change - use define)
              createControllerInstance)                       // function pointer called to be instantiate

  END_FACTORY
  #endif
#pragma mark - AUv2
#elif defined AU_API
  extern "C"
  {
    #ifndef AU_NO_COMPONENT_ENTRY
    //Component Manager
    EXPORT ComponentResult AUV2_ENTRY(ComponentParameters* pParams, void* pPlug)
    {
      return iplug::IPlugAU::IPlugAUEntry(pParams, pPlug);
    }
    #endif

    //>10.7 SDK AUPlugin
    EXPORT void* AUV2_FACTORY(const AudioComponentDescription* pInDesc)
    {
      return iplug::IPlugAUFactory<PLUG_CLASS_NAME, PLUG_DOES_MIDI_IN>::Factory(pInDesc);
    }
  };
#pragma mark - WAM
#elif defined WAM_API
  extern "C"
  {
    EMSCRIPTEN_KEEPALIVE void* createModule()
    {
      Processor* pWAM = dynamic_cast<Processor*>(iplug::MakePlug(iplug::InstanceInfo()));
      return (void*) pWAM;
    }
  }
#pragma mark - WEB
#elif defined WEB_API
#include <memory>
#include "config.h"
  std::unique_ptr<iplug::IPlugWeb> gPlug;
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
      gPlug = std::unique_ptr<iplug::IPlugWeb>(iplug::MakePlug(iplug::InstanceInfo()));
      gPlug->SetHost("www", 0);
      gPlug->OpenWindow(nullptr);
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

    // TODO: this code never runs, so when do we delete?!
    gPlug = nullptr;
    
    return 0;
  }
#elif defined AUv3_API || defined AAX_API || defined APP_API
// Nothing to do here
#else
  #error "No API defined!"
#endif

#pragma mark - ** Instantiation **

BEGIN_IPLUG_NAMESPACE

#pragma mark -
#pragma mark VST2, VST3, AAX, AUv3, APP, WAM, WEB

#if defined VST2_API || defined VST3_API || defined AAX_API || defined AUv3_API || defined APP_API  || defined WAM_API || defined WEB_API

Plugin* MakePlug(const iplug::InstanceInfo& info)
{
  // From VST3 - is this necessary?
  static WDL_Mutex sMutex;
  WDL_MutexLock lock(&sMutex);
  
  return new PLUG_CLASS_NAME(info);
}

#pragma mark - AUv2
#elif defined AU_API

Plugin* MakePlug(void* pMemory)
{
  iplug::InstanceInfo info;
  info.mCocoaViewFactoryClassName.Set(AUV2_VIEW_CLASS_STR);
    
  if (pMemory)
    return new(pMemory) PLUG_CLASS_NAME(info);
  else
    return new PLUG_CLASS_NAME(info);
}

#pragma mark - VST3 Controller
#elif defined VST3C_API

Steinberg::FUnknown* MakeController()
{
  static WDL_Mutex sMutex;
  WDL_MutexLock lock(&sMutex);
  iplug::IPlugVST3Controller::InstanceInfo info;
  info.mOtherGUID = Steinberg::FUID(VST3_PROCESSOR_UID);
  // If you are trying to build a distributed VST3 plug-in and you hit an error here like "no matching constructor..." or 
  // "error: unknown type name 'VST3Controller'", you need to replace all instances of the name of your plug-in class (e.g. IPlugEffect)
  // with the macro PLUG_CLASS_NAME, as defined in your plug-ins config.h, so IPlugEffect::IPlugEffect() {} becomes PLUG_CLASS_NAME::PLUG_CLASS_NAME().
  return static_cast<Steinberg::Vst::IEditController*>(new PLUG_CLASS_NAME(info));
}

#pragma mark - VST3 Processor
#elif defined VST3P_API

Steinberg::FUnknown* MakeProcessor()
{
  static WDL_Mutex sMutex;
  WDL_MutexLock lock(&sMutex);
  iplug::IPlugVST3Processor::InstanceInfo info;
  info.mOtherGUID = Steinberg::FUID(VST3_CONTROLLER_UID);
  return static_cast<Steinberg::Vst::IAudioProcessor*>(new PLUG_CLASS_NAME(info));
}

#else
#error "No API defined!"
#endif

#pragma mark - ** Config Utility **

static Config MakeConfig(int nParams, int nPresets)
{
  return Config(nParams, nPresets, PLUG_CHANNEL_IO, PLUG_NAME, PLUG_NAME, PLUG_MFR, PLUG_VERSION_HEX, PLUG_UNIQUE_ID, PLUG_MFR_ID, PLUG_LATENCY, PLUG_DOES_MIDI_IN, PLUG_DOES_MIDI_OUT, PLUG_DOES_MPE, PLUG_DOES_STATE_CHUNKS, PLUG_TYPE, PLUG_HAS_UI, PLUG_WIDTH, PLUG_HEIGHT, PLUG_HOST_RESIZE, PLUG_MIN_WIDTH, PLUG_MAX_WIDTH, PLUG_MIN_HEIGHT, PLUG_MAX_HEIGHT, BUNDLE_ID, APP_GROUP_ID); // TODO: Product Name?
}

END_IPLUG_NAMESPACE

/*
 #if defined _DEBUG
 #define PLUG_NAME APPEND_TIMESTAMP(PLUG_NAME " DEBUG")
 #elif defined TRACER_BUILD
 #define PLUG_NAME APPEND_TIMESTAMP(PLUG_NAME " TRACER")
 #elif defined TIMESTAMP_PLUG_NAME
 #pragma REMINDER("plug name is timestamped")
 #define PLUG_NAME APPEND_TIMESTAMP(PLUG_NAME)
 #else
 #define PLUG_NAME PLUG_NAME
 #endif
 */

#if !defined NO_IGRAPHICS && !defined VST3P_API
#include "IGraphics_include_in_plug_src.h"
#endif

// clang-format on
