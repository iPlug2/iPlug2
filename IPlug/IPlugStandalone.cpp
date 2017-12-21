#include "IPlugStandalone.h"
#ifdef OS_OSX
#include "swell.h"
#endif
extern HWND gHWND;

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
  : IPLUG_BASE_CLASS(nParams,
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
  , mMidiOutChan(0)
{
  Trace(TRACELOC, "%s%s", effectName, channelIOStr);

  SetInputChannelConnections(0, NInChannels(), true);
  SetOutputChannelConnections(0, NOutChannels(), true);

  SetBlockSize(DEFAULT_BLOCK_SIZE);
  SetHost("standalone", vendorVersion);

  mMidiOutChan = instanceInfo.mMidiOutChan;
  mMidiOut = instanceInfo.mRTMidiOut;
}

void IPlugStandalone::ResizeGraphics(int w, int h)
{
  if (GetHasUI())
  {
    #ifdef OS_OSX
    #define TITLEBAR_BODGE 22
    RECT r;
    GetWindowRect(gHWND, &r);
    SetWindowPos(gHWND, 0, r.left, r.bottom - GetUIHeight() - TITLEBAR_BODGE, GetUIWidth(), GetUIHeight() + TITLEBAR_BODGE, 0);
    #endif
    OnWindowResize();
  }
}

bool IPlugStandalone::SendMidiMsg(IMidiMsg& msg)
{
  if (DoesMIDI())
  {
    // if the midi channel out filter is set, reassign the status byte appropriately
    if (mMidiOutChan != 0)
    {
      msg.mStatus = (*mMidiOutChan)-1 | ((unsigned int) msg.StatusMsg() << 4) ;
    }

    std::vector<unsigned char> message;
    message.push_back( msg.mStatus );
    message.push_back( msg.mData1 );
    message.push_back( msg.mData2 );

    mMidiOut->sendMessage( &message );
    return true;
  }

  return false;
}

bool IPlugStandalone::SendSysEx(ISysEx& msg)
{
  if (mMidiOut)
  {  
    std::vector<unsigned char> message;
    
    for (int i = 0; i < msg.mSize; i++) {
      message.push_back(msg.mData[i]);
    }
    
    mMidiOut->sendMessage(&message);
    return true;
  }
  
  return false;
}
