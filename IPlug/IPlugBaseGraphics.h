#pragma once

#include "IPlugBase.h"

/**
 * @file
 * @copydoc IPlugBaseGraphics
 */

class IGraphics;

/** An IPlugBase class for an IPlug plug-in that uses IGraphics for it's UI */
class IPlugBaseGraphics : public IPlugBase
{
public:
  IPlugBaseGraphics(IPlugConfig config, EAPI plugAPI);
  
  ~IPlugBaseGraphics();
  
  IGraphics* GetGUI() { assert(mGraphics); return mGraphics; }
  void* GetAAXViewInterface() override { return (void*) GetGUI(); }

  int GetUIWidth() override;
  int GetUIHeight() override;

  void* OpenWindow(void* handle) override;
  void CloseWindow() override;
  
  void AttachGraphics(IGraphics* pGraphics);
  void RedrawParamControls() override;
  void SetParameterInUIFromAPI(int paramIdx, double value, bool normalized) override;
  
  void PrintDebugInfo() override;
private:
  IGraphics* mGraphics = nullptr;
};
