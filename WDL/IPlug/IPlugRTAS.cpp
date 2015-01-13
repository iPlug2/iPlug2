#include "IPlugRTAS.h"
#include "RTAS/IPlugProcess.h"
#include "CPluginControl_Continuous.h"
#include "CPluginControl_Discrete.h"

IPlugRTAS::IPlugRTAS(IPlugInstanceInfo instanceInfo,
                     int nParams,
                     const char* channelIOStr,
                     int nPresets,
                     const char* effectName,
                     const char* productName,
                     const char* mfrName,
                     int vendorVersion,
                     int uniqueID,
                     int mfrID,
                     int latency,
                     bool plugDoesMidi,
                     bool plugDoesChunks,
                     bool plugIsInst,
                     int plugScChans)
  : IPlugBase(nParams,
              channelIOStr,
              nPresets,
              effectName,
              productName,
              mfrName,
              vendorVersion,
              uniqueID,
              mfrID,
              latency,
              plugDoesMidi,
              plugDoesChunks,
              plugIsInst,
              kAPIRTAS)
  , mSideChainIsConnected(false)
{
  Trace(TRACELOC, "%s", effectName);

  int nInputs = NInChannels(), nOutputs = NOutChannels();
  
  if (nInputs) 
  {
    mDelay = new NChanDelayLine(nInputs, nOutputs);
    mDelay->SetDelayTime(latency);
  }

  SetInputChannelConnections(0, nInputs, true);
  SetOutputChannelConnections(0, nOutputs, true);

  mHasSideChain = (plugScChans > 0);

  SetBlockSize(DEFAULT_BLOCK_SIZE);
}

void IPlugRTAS::SetIO(int nInputs, int nOutputs)
{
  nInputs += mSideChainIsConnected;

  SetInputChannelConnections(0, nInputs, true);
  SetInputChannelConnections(nInputs, NInChannels() - nInputs, false);

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

void IPlugRTAS::ProcessAudioBypassed(float** inputs, float** outputs, int nFrames)
{
  IMutexLock lock(this);

  AttachInputBuffers(0, NInChannels(), inputs, nFrames);
  AttachOutputBuffers(0, NOutChannels(), outputs);

  PassThroughBuffers((float) 0.0, nFrames);
}

void IPlugRTAS::BeginInformHostOfParamChange(int idx)
{
  if (!mProcess) return;

  idx += kPTParamIdxOffset;

  if(mProcess->IsValidControlIndex(idx))
  {
    mProcess->ProcessTouchControl(idx);
  }
}

void IPlugRTAS::InformHostOfParamChange(int idx, double normalizedValue) // actually we don't use normalized value
{
  if (!mProcess) return;
  
  IParam* pParam = GetParam(idx);

  idx += kPTParamIdxOffset;

  if ( mProcess->IsValidControlIndex(idx) )
  {
    switch (pParam->Type())
    {
      case IParam::kTypeDouble:
      {
        CPluginControl_Continuous *control = dynamic_cast<CPluginControl_Continuous*>(mProcess->GetControl(idx));
        mProcess->SetControlValue(idx, control->ConvertContinuousToControl( pParam->Value() ));
        break;
      }
      case IParam::kTypeInt:
      case IParam::kTypeEnum:
      case IParam::kTypeBool:
      {
        CPluginControl_Discrete *control = dynamic_cast<CPluginControl_Discrete*>(mProcess->GetControl(idx));
        mProcess->SetControlValue(idx, control->ConvertDiscreteToControl( pParam->Int() ));
        break;
      }
      default:
        break;
    }
  }
}

void IPlugRTAS::EndInformHostOfParamChange(int idx)
{
  if (!mProcess) return;

  idx+=kPTParamIdxOffset;

  if ( mProcess->IsValidControlIndex(idx) )
  {
    mProcess->ProcessReleaseControl(idx);
  }
}

void IPlugRTAS::InformHostOfProgramChange()
{
  for (int i = 0; i< NParams(); i++)
  {
    BeginInformHostOfParamChange(i);
    InformHostOfParamChange(i, 0.); // don't used normalized value
    EndInformHostOfParamChange(i);
  }
}

int IPlugRTAS::GetSamplePos()
{
  return mProcess->GetSamplePos();
}

double IPlugRTAS::GetTempo()
{
  return mProcess->GetTempo();
}

void IPlugRTAS::GetTimeSig(int* pNum, int* pDenom)
{
  mProcess->GetTimeSig(pNum, pDenom);
}

void IPlugRTAS::GetTime(ITimeInfo* pTimeInfo)
{
  mProcess->GetTime(&pTimeInfo->mSamplePos, &pTimeInfo->mTempo,
                    &pTimeInfo->mPPQPos, &pTimeInfo->mLastBar,
                    &pTimeInfo->mNumerator, &pTimeInfo->mDenominator,
                    &pTimeInfo->mCycleStart, &pTimeInfo->mCycleEnd,
                    &pTimeInfo->mTransportIsRunning, &pTimeInfo->mTransportLoopEnabled);
}

EHost IPlugRTAS::GetHost()
{
  EHost host = IPlugBase::GetHost();
  if (host == kHostUninit)
  {
    SetHost("ProTools", mProcess->GetHostVersion());
    host = IPlugBase::GetHost();
  }
  return host;
}

void IPlugRTAS::ResizeGraphics(int w, int h)
{
  mProcess->ResizeGraphics(w, h);
}

void IPlugRTAS::Created(class IPlugProcess* pProcess)
{
  mProcess = pProcess;
  PruneUninitializedPresets();
  SetBlockSize(mProcess->GetBlockSize());
  OnHostIdentified();
}

// TODO: SendMidiMsg/s()
bool IPlugRTAS::SendMidiMsg(IMidiMsg* pMsg)
{
  return false;
}

void IPlugRTAS::SetParameter(int idx)
{
  TRACE;
  
  if ( mProcess->IsValidControlIndex(idx) )
  {
    IParam* pParam = GetParam(idx - kPTParamIdxOffset);
    double value;

    switch (pParam->Type())
    {
      case IParam::kTypeDouble:
      {
        CPluginControl_Continuous *control = dynamic_cast<CPluginControl_Continuous*>(mProcess->GetControl(idx));
        value = control->GetContinuous();
        break;
      }
      case IParam::kTypeInt:
      case IParam::kTypeEnum:
      case IParam::kTypeBool:
      {
        CPluginControl_Discrete *control = dynamic_cast<CPluginControl_Discrete*>(mProcess->GetControl(idx));
        value = (double) control->GetDiscrete();
        break;
      }
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

void IPlugRTAS::SetSideChainConnected(bool connected)
{
  if (connected != mSideChainIsConnected)
  {
    ZeroScratchBuffers();
    mSideChainIsConnected = connected;
  }
}

void IPlugRTAS::DirtyPTCompareState() 
{ 
  mProcess->DirtyState(); 
}
