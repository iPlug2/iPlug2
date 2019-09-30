/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file IPlug_include_in_plug_hdr.h
 * @brief IPlug header include
 * Include this file in the main header for your plugin
 * A preprocessor macro for a particular API such as VST2_API should be defined at project level
*/

#include <cstdio>
#include "IPlugPlatform.h"
#include "config.h"

#define API_EXT2
#ifdef VST2_API
#ifdef REAPER_PLUGIN
  #define LICE_PROVIDED_BY_APP
//  #define SWELL_PROVIDED_BY_APP
  #include "IPlugReaperVST2.h"
  #define PLUGIN_API_BASE IPlugReaperVST2

  #ifdef FillRect
  #undef FillRect
  #endif
  #ifdef DrawText
  #undef DrawText
  #endif
  #ifdef Polygon
  #undef Polygon
  #endif

#else
  #include "IPlugVST2.h"
  #define PLUGIN_API_BASE IPlugVST2
#endif
  #define API_EXT "vst"
#elif defined AU_API
  #include "IPlugAU.h"
  #define PLUGIN_API_BASE IPlugAU
  #define API_EXT "audiounit"
#elif defined AUv3_API
  #include "IPlugAUv3.h"
  #define PLUGIN_API_BASE IPlugAUv3
  #define API_EXT "app"
  #undef API_EXT2
  #define API_EXT2 ".AUv3"
#elif defined AAX_API
  #include "IPlugAAX.h"
  #define PLUGIN_API_BASE IPlugAAX
  #define API_EXT "aax"
  #define PROTOOLS
#elif defined APP_API
  #include "IPlugAPP.h"
  #define PLUGIN_API_BASE IPlugAPP
  #define API_EXT "app"
#elif defined WAM_API
  #include "IPlugWAM.h"
  #define PLUGIN_API_BASE IPlugWAM
#elif defined WEB_API
  #include "IPlugWeb.h"
  #define PLUGIN_API_BASE IPlugWeb
#elif defined VST3_API
  #define IPLUG_VST3
  #include "IPlugVST3.h"
  #define PLUGIN_API_BASE IPlugVST3
  #define API_EXT "vst3"
#elif defined VST3C_API
  #define IPLUG_VST3
  #include "IPlugVST3_Controller.h"
  #define PLUGIN_API_BASE IPlugVST3Controller
  #undef PLUG_CLASS_NAME
  #define PLUG_CLASS_NAME VST3Controller
  #define API_EXT "vst3"
#elif defined VST3P_API
  #define IPLUG_VST3
  #include "IPlugVST3_Processor.h"
  #define PLUGIN_API_BASE IPlugVST3Processor
  #define API_EXT "vst3"
#else
  #error "No API defined!"
#endif

BEGIN_IPLUG_NAMESPACE
using Plugin = PLUGIN_API_BASE;
END_IPLUG_NAMESPACE

#ifdef OS_WIN
  #define EXPORT __declspec(dllexport)
  #define BUNDLE_ID ""
#elif defined OS_MAC || defined OS_IOS
  #define BUNDLE_ID BUNDLE_DOMAIN "." BUNDLE_MFR "." API_EXT "." BUNDLE_NAME API_EXT2
  #define EXPORT __attribute__ ((visibility("default")))
#elif defined OS_LINUX
  //TODO:
#elif defined OS_WEB
  #define BUNDLE_ID ""
#else
  #error "No OS defined!"
#endif

#if !defined NO_IGRAPHICS && !defined VST3P_API
#include "IGraphics_include_in_plug_hdr.h"
#endif

#define STRINGISE_IMPL(x) #x
#define STRINGISE(x) STRINGISE_IMPL(x)

// Use: #pragma message WARN("My message")
#if _MSC_VER
#   define FILE_LINE_LINK __FILE__ "(" STRINGISE(__LINE__) ") : "
#   define WARN(exp) (FILE_LINE_LINK "WARNING: " exp)
#else//__GNUC__ - may need other defines for different compilers
#   define WARN(exp) ("WARNING: " exp)
#endif

#ifndef PLUG_NAME
  #error You need to define PLUG_NAME in config.h - The name of your plug-in, with no spaces
#endif

#ifndef PLUG_MFR
  #error You need to define PLUG_MFR in config.h - The manufacturer name
#endif

#ifndef PLUG_TYPE
  #error You need to define PLUG_TYPE in config.h. 0 = Effect, 1 = Instrument, 2 = MIDI Effect
#endif

#ifndef PLUG_VERSION_HEX
  #error You need to define PLUG_VERSION_HEX in config.h - The hexadecimal version number in the form 0xVVVVRRMM: V = version, R = revision, M = minor revision.
#endif

#ifndef PLUG_UNIQUE_ID
  #error You need to define PLUG_UNIQUE_ID in config.h - The unique four char ID for your plug-in, e.g. 'IPeF'
#endif

#ifndef PLUG_MFR_ID
  #error You need to define PLUG_MFR_ID in config.h - The unique four char ID for your manufacturer, e.g. 'Acme'
#endif

#ifndef PLUG_CLASS_NAME
  #error PLUG_CLASS_NAME not defined - this is the name of your main iPlug plug-in class (no spaces allowed)
#endif

#ifndef BUNDLE_NAME
  #error BUNDLE_NAME not defined - this is the product name part of the plug-in's bundle ID (used on macOS and iOS)
#endif

#ifndef BUNDLE_MFR
  #error BUNDLE_MFR not defined - this is the manufacturer name part of the plug-in's bundle ID (used on macOS and iOS)
#endif

#ifndef BUNDLE_DOMAIN
  #error BUNDLE_DOMAIN not defined - this is the domain name part of the plug-in's bundle ID (used on macOS and iOS)
#endif

#ifndef PLUG_CHANNEL_IO
  #error PLUG_CHANNEL_IO not defined - you need to specify the input and output configurations of the plug-in e.g. "2-2"
#endif

#ifndef PLUG_LATENCY
  #define PLUG_LATENCY 0
#endif

#ifndef PLUG_DOES_MIDI_IN
  #pragma message WARN("PLUG_DOES_MIDI_IN not defined, setting to 0")
  #define PLUG_DOES_MIDI_IN 0
#endif

#ifndef PLUG_DOES_MIDI_OUT
  #pragma message WARN("PLUG_DOES_MIDI_OUT not defined, setting to 0")
  #define PLUG_DOES_MIDI_OUT 0
#endif

#ifndef PLUG_DOES_MPE
  #pragma message WARN("PLUG_DOES_MPE not defined, setting to 0")
  #define PLUG_DOES_MPE 0
#endif

#ifndef PLUG_DOES_STATE_CHUNKS
  #pragma message WARN("PLUG_DOES_STATE_CHUNKS not defined, setting to 0")
  #define PLUG_DOES_STATE_CHUNKS 0
#endif

#ifndef PLUG_HAS_UI
  #pragma message WARN("PLUG_HAS_UI not defined, setting to 0")
  #define PLUG_HAS_UI 0
#endif

#ifndef PLUG_WIDTH
  #pragma message WARN("PLUG_WIDTH not defined, setting to 500px")
  #define PLUG_WIDTH 500
#endif

#ifndef PLUG_HEIGHT
  #pragma message WARN("PLUG_HEIGHT not defined, setting to 500px")
  #define PLUG_HEIGHT 500
#endif

#ifndef PLUG_FPS
  #pragma message WARN("PLUG_FPS not defined, setting to 60")
  #define PLUG_FPS 60
#endif

#ifndef PLUG_SHARED_RESOURCES
  #pragma message WARN("PLUG_SHARED_RESOURCES not defined, setting to 0")
  #define PLUG_SHARED_RESOURCES 0
#else
  #ifndef SHARED_RESOURCES_SUBPATH
    #pragma message WARN("SHARED_RESOURCES_SUBPATH not defined, setting to PLUG_NAME")
    #define SHARED_RESOURCES_SUBPATH PLUG_NAME
  #endif
#endif

#ifdef IPLUG_VST3
  #ifndef PLUG_VERSION_STR
    #error You need to define PLUG_VERSION_STR in config.h - A string to identify the version number
  #endif

  #ifndef PLUG_URL_STR
    #pragma message WARN("PLUG_URL_STR not defined, setting to empty string")
    #define PLUG_URL_STR ""
  #endif

  #ifndef PLUG_EMAIL_STR
    #pragma message WARN("PLUG_EMAIL_STR not defined, setting to empty string")
    #define PLUG_EMAIL_STR ""
  #endif

  #ifndef PLUG_COPYRIGHT_STR
    #pragma message WARN("PLUG_COPYRIGHT_STR not defined, setting to empty string")
    #define PLUG_COPYRIGHT_STR ""
  #endif

  #ifndef VST3_SUBCATEGORY
    #pragma message WARN("VST3_SUBCATEGORY not defined, setting to other")
    #define VST3_SUBCATEGORY "Other"
  #endif
#endif

#ifdef AU_API
  #ifndef AUV2_ENTRY
    #error AUV2_ENTRY not defined - the name of the entry point for a component manager AUv2 plug-in, without quotes
  #endif
  #ifndef AUV2_ENTRY_STR
    #error AUV2_ENTRY_STR not defined - the name of the entry point for a component manager AUv2 plug-in, with quotes
  #endif
  #ifndef AUV2_FACTORY
    #error AUV2_FACTORY not defined - the name of the entry point for a AUPlugIn AUv2 plug-in, without quotes
  #endif
  #if PLUG_HAS_UI
    #ifndef AUV2_VIEW_CLASS
      #error AUV2_VIEW_CLASS not defined - the name of the Objective-C class for the AUv2 plug-in's view, without quotes
    #endif
    #ifndef AUV2_VIEW_CLASS_STR
      #error AUV2_VIEW_CLASS_STR not defined - the name of the Objective-C class for the AUv2 plug-in's view,  with quotes
    #endif
  #endif
#endif

#ifdef AAX_API
  #ifndef AAX_TYPE_IDS
    #error AAX_TYPE_IDS not defined - list of comma separated four char IDs, that correspond to the different possible channel layouts of your plug-in, e.g. 'EFN1', 'EFN2'
  #endif

  #ifndef AAX_PLUG_MFR_STR
    #error AAX_PLUG_MFR_STR not defined - The manufacturer name as it will appear in Pro tools preset manager
  #endif

  #ifndef AAX_PLUG_NAME_STR
    #error AAX_PLUG_NAME_STR not defined - The plug-in name string, which may include shorten names separated with newline characters, e.g. "IPlugEffect\nIPEF"
  #endif

  #ifndef AAX_PLUG_CATEGORY_STR
    #error AAX_PLUG_CATEGORY_STR not defined - String defining the category for your plug-in, e.g. "Effect"
  #endif

  #if AAX_DOES_AUDIOSUITE
    #ifndef AAX_TYPE_IDS_AUDIOSUITE
      #error AAX_TYPE_IDS_AUDIOSUITE not defined - list of comma separated four char IDs, that correspond to the different possible channel layouts of your plug-in when running off-line in audio suite mode, e.g. 'EFA1', 'EFA2'
    #endif
  #endif
#endif
