#pragma once
#include <cstring>
#include "wdltypes.h"
#include "wdlstring.h"

#include "IPlugPlatform.h"

#ifdef OS_MAC
#include "swell.h"
#endif

#include "resources/resource.h"

extern HWND gHWND;
extern HINSTANCE gHINSTANCE;
extern UINT gScrollMessage;
extern WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


