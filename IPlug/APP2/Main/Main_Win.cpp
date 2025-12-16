/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#ifdef OS_WIN

#include "IPlugAPP2_Host.h"
#include "config.h"

#include <windows.h>
#include <commctrl.h>

using namespace iplug;

HWND gHWND = nullptr;
HINSTANCE gHINSTANCE = nullptr;

static HANDLE gMutex = nullptr;

// Forward declarations
static bool InitInstance(HINSTANCE hInstance);
static void CleanupInstance();

/**
 * Windows application entry point
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  gHINSTANCE = hInstance;

  // Initialize common controls
  INITCOMMONCONTROLSEX icex;
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC = ICC_WIN95_CLASSES;
  InitCommonControlsEx(&icex);

  // Prevent multiple instances
  gMutex = CreateMutexA(NULL, TRUE, BUNDLE_NAME "_mutex");
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

  // Set DPI awareness (Windows 10+)
  #if defined(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
  #endif

  // Create host
  IPlugAPP2Host* pHost = IPlugAPP2Host::Create();
  if (!pHost)
  {
    MessageBoxA(NULL, "Failed to create application host", PLUG_NAME, MB_OK | MB_ICONERROR);
    CleanupInstance();
    return 1;
  }

  // Initialize
  if (!pHost->Init())
  {
    MessageBoxA(NULL, "Failed to initialize application", PLUG_NAME, MB_OK | MB_ICONERROR);
    CleanupInstance();
    return 1;
  }

  // TODO: Create main dialog window
  // For now, this is a skeleton that needs the dialog resource

  // Message loop
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
  {
    if (!IsDialogMessage(gHWND, &msg))
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  CleanupInstance();
  return static_cast<int>(msg.wParam);
}

static bool InitInstance(HINSTANCE hInstance)
{
  // TODO: Create main window
  return true;
}

static void CleanupInstance()
{
  if (gMutex)
  {
    ReleaseMutex(gMutex);
    CloseHandle(gMutex);
    gMutex = nullptr;
  }
}

#endif // OS_WIN
