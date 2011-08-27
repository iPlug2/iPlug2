#include "IPlugRTAS.h"
#include "RTAS/IPlugProcessRTAS.h"
#include "CPluginControl_Continuous.h"
#include "CPluginControl_Discrete.h"

IPlugRTAS::IPlugRTAS(IPlugInstanceInfo instanceInfo, int nParams, const char* channelIOStr, int nPresets,
      const char* effectName, const char* productName, const char* mfrName,
      int vendorVersion, int uniqueID, int mfrID, int latency, bool plugDoesMidi, bool plugDoesChunks, bool plugIsInst, int plugScChans)
: IPlugBase(nParams, channelIOStr, nPresets, effectName, productName, mfrName,vendorVersion, uniqueID, mfrID, latency, plugDoesMidi, plugDoesChunks, plugIsInst)
, mDoesMidi(plugDoesMidi)
, mSideChainIsConnected(false)
{
  Trace(TRACELOC, "%s", effectName);
  int nInputs = NInChannels(), nOutputs = NOutChannels();
  
  SetInputChannelConnections(0, nInputs, true);
  SetOutputChannelConnections(0, nOutputs, true);
  
  mHasSideChain = (plugScChans > 0);
    
  SetBlockSize(DEFAULT_BLOCK_SIZE);
}

void IPlugRTAS::SetNumInputs(int nInputs)
{
  nInputs += mSideChainIsConnected;
  
  SetInputChannelConnections(0, nInputs, true);
  SetInputChannelConnections(nInputs, NInChannels() - nInputs, false);
}

void IPlugRTAS::SetNumOutputs(int nOutputs)
{
  SetOutputChannelConnections(0, nOutputs, true);
  SetOutputChannelConnections(nOutputs, NOutChannels() - nOutputs, false);
}

void IPlugRTAS::ProcessAudio(float** inputs, float** outputs, int nFrames)
{
  IMutexLock lock(this);
  
  AttachInputBuffers(0, NInChannels(), inputs, nFrames);
  AttachOutputBuffers(0, NOutChannels(), outputs);
  
  ProcessBuffers((float) 0.0, nFrames);
}

void IPlugRTAS::BeginInformHostOfParamChange(int idx)
{
  if (!mRTAS) return;
  
  idx += kPTParamIdxOffset;

  if(mRTAS->IsValidControlIndex(idx)) 
  {
    mRTAS->ProcessTouchControl(idx);
  }
}

void IPlugRTAS::InformHostOfParamChange(int idx, double normalizedValue) // actually we don't use normalized value
{
  if (!mRTAS) 
    return;
  
  IParam* pParam = GetParam(idx);

  idx += kPTParamIdxOffset;
  
  if ( mRTAS->IsValidControlIndex(idx) ) 
  { 
    switch (pParam->Type()) {
      case IParam::kTypeDouble: {
        CPluginControl_Continuous *control = dynamic_cast<CPluginControl_Continuous*>(mRTAS->GetControl(idx));
        mRTAS->SetControlValue(idx, control->ConvertContinuousToControl( pParam->Value() ));
        break; }
      case IParam::kTypeInt:
      case IParam::kTypeEnum:
      case IParam::kTypeBool: {
        CPluginControl_Discrete *control = dynamic_cast<CPluginControl_Discrete*>(mRTAS->GetControl(idx));
        mRTAS->SetControlValue(idx, control->ConvertDiscreteToControl( pParam->Int() ));
        break; }
      default:
        break;
    }
  }
}

void IPlugRTAS::EndInformHostOfParamChange(int idx)
{
  if (!mRTAS) return;
  
  idx+=kPTParamIdxOffset;
  
  if ( mRTAS->IsValidControlIndex(idx) ) 
  {
    mRTAS->ProcessReleaseControl(idx);
  }
}

int IPlugRTAS::GetSamplePos()
{
  return mRTAS->GetSamplePos();
}

double IPlugRTAS::GetTempo()
{
  return mRTAS->GetTempo();
}

void IPlugRTAS::GetTimeSig(int* pNum, int* pDenom)
{
  mRTAS->GetTimeSig(pNum, pDenom);
}

void IPlugRTAS::GetTime(ITimeInfo* pTimeInfo) 
{
  mRTAS->GetTime(&pTimeInfo->mSamplePos, &pTimeInfo->mTempo, 
                 &pTimeInfo->mPPQPos, &pTimeInfo->mLastBar,
                 &pTimeInfo->mNumerator, &pTimeInfo->mDenominator,
                 &pTimeInfo->mCycleStart, &pTimeInfo->mCycleEnd,
                 &pTimeInfo->mTransportIsRunning, &pTimeInfo->mTransportLoopEnabled);
}

EHost IPlugRTAS::GetHost()
{
  EHost host = IPlugBase::GetHost();
  if (host == kHostUninit) {
    SetHost("Pro Tools", 0);
    host = IPlugBase::GetHost();
  }
  return host;
}

// TODO: ResizeGraphics() ??
void IPlugRTAS::ResizeGraphics(int w, int h)
{
}

void IPlugRTAS::Created(class IPlugProcessRTAS *r)
{	
	mRTAS = r;
	
	HostSpecificInit();
	PruneUninitializedPresets();
	
	SetBlockSize(r->GetBlockSize());
}

// TODO: SendMidiMsg()
bool IPlugRTAS::SendMidiMsg(IMidiMsg* pMsg)
{
  return false;
}
// TODO: SendMidiMsgs()
bool IPlugRTAS::SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs)
{
  return false;
}

void IPlugRTAS::SetParameter(int idx)
{  
  if ( mRTAS->IsValidControlIndex(idx) ) 
  { 
    IParam* pParam = GetParam(idx - kPTParamIdxOffset);
    double value;

    switch (pParam->Type()) {
      case IParam::kTypeDouble: {
        CPluginControl_Continuous *control = dynamic_cast<CPluginControl_Continuous*>(mRTAS->GetControl(idx));
        value = control->GetContinuous();
        break; }
      case IParam::kTypeInt:
      case IParam::kTypeEnum:
      case IParam::kTypeBool: {
        CPluginControl_Discrete *control = dynamic_cast<CPluginControl_Discrete*>(mRTAS->GetControl(idx));
        value = (double) control->GetDiscrete();
        break; }
      default:
        break;
    }
    
    IMutexLock lock(this);

    if (GetGUI()) 
      GetGUI()->SetParameterFromPlug(idx - kPTParamIdxOffset, value, false);
    
    pParam->Set(value);
    OnParamChange(idx - kPTParamIdxOffset);
  }
}