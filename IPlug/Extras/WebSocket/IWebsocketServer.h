/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
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
  IWebsocketServer();
  virtual ~IWebsocketServer();
  bool CreateServer(const char* DOCUMENT_ROOT, const char* PORT = "8001");

  void DestroyServer();
  
  void GetURL(WDL_String& url);

  int NClients();
  
  bool SendTextToConnection(int idx, const char* str, int exclude = -1);
  
  bool SendDataToConnection(int idx, void* pData, size_t sizeInBytes, int exclude = -1);
  
  virtual void OnWebsocketReady(int idx);
  
  virtual bool OnWebsocketText(int idx, const char* str, size_t dataSize);
  
  virtual bool OnWebsocketData(int idx, void* pData, size_t dataSize);
  
private:
  bool DoSendToConnection(int idx, int opcode, const char* pData, size_t sizeInBytes, int exclude);
  
  // CivetWebSocketHandler
  bool handleConnection(CivetServer* pServer, const struct mg_connection* pConn) override;
  
  void handleReadyState(CivetServer* pServer, struct mg_connection* pConn) override;
  
  bool handleData(CivetServer* pServer, struct mg_connection* pConn, int bits, char* pData, size_t dataSize) override;
  
  void handleClose(CivetServer* pServer, const struct mg_connection* pConn) override;
  
  WDL_PtrList<const struct mg_connection> mConnections;
  static CivetServer* sServer;
  static int sInstances;

protected:
  WDL_Mutex mMutex;
};
