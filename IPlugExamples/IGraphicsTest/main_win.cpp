#include <windows.h>
#include <commctrl.h>
#include "IGraphicsTest.h"

HWND gHWND;
HINSTANCE gHINSTANCE;
UINT gScrollMessage;

extern WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nShowCmd)
{
  // first check to make sure this is the only instance running
  // http://www.bcbjournal.org/articles/vol3/9911/Single-instance_applications.htm
  try
  {
    // Try to open the mutex.
    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, "IGraphicsTest");

    // If hMutex is 0 then the mutex doesn't exist.
    if (!hMutex)
      hMutex = CreateMutex(0, 0, "IGraphicsTest");
    else
    {
      // This is a second instance. Bring the
      // original instance to the top.
      HWND hWnd = FindWindow(0, "IGraphicsTest");
      SetForegroundWindow(hWnd);

      return 0;
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
    if (gHWND) DestroyWindow(gHWND);

    ReleaseMutex(hMutex);
  }
  catch(...)
  {
    //TODO proper error catching
    DBGMSG("another instance running");
  }
  return 0;
}

