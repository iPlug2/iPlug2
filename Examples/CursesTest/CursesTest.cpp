#include <cstdlib>
#include <cstring>
#include "wdltypes.h"
#include "wdlstring.h"
#include "swell.h"
#define PostQuitMessage SWELL_PostQuitMessage

#include "resources/resource.h"
#include "curses.h"
#include "curses_editor.h"

extern HWND gHWND;
win32CursesCtx g_curses_context;
WDL_CursesEditor g_editor(&g_curses_context);

WDL_DLGRET mainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      gHWND = hwndDlg;
      curses_setWindowContext(GetDlgItem(gHWND, IDC_RECT), &g_curses_context);
      
      g_editor.init("/Users/oli/Dev/MyFaustProjects/Projects/Tambura/Tambura.dsp");
      g_editor.draw();
      ShowWindow(gHWND, SW_SHOW);
      SetTimer(hwndDlg, 1, 1, NULL);
      break;
    }
    case WM_DESTROY:
      gHWND = NULL;
      PostQuitMessage(0);
      break;
    case WM_CLOSE:
      DestroyWindow(hwndDlg);
      break;
    case WM_TIMER:
    {
      g_editor.RunEditor();
      InvalidateRect(GetDlgItem(hwndDlg, IDC_RECT), NULL, FALSE);

      break;
    }
  }
  return 0;
}

