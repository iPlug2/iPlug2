#include "swell.h"
#include "resource.h"
#include "main_dialog_proc.hpp"
#include "render_dialog_proc.hpp"

extern int g_swell_osx_style;

//static HWND CustomControlCreator(HWND parent, const char* cname, int idx, const char* classname, int style, int x, int y, int w, int h)
//{
//  if (!stricmp(classname,"TestRenderingClass"))
//  {
//    HWND hw = CreateDialog(NULL, 0, parent, (DLGPROC) RenderDlgProc);
//    SetWindowLong(hw, GWL_ID, idx);
//    SetWindowPos(hw, HWND_TOP, x, y, w, h, SWP_NOZORDER|SWP_NOACTIVATE);
//    ShowWindow(hw, SW_SHOWNA);
//    return hw;
//  }
//  return 0;
//}

extern HMENU SWELL_app_stocksysmenu;

// main app message loop for SWELL emulation (macOS/linux)
INT_PTR SWELLAppMain(int msg, INT_PTR parm1, INT_PTR parm2)
{
  switch (msg)
  {
    case SWELLAPP_ONLOAD:
      //SWELL_RegisterCustomControlCreator(CustomControlCreator);
      break;
    case SWELLAPP_LOADED:
    {
      // g_swell_osx_style = 1;
      if (SWELL_app_stocksysmenu)
      {
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
          // remove QUIT from our file menu, since it is in the system menu on macOS
          DeleteMenu(sm, ID_QUIT, MF_BYCOMMAND);
          // remove PREFERENCES from the file menu, since it is in the system menu on OSX
          DeleteMenu(sm, ID_PREFERENCES, MF_BYCOMMAND);
          // remove any trailing separators
          int a = GetMenuItemCount(sm);
          
          while (a > 0 && GetMenuItemID(sm, a-1) == 0) {
            DeleteMenu(sm, --a, MF_BYPOSITION);
          }
          // delete file menu
          DeleteMenu(menu, 1, MF_BYPOSITION);
        }
        
        HWND hwnd = CreateDialog(gHINST, MAKEINTRESOURCE(IDD_DIALOG_MAIN), NULL, MainDlgProc);
        
        if (menu)
        {
          SetMenu(hwnd, menu); // set the menu for the dialog to our menu (on Windows that menu is set from the .rc, but on SWELL
          SWELL_SetDefaultModalWindowMenu(menu); // other windows will get the stock (bundle) menus
        }
      }
      break;
    }
    case SWELLAPP_ONCOMMAND:
      break;
    case SWELLAPP_DESTROY:
      break;
    case SWELLAPP_PROCESSMESSAGE:
      break;
  }
  return 0;
}

#define CBS_HASSTRINGS 0
#define SWELL_DLG_SCALE_AUTOGEN 1
#define SET_IDD_DIALOG_PREF_SCALE 1
#if PLUG_HOST_RESIZE
#define SWELL_DLG_FLAGS_AUTOGEN SWELL_DLG_WS_FLIPPED|SWELL_DLG_WS_RESIZABLE
#endif
#include "swell-dlggen.h"
#include "resources/main.rc_mac_dlg"
#include "swell-menugen.h"
#include "resources/main.rc_mac_menu"
