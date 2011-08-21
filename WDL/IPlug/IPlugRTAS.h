#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

#include "IPlugBase.h"

const int kPTParamIdxOffset = 2;

struct IPlugInstanceInfo
{
  int magic;
};

class IPlugRTAS : public IPlugBase
{
public:
  
  // Use IPLUG_CTOR instead of calling directly (defined in IPlug_include_in_plug_src.h).
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
  EHost GetHost();  // GetHostVersion() is inherited.
  
  void GetTime(double *pSamplePos, double *pTempo, 
               double *pMusicalPos, double *pLastBar,
               int* pNum, int* pDenom,
               double *pCycleStart,double *pCycleEnd,
               bool *pTransportRunning,bool *pTransportCycle);
  
  void ResizeGraphics(int w, int h);
  
  void Created(class IPlugProcessRTAS *r);
  class IPlugProcessRTAS *mRTAS;
  
  void ProcessAudio(float** inputs, float** outputs, int nFrames);
  
  void SetNumInputs(int nInputs); 
  void SetNumOutputs(int nOutputs);
  void SetSideChainConnected(bool connected);

  bool PluginDoesStateChunks() { return DoesStateChunks(); }
  
  void SetBlockSize(int blockSize); // Public in IPlugRTAS, protected in IPlugBase

protected:
  virtual void HostSpecificInit() {};
  void AttachGraphics(IGraphics* pGraphics);  
  bool SendMidiMsg(IMidiMsg* pMsg);
  bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs);
  
private:
  bool mDoesMidi;
  bool mHasSideChain, mSideChainIsConnected;
  int mSideChainConnectionNum;
};

IPlugRTAS* MakePlug();

#endif