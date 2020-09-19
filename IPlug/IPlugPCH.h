#pragma once

/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

/**
 * @file
 * @brief IPlug2 Precompiled headers
 */

#include "PAL/Platform.h"  // Needs to be first

#include <array>
#include <bitset>
#include <chrono>
#include <codecvt>
#include <cstring>
#include <functional>
#include <map>
#include <numeric>
#include <stack>
#include <string>
#include <vector>
#include <version>
//#include <algorithm>
//#include <atomic>
//#include <cassert>
//#include <cctype>
//#include <cmath>
//#include <complex>
//#include <cstdarg>
//#include <cstdio>
//#include <cstdlib>
//#include <ctime>
//#include <iostream>
//#include <limits>
//#include <list>
//#include <locale>
//#include <memory>
//#include <set>
//#include <unordered_map>
//#include <utility>

#if __cpp_lib_bit_cast
	#include <bit>
#endif

#undef __alignas_is_defined  // make sure alignas isn't defined as a macro

BEGIN_INCLUDE_DEPENDENCIES
#define WDL_NO_SUPPORT_UTF8
#include <WDL/dirscan.h>
#include <WDL/heapbuf.h>
#include <WDL/jnetlib/jnetlib.h>
#include <WDL/mutex.h>
#include <WDL/ptrlist.h>
#include <WDL/wdl_base64.h>
#include <WDL/wdlendian.h>
#include <WDL/wdlstring.h>
#include <WDL/wdlutf8.h>
#include <yoga/Yoga.h>

#if VST3_API || VST3C_API || VST3P_API
	#include <pluginterfaces/base/ibstream.h>
	#include <pluginterfaces/base/keycodes.h>
	#include <pluginterfaces/base/ustring.h>
	#include "pluginterfaces/vst/ivstcomponent.h"
	#include "pluginterfaces/vst/ivsteditcontroller.h"
	#include "pluginterfaces/vst/ivstmidicontrollers.h"
	#include <pluginterfaces/vst/ivstchannelcontextinfo.h>
	#include <pluginterfaces/vst/ivstcontextmenu.h>
	#include <pluginterfaces/vst/ivstevents.h>
	#include <pluginterfaces/vst/ivstparameterchanges.h>
	#include <pluginterfaces/vst/ivstprocesscontext.h>
	#include <pluginterfaces/vst/vstspeaker.h>
	#include <pluginterfaces/vst/vsttypes.h>
	#include <pluginterfaces/gui/iplugviewcontentscalesupport.h>
	#include <public.sdk/source/vst/vstaudioeffect.h>
	#include <public.sdk/source/vst/vstbus.h>
	#include <public.sdk/source/vst/vsteditcontroller.h>
	#include <public.sdk/source/vst/vsteventshelper.h>
	#include <public.sdk/source/vst/vstparameters.h>
	#include <public.sdk/source/vst/vstsinglecomponenteffect.h>
	#include <public.sdk/source/vst/hosting/parameterchanges.h>
	#include "public.sdk/source/main/pluginfactory.h"
#endif
END_INCLUDE_DEPENDENCIES

#include "IPlugConstants.h"
#include "IPlugLogger.h"
#include "IPlugMath.h"
#include "Extras/Easing.h"
#include "IPlugStructs.h"
#include "IPlugUtilities.h"
#include "IPlugMidi.h"
#include "IPlugParameter.h"
#include "IPlugQueue.h"
#include "IPlugPaths.h"
#include "IPlugTimer.h"
#include "IPlugPluginBase.h"
#include "IPlugAPIBase.h"
#include "IPlugEditorDelegate.h"
#include "IPlugProcessor.h"
#include "ISender.h"
#ifndef NO_IGRAPHICS
	#include "IGraphicsConstants.h"
	#include "IGraphicsStructs.h"
	#include "IGraphicsPopupMenu.h"
	#include "IGraphicsUtilities.h"
	#include "IGraphics.h"
	#include "IGraphics_select.h"
	#include "IControl.h"
	#include "IControls.h"
	#include "IBubbleControl.h"
	#include "IColorPickerControl.h"
	#include "ICornerResizerControl.h"
	#include "IFPSDisplayControl.h"
	#include "IGraphicsLiveEdit.h"
	#include "ILEDControl.h"
	#include "IPopupMenuControl.h"
	#include "IRTTextControl.h"
	#include "ITextEntryControl.h"
	#include "IVDisplayControl.h"
	#include "IVKeyboardControl.h"
	#include "IVMeterControl.h"
	#include "IVMultiSliderControl.h"
	#include "IVScopeControl.h"
#endif


//-----------------------------------------------------------------------------
// The following is used when we're *not* compiling the static library
// previously known as 'IPlug_include_in_plug_hdr.h'

// clang-format off

#ifndef IPLUG2_STATIC
	#include "config.h"

	#define API_EXT2
	#ifdef VST2_API
		#ifdef REAPER_PLUGIN
			#define LICE_PROVIDED_BY_APP
			//  #define SWELL_PROVIDED_BY_APP
			#include "VST2/IPlugReaperVST2.h"
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
			#include "VST2/IPlugVST2.h"
		using Plugin = iplug::IPlugVST2;
		#endif
		#define API_EXT "vst"
	#elif defined AU_API
		#include "AUv2/IPlugAU.h"
		using Plugin = iplug::IPlugAU;
		#define API_EXT "audiounit"
	#elif defined AUv3_API
		#include "AUv3/IPlugAUv3.h"
		using Plugin = iplug::IPlugAUv3;
		#define API_EXT "app"
		#undef API_EXT2
		#define API_EXT2 ".AUv3"
	#elif defined AAX_API
		#include "AAX/IPlugAAX.h"
		using Plugin = iplug::IPlugAAX;
		#define API_EXT "aax"
		#define PROTOOLS
	#elif defined APP_API
		#include "APP/IPlugAPP.h"
		using Plugin = iplug::IPlugAPP;
		#define API_EXT "app"
	#elif defined WAM_API
		#include "WEB/IPlugWAM.h"
		using Plugin = iplug::IPlugWAM;
	#elif defined WEB_API
		#include "WEB/IPlugWeb.h"
		using Plugin = iplug::IPlugWeb;
	#elif defined VST3_API
		#define IPLUG_VST3
		#include "VST3/IPlugVST3.h"
		using Plugin = iplug::IPlugVST3;
		#define API_EXT "vst3"
	#elif defined VST3C_API
		#define IPLUG_VST3
		#include "VST3/IPlugVST3_Controller.h"
		using Plugin = iplug::IPlugVST3Controller;
		#undef PLUG_CLASS_NAME
		#define PLUG_CLASS_NAME VST3Controller
		#define API_EXT "vst3"
	#elif defined VST3P_API
		#define IPLUG_VST3
		#include "VST3/IPlugVST3_Processor.h"
		using Plugin = iplug::IPlugVST3Processor;
		#define API_EXT "vst3"
	#else
		#error "No API defined!"
	#endif

	#if PLATFORM_WINDOWS || PLATFORM_WEB
		#define BUNDLE_ID ""
	#elif PLATFORM_MAC
		#define BUNDLE_ID BUNDLE_DOMAIN "." BUNDLE_MFR "." API_EXT "." BUNDLE_NAME API_EXT2
	#elif PLATFORM_IOS
		#define BUNDLE_ID BUNDLE_DOMAIN "." BUNDLE_MFR "." BUNDLE_NAME API_EXT2
	#elif PLATFORM_LINUX
		//TODO:
	#else
		#error "No OS defined!"
	#endif

	#if !defined NO_IGRAPHICS && !defined VST3P_API
		#include "IGraphics_include_in_plug_hdr.h"
	#endif

	#ifndef PLUG_NAME
		#error You need to define PLUG_NAME in config.h - The name of your plug-in, with no spaces
	#endif

	#ifndef PLUG_MFR
		#error You need to define PLUG_MFR in config.h - The manufacturer name
	#endif

	#ifndef PLUG_TYPE
		#error You need to define PLUG_TYPE in config.h. Effect, Instrument or MIDIEffect
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
		PRAGMA_MESSAGE("PLUG_DOES_MIDI_IN not defined, setting to 0")
		#define PLUG_DOES_MIDI_IN 0
	#endif

	#ifndef PLUG_DOES_MIDI_OUT
		PRAGMA_MESSAGE("PLUG_DOES_MIDI_OUT not defined, setting to 0")
		#define PLUG_DOES_MIDI_OUT 0
	#endif

	#ifndef PLUG_DOES_MPE
		PRAGMA_MESSAGE("PLUG_DOES_MPE not defined, setting to 0")
		#define PLUG_DOES_MPE 0
	#endif

	#ifndef PLUG_DOES_STATE_CHUNKS
		PRAGMA_MESSAGE("PLUG_DOES_STATE_CHUNKS not defined, setting to 0")
		#define PLUG_DOES_STATE_CHUNKS 0
	#endif

	#ifndef PLUG_HAS_UI
		PRAGMA_MESSAGE("PLUG_HAS_UI not defined, setting to 0")
		#define PLUG_HAS_UI 0
	#endif

	#ifndef PLUG_WIDTH
		PRAGMA_MESSAGE("PLUG_WIDTH not defined, setting to 500px")
		#define PLUG_WIDTH 500
	#endif

	#ifndef PLUG_HEIGHT
		PRAGMA_MESSAGE("PLUG_HEIGHT not defined, setting to 500px")
		#define PLUG_HEIGHT 500
	#endif

	// TODO: Remove arbitrary size limits. Let people with their 8K or 16K screens be able to see without magnifying glass
	#ifndef PLUG_MIN_WIDTH
		#define PLUG_MIN_WIDTH (PLUG_WIDTH / 2)
	#endif

	#ifndef PLUG_MIN_HEIGHT
		#define PLUG_MIN_HEIGHT (PLUG_HEIGHT / 2)
	#endif

	#ifndef PLUG_MAX_WIDTH
		#define PLUG_MAX_WIDTH (PLUG_WIDTH * 2)
	#endif

	#ifndef PLUG_MAX_HEIGHT
		#define PLUG_MAX_HEIGHT (PLUG_HEIGHT * 2)
	#endif

	// TODO: FPS should be read from client hardware and then have a FPS limit option client side
	#ifndef PLUG_FPS
		PRAGMA_MESSAGE("PLUG_FPS not defined, setting to 60")
		#define PLUG_FPS 60
	#endif

	#ifndef PLUG_SHARED_RESOURCES
		PRAGMA_MESSAGE("PLUG_SHARED_RESOURCES not defined, setting to 0")
		#define PLUG_SHARED_RESOURCES 0
	#else
		#ifndef SHARED_RESOURCES_SUBPATH
		PRAGMA_MESSAGE("SHARED_RESOURCES_SUBPATH not defined, setting to PLUG_NAME")
			#define SHARED_RESOURCES_SUBPATH PLUG_NAME
		#endif
	#endif

	#ifdef IPLUG_VST3
		#ifndef PLUG_VERSION_STR
			#error You need to define PLUG_VERSION_STR in config.h - A string to identify the version number
		#endif

		#ifndef PLUG_URL_STR
			PRAGMA_MESSAGE("PLUG_URL_STR not defined, setting to empty string")
			#define PLUG_URL_STR ""
		#endif

		#ifndef PLUG_EMAIL_STR
			PRAGMA_MESSAGE("PLUG_EMAIL_STR not defined, setting to empty string")
			#define PLUG_EMAIL_STR ""
		#endif

		#ifndef PLUG_COPYRIGHT_STR
			PRAGMA_MESSAGE("PLUG_COPYRIGHT_STR not defined, setting to empty string")
			#define PLUG_COPYRIGHT_STR ""
		#endif

		#ifndef VST3_SUBCATEGORY
			PRAGMA_MESSAGE("VST3_SUBCATEGORY not defined, setting to other")
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
#endif
// clang-format on
