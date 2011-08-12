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
  
  // ----------------------------------------
  // See IPlugBase for the full list of methods that your plugin class can implement.
  
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
               bool *pTransportRunning,bool *pTransportCycle) { return; };
  
  // Tell the host that the graphics resized.
  // Should be called only by the graphics object when it resizes itself.
  void ResizeGraphics(int w, int h);
  
  void Created(class IPlugProcessRTAS *r);
  class IPlugProcessRTAS *mRTAS;
  
  void ProcessAudio(float** inputs, float** outputs, float** sidechain, int nFrames);
  
  void SetNumInputs(int nInputs); 
  void SetNumOutputs(int nOutputs);
  void SetNumSideChainInputs(int nSideChainInputs);
  void SetSideChainConnectionNum(int connectionNum);

  int GetNumInputs() { return mNumInputs; }
  int GetNumOutputs() { return mNumOutputs; }
  int GetNumSideChainInputs() { return mNumSideChainInputs; }
  int GetSideChainConnectionNum() { return mSideChainConnectionNum; }

  bool PluginDoesStateChunks() { return DoesStateChunks(); }
  
  void SetBlockSize(int blockSize); // Public in IPlugRTAS, protected in IPlugBase

protected:
  void HostSpecificInit();
  void AttachGraphics(IGraphics* pGraphics);  
  void SetLatency(int samples);
  bool SendMidiMsg(IMidiMsg* pMsg);
  bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs);
  
private:
  bool mDoesMidi;
  int mNumInputs;
  int mNumOutputs;
  int mNumSideChainInputs;
  int mSideChainConnectionNum;
};

IPlugRTAS* MakePlug();

#endif