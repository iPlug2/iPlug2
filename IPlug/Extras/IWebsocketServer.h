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

#include "CivetServer.h"
#include <cstring>

#include "ptrlist.h"
#include "IPlugLogger.h"

#ifdef OS_WIN
#include <windows.h>
#else
#include <unistd.h>
#endif

class IWebsocketServer : public CivetWebSocketHandler
{
public:
  IWebsocketServer()
  {
  }

  virtual ~IWebsocketServer()
  {
    DestroyServer();
  }

  void CreateServer(const char* DOCUMENT_ROOT, const char* PORT = "8001")
  {
    const char *options[] = {"document_root", DOCUMENT_ROOT, "listening_ports", PORT, 0};

    std::vector<std::string> cpp_options;
    for (auto i=0; i<(sizeof(options)/sizeof(options[0])-1); i++) {
      cpp_options.push_back(options[i]);
    }
    
    if(sInstances == 0 && sServer == nullptr)
    {
      sServer = new CivetServer(cpp_options);
      sServer->addWebSocketHandler("/ws", this);
      DBGMSG("Websocket server running at http://localhost:%s/ws\n", PORT);
    }
    else {
      WDL_String url;
      GetURL(url);
      DBGMSG("Websocket server allready running at %s\n", url.Get());
    }
    
    sInstances++;
  }

  void DestroyServer()
  {
    sInstances--;
    
    if(sInstances == 0) {
      if(sServer) {
        delete sServer;
        sServer = nullptr;
      }
    }
  }
  
  void GetURL(WDL_String& url)
  {
    if(sServer) {
      std::vector<int> listeningPorts = sServer->getListeningPorts();
      url.SetFormatted(256, "http://localhost:%i", listeningPorts[0]);
    }
  }

  int NClients()
  {
    WDL_MutexLock lock(&mMutex);

    return mConnections.GetSize();
  }
  
  bool SendTextToConnection(int idx, const char* str, int exclude = -1)
  {
    return DoSendToConnection(idx, MG_WEBSOCKET_OPCODE_TEXT, str, strlen(str), exclude);
  }
  
  bool SendDataToConnection(int idx, void* pData, size_t sizeInBytes, int exclude = -1)
  {
    return DoSendToConnection(idx, MG_WEBSOCKET_OPCODE_BINARY, (const char*) pData, sizeInBytes, exclude);
  }
  
  virtual void OnWebsocketReady(int idx)
  {
  }
  
  virtual bool OnWebsocketText(int idx, void* pData, size_t dataSize)
  {
    return true; // return true to keep the connection open
  }
  
  virtual bool OnWebsocketData(int idx, void* pData, size_t dataSize)
  {
    return true; // return true to keep the connection open
  }
  
private:
  bool DoSendToConnection(int idx, int opcode, const char* pData, size_t sizeInBytes, int exclude)
  {
    int nclients = NClients();
    
    std::function<bool(int)> sendFunc = [&](int connIdx) {
      WDL_MutexLock lock(&mMutex);
      mg_connection* pConn = const_cast<mg_connection*>(mConnections.Get(connIdx));
      
      if(pConn) {
        if(mg_websocket_write(pConn, opcode, pData, sizeInBytes))
          return true;
      }
      
      return false;
    };
    
    bool success = false;
    
    if(idx == -1)
    {
      for(int i=0;i<nclients;i++) // TODO: sending to self?
      {
        if(i != exclude)
          success &= sendFunc(i);
      }
    }
    else {
      sendFunc(idx);
    }
    
    return success;
  }
  
  // CivetWebSocketHandler
  // These methods are called on the server thread
  bool handleConnection(CivetServer* pServer, const struct mg_connection* pConn) override
  {
    WDL_MutexLock lock(&mMutex);

    mConnections.Add(pConn);
    
    DBGMSG("WS connected NClients %i\n", NClients());

    return true;
  }
  
  void handleReadyState(CivetServer* pServer, struct mg_connection* pConn) override
  {
    WDL_MutexLock lock(&mMutex);
    
    DBGMSG("WS ready\n");
    
    OnWebsocketReady(NClients()-1); // should defer to main thread
  }
  
  bool handleData(CivetServer* pServer, struct mg_connection* pConn, int bits, char* pData, size_t dataSize) override
  {
    WDL_MutexLock lock(&mMutex);

    uint8_t* firstByte = (uint8_t*) &bits;
    
    if(*firstByte == 129) // TODO: check that
    {
      return OnWebsocketText(mConnections.Find(pConn), const_cast<char*>(pData), dataSize);
    }
    else if(*firstByte == 130) // TODO: check that
    {
      return OnWebsocketData(mConnections.Find(pConn), (void*) pData, dataSize);
    }
    
    return true;
  }
  
  void handleClose(CivetServer* pServer, const struct mg_connection* pConn) override
  {
    WDL_MutexLock lock(&mMutex);

    mConnections.DeletePtr(pConn);
    
    DBGMSG("WS closed NClients %i\n", NClients());
  }
  
  WDL_PtrList<const struct mg_connection> mConnections;
  static CivetServer* sServer;
  static int sInstances;

protected:
  WDL_Mutex mMutex;
};

CivetServer* IWebsocketServer::sServer = nullptr;
int IWebsocketServer::sInstances = 0;
