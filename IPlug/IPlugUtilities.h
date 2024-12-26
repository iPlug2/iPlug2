/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * Utility functions and macros
 * @defgroup IPlugUtilities IPlug::Utilities
 * Utility functions and macros
 * @{
 */

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

#include "heapbuf.h"
#include "wdlstring.h"

#include "IPlugConstants.h"
#include "IPlugPlatform.h"

#ifdef OS_WIN
#include <windows.h>
#pragma warning(disable:4018 4267)	// size_t/signed/unsigned mismatch..
#pragma warning(disable:4800)		// if (pointer) ...
#pragma warning(disable:4805)		// Compare bool and BOOL.
#endif

BEGIN_IPLUG_NAMESPACE

/** Clips the value \p x between \p lo and \p hi
 * @param x Input value
 * @param lo Minimum value to be allowed
 * @param hi Maximum value to be allowed
 * If \p x is outside given range, it will be set to one of the boundaries */
template <typename T>
T Clip(T x, T lo, T hi) { return std::min(std::max(x, lo), hi); }

/** Linear interpolate between values \p a and \p b
* @param a Low value
* @param b High value
* @param f Value betweeen 0-1 for interpolation */
template <typename T>
inline T Lerp(T a, T b, T f) { return ((b - a) * f + a); }

static inline bool CStringHasContents(const char* str) { return str && str[0] != '\0'; }

#define MAKE_QUOTE(str) #str
#define MAKE_STR(str) MAKE_QUOTE(str)

/** @hideinitializer */
#define GET_PARAM_FROM_VARARG(paramType, vp, v) \
{ \
  v = 0.0; \
  switch (paramType) { \
    case IParam::kTypeBool: \
    case IParam::kTypeInt: \
    case IParam::kTypeEnum: { \
      v = (double) va_arg(vp, int); \
      break; \
    } \
    case IParam::kTypeDouble: \
    default: { \
      v = (double) va_arg(vp, double); \
      break; \
    } \
  } \
}

#ifndef REMINDER
#ifdef OS_WIN
// This enables: #pragma REMINDER("change this line!") with click-through from VC++.
#define REMINDER(msg) message(__FILE__   "(" MAKE_STR(__LINE__) "): " msg)
#elif defined __APPLE__
#define REMINDER(msg) WARNING msg
#endif
#endif

/** @brief Calculates gain from a given dB value
 * @param dB Value in dB
 * @return Gain calculated as an approximation of
 * \f$ 10^{\frac{x}{20}} \f$
 * @see #IAMP_DB
 */
static inline double DBToAmp(double dB)
{
  return std::exp(IAMP_DB * dB);
}

/** @return dB calculated as an approximation of
 * \f$ 20*log_{10}(x) \f$
 * @see #AMP_DB */
static inline double AmpToDB(double amp)
{
  return AMP_DB * std::log(std::fabs(amp));
}

/** Helper function to unpack the version number parts as individual integers
 * @param versionInteger The version number packed into an integer
 * @param maj The major version
 * @param min The minor version
 * @param pat The patch version */
static inline void GetVersionParts(int versionInteger, int& maj, int& min, int& pat)
{
  maj = (versionInteger & 0xFFFF0000) >> 16;
  min = (versionInteger & 0x0000FF00) >> 8;
  pat = versionInteger & 0x000000FF;
}

/** Helper function to get the version number as a decimal integer
 * @param versionInteger The version number packed into an integer
 * @return int Decimal version */
static inline int GetDecimalVersion(int versionInteger)
{
  int maj, min, pat;
  GetVersionParts(versionInteger, maj, min, pat);
  return 10000 * maj + 100 * min + pat;
}

/** Helper function to get the semantic version number as a string from an integer
 * @param versionInteger The version number packed into an integer
 * @param str WDL_String to be populated with the version number in MAJOR.MINOR.PATCH format as a string */
static inline void GetVersionStr(int versionInteger, WDL_String& str)
{
  int maj, min, pat;
  GetVersionParts(versionInteger, maj, min, pat);
  str.SetFormatted(MAX_VERSION_STR_LEN, "v%d.%d.%d", maj, min, pat);
}

/** Helper function to  loop through a buffer of samples copying and casting from e.g float to double
 * @tparam SRC The source type
 * @tparam DEST The destination type
 * @param pDest Ptr to the destination buffer
 * @param pSrc Ptr to the source buffer
 * @param n The number of or elements in the buffer */
template <class SRC, class DEST>
void CastCopy(DEST* pDest, SRC* pSrc, int n)
{
  for (int i = 0; i < n; ++i, ++pDest, ++pSrc)
  {
    *pDest = (DEST) *pSrc;
  }
}

/** \todo  
 * @param cDest \todo
 * @param cSrc \todo */
static void ToLower(char* cDest, const char* cSrc)
{
  int i, n = (int) strlen(cSrc);
  for (i = 0; i < n; ++i)
  {
    cDest[i] = tolower(cSrc[i]);
  }
  cDest[i] = '\0';
}

/** Gets the host ID from a human-readable name
 * @param inHost Host name to search for
 * @return Identifier of the host (see ::EHost) */
static EHost LookUpHost(const char* inHost)
{
  char host[256];
  ToLower(host, inHost);

  // C4 is version >= 8.2
  if (strstr(host, "reaper"))               return kHostReaper;
  if (strstr(host, "protools"))             return kHostProTools;
  if (strstr(host, "cubase"))               return kHostCubase;
  if (strstr(host, "nuendo"))               return kHostNuendo;
  if (strstr(host, "cakewalk"))             return kHostSonar;
  if (strstr(host, "vegas"))                return kHostVegas;
  if (strstr(host, "fruity"))               return kHostFL;
  if (strstr(host, "samplitude"))           return kHostSamplitude;
  if (strstr(host, "live"))                 return kHostAbletonLive;
  if (strstr(host, "tracktion"))            return kHostTracktion;
  if (strstr(host, "ntracks"))              return kHostNTracks;
  if (strstr(host, "melodyne"))             return kHostMelodyneStudio;
  if (strstr(host, "vstmanlib"))            return kHostVSTScanner;
  if (strstr(host, "aulab"))                return kHostAULab;
  if (strstr(host, "forte"))                return kHostForte;
  if (strstr(host, "chainer"))              return kHostChainer;
  if (strstr(host, "audition"))             return kHostAudition;
  if (strstr(host, "orion"))                return kHostOrion;
  if (strstr(host, "bias"))                 return kHostBias;
  if (strstr(host, "sawstudio"))            return kHostSAWStudio;
  if (strstr(host, "logic"))                return kHostLogic;
  if (strstr(host, "garageband"))           return kHostGarageBand;
  if (strstr(host, "digital"))              return kHostDigitalPerformer;
  if (strstr(host, "audiomulch"))           return kHostAudioMulch;
  if (strstr(host, "presonus"))             return kHostStudioOne;
  if (strstr(host, "vst3plugintesthost"))   return kHostVST3TestHost;
  if (strstr(host, "ardour"))               return kHostArdour;
  if (strstr(host, "renoise"))              return kHostRenoise;
  if (strstr(host, "openmpt"))              return kHostOpenMPT;
  if (strstr(host, "wavelab elements"))     return kHostWaveLabElements; // check for wavelab elements should come before wavelab ...
  if (strstr(host, "wavelab"))              return kHostWaveLab;
  if (strstr(host, "twistedwave"))          return kHostTwistedWave;
  if (strstr(host, "bitwig studio"))        return kHostBitwig;
  if (strstr(host, "reason"))               return kHostReason;
  if (strstr(host, "gwvst"))                return kHostGoldWave5x;
  if (strstr(host, "waveform"))             return kHostWaveform;
  if (strstr(host, "audacity"))             return kHostAudacity;
  if (strstr(host, "acoustica"))            return kHostAcoustica;
  if (strstr(host, "plugindoctor"))         return kHostPluginDoctor;
  if (strstr(host, "izotope rx"))           return kHostiZotopeRX;
  if (strstr(host, "savihost"))             return kHostSAVIHost;
  if (strstr(host, "blue cat's vst host"))  return kHostBlueCat;
  if (strstr(host, "mixbus"))               return kHostMixbus32C;

  if (strstr(host, "standalone"))           return kHostStandalone;
  if (strstr(host, "www"))                  return kHostWWW;

  return kHostUnknown;

}

/** Gets a human-readable name from host identifier
 * @param host Host identifier (see ::EHost)
 * @param str WDL_String to set
 * @code
 *    int hostID = EHost::kHostAbletonLive;
 *    WDL_String hostName;
 *    GetHostNameStr(hostID, hostName);
 * @endcode*/
static void GetHostNameStr(EHost host, WDL_String& str)
{
  switch (host)
  {
      case kHostReaper:             str.Set("reaper");              break;
      case kHostProTools:           str.Set("protools");            break;
      case kHostCubase:             str.Set("cubase");              break;
      case kHostNuendo:             str.Set("nuendo");              break;
      case kHostSonar:              str.Set("cakewalk");            break;
      case kHostVegas:              str.Set("vegas");               break;
      case kHostFL:                 str.Set("fruity");              break;
      case kHostSamplitude:         str.Set("samplitude");          break;
      case kHostAbletonLive:        str.Set("live");                break;
      case kHostTracktion:          str.Set("tracktion");           break;
      case kHostNTracks:            str.Set("ntracks");             break;
      case kHostMelodyneStudio:     str.Set("melodyne");            break;
      case kHostVSTScanner:         str.Set("vstmanlib");           break;
      case kHostAULab:              str.Set("aulab");               break;
      case kHostForte:              str.Set("forte");               break;
      case kHostChainer:            str.Set("chainer");             break;
      case kHostAudition:           str.Set("audition");            break;
      case kHostOrion:              str.Set("orion");               break;
      case kHostBias:               str.Set("bias");                break;
      case kHostSAWStudio:          str.Set("sawstudio");           break;
      case kHostLogic:              str.Set("logic");               break;
      case kHostGarageBand:         str.Set("garageband");          break;
      case kHostDigitalPerformer:   str.Set("digital");             break;
      case kHostAudioMulch:         str.Set("audiomulch");          break;
      case kHostStudioOne:          str.Set("presonus");            break;
      case kHostVST3TestHost:       str.Set("vst3plugintesthost");  break;
      case kHostArdour:             str.Set("ardour");              break;
      case kHostRenoise:            str.Set("renoise");             break;
      case kHostOpenMPT:            str.Set("OpenMPT");             break;
      case kHostWaveLabElements:    str.Set("wavelab elements");    break;
      case kHostWaveLab:            str.Set("wavelab");             break;
      case kHostTwistedWave:        str.Set("twistedwave");         break;
      case kHostBitwig:             str.Set("bitwig studio");       break;
      case kHostReason:             str.Set("reason");              break;
      case kHostGoldWave5x:         str.Set("gwvst");               break;
      case kHostWaveform:           str.Set("waveform");            break;
      case kHostAudacity:           str.Set("audacity");            break;
      case kHostAcoustica:          str.Set("acoustica");           break;
      case kHostPluginDoctor:       str.Set("plugindoctor");        break;
      case kHostiZotopeRX:          str.Set("izotope rx");          break;
      case kHostSAVIHost:           str.Set("savihost");            break;
      case kHostBlueCat:            str.Set("blue cat's vst host"); break;
      case kHostMixbus32C:          str.Set("mixbus");              break;

      case kHostStandalone:         str.Set("standalone");          break;
      case kHostWWW:                str.Set("www");                 break;

      default:                      str.Set("Unknown"); break;
  }
}

/** \todo 
 * @param midiPitch \todo
 * @param noteName \todo
 * @param cents \todo
 * @param middleCisC4 \todo */
static void MidiNoteName(double midiPitch, WDL_String& noteName, bool cents = false, bool middleCisC4 = false)
{
  static const char noteNames[12][3] = {"C ","C#","D ","D#","E ","F ","F#","G ","G#","A ","A#","B "};
  
  int midiPitchR = (int) std::round(midiPitch);
  int pitchClass = midiPitchR % 12;
  int octave = (midiPitchR / 12) - (middleCisC4? 1 : 2);
  
  if (cents)
  {
    double frac = (midiPitch - (float) midiPitchR) * 100.;
    noteName.SetFormatted(32, "%s%i %.0f", noteNames[pitchClass], octave, frac);
  }
  else
  {
    noteName.SetFormatted(32, "%s%i", noteNames[pitchClass], octave);
  }
}

#if defined OS_WIN

static int UTF8ToUTF16Len(const char* utf8Str)
{
  return std::max(MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0), 1);
}

static void UTF8ToUTF16(wchar_t* wideStr, const char* utf8Str, int maxLen)
{
  int requiredSize = UTF8ToUTF16Len(utf8Str);

  if (requiredSize <= maxLen)
  {
    if (MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wideStr, requiredSize))
      return;
  }

  wideStr[0] = '\0';
}

static void UTF16ToUTF8(WDL_String& utf8Str, const wchar_t* wideStr)
{
  int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, NULL, 0, NULL, NULL);

  if (requiredSize > 0 && utf8Str.SetLen(requiredSize))
  {
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, utf8Str.Get(), requiredSize, NULL, NULL);
    return;
  }

  utf8Str.Set("");
}

class UTF8AsUTF16
{
public:

  UTF8AsUTF16(const char* utf8Str)
  {
    mUTF16Str.Resize(UTF8ToUTF16Len(utf8Str));
    UTF8ToUTF16(mUTF16Str.Get(), utf8Str, mUTF16Str.GetSize());
  }

  UTF8AsUTF16(const WDL_String& utf8Str) : UTF8AsUTF16(utf8Str.Get())
  {
  }

  const wchar_t* Get() const { return mUTF16Str.Get(); }
  int GetLength() const { return mUTF16Str.GetSize(); }

  UTF8AsUTF16& ToUpperCase()
  {
    _wcsupr(mUTF16Str.Get());
    return *this;
  }

  UTF8AsUTF16& ToLowerCase()
  {
    _wcslwr(mUTF16Str.Get());
    return *this;
  }

private:

  WDL_TypedBuf<wchar_t> mUTF16Str;
};

class UTF16AsUTF8
{
public:

  UTF16AsUTF8(const wchar_t* wideStr)
  {
    UTF16ToUTF8(mUTF8Str, wideStr);
  }

  const char* Get() const { return mUTF8Str.Get(); }
  int GetLength() const { return mUTF8Str.GetLength(); }

private:

  WDL_String mUTF8Str;
};
#endif


#if defined OS_WIN

static FILE* fopenUTF8(const char* path, const char* mode)
{
  return _wfopen(UTF8AsUTF16(path).Get(), UTF8AsUTF16(mode).Get());
}

#else

static FILE* fopenUTF8(const char* path, const char* mode)
{
  return fopen(path, mode);
}

#endif

/*
 * DOM Virtual Key Code to iPlug2 Virtual Key Code converter
 * 
 * Virtual key code definitions adapted from Emscripten
 * Copyright 2017 The Emscripten Authors. All rights reserved.
 * Source: https://github.com/emscripten-core/emscripten/
 * Licensed under MIT and University of Illinois/NCSA Open Source License
 */

// DOM Virtual Key codes (subset of most common keys)
enum EDOMVirtualKey {
  DOM_VK_BACK_SPACE = 0x08,
  DOM_VK_TAB = 0x09,
  DOM_VK_RETURN = 0x0D,
  DOM_VK_SHIFT = 0x10,
  DOM_VK_CONTROL = 0x11,
  DOM_VK_ALT = 0x12,
  DOM_VK_PAUSE = 0x13,
  DOM_VK_CAPS_LOCK = 0x14,
  DOM_VK_ESCAPE = 0x1B,
  DOM_VK_SPACE = 0x20,
  DOM_VK_PAGE_UP = 0x21,
  DOM_VK_PAGE_DOWN = 0x22,
  DOM_VK_END = 0x23,
  DOM_VK_HOME = 0x24,
  DOM_VK_LEFT = 0x25,
  DOM_VK_UP = 0x26,
  DOM_VK_RIGHT = 0x27,
  DOM_VK_DOWN = 0x28,
  DOM_VK_INSERT = 0x2D,
  DOM_VK_DELETE = 0x2E,
  DOM_VK_F1 = 0x70,
  DOM_VK_F2 = 0x71,
  DOM_VK_F3 = 0x72,
  DOM_VK_F4 = 0x73,
  DOM_VK_F5 = 0x74,
  DOM_VK_F6 = 0x75,
  DOM_VK_F7 = 0x76,
  DOM_VK_F8 = 0x77,
  DOM_VK_F9 = 0x78,
  DOM_VK_F10 = 0x79,
  DOM_VK_F11 = 0x7A,
  DOM_VK_F12 = 0x7B,
  DOM_VK_NUMPAD0 = 0x60,
  DOM_VK_NUMPAD1 = 0x61,
  DOM_VK_NUMPAD2 = 0x62,
  DOM_VK_NUMPAD3 = 0x63,
  DOM_VK_NUMPAD4 = 0x64,
  DOM_VK_NUMPAD5 = 0x65,
  DOM_VK_NUMPAD6 = 0x66,
  DOM_VK_NUMPAD7 = 0x67,
  DOM_VK_NUMPAD8 = 0x68,
  DOM_VK_NUMPAD9 = 0x69
};

/**
 * @brief Converts a DOM virtual key code to an iPlug2 virtual key code
 * @param domKeyCode The DOM virtual key code to convert
 * @return The corresponding iPlug2 virtual key code, or 0 if no mapping exists
 */
inline int DOMKeyToVirtualKey(uint32_t domKeyCode) {
  switch(domKeyCode) {
    case DOM_VK_BACK_SPACE:     return kVK_BACK;
    case DOM_VK_TAB:            return kVK_TAB;
    case DOM_VK_RETURN:         return kVK_RETURN;
    case DOM_VK_SHIFT:          return kVK_SHIFT;
    case DOM_VK_CONTROL:        return kVK_CONTROL;
    case DOM_VK_ALT:            return kVK_MENU;
    case DOM_VK_PAUSE:          return kVK_PAUSE;
    case DOM_VK_CAPS_LOCK:      return kVK_CAPITAL;
    case DOM_VK_ESCAPE:         return kVK_ESCAPE;
    case DOM_VK_SPACE:          return kVK_SPACE;
    case DOM_VK_PAGE_UP:        return kVK_PRIOR;
    case DOM_VK_PAGE_DOWN:      return kVK_NEXT;
    case DOM_VK_END:            return kVK_END;
    case DOM_VK_HOME:           return kVK_HOME;
    case DOM_VK_LEFT:           return kVK_LEFT;
    case DOM_VK_UP:             return kVK_UP;
    case DOM_VK_RIGHT:          return kVK_RIGHT;
    case DOM_VK_DOWN:           return kVK_DOWN;
    case DOM_VK_INSERT:         return kVK_INSERT;
    case DOM_VK_DELETE:         return kVK_DELETE;
    
    // Numbers (both main keyboard and numpad)
    case 0x30: case DOM_VK_NUMPAD0:  return kVK_0;
    case 0x31: case DOM_VK_NUMPAD1:  return kVK_1;
    case 0x32: case DOM_VK_NUMPAD2:  return kVK_2;
    case 0x33: case DOM_VK_NUMPAD3:  return kVK_3;
    case 0x34: case DOM_VK_NUMPAD4:  return kVK_4;
    case 0x35: case DOM_VK_NUMPAD5:  return kVK_5;
    case 0x36: case DOM_VK_NUMPAD6:  return kVK_6;
    case 0x37: case DOM_VK_NUMPAD7:  return kVK_7;
    case 0x38: case DOM_VK_NUMPAD8:  return kVK_8;
    case 0x39: case DOM_VK_NUMPAD9:  return kVK_9;
    
    // Letters
    case 0x41: return kVK_A;
    case 0x42: return kVK_B;
    case 0x43: return kVK_C;
    case 0x44: return kVK_D;
    case 0x45: return kVK_E;
    case 0x46: return kVK_F;
    case 0x47: return kVK_G;
    case 0x48: return kVK_H;
    case 0x49: return kVK_I;
    case 0x4A: return kVK_J;
    case 0x4B: return kVK_K;
    case 0x4C: return kVK_L;
    case 0x4D: return kVK_M;
    case 0x4E: return kVK_N;
    case 0x4F: return kVK_O;
    case 0x50: return kVK_P;
    case 0x51: return kVK_Q;
    case 0x52: return kVK_R;
    case 0x53: return kVK_S;
    case 0x54: return kVK_T;
    case 0x55: return kVK_U;
    case 0x56: return kVK_V;
    case 0x57: return kVK_W;
    case 0x58: return kVK_X;
    case 0x59: return kVK_Y;
    case 0x5A: return kVK_Z;
    
    // Function keys
    case DOM_VK_F1:  return kVK_F1;
    case DOM_VK_F2:  return kVK_F2;
    case DOM_VK_F3:  return kVK_F3;
    case DOM_VK_F4:  return kVK_F4;
    case DOM_VK_F5:  return kVK_F5;
    case DOM_VK_F6:  return kVK_F6;
    case DOM_VK_F7:  return kVK_F7;
    case DOM_VK_F8:  return kVK_F8;
    case DOM_VK_F9:  return kVK_F9;
    case DOM_VK_F10: return kVK_F10;
    case DOM_VK_F11: return kVK_F11;
    case DOM_VK_F12: return kVK_F12;
    
    default: return 0; // No mapping available
  }
}

END_IPLUG_NAMESPACE

/**@}*/
