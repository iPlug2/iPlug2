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

#include "wdlstring.h"

#include "IPlugConstants.h"
#include "IPlugPlatform.h"

#ifdef OS_WIN
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
  return exp(IAMP_DB * dB);
}

/** @return dB calculated as an approximation of
 * \f$ 20*log_{10}(x) \f$
 * @see #AMP_DB */
static inline double AmpToDB(double amp)
{
  return AMP_DB * log(std::fabs(amp));
}

/** /todo  
 * @param version /todo
 * @param ver /todo
 * @param maj /todo
 * @param min /todo */
static inline void GetVersionParts(int version, int& ver, int& maj, int& min)
{
  ver = (version & 0xFFFF0000) >> 16;
  maj = (version & 0x0000FF00) >> 8;
  min = version & 0x000000FF;
}

/** /todo  
 * @param version /todo
 * @return int /todo */
static inline int GetDecimalVersion(int version)
{
  int ver, rmaj, rmin;
  GetVersionParts(version, ver, rmaj, rmin);
  return 10000 * ver + 100 * rmaj + rmin;
}

/** /todo 
 * @param version /todo
 * @param str /todo */
static inline void GetVersionStr(int version, WDL_String& str)
{
  int ver, rmaj, rmin;
  GetVersionParts(version, ver, rmaj, rmin);
  str.SetFormatted(MAX_VERSION_STR_LEN, "v%d.%d.%d", ver, rmaj, rmin);
}

/** /todo  
 * @tparam SRC 
 * @tparam DEST 
 * @param pDest /todo
 * @param pSrc /todo
 * @param n /todo */
template <class SRC, class DEST>
void CastCopy(DEST* pDest, SRC* pSrc, int n)
{
  for (int i = 0; i < n; ++i, ++pDest, ++pSrc)
  {
    *pDest = (DEST) *pSrc;
  }
}

/** /todo  
 * @param cDest /todo
 * @param cSrc /todo */
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
  if (strstr(host, "OpenMPT"))              return kHostOpenMPT;
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
      
      case kHostStandalone:         str.Set("standalone");          break;
      case kHostWWW:                str.Set("www");                 break;

      default:                      str.Set("Unknown"); break;
  }
}

/** /todo 
 * @param midiPitch /todo
 * @param noteName /todo
 * @param cents /todo
 * @param middleCisC4 /todo */
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

END_IPLUG_NAMESPACE

/**@}*/
