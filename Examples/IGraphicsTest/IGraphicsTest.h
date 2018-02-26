#pragma once
#include "IPlugPlatform.h"

#include "wdltypes.h"
#include "wdlstring.h"

#if defined (OS_WIN)
#include "IGraphicsWin.h"
#elif defined (OS_MAC)
#include "IGraphicsMac.h" 
#include "swell.h"
#define PostQuitMessage SWELL_PostQuitMessage
#elif defined (OS_LINUX)
#include "IGraphicsLinux.h"
#include "swell.h"
#define PostQuitMessage
#endif

#include "IPlugDelegate.h"

#include "resources/resource.h"

extern HWND gHWND;
extern HINSTANCE gHINSTANCE;

#define UI_WIDTH 700
#define UI_HEIGHT 700
#define UI_FPS 60

const int gSizes[4] = { 100, 300, 600, 1000 };

class IGraphics;

class IGraphicsTest : public IDelegate
{
public:
  void init();
  
  void ResizeWindow(HWND hWnd, int w, int h, bool centerOnScreen);
  
  //IDelegate
  void SetControlValueFromDelegate(int controlIdx, double normalizedValue) override {};
  void SendParameterValueToUIFromDelegate(int paramIdx, double value, bool normalized) override {};
  
  const IParam* GetParamObjectFromUI(int paramIdx) override { return nullptr; }
  void SetParameterValueFromUI(int paramIdx, double value) override;
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override { ; }
  void EndInformHostOfParamChangeFromUI(int paramIdx) override { ; }
  void ResizeGraphicsFromUI() override;
  
private:
  IGraphics* mGraphics = nullptr;
  int mSizeIdx = 0;
};

extern WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
