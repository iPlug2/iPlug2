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
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#undef WINVER
#define WINVER 0x0501
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

/** /todo  
 * @param txt /todo
 * @param numLines /todo
 * @param maxLineWidth /todo */
inline void BasicTextMeasure(const char* txt, float& numLines, float& maxLineWidth) {
  float w = 0.0;
  maxLineWidth = 0.0;
  numLines = 0.0;
  while (true) {
    if (*txt == '\0' || *txt == '\n') {
      ++numLines;
      if (w > maxLineWidth)
        maxLineWidth = w;
      if (*txt == '\0')
        break;
      w = 0.0;
      }
    else
      ++w;
    ++txt;
    }
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
 * @param pHostName Pointer to a string to write to
 * @code
 *    int hostID = EHost::kHostAbletonLive;
 *    char buffer[20];
 *    GetHostNameStr(hostID, buffer);
 * @endcode
 *
 * The longest string returned by GetHostNameStr is 18 characters long (+1 for the null terminator).
 * Make sure your buffer can handle the size! */
static void GetHostNameStr(EHost host, char* pHostName)
{
  switch (host)
  {
      case kHostReaper:             strcpy(pHostName, "reaper");              break;
      case kHostProTools:           strcpy(pHostName, "protools");            break;
      case kHostCubase:             strcpy(pHostName, "cubase");              break;
      case kHostNuendo:             strcpy(pHostName, "nuendo");              break;
      case kHostSonar:              strcpy(pHostName, "cakewalk");            break;
      case kHostVegas:              strcpy(pHostName, "vegas");               break;
      case kHostFL:                 strcpy(pHostName, "fruity");              break;
      case kHostSamplitude:         strcpy(pHostName, "samplitude");          break;
      case kHostAbletonLive:        strcpy(pHostName, "live");                break;
      case kHostTracktion:          strcpy(pHostName, "tracktion");           break;
      case kHostNTracks:            strcpy(pHostName, "ntracks");             break;
      case kHostMelodyneStudio:     strcpy(pHostName, "melodyne");            break;
      case kHostVSTScanner:         strcpy(pHostName, "vstmanlib");           break;
      case kHostAULab:              strcpy(pHostName, "aulab");               break;
      case kHostForte:              strcpy(pHostName, "forte");               break;
      case kHostChainer:            strcpy(pHostName, "chainer");             break;
      case kHostAudition:           strcpy(pHostName, "audition");            break;
      case kHostOrion:              strcpy(pHostName, "orion");               break;
      case kHostBias:               strcpy(pHostName, "bias");                break;
      case kHostSAWStudio:          strcpy(pHostName, "sawstudio");           break;
      case kHostLogic:              strcpy(pHostName, "logic");               break;
      case kHostGarageBand:         strcpy(pHostName, "garageband");          break;
      case kHostDigitalPerformer:   strcpy(pHostName, "digital");             break;
      case kHostAudioMulch:         strcpy(pHostName, "audiomulch");          break;
      case kHostStudioOne:          strcpy(pHostName, "presonus");            break;
      case kHostVST3TestHost:       strcpy(pHostName, "vst3plugintesthost");  break;
      case kHostArdour:             strcpy(pHostName, "ardour");              break;
      case kHostRenoise:            strcpy(pHostName, "renoise");             break;
      case kHostOpenMPT:            strcpy(pHostName, "OpenMPT");             break;
      case kHostWaveLabElements:    strcpy(pHostName, "wavelab elements");    break;
      case kHostWaveLab:            strcpy(pHostName, "wavelab");             break;
      case kHostTwistedWave:        strcpy(pHostName, "twistedwave");         break;
      case kHostBitwig:             strcpy(pHostName, "bitwig studio");       break;
      case kHostReason:             strcpy(pHostName, "reason");              break;
      case kHostGoldWave5x:         strcpy(pHostName, "gwvst");               break;
      case kHostWaveform:           strcpy(pHostName, "waveform");            break;
      case kHostAudacity:           strcpy(pHostName, "audacity");            break;
      case kHostAcoustica:          strcpy(pHostName, "acoustica");           break;
      case kHostPluginDoctor:       strcpy(pHostName, "plugindoctor");        break;
      case kHostiZotopeRX:          strcpy(pHostName, "izotope rx");          break;
      case kHostSAVIHost:           strcpy(pHostName, "savihost");            break;
      case kHostBlueCat:            strcpy(pHostName, "blue cat's vst host"); break;
      
      case kHostStandalone:         strcpy(pHostName, "standalone");          break;
      case kHostWWW:                strcpy(pHostName, "www");                 break;

      default:                      strcpy(pHostName, "Unknown"); break;
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
