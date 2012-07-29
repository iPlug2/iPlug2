#if WINDOWS_VERSION
  #include "Mac2Win.H"
#endif

#ifndef __IPLUGPPROCESS_AS__
#define __IPLUGPPROCESS_AS__

#include "FicPluginConnections.h"
#include "CEffectProcessAS.h"
#include "IPlugProcess.h"

#define MAX_AS_BLOCKSIZE 8192

class IPlugProcessAS : public IPlugProcess, public CEffectProcessAS
{
public:
  IPlugProcessAS(OSType type);
  virtual ~IPlugProcessAS();

//  virtual void SetViewOrigin (Point anOrigin);

  virtual int GetBlockSize() { return MAX_AS_BLOCKSIZE; }
  virtual double GetTempo()  { return DEFAULT_TEMPO; }
  virtual void GetTimeSig(int* pNum, int* pDenom) { *pNum = 4; *pDenom = 4;  }
  virtual int GetSamplePos() { return 0; }
  virtual void GetTime( double *pSamplePos,
                        double *pTempo,
                        double *pMusicalPos,
                        double *pLastBar,
                        int* pNum,
                        int* pDenom,
                        double *pCycleStart,
                        double *pCycleEnd,
                        bool *pTransportRunning,
                        bool *pTransportCycle)
  {
    *pSamplePos = *pMusicalPos = *pLastBar = *pCycleStart = *pCycleEnd = 0.;
    *pTempo = DEFAULT_TEMPO;
    *pNum = 4;
    *pDenom = 4;
    *pTransportRunning = false;
    *pTransportCycle = false;
  }

protected:
  //virtual void GetMetersFromDSPorRTAS(long *allMeters, bool *clipIndicators);
  virtual UInt32 ProcessAudio(bool isMasterBypassed);

  float ** inputAudioStreams;
  float ** outputAudioStreams;
};

#endif  // __IPLUGPPROCESS_AS__
