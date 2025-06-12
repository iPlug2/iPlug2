/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * ReaJS VST2 plug-in, requires VST2 SDK to compile
 *
 * currently only functional on Windows, other platforms would likely take some more work
 */

#ifdef _WIN32
  #include <windows.h>
  #include <windowsx.h>
  #include <commctrl.h>
#else
  #include "../../WDL/swell/swell.h"
  #include "../../WDL/swell/swell-dlggen.h"
#endif

#include <math.h>
#include <stdio.h>
#include "../../jmde/aeffectx.h"
#include "../../WDL/mutex.h"
#include "../../WDL/queue.h"

#include "resource.h"

#include "../../WDL/lice/lice.h"
#include "../../WDL/wdltypes.h"
#include "../../WDL/dirscan.h"
#include "../../WDL/wingui/wndsize.h"

#define VENDOR "Cockos"
#define EFFECT_NAME "ReaJS"

#define SAMPLETYPE float

#include "../sfxui.h"
#include "../../WDL/mergesort.h"
#include "../../WDL/wdlcstring.h"

// todo: pin mappings?


#ifndef _WIN32
extern HWND curses_ControlCreator(HWND parent, const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h);
#endif


/* reajs.ini (named after dll but ini instead of dll)
 all fields are optional
 [ReaJS]
 rootpath=JS ; can be absolute path, relative path (relative to reajs.dll), or begin with a \ to be relative to current drive -- note this isnt the path to effects directly, but a dir that should have data\ and effects\ subdirectories... the default is simply "JS" (if reaper isnt installed)
 appendname=/blah  ; makes effect called "ReaJS/blah"
 vstid=unique32bitnumber ; default is CCONST('rejs') but if your host demands it you can pick your own
 defaulteffect=someeffectname ; loads this effect (without path!) by default
 ; defaulteffect=!someeffectname ; loads this effect, and doesnt allow user to switch to other effects!
 vstcat=1   ; 1 = effect, 2= "synth", others, see vst sdk
 preventedit=0 ; set to 1 to disable editing ui
 inputs=2    ; number of audio inputs (0-64)
 outputs=2    ; number of audio outputs (0-64)
 midiflags=3  ; 0 = no midi send/recv, 1=recv midi from host, 2=send midi to host, 3=both

 */

const char *escapeAmpersandsForMenu(const char *in, char *buf, int bufsz) // converts & to && on all platforms, returns either in or buf
{
  const char *oin=in;
  if (!in || !buf || bufsz<1) return oin;

  int nc=0;
  int opos=0;
  while (*in && opos < bufsz - 1)
  {
    if (*in == '&')
    {
      if (opos >= bufsz-2) break;
      buf[opos++] = '&'; nc++;
    }
    buf[opos++] = *in++;
  }
  if (!nc) return oin;

  buf[opos]=0;
  return buf;
}
void InsertMenuItemFilter(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi)
{
  if (mi && (mi->fMask & MIIM_TYPE) && mi->fType == MFT_STRING && mi->dwTypeData && strstr(mi->dwTypeData,"&"))
  {
    MENUITEMINFO m = *mi;
    char tmp[2048];
    m.dwTypeData = (char *)escapeAmpersandsForMenu(mi->dwTypeData,tmp,sizeof(tmp));
    InsertMenuItem(hMenu,pos,byPos,&m);
  }
  else
  {
    InsertMenuItem(hMenu,pos,byPos,mi);
  }
}


WDL_FastString g_effect_name;
int g_num_inputs=2, g_num_outputs=2;
WDL_FastString g_root_path;
char g_default_effect[256]; // if [0] == '!' then disable switching
int g_unique_id = CCONST('r','e','j','s');
bool g_allow_edit=true;
int g_midi_flags=3; // &1 = support recv midi from host, &2=support send midi to host
int g_vst_category = kPlugCategEffect;
int g_num_params = NUM_SLIDERS;

#include "../../WDL/db2val.h"


static int midiTimeEventCompare(const void *a, const void *b)
{
  VstEvent *aa=*(VstEvent **)a;
  VstEvent *bb=*(VstEvent **)b;
  return aa->deltaFrames - bb->deltaFrames;
}

int g_refcnt;

HINSTANCE g_hInst;

class VSTEffectClass
{
public:
  VSTEffectClass(audioMasterCallback cb)
  {
    memset(&m_auto_states,0,sizeof(m_auto_states));
    m_cb=cb;
    m_pelist=0;
    m_pelist_rdpos=0;
    m_last_w=-1;
    m_last_h=-1;
    m_sxinst=0;
    if (!g_refcnt++)
    {
    }
    memset(&m_effect,0,sizeof(m_effect));
    m_samplerate=44100.0;
    m_hwndcfg=0;
    m_sxhwnd=0;
    m_effect.magic = kEffectMagic;
    m_effect.dispatcher = staticDispatcher;
    m_effect.process = staticProcess;
    m_effect.getParameter = staticGetParameter;
    m_effect.setParameter = staticSetParameter;
    m_effect.numPrograms=1;
    m_effect.numParams = g_num_params;
    m_effect.numInputs=g_num_inputs;
    m_effect.numOutputs=g_num_outputs;
    m_effect.uniqueID=g_unique_id;
    m_effect.version=1100;
    m_effect.flags=effFlagsCanReplacing|effFlagsHasEditor|effFlagsProgramChunks;
    m_effect.processReplacing=staticProcessReplacing;
    m_effect.object=this;
    m_effect.ioRatio=1.0;

    onParmChange();

    if (g_default_effect[g_default_effect[0]=='!'])
      Open(g_default_effect+(g_default_effect[0]=='!'));

    Reset();
  }

  void Open(const char *name)
  {
    if (name && name != m_sxname.Get()) m_sxname.Set(name);;
    SX_Instance *newinst = sx_createInstance(g_root_path.Get(),m_sxname.Get(),NULL);

    {
      WDL_MutexLock m(&m_mutex);
      Close();
      m_sxinst = newinst;

      if (m_sxinst)
      {
        sx_updateHostNch(m_sxinst,-1);
        sx_set_midi_ctx(m_sxinst,midi_sendrecv,this);
        sx_set_host_ctx(m_sxinst, this, NULL);
      }
    }

    if (m_hwndcfg)
    {
      SendMessage(m_hwndcfg,WM_USER+1000,0,0);
    }

  }

  void Close()
  {
    if (m_sxinst)
    {
      sx_deleteUI(m_sxinst);
      sx_destroyInstance(m_sxinst);
      m_sxinst=0;
      m_sxhwnd=0;
    }
  }

  ~VSTEffectClass()
  {
    m_midioutqueue.Empty(true,free);

    Close();
    if (m_hwndcfg) DestroyWindow(m_hwndcfg);

    if (!--g_refcnt)
    {
      free(nseel_gmembuf_default);
      nseel_gmembuf_default=0;
    }
  }

  void Reset()
  {
    WDL_MutexLock m(&m_mutex);
    m_effect.initialDelay=0;
    if (m_sxinst)
    {
      sx_processSamples(m_sxinst, NULL, 0, 0, 0, 0.0, 0, 0, -1, 0, 0, 1.0, 1.0, 0);
    }
  }

  void onParmChange()
  {
    WDL_MutexLock m(&m_mutex);


  }
  WDL_Queue m_cfgchunk;
  int GetChunk(void **ptr)
  {
    if (!m_sxinst)
    {
      *ptr=m_cfgchunk.Get();
      return m_cfgchunk.GetSize();
    }

    static WDL_Queue chunk; // override project

    chunk.Clear();
    chunk.Add(m_sxname.Get(),strlen(m_sxname.Get())+1);

    int l = 0;
    const char *ret = sx_saveState(m_sxinst,&l);
    if (ret) chunk.Add(ret,l+1);
    else chunk.Add("",1);

    int a=0;
    const char *p=sx_saveSerState(m_sxinst,&a);
    if (!p) a=0;
    chunk.AddToLE(&a);
    if (a>0) chunk.Add(p,a);

    *ptr=chunk.Get();
    return chunk.GetSize();
  }

  void SetChunk(void *ptr, int size)
  {
    // if unable to load set to m_cfgchunk

    WDL_MutexLock m(&m_mutex);
    Close();

    char *p=(char *)ptr;
    while (p < (char*)ptr + size && *p) p++;
    if (p < (char*)ptr + size)
    {
      m_sxname.Set((char *)ptr);
      m_sxinst = sx_createInstance(g_root_path.Get(),m_sxname.Get(),NULL);
      if (m_sxinst)
      {
        sx_updateHostNch(m_sxinst,-1);
        sx_set_midi_ctx(m_sxinst,midi_sendrecv,this);
        sx_set_host_ctx(m_sxinst, this, NULL);
      }
      char *cfgstr=++p;
      while (p < (char*)ptr + size && *p) p++;

      if (m_sxinst && p < (char*)ptr + size)
      {
        if (*cfgstr) sx_loadState(m_sxinst,cfgstr);
        unsigned char *t=(unsigned char *)++p;

        if ((p+=4)<=(char*)ptr+size)
        {
          int sz=t[0] | (t[1]<<8) | (t[2]<<16) | (t[3]<<24);
          if (p+sz <= (char*)ptr + size)
          {
            sx_loadSerState(m_sxinst,p,sz);
          }
        }
      }
    }
    else m_sxname.Set("");

    m_cfgchunk.Clear();
    if (!m_sxinst)
    {
      m_cfgchunk.Add(ptr,size);
    }

    if (m_hwndcfg)
    {
      SendMessage(m_hwndcfg,WM_USER+1000,0,0);
    }
  }

  WDL_PtrList<VstEvent> m_midioutqueue;

  void PassThruEvent(VstEvent *evt)
  {
    void *buf;
    if (evt->type == kVstSysExType)
    {
      const VstMidiSysexEvent *src = (VstMidiSysexEvent*)evt;
      buf = malloc(src->byteSize + src->dumpBytes);
      VstMidiSysexEvent *e = (VstMidiSysexEvent*)buf;
      memcpy(e,src,src->byteSize);
      e->sysexDump = (char *)e + e->byteSize;
      memcpy(e->sysexDump,src->sysexDump,src->dumpBytes);
    }
    else
    {
      buf = malloc(evt->byteSize);
      memcpy(buf,evt,evt->byteSize);
    }

    m_midioutqueue.Add((VstEvent *)buf);
  }

  double midi_sendrecvFunc(int action, double *ts, double *msg1, double *msg23)
  {
    if (action<0)
    {
      if (m_pelist) while (m_pelist_rdpos < m_pelist->numEvents)
      {
        VstEvent *evt=m_pelist->events[m_pelist_rdpos++];
        if (evt)
        {
          if (evt->type == kVstMidiType && evt->byteSize>=24)
          {
            VstMidiEvent *e = (VstMidiEvent *)evt;

            *ts = (double) e->deltaFrames;
            unsigned char *md = (unsigned char *)e->midiData;
            *msg1 = (double) md[0];
            *msg23 = (double) ((int)md[1] + ((int)md[2] << 8));
            return 1.0;
          }
          PassThruEvent(evt);
        }
      }
      if (!m_pelist_rdpos) m_pelist_rdpos++;
    }
    else if (action == 1) // 3-byte send
    {
      int fo=(int)*ts;
      if (fo<0)fo=0;

      VstMidiEvent *evt=(VstMidiEvent *)malloc(sizeof(VstMidiEvent));
      memset(evt,0,sizeof(VstMidiEvent));
      evt->type = kVstMidiType;
      evt->byteSize = 24;
      evt->deltaFrames=fo;

      int m=(int)*msg1;
      if (m<0x80)m=0x80;
      else if (m > 0xff) m=0xff;
      evt->midiData[0]=(unsigned char) m;
      m=(int)(*msg23)&0xff;
      evt->midiData[1]=(unsigned char) m;
      m=(((int)(*msg23))>>8)&0xff;
      evt->midiData[2]=(unsigned char) m;

      m_midioutqueue.Add((VstEvent *)evt);
      return 1.0;
    }
    else if (action == 2) // sysex
    {
      int fo=(int)*ts;
      if (fo<0)fo=0;

      int len=(int)*msg23;
      if (len < 0) len = 0;

      int offs=0;
      if (len >= 2 && (((int)msg1[0])&0xFF) == 0xF0 && (((int)msg1[len-1])&0xFF) == 0xF7) // we'll add this
      {
        ++offs;
        len -= 2;
      }

      VstMidiSysexEvent* evt=(VstMidiSysexEvent*)malloc(sizeof(VstMidiSysexEvent)+len+2);
      memset(evt, 0, sizeof(VstMidiSysexEvent)+len+2);
      unsigned char* syx=(unsigned char*)evt+sizeof(VstMidiSysexEvent);

      evt->type=kVstSysExType;
      evt->deltaFrames=fo;
      evt->byteSize = sizeof(VstMidiSysexEvent);
      evt->sysexDump=(char*)syx;
      evt->dumpBytes=len+2;

      syx[0]=0xF0;
      int i;
      for (i=0; i < len; ++i) syx[i+1]=(((int)msg1[i+offs])&0xFF);
      syx[len+1]=0xF7;

      m_midioutqueue.Add((VstEvent*)evt);
      return 1.0;
    }

    return 0.0;
  }


  static double midi_sendrecv(void *ctx, int action, double *ts, double *msg1, double *msg23, double *midibus)
  {
    VSTEffectClass *_this = (VSTEffectClass*)ctx;
    return _this->midi_sendrecvFunc(action,ts,msg1,msg23);
  }

  WDL_TypedBuf<EEL_F> m_interleavebuf;
  void ProcessEvents(VstEvents *eventlist)
  {
    if (eventlist && eventlist->numEvents)
      m_pelist=eventlist;
  }

  void ProcessSamples(SAMPLETYPE **inputs, SAMPLETYPE **outputs, VstInt32 sampleframes)
  {
    m_mutex.Enter();

    m_pelist_rdpos=0;
    m_midioutqueue.Empty(true,free);

    {
      int int_nch=max(g_num_inputs,g_num_outputs);
      EEL_F *buf = m_interleavebuf.Resize(int_nch*sampleframes,false);
      // todo: pin management
      int x;
      for(x=0;x<sampleframes;x++)
      {
        int i;
        for(i=0;i<g_num_inputs; i++) *buf++ = inputs[i][x];
        for(;i<int_nch; i++) *buf++ = 0.0;
      }
      buf = m_interleavebuf.Get();

      // todo: state/time sig querying
      if (m_sxinst)
      {
        double tempo=120.0;
        int transport_state=1;
        double pos_beats=0.0;
        double pos_sec=0.0;
        int tsnum=0;
        int tsdenom=0;
        if (m_cb)
        {
          VstTimeInfo *ti = (VstTimeInfo *)m_cb(&m_effect,audioMasterGetTime,0,(kVstAutomationWriting-1) | kVstPpqPosValid | kVstTempoValid, NULL, 0.0f);
          if (ti)
          {
            if (ti->flags & kVstPpqPosValid) pos_beats = ti->ppqPos;
            pos_sec = ti->samplePos / ti->sampleRate;
            transport_state=0;
            if (ti->flags & kVstTransportPlaying) transport_state=1;
            if (ti->flags & kVstTransportRecording) transport_state|=4;
            if (ti->flags & kVstTempoValid) tempo = ti->tempo;
            if (ti->flags & kVstTimeSigValid)
            {
              tsnum = ti->timeSigNumerator;
              tsdenom = ti->timeSigDenominator;
            }
          }

        }
        sx_processSamples(m_sxinst, buf, sampleframes, int_nch,(int)(m_samplerate+0.5),
          tempo,tsnum,tsdenom,transport_state,pos_sec,pos_beats,1.0,1.0,0);
        m_effect.initialDelay=sx_getCurrentLatency(m_sxinst);
      }
      else
        m_effect.initialDelay=0;

      for(x=0;x<sampleframes;x++)
      {
        int i;
        for(i=0;i<g_num_outputs; i++) outputs[i][x]=(float) *buf++;
        buf += int_nch - g_num_outputs; // skip other outputs
      }
    }

    // passthru any leftover events
    if (m_pelist)
    {
      while (m_pelist_rdpos < m_pelist->numEvents)
      {
        VstEvent *evt=m_pelist->events[m_pelist_rdpos++];
        if (evt)
        {
          PassThruEvent(evt);
        }
      }
    }
    m_pelist=0;

    // send events
    if (g_midi_flags&2)
    {
      VstEvents *list = (VstEvents *)m_outlistbuf.Resize(sizeof(VstEvents) + m_midioutqueue.GetSize()*sizeof(void*),false);

      list->numEvents = m_midioutqueue.GetSize();
      list->reserved =0 ;

      int x;
      int lv=-1;
      bool needsort=false;

      for(x=0;x<list->numEvents;x++)
      {
        VstEvent *evt = list->events[x]=m_midioutqueue.Get(x);
        int df=evt->deltaFrames;
        if (df >= sampleframes)
          df=evt->deltaFrames=sampleframes-1;

        if (df<lv) needsort=true;
        lv=df;
      }

      if (needsort)
        WDL_mergesort(list->events,list->numEvents,sizeof(void *),midiTimeEventCompare,(char*)m_sortbuf.Resize(list->numEvents*sizeof(void *),false));

      m_cb(&m_effect,audioMasterProcessEvents,0,0,list,0.0f);
    }

    m_mutex.Leave();
  }
  WDL_HeapBuf m_outlistbuf;
  WDL_HeapBuf m_sortbuf;

  HMENU CreateMenuForPath(const char *path, WDL_PtrList<char> *menufns)
  {
    HMENU hMenu=NULL;

    int subpos=0;
    int itempos=0;

    WDL_DirScan ds;
    if (!ds.First(path))
    {
      do
      {
        const char *fn=ds.GetCurrentFN();
        if (fn[0] != '.')
        {
          if (ds.GetCurrentIsDirectory())
          {
            WDL_FastString tmp(path);
            tmp.Append("/");
            tmp.Append(fn);
            HMENU sub=CreateMenuForPath(tmp.Get(),menufns);
            if (sub)
            {
              if (!hMenu) hMenu=CreatePopupMenu();
              MENUITEMINFO mii={sizeof(mii),MIIM_SUBMENU|MIIM_TYPE|MIIM_STATE,MFT_STRING,MFS_ENABLED,0,sub,0,0,0,(char*)fn};
              InsertMenuItemFilter(hMenu,subpos++,TRUE,&mii);
            }
          }
          else
          {
            const char *ext=fn;
            while (*ext) ext++;
            while (ext > fn && *ext != '.') ext--;
            if (ext==fn || (stricmp(ext,".png") &&
                stricmp(ext,".dll") &&
                stricmp(ext,".jsfx-inc") &&
                stricmp(ext,".jpg") &&
                stricmp(ext,".zip") &&
                stricmp(ext,".exe") &&
                stricmp(ext,".dat") &&
                stricmp(ext,".bmp") &&
                stricmp(ext,".rpl") &&
                stricmp(ext,".db") &&
                stricmp(ext,".wav") &&
                stricmp(ext,".ogg")))
            {
              if (!hMenu) hMenu=CreatePopupMenu();

              WDL_FastString tmp(path);
              tmp.Append("/");
              tmp.Append(fn);
              menufns->Add(strdup(tmp.Get()));

              MENUITEMINFO mii={sizeof(mii),MIIM_TYPE|MIIM_ID|MIIM_STATE,MFT_STRING,MFS_ENABLED,menufns->GetSize(),NULL,0,0,0,(char*)fn};
              InsertMenuItemFilter(hMenu,subpos + itempos++,TRUE,&mii);


            }
          }
        }
      }
      while (!ds.Next());
    }
    return hMenu;
  }

  bool DoEffectMenu(HWND hwndParent, int xpos, int ypos)
  {
    WDL_FastString curpath(g_root_path.Get());
    curpath.Append("/effects");
    WDL_PtrList<char> fns;
    HMENU hMenu=CreateMenuForPath(curpath.Get(),&fns);

    if (!hMenu)
    {
      hMenu=CreatePopupMenu();
      MENUITEMINFO mii={sizeof(mii),MIIM_TYPE|MIIM_STATE,MFT_STRING,MFS_GRAYED,0,NULL,0,0,0,(char*)"No JS effects found!"};
      InsertMenuItem(hMenu,0,TRUE,&mii);
    }
    int ret=TrackPopupMenu(hMenu,TPM_NONOTIFY|TPM_RETURNCMD,xpos,ypos,0,hwndParent,NULL);

    if (fns.Get(ret-1))
    {
      char *p=fns.Get(ret-1);
      if (strlen(p) > strlen(curpath.Get()) && !strnicmp(p,curpath.Get(),strlen(curpath.Get())))
        p+=strlen(curpath.Get())+1;
      Open(p);
    }
    else ret=0;
    fns.Empty(true);
    DestroyMenu(hMenu);
    return !!ret;
  }

  void CreateUI(HWND hwndDlg)
  {
    if (m_sxinst)
    {
      m_sxhwnd = sx_createUI(m_sxinst,g_hInst,hwndDlg,this);
      if (m_sxhwnd)
      {
        RECT r;
        GetWindowRect(GetDlgItem(hwndDlg,IDC_RECT),&r);
        ScreenToClient(hwndDlg,(LPPOINT)&r);
        ScreenToClient(hwndDlg,((LPPOINT)&r)+1);
        bool forcesize = false;
        if (!g_allow_edit)
        {
          WDL_WndSizer__rec *r1 = m_sxinst->resizer.get_item(IDC_EDITFX);
          WDL_WndSizer__rec *r2 = m_sxinst->resizer.get_item(IDC_BUTTON1);
          WDL_WndSizer__rec *r3 = m_sxinst->resizer.get_item(IDC_EFFECTNAME);
          if (r1 && r2 && r3)
          {
            const int adj = r1->real_orig.right - r2->real_orig.right;
            r2->orig.left = (r2->real_orig.left += adj);
            r2->orig.right = (r2->real_orig.right += adj);
            r3->orig.right = (r3->real_orig.right += adj);
          }
          ShowWindow(GetDlgItem(m_sxhwnd,IDC_EDITFX),SW_HIDE);
        }
        if (!m_sxinst->m_config_items.GetSize())
        {
          WDL_WndSizer__rec *r2 = m_sxinst->resizer.get_item(IDC_BUTTON1);
          WDL_WndSizer__rec *r3 = m_sxinst->resizer.get_item(IDC_EFFECTNAME);
          if (r2 && r3)
          {
            const int adj = r2->real_orig.right - r3->real_orig.right;
            r3->orig.right = (r3->real_orig.right += adj);
          }
          ShowWindow(GetDlgItem(m_sxhwnd,IDC_BUTTON1),SW_HIDE);
        }

        SetWindowPos(m_sxhwnd,NULL,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);
        if (forcesize) SendMessage(m_sxhwnd,WM_SIZE,0,0);

        ShowWindow(m_sxhwnd,SW_SHOWNA);
      }
    }
  }

  WDL_DLGRET CfgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    static int m_ignore_editmsg;
    static HWND resizeCap;
    static int resizeCap_xo,resizeCap_yo;
    switch (uMsg)
    {
      case WM_INITDIALOG:
        m_hwndcfg=hwndDlg;
        m_resizer.init(hwndDlg);
        m_resizer.init_item(IDC_FRAME,0,0,1,1);
        m_resizer.init_item(IDC_RECT,0,0,1,1);
        if (g_default_effect[0]!='!')
        {
          m_resizer.init_item(IDC_CUREFFECT,0,0,1,0);
          m_resizer.init_item(IDC_BUTTON1,1,0,1,0);
        }

        SetDlgItemText(hwndDlg,IDC_FRAME,"ReaJS by Cockos Incorporated - www.reaper.fm");

        if (g_default_effect[0]!='!')
          SetDlgItemText(hwndDlg,IDC_CUREFFECT,m_sxname.Get());

        if (m_last_w>0 && m_last_h>0)
        {
          SetWindowPos(hwndDlg,NULL,0,0,m_last_w,m_last_h,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE);
        }

        CreateUI(hwndDlg);

        ShowWindow(hwndDlg,SW_SHOWNA);
      return 0;
      case WM_USER+1040:
        {
          bool isEnd=false;
          if (wParam & 0x80000000)
          {
            wParam&=~0x80000000;
            isEnd=true;
          }


          if (m_cb && wParam < (WPARAM)m_effect.numParams && m_sxinst)
          {

            double minv=0.0,maxv=1.0,v=sx_getParmVal(m_sxinst,wParam,&minv,&maxv,NULL);
            if (v<=minv) v=0.0;
            else if (v>=maxv) v=1.0;
            else v = ((v-minv)/(maxv-minv));

            if (!m_auto_states[wParam])
            {
              m_auto_states[wParam]=true;
              m_cb(&m_effect,audioMasterBeginEdit,wParam,0,NULL,0.0f);
            }

            m_cb(&m_effect,audioMasterAutomate,wParam,0,NULL,(float)v);
            if (isEnd)
            {
              m_auto_states[wParam]=false;
              m_cb(&m_effect,audioMasterEndEdit,wParam,0,NULL,0.0f);
            }
          }
        }

      return 0;
      case WM_SETCURSOR:
        {
          RECT r;
          POINT p;
          GetCursorPos(&p);
          GetClientRect(hwndDlg,&r);
          ScreenToClient(hwndDlg,&p);
          int dx=r.right- p.x;
          int dy=r.bottom - p.y;
          if (dx>=0 && dy>=0 && dx+dy < 16)
          {
            SetCursor(LoadCursor(NULL,IDC_SIZENWSE));
          }
          else SetCursor(LoadCursor(NULL,IDC_ARROW));
        }
      return 1;
      case WM_LBUTTONDOWN:
        {
          RECT r;
          GetClientRect(hwndDlg,&r);
          resizeCap_xo= GET_X_LPARAM(lParam);
          resizeCap_yo= GET_Y_LPARAM(lParam);
          int dx=r.right- resizeCap_xo;
          int dy=r.bottom - resizeCap_yo;
          if (dx>=0 && dy>=0 && dx+dy < 16)
          {
            SetCapture(resizeCap=hwndDlg);
          }
        }
      return 0;
      case WM_MOUSEMOVE:
        if (GetCapture() == hwndDlg && resizeCap==hwndDlg)
        {
          RECT r;
          GetClientRect(hwndDlg,&r);
          int newx = GET_X_LPARAM(lParam) - resizeCap_xo;
          int newy = GET_Y_LPARAM(lParam) - resizeCap_yo;
          if (newx||newy)
          {
            resizeCap_xo+=newx;
            resizeCap_yo+=newy;
            newx += r.right;
            newy += r.bottom;
            if (newx < 100) newx=100;
            if (newy < 100) newy=100;
            m_last_w=newx;
            m_last_h=newy;

            if (newx != r.right || newy != r.bottom)
              SetWindowPos(hwndDlg,NULL,0,0,newx,newy,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);

            cfgRect[0]=r.top;
            cfgRect[1]=r.left;
            cfgRect[2]=r.top+newy;
            cfgRect[3]=r.left+newx;

            int dx=newx-r.right, dy=newy-r.bottom;

            HWND parent=GetParent(hwndDlg);
            if (parent)
            {
              HWND gparent=(GetWindowLong(parent,GWL_STYLE)&WS_CHILD) ? GetParent(parent) : NULL;
              RECT r,r2;
              GetWindowRect(parent,&r);
              if (gparent) GetWindowRect(gparent,&r2);

              SetWindowPos(parent,NULL,0,0,r.right-r.left+dx,abs(r.bottom-r.top)+dy,SWP_NOMOVE|SWP_NOZORDER);
              if (gparent) SetWindowPos(gparent,NULL,0,0,r2.right-r2.left+dx,abs(r2.bottom-r2.top)+dy,SWP_NOMOVE|SWP_NOZORDER);
            }
          }
        }
      return 0;
      case WM_LBUTTONUP:
        ReleaseCapture();
        resizeCap=0;
      return 0;
      case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
          m_resizer.onResize();
          if (m_sxhwnd)
          {
            RECT r;
            GetWindowRect(GetDlgItem(hwndDlg,IDC_RECT),&r);
            ScreenToClient(hwndDlg,(LPPOINT)&r);
            ScreenToClient(hwndDlg,((LPPOINT)&r)+1);
            SetWindowPos(m_sxhwnd,NULL,r.left,r.top,r.right-r.left,r.bottom-r.top,SWP_NOZORDER|SWP_NOACTIVATE);
          }
        }
      return 0;
      case WM_COMMAND:
        switch (LOWORD(wParam))
        {
          case IDC_BUTTON1:
            if (g_default_effect[0]!='!')
            {
              RECT r;
              GetWindowRect(GetDlgItem(hwndDlg,LOWORD(wParam)),&r);
              if (DoEffectMenu(hwndDlg,r.left,r.bottom))
              {
                SetDlgItemText(hwndDlg,IDC_CUREFFECT,m_sxname.Get());
                // todo: notify other stuff?
              }
            }
          break;
        }
      return 0;
      case WM_USER+1000:
        CreateUI(hwndDlg);
        if (g_default_effect[0]!='!')
          SetDlgItemText(hwndDlg,IDC_CUREFFECT,m_sxname.Get());
      return 0;
      case WM_DESTROY:
        m_resizer.init(NULL);

        if (m_sxinst)
        {
          sx_deleteUI(m_sxinst);
          m_sxhwnd=0;
        }
        m_hwndcfg=0;
      return 0;
    }
    return 0;
  }

  static WDL_DLGRET dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    if (uMsg == WM_INITDIALOG) SetWindowLongPtr(hwndDlg,GWLP_USERDATA,lParam);
    VSTEffectClass *_this = (VSTEffectClass *)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
    if (_this) return _this->CfgProc(hwndDlg,uMsg,wParam,lParam);

#ifdef _WIN32
    if (uMsg >= 0x127 && uMsg <= 0x129)
    {
      SetWindowLongPtr(hwndDlg,DWLP_MSGRESULT,0);
      return 1;
    }
#endif
    return 0;

  }

  static VstIntPtr VSTCALLBACK staticDispatcher(AEffect *effect, VstInt32 opCode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
  {
    VSTEffectClass *_this = (VSTEffectClass *)effect->object;
    switch (opCode)
    {
      case effStopProcess:
      case effMainsChanged:
        if (_this && !value)
        {
          WDL_MutexLock m(&_this->m_mutex);
          _this->Reset();
        }
      return 0;
      case effSetBlockSize:

      return 0;

      case effGetInputProperties:
      case effGetOutputProperties:
        if (_this && ptr)
        {
          if (index<0) return 0;

          VstPinProperties *pp=(VstPinProperties*)ptr;
          if (opCode == effGetInputProperties)
          {
            //sx_getPinInfo
          }
          else
          {
          }
        //  pp->flags=0;
      //    pp->arrangementType=kSpeakerArrStereo;
    //      if (index==0||index==2)
  //          pp->flags|=kVstPinIsStereo;
//          return 1;
        }
      return 0;

      case effSetChunk:
        if (_this)
        {
          _this->SetChunk(ptr,value);
          return 1;
        }
      return 0;
      case effGetChunk:
        if (_this) return _this->GetChunk((void **)ptr);
      return 0;
      case effSetSampleRate:
        if (_this)
        {
          WDL_MutexLock m(&_this->m_mutex);
          _this->m_samplerate=opt;
          _this->onParmChange();
        }
      return 0;
      case effGetPlugCategory:
        return g_vst_category;

      case effGetEffectName:
        if (ptr) lstrcpyn((char*)ptr,g_effect_name.Get(),32);
      return 0;

      case effGetProductString:
        if (ptr) lstrcpyn((char*)ptr,g_effect_name.Get(),32);
      return 0;
      case effGetVendorString:
        if (ptr) lstrcpyn((char*)ptr,VENDOR,32);
      return 0;
      case effGetVstVersion: return 2400;
      case effEditOpen:
        if (_this)
        {
          if (_this->m_hwndcfg) DestroyWindow(_this->m_hwndcfg);

          return !!CreateDialogParam(g_hInst,g_default_effect[0]=='!' ? MAKEINTRESOURCE(IDD_VST_CFG2) : MAKEINTRESOURCE(IDD_VST_CFG),(HWND)ptr,dlgProc,(LPARAM)_this);
        }
      return 0;
      case effEditClose:
        if (_this && _this->m_hwndcfg) DestroyWindow(_this->m_hwndcfg);
      return 0;
      case effClose:
        delete _this;
      return 0;
      case effOpen:
      return 0;

      case effCanBeAutomated:
        return index>=0&&index<_this->m_effect.numParams;
      case effGetParamName:
        if (ptr)
        {
          if (index>=0 && index<g_num_params)
          {
            char buf[512];
            sprintf( buf,"Parm%d",index+1);
            lstrcpyn((char *)ptr,buf,9);
          }
          else *(char *)ptr = 0;
        }
      return 0;

      case effGetParamLabel:
        if (ptr)
        {
          if (index>=0 && index<g_num_params)
          {
            lstrcpyn((char *)ptr,"",9);
          }
          else *(char *)ptr = 0;
        }
      return 0;
      case effCanDo:
        if (ptr && !strcmp((char *)ptr,"hasCockosExtensions")) return 0xbeef0000;
        if (ptr && !strcmp((char *)ptr,"hasCockosViewAsConfig")) return 0xbeef0000;
        if (ptr)
        {
          if (
              ((g_midi_flags&2) && (!stricmp((char*)ptr,"sendVstEvents") ||
              !stricmp((char*)ptr,"sendVstMidiEvent")))
              ||
              ((g_midi_flags&1) && (!stricmp((char*)ptr,"receiveVstEvents") ||
              !stricmp((char*)ptr,"receiveVstMidiEvent")))

              )
            return 1;
        }

      return 0;
      /*
      case effVendorSpecific:
        if (ptr && _this && index == 0xdeadbeef+1)
        {
          if (value>=0 && value<g_num_params)
          {
            double *buf=(double *)ptr;
            buf[0]=0.0;
            if (param_infos[value].minval == USE_DB) buf[1]=2.0;
            else buf[1]=param_infos[value].parm_maxval;
            return 0xbeef;
          }
        }
        else if (ptr && _this && index == effGetParamDisplay)
        {
          if (value>=0 && value<g_num_params) { format_parm(value,opt,(char *)ptr); return 0xbeef; }
          else *(char *)ptr = 0;
        }
      return 0;
      */

      case effGetParamDisplay:
        if (ptr&&_this)
        {
          if (index>=0 && index<g_num_params)
          {
            //format_parm(index,_this->m_parms[index],(char *)ptr);
            *(char*)ptr=0;
          }
          else *(char *)ptr = 0;
        }
      return 0;
      case effEditGetRect:
        if (_this)// && _this->m_hwndcfg)
        {
          RECT r;
          if (_this->m_hwndcfg) GetClientRect(_this->m_hwndcfg,&r);
          else {r.left=r.top=0; r.right=400; r.bottom=300; }
          _this->cfgRect[0]=r.top;
          _this->cfgRect[1]=r.left;
          _this->cfgRect[2]=r.bottom;
          _this->cfgRect[3]=r.right;

          *(void **)ptr = _this->cfgRect;

          return 1;

        }
      return 0;
      case effIdentify:
        return CCONST ('N', 'v', 'E', 'f');
      case effGetProgramName:
      case effGetProgramNameIndexed:
        if (ptr) *(char *)ptr=0;
      return 0;
      case effProcessEvents:
        if (ptr && _this && (g_midi_flags&1))
          _this->ProcessEvents((VstEvents*)ptr);
      return 0;
    }
    return 0;
  }
  short cfgRect[4];

  static void VSTCALLBACK staticProcessReplacing(AEffect *effect, SAMPLETYPE **inputs, SAMPLETYPE **outputs, VstInt32 sampleframes)
  {
    VSTEffectClass *_this = (VSTEffectClass *)effect->object;
    if (_this) _this->ProcessSamples(inputs,outputs,sampleframes);
  }



  WDL_TypedBuf<float> m_tmpbuf_add;
  static void VSTCALLBACK staticProcess(AEffect *effect, float **inputs, float **outputs, VstInt32 sampleframes)
  {
    VSTEffectClass *_this = (VSTEffectClass *)effect->object;
    if (_this)
    {
      WDL_TypedBuf<float *> outptrs;
      int no=effect->numOutputs;
      outptrs.Resize(no);
      float *tmpbuf=_this->m_tmpbuf_add.Resize(sampleframes*no,false);
      int x;
      for (x = 0; x < no; x ++)
      {
        outptrs.Get()[x]=tmpbuf;
        tmpbuf+=sampleframes;
      }
      staticProcessReplacing(effect,inputs,outptrs.Get(),sampleframes);
      for (x = 0; x < no; x ++)
      {
        int n=sampleframes;
        float *o=outputs[x];
        float *i=outptrs.Get()[x];
        while (n--)
        {
          *o++ += *i++;
        }
      }
    }

  }



  static void VSTCALLBACK staticSetParameter(AEffect *effect, VstInt32 index, float parameter)
  {
    VSTEffectClass *_this = (VSTEffectClass *)effect->object;
    if (!_this || index < 0 || index >= g_num_params) return;
    WDL_MutexLock m(&_this->m_mutex);

    if (!_this->m_sxinst) return;

    double minv=0.0,maxv=1.0;
    sx_getParmVal(_this->m_sxinst, index, &minv,&maxv,NULL);

    sx_setParmVal(_this->m_sxinst,index,parameter*(maxv-minv) + minv, 0);

    _this->onParmChange();

  }

  static float VSTCALLBACK staticGetParameter(AEffect *effect, VstInt32 index)
  {
    VSTEffectClass *_this = (VSTEffectClass *)effect->object;
    if (!_this || index < 0 || index >= g_num_params) return 0.0;
    WDL_MutexLock m(&_this->m_mutex);

    if (!_this->m_sxinst) return 0.0;

    double minv=0.0,maxv=1.0,v=sx_getParmVal(_this->m_sxinst,index,&minv,&maxv,NULL);
    if (v<=minv)return 0.0;
    if (v>=maxv) return 1.0;
    return (float) ((v-minv)/(maxv-minv));
  }


  bool m_auto_states[NUM_SLIDERS];
  audioMasterCallback m_cb;
  VstEvents *m_pelist;
  int m_pelist_rdpos;

  int m_last_w,m_last_h;
  WDL_WndSizer m_resizer;
  SX_Instance *m_sxinst;
  WDL_FastString m_sxname;

  HWND m_hwndcfg,m_sxhwnd;
  double m_samplerate;
  AEffect m_effect;
  WDL_Mutex m_mutex;
};

void Sliders_Init(HINSTANCE hInst, bool reg, int hslider_res_id=0); // slider-control.cpp
void Meters_Init(HINSTANCE hInst, bool reg); // meter-control.cpp
#ifdef _DEBUG
extern bool g_debug_langpack_has_loaded;
#endif

extern "C" {

#ifdef _WIN32
#if _MSC_VER < 1300
__declspec(dllexport) AEffect *main(audioMasterCallback hostcb)
#else
__declspec(dllexport) AEffect *VSTPluginMain(audioMasterCallback hostcb)
#endif
#else
  __attribute__ ((visibility ("default"))) AEffect *VSTPluginMain(audioMasterCallback hostcb)
#endif
{

  static int init_cnt;
  if (!init_cnt)
  {
    init_cnt=1;
    #ifndef _WIN32
      Sliders_Init(NULL,true);
      Meters_Init(NULL,true);
      SWELL_RegisterCustomControlCreator(curses_ControlCreator);
    #endif

    // could call WDL_LoadLangPack() here
    #ifdef _DEBUG
      g_debug_langpack_has_loaded = true;
    #endif

    get_eel_funcdesc = default_get_eel_funcdesc;
  }

  if (!g_effect_name.Get()[0])
  {
    if (!g_root_path.Get()[0])
    {
#ifdef _WIN32
      HKEY hk;
      if (RegOpenKey(HKEY_LOCAL_MACHINE,"Software\\REAPER",&hk)==ERROR_SUCCESS)
      {
        char buf[2048];
        buf[0]=0;
        long sz=sizeof(buf);
        RegQueryValue(hk,NULL,buf,&sz);
        RegCloseKey(hk);
        if (buf[0])
        {
          g_root_path.Set(buf);

          // check appdata/reaper/effects to see if it exists
          if (RegOpenKeyEx(HKEY_CURRENT_USER,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders",0,KEY_READ,&hk) == ERROR_SUCCESS)
          {
            DWORD b=sizeof(buf)-128;
            DWORD t=REG_SZ;
            if (RegQueryValueEx(hk,"AppData",0,&t,(unsigned char *)buf,&b) == ERROR_SUCCESS && t == REG_SZ)
            {
              if (buf[0])
              {
                char *p=buf+strlen(buf);
                strcpy(p,"\\REAPER\\reaper-install-rev.txt");
                HANDLE hFile = CreateFile(buf,0,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                  CloseHandle(hFile);
                  strcpy(p,"\\REAPER");
                  g_root_path.Set(buf);
                }
              }
            }
            RegCloseKey(hk);
          }
        }
      }
#else
      {
        const char *home = getenv("HOME");
        if (home && *home)
        {
          g_root_path.Set(home);
          g_root_path.Append("/Library/Application Support/REAPER");
          WDL_FastString s(g_root_path.Get());
          s.Append("/reaper-install-rev.txt");
          FILE *fp=fopen(s.Get(),"r");
          if (fp) fclose(fp);
          else g_root_path.Set("");
        }
      }
#endif

      if (!g_root_path.Get()[0])
      {
        char buf[2048];
#ifdef _WIN32
        GetModuleFileName(g_hInst,buf,sizeof(buf));
        WDL_remove_filepart(buf);
        lstrcatn(buf,WDL_DIRCHAR_STR "JS", sizeof(buf));
#else
        Dl_info inf={0,};
        dladdr((void *)VSTPluginMain,&inf);
        if (inf.dli_fname)
        {
          lstrcpyn(buf,inf.dli_fname,sizeof(buf));
          WDL_remove_filepart(buf); // remove /ReaJS
          WDL_remove_filepart(buf); // remove /MacOS
          g_root_path.Set(buf);
        }
#endif
      }
    }

    g_effect_name.Set(EFFECT_NAME);
    if (g_hInst)
    {
      char inifile[2048];
#ifdef _WIN32
      GetModuleFileName(g_hInst,inifile,sizeof(inifile));
      char *p=inifile;
      while (*p) p++;
      while (p>= inifile && *p != '.') p--;
      if (p>=inifile)
      {
        strcpy(++p,"ini");
#else
      Dl_info inf={0,};
      dladdr((void *)VSTPluginMain,&inf);
      if (inf.dli_fname)
      {
        lstrcpyn(inifile,inf.dli_fname,sizeof(inifile));
        WDL_remove_filepart(inifile); // remove /ReaJS
        WDL_remove_filepart(inifile); // remove /MaCOS
        lstrcatn(inifile,"/reajs.ini",sizeof(inifile));
        char *p;
#endif

        char buf[1024];
        buf[0]=0; GetPrivateProfileString("ReaJS","appendname","",buf,sizeof(buf),inifile);
        if (buf[0]) g_effect_name.Append(buf);

        g_num_inputs = GetPrivateProfileInt("ReaJS","inputs",2,inifile);
        if (g_num_inputs<0)g_num_inputs=0; else if (g_num_inputs>64)g_num_inputs=64;
        g_num_outputs = GetPrivateProfileInt("ReaJS","outputs",2,inifile);
        if (g_num_outputs<0)g_num_outputs=0; else if (g_num_outputs>64)g_num_outputs=64;

        int a=GetPrivateProfileInt("ReaJS","vstid",g_unique_id,inifile);
        if (a) g_unique_id=a;


        GetPrivateProfileString("ReaJS","defaulteffect","",g_default_effect,sizeof(g_default_effect),inifile);

        g_midi_flags = GetPrivateProfileInt("ReaJS","midiflags",3,inifile);
        g_vst_category = GetPrivateProfileInt("ReaJS","vstcat",g_vst_category,inifile);

        g_allow_edit = !GetPrivateProfileInt("ReaJS","preventedit",0,inifile);
        g_num_params = GetPrivateProfileInt("ReaJS","numparms",NUM_SLIDERS,inifile);
        if (g_num_params<0) g_num_params=0;
        else if (g_num_params>NUM_SLIDERS) g_num_params=NUM_SLIDERS;

        // begin read rootpath
        buf[0]=0; GetPrivateProfileString("ReaJS","rootpath","",buf,sizeof(buf),inifile);
        if (buf[0])
        {
#ifdef _WIN32
          if (buf[1] == ':' || (buf[0]=='\\' && buf[1]=='\\')) g_root_path.Set(buf);
          else if (buf[0]=='\\' && inifile[1] ==':')
          {
            char tmp[3]={inifile[0],inifile[1],0};
            g_root_path.Set(tmp);
            g_root_path.Append(buf);
          }
#else
          if (buf[0] == '/') g_root_path.Set(buf);
#endif
          else // append buf to
          {
            g_root_path.Set(inifile);
            g_root_path.remove_filepart();
            g_root_path.Append(WDL_DIRCHAR_STR);
            g_root_path.Append(buf); // add relative path
          }
        }
        // end read rootpath


      }
    }
  }

  VSTEffectClass *obj = new VSTEffectClass(hostcb);
  if (obj)
    return &obj->m_effect;
  return 0;
}

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID res)
{
  if (fdwReason==DLL_PROCESS_ATTACH)
  {
    g_hInst=hDllInst;

    Sliders_Init(hDllInst, true, IDB_HSLIDER);
    Meters_Init(hDllInst, true);
  }
  if (fdwReason==DLL_PROCESS_DETACH)
  {
    Sliders_Init(hDllInst, false);
    Meters_Init(hDllInst, false);
  }
  return TRUE;
}

#endif

};

#ifndef _WIN32 // MAC resources
#include "../../WDL/swell/swell-dlggen.h"
#ifndef TBS_AUTOTICKS
#define TBS_AUTOTICKS 0
#endif
#include "../res.rc_mac_dlg"
#undef BEGIN
#undef END
#include "../../WDL/swell/swell-menugen.h"
#include "../res.rc_mac_menu"
#endif
