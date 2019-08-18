/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IWebsocketServer.h"

using namespace iplug;

IWebsocketServer::IWebsocketServer()
{
}

IWebsocketServer::~IWebsocketServer()
{
  DestroyServer();
}

bool IWebsocketServer::CreateServer(const char* DOCUMENT_ROOT, const char* PORT)
{
  const char *options[] = {"document_root", DOCUMENT_ROOT, "listening_ports", PORT, "extra_mime_types", ".wasm=application/wasm", 0};

  std::vector<std::string> cpp_options;
  for (auto i=0; i<(sizeof(options)/sizeof(options[0])-1); i++) {
    cpp_options.push_back(options[i]);
  }
  
  if (!sServer)
  {
    try { sServer = std::make_unique<CivetServer>(cpp_options); }
    catch (const std::exception& e)
    {
      DBGMSG("Couldn't create server, port probably already in use\n");
      return false;
    }
    
    sServer->addWebSocketHandler("/ws", this);
    DBGMSG("Websocket server running at http://localhost:%s/ws serving %s\n", PORT, DOCUMENT_ROOT);
  }
  else
  {
    WDL_String url;
    GetURL(url);
    DBGMSG("Websocket server already running at %s\n", url.Get());
  }
  
  sInstances++;
  
  return true;
}

void IWebsocketServer::DestroyServer()
{
  if (sInstances && (--sInstances == 0))
  {
    sServer = nullptr;
  }
}
  
void IWebsocketServer::GetURL(WDL_String& url)
{
  if(sServer)
  {
    std::vector<int> listeningPorts = sServer->getListeningPorts();
    url.SetFormatted(256, "http://localhost:%i", listeningPorts[0]);
  }
}

int IWebsocketServer::NClients()
{
  WDL_MutexLock lock(&mMutex);

  return mConnections.GetSize();
}

bool IWebsocketServer::SendTextToConnection(int idx, const char* str, int exclude)
{
  return DoSendToConnection(idx, MG_WEBSOCKET_OPCODE_TEXT, str, strlen(str), exclude);
}

bool IWebsocketServer::SendDataToConnection(int idx, void* pData, size_t sizeInBytes, int exclude)
{
  return DoSendToConnection(idx, MG_WEBSOCKET_OPCODE_BINARY, (const char*) pData, sizeInBytes, exclude);
}

void IWebsocketServer::OnWebsocketReady(int idx)
{
}

bool IWebsocketServer::OnWebsocketText(int idx, const char* str, size_t dataSize)
{
  return true; // return true to keep the connection open
}

bool IWebsocketServer::OnWebsocketData(int idx, void* pData, size_t dataSize)
{
  return true; // return true to keep the connection open
}

bool IWebsocketServer::DoSendToConnection(int idx, int opcode, const char* pData, size_t sizeInBytes, int exclude)
{
  int nclients = NClients();
  
  std::function<bool(int)> sendFunc = [&](int connIdx) {
    WDL_MutexLock lock(&mMutex);
    mg_connection* pConn = mConnections.Get(connIdx);
    
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
bool IWebsocketServer::handleConnection(CivetServer* pServer, const struct mg_connection* pConn)
{
  WDL_MutexLock lock(&mMutex);
  DBGMSG("WS connected\n");

  return true;
}

void IWebsocketServer::handleReadyState(CivetServer* pServer, struct mg_connection* pConn)
{
  WDL_MutexLock lock(&mMutex);
  
  mConnections.Add(pConn);
  
  DBGMSG("WS ready NClients %i\n", NClients());
  
  OnWebsocketReady(NClients()-1); // should defer to main thread
}

bool IWebsocketServer::handleData(CivetServer* pServer, struct mg_connection* pConn, int bits, char* pData, size_t dataSize)
{
  WDL_MutexLock lock(&mMutex);

  uint8_t* firstByte = (uint8_t*) &bits;
  
  if(*firstByte == 129) // TODO: check that
  {
    return OnWebsocketText(mConnections.Find(pConn), pData, dataSize);
  }
  else if(*firstByte == 130) // TODO: check that
  {
    return OnWebsocketData(mConnections.Find(pConn), (void*) pData, dataSize);
  }
  
  return true;
}

void IWebsocketServer::handleClose(CivetServer* pServer, const struct mg_connection* pConn)
{
  WDL_MutexLock lock(&mMutex);

  mConnections.DeletePtr(pConn);
  
  DBGMSG("WS closed NClients %i\n", NClients());
}

std::unique_ptr<CivetServer> IWebsocketServer::sServer;
int IWebsocketServer::sInstances = 0;
