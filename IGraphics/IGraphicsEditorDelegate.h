#pragma once

#include "IPlugEditorDelegate.h"

/**
 * @file
 * @copydoc IGEditorDelegate
 */

class IGraphics;
class IControl;

/** An IDelgate base class for a SOMETHING that uses IGraphics for it's UI */
class IGEditorDelegate : public IEditorDelegate
{
public:
  IGEditorDelegate(int nParams);
  ~IGEditorDelegate();

// IEditorDelegate
  void* OpenWindow(void* pHandle) override;
  void CloseWindow() override;
  virtual void SendControlValueFromDelegate(int controlTag, double normalizedValue) override;
  virtual void SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize = 0, const void* pData = nullptr) override;
  virtual void SendMidiMsgFromDelegate(const IMidiMsg& msg) override;
  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
  /** If you override this method you should call this parent, or implement the same functionality in order to get controls to update, when state is restored. */
  virtual void OnRestoreState() override;
  virtual void OnUIOpen() override;

  //IGEditorDelegate
  virtual IGraphics* CreateGraphics() = 0;
  virtual void LayoutUI(IGraphics* pGraphics) = 0;
  IGraphics* GetUI();
  
  void ForControlWithParam(int paramIdx, std::function<void(IControl& control)> func);
  void ForControlInGroup(const char* group, std::function<void(IControl& control)> func);

private:
  IGraphics* mGraphics = nullptr;
};
