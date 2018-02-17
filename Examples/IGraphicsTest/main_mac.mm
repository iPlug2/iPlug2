#include "IGraphicsTest.h"

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
