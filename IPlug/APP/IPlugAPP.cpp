/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugAPP.h"
#include "IPlugAPP_host.h"

#if defined OS_MAC || defined OS_LINUX
#include "swell.h"
#endif

extern HWND gHWND;

IPlugAPP::IPlugAPP(IPlugInstanceInfo instanceInfo, IPlugConfig c)
: IPlugAPIBase(c, kAPIAPP)
, IPlugProcessor<PLUG_SAMPLE_DST>(c, kAPIAPP)
{
  mAppHost = (IPlugAPPHost*) instanceInfo.pAppHost;
  
  Trace(TRACELOC, "%s%s", c.pluginName, c.channelIOStr);

  _SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
  _SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true);

  _SetBlockSize(DEFAULT_BLOCK_SIZE);
  SetHost("standalone", c.vendorVersion);
    
  CreateTimer();
}

void IPlugAPP::ResizeGraphics(int viewWidth, int viewHeight, float scale)
{
  #ifdef OS_MAC
  #define TITLEBAR_BODGE 22 //TODO: sort this out
  RECT r;
  GetWindowRect(gHWND, &r);
  SetWindowPos(gHWND, 0, r.left, r.bottom - viewHeight - TITLEBAR_BODGE, viewWidth, viewHeight + TITLEBAR_BODGE, 0);
  #endif
  
  IPlugAPIBase::ResizeGraphics(viewWidth, viewHeight, scale);
  OnWindowResize();
}

bool IPlugAPP::SendMidiMsg(const IMidiMsg& msg)
{
  if (DoesMIDIOut() && mAppHost->mMidiOut)
  {
    //TODO: midi out channel
//    uint8_t status;
//
//    // if the midi channel out filter is set, reassign the status byte appropriately
//    if(mAppHost->mMidiOutChannel > -1)
//      status = mAppHost->mMidiOutChannel-1 | ((uint8_t) msg.StatusMsg() << 4) ;

    std::vector<uint8_t> message;
    message.push_back(msg.mStatus);
    message.push_back(msg.mData1);
    message.push_back(msg.mData2);

    mAppHost->mMidiOut->sendMessage(&message);
    
    return true;
  }

  return false;
}

bool IPlugAPP::SendSysEx(const ISysEx& msg)
{
  if (DoesMIDIOut() && mAppHost->mMidiOut)
  {
    //TODO: midi out channel
    std::vector<uint8_t> message;
    
    for (int i = 0; i < msg.mSize; i++)
    {
      message.push_back(msg.mData[i]);
    }
    
    mAppHost->mMidiOut->sendMessage(&message);
    return true;
  }
  
  return false;
}

void IPlugAPP::SendSysexMsgFromUI(const ISysEx& msg)
{
  SendSysEx(const_cast<ISysEx&>(msg));
}

void IPlugAPP::AppProcess(double** inputs, double** outputs, int nFrames)
{
  _SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), false); //TODO: go elsewhere - enable inputs
  _SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true); //TODO: go elsewhere
  _AttachBuffers(ERoute::kInput, 0, NChannelsConnected(ERoute::kInput), inputs, GetBlockSize());
  _AttachBuffers(ERoute::kOutput, 0, NChannelsConnected(ERoute::kOutput), outputs, GetBlockSize());
  
  IMidiMsg msg;
  
  while (mMidiMsgsFromCallback.Pop(msg))
  {
    ProcessMidiMsg(msg);
    mMidiMsgsFromProcessor.Push(msg); // queue incoming MIDI for UI
  }
  
  while (mMidiMsgsFromEditor.Pop(msg))
  {
    ProcessMidiMsg(msg);
  }

  //Do not handle Sysex messages here - SendSysexMsgFromUI overridden

  _ProcessBuffers(0.0, GetBlockSize());
}
