#include "IPlugStandalone.h"
#ifndef OS_IOS
  #include "IGraphics.h"
  extern HWND gHWND;
#endif

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
              kAPISA)
{
  Trace(TRACELOC, "%s%s", effectName, channelIOStr);

  SetInputChannelConnections(0, NInChannels(), true);
  SetOutputChannelConnections(0, NOutChannels(), true);

  SetBlockSize(DEFAULT_BLOCK_SIZE);
  SetHost("standalone", vendorVersion);

  #ifdef OS_IOS
  mIOSLink = instanceInfo.mIOSLink;
  #else
  mMidiOutChan = instanceInfo.mMidiOutChan;
  mMidiOut = instanceInfo.mRTMidiOut;
  #endif
}

void IPlugStandalone::ResizeGraphics(int w, int h)
{
  #ifndef OS_IOS
  IGraphics* pGraphics = GetGUI();
  if (pGraphics)
  {
    #ifdef OS_OSX
    #define TITLEBAR_BODGE 22
    RECT r;
    GetWindowRect(gHWND, &r);
    SetWindowPos(gHWND, 0, r.left, r.bottom - pGraphics->Height() - TITLEBAR_BODGE, pGraphics->Width(), pGraphics->Height() + TITLEBAR_BODGE, 0);
    #endif
    OnWindowResize();
  }
  #endif
}

bool IPlugStandalone::SendMidiMsg(IMidiMsg* pMsg)
{
  #ifdef OS_IOS
  mIOSLink->SendMidiMsg(pMsg);
  #else
  if (DoesMIDI())
  {
    IMidiMsg newMsg = *pMsg;

    // if the midi channel out filter is set, reassign the status byte appropriately
    if (!*mMidiOutChan == 0)
    {
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

bool IPlugStandalone::SendSysEx(ISysEx* pSysEx)
{
#ifndef OS_IOS
  if (mMidiOut)
  {  
    std::vector<unsigned char> message;
    
    for (int i = 0; i < pSysEx->mSize; i++) {
      message.push_back(pSysEx->mData[i]);
    }
    
    mMidiOut->sendMessage( &message );
    return true;
  }
#endif
  return false;
}

#ifdef OS_IOS
void IPlugStandalone::LockMutexAndProcessSingleReplacing(float** inputs, float** outputs, int nFrames)
{
  IMutexLock lock(this);
  ProcessSingleReplacing(inputs, outputs, nFrames);
}
#else
void IPlugStandalone::LockMutexAndProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  IMutexLock lock(this);
  ProcessDoubleReplacing(inputs, outputs, nFrames);
}
#endif
