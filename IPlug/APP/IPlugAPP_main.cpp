/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include <memory>
#include "wdltypes.h"
#include "wdlstring.h"

#include "IPlugPlatform.h"
#include "IPlugAPP_host.h"

#include "config.h"
#include "resource.h"

using namespace iplug;

#pragma mark - WINDOWS
#if defined OS_WIN
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

// Include stb_image_write for PNG saving
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../Dependencies/IGraphics/STB/stb_image_write.h"

extern WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWND gHWND;
extern HINSTANCE gHINSTANCE;
UINT gScrollMessage;

// Save a screenshot of the given HWND to a PNG file using Win32 API
bool SaveWindowScreenshot(HWND hwnd, const char* path)
{
  if (!hwnd || !path)
    return false;

  // Get client area dimensions
  RECT clientRect;
  if (!GetClientRect(hwnd, &clientRect))
    return false;

  int width = clientRect.right - clientRect.left;
  int height = clientRect.bottom - clientRect.top;

  if (width <= 0 || height <= 0)
    return false;

  // Create a compatible DC and bitmap
  HDC hdcWindow = GetDC(hwnd);
  if (!hdcWindow)
    return false;

  HDC hdcMem = CreateCompatibleDC(hdcWindow);
  if (!hdcMem)
  {
    ReleaseDC(hwnd, hdcWindow);
    return false;
  }

  // Create a 32-bit DIB section for the screenshot
  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height; // Top-down DIB
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;

  void* pBits = nullptr;
  HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);

  if (!hBitmap || !pBits)
  {
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWindow);
    return false;
  }

  HGDIOBJ hOldBitmap = SelectObject(hdcMem, hBitmap);

  // Use PrintWindow to capture the window content
  // PW_CLIENTONLY captures only the client area
  // PW_RENDERFULLCONTENT renders the window fully, including layered content
#ifndef PW_RENDERFULLCONTENT
  #define PW_RENDERFULLCONTENT 0x00000002
#endif
  BOOL captured = PrintWindow(hwnd, hdcMem, PW_CLIENTONLY | PW_RENDERFULLCONTENT);

  if (!captured)
  {
    // Fallback to BitBlt if PrintWindow fails
    captured = BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);
  }

  SelectObject(hdcMem, hOldBitmap);
  DeleteDC(hdcMem);
  ReleaseDC(hwnd, hdcWindow);

  if (!captured)
  {
    DeleteObject(hBitmap);
    return false;
  }

  // Convert BGRA to RGBA for stb_image_write
  uint8_t* pixels = static_cast<uint8_t*>(pBits);
  for (int i = 0; i < width * height; i++)
  {
    // Swap B and R channels (BGRA -> RGBA)
    uint8_t temp = pixels[i * 4 + 0];
    pixels[i * 4 + 0] = pixels[i * 4 + 2];
    pixels[i * 4 + 2] = temp;
    // Set alpha to 255 (opaque) since Windows DIB may have garbage in alpha
    pixels[i * 4 + 3] = 255;
  }

  // Use stb_image_write to save as PNG
  int result = stbi_write_png(path, width, height, 4, pixels, width * 4);

  DeleteObject(hBitmap);

  return result != 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nShowCmd)
{
  try
  {
#ifndef APP_ALLOW_MULTIPLE_INSTANCES
    HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, BUNDLE_NAME); // BUNDLE_NAME used because it won't have spaces in it
    
    if (!hMutex)
      hMutex = CreateMutex(0, 0, BUNDLE_NAME);
    else
    {
      HWND hWnd = FindWindow(0, BUNDLE_NAME);
      SetForegroundWindow(hWnd);
      return 0;
    }
#endif
    gHINSTANCE = hInstance;
    
    InitCommonControls();
    gScrollMessage = RegisterWindowMessage("MSWHEEL_ROLLMSG");

    IPlugAPPHost* pAppHost = IPlugAPPHost::Create();

    // Parse command line arguments
    if (lpszCmdParam && lpszCmdParam[0])
    {
      char* args = _strdup(lpszCmdParam);
      char* token = strtok(args, " ");
      while (token)
      {
        if (strcmp(token, "--screenshot") == 0)
        {
          token = strtok(nullptr, " ");
          if (token)
            pAppHost->SetScreenshotPath(token);
        }
        else if (strcmp(token, "--no-io") == 0)
        {
          pAppHost->SetNoIO(true);
        }
        token = strtok(nullptr, " ");
      }
      free(args);
    }

    // Screenshot mode implies --no-io
    if (pAppHost->IsScreenshotMode())
      pAppHost->SetNoIO(true);

    pAppHost->Init();
    pAppHost->TryToChangeAudio();

    HACCEL hAccel = LoadAccelerators(gHINSTANCE, MAKEINTRESOURCE(IDR_ACCELERATOR1));

    static UINT(WINAPI *__SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);

    if (!__SetProcessDpiAwarenessContext)
    {
      HINSTANCE h = LoadLibrary("user32.dll");
      if (h) *(void **)&__SetProcessDpiAwarenessContext = GetProcAddress(h, "SetProcessDpiAwarenessContext");
      if (!__SetProcessDpiAwarenessContext)
        *(void **)&__SetProcessDpiAwarenessContext = (void*)(INT_PTR)1;
    }
    if ((UINT_PTR)__SetProcessDpiAwarenessContext > (UINT_PTR)1)
    {
      __SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }

    CreateDialog(gHINSTANCE, MAKEINTRESOURCE(IDD_DIALOG_MAIN), GetDesktopWindow(), IPlugAPPHost::MainDlgProc);

#if !defined _DEBUG || defined NO_IGRAPHICS
    HMENU menu = GetMenu(gHWND);
    RemoveMenu(menu, 1, MF_BYPOSITION);
    DrawMenuBar(gHWND);
#endif

    for (;;)
    {
      MSG msg= {0,};
      int vvv = GetMessage(&msg, NULL, 0, 0);
      
      if (!vvv)
        break;
      
      if (vvv < 0)
      {
        Sleep(10);
        continue;
      }
      
      if (!msg.hwnd)
      {
        DispatchMessage(&msg);
        continue;
      }
      
      if (gHWND && (TranslateAccelerator(gHWND, hAccel, &msg) || IsDialogMessage(gHWND, &msg)))
        continue;
      
      // default processing for other dialogs
      HWND hWndParent = NULL;
      HWND temphwnd = msg.hwnd;
      
      do
      {
        if (GetClassLong(temphwnd, GCW_ATOM) == (INT)32770)
        {
          hWndParent = temphwnd;
          if (!(GetWindowLong(temphwnd, GWL_STYLE) & WS_CHILD))
            break; // not a child, exit
        }
      }
      while (temphwnd = GetParent(temphwnd));
      
      if (hWndParent && IsDialogMessage(hWndParent,&msg))
        continue;

      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
    
    // in case gHWND didnt get destroyed -- this corresponds to SWELLAPP_DESTROY roughly
    if (gHWND)
      DestroyWindow(gHWND);
    
#ifndef APP_ALLOW_MULTIPLE_INSTANCES
    ReleaseMutex(hMutex);
#endif
  }
  catch(std::exception e)
  {
    DBGMSG("Exception: %s", e.what());
    return 1;
  }
  return 0;
}
#pragma mark - MAC
#elif defined(OS_MAC)
#import <Cocoa/Cocoa.h>
#include <dlfcn.h>
#include <cstring>
#include "IPlugSWELL.h"
#include "IPlugPaths.h"

HWND gHWND;

// Function pointer type for CGWindowListCreateImage
typedef CGImageRef (*CGWindowListCreateImageFunc)(CGRect, uint32_t, uint32_t, uint32_t);

// Save a screenshot of the given HWND (NSView*) to a PNG file
extern "C" bool SaveWindowScreenshot(void* hwnd, const char* path)
{
  if (!hwnd || !path)
    return false;

  NSView* view = (__bridge NSView*)hwnd;
  NSWindow* window = [view window];

  if (!window)
    return false;

  // Get CGWindowListCreateImage via dlsym to bypass availability check
  // The function still exists and works in the runtime
  static CGWindowListCreateImageFunc pCGWindowListCreateImage = nullptr;
  if (!pCGWindowListCreateImage)
  {
    void* handle = dlopen("/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics", RTLD_LAZY);
    if (handle)
      pCGWindowListCreateImage = (CGWindowListCreateImageFunc)dlsym(handle, "CGWindowListCreateImage");
  }

  if (!pCGWindowListCreateImage)
    return false;

  // Get the window's CGWindowID
  CGWindowID windowID = (CGWindowID)[window windowNumber];

  // Capture the window content at full resolution (high DPI)
  CGImageRef cgImage = pCGWindowListCreateImage(
    CGRectNull,  // Capture the whole window
    kCGWindowListOptionIncludingWindow,
    windowID,
    kCGWindowImageBoundsIgnoreFraming  // Exclude window frame, capture at screen resolution
  );

  if (!cgImage)
    return false;

  // Create NSBitmapImageRep from CGImage and save as PNG
  NSBitmapImageRep* bitmap = [[NSBitmapImageRep alloc] initWithCGImage:cgImage];
  CGImageRelease(cgImage);

  if (!bitmap)
    return false;

  NSData* pngData = [bitmap representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
  if (!pngData)
    return false;

  NSString* filePath = [NSString stringWithUTF8String:path];
  return [pngData writeToFile:filePath atomically:YES];
}
extern HMENU SWELL_app_stocksysmenu;

static WDL_String gScreenshotPath;
static bool gNoIO = false;

int main(int argc, char *argv[])
{
#if APP_COPY_AUV3
  //if invoked with an argument registerauv3 use plug-in kit to explicitly register auv3 app extension (doesn't happen from debugger)
  if (argc > 2 && std::string_view(argv[2]) == "registerauv3")
  {
    WDL_String appexPath;
    appexPath.SetFormatted(1024, "pluginkit -a %s%s%s.appex", argv[0], "/../../Plugins/", appexPath.get_filepart());
    if (system(appexPath.Get()) > -1)
      NSLog(@"Registered audiounit app extension\n");
    else
      NSLog(@"Failed to register audiounit app extension\n");
  }
#endif

  // Parse command line arguments
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc)
    {
      gScreenshotPath.Set(argv[i + 1]);
      i++; // Skip the path argument
    }
    else if (strcmp(argv[i], "--no-io") == 0)
    {
      gNoIO = true;
    }
  }

  if (AppIsSandboxed())
    DBGMSG("App is sandboxed, file system access etc restricted!\n");

  return NSApplicationMain(argc, (const char**) argv);
}

INT_PTR SWELLAppMain(int msg, INT_PTR parm1, INT_PTR parm2)
{
  IPlugAPPHost* pAppHost = nullptr;
  
  switch (msg)
  {
    case SWELLAPP_ONLOAD:
    {
      pAppHost = IPlugAPPHost::Create();

      // Set CLI options
      if (gScreenshotPath.GetLength() > 0)
      {
        pAppHost->SetScreenshotPath(gScreenshotPath.Get());
        pAppHost->SetNoIO(true); // Implicit --no-io for screenshot mode
      }
      else if (gNoIO)
      {
        pAppHost->SetNoIO(true);
      }

      pAppHost->Init();
      pAppHost->TryToChangeAudio();
      break;
    }
    case SWELLAPP_LOADED:
    {
      pAppHost = IPlugAPPHost::sInstance.get();
      
      HMENU menu = SWELL_GetCurrentMenu();
      
      if (menu)
      {
        // work on a new menu
        menu = SWELL_DuplicateMenu(menu);
        HMENU src = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU1));

        for (int x = 0; x < GetMenuItemCount(src)-1; x++)
        {
          HMENU sm = GetSubMenu(src,x);
          
          if (sm)
          {
            char str[1024];
            MENUITEMINFO mii = {sizeof(mii), MIIM_TYPE};
            mii.dwTypeData = str;
            mii.cch = sizeof(str);
            str[0] = 0;
            GetMenuItemInfo(src, x, TRUE, &mii);
            MENUITEMINFO mi= {sizeof(mi), MIIM_STATE|MIIM_SUBMENU|MIIM_TYPE,MFT_STRING, 0, 0, SWELL_DuplicateMenu(sm), NULL, NULL, 0, str};
            InsertMenuItem(menu, x+1, TRUE, &mi);
          }
        }
      }
      
      if (menu)
      {
        HMENU sm = GetSubMenu(menu, 1);
        DeleteMenu(sm, ID_QUIT, MF_BYCOMMAND); // remove QUIT from our file menu, since it is in the system menu on OSX
        DeleteMenu(sm, ID_PREFERENCES, MF_BYCOMMAND); // remove PREFERENCES from the file menu, since it is in the system menu on OSX
        
        // remove any trailing separators
        int a = GetMenuItemCount(sm);
        
        while (a > 0 && GetMenuItemID(sm, a-1) == 0)
          DeleteMenu(sm, --a, MF_BYPOSITION);
        
        DeleteMenu(menu, 1, MF_BYPOSITION); // delete file menu
      }
      // Always set up screenshot shortcut
      SetMenuItemModifier(menu, ID_SCREENSHOT, MF_BYCOMMAND, 'S', FCONTROL | FSHIFT);

#if !defined _DEBUG || defined NO_IGRAPHICS
      if (menu)
      {
        HMENU sm = GetSubMenu(menu, 1);
        DeleteMenu(sm, ID_LIVE_EDIT, MF_BYCOMMAND);
        DeleteMenu(sm, ID_SHOW_BOUNDS, MF_BYCOMMAND);
        DeleteMenu(sm, ID_SHOW_DRAWN, MF_BYCOMMAND);
        DeleteMenu(sm, ID_SHOW_FPS, MF_BYCOMMAND);

        // remove any trailing separators
        int a = GetMenuItemCount(sm);

        while (a > 0 && GetMenuItemID(sm, a-1) == 0)
          DeleteMenu(sm, --a, MF_BYPOSITION);

        // Only delete debug menu if it's now empty (screenshot should remain)
        if (GetMenuItemCount(sm) == 0)
          DeleteMenu(menu, 1, MF_BYPOSITION);
      }
#else
      SetMenuItemModifier(menu, ID_LIVE_EDIT, MF_BYCOMMAND, 'E', FCONTROL);
      SetMenuItemModifier(menu, ID_SHOW_DRAWN, MF_BYCOMMAND, 'D', FCONTROL);
      SetMenuItemModifier(menu, ID_SHOW_BOUNDS, MF_BYCOMMAND, 'B', FCONTROL);
      SetMenuItemModifier(menu, ID_SHOW_FPS, MF_BYCOMMAND, 'F', FCONTROL);
#endif

      HWND hwnd = CreateDialog(gHINST, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, IPlugAPPHost::MainDlgProc);
      
      if (menu)
      {
        SetMenu(hwnd, menu); // set the menu for the dialog to our menu (on Windows that menu is set from the .rc, but on SWELL
        SWELL_SetDefaultModalWindowMenu(menu); // other windows will get the stock (bundle) menus
      }
      
      break;
    }
    case SWELLAPP_ONCOMMAND:
      // this is to catch commands coming from the system menu etc
      if (gHWND && (parm1&0xffff))
        SendMessage(gHWND, WM_COMMAND, parm1 & 0xffff, 0);
      break;
    case SWELLAPP_DESTROY:
      if (gHWND)
        DestroyWindow(gHWND);
      break;
    case SWELLAPP_PROCESSMESSAGE:
      MSG* pMSG = (MSG*) parm1;
      NSView* pContentView = (NSView*) pMSG->hwnd;
      NSEvent* pEvent = (NSEvent*) parm2;
      int etype = (int) [pEvent type];
      
      bool textField = [pContentView isKindOfClass:[NSText class]];
      
      if (!textField && etype == NSKeyDown)
      {
        int flag, code = SWELL_MacKeyToWindowsKey(pEvent, &flag);
        
        if (!(flag&~FVIRTKEY) && (code == VK_RETURN || code == VK_ESCAPE))
        {
          [pContentView keyDown: pEvent];
          return 1;
        }
      }
      break;
  }
  return 0;
}

#define CBS_HASSTRINGS 0
#define SWELL_DLG_SCALE_AUTOGEN 1
#define SET_IDD_DIALOG_PREF_SCALE 1.5
#if PLUG_HOST_RESIZE
#define SWELL_DLG_FLAGS_AUTOGEN SWELL_DLG_WS_FLIPPED|SWELL_DLG_WS_RESIZABLE
#endif
#include "swell-dlggen.h"
#include "resources/main.rc_mac_dlg"
#include "swell-menugen.h"
#include "resources/main.rc_mac_menu"

#pragma mark - LINUX
#elif defined(OS_LINUX)
//#include <IPlugSWELL.h>
//#include "swell-internal.h" // fixes problem with HWND forward decl
//
//HWND gHWND;
//UINT gScrollMessage;
//extern HMENU SWELL_app_stocksysmenu;
//
//int main(int argc, char **argv)
//{
//  SWELL_initargs(&argc, &argv);
//  SWELL_Internal_PostMessage_Init();
//  SWELL_ExtendedAPI("APPNAME", (void*) "IGraphics Test");
//
//  HMENU menu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU1));
//  CreateDialog(gHINSTANCE, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, MainDlgProc);
//  SetMenu(gHWND, menu);
//
//  while (!gHWND->m_hashaddestroy)
//  {
//    SWELL_RunMessageLoop();
//    Sleep(10);
//  };
//
//  if (gHWND)
//    DestroyWindow(gHWND);
//
//  return 0;
//}
//
//INT_PTR SWELLAppMain(int msg, INT_PTR parm1, INT_PTR parm2)
//{
//  switch (msg)
//  {
//    case SWELLAPP_ONLOAD:
//      break;
//    case SWELLAPP_LOADED:
//    {
//      HMENU menu = SWELL_GetCurrentMenu();
//
//      if (menu)
//      {
//        // work on a new menu
//        menu = SWELL_DuplicateMenu(menu);
//        HMENU src = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU1));
//
//        for (auto x = 0; x < GetMenuItemCount(src)-1; x++)
//        {
//          HMENU sm = GetSubMenu(src,x);
//          if (sm)
//          {
//            char str[1024];
//            MENUITEMINFO mii = {sizeof(mii), MIIM_TYPE};
//            mii.dwTypeData = str;
//            mii.cch = sizeof(str);
//            str[0] = 0;
//            GetMenuItemInfo(src, x, TRUE, &mii);
//            MENUITEMINFO mi= {sizeof(mi), MIIM_STATE|MIIM_SUBMENU|MIIM_TYPE,MFT_STRING, 0, 0, SWELL_DuplicateMenu(sm), NULL, NULL, 0, str};
//            InsertMenuItem(menu, x+1, TRUE, &mi);
//          }
//        }
//      }
//
//      if (menu)
//      {
//        HMENU sm = GetSubMenu(menu, 1);
//        DeleteMenu(sm, ID_QUIT, MF_BYCOMMAND); // remove QUIT from our file menu, since it is in the system menu on OSX
//        DeleteMenu(sm, ID_PREFERENCES, MF_BYCOMMAND); // remove PREFERENCES from the file menu, since it is in the system menu on OSX
//
//        // remove any trailing separators
//        int a = GetMenuItemCount(sm);
//
//        while (a > 0 && GetMenuItemID(sm, a-1) == 0)
//          DeleteMenu(sm, --a, MF_BYPOSITION);
//
//        DeleteMenu(menu, 1, MF_BYPOSITION); // delete file menu
//      }
//
//      // if we want to set any default modifiers for items in the menus, we can use:
//      // SetMenuItemModifier(menu,commandID,MF_BYCOMMAND,'A',FCONTROL) etc.
//
//      HWND hwnd = CreateDialog(gHINST,MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, MainDlgProc);
//
//      if (menu)
//      {
//        SetMenu(hwnd, menu); // set the menu for the dialog to our menu (on Windows that menu is set from the .rc, but on SWELL
//        SWELL_SetDefaultModalWindowMenu(menu); // other windows will get the stock (bundle) menus
//      }
//
//      break;
//    }
//    case SWELLAPP_ONCOMMAND:
//      // this is to catch commands coming from the system menu etc
//      if (gHWND && (parm1&0xffff))
//        SendMessage(gHWND, WM_COMMAND, parm1 & 0xffff, 0);
//      break;
//    case SWELLAPP_DESTROY:
//      if (gHWND)
//        DestroyWindow(gHWND);
//      break;
//    case SWELLAPP_PROCESSMESSAGE: // can hook keyboard input here
//      // parm1 = (MSG*), should we want it -- look in swell.h to see what the return values refer to
//      break;
//  }
//  return 0;
//}
//
//#define CBS_HASSTRINGS 0
//#define SWELL_DLG_SCALE_AUTOGEN 1
//#define SET_IDD_DIALOG_PREF_SCALE 1.5
//#include "swell-dlggen.h"
//#include "resources/main.rc_mac_dlg"
//#include "swell-menugen.h"
//#include "resources/main.rc_mac_menu"
#endif
