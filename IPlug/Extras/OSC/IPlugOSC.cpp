#include "IPlugOSC.h"

using namespace iplug;

std::unique_ptr<Timer> OSCInterface::mTimer;
int OSCInterface::sInstances = 0;
WDL_PtrList<OSCDevice> gDevices;

#ifdef OS_WIN
#define XSleep Sleep
#else
void XSleep(int ms) { usleep(ms?ms*1000:100); }
#endif

OSCDevice::OSCDevice(const char* dest, int maxpacket, int sendsleep, sockaddr_in* listen_addr)
{
  mHasOutput = dest != nullptr;
  mHasInput = listen_addr != nullptr;

  memset(&mSendAddress, 0, sizeof(mSendAddress));
  mMaxMacketSize = maxpacket > 0 ? maxpacket : 1024;
  mSendSleep = sendsleep >= 0 ? sendsleep : 10;
  mSendSocket = socket(AF_INET, SOCK_DGRAM, 0);

  if (mSendSocket == INVALID_SOCKET)
  {
    //TODO:
  }
  else if (listen_addr)
  {
    mReceiveAddress = *listen_addr;
    int on = 1;
    setsockopt(mSendSocket, SOL_SOCKET, SO_BROADCAST, (char*)&on, sizeof(on));
    if (!bind(mSendSocket, (struct sockaddr*) & mReceiveAddress, sizeof(struct sockaddr)))
    {
      SET_SOCK_BLOCK(mSendSocket, false);
    }
    else
    {
      closesocket(mSendSocket);
      mSendSocket = INVALID_SOCKET;
    }
  }
  else
  {
    mDestination.Set(dest);

    WDL_String tmp(dest);
    int sendport = 0;
    char* p = strstr(tmp.Get(), ":");
    if (p)
    {
      *p++ = 0;
      sendport = atoi(p);
    }
    if (!sendport) sendport = 8000;

    mSendAddress.sin_family = AF_INET;
    mSendAddress.sin_addr.s_addr = inet_addr(tmp.Get());
    mSendAddress.sin_port = htons(sendport);

    int on = 1;
    setsockopt(mSendSocket, SOL_SOCKET, SO_BROADCAST, (char*)&on, sizeof(on));
    SET_SOCK_BLOCK(mSendSocket, false);
  }
}

OSCDevice::~OSCDevice()
{
  if (mSendSocket != INVALID_SOCKET)
  {
    shutdown(mSendSocket, SHUT_RDWR);
    closesocket(mSendSocket);
    mSendSocket = INVALID_SOCKET;
  }
}

void OSCDevice::RunInput()
{
  if (mSendSocket == INVALID_SOCKET)
    return;

  struct sockaddr* p = mDestination.GetLength() ? nullptr : (struct sockaddr*) & mSendAddress;

  for (;;)
  {
    char buf[16384];
    buf[0] = 0;
    socklen_t plen = (socklen_t)sizeof(mSendAddress);
    const int len = (int)recvfrom(mSendSocket, buf, sizeof(buf), 0, p, p ? &plen : nullptr);

    if (len < 1)
      break;

    OnMessage(1, (const unsigned char*)buf, len);
  }
}

void OSCDevice::RunOutput()
{
  static char hdr[16] = { '#', 'b', 'u', 'n', 'd', 'l', 'e', 0, 0, 0, 0, 0, 1, 0, 0, 0 };

  // send mSendQueue as UDP blocks
  if (mSendQueue.Available() <= 16)
  {
    if (mSendQueue.Available() > 0) mSendQueue.Clear();
    return;
  }
  // mSendQueue should begin with a 16 byte pad, then messages in OSC

  char* packetstart = (char*)mSendQueue.Get();
  int packetlen = 16;
  bool hasbundle = false;
  mSendQueue.Advance(16); // skip bundle for now, but keep it around

  SET_SOCK_BLOCK(mSendSocket, true);

  while (mSendQueue.Available() >= sizeof(int))
  {
    int len = *(int*)mSendQueue.Get(); // not advancing
    OSC_MAKEINTMEM4BE((char*)&len);

    if (len < 1 || len > MAX_OSC_MSG_LEN || len > mSendQueue.Available()) break;

    if (packetlen > 16 && packetlen + sizeof(int) + len > mMaxMacketSize)
    {
      // packet is full
      if (!hasbundle)
      {
        packetstart += 20;
        packetlen -= 20;
      }
      else
      {
        memcpy(packetstart, hdr, 16);
      }

      sendto(mSendSocket, packetstart, packetlen, 0, (struct sockaddr*) & mSendAddress, sizeof(mSendAddress));
      if (mSendSleep > 0)
        XSleep(mSendSleep);

      packetstart = (char*)mSendQueue.Get() - 16; // safe since we padded the queue start
      packetlen = 16;
      hasbundle = false;
    }

    if (packetlen > 16) hasbundle = true;
    mSendQueue.Advance(sizeof(int) + len);
    packetlen += sizeof(int) + len;
  }

  if (packetlen > 16)
  {
    if (!hasbundle)
    {
      packetstart += 20;
      packetlen -= 20;
    }
    else
    {
      memcpy(packetstart, hdr, 16);
    }
    sendto(mSendSocket, packetstart, packetlen, 0, (struct sockaddr*) & mSendAddress, sizeof(mSendAddress));
    if (mSendSleep > 0)
      XSleep(mSendSleep);
  }
  SET_SOCK_BLOCK(mSendSocket, false);

  mSendQueue.Clear();
}

void OSCDevice::AddInstance(void(*callback)(void* d1, int dev_idx, int msglen, void* msg), void* d1, int dev_idx)
{
  const rec r = { callback, d1, dev_idx };
  mInstances.Add(r);
}

void OSCDevice::OnMessage(char type, const unsigned char* msg, int len)
{
  const int n = mInstances.GetSize();
  const rec* r = mInstances.Get();
  for (int x = 0; x < n; x++)
    if (r[x].callback) r[x].callback(r[x].data1, r[x].dev_idx, len, (void*)msg);
}

void OSCDevice::SendOSC(const char* src, int len)
{
  if (!mSendQueue.GetSize())
    mSendQueue.Add(nullptr, 16);

  int tlen = len;
  OSC_MAKEINTMEM4BE(&tlen);
  mSendQueue.Add(&tlen, sizeof(tlen));
  mSendQueue.Add(src, len);
}

//static
void OSCInterface::MessageCallback(void* d1, int dev_idx, int len, void* msg)
{
  OSCInterface* _this = (OSCInterface*)d1;

  if (_this && msg)
  {
    if (_this->mIncomingEvents.GetSize() < 65536 * 8)
    {
      const int this_sz = ((sizeof(incomingEvent) + (len - 3)) + 7) & ~7;

      _this->mIncomingEvents_mutex.Enter();
      const int oldsz = _this->mIncomingEvents.GetSize();
      _this->mIncomingEvents.Resize(oldsz + this_sz, false);

      if (_this->mIncomingEvents.GetSize() == oldsz + this_sz)
      {
        incomingEvent* item = (incomingEvent*)((char*)_this->mIncomingEvents.Get() + oldsz);
        item->dev_ptr = _this->mDevices.Get(dev_idx);
        item->sz = len;
        memcpy(item->msg, msg, len);
      }
      _this->mIncomingEvents_mutex.Leave();
    }
  }
}

void OSCInterface::OnTimer(Timer& timer)
{
  const int nDevices = gDevices.GetSize();

  for (auto i = 0; i < nDevices; i++)
  {
    auto* pDev = gDevices.Get(i);
    if (pDev->mHasInput)
      pDev->RunInput();
  }

  if (mIncomingEvents.GetSize())
  {
    static WDL_HeapBuf tmp;

    mIncomingEvents_mutex.Enter();
    tmp.CopyFrom(&mIncomingEvents, false);
    mIncomingEvents.Resize(0, false);
    mIncomingEvents_mutex.Leave();

    int pos = 0;
    const int endpos = tmp.GetSize();
    while (pos < endpos + 1 - sizeof(incomingEvent))
    {
      incomingEvent* evt = (incomingEvent*)((char*)tmp.Get() + pos);

      const int this_sz = ((sizeof(incomingEvent) + (evt->sz - 3)) + 7) & ~7;

      if (pos + this_sz > endpos) break;
      pos += this_sz;

      int rd_pos = 0;
      int rd_sz = evt->sz;
      if (evt->sz > 20 && !strcmp((char*)evt->msg, "#bundle"))
      {
        rd_sz = *(int*)(evt->msg + 16);
        OSC_MAKEINTMEM4BE(&rd_sz);
        rd_pos += 20;
      }
      //        if (m_var_msgs[3]) m_var_msgs[3][0] = evt->dev_ptr ? *evt->dev_ptr : -1.0;

      while (rd_pos + rd_sz <= evt->sz && rd_sz >= 0)
      {
        OscMessageRead rmsg((char*)evt->msg + rd_pos, rd_sz);

        const char* mstr = rmsg.GetMessage();
        if (mstr && *mstr)
          OnOSCMessage(rmsg);

        rd_pos += rd_sz + 4;
        if (rd_pos >= evt->sz) break;

        rd_sz = *(int*)(evt->msg + rd_pos - 4);
        OSC_MAKEINTMEM4BE(&rd_sz);
      }
    }
  }

  for (auto i = 0; i < nDevices; i++)
  {
    auto* pDev = gDevices.Get(i);
    if (pDev->mHasOutput)
      pDev->RunOutput();  // send queued messages
  }
}

OSCInterface::OSCInterface(OSCLogFunc logFunc)
: mLogFunc(logFunc)
{
  JNL::open_socketlib();

  if (!mTimer)
    mTimer = std::unique_ptr<Timer>(Timer::Create(std::bind(&OSCInterface::OnTimer, this, std::placeholders::_1), OSC_TIMER_RATE));

  sInstances++;
}

OSCInterface::~OSCInterface()
{
  if (--sInstances == 0) {
    mTimer = nullptr;
    gDevices.Empty(true);
  }
}

OSCDevice* OSCInterface::CreateReceiver(WDL_String& log, int port)
{
  const char buf[] = "127.0.0.1";

  struct sockaddr_in addr;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_family = AF_INET;
  if (buf[0] && buf[0] != '*') addr.sin_addr.s_addr = inet_addr(buf);
  if (addr.sin_addr.s_addr == INADDR_NONE) addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  int x;
  bool isReuse = false;
  OSCDevice* r = nullptr;
  for (x = 0; x < gDevices.GetSize(); x++)
  {
    OSCDevice* dev = gDevices.Get(x);
    if (dev && dev->mHasInput)
    {
      if (dev->mReceiveAddress.sin_port == addr.sin_port && dev->mReceiveAddress.sin_addr.s_addr == addr.sin_addr.s_addr)
      {
        r = dev;
        isReuse = true;

        log.AppendFormatted(1024, "Attached to already-opened listener '%s:%i'\r\n", buf, port);

        break;
      }
    }
  }

  if (!r)
  {
    std::unique_ptr<OSCDevice> device(new OSCDevice(nullptr, 0, -1, &addr));

    if (device->mSendSocket == INVALID_SOCKET)
    {
      log.AppendFormatted(1024, "Error listening for '%s:%i'\r\n", buf, port);
    }
    else
    {
      r = device.release();
      log.AppendFormatted(1024, "Listening for OSC on '%s:%i'\r\n", buf, port);
    }
  }

  if (r)
  {
    r->AddInstance(MessageCallback, this, mDevices.GetSize());
    mDevices.Add(r);

    if (!isReuse)
      gDevices.Add(r);
  }

  return r;
}

OSCDevice* OSCInterface::CreateSender(WDL_String& log, const char* ip, int port)
{
  WDL_String destStr;
  destStr.SetFormatted(256, "%s:%i", ip, port);
  OSCDevice* r = nullptr;
  bool isReuse = false;
  for (auto x = 0; x < gDevices.GetSize(); x++)
  {
    OSCDevice* d = gDevices.Get(x);
    if (d && d->mHasOutput)
    {
      if (!strcmp(d->mDestination.Get(), destStr.Get()))
      {
        isReuse = true;
        r = d; // reuse!
        break;
      }
    }
  }

  if (!r)
  {
    isReuse = false;
    std::unique_ptr<OSCDevice> device(new OSCDevice(destStr.Get(), 0, -1, nullptr));
    if (device->mSendSocket == INVALID_SOCKET)
    {
      log.AppendFormatted(1024, "Warning: failed creating destination for output '%s'\n", destStr.Get());
    }
    else
    {
      r = device.release();
    }
  }

  if (r)
  {
    log.AppendFormatted(1024, "Set destination: '%s'\n", destStr.Get());

    r->AddInstance(MessageCallback, this, mDevices.GetSize());
    mDevices.Add(r);

    if (!isReuse)
      gDevices.Add(r);
  }

  return r;
}

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
    
    if(mLogFunc)
      mLogFunc(log);
  }
}

void OSCSender::SendOSCMessage(OscMessageWrite& msg)
{
  int len;
  const char* msgStr = msg.GetBuffer(&len);
  mDevice->SendOSC(msgStr, len);
}

OSCReceiver::OSCReceiver(int port, OSCLogFunc logFunc)
: OSCInterface(logFunc)
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
    
    if(mLogFunc)
      mLogFunc(log);
  }
}
