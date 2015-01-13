#if WINDOWS_VERSION
  #include "Mac2Win.H"
#endif

#ifndef __IPLUGPPROCESS_RTAS__
#define __IPLUGPPROCESS_RTAS__

#include "FicPluginConnections.h"
#include "IPlugProcess.h"
#include "CEffectProcessRTAS.h"

class IPlugProcessRTAS : public IPlugProcess, public CEffectProcessRTAS
{
public:
  IPlugProcessRTAS(OSType type);
  virtual ~IPlugProcessRTAS() {}

  virtual ComponentResult IsControlAutomatable(long aControlIndex, short *aItIsP);
  virtual ComponentResult GetDelaySamplesLong(long* aNumSamples);

  virtual int GetBlockSize() { return GetMaximumRTASQuantum(); }

  virtual double GetTempo();
  virtual void GetTimeSig(int* pNum, int* pDenom);
  virtual int GetSamplePos();
  virtual void GetTime( double *pSamplePos,
                        double *pTempo,
                        double *pMusicalPos,
                        double *pLastBar,
                        int* pNum,
                        int* pDenom,
                        double *pCycleStart,
                        double *pCycleEnd,
                        bool *pTransportRunning,
                        bool *pTransportCycle);

protected:
  //virtual void GetMetersFromDSPorRTAS(long *allMeters, bool *clipIndicators);
  virtual void RenderAudio(float** inputs, float** outputs, long frames);

private:
  void HandleMIDI();
};

#endif  // __IPLUGPPROCESS_RTAS__
