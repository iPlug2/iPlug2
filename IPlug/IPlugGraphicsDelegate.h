#pragma once

#include "IPlugDelegate.h"

/**
 * @file
 * @copydoc IPlugBaseGraphics
 */

class IGraphics;

/** An IPlug delgate base class for an IPlug plug-in that uses IGraphics for it's UI */
class IGraphicsDelegate : public IDelegate
{
public:
  IGraphicsDelegate(int nParams);
  ~IGraphicsDelegate();

//  //IPlugBase
//  void* GetAAXViewInterface() override { return (void*) GetUI(); }
//  void* OpenWindow(void* pHandle) override;
//  void CloseWindow() override;
//  void SendParameterValueToUIFromAPI(int paramIdx, double value, bool normalized) override;
//  void PrintDebugInfo() const override;

  /** If you override this method you should call this parent, or implement the same functionality in order to get controls to update, when state is restored. */
  virtual void OnRestoreState() override;

  //IDelegate
  void SetControlValueFromDelegate(int controlIdx, double normalizedValue) override;
  void SendParameterValueToUIFromDelegate(int paramIdx, double value, bool normalized) override;
  void ResizeGraphicsFromUI() override;

  //IGraphicsDelegate
  void AttachGraphics(IGraphics* pGraphics);
  IGraphics* GetUI() { assert(mGraphics); return mGraphics; }
private:
  IGraphics* mGraphics = nullptr;
};
