#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

#include "IPlugBase.h"
#include "IGraphics.h"

const int kPTParamIdxOffset = 2;

struct IPlugInstanceInfo
{
  // not used
};

class IPlugRTAS : public IPlugBase
{
public:
  IPlugRTAS(IPlugInstanceInfo instanceInfo,
            int nParams,
            const char* channelIOStr,
            int nPresets,
            const char* effectName,
            const char* productName,
            const char* mfrName,
            int vendorVersion,
            int uniqueID,
            int mfrID,
            int latency = 0,
            bool plugDoesMidi = false,
            bool plugDoesChunks = false,
            bool plugIsInst = false,
            int plugScChans = 0);

  void BeginInformHostOfParamChange(int idx);
  void InformHostOfParamChange(int idx, double normalizedValue);
  void EndInformHostOfParamChange(int idx);
  void InformHostOfProgramChange();

  int GetSamplePos();
  double GetTempo();
  void GetTimeSig(int* pNum, int* pDenom);
  void GetTime(ITimeInfo* pTimeInfo);

  void ResizeGraphics(int w, int h);
  EHost GetHost();  // GetHostVersion() is inherited.
  
  void Created(class IPlugProcess* pProcess);

  void ProcessAudio(float** inputs, float** outputs, int nFrames);
  void ProcessAudioBypassed(float** inputs, float** outputs, int nFrames);

  void SetIO(int nInputs, int nOutputs);
  
  void SetSideChainConnected(bool connected);

  void SetParameter(int idx); // Locks mutex first
  
  void DirtyPTCompareState();

protected:
  bool SendMidiMsg(IMidiMsg* pMsg);

private:
  bool mHasSideChain, mSideChainIsConnected;
  int mSideChainConnectionNum;
  class IPlugProcess *mProcess;
};

IPlugRTAS* MakePlug();

#endif