#pragma once
#include <cstring>
#include "wdltypes.h"
#include "wdlstring.h"

#include "IPlugPlatform.h"
#include "IGraphicsDelegate.h"

#ifdef OS_MAC
#include "swell.h"
#endif

#include "resources/resource.h"

extern HWND gHWND;
extern HINSTANCE gHINSTANCE;
extern UINT gScrollMessage;
extern WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

class MyDelegate: public IGraphicsDelegate
{
public:
  //IGraphicsDelegate
  IGraphics* GetUI() override { return nullptr; }
  IParam* GetParamFromUI(int paramIdx) override { return nullptr; }
  void SetParameterValueFromUI(int paramIdx, double value) override { DBGMSG("HELLO IGRAPHICS PARAMETER %i!\n", paramIdx); }
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override { ; }
  void EndInformHostOfParamChangeFromUI(int paramIdx) override { ; }
};



