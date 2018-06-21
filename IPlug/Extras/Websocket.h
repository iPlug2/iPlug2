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

  ~IWebsocketServer()
  {
    DestroyServer();
  }

  void CreateServer(const char* DOCUMENT_ROOT, const char* PORT = "8001")
  {
    const char *options[] = {
      "document_root", DOCUMENT_ROOT, "listening_ports", PORT, 0};

    std::vector<std::string> cpp_options;
    for (auto i=0; i<(sizeof(options)/sizeof(options[0])-1); i++) {
      cpp_options.push_back(options[i]);
    }

    mServer = new CivetServer(cpp_options);
    mServer->addWebSocketHandler("/websocket", this);
    DBGMSG("Websocket server running at http://localhost:%s/ws\n", PORT);
  }

  void DestroyServer()
  {
    if(mServer)
      delete mServer;
  }
  
  void GetURL(WDL_String& url)
  {
    std::vector<int> listeningPorts = mServer->getListeningPorts();
    url.SetFormatted(256, "http://localhost:%i", listeningPorts[0]);
  }

  int NClients()
  {
    return mConnections.GetSize();
  }
  
  bool SendTextToConnection(int idx, const char* str)
  {
    return DoSendToConnection(idx, MG_WEBSOCKET_OPCODE_TEXT, str, strlen(str));
  }
  
  bool SendDataToConnection(int idx, void* pData, size_t sizeInBytes)
  {
    return DoSendToConnection(idx, MG_WEBSOCKET_OPCODE_BINARY, (const char*) pData, sizeInBytes);
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
  bool DoSendToConnection(int idx, int opcode, const char* pData, size_t sizeInBytes)
  {
    std::function<bool(int)> sendFunc = [&](int connIdx) {
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
      for(int i=0;i<NClients();i++)
      {
        success &= sendFunc(i);
      }
    }
    else {
      sendFunc(idx);
    }
    
    return success;
  }
  
  // CivetWebSocketHandler
  bool handleConnection(CivetServer* pServer, const struct mg_connection* pConn) override
  {    
    mConnections.Add(pConn);
    
    DBGMSG("WS connected NClients %i\n", NClients());

    return true;
  }
  
  void handleReadyState(CivetServer* pServer, struct mg_connection* pConn) override
  {
    DBGMSG("WS ready\n");
    
    OnWebsocketReady(NClients()-1);
  }
  
  bool handleData(CivetServer* pServer, struct mg_connection* pConn, int bits, char* pData, size_t dataSize) override
  {
    DBGMSG("WS data\n");

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
    mConnections.DeletePtr(pConn);
    
    DBGMSG("WS closed NClients %i\n", NClients());
  }
  
  
  WDL_PtrList<const struct mg_connection> mConnections;
  CivetServer* mServer = nullptr;
};
