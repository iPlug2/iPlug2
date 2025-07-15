/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * IDE
 */

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#else
#include "../WDL/swell/swell.h"
#endif
#include <math.h>
#include "sfxui.h"
#include "../WDL/win32_utf8.h"
#include "../WDL/win32_curses/curses.h"
#undef CURSES_INSTANCE
#define CURSES_INSTANCE ((win32CursesCtx*)m_cursesCtx)
#include "../WDL/win32_curses/curses_editor.h"

#include "../WDL/eel2/ns-eel-int.h"
#include "../WDL/localize/localize.h"

#include "../WDL/wdlutf8.h"
#include "sfxui.h"
#include "sfx_edit.h"
#include "resource.h"


static WDL_StringKeyedArray<const char *> s_regvar_list(false);
static void mkregvarlist()
{
  if (s_regvar_list.GetSize()) return;
  static const char * const list[] = {
     "ext_noinit\0set to 1 in @init to prevent @init from being called again",
     "ext_midi_bus\0set to 1 in @init to enable MIDI bus support (see midi_bus)",
     "ext_nodenorm\0set to 1 in @init to disable anti-denormal noise",
     "ext_tail_size\0set to 0 for infinite tail (default), -1 for no tail (automatically flush on silence, good for compressor envelopes, etc), -2 for no tail (no state flush needed), or >0 for exact tail length in samples",
     "ext_gr_meter\0set to 0 in @init, then update with negative values in @block with current gain reduction. set to 10000.0 to disable GR metering.",
     "midi_bus\0bus index 0..127, set by midi_recv(), used by midi_send()",
     "srate\0[RO] current samplerate",
     "samplesblock\0[RO] current block size in samples",
     "ts_num\0[RO] time signature numerator",
     "ts_denom\0[RO] time signature denominator",
     "pdc_delay\0current delay added by the plug-in which should be compensated",
     "pdc_top_ch\0index+1 of the last channel that should be delay compensated",
     "pdc_bot_ch\0index of first channel which should be delay compensated",
     "pdc_midi\0set to 1 if MIDI output should be delay compensated",
     "num_ch\0[RO] specifies the channel count that will potentially be used",
     "tempo\0[RO] current tempo",
     "play_position\0[RO] current project playback position in seconds",
     "beat_position\0[RO] current project playback position in beats",
     "play_state\0[RO] play state, 0=stopped, 1=playing, 2=paused, 5 recording, 6 record-paused",
     "trigger\0a bitmask of triggers that were clicked by the user.",
     "gfx_r\0current drawing color red 0..1 (@gfx only)",
     "gfx_g\0current drawing color green 0..1 (@gfx only)",
     "gfx_b\0current drawing color blue 0..1 (@gfx only)",
     "gfx_a\0current drawing opacity 0..1 (@gfx only)",
     "gfx_a2\0current drawing color alpha 0..1 (@gfx only)",
     "gfx_w\0[RO] current UI width (@gfx only)",
     "gfx_h\0[RO] current UI height (@gfx only)",
     "gfx_x\0current drawing position X (@gfx only)",
     "gfx_y\0current drawing position Y (@gfx only)",
     "gfx_mode\0current drawing mode, 1=additive blend, etc (@gfx only)",
     "gfx_clear\0set to a non-negative value to clear with 0xRRGGBB each frame (@gfx only)",
     "gfx_texth\0current font text line height (@gfx only)",
     "gfx_dest\0current drawing destination, -1 = default (@gfx only)",
     "gfx_ext_retina\0set to 1 in @init, will be set to >1.0 if current view is hidpi/retina",
     "gfx_ext_flags\0[RO] will be set to 1 if TCP/MCP embedded, 2 if running idle (@gfx only)",
     "mouse_x\0[RO] current mouse position X (@gfx only)",
     "mouse_y\0[RO] current mouse position Y (@gfx only)",
     "mouse_cap\0[RO] current mouse/keyboard modifier state (@gfx only)",
     "mouse_wheel\0mouse wheel accumulated move (clear after reading) (@gfx only)",
     "mouse_hwheel\0mouse horizontal wheel accumulated move (clear after reading) (@gfx only)",
     "#dbg_desc\0set value to update description in main UI for debugging purposes",
  };
  int x;
  for (x = 0; x < (int) (sizeof(list)/sizeof(list[0])); x++)
    s_regvar_list.AddUnsorted(list[x],list[x]+strlen(list[x])+1);
  char tmp[64], tmp2[128];
  for (x=0;x<100;x++)
  {
    snprintf(tmp,sizeof(tmp),"reg%02d",x);
    s_regvar_list.AddUnsorted(tmp,"global register (shared across all JSFX)");
  }
  for (x=0;x<MAX_NCH;x++)
  {
    snprintf(tmp,sizeof(tmp),"spl%d",x);
    s_regvar_list.AddUnsorted(tmp,"current sample in @sample");
  }
  for (x=0;x<NUM_SLIDERS;x++)
  {
    snprintf(tmp,sizeof(tmp),"slider%d",x+1);
    snprintf(tmp2,sizeof(tmp2),"slider %d value",x+1);
    s_regvar_list.AddUnsorted(tmp,strdup(tmp2));
  }

  s_regvar_list.Resort();
}

int SX_Editor::namedTokenHighlight(const char *tokStart, int len, int state)
{
  mkregvarlist();
  if (len < 32)
  {
    char tmp[64];
    lstrcpyn_safe(tmp,tokStart,len+1);
    if (s_regvar_list.Get(tmp)) return SYNTAX_REGVAR;
  }
  return EEL_Editor::namedTokenHighlight(tokStart,len,state);
}


int SX_Editor::updateFile()  // overrides for recompile
{
  int rv = EEL_Editor::updateFile();
  if (rv) return rv;
  WDL_FastString str;
  m_parent->DoRecompile(m_parent->m_hwndwatch, 0 /* preserve everything*/, &str);
  draw_message(str.Get()[0]?str.Get():(char*)"RECOMPILED OK!");

  if (plugin_register) plugin_register("jsfx_namecache_remove", (void*)m_filename.Get());

  return 0;
}

int SX_Editor::GetTabCount()
{
  return m_parent ? m_parent->m_editor_list.GetSize() : 0;
}

WDL_CursesEditor *SX_Editor::GetTab(int idx)
{
  return m_parent ? m_parent->m_editor_list.Get(idx) : NULL;
}

bool SX_Editor::AddTab(const char *fn)
{
  if (!m_parent) return false;

  SX_Editor *e = new SX_Editor(m_parent, CURSES_INSTANCE);
  e->init(fn);
  m_parent->m_editor_list.Add(e);
  m_parent->switchToEditor(m_parent->m_editor_list.GetSize()-1);
  return true;
}
void SX_Editor::CloseCurrentTab()
{
  if (!m_parent) return;

  const int idx=m_parent->m_editor_list.Find(this);
  m_parent->m_editor_list.Delete(idx);
  m_cursesCtx=NULL;
  SwitchTab(idx-1,false);
}

void SX_Editor::SwitchTab(int idx, bool rel)
{
  if (!m_parent) return;
  if (rel)
  {
    int a=m_parent->m_editor_list.Find(this);
    if (a<0) idx=0;
    else idx+=a;
  }

  SX_Editor *ed = m_parent->m_editor_list.Get(idx);
  if (ed && ed != this)
    m_parent->switchToEditor(idx);
}

FILE *scanForEffect(const char *fn, WDL_FastString *actualfn, const char *config_effectdir);

FILE *SX_Editor::tryToFindOrCreateFile(const char *fnp, WDL_FastString *s)
{
  FILE *fp = m_parent ? scanForEffect(fnp,s,m_parent->m_effectdir.Get()) : NULL;
  if (fp) return fp;

  s->Set("");
  if (m_filename.Get()[0])
  {
    s->Set(m_filename.Get());
    s->remove_filepart();
  }
  if (m_parent && !s->GetLength()) s->Set(m_parent->m_effectdir.Get());

  if (s->GetLength())
  {
    s->Append(WDL_DIRCHAR_STR);
    s->Append(fnp);
  }
  return NULL;
}

int SX_Editor::is_code_start_line(const char *p)
{
  if (!p) return 1;

  if (*p != '@') return 0;

  static const char *list=
      "@init\0"
      "@slider\0"
      "@block\0"
      "@sample\0"
      "@serialize\0"
      "@gfx\0"
      ;

  const char *pl = list;
  while (*pl)
  {
    const int pl_sz=strlen(pl);
    if (!strncmp(pl,p,pl_sz) && (!p[pl_sz] || isspace_safe(p[pl_sz]))) return pl_sz;
    pl += pl_sz+1;
  }
  return 0;
}

int SX_Editor::overrideSyntaxDrawingForLine(int *skipcnt, const char **p, int *c_comment_state, int *last_attr)
{
  int amt=0, sidx = get_slider_from_name(*p,':', &amt);
  if (sidx > 0)
  {
    draw_string(skipcnt,*p,amt+1,last_attr,*c_comment_state == STATE_BEFORE_CODE ? SYNTAX_HIGHLIGHT1 : SYNTAX_ERROR);
    *p+=amt+1;
    return -2;
  }
  int a=is_code_start_line(*p);
  if (a)
  {
    draw_string(skipcnt,*p,a,last_attr,SYNTAX_HIGHLIGHT1);
    *p+=a;
    return -1;
  }
  if (**p == '@') return 100;

  static const char *prefix_highlights =
        "desc:\0"
        "options:\0"
        "config:\0"
        "tabsize:\0"
        "filename:\0"
        "in_pin:\0"
        "out_pin:\0"
        "import\0"
        ;
  const char *pfptr=prefix_highlights;

  while (*pfptr)
  {
    a=strlen(pfptr);
    if (!strncmp(*p,pfptr,a) && (pfptr[a-1]==':' || !p[a] || isspace_safe((*p)[a])))
    {
      draw_string(skipcnt,*p,a,last_attr,*c_comment_state == STATE_BEFORE_CODE ? SYNTAX_HIGHLIGHT1 : SYNTAX_ERROR);
      *p+=a;
      return 1;
    }
    pfptr+=a+1;
  }
  return 0;
}


void *SX_Editor::peek_get_VM()
{
  SX_Instance *p = m_parent;
  return p ? p->m_vm : NULL;
}

int SX_Editor::peek_get_named_string_value(const char *name, char *sstr, size_t sstr_sz)
{
  int rv=-1;
  SX_Instance *proc = m_parent;
  if (proc)
  {
    proc->m_string_sermutex.Enter();
    int idx=0;
    const char *p=proc->GetNamedStringValue(name,&idx);
    if (p)
    {
      lstrcpyn_safe(sstr,p,sstr_sz);
      rv=idx;
    }
    proc->m_string_sermutex.Leave();
  }
  return rv;
}

bool SX_Editor::peek_get_numbered_string_value(double idx, char *sstr, size_t sstr_sz)
{
  bool rv=false;
  SX_Instance *proc = m_parent;
  if (proc)
  {
    proc->m_string_sermutex.Enter();
    const char *p=proc->GetStringForIndex(idx,NULL);
    if (p)
    {
      lstrcpyn_safe(sstr,p,sstr_sz);
      rv=true;
    }
    proc->m_string_sermutex.Leave();
  }
  return rv;
}
void SX_Editor::peek_lock()
{
  if (m_parent) m_parent->m_mutex.Enter();
}
void SX_Editor::peek_unlock()
{
  if (m_parent) m_parent->m_mutex.Leave();
}

void SX_Editor::get_suggested_token_names(const char *fname, int chkmask, suggested_matchlist *list)
{
  mkregvarlist();
  for (int x=0;x<s_regvar_list.GetSize();x++)
  {
    const char *k=NULL;
    s_regvar_list.Enumerate(x,&k);
    if (WDL_NORMALLY(k))
    {
      int score = fuzzy_match(fname,k);
      if (score>0) list->add(k,score,suggested_matchlist::MODE_REGVAR);
    }
  }
  EEL_Editor::get_suggested_token_names(fname,chkmask,list);
}

void SX_Editor::on_help(const char *str, int curChar) // curChar is current character if str is NULL
{
#ifdef WDL_IS_FAKE_CURSES
  char buf[512];
  if (!str)
  {
    snprintf(buf,sizeof(buf),"http://www.reaper.fm/sdk/js/search.php?ord=%d",(int)curChar);
    ShellExecute(CURSES_INSTANCE ? CURSES_INSTANCE->m_hwnd : NULL,"open",buf,"","",0);
  }
  else
  {
    snprintf(buf,sizeof(buf),"http://www.reaper.fm/sdk/js/search.php?search=%s",str);
    ShellExecute(CURSES_INSTANCE ? CURSES_INSTANCE->m_hwnd : NULL,"open",buf,"","",0);
  }
#endif
}

void SX_Editor::get_extra_filepos_names(WDL_LogicalSortStringKeyedArray<int> * list, int pass)
{
  if (pass == 1)
  {
    int cnt=0;
    for (int x = 0; x < m_text.GetSize(); x ++)
    {
      if (is_code_start_line(m_text.Get(x)->Get()))
      {
        list->AddUnsorted(m_text.Get(x)->Get(),x);
        cnt++;
      }
    }
    if (cnt)
    {
      WDL_HeapBuf tmp;
      const int recsz = sizeof(list->m_data.Get()[0]);
      if (WDL_NORMALLY(tmp.ResizeOK(cnt * recsz)))
      {
        // shuffle our items to the start of the list
        memcpy(tmp.Get(),list->m_data.Get() + list->m_data.GetSize() - cnt, cnt * recsz);
        memmove(list->m_data.Get() + cnt, list->m_data.Get(), (list->m_data.GetSize() - cnt)*recsz);
        memcpy(list->m_data.Get(), tmp.Get(), cnt*recsz);
      }
    }
  }
}

bool SX_Editor::line_has_openable_file(const char *line, int cursor_pos, char *fnout, size_t fnout_sz)
{
  if (!strncmp(line,"import",6) && isspace_safe(line[6]))
  {
    const char *p=line+6;
    while (*p == ' ') p++;
    lstrcpyn_safe(fnout,p,fnout_sz);
    //remove trailing whitespace
    char *ep = fnout;
    while (*ep) ep++;
    while (ep > fnout && (ep[-1] == '\n' || ep[-1] == '\t' || ep[-1] == ' ')) *--ep = 0;
    if (!*fnout) return false;

    WDL_FastString s;

    FILE *fp=NULL;
    const char *ptr = fnout;
    while (!fp && *ptr)
    {
      // first try same path as loading effect
      if (m_filename.Get()[0])
      {
        s.Set(m_filename.Get());
        const char *sp=s.Get()+s.GetLength();
        while (sp>=s.Get() && *sp != '\\' && *sp != '/') sp--;
        s.SetLen(sp + 1 - s.Get());
        if (s.GetLength())
        {
          s.Append(ptr);
          fp=fopenUTF8(s.Get(),"rb");
        }
      }

      // scan past any / or \\, and try again
      if (!fp)
      {
        while (*ptr && *ptr != '\\' && *ptr != '/') ptr++;
        if (*ptr) ptr++;
      }
    }

    if (!fp) fp = tryToFindOrCreateFile(fnout,&s);
    if (fp) fclose(fp);

    lstrcpyn_safe(fnout,s.Get(),fnout_sz);

    return fnout[0] != 0;
  }
  return false;
}

int SX_Editor::peek_get_token_info(const char *name, char *sstr, size_t sstr_sz, int chkmask, int ignoreline)
{
  int r = EEL_Editor::peek_get_token_info(name,sstr,sstr_sz,chkmask,ignoreline);
  if (m_parent && (chkmask & KEYWORD_MASK_USER_VAR))
  {
    EEL_F *p = sx_effect_var_resolver(m_parent,name);
    if (p)
    {
      snprintf(sstr,sstr_sz,"%s=%.14f",name,*p);
      WDL_remove_trailing_decimal_zeros(sstr,2);
      return KEYWORD_MASK_USER_VAR;
    }
  }
  if (r == KEYWORD_MASK_USER_VAR)
  {
    mkregvarlist();
    const char *desc = s_regvar_list.Get(name);
    if (desc && *desc)
    {
      char *p = (char *)strstr(sstr," ");
      if (p && *name != '#') *p = 0; // don't show mem/string values for regvars
      snprintf_append(sstr,sstr_sz, ": %s", desc);
    }
  }
  else if (r == KEYWORD_MASK_BUILTIN_FUNC)
  {
    // cheat and ask reaper for reascript's version of the help
    const char *p = get_eel_funcdesc ? get_eel_funcdesc(name) : NULL;
    if (p&&*p)
    {
      char buf[512];
      const char* q=strchr(p, '\t');
      if (q) lstrcpyn(buf, p, q-p+1);
      snprintf(sstr,sstr_sz, "%s(%s)%s%s", name, q ? buf : p, q?": ":"",q?q+1:"");
    }
    else
    {
      static const char * const doclist[] =
      {
        "file_open\0index/slider/string\0Open file, return handle, <0 on error",
        "file_close\0handle\0Close file opened with file_open()",
        "file_rewind\0handle\0Rewind to beginning of file",
        "file_var\0handle,var\0Read/write var from handle (or handle=0 for serialization context)",
        "file_string\0handle,str\0Read/write string from handle (or handle=0 for serialization context)",
        "file_mem\0handle,offset,length\0Read/write values from handle (or handle=0 for serialization context)",
        "file_riff\0handle,nch,samplerate\0If the file was a media file, nch/samplerate will be set. In REAPER 6.29+ if the caller sets nch to 'rqsr' and samplerate to a valid sample rate, the file will be resampled to the desired sample rate (this must ONLY be called before any file_var() or file_mem() calls and will change the value returned by file_avail())",
        "file_text\0handle\0If the file was a txt file, returns 1",
        "file_avail\0handle\0Returns items remaining in file if handle in read mode, or <0 if in write mode",
        "sliderchange\0mask or sliderX\0Notify host that slider has been changed without automating",
        "slider_automate\0mask or sliderX[,done=1]\0Notify host that slider is being tweaked by the user. Specify a second parameter of 0 to specify that the tweaking will continue.",
        "slider_show\0mask or sliderX,show\0 0=hide, 1=show, -1 toggle visibility of slider or sliders",
        "strcpy_fromslider\0str,slider\0Gets the filename if a file-slider, or the string if the slider specifies string translations, otherwise gets an empty string",
        "midisend\0offset,msg1,msg2[,msg3]\0Send a MIDI message, if msg3 is omitted high byte of msg2 is used for msg3",
        "midisend_buf\0offset,buf,len\0Send a MIDI message (SysEx or normal) from buf",
        "midirecv_buf\0offset,buf,maxlen\0Receives a MIDI message (SysEx or normal) to buf, returns length or 0 on fail",
        "midirecv\0offset,msg1,msg2[,msg3]\0Receives a MIDI message, if msg3 is omitted high byte of msg2 will be used. Returns 0 on fail",
        "midisyx\0offset,msg,len\0deprecated, use midisend_buf",
        "spl\0channel\0spl(zero) is spl0, spl(1) is spl1, etc",
        "slider\0index\0slider(1) is slider1, etc",
        "slider_next_chg\0slider_index,nextval\0Return sample offset and set nextval to the next value. return <= 0 if no more changes",
        "export_buffer_to_project\0memoffs,len_samples,nch,sr,tidx,flags,tempo,planar_pitch\0see p=1583188. planar_pitch is 6.30+, defaults to 0",
        "midisend_str\0offset, string\0Sends a MIDI message encoded as a string, can be SysEx or regular bytes",
        "midirecv_str\0offset, string\0Receives a MIDI messages into a string, returns nonzero if success",
        "get_host_numchan\0(no parameters)\0Get the number of track or media item take channels",
        "set_host_numchan\0numchan\0Set the number of track or media item take channels. only valid in @gfx section",
        "get_pin_mapping\0inout,pin,startchan,chanmask\0Get a bitfield (maximum 32 bits) representing the channel mappings for this pin",
        "set_pin_mapping\0inout,pin,startchan,chanmask,mapping\0Set the channel mappings for this pin/startchan/chanmask. only valid in @gfx section",
        "get_pinmapper_flags\0(no parameters)\0Get the pinmapper flags for this fx. !&1=pass through unmapped output channels, &1=zero out unmapped output channels",
        "set_pinmapper_flags\0flags\0Set the pinmapper flags for this fx. see get_pinmapper_flags. only valid in @gfx section",
        "get_host_placement\0chain_pos,flags\0Returns track index, or -1 for master track, or -2 for hardware output FX. chain_pos will be position in chain. flags will have 1 set if takeFX, 2 if record input, 4 if in inactive project, 8 if in container (in this case, chain_pos will be set to the address, see TrackFX_GetParamEx etc).",
      };
      int x;
      for (x=0;x<sizeof(doclist)/sizeof(doclist[0]);x++)
      {
        if (!stricmp(doclist[x],name))
        {
          const char *p = doclist[x];
          p += strlen(p)+1;
          if (*p)
          {
            snprintf(sstr,sstr_sz, "%s(%s) %s", name, p, p+strlen(p)+1);
          }
          break;
        }
      }
    }
  }
  return r;
}

#define SetListViewRow(list,l,i,name,val,rc) _SetListViewRow(&m_watch_cache, list,l,i,name,val,rc)
static void _SetListViewRow(WDL_PtrList<char> *prev_val_cache, HWND list, int l, int i, const char *name, const char *val, int rc)
{
  char rcb[64];
  if (rc < 0) rcb[0]=0; else sprintf(rcb,"%d",rc);

  const int cache_idx = i*NUM_WATCHLIST_COLS;
  while (prev_val_cache->GetSize() < cache_idx+NUM_WATCHLIST_COLS)
    prev_val_cache->Add(NULL);

  int chg_mask = 0;
  for (int x = 0; x < NUM_WATCHLIST_COLS; x ++)
  {
    const char *tv = x==0?name : x==1?val : x==2?rcb:"";
    char *cv = prev_val_cache->Get(cache_idx+x);
    if (strcmp(tv?tv:"",cv?cv:""))
    {
      free(cv);
      prev_val_cache->Set(cache_idx+x, tv && *tv ? strdup(tv) : NULL);
      chg_mask|=1<<x;
    }
  }

  if (i>=l)
  {
    LVITEM item={0,};
    item.mask = LVIF_TEXT;
    item.iItem = i;
    item.pszText=(char*)name;
    int a=ListView_InsertItem(list, &item);
    ListView_SetItemText(list,a,1,(char*)val);
    ListView_SetItemText(list,a,2,(char*)rcb);
  }
  else
  {
    if (chg_mask&1) ListView_SetItemText(list,i,0,(char*)name);
    if (chg_mask&2) ListView_SetItemText(list,i,1,(char*)val);
    if (chg_mask&4) ListView_SetItemText(list,i,2,(char*)rcb);
  }
}

static bool do_refs_value_compare(const char *filtstr, int refcnt, const double *varval)
{
  if (strnicmp(filtstr,"refs",4) && strnicmp(filtstr,"value",5)) return false;

  const char *tok;
  int toklen;
  const char *filtstr_end = filtstr+strlen(filtstr);

  if (NULL == (tok=nseel_simple_tokenizer(&filtstr,filtstr_end,&toklen,NULL))) return false;

  if ((toklen == 4 && !strnicmp(tok,"refs",4))||(toklen == 5 && !strnicmp(tok,"value",5)))
  {
    const char *fs_work = filtstr;
    int optl;
    const char *op = nseel_simple_tokenizer(&fs_work,filtstr_end,&optl,NULL);

    if (op)
    {
      while (fs_work < filtstr_end && (*fs_work == '=' || *fs_work == '>' || *fs_work == '<')) fs_work++; // skip over multibyte comparisons
      const char *val = nseel_simple_tokenizer(&fs_work,filtstr_end,&optl,NULL);
      if (val)
      {
        const double vv=atof(val);
        double v;
        if (toklen == 5 && varval) v = *varval;
        else if (toklen == 4) v = refcnt;
        else return false;

        bool match = false;

        switch(*op)
        {
          case '<':
            switch (op[1])
            {
              case '=': match = v<=vv; break;
              case '>': match = v!=vv; break;
              default:  match = v<vv; break;
            }
          break;
          case '>':
            switch (op[1])
            {
              case '=': match = v>=vv; break;
              default:  match = v>vv; break;
            }
          break;
          case '=':
            if (op[1] == '=' && op[2] == '=') match = v!=vv;
            else match = fabs(v-vv)<0.0000000001;
          break;
          case '!':
            if (op[1] == '=')
            {
              if (op[2] == '=') match = v!=vv;
              else match = fabs(v-vv)>=0.0000000001;
            }
          break;
          case '&':
            match = v>0.0 && ((int) v & (int) vv);
          break;
          default:
          break;
        }

        if (match)
          return true;
      }
    }
  }
  return false;
}

#define WDL_HASSTRINGS_EXTRA_PARAMETERS ,int refcnt, const double *varval
#define WDL_HASSTRINGS_PRE_MATCH(n) do_refs_value_compare(n,refcnt,varval)
#define WDL_HASSTRINGS_EXPORT static

#include "../WDL/lineparse.h"
#include "../WDL/has_strings.h"


#ifndef _WIN32
static WDL_PtrList<struct HWND__> s_editorWindows;

static int acProc(MSG *msg, accelerator_register_t *ctx)
{
  if (msg->hwnd)
  {
    const int n = s_editorWindows.GetSize();
    for (int x = 0; x < n; x ++)
    {
      HWND h = s_editorWindows.Get(x);
      if (IsChild(h,msg->hwnd))
      {
        if (msg->hwnd == GetDlgItem(h,IDC_EDIT_RECT))
        {
          SendMessage(msg->hwnd,msg->message,msg->wParam,msg->lParam);
          return 1;
        }
        return 0;
      }
    }
  }
  return 0;
}

static accelerator_register_t accelRec = { acProc, true, };

#endif

WDL_DLGRET SX_Instance::_watchDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == WM_INITDIALOG) SetWindowLongPtr(hwndDlg,GWLP_USERDATA,lParam);
  SX_Instance *_this = (SX_Instance *)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
  return _this->WatchDlgProc(hwndDlg,uMsg,wParam,lParam);
}

WDL_DLGRET SX_Instance::WatchDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static RECT m_lastrect;
  static int m_lastrect_max;
  static int m_nopromptondest;
  static int s_size_offs, s_size_cap;
  switch (uMsg)
  {
    case WM_GETMINMAXINFO:
      {
        LPMINMAXINFO p=(LPMINMAXINFO)lParam;
        p->ptMinTrackSize.x = 300;
        p->ptMinTrackSize.y = 150;
      }
    return 0;
    case WM_INITDIALOG:
      {
#ifndef _WIN32
        if (plugin_register && !s_editorWindows.GetSize()) plugin_register("accelerator",&accelRec);
        if (s_editorWindows.Find(hwndDlg)<0) s_editorWindows.Add(hwndDlg);

        if (Mac_MakeDefaultWindowMenu) Mac_MakeDefaultWindowMenu(hwndDlg);
#endif
        m_hwndwatch=hwndDlg;
        m_watchresize.init(hwndDlg); //
        m_watchresize.init_item(IDC_LIST1,1,0,1,1);
        m_watchresize.init_item(IDC_BUTTON2,1,0,1,0);
        m_watchresize.init_item(IDC_BUTTON3,1,0,1,0);
        m_watchresize.init_item(IDC_BUTTON1,1,1,1,1);
        m_watchresize.init_item(IDC_CHECK2,1,1,1,1);
        m_watchresize.init_item(IDC_EDIT_RECT,0,0,1,1);
        m_watchresize.init_item(IDC_WATCH_FILTER,1,1,1,1);

        int wids[]={
#ifdef _WIN32
          100,82,36
#else
          100,110,20
#endif
        };
        if (get_ini_file)
        {
          wids[0]=GetPrivateProfileInt("jsfx","watch_c1",wids[0],get_ini_file());
          wids[1]=GetPrivateProfileInt("jsfx","watch_c2",wids[1],get_ini_file());
          wids[2]=GetPrivateProfileInt("jsfx","watch_c3",wids[2],get_ini_file());
          WDL_CursesEditor::s_search_mode = GetPrivateProfileInt("jsfx","edit_searchmode",WDL_CursesEditor::s_search_mode,get_ini_file());
        }

        HWND list = GetDlgItem(hwndDlg,IDC_LIST1);
        WDL_UTF8_HookListView(list);
        ListView_SetExtendedListViewStyleEx(list, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
        {
          LVCOLUMN lvc={LVCF_TEXT|LVCF_WIDTH,0,wids[0],(char*)__LOCALIZE("Name","jsfx_ide")};
          ListView_InsertColumn(list,0,&lvc);
        }
        {
          LVCOLUMN lvc={LVCF_TEXT|LVCF_WIDTH,0,wids[1],(char*)__LOCALIZE("Value","jsfx_ide")};
          ListView_InsertColumn(list,1,&lvc);
        }
        {
          LVCOLUMN lvc={LVCF_TEXT|LVCF_WIDTH,0,wids[2],(char*)__LOCALIZE("Refs","jsfx_ide")};
          ListView_InsertColumn(list,2,&lvc);
        }
        HWND h=GetDlgItem(hwndDlg,IDC_EDIT_RECT);
        m_editor_cur=0;
        if (h)
        {
          WDL_FastString s;
          s.Set(m_effectdir.Get());
          s.Append("/");
          s.Append(m_fn.Get());

          m_editor_curses.fontsize_ptr = g_config_fontsize_ptr;

          SX_Editor *ed = new SX_Editor(this,&m_editor_curses);

          ed->init(s.Get());
          ed->draw();

          m_editor_list.Add(ed);

          curses_setWindowContext(h,&m_editor_curses);
          bm_cnt=0;
          bm_enabled=true;
        }

        CheckDlgButton(hwndDlg,IDC_CHECK2,BST_CHECKED);
        SetTimer(hwndDlg,1,500,NULL);
        SetTimer(hwndDlg,2,40,NULL);
        SendMessage(hwndDlg,WM_COMMAND,IDC_BUTTON1,0);
        if (get_ini_file)
        {
          int x=GetPrivateProfileInt("jsfx","watch_lx",-1,get_ini_file());
          int y=GetPrivateProfileInt("jsfx","watch_ly",-1,get_ini_file());
          int w=GetPrivateProfileInt("jsfx","watch_lw",-1,get_ini_file());
          int h=GetPrivateProfileInt("jsfx","watch_lh",-1,get_ini_file());
          if ((x!=-1 || y!=-1) && w > 0 && h >0)
          {
            m_lastrect.left=x;
            m_lastrect.top=y;
            m_lastrect.right=x+w;
            m_lastrect.bottom=y+h;
          }
          m_lastrect_max = GetPrivateProfileInt("jsfx","watch_lmax",0,get_ini_file());
        }
        if (m_lastrect.right > m_lastrect.left && m_lastrect.bottom != m_lastrect.top)
        {
          if (EnsureNotCompletelyOffscreen) EnsureNotCompletelyOffscreen(&m_lastrect);

          SetWindowPos(hwndDlg,NULL,m_lastrect.left,m_lastrect.top,m_lastrect.right-m_lastrect.left,m_lastrect.bottom-m_lastrect.top,SWP_NOZORDER|SWP_NOACTIVATE);
        }
        if (m_lastrect_max)
        {
          ShowWindow(hwndDlg,SW_SHOWMAXIMIZED);
        }
        if (get_ini_file)
        {
          int a = GetPrivateProfileInt("jsfx","watch_divpos",0,get_ini_file());
          if (a>0)
            SendMessage(hwndDlg,WM_USER+200,(WPARAM)a,0);
        }
        SetTimer(hwndDlg,3,1000,NULL);
        if (h) PostMessage(h,WM_SIZE,0,0);
      }
    return 0;
    case WM_LBUTTONDOWN:
    case WM_SETCURSOR:
      {
        POINT p;
        GetCursorPos(&p);
        RECT r;
        GetWindowRect(GetDlgItem(hwndDlg,IDC_LIST1),&r);
        r.right = r.left;
        r.left -= 5;
        if (PtInRect(&r,p))
        {
          if (uMsg == WM_SETCURSOR)
          {
            SetCursor(LoadCursor(NULL,IDC_SIZEWE));
            return -1;
          }
          s_size_offs=p.x - r.right;
          s_size_cap=1;
          SetCapture(hwndDlg);
        }
      }
    break;
    case WM_MOUSEMOVE:
      if (GetCapture()==hwndDlg && s_size_cap)
      {
        RECT r;
        GetClientRect(hwndDlg,&r);
        SendMessage(hwndDlg,WM_USER+200,(WPARAM)(r.right - GET_X_LPARAM(lParam) - s_size_offs),0);
      }
    break;
    case WM_LBUTTONUP:
      if (GetCapture() == hwndDlg)
      {
        ReleaseCapture();
        if (s_size_cap)
        {
          s_size_cap=0;
          if (get_ini_file)
          {
            RECT r;
            GetWindowRect(GetDlgItem(hwndDlg,IDC_LIST1),&r);
            char buf[512];
            sprintf(buf,"%d",(int)(r.right-r.left));
            WritePrivateProfileString("jsfx","watch_divpos",buf,get_ini_file());
          }

        }
      }
    break;
    case WM_TIMER:
      if (wParam==1)
      {
        if (IsDlgButtonChecked(hwndDlg,IDC_CHECK2))
        {
          SendMessage(hwndDlg,WM_COMMAND,IDC_BUTTON1,0);
        }
      }
      else if (wParam==2)
      {
        if (g_config_maxsug_ptr)
        {
          g_eel_editor_max_vis_suggestions = *g_config_maxsug_ptr;
        }
        if (g_config_editflag_ptr)
        {
          g_eel_editor_flags = *g_config_editflag_ptr;
        }
        SX_Editor *e=m_editor_list.Get(m_editor_cur);
        if (!e) e=m_editor_list.Get(m_editor_cur=0);

        if (e) e->RunEditor();
      }
      else if (wParam==3)
      {
        WDL_CursesEditor *e=m_editor_list.Get(m_editor_cur);

        HWND foc = GetFocus();
        if (e && foc && (foc == hwndDlg || IsChild(hwndDlg,foc)))
        {
          time_t lastextmod=0;
          struct stat extstat;
          if (!statUTF8(e->GetFileName(), &extstat))
          {
#ifndef __APPLE__
            lastextmod = extstat.st_mtime;
#else

            lastextmod = extstat.st_mtimespec.tv_sec;
#endif
          }
          time_t editor_time = e->GetLastModTime();
          if (!editor_time) e->SetLastModTime(lastextmod);
          else if (lastextmod && lastextmod != editor_time)
          {
            KillTimer(hwndDlg,2);
            KillTimer(hwndDlg,3);

            WDL_DestroyCheck chk(&e->destroy_check);
            WDL_FastString msg;
            msg.SetFormatted(512,__LOCALIZE_VERFMT("The effect %s has been modified externally.\r\nDo you want to reload it?","jsfx_ide"),
                             WDL_get_filepart(e->GetFileName()));
            if (MessageBox(hwndDlg, msg.Get(), __LOCALIZE("Reload JS Effect?","jsfx_ide"), MB_YESNO) == IDYES)
            {
              if (!chk.isOK()) return 0;
              e->reload_file(false); // keep edit cursor
              // keep running old until save is hit
            }
            else
            {
              if (!chk.isOK()) return 0;
              e->SetDirty();
              e->SetLastModTime(lastextmod);
            }
            e->draw();
            e->setCursor();
            SetTimer(hwndDlg,3,1000,NULL);
            SetTimer(hwndDlg,2,40,NULL);
          }
        }
      }
      else if (wParam == 4)
      {
        KillTimer(hwndDlg,4);
        SendMessage(hwndDlg,WM_COMMAND,IDC_BUTTON1,(LPARAM)GetDlgItem(hwndDlg,IDC_WATCH_FILTER));
      }
    return 0;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_WATCH_FILTER:
          if (LOWORD(wParam) == EN_CHANGE)
          {
            KillTimer(hwndDlg,4);
            SetTimer(hwndDlg,4,100,NULL);
          }
        break;
        case IDC_BUTTON1:
        {
          if (!m_vm)
          {
            m_watchlist.Empty(true);
          }
          else if (lParam != (LPARAM)GetDlgItem(hwndDlg,IDC_WATCH_FILTER) && !is_in_gfx())
          {
            int row_pos=0;
            m_mutex.Enter();
            compileContext *ctx=(compileContext *)m_vm;

            int wb;
            const int ni = EEL_GROWBUF_GET_SIZE(&ctx->varNameList);
            varNameRec **hh = EEL_GROWBUF_GET(&ctx->varNameList);
            for (wb = 0; wb < ni; wb ++)
            {
              varNameRec *h = hh[wb];
              SetNameValPair(row_pos++,h->str, h->value[0], h->refcnt,h->isreg?WATCH_ISREG_SYSTEM:WATCH_ISREG_NORMAL,h->value,NULL);
            }

            m_mutex.Leave();

            int in_pins = m_in_pinnames.GetSize();
            if (!in_pins && !m_has_no_inputs) in_pins = m_last_nch;
            int out_pins = m_out_pinnames.GetSize();
            if (!out_pins && !m_has_no_outputs) out_pins = m_last_nch;

            int maxs = wdl_max(in_pins,out_pins);
            if (maxs > MAX_NCH) maxs=MAX_NCH;
            for (int x = 0; x < maxs; x ++)
            {
              char tmp[64];
              snprintf(tmp,sizeof(tmp),"spl%d",x);
              SetNameValPair(row_pos++,tmp, m_spls[x], -1,WATCH_ISREG_SYSTEM,m_spls,NULL);
            }

            {
              NSEEL_HOSTSTUB_EnterMutex();
              nseel_globalVarItem *p = nseel_globalreg_list;
              while (p)
              {
                char tmp[512];
                snprintf(tmp,sizeof(tmp),"_global.%s",p->name);
                SetNameValPair(row_pos++,tmp,p->data,-1,WATCH_ISREG_GLOBAL,&p->data,NULL);
                p=p->_next;
              }
              NSEEL_HOSTSTUB_LeaveMutex();
            }

            // enumerate (isreg=5) named strings
            // enumerate (isreg=6) 0-63
            m_string_sermutex.Enter();
            int x;
            for (x=0;x<1024;x++)
            {
              const char *name=NULL;
              const char *ptr = EnumerateNamedStrings(x,&name);
              if (ptr && name)
                SetNameValPair(row_pos++,name,0.0,0,WATCH_ISREG_NAMEDSTRING,NULL,ptr);
              else break;
            }

            SetNameValPair(row_pos++,"#dbg_desc",0.0,0,WATCH_ISREG_SYSTEM,NULL,m_description.Get());
            for(x=0;x<64;x++)
            {
              const char *ptr=GetStringForIndex(x,NULL);
              if (ptr && *ptr)
              {
                char buf[64];
                sprintf(buf,"%d",x);
                SetNameValPair(row_pos++,buf,0.0,0,WATCH_ISREG_STRINGINDEX,NULL,ptr);
              }
            }

            m_string_sermutex.Leave();

            while (m_watchlist.GetSize()>row_pos)
            {
              m_watchlist.Delete(m_watchlist.GetSize()-1, true);
            }
            qsort(m_watchlist.GetList(),m_watchlist.GetSize(),sizeof(void *),watchlistsortfunc);
          }

          char filtstr[512];
          LineParser filtlp;
          filtstr[0]=0;
          GetDlgItemText(hwndDlg,IDC_WATCH_FILTER,filtstr,sizeof(filtstr));
          if (filtstr[0]) WDL_makeSearchFilter(filtstr,&filtlp);

          HWND list = GetDlgItem(hwndDlg, IDC_LIST1);
          int l=ListView_GetItemCount(list);
          int outcnt=0;
          int i;
          int hadreg=0;
          for(i=0;i<m_watchlist.GetSize();i++)
          {
            watchNameValue *p = m_watchlist.Get(i);
            const int isreg = p->isreg;
            int use_refcnt = p->refcnt;

            char buf[512];
            const char *use_buf = buf;
            if (isreg == WATCH_ISREG_NAMEDSTRING ||
                isreg == WATCH_ISREG_STRINGINDEX ||
                (isreg == WATCH_ISREG_SYSTEM && p->name[0] == '#'))
            {
              use_buf = p->str_val.Get();
              use_refcnt=-1;
            }
            else
            {
              snprintf(buf,sizeof(buf),"%.14f",p->val);
              WDL_remove_trailing_decimal_zeros(buf,0);
              if (0)
              {
                // useful for debugging generated output
                snprintf_append(buf,sizeof(buf)," - %p",p->valptr);
              }
            }

            const char *srchp[2] = { p->name,use_buf };
            if (WDL_hasStringsEx2(srchp,2,&filtlp,use_refcnt, use_buf == buf ? &p->val : NULL))
            {
              if (hadreg < isreg)
              {
                hadreg=isreg;
                if (outcnt) SetListViewRow(list,l,outcnt++,"","",-1);
                if (isreg == WATCH_ISREG_NAMEDSTRING)
                  SetListViewRow(list,l,outcnt++,"Named strings:","--------------",-1);
                else if (isreg == WATCH_ISREG_STRINGINDEX)
                  SetListViewRow(list,l,outcnt++,"Strings 0-63:","--------------",-1);
                else if (isreg == WATCH_ISREG_SYSTEM)
                  SetListViewRow(list,l,outcnt++,"System variables:","--------------",-1);
                else if (isreg == WATCH_ISREG_GLOBAL)
                  SetListViewRow(list,l,outcnt++,"Globals:","--------------",-1);
              }

              SetListViewRow(list,l,outcnt++,p->name,use_buf,use_refcnt);
            }
          }
          {
            char buf[512];
            int *stats;
            int didhdr=0;

#define STATS_DISP_SUB(a) { const char *srchp[2] = { (a), buf }; if (WDL_hasStringsEx2(srchp,2,&filtlp,-1,NULL)) { \
            if (!didhdr++) { \
                if (outcnt) SetListViewRow(list,l,outcnt++,"","",-1); \
                SetListViewRow(list,l,outcnt++,"Stats:","-----------",-1); \
            } \
             SetListViewRow(list,l,outcnt++,a,buf,-1);  \
            } }

  #define STATS_DISP(x,y) \
            stats=get_##x##_code_stats(); \
            if (stats) { \
              sprintf(buf,"%d",*stats++); STATS_DISP_SUB("@" #y "-src") \
              sprintf(buf,"%d+%d",*stats,stats[1]); stats+=2; STATS_DISP_SUB("@" #y "-code") \
              sprintf(buf,"%d",*stats++); STATS_DISP_SUB("@" #y "-data") \
            }
            STATS_DISP(init,init)
            STATS_DISP(slider,slider)
            STATS_DISP(sample,sample)
            STATS_DISP(onblock,block)
            STATS_DISP(gfx,gfx)

            // todo: common code list

            snprintf(buf,sizeof(buf),"%.8f",100.0 * bm_usetime / (bm_lastendtime-bm_starttime));
            STATS_DISP_SUB("@spl%");
            snprintf(buf,sizeof(buf),"%.8f",bm_usetime / (double)bm_cnt);
            STATS_DISP_SUB("@splAvg");
          }
#undef STATS_DISP
#undef STATS_DISP_SUB
          while (l > outcnt)
          {
            --l;
            ListView_DeleteItem(list,l);
          }
          while (m_watch_cache.GetSize() > outcnt*NUM_WATCHLIST_COLS)
          {
            m_watch_cache.Delete(m_watch_cache.GetSize()-1,true,free);
          }

          // repopulate list
        }
        return 0;

        case IDC_BUTTON2:
          DoRecompile(hwndDlg,LOADEFFECT_RESETFLAG_CONFIGITEMS|LOADEFFECT_RESETFLAG_VARS|LOADEFFECT_RESETFLAG_SLIDERS);
        return 0;

        case IDC_BUTTON3:
          OpenExternal(hwndDlg);
        return 0;

        case IDCANCEL:
          PostMessage(hwndDlg,WM_CLOSE,0,0);
        return 0;
      }
    return 0;

    case WM_USER+200:
      {
        RECT r;
        GetClientRect(hwndDlg,&r);
        int widpos = (int)wParam;
        if (widpos < 80) widpos=80;
        if (widpos > r.right-80) widpos=r.right-80;

        GetWindowRect(GetDlgItem(hwndDlg,IDC_LIST1),&r);
        WDL_WndSizer__rec *r1 = m_watchresize.get_item(IDC_LIST1);
        WDL_WndSizer__rec *edit1 = m_watchresize.get_item(IDC_WATCH_FILTER);

        widpos = m_watchresize.dpi_to_sizer(widpos);
        int curw = m_watchresize.dpi_to_sizer(r.right-r.left);

        if (widpos != curw && r1)
        {
          r1->orig=r1->real_orig;
          r1->orig.left = r1->orig.right - widpos;
          int dx = r1->orig.left - r1->real_orig.left;
          int x;

          WDL_WndSizer__rec *r2 = m_watchresize.get_item(IDC_EDIT_RECT);
          WDL_WndSizer__rec *p;
          for(x=0;(p=m_watchresize.get_itembyindex(x));x++)
          {
            if (p == r1)
            {
            }
            else if (p==edit1)
            {
              p->orig.left=r1->orig.left;
            }
            else if (p==r2)
            {
              p->orig.right = p->real_orig.right + dx;
            }
            else
            {
              p->orig = p->real_orig;
              p->orig.right += dx;
              p->orig.left += dx;
            }
          }
          if (!lParam) m_watchresize.onResize();
        }
      }
    break;
    case WM_SIZE:
      if (wParam != SIZE_MINIMIZED)
      {
        RECT r;
        GetWindowRect(GetDlgItem(hwndDlg,IDC_LIST1),&r);
        SendMessage(hwndDlg,WM_USER+200,r.right-r.left,1);
        m_watchresize.onResize();
      }
    return 0;

    case WM_CLOSE:
      {
        int x;
        for (x=0;x<m_editor_list.GetSize();x++)
        {
          SX_Editor *e = m_editor_list.Get(x);
          if (e && e->IsDirty())
          {
            char s[2000];
            snprintf(s,sizeof(s), __LOCALIZE_VERFMT("Save unsaved changes to:\r\n\r\n%s\r\n\r\nbefore closing?","jsfx_ide"),
               e->GetFileName());

            WDL_DestroyCheck chk(&e->destroy_check);
            int a=MessageBox(m_hwnd,s,__LOCALIZE("Save JS modifications?","jsfx_ide"),MB_YESNOCANCEL);
            if (!chk.isOK()) return 0;
            if (a==IDCANCEL) return 1;
            if (a==IDYES)
            {
              e->updateFile();
            }
          }
        }
      }
      m_nopromptondest++;
      DestroyWindow(hwndDlg);
      m_nopromptondest--;
    return 0;

    case WM_DESTROY:

      {
#ifdef _WIN32
      WINDOWPLACEMENT wp={sizeof(wp)};
      GetWindowPlacement(hwndDlg,&wp);
      m_lastrect_max = (wp.showCmd == SW_SHOWMAXIMIZED);
      m_lastrect = wp.rcNormalPosition;
#else
      GetWindowRect(hwndDlg,&m_lastrect);
#endif
      }
      if (get_ini_file)
      {
        char buf[512];
        sprintf(buf,"%d",m_lastrect_max);
        WritePrivateProfileString("jsfx","watch_lmax",buf,get_ini_file());
        sprintf(buf,"%d",(int)m_lastrect.left);
        WritePrivateProfileString("jsfx","watch_lx",buf,get_ini_file());
        sprintf(buf,"%d",(int)wdl_min(m_lastrect.top,m_lastrect.bottom));
        WritePrivateProfileString("jsfx","watch_ly",buf,get_ini_file());
        sprintf(buf,"%d",(int)(m_lastrect.right-m_lastrect.left));
        WritePrivateProfileString("jsfx","watch_lw",buf,get_ini_file());
        sprintf(buf,"%d",m_lastrect.bottom>  m_lastrect.top ? (int)(m_lastrect.bottom-m_lastrect.top) : (int)(m_lastrect.top-m_lastrect.bottom));
        WritePrivateProfileString("jsfx","watch_lh",buf,get_ini_file());


        HWND hlist=GetDlgItem(hwndDlg,IDC_LIST1);
        sprintf(buf,"%d",ListView_GetColumnWidth(hlist, 0));
        WritePrivateProfileString("jsfx","watch_c1",buf,get_ini_file());
        sprintf(buf,"%d",ListView_GetColumnWidth(hlist, 1));
        WritePrivateProfileString("jsfx","watch_c2",buf,get_ini_file());
        sprintf(buf,"%d",ListView_GetColumnWidth(hlist, 2));
        WritePrivateProfileString("jsfx","watch_c3",buf,get_ini_file());
        if (g_config_fontsize_ptr)
        {
          snprintf(buf,sizeof(buf),"%d",*g_config_fontsize_ptr);
          WritePrivateProfileString("reaper","edit_fontsize",buf,get_ini_file());
        }
        sprintf(buf,"%d",WDL_CursesEditor::s_search_mode);
        WritePrivateProfileString("jsfx","edit_searchmode",buf,get_ini_file());
      }

      if (!m_nopromptondest)
      {
        int x;
        for (x=0;x<m_editor_list.GetSize();x++)
        {
          SX_Editor *e = m_editor_list.Get(x);
          if (e && e->IsDirty())
          {
            WDL_FastString s;
            s.Set("Save unsaved changes to:\r\n\r\n");
            s.Append(e->GetFileName());
            s.Append("\r\n\r\nbefore closing?");
            WDL_DestroyCheck chk(&e->destroy_check);
            if (MessageBox(hwndDlg,s.Get(),"Save JS modifications?",MB_YESNO)==IDYES)
            {
              if (!chk.isOK()) return 0;
              e->updateFile();
            }
            if (!chk.isOK()) return 0;
          }
        }
      }
      m_editor_list.Empty(true);
      m_watchresize.init(NULL);
      m_hwndwatch=0;
      m_watch_cache.Empty(true,free);
      bm_enabled=false;
      #ifndef _WIN32
      s_editorWindows.DeletePtr(hwndDlg);
      if (plugin_register && !s_editorWindows.GetSize()) plugin_register("-accelerator",&accelRec);
      #endif
    return 0;
  }
  return 0;
}

void SX_Instance::switchToEditor(int which)
{
  SX_Editor *e = m_editor_list.Get(m_editor_cur);
  if (e) e->m_cursesCtx=NULL;

  if (which<0) which=0;
  else if (which >= m_editor_list.GetSize()) which=m_editor_list.GetSize()-1;

  m_editor_cur=which;
  e = m_editor_list.Get(which);
  m_editor_curses.user_data = e;
  if (e)
  {
    e->m_top_margin=m_editor_list.GetSize()>1 ? 2 : 1;
    e->m_cursesCtx=&m_editor_curses;
    e->draw();
    e->setCursor();
  }
}
