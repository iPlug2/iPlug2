#include "IPlugRTAS.h"
#include "RTAS/IPlugProcessRTAS.h"
#include "CPluginControl_Continuous.h"

IPlugRTAS::IPlugRTAS(IPlugInstanceInfo instanceInfo, int nParams, const char* channelIOStr, int nPresets,
      const char* effectName, const char* productName, const char* mfrName,
      int vendorVersion, int uniqueID, int mfrID, int latency, bool plugDoesMidi, bool plugDoesChunks, bool plugIsInst, int plugScChans)
: IPlugBase(nParams, channelIOStr, nPresets, effectName, productName, mfrName,vendorVersion, uniqueID, mfrID, latency, plugDoesMidi, plugDoesChunks, plugIsInst)
, mDoesMidi(plugDoesMidi)
, mSideChainIsConnected(false)
{
  Trace(TRACELOC, "%s", effectName);
  int nInputs = NInChannels(), nOutputs = NOutChannels();
  
  // Default everything to connected, then disconnect pins if the host says to.
  SetInputChannelConnections(0, nInputs, true);
  SetOutputChannelConnections(0, nOutputs, true);
  
  mHasSideChain = (plugScChans > 0);
    
  SetBlockSize(DEFAULT_BLOCK_SIZE);
}

void IPlugRTAS::SetBlockSize(int blockSize)
{
  IPlugBase::SetBlockSize(blockSize);
}

void IPlugRTAS::SetSideChainConnected(bool connected)
{
  mSideChainIsConnected = connected;
  DBGMSG("mSideChainIsConnected %i\n", mSideChainIsConnected);
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