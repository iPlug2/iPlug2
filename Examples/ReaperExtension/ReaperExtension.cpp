#include <cstdio>
#include "wdltypes.h"

#ifdef _WIN32
  #include <windows.h>
#else
  #include "swell.h"
#endif

#define REAPERAPI_IMPLEMENT
#include "reaper_plugin_functions.h"

#include "resource.h"

// super nasty looking macro here but allows importing functions from Reaper with simple looking code
#define IMPAPI(x) if (!((*((void **)&(x)) = (void *)pRec->GetFunc(#x)))) errorCount++;

int g_registered_command01 = 0;
int g_registered_command02 = 0;

// handle to the dll instance? could be useful for making win32 API calls
REAPER_PLUGIN_HINSTANCE g_hInst;

// global variable that holds the handle to the Reaper main window, useful for various win32 API calls
HWND g_parent;

char g_iniFileName[1024];

// Action default keyboard shortcut and action text set here
gaccel_register_t acreg01={{0,0,0}, "EXAMPLE: Action 1"};
gaccel_register_t acreg02={{0,0,0}, "EXAMPLE: Action 2"};

void doAction1()
{
  MessageBox(g_parent,"Action 1!","Reaper extension API test", MB_OK);
}

// global variable to hold the toggle state of the togglable action
// each togglable action of course needs it's own variable for this...

int g_togglestate; // will be inited in main() function to 0 or value from ini-file

void doAction2()
{
  // this action does nothing else but toggles the global variable that keeps track of the toggle state
  // so it's useless as such but you can see the action state changing in the toolbar buttons and the actions list
  if (g_togglestate == 0)
    g_togglestate = 1; // toggle state on
  else g_togglestate = 0;
  // store new state of toggle action to ini file immediately
  char buf[8];
  // the win32 api for ini-files only deals with strings, so form a string from the action
  // toggle state number.
  sprintf(buf,"%d",g_togglestate);
  WritePrivateProfileString("main","toggle_action_state",buf,g_iniFileName);
}

// Reaper calls back to this when it wants to execute an action registered by the extension plugin
bool hookCommandProc(int command, int flag)
{
  // it might happen Reaper calls with 0 for the command and if the action
  // registration has failed the plugin's command id would also be 0
  // therefore, check the plugins command id is not 0 and then if it matches with
  // what Reaper called with
  if (g_registered_command01!=0 && command == g_registered_command01)
  {
    doAction1();
    return true;
  }
  if (g_registered_command02!=0 && command == g_registered_command02)
  {
    doAction2();
    return true;
  }
  // failed to run relevant action
  return false;
}

// Reaper calls back to this when it wants to know an actions's toggle state
int toggleActionCallback(int command_id)
{
  if (command_id && command_id == g_registered_command02)
    return g_togglestate;
  // -1 if action not provided by this extension or is not togglable
  return -1;
}

extern "C"
{
  REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* pRec)
  {
    g_hInst = hInstance;
    
    if (pRec)
    {
      if (pRec->caller_version != REAPER_PLUGIN_VERSION || !pRec->GetFunc)
        return 0;
      // initialize API function pointers from Reaper
      // REMEMBER to initialize the needed functions here, otherwise calling them results in a crash!
      int errorCount = 0; // this error counter variable will be "magically" incremented by the IMPAPI macro on errors
      IMPAPI(Main_OnCommand);
      IMPAPI(GetResourcePath);
      IMPAPI(CreateLocalOscHandler);
      IMPAPI(DestroyLocalOscHandler);
      IMPAPI(SendLocalOscMessage);
      IMPAPI(AddExtensionsMainMenu);
      // if even one api function fails to initialize, it's not wise to continue, so abort plugin loading
      if (errorCount > 0)
        return 0;
      acreg01.accel.cmd = g_registered_command01 = pRec->Register("command_id",(void*)"EXAMPLE_ACTION_01");
      acreg02.accel.cmd = g_registered_command02 = pRec->Register("command_id",(void*)"EXAMPLE_ACTION_02");
      
      if (!g_registered_command01) return 0; // failed getting a command id, fail!
      if (!g_registered_command02) return 0; // failed getting a command id, fail!

      pRec->Register("gaccel", &acreg01);
      pRec->Register("gaccel", &acreg02);
      pRec->Register("hookcommand", (void*) hookCommandProc);
      pRec->Register("toggleaction", (void*) toggleActionCallback);
      
      AddExtensionsMainMenu();
      
      g_parent = pRec->hwnd_main;
      
      HMENU hMenu = GetSubMenu(GetMenu(g_parent),
#ifdef _WIN32
                               8
#else // OS X has one extra menu
                               9
#endif
                               );
      MENUITEMINFO mi={sizeof(MENUITEMINFO),};
      mi.fMask = MIIM_TYPE | MIIM_ID;
      mi.fType = MFT_STRING;
      mi.dwTypeData = LPSTR("EXAMPLE: Action 1");
      mi.wID = g_registered_command01;
      InsertMenuItem(hMenu, 6, TRUE, &mi);
      
      mi={sizeof(MENUITEMINFO),};
      mi.fMask = MIIM_TYPE | MIIM_ID;
      mi.fType = MFT_STRING;
      mi.dwTypeData = LPSTR("EXAMPLE: Action 2");
      mi.wID = g_registered_command02;
      InsertMenuItem(hMenu, 7, TRUE, &mi);

//      // form an ini file name for the extension to save/load settings
//      const char* inifilepath = GetResourcePath(); // reaper api for ini files location
//      sprintf(g_iniFileName,"%s/test_extension.ini",inifilepath); // form the actual file name
//      // restore extension global settings
//      // saving extension data into reaper project files is another thing and
//      // at the moment not done in this example plugin
//      char buf[8]; // it's just a number 0 or 1 so no big buffer needed for the string
//      GetPrivateProfileString("main", "toggle_action_state", "0", buf, 8, g_iniFileName);
//      g_togglestate = atoi(buf);

      // our plugin registered, return success
      return 1;
    }
    else
    {
      return 0;
    }
  }

};

#ifndef _WIN32
#define SWELL_DLG_FLAGS_AUTOGEN SWELL_DLG_WS_FLIPPED|SWELL_DLG_WS_RESIZABLE
#include "swell-dlggen.h"
#include "main.rc_mac_dlg"
#undef BEGIN
#undef END
#include "swell-menugen.h"
#include "main.rc_mac_menu"
#endif
