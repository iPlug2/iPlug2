#ifndef _PLUGINHOSTS_
#define _PLUGINHOSTS_

#include <stdlib.h>
#include <string.h>

enum EAPI
{
  kAPIVST2 = 0,
  kAPIVST3 = 1,
  kAPIAU = 2,
  kAPIRTAS = 3,
  kAPIAAX = 4,
  kAPISA = 5
};

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
  kHostArdour

  // These hosts don't report the host name:
  // EnergyXT2
  // MiniHost
};

EHost LookUpHost(const char* host);
void GetHostNameStr(EHost host, char* pHostName);

#endif


