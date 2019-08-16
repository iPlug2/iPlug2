/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief IPlug Constant definitions, Types, magic numbers
 * @defgroup IPlugConstants IPlug::Constants
 * IPlug Constant definitions, Types, magic numbers
 * @{
 */

#include <stdint.h>
#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

#if !defined(SAMPLE_TYPE_FLOAT) && !defined(SAMPLE_TYPE_DOUBLE)
#define SAMPLE_TYPE_DOUBLE
#endif

#ifdef SAMPLE_TYPE_DOUBLE
using PLUG_SAMPLE_DST = double;
using PLUG_SAMPLE_SRC = float;
#else
using PLUG_SAMPLE_DST = float;
using PLUG_SAMPLE_SRC = double;
#endif

using sample = PLUG_SAMPLE_DST;

#define LOGFILE "IPlugLog.txt"
#define MAX_PROCESS_TRACE_COUNT 100
#define MAX_IDLE_TRACE_COUNT 15

enum EIPlugPluginType
{
  kEffect = 0,
  kInstrument = 1,
  kMIDIEffect = 2
};

enum EVST3ParamIDs
{
#ifndef IPLUG1_COMPATIBILITY
  kBypassParam = 'bpas',
  kPresetParam = 'prst',
  kMIDICCParamStartIdx
#else
  kBypassParam = 65536,
  kPresetParam, // not used unless baked in presets declared
  kMIDICCParamStartIdx
#endif
};

//TODO: these should be in a namespace, to avoid conflicts with third-party libraries
static const double PI = 3.1415926535897932384626433832795;

/** @brief Magic number for gain to dB conversion.
 * Approximates \f$ 20*log_{10}(x) \f$
 * @see AmpToDB
*/
static const double AMP_DB = 8.685889638065036553;
/** @brief Magic number for dB to gain conversion.
 * Approximates \f$ 10^{\frac{x}{20}} \f$
 * @see DBToAmp
*/
static const double IAMP_DB = 0.11512925464970;
static const double DEFAULT_SAMPLE_RATE = 44100.0;
static const int MAX_PRESET_NAME_LEN = 256;
#define UNUSED_PRESET_NAME "empty"
#define DEFAULT_USER_PRESET_NAME "user preset"

#define AU_MAX_IO_CHANNELS 128

//TODO: check this shit really?
#define MAX_MACOS_PATH_LEN 1024
#define MAX_WIN32_PATH_LEN 256
#define MAX_WIN32_PARAM_LEN 256

#define MAX_PLUGIN_NAME_LEN 128

#define MAX_PARAM_NAME_LEN 32 // e.g. "Gain"
#define MAX_PARAM_LABEL_LEN 32 // e.g. "Percent"
#define MAX_PARAM_DISPLAY_LEN 32 // e.g. "100" / "Mute"
#define MAX_PARAM_GROUP_LEN 32 // e.g. "oscillator section"
#define MAX_BUS_NAME_LEN 32 // e.g. "sidechain input"
#define MAX_CHAN_NAME_LEN 32 // e.g. "input 1"

#define MAX_VERSION_STR_LEN 32
#define MAX_BUILD_INFO_STR_LEN 256
static const int MAX_PARAM_DISPLAY_PRECISION = 6;

#define MAX_AAX_PARAMID_LEN 32

#define PARAM_UNINIT 99.99e-9

#ifndef MAX_BLOB_LENGTH
#define MAX_BLOB_LENGTH 2048
#endif

#ifndef IDLE_TIMER_RATE
#define IDLE_TIMER_RATE 20 // this controls the frequency of data going from processor to editor (and OnIdle calls)
#endif

#ifndef MAX_SYSEX_SIZE
#define MAX_SYSEX_SIZE 512
#endif

#define PARAM_TRANSFER_SIZE 512
#define MIDI_TRANSFER_SIZE 32
#define SYSEX_TRANSFER_SIZE 4

// All version ints are stored as 0xVVVVRRMM: V = version, R = revision, M = minor revision.
#define IPLUG_VERSION 0x010000
#define IPLUG_VERSION_MAGIC 'pfft'

static const int DEFAULT_BLOCK_SIZE = 1024;
static const double DEFAULT_TEMPO = 120.0;
static const int kNoParameter = -1;
static const int kNoValIdx = -1;
static const int kNoTag = -1;

#define MAX_BUS_CHANS 64 // wild cards in channel i/o strings will result in this many channels

//#if defined VST3_API || defined VST3C_API || defined VST3P_API
//#undef stricmp
//#undef strnicmp
//#include "pluginterfaces/vst/vsttypes.h"
//static const uint64_t kInvalidBusType = Steinberg::Vst::SpeakerArr::kEmpty;
//#elif defined AU_API || AUv3_API
//#include <CoreAudio/CoreAudio.h>
//static const uint64_t kInvalidBusType = kAudioChannelLayoutTag_Unknown;
//#elif defined AAX_API
//#include "AAX_Enums.h"
//static const uint64_t kInvalidBusType = AAX_eStemFormat_None;
//#else
//static const uint64_t kInvalidBusType = 0;
//#endif

/** @enum EParamSource
 * Used to identify the source of a parameter change
 */
enum EParamSource
{
  kReset,
  kHost,
  kPresetRecall,
  kUI,
  kDelegate,
  kRecompile, // for FAUST JIT
  kUnknown,
  kNumParamSources
};

static const char* ParamSourceStrs[kNumParamSources] = { "Reset", "Host", "Preset", "UI", "Editor Delegate", "Recompile", "Unknown"};

/** @enum ERoute
 * Used to identify whether a bus/channel connection is an input or an output
 */
enum ERoute
{
  kInput = 0,
  kOutput = 1
};

static const char* RoutingDirStrs[2]  = { "Input", "Output" };

enum EAPI
{
  kAPIVST2 = 0,
  kAPIVST3 = 1,
  kAPIAU = 2,
  kAPIAUv3 = 3,
  kAPIAAX = 4,
  kAPIAPP = 5,
  kAPIWAM = 6,
  kAPIWEB = 7
};

/** @enum EHost
 * Host identifier
 */
enum EHost
{
  kHostUninit = -1,
  kHostUnknown = 0,
  kHostReaper,
  kHostProTools,
  kHostCubase,
  kHostNuendo,
  kHostSonar,
  kHostVegas,
  kHostFL,
  kHostSamplitude,
  kHostAbletonLive,
  kHostTracktion,
  kHostNTracks,
  kHostMelodyneStudio,
  kHostVSTScanner,
  kHostAULab,
  kHostForte,
  kHostChainer,
  kHostAudition,
  kHostOrion,
  kHostBias,
  kHostSAWStudio,
  kHostLogic,
  kHostGarageBand,
  kHostDigitalPerformer,
  kHostStandalone,
  kHostAudioMulch,
  kHostStudioOne,
  kHostVST3TestHost,
  kHostArdour,
  kHostRenoise,
  kHostOpenMPT,
  kHostWaveLab,
  kHostWaveLabElements,
  kHostTwistedWave,
  kHostBitwig,
  kHostWWW,
  // These hosts don't report the host name:
  // EnergyXT2
  // MiniHost
};

enum EResourceLocation
{
  kNotFound = 0,
  kAbsolutePath,
  kWinBinary,
  kPreloadedTexture
};

// These constants come from vstpreset.cpp, allowing saving of VST3 format presets without including the VST3 SDK
typedef char ChunkID[4];

enum ChunkType
{
  kHeader,
  kComponentState,
  kControllerState,
  kProgramData,
  kMetaInfo,
  kChunkList,
  kNumPresetChunks
};

static const ChunkID commonChunks[kNumPresetChunks] = {
  {'V', 'S', 'T', '3'},  // kHeader
  {'C', 'o', 'm', 'p'},  // kComponentState
  {'C', 'o', 'n', 't'},  // kControllerState
  {'P', 'r', 'o', 'g'},  // kProgramData
  {'I', 'n', 'f', 'o'},  // kMetaInfo
  {'L', 'i', 's', 't'}   // kChunkList
};

// Preset Header: header id + version + class id + list offset
static const int32_t kFormatVersion = 1;
static const int32_t kClassIDSize = 32; // ASCII-encoded FUID
static const int32_t kHeaderSize = sizeof (ChunkID) + sizeof (int32_t) + kClassIDSize + sizeof (int64_t);
//static const int32_t kListOffsetPos = kHeaderSize - sizeof (int64_t);

// Preset Version Constants
static const int kFXPVersionNum = 1;
static const int kFXBVersionNum = 2;

END_IPLUG_NAMESPACE

/**@}*/


