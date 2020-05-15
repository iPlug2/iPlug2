/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugAAX_Parameters.h"

using namespace iplug;

#include "IPlugAAX_Parameters.h"

#include "AAX_Exception.h"


AAX_CIPlugParameters::AAX_CIPlugParameters (void)
: AAX_CEffectParameters()
, mSynchronizedParameters()
, mStateCounter(0)
, mDirtyParameters()
, mQueuedParameterChanges()
, mFinishedParameterChanges()
, mFinishedParameterValues()
{
  
}

AAX_CIPlugParameters::~AAX_CIPlugParameters(void)
{
  // Clear the used parameter changes
  DeleteUsedParameterChanges();
}

void AAX_CIPlugParameters::AddSynchronizedParameter(const AAX_IParameter& inParameter)
{
  mSynchronizedParameters.insert(inParameter.Identifier());
  AAX_ASSERT(inParameter.Automatable()); // No point in synchronizing non-automatable parameters
  AAX_ASSERT(kSynchronizedParameterQueueSize >= mSynchronizedParameters.size()); // Avoid buffer overflows
}

AAX_Result AAX_CIPlugParameters::UpdateParameterNormalizedValue(AAX_CParamID iParamID, double aValue, AAX_EUpdateSource inSource)
{
  // Call inherited
  AAX_Result result = AAX_CEffectParameters::UpdateParameterNormalizedValue(iParamID, aValue, inSource);
  if (AAX_SUCCESS != result) { return result; }
  
  const AAX_IParameter* const param = mParameterManager.GetParameterByID(iParamID);
  
  // Defer updating the algorithm with the state of
  // automatable parameters in the synchronization set
  if ((param) && (0 < mSynchronizedParameters.count(iParamID)))
  {
    mDirtyParameters.insert(param);
  }
  
  return result;
}

AAX_Result AAX_CIPlugParameters::GenerateCoefficients()
{
  // Call inherited
  AAX_Result result = AAX_CEffectParameters::GenerateCoefficients();
  if (AAX_SUCCESS != result) { return result; }
  
  const int64_t stateNum = mStateCounter++;
  
  // Check for "dirty" parameters and create a list of these
  // parameters with their values
  TNumberedParamStateList::second_type paramStateList;
  while (false == mDirtyParameters.empty())
  {
    TParamSet::iterator paramIter = mDirtyParameters.begin();
    const AAX_IParameter* const param = *paramIter;
    mDirtyParameters.erase(param);
    if (NULL != param)
    {
      paramStateList.push_back(new TParamValPair(param->Identifier(), param->CloneValue()));
    }
  }
  
  if (false == paramStateList.empty())
  {
    TNumberedParamStateList* const numberedParamState = new TNumberedParamStateList(std::make_pair(stateNum, paramStateList));
    {
      // Use a single atomic operation to add all states for this state number; this prevents
      // the render thread from coming along in the middle of a push and picking up only part
      // of the total state change.
      const AAX_IContainer::EStatus pushResult = mQueuedParameterChanges.Push(numberedParamState);
      AAX_ASSERT(AAX_IContainer::eStatus_Success == pushResult);
    }
  }
  
  result = Controller()->PostPacket(AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mCurrentStateNum), &stateNum, sizeof(int64_t));
  
  return result;
}

AAX_Result AAX_CIPlugParameters::ResetFieldData (AAX_CFieldIndex iFieldIndex, void * oData, uint32_t iDataSize) const  //override from CEffectParameters.
{
  //If this is the MonolithicParameters field, let's initialize it to our this pointer.
  if (iFieldIndex == AAX_FIELD_INDEX (AAX_SIPlugRenderInfo, mPrivateData) )
  {
    //Make sure everything is at least initialized to 0.
    AAX_ASSERT( iDataSize == sizeof(AAX_SIPlugPrivateData) );
    memset(oData, 0, iDataSize);

    //Set all of the private data variables.
    AAX_SIPlugPrivateData* privateData = static_cast <AAX_SIPlugPrivateData*> (oData);
    privateData->mMonolithicParametersPtr = (AAX_CIPlugParameters*) this;
    return AAX_SUCCESS;
  }
  
  //Call into the base class to clear all other private data.
  return AAX_CEffectParameters::ResetFieldData(iFieldIndex, oData, iDataSize);
}

AAX_Result AAX_CIPlugParameters::TimerWakeup()
{
  // Clear the used parameter changes
  DeleteUsedParameterChanges();
  
  return AAX_CEffectParameters::TimerWakeup();
}

//StaticDescribe does all of the basic context setup and pointer passing work.  Call this from Describe.
AAX_Result  AAX_CIPlugParameters::StaticDescribe(AAX_IEffectDescriptor * ioDescriptor, const AAX_SIPlugSetupInfo & setupInfo)
{
  AAX_CheckedResult err;
  AAX_IComponentDescriptor* const compDesc = ioDescriptor->NewComponentDescriptor ();
  if (!compDesc)
    err = AAX_ERROR_NULL_OBJECT;

  // Register MIDI nodes. To avoid context corruption, register small blocks of private data for fields where a node is not needed
  AAX_CFieldIndex globalNodeID = AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mGlobalNode);
  AAX_CFieldIndex localInputNodeID = AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mInputNode);
  AAX_CFieldIndex transportNodeID = AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mTransportNode);

  if (setupInfo.mNeedsGlobalMIDI)
    err = compDesc->AddMIDINode ( globalNodeID, AAX_eMIDINodeType_Global, setupInfo.mGlobalMIDINodeName, setupInfo.mGlobalMIDIEventMask );
  else
    err = compDesc->AddPrivateData( globalNodeID, sizeof(float), AAX_ePrivateDataOptions_DefaultOptions ); //Just here to fill the port.  Not used.

  if (setupInfo.mNeedsInputMIDI)
    err = compDesc->AddMIDINode ( localInputNodeID, AAX_eMIDINodeType_LocalInput, setupInfo.mInputMIDINodeName, setupInfo.mInputMIDIChannelMask );
  else
    err = compDesc->AddPrivateData( localInputNodeID, sizeof(float), AAX_ePrivateDataOptions_DefaultOptions );     //Just here to fill the port.  Not used.

  if (setupInfo.mNeedsTransport)
    err = compDesc->AddMIDINode ( transportNodeID, AAX_eMIDINodeType_Transport, "Transport", 0xffff );
  else
    err = compDesc->AddPrivateData( transportNodeID, sizeof(float), AAX_ePrivateDataOptions_DefaultOptions );  //Just here to fill the port.  Not used.
  
    AAX_ASSERT(setupInfo.mNumAdditionalInputMIDINodes <= kMaxAdditionalMIDINodes);
    for (int32_t index = 0; index < kMaxAdditionalMIDINodes; index++)
    {
        AAX_CFieldIndex nodeID = index + ((AAX_CFieldIndex) (offsetof (AAX_SIPlugRenderInfo, mAdditionalInputMIDINodes) / sizeof (void *)));
        AAX_CString nodeName(setupInfo.mInputMIDINodeName);

        if (index < setupInfo.mNumAdditionalInputMIDINodes)
            err = compDesc->AddMIDINode ( nodeID, AAX_eMIDINodeType_LocalInput, nodeName.CString(), setupInfo.mInputMIDIChannelMask );
        else
            err = compDesc->AddPrivateData( nodeID, sizeof(float), AAX_ePrivateDataOptions_DefaultOptions );  //Just here to fill the port.  Not used.
    }
    
  //Add outputs, meters, info, etc
  err = compDesc->AddAudioIn( AAX_FIELD_INDEX (AAX_SIPlugRenderInfo, mAudioInputs) );
  err = compDesc->AddAudioOut( AAX_FIELD_INDEX (AAX_SIPlugRenderInfo, mAudioOutputs) );
  err = compDesc->AddAudioBufferLength( AAX_FIELD_INDEX (AAX_SIPlugRenderInfo, mNumSamples) );
    err = compDesc->AddClock( AAX_FIELD_INDEX (AAX_SIPlugRenderInfo, mClock) );
  if (setupInfo.mNumMeters > 0)
    err = compDesc->AddMeters( AAX_FIELD_INDEX (AAX_SIPlugRenderInfo, mMeters), setupInfo.mMeterIDs, static_cast<uint32_t>(setupInfo.mNumMeters) );
  else
    err = compDesc->AddPrivateData( AAX_FIELD_INDEX (AAX_SIPlugRenderInfo, mMeters), sizeof(float), AAX_ePrivateDataOptions_DefaultOptions );  //Just here to fill the port.  Not used.
  
    //Add optional aux output stems.
  // NOTE: This will throw for hosts that do not support AOS, which will result in the
  // Effect not being registered. An alternative would be to detect the lack of AOS
  // support and to communicate this to the effect so that it could adjust its
  // algorithm logic accordingly.
    AAX_ASSERT(setupInfo.mNumAuxOutputStems <= kMaxAuxOutputStems);
  for (int32_t index=0; index < setupInfo.mNumAuxOutputStems; index++)
  {
    err = compDesc->AddAuxOutputStem (0 /*not used*/, static_cast<int32_t>(setupInfo.mAuxOutputStemFormats[index]), setupInfo.mAuxOutputStemNames[index] );
  }
  
  //Add pointer to the data model instance and other interesting information.
  err = compDesc->AddPrivateData( AAX_FIELD_INDEX (AAX_SIPlugRenderInfo, mPrivateData), sizeof(AAX_SIPlugPrivateData), AAX_ePrivateDataOptions_DefaultOptions );
  
  //Add a "state number" counter for deferred parameter updates
  err = compDesc->AddDataInPort ( AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mCurrentStateNum), sizeof(uint64_t));
  
  //Additional properties on the algorithm
  AAX_IPropertyMap* const properties = compDesc->NewPropertyMap ();
  if (!properties)
    err = AAX_ERROR_NULL_OBJECT;

    // Host generated GUI or not.
    if (setupInfo.mUseHostGeneratedGUI)
        err = properties->AddProperty ( AAX_eProperty_UsesClientGUI, true );
  
  // AAX Hybrid
  AAX_ASSERT((AAX_eStemFormat_None != setupInfo.mHybridInputStemFormat) == (AAX_eStemFormat_None != setupInfo.mHybridOutputStemFormat));
  if ((AAX_eStemFormat_None != setupInfo.mHybridInputStemFormat) && (AAX_eStemFormat_None != setupInfo.mHybridOutputStemFormat))
  {
    err = properties->AddProperty(AAX_eProperty_HybridInputStemFormat, static_cast<int32_t>(setupInfo.mHybridInputStemFormat));
    err = properties->AddProperty(AAX_eProperty_HybridOutputStemFormat, static_cast<int32_t>(setupInfo.mHybridOutputStemFormat));
  }
  
  // initial latency
  err = properties->AddProperty(AAX_eProperty_LatencyContribution, setupInfo.mLatency);
  err = properties->AddProperty ( AAX_eProperty_InputStemFormat, static_cast<int32_t>(setupInfo.mInputStemFormat) );    // maybe this should be none?
  err = properties->AddProperty ( AAX_eProperty_OutputStemFormat, static_cast<int32_t>(setupInfo.mOutputStemFormat) );
  err = properties->AddProperty ( AAX_eProperty_CanBypass, setupInfo.mCanBypass );
  err = properties->AddProperty ( AAX_eProperty_Constraint_Location, 0x0 | AAX_eConstraintLocationMask_DataModel ); // The alg proc must be co-located with the effect data model
  if (setupInfo.mNeedsTransport) {
    err = properties->AddProperty ( AAX_eProperty_UsesTransport, true);
  }
  err = properties->AddProperty ( AAX_eProperty_ManufacturerID, static_cast<int32_t>(setupInfo.mManufacturerID) );
  err = properties->AddProperty ( AAX_eProperty_ProductID, static_cast<int32_t>(setupInfo.mProductID) );
  err = properties->AddProperty ( AAX_eProperty_PlugInID_RTAS, static_cast<int32_t>(setupInfo.mPluginID) );        //This is a native only convenience layer, so there is no need for a DSP type.
  if (setupInfo.mAudiosuiteID != 'none')
    err = properties->AddProperty ( AAX_eProperty_PlugInID_AudioSuite, static_cast<int32_t>(setupInfo.mAudiosuiteID) );
  if (!setupInfo.mMultiMonoSupport)
    err = properties->AddProperty(AAX_eProperty_Constraint_MultiMonoSupport, 0);
  err = compDesc->AddProcessProc_Native( AAX_CIPlugParameters::StaticRenderAudio, properties );
  err = ioDescriptor->AddComponent( compDesc );
  
  return err;
}

//Static RenderAudio  ( This version would only work for non-distributed algorithms. )
void  AAX_CALLBACK  AAX_CIPlugParameters::StaticRenderAudio(AAX_SIPlugRenderInfo* const  inInstancesBegin [], const void* inInstancesEnd)
{
  for (AAX_SIPlugRenderInfo * const * instanceRenderInfoPtr = inInstancesBegin; instanceRenderInfoPtr != inInstancesEnd; ++instanceRenderInfoPtr)
  {
    AAX_SIPlugPrivateData*  privateData = (*instanceRenderInfoPtr)->mPrivateData;
    if (privateData != 0)
    {
      //Grab the object pointer from the Context
      AAX_CIPlugParameters*  parameters = privateData->mMonolithicParametersPtr;
      if (parameters != 0)
      {
        // Update synchronized parameters to the target state num and get a queue of parameter value changes
        SParamValList paramValList = parameters->GetUpdatesForState(*(*instanceRenderInfoPtr)->mCurrentStateNum);
        
        // Call RenderAudio
        parameters->RenderAudio(*instanceRenderInfoPtr, (const TParamValPair**)paramValList.mElem, paramValList.mSize);
        
        // Queue the parameter value pairs for later deletion
        for (int32_t i = 0; i < paramValList.mSize; ++i)
        {
          const AAX_IContainer::EStatus pushResult = parameters->mFinishedParameterValues.Push(paramValList.mElem[i]);
          AAX_ASSERT(AAX_IContainer::eStatus_Success == pushResult);
        }
      }
    }
  }
}

AAX_CIPlugParameters::SParamValList AAX_CIPlugParameters::GetUpdatesForState(int64_t inTargetStateNum)
{
  SParamValList paramValList;
  TNumberedStateListQueue stateLists;
  
  for (// Initialization
     TNumberedParamStateList* numberedStateList = mQueuedParameterChanges.Peek();
     
     // Condition
     (NULL != numberedStateList) && // there is an element in the queue
     (  (numberedStateList->first <= inTargetStateNum) || // next queued state is before or equal to target state
      ((-0xFFFF > inTargetStateNum) && (0xFFFF < numberedStateList->first)) // target state counter has wrapped around
      );
     
     // Increment
     numberedStateList = mQueuedParameterChanges.Peek()
     )
  {
    // We'll use this state, so pop it from the queue
    const TNumberedParamStateList* const poppedPair = mQueuedParameterChanges.Pop();
    AAX_ASSERT(poppedPair == numberedStateList); // We can trust that this will match because there is only one thread calling Pop() on mQueuedParameterChanges
    const AAX_IContainer::EStatus pushResult = stateLists.Push(numberedStateList);
    AAX_ASSERT(AAX_IContainer::eStatus_Success == pushResult);
    numberedStateList = mQueuedParameterChanges.Peek();
  }
  
  // Transfer all parameter states into the single SParamValList
  for (TNumberedParamStateList* numberedStateList = stateLists.Pop(); NULL != numberedStateList; numberedStateList = stateLists.Pop())
  {
    paramValList.Append(numberedStateList->second);
    numberedStateList->second.clear(); // Ownership of all elements has been transferred to paramValList
    
    // Queue the now-empty container for later deletion
    mFinishedParameterChanges.Push(numberedStateList);
  }
  
  return paramValList;
}

void AAX_CIPlugParameters::DeleteUsedParameterChanges()
{
  // Deep-delete all elements from the used parameter change queue
  for (TNumberedParamStateList* numberedStateList = mFinishedParameterChanges.Pop(); NULL != numberedStateList; numberedStateList = mFinishedParameterChanges.Pop())
  {
    TNumberedParamStateList::second_type& curStateList = numberedStateList->second;
    for (std::list<TParamValPair*>::const_iterator iter = curStateList.begin(); iter != curStateList.end(); ++iter)
    {
      if (*iter) { delete *iter; }
    }
    delete numberedStateList;
  }
  
  // Delete all used parameter values
  for (const TParamValPair* paramVal = mFinishedParameterValues.Pop(); NULL != paramVal; paramVal = mFinishedParameterValues.Pop())
  {
    if (paramVal) { delete paramVal; }
  }
}
