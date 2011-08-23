#include "IPlugStandalone.h"
#ifndef OS_IOS
  #include "IGraphics.h"
#endif
#include <stdio.h>

IPlugStandalone::IPlugStandalone(IPlugInstanceInfo instanceInfo, 
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
: IPlugBase(nParams, channelIOStr, nPresets, effectName, productName, mfrName,
    vendorVersion, uniqueID, mfrID, latency,
    plugDoesMidi, plugDoesChunks, plugIsInst),
    mDoesMidi(plugDoesMidi)/*, mHostSpecificInitDone(false)*/
{
  Trace(TRACELOC, "%s%s", effectName, channelIOStr);

  SetInputChannelConnections(0, NInChannels(), true);
  SetOutputChannelConnections(0, NOutChannels(), true);  
  
  SetBlockSize(DEFAULT_BLOCK_SIZE);
  SetHost("standalone", vendorVersion); // TODO:vendor version correct?
  
#ifdef OS_IOS
  mIOSLink = instanceInfo.mIOSLink;
#else
  mMidiOutChan = instanceInfo.mMidiOutChan;
  mMidiOut = instanceInfo.mRTMidiOut;
#endif
  
}

// TODO: BeginInformHostOfParamChange etc, maybe needed for ios state persistance? 
void IPlugStandalone::BeginInformHostOfParamChange(int idx)
{
}

void IPlugStandalone::InformHostOfParamChange(int idx, double normalizedValue)
{
}

void IPlugStandalone::EndInformHostOfParamChange(int idx)
{
}

void IPlugStandalone::InformHostOfProgramChange()
{
}

// TODO: GetSamplePos()
int IPlugStandalone::GetSamplePos()
{ 
	return 0;
}

// TODO: GetTempo()
double IPlugStandalone::GetTempo()
{
	return 120.;
}

// TODO: GetTime()
void IPlugStandalone::GetTime(ITimeInfo* pTimeInfo)
{
}

// TODO: GetTimeSig()
void IPlugStandalone::GetTimeSig(int* pNum, int* pDenom)
{

}

// TODO: ResizeGraphics()
void IPlugStandalone::ResizeGraphics(int w, int h)
{
  IGraphics* pGraphics = GetGUI();
  if (pGraphics) {
  }
}

void IPlugStandalone::SetSampleRate(double sampleRate)
{
  mSampleRate = sampleRate;
}

void IPlugStandalone::SetBlockSize(int blockSize)
{
  mBlockSize = blockSize;
}

bool IPlugStandalone::SendMidiMsg(IMidiMsg* pMsg)
{
#ifdef OS_IOS
  mIOSLink->SendMidiMsg(pMsg);
#else
  if (mMidiOut) 
  {
    IMidiMsg newMsg = *pMsg;
    
    // if the midi channel out filter is set, reassign the status byte appropriately
    if (!*mMidiOutChan == 0) {
      newMsg.mStatus = (*mMidiOutChan)-1 | ((unsigned int) newMsg.StatusMsg() << 4) ;
    }
    
    std::vector<unsigned char> message;
    message.push_back( newMsg.mStatus );
    message.push_back( newMsg.mData1 );
    message.push_back( newMsg.mData2 );
    
    mMidiOut->sendMessage( &message );
    return true;
  }
#endif
	return false;
}

// TODO: SendMidiMsgs()
bool IPlugStandalone::SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs)
{
  return false;
}

// TODO: HostSpecificInit()
void IPlugStandalone::HostSpecificInit()
{
}

#ifdef OS_IOS
void IPlugStandalone::LockMutexAndProcessSingleReplacing(float** inputs, float** outputs, int nFrames)
{
  IMutexLock lock(this);
  //WDL_MutexLock lock(&mMutex);
  ProcessSingleReplacing(inputs, outputs, nFrames);
}
#else
void IPlugStandalone::LockMutexAndProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  IMutexLock lock(this);
  //WDL_MutexLock lock(&mMutex);
  ProcessDoubleReplacing(inputs, outputs, nFrames);
}
#endif
