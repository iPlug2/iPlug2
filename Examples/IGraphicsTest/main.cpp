#include "IPlugPlatform.h"
#include "IGraphicsTest.h"

#pragma mark - WINDOWS
#if defined(OS_WIN)
#include <windows.h>
#include <commctrl.h>
HWND gHWND;

HINSTANCE gHINSTANCE;
UINT gScrollMessage;

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
#pragma mark - MAC
#elif defined(OS_MAC)
#include "swell.h"
HWND gHWND;
extern HMENU SWELL_app_stocksysmenu;

int main(int argc, char *argv[])
{
  return NSApplicationMain(argc,  (const char **) argv);
}

INT_PTR SWELLAppMain(int msg, INT_PTR parm1, INT_PTR parm2)
{
  switch (msg)
  {
    case SWELLAPP_ONLOAD:
      break;
    case SWELLAPP_LOADED:
    {
      HMENU menu = SWELL_GetCurrentMenu();

      if (menu)
      {
        // work on a new menu
        menu = SWELL_DuplicateMenu(menu);
        HMENU src = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU1));

        for (auto x = 0; x < GetMenuItemCount(src)-1; x++)
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

      // if we want to set any default modifiers for items in the menus, we can use:
      // SetMenuItemModifier(menu,commandID,MF_BYCOMMAND,'A',FCONTROL) etc.

      HWND hwnd = CreateDialog(gHINST, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, MainDlgProc);

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
    case SWELLAPP_PROCESSMESSAGE: // can hook keyboard input here
      // parm1 = (MSG*), should we want it -- look in swell.h to see what the return values refer to
      break;
  }
  return 0;
}

#define CBS_HASSTRINGS 0
#define SWELL_DLG_SCALE_AUTOGEN 1
#define SET_IDD_DIALOG_PREF_SCALE 1.5
#include "swell-dlggen.h"
#include "resources/IGraphicsTest.rc_mac_dlg"
#include "swell-menugen.h"
#include "resources/IGraphicsTest.rc_mac_menu"

#pragma mark - LINUX
#elif defined(OS_LINUX)
#include "swell.h"
#include "swell-internal.h" // fixes problem with HWND forward decl

HWND gHWND;
UINT gScrollMessage;
extern HMENU SWELL_app_stocksysmenu;

int main(int argc, char **argv)
{
  SWELL_initargs(&argc, &argv);
  SWELL_Internal_PostMessage_Init();
  SWELL_ExtendedAPI("APPNAME", (void*) "IGraphics Test");
  
  HMENU menu = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU1));
  CreateDialog(gHINSTANCE, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, MainDlgProc);
  SetMenu(gHWND, menu);
  
  while (!gHWND->m_hashaddestroy)
  {
    SWELL_RunMessageLoop();
    Sleep(10);
  };
  
  if (gHWND)
    DestroyWindow(gHWND);
  
  return 0;
}

INT_PTR SWELLAppMain(int msg, INT_PTR parm1, INT_PTR parm2)
{
  switch (msg)
  {
    case SWELLAPP_ONLOAD:
      break;
    case SWELLAPP_LOADED:
    {
      HMENU menu = SWELL_GetCurrentMenu();
      
      if (menu)
      {
        // work on a new menu
        menu = SWELL_DuplicateMenu(menu);
        HMENU src = LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU1));
        
        for (auto x = 0; x < GetMenuItemCount(src)-1; x++)
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
      
      // if we want to set any default modifiers for items in the menus, we can use:
      // SetMenuItemModifier(menu,commandID,MF_BYCOMMAND,'A',FCONTROL) etc.
      
      HWND hwnd = CreateDialog(gHINST,MAKEINTRESOURCE(IDD_DIALOG_MAIN),NULL,MainDlgProc);
      
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
    case SWELLAPP_PROCESSMESSAGE: // can hook keyboard input here
      // parm1 = (MSG*), should we want it -- look in swell.h to see what the return values refer to
      break;
  }
  return 0;
}

#define CBS_HASSTRINGS 0
#define SWELL_DLG_SCALE_AUTOGEN 1
#define SET_IDD_DIALOG_PREF_SCALE 1.5
#include "swell-dlggen.h"
#include "resources/IGraphicsTest.rc_mac_dlg"
#include "swell-menugen.h"
#include "resources/IGraphicsTest.rc_mac_menu"
#endif
