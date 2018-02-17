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

#include "IPlugParameter.h"
#include "IControls.h"

#include "resources/resource.h"

extern HWND gHWND;
extern HINSTANCE gHINSTANCE;

#define UI_WIDTH 700
#define UI_HEIGHT 700
#define FPS 60

const int gSizes[4] = { 100, 300, 600, 1000 };

class IGraphicsTest : public IDelegate
{
public:
  void init();
  
  void ResizeWindow(HWND hWnd, int w, int h, bool centerOnScreen);
  
  //IDelegate
  void SetControlValueFromDelegate(int controlIdx, double normalizedValue) override {};
  void SendParameterValueToUIFromDelegate(int paramIdx, double value, bool normalized) override {};
  
  const IParam* GetParamObjectFromUI(int paramIdx) override { return nullptr; }
  void SetParameterValueFromUI(int paramIdx, double value) override { DBGMSG("SetParameterValueFromUI p %i %f\n", paramIdx, value); }
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override { ; }
  void EndInformHostOfParamChangeFromUI(int paramIdx) override { ; }
  void ResizeGraphicsFromUI() override { ResizeWindow(gHWND, mGraphics->Width(), mGraphics->Height(), false ); }
  
private:
  IGraphics* mGraphics = nullptr;
  int mSizeIdx = 0;
};

extern WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
