#include "IGraphicsTest.h"

MyGraphicsTest gMyGraphics;

WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      gHWND = hwndDlg;
      
      gMyGraphics.init();
      gMyGraphics.ResizeWindow(gHWND, UI_WIDTH, UI_HEIGHT, true);
      ShowWindow(gHWND, SW_SHOW);
     
      return 1;
    }
    case WM_DESTROY:
      gHWND = NULL;
      PostQuitMessage(0);
      return 0;
    case WM_CLOSE:
      DestroyWindow(hwndDlg);
      return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case ID_QUIT:
          DestroyWindow(hwndDlg);
          return 0;
        case ID_ABOUT:
        {
          WDL_String version;
          version.SetFormatted(100, "Built on %s", __DATE__);
          MessageBox(hwndDlg, version.Get(), "IGraphicsTest", MB_OK);
          return 0;
        }
      }
      return 0;
  }
  return 0;
}
