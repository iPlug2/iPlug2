//TODO:

//#include "easywsclient.hpp"
//#include "IPlugUtilities.h"
//
//using easywsclient::WebSocket;
//
//class IWebsocketClient
//{
//public:
//  IWebsocketClient()
//  {
//  }
//
//  ~IWebsocketClient()
//  {
//    CloseConnection();
//
//    DELETE_NULL(mWS);
//  }
//
//  bool ConnectToServer(const char* url = "ws://127.0.0.1:8001/ws")
//  {
//    assert(mWS == nullptr);
//
//    mWS = WebSocket::from_url(url);
//
//    if(mWS == nullptr)
//      return false;
//
//    while(mWS->getReadyState() != WebSocket::OPEN)
//    {
//      //block
//    }
//
//    OnConnection();
//
//    return true;
//  }
//
//  void CloseConnection()
//  {
//    if(mWS != nullptr)
//      mWS->close();
//  }
//
//  virtual void OnConnection()
//  {
//    DBGMSG("OnConnection\n");
//  }
//
//  virtual void OnWebsocketText(const char* msg)
//  {
//    DBGMSG("OnWebsocketText %s\n", msg);
//  }
//
//  virtual void OnWebsocketData(size_t size, const void* pData)
//  {
//    DBGMSG("OnWebsocketData size %zu\n", size);
//  }
//
//  void SendText(const char* str)
//  {
//    if(mWS)
//      mWS->send(std::string(str));
//  }
//
//  void Tick()
//  {
//    if (mWS->getReadyState() != WebSocket::CLOSED)
//    {
//      mWS->poll();
//      mWS->dispatch([&](const std::string& msg){ this->OnWebsocketText(msg.c_str()); });
//      mWS->dispatchBinary([&](const std::vector<uint8_t>& msg){ this->OnWebsocketData(msg.size(), msg.data()); });
//    }
//  }
//
//  bool IsConnectedToServer()
//  {
//    if(mWS != nullptr)
//      return mWS->getReadyState() == WebSocket::OPEN;
//
//    return false;
//  }
//
//private:
//
//  WebSocket::pointer mWS = nullptr;
//};
