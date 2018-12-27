#pragma once

#include "IGraphicsEditorDelegate.h"
#include "IWebsocketServer.h"
#include "IPlugStructs.h"
#include "IPlugQueue.h"

/**
 * @file
 * @copydoc IWebsocketEditorDelegate
 */

/** An IEditorDelegate base class that embeds a websocket server ... */
class IWebsocketEditorDelegate : public IGEditorDelegate, public IWebsocketServer
{
public:
  IWebsocketEditorDelegate(int nParams);
  virtual ~IWebsocketEditorDelegate();
 
  //IWebsocketServer
  //THESE MESSAGES ARE ALL CALLED ON SERVER THREADS - 1 PER WEBSOCKET CONNECTION
  void OnWebsocketReady(int idx) override;
  bool OnWebsocketText(int idx, const char* pStr, size_t dataSize) override;
  bool OnWebsocketData(int idx, void* pData, size_t dataSize) override;

  //IEditorDelegate
  void SendMidiMsgFromUI(const IMidiMsg& msg) override;
  void SendSysexMsgFromUI(const ISysEx& msg) override;
  void SendArbitraryMsgFromUI(int messageTag, int controlTag, int dataSize, const void* pData) override;
//  virtual void BeginInformHostOfParamChangeFromUI(int paramIdx) override;
  void SendParameterValueFromUI(int paramIdx, double normalizedValue) override;
//  virtual void EndInformHostOfParamChangeFromUI(int paramIdx) override;

  void SendControlValueFromDelegate(int controlTag, double normalizedValue) override;
  void SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, const void* pData) override;
  void SendArbitraryMsgFromDelegate(int messageTag, int dataSize, const void* pData) override;
  void SendMidiMsgFromDelegate(const IMidiMsg& msg) override;
  void SendSysexMsgFromDelegate(const ISysEx& msg) override;
//  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
  
  // Call this repeatedly in order to handle incoming data
  void ProcessWebsocketQueue();
  
private:
  IPlugQueue<IParamChange> mParamChangeFromClients; // TODO: This is a single producer single consumer queue - it is not sufficient, since each client connection will be on a different server thread
  IPlugQueue<IMidiMsg> mMIDIFromClients; // TODO: This is a single producer single consumer queue - it is not sufficient, since each client connection will be on a different server thread
};
