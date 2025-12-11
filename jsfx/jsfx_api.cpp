/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * API entrypoint/necessary inclusion to be loaded by REAPER
 */

#include "sfxui.h"

#include "jsfx_api.h"

#include "resource.h"

const char *g_config_slider_classname = "REAPERhfader";

HINSTANCE g_hInst;

extern "C" {

#ifdef _WIN32
__declspec(dllexport) jsfxAPI JesusonicAPI=
#else
__attribute__((visibility("default"))) jsfxAPI JesusonicAPI=
#endif
{
  JSFX_API_VERSION_CURRENT,

  sx_createInstance,
  sx_processSamples,
  sx_createUI,
  sx_getUIwnd,
  sx_deleteUI,
  sx_destroyInstance,
  sx_getEffectName,
  sx_loadState,
  sx_saveState,

  sx_getNumParms,
  sx_getParmVal,
  sx_getParmName,
  sx_setParmVal,
  sx_set_midi_ctx,

  sx_provideAPIFunctionGetter,

  sx_loadSerState,
  sx_saveSerState,

  sx_getFullFN,
  sx_getCurrentLatency,

  sx_updateHostNch,
  sx_getPinInfo,
  sx_hasParmChanged,

  sx_getParmDisplay,
  sx_parmIsEnum,

  sx_set_host_ctx,

  sx_getCurPresetName,
  sx_loadPreset,
  sx_setCurPresetName,
  sx_inlineEditor,
  sx_getTailSize,
  sx_getParmValueFromString,

  sx_extended,
};


BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID res)
{
  if (fdwReason==DLL_PROCESS_ATTACH)
  {
    g_hInst=hDllInst;
  }
  return TRUE;
}

};

extern int g_last_srate;

INT_PTR sx_extended(SX_Instance *sx, int msg, void *parm1, void *parm2)
{
  if (msg == JSFX_EXT_SET_SRATE)
    g_last_srate = (int) (INT_PTR)parm1;
  if (sx) switch (msg)
  {
    case JSFX_EXT_GETFLAGS: return sx->m_misc_flags;
    case JSFX_EXT_GET_GR:
      if (!sx->m_ext_gr_meter || *sx->m_ext_gr_meter > 9999.0)
        return 0;
      if (parm1) *(double *)parm1 = *sx->m_ext_gr_meter;
    return 1;
  }
  return 0;
}


#ifndef _WIN32 // MAC resources
#define TVS_HASBUTTONS 0
#define TBS_AUTOTICKS 0
#define SS_CENTERIMAGE 0
#include "../WDL/swell/swell-dlggen.h"
#include "res.rc_mac_dlg"
#undef BEGIN
#undef END
#include "../WDL/swell/swell-menugen.h"
#include "res.rc_mac_menu"
#endif
