#include "IPlugRTAS.h"
#include "RTAS/IPlugProcessRTAS.h"
#include "CPluginControl_Continuous.h"

IPlugRTAS::IPlugRTAS(IPlugInstanceInfo instanceInfo, int nParams, const char* channelIOStr, int nPresets,
      const char* effectName, const char* productName, const char* mfrName,
      int vendorVersion, int uniqueID, int mfrID, int latency, bool plugDoesMidi, bool plugDoesChunks, bool plugIsInst, int plugScChans)
: IPlugBase(nParams, channelIOStr, nPresets, effectName, productName, mfrName,vendorVersion, uniqueID, mfrID, latency, plugDoesMidi, plugDoesChunks, plugIsInst)
, mDoesMidi(plugDoesMidi)
, mNumInputs(0)
, mNumOutputs(0)
//, mNumSideChainInputs(0)
//, mSideChainConnectionNum(0)
{
  Trace(TRACELOC, "%s", effectName);
  SetNumInputs(mNumInputs);
  SetNumOutputs(mNumOutputs);
  SetNumSideChainInputs(mNumSideChainInputs);
  
  SetBlockSize(DEFAULT_BLOCK_SIZE);
}

void IPlugRTAS::SetBlockSize(int blockSize)
{
  IPlugBase::SetBlockSize(blockSize);
}

void IPlugRTAS::SetNumInputs(int nInputs)
{ 
  mNumInputs = nInputs; 
  
  SetInputChannelConnections(0, mNumInputs, true); 
  SetInputChannelConnections(mNumInputs, NInChannels()-mNumInputs, false); 
}

void IPlugRTAS::SetNumOutputs(int nOutputs)
{ 
  mNumOutputs = nOutputs; 
  SetOutputChannelConnections(0, mNumOutputs, true); 
  SetOutputChannelConnections(mNumOutputs, NOutChannels()-mNumOutputs, false); 
}

void IPlugRTAS::SetNumSideChainInputs(int nSideChainInputs)
{ 
  mNumSideChainInputs = nSideChainInputs;
  //SetSideChainChannelConnections(0, mNumSideChainInputs, true); 
  //SetSideChainChannelConnections(mNumSideChainInputs, NSideChainChannels()-mNumSideChainInputs, false); 
}

void IPlugRTAS::SetSideChainConnectionNum(int connectionNum)
{
  mSideChainConnectionNum = connectionNum;
}

void IPlugRTAS::ProcessAudio(float** inputs, float** outputs, float** sidechain, int nFrames)
{
  IMutexLock lock(this);
  
  AttachInputBuffers(0, mNumInputs, inputs, nFrames);
  AttachOutputBuffers(0, mNumOutputs, outputs);
  
  //if (mNumSideChainInputs > 0)
  //{
  //  //AttachSideChainBuffers(0, mNumSideChainInputs, sidechain, nFrames);
  //  AttachInputBuffers(mNumInputs + 1, mNumSideChainInputs, sidechain, nFrames);
  //  Trace(TRACELOC, "%s - %d : %d", "Sidechain buffer attached", mNumInputs, mNumSideChainInputs);
  //}
  
  ProcessBuffers((float) 0.0, nFrames);
}

void IPlugRTAS::BeginInformHostOfParamChange(int idx)
{
  if (!mRTAS) return;
  
  idx += kPTParamIdxOffset;

  if ( mRTAS->IsValidControlIndex(idx) ) 
  {
      mRTAS->ProcessTouchControl(idx);
  }
}

void IPlugRTAS::InformHostOfParamChange(int idx, double normalizedValue)
{
  if (!mRTAS) 
    return;
  
  idx += kPTParamIdxOffset;
  
  if ( mRTAS->IsValidControlIndex(idx) ) 
  { 
    CPluginControl_Continuous *control = dynamic_cast<CPluginControl_Continuous*>(mRTAS->GetControl(idx));
    SInt32 controlValue = 0;
    
    if (control) 
    {
      // from the protools SDK source
      static const double kControlMin = -2147483648.0;
      static const double kControlMax = 2147483647.0;
      
      controlValue = kControlMin + floor(normalizedValue*(kControlMax - kControlMin)+0.5);
    }
    mRTAS->SetControlValue(idx, controlValue);
  }
}

void IPlugRTAS::EndInformHostOfParamChange(int idx)
{
  if (!mRTAS) return;
  
  // PT index 1 based + master bypass, so offset by 2
  idx+=2;
  
  if ( mRTAS->IsValidControlIndex(idx) ) 
  {
    mRTAS->ProcessReleaseControl(idx);
  }
}

// TODO: GetSamplePos()
int IPlugRTAS::GetSamplePos()
{
  return 0;
}

double IPlugRTAS::GetTempo()
{
  return mRTAS->GetTempo();
}

// TODO: GetTimeSig()
void IPlugRTAS::GetTimeSig(int* pNum, int* pDenom)
{
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
  RestorePreset(0);
}

void IPlugRTAS::HostSpecificInit()
{
}

void IPlugRTAS::AttachGraphics(IGraphics* pGraphics)
{
  if (pGraphics) {
    IPlugBase::AttachGraphics(pGraphics);
  }
}

// TODO: SetLatency()
void IPlugRTAS::SetLatency(int samples)
{
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