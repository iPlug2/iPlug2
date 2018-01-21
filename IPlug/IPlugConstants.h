#pragma once

/**
 * @file
 * @brief Constant definitions, magic numbers
 */

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

//TODO: these should be in a namespace, to avoid conflicts with third-party libraries
static const double PI = 3.141592653589793238;

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

#define MAX_PATH_LEN 256
#define MAX_PARAM_LEN 256
#define MAX_EFFECT_NAME_LEN 128
#define MAX_PARAM_NAME_LEN 32 // e.g. "Gain"
#define MAX_PARAM_LABEL_LEN 32 // e.g. "Percent"
#define MAX_PARAM_DISPLAY_LEN 32 // e.g. "100" / "Mute"
#define MAX_VERSION_STR_LEN 32
#define MAX_BUILD_INFO_STR_LEN 256
static const int MAX_PARAM_DISPLAY_PRECISION = 6;

#define PARAM_UNINIT 99.99e-9

#ifndef MAX_BLOB_LENGTH
#define MAX_BLOB_LENGTH 2048
#endif

// All version ints are stored as 0xVVVVRRMM: V = version, R = revision, M = minor revision.
#define IPLUG_VERSION 0x010000
#define IPLUG_VERSION_MAGIC 'pfft'

static const int DEFAULT_BLOCK_SIZE = 1024;
static const double DEFAULT_TEMPO = 120.0;
static const int kNoParameter = -1;

enum EAPI
{
  kAPIVST2 = 0,
  kAPIVST3 = 1,
  kAPIAU = 2,
  kAPIAUv3 = 3,
  kAPIAAX = 4,
  kAPISA = 5
};

