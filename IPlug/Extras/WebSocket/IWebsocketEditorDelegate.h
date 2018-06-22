#pragma once

#include "IGraphicsEditorDelegate.h"
#include "Websocket.h"
#include "IPlugStructs.h"
#include "IPlugQueue.h"

/**
 * @file
 * @copydoc IWebsocketEditorDelegate
 */

/** An IDelgate base class ... */
class IWebsocketEditorDelegate : public IGraphicsEditorDelegate, public IWebsocketServer
{
public:
  IWebsocketEditorDelegate(int nParams);
  ~IWebsocketEditorDelegate();
 
  //IWebsocketServer
  //THESE MESSAGES ARE ALL CALLED ON SERVER THREADS
  virtual void OnWebsocketReady(int idx) override;
  virtual bool OnWebsocketText(int idx, void* pData, size_t dataSize) override;
  virtual bool OnWebsocketData(int idx, void* pData, size_t dataSize) override;

  //IEditorDelegate
//  virtual void BeginInformHostOfParamChangeFromUI(int paramIdx) override;
  virtual void SendParameterValueFromUI(int paramIdx, double normalizedValue) override;
//  virtual void EndInformHostOfParamChangeFromUI(int paramIdx) override;

  virtual void SendControlValueFromDelegate(int controlTag, double normalizedValue) override;
  virtual void SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize = 0, const void* pData = nullptr) override;
  virtual void SendArbitraryMsgFromDelegate(int messageTag, int dataSize, const void* pData) override;
  virtual void SendMidiMsgFromDelegate(const IMidiMsg& msg) override;
  virtual void SendSysexMsgFromDelegate(const ISysEx& msg) override;
//  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
  
  // Call this repeatedly in order to handle incoming data
  void ProcessWebsocketQueue();
  
private:
  IPlugQueue<IParamChange> mParamChangeFromClients;
};
