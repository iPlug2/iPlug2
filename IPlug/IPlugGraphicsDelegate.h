#pragma once

#include "IPlugEditorDelegate.h"

/**
 * @file
 * @copydoc IGraphicsEditorDelegate
 */

class IGraphics;

/** An IDelgate base class for a SOMETHING that uses IGraphics for it's UI */
class IGraphicsEditorDelegate : public IEditorDelegate
{
public:
  IGraphicsEditorDelegate(int nParams);
  ~IGraphicsEditorDelegate();

// IEditorDelegate
  void* OpenWindow(void* pHandle) override;
  void CloseWindow() override;
  virtual void SetControlValueFromDelegate(int controlTag, double normalizedValue) override;
  virtual void SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, const void* pData) override;
  
  virtual void OnMidiMsgUI(const IMidiMsg& msg) override;

  void SendParameterValueToUIFromDelegate(int paramIdx, double value, bool normalized) override;
  void ResizeGraphicsFromUI() override;
  /** If you override this method you should call this parent, or implement the same functionality in order to get controls to update, when state is restored. */
  virtual void OnRestoreState() override;
  
  //IGraphicsEditorDelegate
  virtual void AttachGraphics(IGraphics* pGraphics);
  virtual IGraphics* GetUI() { assert(mGraphics); return mGraphics; }
  
private:
  IGraphics* mGraphics = nullptr;
};
