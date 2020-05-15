/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "AAX_CEffectParameters.h"

#include "AAX_IEffectDescriptor.h"
#include "AAX_IComponentDescriptor.h"
#include "AAX_IPropertyMap.h"

#include "AAX_CAtomicQueue.h"
#include "AAX_IParameter.h"
#include "AAX_IMIDINode.h"
#include "AAX_IString.h"

#include "IPlugPlatform.h"

#include <set>
#include <list>
#include <utility>

#define kMaxAdditionalMIDINodes 15
#define kMaxAuxOutputStems 32
#define kSynchronizedParameterQueueSize 32

BEGIN_IPLUG_NAMESPACE

struct AAX_SIPlugSetupInfo
{
  bool mNeedsGlobalMIDI;
  const char* mGlobalMIDINodeName;
  uint32_t mGlobalMIDIEventMask;
  bool mNeedsInputMIDI;
  const char* mInputMIDINodeName;
  uint32_t  mInputMIDIChannelMask;
  int32_t mNumAdditionalInputMIDINodes;
  bool mNeedsOutputMIDI;
  const char* mOutputMIDINodeName;
  uint32_t mOutputMIDIChannelMask;
  int32_t mNumAdditionalOutputMIDINodes;
  
  bool mNeedsTransport;
  const char* mTransportMIDINodeName;
  
  int32_t mNumMeters;
  const AAX_CTypeID* mMeterIDs;
  
  int32_t mNumAuxOutputStems;
  const char* mAuxOutputStemNames[kMaxAuxOutputStems];
  AAX_EStemFormat mAuxOutputStemFormats[kMaxAuxOutputStems];
  
  AAX_EStemFormat mHybridInputStemFormat;
  AAX_EStemFormat mHybridOutputStemFormat;
  
  //General Properties
  AAX_EStemFormat mInputStemFormat;
  AAX_EStemFormat mOutputStemFormat;
  bool mUseHostGeneratedGUI;
  bool mCanBypass;
  AAX_CTypeID mManufacturerID;
  AAX_CTypeID mProductID;
  AAX_CTypeID mPluginID;
  AAX_CTypeID mAudiosuiteID;
  AAX_CBoolean mMultiMonoSupport;
  
  int32_t mLatency;
  
  AAX_SIPlugSetupInfo()
  {
    mNeedsGlobalMIDI = false;
    mGlobalMIDINodeName = "GlobalMIDI";
    mGlobalMIDIEventMask = 0xffff;
    mNeedsInputMIDI = false;
    mInputMIDINodeName = "InputMIDI";
    mInputMIDIChannelMask = 0xffff;
    
    mNeedsOutputMIDI = false;
    mOutputMIDINodeName = "OutputMIDI";
    mOutputMIDIChannelMask = 0xffff;
    mNumAdditionalOutputMIDINodes = 0;
    mNumAdditionalInputMIDINodes = 0;
    mNeedsTransport = false;
    mTransportMIDINodeName = "Transport";
    mNumMeters = 0;
    mMeterIDs = 0;
    mInputStemFormat = AAX_eStemFormat_Mono;
    mOutputStemFormat = AAX_eStemFormat_Mono;
    mUseHostGeneratedGUI = false;
    mCanBypass = true;
    mManufacturerID = 'none';
    mProductID = 'none';
    mPluginID = 'none';
    mLatency = 0;
    mAudiosuiteID = 'none';
    mMultiMonoSupport = true;
        
    mNumAuxOutputStems = 0;
    for (int32_t i=0; i<kMaxAuxOutputStems; i++)
    {
      mAuxOutputStemNames[i] = 0;
      mAuxOutputStemFormats[i] = AAX_eStemFormat_Mono;
    }
    
    mHybridInputStemFormat = AAX_eStemFormat_None;
    mHybridOutputStemFormat = AAX_eStemFormat_None;
  }
};

class AAX_CIPlugParameters;

struct AAX_SIPlugPrivateData
{
  AAX_CIPlugParameters* mMonolithicParametersPtr;
};

struct AAX_SIPlugRenderInfo
{
  float** mAudioInputs;
  float** mAudioOutputs;
  int32_t* mNumSamples;
  AAX_CTimestamp* mClock;
  AAX_IMIDINode* mInputNode;
  AAX_IMIDINode* mOutputNode;
  AAX_IMIDINode* mGlobalNode;
  AAX_IMIDINode* mTransportNode;
  AAX_IMIDINode* mAdditionalInputMIDINodes[kMaxAdditionalMIDINodes];

  AAX_SIPlugPrivateData* mPrivateData;
  float** mMeters;
  
  int64_t* mCurrentStateNum;
};

class AAX_CIPlugParameters : public AAX_CEffectParameters
{
public:
  AAX_CIPlugParameters (void);
  ~AAX_CIPlugParameters (void) AAX_OVERRIDE;
 
protected:
  typedef std::pair<AAX_CParamID const, const AAX_IParameterValue*> TParamValPair;

  virtual void RenderAudio(AAX_SIPlugRenderInfo* ioRenderInfo, const TParamValPair* inSynchronizedParamValues[], int32_t inNumSynchronizedParamValues) {}
  
  /** @name Configuration methods
   */
  //@{
  /** Add a parameter for state synchronization
   *
   *  A parameter should be added for synchronization if:
   *  - It is important for the parameter's automation to be applied at the correct point on the timeline
   *  - It is possible to quickly update the plug-in's state to reflect a parameter change
   *
   *  See \ref advancedTopics_parameterUpdateTiming for more information
   *
   *  @param[in] inParameter
   *    The parameter to be synchronized. This string will be copied internally and is not required to persist
   */
  void        AddSynchronizedParameter(const AAX_IParameter& inParameter);
  //@}
  
public:
  AAX_Result UpdateParameterNormalizedValue(AAX_CParamID iParamID, double aValue, AAX_EUpdateSource inSource) AAX_OVERRIDE;
  AAX_Result GenerateCoefficients() AAX_OVERRIDE;
  AAX_Result ResetFieldData (AAX_CFieldIndex iFieldIndex, void * oData, uint32_t iDataSize) const AAX_OVERRIDE;
  AAX_Result TimerWakeup() AAX_OVERRIDE;

  static AAX_Result StaticDescribe (AAX_IEffectDescriptor * ioDescriptor, const AAX_SIPlugSetupInfo & setupInfo);

  static void AAX_CALLBACK  StaticRenderAudio(AAX_SIPlugRenderInfo* const  inInstancesBegin [], const void* inInstancesEnd);
private:
  // This structure is used on the render thread to set up the contiguous array of TParamValPair* values
  // which is passed to RenderAudio(). The values are drawn from lists of parameter value updates which
  // were queued during GenerateCoefficients()
  struct SParamValList
  {
    // Using 4x the preset queue size: the buffer must be large enough to accommodate the maximum
    // number of updates that we expect to be queued between/before executions of the render callback.
    // The maximum queuing that will likely ever occur is during a preset change (i.e. a call to
    // SetChunk()), in which updates to all parameters may be queued in the same state frame. It is
    // possible that the host would call SetChunk() on the plug-in more than once before the render
    // callback executes, but probably not more than 2-3x. Therefore 4x seems like a safe upper limit
    // for the capacity of this buffer.
    static const int32_t sCap = 4*kSynchronizedParameterQueueSize;
    
    TParamValPair* mElem[sCap];
    int32_t mSize;
    
    SParamValList()
    {
      Clear();
    }
    
    void Add(TParamValPair* inElem)
    {
      AAX_ASSERT(sCap > mSize);
      if (sCap > mSize)
      {
        mElem[mSize++] = inElem;
      }
    }
    
    void Append(const SParamValList& inOther)
    {
      AAX_ASSERT(sCap >= mSize + inOther.mSize);
      for (int32_t i = 0; i < inOther.mSize; ++i)
      {
        Add(inOther.mElem[i]);
      }
    }
    
    void Append(const std::list<TParamValPair*>& inOther)
    {
      AAX_ASSERT(sCap >= mSize + (int64_t)inOther.size());
      for (std::list<TParamValPair*>::const_iterator iter = inOther.begin(); iter != inOther.end(); ++iter)
      {
        Add(*iter);
      }
    }
    
    void Merge(AAX_IPointerQueue<TParamValPair>& inOther)
    {
      do
      {
        TParamValPair* const val = inOther.Pop();
        if (NULL == val) { break; }
        Add(val);
      } while (1);
    }
    
    void Clear() ///< Zeroes out the mElem array; does not destroy any elements
    {
      std::memset(mElem, 0x0, sizeof(mElem));
      mSize = 0;
    }
  };
  
  typedef std::set<const AAX_IParameter*> TParamSet;
  typedef std::pair<int64_t, std::list<TParamValPair*> > TNumberedParamStateList;
  typedef AAX_CAtomicQueue<TNumberedParamStateList, 256> TNumberedStateListQueue;
  typedef AAX_CAtomicQueue<const TParamValPair, 16*kSynchronizedParameterQueueSize> TParamValPairQueue;
  
  
  SParamValList GetUpdatesForState(int64_t inTargetStateNum);
  void DeleteUsedParameterChanges();
  
private:
  std::set<std::string> mSynchronizedParameters;
  int64_t mStateCounter;
  TParamSet mDirtyParameters;
  TNumberedStateListQueue mQueuedParameterChanges;
  TNumberedStateListQueue mFinishedParameterChanges; // Parameter changes ready for deletion
  TParamValPairQueue mFinishedParameterValues; // Parameter values ready for deletion
};

END_IPLUG_NAMESPACE

