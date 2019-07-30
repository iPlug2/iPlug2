/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifndef _IPLUGAPI_
#define _IPLUGAPI_

/**
 * @file
 * @copydoc IPlugAPP
 */


#include "IPlugPlatform.h"
#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

#include "config.h"

struct IPlugInstanceInfo
{
  void* pAppHost;
};

class IPlugAPPHost;

/**  Standalone application base class for an IPlug plug-in
*   @ingroup APIClasses */
class IPlugAPP : public IPlugAPIBase
               , public IPlugProcessor<PLUG_SAMPLE_DST>
{
public:
  IPlugAPP(IPlugInstanceInfo instanceInfo, IPlugConfig config);
  ~IPlugAPP();
  
  //IPlugAPIBase
  void BeginInformHostOfParamChange(int idx) override {};
  void InformHostOfParamChange(int idx, double normalizedValue) override {};
  void EndInformHostOfParamChange(int idx) override {};
  void InformHostOfProgramChange() override {};
  bool EditorResizeFromDelegate(int viewWidth, int viewHeight) override;

  //IEditorDelegate
  void SendSysexMsgFromUI(const ISysEx& msg) override;
  
  //IPlugProcessor
  bool SendMidiMsg(const IMidiMsg& msg) override;
  bool SendSysEx(const ISysEx& msg) override;
  
  //IPlugAPP
  void AppProcess(double** inputs, double** outputs, int nFrames);
  
#if APP_HAS_TRANSPORT_BAR
  /**
   * Overrides the LayoutUI from IGraphicsEditorDelegate to be able to modify
   * GUI and add the transport bar on tob of it
   * @param pGraphics a pointer to the graphics context to which the UI belongs
   */
  void LayoutUI(IGraphics* pGraphics) override;
  
  /**
   * Handles the messages received from the transport bar (such as tempo change or play/stop buttons)
   * @param messageTag the unique identifier of the message type
   * @param controlTag the unique identifier of the control who is sending the message
   * @param dataSize the size of the data passet with the data pointer
   * @param pData a pointer to the data sent by the message
   */
  bool OnMessage(int messageTag, int controlTag, int dataSize, const void* pData) override;
  
  
  /**
   * Call back for the timer that reads the playback status and updates the transport bar control
   * this is used when tempo or play status change when the APP is slaved to MIDI clock and SPP
   * @param t The ITimer reference responsible for the callback
   */
  void UpdateTransportStatus(Timer& t);
  
  /**
   * Sets the APP window title using the APP name and  the current open preset
   * or "untitled" if no  preset is currently open
   */
  void SetWindowTitle();
  
  /**
   * Toggles the grayed out status for the transport bar, called by the APP_host
   * when MIDI clock in is received
   * @param toggle if true the transport bar is grayed out
   */
  void GrayOutTransport(bool toggle);
#endif
  
private:
  IPlugAPPHost* mAppHost = nullptr;
  IPlugQueue<IMidiMsg> mMidiMsgsFromCallback {MIDI_TRANSFER_SIZE};
  IPlugQueue<SysExData> mSysExMsgsFromCallback {SYSEX_TRANSFER_SIZE};
  
#if APP_HAS_TRANSPORT_BAR
  std::unique_ptr<Timer> mTransportTimer;
  WDL_String mCurrentFileName;
#endif
  
  friend class IPlugAPPHost;
};

IPlugAPP* MakePlug(void* pAPPHost);

#endif
