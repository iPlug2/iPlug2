#ifndef _IPLUGAPI_
#define _IPLUGAPI_

#include "IPlugOSDetect.h"
#include "IPlugBase.h"
#include "IGraphics.h"

#include "AAX_CIPlugParameters.h"
#include "AAX_CEffectGUI.h"

#include "AAX_Push8ByteStructAlignment.h"

const int kAAXParamIdxOffset = 1;

struct IPlugInstanceInfo
{
  // not used
};

class AAX_CEffectGUI_IPLUG : public AAX_CEffectGUI
{
public:
  AAX_CEffectGUI_IPLUG() {}
  ~AAX_CEffectGUI_IPLUG() {}
  static AAX_IEffectGUI* AAX_CALLBACK Create();
  
private:
  void CreateViewContents();
  void CreateViewContainer();
  void DeleteViewContainer();
  AAX_Result GetViewSize ( AAX_Point *oEffectViewSize ) const;
  AAX_Result ParameterUpdated (const char* iParameterID);
  
private:
  IGraphics* mGraphics;
};

class IPlugAAX : public IPlugBase, 
                 public AAX_CIPlugParameters
{
public:

  IPlugAAX(IPlugInstanceInfo instanceInfo, 
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
  
  ~IPlugAAX();
  
  AAX_Result UpdateParameterNormalizedValue(AAX_CParamID iParameterID, double iValue, AAX_EUpdateSource iSource );
  
  // AAX_CIPlugParameters Overrides
  static AAX_CEffectParameters *AAX_CALLBACK Create();
  AAX_Result EffectInit();
  void RenderAudio(AAX_SIPlugRenderInfo* ioRenderInfo);
  
  // AAX_CEffectParameters Overrides
  AAX_Result GetChunkIDFromIndex(int32_t index, AAX_CTypeID * chunkID ) const;
  AAX_Result GetChunkSize(AAX_CTypeID chunkID, uint32_t * oSize ) const ;
  AAX_Result GetChunk(AAX_CTypeID chunkID, AAX_SPlugInChunk * oChunk ) const ;   
  AAX_Result SetChunk(AAX_CTypeID chunkID, const AAX_SPlugInChunk * iChunk );
  AAX_Result CompareActiveChunk(const AAX_SPlugInChunk * iChunk, AAX_CBoolean * oIsEqual )  const ;
  
  // IPlugBase Overrides
  void BeginInformHostOfParamChange(int idx);
  void InformHostOfParamChange(int idx, double normalizedValue);
  void EndInformHostOfParamChange(int idx);
  void InformHostOfProgramChange() { }; //NA
      
  int GetSamplePos();
  double GetTempo();
  void GetTimeSig(int* pNum, int* pDenom);
  void GetTime(ITimeInfo* pTimeInfo);

  void ResizeGraphics(int w, int h);

  void SetLatency(int samples);
  void DirtyPTCompareState() { mNumPlugInChanges++; }
  
protected:
  bool SendMidiMsg(IMidiMsg* pMsg);

private:
  AAX_CParameter<bool>* mBypassParameter;
  AAX_ITransport* mTransport;
  WDL_PtrList<WDL_String> mParamIDs;
};

IPlugAAX* MakePlug();

#include "AAX_PopStructAlignment.h"

#endif