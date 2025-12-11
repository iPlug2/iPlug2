/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * processing
 */
#include "sfxui.h"
#include "sfx_edit.h"
#include "../WDL/win32_utf8.h"
#include "../WDL/eel2/ns-eel-int.h"
#include "miscfunc.h"
#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../WDL/dirscan.h"
#include "../WDL/lineparse.h"

#ifndef _WIN32
#include <sys/time.h>
#endif

#include "../WDL/fnv64.h"
#include "../WDL/time_precise.h"

#define DENORM 1.0e-16

static int clearVar(const char *name, EEL_F *val, void *ctx)
{
  SX_Instance *proc = (SX_Instance *)ctx;

  if (val == proc->m_srate ||
      val == proc->m_num_ch ||
      val == proc->m_extwantmidibus ||
      val == proc->m_extnoinit) return 1;

  if (proc->find_slider_by_value_ptr(val)>=0) return 1;

  if (val == proc->m_ext_gr_meter) *val = 10000.0;
  else *val=0.0;
  return 1;
}

effectSlider::effectSlider()
{
  file_dirty = false;
  slider=0;
  range_min=range_max=range_scale=0;
  ui_step_count = 1;
  ui_is_shape = SHAPE_LINEAR;
  external_shaping = false;
  ui_shape_parm2 = ui_shape_parm = ui_lcstart = ui_lcend = 0.0;
  show=1;
  scrpos_x=scrpos_y=0;
  is_file_slider=0;
  currentFile=0;
  parmchg_q_curpos=0;
  memset(&ui,0,sizeof(ui));
}

effectSlider::~effectSlider()
{
  int y;
  for (y = 0; y < FileList.GetSize(); y ++)
  {
    free(FileList.Get(y));
  }
  for (y = 0; y < sliderValueXlateList.GetSize(); y ++)
  {
    free(sliderValueXlateList.Get(y));
  }
  FileList.Empty();
  sliderValueXlateList.Empty();
}

#define EEL_WANT_DOCUMENTATION
#include "../WDL/eel2/ns-eel-func-ref.h"
#define EEL_STRING_DBG_DESC 0x7fffff00

#define EEL_STRING_NAMEDSTRINGCALLBACK_HOOK \
    if (name && !stricmp(name,"dbg_desc")) return EEL_STRING_DBG_DESC;

#define EEL_STRING_MUTEXLOCK_SCOPE   WDL_MutexLock __lock(&((SX_Instance*)(opaque))->m_string_sermutex);
#define EEL_STRING_GETNAMEDVAR(x,y,z) ((SX_Instance*)(opaque))->GetNamedVar(x,y,z)
#define EEL_STRING_GET_CONTEXT_POINTER(opaque) (((SX_Instance*)opaque)->m_eel_string_state)
#define EEL_STRING_GET_FOR_INDEX(x, wr) (((SX_Instance *)opaque)->GetStringForIndex(x, wr, false))
#define EEL_STRING_GET_FOR_WRITE(x, wr) (((SX_Instance *)opaque)->GetStringForIndex(x, wr, true))
#include "../WDL/eel2/eel_strings.h"

static WDL_Mutex atomic_mutex;
#define EEL_ATOMIC_SET_SCOPE(opaque)
#define EEL_ATOMIC_ENTER atomic_mutex.Enter()
#define EEL_ATOMIC_LEAVE atomic_mutex.Leave()

#include "../WDL/eel2/eel_atomic.h"

#include "../WDL/eel2/eel_fft.h"
#include "../WDL/eel2/eel_mdct.h"

#define EEL_LICE_GET_FILENAME_FOR_STRING(idx, fs, p) (((SX_Instance*)opaque)->GetFilenameForParameter(idx,fs,p))
#define EEL_LICE_GET_CONTEXT(opaque) (((opaque) && GetCurrentThreadId()==((SX_Instance *)opaque)->m_main_thread) ? (((SX_Instance *)opaque)->m_lice_state) : NULL)

#define EEL_LICE_STANDALONE_NOINITQUIT
#define EEL_LICE_WANT_STANDALONE
#include "../WDL/eel2/eel_lice.h"

#define EEL_MISC_NO_SLEEP
#include "../WDL/eel2/eel_misc.h"

const char *default_get_eel_funcdesc(const char *fn) // this will hopefully get discarded by the linker if unused
{
  const char *tab[] = {
    nseel_builtin_function_reference,
    eel_strings_function_reference,
    eel_misc_function_reference,
    eel_fft_function_reference,
    eel_mdct_function_reference,
    eel_lice_function_reference,
    NULL
  };
  if (!fn || !fn[0]) return NULL;

  const int fnl = (int)strlen(fn);
  for (int x = 0; tab[x]; x ++)
  {
    const char *p = tab[x];
    while (*p)
    {
      if (!strnicmp(fn,p,fnl) && p[fnl]=='\t') return p + fnl + 1;
      p += strlen(p) + 1;
    }
  }

  return NULL;
}


EEL_F NSEEL_CGEN_CALL _midi_send_str(void *opaque, INT_PTR np, EEL_F **parms)
{
  SX_Instance *epctx = (SX_Instance *)opaque;
  if (epctx->midi_sendrecv)
  {
    EEL_STRING_MUTEXLOCK_SCOPE

    WDL_FastString *fs=NULL;
    const unsigned char *s=(const unsigned char *)EEL_STRING_GET_FOR_INDEX(parms[1][0],&fs);
    if (s)
    {
      const int s_len = fs ? fs->GetLength() : (int)strlen((const char*)s);
      unsigned char *buf=NULL;

      if (s_len>0)
      {
        bool do_hdr=false;
        if (s[0] == 0xff)
        {
          // meta message, pass through
        }
        else if (s_len >= 2 && s[0] == 0xF0 && s[s_len-1] == 0xF7)
        {
          // already valid sysex!
          // for (int i=1;i<s_len-1;i++) if (s[i]&0x80) return 0;
        }
        else if (s_len <= 3 && (s[0]&0x80))
        {
          // normal midi message, most likely
          // for (int i=1;i<s_len;i++) if (s[i]&0x80) return 0;
        }
        else
        {
          // for (int i=0;i<s_len;i++) if (s[i]&0x80) return 0;
          do_hdr = true;
        }

        int ilen = s_len;
        if (do_hdr) ilen += 2;

        if (epctx->midi_sendrecv(epctx->midi_ctxdata,0x100,
            parms[0], //timestamp
            (EEL_F *)&ilen, // length
            (EEL_F *)&buf,
            epctx->m_extwantmidibus && *epctx->m_extwantmidibus ? epctx->m_midibus : NULL) > 0 && buf)
        {
          if (do_hdr)
          {
            buf[0]=0xF0;
            buf[ilen - 1] = 0xF7;
            buf++;
            ilen -= 2;
          }
          memcpy(buf,s,s_len);
          return s_len;
        }
      }
    }
  }
  return 0;
}


EEL_F NSEEL_CGEN_CALL _midi_recv_str(void *opaque, INT_PTR np, EEL_F **parms)
{
  SX_Instance *epctx = (SX_Instance *) opaque;
  if (epctx->midi_sendrecv)
  {
    unsigned char *buf=NULL;
    int used_len=(1<<24); // 16MB max
    if (epctx->midi_sendrecv(epctx->midi_ctxdata,0x101,
        parms[0], //timestamp
        (EEL_F *)&used_len, // length
        (EEL_F *)&buf,
        epctx->m_extwantmidibus && *epctx->m_extwantmidibus ? epctx->m_midibus : NULL) > 0 && buf)
    {
      EEL_STRING_MUTEXLOCK_SCOPE

      WDL_FastString *fs=NULL;
      EEL_STRING_GET_FOR_WRITE(parms[1][0],&fs);
      if (fs) fs->SetRaw((const char*)buf,used_len);
      return used_len;
    }
  }
  return 0.0;
}



int SX_Instance::eel_init_refcnt=0;
void SX_Instance::eel_init()
{
  m_eel_string_state = new eel_string_context_state;

  if (eel_init_refcnt++) return;
  NSEEL_init();

  eel_js_register();

  EEL_fft_register();

  EEL_mdct_register();

  EEL_string_register();

  EEL_atomic_register();

  EEL_misc_register();

  NSEEL_addfunc_exparms("midisend_str",2,NSEEL_PProc_THIS,&_midi_send_str);
  NSEEL_addfunc_exparms("midirecv_str",2,NSEEL_PProc_THIS,&_midi_recv_str);

  eel_lice_register();
  eel_lice_register_standalone(g_hInst,"jsfx_gfx",NULL,NULL);
}

NSEEL_CODEHANDLE SX_Instance::doCompileCode(const char *code, int lineoffs, int flags)
{
  return NSEEL_code_compile_ex(m_vm,code,lineoffs,flags);
}

void SX_Instance::eel_quit()
{
  delete m_eel_string_state;
  m_eel_string_state = NULL;
  if (eel_init_refcnt>0)
  {
    if (!--eel_init_refcnt)
    {
      NSEEL_quit();
    }
  }
}


void SX_Instance::resetVarsToStock(bool wantVmVars, bool wantSliders)
{
  if (m_vm)
  {
    if (m_extwantmidibus) *m_extwantmidibus=0;
    if (m_extnoinit) *m_extnoinit=0;
    if (m_extnodenorm) *m_extnodenorm=0;
    if (m_lice_state) m_lice_state->resetVarsToStock();
    if (m_num_ch) *m_num_ch=m_last_nch;
    if (wantSliders) for (int x=0; x < m_sliders.GetSize(); x ++)
    {
      effectSlider *slid = m_sliders.Get(x);
      if (slid && slid->slider) *slid->slider = 1.0;
    }
    if (wantVmVars)
    {
      NSEEL_VM_remove_all_nonreg_vars(m_vm);
      NSEEL_VM_freeRAM(m_vm);
    }
  }
}

struct GRAM_context
{
  GRAM_context() { refcnt=1; ctx=NULL; }
  ~GRAM_context() { NSEEL_VM_FreeGRAM(&ctx); }
  int refcnt;
  void *ctx;
};
static WDL_StringKeyedArray<GRAM_context *> s_gram_contexts(false);
extern void **(*eel_gmem_attach)(const char *name, bool is_alloc);

void SX_Instance::set_gmem_name(const char *name)
{
  if (!stricmp(m_gmem_name.Get(),name)) return;

  if (m_gmem_name.Get()[0])
  {
    // detach from old
    if (m_vm) NSEEL_VM_SetGRAM(m_vm,NULL);

    if (eel_gmem_attach)
    {
      eel_gmem_attach(m_gmem_name.Get(),false);
    }
    else
    {
      GRAM_context *gc=s_gram_contexts.Get(m_gmem_name.Get());
      if (gc)
      {
        if (!--gc->refcnt)
        {
          s_gram_contexts.Delete(m_gmem_name.Get());
          delete gc;
        }
      }
    }
  }

  m_gmem_name.Set(name);
  if (name[0])
  {
    void **ctx = NULL;
    if (eel_gmem_attach)
    {
      ctx = eel_gmem_attach(name,true);
    }
    else
    {
      GRAM_context *gc=s_gram_contexts.Get(name);
      if (!gc)
      {
        s_gram_contexts.Insert(name,gc = new GRAM_context);
      }
      else
      {
        gc->refcnt++;
      }
      ctx = &gc->ctx;
    }
    // attach to new
    if (m_vm)
    {
      NSEEL_VM_SetGRAM(m_vm,ctx);
    }
  }
}

int *SX_Instance::get_init_code_stats()
{
  return NSEEL_code_getstats(m_init_ch);
}

int *SX_Instance::get_gfx_code_stats()
{
  return NSEEL_code_getstats(m_gfx_ch);
}

int *SX_Instance::get_slider_code_stats()
{
  return NSEEL_code_getstats(m_slider_ch);
}

int *SX_Instance::get_onblock_code_stats()
{
  return NSEEL_code_getstats(m_onblock_ch);
}

int *SX_Instance::get_sample_code_stats()
{
  return NSEEL_code_getstats(m_sample_ch);
}

void SX_Instance::exec_init_code()
{
  int x;
  for (x=0;x<m_common_ch.GetSize();x++) NSEEL_code_execute((NSEEL_CODEHANDLE)m_common_ch.Get(x));
  NSEEL_code_execute(m_init_ch);
}

void SX_Instance::clear_code(bool fullReset)
{
  if (m_vm)
  {
    NSEEL_code_free(m_sample_ch); m_sample_ch=0;
    NSEEL_code_free(m_slider_ch); m_slider_ch=0;
    NSEEL_code_free(m_onblock_ch); m_onblock_ch=0;
    NSEEL_code_free(m_serialize_ch); m_serialize_ch=0;
    NSEEL_code_free(m_gfx_ch); m_gfx_ch=0;

    NSEEL_code_free(m_init_ch); m_init_ch=0;

    int x;
    for(x=0;x<m_common_ch.GetSize();x++)
    {
      NSEEL_code_free((NSEEL_CODEHANDLE)m_common_ch.Get(x));
    }
    m_common_ch.Empty();

    NSEEL_code_compile_ex(m_vm,NULL,0,NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS_RESET);
  }

  m_eel_string_state->clear_state(fullReset);
}
const char *SX_Instance::add_imported_code(const char *code, int lineoffs, WDL_UINT64 *h)
{
  if (!m_vm) return "VM initialization failed!";

  if (!m_common_ch.GetSize()) *h = WDL_FNV64_IV;
  *h=WDL_FNV64(*h,(const unsigned char *)(code?code:""),code?strlen(code)+1:1);

  NSEEL_CODEHANDLE hand=doCompileCode(code,lineoffs,NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);

  if (hand)
  {
    m_common_ch.Add(hand);
    return NULL;
  }
  return NSEEL_code_getcodeerror(m_vm);
}

const char *SX_Instance::set_init_code(const char *code, int lineoffs, WDL_UINT64 h)
{
  m_init_err=0;
  if (!m_vm) return "VM initialization failed!";

  if (!m_common_ch.GetSize()) h = WDL_FNV64_IV;
  h=WDL_FNV64(h,(const unsigned char *)(code?code:""),code?strlen(code)+1:1);

  bool doInit=false;
  if (!m_initcodehash || m_initcodehash != h)
  {
    m_initcodehash=h;
    m_need_init=3;
    doInit = m_extnoinit && *m_extnoinit>=0.5;
  }

  m_init_ch = doCompileCode(code,lineoffs,NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
  if (m_init_ch)
  {
    if (doInit) exec_init_code();
    return NULL;
  }
  char *p=NSEEL_code_getcodeerror(m_vm);
  if (p) m_init_err=1;
  return p;
}

const char *SX_Instance::set_slider_code(const char *code, int lineoffs)
{
  m_slider_err=0;
  if (!m_vm) return "VM initialization failed!";

  WDL_UINT64 h=WDL_FNV64(WDL_FNV64_IV,(const unsigned char *)(code?code:""),code?strlen(code)+1:0);
  if (!m_slidercodehash || m_slidercodehash != h)
  {
    m_slidercodehash=h;
    m_need_init|=1;
  }

  NSEEL_code_free(m_slider_ch);
  m_slider_ch = doCompileCode(code, lineoffs,0);
  if (m_slider_ch) return NULL;
  char *p=NSEEL_code_getcodeerror(m_vm);
  if (p) m_slider_err=1;
  return p;
}

const char *SX_Instance::set_onblock_code(const char *code, int lineoffs)
{
  m_onblock_err=0;
  if (!m_vm) return "VM initialization failed!";
  NSEEL_code_free(m_onblock_ch);
  m_uses_triggers&=~2;
  const int oldtrigrefcnt = NSEEL_VM_get_var_refcnt(m_vm,"trigger");

  m_onblock_ch = doCompileCode(code, lineoffs, NSEEL_CODE_COMPILE_FLAG_NOFPSTATE);
  if (m_onblock_ch)
  {
    if (NSEEL_VM_get_var_refcnt(m_vm,"trigger") > oldtrigrefcnt) m_uses_triggers|=2;
    return NULL;
  }
  char *p=NSEEL_code_getcodeerror(m_vm);
  if (p) m_onblock_err=1;
  return p;
}


const char *SX_Instance::set_sample_code(const char *code, int lineoffs)
{
  m_sample_err=0;
  if (!m_vm) return "VM initialization failed!";
  NSEEL_code_free(m_sample_ch);
  m_uses_triggers&=~1;
  const int oldtrigrefcnt = NSEEL_VM_get_var_refcnt(m_vm,"trigger");
  m_sample_ch = doCompileCode(code, lineoffs,NSEEL_CODE_COMPILE_FLAG_NOFPSTATE);
  if (m_sample_ch)
  {
    if (NSEEL_VM_get_var_refcnt(m_vm,"trigger") > oldtrigrefcnt) m_uses_triggers|=1;
    return NULL;
  }
  char *p=NSEEL_code_getcodeerror(m_vm);
  if (p) m_sample_err=1;
  else if (!m_sample_ch)  // no error, but no @sample code
  {
    m_has_no_inputs = m_has_no_outputs = true;
  }
  return p;
}


const char *SX_Instance::set_gfx_code(const char *code, int lineoffs)
{
  m_gfx_err=0;
  if (!m_vm) return "VM initialization failed!";
  NSEEL_code_free(m_gfx_ch);
  m_gfx_ch = doCompileCode(code, lineoffs,0);

  if (m_gfx_ch) return NULL;
  char *p=NSEEL_code_getcodeerror(m_vm);
  if (p) m_gfx_err=1;
  return p;
}

const char *SX_Instance::set_serialize_code(const char *code, int lineoffs)
{
  m_serialize_err=0;
  if (!m_vm) return "VM initialization failed!";
  NSEEL_code_free(m_serialize_ch);
  m_serialize_ch = doCompileCode(code, lineoffs,0);

  if (m_serialize_ch) return NULL;
  char *p=NSEEL_code_getcodeerror(m_vm);
  if (p) m_serialize_err=1;
  return p;
}

int SX_Instance::load_serialized_state(char *filename)
{
  free(m_need_loadserialize_fn); m_need_loadserialize_fn=0;
  struct stat st;
  if (statUTF8(filename,&st) || st.st_size < 1) return 0;

  m_need_loadserialize_fn=strdup(filename);
  return st.st_size;
}

void SX_Instance::load_serialized_state_frommem(WDL_HeapBuf *hb)
{
  if (m_serialize_ch)
  {
    if (m_need_init&2)
    {
      m_init_mutex.Enter();
      exec_init_code();
      m_need_init&=~2;
      m_init_mutex.Leave();
    }

    m_filehandles[0].OpenMem(hb,0);
    NSEEL_code_execute(m_serialize_ch);
    m_filehandles[0].Close();
  }
}

void SX_Instance::save_serialized_state_tomem(WDL_HeapBuf *hb)
{
  if (!m_serialize_ch) { hb->Resize(0); return; }
  m_filehandles[0].OpenMem(hb,1);
  NSEEL_code_execute(m_serialize_ch);
  m_filehandles[0].Close();
}

int SX_Instance::save_serialized_state(char *filename)
{
  if (!m_serialize_ch) return 0;

  if (!m_filehandles[0].Open(filename,1)) return 0;

  NSEEL_code_execute(m_serialize_ch);

  int l=ftell(m_filehandles[0].m_fp);

  m_filehandles[0].Close();

  return l;
}

int SX_Instance::sliderIsEnum(int s)
{
  effectSlider *slid = m_sliders.Get(s);
  if (slid)
  {
    int v = slid->sliderValueXlateList.GetSize();
    if (v == 1) return 1;
    if (v>1) return v-1;
  }
  return 0;
}

void SX_Instance::getSliderText(int s, char *buf, int buflen, EEL_F* vin)
{
  effectSlider *slid = m_sliders.Get(s);
  if (!slid)
  {
    *buf=0;
    return;
  }

  if (slid->is_file_slider)
  {
    const char *ps = vin ? slid->FileList.Get((int)(*vin + 0.0001)) : slid->currentFile;
    lstrcpyn_safe(buf,ps ? ps : "",buflen);
    WDL_remove_fileext(buf);
  }
  else
  {
    EEL_F v;
    if (vin) v = slid->scaleFromExternal(*vin);
    else v = slid->slider?slid->slider[0]:0.0;

    int a;
    if (slid->sliderValueXlateList.GetSize() && (a=(int)(v+0.0001))>=0 && a < slid->sliderValueXlateList.GetSize())
    {
      lstrcpyn_safe(buf,slid->sliderValueXlateList.Get(a),buflen);
    }
    else
    {
      snprintf(buf,buflen,"%f",v);
      WDL_remove_trailing_decimal_zeros(buf,2); // always show at least X.0
    }
  }
}


EEL_F SX_Instance::peek_effect_value(const char *name, bool *suc)
{
  EEL_F *v = NSEEL_VM_getvar(m_vm,name);
  if (v)
  {
    if (suc) *suc=true;
    return *v;
  }

  if (suc) *suc=false;
  return 0.0;
}


void SX_Instance::recurseScanDir(int whichslider, const char *root, const char *sub)
{
  WDL_FastString s(root);
  if (sub[0])
  {
    s.Append("/");
    s.Append(sub);
  }
  WDL_DirScan dir;
  if (!dir.First(s.Get()))
  {
    do
    {
      if (dir.GetCurrentFN()[0] == '.') continue;

      const char *sp = dir.GetCurrentFN();
      const char *extp = sp;
      while (*extp) extp++;
      while (extp >= sp && *extp != '.') extp--;
      extp++;

      if (dir.GetCurrentIsDirectory())
      {
        WDL_FastString t(sub);
        if (sub[0]) t.Append("/");
        t.Append(sp);

        recurseScanDir(whichslider,root,t.Get());
      }
      else if (extp > sp && (
#ifdef JSFX_SUPPORT_WAV_DECODING
            !stricmp(extp,"wav") ||
#endif
            !stricmp(extp,"txt") ||
            !stricmp(extp,"raw") ||
            (IsMediaExtension && stricmp(extp,"mid") && stricmp(extp,"syx") &&
                           IsMediaExtension(extp,false)
            )
            )
           )
      {
        WDL_FastString t(sub);
        if (sub[0]) t.Append("/");
        t.Append(sp);

        effectSlider *slid = m_sliders.Get(whichslider);
        if (WDL_NORMALLY(slid))
        {
          slid->FileList.Add(strdup(t.Get()));
        }
      }
    }
    while (!dir.Next());
  }
}

static int derefstrcmp(const void *a, const void *b)
{
  return stricmp(*(const char **)a,*(const char **)b);
}

void SX_Instance::rescan_file_sliders()
{
  for (int x = 0; x < m_sliders.GetSize(); x ++)
  {
    effectSlider *slid = m_sliders.Get(x);
    if (!slid || !slid->is_file_slider) continue;

    WDL_FastString last_name(slid->currentFile?slid->currentFile:"");
    slid->FileList.Empty(true,free);

    WDL_FastString s(m_datadir.Get());
    s.Append("/");
    s.Append(slid->FileDirectory.Get());

    recurseScanDir(x,s.Get(),"");

    if (slid->FileList.GetSize())
    {
      qsort(slid->FileList.GetList(), slid->FileList.GetSize(), sizeof(char *), derefstrcmp);

      int i,siz=slid->FileList.GetSize();
      for (i = 0; i < siz; i ++)
      {
        if (!stricmp(slid->FileList.Get(i),last_name.Get()))
        {
          slid->slider[0] = (EEL_F) i;
          break;
        }
      }
    }
  }
}

void SX_Instance::exec_slider_stuff()
{
  for (int x = 0; x < m_sliders.GetSize(); x ++)
  {
    effectSlider *slid = m_sliders.Get(x);
    if (!slid) continue;

    if (slid->file_dirty)
    {
      slid->file_dirty = false;
      if (slid->is_file_slider)
      {
        int a=0;
        if (slid->slider)
        {
          a=(int)(slid->slider[0]+0.0001);
        }
        if (a < 0) a=0;
        if (a >= slid->FileList.GetSize()) a=slid->FileList.GetSize()-1;
        if (slid->slider) slid->slider[0] = (EEL_F) a;

        slid->currentFile = slid->FileList.Get(a);
      }
    }
  }
  NSEEL_code_execute(m_slider_ch);
}

void SX_Instance::on_slider_change(int set_file_index, const char *newv)
{
  if (m_need_init&2)
  {
    if (!m_extnoinit || *m_extnoinit < 0.5)
    {
      m_init_mutex.Enter();
      if (!m_serialize_ch)
      {
        NSEEL_VM_freeRAM(m_vm);
        NSEEL_VM_enumallvars(m_vm,clearVar,this);
      }
      exec_init_code();

      m_init_mutex.Leave();
    }
  }

  if (set_file_index >= 0 && WDL_NORMALLY(newv != NULL))
  {
    effectSlider *slid = m_sliders.Get(set_file_index);
    if (WDL_NORMALLY(slid && slid->is_file_slider))
    {
      WDL_PtrList<char> *list=&slid->FileList;
      int y;
      int maxl=0,maxpos=-1;
      for (y = 0; y < list->GetSize(); y ++)
      {
        const char *thisp=list->Get(y);
        const char *refp=newv;
        int thisl=0;
        while (*thisp && *refp && toupper_safe(*thisp) == toupper_safe(*refp))
        {
          thisl++;
          thisp++;
          refp++;
        }
        if (!*refp && (!*thisp  || (*thisp == '.' && !strchr(thisp+1,'.'))))
        {
          maxpos=y;
          break;
        }
        if (thisl > maxl) { maxl=thisl; maxpos=y; }
      }

      if (maxpos >= 0 && slid->slider)
      {
        slid->slider[0]=(EEL_F)(maxpos);
      }
      slid->file_dirty = true;
    }
  }
  else
  {
    exec_slider_stuff();

    if (NSEEL_VM_wantfreeRAM(m_vm))
    {
      m_init_mutex.Enter();
      NSEEL_VM_freeRAMIfCodeRequested(m_vm);
      m_init_mutex.Leave();
    }
  }

  m_need_init=0;
}


void SX_Instance::gfx_runCode(int flag)
{
  if ((m_need_init&2)) return;

  if (m_vm && m_gfx_ch)
  {
    if (WDL_NORMALLY(m_gfx_ext_flags!=NULL))
      m_gfx_ext_flags[0] = (EEL_F) flag;
    NSEEL_code_execute(m_gfx_ch);
  }

}

int SX_Instance::process_samples(EEL_F *work, int num_samples, int nch, int srate,
  EEL_F tempo, int tsnum, int tsdenom,
  EEL_F playstate, EEL_F playpos, EEL_F playpos_b,
  double lastwet, double newwet, int flags)
{
  const bool ignore_pdc = (flags&1)==1;
  const bool delta_solo = (flags&2)==2;
  const bool parallel_mode = (flags&4)==4;

  m_mutex.Enter();

  if (!num_samples && !nch)
  {
    m_pdc_lastdelay=0;
    m_pdc_queue.clear();
  }

  if (m_last_srate != srate || m_last_nch != nch)
  {
    const bool noInit=m_extnoinit && *(m_extnoinit)>0.5;
    if (!noInit || (nch && srate))
    {
      m_need_init|=noInit ? 1 : 3;
    }
    if (srate) m_last_srate=srate;
    if (nch) m_last_nch=nch;
  }

  if (m_srate && srate) m_srate[0]=(EEL_F) srate;
  if (m_num_ch && nch) m_num_ch[0] = (EEL_F) nch;

  if (num_samples > 0)
  {
    const bool reset_parmqs = m_need_init;

    if (m_need_init) // PDC variables only cleared if resetting
    {
      m_pdc_lastdelay=0;
      m_pdc_queue.clear();
      if (m_pdc_amt) *m_pdc_amt=0;
      if (m_pdc_midiearly) *m_pdc_midiearly=0;
      if (m_pdc_topch) *m_pdc_topch=0;
      if (m_pdc_botch) *m_pdc_botch=0;
    }

    if (m_need_init || m_need_loadserialize_fn)
    {
      if (m_need_init&2)
      {
        if (!m_extnoinit || *m_extnoinit < 0.5)
        {
          m_init_mutex.Enter();
          if (!m_serialize_ch)
          {
            NSEEL_VM_freeRAM(m_vm);
            NSEEL_VM_enumallvars(m_vm,clearVar,this);
          }
          exec_init_code();
          m_init_mutex.Leave();
        }
      }
      if (m_need_loadserialize_fn)
      {
        if (m_serialize_ch)
        {
          if (m_filehandles[0].Open(m_need_loadserialize_fn,-1))
          {
            if (m_filehandles[0].m_itemsleft>0)
            {
              NSEEL_code_execute(m_serialize_ch);
            }
            m_filehandles[0].Close();
          }
        }
        free(m_need_loadserialize_fn);
        m_need_loadserialize_fn=0;
      }

      if (m_need_init&1) exec_slider_stuff();
      m_need_init=0;
    }

    *m_trigger = (double) m_trig;
    *m_samplesblock=(EEL_F)num_samples;

    if (m_tempo) *m_tempo=tempo;
    if (m_tsnum) *m_tsnum=(EEL_F)tsnum;
    if (m_tsdenom) *m_tsdenom=(EEL_F)tsdenom;
    if (m_playstate) *m_playstate=playstate;
    if (m_playpos_s) *m_playpos_s=playpos;
    if (m_playpos_b) *m_playpos_b=playpos_b;

    int fpstate[2];
    eel_enterfp(fpstate);
    NSEEL_code_execute(m_onblock_ch);

    if (NSEEL_VM_wantfreeRAM(m_vm))
    {
      m_init_mutex.Enter();
      NSEEL_VM_freeRAMIfCodeRequested(m_vm);
      m_init_mutex.Leave();
    }

    // pdc vars must be set after @block and before @sample!
    const bool isPDC = nch>0 && m_pdc_amt && m_pdc_topch && m_pdc_botch && num_samples && *m_pdc_amt >= 0.5;
    const bool doWetDry = !m_has_no_outputs && (lastwet != 1.0 || newwet != 1.0 || delta_solo);

    if (doWetDry || isPDC)
    {
      // allocate extra space for PDC work buffers
      int alloc_sz = nch*num_samples;
      if (isPDC) alloc_sz<<=1;
      EEL_F *p=m_drybuf.ResizeOK(alloc_sz,false);
      if (p)
      {
        if (work && (!parallel_mode || delta_solo)) memcpy(p,work,nch*num_samples*sizeof(EEL_F));
        else memset(p,0,nch*num_samples*sizeof(EEL_F));
      }
    }

    double bm_tst=0.0;
    bool use_bench=bm_enabled;
    if (use_bench)
    {
      bm_tst=time_precise();
      if (!bm_cnt)
      {
        bm_starttime=bm_tst;
        bm_usetime=0;
      }
    }

    int i;
    if (reset_parmqs)
    {
      for (i=0; i < m_sliders.GetSize(); ++i)
      {
        effectSlider *slid = m_sliders.Get(i);
        if (slid)
        {
          slid->parmchg_q_curpos=0;
          slid->parmchg_q.DeleteAll(false);
        }
      }
    }

    if (m_sample_ch)
    {
      int numsamples = num_samples;
      EEL_F *ptr=work;
      const int usech = wdl_min(nch,MAX_NCH);
      double DENORM_STATE=DENORM*0.5;

      EEL_F * const SPL = m_spls;
      if (nch<1 || !work)
      {
        while (numsamples--) NSEEL_code_execute(m_sample_ch);
      }
      else if (m_extnodenorm && *m_extnodenorm>0.5)
      {
        if (m_has_no_outputs)
        {
          if (nch > 2) while (numsamples--)
          {
            int i;
            for (i = 0; i < usech; i ++) SPL[i] = ptr[i];
            ptr+=nch;

            NSEEL_code_execute(m_sample_ch);
          }
          else if (nch == 2) while (numsamples--)
          {
            SPL[0] = ptr[0];
            SPL[1] = ptr[1];
            ptr+=2;

            NSEEL_code_execute(m_sample_ch);
          }
          else if (nch == 1) while (numsamples--)
          {
            SPL[1] = SPL[0] = *ptr++;
            NSEEL_code_execute(m_sample_ch);
          }
        }
        else
        {
          if (nch > 2) while (numsamples--)
          {
            int i;
            for (i = 0; i < usech; i ++) SPL[i] = ptr[i];
            NSEEL_code_execute(m_sample_ch);
            for (i = 0; i < usech; i ++)  ptr[i] = SPL[i];

            ptr+=nch;
          }
          else if (nch == 2) while (numsamples--)
          {
            SPL[0] = ptr[0];
            SPL[1] = ptr[1];
            NSEEL_code_execute(m_sample_ch);
            ptr[0]=SPL[0];
            ptr[1]=SPL[1];
            ptr+=2;
          }
          else if (nch == 1) while (numsamples--)
          {
            SPL[1] = SPL[0] = ptr[0];
            NSEEL_code_execute(m_sample_ch);
            *ptr++=SPL[0];
          }
        }
      }
      else
      {
        if (m_has_no_outputs)
        {
          if (nch > 2) while (numsamples--)
          {
            int i;
            for (i = 0; i < usech; i ++) SPL[i] = ptr[i]+DENORM_STATE+DENORM;
            DENORM_STATE=-DENORM_STATE;
            ptr+=nch;

            NSEEL_code_execute(m_sample_ch);
          }
          else if (nch == 2) while (numsamples--)
          {
            SPL[0] = ptr[0]+DENORM_STATE+DENORM;
            SPL[1] = ptr[1]+DENORM_STATE+DENORM;
            DENORM_STATE=-DENORM_STATE;
            ptr+=2;

            NSEEL_code_execute(m_sample_ch);
          }
          else if (nch == 1) while (numsamples--)
          {
            SPL[1] = SPL[0] = ptr[0]+DENORM_STATE+DENORM;
            DENORM_STATE=-DENORM_STATE;
            ptr++;

            NSEEL_code_execute(m_sample_ch);
          }
        }
        else
        {
          if (nch > 2) while (numsamples--)
          {
            int i;
            for (i = 0; i < usech; i ++) SPL[i] = ptr[i]+DENORM_STATE+DENORM;
            DENORM_STATE=-DENORM_STATE;
            NSEEL_code_execute(m_sample_ch);
            for (i = 0; i < usech; i ++)  ptr[i] = SPL[i];

            ptr+=nch;
          }
          else if (nch == 2) while (numsamples--)
          {
            SPL[0] = ptr[0]+DENORM_STATE+DENORM;
            SPL[1] = ptr[1]+DENORM_STATE+DENORM;
            DENORM_STATE=-DENORM_STATE;
            NSEEL_code_execute(m_sample_ch);
            ptr[0]=SPL[0];
            ptr[1]=SPL[1];
            ptr+=2;
          }
          else if (nch == 1) while (numsamples--)
          {
            SPL[1] = SPL[0] = ptr[0]+DENORM_STATE+DENORM;
            DENORM_STATE=-DENORM_STATE;
            NSEEL_code_execute(m_sample_ch);
            *ptr++=SPL[0];
          }
        }
      }
    }
    eel_leavefp(fpstate);

    int need_chg=0;
    for (i=0; i < m_sliders.GetSize(); ++i)
    {
      effectSlider *slid = m_sliders.Get(i);
      if (!slid) continue;

      if (slid->parmchg_q.GetSize() > slid->parmchg_q_curpos)
      {
        // unconsumed parameter changes, apply now
        const double nv = slid->parmchg_q.Enumerate(slid->parmchg_q.GetSize()-1);
        if (nv != slid->slider[0])
        {
          slid->slider[0] = nv;
          need_chg++;
        }
      }
      slid->parmchg_q_curpos=0;
      slid->parmchg_q.DeleteAll(false);
      if (slid->is_file_slider) slid->file_dirty = true;
    }
    if (need_chg) on_slider_change();

    if (use_bench)
    {
      bm_cnt++;
      bm_usetime += (bm_lastendtime=time_precise()) - bm_tst;
    }
    int plugin_delay = isPDC ? (int) (*m_pdc_amt+0.5) : 0;
    const int PDC_LIMIT = 256000;
    if (plugin_delay < 0) plugin_delay=0;
    else if (plugin_delay > PDC_LIMIT) plugin_delay = PDC_LIMIT;
    m_pdc_lastdelay = plugin_delay;

    if (ignore_pdc) plugin_delay = 0; // report latency but do not delay-compensate dry channels if we're in a recording chain (matching VST behavior)

    if (plugin_delay > 0)
      m_pdc_queue.set_nch_length(nch, plugin_delay, plugin_delay);
    else
      m_pdc_queue.free_memory();

    if (m_hwnd && wantOutputMetering()) calcPeaks(work,num_samples,nch,1);

    if (plugin_delay > 0 && work)
    {
      int chan_top=(int) (*m_pdc_topch+0.5);
      int chan_bot=(int) (*m_pdc_botch+0.5);
      if (chan_top < 0) chan_top=0;
      if (chan_bot < 0) chan_bot=0;
      if (chan_top > nch) chan_top=nch;
      if (chan_bot > chan_top) chan_bot=chan_top;

      const int l1 = chan_bot, l2 = chan_top-chan_bot, l3 = nch - chan_top;
      //  l1/l3 are channels ranges which must be delayed by us
      //  l2 is delayed by the plug-in
      // the wet/dry control on l1/l3 is a bit iffy (should probably be done at a later time), but it's the way
      // it's been (and not sure if any plug-ins modify the non-PDC-compensated channels so...)
      //
      // m_drybuf contains this block, dry, and has extra space
      // work contains this block processed, and is the output
      // m_pdc_queue contains history
      // work[l1/l3] = hist
      // work[l2] = wet*work + (1-wet)*hist
      // hist[l1/l3] = wet*work + (1-wet)*m_drybuf
      // hist[l2] = m_drybuf
      // if m_pdc_queue length is less than a block, we must handle that!
      const EEL_F * const drybuf = m_drybuf.Get();
      EEL_F * const hist = m_drybuf.Get() + nch*num_samples;
      int qavail = wdl_min(m_pdc_queue.get_avail_pairs(),num_samples);
      m_pdc_queue.get_pairs(hist,qavail);
      if (qavail < num_samples)
      {
        // l1/l3 get copied from work, l2 from drybuf
        int amt = num_samples-qavail, offs = 0;
        EEL_F *w=hist+qavail*nch;
        while (amt--)
        {
          memcpy(w+offs,work+offs,l1*sizeof(EEL_F)); offs+=l1;
          memcpy(w+offs,drybuf+offs,l2*sizeof(EEL_F)); offs+=l2;
          memcpy(w+offs,work+offs,l3*sizeof(EEL_F)); offs+=l3;
        }
      }

      if (doWetDry)
      {
        const double dwet = delta_solo ? 0.0 : (newwet - lastwet) / (double)num_samples;
        double w = delta_solo ? 1.0 : lastwet;
        double iw = delta_solo ? -1.0 : 1.0-w;
        int a = num_samples, offs=0;
        while (a-->0)
        {
          int n;
          for (n=l1; n--; offs++)
          {
            const EEL_F hist_val = hist[offs];
            hist[offs] = work[offs]*w + iw*drybuf[offs];
            work[offs] = hist_val;
          }
          for (n=l2; n--; offs++)
          {
            work[offs] = w*work[offs] + iw*hist[offs];
            hist[offs] = drybuf[offs];
          }
          for (n=l3; n--; offs++)
          {
            const EEL_F hist_val = hist[offs];
            hist[offs] = work[offs]*w + iw*drybuf[offs];
            work[offs] = hist_val;
          }
          w += dwet;
          iw -= dwet;
        }
      }
      else
      {
        int a = num_samples, offs=0;
        while (a-->0)
        {
          int n;
          for (n=l1; n--; offs++)
          {
            const EEL_F hist_val = hist[offs];
            hist[offs] = work[offs];
            work[offs] = hist_val;
          }
          for (n=l2; n--; offs++)
          {
            hist[offs] = drybuf[offs];
          }
          for (n=l3; n--; offs++)
          {
            const EEL_F hist_val = hist[offs];
            hist[offs] = work[offs];
            work[offs] = hist_val;
          }
        }
      }
      m_pdc_queue.add_pairs(hist + (num_samples-qavail)*nch,qavail);
    }
    else
    {
      // no PDC
      if (doWetDry && work && m_drybuf.GetSize() >= nch*num_samples)
      {
        // wet-dry
        EEL_F *wpo = work;
        const EEL_F *dryMixPtr=m_drybuf.Get();

        const double dwet = delta_solo ? 0.0 : (newwet - lastwet) / (double)num_samples;
        double w = delta_solo ? 1.0 : lastwet;
        double iw = delta_solo ? -1.0 : 1.0-w;

        if (dwet == 0.0)
        {
          if (delta_solo || w != 1.0)
          {
            int n = num_samples * nch;
            while (n-->0)
            {
              wpo[0] = wpo[0] * w + *dryMixPtr++ * iw;
              wpo++;
            }
          }
        }
        else
        {
          int n = num_samples;
          while (n-->0)
          {
            int i=nch;
            while (i--)
            {
              wpo[0] = wpo[0] * w + *dryMixPtr++ * iw;
              wpo++;
            }
            w += dwet;
            iw -= dwet;
          }
        }
      }
    }

    if (NSEEL_VM_wantfreeRAM(m_vm))
    {
      m_init_mutex.Enter();
      NSEEL_VM_freeRAMIfCodeRequested(m_vm);
      m_init_mutex.Leave();
    }

    if (midi_sendrecv)
    {
      midi_sendrecv(midi_ctxdata,0,NULL,NULL,NULL, NULL);
    }
  }

  m_trig=0;

  int ret_val = WantMidiNoDelay() ? 0x1000 : 0;

  m_mutex.Leave();

  return ret_val;
}



bool SX_Instance::GetFilenameForParameter(EEL_F val, WDL_FastString *fsWithDir, int isWrite) // returns true if valid, and set fsWithDir
{
  WDL_MutexLock __lock(&m_string_sermutex);

  int effect_path_len=0; // length of effect path w/ trailing slash
  if (!isWrite && m_full_actual_fn_used.Get()[0])
  {
    const char *p=m_full_actual_fn_used.Get() + m_full_actual_fn_used.GetLength();
    while (p>=m_full_actual_fn_used.Get() && *p != '\\' && *p != '/') p--;
    effect_path_len = p+1 - m_full_actual_fn_used.Get();
  }

  int a=(int) val;
  if (a >= 0 && a < m_localfilenames.GetSize())
  {
    if (effect_path_len>0)
    {
      fsWithDir->Set(m_full_actual_fn_used.Get(),effect_path_len);
      fsWithDir->Append(m_localfilenames.Get(a));

      FILE *fp = fopen(fsWithDir->Get(),"rb");
      if (fp) { fclose(fp); return true; }
    }

    if (m_datadir.GetLength())
    {
      fsWithDir->Set(m_datadir.Get());
      fsWithDir->Append("/");
      fsWithDir->Append(m_localfilenames.Get(a));
      return true;
    }
    return false;
  }

  const char *str = GetStringForIndex(val, NULL);
  if (!str || !*str) return false;

  if (!isWrite)
  {
  #ifdef _WIN32
    if ((str[0] && str[1] == ':') || (str[0] == '\\' && str[1] == '\\'))
  #else
    if (str[0] == '/')
  #endif
    {
      fsWithDir->Set(str);
      return true;
    }
  }

#ifdef _WIN32
  if ((str[0] && str[1] == ':') || strstr(str,"..\\")) return false;
#else
  if (str[0] == '/' || strstr(str,"../")) return false;
#endif

  if (effect_path_len>0)
  {
    fsWithDir->Set(m_full_actual_fn_used.Get(),effect_path_len);
    fsWithDir->Append(str);
    FILE *fp = fopen(fsWithDir->Get(),"rb");
    if (fp) { fclose(fp); return true; }
  }

  if (m_datadir.GetLength())
  {
    fsWithDir->Set(m_datadir.Get());
    fsWithDir->Append("/");
    fsWithDir->Append(str);
    return true;
  }
  return false;
}

EEL_F *SX_Instance::GetNamedVar(const char *s, bool createIfNotExists, EEL_F *altOut)
{
  if (s && !stricmp(s,"#dbg_desc")) { static EEL_F a = EEL_STRING_DBG_DESC; return &a;  }
  if ((!s || s[0] != '#') && createIfNotExists && GetCurrentThreadId() != m_main_thread) createIfNotExists=false;
  return m_eel_string_state->GetNamedVar(s,createIfNotExists,altOut);
}
void SX_Instance::update_var_names()
{
  m_eel_string_state->update_named_vars(m_vm);
}
const char *SX_Instance::GetStringForIndex(EEL_F idx, WDL_FastString **p, bool is_for_write)
{
  if (idx == EEL_STRING_DBG_DESC)
  {
    if (is_for_write) m_description_dirty = true;
    if (p) *p = &m_description;
    return m_description.Get();
  }
  return m_eel_string_state->GetStringForIndex(idx,p,is_for_write);
}
const char *SX_Instance::EnumerateNamedStrings(int idx, const char **name) // called by IDE watch
{
  *name = NULL;
  WDL_FastString *fs = m_eel_string_state->m_named_strings.Get(m_eel_string_state->m_named_strings_names.Enumerate(idx,name) - EEL_STRING_NAMED_BASE);
  return (*name && fs) ? fs->Get() : NULL;
}
const char *SX_Instance::GetNamedStringValue(const char *name, int *indexOut)
{
  if (!stricmp(name,"dbg_desc"))
  {
    if (indexOut) *indexOut = EEL_STRING_DBG_DESC;
    return m_description.Get();
  }

  int idx=m_eel_string_state->m_named_strings_names.Get(name);
  if (indexOut) *indexOut=idx;
  if (!idx) return NULL;

  WDL_FastString *fs = m_eel_string_state->m_named_strings.Get(idx - EEL_STRING_NAMED_BASE);
  if (fs) return fs->Get();
  return NULL;
}

void sx_setParmVal(SX_Instance *sx, int parm, double val, int sampleoffs)
{
  if (!sx || WDL_NOT_NORMALLY(sx->is_in_gfx())) return;

  WDL_MutexLock lock(&sx->m_mutex);
  int a=sx->parmToIndex(parm);
  if (a<0) return;

  effectSlider *slider=sx->m_sliders.Get(a);
  if (WDL_NORMALLY(slider && slider->slider))
  {
    if (slider->is_file_slider)
    {
    }
    else if (slider->external_shaping)
    {
      val = slider->scaleFromExternal(val);
    }
    else
    {
      if (slider->range_scale > 0.0)
      {
        val = floor(val/slider->range_scale+0.5)*slider->range_scale;
      }
      if (slider->range_min < slider->range_max)
      {
        if (val < slider->range_min) val = slider->range_min;
        else if (val > slider->range_max) val = slider->range_max;
      }
      else
      {
        if (val < slider->range_max) val = slider->range_max;
        else if (val > slider->range_min) val = slider->range_min;
      }
    }

    if (sampleoffs)
    {
      slider->parmchg_q.Insert(sampleoffs, val);
    }
    else
    {
      // during playback of automation, this will usually do nothing since
      // the value applied at the end of the last block should match the
      // first value of this block
      if (slider->slider[0] != val)
      {
        slider->slider[0]=val;
        if (slider->is_file_slider) slider->file_dirty = true;
        sx->on_slider_change();
      }
    }
  }
}

bool sx_hasParmChanged(SX_Instance *sx)
{
  if (!sx) return false;

  if (!sx->m_slider_anychanged) return false;
  sx->m_slider_anychanged=false;
  return true;
}

const char **sx_getPinInfo(SX_Instance *sx, int isOutput, int *numPins)
{
  if (!sx)
  {
    *numPins=0;
    return 0;
  }

  WDL_PtrList<char> *p = isOutput ? &sx->m_out_pinnames : &sx->m_in_pinnames;

  int npins = p->GetSize();
  if (!npins)
  {
    if (sx->m_has_no_outputs || sx->m_has_no_inputs || sx->m_out_pinnames.GetSize() || sx->m_in_pinnames.GetSize()) npins = 0;
    else npins = -1;  // we don't know anything
  }
  *numPins = npins;

  return (const char**)p->GetList();
}

void sx_updateHostNch(SX_Instance *sx, int nch)
{
  if (sx) sx->m_hostnch=nch;
}

void sx_destroyInstance(SX_Instance *sx)
{
  delete sx;
}


int sx_getNumParms(SX_Instance *sx)
{
  if (!sx) return 0;

  int cnt=0;
  for (int x = 0; x < sx->m_sliders.GetSize(); x ++)
  {
    effectSlider *slid = sx->m_sliders.Get(x);
    if (slid) cnt++;
  }

  return cnt;
}

double sx_getParmVal(SX_Instance *sx, int parm, double *minval, double *maxval, double *step)
{
  if (minval) *minval=0.0;
  if (maxval) *maxval=1.0;
  if (!sx) return 0.0;

  int a=sx->parmToIndex(parm);
  if (a<0) return 0.0;

  effectSlider *slider=sx->m_sliders.Get(a);
  if (WDL_NOT_NORMALLY(slider == NULL)) return 0.0;

  if (slider->sliderValueXlateList.GetSize() || slider->is_file_slider)
  {
    double val=0.0;
    if (minval) *minval=0.0;
    if (slider->is_file_slider)
    {
      if (maxval)
        *maxval=(double)slider->FileList.GetSize()-1.0;
    }
    else
    {
      if (minval) *minval=0.0;
      if (maxval)
        *maxval=(double)slider->sliderValueXlateList.GetSize()-1.0;
    }
    if (maxval && *maxval < 0.0) *maxval=0.0;
    if (step) *step = 1.0;
    if (slider->slider) val += slider->slider[0];
    return val;
  }

  if (slider->external_shaping)
  {
    if (minval) *minval=slider->ui_lcstart;
    if (maxval) *maxval=slider->ui_lcend;
    return slider->slider ? slider->scaleToExternal(slider->slider[0]) : 0.0;
  }
  if (minval) *minval=slider->range_min;
  if (maxval) *maxval=slider->range_max;
  if (step) *step = slider->range_scale;
  return slider->slider ? slider->slider[0] : 0.0;
}

bool sx_getParmValueFromString(SX_Instance *sx, int parm, const char *buf, double *valptr)
{
  if (!sx) return false;
  int a = sx->parmToIndex(parm);
  effectSlider *slid = sx->m_sliders.Get(a);
  if (WDL_NOT_NORMALLY(!slid)) return false;

  const WDL_PtrList<char> * const l = slid->is_file_slider ? &slid->FileList : &slid->sliderValueXlateList;
  for (int x = 0; x < l->GetSize(); x++)
  {
    const char *p = l->Get(x);
    if (p && !stricmp(p,buf))
    {
      if (valptr) *valptr = x;
      return true;
    }
  }

  if (slid->external_shaping)
  {
    *valptr = slid->scaleToExternal(flexi_atof(buf));
    return true;
  }

  return false;
}

void sx_getParmDisplay(SX_Instance *sx, int parm, char* disptxt, int len, double* inval)
{
  if (!disptxt) return;
  disptxt[0] = 0;

  if (!sx) return;
  int a = sx->parmToIndex(parm);
  if (a<0) return;

  sx->getSliderText(a, disptxt, len, inval);
}

void sx_getParmName(SX_Instance *sx, int parm, char *name, int namelen)
{
  *name=0;
  if (!sx) return;

  int a=sx->parmToIndex(parm);
  if (a<0) return;

  effectSlider *slid = sx->m_sliders.Get(a);
  if (WDL_NOT_NORMALLY(!slid)) return;

  const char *np=slid->name.Get();
  if (*np=='-')
  {
    np++;

    if (*np == '-') np++; // skip second '-'
  }
  lstrcpyn_safe(name,np,namelen);
}

int sx_parmIsEnum(SX_Instance *sx, int parm)
{
  if (!sx) return 0;

  int a = sx->parmToIndex(parm);
  if (a<0) return 0;

  return sx->sliderIsEnum(a);
}


void sx_set_midi_ctx(SX_Instance *sx, double (*midi_sendrecv)(void *ctx, int action, double *ts, double *msg1, double *msg23, double *midibus), void *midi_ctxdata)
{
  if (sx)
  {
    sx->midi_ctxdata = midi_ctxdata;
    sx->midi_sendrecv = midi_sendrecv;
  }
}

void sx_set_host_ctx(SX_Instance *inst, void* hostctx, void (*slider_automate)(void *ctx, int parmidx, bool done))
{
  SX_Instance *sx=(SX_Instance *)inst;
  if (sx)
  {
    sx->m_hostctx = hostctx;
    sx->m_slider_automate = slider_automate;
  }
}

EEL_F *sx_effect_var_resolver(void *userctx, const char *name)
{
  if (!strnicmp(name,"spl",3) && name[3]>='0' && name[3]<='9')
  {
    const char *p = name+4;
    while (*p >= '0' && *p <= '9') p++;
    if (!*p)
    {
      const int idx = atoi(name+3);
      if (idx>=0 && idx < MAX_NCH)
      {
        SX_Instance *inst = (SX_Instance *)userctx;
        return inst->m_spls + idx;
      }
    }
  }
  return NULL;
}

int g_last_srate;
SX_Instance::SX_Instance()
{
  m_embed_refstate = 0;
  curses_registerChildClass(g_hInst);
  memset(&m_editor_curses,0,sizeof(m_editor_curses));
  m_hwndwatch = NULL;
  m_editor_cur=0;
  m_hwnd = NULL;
  m_wnd_bottomofcontrols=30;
  m_last_ionch=0;
  m_last_nch = 2;
  m_last_srate = g_last_srate > 0 ? g_last_srate : 44100;
  m_hostnch=0;
  eel_init();

  memset(m_spls,0,sizeof(m_spls));

  m_misc_flags=0;
  m_trig=0;
  memset(m_peakposdbl,0,sizeof(m_peakposdbl));
  m_preset_reent=0;

  m_last_error[0]=0;

  midi_ctxdata=0;
  midi_sendrecv=0;

  m_hostctx = NULL;
  m_slider_automate = NULL;

  m_pdc_lastdelay=0;
  m_want_all_kb=false;
  m_no_meters=false;
  m_main_thread=GetCurrentThreadId();
  bm_enabled=false;
  bm_cnt=0;
  bm_lastendtime=bm_starttime=bm_usetime=0;
  m_initcodehash= m_slidercodehash=0;
  m_uses_triggers=0;

  m_last_slider_sel=0;

  m_slider_anychanged=0;
  m_slider_vischanged=0;

  m_gfx_want_idle = 0;
  m_gfx_last_inline_runtime = GetTickCount()-10000;
  m_in_gfx=0;
  m_gfx_hz = 0;
  m_gfx_reqw=m_gfx_reqh=0;
  m_need_init=3;
  m_vm=NSEEL_VM_alloc();

  NSEEL_VM_set_var_resolver(m_vm,sx_effect_var_resolver,(void *)this);

  m_extnoinit=NSEEL_VM_regvar(m_vm,"ext_noinit");
  m_extwantmidibus=NSEEL_VM_regvar(m_vm,"ext_midi_bus");
  m_extnodenorm= NSEEL_VM_regvar(m_vm,"ext_nodenorm");
  m_midibus=NSEEL_VM_regvar(m_vm,"midi_bus");
  m_srate = NSEEL_VM_regvar(m_vm,"srate");
  m_samplesblock = NSEEL_VM_regvar(m_vm,"samplesblock");
  m_tsnum = NSEEL_VM_regvar(m_vm, "ts_num");
  m_tsdenom = NSEEL_VM_regvar(m_vm, "ts_denom");
  m_pdc_amt=NSEEL_VM_regvar(m_vm,"pdc_delay");
  m_ext_tail_size = NSEEL_VM_regvar(m_vm,"ext_tail_size");
  m_pdc_topch=NSEEL_VM_regvar(m_vm,"pdc_top_ch");
  m_pdc_botch=NSEEL_VM_regvar(m_vm,"pdc_bot_ch");
  m_pdc_midiearly=NSEEL_VM_regvar(m_vm,"pdc_midi");
  m_num_ch = NSEEL_VM_regvar(m_vm,"num_ch");
  m_tempo = NSEEL_VM_regvar(m_vm,"tempo");
  m_playpos_s = NSEEL_VM_regvar(m_vm,"play_position");
  m_playpos_b = NSEEL_VM_regvar(m_vm,"beat_position");
  m_playstate = NSEEL_VM_regvar(m_vm,"play_state");

  m_trigger=NSEEL_VM_regvar(m_vm,"trigger");
  m_gfx_ext_flags = NSEEL_VM_regvar(m_vm,"gfx_ext_flags");
  m_ext_gr_meter = NSEEL_VM_regvar(m_vm,"ext_gr_meter");
  if (m_ext_gr_meter) *m_ext_gr_meter = 10000.0;

  if (m_vm)
  {
    NSEEL_VM_SetCustomFuncThis(m_vm,this);
    void eel_string_initvm(NSEEL_VMCTX vm);
    eel_string_initvm(m_vm);
  }

  if (m_srate) *m_srate = m_last_srate;
  if (m_num_ch) *m_num_ch = m_last_nch;

  m_lice_state=new eel_lice_state(m_vm,this,128,16);

  resetVarsToStock(false);
  m_slider_ch = m_sample_ch = m_init_ch = m_onblock_ch = NULL;
  m_serialize_ch = NULL;
  m_gfx_ch = NULL;
  m_need_loadserialize_fn = NULL;
  m_init_err=m_slider_err=m_onblock_err=m_serialize_err=m_sample_err=0;
  m_description_dirty = false;
}

SX_Instance::~SX_Instance()
{
  remove_idle();

  if (m_hwndwatch)
  {
    DestroyWindow(m_hwndwatch);
    m_hwndwatch=0;
  }
  curses_unregisterChildClass(g_hInst);
  m_editor_list.Empty(true); // deleting the active m_editor will clear m_editor_curses
  m_watchlist.Empty(true);

  if (m_hwnd)
  {
    DestroyWindow(m_hwnd);
    m_hwnd=0;
  }

  clear_code(true);

  set_gmem_name("");

  NSEEL_VM_free(m_vm);
  if (m_need_loadserialize_fn) free(m_need_loadserialize_fn);

  m_localfilenames.Empty(true,free);
  m_in_pinnames.Empty(true,free);
  m_out_pinnames.Empty(true,free);

  m_sliders.Empty(true);

  m_config_items.Empty(true);

  delete m_lice_state;

  eel_quit();
}


SX_Instance *sx_createInstance(const char *dir_root, const char *effect_name, bool *wantWak)
{
  SX_Instance *sx=new SX_Instance;

  sx->m_name.Set(effect_name);

  sx->m_datadir.Set(dir_root);
  sx->m_effectdir.Set(dir_root);
#ifdef _WIN32
  sx->m_datadir.Append("\\Data");
  sx->m_effectdir.Append("\\Effects");
#else
  sx->m_datadir.Append("/Data");
  sx->m_effectdir.Append("/Effects");
#endif

  sx->m_last_error[0]=0;

#ifndef _WIN32
  {
    char *p = (char *)sx->m_name.Get();
    while (*p)
    {
      if (*p == '\\') *p='/';
      p++;
    }
  }
#endif

  if (sx->LoadEffect(sx->m_name.Get(), SX_Instance::LOADEFFECT_RESETFLAG_CONFIGITEMS|SX_Instance::LOADEFFECT_RESETFLAG_SLIDERS, NULL))
  {
    sx->DoImageLoads(false);
    if (wantWak) *wantWak = sx->m_want_all_kb;

    return sx;
  }

  delete sx;
  return NULL;
}

const char *sx_getEffectName(SX_Instance *sx)
{
  if (!sx) return 0;
  return sx->m_name.Get();
}


void sx_loadSerState(SX_Instance *sx, const char *buf, int len)
{
  if (!sx) return;
  sx->LoadSerState(buf,len);
}

int sx_getCurrentLatency(SX_Instance *sx)
{
  if (!sx) return 0;
  return sx->GetDelaySamples();
}

const char * sx_getFullFN(SX_Instance *sx)
{
  if (!sx) return NULL;
  return sx->m_full_actual_fn_used.Get();
}
const char * sx_saveSerState(SX_Instance *sx, int *len)
{
  if (!sx) { *len=0; return ""; }
  return sx->SaveSerState(len);
}


#define LEGACY_PRESET_CFG_INDEX 64 // 64 parameters, then preset, then any remaining parameters

void sx_loadState(SX_Instance *sx, const char *buf)
{
  if (!sx || WDL_NOT_NORMALLY(sx->is_in_gfx())) return;

  sx->m_curpresetname.Set("");

  int x;
  LineParser lp;
  if (lp.parse(buf)) return;

  if (sx->m_config_items.GetSize())
  {
    for (int x = 0; x < sx->m_config_items.GetSize(); x ++)
    {
      effectConfigItem *item = sx->m_config_items.Get(x);
      item->prev_val = item->cur_val;
      item->cur_val = item->def_val;
    }
    const char *ss = " #CFGSTR:";
    const char *cfg_suffix = strstr(buf,ss);
    while (cfg_suffix)
    {
      for (int x = 0; x < sx->m_config_items.GetSize(); x ++)
      {
        effectConfigItem *item = sx->m_config_items.Get(x);
        if (!strnicmp(item->symbol.Get(),cfg_suffix+9,item->symbol.GetLength()) &&
            cfg_suffix[9+item->symbol.GetLength()] == '=')
        {
          item->cur_val = atof(cfg_suffix+9+item->symbol.GetLength()+1);
          break;
        }
      }
      cfg_suffix = strstr(cfg_suffix + 9,ss);
    }
    for (int x = 0; x < sx->m_config_items.GetSize(); x ++)
    {
      effectConfigItem *item = sx->m_config_items.Get(x);
      if (item->prev_val != item->cur_val)
      {
        sx->DoRecompile(sx->m_hwnd?sx->m_hwnd:NULL, SX_Instance::LOADEFFECT_RESETFLAG_VARS|SX_Instance::LOADEFFECT_RESETFLAG_SLIDERS);
        break;
      }
    }
  }

  if (lp.getnumtokens()<=0) return;

  {
    int y;
    for (y = 0; y < 2; y ++) for (x = 0; x < sx->m_sliders.GetSize(); x ++)
    {
      const char *val=lp.gettoken_str(x + (x >= LEGACY_PRESET_CFG_INDEX ? 1 : 0));

      effectSlider *slid = sx->m_sliders.Get(x);
      if (!slid) continue;

      if (slid->is_file_slider)
      {
        if (y)
        {
          sx->on_slider_change(x, (char *)val);
        }
      }
      else
      {
        if (!y && slid->slider)
        {
          if (val[0] && strcmp(val,"-")) // if -, use default
            slid->slider[0]=flexi_atof(val);
        }
      }
    }
    sx->on_slider_change();

    const char* presetname=lp.gettoken_str(LEGACY_PRESET_CFG_INDEX);
    if (presetname && presetname[0])
    {
      sx->m_curpresetname.Set(presetname);
    }

    if (sx->m_hwnd)
    {
      PostMessage(sx->m_hwnd, WM_USER+4400, 0, 0);
    }
  }
}

static char getConfigStringQuoteChar(const char *p) // copy from projectcontext only slightly modded
{
  if (!p || !*p) return '"';

  char fc = *p;
  int flags=0;
  while (*p && flags!=15)
  {
    char c=*p++;
    if (c=='"') flags|=1;
    else if (c=='\'') flags|=2;
    else if (c=='`') flags|=4;
    else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') flags |= 8;
  }
  if (!(flags & 8) && fc != '"' && fc != '\'' && fc != '`' && fc != '#' && fc != ';') return ' ';
  if (!(flags & 1)) return '"';
  if (!(flags & 2)) return '\'';
  if (!(flags & 4)) return '`';
  return 0;
}

static void appendEscapedConfigString(const char *in, WDL_FastString *out) // modded from  projectcontext
{
  char c;
  if (!in || !*in)
  {
    out->Append("\"\"");
  }
  else if ((c = getConfigStringQuoteChar(in)))
  {
    if (c == ' ')
    {
      out->Append(in);
    }
    else
    {
      out->Append(&c,1);
      out->Append(in);
      out->Append(&c,1);
    }
  }
  else  // ick, change ` into '
  {
    out->Append("`");
    out->Append(in);
    out->Append("`");
    char *p=(char *)out->Get()+1;
    while (*p && p[1])
    {
      if (*p == '`') *p='\'';
      p++;
    }
  }
}

const char *sx_saveState(SX_Instance *sx, int *lOut)
{
  if (!sx) { *lOut = 0; return ""; }
  WDL_FastString &tmp = sx->m_last_cfg;

  tmp.Set("");
  const int np = wdl_max(LEGACY_PRESET_CFG_INDEX, sx->m_sliders.GetSize());
  for (int y = 0; y < np; y ++)
  {
    if (y) tmp.Append(" ");

    effectSlider *slid = sx->m_sliders.Get(y);
    if (slid && slid->is_file_slider)
    {
      if (slid->currentFile)
      {
        const char *ptr=slid->currentFile;
        tmp.Append("\"");
        tmp.Append(ptr);
        tmp.Append("\"");
      }
      else
      {
        tmp.Append("ERR");
      }
    }
    else
    {
      if (!slid)
      {
        tmp.Append("-");
      }
      else
      {
        char buf[4096];
        if (!slid->slider || (slid->slider[0] >= -10000000000000.0 && slid->slider[0] < 1000000000000.0))
        {
          double v = slid->slider ? slid->slider[0] : 0.0;
          snprintf(buf, sizeof(buf), "%lf", v);
          WDL_remove_trailing_decimal_zeros(buf, 0);
        }
        else
        {
          strcpy(buf,"?");
        }
        buf[4095]=0;
        tmp.Append(buf);

      }
    }
    if (y == LEGACY_PRESET_CFG_INDEX-1 && (np > LEGACY_PRESET_CFG_INDEX || sx->m_curpresetname.GetLength()))
    {
      tmp.Append(" ");
      appendEscapedConfigString(sx->m_curpresetname.Get(), &tmp);
    }
  }

  for (int x = 0; x < sx->m_config_items.GetSize(); x ++)
  {
    effectConfigItem *item = sx->m_config_items.Get(x);
    char buf[256];
    snprintf(buf,sizeof(buf)," #CFGSTR:%s=%.10f",item->symbol.Get(),item->cur_val);
    WDL_remove_trailing_decimal_zeros(buf,0);
    tmp.Append(buf);
  }

  *lOut = sx->m_last_cfg.GetLength();
  return sx->m_last_cfg.Get();
}


static WDL_HeapBuf m_serstate;

const char *SX_Instance::SaveSerState(int *buflen)
{
  if (m_serialize_ch && m_need_init && !is_in_gfx())
  {
    m_mutex.Enter();
    on_slider_change();
    m_mutex.Leave();
  }
  m_serstate.SetGranul(256*1024);
  save_serialized_state_tomem(&m_serstate);

  *buflen=m_serstate.GetSize();
  return (char*)m_serstate.Get();
}

void SX_Instance::LoadSerState(const char *buf, int buflen)
{
  if (WDL_NOT_NORMALLY(is_in_gfx()))
  {
    // can't load state if executing gfx
    return;
  }
  if (m_need_init)
  {
    m_mutex.Enter();
    on_slider_change();
    m_mutex.Leave();
  }
  void *wr=m_serstate.ResizeOK(buflen);
  if (wr)
  {
    memcpy(wr,buf,buflen);
    load_serialized_state_frommem(&m_serstate);
  }
}

