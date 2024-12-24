#include "main_dialog_proc.hpp"
#include "resource.h"
#include "wdlstring.h"
#include "render_dialog_proc.hpp"
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkColorSpace.h"

#ifndef _WIN32
#define PostQuitMessage SWELL_PostQuitMessage
#endif

HWND gHWND;
//extern HMENU SWELL_app_stocksysmenu;

HDC memDC;
sk_sp<SkSurface> surface;

sk_sp<SkSurface> CreateSurface(HDC drawContext, int w, int h)
{
  SkImageInfo info = SkImageInfo::Make(w, h, kBGRA_8888_SkColorType, kPremul_SkAlphaType, nullptr);
  void* pixels = SWELL_GetCtxFrameBuffer(drawContext);
  sk_sp<SkSurface> surface = SkSurfaces::WrapPixels(info, pixels, sizeof(uint32_t) * w);
  return surface;
}

// Dialog Procedure for the main app window (To understand: why not a Window Proc?)
WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      gHWND = hwndDlg;
      RECT r;
      GetClientRect(hwndDlg, &r);
      auto w = r.right - r.left;
      auto h = r.bottom - r.top;
      SWELL_EnableMetal(gHWND, 1);
      memDC = SWELL_CreateMemContext(nullptr, w, h);
      surface = CreateSurface(memDC, w, h);
      ShowWindow(hwndDlg, SW_SHOW);
      return 0;
    }
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC dc = BeginPaint(hwndDlg, &ps);
      RECT r;
      GetClientRect(hwndDlg, &r);
      
      auto w = r.right - r.left;
      auto h = r.bottom - r.top;

      SkCanvas* canvas = surface->getCanvas();
      canvas->clear(SK_ColorCYAN);
      SkPaint paint;
      paint.setColor(SK_ColorBLUE);
      paint.setAntiAlias(true);
      canvas->drawCircle(100, 100, 50, paint);
      
      BitBlt(dc, 0, 0, w, h, memDC, 0, 0, 0);
      
      EndPaint(hwndDlg, &ps);
      return 0;
    }
    case WM_SIZE:
    {
      RECT r;
      GetClientRect(hwndDlg, &r);
      InvalidateRect(hwndDlg, &r, 0);
      
      auto w = r.right - r.left;
      auto h = r.bottom - r.top;
      
      SWELL_DeleteGfxContext(memDC);
      memDC = SWELL_CreateMemContext(nullptr, w, h);
      surface = CreateSurface(memDC, w, h);
      return 0;;
    }
    case WM_DESTROY:
      gHWND = NULL;
      PostQuitMessage(hwndDlg); // TODO: check arg isn't a problem on win32
      return 0;
    case WM_CLOSE:
      DestroyWindow(hwndDlg);
      return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDOK:
        {
          HWND hwnd = CreateDialog(gHINSTANCE, 0, nullptr, (DLGPROC) RenderDlgProc);
          // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos
          SetWindowPos(hwnd, HWND_TOP, 500, 500, 500, 500, 0 /* flags */);
          SetWindowLong(hwnd, GWL_STYLE, WS_CAPTION|WS_THICKFRAME /* flags */);
          SetWindowText(hwnd, "Hello");
          ShowWindow(hwnd, SW_SHOW);
          return 0;
        }
        case ID_QUIT:
        {
          DestroyWindow(hwndDlg);
          return 0;
        }
        case ID_ABOUT:
        {
          WDL_String info;
          info.Append("Built on " __DATE__);
          MessageBox(hwndDlg, info.Get(), "IPlugAppExperiment", MB_OK);
          return 0;
        }
        case ID_HELP:
        {
          MessageBox(hwndDlg, "Help!", "IPlugAppExperiment", MB_OK);
          return 0;
        }
        case ID_PREFERENCES:
        {
//          INT_PTR ret = DialogBox(gHINSTANCE, MAKEINTRESOURCE(IDD_DIALOG_PREF), hwndDlg, IPlugAPPHost::PreferencesDlgProc);
          return 0;
        }
      }
      return 0;
    }
  return 0;
}
