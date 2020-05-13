/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include <memory>

#include "IPlugEditorDelegate.h"

/**
 * @file
 * @copydoc IGEditorDelegate
 */

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IGraphics;
class IControl;

/** An editor delegate base class for a SOMETHING that uses IGraphics for it's UI */
class IGEditorDelegate : public IEditorDelegate
{
  friend class IGraphics;
    
public:
  IGEditorDelegate(int nParams);
  ~IGEditorDelegate();

  IGEditorDelegate(const IGEditorDelegate&) = delete;
  IGEditorDelegate& operator=(const IGEditorDelegate&) = delete;
    
  //IEditorDelegate
  void* OpenWindow(void* pHandle) final;
  void CloseWindow() final;
  void SetScreenScale(double scale) final;
  
  bool OnKeyDown(const IKeyPress& key) override;
  bool OnKeyUp(const IKeyPress& key) override;
    
  // Default serialization implementations (which serialize the size/scale) = override for custom behaviours
  bool SerializeEditorState(IByteChunk& chunk) const override;
  int UnserializeEditorState(const IByteChunk& chunk, int startPos) override;
    
  //The rest should be final, but the WebSocketEditorDelegate needs to override them
  void SendControlValueFromDelegate(int ctrlTag, double normalizedValue) override;
  void SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize = 0, const void* pData = nullptr) override;
  void SendMidiMsgFromDelegate(const IMidiMsg& msg) override;
  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;

  /** Called to create the IGraphics instance for this editor. Default impl calls  mMakeGraphicsFunc */
  virtual IGraphics* CreateGraphics()
  {
    if(mMakeGraphicsFunc)
      return mMakeGraphicsFunc();
    else
      return nullptr;
  }
  
  /** Called to layout controls when the GUI is initially opened and again if the UI size changes. On subsequent calls you can check for the existence of controls and behave accordingly. Default impl calls  mLayoutFunc */
  virtual void LayoutUI(IGraphics* pGraphics)
  {
    if(mLayoutFunc)
      mLayoutFunc(pGraphics);
  }
  
  /** Get a pointer to the IGraphics context */
  IGraphics* GetUI() { return mGraphics.get(); };

  /** Serializes the size and scale of the IGraphics.
   * @param chunk The output chunk to serialize to. Will append data if the chunk has already been started.
   * @return \c true if the serialization was successful */
  bool SerializeEditorSize(IByteChunk& data) const;
  
  /** Unserializes the size and scale of the IGraphics.
   * @param chunk The incoming chunk where data is stored to unserialize
   * @param startPos The start position in the chunk where parameter values are stored
   * @return The new chunk position (endPos) */
  int UnserializeEditorSize(const IByteChunk& chunk, int startPos);
    
protected:
  std::function<IGraphics*()> mMakeGraphicsFunc = nullptr;
  std::function<void(IGraphics* pGraphics)> mLayoutFunc = nullptr;
private:
  std::unique_ptr<IGraphics> mGraphics;
  int mLastWidth = 0;
  int mLastHeight = 0;
  float mLastScale = 0.f;
  bool mClosing = false; // used to prevent re-entrancy on closing
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
