/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

#include "IPlugAPP.h"
#if defined OS_MAC || defined OS_LINUX
#include "swell.h"
#endif
extern HWND gHWND;

IPlugAPP::IPlugAPP(IPlugInstanceInfo instanceInfo, IPlugConfig c)
: IPlugAPIBase(c, kAPIAPP)
, IPlugProcessor<PLUG_SAMPLE_DST>(c, kAPIAPP)
{
  Trace(TRACELOC, "%s%s", c.pluginName, c.channelIOStr);

  _SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
  _SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true);

  _SetBlockSize(DEFAULT_BLOCK_SIZE);
  SetHost("standalone", c.vendorVersion);

  mMidiOutChan = instanceInfo.mMidiOutChan;
  mMidiOut = instanceInfo.mRTMidiOut;
}

void IPlugAPP::ResizeGraphics()
{
  if (HasUI())
  {
    #ifdef OS_MAC
    #define TITLEBAR_BODGE 22 //TODO: sort this out
    RECT r;
    GetWindowRect(gHWND, &r);
    SetWindowPos(gHWND, 0, r.left, r.bottom - Height() - TITLEBAR_BODGE, Width(), Height() + TITLEBAR_BODGE, 0);
    #endif
    OnWindowResize();
  }
}

bool IPlugAPP::SendMidiMsg(const IMidiMsg& msg)
{
  uint8_t status;
  if (DoesMIDI())
  {
    // if the midi channel out filter is set, reassign the status byte appropriately
    if (mMidiOutChan)
      status = mMidiOutChan-1 | ((unsigned int) msg.StatusMsg() << 4) ;

    std::vector<uint8_t> message;
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
    
    for (int i = 0; i < msg.mSize; i++)
    {
      message.push_back(msg.mData[i]);
    }
    
    mMidiOut->sendMessage(&message);
    return true;
  }
  
  return false;
}
