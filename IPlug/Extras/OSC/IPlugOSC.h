/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file IPlug Open Sound Control (OSC) support
 * Public classes to implement sending and receiving of Open Sound Control messages
 * To use these classes, you need to add the following files from the WDL/jnetlib library
 * to your project.
 * 
 * WDL/jnetlib/util.cpp
 * WDL/jnetlib/listen.cpp
 * /WDL/jnetlib/asyncdns.cpp
 * /WDL/jnetlib/connection.cpp
 */

#include "IPlugOSC_internal.h"

BEGIN_IPLUG_NAMESPACE

using OSCMessageReceivedFunc = std::function<void(OscMessageRead& msg)>;

/** OSCSender interface, implement in order to send OSC messages */
class OSCSender : private OSCInterface
{
public:
  /** Construct a new OSCSender
   * @param ip The IP address to send messages to
   * @param port The port number on which to listen for messages
   * @param logFunc std::function to log connection details  */
  OSCSender(const char* ip = "127.0.0.1", int port = 8000, OSCLogFunc logFunc = nullptr);
  
  /** Set the destination ip and port
   * @param ip The IP address to send messages to
   * @param port The port number on which to listen for messages */
  void SetDestination(const char* ip, int port);
  
  /** Send an OSC message
   * @param msg The message that should be sent */
  void SendOSCMessage(OscMessageWrite& msg);
  
  /** Set a log function after construction
   * @param logFunc std::function to log connection details */
  void SetLogFunc(OSCLogFunc logFunc) { OSCInterface::SetLogFunc(logFunc); }

private:
  int mPort = 0;
  WDL_String mDestIP;
  OSCDevice* mDevice = nullptr;
};

/** OSCReceiver interface, implement in order to receive OSC messages */
class OSCReceiver : private OSCInterface
{
public:
  /** Construct a new OSCReceiver
   * @param port The port number on which to listen for messages
   * @param logFunc std::function to log connection details  */
  OSCReceiver(int port = 8000, OSCLogFunc logFunc = nullptr, OSCMessageReceivedFunc receiveFunc = nullptr);
  
  /** Set the port number on which to listen for OSC messages
   * @param port The port number on which to listen for messages*/
  void SetReceivePort(int port);
  
  /** Set a log function after construction
   * @param logFunc std::function to log connection details */
  void SetLogFunc(OSCLogFunc logFunc) { OSCInterface::SetLogFunc(logFunc); }

  /** Override to handle incoming OSC messages in a derived class */
  virtual void OnOSCMessage(OscMessageRead& msg)
  {
    if (mReceiveFunc)
    {
      mReceiveFunc(msg);
    }
  }
    
private:
  OSCMessageReceivedFunc mReceiveFunc = nullptr;
  OSCDevice* mDevice = nullptr;
  int mPort = 0;
  char mReadBuf[MAX_OSC_MSG_LEN] = {};
};

END_IPLUG_NAMESPACE
