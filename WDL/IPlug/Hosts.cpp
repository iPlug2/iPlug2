#include "Hosts.h"
#include <ctype.h>
#include "Log.h"

EHost LookUpHost(const char* inHost)
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
  if (strstr(host, "standalone")) return kHostStandalone;
  
  return kHostUnknown;
}

void GetHostNameStr(EHost host, char* pHostName)
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
  case kHostStandalone:
    strcpy(pHostName, "Standalone");
    break;
  default:
    strcpy(pHostName, "Unknown");
    break;
  }
}