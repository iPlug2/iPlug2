#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include <windows.h>

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class IPlugNoIGraphicsTest final : public Plugin
{
public:
  IPlugNoIGraphicsTest(const InstanceInfo& info);
  ~IPlugNoIGraphicsTest();

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;

  void SetScreenScale(float scale) override;

  void* OpenWindow(void* pParent);
  
  // Set the desired size for the parent window
  void SetParentWindowSize(int width, int height);
  
private:
  HWND m_hWnd; // Window handle
  HWND m_hParent; // Parent window handle
  float m_screenScale; // Screen scale factor
  int m_desiredWidth; // Desired width for parent window
  int m_desiredHeight; // Desired height for parent window
  HFONT m_hFont; // Font handle
  
  void UpdateWindowSize(); // Helper function to update window size based on scale
  void UpdateParentWindowSize(); // Helper function to update parent window size
  void UpdateFont(); // Helper function to update font based on scale
  
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};
