#pragma once

#include "IGraphicsEditorDelegate.h"
#include "IWebsocketServer.h"
#include "IPlugStructs.h"
#include "IPlugQueue.h"

/**
 * @file
 * @copydoc IWebsocketEditorDelegate
 */

BEGIN_IPLUG_NAMESPACE

/** An IEditorDelegate base class that embeds a websocket server ... */
class IWebsocketEditorDelegate : public IGEditorDelegate, public IWebsocketServer
{
public:
  static constexpr int MAX_NUM_CLIENTS = 4;
  
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
//void BeginInformHostOfParamChangeFromUI(int paramIdx) override;
  void SendParameterValueFromUI(int paramIdx, double normalizedValue) override;
//void EndInformHostOfParamChangeFromUI(int paramIdx) override;

  void SendControlValueFromDelegate(int controlTag, double normalizedValue) override;
  void SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, const void* pData) override;
  void SendArbitraryMsgFromDelegate(int messageTag, int dataSize, const void* pData) override;
  void SendMidiMsgFromDelegate(const IMidiMsg& msg) override;
  void SendSysexMsgFromDelegate(const ISysEx& msg) override;
//  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
  
  // Call this repeatedly in order to handle incoming data
  void ProcessWebsocketQueue();
  
private:
  void DoSPVFDToClients(int paramIdx, double value, int excludeIdx);
  
  struct ParamTupleCX
  {
    int idx;
    double value;
    int connection;
    
    ParamTupleCX(int idx = kNoParameter, double value = 0., int connection = -1)
    : idx(idx)
    , value(value)
    , connection(connection)
    {}
  };

  IPlugQueue<ParamTupleCX> mParamChangeFromClients {PARAM_TRANSFER_SIZE};
  IPlugQueue<IMidiMsg> mMIDIFromClients {MIDI_TRANSFER_SIZE};
};

END_IPLUG_NAMESPACE
