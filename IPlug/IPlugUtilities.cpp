#include "IPlugUtilities.h"

void GetVersionParts(int version, int* pVer, int* pMaj, int* pMin)
{
  *pVer = (version & 0xFFFF0000) >> 16;
  *pMaj = (version & 0x0000FF00) >> 8;
  *pMin = version & 0x000000FF;
}

int GetDecimalVersion(int version)
{
  int ver, rmaj, rmin;
  GetVersionParts(version, &ver, &rmaj, &rmin);
  return 10000 * ver + 100 * rmaj + rmin;
}

void GetVersionStr(int version, char* str)
{
  int ver, rmaj, rmin;
  GetVersionParts(version, &ver, &rmaj, &rmin);
  sprintf(str, "v%d.%d.%d", ver, rmaj, rmin);
}