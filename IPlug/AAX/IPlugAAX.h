/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifndef _IPLUGAPI_
#define _IPLUGAPI_

/**
 * @file
 * @copydoc IPlugAAX
 */


#include "IPlugPlatform.h"
#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"
#include "IPlugMidi.h"

#include "IPlugAAX_Parameters.h"

#include "AAX_CEffectGUI.h"

#include "AAX_Push8ByteStructAlignment.h"

#if defined OS_WIN
  #if defined _DEBUG
    #if defined ARCH_64BIT
      #pragma comment(lib, "AAXLibrary_x64_D.lib")
    #else
      #pragma comment(lib, "AAXLibrary_D.lib")
    #endif
  #else
  #if defined ARCH_64BIT
    #pragma comment(lib, "AAXLibrary_x64.lib")
  #else
    #pragma comment(lib, "AAXLibrary.lib")
  #endif
  #endif
#endif

BEGIN_IPLUG_NAMESPACE

const int kAAXParamIdxOffset = 1;

/** Used to pass various instance info to the API class */
struct InstanceInfo {};

class IPlugAAX;

/**  AAX_CEffectGUI base class for an IPlug AAX view */
class AAX_CEffectGUI_IPLUG : public AAX_CEffectGUI
{
public:
  AAX_CEffectGUI_IPLUG() {}
  ~AAX_CEffectGUI_IPLUG() {}
  static AAX_IEffectGUI* AAX_CALLBACK Create();
  AAX_Result SetControlHighlightInfo(AAX_CParamID iParameterID, AAX_CBoolean iIsHighlighted, AAX_EHighlightColor iColor);
  
private:
  void CreateViewContents();
  void CreateViewContainer();
  void DeleteViewContainer();
  AAX_Result GetViewSize(AAX_Point *oEffectViewSize) const;
  AAX_Result ParameterUpdated (const char* iParameterID);
private:
  IPlugAAX* mPlug = nullptr;
};

/**  AAX API base class for an IPlug plug-in
*   @ingroup APIClasses */
class IPlugAAX : public IPlugAPIBase
               , public IPlugProcessor
               , public AAX_CIPlugParameters
{
public:
  IPlugAAX(const InstanceInfo& info, const Config& config);
  ~IPlugAAX();
  
  //IPlugAPIBase Overrides
  void BeginInformHostOfParamChange(int idx) override;
  void InformHostOfParamChange(int idx, double normalizedValue) override;
  void EndInformHostOfParamChange(int idx) override;
  
  void InformHostOfProgramChange() override { }; //NA
  
  bool EditorResizeFromDelegate(int viewWidth, int viewHeight) override;
  
  //IPlug Processor Overrides
  void SetLatency(int samples) override;
  bool SendMidiMsg(const IMidiMsg& msg) override;
  
  AAX_Result UpdateParameterNormalizedValue(AAX_CParamID iParameterID, double iValue, AAX_EUpdateSource iSource) override;
  
  //AAX_CIPlugParameters Overrides
  static AAX_CEffectParameters *AAX_CALLBACK Create();
  AAX_Result EffectInit() override;
  void RenderAudio(AAX_SIPlugRenderInfo* ioRenderInfo) override;
  
  //AAX_CEffectParameters Overrides
  AAX_Result GetChunkIDFromIndex(int32_t index, AAX_CTypeID* pChunkID) const override;
  AAX_Result GetChunkSize(AAX_CTypeID chunkID, uint32_t* pChunkSize) const override;
  AAX_Result GetChunk(AAX_CTypeID chunkID, AAX_SPlugInChunk* pChunk) const override;
  AAX_Result SetChunk(AAX_CTypeID chunkID, const AAX_SPlugInChunk* pChunk) override;
  AAX_Result CompareActiveChunk(const AAX_SPlugInChunk* pChunk, AAX_CBoolean* pIsEqual) const override;

  //IPlugAAX
  /** This is needed in chunks based plug-ins to tell PT a non-indexed param changed and to turn on the compare light. You can call this method from your plug-in implementation by doing a dynamic_cast in order to convert an "IPlug" into a "IPlugAAX"
   */
  void DirtyPTCompareState() { mNumPlugInChanges++; }

private:
  AAX_CParameter<bool>* mBypassParameter = nullptr;
  AAX_ITransport* mTransport = nullptr;
  WDL_PtrList<WDL_String> mParamIDs;
  IMidiQueue mMidiOutputQueue;
};

IPlugAAX* MakePlug(const InstanceInfo& info);

#include "AAX_PopStructAlignment.h"

END_IGRAPHICS_NAMESPACE

#endif
