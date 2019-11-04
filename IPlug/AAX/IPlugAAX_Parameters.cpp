/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugAAX_Parameters.h"

using namespace iplug;

AAX_Result AAX_CIPlugParameters::ResetFieldData(AAX_CFieldIndex iFieldIndex, void * oData, uint32_t iDataSize) const  //override from CEffectParameters.
{   
  //If this is the IPlugparameters field, let's initialize it to our this pointer.
  if (iFieldIndex == AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mPrivateData))
  {       
    //Make sure everything is at least initialized to 0.
    AAX_ASSERT(iDataSize == sizeof(AAX_SIPlugPrivateData));
    memset(oData, iDataSize, 0);

    //Set all of the private data variables.
    AAX_SIPlugPrivateData* privateData = static_cast <AAX_SIPlugPrivateData*> (oData);
    privateData->mIPlugParametersPtr = (AAX_CIPlugParameters*) this;

    return AAX_SUCCESS;
  }
  
  //Call into the base class to clear all other private data.
  return AAX_CEffectParameters::ResetFieldData(iFieldIndex, oData, iDataSize);
}

//StaticDescribeAlgorithm does all of the basic context setup and pointer passing work.  Call this from Describe.
AAX_Result  AAX_CIPlugParameters::StaticDescribe(AAX_IEffectDescriptor * ioDescriptor, const AAX_SIPlugSetupInfo & setupInfo)
{ 
  AAX_Result err = AAX_SUCCESS;
  AAX_IComponentDescriptor *  compDesc = ioDescriptor->NewComponentDescriptor ();
  
  // Register MIDI nodes. To avoid context corruption, register small blocks of private data in case node doesn't needed.
  AAX_CFieldIndex globalNodeID = AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mGlobalNode);
  AAX_CFieldIndex localInputNodeID = AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mInputNode);
  AAX_CFieldIndex localOutputNodeID = AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mOutputNode);
  AAX_CFieldIndex transportNodeID = AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mTransportNode);

  if (setupInfo.mNeedsGlobalMIDI)
    err |= compDesc->AddMIDINode(globalNodeID, AAX_eMIDINodeType_Global, setupInfo.mGlobalMIDINodeName, setupInfo.mGlobalMIDIEventMask);
  else
    err |= compDesc->AddPrivateData(globalNodeID, sizeof(float), AAX_ePrivateDataOptions_DefaultOptions);

  if (setupInfo.mNeedsInputMIDI)
    err |= compDesc->AddMIDINode(localInputNodeID, AAX_eMIDINodeType_LocalInput, setupInfo.mInputMIDINodeName, setupInfo.mInputMIDIChannelMask);
  else
    err |= compDesc->AddPrivateData(localInputNodeID, sizeof(float), AAX_ePrivateDataOptions_DefaultOptions);

  if (setupInfo.mNeedsOutputMIDI)
    err |= compDesc->AddMIDINode(localOutputNodeID, AAX_eMIDINodeType_LocalOutput, setupInfo.mOutputMIDINodeName, setupInfo.mOutputMIDIChannelMask);
  else
    err |= compDesc->AddPrivateData(localOutputNodeID, sizeof(float), AAX_ePrivateDataOptions_DefaultOptions);
  
  if (setupInfo.mNeedsTransport)
    err |= compDesc->AddMIDINode(transportNodeID, AAX_eMIDINodeType_Transport, "Transport", 0xffff);
  else
    err |= compDesc->AddPrivateData(transportNodeID, sizeof(float), AAX_ePrivateDataOptions_DefaultOptions);
  
  
//  AAX_ASSERT(setupInfo.mNumAdditionalInputMIDINodes <= kMaxAdditionalMIDINodes);
//    for (int32_t index=0; index < kMaxAdditionalMIDINodes; index++)
//    {      
//        AAX_CFieldIndex nodeID = AAX_FIELD_INDEX(AAX_SInstrumentRenderInfo, mAdditionalInputMIDINodes[index]);
//        AAX_CString nodeName(setupInfo.mInputMIDINodeName);
//        nodeName.AppendNumber(index+1);
//        if (index < setupInfo.mNumAdditionalInputMIDINodes)
//            err |= compDesc->AddMIDINode(nodeID, AAX_eMIDINodeType_LocalInput, nodeName.CString(), setupInfo.mInputMIDIChannelMask);
//        else
//            err |= compDesc->AddPrivateData(nodeID, sizeof(float), AAX_ePrivateDataOptions_DefaultOptions);	//Just here to fill the port.  Not used.
//    }
  
  //Add outputs, meters, info, etc
  err |= compDesc->AddAudioIn(AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mAudioInputs));
  err |= compDesc->AddAudioOut(AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mAudioOutputs));
  err |= compDesc->AddAudioBufferLength(AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mNumSamples));
  if (setupInfo.mNumMeters > 0)
    err |= compDesc->AddMeters(AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mMeters), setupInfo.mMeterIDs, setupInfo.mNumMeters);
  else
    err |= compDesc->AddPrivateData(AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mMeters), sizeof(float), AAX_ePrivateDataOptions_DefaultOptions);
  
  //Add optional aux output stems.
  for (int32_t index=0; index < setupInfo.mNumAuxOutputStems; index++)
  {
    err |= compDesc->AddAuxOutputStem (0 /*not used*/, setupInfo.mAuxOutputStemFormats[index], setupInfo.mAuxOutputStemNames[index]);
  }
  
  //Add pointer to the data model instance and other interesting information.
  err |= compDesc->AddPrivateData(AAX_FIELD_INDEX(AAX_SIPlugRenderInfo, mPrivateData), sizeof(AAX_SIPlugPrivateData), AAX_ePrivateDataOptions_DefaultOptions);
  
  //Additional properties on the algorithm
  AAX_IPropertyMap *  properties = compDesc->NewPropertyMap();

  // Host generated GUI or not.
  if (setupInfo.mUseHostGeneratedGUI)
    properties->AddProperty(AAX_eProperty_UsesClientGUI, true);
  
  // initial latency
  err |= properties->AddProperty(AAX_eProperty_LatencyContribution, setupInfo.mLatency);

  err |= properties->AddProperty(AAX_eProperty_InputStemFormat, setupInfo.mInputStemFormat);
  err |= properties->AddProperty(AAX_eProperty_OutputStemFormat, setupInfo.mOutputStemFormat);
  err |= properties->AddProperty(AAX_eProperty_CanBypass, setupInfo.mCanBypass);
  err |= properties->AddProperty(AAX_eProperty_ManufacturerID, setupInfo.mManufacturerID);
  err |= properties->AddProperty(AAX_eProperty_ProductID, setupInfo.mProductID);
  err |= properties->AddProperty(AAX_eProperty_PlugInID_RTAS, setupInfo.mPluginID);        //This is a native only convenience layer, so there is no need for a DSP type.
  
  if (setupInfo.mAudioSuiteID != 'none') {
    err |= properties->AddProperty(AAX_eProperty_PlugInID_AudioSuite, setupInfo.mAudioSuiteID);
  }
  
  err |= compDesc->AddProcessProc_Native(AAX_CIPlugParameters::StaticRenderAudio, properties);
  err |= ioDescriptor->AddComponent(compDesc);
  
  return err;
} 

//Static RenderAudio  ( This version would only work for non-distributed algorithms. )
void  AAX_CALLBACK  AAX_CIPlugParameters::StaticRenderAudio(AAX_SIPlugRenderInfo* const inInstancesBegin [], const void* inInstancesEnd)
{ 
  for (AAX_SIPlugRenderInfo * const * instanceRenderInfoPtr = inInstancesBegin; instanceRenderInfoPtr != inInstancesEnd; ++instanceRenderInfoPtr)
  {
    AAX_SIPlugPrivateData*  privateData = (*instanceRenderInfoPtr)->mPrivateData;
    if (privateData != 0)
    {
      //Grab the object pointer from the Context and call RenderAudio on it.
      AAX_CIPlugParameters* parameters = privateData->mIPlugParametersPtr;
      if (parameters != 0)
        parameters->RenderAudio(*instanceRenderInfoPtr); 
    }
  }
}
