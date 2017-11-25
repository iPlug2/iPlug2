#pragma once

typedef unsigned char BYTE;

#ifndef VstInt32
#include <stdint.h>
typedef int32_t VstInt32;
#endif

enum EIPlugKeyCodes
{
  KEY_SPACE,
  KEY_UPARROW,
  KEY_DOWNARROW,
  KEY_LEFTARROW,
  KEY_RIGHTARROW,
  KEY_DIGIT_0,
  KEY_DIGIT_9=KEY_DIGIT_0+9,
  KEY_ALPHA_A,
  KEY_ALPHA_Z=KEY_ALPHA_A+25
};

//enum EVST3ParamIDs
//{
//  kBypassParam = 65536,
//  kPresetParam, // not used unless baked in presets declared
//  kMIDICCParamStartIdx
//};

#define PI 3.141592653589793238
#define AMP_DB 8.685889638065036553
#define IAMP_DB 0.11512925464970

const double DEFAULT_SAMPLE_RATE = 44100.0;
const int MAX_PRESET_NAME_LEN = 256;
#define UNUSED_PRESET_NAME "empty"
#define DEFAULT_USER_PRESET_NAME "user preset"

#define MAX_PATH_LEN 256
#define MAX_PARAM_LEN 256
#define MAX_EFFECT_NAME_LEN 128
#define MAX_PARAM_NAME_LEN 32 // e.g. "Gain"
#define MAX_PARAM_LABEL_LEN 32 // e.g. "Percent"
#define MAX_PARAM_DISPLAY_LEN 32 // e.g. "100" / "Mute"
#define MAX_PARAM_DISPLAY_PRECISION 6

#define PARAM_UNINIT 99.99e-9

#ifndef MAX_BLOB_LENGTH
#define MAX_BLOB_LENGTH 2048
#endif

// All version ints are stored as 0xVVVVRRMM: V = version, R = revision, M = minor revision.
#define IPLUG_VERSION 0x010000
#define IPLUG_VERSION_MAGIC 'pfft'

#define DEFAULT_BLOCK_SIZE 1024
#define DEFAULT_TEMPO 120.0

// Uncomment to enable IPlug::OnIdle() and IGraphics::OnGUIIdle().
// #define USE_IDLE_CALLS


