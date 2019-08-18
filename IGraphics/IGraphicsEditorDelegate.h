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
public:
  IGEditorDelegate(int nParams);
  ~IGEditorDelegate();

  IGEditorDelegate(const IGEditorDelegate&) = delete;
  IGEditorDelegate& operator=(const IGEditorDelegate&) = delete;
    
  //IEditorDelegate
  void* OpenWindow(void* pHandle) final;
  void CloseWindow() final;
  void SetScreenScale(double scale) final;

  //The rest should be final, but the WebSocketEditorDelegate needs to override them
  void SendControlValueFromDelegate(int controlTag, double normalizedValue) override;
  void SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize = 0, const void* pData = nullptr) override;
  void SendMidiMsgFromDelegate(const IMidiMsg& msg) override;
  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
  int SetEditorData(const IByteChunk& data, int startPos) override;

  /** If you override this method you must call the parent! */
  void OnUIOpen() override;

  //IGEditorDelegate
  /** Attach IGraphics context - only call this method if creating/populating your UI in your plug-in constructor.
   ** In that case do not override CreateGraphics()! */
  void AttachGraphics(IGraphics* pGraphics);
  
  /** Only override this method if you want to create IGraphics on demand (when UI window opens)! Implementation should return result of MakeGraphics() */
  virtual IGraphics* CreateGraphics()
  {
    if(mMakeGraphicsFunc)
      return mMakeGraphicsFunc();
    else
      return nullptr;
  }
  
  /** Only override this method if you want to create IGraphics on demand (when UI window opens), or layout controls differently for different UI sizes */
  virtual void LayoutUI(IGraphics* pGraphics)
  {
    if(mLayoutFunc)
      mLayoutFunc(pGraphics);
  }
  
  /** Get a pointer to the IGraphics context */
  IGraphics* GetUI() { return mGraphics.get(); };

  /** Called from the UI to resize the editor via the plugin and store editor in the base.
   & This calls through to EditorResizeFromUI after updating the data.
   * @return \c true if the base API resized the window */
  bool EditorResize();
        
  /** Should be called when editor data changes*/
  void EditorDataModified();

  /** Override this method to serialize custom editor state data.
  * @param chunk The output bytechunk where data can be serialized
  * @return \c true if serialization was successful*/
  virtual bool SerializeCustomEditorData(IByteChunk& chunk) const { TRACE; return true; }
    
  /** Override this method to unserialize custom editor state data
  * @param chunk The incoming chunk containing the state data.
  * @param startPos The position in the chunk where the data starts
  * @return The new chunk position (endPos)*/
  virtual int UnserializeCustomEditorData(const IByteChunk& chunk, int startPos) { TRACE; return startPos; }
    
protected:
  std::function<IGraphics*()> mMakeGraphicsFunc = nullptr;
  std::function<void(IGraphics* pGraphics)> mLayoutFunc = nullptr;
private:
    
  int UpdateData(const IByteChunk& data, int startPos);

  std::unique_ptr<IGraphics> mGraphics;
  bool mIGraphicsTransient = false; // If creating IGraphics on demand this will be true
  bool mClosing = false; // used to prevent re-entrancy on closing
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
