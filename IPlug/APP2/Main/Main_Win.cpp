/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#ifdef OS_WIN

#include "../IPlugAPP2_Host.h"
#include "../IPlugAPP2.h"
#include "../Resources/resource.h"
#include "config.h"

#include <windows.h>
#include <commctrl.h>
#include <shellscalingapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shcore.lib")

using namespace iplug;

// Global handles
HWND gHWND = nullptr;
HINSTANCE gHINSTANCE = nullptr;

static HANDLE gMutex = nullptr;
static HACCEL gAccelTable = nullptr;

// Forward declarations
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void CenterWindow(HWND hwnd);
static float GetScaleForHWND(HWND hwnd);

/**
 * Windows application entry point
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  gHINSTANCE = hInstance;

  // Initialize common controls
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
  InitCommonControlsEx(&icex);

  // Prevent multiple instances
  gMutex = CreateMutexA(NULL, TRUE, BUNDLE_NAME "_APP2_mutex");
  if (GetLastError() == ERROR_ALREADY_EXISTS)
  {
    // Find existing window and bring to front
    HWND existingWnd = FindWindowA(NULL, PLUG_NAME);
    if (existingWnd)
    {
      SetForegroundWindow(existingWnd);
      if (IsIconic(existingWnd))
        ShowWindow(existingWnd, SW_RESTORE);
    }
    return 0;
  }

  // Set DPI awareness (Windows 10 1703+)
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  // Create host
  IPlugAPP2Host* pHost = IPlugAPP2Host::Create();
  if (!pHost)
  {
    MessageBoxA(NULL, "Failed to create application host", PLUG_NAME, MB_OK | MB_ICONERROR);
    ReleaseMutex(gMutex);
    CloseHandle(gMutex);
    return 1;
  }

  // Initialize host (creates plugin, loads settings)
  if (!pHost->Init())
  {
    MessageBoxA(NULL, "Failed to initialize application", PLUG_NAME, MB_OK | MB_ICONERROR);
    ReleaseMutex(gMutex);
    CloseHandle(gMutex);
    return 1;
  }

  // Register window class
  WNDCLASSEXA wc = {};
  wc.cbSize = sizeof(WNDCLASSEXA);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = MainWndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wc.lpszMenuName = MAKEINTRESOURCEA(IDR_MENU1);
  wc.lpszClassName = PLUG_NAME "_WndClass";
  wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

  if (!RegisterClassExA(&wc))
  {
    MessageBoxA(NULL, "Failed to register window class", PLUG_NAME, MB_OK | MB_ICONERROR);
    ReleaseMutex(gMutex);
    CloseHandle(gMutex);
    return 1;
  }

  // Get plugin dimensions
  IPlugAPP2* pPlug = pHost->GetPlug();
  int width = pPlug ? pPlug->GetEditorWidth() : 600;
  int height = pPlug ? pPlug->GetEditorHeight() : 400;

  // Create main window
  gHWND = CreateWindowExA(
    0,
    PLUG_NAME "_WndClass",
    PLUG_NAME,
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    width, height,
    NULL,
    NULL,
    hInstance,
    pHost  // Pass host to WM_CREATE
  );

  if (!gHWND)
  {
    MessageBoxA(NULL, "Failed to create main window", PLUG_NAME, MB_OK | MB_ICONERROR);
    ReleaseMutex(gMutex);
    CloseHandle(gMutex);
    return 1;
  }

  // Load accelerator table
  gAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

  // Show window
  ShowWindow(gHWND, nCmdShow);
  UpdateWindow(gHWND);

  // Message loop
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
  {
    if (!TranslateAccelerator(gHWND, gAccelTable, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  // Cleanup
  if (gMutex)
  {
    ReleaseMutex(gMutex);
    CloseHandle(gMutex);
    gMutex = nullptr;
  }

  return static_cast<int>(msg.wParam);
}

/**
 * Main window procedure
 */
static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  IPlugAPP2Host* pHost = nullptr;

  if (msg == WM_NCCREATE)
  {
    CREATESTRUCTA* pCreate = reinterpret_cast<CREATESTRUCTA*>(lParam);
    pHost = static_cast<IPlugAPP2Host*>(pCreate->lpCreateParams);
    SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pHost));
  }
  else
  {
    pHost = reinterpret_cast<IPlugAPP2Host*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
  }

  switch (msg)
  {
    case WM_CREATE:
    {
      gHWND = hwnd;

      if (pHost)
      {
        // Attach plugin UI
        pHost->OpenWindow(hwnd);

        // Resize window to match plugin
        IPlugAPP2* pPlug = pHost->GetPlug();
        if (pPlug)
        {
          int width = pPlug->GetEditorWidth();
          int height = pPlug->GetEditorHeight();

          // Adjust for window chrome
          RECT rcClient = { 0, 0, width, height };
          AdjustWindowRect(&rcClient, GetWindowLong(hwnd, GWL_STYLE), TRUE);

          int windowWidth = rcClient.right - rcClient.left;
          int windowHeight = rcClient.bottom - rcClient.top;

          SetWindowPos(hwnd, NULL, 0, 0, windowWidth, windowHeight,
                       SWP_NOMOVE | SWP_NOZORDER);
          CenterWindow(hwnd);
        }
      }
      return 0;
    }

    case WM_DESTROY:
    {
      if (pHost)
      {
        pHost->StopAudio();
        pHost->CloseWindow();
      }
      gHWND = nullptr;
      PostQuitMessage(0);
      return 0;
    }

    case WM_CLOSE:
    {
      // Check if plugin allows quit
      if (pHost && pHost->GetPlug())
      {
        if (!pHost->GetPlug()->OnHostRequestingQuit())
          return 0;  // Plugin blocked quit
      }
      DestroyWindow(hwnd);
      return 0;
    }

    case WM_COMMAND:
    {
      int wmId = LOWORD(wParam);

      switch (wmId)
      {
        case ID_PREFERENCES:
          if (pHost)
            pHost->ShowSettingsDialog(hwnd);
          return 0;

        case ID_QUIT:
          PostMessage(hwnd, WM_CLOSE, 0, 0);
          return 0;

        case ID_ABOUT:
          if (pHost && pHost->GetPlug())
          {
            if (!pHost->GetPlug()->OnHostRequestingAboutBox())
            {
              // Default about box
              char info[256];
              snprintf(info, sizeof(info), "%s\nBuilt on %s",
                       PLUG_COPYRIGHT_STR, __DATE__);
              MessageBoxA(hwnd, info, PLUG_NAME, MB_OK);
            }
          }
          return 0;

        case ID_HELP:
          if (pHost && pHost->GetPlug())
          {
            if (!pHost->GetPlug()->OnHostRequestingHelp())
            {
              MessageBoxA(hwnd, "See the manual", PLUG_NAME, MB_OK);
            }
          }
          return 0;

#if defined(_DEBUG) && !defined(NO_IGRAPHICS)
        case ID_LIVE_EDIT:
        case ID_SHOW_BOUNDS:
        case ID_SHOW_DRAWN:
        case ID_SHOW_FPS:
          // TODO: Forward to IGraphics debug menu handlers
          return 0;
#endif
      }
      break;
    }

    case WM_SIZE:
    {
      if (pHost && pHost->GetPlug())
      {
        if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED)
        {
          RECT rc;
          GetClientRect(hwnd, &rc);
          // TODO: Notify plugin of resize if it supports host resize
        }
      }
      return 0;
    }

    case WM_GETMINMAXINFO:
    {
      if (pHost && pHost->GetPlug())
      {
        IPlugAPP2* pPlug = pHost->GetPlug();
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);

        RECT rc = { 0, 0, pPlug->GetMinWidth(), pPlug->GetMinHeight() };
        AdjustWindowRect(&rc, GetWindowLong(hwnd, GWL_STYLE), TRUE);
        mmi->ptMinTrackSize.x = rc.right - rc.left;
        mmi->ptMinTrackSize.y = rc.bottom - rc.top;

        rc = { 0, 0, pPlug->GetMaxWidth(), pPlug->GetMaxHeight() };
        AdjustWindowRect(&rc, GetWindowLong(hwnd, GWL_STYLE), TRUE);
        mmi->ptMaxTrackSize.x = rc.right - rc.left;
        mmi->ptMaxTrackSize.y = rc.bottom - rc.top;
      }
      return 0;
    }

    case WM_DPICHANGED:
    {
      // Handle DPI change
      float scale = GetScaleForHWND(hwnd);
      RECT* pRect = reinterpret_cast<RECT*>(lParam);

      SetWindowPos(hwnd, NULL,
                   pRect->left, pRect->top,
                   pRect->right - pRect->left,
                   pRect->bottom - pRect->top,
                   SWP_NOZORDER | SWP_NOACTIVATE);

      // TODO: Notify IGraphics of scale change
      return 0;
    }
  }

  return DefWindowProcA(hwnd, msg, wParam, lParam);
}

/**
 * Center window on screen
 */
static void CenterWindow(HWND hwnd)
{
  RECT rc;
  GetWindowRect(hwnd, &rc);

  int width = rc.right - rc.left;
  int height = rc.bottom - rc.top;

  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int screenHeight = GetSystemMetrics(SM_CYSCREEN);

  int x = (screenWidth - width) / 2;
  int y = (screenHeight - height) / 2;

  SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

/**
 * Get DPI scale factor for window
 */
static float GetScaleForHWND(HWND hwnd)
{
  UINT dpi = GetDpiForWindow(hwnd);
  return static_cast<float>(dpi) / 96.0f;
}

// Export for use by other modules
float GetScaleForHWND(HWND hwnd);

#endif // OS_WIN
