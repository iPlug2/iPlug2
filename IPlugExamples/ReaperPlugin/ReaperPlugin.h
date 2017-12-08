#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include <wdltypes.h>

#ifdef OS_OSX
#include "swell.h"
#else
#include <windows.h>
#endif

class ReaperPlugin : public IPlug
{
public:
  ReaperPlugin(IPlugInstanceInfo instanceInfo);
  ~ReaperPlugin();

  bool GetHasUI() override { return true; }
  int GetUIWidth() override { return 400; }
  int GetUIHeight() override  { return 150; }
  
  void* OpenWindow(void* handle) override;
  void CloseWindow() override;

  void Reset() override;
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) override;

  HWND mHWND = nullptr;
};

