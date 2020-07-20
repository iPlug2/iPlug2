/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file IPlug Open Sound Control (OSC) support
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

/** /todo */
class OSCDevice
{
public:
  /** Construct a new OSCDevice object
   * @param dest 
   * @param maxpacket 
   * @param sendsleep 
   * @param listen_addr */
  OSCDevice(const char* dest, int maxpacket, int sendsleep, struct sockaddr_in* listen_addr);
  
  virtual ~OSCDevice();
  
  /** /todo */
  void RunInput();
  
  /** /todo */
  void RunOutput();
  
  /** /todo */
  void AddInstance(void (*callback)(void* d1, int dev_idx, int msglen, void* msg), void* d1, int dev_idx);

  /** /todo
   * @param type 
   * @param msg 
   * @param len */
  void OnMessage(char type, const unsigned char* msg, int len);
  
  /** /todo
   * @param src 
   * @param len */
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

/** /todo */
class OSCInterface
{
  struct incomingEvent
  {
    OSCDevice* dev_ptr;
    int sz; // size of msg
    unsigned char msg[3];
  };
  
public:
  /** Construct a new OSCInterface object
  * @param logFunc */
  OSCInterface(OSCLogFunc logFunc = nullptr);
  
  virtual ~OSCInterface();
  
  OSCInterface(const OSCInterface&) = delete;
  OSCInterface& operator=(const OSCInterface&) = delete;
  
  /** Create a Receiver object
   * @param log 
   * @param port 
   * @return OSCDevice* */
  OSCDevice* CreateReceiver(WDL_String& log, int port = 8000);
  
  /** Create a Sender object
   * @param log 
   * @param port 
   * @return OSCDevice* */
  OSCDevice* CreateSender(WDL_String& log, const char* ip = "127.0.0.1", int port = 8000);
  
public:
  /** /todo
   * @param msg */
  virtual void OnOSCMessage(OscMessageRead& msg) {};
  
  /** Set the Log Func object
   * @param logFunc */
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

/** /todo */
class OSCSender : public OSCInterface
{
public:
  /** Construct a new OSCSender object
   * @param destIP 
   * @param port 
   * @param logFunc  */
  OSCSender(const char* destIP = "127.0.0.1", int port = 8000, OSCLogFunc logFunc = nullptr);
  
  /** Set the Destination object
   * @param ip 
   * @param port */
  void SetDestination(const char* ip, int port);
  
  /**
   * @param msg */
  void SendOSCMessage(OscMessageWrite& msg);
private:
  int mPort = 0;
  WDL_String mDestIP;
  OSCDevice* mDevice = nullptr;
};

class OSCReceiver : public OSCInterface
{
public:
  /** @brief Construct a new OSCReceiver object
   * @param port 
   * @param logFunc */
  OSCReceiver(int port = 8000, OSCLogFunc logFunc = nullptr);
  
  /** Set the Receive Port object
   * @param port */
  void SetReceivePort(int port);
  
  /** /todo */
  virtual void OnOSCMessage(OscMessageRead& msg) = 0;
  
private:
  OSCDevice* mDevice = nullptr;
  int mPort = 0;
  char mReadBuf[MAX_OSC_MSG_LEN] = {};
};


END_IPLUG_NAMESPACE
