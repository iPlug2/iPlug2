#pragma once

#include "IPlugBase.h"

class IGraphics;

class IPlugBaseGraphics : public IPlugBase
{
public:
  IPlugBaseGraphics(int nParams, const char* channelIOStr, int nPresets, const char* effectName, const char* productName, const char* mfrName, int vendorVersion, int uniqueID, int mfrID, int latency, bool plugDoesMidi, bool plugDoesChunks, bool plugIsInst, EAPI plugAPI);
  
  ~IPlugBaseGraphics();
  
  IGraphics* GetGUI() { return mGraphics; }
  int GetUIWidth() override;
  int GetUIHeight() override;

  void* OpenWindow(void* handle) override;
  void* OpenWindow(void* handle, void* control) override;
  void CloseWindow() override;
  
  void AttachGraphics(IGraphics* pGraphics);
  void RedrawParamControls() override;
  void SetParameterInUIFromAPI(int paramIdx, double value, bool normalized) override;
private:
  IGraphics* mGraphics = nullptr;
};
