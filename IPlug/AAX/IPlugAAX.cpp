/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

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
    mPlug->OpenWindow(pWindow);
    
    IPlugAAXView_Interface* pViewInterface = (IPlugAAXView_Interface*) mPlug->GetAAXViewInterface();
    
    if(pViewInterface)
      pViewInterface->SetViewContainer(GetViewContainer());
  }
}

void AAX_CEffectGUI_IPLUG::DeleteViewContainer() 
{
  mPlug->CloseWindow();
}

AAX_Result AAX_CEffectGUI_IPLUG::GetViewSize(AAX_Point* pNewViewSize) const
{
  if (mPlug->HasUI())
  {
    pNewViewSize->horz = (float) mPlug->GetEditorWidth();
    pNewViewSize->vert = (float) mPlug->GetEditorHeight();
  }
  
  return AAX_SUCCESS; 
}

AAX_Result AAX_CEffectGUI_IPLUG::ParameterUpdated(AAX_CParamID paramID)
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
: IPlugAPIBase(c, kAPIAAX)
, IPlugProcessor<PLUG_SAMPLE_DST>(c, kAPIAAX)
{
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
  
  CreateTimer();
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
      
  for (int i=0; i<NParams(); i++)
  {
    IParam* pParam = GetParam(i);
    AAX_IParameter* pAAXParam = nullptr;
    
    WDL_String* pParamIDStr = new WDL_String("_", 1);
    pParamIDStr->SetFormatted(MAX_AAX_PARAMID_LEN, "%i", i+kAAXParamIdxOffset);
    mParamIDs.Add(pParamIDStr);
    
    switch (pParam->Type())
    {
      case IParam::kTypeDouble:
      {
        pAAXParam = new AAX_CParameter<double>(pParamIDStr->Get(),
                                          AAX_CString(pParam->GetNameForHost()),
                                          pParam->GetDefault(),
                                          AAX_CIPlugTaperDelegate<double>(*pParam),
                                          AAX_CUnitDisplayDelegateDecorator<double>( AAX_CNumberDisplayDelegate<double>(), AAX_CString(pParam->GetLabelForHost())),
                                          pParam->GetCanAutomate());
        
        pAAXParam->SetNumberOfSteps(128); // TODO: check this https://developer.digidesign.com/index.php?L1=5&L2=13&L3=56
        pAAXParam->SetType(AAX_eParameterType_Continuous);

        break;
      }
      case IParam::kTypeInt:
      {
        pAAXParam = new AAX_CParameter<int>(pParamIDStr->Get(),
                                        AAX_CString(pParam->GetNameForHost()),
                                        (int)pParam->GetDefault(),
                                        AAX_CLinearTaperDelegate<int,1>((int)pParam->GetMin(), (int)pParam->GetMax()),
                                        AAX_CUnitDisplayDelegateDecorator<int>( AAX_CNumberDisplayDelegate<int,0>(), AAX_CString(pParam->GetLabelForHost())),
                                        pParam->GetCanAutomate());
        
        pAAXParam->SetNumberOfSteps(128);
        pAAXParam->SetType(AAX_eParameterType_Continuous);

        break;
      }
      case IParam::kTypeEnum:
      case IParam::kTypeBool: 
      {
        int nTexts = pParam->NDisplayTexts();
        
        std::map<int, AAX_CString> displayTexts;
        
        for (int j=0; j<pParam->NDisplayTexts(); j++)
        {
          double value;
          const char* text = pParam->GetDisplayTextAtIdx(j, &value);
          
          displayTexts.insert(std::pair<int, AAX_CString>(value, AAX_CString(text)) );
        }
        
        pAAXParam = new AAX_CParameter<int>(pParamIDStr->Get(),
                                        AAX_CString(pParam->GetNameForHost()),
                                        (int)pParam->GetDefault(),
                                        AAX_CLinearTaperDelegate<int,1>((int) pParam->GetMin(), (int) pParam->GetMax()),
                                        AAX_CStringDisplayDelegate<int>(displayTexts),
                                        pParam->GetCanAutomate());
        
        pAAXParam->SetNumberOfSteps(nTexts);
        pAAXParam->SetType(AAX_eParameterType_Discrete);
                
        break; 
      }
      default:
        break;
    }
    
    mParameterManager.AddParameter(pAAXParam);
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
  
  if ((paramIdx >= 0) && (paramIdx < NParams())) 
  {
    ENTER_PARAMS_MUTEX;
    GetParam(paramIdx)->SetNormalized(iValue);
    _SendParameterValueFromAPI(paramIdx, iValue, true);
    OnParamChange(paramIdx, kHost);
    LEAVE_PARAMS_MUTEX;
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
  
  if (DoesMIDIIn()) 
  {
    AAX_IMIDINode* pMidiIn = pRenderInfo->mInputNode;
    AAX_CMidiStream* pMidiBuffer = pMidiIn->GetNodeBuffer();
    AAX_CMidiPacket* pMidiPacket = pMidiBuffer->mBuffer;
    uint32_t packets_count = pMidiBuffer->mBufferSize;
        
    for (int i = 0; i<packets_count; i++, pMidiPacket++) 
    {
      IMidiMsg msg(pMidiPacket->mTimestamp, pMidiPacket->mData[0], pMidiPacket->mData[1], pMidiPacket->mData[2]);
      ProcessMidiMsg(msg);
      mMidiMsgsFromProcessor.Push(msg);
    }
  }
  
  AAX_IMIDINode* pTransportNode = pRenderInfo->mTransportNode;
  mTransport = pTransportNode->GetTransport();

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
    
    IMidiMsg msg;
    
    while (mMidiMsgsFromEditor.Pop(msg))
    {
      ProcessMidiMsg(msg);
    }
    
    _ProcessBuffers(0.0f, numSamples);
  }
  
  // Midi Out
  if (DoesMIDIOut())
  {
    AAX_IMIDINode* midiOut = pRenderInfo->mOutputNode;
    
    if(midiOut)
    {
      //MIDI
      if (!mMidiOutputQueue.Empty())
      {        
        while (!mMidiOutputQueue.Empty())
        {
          IMidiMsg& msg = mMidiOutputQueue.Peek();
          
          AAX_CMidiPacket packet;
          
          packet.mIsImmediate = true; // TODO: how does this affect sample accuracy?
          
          packet.mTimestamp = (uint32_t) msg.mOffset;
          packet.mLength = 3;
          
          packet.mData[0] = msg.mStatus;
          packet.mData[1] = msg.mData1;
          packet.mData[2] = msg.mData2;
          
          midiOut->PostMIDIPacket (&packet);
          
          mMidiOutputQueue.Remove();
        }
      }
      
      mMidiOutputQueue.Flush(numSamples);
      
      //Output SYSEX from the editor, which has bypassed ProcessSysEx()
      if(mSysExDataFromEditor.ElementsAvailable())
      {
        while (mSysExDataFromEditor.Pop(mSysexBuf))
        {
          int numPackets = (int) ceil((float) mSysexBuf.mSize/4.); // each packet can store 4 bytes of data
          int bytesPos = 0;
          
          for (int p = 0; p < numPackets; p++)
          {
            AAX_CMidiPacket packet;
            
            packet.mTimestamp = (uint32_t) mSysexBuf.mOffset;
            packet.mIsImmediate = true;
            
            int b = 0;
            
            while (b < 4 && bytesPos < mSysexBuf.mSize)
            {
              packet.mData[b++] = e.data.bytes[bytesPos++];
            }
            
            packet.mLength = b;
            
            midiOut->PostMIDIPacket (&packet);
          }
        }
      }
    }
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
      memcpy(pChunk->fData, chunk.GetData(), chunk.Size());
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
    //IByteChunk::GetIPlugVerFromChunk(chunk, pos); // TODO: IPlugVer should be in chunk!
    pos = UnserializeState(chunk, pos);
    
    for (int i = 0; i< NParams(); i++)
      SetParameterNormalizedValue(mParamIDs.Get(i)->Get(), GetParam(i)->GetNormalized() );
    
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

void IPlugAAX::ResizeGraphics(int viewWidth, int viewHeight, float scale)
{
  if (HasUI())
  {
    AAX_Point oEffectViewSize;
    oEffectViewSize.horz = (float) viewWidth;
    oEffectViewSize.vert = (float) viewHeight;
    
    IPlugAAXView_Interface* pViewInterface = (IPlugAAXView_Interface*) GetAAXViewInterface();
    if(pViewInterface)
      pViewInterface->GetViewContainer()->SetViewSize(oEffectViewSize);

    IPlugAPIBase::ResizeGraphics(viewWidth, viewHeight, scale);
    OnWindowResize();
  }
}

void IPlugAAX::SetLatency(int latency)
{
  Controller()->SetSignalLatency(latency);
  
  IPlugProcessor::SetLatency(latency); // will update delay time
}

bool IPlugAAX::SendMidiMsg(const IMidiMsg& msg)
{
  mMidiOutputQueue.Add(msg);
  return true;
}
