#include "IPlugOSC.h"

using namespace iplug;

OSCSender::OSCSender(const char* destIP, int port, OSCLogFunc logFunc)
: OSCInterface(logFunc)
, mDestIP("")
{
  SetDestination(destIP, port);
}

void OSCSender::SetDestination(const char* ip, int port)
{
  if (strcmp(ip, mDestIP.Get()) || port != mPort)
  {
    mDestIP.Set(ip);
    mPort = port;

    // Remove old device before creating new one to prevent memory leak
    RemoveDevice(mDevice);
    mDevice = nullptr;

    WDL_String log;
    mDevice = CreateSender(log, ip, port);

    if (mLogFunc)
      mLogFunc(log);
  }
}

void OSCSender::SendOSCMessage(OscMessageWrite& msg)
{
  int len;
  const char* msgStr = msg.GetBuffer(&len);
  if (mDevice)
    mDevice->SendOSC(msgStr, len);
}

OSCReceiver::OSCReceiver(int port, OSCLogFunc logFunc, OSCMessageReceivedFunc receiveFunc)
: OSCInterface(logFunc)
, mReceiveFunc(receiveFunc)
{
  SetReceivePort(port);
}

void OSCReceiver::SetReceivePort(int port)
{
  if (port != mPort)
  {
    // Remove old device before creating new one to prevent memory leak
    RemoveDevice(mDevice);
    mDevice = nullptr;

    WDL_String log;
    mDevice = CreateReceiver(log, port);
    mPort = port;

    if (mLogFunc)
      mLogFunc(log);
  }
}
