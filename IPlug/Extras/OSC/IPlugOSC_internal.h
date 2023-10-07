/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file IPlug Open Sound Control (OSC) support, internal classes
 * The code in this file is mostly copied/hacked from Cockos OSCII-BOT https://github.com/justinfrankel/oscii-bot
 * OSCII-BOT is licensed under the GPL, but this copy has been authorised for use under the WDL licence by Cockos Inc.
 *
 */

#include <cstring>
#include <ctime>
#include <functional>
#include <memory>

#include "jnetlib/jnetlib.h"

#include "IPlugPlatform.h"
#include "IPlugLogger.h"
#include "IPlugOSC_msg.h"
#include "IPlugTimer.h"


BEGIN_IPLUG_NAMESPACE

#ifndef OSC_TIMER_RATE
static constexpr int OSC_TIMER_RATE = 100;
#endif

using OSCLogFunc = std::function<void(WDL_String& log)>;

class OSCDevice
{
public:
  OSCDevice(const char* dest, int maxpacket, int sendsleep, struct sockaddr_in* listen_addr);
  
  virtual ~OSCDevice();
  void RunInput();
  void RunOutput();
  void AddInstance(void (*callback)(void* d1, int dev_idx, int msglen, void* msg), void* d1, int dev_idx);
  void OnMessage(char type, const unsigned char* msg, int len);
  void SendOSC(const char* src, int len);

private:
  struct rec
  {
    void (*callback)(void* d1, int dev_idx, int msglen, void* msg);
    void* data1;
    int dev_idx;
  };

  WDL_TypedBuf<rec> mInstances;
public:
  double mLastOpenTime = 0;
  bool mHasInput = false;
  bool mHasOutput = false;
  
  SOCKET mSendSocket;
  int mMaxMacketSize, mSendSleep;
  WDL_String mDestination;
  
  struct sockaddr_in mSendAddress, mReceiveAddress;
  WDL_Queue mSendQueue, mReceiveQueue;
};

class OSCInterface
{
  struct incomingEvent
  {
    OSCDevice* dev_ptr;
    int sz; // size of msg
    unsigned char msg[3];
  };
  
public:
  OSCInterface(OSCLogFunc logFunc = nullptr);
  
  virtual ~OSCInterface();
  
  OSCInterface(const OSCInterface&) = delete;
  OSCInterface& operator=(const OSCInterface&) = delete;
  
  OSCDevice* CreateReceiver(WDL_String& log, int port = 8000);
  OSCDevice* CreateSender(WDL_String& log, const char* ip = "127.0.0.1", int port = 8000);
  
public:
  virtual void OnOSCMessage(OscMessageRead& msg) {};
  void SetLogFunc(OSCLogFunc logFunc) { mLogFunc = logFunc; }
  
private:
  static void MessageCallback(void *d1, int dev_idx, int msglen, void *msg);

  void OnTimer(Timer& timer);
  
  // these are non-owned refs
  WDL_PtrList<OSCDevice> mDevices;
  
protected:
  OSCLogFunc mLogFunc;
  static std::unique_ptr<Timer> mTimer;
  static int sInstances;
  WDL_HeapBuf mIncomingEvents;  // incomingEvent list, each is 8-byte aligned
  WDL_Mutex mIncomingEvents_mutex;
};

END_IPLUG_NAMESPACE
