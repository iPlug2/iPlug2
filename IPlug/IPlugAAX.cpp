#include "IPlugAAX.h"
#include "IPlugAAX_view_interface.h"
#include "AAX_CBinaryTaperDelegate.h"
#include "AAX_CBinaryDisplayDelegate.h"
#include "AAX_CStringDisplayDelegate.h"
#include "AAX_CLinearTaperDelegate.h"

// custom taper for IParam::kTypeDouble
#include "IPlugAAX_TaperDelegate.h"
#include "AAX_CNumberDisplayDelegate.h"
#include "AAX_CUnitDisplayDelegateDecorator.h"

AAX_CEffectParameters *AAX_CALLBACK IPlugAAX::Create()
{
  return MakePlug();
}

void AAX_CEffectGUI_IPLUG::CreateViewContents() 
{
  TRACE;
  mPlug = dynamic_cast<IPlugAAX*>(GetEffectParameters());  
}

void AAX_CEffectGUI_IPLUG::CreateViewContainer()
{
  TRACE;
  
  void* pWindow = GetViewContainerPtr();
  
  if (pWindow && mPlug->HasUI())
  {
    IPlugAAXView_Interface* pViewInterface = (IPlugAAXView_Interface*) mPlug->GetAAXViewInterface();
    
    if(pViewInterface)
      pViewInterface->SetViewContainer(GetViewContainer());
    
    mPlug->OpenWindow(pWindow);
  }
}

void AAX_CEffectGUI_IPLUG::DeleteViewContainer() 
{
  mPlug->CloseWindow();
}

AAX_Result AAX_CEffectGUI_IPLUG::GetViewSize(AAX_Point *oEffectViewSize) const
{
  if (mPlug->HasUI())
  {
    oEffectViewSize->horz = (float) mPlug->Width();
    oEffectViewSize->vert = (float) mPlug->Height();
  }
  
  return AAX_SUCCESS; 
}

AAX_Result AAX_CEffectGUI_IPLUG::ParameterUpdated(const char* paramID)
{
  return AAX_SUCCESS;
} 

AAX_IEffectGUI* AAX_CALLBACK AAX_CEffectGUI_IPLUG::Create()
{
  return new AAX_CEffectGUI_IPLUG;
}

AAX_Result AAX_CEffectGUI_IPLUG::SetControlHighlightInfo(AAX_CParamID paramID, AAX_CBoolean iIsHighlighted, AAX_EHighlightColor iColor)
{
  IPlugAAXView_Interface* pViewInterface = (IPlugAAXView_Interface*) mPlug->GetAAXViewInterface();
  
  if (pViewInterface)
  {
    int paramIdx = atoi(paramID) - kAAXParamIdxOffset;

    pViewInterface->SetPTParameterHighlight(paramIdx, (bool) iIsHighlighted, (int) iColor);
    return AAX_SUCCESS;
  }
  
  return AAX_ERROR_INVALID_PARAMETER_ID;
}

#pragma mark IPlugAAX Construct

IPlugAAX::IPlugAAX(IPlugInstanceInfo instanceInfo, IPlugConfig c)
: IPLUG_BASE_CLASS(c, kAPIAAX)
, IPlugProcessor<PLUG_SAMPLE_DST>(c, kAPIAAX)
, IPlugPresetHandler(c, kAPIAAX)
{
  AttachPresetHandler(this);

  Trace(TRACELOC, "%s%s", c.pluginName, c.channelIOStr);

  _SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
  _SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true);
  
  if (MaxNChannels(ERoute::kInput)) 
  {
    mLatencyDelay = new NChanDelayLine<PLUG_SAMPLE_DST>(MaxNChannels(ERoute::kInput), MaxNChannels(ERoute::kOutput));
    mLatencyDelay->SetDelayTime(c.latency);
  }
  
  _SetBlockSize(DEFAULT_BLOCK_SIZE);
  SetHost("ProTools", 0); // TODO:vendor version correct?
}

IPlugAAX::~IPlugAAX()
{
  mParamIDs.Empty(true);
}

AAX_Result IPlugAAX::EffectInit()
{ 
  TRACE;

  AAX_CString bypassID = NULL;
  this->GetMasterBypassParameter( &bypassID );
  mBypassParameter = new AAX_CParameter<bool>(bypassID.CString(), 
                                              AAX_CString("Master Bypass"), 
                                              false, 
                                              AAX_CBinaryTaperDelegate<bool>(),
                                              AAX_CBinaryDisplayDelegate<bool>("bypass", "on"), 
                                              true);
  mBypassParameter->SetNumberOfSteps( 2 );
  mBypassParameter->SetType( AAX_eParameterType_Discrete );
  mParameterManager.AddParameter(mBypassParameter);
      
  for (int i=0;i<NParams();i++)
  {
    IParam *p = GetParam(i);
    AAX_IParameter* param = nullptr;
    
    WDL_String* paramID = new WDL_String("_", 1);
    paramID->SetFormatted(MAX_AAX_PARAMID_LEN, "%i", i+kAAXParamIdxOffset);
    mParamIDs.Add(paramID);
    
    switch (p->Type()) 
    {
      case IParam::kTypeDouble:
      {
        param = new AAX_CParameter<double>(paramID->Get(), 
                                          AAX_CString(p->GetNameForHost()), 
                                          p->GetDefault(), 
                                          AAX_CIPlugTaperDelegate<double>(p->GetMin(), p->GetMax(), p->GetShape()),
                                          AAX_CUnitDisplayDelegateDecorator<double>( AAX_CNumberDisplayDelegate<double>(), AAX_CString(p->GetLabelForHost())), 
                                          p->GetCanAutomate());
        
        param->SetNumberOfSteps(128); // TODO: check this https://developer.digidesign.com/index.php?L1=5&L2=13&L3=56
        param->SetType(AAX_eParameterType_Continuous);

        break;
      }
      case IParam::kTypeInt:
      {
        param = new AAX_CParameter<int>(paramID->Get(), 
                                        AAX_CString(p->GetNameForHost()), 
                                        (int)p->GetDefault(), 
                                        AAX_CLinearTaperDelegate<int>((int)p->GetMin(), (int)p->GetMax()), 
                                        AAX_CUnitDisplayDelegateDecorator<int>( AAX_CNumberDisplayDelegate<int>(), AAX_CString(p->GetLabelForHost())), 
                                        p->GetCanAutomate());
        
        param->SetNumberOfSteps(128);
        param->SetType(AAX_eParameterType_Continuous);

        break;
      }
      case IParam::kTypeEnum:
      case IParam::kTypeBool: 
      {
        int nTexts = p->NDisplayTexts();
        
        std::map<int, AAX_CString> displayTexts;
        
        for (int j=0; j<p->NDisplayTexts(); j++) 
        {
          double value;
          const char* text = p->GetDisplayTextAtIdx(j, &value);
          
          displayTexts.insert(std::pair<int, AAX_CString>(value, AAX_CString(text)) );
        }
        
        param = new AAX_CParameter<int>(paramID->Get(), 
                                        AAX_CString(p->GetNameForHost()), 
                                        (int)p->GetDefault(), 
                                        AAX_CLinearTaperDelegate<int>((int)p->GetMin(), (int)p->GetMax()), 
                                        AAX_CStringDisplayDelegate<int>(displayTexts),
                                        p->GetCanAutomate());
        
        param->SetNumberOfSteps(nTexts);
        param->SetType(AAX_eParameterType_Discrete);
                
        break; 
      }
      default:
        break;
    }
    
    mParameterManager.AddParameter(param);    
  }
  
  AAX_CSampleRate sr;
  Controller()->GetSampleRate(&sr);
  _SetSampleRate(sr);
  OnReset();
  
  return AAX_SUCCESS;
}

AAX_Result IPlugAAX::UpdateParameterNormalizedValue(AAX_CParamID paramID, double iValue, AAX_EUpdateSource iSource )
{
  TRACE;
  
  AAX_Result  result = AAX_SUCCESS;
  
  AAX_IParameter* pAAXParameter = mParameterManager.GetParameterByID(paramID);
    
  if (pAAXParameter == nullptr)
    return AAX_ERROR_INVALID_PARAMETER_ID;
  
  // Store the value into the AAX parameter
  pAAXParameter->UpdateNormalizedValue(iValue);
  
  int paramIdx = atoi(paramID) - kAAXParamIdxOffset;
  
  // TODO: UI thread only? If not, need to lock mParams_mutex
  if ((paramIdx >= 0) && (paramIdx < NParams())) 
  {
    GetParam(paramIdx)->SetNormalized(iValue);
    SendParameterValueToUIFromAPI(paramIdx, iValue, true);
    
    OnParamChange(paramIdx, kAutomation);      
  }
  
  // Now the control has changed
  result = mPacketDispatcher.SetDirty(paramID);
  
  mNumPlugInChanges++;
  
  return result;
}

void IPlugAAX::RenderAudio(AAX_SIPlugRenderInfo* pRenderInfo)
{
  TRACE;

  // Get bypass parameter value
  bool bypass;
  mBypassParameter->GetValueAsBool(&bypass);
  
  AAX_EStemFormat inFormat, outFormat;
  Controller()->GetInputStemFormat(&inFormat);
  Controller()->GetOutputStemFormat(&outFormat);
  
  if (DoesMIDI()) 
  {
    AAX_IMIDINode* midiIn = pRenderInfo->mInputNode;
    AAX_CMidiStream* midiBuffer = midiIn->GetNodeBuffer();
    AAX_CMidiPacket* midiBufferPtr = midiBuffer->mBuffer;
    uint32_t packets_count = midiBuffer->mBufferSize;
    
    // Setup MIDI Out node pointers 
//		AAX_IMIDINode* midiNodeOut = instance->mMIDINodeOutP;
//		AAX_CMidiStream* midiBufferOut = midiNodeOut->GetNodeBuffer();
//		AAX_CMidiPacket* midiBufferOutPtr = midiBufferOut->mBuffer;
        
    for (int i = 0; i<packets_count; i++, midiBufferPtr++) 
    {
      IMidiMsg msg(midiBufferPtr->mTimestamp, midiBufferPtr->mData[0], midiBufferPtr->mData[1], midiBufferPtr->mData[2]);
      ProcessMidiMsg(msg);
    }
  }
  
  AAX_IMIDINode* transportNode = pRenderInfo->mTransportNode;
  mTransport = transportNode->GetTransport();

  int32_t numSamples = *(pRenderInfo->mNumSamples);
  int32_t numInChannels = AAX_STEM_FORMAT_CHANNEL_COUNT(inFormat);
  int32_t numOutChannels = AAX_STEM_FORMAT_CHANNEL_COUNT(outFormat);

  _SetChannelConnections(ERoute::kInput, 0, numInChannels, true);
  _SetChannelConnections(ERoute::kInput, numInChannels, MaxNChannels(ERoute::kInput) - numInChannels, false);
  _AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), pRenderInfo->mAudioInputs, numSamples);
  
  _SetChannelConnections(ERoute::kOutput, 0, numOutChannels, true);
  _SetChannelConnections(ERoute::kOutput, numOutChannels, MaxNChannels(ERoute::kOutput) - numOutChannels, false);
  _AttachBuffers(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), pRenderInfo->mAudioOutputs, numSamples);
  
  if (bypass) 
    _PassThroughBuffers(0.0f, numSamples);
  else 
  {
    int32_t num, denom;
    int64_t ppqPos, samplePos, cStart, cEnd;
    ITimeInfo timeInfo;
    
    mTransport->GetCurrentTempo(&timeInfo.mTempo);
    mTransport->IsTransportPlaying(&timeInfo.mTransportIsRunning);
    
    mTransport->GetCurrentMeter(&num, &denom);
    timeInfo.mNumerator = (int) num;
    timeInfo.mDenominator = (int) denom;
    
    mTransport->GetCurrentTickPosition(&ppqPos);
    timeInfo.mPPQPos = (double) ppqPos / 960000.0;
    
    mTransport->GetCurrentNativeSampleLocation(&samplePos);
    timeInfo.mSamplePos = (double) samplePos;
    
    mTransport->GetCurrentLoopPosition(&timeInfo.mTransportLoopEnabled, &cStart, &cEnd);
    timeInfo.mCycleStart = (double) cStart / 960000.0;
    timeInfo.mCycleEnd = (double) cEnd / 960000.0;
    
    _SetTimeInfo(timeInfo);
    //timeInfo.mLastBar ??
    
    _ProcessBuffers(0.0f, numSamples);
  }
}

AAX_Result IPlugAAX::GetChunkIDFromIndex( int32_t index, AAX_CTypeID* pChunkID) const
{
  IPlugAAX* _this = const_cast<IPlugAAX*>(this);

	if (index != 0)
	{
		*pChunkID = AAX_CTypeID(0);
		return AAX_ERROR_INVALID_CHUNK_INDEX;
	}
	
	*pChunkID = _this->GetUniqueID();
  
	return AAX_SUCCESS;	
}

AAX_Result IPlugAAX::GetChunkSize(AAX_CTypeID chunkID, uint32_t* pSize) const
{
  TRACE;
  
  IPlugAAX* _this = const_cast<IPlugAAX*>(this);
  
  if (chunkID == _this->GetUniqueID()) 
  {
    IByteChunk chunk;
    
    //_this->InitChunkWithIPlugVer(&IPlugChunk);
    
    if (_this->SerializeState(chunk))
    {
      *pSize = chunk.Size();
    }
    
    return AAX_SUCCESS;
  }
  else 
  {
    *pSize = 0;
    return AAX_ERROR_INVALID_CHUNK_ID;
  }
}

AAX_Result IPlugAAX::GetChunk(AAX_CTypeID chunkID, AAX_SPlugInChunk* pChunk) const
{
  TRACE;
  IPlugAAX* _this = const_cast<IPlugAAX*>(this);

  if (chunkID == _this->GetUniqueID()) 
  {
    IByteChunk chunk;
    
    //_this->InitChunkWithIPlugVer(&IPlugChunk); // TODO: IPlugVer should be in chunk!
    
    if (_this->SerializeState(chunk))
    {
      pChunk->fSize = chunk.Size();
      memcpy(pChunk->fData, chunk.GetBytes(), chunk.Size());
      return AAX_SUCCESS;
    }
  }
  
  return AAX_ERROR_INVALID_CHUNK_ID;
}

AAX_Result IPlugAAX::SetChunk(AAX_CTypeID chunkID, const AAX_SPlugInChunk* pChunk)
{
  TRACE;
  // TODO: UI thread only?
  
  if (chunkID == GetUniqueID())
  {    
    IByteChunk chunk;
    chunk.PutBytes(pChunk->fData, pChunk->fSize);
    int pos = 0;
    //GetIPlugVerFromChunk(chunk, pos); // TODO: IPlugVer should be in chunk!
    pos = UnserializeState(chunk, pos);
    
    for (int i = 0; i< NParams(); i++)
    {
      SetParameterNormalizedValue(mParamIDs.Get(i)->Get(), GetParam(i)->GetNormalized() );
    }
    
    OnRestoreState();
    mNumPlugInChanges++; // necessary in order to cause CompareActiveChunk() to get called again and turn off the compare light 
    
    return AAX_SUCCESS;
  }
  
  return AAX_ERROR_INVALID_CHUNK_ID;
}

AAX_Result IPlugAAX::CompareActiveChunk(const AAX_SPlugInChunk* pChunk, AAX_CBoolean* pIsEqual) const
{
  TRACE;

  IPlugAAX* _this = const_cast<IPlugAAX*>(this);

	if (pChunk->fChunkID != _this->GetUniqueID())
	{
		*pIsEqual = true;
		return AAX_SUCCESS; 
	}
  
	*pIsEqual = _this->CompareState((const unsigned char*) pChunk->fData, 0);
    
  return AAX_SUCCESS;
}  

void IPlugAAX::BeginInformHostOfParamChange(int idx)
{
  TRACE;
  TouchParameter(mParamIDs.Get(idx)->Get());
}

void IPlugAAX::InformHostOfParamChange(int idx, double normalizedValue)
{
  TRACE;
  SetParameterNormalizedValue(mParamIDs.Get(idx)->Get(), normalizedValue );
}

void IPlugAAX::EndInformHostOfParamChange(int idx)
{
  TRACE;
  ReleaseParameter(mParamIDs.Get(idx)->Get());
}

void IPlugAAX::ResizeGraphics()
{
  if (HasUI())
  {
    AAX_Point oEffectViewSize;
    oEffectViewSize.horz = (float) Width();
    oEffectViewSize.vert = (float) Height();
    
    IPlugAAXView_Interface* pViewInterface = dynamic_cast<IPlugAAXView_Interface*>(this);
    if(pViewInterface)
      pViewInterface->GetViewContainer()->SetViewSize(oEffectViewSize);

    OnWindowResize();
  }
}

void IPlugAAX::SetLatency(int latency)
{
  Controller()->SetSignalLatency(latency);
  
  IPlugProcessor::SetLatency(latency); // will update delay time
}

// TODO: SendMidiMsg()
bool IPlugAAX::SendMidiMsg(const IMidiMsg& msg)
{
  return false;
}
