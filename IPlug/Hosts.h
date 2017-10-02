#pragma once

#include <cstdlib>
#include <cstring>

enum EAPI
{
  kAPIVST2 = 0,
  kAPIVST3 = 1,
  kAPIAU = 2,
  //kAPIRTAS = 3,
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

EHost LookUpHost(const char* host);
void GetHostNameStr(EHost host, char* pHostName);

