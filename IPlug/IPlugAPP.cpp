#include "IPlugAPP.h"
#ifdef OS_MAC
#include "swell.h"
#endif
extern HWND gHWND;

IPlugAPP::IPlugAPP(IPlugInstanceInfo instanceInfo, IPlugConfig c)
: IPLUG_BASE_CLASS(c, kAPISA)
, IPlugProcessor<PLUG_SAMPLE_DST>(c, kAPISA)
{
  Trace(TRACELOC, "%s%s", c.effectName, c.channelIOStr);

  SetInputChannelConnections(0, NInChannels(), true);
  SetOutputChannelConnections(0, NOutChannels(), true);

  SetBlockSize(DEFAULT_BLOCK_SIZE);
  SetHost("standalone", c.vendorVersion);

  mMidiOutChan = instanceInfo.mMidiOutChan;
  mMidiOut = instanceInfo.mRTMidiOut;
}

void IPlugAPP::ResizeGraphics(int w, int h, double scale)
{
  if (GetHasUI())
  {
    #ifdef OS_MAC
    #define TITLEBAR_BODGE 22 //TODO: sort this out
    RECT r;
    GetWindowRect(gHWND, &r);
    SetWindowPos(gHWND, 0, r.left, r.bottom - GetUIHeight() - TITLEBAR_BODGE, GetUIWidth(), GetUIHeight() + TITLEBAR_BODGE, 0);
    #endif
    OnWindowResize();
  }
}

bool IPlugAPP::SendMidiMsg(IMidiMsg& msg)
{
  if (DoesMIDI())
  {
    // if the midi channel out filter is set, reassign the status byte appropriately
    if (mMidiOutChan)
    {
      msg.mStatus = mMidiOutChan-1 | ((unsigned int) msg.StatusMsg() << 4) ;
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

bool IPlugAPP::SendSysEx(ISysEx& msg)
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
