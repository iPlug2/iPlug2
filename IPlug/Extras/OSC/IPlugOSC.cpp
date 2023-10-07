#include "IPlugOSC.h"

using namespace iplug;

extern WDL_PtrList<OSCDevice> gDevices;

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
    
    if (mDevice != nullptr)
    {
      gDevices.DeletePtr(mDevice, true);
    }

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
  mDevice->SendOSC(msgStr, len);
}

OSCReceiver::OSCReceiver(int port, OSCLogFunc logFunc, OSCMessageReceivedFunc receievFunc)
: OSCInterface(logFunc)
, mReceiveFunc(receievFunc)
{
  SetReceivePort(port);
}

void OSCReceiver::SetReceivePort(int port)
{
  if (port != mPort)
  {
    if (mDevice != nullptr)
    {
      gDevices.DeletePtr(mDevice, true);
    }

    WDL_String log;
    mDevice = CreateReceiver(log, port);
    mPort = port;
    
    if (mLogFunc)
      mLogFunc(log);
  }
}
