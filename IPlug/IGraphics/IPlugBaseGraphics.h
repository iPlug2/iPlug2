#pragma once

#include "IPlugBase.h"

class IGraphics;

class IPlugBaseGraphics : public IPlugBase
{
public:
  IPlugBaseGraphics(IPlugConfig config, EAPI plugAPI);
  
  ~IPlugBaseGraphics();
  
  IGraphics* GetGUI() { return mGraphics; }
  void* GetAAXViewInterface() override { return (void*) GetGUI(); }

  int GetUIWidth() override;
  int GetUIHeight() override;

  void* OpenWindow(void* handle) override;
  void CloseWindow() override;
  
  void AttachGraphics(IGraphics* pGraphics);
  void RedrawParamControls() override;
  void SetParameterInUIFromAPI(int paramIdx, double value, bool normalized) override;
private:
  IGraphics* mGraphics = nullptr;
};
