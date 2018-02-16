#include <windows.h>
#include <commctrl.h>
#include "IGraphicsTest.h"

HWND gHWND;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nShowCmd)
{
  try
  {
    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, "IGraphicsTest");

    if (!hMutex)
      hMutex = CreateMutex(0, 0, "IGraphicsTest");
    else
    {
      HWND hWnd = FindWindow(0, "IGraphicsTest");
      SetForegroundWindow(hWnd);
      return 0; // should return 1?
    }

    gHINSTANCE = hInstance;

    InitCommonControls();
    gScrollMessage = RegisterWindowMessage("MSWHEEL_ROLLMSG");
    CreateDialog(gHINSTANCE, MAKEINTRESOURCE(IDD_DIALOG_MAIN), GetDesktopWindow(), MainDlgProc);
    
    for(;;)
    {
      MSG msg= {0,};
      int vvv = GetMessage(&msg, NULL, 0, 0);

      if (!vvv)
        break;

      if (vvv<0)
      {
        Sleep(10);
        continue;
      }
      
      if (!msg.hwnd)
      {
        DispatchMessage(&msg);
        continue;
      }

      if (gHWND && IsDialogMessage(gHWND, &msg)) continue;

      // default processing for other dialogs
      HWND hWndParent = NULL;
      HWND temphwnd = msg.hwnd;
      
      do
      {
        if (GetClassLong(temphwnd, GCW_ATOM) == (INT)32770)
        {
          hWndParent=temphwnd;
          if (!(GetWindowLong(temphwnd, GWL_STYLE) &WS_CHILD))
            break; // not a child, exit
        }
      }
      while (temphwnd = GetParent(temphwnd));

      if (hWndParent && IsDialogMessage(hWndParent,&msg)) continue;

      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // in case gHWND didnt get destroyed -- this corresponds to SWELLAPP_DESTROY roughly
    if (gHWND)
      DestroyWindow(gHWND);

    ReleaseMutex(hMutex);
  }
  catch(...)
  {
    DBGMSG("another instance running\n");
  }
  return 0;
}

