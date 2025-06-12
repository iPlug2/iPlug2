/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * effect UI related code
 */

#include "sfxui.h"
#include "jsfx_api.h"

#include "../WDL/win32_utf8.h"

#include <math.h>

#ifdef _WIN32
#include <windowsx.h>
#include <commctrl.h>
#include <float.h>
#include "../WDL/swell/swell.h"
#include "../WDL/wingui/dlgitemborder.h"
#else
#include "../WDL/swell/swell-dlggen.h"
#endif


#ifdef DYNAMIC_LICE
#define LOCALIZE_IMPORT_PREFIX "jsfx_"
#include "../WDL/localize/localize-import.h"
#endif

#include "../WDL/localize/localize.h"

#include "resource.h"

#include "sfx_edit.h"

#include "../WDL/fft.h"
#include "../WDL/db2val.h"

#include "../jmde/reaper_plugin_fx_embed.h"

#define EEL_LICE_STANDALONE_NOINITQUIT
#define EEL_LICE_WANT_STANDALONE
#define EEL_LICE_API_ONLY
#include "../WDL/eel2/eel_lice.h"

#if WDL_FFT_REALSIZE == 8 && defined(DYNAMIC_LICE)

#define _IMPORTED_FFT_

void WDL_fft_init()
{
}

static int (*__WDL_fft_permute)(int, int);
static int *(*__WDL_fft_permute_tab)(int);
static void (*WDL_fft_complexmul_8)(WDL_FFT_COMPLEX *dest, WDL_FFT_COMPLEX *src, int len);
static void (*WDL_fft_8)(WDL_FFT_COMPLEX *, int len, int isInverse);
static void (*WDL_fft_real_8)(WDL_FFT_REAL *, int len, int isInverse);

void WDL_fft_complexmul(WDL_FFT_COMPLEX *dest, WDL_FFT_COMPLEX *src, int len)
{
  if (WDL_fft_complexmul_8) WDL_fft_complexmul_8(dest,src,len);
}
void WDL_fft(WDL_FFT_COMPLEX *d, int len, int isInverse)
{
  if (WDL_fft_8) WDL_fft_8(d,len,isInverse);
}
void WDL_real_fft(WDL_FFT_REAL *d, int len, int isInverse)
{
  if (WDL_fft_real_8) WDL_fft_real_8(d,len,isInverse);
}
int WDL_fft_permute(int fftsize, int idx)
{
  if (__WDL_fft_permute) return __WDL_fft_permute(fftsize,idx);
  return 0;
}
int *WDL_fft_permute_tab(int fftsize)
{
  if (__WDL_fft_permute_tab) return __WDL_fft_permute_tab(fftsize);
  return 0;
}
#endif


/// imported APIs
void (*FreeHeapPtr)(void*);
bool (*fxPresetPromptForAction)(HWND hwndDlg, void *parentid, int action, char *buf, int bufsz); // action=0 save, 1=save def, 2=rename
bool *(*DoFxLastTweakParmCtxMenu)(void* fxdsp, HWND h, int, int, const char*);
void *(*fxSetBypass)(void*, bool);
void *(*fxSetWet)(void*, double val, bool done);
bool (*fxGetSetDeltaSolo)(void *ctx, bool *delta_solo);
bool (*fxLoadReaperPreset)(void*, const char*);
int (*fxGetReaperPresetNamesRaw)(void*, char***);
int (*fxGetPlacement)(void *fxdsp, int *chain_pos, int *flags); // returns track index, -1 for master, -2 for hwout. flags will have 1 set if takeFX, 2 if record input, 4 if in inactive project
void (*fxRenameReaperPreset)(void*, const char*, const char*);
int (*fxDoReaperPresetAction)(void*, const char*, int);
bool (*fxImportExportRPL)(void*, HWND, bool);
bool (*fxGetSetWantAllKeys)(void*, bool*);
const char * (*get_ini_file)();
HWND (*GetMainHwnd)();
void (*fxSetUndoPoint)(void*);

int (*fxGetSetHostNumChan)(void *fxdsp, int *set_numchan);
int (*fxGetSetPinMap2)(void *fxdsp, bool isout, unsigned int *mapping, int ch_offs, int *isset_sz);
int (*fxGetSetPinmapperFlags)(void *fxdsp, int *set_flags);

const char *(*get_eel_funcdesc)(const char *);

PCM_source *(*_PCM_Source_CreateFromFile)(const char *filename);
bool (*IsMediaExtension)(const char *extparm, bool wantOthers);
bool (*Plugin_FindNewFileName)(ReaProject *__proj, int trackid, const char *trackname, const char *ext, char *buf, int bufsz);
PCM_sink *(*PCM_Sink_Create)(const char *filename, const void *cfg, int cfg_l, int nch, int srate, bool buildpeaks);
int (*InsertMedia)(const char* file, int mode);
void (*SetEditCurPos)(double time, bool moveview, bool seekplay);
double (*GetProjectLength)(ReaProject *__proj);
bool (*SetTempoTimeSigMarker)(ReaProject* __proj, int ptidx, double tpos, int mpos, double bpos, double bpm, int tsnum, int tsdenom, bool lineartempo);
double (*GetCursorPositionEx)(ReaProject*);
int (*FindTempoTimeSigMarker)(ReaProject* __proj, double tpos);
bool (*GetTempoTimeSigMarker)(ReaProject* __proj, int ptidx, double* tpos, int* mpos, double* bpos, double* bpm, int* tsnum, int* tsdenom, bool* lineartempo);
void (*CSurf_OnTempoChange)(double);
double (*Master_GetTempo)();
void (*EnsureNotCompletelyOffscreen)(RECT *);
int (*GetWindowDPIScalingForDialog)(HWND);
void (*SetWindowAccessibilityString)(HWND h, const char *, int mode);
#ifndef _WIN32
void (*Mac_MakeDefaultWindowMenu)(HWND);
#endif
const char * (*GetAppVersion)(void);

void (*_NSEEL_HOSTSTUB_EnterMutex)();
void (*_NSEEL_HOSTSTUB_LeaveMutex)();
void ** (*eel_gmem_attach)(const char *nm, bool is_alloc);
int (*plugin_register)(const char *, void *);


/// end of imported APIs
/// imported config

int *g_config_fontsize_ptr, *g_config_maxsug_ptr, *g_config_editflag_ptr;

// end of imported config


#define GFX_ID 0xff0f
#define SLIDER_ID_BASE 2048
#define SLIDER_ID_WIDTH 4
#define SLIDER_LABEL_ID(x) (SLIDER_ID_BASE + (x)*SLIDER_ID_WIDTH) // will always have label
#define SLIDER_SLIDER_ID(x) (SLIDER_ID_BASE + (x)*SLIDER_ID_WIDTH + 1)
#define SLIDER_EDIT_ID(x) (SLIDER_ID_BASE + (x)*SLIDER_ID_WIDTH + 2)
#define SLIDER_COMBO_ID(x) (SLIDER_ID_BASE + (x)*SLIDER_ID_WIDTH + 3) // will only have slider+edit or combo, not both

static WDL_PtrList<struct HWND__> s_configWindows;

static int acProc(MSG *msg, accelerator_register_t *ctx)
{
  if (msg->hwnd)
  {
    int x;
    if (msg->wParam == VK_RETURN && (msg->message == WM_KEYDOWN || msg->message == WM_KEYUP))
    {
      const int n = s_configWindows.GetSize();
      for (x = 0; x < n; x ++)
      {
        HWND h=s_configWindows.Get(x);
        if (IsChild(h,msg->hwnd))
        {
          HWND useh = msg->hwnd;
          int idx=GetWindowLong(useh,GWL_ID);

#ifndef _WIN32
          if (idx <= 0)
          {
            useh = GetFocus();
            if (useh) idx = GetWindowLong(useh,GWL_ID);
          }
#endif
          if (idx>=SLIDER_ID_BASE && idx < SLIDER_ID_BASE+SLIDER_ID_WIDTH*NUM_SLIDERS &&
              ((idx-SLIDER_ID_BASE)%SLIDER_ID_WIDTH) == 2 &&
              useh && GetParent(useh) == h)
          {
            SendMessage(h,WM_COMMAND,idx | (EN_KILLFOCUS << 16),0);
            return 1;
          }
          return 0;
        }
      }
    }
  }
  return 0;
}

static accelerator_register_t accelRec =
{
  acProc,
  true,
};

void sx_provideAPIFunctionGetter(void *(*getFunc)(const char *name))
{
  if (WDL_NOT_NORMALLY(!getFunc)) return;

  void *(*get_config_var)(const char *name, int *szout);
  *(void **)&get_config_var = getFunc("get_config_var");
  if (get_config_var)
  {
    int sz=0;
    void *p = get_config_var("edit_fontsize",&sz);
    if (p && sz==sizeof(int)) g_config_fontsize_ptr = (int *)p;
    p = get_config_var("edit_sug",&sz);
    if (p && sz==sizeof(int)) g_config_maxsug_ptr = (int *)p;

    p = get_config_var("edit_flags",&sz);
    if (p && sz==sizeof(int)) g_config_editflag_ptr = (int *)p;

    p = get_config_var("ide_colors",&sz);
    if (p && sz>=32*sizeof(int))
    {
      extern int *curses_win32_global_user_colortab;
      curses_win32_global_user_colortab = (int *)p;
    }
    p = get_config_var("ide_font_face",&sz);
    if (p && sz>0)
    {
      extern const char *curses_win32_global_font_face_name;
      curses_win32_global_font_face_name = (const char *)p;
    }
  }
  *(void**)&FreeHeapPtr = getFunc("FreeHeapPtr");
  *(void**)&fxPresetPromptForAction = getFunc("fxPresetPromptForAction");
  *(void**)&DoFxLastTweakParmCtxMenu = getFunc("DoFxLastTweakParmCtxMenu2");
  *(void**)&fxSetBypass = getFunc("fxSetBypass");
  *(void**)&fxSetWet = getFunc("fxSetWet");
  *(void**)&fxGetSetDeltaSolo = getFunc("fxGetSetDeltaSolo");
  *(void**)&fxLoadReaperPreset = getFunc("fxLoadReaperPreset");
  *(void**)&fxGetReaperPresetNamesRaw = getFunc("fxGetReaperPresetNamesRaw");
  *(void**)&fxGetPlacement = getFunc("fxGetPlacement");
  *(void**)&fxRenameReaperPreset = getFunc("fxRenameReaperPreset");
  *(void**)&fxDoReaperPresetAction = getFunc("fxDoReaperPresetAction");
  *(void**)&fxImportExportRPL = getFunc("fxImportExportRPL");
  *(void**)&fxGetSetWantAllKeys = getFunc("fxGetSetWantAllKeys");
  *(void **)&get_ini_file = getFunc("get_ini_file");
  *(void **)&GetMainHwnd = getFunc("GetMainHwnd");
  *(void **)&_PCM_Source_CreateFromFile = getFunc("PCM_Source_CreateFromFile");
  *(void **)&IsMediaExtension = getFunc("IsMediaExtension");
  *(void**)&fxSetUndoPoint=getFunc("fxSetUndoPoint");

  *(void**)&fxGetSetHostNumChan=getFunc("fxGetSetHostNumChan");
  *(void**)&fxGetSetPinMap2=getFunc("fxGetSetPinMap2");
  *(void**)&fxGetSetPinmapperFlags=getFunc("fxGetSetPinmapperFlags");

  *(void**)&GetAppVersion = getFunc("GetAppVersion");
  *(void**)&InsertMedia = getFunc("InsertMedia");
  *(void**)&Plugin_FindNewFileName = getFunc("Plugin_FindNewFileName");
  *(void**)&PCM_Sink_Create = getFunc("PCM_Sink_Create");
  *(void**)&SetEditCurPos = getFunc("SetEditCurPos");
  *(void**)&GetProjectLength = getFunc("GetProjectLength");
  *(void**)&SetTempoTimeSigMarker = getFunc("SetTempoTimeSigMarker");
  *(void**)&GetCursorPositionEx = getFunc("GetCursorPositionEx");
  *(void**)&FindTempoTimeSigMarker = getFunc("FindTempoTimeSigMarker");
  *(void**)&GetTempoTimeSigMarker = getFunc("GetTempoTimeSigMarker");
  *(void**)&CSurf_OnTempoChange = getFunc("CSurf_OnTempoChange");
  *(void**)&Master_GetTempo=getFunc("Master_GetTempo");
  *(void **)&EnsureNotCompletelyOffscreen = getFunc("EnsureNotCompletelyOffscreen");

  *(void **)&_NSEEL_HOSTSTUB_LeaveMutex = getFunc("NSEEL_HOSTSTUB_LeaveMutex");
  *(void **)&_NSEEL_HOSTSTUB_EnterMutex = getFunc("NSEEL_HOSTSTUB_EnterMutex");
  *(void **)&nseel_gmem_calloc = getFunc("calloc");
  *(void **)&eel_gmem_attach = getFunc("eel_gmem_attach");
  if (!_NSEEL_HOSTSTUB_LeaveMutex || !_NSEEL_HOSTSTUB_EnterMutex || !nseel_gmem_calloc || !eel_gmem_attach)
  {
    _NSEEL_HOSTSTUB_LeaveMutex=NULL;
    _NSEEL_HOSTSTUB_EnterMutex=NULL;
    eel_gmem_attach=NULL;
    nseel_gmem_calloc=NULL;
  }

  *(void **)&get_eel_funcdesc = getFunc("get_eel_funcdesc");

  *(void **)&GetWindowDPIScalingForDialog = getFunc("GetWindowDPIScalingForDialog");

#ifdef LOCALIZE_IMPORT_PREFIX
  *(void **)&importedLocalizeFunc = getFunc("__localizeFunc");
  *(void **)&importedLocalizeMenu = getFunc("__localizeMenu");
  *(void **)&importedLocalizeInitializeDialog = getFunc("__localizeInitializeDialog");
  *(void **)&importedLocalizePrepareDialog = getFunc("__localizePrepareDialog");
#endif

#ifdef _IMPORTED_FFT_
  *(void **)&__WDL_fft_permute = getFunc("WDL_fft_permute");
  *(void **)&__WDL_fft_permute_tab = getFunc("WDL_fft_permute_tab");
  *(void **)&WDL_fft_8=getFunc("WDL_fft_8");
  *(void **)&WDL_fft_real_8=getFunc("WDL_fft_real_8");
  *(void **)&WDL_fft_complexmul_8=getFunc("WDL_fft_complexmul_8");
#endif

#ifdef DYNAMIC_LICE
  eel_lice_initfuncs(getFunc);
#endif

  *(void **)&SetWindowAccessibilityString = getFunc("SetWindowAccessibilityString");
#ifndef _WIN32
  *(void **)&Mac_MakeDefaultWindowMenu = getFunc("Mac_MakeDefaultWindowMenu");
  SWELL_RegisterCustomControlCreator((SWELL_ControlCreatorProc)getFunc("Mac_CustomControlCreator"));
  extern HWND curses_ControlCreator(HWND parent, const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h);
  SWELL_RegisterCustomControlCreator(curses_ControlCreator);
#endif

  *(void **)&plugin_register = getFunc("plugin_register");
  if (plugin_register)
  {
    plugin_register("accelerator",&accelRec);
  }
}

bool SX_Instance::getCurPresetName(char* name, int maxlen)
{
  lstrcpyn_safe(name, m_curpresetname.Get(), maxlen);
  return true;
}

bool SX_Instance::loadPreset(const char* name)
{
  if (!fxLoadReaperPreset) return false;

  ++m_preset_reent;

  bool found=fxLoadReaperPreset(m_hostctx, name);
  if (found) m_curpresetname.Set(name?name:"");

  --m_preset_reent;

  if (m_hwnd) PostMessage(m_hwnd, WM_USER+4400, 0, 0);
  return found;
}

void SX_Instance::DoImageLoads(bool fullReset)
{
  if (fullReset)
  {
    int x;
    for (x=0;x<m_lice_state->m_gfx_images.GetSize();x++)
    {
      m_lice_state->m_gfx_images.Get()[x].clear();
    }
    m_lice_state->m_gfx_font_active=-1;
    for (x=0;x<m_lice_state->m_gfx_fonts.GetSize();x++)
    {
      if (LICE_FUNCTION_VALID(LICE__DestroyFont)) LICE__DestroyFont(m_lice_state->m_gfx_fonts.Get()[x].font);
      memset(&m_lice_state->m_gfx_fonts.Get()[x],0,sizeof(m_lice_state->m_gfx_fonts.Get()[x]));
    }
  }
  if (LICE_FUNCTION_VALID(LICE_LoadImage))
  {
    WDL_FastString tmp;
    int fullfnlen=0;

    if (m_full_actual_fn_used.GetLength())
    {
      const char *ptr=m_full_actual_fn_used.Get()+m_full_actual_fn_used.GetLength();
      while (ptr>=m_full_actual_fn_used.Get() && *ptr != '\\' && *ptr != '/') ptr--;
      fullfnlen = ptr-m_full_actual_fn_used.Get()+1;
    }

    int x;
    for (x = 0; x < m_localfilenames.GetSize(); x ++)
    {
      const char *fn=m_localfilenames.Get(x);
      if (fn && strlen(fn)>4 && !stricmp(fn+strlen(fn)-4,".png"))
      {
        if (x >= m_lice_state->m_gfx_images.GetSize())
        {
          // allow extra filename: lines up to 4096 to match old JSFX behavior
          if (x > 4096) continue;

          const int oldsz=m_lice_state->m_gfx_images.GetSize();
          eel_lice_state::img_state *c = m_lice_state->m_gfx_images.Resize(x+1);

          const int newsz=m_lice_state->m_gfx_images.GetSize();
          if (newsz>oldsz && c) memset(c+oldsz,0,sizeof(eel_lice_state::img_state) * (newsz-oldsz));

          if (x >= newsz) continue;
        }

        m_lice_state->m_gfx_images.Get()[x].clear();

        bool ok = false;
        if (fullfnlen>0)
        {
          // try relative to effect
          tmp.Set(m_full_actual_fn_used.Get(),fullfnlen);
          tmp.Append(fn);
          ok = m_lice_state->do_load_image(x, tmp.Get());
        }

        if (!ok && m_datadir.GetLength())
        {
          // try from data dir
          tmp.Set(m_datadir.Get());
          tmp.Append("/");
          tmp.Append(fn);
          ok = m_lice_state->do_load_image(x, tmp.Get());
        }
      }
    }
  }
    // add all filenames to the list
//      m_full_actual_fn_used
}

void SX_Instance::gfx_notify_general_statechange()
{
  if (GetCurrentThreadId()==m_main_thread)
  {
    if (fxSetUndoPoint) fxSetUndoPoint(m_hostctx);
  }
}

double effectSlider::_sc_inv(double v) const
{
  switch (ui_is_shape)
  {
    case SHAPE_LOG_PLAIN: return exp(v) - ui_shape_parm;
    case SHAPE_POW: return v < 0.0 ? -pow(-v,ui_shape_parm) : pow(v,ui_shape_parm);
    case SHAPE_LOG_MIDPOINT: return range_min + (exp(ui_shape_parm*v) - 1.0) / ui_shape_parm2;
  }
  return v;
}
double effectSlider::_sc(double v) const
{
  switch (ui_is_shape)
  {
    case SHAPE_LOG_PLAIN: return log(wdl_max(v + ui_shape_parm,1e-100));
    case SHAPE_POW: return v < 0.0 ? -pow(-v,1.0/ui_shape_parm) : pow(v,1.0/ui_shape_parm);
    case SHAPE_LOG_MIDPOINT:
      v = (v-range_min) * ui_shape_parm2 + 1.0;
      return log(wdl_max(v,1e-100))/ui_shape_parm;
  }
  return v;
}

double effectSlider::scaleFromUI(int pos) const
{
  if (ui_is_shape != SHAPE_LINEAR)
  {
    const double p = (pos / (double) ui_step_count) * (ui_lcend - ui_lcstart) + ui_lcstart;

    double v = _sc_inv(p);
    if (range_scale > 0.0)
      v = floor(v/range_scale+0.5)*range_scale;

    double minv = range_min, maxv = range_max;
    if (minv > maxv) { maxv = minv; minv = range_max; }
    if (v < minv) v = minv;
    else if (v > maxv) v = maxv;
    return v;
  }
  return range_min + (pos * (range_max-range_min))/(double)ui_step_count;
}

int effectSlider::scaleToUI(double cv) const
{
  if (range_min < range_max)
  {
    if (cv <= range_min) return 0;
    if (cv >= range_max) return ui_step_count;
  }
  else
  {
    if (cv >= range_min) return 0;
    if (cv <= range_max) return ui_step_count;
  }

  if (ui_is_shape != SHAPE_LINEAR)
  {
    const double l = _sc(cv);
    return (int) floor((l - ui_lcstart) * ui_step_count / (ui_lcend - ui_lcstart) + 0.5);
  }

  return (int) (((cv-range_min)*ui_step_count)/(range_max-range_min));
}

double effectSlider::scaleFromExternal(double cv) const
{
  if (!external_shaping) return cv;

  double v = _sc_inv(cv);
  if (range_scale > 0.0)
    v = floor(v/range_scale+0.5)*range_scale;

  return v;
}

double effectSlider::scaleToExternal(double cv) const
{
  if (!external_shaping) return cv;

  return _sc(cv);
}

static void UpdateIOButton(HWND b, int nch, SX_Instance *_this)
{
  if (nch<0)
  {
    return;
  }

  int ins, outs;
  sx_getPinInfo(_this, 0, &ins);
  sx_getPinInfo(_this, 1, &outs);

  if (ins < 0) ins = nch;
  if (outs < 0) outs = nch;

  char buf[256];

  if (ins && outs)
  {
    if (ins <= nch && outs <= nch) snprintf(buf, sizeof(buf), __LOCALIZE_VERFMT("%d in %d out","jsfx"), ins, outs);
    else if (ins <= nch) snprintf(buf,sizeof(buf), __LOCALIZE_VERFMT("%d in %d/%d out","jsfx"), ins, nch, outs);
    else if (outs <= nch) snprintf(buf,sizeof(buf), __LOCALIZE_VERFMT("%d/%d in %d out","jsfx"), nch, ins, outs);
    else snprintf(buf, sizeof(buf), __LOCALIZE_VERFMT("%d/%d in %d/%d out","jsfx"), nch, ins, nch, outs);
  }
  else if (ins)
  {
    if (ins <= nch) snprintf(buf, sizeof(buf), __LOCALIZE_VERFMT("%d in","jsfx"), ins);
    else snprintf(buf, sizeof(buf), __LOCALIZE_VERFMT("%d/%d in","jsfx"), nch, ins);
  }
  else if (outs)
  {
    if (outs <= nch) snprintf(buf, sizeof(buf), __LOCALIZE_VERFMT("%d out","jsfx"), outs);
    else snprintf(buf, sizeof(buf), __LOCALIZE_VERFMT("%d/%d out","jsfx"), nch, outs);
  }
  else
  {
    lstrcpyn_safe(buf, __LOCALIZE("MIDI","jsfx"),sizeof(buf));
  }

  SetWindowText(b,buf);
}

  //{IDC_SL_LABEL0,IDC_SL_0,IDC_SL_EDIT0,IDC_CB0},

#define SPACING_AT_BOTTOM_OF_CONTROLS (4 * cur_dpi/256)

#define SLIDER_HEIGHT (18 * cur_dpi / 256)
#define SLIDER_SPACING (2 * cur_dpi / 256)
#define EMPTY_SLIDER_SIZE (8 * cur_dpi / 256)

#ifdef _WIN32
#define SLIDER_LABEL_YOFFS 0
#define SLIDER_LABEL_YOFFS_COMBO 2
#define SLIDER_SPACING_COMBO 6
#elif defined(__APPLE__)
#define SLIDER_LABEL_YOFFS 3
#define SLIDER_LABEL_YOFFS_COMBO 3
#define SLIDER_SPACING_COMBO 3
#else
#define SLIDER_LABEL_YOFFS 0
#define SLIDER_LABEL_YOFFS_COMBO 0
#define SLIDER_SPACING_COMBO 2
#endif
#define SLIDER_LABEL_SIZE(w) (wdl_min((w)/2,220) * cur_dpi / 256)
#define LABEL_RIGHT_PAD 4
#define EDIT_PAD 2
#define COMBO_SIZE(w) ((w) - (SLIDER_LABEL_SIZE((w))+LABEL_RIGHT_PAD))
#define EDIT_SIZE(w) (wdl_min(70,(w)/6) * cur_dpi/256)

static int _GetWindowDPIScalingForDialog(HWND hwnd)
{
  if (GetWindowDPIScalingForDialog) return GetWindowDPIScalingForDialog(hwnd);

#ifdef REAJS
  const int mm2_dpi = WDL_WndSizer::calc_dpi(hwnd);
  if (mm2_dpi>0) return mm2_dpi;

#ifdef _WIN32
  // win32 dialogs don't support multimonitor hidpi properly
  HDC hdc = GetDC(hwnd);
  if (hdc)
  {
    const int sc = GetDeviceCaps(hdc,LOGPIXELSX);
    ReleaseDC(hwnd,hdc);
    if (sc > 0) return sc * 256 / 96;
  }
#endif // _WIN32

#endif // REAJS

  return 256;
}


void SX_Instance::DoInitDialog(HWND hwndDlg)
{
  resizer.init(hwndDlg);
  resizer_initrect_real = resizer.get_orig_rect();

  resizer.init_item(IDC_EFFECTNAME,0,0,1,0);
  resizer.init_item(IDC_EDITFX,1,0,1,0);

  if (GetDlgItem(hwndDlg,IDC_COMBO1)) resizer.init_item(IDC_COMBO1, 0, 0, 1, 0);
  resizer.init_item(IDC_BUTTON1, 1, 0, 1, 0);
  if (GetDlgItem(hwndDlg,IDC_IO)) resizer.init_item(IDC_IO,1,0,1,0);
  if (GetDlgItem(hwndDlg,IDC_LASTPARM)) resizer.init_item(IDC_LASTPARM, 1, 0, 1, 0);
  HWND knob = GetDlgItem(hwndDlg, IDC_KNOB);
  if (knob) resizer.init_item(IDC_KNOB, 1, 0, 1, 0);
  if (GetDlgItem(hwndDlg,IDC_BYPASS_FX)) resizer.init_item(IDC_BYPASS_FX,1,0,1,0);

  resizer.init_item(IDC_OUTVU,1,0,1,1);
  resizer.init_item(IDC_INVU,0,0,0,1);

  if (knob)
  {
    bool delta_solo=fxGetSetDeltaSolo && fxGetSetDeltaSolo(m_hostctx, NULL);
    SendMessage(knob, WM_USER+1007, WM_USER+4302, (LPARAM)&delta_solo);
  }

  const int cur_dpi = _GetWindowDPIScalingForDialog(hwndDlg);
  m_last_dpi = cur_dpi;

  RECT r;
  GetClientRect(hwndDlg,&r);
  int ow=r.right-r.left;
  GetWindowRect(GetDlgItem(hwndDlg,  m_uses_triggers ? IDC_TRIG0 : IDC_EFFECTNAME),&r);
  ScreenToClient(hwndDlg,((LPPOINT)&r)+1);

  r.bottom+=SLIDER_SPACING;
  int cnt=0;
  int l_space=0,le_space=0;
  for (int x = 0; x < m_sliders.GetSize(); x ++)
  {
    effectSlider *slid = m_sliders.Get(x);
    if (slid && slid->show)
    {
      r.bottom += l_space+le_space;
      r.bottom += SLIDER_HEIGHT;
      l_space = (slid->sliderValueXlateList.GetSize() || slid->is_file_slider) ? SLIDER_SPACING_COMBO : SLIDER_SPACING;
      le_space=0;
      cnt++;
    }
    else
    {
      // an empty slider slot or a doubledash-started slider will put a gap in
      if (!slid || slid->name.Get()[0] == '-')
        le_space = EMPTY_SLIDER_SIZE;
    }
  }

  r.bottom+=SPACING_AT_BOTTOM_OF_CONTROLS+ + (l_space-SLIDER_SPACING);

  m_wnd_bottomofcontrols=r.bottom;
  if (gfx_hasCode())
  {
    if (!m_uses_triggers && !cnt && !m_last_error[0]) // gfx, no triggers, no sliders = full gfx!
    {
      HWND ref = GetDlgItem(hwndDlg,  IDC_IO);
      if (ref)
      {
        GetWindowRect(ref,&r);
        ScreenToClient(hwndDlg,((LPPOINT)&r)+1);
        r.bottom += SPACING_AT_BOTTOM_OF_CONTROLS;
        m_wnd_bottomofcontrols=r.bottom;
        PostMessage(hwndDlg,WM_SIZE,0,0); // need a size event to move things
      }
    }

    r.bottom+=m_gfx_reqh>0 ? m_gfx_reqh : 100;

    if (m_gfx_reqw>0)
    {
      int msz=0;

      if (wantInputMetering())
      {
        RECT t;
        GetWindowRect(GetDlgItem(hwndDlg,IDC_INVU),&t);
        ScreenToClient(hwndDlg,(LPPOINT)&t + 1);
        msz += t.right + 2;
      }
      if (wantOutputMetering())
      {
        RECT t;
        GetWindowRect(GetDlgItem(hwndDlg,IDC_OUTVU),&t);
        ScreenToClient(hwndDlg,(LPPOINT)&t);
        msz += ow - (t.left - 2);
      }
      if (m_gfx_reqw+msz>ow) ow=m_gfx_reqw+msz;
    }
  }


  SendMessage(hwndDlg, WM_USER+4400, 0, 0);

  SetWindowPos(hwndDlg,NULL,0,0,ow,r.bottom,SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOZORDER);
}

void SX_Instance::DoUpdate(HWND hwndDlg)
{
  if (WDL_NOT_NORMALLY(is_in_gfx())) return;

  char buf[512];
  m_string_sermutex.Enter();
  m_description_dirty=false;
  lstrcpyn_safe(buf,m_description.Get(),sizeof(buf));
  m_string_sermutex.Leave();
  SetDlgItemText(hwndDlg,IDC_EFFECTNAME,buf);

  ShowWindow(GetDlgItem(hwndDlg, IDC_INVU), !wantInputMetering() ? SW_HIDE : SW_SHOWNA);
  ShowWindow(GetDlgItem(hwndDlg, IDC_OUTVU),  !wantOutputMetering() ? SW_HIDE : SW_SHOWNA);

  int x;
  int hastrig=m_uses_triggers;
  static const unsigned short trigtab[11]={IDC_TRIGLBL,IDC_TRIG0,IDC_TRIG1,IDC_TRIG2,IDC_TRIG3,IDC_TRIG4,IDC_TRIG5,IDC_TRIG6,IDC_TRIG7,IDC_TRIG8,IDC_TRIG9};
  for (x = 0; x < 11; x ++)
  {
    ShowWindow(GetDlgItem(hwndDlg,trigtab[x]),hastrig?SW_SHOWNA:SW_HIDE);
  }
  const int cur_dpi = _GetWindowDPIScalingForDialog(hwndDlg);
  m_last_dpi = cur_dpi;

  WDL_WndSizer__rec *r1 = resizer.get_item(IDC_EDITFX);
  if (hastrig)
  {
    RECT r;
    GetWindowRect(GetDlgItem(hwndDlg,IDC_TRIG0),&r);
    ScreenToClient(hwndDlg,((LPPOINT) &r)+1);
    m_wnd_bottomofcontrols = r.bottom;
  }
  else
  {
    m_wnd_bottomofcontrols = WDL_NORMALLY(r1) ? r1->real_orig.bottom : 0;
  }
  m_wnd_bottomofcontrols += SLIDER_SPACING;

  RECT cr;
  GetClientRect(hwndDlg,&cr);
  RECT tr;
  GetClientRect(GetDlgItem(hwndDlg,IDC_INVU),&tr);
  if (wantInputMetering()) cr.left += tr.right+2;
  if (wantOutputMetering()) cr.right -= tr.right+8;
  HWND lh=NULL;

#ifndef _WIN32
  SWELL_MakeSetCurParms(1.5,1.5,0,0,hwndDlg,true,false);
#endif

#define DESTROY_CLEAR(x) do { if (x) { DestroyWindow(x); (x)=NULL;  } } while (0)

  int last_spacing=0,le_space=0;
  int cnt=0;
  WDL_PtrList<void> need_show;
  for (x = 0; x < m_sliders.GetSize(); x ++)
  {
    effectSlider *slider=m_sliders.Get(x);
    if (!slider || !slider->show)
    {
      if (slider)
      {
        DESTROY_CLEAR(slider->ui.slider);
        if (slider->ui.edit && SetWindowAccessibilityString)
          SetWindowAccessibilityString(slider->ui.edit, NULL, 0);
        DESTROY_CLEAR(slider->ui.edit);
        DESTROY_CLEAR(slider->ui.label);
        DESTROY_CLEAR(slider->ui.combo);
      }

      // an empty slider slot or a doubledash-started slider will put a gap in
      if (!slider || slider->name.Get()[0] == '-')
        le_space=EMPTY_SLIDER_SIZE;
      continue;
    }

    HWND h=slider->ui.label;
    if (!h)
    {
      //create label
      #ifdef _WIN32
      h=CreateWindowEx(WS_EX_NOPARENTNOTIFY,"Static","",WS_CHILDWINDOW|WS_GROUP|SS_RIGHT|SS_LEFT|SS_CENTERIMAGE,0,0,10,10,hwndDlg,NULL,g_hInst,0);
      if (h)
      {
        SetWindowLong(h,GWL_ID,SLIDER_LABEL_ID(x));
        SendMessage(h,WM_SETFONT,SendMessage(hwndDlg,WM_GETFONT,0,0),false);
      }
      #else
      h = SWELL_MakeLabel(1,"",SLIDER_LABEL_ID(x),0,0,10,10,SS_CENTERIMAGE|WS_GROUP);
      #endif
      slider->ui.label=h;
    }


    if (h)
    {
      cnt++;
      m_wnd_bottomofcontrols += last_spacing+le_space;
      const bool isCb=slider->sliderValueXlateList.GetSize() || slider->is_file_slider;
      SetWindowPos(h,lh,cr.left,m_wnd_bottomofcontrols+(isCb?SLIDER_LABEL_YOFFS_COMBO:SLIDER_LABEL_YOFFS),SLIDER_LABEL_SIZE(cr.right-cr.left),SLIDER_HEIGHT,SWP_NOACTIVATE);
      lh=h;
      SetWindowText(h,slider->name.Get());
      need_show.Add((void *)h);

      if (isCb)
      {
        if (slider->ui.edit && SetWindowAccessibilityString)
          SetWindowAccessibilityString(slider->ui.edit, NULL, 0);
        DESTROY_CLEAR(slider->ui.edit);
        DESTROY_CLEAR(slider->ui.slider);
        HWND combo=slider->ui.combo;
        if (!combo)
        {
        #ifdef _WIN32
          combo = CreateWindowEx(0,"ComboBox","",WS_CHILDWINDOW|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,0,0,10,600,hwndDlg,NULL,g_hInst,0);
          if (combo)
          {
            WDL_UTF8_HookComboBox(combo);
            SetWindowLong(combo,GWL_ID,SLIDER_COMBO_ID(x));
            SendMessage(combo,WM_SETFONT,SendMessage(hwndDlg,WM_GETFONT,0,0),false);
          }
        #else
          combo = SWELL_MakeCombo(SLIDER_COMBO_ID(x),0,0,10,10,CBS_DROPDOWNLIST|WS_TABSTOP);
        #endif
          slider->ui.combo=combo;
        }
        if (combo)
        {
          SetWindowPos(combo,lh,cr.left+SLIDER_LABEL_SIZE(cr.right-cr.left) + LABEL_RIGHT_PAD,m_wnd_bottomofcontrols,COMBO_SIZE(cr.right-cr.left),SLIDER_HEIGHT,SWP_NOACTIVATE);
          need_show.Add((HWND)combo);
          lh=combo;
          SendMessage(combo,CB_RESETCONTENT,0,0);
        }

        int offs=0;
        if (slider->is_file_slider)
        {
          if (combo) for (int i = 0; i < slider->FileList.GetSize(); i ++)
          {
            SendMessage(combo,CB_ADDSTRING,0,(LPARAM)slider->FileList.Get(i));
          }
        }
        else
        {
          int i;
          if (combo) for (i = 0; i < slider->sliderValueXlateList.GetSize(); i ++)
          {
            SendMessage(combo,CB_ADDSTRING,0,(LPARAM)slider->sliderValueXlateList.Get(i));
          }
        }
        if (slider->slider)
        {
          int a=(int) (slider->slider[0]+0.5) - offs;
          if (combo) SendMessage(combo,CB_SETCURSEL,a,0);
        }
      }
      else
      {
        DESTROY_CLEAR(slider->ui.combo);
        HWND hslid=slider->ui.slider;
        if (!hslid)
        {
        #ifdef _WIN32
          hslid = CreateWindowEx(0,g_config_slider_classname,"",WS_CHILDWINDOW|WS_TABSTOP|1,0,0,10,10,hwndDlg,NULL,g_hInst,0);
          if (hslid)
          {
            SetWindowLong(hslid,GWL_ID,SLIDER_SLIDER_ID(x));
          }
        #else
          hslid = SWELL_MakeControl("", SLIDER_SLIDER_ID(x), g_config_slider_classname, WS_TABSTOP|1, 0,0,10,10, 0);
        #endif
          slider->ui.slider=hslid;
        }
        HWND edit=slider->ui.edit;
        if (!edit)
        {
        #ifdef _WIN32
          edit = CreateWindowEx(WS_EX_CLIENTEDGE,"Edit","",WS_CHILDWINDOW|WS_TABSTOP|ES_LEFT|ES_AUTOHSCROLL,0,0,10,10,hwndDlg,NULL,g_hInst,0);
          if (edit)
          {
            SetWindowLong(edit,GWL_ID,SLIDER_EDIT_ID(x));
            SendMessage(edit,WM_SETFONT,SendMessage(hwndDlg,WM_GETFONT,0,0),false);
          }
        #else
          edit = SWELL_MakeEditField(SLIDER_EDIT_ID(x),0,0,10,10,WS_TABSTOP|ES_AUTOHSCROLL);
        #endif

          slider->ui.edit=edit;
        }
        if (slider->ui.edit && SetWindowAccessibilityString)
          SetWindowAccessibilityString(slider->ui.edit, slider->name.Get(), 0);

        if (hslid)
        {
          int xpos=cr.left + SLIDER_LABEL_SIZE(cr.right-cr.left) + LABEL_RIGHT_PAD;
          SetWindowPos(hslid,lh,
            xpos,m_wnd_bottomofcontrols,cr.right - EDIT_SIZE(cr.right-cr.left) - EDIT_PAD - xpos,SLIDER_HEIGHT,SWP_NOACTIVATE); lh=hslid;
          int mv=slider->ui_step_count;
          SetWindowLong(hslid,0,1);
          SendMessage(hslid,TBM_SETRANGE,TRUE,MAKELONG(0,mv));
          SendMessage(hslid,TBM_SETTIC,0,slider->scaleToUI(slider->default_val));
          if (WDL_NORMALLY(slider->slider))
            SendMessage(hslid,TBM_SETPOS,TRUE,slider->scaleToUI(slider->slider[0]));
          need_show.Add((void *)hslid);
          SendMessage(hslid,WM_USER+9999,11 /* copy string*/,(LPARAM)slider->name.Get());
        }
        if (edit)
        {
          SetWindowPos(edit,lh,cr.right - EDIT_SIZE(cr.right-cr.left),m_wnd_bottomofcontrols,EDIT_SIZE(cr.right-cr.left),SLIDER_HEIGHT,SWP_NOACTIVATE);
          need_show.Add((void *)edit);
          lh=edit;
        }
        getSliderText(x,buf,sizeof(buf));
        if (edit) SetWindowText(edit,buf);
        if (hslid) SendMessage(hslid,WM_USER+9999,2,(LPARAM)buf);
      }
      m_wnd_bottomofcontrols+=SLIDER_HEIGHT;
      last_spacing=(isCb?SLIDER_SPACING_COMBO:SLIDER_SPACING);
      le_space=0;
    } // if h
  }
#undef DESTROY_CLEAR
  for (int i = 0; i < need_show.GetSize(); i ++)
    ShowWindow((HWND)need_show.Get(i),SW_SHOWNA);

  m_wnd_bottomofcontrols += SPACING_AT_BOTTOM_OF_CONTROLS + (last_spacing-SLIDER_SPACING);

  bool moveEdit = false;
  if (gfx_hasCode() && !m_uses_triggers && !cnt && !m_last_error[0]) // gfx, no triggers, no sliders = full gfx!
  {
    HWND ref = GetDlgItem(hwndDlg,  IDC_IO);
    if (ref)
    {
      RECT r;
      GetWindowRect(ref,&r);
      ScreenToClient(hwndDlg,((LPPOINT)&r)+1);
      r.bottom += SPACING_AT_BOTTOM_OF_CONTROLS;
      m_wnd_bottomofcontrols=r.bottom;
      ShowWindow(GetDlgItem(hwndDlg,IDC_EFFECTNAME),SW_HIDE);
      moveEdit=true;
    }
  }

  if (!moveEdit)
  {
    ShowWindow(GetDlgItem(hwndDlg,IDC_EFFECTNAME),SW_SHOWNA);
  }


  {
    WDL_WndSizer__rec *r2 = resizer.get_item(IDC_COMBO1);
    WDL_WndSizer__rec *r3 = resizer.get_item(IDC_BUTTON1);
    WDL_WndSizer__rec *r4 = resizer.get_item(IDC_LASTPARM);

    if (r1) r1->orig = r1->real_orig;
    if (r2) r2->orig = r2->real_orig;
    if (r3) r3->orig = r3->real_orig;
    if (r4) r4->orig = r4->real_orig;
    if (moveEdit && r1 && r2 && r3 && r4)
    {
      const int gap = r4->orig.left - r3->orig.right;
      const int dx = r1->orig.right - r1->orig.left + gap;
      r2->orig.right -= dx;
      r3->orig.left -= dx;
      r3->orig.right -= dx;
      r1->orig = r4->orig;
      r4->orig.left -= dx;
      r4->orig.right -= dx;
      r1->orig.left = r4->orig.right + gap;
      r1->orig.right = r1->orig.left + (r1->real_orig.right-r1->real_orig.left);
    }
  }


#ifndef _WIN32
  SWELL_MakeSetCurParms(1.5,1.5,0,0,NULL,true,false);
#endif
  if (m_last_error[0]) SetDlgItemText(hwndDlg,IDC_EFFECTNAME,m_last_error);

  m_last_ionch=0;
  if (m_hostnch != m_last_ionch)
  {
    m_last_ionch = m_hostnch;
    HWND io = GetDlgItem(hwndDlg,IDC_IO);
    if (io)
      UpdateIOButton(io,m_last_ionch,this);
  }
}

#define GFX_TIMER_INTERVAL (m_gfx_hz<1 ? 33 : 1000/wdl_min(m_gfx_hz,300))

void SX_Instance::DoRecompile(HWND hwndDlg, int resetFlags, WDL_FastString *strout)
{
  {
    if (m_in_gfx) return; // FAIL, gfx code running, ugh!

    WDL_PtrList<void> hwnd_destroy_list;
    WDL_FastString foo;
    m_mutex.Enter();

    m_last_error[0]=0;

    LoadEffect(m_fn.Get(),resetFlags, &hwnd_destroy_list);

    foo.Set(m_last_error);

    DoImageLoads(resetFlags & LOADEFFECT_RESETFLAG_VARS);

    m_mutex.Leave();

    for (int x = 0; x < hwnd_destroy_list.GetSize(); x ++)
      DestroyWindow((HWND)hwnd_destroy_list.Get(x));

    if (foo.Get()[0])
    {
      if (strout) strout->Set(foo.Get());
      else MessageBox(hwndDlg,foo.Get(),"Error Recompiling",MB_OK);
    }
    else
      if (m_hwnd) DoUpdate(m_hwnd);

    if (m_hwnd)
    {
      KillTimer(m_hwnd,1);
      SetTimer(m_hwnd,1,GFX_TIMER_INTERVAL,NULL);
      SendMessage(m_hwnd,WM_SIZE,0,0);
      InvalidateRect(m_hwnd,NULL,TRUE);
    }
  }
}

void SX_Instance::OnCommand(HWND hwndDlg, int command, int code, int lParam)
{
  if (WDL_NOT_NORMALLY(is_in_gfx())) return;
  if (command == IDC_COMBO1 && code == CBN_SELCHANGE)
  {
    int a=SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
    int type=SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETITEMDATA, a, 0);
    if (type == 1)
    {
      char buf[512];
      GetDlgItemText(hwndDlg, IDC_COMBO1, buf, sizeof(buf));
      if (buf[0]) loadPreset(buf);
    }
    else if (type == 3)
    {
      loadPreset(0);
    }
    else if (m_curpresetname.Get())
    {
      SendMessage(hwndDlg, WM_USER+4400, 0, 0);
    }
  }
  else if (command == IDC_BUTTON1 && m_hostctx)
  {
    RECT r;
    GetWindowRect(GetDlgItem(hwndDlg, IDC_BUTTON1), &r);
    HMENU menu=LoadMenu(g_hInst,MAKEINTRESOURCE(IDR_MENU2));
    HMENU sm = menu ? GetSubMenu(menu,0) : CreatePopupMenu();

    if (m_config_items.GetSize())
    {
      for (int x = 0; x < m_config_items.GetSize(); x ++)
      {
        effectConfigItem *ci = m_config_items.Get(x);
        HMENU hsub = CreatePopupMenu();
        InsertMenu(hsub,0,MF_BYPOSITION|MF_STRING|MF_GRAYED, 0,
            ci->preserve_config ? __LOCALIZE("Note: changing this will result in a reset of this plug-in but its parameters should be preserved.","jsfx") :
            __LOCALIZE("Note: changing this will result in a full reset of this plug-in and all of its parameters.","jsfx")
            );
        InsertMenu(hsub, 1, MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
        for (int i = 0; i < ci->values.GetSize(); i ++)
        {
          char tmp[64];
          effectConfigItem::rec *r = ci->values.Get(i);
          if (!r->name.GetLength())
          {
            snprintf(tmp,sizeof(tmp),"%.10f",r->v);
            WDL_remove_trailing_decimal_zeros(tmp,0);
          }
          int fl = 0;
#define CONFIG_ITEM_BASE_ID (1<<20)
          if (r->v == ci->cur_val) fl |= MF_CHECKED;
          InsertMenu(hsub, i+2, MF_BYPOSITION|MF_STRING|fl,
              CONFIG_ITEM_BASE_ID + x*4096 + i,r->name.GetLength()?r->name.Get():tmp);
        }
        InsertMenu(sm, x, MF_BYPOSITION|MF_STRING|MF_POPUP, (INT_PTR)hsub, ci->desc.GetLength() ? ci->desc.Get() : ci->symbol.Get());
      }
      if (menu) // not inserting into empty menu
        InsertMenu(sm, m_config_items.GetSize(), MF_BYPOSITION|MF_SEPARATOR, 0, NULL);
    }

    if (fxGetReaperPresetNamesRaw)
    {
      HWND combo = GetDlgItem(hwndDlg,IDC_COMBO1);
      int a=combo ? SendMessage(combo, CB_GETCURSEL, 0, 0) : 0;
      int type=combo ? SendMessage(combo, CB_GETITEMDATA, a, 0) : 0;

      int sz=fxGetReaperPresetNamesRaw(m_hostctx, 0);

      EnableMenuItem(menu, IDC_RENAME_PRESET, MF_BYCOMMAND|(type == 1 || type == 2 ? MF_ENABLED : MF_GRAYED));
      EnableMenuItem(menu, IDC_DELETE_PRESET, MF_BYCOMMAND|(type == 1 ? MF_ENABLED : MF_GRAYED));
      EnableMenuItem(menu, IDC_MOVE_UP_PRESET, MF_BYCOMMAND|(type == 1 && a-2 > 0 ? MF_ENABLED : MF_GRAYED));
      EnableMenuItem(menu, IDC_MOVE_DOWN_PRESET, MF_BYCOMMAND|(type == 1 && a-2 < sz-1 ? MF_ENABLED : MF_GRAYED));

      if (fxDoReaperPresetAction)
      {
        fxDoReaperPresetAction(m_hostctx,(const char *)sm,20);
      }
    }

    int ret=TrackPopupMenu(sm,TPM_RIGHTBUTTON|TPM_LEFTALIGN|TPM_NONOTIFY|TPM_RETURNCMD,r.left,r.bottom,0,hwndDlg,NULL);
    DestroyMenu(menu ? menu : sm);

    if (ret)
    {
      if (ret >= CONFIG_ITEM_BASE_ID && ret < CONFIG_ITEM_BASE_ID + m_config_items.GetSize()*4096)
      {
        effectConfigItem *item = m_config_items.Get((ret-CONFIG_ITEM_BASE_ID) / 4096);
        if (item)
        {
          effectConfigItem::rec *rec = item->values.Get(ret&4095);
          if (rec)
          {
            if (item->cur_val != rec->v)
            {
              item->cur_val = rec->v;
              if (item->preserve_config)
              {
                char *sbuf = NULL;
                int sz=0;
                if (m_serialize_ch)
                {
                  const char *p=SaveSerState(&sz);
                  if (sz>0 && p)
                  {
                    sbuf = (char *)malloc(sz);
                    if (sbuf) memcpy(sbuf,p,sz);
                  }
                }
                DoRecompile(hwndDlg, LOADEFFECT_RESETFLAG_VARS);
                if (sbuf)
                {
                  LoadSerState(sbuf,sz);
                  free(sbuf);
                }
              }
              else
                DoRecompile(hwndDlg, LOADEFFECT_RESETFLAG_VARS|LOADEFFECT_RESETFLAG_SLIDERS);

              if (fxSetUndoPoint) fxSetUndoPoint(m_hostctx);
            }
          }
        }
      }
      else if (ret > 1000 && fxDoReaperPresetAction(m_hostctx,NULL,ret))
      {
      }
      else
      {
        SendMessage(hwndDlg, WM_COMMAND, ret, 0);
      }
    }
  }
  else if ((command == ID_IMPORT_RPL || command == ID_EXPORT_RPL) && fxImportExportRPL)
  {
    if (fxImportExportRPL(m_hostctx, hwndDlg, (command == ID_EXPORT_RPL)))
    {
      if (command == ID_IMPORT_RPL)
      {
        SendMessage(hwndDlg, WM_USER+4400, 0, 0);
      }
    }
  }
  else if (command == IDC_SAVE_PRESET ||
    command == IDC_SAVE_PRESET_ASDEFAULT ||
    command == IDC_DELETE_PRESET ||
    command == IDC_MOVE_UP_PRESET ||
    command == IDC_MOVE_DOWN_PRESET ||
    command == IDC_RENAME_PRESET)
  {
    HWND combo = GetDlgItem(hwndDlg,IDC_COMBO1);
    if (!combo) return;
    int a=SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);
    int type=SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_GETITEMDATA, a, 0);

    char buf[512];
    buf[0]=0;
    GetDlgItemText(hwndDlg, IDC_COMBO1, buf, sizeof(buf));

    if ((command == IDC_SAVE_PRESET || command == IDC_SAVE_PRESET_ASDEFAULT) && fxPresetPromptForAction)
    {
      if (fxPresetPromptForAction(hwndDlg, m_hostctx,command == IDC_SAVE_PRESET_ASDEFAULT?1:0, buf, sizeof(buf)))
      {
        m_curpresetname.Set(buf);
        int flag = (command == IDC_SAVE_PRESET ? 0 : 1);
        fxDoReaperPresetAction(m_hostctx, buf, flag);
        SendMessage(hwndDlg, WM_USER+4400, 0, 0);
      }
    }
    else if (command == IDC_DELETE_PRESET && fxDoReaperPresetAction)
    {
      char buf2[1024];
      snprintf(buf2, sizeof(buf2),"Delete preset \"%s\"?", buf);
      int ret=MessageBox(hwndDlg, buf2, "Confirm Preset Delete", MB_YESNO);
      if (ret == IDYES)
      {
        fxDoReaperPresetAction(m_hostctx, buf, 2);
        m_curpresetname.Set("");
        SendMessage(hwndDlg, WM_USER+4400, 0, 0);
      }
    }
    else if (command == IDC_RENAME_PRESET && fxPresetPromptForAction && fxRenameReaperPreset)
    {
      char obuf[512];
      strcpy(obuf, buf);
      if (fxPresetPromptForAction(hwndDlg, m_hostctx,2, buf, sizeof(buf)) && strcmp(buf, obuf))
      {
        if (type == 1) // reaper preset
        {
          fxRenameReaperPreset(m_hostctx, obuf, buf);
        }
        m_curpresetname.Set(buf);

        SendMessage(hwndDlg, WM_USER+4400, 0, 0);
      }
    }
    else if ((command == IDC_MOVE_UP_PRESET || command == IDC_MOVE_DOWN_PRESET) && fxDoReaperPresetAction)
    {
      int flag = (command == IDC_MOVE_UP_PRESET ? 3 : 4);
      fxDoReaperPresetAction(m_hostctx, buf, flag);
      SendMessage(hwndDlg, WM_USER+4400, 0, 0);
    }
  }
  else if (command == IDC_BYPASS_FX)
  {
    int checked = IsDlgButtonChecked(hwndDlg, IDC_BYPASS_FX);
    if (m_hostctx && fxSetBypass) fxSetBypass(m_hostctx, !checked);
  }
  else if (command == IDC_KNOB)
  {
    double val = (double)LOWORD(lParam)/65535.0;
    bool done = !!HIWORD(lParam);
    if (m_hostctx && fxSetWet) fxSetWet(m_hostctx, val, done);
  }
  else if (command == IDC_LASTPARM)
  {
    if (m_hostctx && DoFxLastTweakParmCtxMenu)
    {
      RECT r;
      GetWindowRect(GetDlgItem(hwndDlg, IDC_LASTPARM), &r);
      DoFxLastTweakParmCtxMenu(m_hostctx, hwndDlg, r.left, r.bottom, NULL);
    }
  }
  else if (command == IDC_EDITFX)
  {
    if (GetAsyncKeyState(VK_SHIFT)&0x8000) { DoRecompile(hwndDlg,0 /* preserve everything */); }
    else if (GetAsyncKeyState(VK_CONTROL)&0x8000) { DoRecompile(hwndDlg,LOADEFFECT_RESETFLAG_CONFIGITEMS|LOADEFFECT_RESETFLAG_VARS|LOADEFFECT_RESETFLAG_SLIDERS); }
    else if (m_hwndwatch) SetForegroundWindow(m_hwndwatch);
    else
    {
      HWND h=CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_JSDEBUG),
        GetMainHwnd && GetMainHwnd() ? GetMainHwnd() : hwndDlg
        ,SX_Instance::_watchDlgProc,(LPARAM)this);

      ShowWindow(h,SW_SHOW);
    }
  }
  else if (command == IDC_IO)
  {
    int ins, outs;
    sx_getPinInfo(this, 0, &ins);
    sx_getPinInfo(this, 1, &outs);
    if (ins || outs)
    {
      SendMessage(GetParent(hwndDlg),WM_USER+1030,(WPARAM)this,(LPARAM)hwndDlg); // notify parent of I/O request
    }
  }
  else if (command == IDC_TRIG0) m_trig|=1;
  else if (command == IDC_TRIG1) m_trig|=2;
  else if (command == IDC_TRIG2) m_trig|=4;
  else if (command == IDC_TRIG3) m_trig|=8;
  else if (command == IDC_TRIG4) m_trig|=16;
  else if (command == IDC_TRIG5) m_trig|=32;
  else if (command == IDC_TRIG6) m_trig|=64;
  else if (command == IDC_TRIG7) m_trig|=128;
  else if (command == IDC_TRIG8) m_trig|=256;
  else if (command == IDC_TRIG9) m_trig|=512;
  else if (command >= SLIDER_ID_BASE && command < SLIDER_ID_BASE+SLIDER_ID_WIDTH*NUM_SLIDERS)
  {
    int x = (command-SLIDER_ID_BASE)/SLIDER_ID_WIDTH;
    int command_type = (command-SLIDER_ID_BASE)%SLIDER_ID_WIDTH;

    if (command_type == 2)
    {
      if (code == EN_KILLFOCUS)
      {
        char buf[512];
        GetDlgItemText(hwndDlg,command,buf,sizeof(buf));
        if (buf[0])
        {
          effectSlider *slider = m_sliders.Get(x);
          if (!slider) return;
          if (slider->slider)
          {
            slider->slider[0] = flexi_atof(buf);
            if (slider->ui.slider)
            {
              SendMessage(slider->ui.slider,TBM_SETPOS,TRUE,slider->scaleToUI(slider->slider[0]));
              SendMessage(slider->ui.slider,WM_USER+9999,2,(LPARAM)buf);
            }
            if (slider->is_file_slider) slider->file_dirty = true;
          }

          m_mutex.Enter();
          on_slider_change();
          m_mutex.Leave();

          if (m_slider_automate) m_slider_automate(m_hostctx, indexToParm(x), false);
        }
      }
      return;
    }
    else if (command_type == 3)
    {
      effectSlider *slider = m_sliders.Get(x);
      if (!slider || !slider->ui.combo) return;
      int a=(int)SendMessage(slider->ui.combo,CB_GETCURSEL,0,0);

      m_mutex.Enter();
      if (slider->is_file_slider) slider->file_dirty = true;
      if (slider->slider) slider->slider[0]=(double)a;
      on_slider_change();
      m_mutex.Leave();

      if (m_slider_automate) m_slider_automate(m_hostctx, indexToParm(x), false);

      return;
    }
  }
}

void SX_Instance::DoScroll(HWND hwndDlg, int x, int code)
{
  if (WDL_NOT_NORMALLY(is_in_gfx())) return;

  effectSlider *slider = m_sliders.Get(x);
  if (!slider) return;
  HWND slid = slider->ui.slider;
  int pos=slid ? SendMessage(slid, TBM_GETPOS, 0, 0) : 0;

  int isCb=slider->sliderValueXlateList.GetSize() || slider->is_file_slider;

  if (!isCb)
  {
    if (slider->slider)
    {
      const double p = slider->scaleFromUI(pos);
      m_mutex.Enter();
      slider->slider[0] = p;
      if (slider->is_file_slider) slider->file_dirty = true;
      on_slider_change();
      m_mutex.Leave();
    }

    char buf[512];
    getSliderText(x,buf,sizeof(buf));
    if (slid) SendMessage(slid,WM_USER+9999,2,(LPARAM)buf);
    if (slider->ui.edit) SetWindowText(slider->ui.edit,buf);

    if (m_slider_automate) m_slider_automate(m_hostctx, indexToParm(x), (code == SB_ENDSCROLL));
  }
}

void SX_Instance::OpenExternal(HWND hwndDlg)
{
  if (m_fn.GetLength())
  {
    char path[2048];
    snprintf(path,sizeof(path),"%s" WDL_DIRCHAR_STR "%s",m_effectdir.Get(),m_fn.Get());

    char exe[1024];
    const char *exep = "";
    if (get_ini_file)
    {
      GetPrivateProfileString("extedit", "jsfx", "", exe, sizeof(exe), get_ini_file());
      char *p = exe;
      while (*p && *p != '|') p++;
      if (*p) *p++=0; // *p will be nonzero if a secondary editor is configured

      if (GetAsyncKeyState(VK_SHIFT)&0x8000)
      {
        if (*p)
        {
          exep = p;
          while (*p && *p != '|') p++;
          *p=0;
        }
      }
      else
      {
        exep = exe;
      }
    }

    ShellExecute(hwndDlg, exep[0] ? NULL : "open", exep[0] ? exep : "notepad", path, NULL, SW_SHOWDEFAULT);
  }
  else MessageBox(hwndDlg,__LOCALIZE("No Effect Loaded!","jsfx"),__LOCALIZE("Error Editing","jsfx"),MB_OK);
}

static WDL_PtrList<SX_Instance> s_idle_list;
static UINT_PTR s_idle_timer;
static void CALLBACK idle_timer_proc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
  for (int x = 0; x < s_idle_list.GetSize(); x ++)
    s_idle_list.Get(x)->run_idle();
}

void SX_Instance::add_idle()
{
  if (WDL_NORMALLY(s_idle_list.Find(this)<0))
  {
    s_idle_list.Add(this);
    if (!s_idle_timer)
      s_idle_timer = SetTimer(NULL,0,66,idle_timer_proc);
  }
}

void SX_Instance::remove_idle()
{
  s_idle_list.DeletePtr(this);
  if (!s_idle_list.GetSize() && s_idle_timer)
  {
    KillTimer(NULL,s_idle_timer);
    s_idle_timer = 0;
  }
}

void SX_Instance::run_idle()
{
  if (!m_gfx_ch ||
      WDL_NOT_NORMALLY(!m_gfx_want_idle) ||
      WDL_NOT_NORMALLY(!m_lice_state) ||
      m_lice_state->hwnd_standalone ||
      (GetTickCount()-m_gfx_last_inline_runtime) < 500 ||
      m_in_gfx)
    return;

  m_in_gfx++;

  if (m_need_init)
  {
    m_mutex.Enter();
    m_init_mutex.Enter();
    if (m_need_init) on_slider_change();
    m_mutex.Leave();
  }
  else
  {
    m_init_mutex.Enter();
  }

  gfx_runCode(2);

  m_init_mutex.Leave();

  m_in_gfx--;
}

WDL_DLGRET SX_Instance::ConfigDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

  switch (uMsg)
  {
    case WM_INITDIALOG:
      {
        m_hwnd=hwndDlg;
        if (s_configWindows.Find(hwndDlg)<0) s_configWindows.Add(hwndDlg);
        for (int x = 0; x < m_sliders.GetSize(); x ++)
        {
          effectSlider *slid = m_sliders.Get(x);
          if (slid) memset(&slid->ui,0,sizeof(slid->ui));
        }

        if (SetWindowAccessibilityString)
        {
          HWND byp = GetDlgItem(hwndDlg,IDC_BYPASS_FX);
          if (byp)
            SetWindowAccessibilityString(byp,__LOCALIZE("FX Active","fx"),0);
          SetWindowAccessibilityString(GetDlgItem(hwndDlg,IDC_BUTTON1),__LOCALIZE("Preset and Performance Management","fx"), 0);
        }

        HWND combo = GetDlgItem(hwndDlg,IDC_COMBO1);
        if (combo) WDL_UTF8_HookComboBox(combo);
        DoInitDialog(hwndDlg); // calls resizer.init()
        DoUpdate(hwndDlg);
        SetTimer(hwndDlg,32,50,NULL);
        SetTimer(hwndDlg,1,GFX_TIMER_INTERVAL,NULL);
      }
    return 0;

    case WM_USER+4300:  // set bypass from outside
    {
      int bypass = (int)lParam;
      if (GetDlgItem(hwndDlg,IDC_BYPASS_FX))
        CheckDlgButton(hwndDlg, IDC_BYPASS_FX, (bypass ? BST_UNCHECKED : BST_CHECKED));
    }
    return 0;

    case WM_USER+4301:  // set wet from outside
    {
      HWND knob = GetDlgItem(hwndDlg,IDC_KNOB);
      if (knob)
        SendMessage(knob, WM_USER+1000, 0, lParam);
    }
    return 0;

    case WM_USER+4302: // set invert wet from knob
      if (fxGetSetDeltaSolo)
      {
        bool delta_solo = wParam ? 1 : 0;
        fxGetSetDeltaSolo(m_hostctx, &delta_solo);
      }
    return 0;

    case WM_USER+4400:  // update preset dropdown
      if (!m_preset_reent && fxGetReaperPresetNamesRaw && FreeHeapPtr)
      {
        HWND combo = GetDlgItem(hwndDlg,IDC_COMBO1);
        if (!combo) return 0;
        ++m_preset_reent;

        SendMessage(combo, CB_RESETCONTENT, 0, 0);

        int cursel=-1; // index of preset that matches m_curpresetname

        char** names=0;
        int numreaperpresets=fxGetReaperPresetNamesRaw(m_hostctx, &names);

        SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("Reset to factory default","jsfx"));
        SendMessage(combo, CB_SETITEMDATA, 0, 3);

        int i;
        for (i=0; i < numreaperpresets; ++i)
        {
          if (!i) SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)__LOCALIZE("----  User Presets (.rpl)  ----","jsfx"));
          const char* name=names[i];
          int a=SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)name);
          SendMessage(combo, CB_SETITEMDATA, a, 1);
          if (cursel < 0 && !strcmp(name, m_curpresetname.Get())) cursel=a;
          FreeHeapPtr(names[i]);
        }
        FreeHeapPtr(names);

        if (cursel < 0) // what we think is the current preset name, wasn't found
        {
          cursel=1;
          SendMessage(combo, CB_INSERTSTRING, cursel, (LPARAM)__LOCALIZE("No preset","jsfx"));
          m_curpresetname.Set("");
        }

        SendMessage(combo, CB_SETCURSEL, cursel, 0);

        --m_preset_reent;
      }
    return 0;

    case WM_USER+2340:
      DoUpdate(hwndDlg);

    return 0;
    case WM_COMMAND:
      {
        OnCommand(hwndDlg,LOWORD(wParam),HIWORD(wParam),lParam);
      }
    return 0;
    case WM_HSCROLL:
      {
        HWND h = (HWND) lParam;
        if (h)
        {
          for (int a=0;a<m_sliders.GetSize();a++)
          {
            effectSlider *slid = m_sliders.Get(a);
            if (slid && slid->ui.slider == h)
            {
              DoScroll(hwndDlg,a,LOWORD(wParam));
              break;
            }
          }
        }
      }
    return 0;
    case WM_CONTEXTMENU:
      if (wParam && (HWND)wParam == GetDlgItem(hwndDlg, IDC_IO))
      {
        SendMessage(GetParent(hwndDlg),WM_USER+1030,(WPARAM)this,0);
        return -1;
      }
      if (wParam && (HWND)wParam == GetDlgItem(hwndDlg, IDC_LASTPARM))
      {
        SendMessage(hwndDlg, WM_COMMAND, IDC_LASTPARM, 0);
        return -1;
      }
      if (wParam && (HWND)wParam == GetDlgItem(hwndDlg,IDC_EDITFX))
      {
        OpenExternal(hwndDlg);
      }
      return 1;
    case WM_MOUSEMOVE:

      if (GetCapture()==hwndDlg)
      {

      }
    return 0;
    case WM_TIMER:
      if (wParam == 1)
      {
        int x;
        for (x = 0; x < 2; x ++)
        {
          double p[2] = { VAL2DB(m_peakposdbl[x][0]), VAL2DB(m_peakposdbl[x][1]) };
          m_peakposdbl[x][0]*=0.80;
          m_peakposdbl[x][1]*=0.80;
          SendDlgItemMessage(hwndDlg, x ? IDC_OUTVU : IDC_INVU, WM_USER+1011, 0, (LPARAM)p);
        }

        if (m_slider_vischanged)
        {
          m_slider_vischanged=0;
          DoUpdate(hwndDlg);
          SendMessage(hwndDlg, WM_SIZE, 0, 0);
        }

        if (m_lice_state && !m_in_gfx)
        {
          m_in_gfx++;
          if (!gfx_hasCode())
          {
            if (m_lice_state->hwnd_standalone)
              DestroyWindow(m_lice_state->hwnd_standalone);
          }
          else
          {
            eel_lice_state *ps = m_lice_state;
            bool doshow=false;
            if (!ps->hwnd_standalone)
            {
              HWND h=ps->create_wnd(hwndDlg,1);
              RECT r;
              GetClientRect(hwndDlg,&r);
              if (wantInputMetering())
              {
                RECT t;
                GetWindowRect(GetDlgItem(hwndDlg,IDC_INVU),&t);
                ScreenToClient(hwndDlg,(LPPOINT)&t + 1);
                r.left = t.right + 2;
              }
              if (wantOutputMetering())
              {
                RECT t;
                GetWindowRect(GetDlgItem(hwndDlg,IDC_OUTVU),&t);
                ScreenToClient(hwndDlg,(LPPOINT)&t);
                r.right = t.left - 2;
              }
              r.top+=m_wnd_bottomofcontrols;
              if (h)
              {
                SetWindowPos(h,NULL,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);
                SetWindowLong(h,GWL_ID,GFX_ID);
              }
              doshow=true;
            }
            if (ps->hwnd_standalone)
            {
              RECT r;
              GetClientRect(ps->hwnd_standalone,&r);

              if (m_need_init)
              {
                m_mutex.Enter();
                m_init_mutex.Enter();
                if (m_need_init) on_slider_change();
                m_mutex.Leave();
              }
              else
              {
                m_init_mutex.Enter();
              }

              int dr = ps->setup_frame(ps->hwnd_standalone,r);
              if (dr>=0)
              {
                gfx_runCode(0);
                if (ps->hwnd_standalone && ps->m_framebuffer_dirty)
                {
                  InvalidateRect(ps->hwnd_standalone,NULL,FALSE);
                  m_embed_refstate |= 1;
                }
                if (doshow) ShowWindow(ps->hwnd_standalone,SW_SHOWNA);
              }
              m_init_mutex.Leave();
            }
          }
          m_in_gfx--;
        }
      }
      else if (wParam == 32)
      {
        if (m_description_dirty)
        {
          char buf[512];
          m_string_sermutex.Enter();
          m_description_dirty=false;
          lstrcpyn_safe(buf,m_description.Get(),sizeof(buf));
          m_string_sermutex.Leave();
          SetDlgItemText(hwndDlg,IDC_EFFECTNAME,buf);
        }

        {
          if (m_hostnch != m_last_ionch)
          {
            HWND io = GetDlgItem(hwndDlg,IDC_IO);
            m_last_ionch = m_hostnch;
            if (io) UpdateIOButton(io,m_last_ionch,this);
          }


          for (int x = 0; x < m_sliders.GetSize(); x ++)
          {
            effectSlider *slider = m_sliders.Get(x);
            if (!slider) continue;

            int isCb=slider->sliderValueXlateList.GetSize() || slider->is_file_slider;

            if (isCb)
            {
              HWND combo = slider->ui.combo;
              if (combo && slider->slider)
              {
                int a=(int) (slider->slider[0]+0.1);
                const int oa=SendMessage(combo,CB_GETCURSEL,0,0);
                if (a != oa)
                {
#ifdef _WIN32
                  if (!SendMessage(combo,CB_GETDROPPEDSTATE,0,0))
#endif
                    SendMessage(combo,CB_SETCURSEL,a,0);
                }
              }
            }
            else
            {
              HWND slid = slider->ui.slider;
              HWND edit = slider->ui.edit;
              if ((!slid || GetCapture() != slid) && (!edit || GetFocus() != edit))
              {
                char obuf[512];
                char buf[512];
                if (slid)
                {
                  int oa = SendMessage(slid, TBM_GETPOS, 0, 0);
                  int a = WDL_NORMALLY(slider->slider) ? slider->scaleToUI(slider->slider[0]) : 0;
                  if (a != oa) SendMessage(slid,TBM_SETPOS,TRUE,a);
                }
                if (edit||slid)
                {
                  buf[0] = 0;
                  getSliderText(x,buf,sizeof(buf));
                  if (edit)
                  {
                    obuf[0] = 0;
                    GetWindowText(edit, obuf, sizeof(obuf));
                    if (strcmp(buf, obuf))
                    {
                      SetWindowText(edit,buf);
                    }
                  }
                  if (slid) SendMessage(slid,WM_USER+9999,2,(LPARAM)buf);
                }
              }
            }

          }
        }
      }
    return 0;
    case WM_DESTROY:
      s_configWindows.DeletePtr(hwndDlg);
      resizer.init(NULL);
      m_hwnd=0;
      {
        for (int x = 0; x < m_sliders.GetSize(); x ++)
        {
          effectSlider *slid = m_sliders.Get(x);
          if (slid) memset(&slid->ui,0,sizeof(slid->ui));
        }
      }
      SendMessage(GetParent(hwndDlg),WM_USER+1030,0,0); // notify parent to close any I/O

      if (SetWindowAccessibilityString)
        SetWindowAccessibilityString(GetDlgItem(hwndDlg,IDC_BUTTON1),NULL,0);
    return 0;
#ifdef _WIN32
    case WM_ERASEBKGND: return 1;
    case WM_PAINT:
      Dlg_DrawChildWindowBorders(hwndDlg,NULL,0,NULL);
    return 0;
#endif
    case WM_SIZE:
      if (wParam != SIZE_MINIMIZED)
      {
        RECT cr;
        GetClientRect(hwndDlg,&cr);

        const int cur_dpi = _GetWindowDPIScalingForDialog(hwndDlg);
        if (m_last_dpi != cur_dpi)
        {
          DoUpdate(hwndDlg);
        }

        RECT tr;
        GetClientRect(GetDlgItem(hwndDlg,IDC_INVU),&tr);

        RECT rrr = resizer_initrect_real;
        rrr.right = resizer.sizer_to_dpi(rrr.right);
        if (wantInputMetering()) cr.left += tr.right+2;
        cr.right -= 2;
        if (wantOutputMetering())
        {
          cr.right -= tr.right+6;
        }
        else
        {
          rrr.right -= tr.right + 4;
        }

        resizer.set_margins(wantInputMetering() ?  0 : -resizer.dpi_to_sizer(tr.right), 0,0,0);
        rrr.right = resizer.dpi_to_sizer(rrr.right);
        resizer.set_orig_rect(&rrr);

        resizer.onResize();

        if (m_lice_state && m_lice_state->hwnd_standalone)
        {
          RECT r;
          GetClientRect(hwndDlg,&r);

          if (wantInputMetering())
          {
            RECT t;
            GetWindowRect(GetDlgItem(hwndDlg,IDC_INVU),&t);
            ScreenToClient(hwndDlg,(LPPOINT)&t + 1);
            r.left = t.right + 2;
          }
          if (wantOutputMetering())
          {
            RECT t;
            GetWindowRect(GetDlgItem(hwndDlg,IDC_OUTVU),&t);
            ScreenToClient(hwndDlg,(LPPOINT)&t);
            r.right = t.left - 2;
          }

          r.top+=m_wnd_bottomofcontrols;

          SetWindowPos(m_lice_state->hwnd_standalone,NULL,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);
        }

        // resize our slidercontrols
        int n= 128+NUM_SLIDERS*SLIDER_ID_WIDTH;
        HWND h=GetWindow(hwndDlg,GW_CHILD);
        cr.right -= 2;
        while (h && n--)
        {
          int idx = GetWindowLong(h,GWL_ID);
          if (idx>=SLIDER_ID_BASE && idx<SLIDER_ID_BASE+NUM_SLIDERS*SLIDER_ID_WIDTH)
          {
            RECT r;
            GetWindowRect(h,&r);
            ScreenToClient(hwndDlg,(LPPOINT)&r);
            ScreenToClient(hwndDlg,((LPPOINT)&r)+1);
            int a=(idx-SLIDER_ID_BASE)%SLIDER_ID_WIDTH;
            switch (a)
            {
              case 0: // label
                SetWindowPos(h,NULL,cr.left,r.top,SLIDER_LABEL_SIZE(cr.right-cr.left),r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);
              break;
              case 1: // slider
                {
                  int xpos=cr.left + SLIDER_LABEL_SIZE(cr.right-cr.left) + LABEL_RIGHT_PAD;
                  SetWindowPos(h,NULL, xpos,r.top,cr.right - EDIT_SIZE(cr.right-cr.left) - EDIT_PAD - xpos,r.bottom-r.top,SWP_NOACTIVATE|SWP_NOZORDER);
                }
              break;
              case 2: // edit
                SetWindowPos(h,NULL,cr.right - EDIT_SIZE(cr.right-cr.left),r.top,EDIT_SIZE(cr.right-cr.left),r.bottom-r.top,SWP_NOACTIVATE|SWP_NOZORDER);
              break;
              case 3: // combo
                SetWindowPos(h,NULL,cr.left+SLIDER_LABEL_SIZE(cr.right-cr.left) + LABEL_RIGHT_PAD,r.top,COMBO_SIZE(cr.right-cr.left),r.bottom-r.top,SWP_NOACTIVATE|SWP_NOZORDER);
              break;
            }
          }
          h=GetWindow(h,GW_HWNDNEXT);
        }

        InvalidateRect(hwndDlg,NULL,TRUE);
      }
    return 0;
  }
  return 0;
}

WDL_DLGRET SX_Instance::_dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == WM_INITDIALOG) SetWindowLongPtr(hwndDlg,GWLP_USERDATA,lParam);
  SX_Instance *_this = (SX_Instance *)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
  return _this->ConfigDlgProc(hwndDlg,uMsg,wParam,lParam);
}

void sx_deleteUI(SX_Instance *sx)
{
  if (sx && sx->m_hwnd)
  {
    DestroyWindow(sx->m_hwnd);
  }
}


HWND sx_createUI(SX_Instance *sx, HINSTANCE hDllInstance, HWND hwndParent, void *hostpostparam)
{
  if (!sx) return 0;
  if (sx->m_hwnd) sx_deleteUI(sx);
  WDL_ASSERT(hostpostparam == sx->m_hostctx);
  return CreateDialogParam(hDllInstance,MAKEINTRESOURCE(IDD_SXUI),hwndParent,SX_Instance::_dlgProc,(LPARAM)sx);
}

HWND sx_getUIwnd(SX_Instance *sx)
{
  return sx ? sx->m_hwnd : NULL;
}

void sx_setCurPresetName(SX_Instance *sx, const char *name)
{
  if (sx && name) sx->m_curpresetname.Set(name);
}

bool sx_getCurPresetName(SX_Instance *sx, char* name, int maxlen)
{
  if (!sx) return false;
  return sx->getCurPresetName(name, maxlen);
}

bool sx_loadPreset(SX_Instance *sx, const char* name)
{
  if (!sx) return false;
  return sx->loadPreset(name);
}

class LICE_IBitmap_if
{
public:
  virtual ~LICE_IBitmap_if() { }

  virtual LICE_pixel *getBits()=0;
  virtual int getWidth()=0;
  virtual int getHeight()=0;
  virtual int getRowSpan()=0; // includes any off-bitmap data
  virtual bool isFlipped() { return false;  }
  virtual bool resize(int w, int h)=0;

  virtual HDC getDC() { return 0; } // only sysbitmaps have to implement this


  virtual INT_PTR Extended(int id, void* data) { return 0; }
};

INT_PTR sx_inlineEditor(SX_Instance *sx, int msg, void *p1, void *p2)
{
  if (!sx || !sx->gfx_hasCode()) return 0;

  switch (msg)
  {
    case 0: return 1;
    case WM_LBUTTONDOWN:
      if (sx->m_hwnd && IsWindowVisible(sx->m_hwnd))
        SetForegroundWindow(sx->m_hwnd);
    return 0;
    case WM_GETMINMAXINFO:
      if (p2)
      {
        MINMAXINFO *mmi = (MINMAXINFO*)p2;
        if (sx->m_gfx_reqw > 0 && sx->m_gfx_reqh > 0)
          mmi->ptReserved.x = (sx->m_gfx_reqw << 16) / sx->m_gfx_reqh;
      }
    return 1;
    case REAPER_FXEMBED_WM_MOUSEWHEEL:
    {
      REAPER_FXEMBED_DrawInfo *inf = (REAPER_FXEMBED_DrawInfo *)p2;
      eel_lice_state *ps = sx->m_lice_state;
      if (ps && ps->m_mouse_wheel && inf)
      {
        *ps->m_mouse_wheel += inf->mousewheel_amt;
        return 1;
      }
    }
    return 0;
    case WM_PAINT:
    {
      LICE_IBitmap *bm = (LICE_IBitmap *)p1;
      REAPER_FXEMBED_DrawInfo *inf = (REAPER_FXEMBED_DrawInfo *)p2;
      eel_lice_state *ps = sx->m_lice_state;
      const int inf_flags = inf->flags;
      if (sx->m_hwnd || !ps || sx->m_in_gfx)
      {
        if (inf_flags&1)
        {
          if (!(sx->m_embed_refstate & 1) && (sx->m_embed_refstate&2)) return 0; // todo: handle updating intelligently
        }

        sx->m_embed_refstate|=2;
        sx->m_embed_refstate &= ~1;

        float alpha = 1.0;
        if (ps && ps->m_framebuffer)
        {
          alpha=0.5;
          int usewidth = inf->width, useheight = inf->height;
#ifdef __APPLE__
         if (ps->m_gfx_ext_retina && *ps->m_gfx_ext_retina >= 2.0)
         {
           usewidth *= 2;
           useheight *= 2;
         }
#endif
          LICE_ScaledBlit(bm,ps->m_framebuffer,0,0,inf->width,inf->height,
                                               0,0,
                                               wdl_min(usewidth,LICE__GetWidth(ps->m_framebuffer)),
                                               wdl_min(useheight,LICE__GetHeight(ps->m_framebuffer)),
                                               1.0f,LICE_BLIT_MODE_COPY);
        }
        else if (!sx->m_hwnd) return 1;

        LICE_FillRect(bm,0,0,LICE__GetWidth(bm),LICE__GetHeight(bm),LICE_RGBA(128,128,128,0),alpha,LICE_BLIT_MODE_COPY);

        const char *str = "PLUG-IN UI OPEN";
        int x1 = inf->width/2 - strlen(str)*8/2;
        int y1 = inf->height/2 - 4;
        if (x1<0)
        {
          str="PLUG-IN\nUI OPEN";
          x1 = inf->width/2 - 7*8/2;
          y1-=4;
        }
        LICE_DrawText(bm,wdl_max(x1,0),wdl_max(y1,0), str,LICE_RGBA(255,255,255,0),1.0f,LICE_BLIT_MODE_COPY);

        return 1;
      }

      sx->m_in_gfx++;

      if (sx->m_need_init)
      {
        sx->m_mutex.Enter();
        sx->m_init_mutex.Enter();
        if (sx->m_need_init) sx->on_slider_change();
        sx->m_mutex.Leave();
      }
      else
      {
        sx->m_init_mutex.Enter();
      }

      ps->m_has_cap = inf_flags&0x70000;
      RECT r = { 0,0, inf->width, inf->height };
      int has_dpi = (int) ((LICE_IBitmap_if*)bm)->Extended(LICE_EXT_GET_ADVISORY_SCALING,NULL);
      if (has_dpi<1) has_dpi=256;

      int dr = ps->setup_frame(NULL,r, inf->mouse_x,inf->mouse_y, has_dpi);
      if (dr>=0)
      {
        sx->m_gfx_last_inline_runtime = GetTickCount();
        sx->gfx_runCode(1);
        if (!(inf_flags&1) || ps->m_framebuffer_dirty || (sx->m_embed_refstate&2))
        {
          LICE_ScaledBlit(bm,ps->m_framebuffer,0,0,inf->width,inf->height,
                                               0,0,inf->width,inf->height,1.0f,LICE_BLIT_MODE_COPY);
          dr = 1;
        }
      }
      sx->m_embed_refstate = 0;
      ps->m_has_cap = 0;
      sx->m_init_mutex.Leave();
      sx->m_in_gfx--;

      return dr>0 ? 1 : 0;
    }
    return 0;
  }
  return 0;
}

double sx_getTailSize(SX_Instance *sx)
{
  if (!sx || !sx->m_ext_tail_size) return 0.0;
  return *sx->m_ext_tail_size;
}

void SX_Instance::calcPeaks(double *buf, int cnt, int nch, int w)
{
  double mv[2]={0,};
  if (nch>1)
  {
    while (cnt--)
    {
      double v1=fabs(buf[0]);
      double v2=fabs(buf[1]);
      if (v1 > mv[0]) mv[0]=v1;
      if (v2 > mv[1]) mv[1]=v2;
      buf+=nch;
    }
  }
  else if (nch==1)
  {
    while (cnt--)
    {
      double v=fabs(*buf);
      buf++;
      if (v > mv[0]) mv[0]=v;
    }
    mv[1]=mv[0];
  }
  if (mv[0] > m_peakposdbl[w][0])m_peakposdbl[w][0]=mv[0];
  if (mv[1] > m_peakposdbl[w][1])m_peakposdbl[w][1]=mv[1];
}

int sx_processSamples(SX_Instance *sx, double *buf, int cnt, int nch, int srate,
                      double tempo, int tsnum, int tsdenom,
                      double playstate, double playpos, double playpos_b,
                      double lastwet, double wet, int flags)
{
  if (!sx) return 0;

  if (sx->m_hwnd && sx->wantInputMetering()) sx->calcPeaks(buf,cnt,nch,0);

  const int rv = sx->process_samples(buf,cnt,nch,srate,
                                     tempo,tsnum,tsdenom,
                                     playstate,playpos,playpos_b,
                                     lastwet,wet,flags);

  return rv;
}


static WDL_Mutex sfx_mutex;

void NSEEL_HOSTSTUB_EnterMutex()
{
  if (_NSEEL_HOSTSTUB_EnterMutex) _NSEEL_HOSTSTUB_EnterMutex();
  else sfx_mutex.Enter();
}

void NSEEL_HOSTSTUB_LeaveMutex()
{
  if (_NSEEL_HOSTSTUB_LeaveMutex) _NSEEL_HOSTSTUB_LeaveMutex();
  else sfx_mutex.Leave();
}
