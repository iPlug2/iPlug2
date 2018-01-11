#pragma once

#include <cstdlib>
#include <cstring>

/**
 * @file
 * @brief API and host definitions
 */

enum EAPI
{
  kAPIVST2 = 0,
  kAPIVST3 = 1,
  kAPIAU = 2,
  //kAPIRTAS = 3,
  kAPIAAX = 4,
  kAPISA = 5
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
  kHostBitwig
  
  // These hosts don't report the host name:
  // EnergyXT2
  // MiniHost
};

/** Gets the host ID from a human-readable name
 * @param host Host name to search for
 * @return Identifier of the host (see ::EHost)
 */
EHost LookUpHost(const char* host);

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
void GetHostNameStr(EHost host, char* pHostName);

