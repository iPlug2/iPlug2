#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "swell.h"

class ReaperPlugin : public IPlug
{
public:
  ReaperPlugin(IPlugInstanceInfo instanceInfo);
  ~ReaperPlugin();

  bool GetHasUI() override { return true; }
  int GetUIWidth() override { return 100; }
  int GetUIHeight() override  { return 100; }
  
  void* OpenWindow(void* handle) override;
  void Reset() override;
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) override;

  HWND mHWND;

private:
  double mGain;
};

