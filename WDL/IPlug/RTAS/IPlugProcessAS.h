#if WINDOWS_VERSION
#include "Mac2Win.H"
#endif

#ifndef __IPLUGPPROCESS_AS__
#define __IPLUGPPROCESS_AS__

#include "FicPluginConnections.h"
#include "CEffectProcessAS.h"
#include "IPlugProcess.h"

class IPlugProcessAS : public IPlugProcess, public CEffectProcessAS
{
  public:
    IPlugProcessAS(OSType type);
    virtual ~IPlugProcessAS();

//    virtual void UpdateControlValueInAlgorithm(long aControlIndex);
//  virtual void SetViewOrigin (Point anOrigin);

  virtual int GetBlockSize() { return 0; }
  
  virtual double GetTempo()  { return 120.; }
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
  { *pSamplePos = *pMusicalPos = *pLastBar = *pCycleStart = *pCycleEnd = 0.;
    *pTempo = 120.;
    *pNum = 4; 
    *pDenom = 4;  
    *pTransportRunning = false;
    *pTransportCycle = false;
  }
  
protected:

    // We use these to track the meter values
//    SFloat32 mMeterVal[EffectLayerDef::MAX_NUM_CONNECTIONS];
//    SFloat32 mMeterMin[EffectLayerDef::MAX_NUM_CONNECTIONS];

  protected:
    virtual void GetMetersFromDSPorRTAS(long *allMeters, bool *clipIndicators); 
    virtual UInt32 ProcessAudio(bool isMasterBypassed);
};

#endif  // __IPLUGPPROCESS_AS__
