#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>

#include "IPlugConstants.h"
#include "IPlugOSDetect.h"

#ifdef OS_WIN
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#undef WINVER
#define WINVER 0x0501
#pragma warning(disable:4018 4267)	// size_t/signed/unsigned mismatch..
#pragma warning(disable:4800)		// if (pointer) ...
#pragma warning(disable:4805)		// Compare bool and BOOL.
#endif

/**
 * @file
 * Utility functions and macros
 */

#define FREE_NULL(p) {free(p);p=nullptr;}
#define DELETE_NULL(p) {delete(p); p=nullptr;}
#define DELETE_ARRAY(p) {delete[](p); (p)=nullptr;}

// TODO: replace BOUNDED with template based alternative
/** Clamps the value \p x between \p lo and \p hi
 * @param x Input value
 * @param lo Minimum value to be allowed
 * @param hi Maximum value to be allowed
 * If \p x is outside given range, it will be set to one of the boundaries
*/
#define BOUNDED(x,lo,hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

#define CSTR_NOT_EMPTY(cStr) ((cStr) && (cStr)[0] != '\0')

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

/** @brief Calculates gain from a given dB value
 * @param dB Value in dB
 * @return Gain calculated as an approximation of
 * \f$ 10^{\frac{x}{20}} \f$
 * @see #IAMP_DB
 */
inline double DBToAmp(double dB)
{
  return exp(IAMP_DB * dB);
}

/**
 * @return dB calculated as an approximation of
 * \f$ 20*log_{10}(x) \f$
 * @see #AMP_DB
 */
inline double AmpToDB(double amp)
{
  return AMP_DB * log(fabs(amp));
}

inline void GetVersionParts(int version, int& ver, int& maj, int& min)
{
  ver = (version & 0xFFFF0000) >> 16;
  maj = (version & 0x0000FF00) >> 8;
  min = version & 0x000000FF;
}

inline int GetDecimalVersion(int version)
{
  int ver, rmaj, rmin;
  GetVersionParts(version, ver, rmaj, rmin);
  return 10000 * ver + 100 * rmaj + rmin;
}

inline void GetVersionStr(int version, WDL_String& str)
{
  int ver, rmaj, rmin;
  GetVersionParts(version, ver, rmaj, rmin);
  str.SetFormatted(MAX_VERSION_STR_LEN, "v%d.%d.%d", ver, rmaj, rmin);
}

inline double ToNormalizedParam(double nonNormalizedValue, double min, double max, double shape)
{
  return std::pow((nonNormalizedValue - min) / (max - min), 1.0 / shape);
}

inline double FromNormalizedParam(double normalizedValue, double min, double max, double shape)
{
  return min + std::pow((double) normalizedValue, shape) * (max - min);
}

template <class SRC, class DEST>
void CastCopy(DEST* pDest, SRC* pSrc, int n)
{
  for (int i = 0; i < n; ++i, ++pDest, ++pSrc)
  {
    *pDest = (DEST) *pSrc;
  }
}

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
 * @param host Host name to search for
 * @return Identifier of the host (see ::EHost)
 */
static EHost LookUpHost(const char* inHost)
{
  char host[256];
  ToLower(host, inHost);
  
  // C4 is version >= 8.2
  if (strstr(host, "cubase")) return kHostCubase;
  if (strstr(host, "reaper")) return kHostReaper;
  if (strstr(host, "nuendo")) return kHostNuendo;
  if (strstr(host, "cakewalk")) return kHostSonar;
  if (strstr(host, "samplitude")) return kHostSamplitude;
  if (strstr(host, "fruity")) return kHostFL;
  if (strstr(host, "live")) return kHostAbletonLive;
  if (strstr(host, "melodyne")) return kHostMelodyneStudio;
  if (strstr(host, "vstmanlib")) return kHostVSTScanner;
  if (strstr(host, "aulab")) return kHostAULab;
  if (strstr(host, "garageband")) return kHostGarageBand;
  if (strstr(host, "forte")) return kHostForte;
  if (strstr(host, "chainer")) return kHostChainer;
  if (strstr(host, "audition")) return kHostAudition;
  if (strstr(host, "orion")) return kHostOrion;
  if (strstr(host, "sawstudio")) return kHostSAWStudio;
  if (strstr(host, "logic")) return kHostLogic;
  if (strstr(host, "digital")) return kHostDigitalPerformer;
  if (strstr(host, "audiomulch")) return kHostAudioMulch;
  if (strstr(host, "presonus")) return kHostStudioOne;
  if (strstr(host, "vst3plugintesthost")) return kHostVST3TestHost;
  if (strstr(host, "protools")) return kHostProTools;
  if (strstr(host, "ardour")) return kHostArdour;
  if (strstr(host, "openmpt")) return kHostOpenMPT;
  if (strstr(host, "renoise")) return kHostRenoise;
  if (strstr(host, "standalone")) return kHostStandalone;
  if (strstr(host, "wavelab")) return kHostWaveLab;
  if (strstr(host, "wavelab elements")) return kHostWaveLabElements;
  if (strstr(host, "bitwig studio")) return kHostBitwig;
  if (strstr(host, "twistedwave")) return kHostTwistedWave;
  
  return kHostUnknown;
}

/**
 * Gets a human-readable name from host identifier
 * @param host Host identifier (see ::EHost)
 * @param pHostName Pointer to a string to write to
 * @code
 *    int hostID = EHost::kHostAbletonLive;
 *    char buffer[20];
 *    GetHostNameStr(hostID, buffer);
 * @endcode
 *
 * The longest string returned by GetHostNameStr is 18 characters long (+1 for the null terminator).
 * Make sure your buffer can handle the size!
 */
static void GetHostNameStr(EHost host, char* pHostName)
{
  switch (host)
  {
    case kHostCubase:
      strcpy(pHostName, "Cubase");
      break;
    case kHostNuendo:
      strcpy(pHostName, "Nuendo");
      break;
    case kHostLogic:
      strcpy(pHostName, "Logic");
      break;
    case kHostAULab:
      strcpy(pHostName, "AULab");
      break;
    case kHostGarageBand:
      strcpy(pHostName, "GarageBand");
      break;
    case kHostAbletonLive:
      strcpy(pHostName, "Live");
      break;
    case kHostReaper:
      strcpy(pHostName, "Reaper");
      break;
    case kHostSonar:
      strcpy(pHostName, "Sonar");
      break;
    case kHostVST3TestHost:
      strcpy(pHostName, "VST3PluginTestHost");
      break;
    case kHostStudioOne:
      strcpy(pHostName, "StudioOne");
      break;
    case kHostSAWStudio:
      strcpy(pHostName, "SAWStudio");
      break;
    case kHostSamplitude:
      strcpy(pHostName, "Samplitude");
      break;
    case kHostOrion:
      strcpy(pHostName, "Orion");
      break;
    case kHostAudition:
      strcpy(pHostName, "Audition");
      break;
    case kHostChainer:
      strcpy(pHostName, "Chainer");
      break;
    case kHostVSTScanner:
      strcpy(pHostName, "VSTScanner"); // ??
      break;
    case kHostForte:
      strcpy(pHostName, "Forte");
      break;
    case kHostVegas:
      strcpy(pHostName, "Vegas");
      break;
    case kHostFL:
      strcpy(pHostName, "FLStudio");
      break;
    case kHostProTools:
      strcpy(pHostName, "ProTools");
      break;
    case kHostAudioMulch:
      strcpy(pHostName, "AudioMulch");
      break;
    case kHostDigitalPerformer:
      strcpy(pHostName, "DigitalPerformer");
      break;
    case kHostArdour:
      strcpy(pHostName, "Ardour");
      break;
    case kHostOpenMPT:
      strcpy(pHostName, "OpenMPT");
      break;
    case kHostRenoise:
      strcpy(pHostName, "Renoise");
      break;
    case kHostStandalone:
      strcpy(pHostName, "Standalone");
      break;
    case kHostWaveLab:
      strcpy(pHostName, "WaveLab");
      break;
    case kHostWaveLabElements:
      strcpy(pHostName, "WaveLabElements");
      break;
    case kHostTwistedWave:
      strcpy(pHostName, "TwistedWave");
      break;
    case kHostBitwig:
      strcpy(pHostName, "Bitwig");
      break;
    default:
      strcpy(pHostName, "Unknown");
      break;
  }
}

#ifndef REMINDER
  #ifdef OS_WIN
    // This enables: #pragma REMINDER("change this line!") with click-through from VC++.
    #define REMINDER(msg) message(__FILE__   "(" MAKE_STR(__LINE__) "): " msg)
  #elif defined __APPLE__
    #define REMINDER(msg) WARNING msg
  #endif
#endif
