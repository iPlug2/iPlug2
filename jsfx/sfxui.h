/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 */

#ifndef _JS_SFXUI_H_
#define _JS_SFXUI_H_

#ifdef _WIN32
#include <windows.h>
#else
#include "../WDL/swell/swell.h"
#endif

#include "../WDL/wingui/wndsize.h"
#include "../WDL/mutex.h"
#include "../WDL/fastqueue.h"
#include "../WDL/wdlcstring.h"
#include "../WDL/wdlstring.h"
#include "../WDL/ptrlist.h"
#include "../WDL/queue.h"
#include "../WDL/assocarray.h"
#include "../WDL/delay_line.h"

#include "../WDL/win32_utf8.h"
#include "../WDL/win32_curses/curses.h"

#include "../WDL/eel2/ns-eel-int.h" // for internals of VM

#include "miscfunc.h"

extern HINSTANCE g_hInst;
extern const char *g_config_slider_classname; // must implement this somewhere (jsfx_api.cpp does, other implementations should set it)


#define NUM_SLIDERS 256
#define MAX_EFFECT_FILES 20
#define MAX_NCH REAPER_MAX_CHANNELS



class effectSlider // jsfx parameter record
{
public:
  effectSlider();
  ~effectSlider();

  EEL_F default_val;
  EEL_F range_min,range_max,range_scale;

  int scaleToUI(double cv) const;
  double scaleFromUI(int pos) const;

  double scaleToExternal(double cv) const; // rescales from [range_min..range_max] if external_shaping, otherwise passthrough
  double scaleFromExternal(double cv) const; // rescales to [range_min..range_max] if external_shaping, otherwise passthrough

  double _sc_inv(double v) const;
  double _sc(double v) const;

  double ui_shape_parm, ui_shape_parm2;
  double ui_lcstart, ui_lcend; // if ui_is_shape, _sc(range_m[in,ax])

  int ui_step_count;
  enum {
    SHAPE_LINEAR=0,
    SHAPE_LOG_PLAIN,
    SHAPE_POW,
    SHAPE_LOG_MIDPOINT,
  };
  int ui_is_shape; // SHAPE_*
  bool external_shaping; // implies ui_is_shape is not SHAPE_LINEAR

  WDL_FastString name;
  EEL_F *slider;
  int scrpos_x,scrpos_y; // last updated x,y on screen, or 0,0 if not on screen
  int show; // visiblity

  WDL_IntKeyedArray<double> parmchg_q;
  int parmchg_q_curpos;

  WDL_PtrList<char> sliderValueXlateList;

  int is_file_slider;
  const char *currentFile;//cached pointer to FileList[(int)*slider]
  WDL_FastString FileDirectory;
  WDL_PtrList<char> FileList;
  bool file_dirty;

  struct {
    HWND combo,label,slider,edit;
  } ui;
};

class effectConfigItem // compile-time config for JSFX, e.g. config: lines in the JSFX, used by the preprocessor
{
  public:
    effectConfigItem(const char *tok, int toklen) : cur_val(0.0), def_val(0.0), prev_val(2.0), preserve_config(false)
    {
      symbol.Set(tok,toklen);
    }
    ~effectConfigItem() { values.Empty(true); }
    WDL_FastString symbol;
    WDL_FastString desc;
    double cur_val, def_val, prev_val;
    bool preserve_config; // if set, changing configitem preserves parameters and serialized state

    struct rec {
      double v;
      WDL_FastString name;
    };
    WDL_PtrList<rec> values;
};

int get_slider_from_name(const char *str, char endc, int *outlen=NULL); // parses "sliderX" and makes sure it ends with endc

class eel_string_context_state;
class eel_lice_state;


class SX_Editor;

class SX_Instance
{
  public:
    SX_Instance();

    ~SX_Instance();

    WDL_FastString m_name;
    WDL_Mutex m_mutex;

    void DoUpdate(HWND hwndDlg);
    void DoInitDialog(HWND hwndDlg);

    enum {
      LOADEFFECT_RESETFLAG_VARS=1, // forces re-run of @init, closing of files, clears vars, etc
      LOADEFFECT_RESETFLAG_CONFIGITEMS=2, // resets config items to stock
      LOADEFFECT_RESETFLAG_SLIDERS=4, // resets sliders to stock
    };
    void DoRecompile(HWND hwndDlg, int resetFlags, WDL_FastString *strout=NULL); // resetFlags=LOADEFFECT_RESETFLAG_VARS etc
    void OnCommand(HWND hwndDlg, int command, int code, int lParam);
    void DoScroll(HWND hwndDlg, int x, int code);

    WDL_DLGRET ConfigDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static WDL_DLGRET _dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    double m_peakposdbl[2][2] WDL_FIXALIGN;
    HWND m_hwnd;

    WDL_WndSizer resizer;
    RECT resizer_initrect_real;
    int m_wnd_bottomofcontrols;

    void calcPeaks(double *buf, int cnt, int nch, int w);

  // IDE watch
    void OpenExternal(HWND hwndDlg);
    WDL_DLGRET WatchDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static WDL_DLGRET _watchDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND m_hwndwatch;
    WDL_WndSizer m_watchresize;

    #define NUM_WATCHLIST_COLS 3
    WDL_PtrList<char> m_watch_cache; // listview last values*NUM_WATCHLIST_COLS, can be NULL for empty string

    win32CursesCtx m_editor_curses;
    WDL_PtrList<SX_Editor> m_editor_list;
    int m_editor_cur;
    void switchToEditor(int which);

    struct watchNameValue
    {
      watchNameValue() { name[0]=0; }
      ~watchNameValue() {}
      char name[200];
      WDL_FastString str_val; // only used if a string (name[0] == '#')
      double val;
      int refcnt;
      int isreg;
      void *valptr; // useful for debugging
    };
    enum { WATCH_ISREG_NORMAL=0, WATCH_ISREG_NAMEDSTRING, WATCH_ISREG_STRINGINDEX, WATCH_ISREG_SYSTEM, WATCH_ISREG_GLOBAL };
    void SetNameValPair(int idx, const char *name, double val, int rc, int isreg, void *valptr, const char *str_val)
    {
      watchNameValue *p=m_watchlist.Get(idx);
      if (!p) m_watchlist.Add(p = new watchNameValue);
      if (p)
      {
        if (isreg==WATCH_ISREG_NAMEDSTRING)
        {
          snprintf(p->name,sizeof(p->name),"#%s",name);
        }
        else
          lstrcpyn_safe(p->name,name,sizeof(p->name));

        if (str_val || p->str_val.GetLength()) p->str_val.Set(str_val?str_val:"");
        p->val = val;
        p->refcnt=rc;
        p->isreg = isreg;
        p->valptr = valptr;
      }
    }

    WDL_PtrList<watchNameValue> m_watchlist;
    static int watchlistsortfunc(const void *a, const void *b)
    {
      watchNameValue *aa = *(watchNameValue **)a;
      watchNameValue *bb = *(watchNameValue **)b;
      int d = aa->isreg - bb->isreg;
      if (d) return d;
      d = (aa->name[0] == '#') - (bb->name[0] == '#'); // named strings last
      if (d) return d;
      return WDL_strcmp_logical_ex(aa->name,bb->name,0,WDL_STRCMP_LOGICAL_EX_FLAG_OLDSORT);
    }

  // general config

    WDL_FastString m_datadir, m_effectdir;

    WDL_FastString m_last_cfg;

    int m_trig;
    int m_misc_flags; // &1=instrument

    int m_last_ionch,m_hostnch;

    int parmToIndex(int parm) const
    {
      int cnt=0;
      for (int x = 0; x < m_sliders.GetSize(); x ++)
      {
        effectSlider *slid = m_sliders.Get(x);
        if (slid && cnt++ == parm) return x;
      }
      return -1;
    }

    int indexToParm(int idx) const
    {
      int cnt=0;
      for (int x = 0; x < m_sliders.GetSize(); x ++)
      {
        effectSlider *slid = m_sliders.Get(x);
        if (slid)
        {
          if (x == idx) return cnt;
          cnt++;
        }
      }
      return -1;
    }

    const char *SaveSerState(int *buflen);
    void LoadSerState(const char *buf, int buflen);

    bool getCurPresetName(char* name, int maxlen);
    bool loadPreset(const char* name);

    int m_preset_reent;
    WDL_FastString m_curpresetname;

    int m_embed_refstate; // 1=mainui had refreshed, 2=mainui was visible (need ref if not mainui)
    int m_last_dpi;

    // if re-compiling hwnd_for_destroy must be set, and those HWNDs should be destroyed
    bool LoadEffect(const char *fn, int resetFlags, WDL_PtrList<void> *hwnd_for_destroy); // resetFlags = LOADEFFECT_RESETFLAG_VARS etc

    char m_last_error[256];

    void *midi_ctxdata;
    EEL_F (*midi_sendrecv)(void *ctx, int action, EEL_F *ts, EEL_F *msg1, EEL_F *msg23, EEL_F *midi_bus);

    void *m_hostctx;
    void (*m_slider_automate)(void *ctx, int parmidx, bool done);

    static EEL_F NSEEL_CGEN_CALL _bypass_set(SX_Instance *pthis, EEL_F *item, EEL_F *val);
    static EEL_F NSEEL_CGEN_CALL _bypass_get(SX_Instance *pthis, EEL_F *item);
    static EEL_F NSEEL_CGEN_CALL _slider_set(SX_Instance *pthis, EEL_F *item, EEL_F *val);
    static EEL_F NSEEL_CGEN_CALL _slider_get(SX_Instance *pthis, EEL_F *item);

  ////////

    static int eel_init_refcnt;
    void eel_init();
    void eel_quit();

    void set_sample_fmt(int srate, int nch);
    void clear_code(bool fullReset=false);
    const char *add_imported_code(const char *code, int lineoffs, WDL_UINT64 *h);
    const char *set_init_code(const char *code, int lineoffs, WDL_UINT64 h);  // NULL on success, error string on error
    const char *set_slider_code(const char *code, int lineoffs);  // NULL on success, error string on error
    const char *set_onblock_code(const char *code, int lineoffs);  // NULL on success, error string on error
    const char *set_sample_code(const char *code, int lineoffs);  // NULL on success, error string on error

    char m_init_err, m_slider_err, m_onblock_err, m_serialize_err, m_sample_err, m_gfx_err;

    EEL_F peek_effect_value(const char *name, bool *suc=NULL);

    const char *set_serialize_code(const char *code, int lineoffs);
    const char *set_gfx_code(const char *code, int lineoffs);
    int load_serialized_state(char *filename);
    int save_serialized_state(char *filename);

    void save_serialized_state_tomem(WDL_HeapBuf *hb);
    void load_serialized_state_frommem(WDL_HeapBuf *hb); // loads immediately!


    int *get_init_code_stats();
    int *get_slider_code_stats();
    int *get_onblock_code_stats();
    int *get_sample_code_stats();
    int *get_gfx_code_stats();

    int process_samples(EEL_F *work, int num_samples, int nch, int srate,
      EEL_F tempo, int tsnum, int tsdenom,
      EEL_F playstate, EEL_F playpos, EEL_F playpos_b,
      double lastwet, double newwet, int flags);


    // makes sure init code has been called
    // then: if no parameters:
    //   - calls exec_slider_stuff(), NSEEL_VM_freeRAMIfCodeRequested
    // otherwise if a parameter:
    //   - matches the best file for "newv" of the file slider
    void on_slider_change(int set_file_index=-1, const char *newv=NULL);

    int m_last_slider_sel;

    WDL_FastString m_fn,  // relative path
                   m_full_actual_fn_used,
                   m_description; // writeable by thread, use m_string_sermutex
    bool m_description_dirty; // if written by #dbg_desc

    void getSliderText(int s, char *buf, int buflen, EEL_F* vin=0);
    int sliderIsEnum(int s);  // >0 if param is selected from a dropdown

    WDL_PtrList<effectSlider> m_sliders; // non-present sliders will be NULL

    int find_slider_by_value_ptr(EEL_F *ptr) const
    {
      for (int i = 0; i < m_sliders.GetSize(); ++i)
      {
        const effectSlider *slid = m_sliders.Get(i);
        if (slid && ptr == slid->slider) return i;
      }
      return -1;
    }
    bool m_slider_anychanged; // used for sfxui
    bool m_slider_vischanged;
    int m_need_init; // &2 = full, &1 = slider only

    // file serialization context code

    WDL_PtrList<char> m_localfilenames;
    fileHandleRec m_filehandles[MAX_EFFECT_FILES+1]; // index 0 is the serialize handle

    void resetVarsToStock(bool wantVmVars, bool wantSliders=true);

    EEL_F *GetNamedVar(const char *s, bool createIfNotExists, EEL_F *altOut); // override for thread-check
    const char *GetStringForIndex(EEL_F idx, WDL_FastString **p=NULL, bool is_for_write=false);
    const char *EnumerateNamedStrings(int idx, const char **name);
    const char *GetNamedStringValue(const char *name, int *indexOut);
    void update_var_names(); // calls m_eel_string_state->update_named_vars()

    eel_string_context_state *m_eel_string_state;

    eel_lice_state *m_lice_state;

    unsigned int m_main_thread;

    bool GetFilenameForParameter(EEL_F val, WDL_FastString *fsWithDir, int isWrite); // returns true if valid, and update fsWithDir

    NSEEL_CODEHANDLE doCompileCode(const char *code, int lineoffs, int flags);

    NSEEL_VMCTX m_vm;

    int GetDelaySamples() const { return m_pdc_lastdelay; }
    bool WantMidiNoDelay() const { return (m_pdc_midiearly && *m_pdc_midiearly > 0.5); }

    WDL_DelayLine<double> m_pdc_queue; // dry samples
    int m_pdc_lastdelay;
    WDL_TypedBuf<double> m_drybuf; // used for wet/dry when PDC is not in use (or when PDC is in use for all pins)

    EEL_F m_spls[MAX_NCH] WDL_FIXALIGN;

    EEL_F *m_pdc_amt, *m_pdc_topch, *m_pdc_botch, *m_pdc_midiearly;

    EEL_F *m_samplesblock;
    EEL_F *m_tsnum;
    EEL_F *m_tsdenom;
    EEL_F *m_srate;
    EEL_F *m_trigger;
    EEL_F *m_num_ch;
    EEL_F *m_tempo,*m_playpos_s, *m_playpos_b, *m_playstate;
    EEL_F *m_extnoinit, *m_extnodenorm, *m_extwantmidibus, *m_midibus;
    EEL_F *m_ext_tail_size;
    EEL_F *m_gfx_ext_flags;
    EEL_F *m_ext_gr_meter;

    void gfx_notify_general_statechange();

    bool gfx_hasCode() const { return m_gfx_ch && m_gfx_want_idle != 2; }
    void gfx_runCode(int ext_flags);

    void run_idle(); // does nothing if idle not supported
    void add_idle(); // adds to global idle list
    void remove_idle(); // removes from global idle list

    void DoImageLoads(bool fullReset);

    WDL_Mutex m_init_mutex, m_string_sermutex;

    int m_gfx_reqw, m_gfx_reqh;
    int m_gfx_hz;

    int m_last_nch, m_last_srate;
    int m_uses_triggers;


    bool wantInputMetering() const { return !m_has_no_inputs && !m_no_meters; }
    bool wantOutputMetering() const { return !m_has_no_outputs && !m_no_meters; }

    bool m_has_no_inputs;
    bool m_has_no_outputs;

    bool m_want_all_kb,m_no_meters;
    WDL_FastString m_gmem_name;
    void set_gmem_name(const char *name);

    int m_in_gfx; // currently calling gfx code from main thread, avoid doing certain things to graphics code/vm if this is set
    int m_gfx_want_idle; // 1=run idle, 2=run idle only
    DWORD m_gfx_last_inline_runtime;

    bool is_in_gfx() const { return m_in_gfx && GetCurrentThreadId() == m_main_thread; }

    bool bm_enabled;
    WDL_INT64 bm_cnt WDL_FIXALIGN;
    double bm_starttime, bm_usetime, bm_lastendtime;

    WDL_PtrList<char> m_in_pinnames,m_out_pinnames;

    WDL_PtrList<effectConfigItem> m_config_items;

    WDL_UINT64 m_initcodehash, m_slidercodehash;

    void exec_init_code(); // executes @init and common imported code

  private:

    NSEEL_CODEHANDLE m_sample_ch, m_init_ch, m_slider_ch, m_onblock_ch, m_serialize_ch, m_gfx_ch;
    WDL_PtrList<void> m_common_ch;
    char *m_need_loadserialize_fn;

    // updates any dirty file sliders:
    //  - reads the value, rounds it to int, writes it back to the var
    //  - and updates currentFile
    // then executes @slider code
    void exec_slider_stuff();

    // rescans all file slider paths
    void rescan_file_sliders();
    void recurseScanDir(int whichslider, const char *root, const char *sub);
};

void sx_provideAPIFunctionGetter(void *(*getFunc)(const char *name));
SX_Instance *sx_createInstance(const char *dir_root, const char *effect_name, bool *wantWak);
const char **sx_getPinInfo(SX_Instance *ctx, int isOutput, int *numPins);
void sx_set_midi_ctx(SX_Instance *sx, double (*midi_sendrecv)(void *ctx, int action, double *ts, double *msg1, double *msg23, double *midibus), void *midi_ctxdata);
void sx_set_host_ctx(SX_Instance *inst, void* hostctx, void (*slider_automate)(void *ctx, int parmidx, bool done));
const char *sx_getEffectName(SX_Instance *sx);
void sx_loadSerState(SX_Instance *sx, const char *buf, int len);
const char * sx_saveSerState(SX_Instance *sx, int *len);
void sx_loadState(SX_Instance *sx, const char *buf);
const char *sx_saveState(SX_Instance *sx, int *lOut);
int sx_getCurrentLatency(SX_Instance *sx);
const char * sx_getFullFN(SX_Instance *sx);
int sx_processSamples(SX_Instance *sx, double *buf, int cnt, int nch, int srate,
                      double tempo, int tsnum, int tsdenom,
                      double playstate, double playpos, double playpos_b,
                      double lastwet, double wet, int flags);
void sx_deleteUI(SX_Instance *sx);
HWND sx_createUI(SX_Instance *sx, HINSTANCE hDllInstance, HWND hwndParent, void *hostpostparam);
HWND sx_getUIwnd(SX_Instance *sx);
void sx_destroyInstance(SX_Instance *sx);
int sx_getNumParms(SX_Instance *sx);
double sx_getParmVal(SX_Instance *sx, int parm, double *minval, double *maxval, double *step);
void sx_getParmDisplay(SX_Instance *sx, int parm, char* disptxt, int len, double* inval);
void sx_getParmName(SX_Instance *sx, int parm, char *name, int namelen);
void sx_setParmVal(SX_Instance *sx, int parm, double val, int sampleoffs);
int sx_parmIsEnum(SX_Instance *sx, int parm);
bool sx_hasParmChanged(SX_Instance *sx);
void sx_setCurPresetName(SX_Instance *sx, const char *name);
bool sx_getCurPresetName(SX_Instance *sx, char* name, int maxlen);
bool sx_loadPreset(SX_Instance *sx, const char* name);
void sx_updateHostNch(SX_Instance *sx, int nch);
INT_PTR sx_inlineEditor(SX_Instance *sx, int msg, void *p1, void *p2);
double sx_getTailSize(SX_Instance *sx);
bool sx_getParmValueFromString(SX_Instance *, int parm, const char *buf, double *valptr);
INT_PTR sx_extended(SX_Instance *, int msg, void *parm1, void *parm2);

// loaded APIs
extern bool (*IsMediaExtension)(const char *extparm, bool wantOthers);
extern void (*FreeHeapPtr)(void*);
extern bool (*fxPresetPromptForAction)(HWND hwndDlg, void *parentid, int action, char *buf, int bufsz); // action=0 save, 1=save def, 2=rename
extern bool *(*DoFxLastTweakParmCtxMenu)(void* fxdsp, HWND h, int, int, const char*);
extern void *(*fxSetBypass)(void*, bool);
extern void *(*fxSetWet)(void*, double val, bool done);
extern bool (*fxGetSetDeltaSolo)(void *ctx, bool *delta_solo);
extern bool (*fxLoadReaperPreset)(void*, const char*);
extern int (*fxGetPlacement)(void *fxdsp, int *chain_pos, int *flags); // returns track index, -1 for master, -2 for hwout. flags will have 1 set if takeFX, 2 if record input, 4 if in inactive project
extern int (*fxGetReaperPresetNamesRaw)(void*, char***);
extern void (*fxRenameReaperPreset)(void*, const char*, const char*);
extern int (*fxDoReaperPresetAction)(void*, const char*, int);
extern bool (*fxImportExportRPL)(void*, HWND, bool);
extern bool (*fxGetSetWantAllKeys)(void*, bool*);
extern const char * (*get_ini_file)();
extern HWND (*GetMainHwnd)();
extern void (*fxSetUndoPoint)(void*);

extern int (*fxGetSetHostNumChan)(void *fxdsp, int *set_numchan);
extern int (*fxGetSetPinMap2)(void *fxdsp, bool isout, unsigned int *mapping, int choffs, int *isset_sz);
extern int (*fxGetSetPinmapperFlags)(void *fxdsp, int *set_flags);

extern const char *(*get_eel_funcdesc)(const char *);

extern PCM_source *(*_PCM_Source_CreateFromFile)(const char *filename);
extern bool (*IsMediaExtension)(const char *extparm, bool wantOthers);
extern bool (*Plugin_FindNewFileName)(ReaProject *__proj, int trackid, const char *trackname, const char *ext, char *buf, int bufsz);
extern PCM_sink *(*PCM_Sink_Create)(const char *filename, const void *cfg, int cfg_l, int nch, int srate, bool buildpeaks);
extern int (*InsertMedia)(const char* file, int mode);
extern const char * (*GetAppVersion)(void);
extern void (*SetEditCurPos)(double time, bool moveview, bool seekplay);
extern double (*GetProjectLength)(ReaProject *__proj);
extern bool (*SetTempoTimeSigMarker)(ReaProject* __proj, int ptidx, double tpos, int mpos, double bpos, double bpm, int tsnum, int tsdenom, bool lineartempo);
extern double (*GetCursorPositionEx)(ReaProject*);
extern int (*FindTempoTimeSigMarker)(ReaProject* __proj, double tpos);
extern bool (*GetTempoTimeSigMarker)(ReaProject* __proj, int ptidx, double* tpos, int* mpos, double* bpos, double* bpm, int* tsnum, int* tsdenom, bool* lineartempo);
extern void (*CSurf_OnTempoChange)(double);
extern double (*Master_GetTempo)();
extern void (*EnsureNotCompletelyOffscreen)(RECT *);
extern int (*GetWindowDPIScalingForDialog)(HWND);
extern void (*SetWindowAccessibilityString)(HWND h, const char *, int mode);
#ifndef _WIN32
extern void (*Mac_MakeDefaultWindowMenu)(HWND);
#endif

extern void (*_NSEEL_HOSTSTUB_EnterMutex)();
extern void (*_NSEEL_HOSTSTUB_LeaveMutex)();
extern void ** (*eel_gmem_attach)(const char *nm, bool is_alloc);
extern int (*plugin_register)(const char *, void *);

// config
extern int *g_config_fontsize_ptr, *g_config_maxsug_ptr, *g_config_editflag_ptr;

EEL_F *sx_effect_var_resolver(void *userctx, const char *name); // userctx is SX_Instance

double flexi_atof(const char *p);


const char *default_get_eel_funcdesc(const char *fn);

#endif
