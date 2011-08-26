#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

#include "IPlugBase.h"
#include "IGraphics.h"

const int kPTParamIdxOffset = 2;

struct IPlugInstanceInfo
{
  int magic;
};

class IPlugRTAS : public IPlugBase
{
public:
  IPlugRTAS(IPlugInstanceInfo instanceInfo, int nParams, const char* channelIOStr, int nPresets,
        const char* effectName, const char* productName, const char* mfrName,
        int vendorVersion, int uniqueID, int mfrID, int latency = 0, 
        bool plugDoesMidi = false, bool plugDoesChunks = false, 
        bool plugIsInst = false, int plugScChans = 0);
  
  void BeginInformHostOfParamChange(int idx);
  void InformHostOfParamChange(int idx, double normalizedValue);
  void EndInformHostOfParamChange(int idx);
  void InformHostOfProgramChange() { return; }
  
  int GetSamplePos();   // Samples since start of project.
  double GetTempo();
  void GetTimeSig(int* pNum, int* pDenom);  
  void GetTime(ITimeInfo* pTimeInfo);
  
  EHost GetHost();  // GetHostVersion() is inherited.

  void ResizeGraphics(int w, int h);
  
  void Created(class IPlugProcessRTAS *r);
  class IPlugProcessRTAS *mRTAS;
  
  void ProcessAudio(float** inputs, float** outputs, int nFrames);
  
  void SetNumInputs(int nInputs); 
  void SetNumOutputs(int nOutputs);
  void SetSideChainConnected(bool connected);
  
  void SetParameter(int idx); // Locks mutex first
  void SetBlockSize(int blockSize); // Public in IPlugRTAS, protected in IPlugBase

protected:
  virtual void HostSpecificInit() {};
  bool SendMidiMsg(IMidiMsg* pMsg);
  bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs);
  
private:
  bool mDoesMidi;
  bool mHasSideChain, mSideChainIsConnected;
  int mSideChainConnectionNum;
};

IPlugRTAS* MakePlug();

#endif