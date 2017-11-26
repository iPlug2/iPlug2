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

  bool OpenWindow(void* handle) override;
  void CloseWindow() override;
  
  void AttachGraphics(IGraphics* pGraphics);
  void RedrawParamControls() override;
  
private:
  IGraphics* mGraphics = nullptr;
};
