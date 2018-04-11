#pragma once

#include "IPlugDelegate_select.h"

/**
 * @file
 * @copydoc IPlugBaseGraphics
 */

class IGraphics;

/** An IPlug delgate base class for an IPlug plug-in that uses IGraphics for it's UI */
class IGraphicsDelegate : public IGRAPHICS_DELEGATE
{
public:
  IGraphicsDelegate(int nParams, int nPresets);
  IGraphicsDelegate(IDelegate& mainDelegate);
//  IGraphicsDelegate(picojson::value json);

  ~IGraphicsDelegate();

// IDelegate
//  void* GetAAXViewInterface() override { return (void*) GetUI(); }
  void* OpenWindow(void* pHandle) override;
  void CloseWindow() override;
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
