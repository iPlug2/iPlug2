#include <cstdlib>
#include <cstring>
#include "wdltypes.h"
#include "wdlstring.h"
#include "swell.h"

#include "resources/resource.h"
#include "curses.h"

extern WDL_DLGRET mainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern LRESULT CALLBACK cursesWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern HWND curses_ControlCreator(HWND parent, const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h);

HWND gHWND;

int main(int argc, char *argv[])
{
  return NSApplicationMain(argc, (const char **) argv);
}

INT_PTR SWELLAppMain(int msg, INT_PTR parm1, INT_PTR parm2)
{
  switch (msg)
  {
    case SWELLAPP_LOADED:
    {
      SWELL_RegisterCustomControlCreator(curses_ControlCreator);
      HWND hwnd = CreateDialog(NULL, MAKEINTRESOURCE(IDD_DIALOG_MAIN), 0, mainDlgProc);
      ShowWindow(hwnd, SW_SHOW);
      break;
    }
    case SWELLAPP_DESTROY:
      if (gHWND)
        DestroyWindow(gHWND);
      break;
    case SWELLAPP_PROCESSMESSAGE:
      // TOOD: keyboard to curses?
      break;
  }
  return 0;
}

#define CBS_HASSTRINGS 0
#define SWELL_DLG_SCALE_AUTOGEN 1
#define SET_IDD_DIALOG_PREF_SCALE 1.5
#include "swell-dlggen.h"
SWELL_DEFINE_DIALOG_RESOURCE_BEGIN(IDD_DIALOG_MAIN,SWELL_DLG_WS_RESIZABLE|SWELL_DLG_WS_FLIPPED,"Curses Test",400,300,1.8)
BEGIN
CONTROL         "", IDC_RECT, "WDLCursesWindow", 0, 0, 0, 400, 300
END
SWELL_DEFINE_DIALOG_RESOURCE_END(IDD_DIALOG_MAIN)

