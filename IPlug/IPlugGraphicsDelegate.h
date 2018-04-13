#pragma once

#include "IPlugDelegate.h"

/**
 * @file
 * @copydoc IGraphicsDelegate
 */

class IGraphics;

/** An IDelgate base class for a SOMETHING that uses IGraphics for it's UI */
class IGraphicsDelegate : public IDelegate
{
public:
  IGraphicsDelegate(int nParams);
  ~IGraphicsDelegate();

// IDelegate
  void* OpenWindow(void* pHandle) override;
  void CloseWindow() override;
  void SetControlValueFromDelegate(int controlIdx, double normalizedValue) override;
  void SendParameterValueToUIFromDelegate(int paramIdx, double value, bool normalized) override;
  void ResizeGraphicsFromUI() override;
  /** If you override this method you should call this parent, or implement the same functionality in order to get controls to update, when state is restored. */
  virtual void OnRestoreState() override;
  
  //IGraphicsDelegate
  void AttachGraphics(IGraphics* pGraphics);
  virtual IGraphics* GetUI() { assert(mGraphics); return mGraphics; }
  
private:
  IGraphics* mGraphics = nullptr;
};
