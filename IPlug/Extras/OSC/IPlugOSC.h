#pragma once

/**
 * @file IPlug Open Sound Control (OSC) support
 * The code in this file is mostly copied from Cockos OSCII-BOT https://github.com/justinfrankel/oscii-bot
 * OSCII-BOT is licensed under the GPL, but this copy has been authorised for use under the WDL licence by Cockos Inc.
 *
 */

#include <cstring>
#include <ctime>
#include <unistd.h>

#include "jnetlib/jnetlib.h"

#include "IPlugOSC_msg.h"
#include "IPlugTimer.h"

class IODevice
{
protected:
  IODevice ()
  {
    m_has_input = false;
    m_has_output = false;
    m_last_open_time = 0.0;
  }
  struct rec
  {
    void (*callback)(void *d1, void *d2, char type, int msglen, void *msg); // type=0 for MIDI, 1=osc
    void *data1;
    void *data2;
  };
  
  WDL_TypedBuf<rec> m_instances;
public:
  double m_last_open_time;
  bool m_has_input, m_has_output;
  
  virtual ~IODevice () {};
  virtual void start() {}
  virtual void run_input(WDL_FastString& textOut)=0;
  virtual void run_output(WDL_FastString& textOut)=0;
  virtual const char *get_type()=0;
  
  virtual void oscSend(const char *src, int len) {}
  virtual void midiSend(const unsigned char *buf, int len) {}
  
  virtual void addinst(void (*callback)(void *d1, void *d2, char type, int msglen, void *msg), void *d1, void *d2)
  {
    const rec r={callback,d1,d2};
    m_instances.Add(r);
  }
  
  virtual void onMessage(char type, const unsigned char *msg, int len)
  {
    const int n=m_instances.GetSize();
    const rec *r = m_instances.Get();
    for (int x=0;x<n; x++)
      if (r[x].callback) r[x].callback(r[x].data1,r[x].data2,type,len,(void*)msg);
  }
};

class OSCDevice : public IODevice
{
public:
  OSCDevice(const char* dest, int maxpacket, int sendsleep, struct sockaddr_in* listen_addr)
  {
    m_has_output = m_has_input = true;
    memset(&m_sendaddr, 0, sizeof(m_sendaddr));
    m_maxpacketsz = maxpacket > 0 ? maxpacket : 1024;
    m_sendsleep = sendsleep >= 0 ? sendsleep : 10;
    m_sendsock = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (m_sendsock == INVALID_SOCKET)
    {
      //TODO:
    }
    else if (listen_addr)
    {
      m_recvaddr = *listen_addr;
      int on = 1;
      setsockopt(m_sendsock, SOL_SOCKET, SO_BROADCAST, (char*)&on, sizeof(on));
      if (!bind(m_sendsock, (struct sockaddr*)&m_recvaddr, sizeof(struct sockaddr)))
      {
        SET_SOCK_BLOCK(m_sendsock, false);
      }
      else
      {
        closesocket(m_sendsock);
        m_sendsock = INVALID_SOCKET;
      }
    }
    else
    {
      m_dest.Set(dest);
      
      WDL_String tmp(dest);
      int sendport = 0;
      char *p = strstr(tmp.Get(),":");
      if (p)
      {
        *p++ = 0;
        sendport = atoi(p);
      }
      if (!sendport) sendport=8000;
      
      m_sendaddr.sin_family = AF_INET;
      m_sendaddr.sin_addr.s_addr = inet_addr(tmp.Get());
      m_sendaddr.sin_port = htons(sendport);
      
      int on = 1;
      setsockopt(m_sendsock, SOL_SOCKET, SO_BROADCAST, (char*)&on, sizeof(on));
      SET_SOCK_BLOCK(m_sendsock, false);
    }
  }
  
  virtual ~OSCDevice()
  {
    if (m_sendsock != INVALID_SOCKET)
    {
      shutdown(m_sendsock, SHUT_RDWR);
      closesocket(m_sendsock);
      m_sendsock = INVALID_SOCKET;
    }
  }
  
  virtual void run_input(WDL_FastString &textOut)
  {
    if (m_sendsock == INVALID_SOCKET) return;
    struct sockaddr *p = m_dest.GetLength() ? nullptr : (struct sockaddr *) &m_sendaddr;
    for (;;)
    {
      char buf[16384];
      buf[0] = 0;
      socklen_t plen = (socklen_t) sizeof(m_sendaddr);
      const int len = (int)recvfrom(m_sendsock, buf, sizeof(buf), 0, p, p?&plen:nullptr);
      if (len<1) break;
      
      onMessage(1,(const unsigned char *)buf,len);
    }
  }
  
  virtual void run_output(WDL_FastString &results)
  {
    static char hdr[16] = { '#', 'b', 'u', 'n', 'd', 'l', 'e', 0, 0, 0, 0, 0, 1, 0, 0, 0 };
    
    // send m_sendq as UDP blocks
    if (m_sendq.Available() <= 16)
    {
      if (m_sendq.Available() > 0) m_sendq.Clear();
      return;
    }
    // m_sendq should begin with a 16 byte pad, then messages in OSC
    
    char* packetstart = (char*) m_sendq.Get();
    int packetlen = 16;
    bool hasbundle = false;
    m_sendq.Advance(16); // skip bundle for now, but keep it around
    
    SET_SOCK_BLOCK(m_sendsock, true);
    
    while (m_sendq.Available() >= sizeof(int))
    {
      int len = *(int*) m_sendq.Get(); // not advancing
      OSC_MAKEINTMEM4BE((char*)&len);
      
      if (len < 1 || len > MAX_OSC_MSG_LEN || len > m_sendq.Available()) break;
      
      if (packetlen > 16 && packetlen+sizeof(int)+len > m_maxpacketsz)
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
        
        sendto(m_sendsock, packetstart, packetlen, 0, (struct sockaddr*)&m_sendaddr, sizeof(m_sendaddr));
        if (m_sendsleep>0) Sleep(m_sendsleep);
        
        packetstart = (char*) m_sendq.Get()-16; // safe since we padded the queue start
        packetlen = 16;
        hasbundle = false;
      }
      
      if (packetlen > 16) hasbundle = true;
      m_sendq.Advance(sizeof(int)+len);
      packetlen += sizeof(int)+len;
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
        memcpy(packetstart,hdr,16);
      }
      sendto(m_sendsock, packetstart, packetlen, 0, (struct sockaddr*) &m_sendaddr, sizeof(m_sendaddr));
      if (m_sendsleep > 0) Sleep(m_sendsleep);
    }
    SET_SOCK_BLOCK(m_sendsock, false);
    
    m_sendq.Clear();
  }
  
  virtual void oscSend(const char* src, int len)
  {
    if (!m_sendq.GetSize()) m_sendq.Add(nullptr,16);
    
    int tlen = len;
    OSC_MAKEINTMEM4BE(&tlen);
    m_sendq.Add(&tlen,sizeof(tlen));
    m_sendq.Add(src,len);
  }
  
  virtual const char* get_type() { return "OSC"; }
  
  SOCKET m_sendsock;
  int m_maxpacketsz, m_sendsleep;
  struct sockaddr_in m_sendaddr;
  WDL_Queue m_sendq;
  WDL_String m_dest;
  
  struct sockaddr_in m_recvaddr;
  WDL_Queue m_recvq;
};

WDL_PtrList<IODevice > g_devices;

class OSCInterface
{
public:
  OSCInterface()
  {
    JNL::open_socketlib();
  }
  
  static void messageCallback(void *d1, void *d2, char type, int msglen, void *msg)
  {
    printf("got something\n");
  }
  
  void CreateReciever(WDL_String& results)
  {
    int port = 8000;
    const char buf[] = "127.0.0.1";
    
    struct sockaddr_in addr;
    addr.sin_addr.s_addr=INADDR_ANY;
    addr.sin_family = AF_INET;
    if (buf[0] && buf[0] != '*') addr.sin_addr.s_addr = inet_addr(buf);
    if (addr.sin_addr.s_addr == INADDR_NONE) addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    int x;
    bool is_reuse = false;
    OSCDevice *r = nullptr;
    for (x=0; x < g_devices.GetSize(); x++)
    {
      IODevice  *dev = g_devices.Get(x);
      if (dev && !strcmp(dev->get_type(),"OSC") && dev->m_has_input)
      {
        OSCDevice *od = (OSCDevice *)dev;
        if (od->m_recvaddr.sin_port == addr.sin_port && od->m_recvaddr.sin_addr.s_addr == addr.sin_addr.s_addr)
        {
          r=od;
          is_reuse=true;
          
          results.AppendFormatted(1024,"\tAttached to already-opened listener '%s:%i'\r\n", buf, port);
          
          break;
        }
      }
    }
    if (!r)
    {
      r = new OSCDevice(nullptr,0,-1,&addr);
      if (r->m_sendsock == INVALID_SOCKET)
      {
        delete r;
        r=nullptr;
        results.AppendFormatted(1024,"\tError listening for '%s:%i'\r\n", buf, port);
      }
      else
      {
        results.AppendFormatted(1024,"\tListening on '%s:%i'\r\n", buf, port);
      }
    }
    if (r)
    {
      r->addinst(messageCallback,this, 0 /*TODO WHAT WITH*/);
      
      if (!is_reuse) g_devices.Add(r);
    }
  }
  
  void CreateSender(WDL_String& results)
  {
    const char dp[] = "127.0.0.1:8000";
    OSCDevice *r = nullptr;
    int x;
    bool is_reuse = false;
    for (x=0; x<g_devices.GetSize(); x++)
    {
      IODevice  *d = g_devices.Get(x);
      if (d && !strcmp(d->get_type(),"OSC") && d->m_has_output)
      {
        OSCDevice *p = (OSCDevice *)d;
        if (!strcmp(p->m_dest.Get(), dp))
        {
          is_reuse = true;
          r = p; // reuse!
          break;
        }
      }
    }
    
    if (!r)
    {
      is_reuse = false;
      r = new OSCDevice(dp, 0, -1, nullptr);
      if (r->m_sendsock == INVALID_SOCKET)
      {
        results.AppendFormatted(1024,"\tWarning: failed creating destination for @output '%s' OSC '%s'\r\n",dp, dp);
        delete r;
        r=nullptr;
      }
    }
    
    if (r)
    {
      r->addinst(messageCallback,this, 0 /*TODO*/);

      if (!is_reuse) g_devices.Add(r);
    }
  }
  
public:
  void SendMsg(const char* msg, int len)
  {
    g_devices.Get(0)->oscSend(msg, len);
  }
};

class IOSCHandler : public ITimerCallback
{
public:
  IOSCHandler(int updateRateMs = 100)
  {
    if(mTimer != nullptr)
      mTimer = Timer::Create(*this, updateRateMs);
  }

  virtual ~IOSCHandler()
  {
    if(mTimer != nullptr)
      delete mTimer;
    
    mTimer = nullptr;
  }
  
  virtual void ProcessInput() {};
  virtual void ProcessOutput() {};

  void OnTimer(Timer& timer)
  {
    ProcessInput();
    ProcessOutput();
  }

protected:
  OSCInterface mOSCInterface;
  static Timer* mTimer; // TODO: probably should be static
  WDL_FastString results;
};

Timer* IOSCHandler::mTimer = nullptr;

class OSCSender : public IOSCHandler
{
public:
  OSCSender()
  {
    WDL_String str;
    mOSCInterface.CreateSender(str);
    DBGMSG("%s\n", str.Get());
  }
  
  virtual void ProcessOutput()
  {
    for (auto x = 0; x < g_devices.GetSize(); x++)
      g_devices.Get(x)->run_output(results);  // send queued messages
  }
  
  void SendOSCMessage(OscMessageWrite& msg)
  {
    int msgLength;
    const char* msgStr = msg.GetBuffer(&msgLength);
    mOSCInterface.SendMsg(msgStr, msgLength);
  }
};

class OSCReciever : public IOSCHandler
{
public:
  OSCReciever()
  {
    WDL_String str;
    mOSCInterface.CreateReciever(str);
    DBGMSG("%s\n", str.Get());
  }
  
  void ProcessInput() override
  {
    const int sizeOfData = results.GetLength();
    
    for (auto x = 0; x < g_devices.GetSize(); x++)
      g_devices.Get(x)->run_input(results);
    
    if (results.GetLength() != sizeOfData) // if some input device added results
    {
      OscMessageRead msg{mReadBuf, sizeOfData};
      OnOSCMessage(msg);
    }
  }
  
  virtual void OnOSCMessage(OscMessageRead& msg) = 0;
private:
  char mReadBuf[MAX_OSC_MSG_LEN];
};
