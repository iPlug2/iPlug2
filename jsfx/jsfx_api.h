/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * API used by REAPER
 */


#ifndef _JESUSONIC_DLL_H_
#define _JESUSONIC_DLL_H_

#ifndef _JS_SFXUI_H_
#define SX_Instance void
#endif

#define JSFX_API_VERSION_CURRENT 0xf0f01091

// to get the API struct, GetProcAddress("JesusonicAPI")

#ifndef _WIN32
#include "../WDL/swell/swell.h"
#endif

typedef struct
{
  unsigned int ver; // JSFX_API_VERSION_CURRENT

  SX_Instance *(*sx_createInstance)(const char *dir_root, const char *effect_name, bool *wantWak);
  int (*sx_processSamples)(SX_Instance *, double *buf, int cnt, int nch, int srate, double tempo, int tsnum, int tsdenom, double playstate, double playpos, double playpos_b, double lastwet, double wet, int flags); // flags&1=ignore pdc, 2=delta solo, 4=zero output before wet/dry
  HWND (*sx_createUI)(SX_Instance *, HINSTANCE hDllInstance, HWND hwndParent, void *hostpostparam);
  HWND (*sx_getUIwnd)(SX_Instance *);
  void (*sx_deleteUI)(SX_Instance *);
  void (*sx_destroyInstance)(SX_Instance *);
  const char *(*sx_getEffectName)(SX_Instance *);
  void (*sx_loadState)(SX_Instance *, const char *);
  const char * (*sx_saveState)(SX_Instance *, int *lOut);

  int (*sx_getNumParms)(SX_Instance *);      // these scale parms to 0..1
  double (*sx_getParmVal)(SX_Instance *, int parm, double *minval, double *maxval, double *step);
  void (*sx_getParmName)(SX_Instance *, int parm, char *name, int namelen);
  void (*sx_setParmVal)(SX_Instance *, int parm, double val, int sampleoffs);
  void (*sx_set_midi_ctx)(SX_Instance *, double (*midi_sendrecv)(void *ctx, int action, double *ts, double *msg1, double *msg23, double *midi_bus), void *midi_ctxdata);

  void (*setREAPERapi)(void *(*getFunc)(const char *name)); // called by loader at startup to provide API-getter function

  void (*sx_loadSerState)(SX_Instance *, const char *buf, int len);
  const char * (*sx_saveSerState)(SX_Instance *, int *len);

  const char * (*sx_getFullFN)(SX_Instance *);

  int (*sx_getCurrentLatency)(SX_Instance *);

  void (*sx_updateHostNch)(SX_Instance *, int nch);
  const char ** (*sx_getPinInfo)(SX_Instance *, int isOutput, int *numPins);

  bool (*sx_hasParmChanged)(SX_Instance *);

  void (*sx_getParmDisplay)(SX_Instance *, int parm, char* disptxt, int len, double* inval);

  int (*sx_parmIsEnum)(SX_Instance *, int parm);

  void (*sx_set_host_ctx)(SX_Instance *, void *ctx, void (*slider_automate)(void* ctx, int parmidx, bool done));

  bool (*sx_getCurPresetName)(SX_Instance *, char *name, int maxlen);
  bool (*sx_loadPreset)(SX_Instance *, const char *name);
  void (*sx_setCurPresetName)(SX_Instance *, const char *name);

  INT_PTR (*sx_inlineEditor)(SX_Instance *, int msg, void *, void *);

  double (*sx_getTailSize)(SX_Instance *); // ext_tail_size (or 0)
  bool (*sx_getParmValueFromString)(SX_Instance *, int parm, const char *buf, double *valptr);

#define JSFX_EXT_GETFLAGS 0 // returns &1 for is instrument
#define JSFX_EXT_GET_GR 1 // returns 1 if supported, sets *(double *)parm1 to value
#define JSFX_EXT_SET_SRATE 2 // parm1 = (void *)(INT_PTR)srate, sets for the next instantiation(s)
  INT_PTR (*sx_extended)(SX_Instance *, int msg, void *parm1, void *parm2);
  int pad[402]; //for future expansion

} jsfxAPI;


#endif
