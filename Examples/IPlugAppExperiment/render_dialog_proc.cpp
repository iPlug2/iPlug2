#include "render_dialog_proc.hpp"
#include "resource.h"
#include "swell.h"

LRESULT WINAPI RenderDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      /*HDC dc = */BeginPaint(hwndDlg, &ps);
      RECT r;
      GetClientRect(hwndDlg, &r);
      HBRUSH brs = CreateSolidBrush(RGB(0, 0, 255));
      FillRect(ps.hdc, &r, brs);
      DeleteObject(brs);
      EndPaint(hwndDlg, &ps);
      break;
    }
    case WM_LBUTTONDOWN:
      ShowCursor(TRUE);
      break;
    case WM_LBUTTONUP:
      ShowCursor(TRUE);
      break;
    case WM_CLOSE:
      DestroyWindow(hwndDlg);
      break;
    default:
      break;
  }

  return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}
